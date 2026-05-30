inline bool
load_contract_wikimyei_report_fragments(RuntimeBindingContract &c,
                                        const RuntimeContext &ctx,
                                        std::string *error = nullptr);
inline bool
save_contract_wikimyei_report_fragments(RuntimeBindingContract &c,
                                        const RuntimeContext &ctx,
                                        std::string *error = nullptr);

class runtime_binding_null_emitter_t final : public Emitter {
public:
  void emit(const Wave &, DirectiveId, Signal) override {}
};

inline void broadcast_contract_runtime_event(RuntimeBindingContract &c,
                                             RuntimeEventKind kind,
                                             const Wave *wave,
                                             RuntimeContext &ctx) {
  runtime_binding_null_emitter_t emitter{};
  for (auto &node : c.nodes) {
    if (!node)
      continue;
    RuntimeEvent event{};
    event.kind = kind;
    event.wave = wave;
    event.source = node.get();
    event.target = node.get();
    (void)node->on_event(event, ctx, emitter);
  }
}

[[nodiscard]] inline bool
validate_contract_component_dsl_fingerprints(const RuntimeBindingContract &c,
                                             std::string *error = nullptr) {
  if (error)
    error->clear();
  for (const auto &fp : c.spec.component_dsl_fingerprints) {
    if (fp.tsi_dsl_path.empty() && fp.tsi_dsl_sha256_hex.empty())
      continue;
    if (fp.tsi_dsl_path.empty()) {
      if (error) {
        *error = "component DSL fingerprint missing path for canonical_path='" +
                 fp.canonical_path + "'";
      }
      return false;
    }
    const std::string current_sha256 =
        cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(
            fp.tsi_dsl_path);
    if (current_sha256.empty()) {
      if (error) {
        *error = "component DSL fingerprint file is missing/unreadable path='" +
                 fp.tsi_dsl_path + "'";
      }
      return false;
    }
    if (!fp.tsi_dsl_sha256_hex.empty() &&
        current_sha256 != fp.tsi_dsl_sha256_hex) {
      if (error) {
        *error = "component DSL drift detected canonical_path='" +
                 fp.canonical_path + "' path='" + fp.tsi_dsl_path +
                 "' expected_sha256='" + fp.tsi_dsl_sha256_hex +
                 "' actual_sha256='" + current_sha256 + "'";
      }
      return false;
    }
  }
  return true;
}

inline std::uint64_t run_runtime_binding_circuit(RuntimeBindingContract &c,
                                                 RuntimeContext &ctx,
                                                 std::string *error = nullptr) {
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
  CircuitIssue issue{};
  if (!c.ensure_compiled(&issue)) {
    if (error) {
      *error = std::string("compile_circuit failed: ");
      error->append(issue.what.data(), issue.what.size());
    }
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
    return 0;
  }
  broadcast_contract_runtime_event(c, RuntimeEventKind::RunStart, &c.seed_wave,
                                   ctx);
  broadcast_contract_runtime_event(c, RuntimeEventKind::EpochStart,
                                   &c.seed_wave, ctx);
  const std::uint64_t steps =
      run_wave_compiled(c.compiled_runtime, c.seed_wave, c.seed_ingress, ctx,
                        c.execution.runtime);
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error)
      *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &c.seed_wave,
                                     ctx);
    return 0;
  }
  broadcast_contract_runtime_event(c, RuntimeEventKind::EpochEnd, &c.seed_wave,
                                   ctx);
  if (!save_contract_wikimyei_report_fragments(c, ctx,
                                               &report_fragment_error)) {
    if (error)
      *error = report_fragment_error;
    broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &c.seed_wave,
                                     ctx);
    return 0;
  }
  broadcast_contract_runtime_event(c, RuntimeEventKind::RunEnd, &c.seed_wave,
                                   ctx);
  return steps;
}

[[nodiscard]] inline Wave wave_for_epoch(const Wave &seed,
                                         std::uint64_t epoch_index) {
  Wave out = seed;
  if (epoch_index >
      (std::numeric_limits<std::uint64_t>::max() - seed.cursor.episode)) {
    out.cursor.episode = std::numeric_limits<std::uint64_t>::max();
  } else {
    out.cursor.episode = seed.cursor.episode + epoch_index;
  }
  return out;
}
