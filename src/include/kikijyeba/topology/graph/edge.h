// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "kikijyeba/topology/graph/node.h"
#include "ujcamei/source/registry/instrument_signature.h"

namespace cuwacunu::kikijyeba::topology::graph {

using edge_index_t = std::int64_t;
using instrument_edge_id_t = std::string;

enum class endpoint_role_t {
  base,
  quote,
};

struct directed_edge_t {
  instrument_edge_id_t edge_id{};
  asset_node_id_t base_node_id{};
  asset_node_id_t quote_node_id{};
};

struct directed_instrument_edge_t {
  instrument_edge_id_t edge_id{};
  asset_node_id_t base_node_id{};
  asset_node_id_t quote_node_id{};
  cuwacunu::ujcamei::source::registry::instrument_signature_t instrument{};
};

[[nodiscard]] inline instrument_edge_id_t instrument_edge_id_from_signature(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &signature) {
  return signature.symbol;
}

[[nodiscard]] inline directed_edge_t
make_directed_edge(instrument_edge_id_t edge_id, asset_node_id_t base_node_id,
                   asset_node_id_t quote_node_id) {
  return directed_edge_t{
      .edge_id = std::move(edge_id),
      .base_node_id = std::move(base_node_id),
      .quote_node_id = std::move(quote_node_id),
  };
}

[[nodiscard]] inline directed_instrument_edge_t make_directed_instrument_edge(
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &signature) {
  return directed_instrument_edge_t{
      .edge_id = instrument_edge_id_from_signature(signature),
      .base_node_id = signature.base_asset,
      .quote_node_id = signature.quote_asset,
      .instrument = signature,
  };
}

} // namespace cuwacunu::kikijyeba::topology::graph
