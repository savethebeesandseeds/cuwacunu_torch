/* expected_value.cpp */
/**
 * @file expected_value.cpp
 * @brief ExpectedValue: non-template implementation with safe checkpointing.
 */

#include "wikimyei/heuristics/semantic_learning/expected_value/expected_value.h"

#include <cmath>
#include <utility>
#include <cstdio>   // std::remove, std::rename

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .cpp
// ------------------------------------------------------------
RUNTIME_WARNING("[expected_value.cpp] Scheduler save/load is best-effort; if not serializable we replay steps approximately.\n");
RUNTIME_WARNING("[expected_value.cpp] select_targets uses from_blob()+clone for indices (tiny extra alloc, safe).\n");
RUNTIME_WARNING("[expected_value.cpp] Channel EMA weights use 1/(ema+eps) with clamp_max to limit volatility.\n");
RUNTIME_WARNING("[expected_value.cpp] Optimizer state is skipped on CUDA during save; loader tolerates its absence.\n");
RUNTIME_WARNING("[expected_value.cpp] Checkpoint uses SAFE state-dict (params/buffers only); avoids JIT pickler & undefined buffers.\n");
RUNTIME_WARNING("[expected_value.cpp] Atomic-ish save: write to .tmp then rename to final path.\n");

namespace cuwacunu {
namespace wikimyei {

// -------------------- safe state-dict helpers ----------------
namespace {
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
ExpectedValue::ExpectedValue(const std::string& component_name_)
: component_name(component_name_) {
  // Static weights and target dims from config/pipeline
  static_channel_weights_     = cuwacunu::camahjucunu::observation_pipeline_t::inst.retrieve_channel_weights();
  static_feature_weights_     = cuwacunu::piaabo::dconfig::config_space_t::get_arr<float>("VALUE_ESTIMATION", "target_weights");
  grad_clip                   = cuwacunu::piaabo::dconfig::config_space_t::get<double>("VALUE_ESTIMATION", "grad_clip");
  optimizer_threshold_reset   = cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "optimizer_threshold_reset");
  target_dims_                = cuwacunu::piaabo::dconfig::config_space_t::get_arr<int64_t>("VALUE_ESTIMATION", "target_dims");

  // Model
  semantic_model = cuwacunu::wikimyei::mdn::MdnModel(
    cuwacunu::piaabo::dconfig::config_space_t::get<int>("VICReg", "encoding_dims"),               // De
    static_cast<int64_t>(target_dims_.size()),                                                    // Dy
    cuwacunu::camahjucunu::observation_pipeline_t::inst.count_channels(),                         // C
    cuwacunu::camahjucunu::observation_pipeline_t::inst.max_future_sequence_length(),             // Hf
    cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "mixture_comps"),     // K
    cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "features_hidden"),   // H
    cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "residual_depth"),    // depth
    cuwacunu::piaabo::dconfig::config_dtype  ("VALUE_ESTIMATION"),
    cuwacunu::piaabo::dconfig::config_device ("VALUE_ESTIMATION"),
    /* display_model */ false
  );

  // Params
  trainable_params_.reserve(64);
  for (auto& p : semantic_model->parameters(/*recurse=*/true)) {
    if (p.requires_grad()) trainable_params_.push_back(p);
  }

  // Builders
  TORCH_CHECK(cuwacunu::jkimyei::jk_setup(component_name).opt_builder,   "[ExpectedValue](ctor) opt_builder is null");
  TORCH_CHECK(cuwacunu::jkimyei::jk_setup(component_name).sched_builder, "[ExpectedValue](ctor) sched_builder is null");

  optimizer = cuwacunu::jkimyei::jk_setup(component_name).opt_builder->build(trainable_params_);
  lr_sched  = cuwacunu::jkimyei::jk_setup(component_name).sched_builder->build(*optimizer);
  loss_obj  = std::make_unique<cuwacunu::wikimyei::mdn::MdnNLLLoss>(cuwacunu::jkimyei::jk_setup(component_name));

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
ExpectedValue::masked_mean_loss_per_channel(const torch::Tensor& nll, const torch::Tensor& mask)
{
  auto valid = mask.to(nll.dtype());
  auto sumB = (nll * valid).sum(/*dim=*/0);           // [C,Hf]
  auto den  = valid.sum(/*dim=*/0).clamp_min(1.0);    // [C,Hf]
  auto ch_mean_over_h = (sumB / den).mean(/*dim=*/1); // [C]
  return ch_mean_over_h;
}

torch::Tensor
ExpectedValue::masked_mean_loss_per_horizon(const torch::Tensor& nll, const torch::Tensor& mask)
{
  auto valid = mask.to(nll.dtype());
  auto sumBC = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0,1}, /*keepdim=*/false); // [Hf]
  auto den   = valid.sum(/*dim=*/std::vector<int64_t>{0,1}, /*keepdim=*/false).clamp_min(1.0); // [Hf]
  return sumBC / den; // [Hf]
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

// ---------- stability helper (norm-aware) ----------
void ExpectedValue::maybe_reset_optimizer_state_by_norm(double grad_norm) {
  if (!optimizer) return;
  if (optimizer_threshold_reset < 0) return;  // disabled
  if (!(grad_norm > static_cast<double>(optimizer_threshold_reset))) return;

  log_warn("[ExpectedValue::opt] grad_norm=%.3g > %d → resetting optimizer state\n",
           grad_norm, optimizer_threshold_reset);

  // Clear common momentum stats (Adam/AdamW, RMSprop); SGD becomes a no-op here.
  auto& st = optimizer->state();  // map<Tensor, unique_ptr<OptimizerParamState>>
  for (auto& kv : st) {
    if (!kv.second) continue;
    if (auto* s = dynamic_cast<torch::optim::AdamParamState*>(kv.second.get())) {
      if (s->exp_avg().defined())     s->exp_avg().zero_();
      if (s->exp_avg_sq().defined())  s->exp_avg_sq().zero_();
#if defined(TORCH_API_INCLUDE_EXTENSION_H) || 1
      if (s->max_exp_avg_sq().defined()) s->max_exp_avg_sq().zero_();
#endif
      s->step(0);
      continue;
    }
    if (auto* s = dynamic_cast<torch::optim::RMSpropParamState*>(kv.second.get())) {
      if (s->square_avg().defined()) s->square_avg().zero_();
      if (s->momentum_buffer().defined()) s->momentum_buffer().zero_();
      if (s->grad_avg().defined()) s->grad_avg().zero_();
      continue;
    }
  }
}


// ---------- telemetry helper: per-(B,C,Hf) NLL map ----------
torch::Tensor
ExpectedValue::compute_nll_map(const cuwacunu::wikimyei::mdn::MdnOut& out,
                               const torch::Tensor& y,
                               const torch::Tensor& mask)
{
  const auto& log_pi = out.log_pi; // [B,C,Hf,K]
  const auto& mu     = out.mu;     // [B,C,Hf,K,Dy]
  auto sigma         = out.sigma;  // [B,C,Hf,K,Dy]
  sigma = sigma.clamp_min(loss_obj->sigma_min_);
  if (loss_obj->sigma_max_ > 0.0) sigma = sigma.clamp_max(loss_obj->sigma_max_);
  auto eps_t = sigma.new_full({}, loss_obj->eps_);

  // Portable LOG2PI (no M_PI)
  static const double LOG2PI = std::log(2.0 * 3.14159265358979323846);

  auto y_b    = y.unsqueeze(3).expand({y.size(0), y.size(1), y.size(2), log_pi.size(3), y.size(3)});
  auto diff   = (y_b - mu) / (sigma + eps_t);
  auto perdim = -0.5 * diff.pow(2) - (sigma + eps_t).log() - 0.5 * LOG2PI; // [B,C,Hf,K,Dy]
  auto comp_logp = perdim.sum(-1);                                         // [B,C,Hf,K]
  auto log_mix   = torch::logsumexp(log_pi + comp_logp, /*dim=*/3);        // [B,C,Hf]
  auto nll_map   = -log_mix;                                               // [B,C,Hf]
  if (mask.defined()) {
    auto valid = mask.to(nll_map.dtype());
    nll_map = nll_map * valid;
  }
  return nll_map;
}

// ==========================
// Checkpointing (SAFE)
// ==========================
bool
ExpectedValue::save_checkpoint(const std::string& path) const {
  const std::string tmp = path + ".tmp";
  try {
    torch::serialize::OutputArchive ar;

    // --- model state (safe, tensor-by-tensor) ---
    ev_save_module_state(ar, *semantic_model, "model");

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
    ar.write("has_optimizer", torch::tensor({wrote_opt}));

    // --- scheduler (best effort) ---
    if (lr_sched) {
      torch::serialize::OutputArchive sch_ar;
      if (try_save_scheduler(*lr_sched, sch_ar)) {
        ar.write("scheduler", sch_ar);
        ar.write("scheduler_serialized", torch::tensor({int64_t(1)}));
      } else {
        ar.write("scheduler_serialized", torch::tensor({int64_t(0)}));
      }
    }

    // --- scalars ---
    ar.write("best_metric",            torch::tensor({best_metric_}));
    ar.write("best_epoch",             torch::tensor({int64_t(best_epoch_)}));
    ar.write("total_iters_trained",    torch::tensor({int64_t(total_iters_trained_)}));
    ar.write("total_epochs_trained",   torch::tensor({int64_t(total_epochs_trained_)}));
    ar.write("step_scheduler_per_iter",torch::tensor({int64_t(step_scheduler_per_iter_ ? 1 : 0)}));

    // --- telemetry (write ONLY if defined; no placeholders) ---
    if (channel_ema_.defined())          ar.write("channel_ema",          channel_ema_.detach().to(torch::kCPU));
    if (last_per_channel_nll_.defined()) ar.write("last_per_channel_nll", last_per_channel_nll_.detach().to(torch::kCPU));
    if (last_per_horizon_nll_.defined()) ar.write("last_per_horizon_nll", last_per_horizon_nll_.detach().to(torch::kCPU));

    // --- atomic-ish write ---
    ar.save_to(tmp);
    std::remove(path.c_str());
    std::rename(tmp.c_str(), path.c_str());

    log_info("%s[ExpectedValue::ckpt]%s saved → %s\n", ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET, path.c_str());
    return true;

  } catch (const c10::Error& e) {
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
ExpectedValue::load_checkpoint(const std::string& path, bool strict) {
  try {
    torch::serialize::InputArchive ar;
    ar.load_from(path.c_str());

    // --- model state (safe, tensor-by-tensor) ---
    ev_load_module_state(ar, *semantic_model, "model");
    semantic_model->to(semantic_model->device, semantic_model->dtype, /*non_blocking=*/false);

    // --- optimizer (optional) ---
    const bool expect_opt = (ar_try_read_scalar_int64(ar, "has_optimizer", 0) != 0);
    if (optimizer && expect_opt) {
      try {
        torch::serialize::InputArchive oa;
        ar.read("optimizer", oa);
        optimizer->load(oa);
      } catch (const c10::Error& e) {
        log_warn("[ExpectedValue::ckpt] optimizer state missing/incompatible; continuing. Err=%s\n", e.what());
      }
    }

    // --- scalars ---
    best_metric_            = ar_try_read_scalar_double(ar, "best_metric", best_metric_);
    best_epoch_             = static_cast<int>(ar_try_read_scalar_int64(ar, "best_epoch", best_epoch_));
    total_iters_trained_    = ar_try_read_scalar_int64(ar, "total_iters_trained", total_iters_trained_);
    total_epochs_trained_   = ar_try_read_scalar_int64(ar, "total_epochs_trained", total_epochs_trained_);
    step_scheduler_per_iter_= (ar_try_read_scalar_int64(ar, "step_scheduler_per_iter", step_scheduler_per_iter_ ? 1 : 0) != 0);

    // --- telemetry (optional) ---
    {
      torch::Tensor t;
      if (ar_try_read_tensor(ar, "channel_ema", t) && t.defined())          channel_ema_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_channel_nll", t) && t.defined()) last_per_channel_nll_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_horizon_nll", t) && t.defined()) last_per_horizon_nll_ = t.to(semantic_model->device);
    }

    // --- scheduler: native load or replay ---
    bool sched_serialized = (ar_try_read_scalar_int64(ar, "scheduler_serialized", 0) != 0);
    if (lr_sched) {
      if (sched_serialized) {
        torch::serialize::InputArchive sch_ar;
        ar.read("scheduler", sch_ar);
        if (!try_load_scheduler(*lr_sched, sch_ar)) replay_scheduler_progress();
      } else {
        replay_scheduler_progress();
      }
    }

    log_info("%s[ExpectedValue::ckpt]%s loaded ← %s (best=%.6f:at.%d, iters=%lld epochs=%lld)\n",
             ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET, path.c_str(),
             best_metric_, best_epoch_,
             static_cast<long long>(total_iters_trained_),
             static_cast<long long>(total_epochs_trained_));
    return true;

  } catch (const c10::Error& e) {
    if (strict) {
      log_err("[ExpectedValue::ckpt] load failed: %s\n", e.what());
      return false;
    }
    log_warn("[ExpectedValue::ckpt] load encountered an error but strict=false; continuing. Err=%s\n", e.what());
    return false;
  } catch (...) {
    if (strict) {
      log_err("[ExpectedValue::ckpt] load failed (unknown exception)\n");
      return false;
    }
    log_warn("[ExpectedValue::ckpt] load encountered unknown error but strict=false; continuing.\n");
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

// ---------- scheduler progress replay ----------
void
ExpectedValue::replay_scheduler_progress() {
  if (!lr_sched) return;
  if (step_scheduler_per_iter_) {
    for (int64_t i = 0; i < total_iters_trained_; ++i) lr_sched->step();
  } else {
    for (int64_t e = 0; e < total_epochs_trained_; ++e) lr_sched->step();
  }
  log_warn("[ExpectedValue::ckpt] scheduler replayed to iters=%lld epochs=%lld (approximate)\n",
           static_cast<long long>(total_iters_trained_),
           static_cast<long long>(total_epochs_trained_));
}

// ---------- stability helper ----------
void ExpectedValue::maybe_reset_optimizer_state(double /*clip_threshold*/) {
  if (!optimizer) return;
  if (optimizer_threshold_reset < 0) return;
  double sumsq = 0.0;
  for (auto& p : semantic_model->parameters()) {
    if (p.grad().defined()) {
      auto g = p.grad();
      sumsq += g.pow(2).sum().item<double>();
    }
  }
  maybe_reset_optimizer_state_by_norm(std::sqrt(sumsq));

}

// ---------- pretty print ----------
void ExpectedValue::display_model(bool display_semantic=false) const {
  // 1) Setup & IDs (never pass raw nullptrs to %s)
  const auto& setup = cuwacunu::jkimyei::jk_setup(component_name);
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
