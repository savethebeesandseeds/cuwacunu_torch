[[nodiscard]] inline bool prepare_snapshot_from_resolved_paths(
    const std::filesystem::path &source_scope_path,
    const std::filesystem::path &source_contract_path,
    const std::filesystem::path &source_wave_path,
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    const std::filesystem::path &output_dir,
    resolved_wave_contract_binding_snapshot_t *out_snapshot,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_snapshot) {
    if (error)
      *error = "missing binding snapshot output pointer";
    return false;
  }
  *out_snapshot = {};

  const std::filesystem::path campaign_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotCampaignFilename);
  const std::filesystem::path contract_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotContractFilename);
  const std::filesystem::path wave_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotWaveFilename);
  const std::filesystem::path instructions_dir = campaign_path.parent_path();
  const std::string snapshot_contract_ref =
      derive_contract_id_from_snapshot_filename(contract_path.filename());
  const std::string campaign_text = build_snapshot_campaign_text(
      bind, snapshot_contract_ref, contract_path.filename().string(),
      wave_path.filename().string());

  staged_dsl_graph_state_t staged_graph{};
  std::vector<cuwacunu::camahjucunu::dsl_variable_t> contract_variables{};
  if (!load_contract_dsl_variables(source_contract_path, &contract_variables,
                                   error)) {
    return false;
  }
  if (!validate_bind_variables_do_not_shadow_contract_variables(
          contract_variables, bind.variables, error)) {
    return false;
  }
  if (!stage_dsl_graph_file(source_contract_path, contract_variables,
                            contract_path, instructions_dir, &staged_graph,
                            error)) {
    return false;
  }
  if (!stage_dsl_graph_file(source_wave_path, bind.variables, wave_path,
                            instructions_dir, &staged_graph, error)) {
    return false;
  }
  if (!apply_contract_components_to_wave_snapshot(contract_path, bind,
                                                  wave_path, error)) {
    return false;
  }
  if (!store_text_file(campaign_path, campaign_text, error))
    return false;

  out_snapshot->binding_id = bind.id;
  out_snapshot->source_scope_dsl_path = source_scope_path.string();
  out_snapshot->source_contract_dsl_path = source_contract_path.string();
  out_snapshot->source_wave_dsl_path = source_wave_path.string();
  out_snapshot->campaign_dsl_path = campaign_path.string();
  out_snapshot->contract_dsl_path = contract_path.string();
  out_snapshot->wave_dsl_path = wave_path.string();
  out_snapshot->original_contract_ref = bind.contract_ref;
  out_snapshot->snapshot_contract_ref = snapshot_contract_ref;
  out_snapshot->wave_ref = bind.wave_ref;
  out_snapshot->variables = bind.variables;
  return true;
}

} // namespace detail

[[nodiscard]] inline bool prepare_campaign_binding_snapshot(
    const std::filesystem::path &source_campaign_dsl_path,
    std::string_view requested_binding_id,
    const std::filesystem::path &output_dir,
    resolved_wave_contract_binding_snapshot_t *out_snapshot,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  std::string grammar_text{};
  if (!detail::load_text_file(detail::configured_campaign_grammar_path(),
                              &grammar_text, error)) {
    return false;
  }
  std::string instruction_text{};
  if (!detail::load_text_file(source_campaign_dsl_path, &instruction_text,
                              error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        grammar_text, instruction_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "cannot decode iitepi campaign DSL '" +
               source_campaign_dsl_path.string() + "': " + e.what();
    }
    return false;
  }

  const std::string binding_id = detail::trim_ascii(requested_binding_id);
  if (binding_id.empty()) {
    if (error) {
      *error = "campaign binding snapshot requires an explicit binding id";
    }
    return false;
  }
  const auto *bind =
      detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    if (error)
      *error = "unknown campaign binding id: " + binding_id;
    return false;
  }
  const auto *contract_decl = detail::find_contract_decl_by_id(
      instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    if (error) {
      *error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    }
    return false;
  }

  const std::filesystem::path base_folder =
      source_campaign_dsl_path.parent_path();
  const std::filesystem::path source_contract_path =
      detail::resolve_path_from_folder(base_folder, contract_decl->file);
  std::string source_wave_path{};
  if (!detail::find_matching_wave_import_path(
          instruction.waves, source_campaign_dsl_path, bind->wave_ref,
          &source_wave_path, error)) {
    return false;
  }

  return detail::prepare_snapshot_from_resolved_paths(
      source_campaign_dsl_path, source_contract_path, source_wave_path, *bind,
      output_dir, out_snapshot, error);
}

[[nodiscard]] inline bool explain_campaign_binding_selection(
    const std::filesystem::path &source_campaign_dsl_path,
    std::string_view requested_binding_id,
    resolved_binding_selection_explanation_t *out_explanation,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out_explanation) {
    if (error)
      *error = "missing binding selection explanation output pointer";
    return false;
  }
  *out_explanation = {};
  out_explanation->source_campaign_dsl_path =
      source_campaign_dsl_path.lexically_normal().string();

  const auto fail = [&](std::string summary, std::string details) -> bool {
    out_explanation->resolved = false;
    out_explanation->summary = std::move(summary);
    out_explanation->details = std::move(details);
    if (error) {
      *error = out_explanation->details.empty() ? out_explanation->summary
                                                : out_explanation->details;
    }
    return false;
  };

  const std::string binding_id = detail::trim_ascii(requested_binding_id);
  if (binding_id.empty()) {
    return fail("binding selection explanation requires an explicit binding id",
                "campaign binding selection explanation requires an explicit "
                "binding id");
  }
  out_explanation->binding_id = binding_id;

  std::string campaign_grammar_text{};
  if (!detail::load_text_file(detail::configured_campaign_grammar_path(),
                              &campaign_grammar_text, error)) {
    return fail("cannot load iitepi campaign grammar",
                error ? *error : "cannot load iitepi campaign grammar");
  }
  std::string campaign_text{};
  if (!detail::load_text_file(source_campaign_dsl_path, &campaign_text,
                              error)) {
    return fail("cannot load campaign DSL",
                error ? *error : "cannot load campaign DSL");
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        campaign_grammar_text, campaign_text);
  } catch (const std::exception &e) {
    return fail("cannot decode iitepi campaign DSL",
                "cannot decode iitepi campaign DSL '" +
                    source_campaign_dsl_path.string() + "': " + e.what());
  }

  const auto *bind =
      detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    return fail("unknown campaign binding id",
                "unknown campaign binding id: " + binding_id);
  }
  out_explanation->contract_ref = detail::trim_ascii(bind->contract_ref);
  out_explanation->wave_ref = detail::trim_ascii(bind->wave_ref);

  const auto *contract_decl = detail::find_contract_decl_by_id(
      instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    return fail("binding references unknown CONTRACT id",
                "binding references unknown CONTRACT id: " +
                    bind->contract_ref);
  }

  const std::filesystem::path base_folder =
      source_campaign_dsl_path.parent_path();
  const std::filesystem::path source_contract_path(
      detail::resolve_path_from_folder(base_folder, contract_decl->file));
  out_explanation->source_contract_dsl_path =
      source_contract_path.lexically_normal().string();

  std::string source_wave_path{};
  if (!detail::find_matching_wave_import_path(
          instruction.waves, source_campaign_dsl_path, bind->wave_ref,
          &source_wave_path, error)) {
    return fail("binding references unknown WAVE id",
                error ? *error : "binding references unknown WAVE id");
  }
  out_explanation->source_wave_dsl_path =
      std::filesystem::path(source_wave_path).lexically_normal().string();

  std::vector<cuwacunu::camahjucunu::dsl_variable_t> contract_variables{};
  if (!detail::load_contract_dsl_variables(source_contract_path,
                                           &contract_variables, error)) {
    return fail("cannot load contract variables",
                error ? *error : "cannot load contract variables");
  }
  if (!detail::validate_bind_variables_do_not_shadow_contract_variables(
          contract_variables, bind->variables, error)) {
    return fail("bind variables shadow contract variables",
                error ? *error : "bind variables shadow contract variables");
  }

  std::string wave_grammar_text{};
  if (!detail::load_text_file(detail::configured_wave_grammar_path(),
                              &wave_grammar_text, error)) {
    return fail("cannot load iitepi wave grammar",
                error ? *error : "cannot load iitepi wave grammar");
  }
  std::string wave_text{};
  if (!detail::load_text_file(source_wave_path, &wave_text, error)) {
    return fail("cannot load wave DSL",
                error ? *error : "cannot load wave DSL");
  }
  std::string resolved_wave_text{};
  if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
          wave_text, bind->variables, &resolved_wave_text, error)) {
    return fail("cannot resolve bind-local wave variables",
                error ? *error : "cannot resolve bind-local wave variables");
  }

  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
        wave_grammar_text, resolved_wave_text);
  } catch (const std::exception &e) {
    return fail("cannot decode selected wave DSL",
                "cannot decode selected wave DSL '" + source_wave_path +
                    "': " + e.what());
  }

  const auto wave_it = std::find_if(
      wave_set.waves.begin(), wave_set.waves.end(), [&](const auto &wave) {
        return detail::trim_ascii(wave.name) ==
               detail::trim_ascii(bind->wave_ref);
      });
  if (wave_it == wave_set.waves.end()) {
    return fail(
        "selected bind references unknown WAVE id in resolved wave DSL",
        "selected bind references unknown WAVE id in resolved wave DSL: " +
            bind->wave_ref);
  }

  const auto contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          source_contract_path.string());
  out_explanation->contract_hash = detail::trim_ascii(contract_hash);
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!contract_snapshot) {
    return fail("cannot load contract snapshot for selection explanation",
                "cannot load contract snapshot for selection explanation: " +
                    source_contract_path.string());
  }
  out_explanation->docking_signature_sha256_hex = detail::trim_ascii(
      contract_snapshot->signature.docking_signature_sha256_hex);
  if (out_explanation->docking_signature_sha256_hex.empty()) {
    return fail("contract snapshot is missing docking signature",
                "contract snapshot is missing docking signature");
  }
  const auto *runtime_signature =
      detail::selected_wave_runtime_instrument_signature(*wave_it, error);
  if (!runtime_signature) {
    return fail("invalid wave runtime instrument signature",
                error ? *error : "invalid wave runtime instrument signature");
  }

  out_explanation->components.reserve(wave_it->wikimyeis.size());

  const auto explain_component = [&](std::string_view component_kind,
                                     std::string_view wave_binding_id,
                                     std::string_view family) -> bool {
    resolved_component_selection_explanation_t explanation{};
    explanation.wave_binding_id = detail::trim_ascii(wave_binding_id);
    explanation.component_kind = std::string(component_kind);
    explanation.family = detail::trim_ascii(family);
    if (const auto *component_compatibility =
            cuwacunu::iitepi::find_component_compatibility_signature(
                *contract_snapshot, explanation.wave_binding_id)) {
      explanation.component_compatibility_sha256_hex =
          detail::trim_ascii(component_compatibility->sha256_hex);
    }
    if (explanation.component_compatibility_sha256_hex.empty()) {
      explanation.summary = "missing component compatibility signature";
      explanation.details =
          "contract snapshot has no component compatibility signature for "
          "wave binding id: " +
          explanation.wave_binding_id;
      out_explanation->components.push_back(explanation);
      return fail("missing component compatibility signature",
                  explanation.details);
    }

    const auto *component_binding =
        detail::find_contract_component_binding_for_wave_slot(
            *contract_snapshot, explanation.wave_binding_id, explanation.family,
            *runtime_signature, error);
    if (!component_binding) {
      explanation.summary = "missing derived component binding";
      if (error && !error->empty()) {
        explanation.details = *error;
      } else {
        explanation.details =
            "contract snapshot has no derived component binding for wave "
            "binding id: " +
            explanation.wave_binding_id;
      }
      out_explanation->components.push_back(explanation);
      return fail("missing derived component binding", explanation.details);
    }
    const auto realized_component =
        cuwacunu::iitepi::realize_contract_component_binding_for_runtime(
            *contract_snapshot, component_binding->binding_id,
            *runtime_signature, error);
    if (!realized_component.has_value()) {
      explanation.summary = "missing realized component binding";
      explanation.details = error ? *error : std::string{};
      out_explanation->components.push_back(explanation);
      return fail("missing realized component binding", explanation.details);
    }
    explanation.component_tag =
        detail::trim_ascii(realized_component->component_tag);
    explanation.component_compatibility_sha256_hex = detail::trim_ascii(
        realized_component->component_compatibility_sha256_hex);
    explanation.derived_hashimyei =
        detail::trim_ascii(realized_component->hashimyei);
    explanation.derived_canonical_path = detail::component_canonical_path(
        realized_component->tsi_type, realized_component->hashimyei);
    if (explanation.derived_hashimyei.empty()) {
      explanation.summary = "derived component binding has no hashimyei";
      explanation.details = "contract component binding for wave binding id '" +
                            explanation.wave_binding_id +
                            "' has no derived hashimyei";
      out_explanation->components.push_back(explanation);
      return fail("derived component binding has no hashimyei",
                  explanation.details);
    }

    explanation.resolved = true;
    explanation.summary = "contract-derived component selected";
    explanation.details =
        "WIKIMYEI <" + explanation.wave_binding_id + "> derives component " +
        explanation.derived_canonical_path + " from component compatibility " +
        explanation.component_compatibility_sha256_hex +
        " selected by runtime instrument " +
        cuwacunu::camahjucunu::instrument_signature_compact_string(
            *runtime_signature) +
        ".";
    out_explanation->components.push_back(explanation);
    return true;
  };

  for (const auto &wikimyei : wave_it->wikimyeis) {
    if (!explain_component("WIKIMYEI", wikimyei.binding_id, wikimyei.family)) {
      return false;
    }
  }

  out_explanation->resolved = true;
  out_explanation->summary =
      "BIND '" + out_explanation->binding_id + "' resolved " +
      std::to_string(out_explanation->components.size()) +
      " derived component(s)";
  out_explanation->details =
      "Selection used contract '" + out_explanation->contract_ref +
      "' (contract_hash=" + out_explanation->contract_hash +
      ", docking_signature_sha256_hex=" +
      out_explanation->docking_signature_sha256_hex + ") and wave '" +
      out_explanation->wave_ref +
      "'. Component ids are derived from contract component compatibility; the "
      "full docking signature remains provenance.";
  return true;
}

} // namespace wave_contract_binding_runtime
} // namespace hero
} // namespace cuwacunu
