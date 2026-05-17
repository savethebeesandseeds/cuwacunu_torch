[[nodiscard]] inline bool load_contract_dsl_variables(
    const std::filesystem::path &contract_path,
    std::vector<cuwacunu::camahjucunu::dsl_variable_t> *out_variables,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_variables) {
    if (error)
      *error = "missing contract variable output";
    return false;
  }
  out_variables->clear();

  std::string raw_text{};
  if (!load_text_file(contract_path, &raw_text, error))
    return false;

  bool begin_seen = false;
  bool end_seen = false;
  bool dock_seen = false;
  bool assembly_seen = false;
  bool circuit_seen = false;
  enum class contract_section_e : std::uint8_t {
    None = 0,
    Dock,
    Assembly,
  };
  contract_section_e section = contract_section_e::None;
  std::istringstream input(raw_text);
  std::string raw_line{};
  bool in_block_comment = false;
  bool in_instrument_signature = false;
  while (std::getline(input, raw_line)) {
    std::string line =
        strip_contract_line_comments(raw_line, &in_block_comment);
    if (line.empty())
      continue;
    if (in_instrument_signature) {
      if (line == "};") {
        in_instrument_signature = false;
        continue;
      }
      if (line == "}") {
        if (error) {
          *error =
              "INSTRUMENT_SIGNATURE block must close with '};' while loading "
              "contract variables";
        }
        return false;
      }
      continue;
    }
    if (!begin_seen) {
      if (line == "-----BEGIN IITEPI CONTRACT-----") {
        begin_seen = true;
        continue;
      }
      if (error) {
        *error = "expected -----BEGIN IITEPI CONTRACT----- while loading "
                 "contract variables";
      }
      return false;
    }
    if (line == "-----END IITEPI CONTRACT-----") {
      if (section != contract_section_e::None) {
        if (error) {
          *error = "missing closing '}' before -----END IITEPI CONTRACT-----";
        }
        return false;
      }
      end_seen = true;
      break;
    }
    if (section == contract_section_e::None) {
      if (line == "DOCK {") {
        if (dock_seen) {
          if (error)
            *error = "duplicate DOCK section while loading contract variables";
          return false;
        }
        if (assembly_seen) {
          if (error) {
            *error =
                "DOCK section must appear before ASSEMBLY in contract wrapper";
          }
          return false;
        }
        dock_seen = true;
        section = contract_section_e::Dock;
        continue;
      }
      if (line == "ASSEMBLY {") {
        if (!dock_seen) {
          if (error) {
            *error = "ASSEMBLY section requires a preceding DOCK in contract "
                     "wrapper";
          }
          return false;
        }
        if (assembly_seen) {
          if (error) {
            *error =
                "duplicate ASSEMBLY section while loading contract variables";
          }
          return false;
        }
        assembly_seen = true;
        section = contract_section_e::Assembly;
        continue;
      }
      if (error) {
        *error = "expected DOCK { or ASSEMBLY { while loading contract "
                 "variables";
      }
      return false;
    }
    if (line == "}" || line == "};") {
      section = contract_section_e::None;
      continue;
    }

    if (section == contract_section_e::Dock &&
        line.rfind("INSTRUMENT_SIGNATURE", 0) == 0) {
      if (line.find('<') == std::string::npos ||
          line.find('>') == std::string::npos ||
          line.find('{') == std::string::npos) {
        if (error) {
          *error = "invalid INSTRUMENT_SIGNATURE header while loading "
                   "contract variables";
        }
        return false;
      }
      in_instrument_signature = true;
      continue;
    }

    if (line.back() != ';') {
      if (error) {
        *error = "expected ';' at statement end while loading contract "
                 "variables";
      }
      return false;
    }
    line.pop_back();
    line = trim_ascii(line);
    if (line.empty()) {
      if (error)
        *error = "empty contract statement while loading contract variables";
      return false;
    }

    const std::size_t eq = line.find('=');
    if (eq != std::string::npos) {
      const std::string variable_name = trim_ascii(line.substr(0, eq));
      if (cuwacunu::camahjucunu::is_dsl_variable_name(variable_name)) {
        const std::string variable_value =
            unquote_if_wrapped(trim_ascii(line.substr(eq + 1)));
        if (!cuwacunu::camahjucunu::append_dsl_variable(
                out_variables, variable_name, variable_value, error)) {
          return false;
        }
        continue;
      }
      if (section == contract_section_e::Dock) {
        if (error) {
          *error = "DOCK only accepts __variable assignments while loading "
                   "contract variables";
        }
        return false;
      }
    } else if (section == contract_section_e::Dock) {
      if (error) {
        *error = "DOCK only accepts __variable assignments while loading "
                 "contract variables";
      }
      return false;
    }

    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
      if (error) {
        *error = "unknown ASSEMBLY statement while loading contract variables";
      }
      return false;
    }
    const std::string head = trim_ascii(line.substr(0, colon));
    const std::string value = trim_ascii(line.substr(colon + 1));
    if (head == "CIRCUIT_FILE") {
      if (circuit_seen) {
        if (error) {
          *error = "duplicate CIRCUIT_FILE statement while loading contract "
                   "variables";
        }
        return false;
      }
      if (value.empty()) {
        if (error) {
          *error = "empty CIRCUIT_FILE path while loading contract variables";
        }
        return false;
      }
      circuit_seen = true;
      continue;
    }
    if (head == "AKNOWLEDGE") {
      if (value.find('=') == std::string::npos) {
        if (error) {
          *error = "AKNOWLEDGE requires alias = family while loading contract "
                   "variables";
        }
        return false;
      }
      continue;
    }
    if (error) {
      *error = "unknown ASSEMBLY statement while loading contract variables";
    }
    return false;
  }
  if (!begin_seen) {
    if (error) {
      *error = "missing -----BEGIN IITEPI CONTRACT----- while loading contract "
               "variables";
    }
    return false;
  }
  if (!end_seen) {
    if (error) {
      *error = "missing -----END IITEPI CONTRACT----- while loading contract "
               "variables";
    }
    return false;
  }
  if (in_instrument_signature) {
    if (error) {
      *error = "missing closing '};' for INSTRUMENT_SIGNATURE while loading "
               "contract variables";
    }
    return false;
  }
  if (!dock_seen) {
    if (error)
      *error = "missing DOCK section while loading contract variables";
    return false;
  }
  if (!assembly_seen) {
    if (error)
      *error = "missing ASSEMBLY section while loading contract variables";
    return false;
  }
  if (!circuit_seen) {
    if (error) {
      *error =
          "missing CIRCUIT_FILE statement in ASSEMBLY while loading contract "
          "variables";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool
validate_bind_variables_do_not_shadow_contract_variables(
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t>
        &contract_variables,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &bind_variables,
    std::string *error) {
  if (error)
    error->clear();
  if (contract_variables.empty() || bind_variables.empty())
    return true;

  std::unordered_set<std::string> contract_names{};
  contract_names.reserve(contract_variables.size());
  for (const auto &variable : contract_variables) {
    const std::string name = trim_ascii(variable.name);
    if (!name.empty())
      contract_names.insert(name);
  }

  for (const auto &variable : bind_variables) {
    const std::string name = trim_ascii(variable.name);
    if (name.empty())
      continue;
    if (contract_names.find(name) == contract_names.end())
      continue;
    if (error) {
      *error =
          "bind variable '" + name +
          "' conflicts with a contract DOCK/ASSEMBLY variable; bind-local "
          "variables are wave-scoped only and may not shadow contract-owned "
          "compatibility or realization variables";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path &source_path,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &variables,
    const std::filesystem::path &target_path,
    const std::filesystem::path &output_dir, staged_dsl_graph_state_t *state,
    std::string *error);

[[nodiscard]] inline std::string sanitize_identifier(std::string value) {
  for (char &ch : value) {
    const bool alpha_num = (ch >= 'a' && ch <= 'z') ||
                           (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
    if (!alpha_num)
      ch = '_';
  }
  std::string out;
  out.reserve(value.size());
  bool last_underscore = false;
  for (const char ch : value) {
    if (ch == '_') {
      if (last_underscore)
        continue;
      last_underscore = true;
      out.push_back(ch);
      continue;
    }
    last_underscore = false;
    out.push_back(ch);
  }
  while (!out.empty() && out.front() == '_')
    out.erase(out.begin());
  while (!out.empty() && out.back() == '_')
    out.pop_back();
  if (out.empty())
    out = "unnamed";
  if (!out.empty() && out.front() >= '0' && out.front() <= '9') {
    out.insert(out.begin(), '_');
  }
  return out;
}

[[nodiscard]] inline std::string derive_contract_id_from_snapshot_filename(
    const std::filesystem::path &filename) {
  std::string file_name = filename.filename().string();
  const auto ends_with = [](std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  if (ends_with(file_name, ".contract.dsl")) {
    file_name.resize(file_name.size() - std::string(".contract.dsl").size());
  } else if (ends_with(file_name, ".dsl")) {
    file_name.resize(file_name.size() - std::string(".dsl").size());
  } else {
    file_name = filename.stem().string();
  }
  return "contract_" + sanitize_identifier(file_name);
}

[[nodiscard]] inline std::string build_snapshot_campaign_text(
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    std::string_view snapshot_contract_ref,
    std::string_view snapshot_contract_filename,
    std::string_view snapshot_wave_filename) {
  std::ostringstream out;
  out << "CAMPAIGN {\n";
  out << "  IMPORT_CONTRACT \"" << snapshot_contract_filename << "\" AS "
      << snapshot_contract_ref << ";\n\n";
  out << "  FROM \"" << snapshot_wave_filename << "\" IMPORT_WAVE "
      << bind.wave_ref << ";\n\n";
  out << "  BIND " << bind.id << " {\n";
  for (const auto &variable : bind.variables) {
    out << "    " << variable.name << " = " << variable.value << ";\n";
  }
  out << "    CONTRACT = " << snapshot_contract_ref << ";\n";
  out << "    WAVE = " << bind.wave_ref << ";\n";
  out << "  };\n";
  out << "\n";
  out << "  RUN " << bind.id << ";\n";
  out << "}\n";
  return out.str();
}
