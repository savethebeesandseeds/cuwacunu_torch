// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace cuwacunu::ujcamei::graph {

using node_index_t = std::int64_t;
using asset_node_id_t = std::string;

struct asset_node_t {
  asset_node_id_t node_id{};
};

} // namespace cuwacunu::ujcamei::graph
