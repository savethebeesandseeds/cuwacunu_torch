template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_binding_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    const cuwacunu::iitepi::wave_hash_t &wave_hash,
    const std::shared_ptr<const cuwacunu::iitepi::wave_record_t> &wave_record,
    std::string wave_id, RuntimeBinding *out, std::string *error = nullptr) {
  if (!out)
    return false;
  if (!contract_record) {
    if (error)
      *error = "missing contract record";
    return false;
  }
  if (!wave_record) {
    if (error)
      *error = "missing wave record";
    return false;
  }
  (void)wave_hash;

  const auto contract_report =
      validate_contract_definition(inst, contract_hash);
  if (!contract_report.ok) {
    if (error) {
      *error = contract_report.indicators.empty()
                   ? "contract validation failed"
                   : contract_report.indicators.front().message;
    }
    return false;
  }

  std::string observation_sources_dsl;
  std::string observation_channels_dsl;
  std::string jkimyei_specs_dsl;
  std::string wave_dsl;
  if (!load_required_dsl_text(kRuntimeBindingContractWaveDslKey,
                              wave_record->wave.dsl, &wave_dsl, error)) {
    return false;
  }

  cuwacunu::camahjucunu::observation_spec_t observation_instruction{};
  cuwacunu::camahjucunu::jkimyei_specs_t jkimyei_specs{};
  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = wave_record->wave.decoded();
  } catch (const std::exception &e) {
    if (error)
      *error = std::string("failed to decode wave DSL payloads: ") + e.what();
    return false;
  }

  const auto *selected_wave =
      select_wave_by_id(wave_set, std::move(wave_id), error);
  if (!selected_wave)
    return false;
  const auto wave_report =
      validate_wave_definition(*selected_wave, contract_hash);
  if (!wave_report.ok) {
    if (error) {
      *error = wave_report.indicators.empty()
                   ? "wave validation failed"
                   : wave_report.indicators.front().message;
    }
    return false;
  }

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t effective_inst{};
  if (!select_contract_circuit(inst, selected_wave->circuit_name,
                               &effective_inst, error)) {
    return false;
  }

  out->contract_hash = contract_hash;
  out->wave_hash = wave_hash;
  out->contracts.clear();
  out->contracts.reserve(effective_inst.circuits.size());

  if (!load_contract_jkimyei_payloads(contract_record, &jkimyei_specs_dsl,
                                      &jkimyei_specs, error)) {
    return false;
  }
  if (!load_wave_dataloader_observation_payloads(
          contract_record, wave_record, *selected_wave,
          &observation_sources_dsl, &observation_channels_dsl,
          &observation_instruction, error)) {
    return false;
  }
  if (!validate_selected_wave_instrument_signatures(
          contract_record, effective_inst, *selected_wave,
          observation_instruction, error)) {
    return false;
  }
  if (selected_wave->sources.size() != 1U) {
    if (error) {
      *error = "runtime component identity derivation requires exactly one "
               "SOURCE block in selected wave";
    }
    return false;
  }
  const auto &runtime_instrument_signature =
      selected_wave->sources.front().runtime_instrument_signature;
  const auto compat_report = validate_wave_contract_compatibility(
      inst, *selected_wave, contract_hash, &jkimyei_specs, contract_hash,
      selected_wave->name);
  if (!compat_report.ok) {
    if (error) {
      *error = compat_report.indicators.empty()
                   ? "wave/contract compatibility validation failed"
                   : compat_report.indicators.front().message;
    }
    return false;
  }

  for (std::size_t i = 0; i < effective_inst.circuits.size(); ++i) {
    RuntimeBindingContract c{};
    std::string local_error;
    if (!build_runtime_circuit_from_decl<Datatype_t, Sampler_t>(
            effective_inst.circuits[i], contract_hash, observation_instruction,
            jkimyei_specs, jkimyei_specs_dsl, selected_wave, device, &c,
            &local_error)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      }
      return false;
    }
    c.spec.component_dsl_fingerprints.clear();
    c.spec.component_dsl_fingerprints.reserve(
        contract_record->signature.bindings.size());
    for (const auto &binding : contract_record->signature.bindings) {
      const auto maybe_type = parse_tsi_type_id(binding.tsi_type);
      const bool uses_hashimyei =
          maybe_type.has_value() && tsi_type_instance_policy(*maybe_type) ==
                                        TsiInstancePolicy::HashimyeiInstances;
      const cuwacunu::iitepi::contract_component_binding_t *effective_binding =
          &binding;
      cuwacunu::iitepi::contract_component_binding_t realized_binding{};
      if (uses_hashimyei) {
        const auto maybe_realized =
            cuwacunu::iitepi::realize_contract_component_binding_for_runtime(
                *contract_record, binding.binding_id,
                runtime_instrument_signature, &local_error);
        if (!maybe_realized.has_value()) {
          if (error) {
            *error = "contract[" + std::to_string(i) +
                     "] cannot realize component identity for <" +
                     binding.binding_id + ">: " + local_error;
          }
          return false;
        }
        realized_binding = *maybe_realized;
        effective_binding = &realized_binding;
      }
      RuntimeBindingContract::ComponentDslFingerprint fp{};
      fp.binding_id = effective_binding->binding_id;
      fp.canonical_path = effective_binding->canonical_path;
      fp.tsi_type = effective_binding->tsi_type;
      fp.hashimyei = effective_binding->hashimyei;
      fp.component_tag = effective_binding->component_tag;
      fp.component_compatibility_sha256_hex =
          effective_binding->component_compatibility_sha256_hex;
      if (uses_hashimyei) {
        fp.instrument_signature = effective_binding->instrument_signature;
      } else if (const auto *component_compatibility =
                     cuwacunu::iitepi::find_component_compatibility_signature(
                         *contract_record, effective_binding->binding_id)) {
        fp.instrument_signature = component_compatibility->instrument_signature;
      }
      fp.tsi_dsl_path = effective_binding->tsi_dsl_path;
      fp.tsi_dsl_sha256_hex = effective_binding->tsi_dsl_sha256_hex;
      c.spec.component_dsl_fingerprints.push_back(std::move(fp));
    }
    std::string circuit_dsl;
    if (!render_contract_circuit_dsl(contract_hash, effective_inst.circuits[i],
                                     &circuit_dsl, &local_error)) {
      if (error)
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      return false;
    }
    if (is_blank_ascii(circuit_dsl)) {
      if (error) {
        *error = "contract[" + std::to_string(i) +
                 "] missing required DSL text for key: " +
                 std::string(kRuntimeBindingContractCircuitDslKey);
      }
      return false;
    }

    c.set_dsl_segment(kRuntimeBindingContractCircuitDslKey,
                      std::move(circuit_dsl));
    c.set_dsl_segment(kRuntimeBindingContractWaveDslKey, wave_dsl);

    std::string_view missing_key{};
    if (!c.has_required_dsl_segments(&missing_key)) {
      if (error) {
        *error =
            "contract[" + std::to_string(i) +
            "] missing required DSL text for key: " + std::string(missing_key);
      }
      return false;
    }

    c.seed_wave.cursor.id = static_cast<WaveId>(i);
    out->contracts.push_back(std::move(c));
  }
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_binding_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const cuwacunu::iitepi::wave_hash_t &wave_hash, std::string wave_id,
    RuntimeBinding *out, std::string *error = nullptr) {
  if (!out)
    return false;
  try {
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(wave_hash);
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
    return build_runtime_binding_from_instruction<Datatype_t, Sampler_t>(
        inst, device, contract_hash, contract_itself, wave_hash, wave_itself,
        std::move(wave_id), out, error);
  } catch (const std::exception &e) {
    if (error)
      *error = std::string("failed to load required DSL text from config: ") +
               e.what();
    return false;
  }
}

} // namespace runtime_binding_builder
} // namespace tsiemene
