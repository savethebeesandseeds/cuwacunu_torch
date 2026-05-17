// SPDX-License-Identifier: MIT
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace srl = cuwacunu::wikimyei::expression::nodelift::srl;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[SRL test] " << message << "\n";
    std::exit(1);
  }
}

double scalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kFloat64).item<double>();
}

bool bscalar(const torch::Tensor &t) {
  return t.detach().to(torch::kCPU).to(torch::kBool).item<bool>();
}

void close(double actual, double expected, double tol, const std::string &msg) {
  if (std::fabs(actual - expected) > tol) {
    std::cerr << "[SRL test] " << msg << " actual=" << actual
              << " expected=" << expected << "\n";
    std::exit(1);
  }
}

template <typename Fn> void expect_throw(Fn &&fn, const std::string &msg) {
  try {
    fn();
  } catch (const c10::Error &) {
    return;
  } catch (const std::exception &) {
    return;
  }
  std::cerr << "[SRL test] expected throw: " << msg << "\n";
  std::exit(1);
}

srl::graph_t make_graph(std::initializer_list<int64_t> base,
                        std::initializer_list<int64_t> quote, int64_t nodes) {
  srl::graph_t graph{};
  for (int64_t i = 0; i < nodes; ++i) {
    graph.node_ids.push_back("n" + std::to_string(i));
  }
  const int64_t L = static_cast<int64_t>(base.size());
  for (int64_t e = 0; e < L; ++e) {
    graph.edge_ids.push_back("e" + std::to_string(e));
  }
  graph.base_index = torch::tensor(std::vector<int64_t>(base),
                                   torch::TensorOptions().dtype(torch::kInt64));
  graph.quote_index = torch::tensor(
      std::vector<int64_t>(quote), torch::TensorOptions().dtype(torch::kInt64));
  return graph;
}

srl::nodelift_input_t make_input(int64_t L, int64_t C = 1, int64_t Hx = 1) {
  srl::nodelift_input_t input{};
  input.edge_features = torch::zeros(
      {1, L, C, Hx, 9}, torch::TensorOptions().dtype(torch::kFloat32));
  input.edge_mask =
      torch::zeros({1, L, C, Hx}, torch::TensorOptions().dtype(torch::kBool));
  return input;
}

void set_price(srl::nodelift_input_t &input, int64_t edge, double value,
               int64_t coord = 0) {
  input.edge_features.index_put_({0, edge, 0, 0, coord}, value);
}

void test_single_edge() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  input.edge_features.index_put_({0, 0, 0, 0, 0}, 4.0);
  input.edge_mask.index_put_({0, 0, 0, 0}, true);
  auto out = srl::featurewise_node_lift(graph, input);
  close(scalar(out.node_features.index({0, 0, 0, 0, 0})), 2.0, 1e-5,
        "single edge base coordinate");
  close(scalar(out.node_features.index({0, 0, 0, 1, 0})), -2.0, 1e-5,
        "single edge quote coordinate");
  close(scalar(out.price_residual.index({0, 0, 0, 0, 0})), 0.0, 1e-5,
        "single edge residual");
  check(bscalar(out.node_mask.index({0, 0, 0, 0, 0})), "single edge base mask");
  check(bscalar(out.node_mask.index({0, 0, 0, 1, 0})),
        "single edge quote mask");
}

void test_tree_exact_gauge_and_translation() {
  auto graph = make_graph({0, 1}, {1, 2}, 3);
  auto input = make_input(2);
  set_price(input, 0, 2.0);
  set_price(input, 1, 2.0);
  input.edge_mask.fill_(true);
  auto out = srl::featurewise_node_lift(graph, input);
  close(scalar(out.node_features.index({0, 0, 0, 0, 0})), 2.0, 1e-5,
        "tree node 0");
  close(scalar(out.node_features.index({0, 0, 0, 1, 0})), 0.0, 1e-5,
        "tree node 1");
  close(scalar(out.node_features.index({0, 0, 0, 2, 0})), -2.0, 1e-5,
        "tree node 2");
  close(scalar(out.node_features.index({0, 0, 0, torch::indexing::Slice(), 0})
                   .sum()),
        0.0, 1e-5, "uniform gauge mean");
  close(scalar(out.price_residual.abs().sum()), 0.0, 1e-5,
        "tree residual zero");

  auto shifted = make_input(2);
  set_price(shifted, 0, 2.0);
  set_price(shifted, 1, 2.0);
  shifted.edge_mask.fill_(true);
  auto shifted_out = srl::featurewise_node_lift(graph, shifted);
  close(scalar((out.node_features - shifted_out.node_features).abs().max()),
        0.0, 1e-5, "translation-invariant edge differences");
}

void test_triangle_consistent_and_inconsistent() {
  auto graph = make_graph({0, 1, 0}, {1, 2, 2}, 3);
  auto consistent = make_input(3);
  set_price(consistent, 0, 1.0);
  set_price(consistent, 1, 1.0);
  set_price(consistent, 2, 2.0);
  consistent.edge_mask.fill_(true);
  auto consistent_out = srl::featurewise_node_lift(graph, consistent);
  close(scalar(consistent_out.diagnostics.residual_energy.index({0, 0, 0, 0})),
        0.0, 1e-5, "consistent triangle residual");

  auto inconsistent = make_input(3);
  set_price(inconsistent, 0, 1.0);
  set_price(inconsistent, 1, 1.0);
  set_price(inconsistent, 2, 3.0);
  inconsistent.edge_mask.fill_(true);
  auto inconsistent_out = srl::featurewise_node_lift(graph, inconsistent);
  check(scalar(inconsistent_out.diagnostics.residual_energy.index(
            {0, 0, 0, 0})) > 0.1,
        "inconsistent triangle residual energy should be positive");
}

void test_reverse_edges() {
  auto graph = make_graph({0, 1}, {1, 0}, 2);
  auto consistent = make_input(2);
  set_price(consistent, 0, 4.0);
  set_price(consistent, 1, -4.0);
  consistent.edge_mask.fill_(true);
  auto consistent_out = srl::featurewise_node_lift(graph, consistent);
  close(scalar(consistent_out.diagnostics.residual_energy.index({0, 0, 0, 0})),
        0.0, 1e-5, "reverse-edge consistent residual");

  auto disagreement = make_input(2);
  set_price(disagreement, 0, 4.0);
  set_price(disagreement, 1, -3.0);
  disagreement.edge_mask.fill_(true);
  auto disagreement_out = srl::featurewise_node_lift(graph, disagreement);
  check(scalar(disagreement_out.diagnostics.residual_energy.index(
            {0, 0, 0, 0})) > 0.1,
        "reverse-edge disagreement residual");
}

void test_disconnected_and_isolated() {
  auto graph = make_graph({0, 2}, {1, 3}, 5);
  auto input = make_input(2);
  input.edge_features.index_put_({0, 0, 0, 0, 0}, 2.0);
  input.edge_features.index_put_({0, 1, 0, 0, 0}, 6.0);
  input.edge_mask.fill_(true);
  auto out = srl::featurewise_node_lift(graph, input);
  close(scalar(out.node_features.index({0, 0, 0, 0, 0})), 1.0, 1e-5,
        "component one gauge");
  close(scalar(out.node_features.index({0, 0, 0, 2, 0})), 3.0, 1e-5,
        "component two gauge");
  check(!bscalar(out.node_mask.index({0, 0, 0, 4, 0})),
        "isolated node price mask should be false");
  close(scalar(out.diagnostics.component_count.index({0, 0, 0, 0})), 2.0, 1e-5,
        "component count");
}

void test_coordinatewise_mask() {
  auto graph = make_graph({0, 1}, {1, 2}, 3);
  auto input = make_input(2);
  input.edge_features.index_put_({0, 0, 0, 0, 0}, 1.0);
  input.edge_features.index_put_({0, 1, 0, 0, 0}, 1.0);
  input.edge_features.index_put_({0, 0, 0, 0, 1}, 1.0);
  input.edge_features.index_put_({0, 1, 0, 0, 1}, 1.0);
  input.edge_mask.fill_(true);
  auto coord_mask = torch::ones({1, 2, 1, 1, 9}, torch::kBool);
  coord_mask.index_put_({0, 1, 0, 0, 1}, false);
  input.edge_coord_mask = coord_mask;
  auto out = srl::featurewise_node_lift(graph, input);
  check(bscalar(out.node_mask.index({0, 0, 0, 2, 0})),
        "coord 0 should recover node 2");
  check(!bscalar(out.node_mask.index({0, 0, 0, 2, 1})),
        "coord 1 should not recover node 2");
}

void test_edge_mask_overrides_coord_mask() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  set_price(input, 0, 4.0);
  input.edge_mask.fill_(false);
  input.edge_coord_mask = torch::ones({1, 1, 1, 1, 9}, torch::kBool);
  auto out = srl::featurewise_node_lift(graph, input);
  check(!bscalar(out.node_mask.index({0, 0, 0, 0, 0})),
        "edge_mask false should override coord_mask true");
  check(!bscalar(out.price_residual_mask.index({0, 0, 0, 0, 0})),
        "edge_mask false should mask residual");
  close(scalar(out.diagnostics.valid_edge_count.index({0, 0, 0, 0})), 0.0, 1e-5,
        "edge_mask override valid count");
  close(scalar(out.node_features.abs().sum()), 0.0, 1e-8,
        "edge_mask override zero-filled node features");
}

void test_nonfinite_values_are_masked() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  input.edge_mask.fill_(true);
  input.edge_coord_mask = torch::ones({1, 1, 1, 1, 9}, torch::kBool);
  input.edge_features.index_put_({0, 0, 0, 0, 0},
                                 std::numeric_limits<double>::quiet_NaN());
  input.edge_features.index_put_({0, 0, 0, 0, 4},
                                 std::numeric_limits<double>::infinity());
  auto out = srl::featurewise_node_lift(graph, input);
  check(!bscalar(out.node_mask.index({0, 0, 0, 0, 0})),
        "NaN price coordinate should be masked");
  check(!bscalar(out.node_mask.index({0, 0, 0, 0, 4})),
        "Inf activity coordinate should be masked");
  close(scalar(out.diagnostics.valid_edge_count.index({0, 0, 0, 0})), 0.0, 1e-5,
        "NaN price valid count");
  close(scalar(out.diagnostics.valid_edge_count.index({0, 0, 0, 4})), 0.0, 1e-5,
        "Inf activity valid count");
  check(torch::isfinite(out.node_features).all().item<bool>(),
        "nonfinite input should not leak to node features");
  check(torch::isfinite(out.price_residual).all().item<bool>(),
        "nonfinite input should not leak to residuals");
}

void test_activity() {
  auto graph = make_graph({0, 0, 3}, {1, 2, 4}, 5);
  auto input = make_input(3);
  input.edge_mask.fill_(true);
  input.edge_features.index_put_({0, 0, 0, 0, 4}, std::log1p(10.0));
  input.edge_features.index_put_({0, 1, 0, 0, 4}, std::log1p(10.0));
  input.edge_features.index_put_({0, 2, 0, 0, 4}, std::log1p(10.0));
  input.edge_features.index_put_({0, 0, 0, 0, 5}, std::log1p(7.0));
  input.edge_features.index_put_({0, 0, 0, 0, 6}, std::log1p(3.0));
  auto coord_mask = torch::ones({1, 3, 1, 1, 9}, torch::kBool);
  coord_mask.index_put_({0, 1, 0, 0, 6}, false);
  coord_mask.index_put_({0, 2, 0, 0, 6}, false);
  input.edge_coord_mask = coord_mask;
  auto out = srl::featurewise_node_lift(graph, input);
  close(scalar(out.node_features.index({0, 0, 0, 0, 4})), std::log1p(10.0),
        1e-5, "support-normalized base volume");
  close(scalar(out.node_features.index({0, 0, 0, 3, 4})), std::log1p(10.0),
        1e-5, "single-edge support-normalized base volume");
  close(scalar(out.activity_total->index({0, 0, 0, 0, 0})), 20.0, 1e-4,
        "activity total");
  close(scalar(out.activity_total->index({0, 0, 0, 3, 0})), 10.0, 1e-4,
        "single-edge activity total");
  close(scalar(out.activity_support->index({0, 0, 0, 0, 0})), 2.0, 1e-5,
        "activity support");
  close(scalar(out.activity_support->index({0, 0, 0, 3, 0})), 1.0, 1e-5,
        "single-edge activity support");
  close(scalar(out.activity_coverage->index({0, 0, 0, 0, 0})), 1.0, 1e-5,
        "activity coverage");
  close(scalar(out.activity_coverage->index({0, 0, 0, 0, 2})), 0.5, 1e-5,
        "trade activity coverage");
  close(scalar(out.node_features.index({0, 0, 0, 1, 5})), std::log1p(7.0), 1e-5,
        "quote endpoint activity");
  close(scalar(out.node_features.index({0, 0, 0, 0, 6})), std::log1p(3.0), 1e-5,
        "trade base endpoint activity");
  close(scalar(out.node_features.index({0, 0, 0, 1, 6})), std::log1p(3.0), 1e-5,
        "trade quote endpoint activity");
}

void test_all_invalid_and_no_nan() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  auto out = srl::featurewise_node_lift(graph, input);
  check(!bscalar(out.node_mask.index({0, 0, 0, 0, 0})),
        "all-invalid node mask");
  close(scalar(out.node_features.abs().sum()), 0.0, 1e-8,
        "all-invalid zero-filled node features");
  check(torch::isfinite(out.node_features).all().item<bool>(),
        "node features finite");
  check(torch::isfinite(out.price_residual).all().item<bool>(),
        "price residual finite");
}

void test_edge_order_permutation() {
  auto graph_a = make_graph({0, 1}, {1, 2}, 3);
  auto input_a = make_input(2);
  input_a.edge_features.index_put_({0, 0, 0, 0, 0}, 1.0);
  input_a.edge_features.index_put_({0, 1, 0, 0, 0}, 2.0);
  input_a.edge_mask.fill_(true);
  auto out_a = srl::featurewise_node_lift(graph_a, input_a);

  auto graph_b = make_graph({1, 0}, {2, 1}, 3);
  auto input_b = make_input(2);
  input_b.edge_features.index_put_({0, 0, 0, 0, 0}, 2.0);
  input_b.edge_features.index_put_({0, 1, 0, 0, 0}, 1.0);
  input_b.edge_mask.fill_(true);
  auto out_b = srl::featurewise_node_lift(graph_b, input_b);
  close(scalar((out_a.node_features - out_b.node_features).abs().max()), 0.0,
        1e-5, "edge-order permutation");
}

void test_validation() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  input.edge_mask.fill_(true);

  srl::nodelift_options_t reordered_activity{};
  reordered_activity.activity_coords = {5, 4, 6, 7, 8};
  expect_throw(
      [&] {
        auto out = srl::featurewise_node_lift(graph, input, reordered_activity);
        (void)out;
      },
      "reordered activity coordinates");

  srl::nodelift_options_t bad_dtype{};
  bad_dtype.output_dtype = torch::kInt64;
  expect_throw(
      [&] {
        auto out = srl::featurewise_node_lift(graph, input, bad_dtype);
        (void)out;
      },
      "non-floating output dtype");

  auto graph_without_nodes = graph;
  graph_without_nodes.node_ids.clear();
  expect_throw(
      [&] {
        auto out = srl::featurewise_node_lift(graph_without_nodes, input);
        (void)out;
      },
      "missing node ids");
}

void test_output_dtype_policy() {
  auto graph = make_graph({0}, {1}, 2);
  auto input = make_input(1);
  input.edge_features = input.edge_features.to(torch::kFloat64);
  set_price(input, 0, 4.0);
  input.edge_mask.fill_(true);

  auto default_out = srl::featurewise_node_lift(graph, input);
  check(default_out.node_features.scalar_type() == torch::kFloat32,
        "float64 input should default to float32 output");
  check(default_out.price_residual.scalar_type() == torch::kFloat32,
        "price residual should default to float32 output");

  srl::nodelift_options_t options{};
  options.output_dtype = torch::kFloat64;
  auto float64_out = srl::featurewise_node_lift(graph, input, options);
  check(float64_out.node_features.scalar_type() == torch::kFloat64,
        "explicit float64 output dtype");
  check(float64_out.price_residual.scalar_type() == torch::kFloat64,
        "explicit float64 residual dtype");
}

} // namespace

int main() {
  test_single_edge();
  test_tree_exact_gauge_and_translation();
  test_triangle_consistent_and_inconsistent();
  test_reverse_edges();
  test_disconnected_and_isolated();
  test_coordinatewise_mask();
  test_edge_mask_overrides_coord_mask();
  test_nonfinite_values_are_masked();
  test_activity();
  test_all_invalid_and_no_nan();
  test_edge_order_permutation();
  test_validation();
  test_output_dtype_policy();
  std::cout << "[SRL test] ok\n";
  return 0;
}
