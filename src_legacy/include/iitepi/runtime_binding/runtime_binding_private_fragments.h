[[nodiscard]] inline bool populate_runtime_wikimyei_component_evidence(
    const RuntimeBindingContract &c,
    const RuntimeBindingContract::WikimyeiBindingIdentity &binding,
    TsiWikimyeiRuntimeIoContext *io_ctx, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!io_ctx) {
    if (error)
      *error = "missing wikimyei runtime I/O context";
    return false;
  }

  const auto *selected_fp = find_runtime_component_dsl_fingerprint(c, binding);
  if (!selected_fp) {
    if (error) {
      *error = "missing component DSL fingerprint for wikimyei binding: " +
               binding.binding_id;
    }
    return false;
  }
  if (selected_fp->tsi_type != binding.canonical_type) {
    if (error) {
      *error = "component DSL fingerprint type mismatch for binding '" +
               binding.binding_id + "': fingerprint=" + selected_fp->tsi_type +
               " runtime=" + binding.canonical_type;
    }
    return false;
  }
  if (selected_fp->component_tag.empty()) {
    if (error) {
      *error = "component DSL fingerprint is missing component TAG for "
               "binding: " +
               binding.binding_id;
    }
    return false;
  }
  if (selected_fp->component_compatibility_sha256_hex.empty()) {
    if (error) {
      *error =
          "component DSL fingerprint is missing component compatibility hash "
          "for binding: " +
          binding.binding_id;
    }
    return false;
  }

  std::string normalized_binding_hashimyei{};
  std::string normalized_fp_hashimyei{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(
          binding.hashimyei, &normalized_binding_hashimyei)) {
    if (error) {
      *error = "wikimyei binding hashimyei is not a valid 0x... ordinal for "
               "binding: " +
               binding.binding_id;
    }
    return false;
  }
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(selected_fp->hashimyei,
                                                    &normalized_fp_hashimyei) ||
      normalized_fp_hashimyei != normalized_binding_hashimyei) {
    if (error) {
      *error = "component DSL fingerprint hashimyei mismatch for binding: " +
               binding.binding_id;
    }
    return false;
  }

  std::string signature_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          selected_fp->instrument_signature, /*allow_any=*/true,
          "component INSTRUMENT_SIGNATURE <" + binding.binding_id + ">",
          &signature_error)) {
    if (error)
      *error = signature_error;
    return false;
  }
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          c.spec.runtime_instrument_signature, /*allow_any=*/false,
          "runtime instrument signature <" + binding.binding_id + ">",
          &signature_error)) {
    if (error)
      *error = signature_error;
    return false;
  }

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(c.spec.contract_hash);
  if (!contract_snapshot) {
    if (error) {
      *error = "missing contract snapshot for wikimyei component evidence on "
               "binding: " +
               binding.binding_id;
    }
    return false;
  }
  if (contract_snapshot->signature.docking_signature_sha256_hex.empty()) {
    if (error) {
      *error = "contract docking signature hash is empty for binding: " +
               binding.binding_id;
    }
    return false;
  }

  io_ctx->component_tag = selected_fp->component_tag;
  io_ctx->component_compatibility_sha256_hex =
      lowercase_copy(selected_fp->component_compatibility_sha256_hex);
  io_ctx->docking_signature_sha256_hex =
      lowercase_copy(contract_snapshot->signature.docking_signature_sha256_hex);
  io_ctx->instrument_signature = selected_fp->instrument_signature;
  io_ctx->runtime_instrument_signature = c.spec.runtime_instrument_signature;
  return true;
}

inline bool load_contract_wikimyei_report_fragments(RuntimeBindingContract &c,
                                                    const RuntimeContext &ctx,
                                                    std::string *error) {
  if (error)
    error->clear();

  for (auto &node : c.nodes) {
    auto *wik = dynamic_cast<TsiWikimyei *>(node.get());
    if (!wik)
      continue;
    if (!wik->supports_init_report_fragments())
      continue;
    if (!wik->runtime_autoload_report_fragments())
      continue;

    TsiWikimyeiRuntimeIoContext io_ctx{};
    io_ctx.enable_debug_outputs = c.execution.debug_enabled;
    io_ctx.binding_id = ctx.binding_id;
    io_ctx.run_id = ctx.run_id;
    io_ctx.source_runtime_cursor = ctx.source_runtime_cursor;
    io_ctx.has_wave_cursor = ctx.has_wave_cursor;
    io_ctx.wave_cursor = ctx.wave_cursor;
    RuntimeBindingContract::WikimyeiBindingIdentity binding{};
    std::string local_error;
    if (!resolve_runtime_wikimyei_binding(c, *wik, &binding, &local_error)) {
      if (error) {
        *error = "failed to resolve wikimyei binding for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!populate_runtime_wikimyei_component_evidence(c, binding, &io_ctx,
                                                      &local_error)) {
      if (error) {
        *error = "failed to resolve wikimyei component evidence for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!wik->runtime_load_from_hashimyei(binding.hashimyei, &local_error,
                                          &io_ctx)) {
      // Training-enabled wikimyei are allowed to bootstrap from scratch when
      // the configured hashimyei has not been founded yet. The first
      // successful run will persist the freshly constructed state as the
      // initial report_fragment set.
      const bool missing_report_fragment =
          local_error.find("not found") != std::string::npos;
      if (missing_report_fragment && wik->runtime_autosave_report_fragments()) {
        const std::string instance_name(wik->instance_name());
        log_warn(
            "[runtime_binding.contract.run] configured hashimyei missing for "
            "node=%s hashimyei=%s; bootstrapping a new hashimyei from the "
            "active founding_dsl_bundle/runtime state\n",
            instance_name.c_str(), binding.hashimyei.c_str());
        continue;
      }
      if (error) {
        *error = "failed to load wikimyei report fragments for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    bool validated_from_component_manifest = false;
    std::string manifest_error{};
    if (const auto manifest_opt =
            build_runtime_component_manifest(c, *wik, &manifest_error);
        manifest_opt.has_value()) {
      const auto store_root = cuwacunu::hashimyei::store_root();
      const std::string component_id =
          cuwacunu::hero::hashimyei::compute_component_manifest_id(
              *manifest_opt);
      const auto manifest_path =
          cuwacunu::hero::hashimyei::component_manifest_path(
              store_root, manifest_opt->canonical_path, component_id);
      cuwacunu::hero::hashimyei::component_manifest_t stored_manifest{};
      if (cuwacunu::hero::hashimyei::load_component_manifest(
              manifest_path, &stored_manifest, &manifest_error)) {
        const auto contract_snapshot =
            cuwacunu::iitepi::contract_space_t::contract_itself(
                c.spec.contract_hash);
        if (!contract_snapshot) {
          if (error) {
            *error = "missing contract snapshot for component compatibility "
                     "validation on node '" +
                     std::string(wik->instance_name()) + "'";
          }
          return false;
        }
        if (!validate_runtime_component_manifest_component_compatibility(
                stored_manifest, c.spec.contract_hash, binding.binding_id,
                &local_error)) {
          if (error) {
            *error = "configured hashimyei failed component compatibility "
                     "validation for node '" +
                     std::string(wik->instance_name()) + "': " + local_error;
          }
          return false;
        }
        validated_from_component_manifest = true;
      }
    }
    if (!validated_from_component_manifest &&
        !validate_runtime_hashimyei_component_compatibility(
            binding.canonical_type, binding.hashimyei, c.spec.contract_hash,
            binding.binding_id,
            /*require_registered_manifest=*/true, &local_error)) {
      if (error) {
        *error =
            "configured hashimyei failed component compatibility validation "
            "for node '" +
            std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}

inline bool save_contract_wikimyei_report_fragments(RuntimeBindingContract &c,
                                                    const RuntimeContext &ctx,
                                                    std::string *error) {
  if (error)
    error->clear();

  for (auto &node : c.nodes) {
    auto *wik = dynamic_cast<TsiWikimyei *>(node.get());
    if (!wik)
      continue;
    if (!wik->supports_init_report_fragments())
      continue;

    TsiWikimyeiRuntimeIoContext io_ctx{};
    io_ctx.enable_debug_outputs = c.execution.debug_enabled;
    io_ctx.binding_id = ctx.binding_id;
    io_ctx.run_id = ctx.run_id;
    io_ctx.source_runtime_cursor = ctx.source_runtime_cursor;
    io_ctx.has_wave_cursor = ctx.has_wave_cursor;
    io_ctx.wave_cursor = ctx.wave_cursor;
    const bool should_save_report_fragments =
        wik->runtime_autosave_report_fragments() || io_ctx.enable_debug_outputs;
    if (!should_save_report_fragments)
      continue;
    RuntimeBindingContract::WikimyeiBindingIdentity binding{};
    std::string local_error;
    if (!resolve_runtime_wikimyei_binding(c, *wik, &binding, &local_error)) {
      if (error) {
        *error = "failed to resolve wikimyei binding for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!populate_runtime_wikimyei_component_evidence(c, binding, &io_ctx,
                                                      &local_error)) {
      if (error) {
        *error = "failed to resolve wikimyei component evidence for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!wik->runtime_save_to_hashimyei(binding.hashimyei, &local_error,
                                        &io_ctx)) {
      if (error) {
        *error = "failed to save wikimyei report fragments for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!persist_runtime_component_lineage(c, *wik, &local_error)) {
      if (error) {
        *error = "failed to persist wikimyei component lineage for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}
