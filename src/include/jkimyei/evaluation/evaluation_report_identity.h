// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "kikijyeba/lattice/runtime_report/runtime_lls.h"

namespace cuwacunu::jkimyei::evaluation {

struct evaluation_report_identity_t {
  std::string semantic_taxon{};
  std::string canonical_path{};
  std::string binding_id{};
  std::string source_runtime_cursor{};
  bool has_wave_cursor{false};
  std::uint64_t wave_cursor{0};
};

[[nodiscard]] inline evaluation_report_identity_t
make_evaluation_report_identity(std::string_view semantic_taxon,
                                std::string_view binding_id = {},
                                std::string_view canonical_path = {},
                                std::string_view source_runtime_cursor = {}) {
  return {
      .semantic_taxon = std::string(semantic_taxon),
      .canonical_path = std::string(canonical_path),
      .binding_id = std::string(binding_id),
      .source_runtime_cursor = std::string(source_runtime_cursor),
  };
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::
    runtime_report::runtime_report_header_t
    make_runtime_report_header(const evaluation_report_identity_t &identity) {
  return {
      .semantic_taxon = identity.semantic_taxon,
      .context =
          {
              .canonical_path = identity.canonical_path,
              .binding_id = identity.binding_id,
              .source_runtime_cursor = identity.source_runtime_cursor,
              .has_wave_cursor = identity.has_wave_cursor,
              .wave_cursor = identity.wave_cursor,
          },
  };
}

inline void append_evaluation_report_identity_entries(
    cuwacunu::kikijyeba::lattice::runtime_report::
        runtime_lls_document_t *document,
    const evaluation_report_identity_t &identity) {
  if (!document) {
    return;
  }
  cuwacunu::kikijyeba::lattice::runtime_report::
      append_runtime_report_header_entries(
          document, make_runtime_report_header(identity));
}

} // namespace cuwacunu::jkimyei::evaluation
