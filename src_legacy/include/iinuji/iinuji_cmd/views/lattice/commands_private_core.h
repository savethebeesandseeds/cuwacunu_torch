#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
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
    const std::string_view segment = dot == std::string_view::npos
                                         ? text.substr(start)
                                         : text.substr(start, dot - start);
    if (!segment.empty())
      out.emplace_back(segment);
    if (dot == std::string_view::npos)
      break;
    start = dot + 1;
  }
  return out;
}

inline std::string
normalize_lattice_canonical_path(std::string_view canonical_path) {
  return cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
      trim_copy(std::string(canonical_path)));
}

inline bool lattice_has_hashimyei_suffix(std::string_view canonical_path,
                                         std::string *out_hashimyei = nullptr) {
  const std::size_t dot = canonical_path.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical_path.size()) {
    return false;
  }
  std::string normalized{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(
          std::string_view(canonical_path).substr(dot + 1), &normalized)) {
    return false;
  }
  if (out_hashimyei)
    *out_hashimyei = std::move(normalized);
  return true;
}

inline std::string
lattice_project_path_for_canonical(std::string_view canonical_path) {
  const std::string normalized =
      normalize_lattice_canonical_path(canonical_path);
  if (!lattice_has_hashimyei_suffix(normalized))
    return normalized;
  const std::size_t dot = normalized.rfind('.');
  return dot == std::string::npos ? normalized : normalized.substr(0, dot);
}

inline std::string
lattice_family_from_canonical(std::string_view canonical_path) {
  const auto segments = lattice_split_segments(canonical_path);
  if (segments.size() >= 3 && segments[0] == "tsi" &&
      segments[1] == "wikimyei") {
    return segments[2];
  }
  if (segments.size() >= 2 && segments[0] == "tsi")
    return segments[1];
  if (!segments.empty())
    return segments.front();
  return "unknown";
}

inline std::string
lattice_display_name_for_member(std::string_view canonical_path,
                                std::string_view hashimyei) {
  if (!trim_copy(std::string(hashimyei)).empty())
    return std::string(hashimyei);
  const std::string project_path =
      lattice_project_path_for_canonical(canonical_path);
  const auto segments = lattice_split_segments(project_path);
  if (!segments.empty())
    return segments.back();
  return std::string(canonical_path);
}

inline std::string
lattice_contract_hash_for_member(const LatticeMemberEntry &member) {
  if (!member.contract_hash.empty())
    return member.contract_hash;
  if (!member.has_component)
    return {};
  return cuwacunu::hero::hashimyei::contract_hash_from_identity(
      member.component.manifest.contract_identity);
}

inline std::string lattice_component_compatibility_sha256_hex_for_member(
    const LatticeMemberEntry &member) {
  if (!member.component_compatibility_sha256_hex.empty())
    return member.component_compatibility_sha256_hex;
  if (!member.has_component)
    return {};
  return trim_copy(
      member.component.manifest.component_compatibility_sha256_hex);
}

inline std::uint64_t
lattice_member_latest_ts_ms(const LatticeMemberEntry &member) {
  std::uint64_t ts_ms = 0;
  if (member.has_component)
    ts_ms = std::max(ts_ms, member.component.ts_ms);
  if (member.has_fact)
    ts_ms = std::max(ts_ms, member.fact.latest_ts_ms);
  return ts_ms;
}

inline std::string
lattice_member_lineage_state_text(const LatticeMemberEntry &member) {
  if (!trim_copy(member.lineage_state).empty()) {
    return member.lineage_state;
  }
  if (member.has_component &&
      !member.component.manifest.lineage_state.empty()) {
    return member.component.manifest.lineage_state;
  }
  if (member.has_fact)
    return "fact-only";
  return "unknown";
}

inline std::string
lattice_member_parent_hashimyei_text(const LatticeMemberEntry &member) {
  if (!member.has_component ||
      !member.component.manifest.parent_identity.has_value()) {
    return {};
  }
  return member.component.manifest.parent_identity->name;
}

inline std::size_t count_lattice_projects(const CmdState &st) {
  std::size_t count = 0;
  for (const auto &family : st.lattice.families)
    count += family.projects.size();
  return count;
}

inline std::size_t count_lattice_members(const CmdState &st) {
  std::size_t count = 0;
  for (const auto &family : st.lattice.families)
    count += family.member_count;
  return count;
}

inline std::size_t count_lattice_component_members(const CmdState &st) {
  std::size_t count = 0;
  for (const auto &root : st.lattice.component_roots) {
    count += root.component_count;
  }
  return count;
}

inline std::size_t count_lattice_fact_members(const CmdState &st) {
  std::size_t count = 0;
  for (const auto &family : st.lattice.families) {
    for (const auto &project : family.projects) {
      count += static_cast<std::size_t>(
          std::count_if(project.members.begin(), project.members.end(),
                        [](const auto &member) { return member.has_fact; }));
    }
  }
  return count;
}

inline bool
lattice_member_is_component_cataloged(const LatticeMemberEntry &member) {
  return member.in_hashimyei_catalog || member.has_component ||
         member.component_count > 0;
}

inline std::string lattice_pluralize_component_label(std::string_view text) {
  const std::string base = trim_copy(std::string(text));
  if (base.empty())
    return "components";
  if (!base.empty() && base.back() == 's')
    return base;
  if (base == "dataloader")
    return "dataloaders";
  if (base == "representation")
    return "representations";
  if (base == "inference")
    return "inferences";
  return base + "s";
}

inline std::string
lattice_component_root_key_for_member(const LatticeMemberEntry &member) {
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
  if (root_key == "sources")
    return "Sources";
  if (root_key == "wikimyei")
    return "Wikimyei";
  return "Other";
}

inline std::string
lattice_component_category_key_for_member(const LatticeMemberEntry &member) {
  const auto root_key = lattice_component_root_key_for_member(member);
  const auto segments = lattice_split_segments(member.canonical_path);
  if (root_key == "sources") {
    if (segments.size() >= 3)
      return lattice_pluralize_component_label(segments[2]);
    return "sources";
  }
  if (root_key == "wikimyei") {
    if (!member.family.empty())
      return lattice_pluralize_component_label(member.family);
    if (segments.size() >= 3)
      return lattice_pluralize_component_label(segments[2]);
    return "wikimyei";
  }
  if (segments.size() >= 2) {
    return lattice_pluralize_component_label(segments.back());
  }
  return "components";
}

inline std::string
lattice_component_category_label(std::string_view category_key) {
  if (category_key.empty())
    return "Components";
  std::string label = std::string(category_key);
  if (!label.empty())
    label[0] = static_cast<char>(std::toupper(label[0]));
  return label;
}

inline int lattice_component_root_rank(std::string_view root_key) {
  if (root_key == "sources")
    return 0;
  if (root_key == "wikimyei")
    return 1;
  return 2;
}

inline int lattice_component_category_rank(std::string_view category_key) {
  if (category_key == "dataloaders")
    return 0;
  if (category_key == "representations")
    return 1;
  if (category_key == "inferences")
    return 2;
  return 3;
}

inline std::uint64_t lattice_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline constexpr std::uint64_t kLatticeRefreshUiTimeoutMs = 3000;

inline std::string lattice_source_mode_text(const CmdState &st) {
  if (st.lattice.used_hashimyei_fallback)
    return "hashimyei";
  if (st.lattice.used_hashimyei_enrichment)
    return "lattice+hashimyei";
  return "lattice";
}

inline const LatticeVisibleRow *selected_lattice_row_ptr(const CmdState &st) {
  if (st.lattice.rows.empty())
    return nullptr;
  const std::size_t index =
      std::min(st.lattice.selected_row, st.lattice.rows.size() - 1);
  return &st.lattice.rows[index];
}

inline const LatticeFamilyGroup *
selected_lattice_family_ptr(const CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row || row->family_index >= st.lattice.families.size())
    return nullptr;
  return &st.lattice.families[row->family_index];
}

inline const LatticeProjectGroup *
selected_lattice_project_ptr(const CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row)
    return nullptr;
  const auto *family = selected_lattice_family_ptr(st);
  if (!family || row->project_index >= family->projects.size())
    return nullptr;
  if (row->kind == LatticeRowKind::Family)
    return nullptr;
  return &family->projects[row->project_index];
}

inline const LatticeMemberEntry *
selected_lattice_member_ptr(const CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row || row->kind != LatticeRowKind::Member)
    return nullptr;
  const auto *project = selected_lattice_project_ptr(st);
  if (!project || row->member_index >= project->members.size())
    return nullptr;
  return &project->members[row->member_index];
}

inline const LatticeViewDefinition *
selected_lattice_view_ptr(const CmdState &st) {
  if (st.lattice.views.empty())
    return nullptr;
  const std::size_t index =
      std::min(st.lattice.selected_view_index, st.lattice.views.size() - 1);
  return &st.lattice.views[index];
}

inline LatticeFamilyGroup *selected_lattice_family_mutable_ptr(CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row || row->family_index >= st.lattice.families.size())
    return nullptr;
  return &st.lattice.families[row->family_index];
}

inline LatticeProjectGroup *selected_lattice_project_mutable_ptr(CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row)
    return nullptr;
  auto *family = selected_lattice_family_mutable_ptr(st);
  if (!family || row->project_index >= family->projects.size())
    return nullptr;
  if (row->kind == LatticeRowKind::Family)
    return nullptr;
  return &family->projects[row->project_index];
}

inline LatticeMemberEntry *selected_lattice_member_mutable_ptr(CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row || row->kind != LatticeRowKind::Member)
    return nullptr;
  auto *project = selected_lattice_project_mutable_ptr(st);
  if (!project || row->member_index >= project->members.size())
    return nullptr;
  return &project->members[row->member_index];
}

inline const LatticeComponentVisibleRow *
selected_component_row_ptr(const CmdState &st) {
  if (st.lattice.component_rows.empty())
    return nullptr;
  const std::size_t index = std::min(st.lattice.selected_component_row,
                                     st.lattice.component_rows.size() - 1);
  return &st.lattice.component_rows[index];
}

inline const LatticeComponentRootGroup *
selected_component_root_ptr(const CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row || row->root_index >= st.lattice.component_roots.size())
    return nullptr;
  return &st.lattice.component_roots[row->root_index];
}

inline const LatticeComponentCategory *
selected_component_category_ptr(const CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return nullptr;
  const auto *root = selected_component_root_ptr(st);
  if (!root || row->category_index >= root->categories.size())
    return nullptr;
  if (row->kind == LatticeComponentRowKind::Root)
    return nullptr;
  return &root->categories[row->category_index];
}

inline const LatticeMemberEntry *
selected_component_member_ptr(const CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row || row->kind != LatticeComponentRowKind::Member)
    return nullptr;
  const auto *category = selected_component_category_ptr(st);
  if (!category || row->member_index >= category->members.size())
    return nullptr;
  const auto &ref = category->members[row->member_index];
  if (ref.family_index >= st.lattice.families.size())
    return nullptr;
  const auto &family = st.lattice.families[ref.family_index];
  if (ref.project_index >= family.projects.size())
    return nullptr;
  const auto &project = family.projects[ref.project_index];
  if (ref.member_index >= project.members.size())
    return nullptr;
  return &project.members[ref.member_index];
}

inline LatticeMode selected_lattice_root_mode(const CmdState &st) {
  return st.lattice.selected_mode_row == 0 ? LatticeMode::Facts
                                           : LatticeMode::Views;
}

inline const LatticeMemberEntry *
selected_lattice_detail_member_ptr(const CmdState &st) {
  if (st.lattice.selected_section == LatticeSection::Components) {
    return selected_component_member_ptr(st);
  }
  if (st.lattice.selected_section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Facts) {
    return selected_lattice_member_ptr(st);
  }
  return nullptr;
}

inline bool lattice_is_navigator_focus(const CmdState &st) {
  return st.lattice.focus == LatticeFocus::Navigator;
}

inline bool lattice_is_worklist_focus(const CmdState &st) {
  return st.lattice.focus == LatticeFocus::Worklist;
}

inline bool lattice_focus_worklist(CmdState &st) {
  if (st.lattice.focus == LatticeFocus::Worklist)
    return false;
  st.lattice.focus = LatticeFocus::Worklist;
  return true;
}

inline bool lattice_focus_navigator(CmdState &st) {
  if (st.lattice.focus == LatticeFocus::Navigator)
    return false;
  st.lattice.focus = LatticeFocus::Navigator;
  return true;
}

inline void clamp_lattice_state(CmdState &st) {
  if (st.lattice.rows.empty()) {
    st.lattice.selected_row = 0;
  } else if (st.lattice.selected_row >= st.lattice.rows.size()) {
    st.lattice.selected_row = st.lattice.rows.size() - 1;
  }
  if (st.lattice.component_rows.empty()) {
    st.lattice.selected_component_row = 0;
  } else if (st.lattice.selected_component_row >=
             st.lattice.component_rows.size()) {
    st.lattice.selected_component_row = st.lattice.component_rows.size() - 1;
  }
  st.lattice.selected_mode_row =
      std::min<std::size_t>(st.lattice.selected_mode_row, 1);
  if (st.lattice.views.empty()) {
    st.lattice.selected_view_index = 0;
  } else if (st.lattice.selected_view_index >= st.lattice.views.size()) {
    st.lattice.selected_view_index = st.lattice.views.size() - 1;
  }
}

inline void rebuild_lattice_visible_rows(CmdState &st) {
  st.lattice.rows.clear();
  for (std::size_t i = 0; i < st.lattice.families.size(); ++i) {
    bool family_has_facts = false;
    for (std::size_t j = 0; j < st.lattice.families[i].projects.size(); ++j) {
      bool project_has_facts = false;
      for (std::size_t k = 0;
           k < st.lattice.families[i].projects[j].members.size(); ++k) {
        if (!st.lattice.families[i].projects[j].members[k].has_fact)
          continue;
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

inline void rebuild_lattice_component_rows(CmdState &st) {
  st.lattice.component_roots.clear();
  st.lattice.component_rows.clear();

  for (std::size_t i = 0; i < st.lattice.families.size(); ++i) {
    const auto &family = st.lattice.families[i];
    for (std::size_t j = 0; j < family.projects.size(); ++j) {
      const auto &project = family.projects[j];
      for (std::size_t k = 0; k < project.members.size(); ++k) {
        const auto &member = project.members[k];
        if (!lattice_member_is_component_cataloged(member))
          continue;

        const std::string root_key =
            lattice_component_root_key_for_member(member);
        const std::string category_key =
            lattice_component_category_key_for_member(member);

        auto root_it = std::find_if(
            st.lattice.component_roots.begin(),
            st.lattice.component_roots.end(),
            [&](const auto &root) { return root.root_key == root_key; });
        if (root_it == st.lattice.component_roots.end()) {
          st.lattice.component_roots.push_back(LatticeComponentRootGroup{
              .root_key = root_key,
              .root_label = lattice_component_root_label(root_key),
          });
          root_it = std::prev(st.lattice.component_roots.end());
        }
        root_it->component_count += 1;
        root_it->report_fragment_count += member.report_fragment_count;

        auto category_it =
            std::find_if(root_it->categories.begin(), root_it->categories.end(),
                         [&](const auto &category) {
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
        category_it->members.push_back(LatticeComponentMemberRef{i, j, k});
      }
    }
  }

  std::sort(st.lattice.component_roots.begin(),
            st.lattice.component_roots.end(), [](const auto &a, const auto &b) {
              const int ar = lattice_component_root_rank(a.root_key);
              const int br = lattice_component_root_rank(b.root_key);
              if (ar != br)
                return ar < br;
              return a.root_label < b.root_label;
            });
  for (auto &root : st.lattice.component_roots) {
    std::sort(root.categories.begin(), root.categories.end(),
              [](const auto &a, const auto &b) {
                const int ar = lattice_component_category_rank(a.category_key);
                const int br = lattice_component_category_rank(b.category_key);
                if (ar != br)
                  return ar < br;
                return a.category_label < b.category_label;
              });
    for (auto &category : root.categories) {
      std::sort(category.members.begin(), category.members.end(),
                [&](const auto &a, const auto &b) {
                  const auto &am = st.lattice.families[a.family_index]
                                       .projects[a.project_index]
                                       .members[a.member_index];
                  const auto &bm = st.lattice.families[b.family_index]
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
           k < st.lattice.component_roots[i].categories[j].members.size();
           ++k) {
        st.lattice.component_rows.push_back(LatticeComponentVisibleRow{
            LatticeComponentRowKind::Member, i, j, k});
      }
    }
  }

  clamp_lattice_state(st);
}

inline void set_lattice_status(CmdState &st, std::string status,
                               bool is_error) {
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

inline const lattice_json_value_t *
lattice_json_field(const lattice_json_value_t *object, std::string_view key) {
  if (object == nullptr || object->type != lattice_json_type_t::OBJECT ||
      !object->objectValue) {
    return nullptr;
  }
  const auto it = object->objectValue->find(std::string(key));
  if (it == object->objectValue->end())
    return nullptr;
  return &it->second;
}

inline std::string lattice_json_string(const lattice_json_value_t *value,
                                       std::string fallback = {}) {
  if (value == nullptr)
    return fallback;
  if (value->type == lattice_json_type_t::STRING)
    return value->stringValue;
  return fallback;
}

inline bool lattice_json_bool(const lattice_json_value_t *value,
                              bool fallback = false) {
  if (value == nullptr)
    return fallback;
  if (value->type == lattice_json_type_t::BOOLEAN)
    return value->boolValue;
  return fallback;
}

inline std::uint64_t lattice_json_u64(const lattice_json_value_t *value,
                                      std::uint64_t fallback = 0) {
  if (value == nullptr)
    return fallback;
  if (value->type == lattice_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  if (value->type == lattice_json_type_t::STRING) {
    std::uint64_t out = 0;
    if (cuwacunu::hero::wave::lattice_catalog_store_t::
            parse_runtime_wave_cursor_token(value->stringValue, &out)) {
      return out;
    }
  }
  return fallback;
}

inline std::optional<std::uint64_t>
lattice_json_optional_u64(const lattice_json_value_t *value) {
  if (value == nullptr || value->type == lattice_json_type_t::NULL_TYPE) {
    return std::nullopt;
  }
  if (value->type == lattice_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  return std::nullopt;
}

inline std::vector<std::string>
lattice_json_string_array(const lattice_json_value_t *value) {
  std::vector<std::string> out{};
  if (value == nullptr || value->type != lattice_json_type_t::ARRAY ||
      !value->arrayValue) {
    return out;
  }
  out.reserve(value->arrayValue->size());
  for (const auto &entry : *value->arrayValue) {
    if (entry.type == lattice_json_type_t::STRING) {
      out.push_back(entry.stringValue);
    }
  }
  return out;
}

inline bool
lattice_fill_latest_fact_summary_from_json(const lattice_json_value_t *object,
                                           LatticeFactSummary *out,
                                           std::string *error) {
  if (error)
    error->clear();
  if (object == nullptr || out == nullptr ||
      object->type != lattice_json_type_t::OBJECT) {
    if (error)
      *error = "invalid lattice fact summary";
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
  out->source_runtime_cursors = lattice_json_string_array(
      lattice_json_field(object, "source_runtime_cursors"));
  return !out->canonical_path.empty();
}

struct lattice_hashimyei_ref_t {
  std::string canonical_path{};
  std::string hashimyei{};
  std::size_t component_count{0};
  std::size_t report_fragment_count{0};
};

inline bool
lattice_fill_hashimyei_ref_from_json(const lattice_json_value_t *object,
                                     lattice_hashimyei_ref_t *out,
                                     std::string *error) {
  if (error)
    error->clear();
  if (object == nullptr || out == nullptr ||
      object->type != lattice_json_type_t::OBJECT) {
    if (error)
      *error = "invalid hashimyei ref row";
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

inline void lattice_append_unique_value(std::vector<std::string> *out,
                                        std::string value) {
  if (out == nullptr)
    return;
  value = trim_copy(std::move(value));
  if (value.empty())
    return;
  if (std::find(out->begin(), out->end(), value) != out->end())
    return;
  out->push_back(std::move(value));
}

inline void
lattice_merge_string_values(std::vector<std::string> *out,
                            const std::vector<std::string> &values) {
  if (out == nullptr)
    return;
  for (const auto &value : values)
    lattice_append_unique_value(out, value);
}

inline bool
lattice_parse_tool_structured_content(std::string_view tool_result_json,
                                      lattice_json_value_t *out_structured,
                                      std::string *error) {
  if (error)
    error->clear();
  if (out_structured == nullptr) {
    if (error)
      *error = "structured content output pointer is null";
    return false;
  }
  *out_structured = lattice_json_value_t{};
  try {
    const lattice_json_value_t root =
        cuwacunu::piaabo::JsonParser(std::string(tool_result_json)).parse();
    const auto *is_error = lattice_json_field(&root, "isError");
    if (lattice_json_bool(is_error, false)) {
      const auto *structured = lattice_json_field(&root, "structuredContent");
      std::string message{};
      if (structured != nullptr) {
        message = lattice_json_string(lattice_json_field(structured, "error"));
      }
      if (message.empty()) {
        if (const auto *content = lattice_json_field(&root, "content");
            content != nullptr && content->type == lattice_json_type_t::ARRAY &&
            content->arrayValue && !content->arrayValue->empty()) {
          message = lattice_json_string(
              lattice_json_field(&content->arrayValue->front(), "text"));
        }
      }
      if (error)
        *error = message.empty() ? "tool returned error" : message;
      return false;
    }
    const auto *structured = lattice_json_field(&root, "structuredContent");
    if (structured == nullptr) {
      if (error)
        *error = "tool result missing structuredContent";
      return false;
    }
    *out_structured = *structured;
    return true;
  } catch (const std::exception &ex) {
    if (error)
      *error = ex.what();
    return false;
  }
}

inline bool lattice_invoke_tool(cuwacunu::hero::lattice_mcp::app_context_t *app,
                                const std::string &tool_name,
                                const std::string &arguments_json,
                                lattice_json_value_t *out_structured,
                                std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing lattice app context";
    return false;
  }
  if (!cuwacunu::hero::lattice_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return lattice_parse_tool_structured_content(result_json, out_structured,
                                               error);
}

inline bool lattice_invoke_tool(CmdState &st, const std::string &tool_name,
                                const std::string &arguments_json,
                                lattice_json_value_t *out_structured,
                                std::string *error) {
  return lattice_invoke_tool(&st.lattice.app, tool_name, arguments_json,
                             out_structured, error);
}

inline bool
lattice_load_view_definitions(cuwacunu::hero::lattice_mcp::app_context_t *app,
                              std::vector<LatticeViewDefinition> *out,
                              std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "lattice view output pointer is null";
    return false;
  }
  out->clear();
  lattice_json_value_t structured{};
  if (!lattice_invoke_tool(app, "hero.lattice.list_views", "{}", &structured,
                           error)) {
    return false;
  }
  const auto *views = lattice_json_field(&structured, "views");
  if (views == nullptr || views->type != lattice_json_type_t::ARRAY ||
      !views->arrayValue) {
    if (error)
      *error = "lattice views result missing views";
    return false;
  }
  out->reserve(views->arrayValue->size());
  for (const auto &entry : *views->arrayValue) {
    if (entry.type != lattice_json_type_t::OBJECT)
      continue;
    LatticeViewDefinition view{};
    view.view_kind =
        lattice_json_string(lattice_json_field(&entry, "view_kind"));
    view.preferred_selector =
        lattice_json_string(lattice_json_field(&entry, "preferred_selector"));
    view.required_selectors = lattice_json_string_array(
        lattice_json_field(&entry, "required_selectors"));
    view.optional_selectors = lattice_json_string_array(
        lattice_json_field(&entry, "optional_selectors"));
    view.ready = lattice_json_bool(lattice_json_field(&entry, "ready"));
    if (!view.view_kind.empty())
      out->push_back(std::move(view));
  }
  return true;
}

inline bool lattice_load_latest_fact_summaries(
    cuwacunu::hero::lattice_mcp::app_context_t *app,
    std::vector<LatticeFactSummary> *out, std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "lattice fact output pointer is null";
    return false;
  }
  out->clear();
  lattice_json_value_t structured{};
  if (!lattice_invoke_tool(app, "hero.lattice.list_facts", "{}", &structured,
                           error)) {
    return false;
  }
  const auto *facts = lattice_json_field(&structured, "facts");
  if (facts == nullptr || facts->type != lattice_json_type_t::ARRAY ||
      !facts->arrayValue) {
    if (error)
      *error = "lattice facts result missing facts";
    return false;
  }
  out->reserve(facts->arrayValue->size());
  for (const auto &entry : *facts->arrayValue) {
    LatticeFactSummary fact{};
    std::string fact_error{};
    if (!lattice_fill_latest_fact_summary_from_json(&entry, &fact,
                                                    &fact_error)) {
      if (error)
        *error = fact_error;
      return false;
    }
    out->push_back(std::move(fact));
  }
  return true;
}

inline bool
lattice_load_hashimyei_refs(cuwacunu::hero::hashimyei_mcp::app_context_t *app,
                            std::vector<lattice_hashimyei_ref_t> *out,
                            std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "hashimyei ref output pointer is null";
    return false;
  }
  out->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing hashimyei app context";
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
  const auto *rows = lattice_json_field(&structured, "hashimyeis");
  if (rows == nullptr || rows->type != lattice_json_type_t::ARRAY ||
      !rows->arrayValue) {
    if (error)
      *error = "hashimyei list result missing hashimyeis";
    return false;
  }
  out->reserve(rows->arrayValue->size());
  for (const auto &entry : *rows->arrayValue) {
    lattice_hashimyei_ref_t ref{};
    std::string ref_error{};
    if (!lattice_fill_hashimyei_ref_from_json(&entry, &ref, &ref_error)) {
      if (error)
        *error = ref_error;
      return false;
    }
    out->push_back(std::move(ref));
  }
  return true;
}

inline bool ensure_lattice_defaults_loaded(CmdState &st, std::string *error) {
  if (error)
    error->clear();
  if (st.lattice.defaults_loaded)
    return true;

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
  st.lattice.app.lattice_catalog_path =
      st.lattice.lattice_defaults.catalog_path;
  st.lattice.app.hashimyei_catalog_path =
      st.lattice.hashimyei_defaults.catalog_path;
  st.lattice.app.close_catalog_after_execute = true;
  st.lattice.defaults_loaded = true;
  return true;
}
