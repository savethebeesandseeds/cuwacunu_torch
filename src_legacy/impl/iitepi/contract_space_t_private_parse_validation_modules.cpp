
for (const char *key : bool_keys)
  validate_bool_key(key, /*required=*/true);
for (const char *key : optional_bool_keys)
  validate_bool_key(key, /*required=*/false);

for (const char *key : arr_int_keys) {
  validate_int_array_key(key, /*required=*/true);
}
for (const char *key : optional_arr_int_keys) {
  validate_int_array_key(key, /*required=*/false);
}

for (const char *key : arr_float_keys) {
  validate_float_array_key(key, /*required=*/true);
}
for (const char *key : optional_arr_float_keys) {
  validate_float_array_key(key, /*required=*/false);
}
}
;

validate_module_instruction_file(kModuleSectionVicreg, vicreg_config_path,
                                 vicreg_grammar_text,
                                 /* string_keys */
                                 {"TAG", "dtype", "device", "jkimyei_dsl_file",
                                  "network_design_dsl_file"},
                                 /* int_keys */
                                 {},
                                 /* float_keys */
                                 {},
                                 /* bool_keys */
                                 {"enable_buffer_averaging"},
                                 /* arr_int_keys */
                                 {},
                                 /* arr_float_keys */
                                 {},
                                 /* optional_string_keys */
                                 {},
                                 /* optional_int_keys */
                                 {},
                                 /* optional_float_keys */
                                 {},
                                 /* optional_bool_keys */
                                 {},
                                 /* optional_arr_int_keys */
                                 {},
                                 /* optional_arr_float_keys */
                                 {});

validate_module_instruction_file(
    kModuleSectionExpectedValue, expected_value_config_path,
    expected_value_grammar_text,
    /* string_keys */
    {"TAG", "model_path", "dtype", "device", "network_design_dsl_file",
     "jkimyei_dsl_file"},
    /* int_keys */
    {"telemetry_every", "optimizer_threshold_reset"},
    /* float_keys */
    {"grad_clip"},
    /* bool_keys */
    {"verbose_train"},
    /* arr_int_keys */
    {},
    /* arr_float_keys */
    {"target_weights"},
    /* optional_string_keys */
    {},
    /* optional_int_keys */
    {},
    /* optional_float_keys */
    {},
    /* optional_bool_keys */
    {},
    /* optional_arr_int_keys */
    {},
    /* optional_arr_float_keys */
    {});

validate_module_instruction_file(kModuleSectionEmbeddingSequenceAnalytics,
                                 embedding_sequence_eval_config_path,
                                 embedding_sequence_eval_grammar_text,
                                 /* string_keys */
                                 {},
                                 /* int_keys */
                                 {},
                                 /* float_keys */
                                 {},
                                 /* bool_keys */
                                 {},
                                 /* arr_int_keys */
                                 {},
                                 /* arr_float_keys */
                                 {},
                                 /* optional_string_keys */
                                 {},
                                 /* optional_int_keys */
                                 {"max_samples", "max_features"},
                                 /* optional_float_keys */
                                 {"mask_epsilon", "standardize_epsilon"},
                                 /* optional_bool_keys */
                                 {},
                                 /* optional_arr_int_keys */
                                 {},
                                 /* optional_arr_float_keys */
                                 {});

validate_module_instruction_file(
    kModuleSectionTransferMatrixEvaluation, transfer_matrix_eval_config_path,
    transfer_matrix_eval_grammar_text,
    /* string_keys */
    {},
    /* int_keys */
    {},
    /* float_keys */
    {},
    /* bool_keys */
    {},
    /* arr_int_keys */
    {},
    /* arr_float_keys */
    {},
    /* optional_string_keys */
    {"dtype", "device"},
    /* optional_int_keys */
    {"summary_every_steps", "mdn_mixture_comps", "mdn_features_hidden",
     "mdn_residual_depth", "prequential_blocks", "control_shuffle_block",
     "control_shuffle_seed"},
    /* optional_float_keys */
    {"optimizer_lr", "optimizer_weight_decay", "optimizer_beta1",
     "optimizer_beta2", "optimizer_eps", "grad_clip", "nll_eps",
     "nll_sigma_min", "nll_sigma_max", "anchor_train_ratio", "anchor_val_ratio",
     "anchor_test_ratio", "linear_ridge_lambda", "gaussian_var_min"},
    /* optional_bool_keys */
    {"check_temporal_order", "validate_vicreg_out", "report_shapes",
     "reset_hashimyei_on_start"},
    /* optional_arr_int_keys */
    {"mdn_target_feature_indices"},
    /* optional_arr_float_keys */
    {});

const auto validate_jkimyei_dsl_file =
    [&](const char *module_name, const std::string &module_config_path,
        const parsed_config_section_t &module_values) {
      const auto jkimyei_dsl_it = module_values.find("jkimyei_dsl_file");
      if (jkimyei_dsl_it == module_values.end()) {
        log_warn("Missing key <jkimyei_dsl_file> in module config [%s] file: "
                 "%s\n",
                 module_name, module_config_path.c_str());
        ok = false;
        return;
      }
      const std::string raw_jkimyei_path =
          unquote_if_wrapped(trim_ascii_ws_copy(jkimyei_dsl_it->second));
      if (!has_non_ws_ascii(raw_jkimyei_path)) {
        log_warn("Empty key <jkimyei_dsl_file> in module config [%s] file: "
                 "%s\n",
                 module_name, module_config_path.c_str());
        ok = false;
        return;
      }

      std::string module_folder{};
      const std::filesystem::path module_path_fs(module_config_path);
      if (module_path_fs.has_parent_path()) {
        module_folder = module_path_fs.parent_path().string();
      }
      const std::string resolved_jkimyei_path =
          resolve_path_from_folder(module_folder, raw_jkimyei_path);
      if (!has_non_ws_ascii(resolved_jkimyei_path) ||
          !std::filesystem::exists(resolved_jkimyei_path) ||
          !std::filesystem::is_regular_file(resolved_jkimyei_path)) {
        log_warn("Invalid path for <jkimyei_dsl_file> in module config [%s] "
                 "file %s: %s\n",
                 module_name, module_config_path.c_str(),
                 resolved_jkimyei_path.c_str());
        ok = false;
        return;
      }

      std::string jkimyei_grammar_path;
      if (!require_existing_global_path("BNF", "jkimyei_specs_grammar_filename",
                                        &jkimyei_grammar_path)) {
        return;
      }
      const std::string jkimyei_grammar_text =
          piaabo::dfiles::readFileToString(jkimyei_grammar_path);
      const std::string jkimyei_dsl_text =
          piaabo::dfiles::readFileToString(resolved_jkimyei_path);
      if (!has_non_ws_ascii(jkimyei_grammar_text)) {
        log_warn("[dconfig] empty [BNF].jkimyei_specs_grammar_filename "
                 "payload\n");
        ok = false;
        return;
      }
      if (!has_non_ws_ascii(jkimyei_dsl_text)) {
        log_warn("Empty jkimyei DSL payload in module config [%s] file %s: "
                 "%s\n",
                 module_name, module_config_path.c_str(),
                 resolved_jkimyei_path.c_str());
        ok = false;
        return;
      }
      try {
        (void)cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
            jkimyei_grammar_text, jkimyei_dsl_text, resolved_jkimyei_path);
      } catch (const std::exception &e) {
        log_warn("Invalid jkimyei DSL referenced by module config [%s] file "
                 "%s: %s\n",
                 module_name, module_config_path.c_str(), e.what());
        ok = false;
      }
    };

{
  const parsed_config_section_t vicreg_values = parse_instruction_file(
      vicreg_config_path, vicreg_grammar_text, contract_variable_section);
  validate_jkimyei_dsl_file(kModuleSectionVicreg, vicreg_config_path,
                            vicreg_values);

  const auto net_dsl_it = vicreg_values.find("network_design_dsl_file");
  if (net_dsl_it == vicreg_values.end()) {
    log_warn("Missing key <network_design_dsl_file> in module config "
             "[VICReg] file: %s\n",
             vicreg_config_path.c_str());
    ok = false;
  } else {
    const std::string raw_network_design_path =
        unquote_if_wrapped(trim_ascii_ws_copy(net_dsl_it->second));
    if (!has_non_ws_ascii(raw_network_design_path)) {
      log_warn("Empty key <network_design_dsl_file> in module config "
               "[VICReg] file: %s\n",
               vicreg_config_path.c_str());
      ok = false;
    } else {
      std::string vicreg_folder{};
      const std::filesystem::path vicreg_path_fs(vicreg_config_path);
      if (vicreg_path_fs.has_parent_path()) {
        vicreg_folder = vicreg_path_fs.parent_path().string();
      }
      const std::string resolved_network_design_path =
          resolve_path_from_folder(vicreg_folder, raw_network_design_path);
      if (!has_non_ws_ascii(resolved_network_design_path) ||
          !std::filesystem::exists(resolved_network_design_path) ||
          !std::filesystem::is_regular_file(resolved_network_design_path)) {
        log_warn("Invalid path for <network_design_dsl_file> in module "
                 "config [VICReg] file %s: %s\n",
                 vicreg_config_path.c_str(),
                 resolved_network_design_path.c_str());
        ok = false;
      } else {
        std::string network_design_grammar_path;
        if (require_existing_global_path("BNF",
                                         "network_design_grammar_filename",
                                         &network_design_grammar_path)) {
          const std::string network_design_grammar_text =
              piaabo::dfiles::readFileToString(network_design_grammar_path);
          std::string network_design_dsl_text =
              piaabo::dfiles::readFileToString(resolved_network_design_path);
          if (contract_variable_section != nullptr &&
              !contract_variable_section->empty()) {
            network_design_dsl_text = resolve_dsl_variables_or_fail(
                network_design_dsl_text,
                contract_dsl_variables_from_section(*contract_variable_section),
                resolved_network_design_path);
          }
          if (!has_non_ws_ascii(network_design_grammar_text)) {
            log_warn("[dconfig] empty [BNF].network_design_grammar_filename "
                     "payload\n");
            ok = false;
          } else if (!has_non_ws_ascii(network_design_dsl_text)) {
            log_warn("Empty network_design DSL payload in module config "
                     "[VICReg] file %s: %s\n",
                     vicreg_config_path.c_str(),
                     resolved_network_design_path.c_str());
            ok = false;
          } else {
            try {
              const auto network_design =
                  cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
                      network_design_grammar_text, network_design_dsl_text);
              const auto var_value_or_empty =
                  [&](std::string_view key) -> std::string {
                if (contract_variable_section == nullptr)
                  return {};
                const auto it =
                    contract_variable_section->find(std::string(key));
                if (it == contract_variable_section->end())
                  return {};
                return unquote_if_wrapped(trim_ascii_ws_copy(it->second));
              };
              const std::string observation_sources_path =
                  resolve_path_from_folder(
                      cfg_folder,
                      var_value_or_empty("__observation_sources_dsl_file"));
              const std::string observation_channels_path =
                  resolve_path_from_folder(
                      cfg_folder,
                      var_value_or_empty("__observation_channels_dsl_file"));
              const bool has_observation_sources =
                  has_non_ws_ascii(observation_sources_path);
              const bool has_observation_channels =
                  has_non_ws_ascii(observation_channels_path);
              if (has_observation_sources != has_observation_channels) {
                log_warn("[dconfig] contract-owned observation docking "
                         "requires both "
                         "__observation_sources_dsl_file and "
                         "__observation_channels_dsl_file when "
                         "VICReg.network_design_dsl_file is set\n");
                ok = false;
              } else if (has_observation_sources && has_observation_channels) {
                std::string observation_sources_grammar_path{};
                std::string observation_channels_grammar_path{};
                if (require_existing_global_path(
                        "BNF", "observation_sources_grammar_filename",
                        &observation_sources_grammar_path) &&
                    require_existing_global_path(
                        "BNF", "observation_channels_grammar_filename",
                        &observation_channels_grammar_path)) {
                  if (!std::filesystem::exists(observation_sources_path) ||
                      !std::filesystem::is_regular_file(
                          observation_sources_path)) {
                    log_warn("[dconfig] invalid contract "
                             "__observation_sources_dsl_file path: %s\n",
                             observation_sources_path.c_str());
                    ok = false;
                  } else if (!std::filesystem::exists(
                                 observation_channels_path) ||
                             !std::filesystem::is_regular_file(
                                 observation_channels_path)) {
                    log_warn("[dconfig] invalid contract "
                             "__observation_channels_dsl_file path: %s\n",
                             observation_channels_path.c_str());
                    ok = false;
                  } else {
                    const std::string observation_sources_grammar_text =
                        piaabo::dfiles::readFileToString(
                            observation_sources_grammar_path);
                    const std::string observation_channels_grammar_text =
                        piaabo::dfiles::readFileToString(
                            observation_channels_grammar_path);
                    const std::string observation_sources_dsl_text =
                        piaabo::dfiles::readFileToString(
                            observation_sources_path);
                    const std::string observation_channels_dsl_text =
                        piaabo::dfiles::readFileToString(
                            observation_channels_path);
                    if (!has_non_ws_ascii(observation_sources_grammar_text) ||
                        !has_non_ws_ascii(observation_channels_grammar_text)) {
                      log_warn("[dconfig] empty observation grammar payload "
                               "while validating contract docking\n");
                      ok = false;
                    } else if (!has_non_ws_ascii(
                                   observation_sources_dsl_text) ||
                               !has_non_ws_ascii(
                                   observation_channels_dsl_text)) {
                      log_warn("[dconfig] empty observation DSL payload "
                               "while validating contract docking\n");
                      ok = false;
                    } else {
                      try {
                        auto observation = cuwacunu::camahjucunu::
                            decode_observation_spec_from_split_dsl(
                                observation_sources_grammar_text,
                                observation_sources_dsl_text,
                                observation_channels_grammar_text,
                                observation_channels_dsl_text);
                        cuwacunu::wikimyei::vicreg_rank4::
                            vicreg_network_design_spec_t semantic{};
                        std::string semantic_error{};
                        if (!cuwacunu::wikimyei::vicreg_rank4::
                                resolve_vicreg_network_design(
                                    network_design, &semantic,
                                    &semantic_error)) {
                          log_warn("[dconfig] invalid VICReg network_design "
                                   "semantic payload while validating "
                                   "docking: %s\n",
                                   semantic_error.c_str());
                          ok = false;
                        } else {
                          const auto observed_channels =
                              observation.count_channels();
                          const auto observed_seq_length =
                              observation.max_sequence_length();
                          const std::string configured_channels_text =
                              var_value_or_empty("__obs_channels");
                          const std::string configured_seq_length_text =
                              var_value_or_empty("__obs_seq_length");
                          const std::string configured_embedding_dims_text =
                              var_value_or_empty("__embedding_dims");
                          const std::string configured_feature_dim_text =
                              var_value_or_empty("__obs_feature_dim");
                          const auto parse_positive_contract_int =
                              [&](std::string_view key, const std::string &text,
                                  long long *out_value) -> bool {
                            if (out_value == nullptr) {
                              throw std::runtime_error(
                                  "parse_positive_contract_int received null "
                                  "output");
                            }
                            if (!has_non_ws_ascii(text))
                              return false;
                            long long value = 0;
                            const auto [ptr, ec] = std::from_chars(
                                text.data(), text.data() + text.size(), value);
                            if (ec != std::errc{} ||
                                ptr != text.data() + text.size() ||
                                value <= 0) {
                              log_warn("[dconfig] invalid contract %.*s while "
                                       "validating observation docking: %s\n",
                                       static_cast<int>(key.size()), key.data(),
                                       text.c_str());
                              ok = false;
                              return false;
                            }
                            *out_value = value;
                            return true;
                          };
                          long long configured_channels = 0;
                          if (parse_positive_contract_int(
                                  "__obs_channels", configured_channels_text,
                                  &configured_channels) &&
                              configured_channels != observed_channels) {
                            log_warn(
                                "[dconfig] contract __obs_channels=%lld does "
                                "not match observation active channel "
                                "count=%lld derived from "
                                "__observation_channels_dsl_file\n",
                                configured_channels,
                                static_cast<long long>(observed_channels));
                            ok = false;
                          }
                          long long configured_seq_length = 0;
                          if (parse_positive_contract_int(
                                  "__obs_seq_length",
                                  configured_seq_length_text,
                                  &configured_seq_length) &&
                              configured_seq_length != observed_seq_length) {
                            log_warn(
                                "[dconfig] contract __obs_seq_length=%lld "
                                "does not match observation max active "
                                "seq_length=%lld derived from "
                                "__observation_channels_dsl_file\n",
                                configured_seq_length,
                                static_cast<long long>(observed_seq_length));
                            ok = false;
                          }
                          long long configured_embedding_dims = 0;
                          if (parse_positive_contract_int(
                                  "__embedding_dims",
                                  configured_embedding_dims_text,
                                  &configured_embedding_dims) &&
                              configured_embedding_dims !=
                                  semantic.encoding_dims) {
                            log_warn("[dconfig] contract __embedding_dims=%lld "
                                     "does not match VICReg network_design "
                                     "encoder.encoding_dims=%d\n",
                                     configured_embedding_dims,
                                     semantic.encoding_dims);
                            ok = false;
                          }
                          if (semantic.C != observed_channels) {
                            log_warn(
                                "[dconfig] VICReg network_design INPUT.C=%d "
                                "does not match contract observation active "
                                "channel count=%lld\n",
                                semantic.C,
                                static_cast<long long>(observed_channels));
                            ok = false;
                          }
                          if (semantic.T != observed_seq_length) {
                            log_warn(
                                "[dconfig] VICReg network_design INPUT.T=%d "
                                "does not match contract observation max "
                                "seq_length=%lld\n",
                                semantic.T,
                                static_cast<long long>(observed_seq_length));
                            ok = false;
                          }
                          if (has_non_ws_ascii(configured_feature_dim_text)) {
                            int configured_feature_dim = 0;
                            const auto [ptr, ec] = std::from_chars(
                                configured_feature_dim_text.data(),
                                configured_feature_dim_text.data() +
                                    configured_feature_dim_text.size(),
                                configured_feature_dim);
                            if (ec != std::errc{} ||
                                ptr != configured_feature_dim_text.data() +
                                           configured_feature_dim_text.size() ||
                                configured_feature_dim <= 0) {
                              log_warn("[dconfig] invalid contract "
                                       "__obs_feature_dim while validating "
                                       "VICReg docking: %s\n",
                                       configured_feature_dim_text.c_str());
                              ok = false;
                            } else if (semantic.D != configured_feature_dim) {
                              log_warn("[dconfig] VICReg network_design "
                                       "INPUT.D=%d does not match contract "
                                       "__obs_feature_dim=%d\n",
                                       semantic.D, configured_feature_dim);
                              ok = false;
                            }
                          }
                        }
                      } catch (const std::exception &e) {
                        log_warn("[dconfig] invalid contract observation DSL "
                                 "while validating VICReg docking: %s\n",
                                 e.what());
                        ok = false;
                      }
                    }
                  }
                }
              }
            } catch (const std::exception &e) {
              log_warn("Invalid network_design DSL referenced by module "
                       "config [VICReg] file %s: %s\n",
                       vicreg_config_path.c_str(), e.what());
              ok = false;
            }
          }
        }
      }
    }
  }
}

if (has_expected_value_config) {
  const parsed_config_section_t expected_value_values = parse_instruction_file(
      expected_value_config_path, expected_value_grammar_text,
      contract_variable_section);
  validate_jkimyei_dsl_file(kModuleSectionExpectedValue,
                            expected_value_config_path, expected_value_values);
  const auto net_dsl_it = expected_value_values.find("network_design_dsl_file");
  if (net_dsl_it == expected_value_values.end()) {
    log_warn("Missing key <network_design_dsl_file> in module config "
             "[EXPECTED_VALUE] file: %s\n",
             expected_value_config_path.c_str());
    ok = false;
  } else {
    const std::string raw_network_design_path =
        unquote_if_wrapped(trim_ascii_ws_copy(net_dsl_it->second));
    std::string expected_value_folder{};
    const std::filesystem::path expected_value_path_fs(
        expected_value_config_path);
    if (expected_value_path_fs.has_parent_path()) {
      expected_value_folder = expected_value_path_fs.parent_path().string();
    }
    const std::string resolved_network_design_path = resolve_path_from_folder(
        expected_value_folder, raw_network_design_path);
    if (!has_non_ws_ascii(resolved_network_design_path) ||
        !std::filesystem::exists(resolved_network_design_path) ||
        !std::filesystem::is_regular_file(resolved_network_design_path)) {
      log_warn("Invalid path for <network_design_dsl_file> in module config "
               "[EXPECTED_VALUE] file %s: %s\n",
               expected_value_config_path.c_str(),
               resolved_network_design_path.c_str());
      ok = false;
    } else {
      std::string network_design_grammar_path;
      if (require_existing_global_path("BNF", "network_design_grammar_filename",
                                       &network_design_grammar_path)) {
        const std::string network_design_grammar_text =
            piaabo::dfiles::readFileToString(network_design_grammar_path);
        std::string network_design_dsl_text =
            piaabo::dfiles::readFileToString(resolved_network_design_path);
        if (contract_variable_section != nullptr &&
            !contract_variable_section->empty()) {
          network_design_dsl_text = resolve_dsl_variables_or_fail(
              network_design_dsl_text,
              contract_dsl_variables_from_section(*contract_variable_section),
              resolved_network_design_path);
        }
        if (!has_non_ws_ascii(network_design_grammar_text)) {
          log_warn("[dconfig] empty [BNF].network_design_grammar_filename "
                   "payload\n");
          ok = false;
        } else if (!has_non_ws_ascii(network_design_dsl_text)) {
          log_warn("Empty network_design DSL payload in module config "
                   "[EXPECTED_VALUE] file %s: %s\n",
                   expected_value_config_path.c_str(),
                   resolved_network_design_path.c_str());
          ok = false;
        } else {
          try {
            const auto network_design =
                cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
                    network_design_grammar_text, network_design_dsl_text);
            cuwacunu::wikimyei::expected_value_network_design_spec_t semantic{};
            std::string semantic_error{};
            if (!cuwacunu::wikimyei::resolve_expected_value_network_design(
                    network_design, &semantic, &semantic_error)) {
              log_warn("[dconfig] invalid ExpectedValue network_design "
                       "semantic payload: %s\n",
                       semantic_error.c_str());
              ok = false;
            } else {
              const auto var_value_or_empty =
                  [&](std::string_view key) -> std::string {
                if (contract_variable_section == nullptr)
                  return {};
                const auto it =
                    contract_variable_section->find(std::string(key));
                if (it == contract_variable_section->end())
                  return {};
                return unquote_if_wrapped(trim_ascii_ws_copy(it->second));
              };
              const auto parse_contract_int =
                  [&](std::string_view key, const std::string &text,
                      long long *out_value) -> bool {
                if (out_value == nullptr)
                  throw std::runtime_error(
                      "parse_contract_int received null output");
                if (!has_non_ws_ascii(text))
                  return false;
                long long value = 0;
                const auto [ptr, ec] = std::from_chars(
                    text.data(), text.data() + text.size(), value);
                if (ec != std::errc{} || ptr != text.data() + text.size()) {
                  log_warn("[dconfig] invalid contract %.*s while validating "
                           "ExpectedValue docking: %s\n",
                           static_cast<int>(key.size()), key.data(),
                           text.c_str());
                  ok = false;
                  return false;
                }
                *out_value = value;
                return true;
              };
              const auto parse_contract_int_list =
                  [&](std::string_view key, const std::string &text,
                      std::vector<long long> *out_values) -> bool {
                if (out_values == nullptr)
                  throw std::runtime_error(
                      "parse_contract_int_list received null output");
                out_values->clear();
                if (!has_non_ws_ascii(text))
                  return false;
                std::string list = trim_ascii_ws_copy(text);
                if (list.size() >= 2 && list.front() == '[' &&
                    list.back() == ']') {
                  list = trim_ascii_ws_copy(list.substr(1, list.size() - 2));
                }
                std::stringstream ss(list);
                std::string tok;
                while (std::getline(ss, tok, ',')) {
                  tok = trim_ascii_ws_copy(tok);
                  long long value = 0;
                  if (!parse_contract_int(key, tok, &value))
                    return false;
                  out_values->push_back(value);
                }
                if (out_values->empty()) {
                  log_warn("[dconfig] empty contract %.*s while validating "
                           "ExpectedValue docking\n",
                           static_cast<int>(key.size()), key.data());
                  ok = false;
                  return false;
                }
                return true;
              };

              long long configured_embedding_dims = 0;
              if (parse_contract_int("__embedding_dims",
                                     var_value_or_empty("__embedding_dims"),
                                     &configured_embedding_dims) &&
                  configured_embedding_dims != semantic.encoding_dims) {
                log_warn("[dconfig] contract __embedding_dims=%lld does not "
                         "match ExpectedValue network_design ENCODING.De=%d\n",
                         configured_embedding_dims, semantic.encoding_dims);
                ok = false;
              }

              long long configured_feature_width = 0;
              if (parse_contract_int("__obs_feature_dim",
                                     var_value_or_empty("__obs_feature_dim"),
                                     &configured_feature_width)) {
                if (configured_feature_width <= 0) {
                  log_warn("[dconfig] contract __obs_feature_dim=%lld must "
                           "be positive for ExpectedValue target feature "
                           "index validation\n",
                           configured_feature_width);
                  ok = false;
                } else {
                  for (const int target_index :
                       semantic.target_feature_indices) {
                    if (static_cast<long long>(target_index) >=
                        configured_feature_width) {
                      log_warn("[dconfig] ExpectedValue network_design "
                               "target_feature_index=%d is out of range for "
                               "contract "
                               "__obs_feature_dim=%lld\n",
                               target_index, configured_feature_width);
                      ok = false;
                    }
                  }
                }
              }

              std::vector<long long> configured_target_feature_indices{};
              if (parse_contract_int_list(
                      "__future_target_feature_indices",
                      var_value_or_empty("__future_target_feature_indices"),
                      &configured_target_feature_indices)) {
                if (configured_target_feature_indices.size() !=
                    semantic.target_feature_indices.size()) {
                  log_warn("[dconfig] contract "
                           "__future_target_feature_indices size=%zu "
                           "does not match ExpectedValue network_design "
                           "FUTURE_TARGET.target_feature_indices size=%zu\n",
                           configured_target_feature_indices.size(),
                           semantic.target_feature_indices.size());
                  ok = false;
                } else {
                  for (std::size_t i = 0;
                       i < configured_target_feature_indices.size(); ++i) {
                    if (configured_target_feature_indices[i] !=
                        semantic.target_feature_indices[i]) {
                      log_warn("[dconfig] contract "
                               "__future_target_feature_indices[%zu]=%lld "
                               "does not match ExpectedValue network_design "
                               "target_feature_indices[%zu]=%d\n",
                               i, configured_target_feature_indices[i], i,
                               semantic.target_feature_indices[i]);
                      ok = false;
                    }
                  }
                }
              }
            }
          } catch (const std::exception &e) {
            log_warn("Invalid network_design DSL referenced by module "
                     "config [EXPECTED_VALUE] file %s: %s\n",
                     expected_value_config_path.c_str(), e.what());
            ok = false;
          }
        }
      }
    }
  }
}

if (!ok) {
  log_terminate_gracefully(
      "Invalid runtime-binding contract configuration, aborting.\n");
}
return ok;
}
