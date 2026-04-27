template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_circuit_from_decl(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t &parsed,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const cuwacunu::camahjucunu::observation_spec_t &observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
    const cuwacunu::camahjucunu::iitepi_wave_t *wave, torch::Device device,
    RuntimeBindingContract *out, std::string *error = nullptr) {
  if (!out)
    return false;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_decl(parsed,
                                                      &semantic_error)) {
      if (error)
        *error = semantic_error;
      return false;
    }
  }

  std::string effective_invoke_name = parsed.invoke_name;
  if (is_blank_ascii(effective_invoke_name))
    effective_invoke_name = parsed.name;
  std::string effective_invoke_payload = trim_ascii_copy(parsed.invoke_payload);

  if (is_blank_ascii(effective_invoke_name)) {
    if (error)
      *error = "empty circuit invoke name";
    return false;
  }

  out->name = parsed.name;
  out->invoke_name = effective_invoke_name;
  out->invoke_payload.clear();
  out->invoke_source_command.clear();
  out->nodes.clear();
  out->hops.clear();
  out->invalidate_compiled_runtime();
  out->spec = RuntimeBindingContract::Spec{};
  out->execution = RuntimeBindingContract::Execution{};
  out->spec.contract_hash = contract_hash;
  out->spec.sample_type = contract_sample_type_name<Datatype_t>();
  out->spec.sourced_from_config = true;
  {
    const int trace_level_cfg = cuwacunu::iitepi::config_space_t::get<int>(
        "TSI_RUNTIME", "runtime_trace_level",
        std::optional<int>{static_cast<int>(RuntimeTraceLevel::Verbose)});
    if (trace_level_cfg <= 0) {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Off;
    } else if (trace_level_cfg == 1) {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Step;
    } else {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Verbose;
    }

    const int max_queue_cfg = cuwacunu::iitepi::config_space_t::get<int>(
        "TSI_RUNTIME", "runtime_max_queue_size", std::optional<int>{0});
    if (max_queue_cfg < 0) {
      if (error) {
        *error = "TSI_RUNTIME.runtime_max_queue_size must be >= 0";
      }
      return false;
    }
    out->execution.runtime.max_queue_size =
        static_cast<std::size_t>(max_queue_cfg);
  }
  bool wave_mode_train_enabled = false;
  if (wave) {
    std::uint64_t mode_flags = wave->mode_flags;
    if (mode_flags == 0) {
      std::string mode_error;
      if (!cuwacunu::camahjucunu::parse_iitepi_wave_mode_flags(
              wave->mode, &mode_flags, nullptr, &mode_error)) {
        if (error) {
          *error =
              "wave MODE parse failed for '" + wave->name + "': " + mode_error;
        }
        return false;
      }
    }
    out->execution.wave_mode_flags = mode_flags;
    out->execution.debug_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
        mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Debug);
    wave_mode_train_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
        mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train);
    out->execution.epochs = wave->epochs;
    out->execution.batch_size = wave->batch_size;
  }
  if (wave && wave->batch_size > static_cast<std::uint64_t>(
                                     std::numeric_limits<std::size_t>::max())) {
    if (error) {
      *error = "wave BATCH_SIZE exceeds runtime supported range";
    }
    return false;
  }
  const std::size_t source_batch_size_override =
      (out->execution.batch_size > 0)
          ? static_cast<std::size_t>(out->execution.batch_size)
          : 0;
  std::uint64_t dataloader_workers = 0;
  bool dataloader_force_rebuild_cache = true;
  std::uint64_t dataloader_range_warn_batches = 256;

  std::unordered_map<std::string, Tsi *> alias_to_tsi;
  std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved_hops;
  DataloaderT<Datatype_t, Sampler_t> *first_dataloader = nullptr;
  std::uint64_t next_id = 1;
  std::size_t circuit_source_component_count = 0;
  std::unordered_map<std::string, node_path_ref_t> circuit_alias_refs{};
  std::vector<node_path_ref_t> circuit_wikimyei_refs{};
  std::vector<node_path_ref_t> circuit_source_refs{};
  std::vector<node_path_ref_t> circuit_probe_refs{};
  std::vector<node_path_ref_t> circuit_sink_refs{};
  const cuwacunu::camahjucunu::iitepi_wave_source_decl_t *selected_wave_source =
      nullptr;
  struct pending_decl_t {
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t *decl{nullptr};
    TsiTypeId type_id{TsiTypeId::SourceDataloader};
    const TsiTypeDescriptor *type_desc{nullptr};
    std::string canonical_identity{};
    std::string decl_path{};
    std::string probe_hashimyei{};
    std::string resolved_wikimyei_hashimyei{};
    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl{nullptr};
    const cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *wave_probe_decl{
        nullptr};
  };
  std::vector<pending_decl_t> pending_decls;
  pending_decls.reserve(parsed.instances.size());

  for (const auto &decl : parsed.instances) {
    std::string type_path_error{};
    const auto type_path = resolve_runtime_node_path(
        decl.tsi_type, contract_hash, /*allow_family_without_hash=*/true,
        &type_path_error);
    if (!type_path.has_value()) {
      if (error) {
        *error = "invalid tsi_type canonical path for alias " + decl.alias +
                 ": " + type_path_error;
      }
      return false;
    }

    const auto type_id = std::optional<TsiTypeId>(type_path->type_id);
    const auto *type_desc = find_tsi_type(*type_id);
    if (!type_desc) {
      if (error) {
        *error = "missing tsi type descriptor in manifest for: " +
                 type_path->canonical_identity;
      }
      return false;
    }

    const std::string canonical_type(type_desc->canonical);
    if (std::find(out->spec.component_types.begin(),
                  out->spec.component_types.end(),
                  canonical_type) == out->spec.component_types.end()) {
      out->spec.component_types.push_back(canonical_type);
    }
    if (type_desc->domain == TsiDomain::Source &&
        out->spec.source_type.empty()) {
      out->spec.source_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei &&
        out->spec.representation_type.empty()) {
      out->spec.representation_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei &&
        out->spec.representation_hashimyei.empty() &&
        !type_path->hashimyei.empty()) {
      out->spec.representation_hashimyei = type_path->hashimyei;
    }
    const std::string decl_path = type_path->runtime_path;
    std::string resolved_wikimyei_hashimyei = type_path->hashimyei;
    const std::string decl_binding_id = trim_ascii_copy(decl.alias);
    if (decl_binding_id.empty()) {
      if (error) {
        *error = "empty circuit binding id is not allowed";
      }
      return false;
    }
    if (!circuit_alias_refs.emplace(decl_binding_id, *type_path).second) {
      if (error) {
        *error = "duplicated circuit binding id in runtime declaration: " +
                 decl_binding_id;
      }
      return false;
    }

    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl = nullptr;
    const cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *wave_probe_decl =
        nullptr;
    if (wave && type_desc->domain == TsiDomain::Wikimyei) {
      circuit_wikimyei_refs.push_back(*type_path);
      wave_wikimyei_decl = find_wave_wikimyei_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_wikimyei_decl) {
        if (error) {
          *error = "missing WIKIMYEI wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
      std::string wave_path_error{};
      const auto wave_wikimyei_path = resolve_runtime_node_path(
          wave_wikimyei_decl->wikimyei_path, contract_hash,
          /*allow_family_without_hash=*/false, &wave_path_error);
      if (!wave_wikimyei_path.has_value()) {
        if (error) {
          *error = "invalid WIKIMYEI PATH for binding id '" + decl.alias +
                   "': " + wave_path_error;
        }
        return false;
      }
      if (!wave_wikimyei_path->hashimyei.empty()) {
        resolved_wikimyei_hashimyei = wave_wikimyei_path->hashimyei;
        if (out->spec.representation_hashimyei.empty()) {
          out->spec.representation_hashimyei = wave_wikimyei_path->hashimyei;
        }
      }
    }
    if (wave && type_desc->domain == TsiDomain::Source) {
      ++circuit_source_component_count;
      circuit_source_refs.push_back(*type_path);
      const auto *wave_source_decl =
          find_wave_source_decl_for_binding_id_or_path(
              *wave, *type_path, decl.alias, contract_hash);
      if (!wave_source_decl) {
        if (error) {
          *error = "missing SOURCE wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
      if (!selected_wave_source)
        selected_wave_source = wave_source_decl;
      // Source dataloader construction requires instrument up-front.
      if (out->spec.instrument.empty()) {
        out->spec.instrument = trim_ascii_copy(
            wave_source_decl->runtime_instrument_signature.symbol);
      }
      if (out->spec.runtime_instrument_signature.symbol.empty()) {
        out->spec.runtime_instrument_signature =
            wave_source_decl->runtime_instrument_signature;
      }
      if (out->spec.source_runtime_cursor.empty()) {
        out->spec.source_runtime_cursor =
            cuwacunu::source::dataloader::make_source_runtime_cursor(
                wave_source_decl->runtime_instrument_signature,
                wave_source_decl->from, wave_source_decl->to);
      }
    }
    if (wave && type_desc->domain == TsiDomain::Probe) {
      circuit_probe_refs.push_back(*type_path);
      wave_probe_decl = find_wave_probe_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_probe_decl) {
        if (error) {
          *error = "missing PROBE wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
    }
    if (wave && type_desc->domain == TsiDomain::Sink) {
      circuit_sink_refs.push_back(*type_path);
      const auto *wave_sink_decl = find_wave_sink_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_sink_decl) {
        if (error) {
          *error = "missing SINK wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
    }

    std::string probe_hashimyei = type_path->hashimyei;
    if (type_desc->domain == TsiDomain::Probe) {
      if (probe_hashimyei.empty() && wave_probe_decl) {
        const auto probe_path = resolve_runtime_node_path(
            wave_probe_decl->probe_path, contract_hash,
            /*allow_family_without_hash=*/false, error);
        if (!probe_path.has_value())
          return false;
        probe_hashimyei = probe_path->hashimyei;
      }
      if (probe_hashimyei.empty()) {
        if (error) {
          *error = "probe tsi_type for alias '" + decl.alias +
                   "' requires hashimyei from wave PROBE.PATH";
        }
        return false;
      }
      if (!cuwacunu::hashimyei::is_hex_hash_name(probe_hashimyei)) {
        if (error) {
          *error = "probe hashimyei suffix is invalid for alias '" +
                   decl.alias + "': " + probe_hashimyei;
        }
        return false;
      }
    }

    pending_decls.push_back(pending_decl_t{
        .decl = &decl,
        .type_id = *type_id,
        .type_desc = type_desc,
        .canonical_identity = std::string(type_path->canonical_identity),
        .decl_path = decl_path,
        .probe_hashimyei = probe_hashimyei,
        .resolved_wikimyei_hashimyei = resolved_wikimyei_hashimyei,
        .wave_wikimyei_decl = wave_wikimyei_decl,
        .wave_probe_decl = wave_probe_decl,
    });
  }

  if (wave) {
    const auto has_matching_contract_ref =
        [&](const std::vector<node_path_ref_t> &contract_refs,
            const node_path_ref_t &wave_path) {
          return std::any_of(contract_refs.begin(), contract_refs.end(),
                             [&](const node_path_ref_t &contract_path) {
                               return wave_decl_matches_contract_path(
                                   contract_path, wave_path);
                             });
        };

    const auto validate_wave_binding_id_and_path =
        [&](const char *kind, const std::string &binding_id,
            const std::string &raw_wave_path,
            const std::vector<node_path_ref_t> &domain_contract_refs) -> bool {
      const std::string normalized_binding_id = trim_ascii_copy(binding_id);
      if (normalized_binding_id.empty()) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind +
                   " component is missing binding id (<...>)";
        }
        return false;
      }
      const auto contract_it = circuit_alias_refs.find(normalized_binding_id);
      if (contract_it == circuit_alias_refs.end()) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind + " binding id '" +
                   normalized_binding_id +
                   "' is not present in contract circuit acknowledgements";
        }
        return false;
      }
      const auto wave_path = resolve_runtime_node_path(
          raw_wave_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value()) {
        if (error) {
          *error = "wave '" + wave->name + "' has invalid " + kind +
                   " PATH for binding id '" + normalized_binding_id +
                   "': " + raw_wave_path;
        }
        return false;
      }
      if (!wave_decl_matches_contract_path(contract_it->second, *wave_path)) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind + " binding id '" +
                   normalized_binding_id + "' path mismatch: contract=" +
                   contract_it->second.runtime_path +
                   " wave=" + wave_path->runtime_path;
        }
        return false;
      }
      if (!has_matching_contract_ref(domain_contract_refs, *wave_path)) {
        if (error) {
          *error =
              "wave '" + wave->name + "' " + kind +
              " PATH not present in circuit domain: " + wave_path->runtime_path;
        }
        return false;
      }
      return true;
    };

    for (const auto &w : wave->wikimyeis) {
      if (!validate_wave_binding_id_and_path("WIKIMYEI", w.binding_id,
                                             w.wikimyei_path,
                                             circuit_wikimyei_refs)) {
        return false;
      }
    }
    for (const auto &s : wave->sources) {
      if (!validate_wave_binding_id_and_path(
              "SOURCE", s.binding_id, s.source_path, circuit_source_refs)) {
        return false;
      }
    }
    for (const auto &p : wave->probes) {
      if (!validate_wave_binding_id_and_path(
              "PROBE", p.binding_id, p.probe_path, circuit_probe_refs)) {
        return false;
      }
    }
    for (const auto &s : wave->sinks) {
      if (!validate_wave_binding_id_and_path("SINK", s.binding_id, s.sink_path,
                                             circuit_sink_refs)) {
        return false;
      }
    }

    if (circuit_source_component_count != 1) {
      if (error) {
        *error = "runtime requires exactly one SOURCE component in contract "
                 "circuit for wave binding";
      }
      return false;
    }
    if (wave->sources.size() != 1U) {
      if (error) {
        *error = "wave '" + wave->name +
                 "' must declare exactly one SOURCE block for contract binding";
      }
      return false;
    }
    if (!selected_wave_source) {
      if (error) {
        *error = "wave '" + wave->name +
                 "' missing SOURCE block for circuit source path";
      }
      return false;
    }
    dataloader_workers = selected_wave_source->workers;
    dataloader_force_rebuild_cache = selected_wave_source->force_rebuild_cache;
    dataloader_range_warn_batches = selected_wave_source->range_warn_batches;
    if (dataloader_range_warn_batches == 0) {
      if (error) {
        *error = "wave SOURCE.RANGE_WARN_BATCHES must be >= 1";
      }
      return false;
    }
    effective_invoke_payload =
        compose_invoke_payload_from_wave_source(*selected_wave_source, *wave);
  } else if (is_blank_ascii(effective_invoke_payload)) {
    if (error)
      *error = "empty circuit invoke payload";
    return false;
  }

  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain != TsiDomain::Wikimyei)
      continue;
    if (!validate_runtime_hashimyei_component_compatibility(
            pending.type_desc->canonical, pending.resolved_wikimyei_hashimyei,
            contract_hash, pending.decl ? pending.decl->alias : std::string{},
            /*require_registered_manifest=*/false, error)) {
      return false;
    }
  }

  out->invoke_payload = effective_invoke_payload;
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t invoke_decl = parsed;
  invoke_decl.invoke_name = effective_invoke_name;
  invoke_decl.invoke_payload = effective_invoke_payload;

  cuwacunu::camahjucunu::iitepi_wave_invoke_t invoke{};
  if (!cuwacunu::camahjucunu::parse_circuit_invoke_wave(invoke_decl, &invoke,
                                                        error)) {
    return false;
  }
  out->invoke_source_command = invoke.source_command;
  out->spec.instrument = invoke.source_symbol;
  if (selected_wave_source) {
    out->spec.runtime_instrument_signature =
        selected_wave_source->runtime_instrument_signature;
  }
  if (invoke.total_epochs > 0) {
    out->execution.epochs = invoke.total_epochs;
  }
  if (out->spec.instrument.empty()) {
    if (error) {
      *error = "empty instrument in invoke payload generated from "
               "RUNTIME_INSTRUMENT_SIGNATURE.SYMBOL: " +
               effective_invoke_payload;
    }
    return false;
  }

  std::unordered_map<std::string, std::unique_ptr<Tsi>> nodes_by_alias;
  nodes_by_alias.reserve(pending_decls.size());
  const auto emplace_node_for_decl =
      [&](const pending_decl_t &pending) -> bool {
    bool made_dataloader = false;
    std::string made_runtime_component_name{};
    std::unique_ptr<Tsi> node = make_tsi_for_decl<Datatype_t, Sampler_t>(
        next_id++, contract_hash, pending.type_id, *pending.decl, &out->spec,
        observation_instruction, jkimyei_specs, jkimyei_specs_dsl_text,
        parsed.name, device, source_batch_size_override, dataloader_workers,
        dataloader_force_rebuild_cache, dataloader_range_warn_batches,
        first_dataloader, pending.resolved_wikimyei_hashimyei,
        pending.wave_wikimyei_decl, wave_mode_train_enabled, &made_dataloader,
        &made_runtime_component_name, error);

    if (!node) {
      if (error) {
        if (!error->empty()) {
          /* preserve richer error from make_tsi_for_decl */
        } else if ((pending.type_id ==
                        TsiTypeId::WikimyeiRepresentationEncodingVicreg ||
                    pending.type_id ==
                        TsiTypeId::WikimyeiInferenceExpectedValueMdn) &&
                   !first_dataloader) {
          *error = "wikimyei requires a source dataloader in the same circuit";
        } else {
          *error = "unsupported tsi_type: " + pending.canonical_identity;
        }
      }
      return false;
    }

    auto [ins_it, inserted] =
        nodes_by_alias.emplace(pending.decl->alias, std::move(node));
    if (!inserted) {
      if (error)
        *error = "duplicated instance alias: " + pending.decl->alias;
      return false;
    }

    if (made_dataloader && !first_dataloader) {
      first_dataloader = dynamic_cast<DataloaderT<Datatype_t, Sampler_t> *>(
          ins_it->second.get());
      if (first_dataloader) {
        out->spec.channels = first_dataloader->C();
        out->spec.timesteps = first_dataloader->T();
        out->spec.features = first_dataloader->D();
        out->spec.batch_size_hint = first_dataloader->batch_size_hint();
        if (out->execution.batch_size == 0 && out->spec.batch_size_hint > 0) {
          out->execution.batch_size =
              static_cast<std::uint64_t>(out->spec.batch_size_hint);
        }
      }
    }
    if (pending.type_desc->domain == TsiDomain::Wikimyei) {
      RuntimeBindingContract::WikimyeiBindingIdentity binding{};
      binding.binding_id = trim_ascii_copy(pending.decl->alias);
      binding.canonical_type = std::string(pending.type_desc->canonical);
      binding.hashimyei = trim_ascii_copy(pending.resolved_wikimyei_hashimyei);
      binding.runtime_component_name = std::move(made_runtime_component_name);
      out->spec.wikimyei_bindings.push_back(std::move(binding));
    }
    return true;
  };

  // Build source dataloaders first to remove declaration-order coupling for
  // downstream wikimyei components that depend on discovered C/Hx/Dx hints.
  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain != TsiDomain::Source)
      continue;
    if (!emplace_node_for_decl(pending))
      return false;
  }
  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain == TsiDomain::Source)
      continue;
    if (!emplace_node_for_decl(pending))
      return false;
  }

  alias_to_tsi.clear();
  alias_to_tsi.reserve(pending_decls.size());
  for (const auto &decl : parsed.instances) {
    const auto node_it = nodes_by_alias.find(decl.alias);
    if (node_it == nodes_by_alias.end() || !node_it->second) {
      if (error) {
        *error = "internal runtime builder error: missing node for alias '" +
                 decl.alias + "'";
      }
      return false;
    }
    alias_to_tsi.emplace(decl.alias, node_it->second.get());
    out->nodes.push_back(std::move(node_it->second));
  }

  if (first_dataloader) {
    out->spec.channels = first_dataloader->C();
    out->spec.timesteps = first_dataloader->T();
    out->spec.features = first_dataloader->D();
    out->spec.batch_size_hint = first_dataloader->batch_size_hint();
    if (out->execution.batch_size == 0 && out->spec.batch_size_hint > 0) {
      out->execution.batch_size =
          static_cast<std::uint64_t>(out->spec.batch_size_hint);
    }
  }
  auto observation_instruction_copy = observation_instruction;
  out->spec.future_timesteps =
      observation_instruction_copy.max_future_sequence_length();

  if (!cuwacunu::camahjucunu::resolve_hops(parsed, &resolved_hops, error))
    return false;

  out->hops.reserve(resolved_hops.size());
  for (const auto &h : resolved_hops) {
    const auto it_from = alias_to_tsi.find(h.from.instance);
    const auto it_to = alias_to_tsi.find(h.to.instance);
    if (it_from == alias_to_tsi.end() || it_to == alias_to_tsi.end()) {
      if (error)
        *error = "hop references unknown instance alias: " + h.from.instance +
                 " -> " + h.to.instance;
      return false;
    }

    const auto *out_spec =
        find_directive(*it_from->second, h.from.directive, DirectiveDir::Out);
    const auto *in_spec =
        find_directive(*it_to->second, h.to.directive, DirectiveDir::In);
    if (!out_spec || !in_spec) {
      if (error) {
        *error =
            "hop directive not found on tsi declarations: " + h.from.instance +
            "@" + std::string(h.from.directive) + " -> " + h.to.instance + "@" +
            std::string(h.to.directive);
      }
      return false;
    }

    if (out_spec->kind.kind != h.from.kind) {
      if (error) {
        *error = "hop source kind mismatch against tsi declarations: " +
                 h.from.instance + "@" + std::string(h.from.directive);
      }
      return false;
    }
    if (!it_to->second->is_compatible(h.to.directive, out_spec->kind.kind)) {
      if (error) {
        *error = "hop target is not compatible with source kind: " +
                 h.from.instance + "@" + std::string(h.from.directive) +
                 " -> " + h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }
    // Do not enforce a single static input kind per directive here.
    // Some components (for example sink log) intentionally accept
    // multiple incoming payload kinds on the same directive.

    out->hops.push_back(hop(ep(*it_from->second, h.from.directive),
                            ep(*it_to->second, h.to.directive)));
  }

  out->seed_wave = normalize_wave_span(Wave{
      .cursor =
          WaveCursor{
              .id = 0,
              .i = invoke.wave_i,
              .episode = invoke.episode,
              .batch = invoke.batch,
          },
      .max_batches_per_epoch = invoke.max_batches_per_epoch,
      .span_begin_ms = invoke.span_begin_ms,
      .span_end_ms = invoke.span_end_ms,
      .has_time_span = invoke.has_time_span,
  });
  out->seed_ingress = Ingress{.directive = pick_start_directive(out->view()),
                              .signal = string_signal(invoke.source_command)};
  return true;
}
