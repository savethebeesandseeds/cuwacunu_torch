[[nodiscard]] static parsed_config_t
parse_config_file(const std::string &path) {
  std::ifstream file = cuwacunu::piaabo::dfiles::readFileToStream(path);
  if (!file) {
    log_fatal("[dconfig] cannot open iitepi contract file: %s\n", path.c_str());
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  const std::string raw_text = oss.str();
  std::string stripped{};
  {
    std::istringstream in(raw_text);
    std::string line{};
    bool in_block_comment = false;
    while (std::getline(in, line)) {
      bool in_single = false;
      bool in_double = false;
      std::string out_line{};
      out_line.reserve(line.size());
      for (std::size_t i = 0; i < line.size();) {
        const char c = line[i];
        const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
        if (in_block_comment) {
          if (c == '*' && next == '/') {
            in_block_comment = false;
            i += 2;
          } else {
            ++i;
          }
          continue;
        }
        if (!in_single && !in_double && c == '/' && next == '*') {
          in_block_comment = true;
          i += 2;
          continue;
        }
        if (!in_single && !in_double && c == '/' && next == '/') {
          break;
        }
        if (!in_single && !in_double && c == '#') {
          break;
        }
        if (c == '\'' && !in_double) {
          in_single = !in_single;
        } else if (c == '"' && !in_single) {
          in_double = !in_double;
        }
        out_line.push_back(c);
        ++i;
      }
      stripped.append(out_line);
      stripped.push_back('\n');
    }
  }

  enum class contract_body_section_e : std::uint8_t {
    None = 0,
    Dock,
    Assembly,
  };

  parsed_config_t parsed;
  parsed[kContractSectionDock];
  parsed[kContractSectionAssembly];
  parsed[kContractSectionAknowledge];

  auto fail_parse = [&](std::size_t line_no, const std::string &message) {
    log_fatal("[dconfig] invalid iitepi contract file %s:%zu: %s\n",
              path.c_str(), line_no, message.c_str());
  };

  {
    bool begin_seen = false;
    bool end_seen = false;
    bool dock_seen = false;
    bool assembly_seen = false;
    contract_body_section_e section = contract_body_section_e::None;
    std::size_t line_no = 0;
    std::istringstream input(stripped);
    std::string raw_line{};
    while (std::getline(input, raw_line)) {
      ++line_no;
      std::string line = trim_ascii_ws_copy(raw_line);
      if (!has_non_ws_ascii(line))
        continue;

      if (!begin_seen) {
        if (line == "-----BEGIN IITEPI CONTRACT-----") {
          begin_seen = true;
          continue;
        }
        fail_parse(line_no, "expected -----BEGIN IITEPI CONTRACT-----");
      }

      if (line == "-----END IITEPI CONTRACT-----") {
        if (section != contract_body_section_e::None) {
          fail_parse(
              line_no,
              "missing closing '}' before -----END IITEPI CONTRACT-----");
        }
        end_seen = true;
        break;
      }

      if (section == contract_body_section_e::None) {
        if (line == "DOCK {") {
          if (dock_seen)
            fail_parse(line_no, "duplicate DOCK section");
          if (assembly_seen) {
            fail_parse(line_no, "DOCK section must appear before ASSEMBLY");
          }
          dock_seen = true;
          section = contract_body_section_e::Dock;
          continue;
        }
        if (line == "ASSEMBLY {") {
          if (!dock_seen) {
            fail_parse(line_no, "ASSEMBLY section requires a preceding DOCK");
          }
          if (assembly_seen)
            fail_parse(line_no, "duplicate ASSEMBLY section");
          assembly_seen = true;
          section = contract_body_section_e::Assembly;
          continue;
        }
        fail_parse(line_no, "expected DOCK { or ASSEMBLY {");
      }

      if (line == "}" || line == "};") {
        section = contract_body_section_e::None;
        continue;
      }

      if (line.back() != ';') {
        fail_parse(line_no, "expected ';' at statement end");
      }
      line.pop_back();
      line = trim_ascii_ws_copy(std::move(line));
      if (!has_non_ws_ascii(line)) {
        fail_parse(line_no, "empty statement");
      }

      const std::size_t eq = line.find('=');
      if (eq == std::string::npos) {
        if (section == contract_body_section_e::Dock) {
          fail_parse(line_no, "DOCK only accepts __variable assignments");
        }
        continue;
      }
      const std::string variable_name = trim_ascii_ws_copy(line.substr(0, eq));
      if (!cuwacunu::camahjucunu::is_dsl_variable_name(variable_name)) {
        if (section == contract_body_section_e::Dock) {
          fail_parse(line_no, "DOCK only accepts __variable assignments");
        }
        continue;
      }
      const std::string variable_value =
          unquote_if_wrapped(trim_ascii_ws_copy(line.substr(eq + 1)));
      const char *target_section = section == contract_body_section_e::Dock
                                       ? kContractSectionDock
                                       : kContractSectionAssembly;
      if (parsed[target_section].find(variable_name) !=
              parsed[target_section].end() ||
          (target_section == kContractSectionDock &&
           parsed[kContractSectionAssembly].find(variable_name) !=
               parsed[kContractSectionAssembly].end()) ||
          (target_section == kContractSectionAssembly &&
           parsed[kContractSectionDock].find(variable_name) !=
               parsed[kContractSectionDock].end())) {
        fail_parse(line_no, "duplicate contract variable: " + variable_name);
      }
      parsed[target_section][variable_name] = variable_value;
    }

    if (!begin_seen) {
      fail_parse(1, "missing -----BEGIN IITEPI CONTRACT-----");
    }
    if (!end_seen) {
      fail_parse(line_no, "missing -----END IITEPI CONTRACT-----");
    }
    if (!dock_seen) {
      fail_parse(line_no, "missing DOCK section");
    }
    if (!assembly_seen) {
      fail_parse(line_no, "missing ASSEMBLY section");
    }
  }

  const auto contract_variables = contract_dsl_variables_from_config(parsed);
  bool begin_seen = false;
  bool end_seen = false;
  bool circuit_seen = false;
  contract_body_section_e section = contract_body_section_e::None;
  std::size_t line_no = 0;
  std::istringstream input(stripped);
  std::string raw_line{};
  while (std::getline(input, raw_line)) {
    ++line_no;
    std::string line = trim_ascii_ws_copy(raw_line);
    if (!has_non_ws_ascii(line))
      continue;

    if (!begin_seen) {
      if (line == "-----BEGIN IITEPI CONTRACT-----") {
        begin_seen = true;
        continue;
      }
      fail_parse(line_no, "expected -----BEGIN IITEPI CONTRACT-----");
    }

    if (line == "-----END IITEPI CONTRACT-----") {
      if (section != contract_body_section_e::None) {
        fail_parse(line_no,
                   "missing closing '}' before -----END IITEPI CONTRACT-----");
      }
      end_seen = true;
      break;
    }

    if (section == contract_body_section_e::None) {
      if (line == "DOCK {") {
        section = contract_body_section_e::Dock;
        continue;
      }
      if (line == "ASSEMBLY {") {
        section = contract_body_section_e::Assembly;
        continue;
      }
      fail_parse(line_no, "expected DOCK { or ASSEMBLY {");
    }

    if (line == "}" || line == "};") {
      section = contract_body_section_e::None;
      continue;
    }

    if (line.back() != ';') {
      fail_parse(line_no, "expected ';' at statement end");
    }
    line.pop_back();
    line = trim_ascii_ws_copy(std::move(line));
    if (!has_non_ws_ascii(line)) {
      fail_parse(line_no, "empty statement");
    }

    const std::size_t eq = line.find('=');
    if (eq != std::string::npos) {
      const std::string variable_name = trim_ascii_ws_copy(line.substr(0, eq));
      if (cuwacunu::camahjucunu::is_dsl_variable_name(variable_name)) {
        if (section == contract_body_section_e::Dock ||
            section == contract_body_section_e::Assembly) {
          continue;
        }
      }
    }

    if (section == contract_body_section_e::Dock) {
      fail_parse(line_no, "DOCK only accepts __variable assignments");
    }

    if (!contract_variables.empty()) {
      line = resolve_dsl_variables_or_fail(
          line, contract_variables, path + ":" + std::to_string(line_no));
    }

    const auto split_head_value = [&](const char *expected_head,
                                      std::string *out_value) {
      const std::size_t colon = line.find(':');
      if (colon == std::string::npos) {
        fail_parse(line_no, "expected ':' after statement head");
      }
      const std::string head = trim_ascii_ws_copy(line.substr(0, colon));
      if (head != expected_head)
        return false;
      *out_value = trim_ascii_ws_copy(line.substr(colon + 1));
      return true;
    };

    std::string statement_value{};
    if (split_head_value("CIRCUIT_FILE", &statement_value)) {
      if (circuit_seen) {
        fail_parse(line_no, "duplicate CIRCUIT_FILE entry");
      }
      statement_value = unquote_if_wrapped(std::move(statement_value));
      if (!has_non_ws_ascii(statement_value)) {
        fail_parse(line_no, "empty CIRCUIT_FILE path");
      }
      parsed[kContractSectionAssembly][kAssemblyKeyCircuitDslFilename] =
          statement_value;
      circuit_seen = true;
      continue;
    }
    if (split_head_value("AKNOWLEDGE", &statement_value)) {
      const std::size_t eq = statement_value.find('=');
      if (eq == std::string::npos) {
        fail_parse(line_no, "AKNOWLEDGE requires alias = family");
      }
      std::string alias = trim_ascii_ws_copy(statement_value.substr(0, eq));
      std::string family = unquote_if_wrapped(
          trim_ascii_ws_copy(statement_value.substr(eq + 1)));
      if (!has_non_ws_ascii(alias) || !has_non_ws_ascii(family)) {
        fail_parse(line_no, "AKNOWLEDGE alias/family must be non-empty");
      }
      if (parsed[kContractSectionAknowledge].find(alias) !=
          parsed[kContractSectionAknowledge].end()) {
        fail_parse(line_no, "duplicate AKNOWLEDGE alias: " + alias);
      }
      parsed[kContractSectionAknowledge][alias] = family;
      continue;
    }

    fail_parse(line_no, "unknown ASSEMBLY statement (expected CIRCUIT_FILE, "
                        "AKNOWLEDGE, or __variable assignment)");
  }

  if (!begin_seen) {
    fail_parse(1, "missing -----BEGIN IITEPI CONTRACT-----");
  }
  if (!end_seen) {
    fail_parse(line_no, "missing -----END IITEPI CONTRACT-----");
  }
  if (!circuit_seen) {
    fail_parse(line_no, "missing CIRCUIT_FILE statement in ASSEMBLY");
  }

  return parsed;
}

template <class T> static T parse_scalar_from_string(const std::string &s) {
  if constexpr (std::is_same_v<T, std::string>) {
    return s;
  } else if constexpr (std::is_same_v<T, bool>) {
    const std::string trimmed = trim_ascii_ws_copy(s);
    std::string v;
    v.reserve(trimmed.size());
    std::transform(
        trimmed.begin(), trimmed.end(), std::back_inserter(v),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "1" || v == "true" || v == "yes" || v == "y" || v == "on") {
      return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "n" || v == "off") {
      return false;
    }
    throw bad_config_access("Invalid bool '" + s + "'");
  } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
    T v{};
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
      throw bad_config_access("Invalid integer '" + s + "'");
    }
    return v;
  } else if constexpr (std::is_floating_point_v<T>) {
    char *end{};
    errno = 0;
    const long double ld = std::strtold(s.c_str(), &end);
    if (errno || end != s.data() + s.size()) {
      throw bad_config_access("Invalid float '" + s + "'");
    }
    return static_cast<T>(ld);
  } else {
    static_assert(!std::is_same_v<T, T>,
                  "from_string<T>() not specialized for this T");
  }
}

[[nodiscard]] static bool
section_supports_instruction_file(std::string_view section) noexcept {
  return section == kModuleSectionVicreg ||
         section == kModuleSectionExpectedValue ||
         section == kModuleSectionEmbeddingSequenceAnalytics ||
         section == kModuleSectionTransferMatrixEvaluation;
}

[[nodiscard]] static std::optional<std::string>
module_config_path_for_section(const contract_record_t &snapshot,
                               const std::string &section) {
  if (!section_supports_instruction_file(section))
    return std::nullopt;
  const auto path_it = snapshot.module_section_paths.find(section);
  if (path_it == snapshot.module_section_paths.end())
    return std::nullopt;
  if (!has_non_ws_ascii(path_it->second))
    return std::nullopt;
  return path_it->second;
}

[[nodiscard]] static parsed_config_section_t
parse_instruction_file(const std::string &path,
                       const std::string &latent_lineage_state_grammar_text,
                       const parsed_config_section_t *contract_variables,
                       std::string *out_resolved_text) {
  std::ifstream file = cuwacunu::piaabo::dfiles::readFileToStream(path);
  if (!file) {
    log_fatal("[dconfig] cannot open module config file: %s\n", path.c_str());
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  std::string instruction_text = oss.str();
  if (contract_variables != nullptr && !contract_variables->empty()) {
    instruction_text = resolve_dsl_variables_or_fail(
        instruction_text,
        contract_dsl_variables_from_section(*contract_variables), path);
  }
  if (out_resolved_text)
    *out_resolved_text = instruction_text;

  if (!has_non_ws_ascii(latent_lineage_state_grammar_text)) {
    log_fatal("[dconfig] empty latent_lineage_state grammar payload while "
              "decoding module file: %s\n",
              path.c_str());
  }

  try {
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            latent_lineage_state_grammar_text, instruction_text);
    return decoded.to_map();
  } catch (const std::exception &e) {
    log_fatal("[dconfig] invalid latent_lineage_state DSL in %s: %s\n",
              path.c_str(), e.what());
  }
  return {};
}

[[nodiscard]] static std::optional<std::string>
contract_instruction_section_value_or_empty(const contract_record_t &snapshot,
                                            const std::string &section,
                                            const std::string &key) {
  const auto sec_it = snapshot.module_sections.find(section);
  if (sec_it == snapshot.module_sections.end())
    return std::nullopt;
  const auto value_it = sec_it->second.find(key);
  if (value_it == sec_it->second.end())
    return std::nullopt;
  return value_it->second;
}

static void normalize_vicreg_module_architecture_from_network_design(
    contract_record_t *record) {
  if (record == nullptr || !record->vicreg_network_design.has_payload())
    return;
  auto sec_it = record->module_sections.find(kModuleSectionVicreg);
  if (sec_it == record->module_sections.end())
    return;

  try {
    const auto &ir = record->vicreg_network_design.decoded();
    cuwacunu::wikimyei::vicreg_4d::vicreg_network_design_spec_t semantic{};
    std::string semantic_error{};
    if (!cuwacunu::wikimyei::vicreg_4d::resolve_vicreg_network_design(
            ir, &semantic, &semantic_error)) {
      log_fatal("[dconfig] invalid VICReg network_design semantic payload "
                "while normalizing module snapshot: %s\n",
                semantic_error.c_str());
    }

    auto &vicreg_section = sec_it->second;
    vicreg_section["encoding_dims"] = std::to_string(semantic.encoding_dims);
    vicreg_section["channel_expansion_dim"] =
        std::to_string(semantic.channel_expansion_dim);
    vicreg_section["fused_feature_dim"] =
        std::to_string(semantic.fused_feature_dim);
    vicreg_section["encoder_hidden_dims"] =
        std::to_string(semantic.encoder_hidden_dims);
    vicreg_section["encoder_depth"] = std::to_string(semantic.encoder_depth);
    vicreg_section["projector_mlp_spec"] = semantic.projector_mlp_spec;
    vicreg_section["projector_norm"] = semantic.projector_norm;
    vicreg_section["projector_activation"] = semantic.projector_activation;
    vicreg_section["projector_hidden_bias"] =
        bool_token(semantic.projector_hidden_bias);
    vicreg_section["projector_last_bias"] =
        bool_token(semantic.projector_last_bias);
    vicreg_section["projector_bn_in_fp32"] =
        bool_token(semantic.projector_bn_in_fp32);
  } catch (const std::exception &e) {
    log_fatal("[dconfig] failed to normalize VICReg module architecture from "
              "network_design: %s\n",
              e.what());
  }
}

static void normalize_expected_value_module_architecture_from_network_design(
    contract_record_t *record) {
  if (record == nullptr || !record->expected_value_network_design.has_payload())
    return;
  auto sec_it = record->module_sections.find(kModuleSectionExpectedValue);
  if (sec_it == record->module_sections.end())
    return;

  try {
    const auto &ir = record->expected_value_network_design.decoded();
    cuwacunu::wikimyei::expected_value_network_design_spec_t semantic{};
    std::string semantic_error{};
    if (!cuwacunu::wikimyei::resolve_expected_value_network_design(
            ir, &semantic, &semantic_error)) {
      log_fatal("[dconfig] invalid ExpectedValue network_design semantic "
                "payload while normalizing module snapshot: %s\n",
                semantic_error.c_str());
    }

    auto &expected_value_section = sec_it->second;
    expected_value_section["encoding_dims"] =
        std::to_string(semantic.encoding_dims);
    expected_value_section["encoding_temporal_reducer"] =
        semantic.encoding_temporal_reducer;
    {
      std::ostringstream dims;
      for (std::size_t i = 0; i < semantic.target_dims.size(); ++i) {
        if (i != 0)
          dims << ",";
        dims << semantic.target_dims[i];
      }
      expected_value_section["target_dims"] = dims.str();
    }
    expected_value_section["mixture_comps"] =
        std::to_string(semantic.mixture_comps);
    expected_value_section["features_hidden"] =
        std::to_string(semantic.features_hidden);
    expected_value_section["residual_depth"] =
        std::to_string(semantic.residual_depth);
  } catch (const std::exception &e) {
    log_fatal("[dconfig] failed to normalize ExpectedValue module "
              "architecture from network_design: %s\n",
              e.what());
  }
}

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
      };

  validate_module_instruction_file(
      kModuleSectionVicreg, vicreg_config_path, vicreg_grammar_text,
      /* string_keys */
      {"dtype", "device", "jkimyei_dsl_file", "network_design_dsl_file"},
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
      {"model_path", "dtype", "device", "network_design_dsl_file",
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
       "nll_sigma_min", "nll_sigma_max", "anchor_train_ratio",
       "anchor_val_ratio", "anchor_test_ratio", "linear_ridge_lambda",
       "gaussian_var_min"},
      /* optional_bool_keys */
      {"check_temporal_order", "validate_vicreg_out", "report_shapes",
       "reset_hashimyei_on_start"},
      /* optional_arr_int_keys */
      {"mdn_target_dims", "target_dims"},
      /* optional_arr_float_keys */
      {});

  {
    const parsed_config_section_t vicreg_values = parse_instruction_file(
        vicreg_config_path, vicreg_grammar_text, contract_variable_section);
    const auto jkimyei_dsl_it = vicreg_values.find("jkimyei_dsl_file");
    if (jkimyei_dsl_it == vicreg_values.end()) {
      log_warn(
          "Missing key <jkimyei_dsl_file> in module config [VICReg] file: %s\n",
          vicreg_config_path.c_str());
      ok = false;
    } else {
      const std::string raw_jkimyei_path =
          unquote_if_wrapped(trim_ascii_ws_copy(jkimyei_dsl_it->second));
      if (!has_non_ws_ascii(raw_jkimyei_path)) {
        log_warn(
            "Empty key <jkimyei_dsl_file> in module config [VICReg] file: %s\n",
            vicreg_config_path.c_str());
        ok = false;
      } else {
        std::string vicreg_folder{};
        const std::filesystem::path vicreg_path_fs(vicreg_config_path);
        if (vicreg_path_fs.has_parent_path()) {
          vicreg_folder = vicreg_path_fs.parent_path().string();
        }
        const std::string resolved_jkimyei_path =
            resolve_path_from_folder(vicreg_folder, raw_jkimyei_path);
        if (!has_non_ws_ascii(resolved_jkimyei_path) ||
            !std::filesystem::exists(resolved_jkimyei_path) ||
            !std::filesystem::is_regular_file(resolved_jkimyei_path)) {
          log_warn("Invalid path for <jkimyei_dsl_file> in module config "
                   "[VICReg] file %s: %s\n",
                   vicreg_config_path.c_str(), resolved_jkimyei_path.c_str());
          ok = false;
        } else {
          std::string jkimyei_grammar_path;
          if (require_existing_global_path("BNF",
                                           "jkimyei_specs_grammar_filename",
                                           &jkimyei_grammar_path)) {
            const std::string jkimyei_grammar_text =
                piaabo::dfiles::readFileToString(jkimyei_grammar_path);
            const std::string jkimyei_dsl_text =
                piaabo::dfiles::readFileToString(resolved_jkimyei_path);
            if (!has_non_ws_ascii(jkimyei_grammar_text)) {
              log_warn("[dconfig] empty [BNF].jkimyei_specs_grammar_filename "
                       "payload\n");
              ok = false;
            } else if (!has_non_ws_ascii(jkimyei_dsl_text)) {
              log_warn("Empty jkimyei DSL payload in module config [VICReg] "
                       "file %s: %s\n",
                       vicreg_config_path.c_str(),
                       resolved_jkimyei_path.c_str());
              ok = false;
            } else {
              try {
                (void)cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
                    jkimyei_grammar_text, jkimyei_dsl_text,
                    resolved_jkimyei_path);
              } catch (const std::exception &e) {
                log_warn("Invalid jkimyei DSL referenced by module config "
                         "[VICReg] file %s: %s\n",
                         vicreg_config_path.c_str(), e.what());
                ok = false;
              }
            }
          }
        }
      }
    }

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
                  contract_dsl_variables_from_section(
                      *contract_variable_section),
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
                } else if (has_observation_sources &&
                           has_observation_channels) {
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
                          !has_non_ws_ascii(
                              observation_channels_grammar_text)) {
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
                          cuwacunu::wikimyei::vicreg_4d::
                              vicreg_network_design_spec_t semantic{};
                          std::string semantic_error{};
                          if (!cuwacunu::wikimyei::vicreg_4d::
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
                                [&](std::string_view key,
                                    const std::string &text,
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
                                  text.data(), text.data() + text.size(),
                                  value);
                              if (ec != std::errc{} ||
                                  ptr != text.data() + text.size() ||
                                  value <= 0) {
                                log_warn(
                                    "[dconfig] invalid contract %.*s while "
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
                              log_warn(
                                  "[dconfig] contract __embedding_dims=%lld "
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
                                  ptr !=
                                      configured_feature_dim_text.data() +
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
    const parsed_config_section_t expected_value_values =
        parse_instruction_file(expected_value_config_path,
                               expected_value_grammar_text,
                               contract_variable_section);
    const auto net_dsl_it =
        expected_value_values.find("network_design_dsl_file");
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
                     "[EXPECTED_VALUE] file %s: %s\n",
                     expected_value_config_path.c_str(),
                     resolved_network_design_path.c_str());
            ok = false;
          } else {
            try {
              const auto network_design =
                  cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
                      network_design_grammar_text, network_design_dsl_text);
              cuwacunu::wikimyei::expected_value_network_design_spec_t
                  semantic{};
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
                  std::stringstream ss(text);
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
                  log_warn(
                      "[dconfig] contract __embedding_dims=%lld does not "
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
                             "be positive for ExpectedValue target_dims "
                             "validation\n",
                             configured_feature_width);
                    ok = false;
                  } else {
                    for (const int target_dim : semantic.target_dims) {
                      if (static_cast<long long>(target_dim) >=
                          configured_feature_width) {
                        log_warn("[dconfig] ExpectedValue network_design "
                                 "target_dim=%d is out of range for contract "
                                 "__obs_feature_dim=%lld\n",
                                 target_dim, configured_feature_width);
                        ok = false;
                      }
                    }
                  }
                }

                std::vector<long long> configured_target_dims{};
                if (parse_contract_int_list(
                        "__future_target_dims",
                        var_value_or_empty("__future_target_dims"),
                        &configured_target_dims)) {
                  if (configured_target_dims.size() !=
                      semantic.target_dims.size()) {
                    log_warn("[dconfig] contract __future_target_dims size=%zu "
                             "does not match ExpectedValue network_design "
                             "FUTURE_TARGET.target_dims size=%zu\n",
                             configured_target_dims.size(),
                             semantic.target_dims.size());
                    ok = false;
                  } else {
                    for (std::size_t i = 0; i < configured_target_dims.size();
                         ++i) {
                      if (configured_target_dims[i] !=
                          semantic.target_dims[i]) {
                        log_warn(
                            "[dconfig] contract __future_target_dims[%zu]=%lld "
                            "does not match ExpectedValue network_design "
                            "target_dims[%zu]=%d\n",
                            i, configured_target_dims[i], i,
                            semantic.target_dims[i]);
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

