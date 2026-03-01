/* expected_value.cpp */
/**
 * @file expected_value.cpp
 * @brief ExpectedValue: non-template implementation with safe checkpointing.
 */

#include "wikimyei/inference/expected_value/expected_value.h"

#include <cmath>
#include <utility>
#include <cstdio>   // std::remove, std::rename
#include <cstring>
#include <filesystem>

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .cpp
// ------------------------------------------------------------
RUNTIME_WARNING("[expected_value.cpp] select_targets uses from_blob()+clone for indices (tiny extra alloc, safe).\n");
RUNTIME_WARNING("[expected_value.cpp] Channel EMA weights use 1/(ema+eps) with clamp_max to limit volatility.\n");
RUNTIME_WARNING("[expected_value.cpp] Optimizer state is skipped on CUDA during save; loader tolerates its absence.\n");
RUNTIME_WARNING("[expected_value.cpp] Checkpoint uses SAFE state-dict (params/buffers only); avoids JIT pickler & undefined buffers.\n");
RUNTIME_WARNING("[expected_value.cpp] Checkpoint save writes to .tmp and requires successful rename to final path.\n");

namespace cuwacunu {
namespace wikimyei {

// -------------------- safe state-dict helpers ----------------
namespace {
constexpr int64_t kExpectedValueCheckpointFormatVersion = 2;

// write CPU clone of a tensor under key
inline void ev_write_tensor(torch::serialize::OutputArchive& ar,
                            const std::string& key, const torch::Tensor& t) {
  ar.write(key, t.detach().to(torch::kCPU));
}

// try read tensor; return true on success
inline bool ev_try_read_tensor(torch::serialize::InputArchive& ar,
                               const std::string& key, torch::Tensor& out) {
  try { ar.read(key, out); return out.defined(); } catch (...) { return false; }
}

inline void ev_write_string(torch::serialize::OutputArchive& ar,
                            const std::string& key,
                            const std::string& value) {
  auto t = torch::empty({static_cast<int64_t>(value.size())}, torch::kChar);
  if (t.numel() > 0) {
    std::memcpy(t.data_ptr<int8_t>(), value.data(), value.size());
  }
  ar.write(key, t);
}

inline bool ev_try_read_string(torch::serialize::InputArchive& ar,
                               const std::string& key,
                               std::string& out) {
  torch::Tensor t;
  if (!ev_try_read_tensor(ar, key, t)) return false;
  t = t.to(torch::kCPU);
  TORCH_CHECK(t.dim() == 1, "[ExpectedValue::ckpt] key '", key, "' must be a 1-D byte tensor.");
  out.resize(static_cast<size_t>(t.numel()));
  if (t.numel() > 0) {
    std::memcpy(out.data(), t.data_ptr<int8_t>(), static_cast<size_t>(t.numel()));
  }
  return true;
}

inline int64_t ev_require_scalar_i64(torch::serialize::InputArchive& ar,
                                     const std::string& key) {
  torch::Tensor t;
  TORCH_CHECK(ev_try_read_tensor(ar, key, t), "[ExpectedValue::ckpt] missing required key '", key, "'.");
  t = t.to(torch::kCPU);
  TORCH_CHECK(t.numel() >= 1, "[ExpectedValue::ckpt] key '", key, "' must contain a scalar.");
  return t.item<int64_t>();
}

inline std::string ev_require_string(torch::serialize::InputArchive& ar,
                                     const std::string& key) {
  std::string value;
  TORCH_CHECK(ev_try_read_string(ar, key, value), "[ExpectedValue::ckpt] missing required key '", key, "'.");
  return value;
}

// Save only defined params/buffers (CPU clones), no JIT/module pickling.
inline void ev_save_module_state(torch::serialize::OutputArchive& ar,
                                 const torch::nn::Module& m,
                                 const std::string& base) {
  // parameters
  for (const auto& np : m.named_parameters(/*recurse=*/true)) {
    ev_write_tensor(ar, base + "/param/" + np.key(), np.value());
  }
  // buffers (skip undefined)
  for (const auto& nb : m.named_buffers(/*recurse=*/true)) {
    if (nb.value().defined()) {
      ev_write_tensor(ar, base + "/buffer/" + nb.key(), nb.value());
    } else {
      log_warn("[ExpectedValue::ckpt] skipping undefined buffer '%s'.\n", nb.key().c_str());
    }
  }
}

// Load by name; if missing, warn and continue.
inline void ev_load_module_state(torch::serialize::InputArchive& ar,
                                 torch::nn::Module& m,
                                 const std::string& base) {
  torch::NoGradGuard _ng;
  // parameters
  for (auto& np : m.named_parameters(/*recurse=*/true)) {
    const auto& name = np.key();
    auto&       p    = np.value();
    torch::Tensor t;
    if (ev_try_read_tensor(ar, base + "/param/" + name, t)) {
      p.detach().to(p.device(), p.dtype()).copy_(t.to(p.device(), p.dtype()));
    } else {
      log_warn("[ExpectedValue::ckpt] missing param '%s' in checkpoint; keeping current.\n", name.c_str());
    }
  }
  // buffers: copy only if buffer exists & defined
  for (auto& nb : m.named_buffers(/*recurse=*/true)) {
    const auto& name = nb.key();
    auto&       b    = nb.value();
    torch::Tensor t;
    if (ev_try_read_tensor(ar, base + "/buffer/" + name, t)) {
      if (b.defined()) {
        b.detach().to(b.device(), b.dtype()).copy_(t.to(b.device(), b.dtype()));
      } else {
        // Buffer not present or runtime-only; skip silently
      }
    }
  }
}
} // anonymous namespace

// ---------- ctor ----------
ExpectedValue::ExpectedValue(
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash_,
    const std::string& component_name_)
: contract_hash(contract_hash_),
  component_name(component_name_) {
  // Static weights and target dims from the bound contract snapshot.
  auto observation_instruction =
      cuwacunu::camahjucunu::decode_observation_spec_from_contract(
          contract_hash);
  static_channel_weights_     = observation_instruction.retrieve_channel_weights();
  static_feature_weights_     = cuwacunu::piaabo::dconfig::contract_space_t::get_arr<float>(contract_hash, "VALUE_ESTIMATION", "target_weights");
  grad_clip                   = cuwacunu::piaabo::dconfig::contract_space_t::get<double>(contract_hash, "VALUE_ESTIMATION", "grad_clip");
  optimizer_threshold_reset   = cuwacunu::piaabo::dconfig::contract_space_t::get<int>(contract_hash, "VALUE_ESTIMATION", "optimizer_threshold_reset");
  target_dims_                = cuwacunu::piaabo::dconfig::contract_space_t::get_arr<int64_t>(contract_hash, "VALUE_ESTIMATION", "target_dims");

  // Model
  semantic_model = cuwacunu::wikimyei::mdn::MdnModel(
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(contract_hash, "VICReg", "encoding_dims"),               // De
    static_cast<int64_t>(target_dims_.size()),                                                    // Dy
    observation_instruction.count_channels(),                                                      // C
    observation_instruction.max_future_sequence_length(),                                          // Hf
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(contract_hash, "VALUE_ESTIMATION", "mixture_comps"),     // K
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(contract_hash, "VALUE_ESTIMATION", "features_hidden"),   // H
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(contract_hash, "VALUE_ESTIMATION", "residual_depth"),    // depth
    cuwacunu::piaabo::dconfig::config_dtype(contract_hash, "VALUE_ESTIMATION"),
    cuwacunu::piaabo::dconfig::config_device(contract_hash, "VALUE_ESTIMATION"),
    /* display_model */ false
  );

  // Params
  trainable_params_.reserve(64);
  for (auto& p : semantic_model->parameters(/*recurse=*/true)) {
    if (p.requires_grad()) trainable_params_.push_back(p);
  }

  // Builders
  TORCH_CHECK(cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_builder,   "[ExpectedValue](ctor) opt_builder is null");
  TORCH_CHECK(cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sched_builder, "[ExpectedValue](ctor) sched_builder is null");

  optimizer = cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_builder->build(trainable_params_);
  lr_sched  = cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sched_builder->build(*optimizer);
  loss_obj  = std::make_unique<cuwacunu::wikimyei::mdn::MdnNLLLoss>(cuwacunu::jkimyei::jk_setup(component_name, contract_hash));

  display_model(/* display_semantic */ true);
}

// ---------- helpers: targets & weights ----------
torch::Tensor
ExpectedValue::select_targets(const torch::Tensor& future_features,
                              const std::vector<int64_t>& target_dims)
{
  TORCH_CHECK(future_features.defined(), "[ExpectedValue::select_targets] future_features undefined");
  TORCH_CHECK(future_features.dim() == 4, "[ExpectedValue::select_targets] expecting [B,C,Hf,D]");
  const auto B  = future_features.size(0);
  const auto C  = future_features.size(1);
  const auto Hf = future_features.size(2);
  const auto D  = future_features.size(3);
  TORCH_CHECK(!target_dims.empty(), "[ExpectedValue::select_targets] empty target_dims");
  for (auto d : target_dims) {
    TORCH_CHECK(d >= 0 && d < D, "[ExpectedValue::select_targets] target dim out of range");
  }
  const auto Dy = static_cast<int64_t>(target_dims.size());
  auto idx = torch::tensor(target_dims, torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU)).to(future_features.device());

  auto flat = future_features.reshape({B*C*Hf, D});
  auto idx2 = idx.unsqueeze(0).expand({B*C*Hf, Dy});
  auto y_sel = flat.gather(/*dim=*/1, idx2);
  return y_sel.view({B, C, Hf, Dy});
}

torch::Tensor
ExpectedValue::build_horizon_weights(int64_t Hf, torch::Device dev, torch::Dtype dt) const
{
  if (Hf <= 0) return torch::Tensor();
  std::vector<float> w(static_cast<size_t>(Hf), 1.f);
  switch (horizon_policy_) {
    case HorizonPolicy::Uniform: break;
    case HorizonPolicy::NearTerm: {
      float g = gamma_near_;
      for (size_t t = 0; t < w.size(); ++t) w[t] = std::pow(g, static_cast<float>(t));
      break;
    }
    case HorizonPolicy::VeryNearTerm: {
      float g = gamma_very_;
      for (size_t t = 0; t < w.size(); ++t) w[t] = std::pow(g, static_cast<float>(t));
      break;
    }
  }
  auto wt = torch::tensor(w, torch::TensorOptions().dtype(torch::kFloat32));
  if (wt.dtype() != dt) wt = wt.to(dt);
  return wt.to(dev);
}

torch::Tensor
ExpectedValue::build_channel_weights(int64_t C, torch::Device dev, torch::Dtype dt)
{
  if (C <= 0) return torch::Tensor();
  auto w = torch::ones({C}, torch::TensorOptions().dtype(dt).device(dev));

  if (!static_channel_weights_.empty()) {
    TORCH_CHECK(static_cast<int64_t>(static_channel_weights_.size()) == C,
                "[ExpectedValue] static_channel_weights size must equal C");
    auto ws = torch::tensor(static_channel_weights_, torch::TensorOptions().dtype(torch::kFloat32));
    if (ws.dtype() != dt) ws = ws.to(dt);
    w = w * ws.to(dev);
  }
  if (use_channel_ema_weights_) {
    auto w_ema = channel_weights_from_ema(C);
    if (w_ema.dtype() != dt) w_ema = w_ema.to(dt);
    w = w * w_ema;
  }
  return w;
}

torch::Tensor
ExpectedValue::build_feature_weights(int64_t Dy, torch::Device dev, torch::Dtype dt) const
{
  if (Dy <= 0) return torch::Tensor();
  if (!static_feature_weights_.empty()) {
    TORCH_CHECK(static_cast<int64_t>(static_feature_weights_.size()) == Dy,
                "[ExpectedValue] static_feature_weights size must equal Dy");
    auto wd = torch::tensor(static_feature_weights_, torch::TensorOptions().dtype(torch::kFloat32));
    if (wd.dtype() != dt) wd = wd.to(dt);
    return wd.to(dev);
  }
  return torch::ones({Dy}, torch::TensorOptions().dtype(dt).device(dev));
}

torch::Tensor
ExpectedValue::channel_weights_from_ema(int64_t C)
{
  if (!use_channel_ema_weights_) {
    return torch::ones({C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
  }
  if (!channel_ema_.defined() || channel_ema_.numel() != C) {
    channel_ema_ = torch::ones({C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
  }
  const float eps = 1e-6f;
  auto w = 1.0f / (channel_ema_ + eps);
  return w.clamp_max(10.0f); // limit volatility
}

void
ExpectedValue::update_channel_ema(const torch::Tensor& ch_mean_loss)
{
  if (!use_channel_ema_weights_) return;
  if (!channel_ema_.defined()) {
    channel_ema_ = ch_mean_loss.detach();
    return;
  }
  channel_ema_.mul_(ema_alpha_).add_(ch_mean_loss.detach() * (1.0 - ema_alpha_));
}

const char*
ExpectedValue::scheduler_mode_name(cuwacunu::jkimyei::LRSchedulerAny::Mode mode) {
  switch (mode) {
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch:
      return "PerBatch";
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch:
      return "PerEpoch";
    case cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric:
      return "PerEpochWithMetric";
  }
  return "Unknown";
}

// ==========================
// Checkpointing (SAFE, strict v2)
// ==========================
bool
ExpectedValue::save_checkpoint(const std::string& path) const {
  const std::string tmp = path + ".tmp";
  try {
    torch::serialize::OutputArchive ar;

    // --- model state (safe, tensor-by-tensor) ---
    ev_save_module_state(ar, *semantic_model, "model");

    // --- strict metadata ---
    ar.write("format_version", torch::tensor({kExpectedValueCheckpointFormatVersion},
                                             torch::TensorOptions().dtype(torch::kInt64)));
    ev_write_string(ar, "meta/contract_hash", contract_hash);
    ev_write_string(ar, "meta/component_name", component_name);
    ev_write_string(ar, "meta/scheduler_mode",
                    lr_sched ? scheduler_mode_name(lr_sched->mode) : std::string("None"));
    ar.write("meta/scheduler_batch_steps", torch::tensor({scheduler_batch_steps_},
                                                         torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/scheduler_epoch_steps", torch::tensor({scheduler_epoch_steps_},
                                                         torch::TensorOptions().dtype(torch::kInt64)));

    // --- optimizer (skip on CUDA, keep flag) ---
    int64_t wrote_opt = 0;
    if (optimizer) {
      if (!semantic_model->device.is_cuda()) {
        try {
          torch::serialize::OutputArchive oa;
          optimizer->save(oa);
          ar.write("optimizer", oa);
          wrote_opt = 1;
        } catch (const c10::Error& e) {
          log_warn("[ExpectedValue::ckpt] optimizer->save failed; skipping. Err=%s\n", e.what());
        }
      } else {
        log_warn("[ExpectedValue::ckpt] skipping optimizer state save (on CUDA).\n");
      }
    }
    ar.write("has_optimizer", torch::tensor({wrote_opt},
                                            torch::TensorOptions().dtype(torch::kInt64)));

    // --- scheduler state (optional native serialization) ---
    int64_t scheduler_serialized = 0;
    if (lr_sched) {
      torch::serialize::OutputArchive sch_ar;
      if (try_save_scheduler(*lr_sched, sch_ar)) {
        ar.write("scheduler", sch_ar);
        scheduler_serialized = 1;
      }
    }
    ar.write("scheduler_serialized", torch::tensor({scheduler_serialized},
                                                   torch::TensorOptions().dtype(torch::kInt64)));

    // --- scalars ---
    ar.write("best_metric",          torch::tensor({best_metric_}));
    ar.write("best_epoch",           torch::tensor({int64_t(best_epoch_)},
                                                   torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("total_iters_trained",  torch::tensor({int64_t(total_iters_trained_)},
                                                   torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("total_epochs_trained", torch::tensor({int64_t(total_epochs_trained_)},
                                                   torch::TensorOptions().dtype(torch::kInt64)));

    // --- telemetry (write ONLY if defined; no placeholders) ---
    if (channel_ema_.defined())          ar.write("channel_ema",          channel_ema_.detach().to(torch::kCPU));
    if (last_per_channel_nll_.defined()) ar.write("last_per_channel_nll", last_per_channel_nll_.detach().to(torch::kCPU));
    if (last_per_horizon_nll_.defined()) ar.write("last_per_horizon_nll", last_per_horizon_nll_.detach().to(torch::kCPU));

    // --- strict atomic write: write tmp, verify, rename, verify ---
    ar.save_to(tmp);
    std::error_code ec;
    if (!std::filesystem::exists(tmp, ec) || ec) {
      log_err("[ExpectedValue::ckpt] save failed: temporary checkpoint was not created (%s)\n",
              tmp.c_str());
      std::remove(tmp.c_str());
      return false;
    }
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
      log_err("[ExpectedValue::ckpt] save failed: rename(%s -> %s) failed: %s\n",
              tmp.c_str(), path.c_str(), ec.message().c_str());
      std::remove(tmp.c_str());
      return false;
    }
    if (!std::filesystem::exists(path, ec) || ec) {
      log_err("[ExpectedValue::ckpt] save failed: final checkpoint path missing after rename (%s)\n",
              path.c_str());
      return false;
    }

    log_info("%s[ExpectedValue::ckpt]%s saved → %s\n",
             ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET, path.c_str());
    return true;

  } catch (const c10::Error& e) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed: %s\n", e.what());
    return false;
  } catch (const std::exception& e) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed: %s\n", e.what());
    return false;
  } catch (...) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed (unknown exception)\n");
    return false;
  }
}

bool
ExpectedValue::load_checkpoint(const std::string& path) {
  try {
    torch::serialize::InputArchive ar;
    ar.load_from(path.c_str());

    const int64_t format_version = ev_require_scalar_i64(ar, "format_version");
    TORCH_CHECK(format_version == kExpectedValueCheckpointFormatVersion,
                "[ExpectedValue::ckpt] unsupported checkpoint format_version=",
                format_version, " (expected ",
                kExpectedValueCheckpointFormatVersion, ").");

    const std::string saved_contract_hash  = ev_require_string(ar, "meta/contract_hash");
    const std::string saved_component_name = ev_require_string(ar, "meta/component_name");
    const std::string saved_scheduler_mode = ev_require_string(ar, "meta/scheduler_mode");
    const int64_t saved_scheduler_batch_steps = ev_require_scalar_i64(ar, "meta/scheduler_batch_steps");
    const int64_t saved_scheduler_epoch_steps = ev_require_scalar_i64(ar, "meta/scheduler_epoch_steps");

    TORCH_CHECK(!saved_contract_hash.empty(),
                "[ExpectedValue::ckpt] meta/contract_hash must be non-empty.");
    TORCH_CHECK(!saved_component_name.empty(),
                "[ExpectedValue::ckpt] meta/component_name must be non-empty.");
    TORCH_CHECK(!saved_scheduler_mode.empty(),
                "[ExpectedValue::ckpt] meta/scheduler_mode must be non-empty.");
    TORCH_CHECK(saved_scheduler_batch_steps >= 0,
                "[ExpectedValue::ckpt] meta/scheduler_batch_steps must be >= 0.");
    TORCH_CHECK(saved_scheduler_epoch_steps >= 0,
                "[ExpectedValue::ckpt] meta/scheduler_epoch_steps must be >= 0.");
    TORCH_CHECK(saved_contract_hash == contract_hash,
                "[ExpectedValue::ckpt] contract hash mismatch (ckpt='", saved_contract_hash,
                "', runtime='", contract_hash, "').");
    TORCH_CHECK(saved_component_name == component_name,
                "[ExpectedValue::ckpt] component mismatch (ckpt='", saved_component_name,
                "', runtime='", component_name, "').");

    const std::string runtime_scheduler_mode =
        lr_sched ? scheduler_mode_name(lr_sched->mode) : "None";
    TORCH_CHECK(saved_scheduler_mode == runtime_scheduler_mode,
                "[ExpectedValue::ckpt] scheduler mode mismatch (ckpt='", saved_scheduler_mode,
                "', runtime='", runtime_scheduler_mode, "').");

    // --- model state (safe, tensor-by-tensor) ---
    ev_load_module_state(ar, *semantic_model, "model");
    semantic_model->to(semantic_model->device, semantic_model->dtype, /*non_blocking=*/false);

    // --- optimizer (optional) ---
    const bool expect_opt = (ar_try_read_scalar_int64(ar, "has_optimizer", 0) != 0);
    if (optimizer && expect_opt) {
      torch::serialize::InputArchive oa;
      bool has_optimizer_state = true;
      try {
        ar.read("optimizer", oa);
      } catch (...) {
        has_optimizer_state = false;
      }
      TORCH_CHECK(has_optimizer_state,
                  "[ExpectedValue::ckpt] optimizer state declared in checkpoint but missing.");
      optimizer->load(oa);
    }

    // --- scalars ---
    best_metric_          = ar_try_read_scalar_double(ar, "best_metric", best_metric_);
    best_epoch_           = static_cast<int>(ar_try_read_scalar_int64(ar, "best_epoch", best_epoch_));
    total_iters_trained_  = ar_try_read_scalar_int64(ar, "total_iters_trained", total_iters_trained_);
    total_epochs_trained_ = ar_try_read_scalar_int64(ar, "total_epochs_trained", total_epochs_trained_);
    scheduler_batch_steps_ = saved_scheduler_batch_steps;
    scheduler_epoch_steps_ = saved_scheduler_epoch_steps;

    // --- telemetry (optional) ---
    {
      torch::Tensor t;
      if (ar_try_read_tensor(ar, "channel_ema", t) && t.defined())          channel_ema_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_channel_nll", t) && t.defined()) last_per_channel_nll_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_horizon_nll", t) && t.defined()) last_per_horizon_nll_ = t.to(semantic_model->device);
    }

    // --- scheduler strict restore ---
    const bool sched_serialized = (ar_try_read_scalar_int64(ar, "scheduler_serialized", 0) != 0);
    if (lr_sched) {
      const auto mode = lr_sched->mode;
      if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
        TORCH_CHECK(scheduler_epoch_steps_ == 0,
                    "[ExpectedValue::ckpt] invalid scheduler counters for PerBatch mode.");
      } else {
        TORCH_CHECK(scheduler_batch_steps_ == 0,
                    "[ExpectedValue::ckpt] invalid scheduler counters for non-PerBatch mode.");
      }

      if (sched_serialized) {
        torch::serialize::InputArchive sch_ar;
        bool has_scheduler_archive = true;
        try {
          ar.read("scheduler", sch_ar);
        } catch (...) {
          has_scheduler_archive = false;
        }
        TORCH_CHECK(has_scheduler_archive,
                    "[ExpectedValue::ckpt] scheduler marked serialized but archive is missing.");
        TORCH_CHECK(try_load_scheduler(*lr_sched, sch_ar),
                    "[ExpectedValue::ckpt] scheduler archive present but scheduler does not support load().");
      } else {
        if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
          for (int64_t i = 0; i < scheduler_batch_steps_; ++i) lr_sched->step();
        } else if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch) {
          for (int64_t e = 0; e < scheduler_epoch_steps_; ++e) lr_sched->step();
        } else {
          TORCH_CHECK(scheduler_epoch_steps_ == 0,
                      "[ExpectedValue::ckpt] PerEpochWithMetric checkpoints require serialized scheduler state.");
        }
      }
    }

    log_info("%s[ExpectedValue::ckpt]%s loaded ← %s (best=%.6f:at.%d, iters=%lld epochs=%lld, sch[b=%lld,e=%lld])\n",
             ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET, path.c_str(),
             best_metric_, best_epoch_,
             static_cast<long long>(total_iters_trained_),
             static_cast<long long>(total_epochs_trained_),
             static_cast<long long>(scheduler_batch_steps_),
             static_cast<long long>(scheduler_epoch_steps_));
    return true;

  } catch (const c10::Error& e) {
    log_err("[ExpectedValue::ckpt] load failed: %s\n", e.what());
    return false;
  } catch (const std::exception& e) {
    log_err("[ExpectedValue::ckpt] load failed: %s\n", e.what());
    return false;
  } catch (...) {
    log_err("[ExpectedValue::ckpt] load failed (unknown exception)\n");
    return false;
  }
}

// ---------- archive helpers ----------
double
ExpectedValue::ar_try_read_scalar_double(torch::serialize::InputArchive& ar, const char* key, double def) {
  try {
    torch::Tensor t;
    ar.read(key, t);
    if (t.defined() && t.numel() > 0) return t.item().toDouble();
  } catch (...) {}
  return def;
}

int64_t
ExpectedValue::ar_try_read_scalar_int64(torch::serialize::InputArchive& ar, const char* key, int64_t def) {
  try {
    torch::Tensor t;
    ar.read(key, t);
    if (t.defined() && t.numel() > 0) return t.item().toLong();
  } catch (...) {}
  return def;
}

bool
ExpectedValue::ar_try_read_tensor(torch::serialize::InputArchive& ar, const char* key, torch::Tensor& out) {
  try {
    torch::Tensor t;
    ar.read(key, t);
    out = t;
    return true;
  } catch (...) { return false; }
}

// ---------- pretty print ----------
void ExpectedValue::display_model(bool display_semantic=false) const {
  // 1) Setup & IDs (never pass raw nullptrs to %s)
  const auto& setup = cuwacunu::jkimyei::jk_setup(component_name, contract_hash);
  const std::string opt_id  = setup.opt_conf.id.empty()  ? std::string("<unset>") : setup.opt_conf.id;
  const std::string sch_id  = setup.sch_conf.id.empty()  ? std::string("<unset>") : setup.sch_conf.id;
  const std::string loss_id = setup.loss_conf.id.empty() ? std::string("<unset>") : setup.loss_conf.id;

  // 2) Current LR
  double lr_now = 0.0;
  if (optimizer) {
    lr_now = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);
  }

  // 3) Horizon policy text
  const char* horizon_policy_str = "Uniform";
  switch (horizon_policy_) {
    case HorizonPolicy::Uniform:      horizon_policy_str = "Uniform";      break;
    case HorizonPolicy::NearTerm:     horizon_policy_str = "NearTerm";     break;
    case HorizonPolicy::VeryNearTerm: horizon_policy_str = "VeryNearTerm"; break;
  }

  // 4) Static weights preview
  const auto C  = semantic_model ? semantic_model->C_axes : 0;
  const auto Dy = semantic_model ? semantic_model->Dy     : 0;

  auto preview_vec = [](const std::vector<float>& v, size_t n=4) {
    if (v.empty()) return std::string("none");
    std::ostringstream oss; oss.setf(std::ios::fixed); oss << std::setprecision(4);
    oss << "[";
    for (size_t i=0; i<v.size() && i<n; ++i) { if (i) oss << ", "; oss << v[i]; }
    if (v.size() > n) oss << ", ...";
    oss << "]";
    return oss.str();
  };
  auto preview_targets = [](const std::vector<int64_t>& v, size_t n=6) {
    if (v.empty()) return std::string("[]");
    std::ostringstream oss; oss << "[";
    for (size_t i=0; i<v.size() && i<n; ++i) { if (i) oss << ", "; oss << v[i]; }
    if (v.size() > n) oss << ", ...";
    oss << "]";
    return oss.str();
  };

  // 5) EMA stats
  bool   ema_on         = use_channel_ema_weights_;
  double ema_alpha_show = ema_alpha_;
  double ema_min = 0.0, ema_max = 0.0;
  bool   ema_has_values = channel_ema_.defined() && channel_ema_.numel() > 0;
  if (ema_has_values) {
    auto ema_cpu = channel_ema_.detach().to(torch::kCPU);
    ema_min = ema_cpu.min().item<double>();
    ema_max = ema_cpu.max().item<double>();
  }

  // 6) Loss params (from your MdnNLLLoss)
  const double loss_eps  = (loss_obj ? loss_obj->eps_       : 0.0);
  const double s_min     = (loss_obj ? loss_obj->sigma_min_ : 0.0);
  const double s_max     = (loss_obj ? loss_obj->sigma_max_ : 0.0);
  const char*  red_name  = "mean"; // current default; update if you add options

  // 7) Tiny helper for colors (wrap and reset)
  auto colorize = [](const char* color, const std::string& txt) -> std::string {
    return std::string(color) + txt + ANSI_COLOR_RESET;
  };
  auto k = [&](const char* key){ return colorize(ANSI_COLOR_Bright_Grey, key); };
  auto vS = [&](const std::string& s){ return colorize(ANSI_COLOR_Bright_Blue, s); };
  auto vD = [&](double d, int prec=3){ std::ostringstream o; o.setf(std::ios::fixed); o<<std::setprecision(prec)<<d; return colorize(ANSI_COLOR_Bright_Blue, o.str()); };
  auto vI = [&](long long i){ return colorize(ANSI_COLOR_Bright_Blue, std::to_string(i)); };

  // 8) Compose a single, safe string
  std::ostringstream out;
  out << "\n" << ANSI_COLOR_Dim_Green << "\t[Value Estimator]" << ANSI_COLOR_RESET
      << "\n\t\t"       << k("Optimizer:")              << "                "      << vS(opt_id)
      << "\n\t\t"       << k("LR Scheduler:")           << "             "         << vS(sch_id)
      << "\n\t\t    "   << k("- lr:")                   << "                 "     << vD(lr_now, 3)
      << "\n\t\t"       << k("Loss:")                   << "                     " << vS(loss_id)
      << "\n\t\t    "   << k("- eps:")                  << "                "      << vD(loss_eps, 2)
      << "\n\t\t    "   << k("- sigma_min:")            << "          "            << vD(s_min, 2)
      << "\n\t\t    "   << k("- sigma_max:")            << "          "            << vD(s_max, 2)
      << "\n\t\t    "   << k("- reduction:")            << "          "            << vS(red_name)
      << "\n\t\t"       << k("Horizon policy:")         << "           "           << vS(horizon_policy_str)
      << "\n\t\t    "   << k("- γ_near:")               << "             "         << vD(gamma_near_, 3) 
      << "\n\t\t    "   << k("- γ_very:")               << "             "         << vD(gamma_very_, 3)
      << "\n\t\t"       << k("Channels (C):")           << "             "         << vI(static_cast<long long>(C))
      << "\n\t\t    "   << k("- Static ch weights:" )   << "  "                    << vS(preview_vec(static_channel_weights_))
      << "\n\t\t"       << k("Target dims (Dy):")       << "         "             << vI(static_cast<long long>(Dy))
      << "\n\t\t"       << k("Target dims list:")       << "         "             << vS(preview_targets(target_dims_))
      << "\n\t\t    "   << k("- Static feat weights:")  << " "                     << vS(preview_vec(static_feature_weights_))
      << "\n\t\t"       << k("Channel EMA:")            << "              "        << vS(ema_on ? "ON" : "OFF")
      << "\n\t\t    "   << k("- α:")                    << "                  "    << vD(ema_alpha_show, 3)
      << "\n\t\t    "   << k("- min:")                  << "                "      << vD(ema_has_values ? ema_min : 0.0, 4)
      << "\n\t\t    "   << k("- max:")                  << "                "      << vD(ema_has_values ? ema_max : 0.0, 4)
      << "\n\t\t"       << k("Grad clip:")              << "                "      << vD(grad_clip, 3)
      << "\n\t\t"       << k("opt_threshold_reset:")    << "      "                << vI(static_cast<long long>(optimizer_threshold_reset))
      << "\n\t\t"       << k("Telemetry every:")        << "          "            << vI(telemetry_every_)
      << "\n\t\t"       << k("Progress:")         
      << "\n\t\t    "   << k("- epochs:")               << "             "         << vI(static_cast<long long>(total_epochs_trained_))
      << "\n\t\t    "   << k("- iters:")                << "              "        << vI(static_cast<long long>(total_iters_trained_))
      << "\n\t\t    "   << k("- best:")                 << "               "       << vD(best_metric_, 6) << k(".at:") << vI(best_epoch_)
      << "\n";

  // 9) Print once, safely.
  log_info("%s", out.str().c_str());

  // 10) First, show the underlying MDN model spec.
  if (semantic_model && display_semantic) {
    semantic_model->display_model();
  }
}

} // namespace wikimyei
} // namespace cuwacunu
