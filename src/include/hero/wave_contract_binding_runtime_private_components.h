[[nodiscard]] inline std::string
render_wave_component_identity_line(std::string_view path,
                                    std::string_view family) {
  const std::string trimmed_path = trim_ascii(path);
  const std::string trimmed_family = trim_ascii(family);
  if (!trimmed_path.empty() && trimmed_path != trimmed_family) {
    return "PATH: " + trimmed_path + ";";
  }
  if (!trimmed_family.empty()) {
    return "FAMILY: " + trimmed_family + ";";
  }
  if (!trimmed_path.empty()) {
    return "PATH: " + trimmed_path + ";";
  }
  return {};
}

[[nodiscard]] inline std::string
component_canonical_path(std::string_view family, std::string_view hashimyei) {
  const std::string family_token = trim_ascii(family);
  const std::string hash_token = trim_ascii(hashimyei);
  if (family_token.empty())
    return hash_token;
  if (hash_token.empty())
    return family_token;
  return family_token + "." + hash_token;
}

[[nodiscard]] inline std::string render_iitepi_wave_set_to_dsl(
    const cuwacunu::camahjucunu::iitepi_wave_set_t &wave_set) {
  using namespace cuwacunu::camahjucunu;
  std::ostringstream out;
  for (std::size_t wave_index = 0; wave_index < wave_set.waves.size();
       ++wave_index) {
    const auto &wave = wave_set.waves[wave_index];
    out << "WAVE " << wave.name << " {\n";
    if (!trim_ascii(wave.circuit_name).empty()) {
      out << "  CIRCUIT: " << trim_ascii(wave.circuit_name) << ";\n";
    }
    if (wave.mode_flags != 0) {
      out << "  MODE: " << canonical_iitepi_wave_mode(wave.mode_flags) << ";\n";
    } else {
      out << "  MODE: " << trim_ascii(wave.mode) << ";\n";
    }
    if (!trim_ascii(wave.determinism_policy).empty()) {
      out << "  DETERMINISM: " << trim_ascii(wave.determinism_policy) << ";\n";
    }
    out << "  SAMPLER: " << trim_ascii(wave.sampler) << ";\n";
    out << "  EPOCHS: " << wave.epochs << ";\n";
    out << "  BATCH_SIZE: " << wave.batch_size << ";\n";
    out << "  MAX_BATCHES_PER_EPOCH: " << wave.max_batches_per_epoch << ";\n";
    out << "\n";

    for (const auto &source : wave.sources) {
      out << "  SOURCE: <" << source.binding_id << "> {\n";
      const std::string identity = render_wave_component_identity_line(
          source.source_path, source.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "    SETTINGS: {\n";
      out << "      WORKERS: " << source.workers << ";\n";
      out << "      FORCE_REBUILD_CACHE: "
          << (source.force_rebuild_cache ? "true" : "false") << ";\n";
      out << "      RANGE_WARN_BATCHES: " << source.range_warn_batches << ";\n";
      out << "    };\n";
      out << "    RUNTIME: {\n";
      out << "      RUNTIME_INSTRUMENT_SIGNATURE: {\n";
      out << "        SYMBOL: "
          << trim_ascii(source.runtime_instrument_signature.symbol) << ";\n";
      out << "        RECORD_TYPE: "
          << trim_ascii(source.runtime_instrument_signature.record_type)
          << ";\n";
      out << "        MARKET_TYPE: "
          << trim_ascii(source.runtime_instrument_signature.market_type)
          << ";\n";
      out << "        VENUE: "
          << trim_ascii(source.runtime_instrument_signature.venue) << ";\n";
      out << "        BASE_ASSET: "
          << trim_ascii(source.runtime_instrument_signature.base_asset)
          << ";\n";
      out << "        QUOTE_ASSET: "
          << trim_ascii(source.runtime_instrument_signature.quote_asset)
          << ";\n";
      out << "      };\n";
      out << "      FROM: " << trim_ascii(source.from) << ";\n";
      out << "      TO: " << trim_ascii(source.to) << ";\n";
      out << "    };\n";
      out << "  };\n\n";
    }

    for (const auto &wikimyei : wave.wikimyeis) {
      out << "  WIKIMYEI: <" << wikimyei.binding_id << "> {\n";
      const std::string identity = render_wave_component_identity_line(
          wikimyei.wikimyei_path, wikimyei.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "    JKIMYEI: {\n";
      out << "      HALT_TRAIN: " << (wikimyei.halt_train ? "true" : "false")
          << ";\n";
      if (!trim_ascii(wikimyei.profile_id).empty()) {
        out << "      PROFILE_ID: " << trim_ascii(wikimyei.profile_id) << ";\n";
      }
      out << "    };\n";
      out << "  };\n\n";
    }

    for (const auto &probe : wave.probes) {
      out << "  PROBE: <" << probe.binding_id << "> {\n";
      const std::string identity =
          render_wave_component_identity_line(probe.probe_path, probe.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      if (probe.has_runtime) {
        out << "    RUNTIME: {\n";
        out << "      TRAINING_WINDOW: incoming_batch;\n";
        out << "      REPORT_POLICY: epoch_end_log;\n";
        out << "      OBJECTIVE: future_target_feature_indices_nll;\n";
        out << "    };\n";
      }
      out << "  };\n\n";
    }

    for (const auto &sink : wave.sinks) {
      out << "  SINK: <" << sink.binding_id << "> {\n";
      const std::string identity =
          render_wave_component_identity_line(sink.sink_path, sink.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "  };\n\n";
    }

    out << "}\n";
    if (wave_index + 1 != wave_set.waves.size()) {
      out << "\n";
    }
  }
  return out.str();
}

[[nodiscard]] inline const cuwacunu::camahjucunu::instrument_signature_t *
selected_wave_runtime_instrument_signature(
    const cuwacunu::camahjucunu::iitepi_wave_t &wave, std::string *error) {
  if (wave.sources.size() != 1U) {
    if (error) {
      *error = "component derivation requires exactly one SOURCE block in "
               "selected wave";
    }
    return nullptr;
  }
  const auto &runtime_signature =
      wave.sources.front().runtime_instrument_signature;
  std::string signature_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          runtime_signature, /*allow_any=*/false,
          "RUNTIME_INSTRUMENT_SIGNATURE", &signature_error)) {
    if (error)
      *error = signature_error;
    return nullptr;
  }
  return &runtime_signature;
}

[[nodiscard]] inline const cuwacunu::iitepi::contract_component_binding_t *
find_contract_component_binding_for_wave_slot(
    const cuwacunu::iitepi::contract_record_t &contract,
    std::string_view binding_id, std::string_view family,
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    std::string *error) {
  const std::string id = trim_ascii(binding_id);
  const std::string family_token = trim_ascii(family);
  if (id.empty()) {
    if (error)
      *error = "wave component binding id is empty";
    return nullptr;
  }
  if (family_token.empty()) {
    if (error)
      *error = "wave component family is empty for binding id: " + id;
    return nullptr;
  }

  const cuwacunu::iitepi::contract_component_binding_t *by_id = nullptr;
  for (const auto &binding : contract.signature.bindings) {
    if (trim_ascii(binding.binding_id) == id)
      by_id = &binding;
  }
  if (!by_id) {
    if (error) {
      *error = "contract has no component binding for wave binding id: " + id;
    }
    return nullptr;
  }
  if (trim_ascii(by_id->tsi_type) != family_token) {
    if (error) {
      *error = "contract component binding family mismatch for <" + id +
               ">: contract=" + trim_ascii(by_id->tsi_type) +
               " wave=" + family_token;
    }
    return nullptr;
  }

  const auto *compatibility =
      cuwacunu::iitepi::find_component_compatibility_signature(contract, id);
  if (!compatibility) {
    if (error) {
      *error = "contract has no component compatibility signature for wave "
               "binding id: " +
               id;
    }
    return nullptr;
  }
  std::string signature_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_accepts_runtime(
          compatibility->instrument_signature, runtime_signature,
          "INSTRUMENT_SIGNATURE <" + id + ">", &signature_error)) {
    if (error)
      *error = signature_error;
    return nullptr;
  }
  return by_id;
}

[[nodiscard]] inline bool apply_contract_components_to_wave_snapshot(
    const std::filesystem::path &staged_contract_path,
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    const std::filesystem::path &staged_wave_path, std::string *error) {
  if (error)
    error->clear();
  std::string grammar_text{};
  if (!load_text_file(configured_wave_grammar_path(), &grammar_text, error)) {
    return false;
  }
  std::string wave_text{};
  if (!load_text_file(staged_wave_path, &wave_text, error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
        grammar_text, wave_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "cannot decode staged wave DSL '" + staged_wave_path.string() +
               "': " + e.what();
    }
    return false;
  }

  auto wave_it = std::find_if(
      wave_set.waves.begin(), wave_set.waves.end(), [&](const auto &wave) {
        return trim_ascii(wave.name) == trim_ascii(bind.wave_ref);
      });
  if (wave_it == wave_set.waves.end()) {
    if (error) {
      *error = "selected bind references unknown WAVE id in staged wave DSL: " +
               bind.wave_ref;
    }
    return false;
  }

  const auto contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          staged_contract_path.string());
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!contract_snapshot) {
    if (error) {
      *error =
          "cannot load staged contract snapshot for component derivation: " +
          staged_contract_path.string();
    }
    return false;
  }
  const std::string docking_signature_sha256_hex =
      trim_ascii(contract_snapshot->signature.docking_signature_sha256_hex);
  if (docking_signature_sha256_hex.empty()) {
    if (error) {
      *error = "staged contract snapshot is missing docking signature";
    }
    return false;
  }
  const auto *runtime_signature =
      selected_wave_runtime_instrument_signature(*wave_it, error);
  if (!runtime_signature)
    return false;
  auto apply_component =
      [&](std::string_view component_kind, std::string *component_path,
          const std::string &binding_id, const std::string &family) -> bool {
    std::string selection_error{};
    const auto *component_binding =
        find_contract_component_binding_for_wave_slot(
            *contract_snapshot, binding_id, family, *runtime_signature,
            &selection_error);
    if (!component_binding) {
      if (error) {
        *error = "BIND '" + bind.id + "' cannot derive component binding for " +
                 std::string(component_kind) + " <" + binding_id +
                 ">: " + selection_error;
      }
      return false;
    }
    const auto realized_component =
        cuwacunu::iitepi::realize_contract_component_binding_for_runtime(
            *contract_snapshot, component_binding->binding_id,
            *runtime_signature, &selection_error);
    if (!realized_component.has_value()) {
      if (error) {
        *error = "BIND '" + bind.id +
                 "' cannot realize component binding for " +
                 std::string(component_kind) + " <" + binding_id +
                 ">: " + selection_error;
      }
      return false;
    }
    if (trim_ascii(realized_component->hashimyei).empty()) {
      if (error) {
        *error = "BIND '" + bind.id +
                 "' contract component binding has no derived hashimyei for <" +
                 binding_id + ">";
      }
      return false;
    }
    *component_path = component_canonical_path(realized_component->tsi_type,
                                               realized_component->hashimyei);
    return true;
  };

  for (auto &wikimyei : wave_it->wikimyeis) {
    if (!apply_component("WIKIMYEI", &wikimyei.wikimyei_path,
                         wikimyei.binding_id, wikimyei.family)) {
      return false;
    }
  }

  return store_text_file(staged_wave_path,
                         render_iitepi_wave_set_to_dsl(wave_set), error);
}

[[nodiscard]] inline bool
wave_text_contains_wave_name(std::string_view text,
                             std::string_view wave_name) {
  bool in_block_comment = false;
  bool in_line_comment = false;
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';
    if (in_line_comment) {
      if (c == '\n')
        in_line_comment = false;
      ++i;
      continue;
    }
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && !in_double_quote) {
      if (c == '/' && next == '/') {
        in_line_comment = true;
        i += 2;
        continue;
      }
      if (c == '/' && next == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      if (c == '#') {
        in_line_comment = true;
        ++i;
        continue;
      }
      if (i + 4 <= text.size() && text.compare(i, 4, "WAVE") == 0 &&
          token_boundary_before(text, i) && token_boundary_after(text, i, 4)) {
        std::size_t pos = i + 4;
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
          ++pos;
        }
        const std::size_t name_begin = pos;
        while (pos < text.size() && is_ident_char(text[pos]))
          ++pos;
        if (pos == name_begin) {
          i += 4;
          continue;
        }
        const std::string found_name =
            std::string(text.substr(name_begin, pos - name_begin));
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
          ++pos;
        }
        if (pos < text.size() && text[pos] == '{' && found_name == wave_name) {
          return true;
        }
      }
    }
    ++i;
  }
  return false;
}

[[nodiscard]] inline bool find_matching_wave_import_path(
    const std::vector<cuwacunu::camahjucunu::iitepi_campaign_wave_decl_t>
        &waves,
    const std::filesystem::path &source_scope_path, std::string_view wave_ref,
    std::string *out_path, std::string *error) {
  if (error)
    error->clear();
  if (out_path)
    out_path->clear();
  const auto wave_it =
      std::find_if(waves.begin(), waves.end(), [&](const auto &wave_decl) {
        return wave_decl.id == wave_ref;
      });
  if (wave_it == waves.end()) {
    if (error) {
      *error = "binding references unknown WAVE id: " + std::string(wave_ref);
    }
    return false;
  }
  const std::string candidate =
      resolve_path_from_folder(source_scope_path.parent_path(), wave_it->file);
  std::string text{};
  if (!load_text_file(candidate, &text, error))
    return false;
  if (!wave_text_contains_wave_name(text, wave_it->id)) {
    if (error) {
      *error = "imported WAVE id not found in declared wave DSL file: " +
               std::string(wave_it->id);
    }
    return false;
  }
  if (out_path)
    *out_path = candidate;
  return true;
}

template <typename ContractDeclT>
[[nodiscard]] inline const ContractDeclT *
find_contract_decl_by_id(const std::vector<ContractDeclT> &contracts,
                         std::string_view contract_ref) {
  for (const auto &contract : contracts) {
    if (contract.id == contract_ref)
      return &contract;
  }
  return nullptr;
}

template <typename BindDeclT>
[[nodiscard]] inline const BindDeclT *
find_bind_decl_by_id(const std::vector<BindDeclT> &binds,
                     std::string_view binding_id) {
  for (const auto &bind : binds) {
    if (bind.id == binding_id)
      return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path &source_path,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &variables,
    const std::filesystem::path &target_path,
    const std::filesystem::path &output_dir, staged_dsl_graph_state_t *state,
    std::string *error) {
  if (error)
    error->clear();
  if (!state) {
    if (error)
      *error = "missing staged DSL graph state";
    return false;
  }
  const std::filesystem::path normalized_source =
      source_path.lexically_normal();
  const std::filesystem::path normalized_target =
      target_path.lexically_normal();
  const std::string source_key = normalize_path_key(normalized_source);
  const std::string target_key = normalize_path_key(normalized_target);

  const auto source_it = state->source_to_snapshot.find(source_key);
  const bool already_staged_in_current_graph =
      source_it != state->source_to_snapshot.end();
  if (already_staged_in_current_graph &&
      source_it->second != normalized_target) {
    if (error) {
      *error = "conflicting snapshot targets for DSL source: " + source_key;
    }
    return false;
  }
  if (const auto it = state->snapshot_to_source.find(target_key);
      it != state->snapshot_to_source.end() && it->second != source_key) {
    if (error) {
      *error =
          "conflicting staged DSL destination: " + normalized_target.string();
    }
    return false;
  }

  state->source_to_snapshot[source_key] = normalized_target;
  state->snapshot_to_source[target_key] = source_key;
  if (is_active_source(*state, source_key))
    return true;

  state->active_sources.push_back(source_key);
  const auto cleanup_active = [&]() {
    if (!state->active_sources.empty() &&
        state->active_sources.back() == source_key) {
      state->active_sources.pop_back();
      return;
    }
    for (auto it = state->active_sources.begin();
         it != state->active_sources.end(); ++it) {
      if (*it == source_key) {
        state->active_sources.erase(it);
        break;
      }
    }
  };

  std::string raw_text{};
  if (!load_text_file(normalized_source, &raw_text, error)) {
    cleanup_active();
    return false;
  }

  std::string resolved_text{};
  if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
          raw_text, variables, &resolved_text, error)) {
    cleanup_active();
    return false;
  }

  std::ostringstream rebuilt{};
  std::size_t pos = 0;
  bool first = true;
  bool in_block_comment = false;
  while (pos <= resolved_text.size()) {
    const std::size_t end = resolved_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? resolved_text.substr(pos)
                           : resolved_text.substr(pos, end - pos);

    dsl_path_assignment_t path_assignment{};
    if (find_dsl_path_assignment(line, in_block_comment, &path_assignment)) {
      const std::filesystem::path child_source(resolve_path_from_folder(
          normalized_source.parent_path(), path_assignment.raw_value));
      std::error_code exists_ec{};
      if (!std::filesystem::exists(child_source, exists_ec) ||
          !std::filesystem::is_regular_file(child_source, exists_ec)) {
        cleanup_active();
        if (error) {
          *error = "staged DSL graph references missing DSL file: " +
                   child_source.string();
        }
        return false;
      }

      const std::filesystem::path child_target =
          make_unique_snapshot_path(output_dir, child_source.filename(),
                                    normalize_path_key(child_source), state);
      if (!stage_dsl_graph_file(child_source, variables, child_target,
                                output_dir, state, error)) {
        cleanup_active();
        return false;
      }

      std::filesystem::path relative_child =
          child_target.lexically_relative(normalized_target.parent_path());
      if (relative_child.empty())
        relative_child = child_target.filename();
      const std::string replacement =
          path_assignment.quoted
              ? std::string(1, path_assignment.quote_char) +
                    relative_child.string() +
                    std::string(1, path_assignment.quote_char)
              : relative_child.string();
      line = line.substr(0, path_assignment.replace_begin) + replacement +
             line.substr(path_assignment.replace_end);
    }

    if (!first)
      rebuilt << "\n";
    first = false;
    rebuilt << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos)
      break;
    pos = end + 1;
  }

  std::error_code mkdir_ec{};
  std::filesystem::create_directories(normalized_target.parent_path(),
                                      mkdir_ec);
  if (mkdir_ec) {
    cleanup_active();
    if (error) {
      *error = "cannot create staged DSL directory: " +
               normalized_target.parent_path().string();
    }
    return false;
  }
  if (!store_text_file(normalized_target, rebuilt.str(), error)) {
    cleanup_active();
    return false;
  }
  cleanup_active();
  return true;
}
