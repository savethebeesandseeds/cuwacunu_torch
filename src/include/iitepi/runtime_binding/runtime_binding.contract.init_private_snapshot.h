template <typename Datatype_t, typename Sampler_t>
[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device) {
  runtime_binding_contract_init_record_t out{};
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  if (runtime_binding_itself)
    out.source_config_path = runtime_binding_itself->config_file_path;
  log_dbg("[runtime_binding.contract.init] start campaign_hash=%s binding=%s "
           "device=%s\n",
           runtime_binding_log_value_or_empty(out.campaign_hash),
           runtime_binding_log_value_or_empty(out.binding_id),
           device.str().c_str());

  try {
    const auto context = resolve_runtime_binding_snapshot_context_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself);
    out.contract_hash = context.contract_hash;
    out.wave_hash = context.wave_hash;
    out.source_config_path = context.source_config_path;
    if (!context.ok) {
      out.error = context.error;
      log_err("[runtime_binding.contract.init] context resolution failed "
              "campaign_hash=%s binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(out.campaign_hash),
              runtime_binding_log_value_or_empty(out.binding_id),
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }

    log_dbg("[runtime_binding.contract.init] resolved contract_hash=%s "
             "wave_hash=%s\n",
             runtime_binding_log_value_or_empty(out.contract_hash),
             runtime_binding_log_value_or_empty(out.wave_hash));
    if (!has_non_ws_text(context.contract_itself->circuit.dsl)) {
      out.error = "missing tsiemene circuit DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(context.wave_itself->wave.dsl)) {
      out.error = "missing iitepi wave DSL text in bound wave file";
      return out;
    }
    if (!has_non_ws_text(context.contract_itself->circuit.grammar)) {
      out.error = "missing tsiemene circuit grammar text in contract";
      return out;
    }

    // Build runtime binding payload from validated contract + wave DSLs.
    const auto &parsed = context.contract_itself->circuit.decoded();
    std::string build_error;
    if (!runtime_binding_builder::build_runtime_binding_from_instruction<
            Datatype_t, Sampler_t>(parsed, device, out.contract_hash,
                                   context.contract_itself, out.wave_hash,
                                   context.wave_itself, context.wave_id,
                                   &out.runtime_binding, &build_error)) {
      out.error = "failed to build runtime binding: " + build_error;
      return out;
    }
    out.runtime_binding.campaign_hash = out.campaign_hash;
    out.runtime_binding.runtime_binding_path =
        runtime_binding_itself->config_file_path;
    out.runtime_binding.binding_id = out.binding_id;
    out.runtime_binding.contract_hash = out.contract_hash;
    out.runtime_binding.wave_hash = out.wave_hash;

    // Final validation before exposing initialized runtime binding.
    RuntimeBindingIssue issue{};
    if (!validate_runtime_binding(out.runtime_binding, &issue)) {
      out.error =
          "invalid runtime binding: " + std::string(issue.circuit_issue.what);
      log_err("[runtime_binding.contract.init] validation failed "
              "campaign_hash=%s binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(out.campaign_hash),
              runtime_binding_log_value_or_empty(out.binding_id),
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }

    out.ok = true;
    log_dbg("[runtime_binding.contract.init] done campaign_hash=%s binding=%s "
             "contracts=%zu\n",
             runtime_binding_log_value_or_empty(out.campaign_hash),
             runtime_binding_log_value_or_empty(out.binding_id),
             out.runtime_binding.contracts.size());
    return out;
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.contract.init] exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: unknown";
    log_err("[runtime_binding.contract.init] exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device) {
  runtime_binding_contract_init_record_t out{};
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  if (runtime_binding_itself)
    out.source_config_path = runtime_binding_itself->config_file_path;
  log_dbg("[runtime_binding.contract.init] infer runtime campaign_hash=%s "
           "binding=%s device=%s\n",
           runtime_binding_log_value_or_empty(out.campaign_hash),
           runtime_binding_log_value_or_empty(out.binding_id),
           device.str().c_str());

  try {
    const auto preflight = resolve_runtime_binding_preflight_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself);
    out.contract_hash = preflight.contract_hash;
    out.wave_hash = preflight.wave_hash;
    out.resolved_record_type = preflight.resolved_record_type;
    out.resolved_sampler = preflight.resolved_sampler;
    if (!preflight.ok) {
      out.error = preflight.error;
      return out;
    }

    log_dbg("[runtime_binding.contract.init] resolved runtime "
             "campaign_hash=%s binding=%s record_type=%s sampler=%s\n",
             runtime_binding_log_value_or_empty(out.campaign_hash),
             runtime_binding_log_value_or_empty(out.binding_id),
             runtime_binding_log_value_or_empty(out.resolved_record_type),
             runtime_binding_log_value_or_empty(out.resolved_sampler));
    auto typed = invoke_runtime_binding_contract_init_from_preflight(
        preflight, runtime_binding_itself, device);
    if (!typed.ok) {
      log_err("[runtime_binding.contract.init] typed init failed "
              "campaign_hash=%s binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(typed.campaign_hash),
              runtime_binding_log_value_or_empty(typed.binding_id),
              runtime_binding_log_value_or_empty(typed.error));
    }
    return typed;
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.contract.init] exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: unknown";
    log_err("[runtime_binding.contract.init] exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}
