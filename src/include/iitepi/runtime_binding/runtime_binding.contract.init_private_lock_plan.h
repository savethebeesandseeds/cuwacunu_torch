struct runtime_binding_lock_plan_t {
  std::vector<std::string> mutable_resource_keys{};
};

[[nodiscard]] inline std::string
runtime_binding_normalize_resource_path_key(std::string path) {
  path = runtime_binding_init_trim_ascii_copy(std::move(path));
  if (path.empty())
    return {};
  return std::filesystem::path(path).lexically_normal().string();
}

[[nodiscard]] inline std::string
runtime_binding_mutable_wikimyei_resource_key(std::string family,
                                              std::string hashimyei) {
  family = runtime_binding_init_lower_ascii_copy(
      runtime_binding_init_trim_ascii_copy(std::move(family)));
  hashimyei = runtime_binding_init_trim_ascii_copy(std::move(hashimyei));
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&hashimyei);
  if (family.empty() || hashimyei.empty())
    return {};
  return "wikimyei." + family + "." + hashimyei;
}

[[nodiscard]] inline std::string
runtime_binding_mutable_source_cache_key(std::string scope, std::string path) {
  scope = runtime_binding_init_lower_ascii_copy(
      runtime_binding_init_trim_ascii_copy(std::move(scope)));
  path = runtime_binding_normalize_resource_path_key(std::move(path));
  if (scope.empty() || path.empty())
    return {};
  return "source-cache." + scope + "." + path;
}

inline void runtime_binding_append_source_cache_resource_keys(
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    runtime_binding_lock_plan_t *out) {
  if (!out)
    return;
  std::string signature_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          runtime_signature, /*allow_any=*/false,
          "runtime lock source signature", &signature_error)) {
    return;
  }
  const std::string normalized_record_type =
      runtime_binding_init_trim_ascii_copy(runtime_signature.record_type);

  for (const auto &channel_form : observation.channel_forms) {
    const std::string active = runtime_binding_init_lower_ascii_copy(
        runtime_binding_init_trim_ascii_copy(channel_form.active));
    if (active != "true")
      continue;
    const std::string channel_record_type =
        runtime_binding_init_trim_ascii_copy(channel_form.record_type);
    if (channel_record_type != normalized_record_type)
      continue;

    std::string normalization_policy =
        runtime_binding_init_trim_ascii_copy(channel_form.normalization_policy);
    if (normalization_policy.empty())
      normalization_policy = "none";
    const std::string canonical_normalization_policy =
        cuwacunu::camahjucunu::data::canonical_normalization_policy(
            normalization_policy);

    for (const auto &source_form : observation.filter_source_forms(
             runtime_signature, channel_form.interval)) {
      const std::string source_csv =
          runtime_binding_normalize_resource_path_key(source_form.source);
      if (source_csv.empty())
        continue;

      const std::string raw_cache = runtime_binding_mutable_source_cache_key(
          "raw", cuwacunu::camahjucunu::data::raw_binary_for_csv(source_csv));
      if (!raw_cache.empty())
        out->mutable_resource_keys.push_back(raw_cache);

      if (!cuwacunu::camahjucunu::data::normalization_policy_enabled(
              canonical_normalization_policy)) {
        continue;
      }

      const std::string normalized_cache =
          runtime_binding_mutable_source_cache_key(
              "norm", cuwacunu::camahjucunu::data::normalized_bin_for_csv(
                          source_csv, canonical_normalization_policy));
      if (!normalized_cache.empty())
        out->mutable_resource_keys.push_back(normalized_cache);
    }
  }
}

[[nodiscard]] inline runtime_binding_snapshot_context_t
resolve_runtime_binding_snapshot_context_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself) {
  runtime_binding_snapshot_context_t out{};
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  out.runtime_binding_itself = runtime_binding_itself;
  if (runtime_binding_itself)
    out.source_config_path = runtime_binding_itself->config_file_path;
  if (!runtime_binding_itself) {
    out.error = "missing runtime binding record";
    return out;
  }

  const auto &runtime_binding_instruction =
      runtime_binding_itself->runtime_binding.decoded();
  const auto *bind = find_bind_by_id(runtime_binding_instruction, binding_id);
  if (!bind) {
    out.error = "unknown runtime binding id: " + binding_id;
    return out;
  }
  const auto *contract_decl =
      find_contract_decl_by_id(runtime_binding_instruction, bind->contract_ref);
  if (!contract_decl) {
    out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    return out;
  }

  const std::string contract_path =
      (std::filesystem::path(runtime_binding_itself->config_file_path)
           .parent_path() /
       contract_decl->file)
          .string();
  out.contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(contract_path);
  out.wave_hash =
      cuwacunu::iitepi::runtime_binding_space_t::wave_hash_for_binding(
          campaign_hash, binding_id);
  out.wave_id = bind->wave_ref;
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
      out.contract_hash);
  cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(out.wave_hash);
  out.contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);
  out.wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(out.wave_hash);
  if (!out.contract_itself) {
    out.error = "missing contract snapshot for hash: " + out.contract_hash;
    return out;
  }
  if (!out.wave_itself) {
    out.error = "missing wave snapshot for hash: " + out.wave_hash;
    return out;
  }
  out.ok = true;
  return out;
}

[[nodiscard]] inline bool build_runtime_binding_lock_plan_from_snapshot_context(
    const runtime_binding_snapshot_context_t &context,
    const cuwacunu::camahjucunu::iitepi_wave_t &selected_wave,
    const cuwacunu::camahjucunu::observation_spec_t &effective_observation,
    std::string_view resolved_record_type, runtime_binding_lock_plan_t *out,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "runtime lock output plan is null";
    return false;
  }
  out->mutable_resource_keys.clear();
  if (!context.contract_itself) {
    if (error)
      *error = "missing contract snapshot while building runtime lock plan";
    return false;
  }

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t effective_instruction{};
  const auto &contract_instruction = context.contract_itself->circuit.decoded();
  if (!runtime_binding_builder::select_contract_circuit(
          contract_instruction, selected_wave.circuit_name,
          &effective_instruction, error)) {
    return false;
  }

  for (const auto &circuit : effective_instruction.circuits) {
    bool has_dataloader_source = false;
    cuwacunu::camahjucunu::instrument_signature_t runtime_signature{};

    for (const auto &decl : circuit.instances) {
      std::string path_error{};
      const auto contract_path =
          runtime_binding_builder::resolve_runtime_node_path(
              decl.tsi_type, context.contract_hash,
              /*
                Contract circuits may acknowledge hashimyei-backed wikimyei
                slots by family only; the contract-derived component identity
                contributes the exact hashimyei path later in this lock-plan
                build.
              */
              /*allow_family_without_hash=*/true, &path_error);
      if (!contract_path.has_value()) {
        if (error) {
          *error = "invalid runtime node path for alias '" + decl.alias +
                   "': " + path_error;
        }
        return false;
      }

      const auto *type_desc = find_tsi_type(contract_path->type_id);
      if (!type_desc) {
        if (error) {
          *error = "missing tsi type descriptor for alias '" + decl.alias + "'";
        }
        return false;
      }

      if (type_desc->domain == TsiDomain::Wikimyei) {
        std::string hashimyei = contract_path->hashimyei;
        if (const auto *wave_wikimyei_decl = runtime_binding_builder::
                find_wave_wikimyei_decl_for_binding_id_or_path(
                    selected_wave, *contract_path, decl.alias,
                    context.contract_hash)) {
          std::string wave_path_error{};
          const auto wave_path =
              runtime_binding_builder::resolve_runtime_node_path(
                  wave_wikimyei_decl->wikimyei_path, context.contract_hash,
                  /*allow_family_without_hash=*/false, &wave_path_error);
          if (!wave_path.has_value()) {
            if (error) {
              *error = "invalid WIKIMYEI path for alias '" + decl.alias +
                       "': " + wave_path_error;
            }
            return false;
          }
          if (!wave_path->hashimyei.empty())
            hashimyei = wave_path->hashimyei;
        }

        const std::string key = runtime_binding_mutable_wikimyei_resource_key(
            std::string(type_desc->canonical), std::move(hashimyei));
        if (!key.empty())
          out->mutable_resource_keys.push_back(key);
      }

      if (type_desc->domain == TsiDomain::Source &&
          type_desc->canonical ==
              cuwacunu::source::dataloader::kCanonicalType) {
        const auto *wave_source_decl = runtime_binding_builder::
            find_wave_source_decl_for_binding_id_or_path(
                selected_wave, *contract_path, decl.alias,
                context.contract_hash);
        if (!wave_source_decl) {
          if (error) {
            *error =
                "missing SOURCE wave block for binding id '" + decl.alias + "'";
          }
          return false;
        }
        has_dataloader_source = true;
        if (runtime_signature.symbol.empty()) {
          runtime_signature = wave_source_decl->runtime_instrument_signature;
        }
      }
    }

    if (has_dataloader_source) {
      if (runtime_signature.symbol.empty()) {
        if (error) {
          *error = "empty source runtime signature while building runtime lock "
                   "plan";
        }
        return false;
      }
      if (runtime_binding_init_trim_ascii_copy(runtime_signature.record_type) !=
          runtime_binding_init_trim_ascii_copy(
              std::string(resolved_record_type))) {
        if (error) {
          *error = "source runtime signature record_type does not match "
                   "resolved record type while building runtime lock plan";
        }
        return false;
      }
      runtime_binding_append_source_cache_resource_keys(effective_observation,
                                                        runtime_signature, out);
    }
  }

  std::sort(out->mutable_resource_keys.begin(),
            out->mutable_resource_keys.end());
  out->mutable_resource_keys.erase(
      std::unique(out->mutable_resource_keys.begin(),
                  out->mutable_resource_keys.end()),
      out->mutable_resource_keys.end());
  return true;
}
