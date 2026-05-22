// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "jkimyei/evaluation/source/data_analytics.h"
#include "kikijyeba/lattice/runtime_report/report_taxonomy.h"

namespace cuwacunu::jkimyei::evaluation {

struct source_data_analytics_artifact_paths_t {
  std::filesystem::path numeric_file{};
  std::filesystem::path symbolic_file{};
};

[[nodiscard]] inline source_data_analytics_artifact_paths_t
latest_source_data_analytics_artifact_paths(
    std::string_view contract_hash, std::string_view canonical_path,
    std::string_view source_runtime_cursor) {
  return {
      .numeric_file =
          cuwacunu::jkimyei::evaluation::source_data_analytics_latest_file_path(
              contract_hash, canonical_path, source_runtime_cursor),
      .symbolic_file = cuwacunu::jkimyei::evaluation::
          source_data_analytics_symbolic_latest_file_path(
              contract_hash, canonical_path, source_runtime_cursor),
  };
}

[[nodiscard]] inline evaluation_report_identity_t
make_source_data_analytics_report_identity(
    std::string_view binding_id, std::string_view source_runtime_cursor) {
  return make_evaluation_report_identity(
      cuwacunu::kikijyeba::lattice::runtime_report::
          report_taxonomy::kSourceData,
      binding_id, "ujcamei.source.retrieval", source_runtime_cursor);
}

[[nodiscard]] inline evaluation_report_identity_t
make_source_data_symbolic_analytics_report_identity(
    std::string_view binding_id, std::string_view source_runtime_cursor) {
  return make_evaluation_report_identity(
      cuwacunu::kikijyeba::lattice::runtime_report::
          report_taxonomy::kSourceData,
      binding_id, "ujcamei.source.retrieval.symbolic", source_runtime_cursor);
}

} // namespace cuwacunu::jkimyei::evaluation
