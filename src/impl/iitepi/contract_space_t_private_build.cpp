[[nodiscard]] static std::shared_ptr<contract_record_t>
build_contract_record_from_contract_path(
    const std::string &contract_file_path) {
  const std::string resolved_contract_path =
      canonicalize_path_best_effort(contract_file_path);
  if (!has_non_ws_ascii(resolved_contract_path)) {
    log_fatal("[dconfig] cannot resolve runtime-binding contract file path "
              "from: %s\n",
              contract_file_path.c_str());
  }
  if (!std::filesystem::exists(resolved_contract_path)) {
    log_fatal("[dconfig] contract file path does not exist: %s\n",
              resolved_contract_path.c_str());
  }

  std::string contract_folder;
  const std::filesystem::path contract_path(resolved_contract_path);
  if (contract_path.has_parent_path()) {
    contract_folder = contract_path.parent_path().string();
  }

  parsed_config_t parsed = parse_config_file(resolved_contract_path);
  (void)validate_contract_config_or_terminate(parsed, contract_folder);

  auto record = std::make_shared<contract_record_t>();
  record->config_file_path = resolved_contract_path;
  record->config_file_path_canonical =
      canonicalize_path_best_effort(resolved_contract_path);
  record->config = parsed;
  const parsed_config_section_t merged_contract_variables =
      merged_contract_variable_section(record->config);
  const parsed_config_section_t *contract_variable_section =
      merged_contract_variables.empty() ? nullptr : &merged_contract_variables;

  const std::string vicreg_path = contract_required_resolved_variable_path(
      record->config, contract_folder, kContractVariableVicregConfigDslFile);
  const auto expected_value_path = contract_optional_resolved_variable_path(
      record->config, contract_folder,
      kContractVariableExpectedValueConfigDslFile);
  const auto embedding_sequence_eval_path =
      contract_optional_resolved_variable_path(
          record->config, contract_folder,
          kContractVariableEmbeddingSequenceAnalyticsDslFile);
  const auto transfer_matrix_eval_path =
      contract_optional_resolved_variable_path(
          record->config, contract_folder,
          kContractVariableTransferMatrixEvaluationDslFile);
  const std::string vicreg_grammar_path =
      global_required_resolved_path("BNF", "vicreg_grammar_filename");
  const std::string vicreg_grammar_text =
      piaabo::dfiles::readFileToString(vicreg_grammar_path);
  std::optional<std::string> expected_value_grammar_path{};
  std::optional<std::string> expected_value_grammar_text{};
  std::optional<std::string> embedding_sequence_eval_grammar_path{};
  std::optional<std::string> embedding_sequence_eval_grammar_text{};
  std::optional<std::string> transfer_matrix_eval_grammar_path{};
  std::optional<std::string> transfer_matrix_eval_grammar_text{};
  std::optional<std::string> network_design_dsl_path{};
  std::optional<std::string> expected_value_network_design_dsl_path{};
  std::optional<std::string> network_design_grammar_path{};
  std::optional<std::string> network_design_grammar_text{};
  std::string vicreg_resolved_text{};
  std::string expected_value_resolved_text{};
  std::string embedding_sequence_eval_resolved_text{};
  std::string transfer_matrix_eval_resolved_text{};
  std::string network_design_resolved_text{};
  std::string expected_value_network_design_resolved_text{};

  record->module_section_paths[kModuleSectionVicreg] = vicreg_path;
  record->module_sections[kModuleSectionVicreg] =
      parse_instruction_file(vicreg_path, vicreg_grammar_text,
                             contract_variable_section, &vicreg_resolved_text);
  {
    const auto vicreg_section_it =
        record->module_sections.find(kModuleSectionVicreg);
    if (vicreg_section_it != record->module_sections.end()) {
      const auto net_it =
          vicreg_section_it->second.find("network_design_dsl_file");
      if (net_it == vicreg_section_it->second.end()) {
        log_fatal("[dconfig] missing required network_design_dsl_file in "
                  "VICReg module config %s\n",
                  vicreg_path.c_str());
      }
      const std::string raw_path =
          unquote_if_wrapped(trim_ascii_ws_copy(net_it->second));
      if (!has_non_ws_ascii(raw_path)) {
        log_fatal("[dconfig] empty required network_design_dsl_file in VICReg "
                  "module config %s\n",
                  vicreg_path.c_str());
      }
      std::string vicreg_folder{};
      const std::filesystem::path vicreg_path_fs(vicreg_path);
      if (vicreg_path_fs.has_parent_path()) {
        vicreg_folder = vicreg_path_fs.parent_path().string();
      }
      const std::string resolved_network_design_path =
          resolve_path_from_folder(vicreg_folder, raw_path);
      if (!has_non_ws_ascii(resolved_network_design_path) ||
          !std::filesystem::exists(resolved_network_design_path) ||
          !std::filesystem::is_regular_file(resolved_network_design_path)) {
        log_fatal("[dconfig] invalid network_design_dsl_file in VICReg module "
                  "config %s: %s\n",
                  vicreg_path.c_str(), resolved_network_design_path.c_str());
      }
      network_design_dsl_path = resolved_network_design_path;
    }
  }

  if (expected_value_path.has_value()) {
    expected_value_grammar_path =
        global_required_resolved_path("BNF", "expected_value_grammar_filename");
    expected_value_grammar_text =
        piaabo::dfiles::readFileToString(*expected_value_grammar_path);
  }
  if (embedding_sequence_eval_path.has_value()) {
    embedding_sequence_eval_grammar_path = global_required_resolved_path(
        "BNF", kGlobalBnfEmbeddingSequenceAnalyticsGrammarKey);
    embedding_sequence_eval_grammar_text =
        piaabo::dfiles::readFileToString(*embedding_sequence_eval_grammar_path);
  }
  if (transfer_matrix_eval_path.has_value()) {
    transfer_matrix_eval_grammar_path = global_required_resolved_path(
        "BNF", kGlobalBnfTransferMatrixGrammarKey);
    transfer_matrix_eval_grammar_text =
        piaabo::dfiles::readFileToString(*transfer_matrix_eval_grammar_path);
  }
  if (network_design_dsl_path.has_value()) {
    network_design_grammar_path =
        global_required_resolved_path("BNF", "network_design_grammar_filename");
    network_design_grammar_text =
        piaabo::dfiles::readFileToString(*network_design_grammar_path);
  }
  if (expected_value_path.has_value()) {
    record->module_section_paths[kModuleSectionExpectedValue] =
        *expected_value_path;
    record->module_sections[kModuleSectionExpectedValue] =
        parse_instruction_file(
            *expected_value_path, *expected_value_grammar_text,
            contract_variable_section, &expected_value_resolved_text);
    const auto expected_value_section_it =
        record->module_sections.find(kModuleSectionExpectedValue);
    if (expected_value_section_it != record->module_sections.end()) {
      const auto net_it =
          expected_value_section_it->second.find("network_design_dsl_file");
      if (net_it == expected_value_section_it->second.end()) {
        log_fatal("[dconfig] missing required network_design_dsl_file in "
                  "EXPECTED_VALUE module config %s\n",
                  expected_value_path->c_str());
      }
      const std::string raw_path =
          unquote_if_wrapped(trim_ascii_ws_copy(net_it->second));
      if (!has_non_ws_ascii(raw_path)) {
        log_fatal("[dconfig] empty required network_design_dsl_file in "
                  "EXPECTED_VALUE module config %s\n",
                  expected_value_path->c_str());
      }
      std::string expected_value_folder{};
      const std::filesystem::path expected_value_path_fs(*expected_value_path);
      if (expected_value_path_fs.has_parent_path()) {
        expected_value_folder = expected_value_path_fs.parent_path().string();
      }
      const std::string resolved_network_design_path =
          resolve_path_from_folder(expected_value_folder, raw_path);
      if (!has_non_ws_ascii(resolved_network_design_path) ||
          !std::filesystem::exists(resolved_network_design_path) ||
          !std::filesystem::is_regular_file(resolved_network_design_path)) {
        log_fatal("[dconfig] invalid network_design_dsl_file in "
                  "EXPECTED_VALUE module config %s: %s\n",
                  expected_value_path->c_str(),
                  resolved_network_design_path.c_str());
      }
      expected_value_network_design_dsl_path = resolved_network_design_path;
    }
  }
  if (embedding_sequence_eval_path.has_value()) {
    record->module_section_paths[kModuleSectionEmbeddingSequenceAnalytics] =
        *embedding_sequence_eval_path;
    record->module_sections[kModuleSectionEmbeddingSequenceAnalytics] =
        parse_instruction_file(*embedding_sequence_eval_path,
                               *embedding_sequence_eval_grammar_text,
                               contract_variable_section,
                               &embedding_sequence_eval_resolved_text);
  }
  if (transfer_matrix_eval_path.has_value()) {
    record->module_section_paths[kModuleSectionTransferMatrixEvaluation] =
        *transfer_matrix_eval_path;
    record->module_sections[kModuleSectionTransferMatrixEvaluation] =
        parse_instruction_file(
            *transfer_matrix_eval_path, *transfer_matrix_eval_grammar_text,
            contract_variable_section, &transfer_matrix_eval_resolved_text);
  }

  constexpr const char *kRequiredContractDslPathKeys[] = {
      kAssemblyKeyCircuitDslFilename,
  };
  constexpr const char *kRequiredGlobalGrammarPathKeys[] = {
      "tsiemene_circuit_grammar_filename",
      "canonical_path_grammar_filename",
  };

  std::set<std::string> dependency_paths;
  dependency_paths.insert(record->config_file_path_canonical);
  dependency_paths.insert(canonicalize_path_best_effort(vicreg_path));
  dependency_paths.insert(canonicalize_path_best_effort(vicreg_grammar_path));
  if (network_design_dsl_path.has_value()) {
    dependency_paths.insert(
        canonicalize_path_best_effort(*network_design_dsl_path));
  }
  if (expected_value_network_design_dsl_path.has_value()) {
    dependency_paths.insert(
        canonicalize_path_best_effort(*expected_value_network_design_dsl_path));
  }
  if (network_design_grammar_path.has_value()) {
    dependency_paths.insert(
        canonicalize_path_best_effort(*network_design_grammar_path));
  }
  if (expected_value_path.has_value()) {
    dependency_paths.insert(
        canonicalize_path_best_effort(*expected_value_path));
    dependency_paths.insert(
        canonicalize_path_best_effort(*expected_value_grammar_path));
  }
  if (transfer_matrix_eval_path.has_value()) {
    dependency_paths.insert(
        canonicalize_path_best_effort(*transfer_matrix_eval_path));
    dependency_paths.insert(
        canonicalize_path_best_effort(*transfer_matrix_eval_grammar_path));
  }

  std::unordered_map<std::string, std::string> asset_text_by_key{};
  for (const char *key : kRequiredContractDslPathKeys) {
    const std::string path = contract_required_resolved_path(
        record->config, contract_folder, kContractSectionAssembly, key);
    std::string text = piaabo::dfiles::readFileToString(path);
    if (contract_variable_section != nullptr &&
        !contract_variable_section->empty()) {
      text = resolve_dsl_variables_or_fail(
          text, contract_dsl_variables_from_section(*contract_variable_section),
          path);
    }
    asset_text_by_key[key] = std::move(text);
    dependency_paths.insert(canonicalize_path_best_effort(path));
  }
  for (const char *key : kRequiredGlobalGrammarPathKeys) {
    const std::string path = global_required_resolved_path("BNF", key);
    asset_text_by_key[key] = piaabo::dfiles::readFileToString(path);
    dependency_paths.insert(canonicalize_path_best_effort(path));
  }
  if (contract_variable_section != nullptr) {
    for (const char *var_name : {"__observation_sources_dsl_file",
                                 "__observation_channels_dsl_file"}) {
      const auto it = contract_variable_section->find(var_name);
      if (it == contract_variable_section->end())
        continue;
      const std::string raw_path =
          unquote_if_wrapped(trim_ascii_ws_copy(it->second));
      if (!has_non_ws_ascii(raw_path))
        continue;
      dependency_paths.insert(canonicalize_path_best_effort(
          resolve_path_from_folder(contract_folder, raw_path)));
    }
  }

  const auto asset_text_or_fail = [&](const char *key) -> const std::string & {
    const auto it = asset_text_by_key.find(key);
    if (it == asset_text_by_key.end()) {
      log_fatal("[dconfig] missing required DSL/grammar asset <%s> while "
                "building contract record\n",
                key);
    }
    return it->second;
  };

  record->circuit.grammar =
      asset_text_or_fail("tsiemene_circuit_grammar_filename");
  record->circuit.dsl = asset_text_or_fail(kAssemblyKeyCircuitDslFilename);

  record->canonical_path.grammar =
      asset_text_or_fail("canonical_path_grammar_filename");

  if (network_design_dsl_path.has_value()) {
    if (!network_design_grammar_text.has_value() ||
        !has_non_ws_ascii(*network_design_grammar_text)) {
      log_fatal("[dconfig] missing/empty network design grammar while "
                "VICReg.network_design_dsl_file is configured\n");
    }
    std::string network_design_dsl_text =
        piaabo::dfiles::readFileToString(*network_design_dsl_path);
    if (contract_variable_section != nullptr &&
        !contract_variable_section->empty()) {
      network_design_dsl_text = resolve_dsl_variables_or_fail(
          network_design_dsl_text,
          contract_dsl_variables_from_section(*contract_variable_section),
          *network_design_dsl_path);
    }
    if (!has_non_ws_ascii(network_design_dsl_text)) {
      log_fatal("[dconfig] empty network design DSL payload: %s\n",
                network_design_dsl_path->c_str());
    }
    record->vicreg_network_design.grammar = *network_design_grammar_text;
    record->vicreg_network_design.dsl = network_design_dsl_text;
    network_design_resolved_text = network_design_dsl_text;
    normalize_vicreg_module_architecture_from_network_design(record.get());
  }
  if (expected_value_network_design_dsl_path.has_value()) {
    if (!network_design_grammar_text.has_value() ||
        !has_non_ws_ascii(*network_design_grammar_text)) {
      log_fatal("[dconfig] missing/empty network design grammar while "
                "EXPECTED_VALUE.network_design_dsl_file is configured\n");
    }
    std::string network_design_dsl_text = piaabo::dfiles::readFileToString(
        *expected_value_network_design_dsl_path);
    if (contract_variable_section != nullptr &&
        !contract_variable_section->empty()) {
      network_design_dsl_text = resolve_dsl_variables_or_fail(
          network_design_dsl_text,
          contract_dsl_variables_from_section(*contract_variable_section),
          *expected_value_network_design_dsl_path);
    }
    if (!has_non_ws_ascii(network_design_dsl_text)) {
      log_fatal("[dconfig] empty network design DSL payload: %s\n",
                expected_value_network_design_dsl_path->c_str());
    }
    record->expected_value_network_design.grammar =
        *network_design_grammar_text;
    record->expected_value_network_design.dsl = network_design_dsl_text;
    expected_value_network_design_resolved_text = network_design_dsl_text;
    normalize_expected_value_module_architecture_from_network_design(
        record.get());
  }

  if (!has_non_ws_ascii(record->circuit.dsl)) {
    log_fatal("[dconfig] missing effective circuit DSL payload\n");
  }

  const auto &decoded_circuit = record->circuit.decoded();
  record->signature.circuit_dsl_sha256_hex = sha256_hex_from_bytes(
      record->circuit.dsl.data(), record->circuit.dsl.size());
  if (!has_non_ws_ascii(record->signature.circuit_dsl_sha256_hex)) {
    log_fatal("[dconfig] failed to compute circuit DSL sha256\n");
  }
  if (contract_variable_section != nullptr) {
    record->signature.variable_assignments =
        contract_variable_assignments_from_section(*contract_variable_section);
  }
  if (const auto dock_it = record->config.find(kContractSectionDock);
      dock_it != record->config.end()) {
    record->docking_signature.variable_assignments =
        contract_variable_assignments_from_section(dock_it->second);
  }
  {
    std::unordered_set<std::string> seen_circuits{};
    for (const auto &circuit : decoded_circuit.circuits) {
      const std::string circuit_name = trim_ascii_ws_copy(circuit.name);
      if (!has_non_ws_ascii(circuit_name))
        continue;
      if (!seen_circuits.insert(circuit_name).second)
        continue;
      record->docking_signature.compatible_circuits.push_back(circuit_name);
    }
  }
  const std::string vicreg_module_sha256_hex =
      contract_space_t::sha256_hex_for_file(vicreg_path);
  const std::string expected_value_module_sha256_hex =
      expected_value_path.has_value()
          ? contract_space_t::sha256_hex_for_file(*expected_value_path)
          : std::string{};
  record->signature.bindings = derive_contract_component_bindings_or_fail(
      decoded_circuit, "", vicreg_path, vicreg_module_sha256_hex,
      expected_value_path.has_value() ? *expected_value_path : std::string{},
      expected_value_module_sha256_hex);
  for (const auto &b : record->signature.bindings) {
    if (has_non_ws_ascii(b.tsi_dsl_path)) {
      dependency_paths.insert(canonicalize_path_best_effort(b.tsi_dsl_path));
    }
  }

  std::unordered_set<std::string> seen_signature_modules{};
  const auto append_module_signature_entry =
      [&](std::string_view module_id, const std::string &raw_module_path,
          std::string_view resolved_module_text = std::string_view{}) {
        std::string module_path =
            canonicalize_path_best_effort(raw_module_path);
        if (!has_non_ws_ascii(module_path)) {
          module_path = trim_ascii_ws_copy(raw_module_path);
        }
        if (!has_non_ws_ascii(module_path))
          return;
        if (!seen_signature_modules
                 .insert(std::string(module_id) + "|" + module_path)
                 .second) {
          return;
        }
        if (!std::filesystem::exists(module_path) ||
            !std::filesystem::is_regular_file(module_path)) {
          log_fatal("[dconfig] invalid module signature path for [%s]: %s\n",
                    std::string(module_id).c_str(), module_path.c_str());
        }
        contract_module_signature_entry_t entry{};
        entry.module_id = std::string(module_id);
        entry.module_dsl_path = module_path;
        if (!resolved_module_text.empty()) {
          entry.module_dsl_sha256_hex = sha256_hex_from_bytes(
              resolved_module_text.data(), resolved_module_text.size());
        } else {
          entry.module_dsl_sha256_hex = sha256_hex_from_file(module_path);
        }
        if (!has_non_ws_ascii(entry.module_dsl_sha256_hex)) {
          log_fatal("[dconfig] failed to fingerprint module signature path for "
                    "[%s]: %s\n",
                    entry.module_id.c_str(), entry.module_dsl_path.c_str());
        }
        record->signature.module_dsl_entries.push_back(std::move(entry));
      };
  const auto resolve_contract_variable_path =
      [&](const char *var_name) -> std::optional<std::string> {
    return contract_optional_resolved_variable_path(record->config,
                                                    contract_folder, var_name);
  };
  const auto resolve_contract_surface_text =
      [&](const std::string &resolved_path) -> std::string {
    std::string dsl_text = piaabo::dfiles::readFileToString(resolved_path);
    if (contract_variable_section != nullptr &&
        !contract_variable_section->empty()) {
      dsl_text = resolve_dsl_variables_or_fail(
          dsl_text,
          contract_dsl_variables_from_section(*contract_variable_section),
          resolved_path);
    }
    return dsl_text;
  };
  if (const auto observation_sources_path =
          resolve_contract_variable_path("__observation_sources_dsl_file");
      observation_sources_path.has_value()) {
    append_module_signature_entry(
        "contract.observation.sources", *observation_sources_path,
        resolve_contract_surface_text(*observation_sources_path));
  }
  if (const auto observation_channels_path =
          resolve_contract_variable_path("__observation_channels_dsl_file");
      observation_channels_path.has_value()) {
    append_module_signature_entry(
        "contract.observation.channels", *observation_channels_path,
        resolve_contract_surface_text(*observation_channels_path));
  }
  append_module_signature_entry(kModuleSectionVicreg, vicreg_path,
                                vicreg_resolved_text);
  if (expected_value_path.has_value()) {
    append_module_signature_entry(kModuleSectionExpectedValue,
                                  *expected_value_path,
                                  expected_value_resolved_text);
  }
  if (embedding_sequence_eval_path.has_value()) {
    append_module_signature_entry(kModuleSectionEmbeddingSequenceAnalytics,
                                  *embedding_sequence_eval_path,
                                  embedding_sequence_eval_resolved_text);
  }
  if (transfer_matrix_eval_path.has_value()) {
    append_module_signature_entry(kModuleSectionTransferMatrixEvaluation,
                                  *transfer_matrix_eval_path,
                                  transfer_matrix_eval_resolved_text);
  }
  if (network_design_dsl_path.has_value()) {
    append_module_signature_entry("VICReg.network_design_dsl_file",
                                  *network_design_dsl_path,
                                  network_design_resolved_text);
  }
  if (expected_value_network_design_dsl_path.has_value()) {
    append_module_signature_entry("EXPECTED_VALUE.network_design_dsl_file",
                                  *expected_value_network_design_dsl_path,
                                  expected_value_network_design_resolved_text);
  }

  {
    std::unordered_set<std::string> seen_docking_surfaces{};
    const auto append_docking_surface = [&](std::string_view surface_id,
                                            const std::string &raw_path,
                                            std::string_view resolved_text =
                                                std::string_view{}) {
      std::string canonical_path =
          canonicalize_path_best_effort(trim_ascii_ws_copy(raw_path));
      if (!has_non_ws_ascii(canonical_path)) {
        canonical_path = trim_ascii_ws_copy(raw_path);
      }
      if (!has_non_ws_ascii(canonical_path))
        return;
      const std::string dedup_key =
          std::string(surface_id) + "|" + canonical_path;
      if (!seen_docking_surfaces.insert(dedup_key).second)
        return;

      contract_docking_surface_entry_t entry{};
      entry.surface_id = std::string(surface_id);
      entry.canonical_path = canonical_path;
      if (!resolved_text.empty()) {
        entry.sha256_hex =
            sha256_hex_from_bytes(resolved_text.data(), resolved_text.size());
      } else {
        entry.sha256_hex = sha256_hex_from_file(canonical_path);
      }
      if (!has_non_ws_ascii(entry.sha256_hex)) {
        log_fatal("[dconfig] failed to fingerprint docking surface [%s]: %s\n",
                  entry.surface_id.c_str(), entry.canonical_path.c_str());
      }
      record->docking_signature.surfaces.push_back(std::move(entry));
    };

    append_docking_surface(
        "contract.circuit",
        contract_required_resolved_path(record->config, contract_folder,
                                        kContractSectionAssembly,
                                        kAssemblyKeyCircuitDslFilename),
        record->circuit.dsl);
    append_docking_surface("contract.module.VICReg", vicreg_path,
                           vicreg_resolved_text);
    if (network_design_dsl_path.has_value()) {
      append_docking_surface("contract.module.VICReg.network_design",
                             *network_design_dsl_path,
                             network_design_resolved_text);
    }
    if (expected_value_path.has_value()) {
      append_docking_surface("contract.module.EXPECTED_VALUE",
                             *expected_value_path,
                             expected_value_resolved_text);
    }
    if (expected_value_network_design_dsl_path.has_value()) {
      append_docking_surface("contract.module.EXPECTED_VALUE.network_design",
                             *expected_value_network_design_dsl_path,
                             expected_value_network_design_resolved_text);
    }

    const auto append_contract_variable_surface = [&](std::string_view
                                                          surface_id,
                                                      const char *var_name) {
      const auto resolved_path = resolve_contract_variable_path(var_name);
      if (!resolved_path.has_value())
        return;
      if (!std::filesystem::exists(*resolved_path) ||
          !std::filesystem::is_regular_file(*resolved_path)) {
        log_fatal(
            "[dconfig] invalid contract docking surface path for [%s]: %s\n",
            std::string(surface_id).c_str(), resolved_path->c_str());
      }
      std::string dsl_text = resolve_contract_surface_text(*resolved_path);
      append_docking_surface(surface_id, *resolved_path, dsl_text);
    };
    append_contract_variable_surface("contract.observation.sources",
                                     "__observation_sources_dsl_file");
    append_contract_variable_surface("contract.observation.channels",
                                     "__observation_channels_dsl_file");
  }
  const std::string docking_signature_json =
      canonical_contract_docking_signature_json(record->docking_signature);
  record->docking_signature.sha256_hex = sha256_hex_from_bytes(
      docking_signature_json.data(), docking_signature_json.size());
  if (!has_non_ws_ascii(record->docking_signature.sha256_hex)) {
    log_fatal(
        "[dconfig] failed to compute contract docking signature sha256\n");
  }
  record->signature.docking_signature_sha256_hex =
      record->docking_signature.sha256_hex;

  for (const auto &dep_path : dependency_paths) {
    if (!has_non_ws_ascii(dep_path))
      continue;
    record->dependency_manifest.files.push_back(fingerprint_file(dep_path));
  }
  record->dependency_manifest.dependency_manifest_aggregate_sha256_hex =
      compute_manifest_digest_hex(record->dependency_manifest.files);
  return record;
}

