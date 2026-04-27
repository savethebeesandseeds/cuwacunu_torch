template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
std::unique_ptr<Tsi> make_tsi_for_decl(
    TsiId id, const cuwacunu::iitepi::contract_hash_t &contract_hash,
    TsiTypeId type_id,
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t &decl,
    RuntimeBindingContract::Spec *spec,
    const cuwacunu::camahjucunu::observation_spec_t &observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text, std::string_view circuit_name,
    torch::Device device, std::size_t source_batch_size_override,
    std::uint64_t dataloader_workers, bool dataloader_force_rebuild_cache,
    std::uint64_t dataloader_range_warn_batches,
    const DataloaderT<Datatype_t, Sampler_t> *first_dataloader,
    std::string_view resolved_wikimyei_hashimyei,
    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl,
    bool wave_mode_train_enabled, bool *made_dataloader,
    std::string *runtime_component_name_out, std::string *error) {
  if (made_dataloader)
    *made_dataloader = false;
  if (runtime_component_name_out)
    runtime_component_name_out->clear();
  if (!spec)
    return nullptr;

  switch (type_id) {
  case TsiTypeId::SourceDataloader:
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.source.dataloader does not accept instance arguments for "
                 "alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }
    if (made_dataloader)
      *made_dataloader = true;
    return std::make_unique<DataloaderT<Datatype_t, Sampler_t>>(
        id, spec->runtime_instrument_signature, observation_instruction, device,
        source_batch_size_override, dataloader_workers,
        dataloader_force_rebuild_cache, dataloader_range_warn_batches,
        contract_hash);
  case TsiTypeId::WikimyeiRepresentationEncodingVicreg: {
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.wikimyei.representation.encoding.vicreg does not accept "
                 "instance "
                 "arguments for alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }
    if (!first_dataloader)
      return nullptr;

    const std::string node_hashimyei =
        trim_ascii_copy(std::string(resolved_wikimyei_hashimyei)).empty()
            ? spec->representation_hashimyei
            : trim_ascii_copy(std::string(resolved_wikimyei_hashimyei));
    std::string base_component_lookup_name{};
    if (!resolve_vicreg_component_lookup_name(node_hashimyei, jkimyei_specs,
                                              &base_component_lookup_name,
                                              error)) {
      return nullptr;
    }
    std::string lookup_component_name = base_component_lookup_name;
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *selected_profile_row =
        nullptr;

    if (wave_wikimyei_decl) {
      spec->vicreg_train = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }
    {
      std::string selected_profile_id =
          wave_wikimyei_decl ? trim_ascii_copy(wave_wikimyei_decl->profile_id)
                             : std::string{};
      const bool profile_selected_by_wave = !selected_profile_id.empty();
      if (selected_profile_id.empty()) {
        const std::vector<std::string> profile_ids =
            collect_component_profile_ids(jkimyei_specs,
                                          {base_component_lookup_name});
        if (profile_ids.size() == 1) {
          selected_profile_id = profile_ids.front();
        } else {
          if (error) {
            if (profile_ids.empty()) {
              *error = "no compatible jkimyei profile found for component '" +
                       base_component_lookup_name + "'";
            } else {
              std::ostringstream oss;
              for (std::size_t i = 0; i < profile_ids.size(); ++i) {
                if (i != 0)
                  oss << ", ";
                oss << profile_ids[i];
              }
              *error = "wave omitted PROFILE_ID for component '" +
                       base_component_lookup_name +
                       "' but multiple compatible profiles exist: [" +
                       oss.str() + "]";
            }
          }
          return nullptr;
        }
      }
      selected_profile_row = find_jkimyei_component_profile_row(
          jkimyei_specs, base_component_lookup_name, selected_profile_id);
      if (!selected_profile_row) {
        if (error) {
          *error = (profile_selected_by_wave ? "wave profile_id '"
                                             : "resolved profile_id '") +
                   selected_profile_id + "' not found for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      const auto row_id_it = selected_profile_row->find(ROW_ID_COLUMN_HEADER);
      if (row_id_it == selected_profile_row->end() ||
          trim_ascii_copy(row_id_it->second).empty()) {
        if (error) {
          *error = "selected jkimyei component profile row has empty row_id "
                   "for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      lookup_component_name = trim_ascii_copy(row_id_it->second);
    }

    const auto *component_row = find_jkimyei_row_by_id(
        jkimyei_specs, "components_table", base_component_lookup_name);
    if (selected_profile_row) {
      apply_vicreg_flag_overrides_from_component_row(spec,
                                                     selected_profile_row);
    } else {
      apply_vicreg_flag_overrides_from_component_row(spec, component_row);
    }
    if (wave_wikimyei_decl) {
      spec->vicreg_train = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }

    const std::string runtime_component_name =
        compose_wikimyei_runtime_component_name(lookup_component_name,
                                                circuit_name, decl.alias);
    spec->representation_component_name = runtime_component_name;
    if (runtime_component_name_out) {
      *runtime_component_name_out = runtime_component_name;
    }
    if (!jkimyei_specs_dsl_text.empty()) {
      cuwacunu::jkimyei::jk_setup_t::registry
          .set_component_instruction_override(
              contract_hash, runtime_component_name, lookup_component_name,
              std::string(jkimyei_specs_dsl_text));
    }

    // Runtime ctor settings are derived from contract spec.
    const int C = (spec->channels > 0)
                      ? static_cast<int>(spec->channels)
                      : static_cast<int>(first_dataloader->C());
    const int T = (spec->timesteps > 0)
                      ? static_cast<int>(spec->timesteps)
                      : static_cast<int>(first_dataloader->T());
    const int D = (spec->features > 0)
                      ? static_cast<int>(spec->features)
                      : static_cast<int>(first_dataloader->D());
    return std::make_unique<TsiWikimyeiRepresentationEncodingVicreg>(
        id, decl.alias, contract_hash, node_hashimyei, runtime_component_name,
        C, T, D,
        /*train=*/spec->vicreg_train,
        /*use_swa=*/spec->vicreg_use_swa,
        /*detach_to_cpu=*/spec->vicreg_detach_to_cpu);
  }
  case TsiTypeId::WikimyeiInferenceExpectedValueMdn: {
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.wikimyei.inference.expected_value.mdn does not accept "
                 "instance "
                 "arguments for alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }

    const std::string node_hashimyei =
        trim_ascii_copy(std::string(resolved_wikimyei_hashimyei));
    std::string base_component_lookup_name{};
    if (!resolve_expected_value_component_lookup_name(
            node_hashimyei, jkimyei_specs, &base_component_lookup_name,
            error)) {
      return nullptr;
    }
    std::string lookup_component_name = base_component_lookup_name;
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *selected_profile_row =
        nullptr;
    bool train_enabled = wave_mode_train_enabled;
    if (wave_wikimyei_decl) {
      train_enabled = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }

    {
      std::string selected_profile_id =
          wave_wikimyei_decl ? trim_ascii_copy(wave_wikimyei_decl->profile_id)
                             : std::string{};
      const bool profile_selected_by_wave = !selected_profile_id.empty();
      if (selected_profile_id.empty()) {
        const std::vector<std::string> profile_ids =
            collect_component_profile_ids(jkimyei_specs,
                                          {base_component_lookup_name});
        if (profile_ids.size() == 1) {
          selected_profile_id = profile_ids.front();
        } else {
          if (error) {
            if (profile_ids.empty()) {
              *error = "no compatible jkimyei profile found for component '" +
                       base_component_lookup_name + "'";
            } else {
              std::ostringstream oss;
              for (std::size_t i = 0; i < profile_ids.size(); ++i) {
                if (i != 0)
                  oss << ", ";
                oss << profile_ids[i];
              }
              *error = "wave omitted PROFILE_ID for component '" +
                       base_component_lookup_name +
                       "' but multiple compatible profiles exist: [" +
                       oss.str() + "]";
            }
          }
          return nullptr;
        }
      }
      selected_profile_row = find_jkimyei_component_profile_row(
          jkimyei_specs, base_component_lookup_name, selected_profile_id);
      if (!selected_profile_row) {
        if (error) {
          *error = (profile_selected_by_wave ? "wave profile_id '"
                                             : "resolved profile_id '") +
                   selected_profile_id + "' not found for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      const auto row_id_it = selected_profile_row->find(ROW_ID_COLUMN_HEADER);
      if (row_id_it == selected_profile_row->end() ||
          trim_ascii_copy(row_id_it->second).empty()) {
        if (error) {
          *error = "selected jkimyei component profile row has empty row_id "
                   "for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      lookup_component_name = trim_ascii_copy(row_id_it->second);
    }

    const std::string runtime_component_name =
        compose_wikimyei_runtime_component_name(lookup_component_name,
                                                circuit_name, decl.alias);
    if (runtime_component_name_out) {
      *runtime_component_name_out = runtime_component_name;
    }
    if (!jkimyei_specs_dsl_text.empty()) {
      cuwacunu::jkimyei::jk_setup_t::registry
          .set_component_instruction_override(
              contract_hash, runtime_component_name, lookup_component_name,
              std::string(jkimyei_specs_dsl_text));
    }

    return std::make_unique<TsiWikimyeiInferenceExpectedValueMdn>(
        id, decl.alias, contract_hash, node_hashimyei, runtime_component_name,
        train_enabled);
  }
  case TsiTypeId::SinkNull:
    if (!decl.args.empty()) {
      if (error) {
        *error =
            "tsi.sink.null does not accept instance arguments for alias '" +
            decl.alias + "'";
      }
      return nullptr;
    }
    return std::make_unique<TsiSinkNull>(id, decl.alias);
  case TsiTypeId::SinkLogSys: {
    TsiSinkLogSys::LogMode log_mode = TsiSinkLogSys::LogMode::EachBatch;
    if (!parse_probe_log_mode_from_instance_args(decl, &log_mode, error)) {
      return nullptr;
    }
    return std::make_unique<TsiSinkLogSys>(id, decl.alias, log_mode);
  }
  }
  return nullptr;
}
