inline std::uint64_t
run_runtime_binding_contract(RuntimeBindingContract &c, RuntimeContext &ctx,
                             std::string *error = nullptr) {
  // Contract runtime initialization: compile + report_fragment preload.
  if (error)
    error->clear();
  ctx.wave_mode_flags = c.execution.wave_mode_flags;
  ctx.debug_enabled = c.execution.debug_enabled;
  if (ctx.run_id.empty())
    ctx.run_id = make_runtime_run_id(c);
  if (ctx.source_runtime_cursor.empty()) {
    ctx.source_runtime_cursor = c.spec.source_runtime_cursor;
  }
  (void)ensure_runtime_context_wave_cursor(&ctx);
  log_dbg("[runtime_binding.contract.run] start contract=%s epochs=%llu\n",
           c.name.empty() ? "<empty>" : c.name.c_str(),
           static_cast<unsigned long long>(
               std::max<std::uint64_t>(1, c.execution.epochs)));
  CircuitIssue issue{};
  if (!c.ensure_compiled(&issue)) {
    if (error) {
      *error = std::string("compile_circuit failed: ");
      error->append(issue.what.data(), issue.what.size());
    }
    log_err(
        "[runtime_binding.contract.run] compile failed contract=%s error=%s\n",
        c.name.empty() ? "<empty>" : c.name.c_str(),
        error && !error->empty() ? error->c_str() : "<empty>");
    return 0;
  }
  std::string dsl_guard_error;
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error)
      *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    return 0;
  }
  std::string report_fragment_error;
  if (!load_contract_wikimyei_report_fragments(c, ctx,
                                               &report_fragment_error)) {
    if (error)
      *error = report_fragment_error;
    log_err(
        "[runtime_binding.contract.run] preload failed contract=%s error=%s\n",
        c.name.empty() ? "<empty>" : c.name.c_str(),
        report_fragment_error.empty() ? "<empty>"
                                      : report_fragment_error.c_str());
    return 0;
  }
  broadcast_contract_runtime_event(c, RuntimeEventKind::RunStart, &c.seed_wave,
                                   ctx);
  Wave run_end_wave = c.seed_wave;

  // Epoch execution + callbacks.
  const std::uint64_t epochs = std::max<std::uint64_t>(1, c.execution.epochs);
  std::uint64_t total_steps = 0;
  const auto *trace_ctx =
      static_cast<const runtime_binding_trace_context_t *>(ctx.user);
  for (std::uint64_t epoch = 0; epoch < epochs; ++epoch) {
    if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
      if (error)
        *error = dsl_guard_error;
      log_warn("[runtime_binding.contract.run] abort contract=%s epoch=%llu "
               "reason=%s\n",
               c.name.empty() ? "<empty>" : c.name.c_str(),
               static_cast<unsigned long long>(epoch + 1),
               dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
      broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd,
                                       &run_end_wave, ctx);
      return 0;
    }
    const Wave start_wave = wave_for_epoch(c.seed_wave, epoch);
    run_end_wave = start_wave;
    if (trace_ctx) {
      emit_runtime_binding_trace_event(
          {.phase = "epoch_start",
           .campaign_hash = trace_ctx->campaign_hash,
           .binding_id = trace_ctx->binding_id,
           .contract_hash = trace_ctx->contract_hash,
           .wave_hash = trace_ctx->wave_hash,
           .contract_name = trace_ctx->contract_name,
           .run_id = ctx.run_id,
           .source_runtime_cursor = ctx.source_runtime_cursor,
           .contract_index = trace_ctx->contract_index,
           .contract_count = trace_ctx->contract_count,
           .epochs = epochs,
           .epoch = epoch + 1,
           .total_steps = total_steps,
           .ok = true});
    }
    broadcast_contract_runtime_event(c, RuntimeEventKind::EpochStart,
                                     &start_wave, ctx);
    const std::uint64_t epoch_steps =
        run_wave_compiled(c.compiled_runtime, start_wave, c.seed_ingress, ctx,
                          c.execution.runtime);
    if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
      if (error)
        *error = dsl_guard_error;
      log_warn("[runtime_binding.contract.run] abort contract=%s epoch=%llu "
               "reason=%s\n",
               c.name.empty() ? "<empty>" : c.name.c_str(),
               static_cast<unsigned long long>(epoch + 1),
               dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
      broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd,
                                       &run_end_wave, ctx);
      return 0;
    }
    total_steps += epoch_steps;
    if (trace_ctx) {
      emit_runtime_binding_trace_event(
          {.phase = "epoch_end",
           .campaign_hash = trace_ctx->campaign_hash,
           .binding_id = trace_ctx->binding_id,
           .contract_hash = trace_ctx->contract_hash,
           .wave_hash = trace_ctx->wave_hash,
           .contract_name = trace_ctx->contract_name,
           .run_id = ctx.run_id,
           .source_runtime_cursor = ctx.source_runtime_cursor,
           .contract_index = trace_ctx->contract_index,
           .contract_count = trace_ctx->contract_count,
           .epochs = epochs,
           .epoch = epoch + 1,
           .epoch_steps = epoch_steps,
           .total_steps = total_steps,
           .ok = true});
    }
    log_dbg("[runtime_binding.contract.run] epoch done contract=%s "
             "epoch=%llu/%llu steps=%llu cumulative=%llu\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             static_cast<unsigned long long>(epoch + 1),
             static_cast<unsigned long long>(epochs),
             static_cast<unsigned long long>(epoch_steps),
             static_cast<unsigned long long>(total_steps));
    broadcast_contract_runtime_event(c, RuntimeEventKind::EpochEnd, &start_wave,
                                     ctx);
  }

  // Finalization: persist autosave report fragments after the execution loop.
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error)
      *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &run_end_wave,
                                     ctx);
    return 0;
  }
  if (!save_contract_wikimyei_report_fragments(c, ctx,
                                               &report_fragment_error)) {
    if (error)
      *error = report_fragment_error;
    log_err(
        "[runtime_binding.contract.run] finalize failed contract=%s error=%s\n",
        c.name.empty() ? "<empty>" : c.name.c_str(),
        report_fragment_error.empty() ? "<empty>"
                                      : report_fragment_error.c_str());
    broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &run_end_wave,
                                     ctx);
    return 0;
  }
  broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &run_end_wave,
                                   ctx);
  log_dbg("[runtime_binding.contract.run] done contract=%s total_steps=%llu\n",
           c.name.empty() ? "<empty>" : c.name.c_str(),
           static_cast<unsigned long long>(total_steps));
  return total_steps;
}

} // namespace tsiemene
