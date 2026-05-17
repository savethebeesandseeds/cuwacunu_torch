// SPDX-License-Identifier: MIT
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <torch/torch.h>

namespace srl = cuwacunu::wikimyei::expression::nodelift::srl;

namespace {

enum class graph_profile_t {
  tree,
  sparse_connected,
  dense_directed,
  disconnected,
};

struct case_t {
  std::string label;
  graph_profile_t profile;
  int64_t B;
  int64_t C;
  int64_t H;
  int64_t N;
  int64_t L;
  int64_t expected_components;
  int64_t expected_cycle_dimension;
};

const char *profile_name(graph_profile_t profile) {
  switch (profile) {
  case graph_profile_t::tree:
    return "tree";
  case graph_profile_t::sparse_connected:
    return "sparse_connected";
  case graph_profile_t::dense_directed:
    return "dense_directed";
  case graph_profile_t::disconnected:
    return "disconnected";
  }
  return "unknown";
}

srl::graph_t make_graph(int64_t N, int64_t L, graph_profile_t profile) {
  TORCH_CHECK(N > 1, "[NodeLiftComplexity test] N must be > 1");
  TORCH_CHECK(L > 0 && L <= N * (N - 1),
              "[NodeLiftComplexity test] L must fit directed complete graph");
  srl::graph_t graph{};
  graph.node_ids.reserve(static_cast<std::size_t>(N));
  for (int64_t n = 0; n < N; ++n) {
    graph.node_ids.push_back("n" + std::to_string(n));
  }

  std::vector<int64_t> base;
  std::vector<int64_t> quote;
  base.reserve(static_cast<std::size_t>(L));
  quote.reserve(static_cast<std::size_t>(L));
  std::set<std::pair<int64_t, int64_t>> used;
  auto add_edge = [&](int64_t u, int64_t v) {
    if (u == v || static_cast<int64_t>(base.size()) >= L ||
        !used.insert({u, v}).second) {
      return;
    }
    graph.edge_ids.push_back("e" + std::to_string(base.size()));
    base.push_back(u);
    quote.push_back(v);
  };

  auto fill_dense = [&] {
    for (int64_t u = 0; u < N && static_cast<int64_t>(base.size()) < L; ++u) {
      for (int64_t v = 0; v < N && static_cast<int64_t>(base.size()) < L; ++v) {
        add_edge(u, v);
      }
    }
  };

  switch (profile) {
  case graph_profile_t::tree:
    TORCH_CHECK(L == N - 1,
                "[NodeLiftComplexity test] tree profile requires L=N-1");
    for (int64_t n = 0; n + 1 < N; ++n) {
      add_edge(n, n + 1);
    }
    break;
  case graph_profile_t::sparse_connected:
    TORCH_CHECK(L >= N - 1,
                "[NodeLiftComplexity test] sparse profile requires L>=N-1");
    for (int64_t n = 0; n + 1 < N; ++n) {
      add_edge(n, n + 1);
    }
    fill_dense();
    break;
  case graph_profile_t::dense_directed:
    fill_dense();
    break;
  case graph_profile_t::disconnected: {
    TORCH_CHECK(N >= 4,
                "[NodeLiftComplexity test] disconnected profile requires N>=4");
    const int64_t split = N / 2;
    for (int64_t n = 0; n + 1 < split; ++n) {
      add_edge(n, n + 1);
    }
    for (int64_t n = split; n + 1 < N; ++n) {
      add_edge(n, n + 1);
    }
    TORCH_CHECK(static_cast<int64_t>(base.size()) == L,
                "[NodeLiftComplexity test] disconnected fixture expected "
                "two tree components");
    break;
  }
  }

  TORCH_CHECK(static_cast<int64_t>(base.size()) == L,
              "[NodeLiftComplexity test] graph profile did not produce L "
              "edges");
  graph.base_index =
      torch::tensor(base, torch::TensorOptions().dtype(torch::kInt64));
  graph.quote_index =
      torch::tensor(quote, torch::TensorOptions().dtype(torch::kInt64));
  graph.validate();
  return graph;
}

void run_case(const case_t &c) {
  torch::NoGradGuard no_grad;
  auto graph = make_graph(c.N, c.L, c.profile);
  srl::nodelift_input_t input{};
  input.edge_features = torch::randn({c.B, c.L, c.C, c.H, 9}, torch::kFloat32);
  input.edge_mask = torch::ones({c.B, c.L, c.C, c.H}, torch::kBool);

  srl::nodelift_options_t options{};
  for (int warmup = 0; warmup < 1; ++warmup) {
    auto out = srl::featurewise_node_lift(graph, input, options);
    TORCH_CHECK(out.node_features.defined(),
                "[NodeLiftComplexity test] warmup output missing");
  }

  constexpr int kRuns = 3;
  double elapsed_ms = 0.0;
  srl::nodelift_output_t last{};
  for (int run = 0; run < kRuns; ++run) {
    const auto begin = std::chrono::steady_clock::now();
    last = srl::featurewise_node_lift(graph, input, options);
    const auto end = std::chrono::steady_clock::now();
    elapsed_ms +=
        std::chrono::duration<double, std::milli>(end - begin).count();
  }

  TORCH_CHECK(last.node_features.sizes() ==
                  torch::IntArrayRef({c.B, c.C, c.H, c.N, 9}),
              "[NodeLiftComplexity test] node feature shape mismatch");
  TORCH_CHECK(last.price_residual.sizes() ==
                  torch::IntArrayRef({c.B, c.C, c.H, c.L, 4}),
              "[NodeLiftComplexity test] residual shape mismatch");
  TORCH_CHECK(!torch::isnan(last.node_features).any().item<bool>(),
              "[NodeLiftComplexity test] node features contain NaNs");
  TORCH_CHECK(last.diagnostics.component_count.max().item<int64_t>() ==
                  c.expected_components,
              "[NodeLiftComplexity test] component count mismatch");
  TORCH_CHECK(last.diagnostics.cycle_dimension.max().item<int64_t>() ==
                  c.expected_cycle_dimension,
              "[NodeLiftComplexity test] cycle dimension mismatch");
  TORCH_CHECK(last.diagnostics.failed_solve_count.sum().item<int64_t>() == 0,
              "[NodeLiftComplexity test] synthetic gauge solve failed");

  const double avg_ms = elapsed_ms / static_cast<double>(kRuns);
  const double graph_windows = static_cast<double>(c.B) *
                               static_cast<double>(c.C) *
                               static_cast<double>(c.H);
  std::cout << "[NodeLiftComplexity] label=" << c.label
            << " profile=" << profile_name(c.profile) << " B=" << c.B
            << " C=" << c.C << " H=" << c.H << " N=" << c.N << " L=" << c.L
            << " components=" << c.expected_components
            << " cycle_dimension=" << c.expected_cycle_dimension
            << " windows=" << graph_windows << " avg_ms=" << avg_ms
            << " us_per_window=" << (avg_ms * 1000.0 / graph_windows) << "\n";
}

} // namespace

int main() {
  try {
    torch::manual_seed(1701);
    const std::vector<case_t> cases{
        {.label = "tree_small",
         .profile = graph_profile_t::tree,
         .B = 1,
         .C = 1,
         .H = 4,
         .N = 6,
         .L = 5,
         .expected_components = 1,
         .expected_cycle_dimension = 0},
        {.label = "sparse_connected",
         .profile = graph_profile_t::sparse_connected,
         .B = 2,
         .C = 2,
         .H = 8,
         .N = 8,
         .L = 12,
         .expected_components = 1,
         .expected_cycle_dimension = 5},
        {.label = "dense_directed",
         .profile = graph_profile_t::dense_directed,
         .B = 2,
         .C = 2,
         .H = 8,
         .N = 10,
         .L = 30,
         .expected_components = 1,
         .expected_cycle_dimension = 21},
        {.label = "disconnected_two_trees",
         .profile = graph_profile_t::disconnected,
         .B = 1,
         .C = 2,
         .H = 6,
         .N = 10,
         .L = 8,
         .expected_components = 2,
         .expected_cycle_dimension = 0},
    };
    for (const auto &c : cases) {
      run_case(c);
    }
    std::cout << "[NodeLiftComplexity test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[NodeLiftComplexity test] failure: " << ex.what() << "\n";
    return 1;
  }
}
