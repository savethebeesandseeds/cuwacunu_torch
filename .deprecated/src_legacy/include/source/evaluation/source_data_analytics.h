// ./include/source/evaluation/source_data_analytics.h
// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "piaabo/latent_lineage_state/report_taxonomy.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "tsiemene/tsi.report.h"

namespace cuwacunu::source::evaluation {

struct source_data_analytics_artifact_paths_t {
  std::filesystem::path numeric_file{};
  std::filesystem::path symbolic_file{};
};

[[nodiscard]] inline source_data_analytics_artifact_paths_t
latest_source_data_analytics_artifact_paths(std::string_view contract_hash,
                                            std::string_view canonical_path,
                                            std::string_view source_runtime_cursor) {
  return {
      .numeric_file =
          cuwacunu::piaabo::torch_compat::source_data_analytics_latest_file_path(
              contract_hash, canonical_path, source_runtime_cursor),
      .symbolic_file = cuwacunu::piaabo::torch_compat::
          source_data_analytics_symbolic_latest_file_path(
              contract_hash, canonical_path, source_runtime_cursor),
  };
}

[[nodiscard]] inline ::tsiemene::component_report_identity_t
make_source_data_analytics_report_identity(std::string_view binding_id,
                                           std::string_view source_runtime_cursor) {
  auto identity = ::tsiemene::make_component_report_identity(
      "tsi.source.dataloader",
      binding_id,
      cuwacunu::piaabo::latent_lineage_state::report_taxonomy::kSourceData);
  identity.source_runtime_cursor = std::string(source_runtime_cursor);
  return identity;
}

[[nodiscard]] inline ::tsiemene::component_report_identity_t
make_source_data_symbolic_analytics_report_identity(
    std::string_view binding_id, std::string_view source_runtime_cursor) {
  auto identity = ::tsiemene::make_component_report_identity(
      "tsi.source.dataloader",
      binding_id,
      cuwacunu::piaabo::latent_lineage_state::report_taxonomy::kSourceData);
  identity.source_runtime_cursor = std::string(source_runtime_cursor);
  return identity;
}

}  // namespace cuwacunu::source::evaluation
