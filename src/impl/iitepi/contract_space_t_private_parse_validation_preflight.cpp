static bool
validate_contract_config_or_terminate(const parsed_config_t &cfg,
                                      const std::string &cfg_folder) {
  bool ok = true;
  const parsed_config_section_t *dock_section = nullptr;
  if (const auto dock_it = cfg.find(kContractSectionDock);
      dock_it != cfg.end()) {
    dock_section = &dock_it->second;
  }
  const parsed_config_section_t *assembly_section = nullptr;
  if (const auto assembly_it = cfg.find(kContractSectionAssembly);
      assembly_it != cfg.end()) {
    assembly_section = &assembly_it->second;
  }
  const parsed_config_section_t merged_contract_variables =
      merged_contract_variable_section(cfg);
  const parsed_config_section_t *contract_variable_section =
      merged_contract_variables.empty() ? nullptr : &merged_contract_variables;

  const auto require_value = [&](const char *section, const char *key,
                                 std::string *out_value) -> bool {
    const auto sec_it = cfg.find(section);
    if (sec_it == cfg.end()) {
      log_warn("Missing contract section [%s]\n", section);
      ok = false;
      return false;
    }

    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
      log_warn("Missing field <%s> in contract section [%s]\n", key, section);
      ok = false;
      return false;
    }

    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) {
      log_warn("Empty field <%s> in contract section [%s]\n", key, section);
      ok = false;
      return false;
    }
    if (out_value)
      *out_value = value;
    return true;
  };

  const auto require_existing_path = [&](const char *section, const char *key,
                                         std::string *out_path) -> bool {
    std::string raw_path;
    if (!require_value(section, key, &raw_path))
      return false;

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn("Unable to resolve path for <%s> in contract section [%s]\n",
               key, section);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn("Configured path does not exist for <%s> in contract section "
               "[%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (!std::filesystem::is_regular_file(resolved)) {
      log_warn("Configured path is not a regular file for <%s> in contract "
               "section [%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path)
      *out_path = resolved;
    return true;
  };

  const auto resolve_existing_path_if_present =
      [&](const char *section, const char *key, std::string *out_path) -> bool {
    const auto sec_it = cfg.find(section);
    if (sec_it == cfg.end())
      return false;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end())
      return false;

    const std::string raw_path = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(raw_path))
      return false;

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn(
          "Unable to resolve optional path for <%s> in contract section [%s]\n",
          key, section);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn("Configured optional path does not exist for <%s> in contract "
               "section [%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (!std::filesystem::is_regular_file(resolved)) {
      log_warn("Configured optional path is not a regular file for <%s> in "
               "contract section [%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path)
      *out_path = resolved;
    return true;
  };

  const auto require_existing_assembly_variable_path =
      [&](const char *variable_name, std::string *out_path) -> bool {
    if (assembly_section == nullptr) {
      log_warn("Missing contract section [%s]\n", kContractSectionAssembly);
      ok = false;
      return false;
    }
    const auto value_it = assembly_section->find(variable_name);
    if (value_it == assembly_section->end()) {
      log_warn("Missing assembly variable <%s> in contract [%s]\n",
               variable_name, kContractSectionAssembly);
      ok = false;
      return false;
    }

    const std::string raw_path =
        unquote_if_wrapped(trim_ascii_ws_copy(value_it->second));
    if (!has_non_ws_ascii(raw_path)) {
      log_warn("Empty assembly variable <%s> in contract [%s]\n", variable_name,
               kContractSectionAssembly);
      ok = false;
      return false;
    }

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn("Unable to resolve assembly variable path for <%s>\n",
               variable_name);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn("Configured assembly path does not exist for <%s>: %s\n",
               variable_name, resolved.c_str());
      ok = false;
      return false;
    }
    if (!std::filesystem::is_regular_file(resolved)) {
      log_warn("Configured assembly path is not a regular file for <%s>: %s\n",
               variable_name, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path)
      *out_path = resolved;
    return true;
  };

  const auto resolve_existing_assembly_variable_path_if_present =
      [&](const char *variable_name, std::string *out_path) -> bool {
    if (assembly_section == nullptr)
      return false;
    const auto value_it = assembly_section->find(variable_name);
    if (value_it == assembly_section->end())
      return false;

    const std::string raw_path =
        unquote_if_wrapped(trim_ascii_ws_copy(value_it->second));
    if (!has_non_ws_ascii(raw_path))
      return false;

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn("Unable to resolve optional assembly variable path for <%s>\n",
               variable_name);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn(
          "Configured optional assembly path does not exist for <%s>: %s\n",
          variable_name, resolved.c_str());
      ok = false;
      return false;
    }
    if (!std::filesystem::is_regular_file(resolved)) {
      log_warn("Configured optional assembly path is not a regular file for "
               "<%s>: %s\n",
               variable_name, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path)
      *out_path = resolved;
    return true;
  };

  const auto require_existing_global_path =
      [&](const char *section, const char *key, std::string *out_path) -> bool {
    std::string raw_path;
    std::string cfg_folder;
    {
      LOCK_GUARD(config_mutex);
      const auto sec_it = config_space_t::config.find(section);
      if (sec_it == config_space_t::config.end()) {
        log_warn("Missing global section [%s]\n", section);
        ok = false;
        return false;
      }
      const auto key_it = sec_it->second.find(key);
      if (key_it == sec_it->second.end()) {
        log_warn("Missing global field <%s> in section [%s]\n", key, section);
        ok = false;
        return false;
      }
      raw_path = trim_ascii_ws_copy(key_it->second);
      cfg_folder = parent_folder_from_path(config_space_t::config_file_path);
    }
    if (!has_non_ws_ascii(raw_path)) {
      log_warn("Empty global field <%s> in section [%s]\n", key, section);
      ok = false;
      return false;
    }

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn("Unable to resolve global path for <%s> in section [%s]\n", key,
               section);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn("Configured global path does not exist for <%s> in section "
               "[%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (!std::filesystem::is_regular_file(resolved)) {
      log_warn("Configured global path is not a regular file for <%s> in "
               "section [%s]: %s\n",
               key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path)
      *out_path = resolved;
    return true;
  };

  if (dock_section == nullptr) {
    log_warn("Missing contract section [%s]\n", kContractSectionDock);
    ok = false;
  }
  if (assembly_section == nullptr) {
    log_warn("Missing contract section [%s]\n", kContractSectionAssembly);
    ok = false;
  }
  if (dock_section != nullptr) {
    for (const auto &[name, value] : *dock_section) {
      (void)value;
      if (is_private_assembly_contract_variable(name)) {
        log_warn("[dconfig] DOCK variable <%s> is assembly-owned and must move "
                 "to ASSEMBLY\n",
                 name.c_str());
        ok = false;
      }
    }
  }
  if (assembly_section != nullptr) {
    for (const auto &[name, value] : *assembly_section) {
      (void)value;
      if (is_current_dock_contract_variable(name)) {
        log_warn("[dconfig] ASSEMBLY variable <%s> is dock-owned and must move "
                 "to DOCK\n",
                 name.c_str());
        ok = false;
      }
    }
  }

  {
    std::string target_indices_raw;
    if (require_value(kContractSectionDock, "__future_target_feature_indices",
                      &target_indices_raw)) {
      const auto items =
          split_string_items(unquote_if_wrapped(target_indices_raw));
      std::vector<std::int64_t> target_feature_indices{};
      target_feature_indices.reserve(items.size());
      if (items.empty()) {
        log_warn("[dconfig] DOCK __future_target_feature_indices must be a "
                 "non-empty integer list\n");
        ok = false;
      }
      for (const auto &item : items) {
        try {
          target_feature_indices.push_back(parse_scalar_from_string<int64_t>(
              unquote_if_wrapped(trim_ascii_ws_copy(item))));
        } catch (const std::exception &e) {
          log_warn("[dconfig] invalid DOCK __future_target_feature_indices "
                   "item <%s>: %s\n",
                   item.c_str(), e.what());
          ok = false;
        }
      }
      if (!target_feature_indices.empty()) {
        auto sorted_indices = target_feature_indices;
        std::sort(sorted_indices.begin(), sorted_indices.end());
        if (sorted_indices.front() < 0) {
          log_warn("[dconfig] DOCK __future_target_feature_indices must be "
                   "non-negative\n");
          ok = false;
        }
        if (std::adjacent_find(sorted_indices.begin(), sorted_indices.end()) !=
            sorted_indices.end()) {
          log_warn("[dconfig] DOCK __future_target_feature_indices must be "
                   "unique\n");
          ok = false;
        }

        std::string feature_width_raw;
        if (require_value(kContractSectionDock, "__obs_feature_dim",
                          &feature_width_raw)) {
          try {
            const auto future_feature_width =
                parse_scalar_from_string<int64_t>(feature_width_raw);
            if (future_feature_width <= 0) {
              log_warn("[dconfig] DOCK __obs_feature_dim must be positive for "
                       "__future_target_feature_indices validation\n");
              ok = false;
            } else if (sorted_indices.back() >= future_feature_width) {
              log_warn(
                  "[dconfig] DOCK __future_target_feature_indices max=%lld "
                  "is out of range for __obs_feature_dim=%lld\n",
                  static_cast<long long>(sorted_indices.back()),
                  static_cast<long long>(future_feature_width));
              ok = false;
            }
          } catch (const std::exception &e) {
            log_warn("[dconfig] invalid DOCK __obs_feature_dim while "
                     "validating __future_target_feature_indices: %s\n",
                     e.what());
            ok = false;
          }
        }
      }
    }
  }

  std::string vicreg_config_path;
  std::string vicreg_grammar_path;
  std::string vicreg_grammar_text;
  std::string expected_value_config_path;
  std::string expected_value_grammar_path;
  std::string expected_value_grammar_text;
  std::string embedding_sequence_eval_config_path;
  std::string embedding_sequence_eval_grammar_path;
  std::string embedding_sequence_eval_grammar_text;
  std::string transfer_matrix_eval_config_path;
  std::string transfer_matrix_eval_grammar_path;
  std::string transfer_matrix_eval_grammar_text;
  (void)require_existing_assembly_variable_path(
      kContractVariableVicregConfigDslFile, &vicreg_config_path);
  (void)require_existing_global_path("BNF", "vicreg_grammar_filename",
                                     &vicreg_grammar_path);
  vicreg_grammar_text =
      has_non_ws_ascii(vicreg_grammar_path)
          ? piaabo::dfiles::readFileToString(vicreg_grammar_path)
          : std::string{};
  if (!has_non_ws_ascii(vicreg_grammar_text)) {
    log_warn("[dconfig] missing/empty [BNF].vicreg_grammar_filename payload\n");
    ok = false;
  }

  const bool has_expected_value_config =
      resolve_existing_assembly_variable_path_if_present(
          kContractVariableExpectedValueConfigDslFile,
          &expected_value_config_path);
  if (has_expected_value_config) {
    (void)require_existing_global_path("BNF", "expected_value_grammar_filename",
                                       &expected_value_grammar_path);
    expected_value_grammar_text =
        piaabo::dfiles::readFileToString(expected_value_grammar_path);
    if (!has_non_ws_ascii(expected_value_grammar_text)) {
      log_warn(
          "[dconfig] empty [BNF].expected_value_grammar_filename payload\n");
      ok = false;
    }
  }

  const bool has_embedding_sequence_config =
      resolve_existing_assembly_variable_path_if_present(
          kContractVariableEmbeddingSequenceAnalyticsDslFile,
          &embedding_sequence_eval_config_path);
  if (has_embedding_sequence_config) {
    (void)require_existing_global_path(
        "BNF", kGlobalBnfEmbeddingSequenceAnalyticsGrammarKey,
        &embedding_sequence_eval_grammar_path);
    embedding_sequence_eval_grammar_text =
        piaabo::dfiles::readFileToString(embedding_sequence_eval_grammar_path);
    if (!has_non_ws_ascii(embedding_sequence_eval_grammar_text)) {
      log_warn("[dconfig] empty "
               "[BNF].wikimyei_evaluation_embedding_sequence_analytics_grammar_"
               "filename payload\n");
      ok = false;
    }
  }

  const bool has_transfer_config =
      resolve_existing_assembly_variable_path_if_present(
          kContractVariableTransferMatrixEvaluationDslFile,
          &transfer_matrix_eval_config_path);
  if (has_transfer_config) {
    (void)require_existing_global_path("BNF",
                                       kGlobalBnfTransferMatrixGrammarKey,
                                       &transfer_matrix_eval_grammar_path);
    transfer_matrix_eval_grammar_text =
        piaabo::dfiles::readFileToString(transfer_matrix_eval_grammar_path);
    if (!has_non_ws_ascii(transfer_matrix_eval_grammar_text)) {
      log_warn("[dconfig] empty [BNF].%s payload\n",
               kGlobalBnfTransferMatrixGrammarKey);
      ok = false;
    }
  }

  constexpr const char *kRequiredContractDslPathKeys[] = {
      kAssemblyKeyCircuitDslFilename,
  };
  for (const char *key : kRequiredContractDslPathKeys) {
    (void)require_existing_path(kContractSectionAssembly, key, nullptr);
  }

  constexpr const char *kRequiredGlobalGrammarPathKeys[] = {
      "tsiemene_circuit_grammar_filename",
      "canonical_path_grammar_filename",
  };
  for (const char *key : kRequiredGlobalGrammarPathKeys) {
    (void)require_existing_global_path("BNF", key, nullptr);
  }

  const bool has_train_circuit = resolve_existing_path_if_present(
      kContractSectionAssembly, "circuit_train_dsl_filename", nullptr);
  const bool has_run_circuit = resolve_existing_path_if_present(
      kContractSectionAssembly, "circuit_run_dsl_filename", nullptr);
  if (has_train_circuit || has_run_circuit) {
    log_warn("[dconfig] split circuit keys "
             "<circuit_train_dsl_filename>/<circuit_run_dsl_filename> "
             "are removed; use ASSEMBLY.CIRCUIT_FILE\n");
    ok = false;
  }

  std::string circuit_dsl_path{};
  std::string circuit_grammar_path{};
  (void)require_existing_path(kContractSectionAssembly,
                              kAssemblyKeyCircuitDslFilename,
                              &circuit_dsl_path);
  (void)require_existing_global_path("BNF", "tsiemene_circuit_grammar_filename",
                                     &circuit_grammar_path);

  if (has_non_ws_ascii(circuit_dsl_path) &&
      has_non_ws_ascii(circuit_grammar_path)) {
    std::string circuit_dsl_text =
        piaabo::dfiles::readFileToString(circuit_dsl_path);
    const std::string circuit_grammar_text =
        piaabo::dfiles::readFileToString(circuit_grammar_path);
    if (contract_variable_section != nullptr &&
        !contract_variable_section->empty()) {
      circuit_dsl_text = resolve_dsl_variables_or_fail(
          circuit_dsl_text,
          contract_dsl_variables_from_section(*contract_variable_section),
          circuit_dsl_path);
    }
    if (!has_non_ws_ascii(circuit_dsl_text) ||
        !has_non_ws_ascii(circuit_grammar_text)) {
      log_warn("[dconfig] empty circuit DSL or grammar payload\n");
      ok = false;
    } else {
      try {
        auto circuit_parser =
            cuwacunu::camahjucunu::dsl::tsiemeneCircuits(circuit_grammar_text);
        const auto instruction = circuit_parser.decode(circuit_dsl_text);

        const auto ack_it = cfg.find(kContractSectionAknowledge);
        if (ack_it == cfg.end() || ack_it->second.empty()) {
          log_warn("[dconfig] missing AKNOWLEDGE declarations in ASSEMBLY\n");
          ok = false;
        } else {
          std::unordered_set<std::string> instance_aliases{};
          for (const auto &circuit : instruction.circuits) {
            for (const auto &instance : circuit.instances) {
              const std::string alias = trim_ascii_ws_copy(instance.alias);
              const std::string instance_type =
                  trim_ascii_ws_copy(instance.tsi_type);
              if (!has_non_ws_ascii(alias) || !has_non_ws_ascii(instance_type))
                continue;
              instance_aliases.insert(alias);

              const auto ack_type_it = ack_it->second.find(alias);
              if (ack_type_it == ack_it->second.end()) {
                log_warn("[dconfig] missing AKNOWLEDGE declaration for circuit "
                         "alias '%s'\n",
                         alias.c_str());
                ok = false;
                continue;
              }
              const std::string ack_type =
                  trim_ascii_ws_copy(ack_type_it->second);
              const auto instance_type_id =
                  tsiemene::parse_tsi_type_id(instance_type);
              const auto ack_type_id = tsiemene::parse_tsi_type_id(ack_type);
              if (!instance_type_id.has_value() || !ack_type_id.has_value()) {
                log_warn(
                    "[dconfig] invalid AKNOWLEDGE type for alias '%s': %s\n",
                    alias.c_str(), ack_type.c_str());
                ok = false;
                continue;
              }
              if (*instance_type_id != *ack_type_id) {
                log_warn("[dconfig] AKNOWLEDGE type mismatch for alias '%s': "
                         "circuit=%s acknowledge=%s\n",
                         alias.c_str(), instance_type.c_str(),
                         ack_type.c_str());
                ok = false;
              }
              const std::string expected_ack =
                  std::string(tsiemene::tsi_type_token(*ack_type_id));
              if (ack_type != expected_ack) {
                log_warn("[dconfig] AKNOWLEDGE for alias '%s' must use family "
                         "token '%s' (got '%s')\n",
                         alias.c_str(), expected_ack.c_str(), ack_type.c_str());
                ok = false;
              }
            }
          }

          for (const auto &[alias, family] : ack_it->second) {
            (void)family;
            if (instance_aliases.find(alias) != instance_aliases.end())
              continue;
            log_warn("[dconfig] AKNOWLEDGE alias '%s' does not exist in "
                     "circuit instances\n",
                     alias.c_str());
            ok = false;
          }
        }
      } catch (const std::exception &e) {
        log_warn("[dconfig] invalid circuit DSL while validating contract "
                 "wrapper: %s\n",
                 e.what());
        ok = false;
      }
    }
  }

  constexpr std::array<const char *, 4> kReservedModuleSections = {
      kModuleSectionVicreg,
      kModuleSectionExpectedValue,
      kModuleSectionEmbeddingSequenceAnalytics,
      kModuleSectionTransferMatrixEvaluation,
  };
  for (const char *section : kReservedModuleSections) {
    const auto sec_it = cfg.find(section);
    if (sec_it == cfg.end())
      continue;
    bool has_payload = false;
    for (const auto &[key, value] : sec_it->second) {
      if (!has_non_ws_ascii(trim_ascii_ws_copy(value)))
        continue;
      log_warn("[dconfig] contract section [%s] key <%s> is not allowed; "
               "module sections are loaded from ASSEMBLY-owned module config "
               "variables\n",
               section, key.c_str());
      has_payload = true;
    }
    if (has_payload)
      ok = false;
  }

  std::unordered_map<std::string, std::string> component_tag_owners;
  const auto validate_module_instruction_file =
      [&](const char *module_name, const std::string &module_path,
          const std::string &module_grammar_text,
          std::initializer_list<const char *> string_keys,
          std::initializer_list<const char *> int_keys,
          std::initializer_list<const char *> float_keys,
          std::initializer_list<const char *> bool_keys,
          std::initializer_list<const char *> arr_int_keys,
          std::initializer_list<const char *> arr_float_keys,
          std::initializer_list<const char *> optional_string_keys,
          std::initializer_list<const char *> optional_int_keys,
          std::initializer_list<const char *> optional_float_keys,
          std::initializer_list<const char *> optional_bool_keys,
          std::initializer_list<const char *> optional_arr_int_keys,
          std::initializer_list<const char *> optional_arr_float_keys) {
        if (!has_non_ws_ascii(module_path))
          return;
        if (!has_non_ws_ascii(module_grammar_text)) {
          log_warn(
              "[dconfig] missing grammar payload for module config [%s]: %s\n",
              module_name, module_path.c_str());
          ok = false;
          return;
        }

        const parsed_config_section_t module_values = parse_instruction_file(
            module_path, module_grammar_text, contract_variable_section);
        const auto lower_trim_ascii = [](std::string value) -> std::string {
          value = trim_ascii_ws_copy(std::move(value));
          std::transform(value.begin(), value.end(), value.begin(),
                         [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                         });
          return value;
        };
        const auto strip_type_prefixes = [&](std::string value) -> std::string {
          value = lower_trim_ascii(std::move(value));
          if (value.rfind("torch::", 0) == 0)
            value = value.substr(7);
          if (value.rfind("at::", 0) == 0)
            value = value.substr(4);
          if (value.rfind("k", 0) == 0 && value.size() > 1 &&
              std::isalpha(static_cast<unsigned char>(value[1]))) {
            value = value.substr(1);
          }
          return value;
        };
        const auto is_valid_dtype_token = [&](std::string value) -> bool {
          value = strip_type_prefixes(std::move(value));
          return value == "bool" || value == "int8" || value == "int16" ||
                 value == "int32" || value == "int64" || value == "float16" ||
                 value == "half" || value == "f16" || value == "float32" ||
                 value == "float" || value == "f32" || value == "float64" ||
                 value == "double" || value == "f64";
        };
        const auto is_valid_device_token = [&](std::string value) -> bool {
          value = lower_trim_ascii(std::move(value));
          if (value == "cpu" || value == "cuda" || value == "gpu")
            return true;
          if (value == "torch::kcpu" || value == "kcpu" ||
              value == "torch::kcuda" || value == "kcuda") {
            return true;
          }
          if (value.rfind("cuda:", 0) == 0 || value.rfind("gpu:", 0) == 0) {
            const std::string suffix = value.substr(value.find(':') + 1);
            return !suffix.empty() &&
                   std::all_of(suffix.begin(), suffix.end(),
                               [](unsigned char c) { return std::isdigit(c); });
          }
          return false;
        };

        std::unordered_set<std::string> allowed_keys;
        const auto register_allowed_keys =
            [&](std::initializer_list<const char *> keys) {
              for (const char *key : keys) {
                allowed_keys.insert(std::string(key));
              }
            };
        register_allowed_keys(string_keys);
        register_allowed_keys(int_keys);
        register_allowed_keys(float_keys);
        register_allowed_keys(bool_keys);
        register_allowed_keys(arr_int_keys);
        register_allowed_keys(arr_float_keys);
        register_allowed_keys(optional_string_keys);
        register_allowed_keys(optional_int_keys);
        register_allowed_keys(optional_float_keys);
        register_allowed_keys(optional_bool_keys);
        register_allowed_keys(optional_arr_int_keys);
        register_allowed_keys(optional_arr_float_keys);

        for (const auto &[key, value] : module_values) {
          (void)value;
          if (allowed_keys.find(key) != allowed_keys.end())
            continue;
          log_warn("Unknown key <%s> in module config [%s] file: %s\n",
                   key.c_str(), module_name, module_path.c_str());
          ok = false;
        }

        const auto read_module_value = [&](const char *key, bool required,
                                           std::string *out_value) -> bool {
          const auto it = module_values.find(key);
          if (it == module_values.end()) {
            if (required) {
              log_warn("Missing key <%s> in module config [%s] file: %s\n", key,
                       module_name, module_path.c_str());
              ok = false;
            }
            return false;
          }

          const std::string value = trim_ascii_ws_copy(it->second);
          if (!has_non_ws_ascii(value)) {
            log_warn("Empty key <%s> in module config [%s] file: %s\n", key,
                     module_name, module_path.c_str());
            ok = false;
            return false;
          }

          if (out_value)
            *out_value = value;
          return true;
        };

        const auto validate_string_key = [&](const char *key, bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          if (std::string_view(key) == "dtype" &&
              !is_valid_dtype_token(value)) {
            log_warn("Invalid dtype token in module config [%s] file %s: %s\n",
                     module_name, module_path.c_str(), value.c_str());
            ok = false;
          }
          if (std::string_view(key) == "device" &&
              !is_valid_device_token(value)) {
            log_warn("Invalid device token in module config [%s] file %s: %s\n",
                     module_name, module_path.c_str(), value.c_str());
            ok = false;
          }
          if (std::string_view(key) == "TAG") {
            const std::string component_tag =
                unquote_if_wrapped(trim_ascii_ws_copy(std::move(value)));
            if (!is_valid_component_tag_token(component_tag)) {
              log_warn("Invalid component TAG token in module config [%s] "
                       "file %s: %s\n",
                       module_name, module_path.c_str(),
                       component_tag.c_str());
              ok = false;
              return;
            }
            const std::string owner =
                std::string(module_name) + " file " + module_path;
            const auto [it, inserted] =
                component_tag_owners.emplace(component_tag, owner);
            if (!inserted) {
              log_warn("Duplicate component TAG <%s> in module config [%s] "
                       "file %s; already used by %s\n",
                       component_tag.c_str(), module_name,
                       module_path.c_str(), it->second.c_str());
              ok = false;
            }
          }
        };

        const auto validate_int_key = [&](const char *key, bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          try {
            (void)parse_scalar_from_string<int64_t>(value);
          } catch (const std::exception &e) {
            log_warn("Invalid int value for <%s> in module config [%s] file "
                     "%s: %s\n",
                     key, module_name, module_path.c_str(), e.what());
            ok = false;
          }
        };

        const auto validate_float_key = [&](const char *key, bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          try {
            (void)parse_scalar_from_string<double>(value);
          } catch (const std::exception &e) {
            log_warn("Invalid float value for <%s> in module config [%s] file "
                     "%s: %s\n",
                     key, module_name, module_path.c_str(), e.what());
            ok = false;
          }
        };

        const auto validate_bool_key = [&](const char *key, bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          try {
            (void)parse_scalar_from_string<bool>(value);
          } catch (const std::exception &e) {
            log_warn("Invalid bool value for <%s> in module config [%s] file "
                     "%s: %s\n",
                     key, module_name, module_path.c_str(), e.what());
            ok = false;
          }
        };

        const auto validate_int_array_key = [&](const char *key,
                                                bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          const auto items = split_string_items(value);
          if (items.empty()) {
            log_warn(
                "Empty integer array for <%s> in module config [%s] file: %s\n",
                key, module_name, module_path.c_str());
            ok = false;
            return;
          }
          for (const auto &item : items) {
            try {
              (void)parse_scalar_from_string<int64_t>(item);
            } catch (const std::exception &e) {
              log_warn("Invalid integer array item for <%s> in module config "
                       "[%s] file %s: %s\n",
                       key, module_name, module_path.c_str(), e.what());
              ok = false;
            }
          }
        };

        const auto validate_float_array_key = [&](const char *key,
                                                  bool required) {
          std::string value;
          if (!read_module_value(key, required, &value))
            return;
          const auto items = split_string_items(value);
          if (items.empty()) {
            log_warn(
                "Empty float array for <%s> in module config [%s] file: %s\n",
                key, module_name, module_path.c_str());
            ok = false;
            return;
          }
          for (const auto &item : items) {
            try {
              (void)parse_scalar_from_string<double>(item);
            } catch (const std::exception &e) {
              log_warn("Invalid float array item for <%s> in module config "
                       "[%s] file %s: %s\n",
                       key, module_name, module_path.c_str(), e.what());
              ok = false;
            }
          }
        };

        for (const char *key : string_keys)
          validate_string_key(key, /*required=*/true);
        for (const char *key : optional_string_keys) {
          validate_string_key(key, /*required=*/false);
        }

        for (const char *key : int_keys)
          validate_int_key(key, /*required=*/true);
        for (const char *key : optional_int_keys)
          validate_int_key(key, /*required=*/false);

        for (const char *key : float_keys)
          validate_float_key(key, /*required=*/true);
        for (const char *key : optional_float_keys) {
          validate_float_key(key, /*required=*/false);
        }
