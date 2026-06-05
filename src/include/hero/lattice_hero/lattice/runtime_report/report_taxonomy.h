// ./include/hero/lattice_hero/lattice/runtime_report/report_taxonomy.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string_view>

namespace cuwacunu::hero::lattice::runtime_report::report_taxonomy {

inline constexpr std::string_view kSourceData = "source.data";
inline constexpr std::string_view kEmbeddingData = "embedding.data";
inline constexpr std::string_view kEmbeddingEvaluation = "embedding.evaluation";
inline constexpr std::string_view kEmbeddingNetwork = "embedding.network";

}  // namespace cuwacunu::hero::lattice::runtime_report::report_taxonomy
