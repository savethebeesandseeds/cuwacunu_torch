#pragma once

#include <array>
#include <sstream>
#include <string_view>

#include "iinuji/iinuji_cmd/views/lattice/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct lattice_left_panel_t {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  std::optional<std::size_t> selected_line{};
};

inline void push_lattice_line(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out, std::string text,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
  });
}

inline cuwacunu::iinuji::text_line_emphasis_t lattice_family_emphasis(
    std::string_view family) {
  if (family == "representation") {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  if (family == "source") {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Warning;
}

inline cuwacunu::iinuji::text_line_emphasis_t lattice_member_emphasis(
    const LatticeMemberEntry& member, bool selected) {
  if (selected) return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  const std::string state = lattice_member_lineage_state_text(member);
  if ((state == "replaced") ||
      !trim_copy(member.has_component ? member.component.manifest.replaced_by
                                      : std::string{})
           .empty()) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (member.family_rank.has_value() && *member.family_rank == 0 &&
      state == "active") {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  if (state == "active" && member.has_fact) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (!member.has_component && member.has_fact) {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  if (!member.has_fact) return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline std::string lattice_member_rank_text(const LatticeMemberEntry& member) {
  if (!member.family_rank.has_value()) return "-";
  if (*member.family_rank == 0) return "best";
  return "r" + std::to_string(*member.family_rank);
}

inline std::string lattice_compact_project_label(std::string_view project_path,
                                                 std::string_view family) {
  const auto segments = lattice_split_segments(project_path);
  if (segments.empty()) return std::string(project_path);

  std::size_t start = 0;
  if (segments.size() >= 4 && segments[0] == "tsi" && segments[1] == "wikimyei" &&
      segments[2] == family) {
    start = 3;
  } else if (segments.size() >= 3 && segments[0] == "tsi" &&
             segments[1] == family) {
    start = 2;
  } else {
    for (std::size_t i = 0; i < segments.size(); ++i) {
      if (segments[i] == family) {
        start = i + 1;
        break;
      }
    }
  }

  std::ostringstream oss;
  for (std::size_t i = start; i < segments.size(); ++i) {
    if (i > start) oss << '.';
    oss << segments[i];
  }
  const std::string label = trim_copy(oss.str());
  if (!label.empty()) return label;
  return segments.back();
}

inline std::string lattice_project_label(const LatticeProjectGroup& project) {
  return trim_to_width(
      lattice_compact_project_label(project.project_path, project.family), 34);
}

inline std::string lattice_member_brief(const LatticeMemberEntry& member,
                                        std::size_t name_width = 18) {
  return trim_to_width(member.display_name, name_width) + " | " +
         trim_to_width(lattice_member_lineage_state_text(member), 10) + " | " +
         lattice_member_rank_text(member) + " | " +
         (member.has_fact ? std::to_string(member.fact.fragment_count)
                          : std::string("0")) +
         " lls";
}

inline std::string lattice_join_selector_list(
    const std::vector<std::string>& selectors) {
  if (selectors.empty()) return "<none>";
  return join_trimmed_values(selectors, 8, 18);
}

inline std::string lattice_bool_word(bool value) {
  return value ? "yes" : "no";
}

inline std::string lattice_worklist_panel_title(const CmdState& st) {
  (void)st;
  return "worklist";
}

inline std::string lattice_primary_panel_title(const CmdState& st) {
  switch (st.lattice.selected_section) {
    case LatticeSection::Overview:
      return "overview";
    case LatticeSection::Components:
      return "details";
    case LatticeSection::Lattice:
      return st.lattice.mode == LatticeMode::Views ? "view" : "details";
  }
  return "details";
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_lattice_navigation_styled_lines(const CmdState& st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  for (const auto section :
       {LatticeSection::Overview, LatticeSection::Components,
        LatticeSection::Lattice}) {
    std::string label = "[" + lattice_section_label(section) + "]";
    if (section == st.lattice.selected_section) label = mark_selected_line(label);
    push_lattice_line(out, std::move(label),
                      section == st.lattice.selected_section
                          ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                          : cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_lattice_line(out, {});
  }
  if (!out.empty()) out.pop_back();
  return out;
}

inline lattice_left_panel_t make_lattice_components_worklist_panel(
    const CmdState& st) {
  lattice_left_panel_t panel{};
  auto& out = panel.lines;
  const auto* selected_row = selected_component_row_ptr(st);
  std::size_t line_index = 0;

  for (std::size_t i = 0; i < st.lattice.component_roots.size(); ++i) {
    const auto& root = st.lattice.component_roots[i];
    const bool root_selected =
        selected_row != nullptr && selected_row->root_index == i;
    const bool root_focused =
        selected_row != nullptr && selected_row->kind == LatticeComponentRowKind::Root &&
        selected_row->root_index == i;

    std::ostringstream root_row;
    root_row << "[" << root.root_label << "] " << root.component_count
             << " components | " << root.report_fragment_count << " lls";
    std::string root_text = "  " + root_row.str();
    if (root_focused) {
      root_text = mark_selected_line(root_text);
      panel.selected_line = line_index;
    }
    push_lattice_line(
        out, root_text,
        root_selected ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                      : cuwacunu::iinuji::text_line_emphasis_t::Info);
    ++line_index;

    if (root_selected) {
      for (std::size_t j = 0; j < root.categories.size(); ++j) {
        const auto& category = root.categories[j];
        const bool category_selected =
            selected_row != nullptr &&
            selected_row->root_index == i &&
            selected_row->category_index == j &&
            selected_row->kind != LatticeComponentRowKind::Root;
        const bool category_focused =
            selected_row != nullptr &&
            selected_row->kind == LatticeComponentRowKind::Category &&
            selected_row->root_index == i &&
            selected_row->category_index == j;

        std::ostringstream category_row;
        category_row << "    " << category.category_label << " | "
                     << category.component_count << " components | "
                     << category.report_fragment_count << " lls";
        std::string category_text = category_row.str();
        if (category_focused) {
          category_text = mark_selected_line(category_text);
          panel.selected_line = line_index;
        }
        push_lattice_line(
            out, category_text,
            category_selected ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                              : cuwacunu::iinuji::text_line_emphasis_t::Info);
        ++line_index;

        if (category_selected) {
          for (std::size_t k = 0; k < category.members.size(); ++k) {
            const auto& ref = category.members[k];
            const auto& member =
                st.lattice.families[ref.family_index]
                    .projects[ref.project_index]
                    .members[ref.member_index];
            const bool member_focused =
                selected_row != nullptr &&
                selected_row->kind == LatticeComponentRowKind::Member &&
                selected_row->root_index == i &&
                selected_row->category_index == j &&
                selected_row->member_index == k;

            std::ostringstream member_row;
            member_row << "      " << trim_to_width(member.display_name, 18)
                       << " | "
                       << trim_to_width(lattice_member_lineage_state_text(member), 10)
                       << " | " << member.report_fragment_count << " lls";
            std::string member_text = member_row.str();
            if (member_focused) {
              member_text = mark_selected_line(member_text);
              panel.selected_line = line_index;
            }
            push_lattice_line(out, member_text,
                              lattice_member_emphasis(member, member_focused));
            ++line_index;
          }
        }
      }
      push_lattice_line(out, {});
      ++line_index;
    }
  }

  if (!out.empty()) out.pop_back();
  if (out.empty()) {
    push_lattice_line(out, "  (no hashimyei components discovered yet)",
                      cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  return panel;
}

inline void append_lattice_fact_rows(
    lattice_left_panel_t* panel,
    const CmdState& st, std::size_t* line_index, bool enable_selection) {
  if (panel == nullptr || line_index == nullptr) return;
  auto& out = panel->lines;
  const auto* selected_row =
      enable_selection ? selected_lattice_row_ptr(st) : nullptr;
  bool any = false;

  for (std::size_t i = 0; i < st.lattice.families.size(); ++i) {
    const auto& family = st.lattice.families[i];
    bool family_has_facts = false;
    for (const auto& project : family.projects) {
      family_has_facts = family_has_facts ||
                         std::any_of(project.members.begin(), project.members.end(),
                                     [](const auto& member) { return member.has_fact; });
    }
    if (!family_has_facts) continue;
    any = true;

    const bool family_selected =
        selected_row != nullptr && selected_row->kind == LatticeRowKind::Family &&
        selected_row->family_index == i;

    std::ostringstream family_row;
    family_row << "    [" << family.family << "] "
               << std::count_if(family.projects.begin(), family.projects.end(),
                                [](const auto& project) {
                                  return std::any_of(
                                      project.members.begin(), project.members.end(),
                                      [](const auto& member) {
                                        return member.has_fact;
                                      });
                                })
               << " projects | " << family.fragment_count << " lls";
    std::string family_text = family_row.str();
    if (family_selected) {
      family_text = mark_selected_line(family_text);
      panel->selected_line = *line_index;
    }
    push_lattice_line(out, family_text, lattice_family_emphasis(family.family));
    ++(*line_index);

    for (std::size_t j = 0; j < family.projects.size(); ++j) {
      const auto& project = family.projects[j];
      const std::size_t fact_member_count = static_cast<std::size_t>(std::count_if(
          project.members.begin(), project.members.end(),
          [](const auto& member) { return member.has_fact; }));
      if (fact_member_count == 0) continue;
      const bool project_selected =
          selected_row != nullptr && selected_row->kind == LatticeRowKind::Project &&
          selected_row->family_index == i && selected_row->project_index == j;

      std::ostringstream project_row;
      project_row << "      " << lattice_project_label(project) << " | "
                  << fact_member_count << " members | "
                  << project.fragment_count << " lls | "
                  << project.available_context_count << " ctx";
      std::string project_text = project_row.str();
      if (project_selected) {
        project_text = mark_selected_line(project_text);
        panel->selected_line = *line_index;
      }
      push_lattice_line(out, project_text,
                        project_selected
                            ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                            : cuwacunu::iinuji::text_line_emphasis_t::Info);
      ++(*line_index);

      for (std::size_t k = 0; k < project.members.size(); ++k) {
        const auto& member = project.members[k];
        if (!member.has_fact) continue;
        const bool member_selected =
            selected_row != nullptr && selected_row->kind == LatticeRowKind::Member &&
            selected_row->family_index == i && selected_row->project_index == j &&
            selected_row->member_index == k;

        std::string member_text = "        " + lattice_member_brief(member, 16);
        if (member_selected) {
          member_text = mark_selected_line(member_text);
          panel->selected_line = *line_index;
        }
        push_lattice_line(out, member_text,
                          lattice_member_emphasis(member, member_selected));
        ++(*line_index);
      }
    }
  }

  if (!any) {
    push_lattice_line(out, "    (no lattice facts discovered yet)",
                      cuwacunu::iinuji::text_line_emphasis_t::Debug);
    ++(*line_index);
  }
}

inline void append_lattice_view_rows(
    lattice_left_panel_t* panel,
    const CmdState& st, std::size_t* line_index, bool enable_selection) {
  if (panel == nullptr || line_index == nullptr) return;
  auto& out = panel->lines;
  if (st.lattice.views.empty()) {
    push_lattice_line(out, "    (no lattice views reported)",
                      cuwacunu::iinuji::text_line_emphasis_t::Debug);
    ++(*line_index);
    return;
  }
  for (std::size_t i = 0; i < st.lattice.views.size(); ++i) {
    const auto& view = st.lattice.views[i];
    const bool selected = enable_selection && i == st.lattice.selected_view_index;
    std::string row = "    [" + std::to_string(i + 1) + "] " + view.view_kind +
                      " | ready=" + lattice_bool_word(view.ready);
    if (selected) {
      row = mark_selected_line(row);
      panel->selected_line = *line_index;
    }
    push_lattice_line(out, std::move(row),
                      selected
                          ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                          : (view.ready
                                 ? cuwacunu::iinuji::text_line_emphasis_t::Success
                                 : cuwacunu::iinuji::text_line_emphasis_t::Info));
    ++(*line_index);
  }
}

inline lattice_left_panel_t make_lattice_overview_worklist_panel(
    const CmdState& st) {
  lattice_left_panel_t panel{};
  auto& out = panel.lines;
  std::size_t fragment_count = 0;
  for (const auto& family : st.lattice.families) fragment_count += family.fragment_count;
  push_lattice_line(out, "mode         " + lattice_source_mode_text(st),
                    cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_lattice_line(out,
                    "components   " +
                        std::to_string(count_lattice_component_members(st)),
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  push_lattice_line(out, "facts        " +
                             std::to_string(count_lattice_fact_members(st)),
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  push_lattice_line(out, "views        " + std::to_string(st.lattice.views.size()),
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  push_lattice_line(out, "lls reports  " + std::to_string(fragment_count),
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  push_lattice_line(out, "hashimyei    " +
                             std::to_string(st.lattice.discovered_component_count) +
                             " comps / " +
                             std::to_string(st.lattice.discovered_report_manifest_count) +
                             " fragments",
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  if (fragment_count == 0) {
    push_lattice_line(out, "no .lls reports cataloged yet",
                      cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  return panel;
}

inline lattice_left_panel_t make_lattice_roots_worklist_panel(
    const CmdState& st) {
  lattice_left_panel_t panel{};
  auto& out = panel.lines;
  std::size_t line_index = 0;

  {
    const bool facts_active = st.lattice.selected_mode_row == 0;
    const bool facts_focused =
        facts_active && st.lattice.mode == LatticeMode::Roots;
    std::string row = "[facts] " + std::to_string(count_lattice_fact_members(st)) +
                      " members | " + std::to_string(st.lattice.rows.size()) +
                      " rows";
    if (facts_focused) {
      row = mark_selected_line(row);
      panel.selected_line = line_index;
    }
    push_lattice_line(out, std::move(row),
                      facts_active ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                                   : cuwacunu::iinuji::text_line_emphasis_t::Info);
    ++line_index;
    append_lattice_fact_rows(&panel, st, &line_index,
                             st.lattice.mode == LatticeMode::Facts);
  }

  push_lattice_line(out, {});
  ++line_index;

  {
    const bool views_active = st.lattice.selected_mode_row == 1;
    const bool views_focused =
        views_active && st.lattice.mode == LatticeMode::Roots;
    std::string row =
        "[views] " + std::to_string(st.lattice.views.size()) + " reported";
    if (views_focused) {
      row = mark_selected_line(row);
      panel.selected_line = line_index;
    }
    push_lattice_line(out, std::move(row),
                      views_active ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                                   : cuwacunu::iinuji::text_line_emphasis_t::Info);
    ++line_index;
    append_lattice_view_rows(&panel, st, &line_index,
                             st.lattice.mode == LatticeMode::Views);
  }

  return panel;
}

inline lattice_left_panel_t make_lattice_worklist_panel(const CmdState& st) {
  switch (st.lattice.selected_section) {
    case LatticeSection::Overview:
      return make_lattice_overview_worklist_panel(st);
    case LatticeSection::Components:
      return make_lattice_components_worklist_panel(st);
    case LatticeSection::Lattice:
      return make_lattice_roots_worklist_panel(st);
  }
  return make_lattice_overview_worklist_panel(st);
}

inline std::string make_lattice_left(const CmdState& st) {
  const auto panel = make_lattice_worklist_panel(st);
  std::ostringstream oss;
  for (std::size_t i = 0; i < panel.lines.size(); ++i) {
    if (i > 0) oss << '\n';
    oss << panel.lines[i].text;
  }
  return oss.str();
}

inline std::string lattice_detail_rule() { return panel_rule('-', 56); }

inline void append_lattice_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    std::string_view title,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Accent) {
  push_lattice_line(out, lattice_detail_rule(), emphasis);
  push_lattice_line(out, std::string(title), emphasis);
  push_lattice_line(out, lattice_detail_rule(), emphasis);
}

inline std::string lattice_detail_meta_prefix(std::string_view key) {
  constexpr std::size_t kLatticeDetailKeyWidth = 18;
  std::string prefix = std::string(key) + ":";
  if (prefix.size() < kLatticeDetailKeyWidth) {
    prefix.append(kLatticeDetailKeyWidth - prefix.size(), ' ');
  } else {
    prefix.push_back(' ');
  }
  return prefix;
}

inline void append_lattice_meta(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out, std::string_view key,
    std::string value,
    cuwacunu::iinuji::text_line_emphasis_t key_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info,
    cuwacunu::iinuji::text_line_emphasis_t value_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  if (value.empty()) value = "<none>";
  const std::string prefix = lattice_detail_meta_prefix(key);
  const auto line_emphasis =
      (value_emphasis == cuwacunu::iinuji::text_line_emphasis_t::None ||
       value_emphasis == cuwacunu::iinuji::text_line_emphasis_t::Debug)
          ? key_emphasis
          : value_emphasis;
  std::size_t start = 0;
  while (start <= value.size()) {
    const std::size_t end = value.find('\n', start);
    const std::string_view line =
        end == std::string::npos
            ? std::string_view(value).substr(start)
            : std::string_view(value).substr(start, end - start);
    push_lattice_line(out,
                      (start == 0 ? prefix
                                  : std::string(prefix.size(), ' ')) +
                          std::string(line),
                      line_emphasis);
    if (end == std::string::npos) break;
    start = end + 1;
    if (start == value.size()) break;
  }
}

inline void append_lattice_text(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out, std::string_view text,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  push_lattice_line(out, std::string(text), emphasis);
}

inline void append_lattice_preview_block(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    std::string_view title, std::string_view text, std::size_t max_lines = 14,
    std::size_t max_width = 92) {
  if (trim_copy(std::string(text)).empty()) return;
  if (!std::string(title).empty()) append_lattice_section(out, title);
  std::istringstream iss{std::string(text)};
  std::string line{};
  std::size_t emitted = 0;
  while (emitted < max_lines && std::getline(iss, line)) {
    append_lattice_text(out, trim_to_width(line, max_width),
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
    ++emitted;
  }
  if (!iss.eof()) {
    append_lattice_text(out, "...",
                        cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
}

inline bool lattice_has_taxon(const std::vector<std::string>& taxa,
                              std::string_view wanted) {
  return std::find(taxa.begin(), taxa.end(), std::string(wanted)) != taxa.end();
}

inline void append_lattice_overview_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out, const CmdState& st) {
  std::size_t fragment_count = 0;
  std::set<std::string> context_keys{};
  for (const auto& family : st.lattice.families) {
    fragment_count += family.fragment_count;
    context_keys.insert(family.wave_cursors.begin(), family.wave_cursors.end());
  }
  const bool refresh_pending = lattice_is_refresh_pending(st);
  const std::string status_value =
      refresh_pending
          ? (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore
                 ? std::string("syncing")
                 : std::string("loading"))
          : (st.lattice.ok ? std::string("ready") : std::string("error"));
  const auto status_emphasis =
      refresh_pending
          ? (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore
                 ? cuwacunu::iinuji::text_line_emphasis_t::Warning
                 : cuwacunu::iinuji::text_line_emphasis_t::Info)
          : (st.lattice.ok ? cuwacunu::iinuji::text_line_emphasis_t::Success
                           : cuwacunu::iinuji::text_line_emphasis_t::Error);

  append_lattice_section(out, "Lattice Overview");
  append_lattice_meta(out, "status", status_value, status_emphasis,
                      refresh_pending ? status_emphasis
                                      : (st.lattice.ok
                                             ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                                             : cuwacunu::iinuji::text_line_emphasis_t::Error));
  append_lattice_meta(out, "mode", lattice_source_mode_text(st));
  append_lattice_meta(out, "components",
                      std::to_string(count_lattice_component_members(st)));
  append_lattice_meta(out, "facts",
                      std::to_string(count_lattice_fact_members(st)));
  append_lattice_meta(out, "views", std::to_string(st.lattice.views.size()));
  append_lattice_meta(out, "families", std::to_string(st.lattice.families.size()));
  append_lattice_meta(out, "lls reports", std::to_string(fragment_count));
  append_lattice_meta(out, "contexts", std::to_string(context_keys.size()));
  append_lattice_meta(out, "hashimyei components",
                      std::to_string(st.lattice.discovered_component_count));
  append_lattice_meta(out, "hashimyei fragments",
                      std::to_string(st.lattice.discovered_report_manifest_count));
  if (fragment_count == 0) {
    append_lattice_text(
        out,
        "  No .lls reports are cataloged yet. Components can still appear from "
        "Hero Hashimyei, but lattice facts and views will stay limited until "
        "evidence lands.",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!st.lattice.status.empty()) {
    append_lattice_meta(
        out, "message", st.lattice.status,
        st.lattice.status_is_error
            ? cuwacunu::iinuji::text_line_emphasis_t::Error
            : cuwacunu::iinuji::text_line_emphasis_t::Info,
        st.lattice.status_is_error
            ? cuwacunu::iinuji::text_line_emphasis_t::Error
            : cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
}

inline void append_component_root_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeComponentRootGroup& root) {
  append_lattice_section(out, "Component Root");
  append_lattice_meta(out, "root", root.root_label,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "categories",
                      std::to_string(root.categories.size()));
  append_lattice_meta(out, "components",
                      std::to_string(root.component_count));
  append_lattice_meta(out, "lls reports",
                      std::to_string(root.report_fragment_count));
  append_lattice_section(out, "Categories");
  for (const auto& category : root.categories) {
    append_lattice_text(out,
                        "  " + category.category_label + " | components=" +
                            std::to_string(category.component_count) +
                            " | lls=" +
                            std::to_string(category.report_fragment_count),
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
  }
}

inline void append_component_category_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeComponentRootGroup& root,
    const LatticeComponentCategory& category, const CmdState& st) {
  append_lattice_section(out, "Component Category");
  append_lattice_meta(out, "root", root.root_label);
  append_lattice_meta(out, "category", category.category_label,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "components",
                      std::to_string(category.component_count));
  append_lattice_meta(out, "lls reports",
                      std::to_string(category.report_fragment_count));
  append_lattice_section(out, "Members");
  for (const auto& ref : category.members) {
    const auto& member =
        st.lattice.families[ref.family_index]
            .projects[ref.project_index]
            .members[ref.member_index];
    append_lattice_text(out,
                        "  " + trim_to_width(member.display_name, 22) + " | " +
                            trim_to_width(lattice_member_lineage_state_text(member), 10) +
                            " | lls=" +
                            std::to_string(member.report_fragment_count),
                        lattice_member_emphasis(member, false));
  }
}

inline void append_component_member_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeMemberEntry& member) {
  append_lattice_section(out, "Component");
  append_lattice_meta(out, "member", member.display_name,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "lineage state",
                      lattice_member_lineage_state_text(member),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      lattice_member_emphasis(member, false));
  append_lattice_meta(out, "family", member.family,
                      lattice_family_emphasis(member.family),
                      lattice_family_emphasis(member.family));
  append_lattice_meta(out, "project",
                      lattice_compact_project_label(member.project_path, member.family));
  append_lattice_meta(out, "canonical path", member.canonical_path,
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_lattice_meta(out, "hashimyei",
                      member.hashimyei.empty() ? std::string("<none>")
                                               : member.hashimyei);
  append_lattice_meta(out, "cataloged",
                      lattice_bool_word(member.in_hashimyei_catalog));
  append_lattice_meta(out, "component refs",
                      std::to_string(member.component_count));
  append_lattice_meta(out, "contract hash", lattice_contract_hash_for_member(member));
  append_lattice_meta(out, "dock hash", lattice_dock_hash_for_member(member));
  append_lattice_meta(out, "parent", lattice_member_parent_hashimyei_text(member));
  append_lattice_meta(
      out, "replaced by",
      member.has_component ? member.component.manifest.replaced_by : std::string{});

  append_lattice_section(out, "Lattice Coverage");
  append_lattice_meta(out, "lls reports",
                      std::to_string(member.report_fragment_count));
  append_lattice_meta(out, "contexts",
                      member.has_fact
                          ? std::to_string(member.fact.available_context_count)
                          : std::string("0"));
  append_lattice_meta(out, "latest wave",
                      member.has_fact ? member.fact.latest_wave_cursor
                                      : std::string{});
  if (!member.has_fact) {
    append_lattice_text(
        out,
        "  No .lls reports are available yet for this component. It exists in "
        "Hashimyei, but there is no lattice fact bundle to inspect.",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_lattice_mode_root_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const CmdState& st) {
  if (selected_lattice_root_mode(st) == LatticeMode::Facts) {
    append_lattice_section(out, "Facts");
    append_lattice_meta(out, "fact members",
                        std::to_string(count_lattice_fact_members(st)));
    append_lattice_meta(out, "rows", std::to_string(st.lattice.rows.size()));
    if (st.lattice.rows.empty()) {
      append_lattice_text(
          out,
          "  No lattice facts are available yet. Components may still exist in "
          "Hashimyei, but there are no .lls-backed fact bundles to inspect.",
          cuwacunu::iinuji::text_line_emphasis_t::Warning);
    } else {
      append_lattice_text(
          out,
          "  Press Enter on facts to drill into family, project, and member "
          "evidence rows.",
          cuwacunu::iinuji::text_line_emphasis_t::Info);
    }
    return;
  }

  append_lattice_section(out, "Views");
  append_lattice_meta(out, "reported views",
                      std::to_string(st.lattice.views.size()));
  append_lattice_text(
      out,
      "  Views are derived lattice transports. Enter this branch to inspect "
      "the view kinds reported by hero.lattice.list_views.",
      cuwacunu::iinuji::text_line_emphasis_t::Info);
}

inline void append_lattice_family_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeFamilyGroup& family) {
  const std::size_t fact_project_count = static_cast<std::size_t>(std::count_if(
      family.projects.begin(), family.projects.end(), [](const auto& project) {
        return std::any_of(project.members.begin(), project.members.end(),
                           [](const auto& member) { return member.has_fact; });
      }));
  std::size_t fact_member_count = 0;
  for (const auto& project : family.projects) {
    fact_member_count += static_cast<std::size_t>(std::count_if(
        project.members.begin(), project.members.end(),
        [](const auto& member) { return member.has_fact; }));
  }
  append_lattice_section(out, "Family");
  append_lattice_meta(out, "family", family.family, lattice_family_emphasis(family.family),
                      lattice_family_emphasis(family.family));
  append_lattice_meta(out, "projects", std::to_string(fact_project_count));
  append_lattice_meta(out, "members", std::to_string(fact_member_count));
  append_lattice_meta(out, "lls reports", std::to_string(family.fragment_count));
  append_lattice_meta(out, "wave contexts",
                      join_trimmed_values(family.wave_cursors, 6, 16));
  append_lattice_meta(out, "bindings", join_trimmed_values(family.binding_ids, 6, 24));
  append_lattice_meta(out, "taxa", join_trimmed_values(family.semantic_taxa, 6, 24));

  append_lattice_section(out, "Projects", lattice_family_emphasis(family.family));
  for (const auto& project : family.projects) {
    const std::size_t fact_member_count = static_cast<std::size_t>(std::count_if(
        project.members.begin(), project.members.end(),
        [](const auto& member) { return member.has_fact; }));
    if (fact_member_count == 0) continue;
    append_lattice_text(out,
                        "  " + lattice_project_label(project) + " | members=" +
                            std::to_string(fact_member_count) + " | lls=" +
                            std::to_string(project.fragment_count) + " | ctx=" +
                            std::to_string(project.available_context_count),
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
  }
}

inline void append_lattice_project_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeProjectGroup& project) {
  append_lattice_section(out, "Project");
  append_lattice_meta(out, "project", lattice_project_label(project),
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "full path", project.project_path,
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_lattice_meta(out, "family", project.family,
                      lattice_family_emphasis(project.family),
                      lattice_family_emphasis(project.family));
  append_lattice_meta(
      out, "members",
      std::to_string(static_cast<std::size_t>(std::count_if(
          project.members.begin(), project.members.end(),
          [](const auto& member) { return member.has_fact; }))));
  append_lattice_meta(out, "lls reports", std::to_string(project.fragment_count));
  append_lattice_meta(out, "wave contexts",
                      join_trimmed_values(project.wave_cursors, 6, 16));
  append_lattice_meta(out, "bindings", join_trimmed_values(project.binding_ids, 6, 24));
  append_lattice_meta(out, "taxa", join_trimmed_values(project.semantic_taxa, 6, 24));

  append_lattice_section(out, "Members");
  for (const auto& member : project.members) {
    if (!member.has_fact) continue;
    append_lattice_text(out,
                        "  " + lattice_member_brief(member) + " | " +
                            std::to_string(member.fact.available_context_count) +
                            " ctx",
                        lattice_member_emphasis(member, false));
  }
}

inline void append_lattice_member_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const LatticeProjectGroup& project, const LatticeMemberEntry& member) {
  append_lattice_section(out, "Identity");
  append_lattice_meta(out, "member", member.display_name,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "lineage state", lattice_member_lineage_state_text(member),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      lattice_member_emphasis(member, false));
  append_lattice_meta(out, "rank", lattice_member_rank_text(member),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      member.family_rank.has_value()
                          ? cuwacunu::iinuji::text_line_emphasis_t::Success
                          : cuwacunu::iinuji::text_line_emphasis_t::Debug);
  append_lattice_meta(out, "family", member.family, lattice_family_emphasis(member.family),
                      lattice_family_emphasis(member.family));
  append_lattice_meta(out, "project", lattice_project_label(project));
  append_lattice_meta(out, "canonical path", member.canonical_path,
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_lattice_meta(out, "hashimyei",
                      member.hashimyei.empty() ? std::string("<none>")
                                               : member.hashimyei);
  append_lattice_meta(out, "component refs",
                      std::to_string(member.component_count));
  append_lattice_meta(out, "fragment refs",
                      std::to_string(member.report_fragment_count));
  append_lattice_meta(out, "parent", lattice_member_parent_hashimyei_text(member));
  append_lattice_meta(
      out, "replaced by",
      member.has_component ? member.component.manifest.replaced_by : std::string{});
  append_lattice_meta(out, "contract hash", lattice_contract_hash_for_member(member));
  append_lattice_meta(out, "dock hash", lattice_dock_hash_for_member(member));
  append_lattice_meta(out, "detail source", "hero.lattice + hero.hashimyei");

  append_lattice_section(out, "Coverage");
  append_lattice_meta(out, "lls reports",
                      member.has_fact ? std::to_string(member.fact.fragment_count)
                                      : std::string("0"));
  append_lattice_meta(out, "contexts",
                      member.has_fact
                          ? std::to_string(member.fact.available_context_count)
                          : std::string("0"));
  append_lattice_meta(
      out, "latest wave cursor",
      member.has_fact ? member.fact.latest_wave_cursor : std::string{});
  append_lattice_meta(
      out, "latest fact time",
      member.has_fact ? format_time_marker_ms(member.fact.latest_ts_ms)
                      : std::string{});
  append_lattice_meta(out, "wave contexts",
                      join_trimmed_values(member.fact.wave_cursors, 6, 16));
  append_lattice_meta(out, "bindings",
                      join_trimmed_values(member.fact.binding_ids, 6, 24));
  append_lattice_meta(out, "taxa",
                      join_trimmed_values(member.fact.semantic_taxa, 6, 24));
  append_lattice_meta(out, "source cursors",
                      join_trimmed_values(member.fact.source_runtime_cursors, 4, 24));
  if (!member.has_fact) {
    append_lattice_text(
        out,
        "  No .lls reports are available yet for this member, so lattice facts and views cannot be materialized from evidence.",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }

  append_lattice_section(out, "Available Views");
  append_lattice_meta(out, "family_evaluation_report",
                      !member.canonical_path.empty() &&
                              !lattice_dock_hash_for_member(member).empty()
                          ? std::string("ready")
                          : std::string("needs canonical_path + dock_hash"),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      !member.canonical_path.empty() &&
                              !lattice_dock_hash_for_member(member).empty()
                          ? cuwacunu::iinuji::text_line_emphasis_t::Success
                          : cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_lattice_meta(out, "entropic_capacity_comparison",
                      !member.fact.latest_wave_cursor.empty()
                          ? std::string("ready")
                          : std::string("needs a wave cursor from a .lls report"),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      !member.fact.latest_wave_cursor.empty()
                          ? cuwacunu::iinuji::text_line_emphasis_t::Success
                          : cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_lattice_meta(
      out, "project coverage",
      std::string("source.data=") +
          (lattice_has_taxon(project.semantic_taxa, "source.data") ? "yes" : "no") +
          " | embedding.network=" +
          (lattice_has_taxon(project.semantic_taxa, "embedding.network") ? "yes"
                                                                         : "no") +
          " | embedding.evaluation=" +
          (lattice_has_taxon(project.semantic_taxa, "embedding.evaluation") ? "yes"
                                                                            : "no"),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Debug);
  if (!trim_copy(member.latest_fact_lls).empty()) {
    append_lattice_section(out, "Latest Fact");
    append_lattice_meta(out, "wave cursor", member.fact.latest_wave_cursor);
    append_lattice_preview_block(out, {}, member.latest_fact_lls);
  }
}

inline void append_lattice_view_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t>& out,
    const CmdState& st, const LatticeViewDefinition* view) {
  append_lattice_section(out, "View");
  if (view == nullptr) {
    append_lattice_text(out,
                        "  No lattice views are available yet. Run the "
                        "lattice catalog refresh or wait for reports to land.",
                        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    return;
  }

  append_lattice_meta(out, "view", view->view_kind,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent,
                      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_lattice_meta(out, "ready", lattice_bool_word(view->ready),
                      cuwacunu::iinuji::text_line_emphasis_t::Info,
                      view->ready
                          ? cuwacunu::iinuji::text_line_emphasis_t::Success
                          : cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_lattice_meta(out, "preferred selector",
                      view->preferred_selector.empty()
                          ? std::string("<none>")
                          : view->preferred_selector);
  append_lattice_meta(out, "required selectors",
                      lattice_join_selector_list(view->required_selectors));
  append_lattice_meta(out, "optional selectors",
                      lattice_join_selector_list(view->optional_selectors));
  append_lattice_meta(out, "reported views",
                      std::to_string(st.lattice.views.size()));
  append_lattice_text(
      out,
      "  Views are derived lattice transports. They become material when the "
      "required selectors are present in the evidence or current selection.",
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (const auto* member = selected_lattice_member_ptr(st); member != nullptr) {
    append_lattice_section(out, "Selectors");
    append_lattice_meta(out, "canonical path", member->canonical_path);
    append_lattice_meta(out, "contract hash", member->contract_hash);
    append_lattice_meta(out, "dock hash", lattice_dock_hash_for_member(*member));
    append_lattice_meta(out, "wave cursor", member->fact.latest_wave_cursor);
    if (st.lattice.view_snapshot.ok &&
        st.lattice.view_snapshot.view_kind == view->view_kind &&
        st.lattice.view_snapshot.canonical_path == member->canonical_path) {
      append_lattice_section(out, "Transport");
      append_lattice_meta(out, "match count",
                          std::to_string(st.lattice.view_snapshot.match_count));
      append_lattice_meta(
          out, "ambiguity count",
          std::to_string(st.lattice.view_snapshot.ambiguity_count));
      append_lattice_preview_block(out, {}, st.lattice.view_snapshot.view_lls);
    } else if (!st.lattice.view_snapshot.error.empty() &&
               st.lattice.view_snapshot.view_kind == view->view_kind &&
               st.lattice.view_snapshot.canonical_path == member->canonical_path) {
      append_lattice_text(out, "  " + st.lattice.view_snapshot.error,
                          cuwacunu::iinuji::text_line_emphasis_t::Warning);
    }
  } else {
    append_lattice_text(out,
                        "  Select a fact member in Lattice/Facts so this view can be "
                        "materialized with real selectors.",
                        cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_lattice_right_styled_lines(const CmdState& st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  if (st.lattice.selected_section == LatticeSection::Overview) {
    append_lattice_overview_detail(out, st);
    return out;
  }

  if (st.lattice.selected_section == LatticeSection::Components) {
    if (const auto* member = selected_component_member_ptr(st); member != nullptr) {
      append_component_member_detail(out, *member);
      return out;
    }
    if (const auto* category = selected_component_category_ptr(st);
        category != nullptr) {
      const auto* root = selected_component_root_ptr(st);
      if (root != nullptr) {
        append_component_category_detail(out, *root, *category, st);
        return out;
      }
    }
    if (const auto* root = selected_component_root_ptr(st); root != nullptr) {
      append_component_root_detail(out, *root);
      return out;
    }
    append_lattice_section(out, "Components");
    append_lattice_text(
        out,
        "  No hashimyei components are cataloged yet. Press r to refresh, or "
        "press Enter here to trigger a catalog refresh from the UI.",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    return out;
  }

  if (st.lattice.mode == LatticeMode::Roots) {
    append_lattice_mode_root_detail(out, st);
    return out;
  }

  if (st.lattice.mode == LatticeMode::Views) {
    append_lattice_view_detail(out, st, selected_lattice_view_ptr(st));
    return out;
  }

  const auto* family = selected_lattice_family_ptr(st);
  const auto* project = selected_lattice_project_ptr(st);
  const auto* member = selected_lattice_member_ptr(st);

  if (member != nullptr && project != nullptr) {
    append_lattice_member_detail(out, *project, *member);
    return out;
  }
  if (project != nullptr) {
    append_lattice_project_detail(out, *project);
    return out;
  }
  if (family != nullptr) {
    append_lattice_family_detail(out, *family);
    return out;
  }

  append_lattice_section(out, "Facts");
  append_lattice_text(
      out,
      st.lattice.rows.empty()
          ? "  No lattice facts are available yet. Components may still exist "
            "in Hashimyei, but lattice evidence has not materialized into "
            "fact bundles."
          : "  Select a family, project, or member from the facts worklist to "
            "inspect it here.",
      st.lattice.rows.empty()
          ? cuwacunu::iinuji::text_line_emphasis_t::Warning
          : cuwacunu::iinuji::text_line_emphasis_t::Info);
  return out;
}

inline std::string make_lattice_right(const CmdState& st) {
  const auto lines = make_lattice_right_styled_lines(st);
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
