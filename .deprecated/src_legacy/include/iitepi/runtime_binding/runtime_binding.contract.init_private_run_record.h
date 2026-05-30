[[nodiscard]] inline runtime_binding_run_record_t
run_initialized_runtime_binding(runtime_binding_contract_init_record_t init) {
  // Finalize the init snapshot into one immutable run record.
  runtime_binding_run_record_t out{};
  out.campaign_hash = init.campaign_hash;
  out.binding_id = init.binding_id;
  out.source_config_path = init.source_config_path;

  if (!init.ok) {
    out.error = std::move(init.error);
    out.contract_hash = std::move(init.contract_hash);
    out.wave_hash = std::move(init.wave_hash);
    out.resolved_record_type = std::move(init.resolved_record_type);
    out.resolved_sampler = std::move(init.resolved_sampler);
    emit_runtime_binding_trace_event(
        {.phase = "run_end",
         .campaign_hash = out.campaign_hash,
         .binding_id = out.binding_id,
         .contract_hash = out.contract_hash,
         .wave_hash = out.wave_hash,
         .resolved_record_type = out.resolved_record_type,
         .resolved_sampler = out.resolved_sampler,
         .contract_count = 0,
         .ok = false,
         .error = out.error});
    log_err("[runtime_binding.run] init failed campaign_hash=%s binding=%s "
            "error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }

  out.contract_hash = std::move(init.contract_hash);
  out.wave_hash = std::move(init.wave_hash);
  out.resolved_record_type = std::move(init.resolved_record_type);
  out.resolved_sampler = std::move(init.resolved_sampler);
  out.runtime_binding = std::move(init.runtime_binding);
  out.contract_steps.reserve(out.runtime_binding.contracts.size());
  out.run_ids.reserve(out.runtime_binding.contracts.size());
  emit_runtime_binding_trace_event(
      {.phase = "run_start",
       .campaign_hash = out.campaign_hash,
       .binding_id = out.binding_id,
       .contract_hash = out.contract_hash,
       .wave_hash = out.wave_hash,
       .resolved_record_type = out.resolved_record_type,
       .resolved_sampler = out.resolved_sampler,
       .contract_count =
           static_cast<std::uint64_t>(out.runtime_binding.contracts.size()),
       .ok = true});
  log_dbg("[runtime_binding.run] start campaign_hash=%s binding=%s "
           "contracts=%zu record_type=%s sampler=%s\n",
           runtime_binding_log_value_or_empty(out.campaign_hash),
           runtime_binding_log_value_or_empty(out.binding_id),
           out.runtime_binding.contracts.size(),
           runtime_binding_log_value_or_empty(out.resolved_record_type),
           runtime_binding_log_value_or_empty(out.resolved_sampler));

  // Execution phase: contracts are evaluated in runtime-binding declaration
  // order.
  for (std::size_t i = 0; i < out.runtime_binding.contracts.size(); ++i) {
    auto &contract = out.runtime_binding.contracts[i];
    log_dbg("[runtime_binding.run] contract[%zu] begin name=%s epochs=%llu\n",
            i, runtime_binding_log_value_or_empty(contract.name),
            static_cast<unsigned long long>(
                std::max<std::uint64_t>(1, contract.execution.epochs)));
    RuntimeContext ctx{};
    ctx.wave_mode_flags = contract.execution.wave_mode_flags;
    ctx.debug_enabled = contract.execution.debug_enabled;
    ctx.binding_id = out.binding_id;
    ctx.run_id = make_runtime_binding_contract_run_id(out, i);
    ctx.source_runtime_cursor = contract.spec.source_runtime_cursor;
    runtime_binding_trace_context_t trace_ctx{
        .campaign_hash = out.campaign_hash,
        .binding_id = out.binding_id,
        .contract_hash = contract.spec.contract_hash,
        .wave_hash = out.wave_hash,
        .contract_name = contract.name,
        .contract_index = static_cast<std::uint64_t>(i),
        .contract_count =
            static_cast<std::uint64_t>(out.runtime_binding.contracts.size()),
        .epochs = std::max<std::uint64_t>(1, contract.execution.epochs)};
    ctx.user = &trace_ctx;
    out.run_ids.push_back(ctx.run_id);
    emit_runtime_binding_trace_event(
        {.phase = "contract_start",
         .campaign_hash = trace_ctx.campaign_hash,
         .binding_id = trace_ctx.binding_id,
         .contract_hash = trace_ctx.contract_hash,
         .wave_hash = trace_ctx.wave_hash,
         .contract_name = trace_ctx.contract_name,
         .run_id = ctx.run_id,
         .source_runtime_cursor = ctx.source_runtime_cursor,
         .contract_index = trace_ctx.contract_index,
         .contract_count = trace_ctx.contract_count,
         .epochs = trace_ctx.epochs,
         .ok = true});
    std::string run_error;
    const std::uint64_t steps =
        run_runtime_binding_contract(contract, ctx, &run_error);
    if (!run_error.empty()) {
      out.error = "run_contract failed for contract[" + std::to_string(i) +
                  "]: " + run_error;
      emit_runtime_binding_trace_event(
          {.phase = "contract_end",
           .campaign_hash = trace_ctx.campaign_hash,
           .binding_id = trace_ctx.binding_id,
           .contract_hash = trace_ctx.contract_hash,
           .wave_hash = trace_ctx.wave_hash,
           .contract_name = trace_ctx.contract_name,
           .run_id = ctx.run_id,
           .source_runtime_cursor = ctx.source_runtime_cursor,
           .contract_index = trace_ctx.contract_index,
           .contract_count = trace_ctx.contract_count,
           .epochs = trace_ctx.epochs,
           .total_steps = steps,
           .ok = false,
           .error = out.error});
      emit_runtime_binding_trace_event(
          {.phase = "run_end",
           .campaign_hash = out.campaign_hash,
           .binding_id = out.binding_id,
           .contract_hash = out.contract_hash,
           .wave_hash = out.wave_hash,
           .resolved_record_type = out.resolved_record_type,
           .resolved_sampler = out.resolved_sampler,
           .contract_index = trace_ctx.contract_index,
           .contract_count = trace_ctx.contract_count,
           .total_steps = out.total_steps,
           .ok = false,
           .error = out.error});
      log_err("[runtime_binding.run] contract[%zu] failed name=%s error=%s\n",
              i, runtime_binding_log_value_or_empty(contract.name),
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    out.contract_steps.push_back(steps);
    out.total_steps += steps;
    emit_runtime_binding_trace_event(
        {.phase = "contract_end",
         .campaign_hash = trace_ctx.campaign_hash,
         .binding_id = trace_ctx.binding_id,
         .contract_hash = trace_ctx.contract_hash,
         .wave_hash = trace_ctx.wave_hash,
         .contract_name = trace_ctx.contract_name,
         .run_id = ctx.run_id,
         .source_runtime_cursor = ctx.source_runtime_cursor,
         .contract_index = trace_ctx.contract_index,
         .contract_count = trace_ctx.contract_count,
         .epochs = trace_ctx.epochs,
         .total_steps = out.total_steps,
         .ok = true});
    log_dbg("[runtime_binding.run] contract[%zu] done steps=%llu "
             "cumulative_steps=%llu\n",
             i, static_cast<unsigned long long>(steps),
             static_cast<unsigned long long>(out.total_steps));
  }

  // Finalization: publish completed run counters.
  out.ok = true;
  emit_runtime_binding_trace_event(
      {.phase = "run_end",
       .campaign_hash = out.campaign_hash,
       .binding_id = out.binding_id,
       .contract_hash = out.contract_hash,
       .wave_hash = out.wave_hash,
       .resolved_record_type = out.resolved_record_type,
       .resolved_sampler = out.resolved_sampler,
       .contract_count = static_cast<std::uint64_t>(out.contract_steps.size()),
       .total_steps = out.total_steps,
       .ok = true});
  log_dbg("[runtime_binding.run] done campaign_hash=%s binding=%s "
           "total_steps=%llu contracts=%zu\n",
           runtime_binding_log_value_or_empty(out.campaign_hash),
           runtime_binding_log_value_or_empty(out.binding_id),
           static_cast<unsigned long long>(out.total_steps),
           out.contract_steps.size());
  return out;
}
