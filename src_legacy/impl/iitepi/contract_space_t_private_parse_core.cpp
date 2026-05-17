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

  std::string filtered{};
  {
    contract_body_section_e section = contract_body_section_e::None;
    bool begin_seen = false;
    bool in_signature = false;
    std::string signature_alias{};
    std::unordered_set<std::string> signature_fields{};
    std::size_t line_no = 0;
    std::istringstream input(stripped);
    std::string raw_line{};
    while (std::getline(input, raw_line)) {
      ++line_no;
      std::string line = trim_ascii_ws_copy(raw_line);

      if (in_signature) {
        filtered.push_back('\n');
        if (!has_non_ws_ascii(line))
          continue;
        if (line == "};") {
          if (signature_fields.size() != 6U) {
            fail_parse(line_no,
                       "INSTRUMENT_SIGNATURE <" + signature_alias +
                           "> must declare SYMBOL, RECORD_TYPE, MARKET_TYPE, "
                           "VENUE, BASE_ASSET, and QUOTE_ASSET");
          }
          in_signature = false;
          signature_alias.clear();
          signature_fields.clear();
          continue;
        }
        if (line == "}") {
          fail_parse(line_no,
                     "INSTRUMENT_SIGNATURE block must close with '};'");
        }
        if (line.back() != ';') {
          fail_parse(line_no, "expected ';' at INSTRUMENT_SIGNATURE field end");
        }
        line.pop_back();
        const std::size_t colon = line.find(':');
        const std::size_t eq = line.find('=');
        std::size_t delim = std::string::npos;
        if (colon != std::string::npos && eq != std::string::npos) {
          delim = std::min(colon, eq);
        } else {
          delim = colon != std::string::npos ? colon : eq;
        }
        if (delim == std::string::npos) {
          fail_parse(line_no, "INSTRUMENT_SIGNATURE field requires ':' or '='");
        }
        const std::string field = trim_ascii_ws_copy(line.substr(0, delim));
        const std::string value =
            unquote_if_wrapped(trim_ascii_ws_copy(line.substr(delim + 1)));
        if (!has_non_ws_ascii(field) || !has_non_ws_ascii(value)) {
          fail_parse(line_no,
                     "INSTRUMENT_SIGNATURE field/value must be non-empty");
        }
        cuwacunu::camahjucunu::instrument_signature_t probe{};
        std::string field_error{};
        if (!cuwacunu::camahjucunu::instrument_signature_set_field(
                &probe, field, value, &field_error)) {
          fail_parse(line_no, field_error);
        }
        if (!signature_fields.insert(field).second) {
          fail_parse(line_no, "duplicate INSTRUMENT_SIGNATURE field: " + field);
        }
        parsed[std::string(kContractSectionInstrumentSignaturePrefix) +
               signature_alias][field] = value;
        continue;
      }

      if (!begin_seen) {
        if (line == "-----BEGIN IITEPI CONTRACT-----") {
          begin_seen = true;
        }
        filtered.append(raw_line);
        filtered.push_back('\n');
        continue;
      }

      if (section == contract_body_section_e::None) {
        if (line == "DOCK {") {
          section = contract_body_section_e::Dock;
        } else if (line == "ASSEMBLY {") {
          section = contract_body_section_e::Assembly;
        }
        filtered.append(raw_line);
        filtered.push_back('\n');
        continue;
      }

      if (line == "}" || line == "};") {
        section = contract_body_section_e::None;
        filtered.append(raw_line);
        filtered.push_back('\n');
        continue;
      }

      if (section == contract_body_section_e::Dock &&
          line.rfind("INSTRUMENT_SIGNATURE", 0) == 0) {
        const std::size_t open_angle = line.find('<');
        const std::size_t close_angle = line.find('>');
        const std::size_t open_brace = line.find('{');
        if (open_angle == std::string::npos ||
            close_angle == std::string::npos ||
            open_brace == std::string::npos || open_angle > close_angle ||
            close_angle > open_brace) {
          fail_parse(line_no,
                     "INSTRUMENT_SIGNATURE must use '<alias> {' header");
        }
        const std::string head = trim_ascii_ws_copy(line.substr(0, open_angle));
        if (head != "INSTRUMENT_SIGNATURE") {
          fail_parse(line_no, "invalid INSTRUMENT_SIGNATURE header");
        }
        const std::string tail =
            trim_ascii_ws_copy(line.substr(open_brace + 1));
        if (tail != "") {
          fail_parse(line_no,
                     "INSTRUMENT_SIGNATURE header cannot contain fields");
        }
        signature_alias = trim_ascii_ws_copy(
            line.substr(open_angle + 1, close_angle - open_angle - 1));
        if (!has_non_ws_ascii(signature_alias)) {
          fail_parse(line_no, "INSTRUMENT_SIGNATURE alias cannot be empty");
        }
        const std::string section_name =
            std::string(kContractSectionInstrumentSignaturePrefix) +
            signature_alias;
        if (parsed.find(section_name) != parsed.end()) {
          fail_parse(line_no, "duplicate INSTRUMENT_SIGNATURE alias: " +
                                  signature_alias);
        }
        parsed[section_name];
        in_signature = true;
        filtered.push_back('\n');
        continue;
      }

      filtered.append(raw_line);
      filtered.push_back('\n');
    }
    if (in_signature) {
      fail_parse(line_no, "missing closing '};' for INSTRUMENT_SIGNATURE <" +
                              signature_alias + ">");
    }
  }

  {
    bool begin_seen = false;
    bool end_seen = false;
    bool dock_seen = false;
    bool assembly_seen = false;
    contract_body_section_e section = contract_body_section_e::None;
    std::size_t line_no = 0;
    std::istringstream input(filtered);
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
  std::istringstream input(filtered);
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
    cuwacunu::wikimyei::vicreg_rank4::vicreg_network_design_spec_t semantic{};
    std::string semantic_error{};
    if (!cuwacunu::wikimyei::vicreg_rank4::resolve_vicreg_network_design(
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
      for (std::size_t i = 0; i < semantic.target_feature_indices.size(); ++i) {
        if (i != 0)
          dims << ",";
        dims << semantic.target_feature_indices[i];
      }
      expected_value_section["target_feature_indices"] = dims.str();
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
