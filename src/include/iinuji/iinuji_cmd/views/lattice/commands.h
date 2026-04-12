#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <map>
#include <sstream>
#include <set>
#include <utility>
#include <vector>

#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "piaabo/djson_parsing.h"
#include "piaabo/dlogs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::vector<std::string> lattice_split_segments(std::string_view text) {
  std::vector<std::string> out{};
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t dot = text.find('.', start);
    const std::string_view segment =
        dot == std::string_view::npos ? text.substr(start)
                                      : text.substr(start, dot - start);
    if (!segment.empty()) out.emplace_back(segment);
    if (dot == std::string_view::npos) break;
    start = dot + 1;
  }
  return out;
}

inline std::string normalize_lattice_canonical_path(
    std::string_view canonical_path) {
  return cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
      trim_copy(std::string(canonical_path)));
}

inline bool lattice_has_hashimyei_suffix(std::string_view canonical_path,
                                         std::string* out_hashimyei = nullptr) {
  const std::size_t dot = canonical_path.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical_path.size()) {
    return false;
  }
  std::string normalized{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(
          std::string_view(canonical_path).substr(dot + 1), &normalized)) {
    return false;
  }
  if (out_hashimyei) *out_hashimyei = std::move(normalized);
  return true;
}

inline std::string lattice_project_path_for_canonical(
    std::string_view canonical_path) {
  const std::string normalized = normalize_lattice_canonical_path(canonical_path);
  if (!lattice_has_hashimyei_suffix(normalized)) return normalized;
  const std::size_t dot = normalized.rfind('.');
  return dot == std::string::npos ? normalized : normalized.substr(0, dot);
}

inline std::string lattice_family_from_canonical(std::string_view canonical_path) {
  const auto segments = lattice_split_segments(canonical_path);
  if (segments.size() >= 3 && segments[0] == "tsi" && segments[1] == "wikimyei") {
    return segments[2];
  }
  if (segments.size() >= 2 && segments[0] == "tsi") return segments[1];
  if (!segments.empty()) return segments.front();
  return "unknown";
}

inline std::string lattice_display_name_for_member(std::string_view canonical_path,
                                                   std::string_view hashimyei) {
  if (!trim_copy(std::string(hashimyei)).empty()) return std::string(hashimyei);
  const std::string project_path = lattice_project_path_for_canonical(canonical_path);
  const auto segments = lattice_split_segments(project_path);
  if (!segments.empty()) return segments.back();
  return std::string(canonical_path);
}

inline std::string lattice_contract_hash_for_member(
    const LatticeMemberEntry& member) {
  if (!member.contract_hash.empty()) return member.contract_hash;
  if (!member.has_component) return {};
  return cuwacunu::hero::hashimyei::contract_hash_from_identity(
      member.component.manifest.contract_identity);
}

inline std::uint64_t lattice_member_latest_ts_ms(const LatticeMemberEntry& member) {
  std::uint64_t ts_ms = 0;
  if (member.has_component) ts_ms = std::max(ts_ms, member.component.ts_ms);
  if (member.has_fact) ts_ms = std::max(ts_ms, member.fact.latest_ts_ms);
  return ts_ms;
}

inline std::string lattice_member_lineage_state_text(
    const LatticeMemberEntry& member) {
  if (!trim_copy(member.lineage_state).empty()) {
    return member.lineage_state;
  }
  if (member.has_component && !member.component.manifest.lineage_state.empty()) {
    return member.component.manifest.lineage_state;
  }
  if (member.has_fact) return "fact-only";
  return "unknown";
}

inline std::string lattice_member_parent_hashimyei_text(
    const LatticeMemberEntry& member) {
  if (!member.has_component || !member.component.manifest.parent_identity.has_value()) {
    return {};
  }
  return member.component.manifest.parent_identity->name;
}

inline std::size_t count_lattice_projects(const CmdState& st) {
  std::size_t count = 0;
  for (const auto& family : st.lattice.families) count += family.projects.size();
  return count;
}

inline std::size_t count_lattice_members(const CmdState& st) {
  std::size_t count = 0;
  for (const auto& family : st.lattice.families) count += family.member_count;
  return count;
}

inline std::size_t count_lattice_component_members(const CmdState& st) {
  std::size_t count = 0;
  for (const auto& root : st.lattice.component_roots) {
    count += root.component_count;
  }
  return count;
}

inline std::size_t count_lattice_fact_members(const CmdState& st) {
  std::size_t count = 0;
  for (const auto& family : st.lattice.families) {
    for (const auto& project : family.projects) {
      count += static_cast<std::size_t>(std::count_if(
          project.members.begin(), project.members.end(),
          [](const auto& member) { return member.has_fact; }));
    }
  }
  return count;
}

inline bool lattice_member_is_component_cataloged(
    const LatticeMemberEntry& member) {
  return member.in_hashimyei_catalog || member.has_component ||
         member.component_count > 0;
}

inline std::string lattice_pluralize_component_label(std::string_view text) {
  const std::string base = trim_copy(std::string(text));
  if (base.empty()) return "components";
  if (!base.empty() && base.back() == 's') return base;
  if (base == "dataloader") return "dataloaders";
  if (base == "representation") return "representations";
  if (base == "inference") return "inferences";
  return base + "s";
}

inline std::string lattice_component_root_key_for_member(
    const LatticeMemberEntry& member) {
  const auto segments = lattice_split_segments(member.canonical_path);
  if (segments.size() >= 2 && segments[0] == "tsi" && segments[1] == "source") {
    return "sources";
  }
  if (segments.size() >= 2 && segments[0] == "tsi" &&
      segments[1] == "wikimyei") {
    return "wikimyei";
  }
  return "other";
}

inline std::string lattice_component_root_label(std::string_view root_key) {
  if (root_key == "sources") return "Sources";
  if (root_key == "wikimyei") return "Wikimyei";
  return "Other";
}

inline std::string lattice_component_category_key_for_member(
    const LatticeMemberEntry& member) {
  const auto root_key = lattice_component_root_key_for_member(member);
  const auto segments = lattice_split_segments(member.canonical_path);
  if (root_key == "sources") {
    if (segments.size() >= 3) return lattice_pluralize_component_label(segments[2]);
    return "sources";
  }
  if (root_key == "wikimyei") {
    if (!member.family.empty()) return lattice_pluralize_component_label(member.family);
    if (segments.size() >= 3) return lattice_pluralize_component_label(segments[2]);
    return "wikimyei";
  }
  if (segments.size() >= 2) {
    return lattice_pluralize_component_label(segments.back());
  }
  return "components";
}

inline std::string lattice_component_category_label(std::string_view category_key) {
  if (category_key.empty()) return "Components";
  std::string label = std::string(category_key);
  if (!label.empty()) label[0] = static_cast<char>(std::toupper(label[0]));
  return label;
}

inline int lattice_component_root_rank(std::string_view root_key) {
  if (root_key == "sources") return 0;
  if (root_key == "wikimyei") return 1;
  return 2;
}

inline int lattice_component_category_rank(std::string_view category_key) {
  if (category_key == "dataloaders") return 0;
  if (category_key == "representations") return 1;
  if (category_key == "inferences") return 2;
  return 3;
}

inline std::uint64_t lattice_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline constexpr std::uint64_t kLatticeRefreshUiTimeoutMs = 3000;

inline std::string lattice_source_mode_text(const CmdState& st) {
  if (st.lattice.used_hashimyei_fallback) return "hashimyei";
  if (st.lattice.used_hashimyei_enrichment) return "lattice+hashimyei";
  return "lattice";
}

inline const LatticeVisibleRow* selected_lattice_row_ptr(const CmdState& st) {
  if (st.lattice.rows.empty()) return nullptr;
  const std::size_t index =
      std::min(st.lattice.selected_row, st.lattice.rows.size() - 1);
  return &st.lattice.rows[index];
}

inline const LatticeFamilyGroup* selected_lattice_family_ptr(const CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row || row->family_index >= st.lattice.families.size()) return nullptr;
  return &st.lattice.families[row->family_index];
}

inline const LatticeProjectGroup* selected_lattice_project_ptr(const CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row) return nullptr;
  const auto* family = selected_lattice_family_ptr(st);
  if (!family || row->project_index >= family->projects.size()) return nullptr;
  if (row->kind == LatticeRowKind::Family) return nullptr;
  return &family->projects[row->project_index];
}

inline const LatticeMemberEntry* selected_lattice_member_ptr(const CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row || row->kind != LatticeRowKind::Member) return nullptr;
  const auto* project = selected_lattice_project_ptr(st);
  if (!project || row->member_index >= project->members.size()) return nullptr;
  return &project->members[row->member_index];
}

inline const LatticeViewDefinition* selected_lattice_view_ptr(const CmdState& st) {
  if (st.lattice.views.empty()) return nullptr;
  const std::size_t index =
      std::min(st.lattice.selected_view_index, st.lattice.views.size() - 1);
  return &st.lattice.views[index];
}

inline LatticeFamilyGroup* selected_lattice_family_mutable_ptr(CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row || row->family_index >= st.lattice.families.size()) return nullptr;
  return &st.lattice.families[row->family_index];
}

inline LatticeProjectGroup* selected_lattice_project_mutable_ptr(CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row) return nullptr;
  auto* family = selected_lattice_family_mutable_ptr(st);
  if (!family || row->project_index >= family->projects.size()) return nullptr;
  if (row->kind == LatticeRowKind::Family) return nullptr;
  return &family->projects[row->project_index];
}

inline LatticeMemberEntry* selected_lattice_member_mutable_ptr(CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row || row->kind != LatticeRowKind::Member) return nullptr;
  auto* project = selected_lattice_project_mutable_ptr(st);
  if (!project || row->member_index >= project->members.size()) return nullptr;
  return &project->members[row->member_index];
}

inline const LatticeComponentVisibleRow* selected_component_row_ptr(
    const CmdState& st) {
  if (st.lattice.component_rows.empty()) return nullptr;
  const std::size_t index = std::min(st.lattice.selected_component_row,
                                     st.lattice.component_rows.size() - 1);
  return &st.lattice.component_rows[index];
}

inline const LatticeComponentRootGroup* selected_component_root_ptr(
    const CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row || row->root_index >= st.lattice.component_roots.size()) return nullptr;
  return &st.lattice.component_roots[row->root_index];
}

inline const LatticeComponentCategory* selected_component_category_ptr(
    const CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return nullptr;
  const auto* root = selected_component_root_ptr(st);
  if (!root || row->category_index >= root->categories.size()) return nullptr;
  if (row->kind == LatticeComponentRowKind::Root) return nullptr;
  return &root->categories[row->category_index];
}

inline const LatticeMemberEntry* selected_component_member_ptr(
    const CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row || row->kind != LatticeComponentRowKind::Member) return nullptr;
  const auto* category = selected_component_category_ptr(st);
  if (!category || row->member_index >= category->members.size()) return nullptr;
  const auto& ref = category->members[row->member_index];
  if (ref.family_index >= st.lattice.families.size()) return nullptr;
  const auto& family = st.lattice.families[ref.family_index];
  if (ref.project_index >= family.projects.size()) return nullptr;
  const auto& project = family.projects[ref.project_index];
  if (ref.member_index >= project.members.size()) return nullptr;
  return &project.members[ref.member_index];
}

inline LatticeMode selected_lattice_root_mode(const CmdState& st) {
  return st.lattice.selected_mode_row == 0 ? LatticeMode::Facts
                                           : LatticeMode::Views;
}

inline const LatticeMemberEntry* selected_lattice_detail_member_ptr(
    const CmdState& st) {
  if (st.lattice.selected_section == LatticeSection::Components) {
    return selected_component_member_ptr(st);
  }
  if (st.lattice.selected_section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Facts) {
    return selected_lattice_member_ptr(st);
  }
  return nullptr;
}

inline bool lattice_is_navigator_focus(const CmdState& st) {
  return st.lattice.focus == LatticeFocus::Navigator;
}

inline bool lattice_is_worklist_focus(const CmdState& st) {
  return st.lattice.focus == LatticeFocus::Worklist;
}

inline bool lattice_focus_worklist(CmdState& st) {
  if (st.lattice.focus == LatticeFocus::Worklist) return false;
  st.lattice.focus = LatticeFocus::Worklist;
  return true;
}

inline bool lattice_focus_navigator(CmdState& st) {
  if (st.lattice.focus == LatticeFocus::Navigator) return false;
  st.lattice.focus = LatticeFocus::Navigator;
  return true;
}

inline void clamp_lattice_state(CmdState& st) {
  if (st.lattice.rows.empty()) {
    st.lattice.selected_row = 0;
  } else if (st.lattice.selected_row >= st.lattice.rows.size()) {
    st.lattice.selected_row = st.lattice.rows.size() - 1;
  }
  if (st.lattice.component_rows.empty()) {
    st.lattice.selected_component_row = 0;
  } else if (st.lattice.selected_component_row >= st.lattice.component_rows.size()) {
    st.lattice.selected_component_row = st.lattice.component_rows.size() - 1;
  }
  st.lattice.selected_mode_row = std::min<std::size_t>(st.lattice.selected_mode_row, 1);
  if (st.lattice.views.empty()) {
    st.lattice.selected_view_index = 0;
  } else if (st.lattice.selected_view_index >= st.lattice.views.size()) {
    st.lattice.selected_view_index = st.lattice.views.size() - 1;
  }
}

inline void rebuild_lattice_visible_rows(CmdState& st) {
  st.lattice.rows.clear();
  for (std::size_t i = 0; i < st.lattice.families.size(); ++i) {
    bool family_has_facts = false;
    for (std::size_t j = 0; j < st.lattice.families[i].projects.size(); ++j) {
      bool project_has_facts = false;
      for (std::size_t k = 0; k < st.lattice.families[i].projects[j].members.size();
           ++k) {
        if (!st.lattice.families[i].projects[j].members[k].has_fact) continue;
        if (!family_has_facts) {
          st.lattice.rows.push_back(
              LatticeVisibleRow{LatticeRowKind::Family, i, 0, 0});
          family_has_facts = true;
        }
        if (!project_has_facts) {
          st.lattice.rows.push_back(
              LatticeVisibleRow{LatticeRowKind::Project, i, j, 0});
          project_has_facts = true;
        }
        st.lattice.rows.push_back(
            LatticeVisibleRow{LatticeRowKind::Member, i, j, k});
      }
    }
  }
  clamp_lattice_state(st);
}

inline void rebuild_lattice_component_rows(CmdState& st) {
  st.lattice.component_roots.clear();
  st.lattice.component_rows.clear();

  for (std::size_t i = 0; i < st.lattice.families.size(); ++i) {
    const auto& family = st.lattice.families[i];
    for (std::size_t j = 0; j < family.projects.size(); ++j) {
      const auto& project = family.projects[j];
      for (std::size_t k = 0; k < project.members.size(); ++k) {
        const auto& member = project.members[k];
        if (!lattice_member_is_component_cataloged(member)) continue;

        const std::string root_key = lattice_component_root_key_for_member(member);
        const std::string category_key =
            lattice_component_category_key_for_member(member);

        auto root_it = std::find_if(
            st.lattice.component_roots.begin(), st.lattice.component_roots.end(),
            [&](const auto& root) { return root.root_key == root_key; });
        if (root_it == st.lattice.component_roots.end()) {
          st.lattice.component_roots.push_back(LatticeComponentRootGroup{
              .root_key = root_key,
              .root_label = lattice_component_root_label(root_key),
          });
          root_it = std::prev(st.lattice.component_roots.end());
        }
        root_it->component_count += 1;
        root_it->report_fragment_count += member.report_fragment_count;

        auto category_it = std::find_if(
            root_it->categories.begin(), root_it->categories.end(),
            [&](const auto& category) {
              return category.category_key == category_key;
            });
        if (category_it == root_it->categories.end()) {
          root_it->categories.push_back(LatticeComponentCategory{
              .root_key = root_key,
              .root_label = root_it->root_label,
              .category_key = category_key,
              .category_label = lattice_component_category_label(category_key),
          });
          category_it = std::prev(root_it->categories.end());
        }
        category_it->component_count += 1;
        category_it->report_fragment_count += member.report_fragment_count;
        category_it->members.push_back(
            LatticeComponentMemberRef{i, j, k});
      }
    }
  }

  std::sort(st.lattice.component_roots.begin(), st.lattice.component_roots.end(),
            [](const auto& a, const auto& b) {
              const int ar = lattice_component_root_rank(a.root_key);
              const int br = lattice_component_root_rank(b.root_key);
              if (ar != br) return ar < br;
              return a.root_label < b.root_label;
            });
  for (auto& root : st.lattice.component_roots) {
    std::sort(root.categories.begin(), root.categories.end(),
              [](const auto& a, const auto& b) {
                const int ar = lattice_component_category_rank(a.category_key);
                const int br = lattice_component_category_rank(b.category_key);
                if (ar != br) return ar < br;
                return a.category_label < b.category_label;
              });
    for (auto& category : root.categories) {
      std::sort(category.members.begin(), category.members.end(),
                [&](const auto& a, const auto& b) {
                  const auto& am =
                      st.lattice.families[a.family_index]
                          .projects[a.project_index]
                          .members[a.member_index];
                  const auto& bm =
                      st.lattice.families[b.family_index]
                          .projects[b.project_index]
                          .members[b.member_index];
                  return am.canonical_path < bm.canonical_path;
                });
    }
  }

  for (std::size_t i = 0; i < st.lattice.component_roots.size(); ++i) {
    st.lattice.component_rows.push_back(
        LatticeComponentVisibleRow{LatticeComponentRowKind::Root, i, 0, 0});
    for (std::size_t j = 0; j < st.lattice.component_roots[i].categories.size();
         ++j) {
      st.lattice.component_rows.push_back(LatticeComponentVisibleRow{
          LatticeComponentRowKind::Category, i, j, 0});
      for (std::size_t k = 0;
           k < st.lattice.component_roots[i].categories[j].members.size(); ++k) {
        st.lattice.component_rows.push_back(LatticeComponentVisibleRow{
            LatticeComponentRowKind::Member, i, j, k});
      }
    }
  }

  clamp_lattice_state(st);
}

inline void set_lattice_status(CmdState& st, std::string status, bool is_error) {
  st.lattice.status = std::move(status);
  st.lattice.status_is_error = is_error;
  if (is_error) {
    st.lattice.ok = false;
    st.lattice.error = st.lattice.status;
  } else {
    st.lattice.error.clear();
  }
}

using lattice_json_value_t = cuwacunu::piaabo::JsonValue;
using lattice_json_type_t = cuwacunu::piaabo::JsonValueType;

inline const lattice_json_value_t* lattice_json_field(
    const lattice_json_value_t* object, std::string_view key) {
  if (object == nullptr || object->type != lattice_json_type_t::OBJECT ||
      !object->objectValue) {
    return nullptr;
  }
  const auto it = object->objectValue->find(std::string(key));
  if (it == object->objectValue->end()) return nullptr;
  return &it->second;
}

inline std::string lattice_json_string(const lattice_json_value_t* value,
                                       std::string fallback = {}) {
  if (value == nullptr) return fallback;
  if (value->type == lattice_json_type_t::STRING) return value->stringValue;
  return fallback;
}

inline bool lattice_json_bool(const lattice_json_value_t* value,
                              bool fallback = false) {
  if (value == nullptr) return fallback;
  if (value->type == lattice_json_type_t::BOOLEAN) return value->boolValue;
  return fallback;
}

inline std::uint64_t lattice_json_u64(const lattice_json_value_t* value,
                                      std::uint64_t fallback = 0) {
  if (value == nullptr) return fallback;
  if (value->type == lattice_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  if (value->type == lattice_json_type_t::STRING) {
    std::uint64_t out = 0;
    if (cuwacunu::hero::wave::lattice_catalog_store_t::parse_runtime_wave_cursor_token(
            value->stringValue, &out)) {
      return out;
    }
  }
  return fallback;
}

inline std::optional<std::uint64_t> lattice_json_optional_u64(
    const lattice_json_value_t* value) {
  if (value == nullptr || value->type == lattice_json_type_t::NULL_TYPE) {
    return std::nullopt;
  }
  if (value->type == lattice_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  return std::nullopt;
}

inline std::vector<std::string> lattice_json_string_array(
    const lattice_json_value_t* value) {
  std::vector<std::string> out{};
  if (value == nullptr || value->type != lattice_json_type_t::ARRAY ||
      !value->arrayValue) {
    return out;
  }
  out.reserve(value->arrayValue->size());
  for (const auto& entry : *value->arrayValue) {
    if (entry.type == lattice_json_type_t::STRING) {
      out.push_back(entry.stringValue);
    }
  }
  return out;
}

inline bool lattice_fill_latest_fact_summary_from_json(
    const lattice_json_value_t* object, LatticeFactSummary* out,
    std::string* error) {
  if (error) error->clear();
  if (object == nullptr || out == nullptr ||
      object->type != lattice_json_type_t::OBJECT) {
    if (error) *error = "invalid lattice fact summary";
    return false;
  }
  *out = LatticeFactSummary{};
  out->canonical_path =
      lattice_json_string(lattice_json_field(object, "canonical_path"));
  out->latest_wave_cursor =
      lattice_json_string(lattice_json_field(object, "latest_wave_cursor"));
  out->latest_ts_ms =
      lattice_json_u64(lattice_json_field(object, "latest_ts_ms"));
  out->fragment_count =
      lattice_json_u64(lattice_json_field(object, "fragment_count"));
  out->available_context_count =
      lattice_json_u64(lattice_json_field(object, "available_context_count"));
  out->binding_ids =
      lattice_json_string_array(lattice_json_field(object, "binding_ids"));
  out->semantic_taxa =
      lattice_json_string_array(lattice_json_field(object, "semantic_taxa"));
  out->source_runtime_cursors =
      lattice_json_string_array(lattice_json_field(object, "source_runtime_cursors"));
  return !out->canonical_path.empty();
}

struct lattice_hashimyei_ref_t {
  std::string canonical_path{};
  std::string hashimyei{};
  std::size_t component_count{0};
  std::size_t report_fragment_count{0};
};

inline bool lattice_fill_hashimyei_ref_from_json(
    const lattice_json_value_t* object, lattice_hashimyei_ref_t* out,
    std::string* error) {
  if (error) error->clear();
  if (object == nullptr || out == nullptr ||
      object->type != lattice_json_type_t::OBJECT) {
    if (error) *error = "invalid hashimyei ref row";
    return false;
  }
  *out = lattice_hashimyei_ref_t{};
  out->canonical_path =
      lattice_json_string(lattice_json_field(object, "canonical_path"));
  out->hashimyei = lattice_json_string(lattice_json_field(object, "hashimyei"));
  out->component_count =
      lattice_json_u64(lattice_json_field(object, "component_count"));
  out->report_fragment_count =
      lattice_json_u64(lattice_json_field(object, "report_fragment_count"));
  return !out->canonical_path.empty();
}

inline void lattice_append_unique_value(std::vector<std::string>* out,
                                        std::string value) {
  if (out == nullptr) return;
  value = trim_copy(std::move(value));
  if (value.empty()) return;
  if (std::find(out->begin(), out->end(), value) != out->end()) return;
  out->push_back(std::move(value));
}

inline void lattice_merge_string_values(std::vector<std::string>* out,
                                        const std::vector<std::string>& values) {
  if (out == nullptr) return;
  for (const auto& value : values) lattice_append_unique_value(out, value);
}

inline bool lattice_parse_tool_structured_content(
    std::string_view tool_result_json, lattice_json_value_t* out_structured,
    std::string* error) {
  if (error) error->clear();
  if (out_structured == nullptr) {
    if (error) *error = "structured content output pointer is null";
    return false;
  }
  *out_structured = lattice_json_value_t{};
  try {
    const lattice_json_value_t root =
        cuwacunu::piaabo::JsonParser(std::string(tool_result_json)).parse();
    const auto* is_error = lattice_json_field(&root, "isError");
    if (lattice_json_bool(is_error, false)) {
      const auto* structured = lattice_json_field(&root, "structuredContent");
      std::string message{};
      if (structured != nullptr) {
        message = lattice_json_string(lattice_json_field(structured, "error"));
      }
      if (message.empty()) {
        if (const auto* content = lattice_json_field(&root, "content");
            content != nullptr && content->type == lattice_json_type_t::ARRAY &&
            content->arrayValue && !content->arrayValue->empty()) {
          message = lattice_json_string(
              lattice_json_field(&content->arrayValue->front(), "text"));
        }
      }
      if (error) *error = message.empty() ? "tool returned error" : message;
      return false;
    }
    const auto* structured = lattice_json_field(&root, "structuredContent");
    if (structured == nullptr) {
      if (error) *error = "tool result missing structuredContent";
      return false;
    }
    *out_structured = *structured;
    return true;
  } catch (const std::exception& ex) {
    if (error) *error = ex.what();
    return false;
  }
}

inline bool lattice_invoke_tool(
    cuwacunu::hero::lattice_mcp::app_context_t* app,
    const std::string& tool_name, const std::string& arguments_json,
    lattice_json_value_t* out_structured, std::string* error) {
  if (error) error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error) *error = "missing lattice app context";
    return false;
  }
  if (!cuwacunu::hero::lattice_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return lattice_parse_tool_structured_content(result_json, out_structured, error);
}

inline bool lattice_invoke_tool(
    CmdState& st, const std::string& tool_name, const std::string& arguments_json,
    lattice_json_value_t* out_structured, std::string* error) {
  return lattice_invoke_tool(&st.lattice.app, tool_name, arguments_json,
                             out_structured, error);
}

inline bool lattice_load_view_definitions(
    cuwacunu::hero::lattice_mcp::app_context_t* app,
    std::vector<LatticeViewDefinition>* out, std::string* error) {
  if (error) error->clear();
  if (out == nullptr) {
    if (error) *error = "lattice view output pointer is null";
    return false;
  }
  out->clear();
  lattice_json_value_t structured{};
  if (!lattice_invoke_tool(app, "hero.lattice.list_views", "{}", &structured,
                           error)) {
    return false;
  }
  const auto* views = lattice_json_field(&structured, "views");
  if (views == nullptr || views->type != lattice_json_type_t::ARRAY ||
      !views->arrayValue) {
    if (error) *error = "lattice views result missing views";
    return false;
  }
  out->reserve(views->arrayValue->size());
  for (const auto& entry : *views->arrayValue) {
    if (entry.type != lattice_json_type_t::OBJECT) continue;
    LatticeViewDefinition view{};
    view.view_kind = lattice_json_string(lattice_json_field(&entry, "view_kind"));
    view.preferred_selector =
        lattice_json_string(lattice_json_field(&entry, "preferred_selector"));
    view.required_selectors =
        lattice_json_string_array(lattice_json_field(&entry, "required_selectors"));
    view.optional_selectors =
        lattice_json_string_array(lattice_json_field(&entry, "optional_selectors"));
    view.ready = lattice_json_bool(lattice_json_field(&entry, "ready"));
    if (!view.view_kind.empty()) out->push_back(std::move(view));
  }
  return true;
}

inline bool lattice_load_latest_fact_summaries(
    cuwacunu::hero::lattice_mcp::app_context_t* app,
    std::vector<LatticeFactSummary>* out, std::string* error) {
  if (error) error->clear();
  if (out == nullptr) {
    if (error) *error = "lattice fact output pointer is null";
    return false;
  }
  out->clear();
  lattice_json_value_t structured{};
  if (!lattice_invoke_tool(app, "hero.lattice.list_facts", "{}", &structured,
                           error)) {
    return false;
  }
  const auto* facts = lattice_json_field(&structured, "facts");
  if (facts == nullptr || facts->type != lattice_json_type_t::ARRAY ||
      !facts->arrayValue) {
    if (error) *error = "lattice facts result missing facts";
    return false;
  }
  out->reserve(facts->arrayValue->size());
  for (const auto& entry : *facts->arrayValue) {
    LatticeFactSummary fact{};
    std::string fact_error{};
    if (!lattice_fill_latest_fact_summary_from_json(&entry, &fact, &fact_error)) {
      if (error) *error = fact_error;
      return false;
    }
    out->push_back(std::move(fact));
  }
  return true;
}

inline bool lattice_load_hashimyei_refs(
    cuwacunu::hero::hashimyei_mcp::app_context_t* app,
    std::vector<lattice_hashimyei_ref_t>* out, std::string* error) {
  if (error) error->clear();
  if (out == nullptr) {
    if (error) *error = "hashimyei ref output pointer is null";
    return false;
  }
  out->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error) *error = "missing hashimyei app context";
    return false;
  }
  if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
          "hero.hashimyei.list", "{\"include_non_hashimyei\":true}", app,
          &result_json, error)) {
    return false;
  }
  lattice_json_value_t structured{};
  if (!lattice_parse_tool_structured_content(result_json, &structured, error)) {
    return false;
  }
  const auto* rows = lattice_json_field(&structured, "hashimyeis");
  if (rows == nullptr || rows->type != lattice_json_type_t::ARRAY ||
      !rows->arrayValue) {
    if (error) *error = "hashimyei list result missing hashimyeis";
    return false;
  }
  out->reserve(rows->arrayValue->size());
  for (const auto& entry : *rows->arrayValue) {
    lattice_hashimyei_ref_t ref{};
    std::string ref_error{};
    if (!lattice_fill_hashimyei_ref_from_json(&entry, &ref, &ref_error)) {
      if (error) *error = ref_error;
      return false;
    }
    out->push_back(std::move(ref));
  }
  return true;
}

inline bool ensure_lattice_defaults_loaded(CmdState& st, std::string* error) {
  if (error) error->clear();
  if (st.lattice.defaults_loaded) return true;

  const std::filesystem::path global_config_path =
      st.lattice.global_config_path.empty()
          ? std::filesystem::path(DEFAULT_GLOBAL_CONFIG_PATH)
          : std::filesystem::path(st.lattice.global_config_path);

  st.lattice.hashimyei_hero_dsl_path =
      cuwacunu::hero::hashimyei_mcp::resolve_hashimyei_hero_dsl_path(
          global_config_path);
  if (st.lattice.hashimyei_hero_dsl_path.empty()) {
    if (error) {
      *error = "missing [REAL_HERO].hashimyei_hero_dsl_filename in " +
               global_config_path.string();
    }
    return false;
  }
  if (!cuwacunu::hero::hashimyei_mcp::load_hashimyei_runtime_defaults(
          st.lattice.hashimyei_hero_dsl_path, global_config_path,
          &st.lattice.hashimyei_defaults, error)) {
    return false;
  }

  st.lattice.lattice_hero_dsl_path =
      cuwacunu::hero::lattice_mcp::resolve_lattice_hero_dsl_path(
          global_config_path);
  if (st.lattice.lattice_hero_dsl_path.empty()) {
    if (error) {
      *error = "missing [REAL_HERO].lattice_hero_dsl_filename in " +
               global_config_path.string();
    }
    return false;
  }
  if (!cuwacunu::hero::lattice_mcp::load_wave_runtime_defaults(
          st.lattice.lattice_hero_dsl_path, global_config_path,
          &st.lattice.lattice_defaults, error)) {
    return false;
  }

  st.lattice.app.store_root = st.lattice.lattice_defaults.store_root;
  st.lattice.app.lattice_catalog_path = st.lattice.lattice_defaults.catalog_path;
  st.lattice.app.hashimyei_catalog_path = st.lattice.hashimyei_defaults.catalog_path;
  st.lattice.app.close_catalog_after_execute = true;
  st.lattice.defaults_loaded = true;
  return true;
}

inline void queue_lattice_selected_member_detail_if_needed(CmdState& st);
inline void queue_lattice_selected_view_if_needed(CmdState& st);

inline bool select_prev_lattice_row(CmdState& st) {
  if (st.lattice.rows.empty() || st.lattice.selected_row == 0) return false;
  --st.lattice.selected_row;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_next_lattice_row(CmdState& st) {
  if (st.lattice.rows.empty() ||
      st.lattice.selected_row + 1 >= st.lattice.rows.size()) {
    return false;
  }
  ++st.lattice.selected_row;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_first_lattice_row(CmdState& st) {
  if (st.lattice.rows.empty() || st.lattice.selected_row == 0) return false;
  st.lattice.selected_row = 0;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_last_lattice_row(CmdState& st) {
  if (st.lattice.rows.empty()) return false;
  const std::size_t last = st.lattice.rows.size() - 1;
  if (st.lattice.selected_row == last) return false;
  st.lattice.selected_row = last;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_lattice_parent_row(CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeRowKind::Member) {
    for (std::size_t i = st.lattice.selected_row; i-- > 0;) {
      const auto& candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Project &&
          candidate.family_index == row->family_index &&
          candidate.project_index == row->project_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeRowKind::Project) {
    for (std::size_t i = st.lattice.selected_row; i-- > 0;) {
      const auto& candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Family &&
          candidate.family_index == row->family_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
  }
  return false;
}

inline bool select_lattice_child_row(CmdState& st) {
  const auto* row = selected_lattice_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeRowKind::Family) {
    for (std::size_t i = st.lattice.selected_row + 1; i < st.lattice.rows.size();
         ++i) {
      const auto& candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Project &&
          candidate.family_index == row->family_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.family_index != row->family_index) break;
    }
    return false;
  }
  if (row->kind == LatticeRowKind::Project) {
    for (std::size_t i = st.lattice.selected_row + 1; i < st.lattice.rows.size();
         ++i) {
      const auto& candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Member &&
          candidate.family_index == row->family_index &&
          candidate.project_index == row->project_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.family_index != row->family_index ||
          candidate.project_index != row->project_index) {
        break;
      }
    }
  }
  return false;
}

inline bool select_prev_lattice_component_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index) break;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
    const auto& candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      break;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_next_lattice_component_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index) break;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.selected_component_row + 1;
       i < st.lattice.component_rows.size(); ++i) {
    const auto& candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      break;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_first_lattice_component_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        if (st.lattice.selected_component_row == i) return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index) continue;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        if (st.lattice.selected_component_row == i) return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
    const auto& candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      continue;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      if (st.lattice.selected_component_row == i) return false;
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_last_lattice_component_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        if (st.lattice.selected_component_row == i) return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index) continue;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        if (st.lattice.selected_component_row == i) return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
    const auto& candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      continue;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      if (st.lattice.selected_component_row == i) return false;
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_lattice_component_parent_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Member) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Category &&
          candidate.root_index == row->root_index &&
          candidate.category_index == row->category_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Root &&
          candidate.root_index == row->root_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
  }
  return false;
}

inline bool select_lattice_component_child_row(CmdState& st) {
  const auto* row = selected_component_row_ptr(st);
  if (!row) return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Category &&
          candidate.root_index == row->root_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.root_index != row->root_index) break;
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto& candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Member &&
          candidate.root_index == row->root_index &&
          candidate.category_index == row->category_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.root_index != row->root_index ||
          candidate.category_index != row->category_index) {
        break;
      }
    }
  }
  return false;
}

inline bool select_prev_lattice_mode_root(CmdState& st) {
  if (st.lattice.selected_mode_row == 0) return false;
  --st.lattice.selected_mode_row;
  return true;
}

inline bool select_next_lattice_mode_root(CmdState& st) {
  if (st.lattice.selected_mode_row >= 1) return false;
  ++st.lattice.selected_mode_row;
  return true;
}

inline bool lattice_enter_selected_mode(CmdState& st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode != LatticeMode::Roots) {
    return false;
  }
  st.lattice.mode = selected_lattice_root_mode(st);
  if (st.lattice.mode == LatticeMode::Facts) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else if (st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  }
  return true;
}

inline bool lattice_exit_mode_to_roots(CmdState& st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode == LatticeMode::Roots) {
    return false;
  }
  st.lattice.selected_mode_row =
      st.lattice.mode == LatticeMode::Views ? 1 : 0;
  st.lattice.mode = LatticeMode::Roots;
  st.lattice.view_requeue = false;
  return true;
}

inline std::size_t lattice_count_active_members(
    const std::vector<LatticeMemberEntry>& members) {
  return static_cast<std::size_t>(std::count_if(
      members.begin(), members.end(), [](const auto& member) {
        return lattice_member_lineage_state_text(member) == "active";
      }));
}

struct lattice_refresh_selection_key_t {
  LatticeRowKind kind{LatticeRowKind::Family};
  std::string family{};
  std::string project{};
  std::string canonical_path{};
};

inline lattice_refresh_selection_key_t capture_lattice_selection_key(
    const CmdState& st) {
  lattice_refresh_selection_key_t key{};
  if (const auto* row = selected_lattice_row_ptr(st); row != nullptr) {
    key.kind = row->kind;
    if (const auto* family = selected_lattice_family_ptr(st); family != nullptr) {
      key.family = family->family;
    }
    if (const auto* project = selected_lattice_project_ptr(st); project != nullptr) {
      key.project = project->project_path;
    }
    if (const auto* member = selected_lattice_member_ptr(st); member != nullptr) {
      key.canonical_path = member->canonical_path;
    }
  }
  return key;
}

inline bool lattice_has_cached_snapshot(const CmdState& st) {
  return !st.lattice.families.empty() || !st.lattice.component_rows.empty() ||
         !st.lattice.views.empty();
}

inline std::string lattice_loading_status_text(LatticeRefreshMode mode) {
  if (mode == LatticeRefreshMode::SyncStore) {
    return "Refreshing hero.lattice and hero.hashimyei catalogs; full reingest can take about a minute...";
  }
  return "Loading lattice components, facts, and views...";
}

inline const char* lattice_refresh_mode_name(LatticeRefreshMode mode) {
  return mode == LatticeRefreshMode::SyncStore ? "sync_store" : "snapshot";
}

inline bool lattice_is_refresh_pending(const CmdState& st) {
  return st.lattice.refresh_pending;
}

inline bool lattice_is_visibly_loading(const CmdState& st) {
  return st.lattice.refresh_pending && !st.lattice.refresh_timed_out;
}

inline void queue_lattice_refresh(
    CmdState& st, LatticeRefreshMode mode, std::string status = {});

inline bool lattice_handle_refresh_failure(CmdState& st, std::string_view error) {
  if (lattice_has_cached_snapshot(st)) {
    set_lattice_status(st,
                       "Lattice refresh skipped; showing cached lineage snapshot: " +
                           std::string(error),
                       false);
    return true;
  }
  st.lattice.families.clear();
  st.lattice.component_roots.clear();
  st.lattice.rows.clear();
  st.lattice.component_rows.clear();
  st.lattice.selected_row = 0;
  st.lattice.selected_component_row = 0;
  set_lattice_status(st, std::string(error), true);
  return false;
}

inline void lattice_ensure_member_identity(LatticeMemberEntry* member);
inline bool poll_lattice_async_updates(CmdState& st);
inline void queue_lattice_selected_view_if_needed(CmdState& st);

inline void queue_lattice_selected_member_detail_if_needed(CmdState& st) {
  if (st.lattice.selected_section == LatticeSection::Overview ||
      (st.lattice.selected_section == LatticeSection::Lattice &&
       st.lattice.mode != LatticeMode::Facts)) {
    st.lattice.detail_requeue = false;
    return;
  }
  const auto* member = selected_lattice_detail_member_ptr(st);
  if (member == nullptr || member->detail_loaded) {
    st.lattice.detail_requeue = false;
    return;
  }
  if (st.lattice.detail_refresh_pending) {
    st.lattice.detail_requeue = true;
    return;
  }
  st.lattice.detail_refresh_pending = true;
  st.lattice.detail_requeue = false;
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  const auto member_copy = *member;
  st.lattice.detail_future = std::async(
      std::launch::async,
      [lattice_defaults, hashimyei_defaults,
       member_copy]() mutable -> LatticeMemberDetailSnapshot {
        LatticeMemberDetailSnapshot snapshot{};
        snapshot.member = member_copy;
        cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
        lattice_app.store_root = lattice_defaults.store_root;
        lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
        lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
        lattice_app.close_catalog_after_execute = true;

        cuwacunu::hero::hashimyei_mcp::app_context_t hashimyei_app{};
        hashimyei_app.store_root = hashimyei_defaults.store_root;
        hashimyei_app.lattice_catalog_path = lattice_defaults.catalog_path;
        hashimyei_app.catalog_options.catalog_path = hashimyei_defaults.catalog_path;
        hashimyei_app.catalog_options.encrypted = false;
        hashimyei_app.close_catalog_after_execute = true;

        bool component_ok = false;
        bool facts_ok = false;
        bool fact_bundle_ok = false;
        std::string first_error{};

        if (!member_copy.canonical_path.empty()) {
          std::string component_result_json{};
          std::string component_error{};
          const std::string component_args =
              std::string("{\"canonical_path\":") +
              config_json_quote(member_copy.canonical_path) + "}";
          if (cuwacunu::hero::hashimyei_mcp::execute_tool_json(
                  "hero.hashimyei.get_component_manifest", component_args,
                  &hashimyei_app, &component_result_json, &component_error)) {
            lattice_json_value_t structured{};
            if (lattice_parse_tool_structured_content(component_result_json, &structured,
                                                     &component_error)) {
              snapshot.member.has_component = true;
              snapshot.member.component_count =
                  std::max<std::size_t>(snapshot.member.component_count, 1);
              snapshot.member.hashimyei =
                  lattice_json_string(lattice_json_field(&structured, "hashimyei"),
                                      snapshot.member.hashimyei);
              snapshot.member.canonical_path = lattice_json_string(
                  lattice_json_field(&structured, "canonical_path"),
                  snapshot.member.canonical_path);
              if (const auto* component =
                      lattice_json_field(&structured, "component");
                  component != nullptr) {
                snapshot.member.component.component_id = lattice_json_string(
                    lattice_json_field(component, "component_id"));
                snapshot.member.component.ts_ms =
                    lattice_json_u64(lattice_json_field(component, "ts_ms"));
                snapshot.member.component.manifest_path = lattice_json_string(
                    lattice_json_field(component, "manifest_path"));
                snapshot.member.component.family_rank = lattice_json_optional_u64(
                    lattice_json_field(component, "family_rank"));
                if (const auto* manifest =
                        lattice_json_field(component, "manifest");
                    manifest != nullptr) {
                  snapshot.member.family = lattice_json_string(
                      lattice_json_field(manifest, "family"),
                      snapshot.member.family);
                  snapshot.member.lineage_state = lattice_json_string(
                      lattice_json_field(manifest, "lineage_state"),
                      snapshot.member.lineage_state);
                  snapshot.member.component.manifest.canonical_path =
                      lattice_json_string(lattice_json_field(manifest,
                                                            "canonical_path"));
                  snapshot.member.component.manifest.family = lattice_json_string(
                      lattice_json_field(manifest, "family"));
                  snapshot.member.component.manifest.lineage_state =
                      lattice_json_string(lattice_json_field(manifest,
                                                            "lineage_state"));
                  snapshot.member.component.manifest.replaced_by =
                      lattice_json_string(lattice_json_field(manifest,
                                                            "replaced_by"));
                  if (const auto* hashimyei_identity =
                          lattice_json_field(manifest, "hashimyei_identity");
                      hashimyei_identity != nullptr) {
                    snapshot.member.component.manifest.hashimyei_identity.name =
                        lattice_json_string(
                            lattice_json_field(hashimyei_identity, "name"));
                  }
                  if (const auto* contract_identity =
                          lattice_json_field(manifest, "contract_identity");
                      contract_identity != nullptr) {
                    snapshot.member.component.manifest.contract_identity.name =
                        lattice_json_string(
                            lattice_json_field(contract_identity, "name"));
                    snapshot.member.contract_hash =
                        snapshot.member.component.manifest.contract_identity.name;
                  }
                  if (const auto* parent_identity =
                          lattice_json_field(manifest, "parent_identity");
                      parent_identity != nullptr &&
                      parent_identity->type == lattice_json_type_t::OBJECT) {
                    snapshot.member.component.manifest.parent_identity =
                        cuwacunu::hashimyei::hashimyei_t{};
                    snapshot.member.component.manifest.parent_identity->name =
                        lattice_json_string(
                            lattice_json_field(parent_identity, "name"));
                  }
                }
              }
              component_ok = true;
            }
          }
          if (!component_ok && first_error.empty() && !component_error.empty()) {
            first_error = component_error;
          }

          lattice_json_value_t facts_structured{};
          std::string facts_error{};
          const std::string facts_args =
              std::string("{\"canonical_path\":") +
              config_json_quote(member_copy.canonical_path) + "}";
          if (lattice_invoke_tool(&lattice_app, "hero.lattice.list_facts",
                                  facts_args, &facts_structured, &facts_error)) {
            if (const auto* facts = lattice_json_field(&facts_structured, "facts");
                facts != nullptr && facts->type == lattice_json_type_t::ARRAY &&
                facts->arrayValue) {
              snapshot.member.fact.wave_cursors.clear();
              snapshot.member.fact.binding_ids.clear();
              snapshot.member.fact.semantic_taxa.clear();
              snapshot.member.fact.source_runtime_cursors.clear();
              snapshot.member.fact.available_context_count = facts->arrayValue->size();
              std::uint64_t latest_ts_ms = snapshot.member.fact.latest_ts_ms;
              std::string latest_wave_cursor = snapshot.member.fact.latest_wave_cursor;
              std::size_t latest_fragment_count = snapshot.member.fact.fragment_count;
              for (const auto& entry : *facts->arrayValue) {
                const std::string wave_cursor =
                    lattice_json_string(lattice_json_field(&entry, "wave_cursor"));
                const std::uint64_t latest_ts =
                    lattice_json_u64(lattice_json_field(&entry, "latest_ts_ms"));
                lattice_append_unique_value(&snapshot.member.fact.wave_cursors,
                                            wave_cursor);
                lattice_merge_string_values(
                    &snapshot.member.fact.binding_ids,
                    lattice_json_string_array(lattice_json_field(&entry,
                                                                 "binding_ids")));
                lattice_merge_string_values(
                    &snapshot.member.fact.semantic_taxa,
                    lattice_json_string_array(lattice_json_field(&entry,
                                                                 "semantic_taxa")));
                lattice_merge_string_values(
                    &snapshot.member.fact.source_runtime_cursors,
                    lattice_json_string_array(lattice_json_field(
                        &entry, "source_runtime_cursors")));
                if (latest_ts >= latest_ts_ms) {
                  latest_ts_ms = latest_ts;
                  latest_wave_cursor = wave_cursor;
                  latest_fragment_count =
                      lattice_json_u64(lattice_json_field(&entry, "fragment_count"),
                                       latest_fragment_count);
                }
              }
              snapshot.member.has_fact =
                  snapshot.member.has_fact || !facts->arrayValue->empty();
              snapshot.member.fact.latest_ts_ms = latest_ts_ms;
              snapshot.member.fact.latest_wave_cursor = latest_wave_cursor;
              snapshot.member.fact.fragment_count = latest_fragment_count;
              snapshot.member.report_fragment_count =
                  std::max(snapshot.member.report_fragment_count,
                           latest_fragment_count);
              facts_ok = true;
            }
          }
          if (!facts_ok && first_error.empty() && !facts_error.empty()) {
            first_error = facts_error;
          }

          if (snapshot.member.has_fact) {
            lattice_json_value_t fact_structured{};
            std::string fact_error{};
            if (lattice_invoke_tool(&lattice_app, "hero.lattice.get_fact",
                                    facts_args, &fact_structured, &fact_error)) {
              snapshot.member.latest_fact_lls =
                  lattice_json_string(lattice_json_field(&fact_structured,
                                                         "fact_lls"));
              snapshot.member.fact.latest_wave_cursor = lattice_json_string(
                  lattice_json_field(&fact_structured, "wave_cursor"),
                  snapshot.member.fact.latest_wave_cursor);
              snapshot.member.fact.latest_ts_ms = lattice_json_u64(
                  lattice_json_field(&fact_structured, "latest_ts_ms"),
                  snapshot.member.fact.latest_ts_ms);
              snapshot.member.fact.fragment_count = lattice_json_u64(
                  lattice_json_field(&fact_structured, "fragment_count"),
                  snapshot.member.fact.fragment_count);
              lattice_merge_string_values(
                  &snapshot.member.fact.binding_ids,
                  lattice_json_string_array(lattice_json_field(&fact_structured,
                                                               "binding_ids")));
              lattice_merge_string_values(
                  &snapshot.member.fact.semantic_taxa,
                  lattice_json_string_array(lattice_json_field(&fact_structured,
                                                               "semantic_taxa")));
              lattice_merge_string_values(
                  &snapshot.member.fact.source_runtime_cursors,
                  lattice_json_string_array(lattice_json_field(
                      &fact_structured, "source_runtime_cursors")));
              fact_bundle_ok = true;
            }
            if (!fact_bundle_ok && first_error.empty() && !fact_error.empty()) {
              first_error = fact_error;
            }
          }
        }

        lattice_ensure_member_identity(&snapshot.member);
        snapshot.member.detail_loaded = true;
        snapshot.ok = component_ok || facts_ok || fact_bundle_ok ||
                      (!snapshot.member.canonical_path.empty());
        if (!snapshot.ok && !first_error.empty()) {
          snapshot.error = std::move(first_error);
        }
        return snapshot;
      });
}

inline LatticeMemberEntry* lattice_find_member_by_canonical_path_mutable(
    CmdState& st, std::string_view canonical_path) {
  for (auto& family : st.lattice.families) {
    for (auto& project : family.projects) {
      for (auto& member : project.members) {
        if (member.canonical_path == canonical_path) return &member;
      }
    }
  }
  return nullptr;
}

inline bool lattice_apply_member_detail_snapshot(
    CmdState& st, const LatticeMemberDetailSnapshot& snapshot) {
  if (!snapshot.ok) {
    set_lattice_status(
        st,
        "Lattice member detail unavailable; showing summary view: " +
            snapshot.error,
        false);
    return false;
  }
  auto* member = lattice_find_member_by_canonical_path_mutable(
      st, snapshot.member.canonical_path);
  if (member == nullptr) return false;
  *member = snapshot.member;
  return true;
}

inline bool lattice_apply_view_transport_snapshot(
    CmdState& st, const LatticeViewTransportSnapshot& snapshot) {
  st.lattice.view_snapshot = snapshot;
  return true;
}

inline void queue_lattice_selected_view_if_needed(CmdState& st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode != LatticeMode::Views) {
    st.lattice.view_requeue = false;
    return;
  }
  const auto* view = selected_lattice_view_ptr(st);
  const auto* member = selected_lattice_member_ptr(st);
  if (view == nullptr) {
    st.lattice.view_requeue = false;
    st.lattice.view_snapshot = LatticeViewTransportSnapshot{};
    return;
  }
  if (member == nullptr) {
    st.lattice.view_requeue = false;
    st.lattice.view_snapshot = LatticeViewTransportSnapshot{
        .ok = false,
        .error = "Select a fact member in Lattice/Facts to materialize this view.",
        .view_kind = view->view_kind,
    };
    return;
  }
  if (!member->detail_loaded) {
    queue_lattice_selected_member_detail_if_needed(st);
    if (st.lattice.detail_refresh_pending) {
      st.lattice.view_requeue = true;
      return;
    }
  }
  if (st.lattice.view_refresh_pending) {
    st.lattice.view_requeue = true;
    return;
  }

  const std::string canonical_path = member->canonical_path;
  const std::string contract_hash = member->contract_hash;
  const std::string wave_cursor = member->fact.latest_wave_cursor;
  if (st.lattice.view_snapshot.view_kind == view->view_kind &&
      st.lattice.view_snapshot.canonical_path == canonical_path &&
      st.lattice.view_snapshot.contract_hash == contract_hash &&
      st.lattice.view_snapshot.wave_cursor == wave_cursor &&
      (st.lattice.view_snapshot.ok || !st.lattice.view_snapshot.error.empty())) {
    st.lattice.view_requeue = false;
    return;
  }

  st.lattice.view_refresh_pending = true;
  st.lattice.view_requeue = false;
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  const auto view_copy = *view;
  const auto member_copy = *member;
  st.lattice.view_future = std::async(
      std::launch::async,
      [lattice_defaults, hashimyei_defaults, view_copy,
       member_copy]() mutable -> LatticeViewTransportSnapshot {
        LatticeViewTransportSnapshot snapshot{};
        snapshot.view_kind = view_copy.view_kind;
        snapshot.canonical_path = member_copy.canonical_path;
        snapshot.contract_hash = member_copy.contract_hash;
        snapshot.wave_cursor = member_copy.fact.latest_wave_cursor;

        if (view_copy.view_kind == "family_evaluation_report") {
          if (snapshot.canonical_path.empty() || snapshot.contract_hash.empty()) {
            snapshot.error =
                "family_evaluation_report needs canonical_path + contract_hash";
            return snapshot;
          }
        } else if (view_copy.view_kind == "entropic_capacity_comparison") {
          if (snapshot.wave_cursor.empty()) {
            snapshot.error = "entropic_capacity_comparison needs wave_cursor";
            return snapshot;
          }
        }

        cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
        lattice_app.store_root = lattice_defaults.store_root;
        lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
        lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
        lattice_app.close_catalog_after_execute = true;

        std::ostringstream args;
        args << "{\"view_kind\":" << config_json_quote(view_copy.view_kind);
        if (!snapshot.canonical_path.empty()) {
          args << ",\"canonical_path\":"
               << config_json_quote(snapshot.canonical_path);
        }
        if (!snapshot.contract_hash.empty()) {
          args << ",\"contract_hash\":"
               << config_json_quote(snapshot.contract_hash);
        }
        if (!snapshot.wave_cursor.empty()) {
          args << ",\"wave_cursor\":"
               << config_json_quote(snapshot.wave_cursor);
        }
        args << "}";

        lattice_json_value_t structured{};
        std::string error{};
        if (!lattice_invoke_tool(&lattice_app, "hero.lattice.get_view",
                                 args.str(), &structured, &error)) {
          snapshot.error = std::move(error);
          return snapshot;
        }
        snapshot.canonical_path = lattice_json_string(
            lattice_json_field(&structured, "canonical_path"),
            snapshot.canonical_path);
        snapshot.contract_hash = lattice_json_string(
            lattice_json_field(&structured, "contract_hash"),
            snapshot.contract_hash);
        snapshot.wave_cursor = lattice_json_string(
            lattice_json_field(&structured, "wave_cursor"), snapshot.wave_cursor);
        snapshot.match_count =
            lattice_json_u64(lattice_json_field(&structured, "match_count"));
        snapshot.ambiguity_count =
            lattice_json_u64(lattice_json_field(&structured, "ambiguity_count"));
        snapshot.view_lls =
            lattice_json_string(lattice_json_field(&structured, "view_lls"));
        snapshot.ok = true;
        return snapshot;
      });
}

inline bool lattice_set_section(CmdState& st, LatticeSection section) {
  if (st.lattice.selected_section == section) return false;
  st.lattice.selected_section = section;
  if (section == LatticeSection::Lattice) {
    st.lattice.mode = LatticeMode::Roots;
  }
  clamp_lattice_state(st);
  if (section == LatticeSection::Components) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else if (section == LatticeSection::Lattice &&
             st.lattice.mode == LatticeMode::Facts) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else {
    st.lattice.detail_requeue = false;
  }
  if (section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  } else {
    st.lattice.view_requeue = false;
  }
  return true;
}

inline bool lattice_select_next_section(CmdState& st) {
  switch (st.lattice.selected_section) {
    case LatticeSection::Overview:
      return lattice_set_section(st, LatticeSection::Components);
    case LatticeSection::Components:
      return lattice_set_section(st, LatticeSection::Lattice);
    case LatticeSection::Lattice:
      return lattice_set_section(st, LatticeSection::Overview);
  }
  return false;
}

inline bool lattice_select_prev_section(CmdState& st) {
  switch (st.lattice.selected_section) {
    case LatticeSection::Overview:
      return lattice_set_section(st, LatticeSection::Lattice);
    case LatticeSection::Components:
      return lattice_set_section(st, LatticeSection::Overview);
    case LatticeSection::Lattice:
      return lattice_set_section(st, LatticeSection::Components);
  }
  return false;
}

inline bool lattice_select_prev_view(CmdState& st) {
  if (st.lattice.views.empty() || st.lattice.selected_view_index == 0) return false;
  --st.lattice.selected_view_index;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_next_view(CmdState& st) {
  if (st.lattice.views.empty() ||
      st.lattice.selected_view_index + 1 >= st.lattice.views.size()) {
    return false;
  }
  ++st.lattice.selected_view_index;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_first_view(CmdState& st) {
  if (st.lattice.views.empty() || st.lattice.selected_view_index == 0) return false;
  st.lattice.selected_view_index = 0;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_last_view(CmdState& st) {
  if (st.lattice.views.empty()) return false;
  const std::size_t last = st.lattice.views.size() - 1;
  if (st.lattice.selected_view_index == last) return false;
  st.lattice.selected_view_index = last;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline void lattice_ensure_member_identity(LatticeMemberEntry* member) {
  if (member == nullptr || member->canonical_path.empty()) return;
  if (member->project_path.empty()) {
    member->project_path = lattice_project_path_for_canonical(member->canonical_path);
  }
  if (member->family.empty()) {
    member->family = lattice_family_from_canonical(member->canonical_path);
  }
  if (member->hashimyei.empty()) {
    std::string hashimyei{};
    if (lattice_has_hashimyei_suffix(member->canonical_path, &hashimyei)) {
      member->hashimyei = std::move(hashimyei);
    }
  }
  if (member->display_name.empty()) {
    member->display_name =
        lattice_display_name_for_member(member->canonical_path, member->hashimyei);
  }
}

inline LatticeFamilyGroup* lattice_ensure_family_group(
    std::vector<LatticeFamilyGroup>* families, std::string_view family_name) {
  if (families == nullptr) return nullptr;
  const std::string family = trim_copy(std::string(family_name));
  for (auto& entry : *families) {
    if (entry.family == family) return &entry;
  }
  families->push_back(LatticeFamilyGroup{.family = family});
  return &families->back();
}

inline LatticeProjectGroup* lattice_ensure_project_group(
    LatticeFamilyGroup* family, std::string_view project_path) {
  if (family == nullptr) return nullptr;
  const std::string project = trim_copy(std::string(project_path));
  for (auto& entry : family->projects) {
    if (entry.project_path == project) return &entry;
  }
  family->projects.push_back(
      LatticeProjectGroup{.project_path = project, .family = family->family});
  return &family->projects.back();
}

inline LatticeMemberEntry* lattice_ensure_member_entry(
    std::vector<LatticeFamilyGroup>* families, std::string_view canonical_path) {
  if (families == nullptr) return nullptr;
  const std::string canonical = normalize_lattice_canonical_path(canonical_path);
  const std::string family_name = lattice_family_from_canonical(canonical);
  const std::string project_path = lattice_project_path_for_canonical(canonical);
  auto* family = lattice_ensure_family_group(families, family_name);
  auto* project = lattice_ensure_project_group(family, project_path);
  if (project == nullptr) return nullptr;
  for (auto& member : project->members) {
    if (member.canonical_path == canonical) return &member;
  }
  project->members.push_back(LatticeMemberEntry{
      .canonical_path = canonical,
      .project_path = project_path,
      .family = family_name,
  });
  lattice_ensure_member_identity(&project->members.back());
  return &project->members.back();
}

inline void lattice_apply_hashimyei_ref_to_member(
    LatticeMemberEntry* member, const lattice_hashimyei_ref_t& ref) {
  if (member == nullptr) return;
  if (member->canonical_path.empty()) member->canonical_path = ref.canonical_path;
  if (member->hashimyei.empty()) member->hashimyei = ref.hashimyei;
  member->in_hashimyei_catalog = true;
  member->component_count = std::max(member->component_count, ref.component_count);
  member->report_fragment_count =
      std::max(member->report_fragment_count, ref.report_fragment_count);
  lattice_ensure_member_identity(member);
}

inline void lattice_apply_latest_fact_summary_to_member(
    LatticeMemberEntry* member, const LatticeFactSummary& fact) {
  if (member == nullptr) return;
  member->has_fact = true;
  member->fact = fact;
  if (member->canonical_path.empty()) member->canonical_path = fact.canonical_path;
  member->report_fragment_count =
      std::max(member->report_fragment_count, fact.fragment_count);
  lattice_ensure_member_identity(member);
}

inline void lattice_merge_member_entry(LatticeMemberEntry* dst,
                                       const LatticeMemberEntry& src) {
  if (dst == nullptr) return;
  if (dst->canonical_path.empty()) dst->canonical_path = src.canonical_path;
  if (dst->project_path.empty()) dst->project_path = src.project_path;
  if (dst->family.empty()) dst->family = src.family;
  if (dst->display_name.empty()) dst->display_name = src.display_name;
  if (dst->hashimyei.empty()) dst->hashimyei = src.hashimyei;
  if (dst->contract_hash.empty()) dst->contract_hash = src.contract_hash;
  if (dst->lineage_state.empty() ||
      dst->lineage_state == "unknown" || dst->lineage_state == "fact-only") {
    if (!src.lineage_state.empty()) dst->lineage_state = src.lineage_state;
  }
  dst->in_hashimyei_catalog =
      dst->in_hashimyei_catalog || src.in_hashimyei_catalog;
  const bool had_component = dst->has_component;
  dst->has_component = had_component || src.has_component;
  if (src.has_component &&
      (!had_component || src.component.ts_ms >= dst->component.ts_ms)) {
    dst->component = src.component;
  }
  dst->has_fact = dst->has_fact || src.has_fact;
  if (src.fact.fragment_count > dst->fact.fragment_count) {
    dst->fact.fragment_count = src.fact.fragment_count;
  }
  if (src.fact.available_context_count > dst->fact.available_context_count) {
    dst->fact.available_context_count = src.fact.available_context_count;
  }
  if (src.fact.latest_ts_ms >= dst->fact.latest_ts_ms) {
    if (!src.fact.latest_wave_cursor.empty()) {
      dst->fact.latest_wave_cursor = src.fact.latest_wave_cursor;
    }
    dst->fact.latest_ts_ms = src.fact.latest_ts_ms;
  }
  lattice_merge_string_values(&dst->fact.wave_cursors, src.fact.wave_cursors);
  lattice_merge_string_values(&dst->fact.binding_ids, src.fact.binding_ids);
  lattice_merge_string_values(&dst->fact.semantic_taxa, src.fact.semantic_taxa);
  lattice_merge_string_values(&dst->fact.source_runtime_cursors,
                              src.fact.source_runtime_cursors);
  if (src.family_rank.has_value() &&
      (!dst->family_rank.has_value() || *src.family_rank < *dst->family_rank)) {
    dst->family_rank = src.family_rank;
  }
  dst->detail_loaded = dst->detail_loaded || src.detail_loaded;
  lattice_ensure_member_identity(dst);
}

inline void lattice_recompute_rollups(
    std::vector<LatticeFamilyGroup>* families) {
  if (families == nullptr) return;
  for (auto& family : *families) {
    family.member_count = 0;
    family.active_member_count = 0;
    family.fragment_count = 0;
    family.available_context_count = 0;
    family.latest_ts_ms = 0;
    family.wave_cursors.clear();
    family.binding_ids.clear();
    family.semantic_taxa.clear();
    family.contract_hashes.clear();

    for (auto& project : family.projects) {
      std::sort(project.members.begin(), project.members.end(),
                [](const auto& a, const auto& b) {
                  return a.canonical_path < b.canonical_path;
                });
      project.fragment_count = 0;
      project.available_context_count = 0;
      project.latest_ts_ms = 0;
      project.wave_cursors.clear();
      project.binding_ids.clear();
      project.semantic_taxa.clear();
      project.source_runtime_cursors.clear();
      project.contract_hashes.clear();

      for (const auto& member : project.members) {
        ++family.member_count;
        if (lattice_member_lineage_state_text(member) == "active") {
          ++family.active_member_count;
        }
        project.fragment_count += member.fact.fragment_count;
        project.available_context_count += member.fact.available_context_count;
        project.latest_ts_ms =
            std::max(project.latest_ts_ms, lattice_member_latest_ts_ms(member));
        lattice_merge_string_values(&project.wave_cursors, member.fact.wave_cursors);
        lattice_merge_string_values(&project.binding_ids, member.fact.binding_ids);
        lattice_merge_string_values(&project.semantic_taxa, member.fact.semantic_taxa);
        lattice_merge_string_values(&project.source_runtime_cursors,
                                    member.fact.source_runtime_cursors);
        lattice_append_unique_value(&project.contract_hashes,
                                    lattice_contract_hash_for_member(member));
      }

      family.fragment_count += project.fragment_count;
      family.available_context_count += project.available_context_count;
      family.latest_ts_ms = std::max(family.latest_ts_ms, project.latest_ts_ms);
      lattice_merge_string_values(&family.wave_cursors, project.wave_cursors);
      lattice_merge_string_values(&family.binding_ids, project.binding_ids);
      lattice_merge_string_values(&family.semantic_taxa, project.semantic_taxa);
      lattice_merge_string_values(&family.contract_hashes, project.contract_hashes);
    }

    std::sort(family.projects.begin(), family.projects.end(),
              [](const auto& a, const auto& b) {
                return a.project_path < b.project_path;
              });
  }
  std::sort(families->begin(), families->end(), [](const auto& a, const auto& b) {
    return a.family < b.family;
  });
}

inline void lattice_merge_family_snapshots(
    std::vector<LatticeFamilyGroup>* base,
    const std::vector<LatticeFamilyGroup>& overlay) {
  if (base == nullptr) return;
  for (const auto& overlay_family : overlay) {
    for (const auto& overlay_project : overlay_family.projects) {
      for (const auto& overlay_member : overlay_project.members) {
        auto* member = lattice_ensure_member_entry(base, overlay_member.canonical_path);
        if (member == nullptr) continue;
        lattice_merge_member_entry(member, overlay_member);
      }
    }
  }
  lattice_recompute_rollups(base);
}

inline bool lattice_restore_selection(
    CmdState& st, const lattice_refresh_selection_key_t& previous_selection) {
  if (st.lattice.rows.empty()) return false;
  for (std::size_t i = 0; i < st.lattice.rows.size(); ++i) {
    const auto& row = st.lattice.rows[i];
    if (previous_selection.kind != row.kind) continue;
    const auto& family = st.lattice.families[row.family_index];
    if (row.kind == LatticeRowKind::Family &&
        family.family == previous_selection.family) {
      st.lattice.selected_row = i;
      return true;
    }
    if (row.kind == LatticeRowKind::Project) {
      const auto& project = family.projects[row.project_index];
      if (project.project_path == previous_selection.project) {
        st.lattice.selected_row = i;
        return true;
      }
    }
    if (row.kind == LatticeRowKind::Member) {
      const auto& project = family.projects[row.project_index];
      const auto& member = project.members[row.member_index];
      if (member.canonical_path == previous_selection.canonical_path) {
        st.lattice.selected_row = i;
        return true;
      }
    }
  }
  return false;
}

inline LatticeRefreshSnapshot lattice_collect_refresh_snapshot(
    const cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t& lattice_defaults,
    const cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t&
        hashimyei_defaults,
    LatticeRefreshMode mode) {
  LatticeRefreshSnapshot snapshot{};
  std::string error{};
  cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
  lattice_app.store_root = lattice_defaults.store_root;
  lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
  lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
  lattice_app.close_catalog_after_execute = true;

  cuwacunu::hero::hashimyei_mcp::app_context_t hashimyei_app{};
  hashimyei_app.store_root = hashimyei_defaults.store_root;
  hashimyei_app.lattice_catalog_path = lattice_defaults.catalog_path;
  hashimyei_app.catalog_options.catalog_path = hashimyei_defaults.catalog_path;
  hashimyei_app.catalog_options.encrypted = false;
  hashimyei_app.close_catalog_after_execute = true;

  if (mode == LatticeRefreshMode::SyncStore) {
    std::string hashimyei_result{};
    if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
            "hero.hashimyei.reset_catalog", "{\"reingest\":true}",
            &hashimyei_app, &hashimyei_result, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
    lattice_json_value_t hashimyei_structured{};
    if (!lattice_parse_tool_structured_content(hashimyei_result,
                                               &hashimyei_structured, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
    lattice_json_value_t ignored{};
    if (!lattice_invoke_tool(&lattice_app, "hero.lattice.refresh",
                             "{\"reingest\":true}", &ignored, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
  }

  std::string views_error{};
  const bool views_ok =
      lattice_load_view_definitions(&lattice_app, &snapshot.views, &views_error);
  (void)views_ok;

  std::vector<lattice_hashimyei_ref_t> hashimyei_refs{};
  std::string hashimyei_error{};
  const bool hashimyei_ok =
      lattice_load_hashimyei_refs(&hashimyei_app, &hashimyei_refs, &hashimyei_error);

  std::vector<LatticeFactSummary> facts{};
  std::string facts_error{};
  const bool facts_ok =
      lattice_load_latest_fact_summaries(&lattice_app, &facts, &facts_error);

  if (!hashimyei_ok && !facts_ok) {
    snapshot.error = !facts_error.empty()
                         ? facts_error
                         : (!hashimyei_error.empty()
                                ? hashimyei_error
                                : std::string("lattice refresh failed"));
    return snapshot;
  }

  snapshot.discovered_component_count = 0;
  snapshot.discovered_report_manifest_count = 0;
  for (const auto& ref : hashimyei_refs) {
    snapshot.discovered_component_count += ref.component_count;
    snapshot.discovered_report_manifest_count += ref.report_fragment_count;
    auto* member = lattice_ensure_member_entry(&snapshot.families, ref.canonical_path);
    if (member == nullptr) continue;
    lattice_apply_hashimyei_ref_to_member(member, ref);
  }
  for (const auto& fact : facts) {
    auto* member = lattice_ensure_member_entry(&snapshot.families, fact.canonical_path);
    if (member == nullptr) continue;
    lattice_apply_latest_fact_summary_to_member(member, fact);
  }

  if (facts.empty() && !hashimyei_refs.empty()) {
    snapshot.used_hashimyei_fallback = true;
  } else if (!facts.empty() && !hashimyei_refs.empty()) {
    snapshot.used_hashimyei_enrichment = true;
  }
  lattice_recompute_rollups(&snapshot.families);
  snapshot.ok = true;
  return snapshot;
}

inline bool lattice_apply_refresh_snapshot(
    CmdState& st, LatticeRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    return lattice_handle_refresh_failure(st, snapshot.error);
  }

  const lattice_refresh_selection_key_t previous_selection =
      capture_lattice_selection_key(st);
  st.lattice.families = std::move(snapshot.families);
  st.lattice.views = std::move(snapshot.views);
  st.lattice.used_hashimyei_fallback = snapshot.used_hashimyei_fallback;
  st.lattice.used_hashimyei_enrichment = snapshot.used_hashimyei_enrichment;
  st.lattice.discovered_component_count = snapshot.discovered_component_count;
  st.lattice.discovered_report_manifest_count =
      snapshot.discovered_report_manifest_count;
  st.lattice.view_snapshot = LatticeViewTransportSnapshot{};
  rebuild_lattice_visible_rows(st);
  rebuild_lattice_component_rows(st);
  if (!lattice_restore_selection(st, previous_selection)) {
    clamp_lattice_state(st);
  }
  if (st.lattice.selected_section == LatticeSection::Components ||
      (st.lattice.selected_section == LatticeSection::Lattice &&
       st.lattice.mode == LatticeMode::Facts)) {
    queue_lattice_selected_member_detail_if_needed(st);
  }
  if (st.lattice.selected_section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  }

  st.lattice.ok = true;
  st.lattice.error.clear();
  if (st.lattice.rows.empty()) {
    if (st.lattice.used_hashimyei_fallback ||
        st.lattice.used_hashimyei_enrichment) {
      set_lattice_status(
          st, "Showing Hashimyei components; no lattice facts are available yet.",
          false);
    } else {
      set_lattice_status(
          st, "Lattice is ready but no components or lattice facts exist yet.",
          false);
    }
  } else {
    if (st.lattice.used_hashimyei_fallback) {
      set_lattice_status(
          st, "Showing Hashimyei components while lattice facts are still empty.",
          false);
    } else if (st.lattice.used_hashimyei_enrichment) {
      set_lattice_status(
          st,
          "Showing lattice facts with Hashimyei identity enrichment for uncovered rows.",
          false);
    } else {
      set_lattice_status(st, {}, false);
    }
  }
  log_dbg(
      "[iinuji:lattice] refresh_apply mode=%s families=%zu rows=%zu views=%zu fallback=%d enrichment=%d\n",
      lattice_refresh_mode_name(st.lattice.refresh_mode),
      st.lattice.families.size(), st.lattice.rows.size(), st.lattice.views.size(),
      st.lattice.used_hashimyei_fallback ? 1 : 0,
      st.lattice.used_hashimyei_enrichment ? 1 : 0);
  return true;
}

inline bool refresh_lattice_state(
    CmdState& st,
    LatticeRefreshMode mode = LatticeRefreshMode::Snapshot) {
  log_dbg("[iinuji:lattice] refresh_immediate_begin mode=%s\n",
          lattice_refresh_mode_name(mode));
  st.lattice.refresh_pending = false;
  st.lattice.refresh_started_at_ms = 0;
  st.lattice.refresh_timed_out = false;
  st.lattice.detail_refresh_pending = false;
  st.lattice.view_refresh_pending = false;
  st.lattice.refresh_requeue = false;
  st.lattice.detail_requeue = false;
  st.lattice.view_requeue = false;
  st.lattice.refresh_mode = LatticeRefreshMode::Snapshot;

  std::string error{};
  if (!ensure_lattice_defaults_loaded(st, &error)) {
    st.lattice.families.clear();
    st.lattice.component_roots.clear();
    st.lattice.rows.clear();
    st.lattice.component_rows.clear();
    st.lattice.selected_row = 0;
    st.lattice.selected_component_row = 0;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    set_lattice_status(st, error, true);
    log_warn("[iinuji:lattice] refresh_immediate_defaults_failed mode=%s error=%s\n",
             lattice_refresh_mode_name(mode), error.c_str());
    return false;
  }
  const bool ok = lattice_apply_refresh_snapshot(
      st, lattice_collect_refresh_snapshot(st.lattice.lattice_defaults,
                                           st.lattice.hashimyei_defaults, mode));
  log_dbg("[iinuji:lattice] refresh_immediate_finish mode=%s ok=%d rows=%zu views=%zu\n",
          lattice_refresh_mode_name(mode), ok ? 1 : 0,
          st.lattice.rows.size(), st.lattice.views.size());
  return ok;
}

inline void queue_lattice_refresh(
    CmdState& st, LatticeRefreshMode mode,
    std::string status) {
  std::string error{};
  if (!ensure_lattice_defaults_loaded(st, &error)) {
    st.lattice.families.clear();
    st.lattice.component_roots.clear();
    st.lattice.rows.clear();
    st.lattice.component_rows.clear();
    st.lattice.selected_row = 0;
    st.lattice.selected_component_row = 0;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    set_lattice_status(st, error, true);
    log_warn("[iinuji:lattice] refresh_queue_defaults_failed mode=%s error=%s\n",
             lattice_refresh_mode_name(mode), error.c_str());
    return;
  }
  if (st.lattice.refresh_pending) {
    if (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore &&
        mode == LatticeRefreshMode::SyncStore) {
      set_lattice_status(
          st,
          "Catalog refresh already running in the background; full reingest can take about a minute.",
          false);
      log_dbg("[iinuji:lattice] refresh_queue_duplicate_sync_ignored\n");
      return;
    }
    if (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore &&
        mode == LatticeRefreshMode::Snapshot) {
      log_dbg("[iinuji:lattice] refresh_queue_ignored current=%s requested=%s\n",
              lattice_refresh_mode_name(st.lattice.refresh_mode),
              lattice_refresh_mode_name(mode));
      return;
    }
    st.lattice.refresh_requeue = true;
    st.lattice.refresh_requeue_mode = mode;
    st.lattice.refresh_mode = mode;
    if (st.lattice.refresh_timed_out) {
      set_lattice_status(
          st,
          "Lattice refresh exceeded 3s; retry queued behind the stalled "
          "attempt. Press r again after it clears, or restart after runtime "
          "resets.",
          true);
    } else {
      set_lattice_status(st,
                         status.empty() ? lattice_loading_status_text(mode)
                                        : std::move(status),
                         false);
    }
    log_dbg(
        "[iinuji:lattice] refresh_requeued current=%s requested=%s timed_out=%d\n",
        lattice_refresh_mode_name(st.lattice.refresh_mode),
        lattice_refresh_mode_name(mode), st.lattice.refresh_timed_out ? 1 : 0);
    return;
  }
  st.lattice.refresh_pending = true;
  st.lattice.refresh_requeue = false;
  st.lattice.refresh_requeue_mode = LatticeRefreshMode::Snapshot;
  st.lattice.view_requeue = false;
  st.lattice.refresh_mode = mode;
  st.lattice.refresh_started_at_ms = lattice_now_ms();
  st.lattice.refresh_timed_out = false;
  set_lattice_status(st,
                     status.empty() ? lattice_loading_status_text(mode)
                                    : std::move(status),
                     false);
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  st.lattice.refresh_future = std::async(
      std::launch::async,
      [lattice_defaults,
       hashimyei_defaults, mode]() mutable -> LatticeRefreshSnapshot {
        return lattice_collect_refresh_snapshot(lattice_defaults,
                                                hashimyei_defaults, mode);
      });
  log_dbg("[iinuji:lattice] refresh_async_started mode=%s cached_rows=%zu cached_views=%zu\n",
          lattice_refresh_mode_name(mode), st.lattice.rows.size(),
          st.lattice.views.size());
}

inline bool poll_lattice_async_updates(CmdState& st) {
  bool dirty = false;
  if (st.lattice.refresh_pending && !st.lattice.refresh_timed_out &&
      st.lattice.refresh_started_at_ms > 0) {
    const std::uint64_t elapsed_ms =
        lattice_now_ms() - st.lattice.refresh_started_at_ms;
    if (st.lattice.refresh_mode == LatticeRefreshMode::Snapshot &&
        elapsed_ms >= kLatticeRefreshUiTimeoutMs) {
      st.lattice.refresh_timed_out = true;
      set_lattice_status(
          st,
          "Lattice refresh exceeded 3s and looks stuck. Waiting in the "
          "background; keep using cached data, or press r to queue one retry.",
          true);
      log_warn("[iinuji:lattice] refresh_async_timeout mode=%s elapsed_ms=%llu\n",
               lattice_refresh_mode_name(st.lattice.refresh_mode),
               static_cast<unsigned long long>(elapsed_ms));
      dirty = true;
    }
  }
  if (st.lattice.refresh_pending && st.lattice.refresh_future.valid() &&
      st.lattice.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    const std::uint64_t elapsed_ms =
        st.lattice.refresh_started_at_ms > 0
            ? lattice_now_ms() - st.lattice.refresh_started_at_ms
            : 0;
    LatticeRefreshSnapshot snapshot = st.lattice.refresh_future.get();
    st.lattice.refresh_pending = false;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    if (snapshot.ok) {
      log_dbg(
          "[iinuji:lattice] refresh_async_complete mode=%s elapsed_ms=%llu requeue=%d\n",
          lattice_refresh_mode_name(st.lattice.refresh_mode),
          static_cast<unsigned long long>(elapsed_ms),
          st.lattice.refresh_requeue ? 1 : 0);
    } else {
      log_warn(
          "[iinuji:lattice] refresh_async_failed mode=%s elapsed_ms=%llu error=%s\n",
          lattice_refresh_mode_name(st.lattice.refresh_mode),
          static_cast<unsigned long long>(elapsed_ms), snapshot.error.c_str());
    }
    dirty = lattice_apply_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.lattice.refresh_requeue) {
      const auto mode = st.lattice.refresh_requeue_mode;
      st.lattice.refresh_requeue = false;
      st.lattice.refresh_requeue_mode = LatticeRefreshMode::Snapshot;
      log_dbg("[iinuji:lattice] refresh_async_requeue_dispatch mode=%s\n",
              lattice_refresh_mode_name(mode));
      queue_lattice_refresh(st, mode);
    }
  }
  if (st.lattice.detail_refresh_pending && st.lattice.detail_future.valid() &&
      st.lattice.detail_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    LatticeMemberDetailSnapshot snapshot = st.lattice.detail_future.get();
    st.lattice.detail_refresh_pending = false;
    dirty = lattice_apply_member_detail_snapshot(st, snapshot) || dirty;
    if (st.lattice.detail_requeue) {
      st.lattice.detail_requeue = false;
      queue_lattice_selected_member_detail_if_needed(st);
    }
    if ((st.lattice.selected_section == LatticeSection::Lattice &&
         st.lattice.mode == LatticeMode::Views) ||
        st.lattice.view_requeue) {
      queue_lattice_selected_view_if_needed(st);
    }
  }
  if (st.lattice.view_refresh_pending && st.lattice.view_future.valid() &&
      st.lattice.view_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    LatticeViewTransportSnapshot snapshot = st.lattice.view_future.get();
    st.lattice.view_refresh_pending = false;
    dirty = lattice_apply_view_transport_snapshot(st, snapshot) || dirty;
    if (st.lattice.view_requeue) {
      st.lattice.view_requeue = false;
      queue_lattice_selected_view_if_needed(st);
    }
  }
  return dirty;
}

template <class AppendLog>
inline void append_lattice_show_lines(const CmdState& st, AppendLog&& append_log) {
  append_log("screen=lattice", "show", "#d8d8ff");
  append_log("section=" + lattice_section_label(st.lattice.selected_section), "show",
             "#d8d8ff");
  append_log("mode=" + lattice_mode_label(st.lattice.mode), "show", "#d8d8ff");
  append_log("families=" + std::to_string(st.lattice.families.size()) +
                 " projects=" + std::to_string(count_lattice_projects(st)) +
                 " members=" + std::to_string(count_lattice_members(st)),
             "show", "#d8d8ff");
  std::size_t fragments = 0;
  for (const auto& family : st.lattice.families) fragments += family.fragment_count;
  append_log("report_fragments=" + std::to_string(fragments), "show", "#d8d8ff");
  if (const auto* row = selected_lattice_row_ptr(st); row != nullptr) {
    append_log("selection.kind=" + lattice_row_kind_label(row->kind), "show",
               "#d8d8ff");
  }
  if (const auto* family = selected_lattice_family_ptr(st); family != nullptr) {
    append_log("selection.family=" + family->family, "show", "#d8d8ff");
  }
  if (const auto* project = selected_lattice_project_ptr(st); project != nullptr) {
    append_log("selection.project=" + project->project_path, "show", "#d8d8ff");
  }
  if (const auto* member = selected_lattice_member_ptr(st); member != nullptr) {
    append_log("selection.member=" + member->canonical_path, "show", "#d8d8ff");
  }
  if (const auto* view = selected_lattice_view_ptr(st); view != nullptr) {
    append_log("selection.view=" + view->view_kind, "show", "#d8d8ff");
  }
  if (const auto* component = selected_component_member_ptr(st); component != nullptr) {
    append_log("selection.component=" + component->canonical_path, "show",
               "#d8d8ff");
  }
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
