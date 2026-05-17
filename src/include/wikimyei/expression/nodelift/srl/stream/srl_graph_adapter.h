// SPDX-License-Identifier: MIT
#pragma once

#include <optional>

#include <torch/torch.h>

#include "ujcamei/graph/graph.h"
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl::stream {

[[nodiscard]] cuwacunu::wikimyei::expression::nodelift::srl::graph_t
make_srl_graph(const cuwacunu::ujcamei::graph::market_graph_t &market_graph,
               std::optional<torch::Device> device = std::nullopt);

} // namespace cuwacunu::wikimyei::expression::nodelift::srl::stream
