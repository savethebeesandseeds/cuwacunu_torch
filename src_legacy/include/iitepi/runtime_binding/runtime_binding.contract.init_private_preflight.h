struct runtime_binding_preflight_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{"runtime_binding.preflight"};
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string source_config_path{};
  runtime_binding_lock_plan_t lock_plan{};
};

[[nodiscard]] inline bool acquire_runtime_binding_lock_plan(
    const runtime_binding_lock_plan_t &lock_plan,
    std::vector<cuwacunu::hero::runtime_lock::scoped_lock_t> *out_locks,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out_locks) {
    if (error)
      *error = "runtime lock output vector is null";
    return false;
  }
  out_locks->clear();
  out_locks->reserve(lock_plan.mutable_resource_keys.size());
  for (const auto &key : lock_plan.mutable_resource_keys) {
    cuwacunu::hero::runtime_lock::scoped_lock_t lock{};
    std::string local_error{};
    if (!cuwacunu::hero::runtime_lock::acquire_mutable_resource(&lock, key,
                                                                &local_error)) {
      if (error) {
        *error = "failed to acquire mutable runtime resource lock for '" + key +
                 "': " + local_error;
      }
      out_locks->clear();
      return false;
    }
    log_dbg("[runtime_binding.run] mutable runtime resource lock acquired "
            "key=%s "
            "lock_path=%s\n",
            runtime_binding_log_value_or_empty(key),
            runtime_binding_log_value_or_empty(lock.path));
    out_locks->push_back(std::move(lock));
  }
  return true;
}

[[nodiscard]] inline bool
runtime_binding_init_parse_bool_ascii(std::string value, bool *out) {
  if (!out)
    return false;
  value = runtime_binding_init_lower_ascii_copy(
      runtime_binding_init_trim_ascii_copy(std::move(value)));
  if (value == "1" || value == "true" || value == "yes" || value == "on") {
    *out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "no" || value == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] inline std::uint64_t runtime_binding_now_ms_utc() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

[[nodiscard]] inline std::string make_runtime_binding_contract_run_id(
    const runtime_binding_run_record_t &run_record,
    std::size_t contract_index) {
  std::ostringstream out;
  out << "run."
      << runtime_binding_init_trim_ascii_copy(run_record.campaign_hash) << "."
      << runtime_binding_init_trim_ascii_copy(run_record.binding_id) << "."
      << contract_index << "." << runtime_binding_now_ms_utc();
  return out.str();
}

[[nodiscard]] inline bool resolve_active_record_type_from_observation(
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    std::string *out_record_type, std::string *error) {
  if (!out_record_type) {
    if (error) {
      *error = "missing output pointer while resolving observation record_type";
    }
    return false;
  }
  std::unordered_set<std::string> active_types{};
  for (const auto &ch : observation.channel_forms) {
    bool active = false;
    if (!runtime_binding_init_parse_bool_ascii(ch.active, &active)) {
      if (error) {
        *error = "invalid observation channel active flag '" + ch.active +
                 "' for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) +
                 "'";
      }
      return false;
    }
    if (!active)
      continue;
    const std::string record = runtime_binding_init_lower_ascii_copy(
        runtime_binding_init_trim_ascii_copy(ch.record_type));
    if (record.empty()) {
      if (error) {
        *error =
            "active observation channel has empty record_type for interval '" +
            cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) + "'";
      }
      return false;
    }
    active_types.insert(record);
  }

  if (active_types.empty()) {
    if (error)
      *error = "no active observation channels found";
    return false;
  }
  if (active_types.size() != 1) {
    std::string joined;
    for (const auto &item : active_types) {
      if (!joined.empty())
        joined += ", ";
      joined += item;
    }
    if (error) {
      *error =
          "active observation channels must share one record_type; found: " +
          joined;
    }
    return false;
  }

  *out_record_type = *active_types.begin();
  return true;
}

[[nodiscard]] inline bool resolve_binding_wave_sampler(
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    const cuwacunu::camahjucunu::runtime_binding_bind_decl_t &bind,
    const std::string &wave_hash, std::string *out_sampler,
    std::string *error) {
  if (!runtime_binding_itself || !out_sampler) {
    if (error)
      *error = "missing runtime binding record while resolving wave sampler";
    return false;
  }
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  const auto &wave_set = wave_itself->wave.decoded();
  const auto *wave = find_wave_by_id(wave_set, bind.wave_ref);
  if (!wave) {
    if (error)
      *error = "binding references unknown WAVE id: " + bind.wave_ref;
    return false;
  }
  const std::string sampler = runtime_binding_init_lower_ascii_copy(
      runtime_binding_init_trim_ascii_copy(wave->sampler));
  if (sampler != "sequential" && sampler != "random") {
    if (error) {
      *error = "unsupported wave sampler '" + wave->sampler +
               "' (expected sequential|random)";
    }
    return false;
  }
  *out_sampler = sampler;
  return true;
}

[[nodiscard]] inline runtime_binding_preflight_record_t
resolve_runtime_binding_preflight_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself) {
  runtime_binding_preflight_record_t out{};
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  if (runtime_binding_itself)
    out.source_config_path = runtime_binding_itself->config_file_path;

  const auto context = resolve_runtime_binding_snapshot_context_from_snapshot(
      campaign_hash, binding_id, runtime_binding_itself);
  out.contract_hash = context.contract_hash;
  out.wave_hash = context.wave_hash;
  if (!context.ok) {
    out.error = context.error;
    return out;
  }

  const auto &wave_set = context.wave_itself->wave.decoded();
  const auto *wave = find_wave_by_id(wave_set, context.wave_id);
  if (!wave) {
    out.error = "binding references unknown WAVE id: " + context.wave_id;
    return out;
  }

  cuwacunu::camahjucunu::observation_spec_t effective_observation{};
  if (!runtime_binding_builder::load_wave_dataloader_observation_payloads(
          context.contract_itself, context.wave_itself, *wave, nullptr, nullptr,
          &effective_observation, &out.error)) {
    return out;
  }

  if (!resolve_active_record_type_from_observation(
          effective_observation, &out.resolved_record_type, &out.error)) {
    return out;
  }
  {
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
        effective_instruction{};
    if (!runtime_binding_builder::select_contract_circuit(
            context.contract_itself->circuit.decoded(), wave->circuit_name,
            &effective_instruction, &out.error)) {
      return out;
    }
    if (!runtime_binding_builder::validate_selected_wave_instrument_signatures(
            context.contract_itself, effective_instruction, *wave,
            effective_observation, &out.error)) {
      return out;
    }
  }

  const auto &runtime_binding_instruction =
      runtime_binding_itself->runtime_binding.decoded();
  const auto *bind = find_bind_by_id(runtime_binding_instruction, binding_id);
  if (!bind) {
    out.error = "unknown runtime binding id: " + binding_id;
    return out;
  }
  if (!resolve_binding_wave_sampler(runtime_binding_itself, *bind,
                                    context.wave_hash, &out.resolved_sampler,
                                    &out.error)) {
    return out;
  }

  if (!build_runtime_binding_lock_plan_from_snapshot_context(
          context, *wave, effective_observation, out.resolved_record_type,
          &out.lock_plan, &out.error)) {
    return out;
  }

  out.ok = true;
  return out;
}

[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_preflight(
    const runtime_binding_preflight_record_t &preflight,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  if (preflight.resolved_record_type == "kline") {
    if (preflight.resolved_sampler == "sequential") {
      auto typed = invoke_runtime_binding_contract_init_from_snapshot<
          cuwacunu::camahjucunu::exchange::kline_t,
          torch::data::samplers::SequentialSampler>(
          preflight.campaign_hash, preflight.binding_id, runtime_binding_itself,
          device);
      typed.resolved_record_type = preflight.resolved_record_type;
      typed.resolved_sampler = preflight.resolved_sampler;
      return typed;
    }
    if (preflight.resolved_sampler == "random") {
      auto typed = invoke_runtime_binding_contract_init_from_snapshot<
          cuwacunu::camahjucunu::exchange::kline_t,
          torch::data::samplers::RandomSampler>(preflight.campaign_hash,
                                                preflight.binding_id,
                                                runtime_binding_itself, device);
      typed.resolved_record_type = preflight.resolved_record_type;
      typed.resolved_sampler = preflight.resolved_sampler;
      return typed;
    }
  }

  runtime_binding_contract_init_record_t out{};
  out.campaign_hash = preflight.campaign_hash;
  out.binding_id = preflight.binding_id;
  out.contract_hash = preflight.contract_hash;
  out.wave_hash = preflight.wave_hash;
  out.source_config_path = preflight.source_config_path;
  out.resolved_record_type = preflight.resolved_record_type;
  out.resolved_sampler = preflight.resolved_sampler;
  out.error = "unsupported runtime combination record_type='" +
              preflight.resolved_record_type + "' sampler='" +
              preflight.resolved_sampler +
              "' (supported now: kline + sequential|random)";
  return out;
}
