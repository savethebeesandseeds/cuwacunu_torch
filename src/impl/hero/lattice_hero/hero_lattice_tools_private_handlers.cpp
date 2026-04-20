using lattice_tool_handler_t = bool (*)(app_context_t*, const std::string&,
                                        std::string*, std::string*);

struct lattice_tool_descriptor_t {
  const char* name;
  const char* description;
  const char* input_schema_json;
  lattice_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_list_facts(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_get_fact(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_list_views(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_get_view(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_browser_tree(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error);
[[nodiscard]] bool handle_tool_browser_detail(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error);
[[nodiscard]] bool handle_tool_refresh(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error);

constexpr lattice_tool_descriptor_t kLatticeTools[] = {
#define HERO_LATTICE_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  lattice_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/lattice_hero/hero_lattice_tools.def"
#undef HERO_LATTICE_TOOL
};

[[nodiscard]] const lattice_tool_descriptor_t* find_lattice_tool_descriptor(
    std::string_view tool_name) {
  for (const auto& descriptor : kLatticeTools) {
    if (tool_name == descriptor.name) return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{"
      << "\"name\":" << json_quote(kServerName) << ","
      << "\"version\":" << json_quote(kServerVersion)
      << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions)
      << "}";
  return out.str();
}

struct fact_bundle_summary_t {
  std::string canonical_path{};
  std::uint64_t wave_cursor{0};
  std::uint64_t latest_ts_ms{0};
  std::size_t fragment_count{0};
  std::set<std::string> binding_ids{};
  std::set<std::string> semantic_taxa{};
  std::set<std::string> source_runtime_cursors{};
};

[[nodiscard]] bool build_fact_bundle_summaries(
    std::string_view canonical_path,
    const std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>& rows,
    std::vector<fact_bundle_summary_t>* out) {
  if (!out) return false;
  out->clear();
  std::map<std::uint64_t, fact_bundle_summary_t> by_wave{};
  for (const auto& row : rows) {
    std::uint64_t row_wave_cursor = 0;
    if (!fragment_wave_cursor(row, &row_wave_cursor)) {
      continue;
    }
    auto& summary = by_wave[row_wave_cursor];
    summary.canonical_path = normalize_source_hashimyei_cursor(canonical_path);
    summary.wave_cursor = row_wave_cursor;
    ++summary.fragment_count;
    if (!row.binding_id.empty()) {
      summary.binding_ids.insert(trim_ascii(row.binding_id));
    }
    if (!row.semantic_taxon.empty()) {
      summary.semantic_taxa.insert(trim_ascii(row.semantic_taxon));
    }
    if (!row.source_runtime_cursor.empty()) {
      summary.source_runtime_cursors.insert(trim_ascii(row.source_runtime_cursor));
    }
    if (row.ts_ms > summary.latest_ts_ms) {
      summary.latest_ts_ms = row.ts_ms;
    }
  }
  out->reserve(by_wave.size());
  for (const auto& [_, summary] : by_wave) {
    out->push_back(summary);
  }
  std::sort(out->begin(), out->end(), [](const auto& a, const auto& b) {
    if (a.latest_ts_ms != b.latest_ts_ms) return a.latest_ts_ms > b.latest_ts_ms;
    return a.wave_cursor > b.wave_cursor;
  });
  return true;
}

[[nodiscard]] bool tool_requires_catalog_sync(std::string_view tool_name) {
  return tool_name == "hero.lattice.list_facts" ||
         tool_name == "hero.lattice.get_fact" ||
         tool_name == "hero.lattice.get_view";
}

[[nodiscard]] bool tool_requires_catalog_preopen(std::string_view tool_name) {
  return tool_name != "hero.lattice.refresh";
}

[[nodiscard]] std::vector<std::string> split_browser_segments(
    std::string_view text) {
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

[[nodiscard]] bool browser_has_hashimyei_suffix(std::string_view canonical_path,
                                                std::string* out_hashimyei =
                                                    nullptr) {
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

[[nodiscard]] std::string browser_project_path_for_canonical(
    std::string_view canonical_path) {
  const std::string normalized = normalize_source_hashimyei_cursor(canonical_path);
  if (!browser_has_hashimyei_suffix(normalized)) return normalized;
  const std::size_t dot = normalized.rfind('.');
  return dot == std::string::npos ? normalized : normalized.substr(0, dot);
}

[[nodiscard]] std::string browser_family_from_canonical(
    std::string_view canonical_path) {
  const auto segments = split_browser_segments(canonical_path);
  if (segments.size() >= 3 && segments[0] == "tsi" && segments[1] == "wikimyei") {
    return segments[2];
  }
  if (segments.size() >= 2 && segments[0] == "tsi") return segments[1];
  if (!segments.empty()) return segments.front();
  return "unknown";
}

[[nodiscard]] std::string browser_display_name_for_member(
    std::string_view canonical_path, std::string_view hashimyei) {
  const std::string normalized_hash = trim_ascii(hashimyei);
  if (!normalized_hash.empty()) return normalized_hash;
  const auto segments =
      split_browser_segments(browser_project_path_for_canonical(canonical_path));
  if (!segments.empty()) return segments.back();
  return std::string(canonical_path);
}

[[nodiscard]] std::uint64_t browser_member_latest_ts_ms(
    const cuwacunu::hero::hashimyei::component_state_t* component,
    const cuwacunu::hero::wave::runtime_fact_summary_t* fact) {
  std::uint64_t ts_ms = 0;
  if (component) ts_ms = std::max(ts_ms, component->ts_ms);
  if (fact) ts_ms = std::max(ts_ms, fact->latest_ts_ms);
  return ts_ms;
}

[[nodiscard]] std::string browser_contract_hash_from_component(
    const cuwacunu::hero::hashimyei::component_state_t& component) {
  return cuwacunu::hero::hashimyei::contract_hash_from_identity(
      component.manifest.contract_identity);
}

[[nodiscard]] std::string browser_dock_hash_from_component(
    const cuwacunu::hero::hashimyei::component_state_t& component) {
  return trim_ascii(component.manifest.docking_signature_sha256_hex);
}

void append_unique_browser_string(std::string_view value,
                                  std::vector<std::string>* values) {
  if (!values) return;
  const std::string token = trim_ascii(value);
  if (token.empty()) return;
  if (std::find(values->begin(), values->end(), token) != values->end()) return;
  values->push_back(token);
}

struct browser_member_row_t {
  std::string canonical_path{};
  std::string project_path{};
  std::string family{};
  std::string display_name{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string dock_hash{};
  std::string lineage_state{"unknown"};
  bool has_component{false};
  cuwacunu::hero::hashimyei::component_state_t component{};
  bool has_fact{false};
  cuwacunu::hero::wave::runtime_fact_summary_t fact{};
  std::optional<std::uint64_t> family_rank{};
};

struct browser_project_row_t {
  std::string project_path{};
  std::string family{};
  std::vector<browser_member_row_t> members{};
  std::size_t fragment_count{0};
  std::size_t available_context_count{0};
  std::uint64_t latest_ts_ms{0};
  std::vector<std::string> wave_cursors{};
  std::vector<std::string> binding_ids{};
  std::vector<std::string> semantic_taxa{};
  std::vector<std::string> source_runtime_cursors{};
  std::vector<std::string> report_schemas{};
  std::vector<std::string> contract_hashes{};
};

struct browser_family_row_t {
  std::string family{};
  std::vector<browser_project_row_t> projects{};
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

[[nodiscard]] bool open_hashimyei_catalog_if_needed(app_context_t* app,
                                                    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->hashimyei_catalog.opened()) return true;
  if (app->hashimyei_catalog_path.empty()) {
    if (out_error) *out_error = "hashimyei catalog path is empty";
    return false;
  }
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t opts{};
  opts.catalog_path = app->hashimyei_catalog_path;
  opts.encrypted = false;
  return app->hashimyei_catalog.open(opts, out_error);
}

[[nodiscard]] bool close_hashimyei_catalog_if_open(app_context_t* app,
                                                   std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !app->hashimyei_catalog.opened()) return true;
  return app->hashimyei_catalog.close(out_error);
}

void finalize_browser_member_row(browser_member_row_t* member) {
  if (!member) return;
  if (member->project_path.empty()) {
    member->project_path = browser_project_path_for_canonical(member->canonical_path);
  }
  if (member->family.empty()) {
    member->family = browser_family_from_canonical(member->canonical_path);
  }
  if (member->hashimyei.empty()) {
    std::string hashimyei{};
    if (browser_has_hashimyei_suffix(member->canonical_path, &hashimyei)) {
      member->hashimyei = std::move(hashimyei);
    }
  }
  if (member->display_name.empty()) {
    member->display_name =
        browser_display_name_for_member(member->canonical_path, member->hashimyei);
  }
  if (member->has_component && member->lineage_state == "unknown" &&
      !member->component.manifest.lineage_state.empty()) {
    member->lineage_state = member->component.manifest.lineage_state;
  }
  if (!member->has_component && member->has_fact) {
    member->lineage_state = "fact-only";
  }
}

[[nodiscard]] bool build_browser_tree_rows(
    app_context_t* app, std::vector<browser_family_row_t>* out,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out) return false;
  out->clear();

  if (!open_hashimyei_catalog_if_needed(app, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->hashimyei_catalog.list_component_heads(0, 0, true, &components,
                                                   out_error)) {
    return false;
  }

  std::vector<cuwacunu::hero::wave::runtime_fact_summary_t> facts{};
  if (!app->catalog.list_runtime_fact_summaries(0, 0, true, &facts, out_error)) {
    return false;
  }

  std::unordered_map<std::string, browser_member_row_t> members_by_canonical{};
  members_by_canonical.reserve(components.size() + facts.size());

  for (const auto& component : components) {
    const std::string canonical_path =
        normalize_source_hashimyei_cursor(component.manifest.canonical_path);
    if (canonical_path.empty()) continue;
    auto& member = members_by_canonical[canonical_path];
    member.canonical_path = canonical_path;
    member.has_component = true;
    member.component = component;
    member.family = component.manifest.family.empty()
                        ? browser_family_from_canonical(canonical_path)
                        : component.manifest.family;
    member.project_path = browser_project_path_for_canonical(canonical_path);
    member.hashimyei = component.manifest.hashimyei_identity.name;
    member.contract_hash = browser_contract_hash_from_component(component);
    member.dock_hash = browser_dock_hash_from_component(component);
    member.family_rank = component.family_rank;
    member.lineage_state = component.manifest.lineage_state.empty()
                               ? std::string("unknown")
                               : component.manifest.lineage_state;
    finalize_browser_member_row(&member);
  }

  for (const auto& fact : facts) {
    if (fact.canonical_path.empty()) continue;
    auto& member = members_by_canonical[fact.canonical_path];
    member.canonical_path = fact.canonical_path;
    member.has_fact = true;
    member.fact = fact;
    finalize_browser_member_row(&member);
  }

  std::map<std::string, std::map<std::string, std::vector<browser_member_row_t>>>
      grouped{};
  for (auto& [canonical_path, member] : members_by_canonical) {
    (void)canonical_path;
    finalize_browser_member_row(&member);
    grouped[member.family][member.project_path].push_back(std::move(member));
  }

  out->reserve(grouped.size());
  for (auto& [family_name, projects_map] : grouped) {
    browser_family_row_t family{};
    family.family = family_name;
    for (auto& [project_path, members] : projects_map) {
      std::sort(members.begin(), members.end(),
                [](const browser_member_row_t& a, const browser_member_row_t& b) {
                  if (a.family_rank.has_value() && b.family_rank.has_value() &&
                      a.family_rank != b.family_rank) {
                    return *a.family_rank < *b.family_rank;
                  }
                  if (a.family_rank.has_value() != b.family_rank.has_value()) {
                    return a.family_rank.has_value();
                  }
                  const std::uint64_t a_ts = browser_member_latest_ts_ms(
                      a.has_component ? &a.component : nullptr,
                      a.has_fact ? &a.fact : nullptr);
                  const std::uint64_t b_ts = browser_member_latest_ts_ms(
                      b.has_component ? &b.component : nullptr,
                      b.has_fact ? &b.fact : nullptr);
                  if (a_ts != b_ts) return a_ts > b_ts;
                  return a.display_name < b.display_name;
                });

      browser_project_row_t project{};
      project.project_path = project_path;
      project.family = family_name;
      project.members = std::move(members);
      for (const auto& member : project.members) {
        const std::uint64_t member_ts = browser_member_latest_ts_ms(
            member.has_component ? &member.component : nullptr,
            member.has_fact ? &member.fact : nullptr);
        project.latest_ts_ms = std::max(project.latest_ts_ms, member_ts);
        family.latest_ts_ms = std::max(family.latest_ts_ms, member_ts);
        ++family.member_count;
        if (member.lineage_state == "active") ++family.active_member_count;
        if (member.has_fact) {
          project.fragment_count += member.fact.fragment_count;
          family.fragment_count += member.fact.fragment_count;
          for (const auto wave_cursor : member.fact.wave_cursors) {
            append_unique_browser_string(
                cuwacunu::hero::wave::lattice_catalog_store_t::
                    format_runtime_wave_cursor(wave_cursor),
                &project.wave_cursors);
            append_unique_browser_string(
                cuwacunu::hero::wave::lattice_catalog_store_t::
                    format_runtime_wave_cursor(wave_cursor),
                &family.wave_cursors);
          }
          for (const auto& value : member.fact.binding_ids) {
            append_unique_browser_string(value, &project.binding_ids);
            append_unique_browser_string(value, &family.binding_ids);
          }
          for (const auto& value : member.fact.semantic_taxa) {
            append_unique_browser_string(value, &project.semantic_taxa);
            append_unique_browser_string(value, &family.semantic_taxa);
          }
          for (const auto& value : member.fact.source_runtime_cursors) {
            append_unique_browser_string(value, &project.source_runtime_cursors);
          }
          for (const auto& value : member.fact.report_schemas) {
            append_unique_browser_string(value, &project.report_schemas);
          }
        }
        if (!member.contract_hash.empty()) {
          append_unique_browser_string(member.contract_hash, &project.contract_hashes);
          append_unique_browser_string(member.contract_hash, &family.contract_hashes);
        }
      }
      project.available_context_count = project.wave_cursors.size();
      family.available_context_count = family.wave_cursors.size();
      std::sort(project.wave_cursors.begin(), project.wave_cursors.end());
      std::sort(project.binding_ids.begin(), project.binding_ids.end());
      std::sort(project.semantic_taxa.begin(), project.semantic_taxa.end());
      std::sort(project.source_runtime_cursors.begin(),
                project.source_runtime_cursors.end());
      std::sort(project.report_schemas.begin(), project.report_schemas.end());
      std::sort(project.contract_hashes.begin(), project.contract_hashes.end());
      family.projects.push_back(std::move(project));
    }

    std::sort(family.projects.begin(), family.projects.end(),
              [](const browser_project_row_t& a, const browser_project_row_t& b) {
                if (a.latest_ts_ms != b.latest_ts_ms) {
                  return a.latest_ts_ms > b.latest_ts_ms;
                }
                return a.project_path < b.project_path;
              });
    std::sort(family.wave_cursors.begin(), family.wave_cursors.end());
    std::sort(family.binding_ids.begin(), family.binding_ids.end());
    std::sort(family.semantic_taxa.begin(), family.semantic_taxa.end());
    std::sort(family.contract_hashes.begin(), family.contract_hashes.end());
    family.available_context_count = family.wave_cursors.size();
    out->push_back(std::move(family));
  }

  std::sort(out->begin(), out->end(),
            [](const browser_family_row_t& a, const browser_family_row_t& b) {
              if (a.latest_ts_ms != b.latest_ts_ms) return a.latest_ts_ms > b.latest_ts_ms;
              return a.family < b.family;
            });
  return true;
}

[[nodiscard]] std::string browser_member_summary_json(
    const browser_member_row_t& member) {
  std::ostringstream out;
  out << "{"
      << "\"canonical_path\":" << json_quote(member.canonical_path)
      << ",\"project_path\":" << json_quote(member.project_path)
      << ",\"family\":" << json_quote(member.family)
      << ",\"display_name\":" << json_quote(member.display_name)
      << ",\"hashimyei\":"
      << (member.hashimyei.empty() ? "null" : json_quote(member.hashimyei))
      << ",\"contract_hash\":"
      << (member.contract_hash.empty() ? "null" : json_quote(member.contract_hash))
      << ",\"dock_hash\":"
      << (member.dock_hash.empty() ? "null" : json_quote(member.dock_hash))
      << ",\"lineage_state\":" << json_quote(member.lineage_state)
      << ",\"has_component\":" << (member.has_component ? "true" : "false")
      << ",\"has_fact\":" << (member.has_fact ? "true" : "false")
      << ",\"family_rank\":";
  if (member.family_rank.has_value()) {
    out << *member.family_rank;
  } else {
    out << "null";
  }
  out << ",\"latest_ts_ms\":"
      << (member.has_fact ? member.fact.latest_ts_ms : 0)
      << ",\"fragment_count\":"
      << (member.has_fact ? member.fact.fragment_count : 0)
      << ",\"available_context_count\":"
      << (member.has_fact ? member.fact.available_context_count : 0)
      << ",\"latest_wave_cursor\":";
  if (member.has_fact && member.fact.latest_wave_cursor != 0) {
    out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                          format_runtime_wave_cursor(member.fact.latest_wave_cursor));
  } else {
    out << "null";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string browser_project_tree_json(
    const browser_project_row_t& project) {
  std::ostringstream out;
  out << "{"
      << "\"project_path\":" << json_quote(project.project_path)
      << ",\"family\":" << json_quote(project.family)
      << ",\"member_count\":" << project.members.size()
      << ",\"active_member_count\":"
      << std::count_if(project.members.begin(), project.members.end(),
                       [](const browser_member_row_t& member) {
                         return member.lineage_state == "active";
                       })
      << ",\"fragment_count\":" << project.fragment_count
      << ",\"available_context_count\":" << project.available_context_count
      << ",\"latest_ts_ms\":" << project.latest_ts_ms
      << ",\"wave_cursors\":"
      << json_string_array_from_vector(project.wave_cursors)
      << ",\"binding_ids\":"
      << json_string_array_from_vector(project.binding_ids)
      << ",\"semantic_taxa\":"
      << json_string_array_from_vector(project.semantic_taxa)
      << ",\"source_runtime_cursors\":"
      << json_string_array_from_vector(project.source_runtime_cursors)
      << ",\"report_schemas\":"
      << json_string_array_from_vector(project.report_schemas)
      << ",\"contract_hashes\":"
      << json_string_array_from_vector(project.contract_hashes)
      << ",\"members\":[";
  for (std::size_t i = 0; i < project.members.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_member_summary_json(project.members[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string browser_family_tree_json(
    const browser_family_row_t& family) {
  std::ostringstream out;
  out << "{"
      << "\"family\":" << json_quote(family.family)
      << ",\"member_count\":" << family.member_count
      << ",\"active_member_count\":" << family.active_member_count
      << ",\"fragment_count\":" << family.fragment_count
      << ",\"available_context_count\":" << family.available_context_count
      << ",\"latest_ts_ms\":" << family.latest_ts_ms
      << ",\"wave_cursors\":"
      << json_string_array_from_vector(family.wave_cursors)
      << ",\"binding_ids\":"
      << json_string_array_from_vector(family.binding_ids)
      << ",\"semantic_taxa\":"
      << json_string_array_from_vector(family.semantic_taxa)
      << ",\"contract_hashes\":"
      << json_string_array_from_vector(family.contract_hashes)
      << ",\"projects\":[";
  for (std::size_t i = 0; i < family.projects.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_project_tree_json(family.projects[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string browser_recent_fragments_json(
    const std::vector<cuwacunu::hero::wave::runtime_report_fragment_t>& fragments) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < fragments.size(); ++i) {
    if (i != 0) out << ",";
    const auto& fragment = fragments[i];
    out << "{"
        << "\"report_fragment_id\":" << json_quote(fragment.report_fragment_id)
        << ",\"schema\":" << json_quote(fragment.schema)
        << ",\"semantic_taxon\":"
        << (fragment.semantic_taxon.empty() ? "null"
                                            : json_quote(fragment.semantic_taxon))
        << ",\"wave_cursor\":";
    if (fragment.wave_cursor != 0) {
      out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                            format_runtime_wave_cursor(fragment.wave_cursor));
    } else {
      out << "null";
    }
    out << ",\"ts_ms\":" << fragment.ts_ms << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool handle_tool_list_facts(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);

  std::size_t limit = 20;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  std::ostringstream facts_json;
  facts_json << "[";
  std::size_t emitted = 0;
  std::size_t total = 0;

  if (canonical_path.empty()) {
    std::vector<cuwacunu::hero::wave::runtime_fact_summary_t> facts{};
    if (!app->catalog.list_runtime_fact_summaries(0, 0, true, &facts, out_error)) {
      return false;
    }
    total = facts.size();
    const std::size_t off = std::min(offset, facts.size());
    std::size_t count = limit;
    if (count == 0) count = facts.size() - off;
    count = std::min(count, facts.size() - off);
    for (std::size_t i = 0; i < count; ++i) {
      const auto& fact = facts[off + i];
      if (emitted != 0) facts_json << ",";
      ++emitted;
      facts_json << "{"
                 << "\"canonical_path\":" << json_quote(fact.canonical_path)
                 << ",\"latest_wave_cursor\":"
                 << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                   format_runtime_wave_cursor(
                                       fact.latest_wave_cursor))
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"binding_ids\":"
                 << json_string_array_from_vector(fact.binding_ids)
                 << ",\"semantic_taxa\":"
                 << json_string_array_from_vector(fact.semantic_taxa)
                 << ",\"source_runtime_cursors\":"
                 << json_string_array_from_vector(fact.source_runtime_cursors)
                 << ",\"available_context_count\":" << fact.available_context_count
                 << ",\"fragment_count\":" << fact.fragment_count << "}";
    }
  } else {
    std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> rows{};
    if (!app->catalog.list_runtime_report_fragments(canonical_path, "", 0, 0,
                                                    true, &rows, out_error)) {
      return false;
    }
    std::vector<fact_bundle_summary_t> facts{};
    if (!build_fact_bundle_summaries(canonical_path, rows, &facts)) {
      *out_error = "failed to summarize available fact bundles";
      return false;
    }
    total = facts.size();
    const std::size_t off = std::min(offset, facts.size());
    std::size_t count = limit;
    if (count == 0) count = facts.size() - off;
    count = std::min(count, facts.size() - off);
    for (std::size_t i = 0; i < count; ++i) {
      const auto& fact = facts[off + i];
      if (emitted != 0) facts_json << ",";
      ++emitted;
      facts_json << "{"
                 << "\"canonical_path\":" << json_quote(fact.canonical_path)
                 << ",\"wave_cursor\":"
                 << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                   format_runtime_wave_cursor(fact.wave_cursor))
                 << ",\"latest_ts_ms\":" << fact.latest_ts_ms
                 << ",\"binding_ids\":"
                 << json_string_array_from_set(fact.binding_ids)
                 << ",\"semantic_taxa\":"
                 << json_string_array_from_set(fact.semantic_taxa)
                 << ",\"source_runtime_cursors\":"
                 << json_string_array_from_set(fact.source_runtime_cursors)
                 << ",\"fragment_count\":" << fact.fragment_count << "}";
    }
  }
  facts_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":"
      << (canonical_path.empty() ? "null" : json_quote(canonical_path))
      << ",\"count\":" << emitted
      << ",\"total\":" << total
      << ",\"facts\":" << facts_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_fact(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  if (canonical_path.empty()) {
    *out_error = "get_fact requires arguments.canonical_path";
    return false;
  }

  std::uint64_t selected_wave_cursor = 0;
  const bool use_wave_cursor =
      extract_json_wave_cursor_field(arguments_json, "wave_cursor",
                                    &selected_wave_cursor);

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> rows{};
  if (!app->catalog.list_runtime_report_fragments(canonical_path, "", 0, 0, true,
                                                  &rows, out_error)) {
    return false;
  }
  if (rows.empty()) {
    *out_error = "fact not found for requested canonical_path";
    return false;
  }

  if (!use_wave_cursor) {
    bool found_latest = false;
    for (const auto& row : rows) {
      if (fragment_wave_cursor(row, &selected_wave_cursor)) {
        found_latest = true;
        break;
      }
    }
    if (!found_latest) {
      *out_error = "fact rows do not carry a valid wave_cursor";
      return false;
    }
  }

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> selected{};
  for (const auto& row : rows) {
    std::uint64_t row_wave_cursor = 0;
    if (!fragment_wave_cursor(row, &row_wave_cursor)) {
      continue;
    }
    if (row_wave_cursor != selected_wave_cursor) continue;
    selected.push_back(row);
  }
  if (selected.empty()) {
    *out_error = "fact bundle not found for requested selector";
    return false;
  }

  const std::string fact_text = build_joined_report_lls(canonical_path, selected);
  std::uint64_t latest_ts_ms = selected.front().ts_ms;
  std::set<std::string> binding_ids{};
  std::set<std::string> semantic_taxa{};
  std::set<std::string> source_runtime_cursors{};
  for (const auto& row : selected) {
    if (row.ts_ms > latest_ts_ms) {
      latest_ts_ms = row.ts_ms;
    }
    if (!row.binding_id.empty()) {
      binding_ids.insert(trim_ascii(row.binding_id));
    }
    if (!row.semantic_taxon.empty()) {
      semantic_taxa.insert(trim_ascii(row.semantic_taxon));
    }
    if (!row.source_runtime_cursor.empty()) {
      source_runtime_cursors.insert(trim_ascii(row.source_runtime_cursor));
    }
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"selection_mode\":" << json_quote(use_wave_cursor ? "historical" : "latest")
      << ",\"wave_cursor\":"
      << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                        format_runtime_wave_cursor(selected_wave_cursor))
      << ",\"latest_ts_ms\":" << latest_ts_ms
      << ",\"binding_ids\":" << json_string_array_from_set(binding_ids)
      << ",\"semantic_taxa\":" << json_string_array_from_set(semantic_taxa)
      << ",\"source_runtime_cursors\":"
      << json_string_array_from_set(source_runtime_cursors)
      << ",\"fragment_count\":" << selected.size()
      << ",\"fact_lls\":" << json_quote(fact_text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_list_views(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();
  (void)arguments_json;

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> source_rows{};
  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> network_rows{};
  if (!app->catalog.list_runtime_report_fragments(
          "tsi.source.dataloader", "piaabo.torch_compat.data_analytics.v2", 1,
          0, true, &source_rows, out_error)) {
    return false;
  }
  if (!app->catalog.list_runtime_report_fragments(
          "tsi.wikimyei.representation.vicreg",
          "piaabo.torch_compat.network_analytics.v5", 1, 0, true, &network_rows,
          out_error)) {
    return false;
  }

  std::ostringstream out;
  const bool has_family_reports = !network_rows.empty();
  out << "{\"count\":2,\"views\":[{"
      << "\"view_kind\":\"entropic_capacity_comparison\""
      << ",\"preferred_selector\":\"wave_cursor\""
      << ",\"required_selectors\":[\"wave_cursor\"]"
      << ",\"optional_selectors\":[\"canonical_path\",\"contract_hash\"]"
      << ",\"ready\":"
      << ((!source_rows.empty() && !network_rows.empty()) ? "true" : "false")
      << "},{"
      << "\"view_kind\":\"family_evaluation_report\""
      << ",\"preferred_selector\":\"canonical_path\""
      << ",\"required_selectors\":[\"canonical_path\",\"dock_hash\"]"
      << ",\"optional_selectors\":[\"wave_cursor\"]"
      << ",\"ready\":" << (has_family_reports ? "true" : "false")
      << "}]}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_view(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string view_kind{};
  std::string canonical_path{};
  std::string contract_hash{};
  std::string dock_hash{};
  (void)extract_json_string_field(arguments_json, "view_kind", &view_kind);
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "contract_hash",
                                  &contract_hash);
  (void)extract_json_string_field(arguments_json, "dock_hash", &dock_hash);
  view_kind = trim_ascii(view_kind);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  contract_hash = trim_ascii(contract_hash);
  dock_hash = trim_ascii(dock_hash);
  if (view_kind.empty()) {
    *out_error = "get_view requires arguments.view_kind";
    return false;
  }

  std::uint64_t wave_cursor = 0;
  const bool use_wave_cursor =
      extract_json_wave_cursor_field(arguments_json, "wave_cursor",
                                    &wave_cursor);
  std::string internal_intersection_cursor{};
  if (!canonical_path.empty() && use_wave_cursor) {
    internal_intersection_cursor =
        canonical_path + "|" +
        cuwacunu::hero::wave::lattice_catalog_store_t::format_runtime_wave_cursor(
            wave_cursor);
  } else if (!canonical_path.empty() &&
             view_kind == "family_evaluation_report") {
    internal_intersection_cursor = canonical_path;
  }

  cuwacunu::hero::wave::runtime_view_report_t view{};
  if (!app->catalog.get_runtime_view_lls(
          view_kind, internal_intersection_cursor, wave_cursor, use_wave_cursor,
          contract_hash, dock_hash, &view, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(view.canonical_path)
      << ",\"view_kind\":" << json_quote(view.view_kind)
      << ",\"selector_canonical_path\":"
      << (view.selector_hashimyei_cursor.empty()
              ? "null"
              : json_quote(view.selector_hashimyei_cursor))
      << ",\"contract_hash\":"
      << (view.contract_hash.empty() ? "null" : json_quote(view.contract_hash))
      << ",\"dock_hash\":"
      << (view.dock_hash.empty() ? "null" : json_quote(view.dock_hash))
      << ",\"wave_cursor\":"
      << (view.has_wave_cursor
              ? json_quote(
                    cuwacunu::hero::wave::lattice_catalog_store_t::
                        format_runtime_wave_cursor(view.wave_cursor))
              : "null")
      << ",\"match_count\":" << view.match_count
      << ",\"ambiguity_count\":" << view.ambiguity_count
      << ",\"view_lls\":" << json_quote(view.view_lls) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_browser_tree(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();
  (void)arguments_json;

  std::vector<browser_family_row_t> families{};
  if (!build_browser_tree_rows(app, &families, out_error)) return false;

  std::size_t project_count = 0;
  std::size_t member_count = 0;
  for (const auto& family : families) {
    project_count += family.projects.size();
    member_count += family.member_count;
  }

  std::ostringstream out;
  out << "{\"family_count\":" << families.size()
      << ",\"project_count\":" << project_count
      << ",\"member_count\":" << member_count
      << ",\"families\":[";
  for (std::size_t i = 0; i < families.size(); ++i) {
    if (i != 0) out << ",";
    out << browser_family_tree_json(families[i]);
  }
  out << "]}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_browser_detail(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string kind{};
  std::string family_name{};
  std::string project_path{};
  std::string canonical_path{};
  (void)extract_json_string_field(arguments_json, "kind", &kind);
  (void)extract_json_string_field(arguments_json, "family", &family_name);
  (void)extract_json_string_field(arguments_json, "project_path", &project_path);
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  kind = trim_ascii(kind);
  family_name = trim_ascii(family_name);
  project_path = normalize_source_hashimyei_cursor(project_path);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  if (kind.empty()) {
    *out_error = "browser_detail requires arguments.kind";
    return false;
  }

  std::vector<browser_family_row_t> families{};
  if (!build_browser_tree_rows(app, &families, out_error)) return false;

  if (kind == "family") {
    for (const auto& family : families) {
      if (family.family != family_name) continue;
      std::vector<std::string> project_paths{};
      project_paths.reserve(family.projects.size());
      for (const auto& project : family.projects) {
        project_paths.push_back(project.project_path);
      }
      std::ostringstream out;
      out << "{\"kind\":\"family\""
          << ",\"family\":" << json_quote(family.family)
          << ",\"project_count\":" << family.projects.size()
          << ",\"member_count\":" << family.member_count
          << ",\"active_member_count\":" << family.active_member_count
          << ",\"fragment_count\":" << family.fragment_count
          << ",\"available_context_count\":" << family.available_context_count
          << ",\"latest_ts_ms\":" << family.latest_ts_ms
          << ",\"wave_cursors\":"
          << json_string_array_from_vector(family.wave_cursors)
          << ",\"binding_ids\":"
          << json_string_array_from_vector(family.binding_ids)
          << ",\"semantic_taxa\":"
          << json_string_array_from_vector(family.semantic_taxa)
          << ",\"contract_hashes\":"
          << json_string_array_from_vector(family.contract_hashes)
          << ",\"project_paths\":"
          << json_string_array_from_vector(project_paths) << "}";
      *out_structured = out.str();
      return true;
    }
    *out_error = "browser_detail family not found";
    return false;
  }

  if (kind == "project") {
    for (const auto& family : families) {
      for (const auto& project : family.projects) {
        if (project.project_path != project_path) continue;
        std::vector<std::string> member_paths{};
        member_paths.reserve(project.members.size());
        for (const auto& member : project.members) {
          member_paths.push_back(member.canonical_path);
        }
        std::ostringstream out;
        out << "{\"kind\":\"project\""
            << ",\"family\":" << json_quote(project.family)
            << ",\"project_path\":" << json_quote(project.project_path)
            << ",\"member_count\":" << project.members.size()
            << ",\"active_member_count\":"
            << std::count_if(project.members.begin(), project.members.end(),
                             [](const browser_member_row_t& member) {
                               return member.lineage_state == "active";
                             })
            << ",\"fragment_count\":" << project.fragment_count
            << ",\"available_context_count\":" << project.available_context_count
            << ",\"latest_ts_ms\":" << project.latest_ts_ms
            << ",\"wave_cursors\":"
            << json_string_array_from_vector(project.wave_cursors)
            << ",\"binding_ids\":"
            << json_string_array_from_vector(project.binding_ids)
            << ",\"semantic_taxa\":"
            << json_string_array_from_vector(project.semantic_taxa)
            << ",\"source_runtime_cursors\":"
            << json_string_array_from_vector(project.source_runtime_cursors)
            << ",\"report_schemas\":"
            << json_string_array_from_vector(project.report_schemas)
            << ",\"contract_hashes\":"
            << json_string_array_from_vector(project.contract_hashes)
            << ",\"member_paths\":"
            << json_string_array_from_vector(member_paths) << "}";
        *out_structured = out.str();
        return true;
      }
    }
    *out_error = "browser_detail project not found";
    return false;
  }

  if (kind == "member") {
    for (const auto& family : families) {
      for (const auto& project : family.projects) {
        for (const auto& member : project.members) {
          if (member.canonical_path != canonical_path) continue;
          std::ostringstream out;
          out << "{"
              << "\"kind\":\"member\""
              << ",\"family\":" << json_quote(member.family)
              << ",\"project_path\":" << json_quote(member.project_path)
              << ",\"canonical_path\":" << json_quote(member.canonical_path)
              << ",\"display_name\":" << json_quote(member.display_name)
              << ",\"hashimyei\":"
              << (member.hashimyei.empty() ? "null" : json_quote(member.hashimyei))
              << ",\"contract_hash\":"
              << (member.contract_hash.empty() ? "null"
                                              : json_quote(member.contract_hash))
              << ",\"lineage_state\":" << json_quote(member.lineage_state)
              << ",\"family_rank\":";
          if (member.family_rank.has_value()) {
            out << *member.family_rank;
          } else {
            out << "null";
          }
          out << ",\"has_component\":" << (member.has_component ? "true" : "false")
              << ",\"has_fact\":" << (member.has_fact ? "true" : "false")
              << ",\"parent_hashimyei\":";
          if (member.has_component &&
              member.component.manifest.parent_identity.has_value() &&
              !member.component.manifest.parent_identity->name.empty()) {
            out << json_quote(member.component.manifest.parent_identity->name);
          } else {
            out << "null";
          }
          out << ",\"replaced_by\":";
          if (member.has_component &&
              !trim_ascii(member.component.manifest.replaced_by).empty()) {
            out << json_quote(member.component.manifest.replaced_by);
          } else {
            out << "null";
          }
          out << ",\"fragment_count\":"
              << (member.has_fact ? member.fact.fragment_count : 0)
              << ",\"available_context_count\":"
              << (member.has_fact ? member.fact.available_context_count : 0)
              << ",\"latest_ts_ms\":"
              << (member.has_fact ? member.fact.latest_ts_ms : 0)
              << ",\"latest_wave_cursor\":";
          if (member.has_fact && member.fact.latest_wave_cursor != 0) {
            out << json_quote(cuwacunu::hero::wave::lattice_catalog_store_t::
                                  format_runtime_wave_cursor(
                                      member.fact.latest_wave_cursor));
          } else {
            out << "null";
          }
          out << ",\"wave_cursors\":";
          if (member.has_fact) {
            out << json_wave_cursor_array_from_vector(member.fact.wave_cursors);
          } else {
            out << "[]";
          }
          out << ",\"binding_ids\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.binding_ids);
          } else {
            out << "[]";
          }
          out << ",\"semantic_taxa\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.semantic_taxa);
          } else {
            out << "[]";
          }
          out << ",\"source_runtime_cursors\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.source_runtime_cursors);
          } else {
            out << "[]";
          }
          out << ",\"report_schemas\":";
          if (member.has_fact) {
            out << json_string_array_from_vector(member.fact.report_schemas);
          } else {
            out << "[]";
          }
          out << ",\"recent_fragments\":";
          if (member.has_fact) {
            out << browser_recent_fragments_json(member.fact.recent_fragments);
          } else {
            out << "[]";
          }
          out << "}";
          *out_structured = out.str();
          return true;
        }
      }
    }
    *out_error = "browser_detail member not found";
    return false;
  }

  *out_error = "browser_detail kind must be family, project, or member";
  return false;
}

[[nodiscard]] bool handle_tool_refresh(app_context_t* app,
                                       const std::string& arguments_json,
                                       std::string* out_structured,
                                       std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  bool reingest = true;
  (void)extract_json_bool_field(arguments_json, "reingest", &reingest);
  if (!reset_lattice_catalog(app, reingest, out_error)) return false;

  std::vector<cuwacunu::hero::wave::runtime_report_fragment_t> report_fragments{};
  if (!app->catalog.list_runtime_report_fragments(
          "", "", 0, 0, true, &report_fragments, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"catalog_path\":" << json_quote(app->lattice_catalog_path.string())
      << ",\"reingest\":" << (reingest ? "true" : "false")
      << ",\"runtime_report_fragment_count\":" << report_fragments.size()
      << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kLatticeTools); ++i) {
    if (i != 0) out << ",";
    const auto& descriptor = kLatticeTools[i];
    const bool read_only =
        std::string_view(descriptor.name) != "hero.lattice.refresh";
    out << "{\"name\":" << json_quote(descriptor.name)
        << ",\"description\":" << json_quote(descriptor.description)
        << ",\"inputSchema\":" << descriptor.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (read_only ? "true" : "false")
        << ",\"destructiveHint\":false,\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto& descriptor : kLatticeTools) {
    out << descriptor.name << "\t" << descriptor.description << "\n";
  }
  return out.str();
}

[[nodiscard]] bool open_lattice_catalog_if_needed(app_context_t* app,
                                                  std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->catalog.opened()) return true;
  if (app->lattice_catalog_path.empty()) {
    if (out_error) *out_error = "lattice catalog path is empty";
    return false;
  }
  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  std::error_code ec{};
  opts.read_only = std::filesystem::exists(app->lattice_catalog_path, ec) && !ec;
  opts.projection_version = 2;
  opts.runtime_only_indexes = true;
  return app->catalog.open(opts, out_error);
}

[[nodiscard]] bool close_lattice_catalog_if_open(app_context_t* app,
                                                 std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) return true;
  if (app->hashimyei_catalog.opened()) {
    std::string hashimyei_error{};
    if (!close_hashimyei_catalog_if_open(app, &hashimyei_error)) {
      if (out_error) *out_error = "hashimyei catalog close failed: " + hashimyei_error;
      return false;
    }
  }
  if (!app->catalog.opened()) return true;
  return app->catalog.close(out_error);
}

[[nodiscard]] bool handle_tool_call(app_context_t* app,
                                    const std::string& tool_name,
                                    const std::string& arguments_json,
                                    std::string* out_result_json,
                                    std::string* out_error_json) {
  if (!app || !out_result_json || !out_error_json) return false;
  out_result_json->clear();
  out_error_json->clear();

  const auto* descriptor = find_lattice_tool_descriptor(tool_name);
  if (!descriptor) {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }
  if (tool_requires_catalog_sync(tool_name)) {
    std::string sync_error;
    if (!ensure_lattice_catalog_synced_to_store(app, false, &sync_error)) {
      *out_error_json = "catalog sync failed: " + sync_error;
      return false;
    }
  }

  std::string structured;
  std::string err;
  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    const std::string fallback =
        "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
    *out_result_json = make_tool_result_json(err, fallback, true);
    return true;
  }

  *out_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

void run_jsonrpc_stdio_loop_impl(app_context_t* app) {
  if (!app) return;

  bool shutdown_requested = false;
  for (;;) {
    if (!wait_for_stdio_activity()) return;

    std::string message;
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &message, &used_content_length)) {
      return;
    }
    if (message.empty()) continue;
    if (used_content_length) g_jsonrpc_use_content_length_framing = true;

    std::string method;
    if (!extract_json_string_field(message, "method", &method)) {
      write_jsonrpc_error("null", -32600, "missing method");
      continue;
    }

    if (method == "exit") return;

    std::string id_json = "null";
    const bool has_id = extract_json_id_field(message, &id_json);
    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (!has_id) {
      write_jsonrpc_error("null", -32700, "invalid JSON-RPC id");
      continue;
    }
    if (method == "shutdown") {
      write_jsonrpc_result(id_json, "{}");
      shutdown_requested = true;
      continue;
    }
    if (shutdown_requested) {
      write_jsonrpc_error(id_json, -32000, "server is shutting down");
      continue;
    }
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "ping") {
      write_jsonrpc_result(id_json, "{}");
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, build_tools_list_result_json());
      continue;
    }
    if (method == "resources/list") {
      write_jsonrpc_result(id_json, "{\"resources\":[]}");
      continue;
    }
    if (method == "resources/templates/list") {
      write_jsonrpc_result(id_json, "{\"resourceTemplates\":[]}");
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "method not found");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(message, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }

    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }

    std::string arguments = "{}";
    (void)extract_json_object_field(params, "arguments", &arguments);

    const bool catalog_already_open = app->catalog.opened();
    const std::uint64_t tool_started_at_ms = now_ms_utc();
    std::uint64_t catalog_hold_started_at_ms = 0;
    log_lattice_tool_begin(app, name, catalog_already_open);

    const bool needs_preopen = tool_requires_catalog_preopen(name);
    std::string open_error;
    if (needs_preopen && !open_lattice_catalog_if_needed(app, &open_error)) {
      const std::string err = "catalog open failed: " + open_error;
      log_lattice_tool_end(app, name, catalog_already_open, false, false,
                           now_ms_utc() - tool_started_at_ms, 0, err);
      const std::string fallback =
          "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
      write_jsonrpc_result(id_json, make_tool_result_json(err, fallback, true));
      continue;
    }
    if (!catalog_already_open && needs_preopen) {
      catalog_hold_started_at_ms = now_ms_utc();
    }

    std::string tool_result;
    std::string tool_error;
    const bool ok =
        handle_tool_call(app, name, arguments, &tool_result, &tool_error);
    std::string close_error;
    if (!close_lattice_catalog_if_open(app, &close_error)) {
      const std::string err = "catalog close failed: " + close_error;
      log_lattice_tool_end(
          app, name, catalog_already_open, false, false,
          now_ms_utc() - tool_started_at_ms,
          catalog_hold_started_at_ms == 0 ? 0
                                          : now_ms_utc() -
                                                catalog_hold_started_at_ms,
          err);
      write_jsonrpc_error(id_json, -32603,
                          "catalog close failed: " + close_error);
      continue;
    }
    const std::uint64_t finished_at_ms = now_ms_utc();
    log_lattice_tool_end(
        app, name, catalog_already_open, ok, true,
        finished_at_ms - tool_started_at_ms,
        catalog_hold_started_at_ms == 0 ? 0
                                        : finished_at_ms -
                                              catalog_hold_started_at_ms,
        ok ? std::string_view{} : std::string_view(tool_error));
    if (!ok) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }
}

}  // namespace
