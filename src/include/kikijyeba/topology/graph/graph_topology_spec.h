// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>

namespace cuwacunu::kikijyeba::topology::graph {

struct graph_node_form_t {
  std::string node_id;
  std::string node_kind;
  std::string active;
};

struct graph_edge_form_t {
  std::string edge_id;
  std::string base_node;
  std::string quote_node;
  std::string source_instrument;
  std::string active;
};

struct graph_topology_spec_t {
  std::vector<graph_node_form_t> graph_node_forms{};
  std::vector<graph_edge_form_t> graph_edge_forms{};
  std::string edge_resolution_policy{"explicit_only"};
  std::string edge_source_kind{"real"};
  std::string fetch_mode{"serial"};
  std::string max_fetch_workers{"0"};
  std::string parallel_min_work_items{"16"};

  [[nodiscard]] bool empty() const {
    return graph_node_forms.empty() && graph_edge_forms.empty();
  }
};

} // namespace cuwacunu::kikijyeba::topology::graph
