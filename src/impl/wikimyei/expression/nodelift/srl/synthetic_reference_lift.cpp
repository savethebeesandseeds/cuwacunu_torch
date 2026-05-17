// SPDX-License-Identifier: MIT
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace cuwacunu::wikimyei::expression::nodelift::srl {
namespace {

struct dsu_t {
  std::vector<int64_t> parent;
  std::vector<int64_t> rank;

  explicit dsu_t(int64_t n)
      : parent(static_cast<std::size_t>(n)),
        rank(static_cast<std::size_t>(n), 0) {
    std::iota(parent.begin(), parent.end(), 0);
  }

  int64_t find(int64_t x) {
    auto ux = static_cast<std::size_t>(x);
    if (parent[ux] != x) {
      parent[ux] = find(parent[ux]);
    }
    return parent[ux];
  }

  void unite(int64_t a, int64_t b) {
    int64_t ra = find(a);
    int64_t rb = find(b);
    if (ra == rb) {
      return;
    }
    auto ura = static_cast<std::size_t>(ra);
    auto urb = static_cast<std::size_t>(rb);
    if (rank[ura] < rank[urb]) {
      std::swap(ra, rb);
      std::swap(ura, urb);
    }
    parent[urb] = ra;
    if (rank[ura] == rank[urb]) {
      ++rank[ura];
    }
  }
};

[[nodiscard]] bool floating_scalar_type(c10::ScalarType dtype) {
  return dtype == torch::kFloat16 || dtype == torch::kBFloat16 ||
         dtype == torch::kFloat32 || dtype == torch::kFloat64;
}

void validate_coord_list(const std::array<int64_t, kPriceWidth> &coords,
                         int64_t width, const char *label) {
  std::unordered_set<int64_t> seen;
  for (const int64_t coord : coords) {
    TORCH_CHECK(coord >= 0 && coord < width, label,
                " coordinate out of range: ", coord);
    TORCH_CHECK(seen.insert(coord).second, label,
                " coordinates must be unique");
  }
}

void validate_coord_list(const std::array<int64_t, kActivityWidth> &coords,
                         int64_t width, const char *label) {
  std::unordered_set<int64_t> seen;
  for (const int64_t coord : coords) {
    TORCH_CHECK(coord >= 0 && coord < width, label,
                " coordinate out of range: ", coord);
    TORCH_CHECK(seen.insert(coord).second, label,
                " coordinates must be unique");
  }
}

void validate_options(const nodelift_options_t &options) {
  validate_coord_list(options.price_coords, kFeatureWidth, "price");
  validate_coord_list(options.activity_coords, kFeatureWidth, "activity");
  TORCH_CHECK(options.price_coords == kDefaultPriceCoords,
              "[SRL] v1 requires default price coordinate order {0,1,2,3}");
  TORCH_CHECK(
      options.activity_coords == kDefaultActivityCoords,
      "[SRL] v1 requires default activity coordinate order {4,5,6,7,8}");

  std::unordered_set<int64_t> used_coords;
  for (const int64_t coord : options.price_coords) {
    TORCH_CHECK(used_coords.insert(coord).second,
                "[SRL] duplicate price/activity coordinate: ", coord);
  }
  for (const int64_t coord : options.activity_coords) {
    TORCH_CHECK(used_coords.insert(coord).second,
                "[SRL] price/activity coordinates must be disjoint: ", coord);
  }

  TORCH_CHECK(options.gauge_policy == gauge_policy_t::uniform_per_component,
              "[SRL] v1 only supports uniform_per_component gauge policy");
  TORCH_CHECK(options.precision_policy == precision_policy_t::identity,
              "[SRL] v1 only supports identity precision policy");
  TORCH_CHECK(options.activity_mode == activity_mode_t::support_mean,
              "[SRL] v1 only supports support_mean activity mode");
  TORCH_CHECK(std::isfinite(options.eps) && options.eps > 0.0,
              "[SRL] eps must be finite and positive");
  TORCH_CHECK(std::isfinite(options.activity_max_exp_arg) &&
                  options.activity_max_exp_arg > 0.0,
              "[SRL] activity_max_exp_arg must be finite and positive");
}

torch::Tensor solve_synthetic_gauge(const torch::Tensor &A,
                                    const torch::Tensor &x, double eps,
                                    bool *failed) {
  const int64_t n = A.size(1);
  auto opts = torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
  auto kkt = torch::zeros({n + 1, n + 1}, opts);
  auto rhs = torch::zeros({n + 1, 1}, opts);
  const auto normal = A.transpose(0, 1).matmul(A);
  const auto atx = A.transpose(0, 1).matmul(x);
  const auto w = torch::full({n, 1}, 1.0 / static_cast<double>(n), opts);

  kkt.index_put_({torch::indexing::Slice(0, n), torch::indexing::Slice(0, n)},
                 normal);
  kkt.index_put_({torch::indexing::Slice(0, n), n}, w.squeeze(1));
  kkt.index_put_({n, torch::indexing::Slice(0, n)}, w.squeeze(1));
  rhs.index_put_({torch::indexing::Slice(0, n), 0}, atx.squeeze(1));

  auto extract_y = [n](const torch::Tensor &solution) {
    return solution.index({torch::indexing::Slice(0, n), 0}).contiguous();
  };

  try {
    auto solution = at::linalg_solve(kkt, rhs, /*left=*/true);
    auto y = extract_y(solution);
    if (torch::isfinite(y).all().item<bool>()) {
      *failed = false;
      return y;
    }
  } catch (const c10::Error &) {
  } catch (const std::exception &) {
  }

  try {
    auto solution = kkt.pinverse(eps).matmul(rhs);
    auto y = extract_y(solution);
    if (torch::isfinite(y).all().item<bool>()) {
      *failed = false;
      return y;
    }
  } catch (const c10::Error &) {
  } catch (const std::exception &) {
  }

  *failed = true;
  return torch::zeros({n}, opts);
}

template <typename TensorT>
TensorT move_float_tensor(TensorT tensor, const torch::Device &device,
                          c10::ScalarType dtype) {
  return tensor.to(device, dtype);
}

torch::Tensor move_index_or_mask_tensor(torch::Tensor tensor,
                                        const torch::Device &device) {
  return tensor.to(device);
}

} // namespace

int64_t graph_t::num_nodes() const {
  if (!node_ids.empty()) {
    return static_cast<int64_t>(node_ids.size());
  }
  if (base_index.defined() && quote_index.defined() && base_index.numel() > 0) {
    auto base_cpu = base_index.to(torch::kCPU).to(torch::kInt64).contiguous();
    auto quote_cpu = quote_index.to(torch::kCPU).to(torch::kInt64).contiguous();
    const auto *b = base_cpu.data_ptr<int64_t>();
    const auto *q = quote_cpu.data_ptr<int64_t>();
    int64_t max_node = -1;
    for (int64_t e = 0; e < base_cpu.numel(); ++e) {
      max_node = std::max(max_node, std::max(b[e], q[e]));
    }
    return max_node + 1;
  }
  return 0;
}

int64_t graph_t::num_edges() const {
  if (!edge_ids.empty()) {
    return static_cast<int64_t>(edge_ids.size());
  }
  return base_index.defined() ? base_index.numel() : 0;
}

void graph_t::validate() const {
  TORCH_CHECK(base_index.defined(), "[SRL graph] missing base_index");
  TORCH_CHECK(quote_index.defined(), "[SRL graph] missing quote_index");
  TORCH_CHECK(base_index.dim() == 1, "[SRL graph] base_index must be [L]");
  TORCH_CHECK(quote_index.dim() == 1, "[SRL graph] quote_index must be [L]");
  TORCH_CHECK(base_index.numel() == quote_index.numel(),
              "[SRL graph] base/quote index length mismatch");
  TORCH_CHECK(!node_ids.empty(), "[SRL graph] node_ids required for stable N");
  const int64_t L = base_index.numel();
  const int64_t N = num_nodes();
  TORCH_CHECK(N > 0, "[SRL graph] graph must contain at least one node");
  if (!edge_ids.empty()) {
    TORCH_CHECK(static_cast<int64_t>(edge_ids.size()) == L,
                "[SRL graph] edge_ids size must match L");
    if (!allow_duplicate_edge_ids) {
      std::unordered_set<std::string> seen;
      for (const auto &edge_id : edge_ids) {
        TORCH_CHECK(seen.insert(edge_id).second,
                    "[SRL graph] duplicate edge_id: ", edge_id);
      }
    }
  }
  TORCH_CHECK(static_cast<int64_t>(node_ids.size()) == N,
              "[SRL graph] node_ids size must match N");

  auto base_cpu = base_index.to(torch::kCPU).to(torch::kInt64).contiguous();
  auto quote_cpu = quote_index.to(torch::kCPU).to(torch::kInt64).contiguous();
  const auto *b = base_cpu.data_ptr<int64_t>();
  const auto *q = quote_cpu.data_ptr<int64_t>();
  for (int64_t e = 0; e < L; ++e) {
    TORCH_CHECK(b[e] >= 0 && b[e] < N, "[SRL graph] base index out of range");
    TORCH_CHECK(q[e] >= 0 && q[e] < N, "[SRL graph] quote index out of range");
    TORCH_CHECK(b[e] != q[e], "[SRL graph] self edges are not valid");
  }
  if (!graph_order_fingerprint.empty()) {
    TORCH_CHECK(graph_order_fingerprint == computed_graph_order_fingerprint(),
                "[SRL graph] graph_order_fingerprint mismatch");
  }
}

nodelift_output_t featurewise_node_lift(const graph_t &graph,
                                        const nodelift_input_t &input,
                                        const nodelift_options_t &options) {
  graph.validate();
  TORCH_CHECK(input.edge_features.defined(),
              "[SRL] edge_features must be defined");
  TORCH_CHECK(input.edge_mask.defined(), "[SRL] edge_mask must be defined");
  TORCH_CHECK(input.edge_features.dim() == 5,
              "[SRL] edge_features must be [B,L,C,Hx,9]");
  TORCH_CHECK(input.edge_mask.dim() == 4, "[SRL] edge_mask must be [B,L,C,Hx]");
  TORCH_CHECK(input.edge_features.size(4) == kFeatureWidth,
              "[SRL] v1 requires feature width 9");
  TORCH_CHECK(floating_scalar_type(input.edge_features.scalar_type()),
              "[SRL] edge_features must use a floating dtype");

  const int64_t Bg = input.edge_features.size(0);
  const int64_t L = input.edge_features.size(1);
  const int64_t C = input.edge_features.size(2);
  const int64_t Hx = input.edge_features.size(3);
  const int64_t N = graph.num_nodes();
  TORCH_CHECK(graph.num_edges() == L,
              "[SRL] graph edge count must match edge_features L");
  TORCH_CHECK(input.edge_mask.sizes() == torch::IntArrayRef({Bg, L, C, Hx}),
              "[SRL] edge_mask shape mismatch");
  validate_options(options);

  const torch::Device target_device =
      options.output_device.value_or(input.edge_features.device());
  c10::ScalarType target_dtype = options.output_dtype.value_or(torch::kFloat32);
  TORCH_CHECK(floating_scalar_type(target_dtype),
              "[SRL] output dtype must be floating");

  auto work_opts =
      torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
  auto bool_opts =
      torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU);
  auto int_opts =
      torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU);

  auto features_cpu = input.edge_features.detach()
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  auto edge_mask_cpu =
      input.edge_mask.detach().to(torch::kCPU).to(torch::kBool).contiguous();
  auto finite_cpu = torch::isfinite(features_cpu);
  torch::Tensor coord_mask_cpu;
  if (input.edge_coord_mask.has_value() && input.edge_coord_mask->defined()) {
    TORCH_CHECK(input.edge_coord_mask->sizes() == input.edge_features.sizes(),
                "[SRL] edge_coord_mask shape mismatch");
    coord_mask_cpu = input.edge_coord_mask->detach()
                         .to(torch::kCPU)
                         .to(torch::kBool)
                         .contiguous();
    coord_mask_cpu = coord_mask_cpu.logical_and(edge_mask_cpu.unsqueeze(-1))
                         .logical_and(finite_cpu);
  } else {
    coord_mask_cpu = edge_mask_cpu.unsqueeze(-1).logical_and(finite_cpu);
  }

  auto base_cpu =
      graph.base_index.to(torch::kCPU).to(torch::kInt64).contiguous();
  auto quote_cpu =
      graph.quote_index.to(torch::kCPU).to(torch::kInt64).contiguous();
  const auto *base = base_cpu.data_ptr<int64_t>();
  const auto *quote = quote_cpu.data_ptr<int64_t>();

  auto node_features_cpu =
      torch::zeros({Bg, C, Hx, N, kFeatureWidth}, work_opts);
  auto node_mask_cpu = torch::zeros({Bg, C, Hx, N, kFeatureWidth}, bool_opts);
  auto node_mask_any_cpu = torch::zeros({Bg, C, Hx, N}, bool_opts);
  auto node_mask_all_cpu = torch::zeros({Bg, C, Hx, N}, bool_opts);
  auto price_residual_cpu =
      torch::zeros({Bg, C, Hx, L, kPriceWidth}, work_opts);
  auto price_residual_mask_cpu =
      torch::zeros({Bg, C, Hx, L, kPriceWidth}, bool_opts);
  auto activity_total_cpu =
      torch::zeros({Bg, C, Hx, N, kActivityWidth}, work_opts);
  auto activity_support_cpu =
      torch::zeros({Bg, C, Hx, N, kActivityWidth}, int_opts);
  auto activity_coverage_cpu =
      torch::zeros({Bg, C, Hx, N, kActivityWidth}, work_opts);

  auto valid_edge_count_cpu =
      torch::zeros({Bg, C, Hx, kFeatureWidth}, int_opts);
  auto component_count_cpu = torch::zeros({Bg, C, Hx, kPriceWidth}, int_opts);
  auto recoverable_component_count_cpu =
      torch::zeros({Bg, C, Hx, kPriceWidth}, int_opts);
  auto cycle_dimension_cpu = torch::zeros({Bg, C, Hx, kPriceWidth}, int_opts);
  auto residual_energy_cpu = torch::zeros({Bg, C, Hx, kPriceWidth}, work_opts);
  auto failed_solve_count_cpu =
      torch::zeros({Bg, C, Hx, kPriceWidth}, int_opts);

  auto features = features_cpu.accessor<double, 5>();
  auto coord_mask = coord_mask_cpu.accessor<bool, 5>();
  auto node_features = node_features_cpu.accessor<double, 5>();
  auto node_mask = node_mask_cpu.accessor<bool, 5>();
  auto price_residual = price_residual_cpu.accessor<double, 5>();
  auto price_residual_mask = price_residual_mask_cpu.accessor<bool, 5>();
  auto activity_total = activity_total_cpu.accessor<double, 5>();
  auto activity_support = activity_support_cpu.accessor<int64_t, 5>();
  auto activity_coverage = activity_coverage_cpu.accessor<double, 5>();
  auto valid_edge_count = valid_edge_count_cpu.accessor<int64_t, 4>();
  auto component_count = component_count_cpu.accessor<int64_t, 4>();
  auto recoverable_component_count =
      recoverable_component_count_cpu.accessor<int64_t, 4>();
  auto cycle_dimension = cycle_dimension_cpu.accessor<int64_t, 4>();
  auto residual_energy = residual_energy_cpu.accessor<double, 4>();
  auto failed_solve_count = failed_solve_count_cpu.accessor<int64_t, 4>();

  std::array<std::vector<int64_t>, kActivityWidth> staged_support;
  for (auto &support : staged_support) {
    support.assign(static_cast<std::size_t>(N), 0);
  }
  for (int64_t e = 0; e < L; ++e) {
    ++staged_support[0][static_cast<std::size_t>(base[e])];
    ++staged_support[1][static_cast<std::size_t>(quote[e])];
    ++staged_support[2][static_cast<std::size_t>(base[e])];
    ++staged_support[2][static_cast<std::size_t>(quote[e])];
    ++staged_support[3][static_cast<std::size_t>(base[e])];
    ++staged_support[4][static_cast<std::size_t>(quote[e])];
  }

  for (int64_t b = 0; b < Bg; ++b) {
    for (int64_t c = 0; c < C; ++c) {
      for (int64_t h = 0; h < Hx; ++h) {
        for (int64_t p = 0; p < kPriceWidth; ++p) {
          const int64_t d = options.price_coords[static_cast<std::size_t>(p)];
          std::vector<int64_t> valid_edges;
          valid_edges.reserve(static_cast<std::size_t>(L));
          for (int64_t e = 0; e < L; ++e) {
            if (coord_mask[b][e][c][h][d]) {
              valid_edges.push_back(e);
            }
          }
          valid_edge_count[b][c][h][d] =
              static_cast<int64_t>(valid_edges.size());
          if (valid_edges.empty()) {
            continue;
          }

          dsu_t dsu(N);
          std::vector<bool> touched(static_cast<std::size_t>(N), false);
          for (const int64_t e : valid_edges) {
            dsu.unite(base[e], quote[e]);
            touched[static_cast<std::size_t>(base[e])] = true;
            touched[static_cast<std::size_t>(quote[e])] = true;
          }

          std::unordered_map<int64_t, std::vector<int64_t>> root_nodes;
          for (int64_t n = 0; n < N; ++n) {
            if (touched[static_cast<std::size_t>(n)]) {
              root_nodes[dsu.find(n)].push_back(n);
            }
          }
          component_count[b][c][h][p] = static_cast<int64_t>(root_nodes.size());

          for (const auto &[root, nodes] : root_nodes) {
            std::vector<int64_t> comp_edges;
            for (const int64_t e : valid_edges) {
              if (dsu.find(base[e]) == root && dsu.find(quote[e]) == root) {
                comp_edges.push_back(e);
              }
            }
            const int64_t n_comp = static_cast<int64_t>(nodes.size());
            const int64_t m_comp = static_cast<int64_t>(comp_edges.size());
            if (n_comp < 2 || m_comp < n_comp - 1) {
              continue;
            }
            ++recoverable_component_count[b][c][h][p];
            cycle_dimension[b][c][h][p] +=
                std::max<int64_t>(0, m_comp - n_comp + 1);

            std::unordered_map<int64_t, int64_t> local;
            for (int64_t idx = 0; idx < n_comp; ++idx) {
              local[nodes[static_cast<std::size_t>(idx)]] = idx;
            }
            auto A = torch::zeros({m_comp, n_comp}, work_opts);
            auto x = torch::zeros({m_comp, 1}, work_opts);
            for (int64_t r = 0; r < m_comp; ++r) {
              const int64_t e = comp_edges[static_cast<std::size_t>(r)];
              A.index_put_({r, local[base[e]]}, 1.0);
              A.index_put_({r, local[quote[e]]}, -1.0);
              x.index_put_({r, 0}, features[b][e][c][h][d]);
            }

            bool failed = false;
            auto y = solve_synthetic_gauge(A, x, options.eps, &failed);
            if (failed) {
              ++failed_solve_count[b][c][h][p];
              continue;
            }

            auto y_acc = y.accessor<double, 1>();
            for (int64_t idx = 0; idx < n_comp; ++idx) {
              const int64_t node = nodes[static_cast<std::size_t>(idx)];
              node_features[b][c][h][node][d] = y_acc[idx];
              node_mask[b][c][h][node][d] = true;
            }

            for (int64_t r = 0; r < m_comp; ++r) {
              const int64_t e = comp_edges[static_cast<std::size_t>(r)];
              const double prediction =
                  y_acc[local[base[e]]] - y_acc[local[quote[e]]];
              const double residual = features[b][e][c][h][d] - prediction;
              price_residual[b][c][h][e][p] = residual;
              price_residual_mask[b][c][h][e][p] = true;
              residual_energy[b][c][h][p] += residual * residual;
            }
          }
        }

        for (int64_t a = 0; a < kActivityWidth; ++a) {
          const int64_t d =
              options.activity_coords[static_cast<std::size_t>(a)];
          std::vector<int64_t> support(static_cast<std::size_t>(N), 0);
          std::vector<double> total(static_cast<std::size_t>(N), 0.0);
          int64_t valid_count = 0;
          for (int64_t e = 0; e < L; ++e) {
            if (!coord_mask[b][e][c][h][d]) {
              continue;
            }
            ++valid_count;
            const double clipped =
                std::min(features[b][e][c][h][d], options.activity_max_exp_arg);
            const double raw_activity = std::max(0.0, std::expm1(clipped));
            auto add = [&](int64_t node) {
              auto idx = static_cast<std::size_t>(node);
              ++support[idx];
              total[idx] += raw_activity;
            };
            if (a == 0 || a == 3) {
              add(base[e]);
            } else if (a == 1 || a == 4) {
              add(quote[e]);
            } else {
              add(base[e]);
              add(quote[e]);
            }
          }
          valid_edge_count[b][c][h][d] = valid_count;
          for (int64_t n = 0; n < N; ++n) {
            const auto idx = static_cast<std::size_t>(n);
            const int64_t kappa = support[idx];
            activity_support[b][c][h][n][a] = kappa;
            activity_total[b][c][h][n][a] = total[idx];
            const double mean =
                total[idx] / static_cast<double>(std::max<int64_t>(1, kappa));
            activity_coverage[b][c][h][n][a] =
                static_cast<double>(kappa) /
                static_cast<double>(std::max<int64_t>(
                    1, staged_support[static_cast<std::size_t>(a)][idx]));
            if (kappa > 0) {
              node_features[b][c][h][n][d] = std::log1p(mean);
              node_mask[b][c][h][n][d] = true;
            }
          }
        }
      }
    }
  }

  auto node_mask_any = node_mask_cpu.any(/*dim=*/4);
  auto node_mask_all = node_mask_cpu.all(/*dim=*/4);

  nodelift_output_t out{};
  out.node_features =
      move_float_tensor(node_features_cpu, target_device, target_dtype);
  out.node_mask = move_index_or_mask_tensor(node_mask_cpu, target_device);
  if (options.return_coarse_masks) {
    out.node_mask_any = move_index_or_mask_tensor(node_mask_any, target_device);
    out.node_mask_all = move_index_or_mask_tensor(node_mask_all, target_device);
  }
  out.price_residual =
      move_float_tensor(price_residual_cpu, target_device, target_dtype);
  out.price_residual_mask =
      move_index_or_mask_tensor(price_residual_mask_cpu, target_device);
  if (options.return_activity_total) {
    out.activity_total =
        move_float_tensor(activity_total_cpu, target_device, target_dtype);
  }
  if (options.return_activity_support) {
    out.activity_support =
        move_index_or_mask_tensor(activity_support_cpu, target_device);
  }
  if (options.return_activity_coverage) {
    out.activity_coverage =
        move_float_tensor(activity_coverage_cpu, target_device, target_dtype);
  }

  out.diagnostics.valid_edge_count =
      move_index_or_mask_tensor(valid_edge_count_cpu, target_device);
  out.diagnostics.component_count =
      move_index_or_mask_tensor(component_count_cpu, target_device);
  out.diagnostics.recoverable_component_count =
      move_index_or_mask_tensor(recoverable_component_count_cpu, target_device);
  out.diagnostics.cycle_dimension =
      move_index_or_mask_tensor(cycle_dimension_cpu, target_device);
  out.diagnostics.residual_energy =
      move_float_tensor(residual_energy_cpu, target_device, target_dtype);
  out.diagnostics.failed_solve_count =
      move_index_or_mask_tensor(failed_solve_count_cpu, target_device);
  return out;
}

} // namespace cuwacunu::wikimyei::expression::nodelift::srl
