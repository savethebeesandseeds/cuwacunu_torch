// SPDX-License-Identifier: MIT
#include "wikimyei/expression/nodelift/srl/stream/srl_graph_adapter.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl::stream {

cuwacunu::wikimyei::expression::nodelift::srl::graph_t
make_srl_graph(const cuwacunu::ujcamei::graph::market_graph_t &market_graph,
               std::optional<torch::Device> device) {
  market_graph.validate();

  const torch::Device target_device =
      device.value_or(torch::Device(torch::kCPU));
  const auto index_options =
      torch::TensorOptions().dtype(torch::kInt64).device(target_device);

  cuwacunu::wikimyei::expression::nodelift::srl::graph_t graph{};
  graph.node_ids = market_graph.node_ids;
  graph.edge_ids = market_graph.edge_ids;
  graph.base_index = torch::tensor(market_graph.base_index, index_options);
  graph.quote_index = torch::tensor(market_graph.quote_index, index_options);
  graph.graph_order_fingerprint =
      market_graph.computed_graph_order_fingerprint();
  graph.validate();
  return graph;
}

} // namespace cuwacunu::wikimyei::expression::nodelift::srl::stream
