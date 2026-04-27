#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hero_hashimyei_tools.h"
#include "hero/lattice_hero/hero_lattice_tools.h"
#include "hero/lattice_hero/lattice_catalog.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class LatticeRowKind : std::uint8_t {
  Family = 0,
  Project = 1,
  Member = 2,
};

inline std::string lattice_row_kind_label(LatticeRowKind kind) {
  switch (kind) {
  case LatticeRowKind::Family:
    return "family";
  case LatticeRowKind::Project:
    return "project";
  case LatticeRowKind::Member:
    return "member";
  }
  return "family";
}

enum class LatticeSection : std::uint8_t {
  Overview = 0,
  Components = 1,
  Lattice = 2,
};

inline std::string lattice_section_label(LatticeSection section) {
  switch (section) {
  case LatticeSection::Overview:
    return "overview";
  case LatticeSection::Components:
    return "components";
  case LatticeSection::Lattice:
    return "lattice";
  }
  return "overview";
}

struct LatticeFactSummary {
  std::string canonical_path{};
  std::string latest_wave_cursor{};
  std::uint64_t latest_ts_ms{0};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> source_runtime_cursors{};
};

struct LatticeViewDefinition {
  std::string view_kind{};
  std::string preferred_selector{};
  std::vector<std::string> required_selectors{};
  std::vector<std::string> optional_selectors{};
  bool ready{false};
};

struct LatticeMemberEntry {
  std::string canonical_path{};
  std::string project_path{};
  std::string family{};
  std::string display_name{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string component_compatibility_sha256_hex{};
  std::string lineage_state{};
  std::size_t component_count{0};
  std::size_t report_fragment_count{0};
  bool in_hashimyei_catalog{false};
  bool has_component{false};
  cuwacunu::hero::hashimyei::component_state_t component{};
  bool has_fact{false};
  LatticeFactSummary fact{};
  std::string latest_fact_lls{};
  std::optional<std::uint64_t> family_rank{};
  bool detail_loaded{false};
};

struct LatticeProjectGroup {
  std::string project_path{};
  std::string family{};
  std::vector<LatticeMemberEntry> members{};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::uint64_t latest_ts_ms{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> source_runtime_cursors{};
  std::vector<std::string> contract_hashes{};
};

struct LatticeFamilyGroup {
  std::string family{};
  std::vector<LatticeProjectGroup> projects{};
  std::size_t member_count{0};
  std::size_t active_member_count{0};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::uint64_t latest_ts_ms{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> contract_hashes{};
};

struct LatticeVisibleRow {
  LatticeRowKind kind{LatticeRowKind::Family};
  std::size_t family_index{0};
  std::size_t project_index{0};
  std::size_t member_index{0};
};

enum class LatticeComponentRowKind : std::uint8_t {
  Root = 0,
  Category = 1,
  Member = 2,
};

struct LatticeComponentMemberRef {
  std::size_t family_index{0};
  std::size_t project_index{0};
  std::size_t member_index{0};
};

struct LatticeComponentCategory {
  std::string root_key{};
  std::string root_label{};
  std::string category_key{};
  std::string category_label{};
  std::vector<LatticeComponentMemberRef> members{};
  std::size_t component_count{0};
  std::size_t report_fragment_count{0};
};

struct LatticeComponentRootGroup {
  std::string root_key{};
  std::string root_label{};
  std::vector<LatticeComponentCategory> categories{};
  std::size_t component_count{0};
  std::size_t report_fragment_count{0};
};

struct LatticeComponentVisibleRow {
  LatticeComponentRowKind kind{LatticeComponentRowKind::Root};
  std::size_t root_index{0};
  std::size_t category_index{0};
  std::size_t member_index{0};
};

enum class LatticeMode : std::uint8_t {
  Roots = 0,
  Facts = 1,
  Views = 2,
};

inline std::string lattice_mode_label(LatticeMode mode) {
  switch (mode) {
  case LatticeMode::Roots:
    return "lattice";
  case LatticeMode::Facts:
    return "facts";
  case LatticeMode::Views:
    return "views";
  }
  return "lattice";
}

enum class LatticeFocus : std::uint8_t {
  Navigator = 0,
  Worklist = 1,
};

enum class LatticeRefreshMode : std::uint8_t {
  Snapshot = 0,
  SyncStore = 1,
};

struct LatticeRefreshSnapshot {
  bool ok{false};
  std::string error{};
  std::vector<LatticeFamilyGroup> families{};
  std::vector<LatticeViewDefinition> views{};
  bool used_hashimyei_fallback{false};
  bool used_hashimyei_enrichment{false};
  std::size_t discovered_component_count{0};
  std::size_t discovered_report_manifest_count{0};
};

struct LatticeMemberDetailSnapshot {
  bool ok{false};
  std::string error{};
  LatticeMemberEntry member{};
};

struct LatticeViewTransportSnapshot {
  bool ok{false};
  std::string error{};
  std::string view_kind{};
  std::string canonical_path{};
  std::string contract_hash{};
  std::string component_compatibility_sha256_hex{};
  std::string wave_cursor{};
  std::size_t match_count{0};
  std::size_t ambiguity_count{0};
  std::string view_lls{};
};

struct LatticeState {
  bool ok{false};
  std::string error{};
  std::string global_config_path{};
  std::filesystem::path lattice_hero_dsl_path{};
  std::filesystem::path hashimyei_hero_dsl_path{};
  cuwacunu::hero::lattice_mcp::app_context_t app{};
  cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t lattice_defaults{};
  cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t
      hashimyei_defaults{};
  bool defaults_loaded{false};
  std::vector<LatticeFamilyGroup> families{};
  std::vector<LatticeComponentRootGroup> component_roots{};
  std::vector<LatticeViewDefinition> views{};
  std::vector<LatticeVisibleRow> rows{};
  std::vector<LatticeComponentVisibleRow> component_rows{};
  LatticeSection selected_section{LatticeSection::Overview};
  std::size_t selected_row{0};
  std::size_t selected_component_row{0};
  std::size_t selected_view_index{0};
  LatticeMode mode{LatticeMode::Roots};
  LatticeFocus focus{LatticeFocus::Navigator};
  std::size_t selected_mode_row{0};
  std::string status{};
  bool status_is_error{false};
  bool used_hashimyei_fallback{false};
  bool used_hashimyei_enrichment{false};
  std::size_t discovered_component_count{0};
  std::size_t discovered_report_manifest_count{0};
  bool refresh_pending{false};
  bool refresh_requeue{false};
  LatticeRefreshMode refresh_requeue_mode{LatticeRefreshMode::Snapshot};
  std::future<LatticeRefreshSnapshot> refresh_future{};
  std::uint64_t refresh_started_at_ms{0};
  bool refresh_timed_out{false};
  bool detail_refresh_pending{false};
  bool detail_requeue{false};
  std::future<LatticeMemberDetailSnapshot> detail_future{};
  bool view_refresh_pending{false};
  bool view_requeue{false};
  std::future<LatticeViewTransportSnapshot> view_future{};
  LatticeViewTransportSnapshot view_snapshot{};
  LatticeRefreshMode refresh_mode{LatticeRefreshMode::Snapshot};
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
