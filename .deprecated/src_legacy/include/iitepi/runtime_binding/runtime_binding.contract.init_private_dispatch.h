template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  log_dbg("[runtime_binding.run] snapshot start campaign_hash=%s binding=%s\n",
           runtime_binding_log_value_or_empty(campaign_hash),
           runtime_binding_log_value_or_empty(binding_id));
  try {
    const auto preflight = resolve_runtime_binding_preflight_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself);
    std::vector<cuwacunu::hero::runtime_lock::scoped_lock_t> resource_locks{};
    runtime_binding_contract_init_record_t init{};
    init.campaign_hash = campaign_hash;
    init.binding_id = binding_id;
    init.source_config_path = runtime_binding_itself
                                  ? runtime_binding_itself->config_file_path
                                  : std::string{};
    init.contract_hash = preflight.contract_hash;
    init.wave_hash = preflight.wave_hash;
    init.resolved_record_type = preflight.resolved_record_type;
    init.resolved_sampler = preflight.resolved_sampler;
    if (!preflight.ok) {
      init.error = preflight.error;
    } else {
      std::string lock_error{};
      if (!acquire_runtime_binding_lock_plan(preflight.lock_plan,
                                             &resource_locks, &lock_error)) {
        init.error = lock_error;
      } else {
        init = invoke_runtime_binding_contract_init_from_snapshot<Datatype_t,
                                                                  Sampler_t>(
            campaign_hash, binding_id, runtime_binding_itself, device);
        init.resolved_record_type = preflight.resolved_record_type;
        init.resolved_sampler = preflight.resolved_sampler;
      }
    }
    auto out = run_initialized_runtime_binding(std::move(init));
    if (!out.ok) {
      log_err("[runtime_binding.run] snapshot failed campaign_hash=%s "
              "binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(out.campaign_hash),
              runtime_binding_log_value_or_empty(out.binding_id),
              runtime_binding_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception &e) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself)
      out.source_config_path = runtime_binding_itself->config_file_path;
    out.error = std::string(kRuntimeBindingRunCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.run] snapshot exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself)
      out.source_config_path = runtime_binding_itself->config_file_path;
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] snapshot exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  log_dbg("[runtime_binding.run] snapshot start campaign_hash=%s binding=%s\n",
           runtime_binding_log_value_or_empty(campaign_hash),
           runtime_binding_log_value_or_empty(binding_id));
  try {
    const auto preflight = resolve_runtime_binding_preflight_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself);
    std::vector<cuwacunu::hero::runtime_lock::scoped_lock_t> resource_locks{};
    runtime_binding_contract_init_record_t init{};
    init.campaign_hash = campaign_hash;
    init.binding_id = binding_id;
    init.source_config_path = runtime_binding_itself
                                  ? runtime_binding_itself->config_file_path
                                  : std::string{};
    init.contract_hash = preflight.contract_hash;
    init.wave_hash = preflight.wave_hash;
    init.resolved_record_type = preflight.resolved_record_type;
    init.resolved_sampler = preflight.resolved_sampler;
    if (!preflight.ok) {
      init.error = preflight.error;
    } else {
      std::string lock_error{};
      if (!acquire_runtime_binding_lock_plan(preflight.lock_plan,
                                             &resource_locks, &lock_error)) {
        init.error = lock_error;
      } else {
        init = invoke_runtime_binding_contract_init_from_preflight(
            preflight, runtime_binding_itself, device);
      }
    }
    auto out = run_initialized_runtime_binding(std::move(init));
    if (!out.ok) {
      log_err("[runtime_binding.run] snapshot failed campaign_hash=%s "
              "binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(out.campaign_hash),
              runtime_binding_log_value_or_empty(out.binding_id),
              runtime_binding_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception &e) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself)
      out.source_config_path = runtime_binding_itself->config_file_path;
    out.error = std::string(kRuntimeBindingRunCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.run] snapshot exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself)
      out.source_config_path = runtime_binding_itself->config_file_path;
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] snapshot exception campaign_hash=%s "
            "binding=%s error=%s\n",
            runtime_binding_log_value_or_empty(out.campaign_hash),
            runtime_binding_log_value_or_empty(out.binding_id),
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t
run_runtime_binding(const std::string &binding_id,
                    torch::Device device = torch::kCPU) {
  runtime_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string normalized_binding_id =
        runtime_binding_init_trim_ascii_copy(binding_id);
    if (!has_non_ws_text(normalized_binding_id)) {
      out.error = "run_runtime_binding requires non-empty binding_id";
      log_err("[runtime_binding.run] invalid request error=%s\n",
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::
        locked_runtime_binding_hash();
    log_dbg("[runtime_binding.run] dispatch campaign_hash=%s binding=%s\n",
             runtime_binding_log_value_or_empty(campaign_hash),
             runtime_binding_log_value_or_empty(normalized_binding_id));
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    return run_runtime_binding_snapshot<Datatype_t, Sampler_t>(
        campaign_hash, normalized_binding_id, runtime_binding_itself, device);
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingRunCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_run_record_t
run_runtime_binding(const std::string &binding_id,
                    torch::Device device = torch::kCPU) {
  runtime_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string normalized_binding_id =
        runtime_binding_init_trim_ascii_copy(binding_id);
    if (!has_non_ws_text(normalized_binding_id)) {
      out.error = "run_runtime_binding requires non-empty binding_id";
      log_err("[runtime_binding.run] invalid request error=%s\n",
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::
        locked_runtime_binding_hash();
    log_dbg("[runtime_binding.run] dispatch campaign_hash=%s binding=%s\n",
             runtime_binding_log_value_or_empty(campaign_hash),
             runtime_binding_log_value_or_empty(normalized_binding_id));
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    return run_runtime_binding_snapshot(campaign_hash, normalized_binding_id,
                                        runtime_binding_itself, device);
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingRunCanonicalAction) +
                " exception: " + e.what();
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_file(
    const std::string &runtime_binding_file_path, const std::string &binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};

  try {
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::
        register_runtime_binding_file(runtime_binding_file_path);
    cuwacunu::iitepi::runtime_binding_space_t::assert_intact_or_fail_fast(
        campaign_hash);
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    return invoke_runtime_binding_contract_init_from_snapshot<Datatype_t,
                                                              Sampler_t>(
        campaign_hash, binding_id, runtime_binding_itself, device);
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: unknown";
    return out;
  }
}

[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_file(
    const std::string &runtime_binding_file_path, const std::string &binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};

  try {
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::
        register_runtime_binding_file(runtime_binding_file_path);
    cuwacunu::iitepi::runtime_binding_space_t::assert_intact_or_fail_fast(
        campaign_hash);
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    return invoke_runtime_binding_contract_init_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself, device);
  } catch (const std::exception &e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) +
                " exception: unknown";
    return out;
  }
}

} // namespace tsiemene

namespace cuwacunu::iitepi {

using runtime_binding_contract_init_record_t =
    ::tsiemene::runtime_binding_contract_init_record_t;
using runtime_binding_run_record_t = ::tsiemene::runtime_binding_run_record_t;

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t
run_runtime_binding(const std::string &binding_id,
                    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_runtime_binding<Datatype_t, Sampler_t>(binding_id,
                                                                device);
}

[[nodiscard]] inline runtime_binding_run_record_t
run_runtime_binding(const std::string &binding_id,
                    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_runtime_binding(binding_id, device);
}

} // namespace cuwacunu::iitepi
