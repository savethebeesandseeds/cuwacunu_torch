/* vicreg_4d.cpp */
#include <torch/torch.h>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <string>

#include "wikimyei/representation/VICReg/vicreg_4d.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/training_setup/jk_setup.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

using cuwacunu::piaabo::dconfig::contract_space_t;

// -------------------- local helpers (strict ckpt meta & config parsers) --------------------
namespace {

inline torch::Tensor read_tensor_strict(const std::string& path, const char* key) {
  torch::serialize::InputArchive ar; ar.load_from(path);
  torch::Tensor t;
  const bool ok = ar.try_read(key, t);
  TORCH_CHECK(ok, "[VICReg] Missing checkpoint meta key: '", key, "' in ", path);
  return t.to(torch::kCPU);
}

inline int64_t read_i64_strict(const std::string& path, const char* key) {
  auto t = read_tensor_strict(path, key);
  TORCH_CHECK(t.numel() >= 1, "[VICReg] Empty tensor for key: '", key, "'");
  return t.item<int64_t>();
}

inline std::string read_str_strict(const std::string& path, const char* key) {
  auto t = read_tensor_strict(path, key);
  std::string s;
  s.resize(static_cast<size_t>(t.numel()));
  if (t.numel() > 0) std::memcpy(s.data(), t.data_ptr<int8_t>(), static_cast<size_t>(t.numel()));
  return s;
}

inline torch::Dtype dtype_from_tag_strict(const std::string& tag) {
  if (tag == "f16") return torch::kFloat16;
  if (tag == "f32") return torch::kFloat32;
  if (tag == "f64") return torch::kFloat64;
  TORCH_CHECK(false, "[VICReg] Unknown dtype tag in checkpoint meta: '", tag, "'. Expected f16|f32|f64.");
  return torch::kFloat32; // unreachable
}

inline std::string read_component_name(const std::string& path) {
  const auto component_name = read_str_strict(path, "meta/jk/component_name");
  TORCH_CHECK(!component_name.empty(), "[VICReg] Saved component_name is empty.");
  return component_name;
}

// ---- strict parsers for projector options ----
inline std::string to_lower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool parse_bool_strict(std::string s) {
  s = to_lower(s);
  if (s == "true" || s == "1" || s == "yes")  return true;
  if (s == "false"|| s == "0" || s == "no")   return false;
  throw std::runtime_error("Invalid boolean: " + s + " (expected true/false/1/0/yes/no)");
}

inline NormKind parse_norm_kind_strict(std::string s) {
  s = to_lower(s);
  if (s == "batchnorm1d" || s == "bn"  || s == "batchnorm") return NormKind::BatchNorm1d;
  if (s == "layernorm"   || s == "ln")                       return NormKind::LayerNorm;
  if (s == "none"        || s == "null"|| s == "identity")   return NormKind::None;
  throw std::runtime_error("Invalid projector_norm: " + s + " (expected BatchNorm1d/LayerNorm/None)");
}

inline ActKind parse_act_kind_strict(std::string s) {
  s = to_lower(s);
  if (s == "relu") return ActKind::ReLU;
  if (s == "silu" || s == "swish") return ActKind::SiLU;
  throw std::runtime_error("Invalid projector_activation: " + s + " (expected ReLU/SiLU)");
}

static inline void write_str(torch::serialize::OutputArchive& root,
                             const char* key,
                             const std::string& s) {
  auto t = torch::empty({static_cast<int64_t>(s.size())}, torch::kChar);
  if (t.numel() > 0) std::memcpy(t.data_ptr<int8_t>(), s.data(), s.size());
  root.write(key, t);
}

} // anon

// -------------------- constructors --------------------

VICReg_4D::VICReg_4D(
  const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash_,
  const std::string& component_name_, 
  int C_, int T_, int D_,
  int encoding_dims_,
  int channel_expansion_dim_,
  int fused_feature_dim_,
  int encoder_hidden_dims_,
  int encoder_depth_,
  std::string projector_mlp_spec_,
  torch::Dtype dtype_,
  torch::Device device_,
  int optimizer_threshold_reset_,
  bool enable_buffer_averaging_
)
: contract_hash(contract_hash_),
  component_name(component_name_),
  C(C_), T(T_), D(D_),
  encoding_dims(encoding_dims_),
  channel_expansion_dim(channel_expansion_dim_),
  fused_feature_dim(fused_feature_dim_),
  encoder_hidden_dims(encoder_hidden_dims_),
  encoder_depth(encoder_depth_),
  projector_mlp_spec(projector_mlp_spec_),
  dtype(dtype_),
  device(device_),
  optimizer_threshold_reset(optimizer_threshold_reset_),
  enable_buffer_averaging(enable_buffer_averaging_),

  _encoder_net(register_module("_encoder_net",
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D_Encoder(
      C, T, D,
      encoding_dims_,
      channel_expansion_dim_,
      fused_feature_dim_,
      encoder_hidden_dims_,
      encoder_depth_,
      dtype_,
      device_
  ))),
  _swa_encoder_net(register_module("_swa_encoder_net",
    cuwacunu::wikimyei::vicreg_4d::StochasticWeightAverage_Encoder(
      _encoder_net,
      enable_buffer_averaging_,
      dtype_,
      device_
  ))),
  // Projector constructed with explicit options from config (strict parsing)
  _projector_net(nullptr),
  aug(cuwacunu::jkimyei::jk_setup(component_name, contract_hash).inst.retrive_table("vicreg_augmentations")),
  trainable_params_(),
  optimizer(nullptr),
  lr_sched(nullptr),
  loss_obj(nullptr)
{
  // ---- Read projector options from dconfig (strings) and cast strictly ----
  auto norm_s =
      contract_space_t::get<std::string>(contract_hash, "VICReg", "projector_norm");
  auto act_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_activation");
  auto hbias_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_hidden_bias");
  auto lbias_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_last_bias");
  auto bnfp32_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_bn_in_fp32");

  ProjectorOptions popts{
    /* norm_kind       */ parse_norm_kind_strict(norm_s),
    /* act_kind        */ parse_act_kind_strict(act_s),
    /* use_hidden_bias */ parse_bool_strict(hbias_s),
    /* use_last_bias   */ parse_bool_strict(lbias_s),
    /* bn_in_fp32      */ parse_bool_strict(bnfp32_s)
  };

  _projector_net = register_module("_projector_net",
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D_Projector(
      encoding_dims_,
      projector_mlp_spec_,
      popts,
      dtype_,
      device_
  ));

  // ---- Optimizer / scheduler / loss ----
  trainable_params_.reserve(
    _encoder_net->parameters().size() + _projector_net->parameters().size());
  for (auto& p : _encoder_net->parameters())   trainable_params_.push_back(p);
  for (auto& p : _projector_net->parameters()) trainable_params_.push_back(p);

  auto& jk_component = cuwacunu::jkimyei::jk_setup(component_name, contract_hash);
  TORCH_CHECK(jk_component.opt_builder, "[VICReg_4D::ctor] opt_builder is null");
  TORCH_CHECK(jk_component.sched_builder, "[VICReg_4D::ctor] sched_builder is null");

  optimizer = jk_component.opt_builder->build(trainable_params_);
  lr_sched  = jk_component.sched_builder->build(*optimizer);
  loss_obj  = std::make_unique<VicRegLoss>(jk_component);
  load_jkimyei_training_policy(jk_component);

  display_model();
  warm_up();
}

VICReg_4D::VICReg_4D(
  const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash_,
  const std::string& component_name_, 
  int C_, int T_, int D_
)
: VICReg_4D(
    contract_hash_,
    component_name_,
    C_, T_, D_,
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
        contract_hash_, "VICReg", "encoding_dims"),
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
        contract_hash_, "VICReg", "channel_expansion_dim"),
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
        contract_hash_, "VICReg", "fused_feature_dim"),
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
        contract_hash_, "VICReg", "encoder_hidden_dims"),
    cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
        contract_hash_, "VICReg", "encoder_depth"),
    cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
        contract_hash_, "VICReg", "projector_mlp_spec"),
    cuwacunu::piaabo::dconfig::config_dtype(contract_hash_, "VICReg"),
    cuwacunu::piaabo::dconfig::config_device(contract_hash_, "VICReg"),
    -1,
    cuwacunu::piaabo::dconfig::contract_space_t::get<bool>(
        contract_hash_, "VICReg", "enable_buffer_averaging")
) {
  // Force presence of projector option keys early (fail fast)
  (void)cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_norm");
  (void)cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_activation");
  (void)cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_hidden_bias");
  (void)cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_last_bias");
  (void)cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_bn_in_fp32");

  log_info("Initialized VICReg encoder from Configuration file...\n");
}

VICReg_4D::VICReg_4D(
                     const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash_,
                     const std::string& checkpoint_path,
                     torch::Device override_device)
: VICReg_4D(
    contract_hash_,
    read_component_name(checkpoint_path),
    read_i64_strict(checkpoint_path, "meta/C"),
    read_i64_strict(checkpoint_path, "meta/T"),
    read_i64_strict(checkpoint_path, "meta/D"),
    read_i64_strict(checkpoint_path, "meta/encoding_dims"),
    read_i64_strict(checkpoint_path, "meta/channel_expansion_dim"),
    read_i64_strict(checkpoint_path, "meta/fused_feature_dim"),
    read_i64_strict(checkpoint_path, "meta/encoder_hidden_dims"),
    read_i64_strict(checkpoint_path, "meta/encoder_depth"),
    read_str_strict(checkpoint_path, "meta/projector_mlp_spec"),
    dtype_from_tag_strict(read_str_strict(checkpoint_path, "meta/dtype")),
    override_device,
    static_cast<int>(read_i64_strict(checkpoint_path, "meta/optimizer_threshold_reset")),
    static_cast<bool>(read_i64_strict(checkpoint_path, "meta/enable_buffer_averaging"))
) {
  this->load(checkpoint_path);
}

void VICReg_4D::reset_runtime_training_state() {
  runtime_iter_count_ = 0;
  runtime_accum_counter_ = 0;
  runtime_has_pending_grad_ = false;
  runtime_pending_loss_sum_ = 0.0;
  runtime_pending_loss_count_ = 0;
  runtime_last_committed_loss_mean_ = 0.0;
  if (optimizer) optimizer->zero_grad(jk_zero_grad_set_to_none);
}

VICReg_4D::train_step_result_t VICReg_4D::train_one_batch(
    const torch::Tensor& data,
    const torch::Tensor& mask,
    int swa_start_iter,
    bool verbose) {
  train_step_result_t result{};

  TORCH_CHECK(optimizer, "[VICReg_4D::train_one_batch] optimizer is null");
  TORCH_CHECK(loss_obj, "[VICReg_4D::train_one_batch] loss object is null");
  TORCH_CHECK(data.defined(), "[VICReg_4D::train_one_batch] data is undefined");
  TORCH_CHECK(mask.defined(), "[VICReg_4D::train_one_batch] mask is undefined");
  TORCH_CHECK(!data.requires_grad() && !data.grad_fn(),
      "[VICReg_4D::train_one_batch] data must not require grad");
  TORCH_CHECK(!mask.requires_grad() && !mask.grad_fn(),
      "[VICReg_4D::train_one_batch] mask must not require grad");
  TORCH_CHECK(data.dim() == 4,
      "[VICReg_4D::train_one_batch] data.dim()=" + std::to_string(data.dim()) +
      " (expected 4: [B,C,T,D])");
  TORCH_CHECK(data.size(1) == C && data.size(2) == T && data.size(3) == D,
      "[VICReg_4D::train_one_batch] data shape mismatch");
  TORCH_CHECK(mask.dim() == 3,
      "[VICReg_4D::train_one_batch] mask.dim()=" + std::to_string(mask.dim()) +
      " (expected 3: [B,C,T])");
  TORCH_CHECK(mask.size(1) == C && mask.size(2) == T,
      "[VICReg_4D::train_one_batch] mask shape mismatch");

  if (!runtime_has_pending_grad_) {
    optimizer->zero_grad(jk_zero_grad_set_to_none);
  }

  _encoder_net->train();
  _projector_net->train();
  _swa_encoder_net->encoder().train();

  auto data_d = data.to(device);
  auto mask_d = mask.to(device);

  auto [d1, m1] = aug.augment(data_d, mask_d);
  auto [d2, m2] = aug.augment(data_d, mask_d);

  if (verbose && (runtime_iter_count_ % 100 == 0)) {
    auto shared_bt = (m1 & m2).all(/*dim=*/1);
    auto shared_mask =
        shared_bt.unsqueeze(1).unsqueeze(-1).expand_as(d1);
    const double diff =
        (d1.masked_select(shared_mask) - d2.masked_select(shared_mask))
            .abs()
            .mean()
            .template item<double>();
    TORCH_CHECK(
        diff > 1e-6,
        "[VICReg_4D::train_one_batch] augmentation produced identical views on shared valid region");
  }

  auto k1 = _encoder_net(d1, m1);
  auto k2 = _encoder_net(d2, m2);

  auto valid_bt = (m1 & m2).all(/*dim=*/1);
  auto expand_E = [&](int64_t E) {
    return valid_bt.unsqueeze(-1).expand({k1.size(0), k1.size(1), E});
  };
  auto k1v =
      k1.masked_select(expand_E(k1.size(-1))).view({-1, k1.size(-1)});
  auto k2v =
      k2.masked_select(expand_E(k2.size(-1))).view({-1, k2.size(-1)});

  const int64_t N_eff = k1v.size(0);
  if (N_eff <= 1) {
    result.skipped = true;
    return result;
  }

  auto z1v = _projector_net->forward_flat(k1v);
  auto z2v = _projector_net->forward_flat(k2v);

  auto terms = loss_obj->forward_terms(z1v, z2v);
  constexpr int cov_ramp_iters = 3000;
  const double cov_boost = (runtime_iter_count_ < cov_ramp_iters)
      ? (3.0 - 2.0 * (static_cast<double>(runtime_iter_count_) /
                      static_cast<double>(cov_ramp_iters)))
      : 1.0;
  auto loss = loss_obj->sim_coeff_ * terms.inv +
              loss_obj->std_coeff_ * terms.var +
              (loss_obj->cov_coeff_ * cov_boost) * terms.cov;

  const double loss_scalar = loss.template item<double>();
  if (!std::isfinite(loss_scalar)) {
    if (jk_skip_on_nan) {
      optimizer->zero_grad(jk_zero_grad_set_to_none);
      runtime_accum_counter_ = 0;
      runtime_has_pending_grad_ = false;
      runtime_pending_loss_sum_ = 0.0;
      runtime_pending_loss_count_ = 0;
      result.skipped = true;
      return result;
    }
    TORCH_CHECK(
        false,
        "[VICReg_4D::train_one_batch] non-finite loss detected and skip_on_nan=false");
  }

  runtime_pending_loss_sum_ += loss_scalar;
  ++runtime_pending_loss_count_;

  auto backprop_loss = (jk_accumulate_steps > 1)
      ? (loss / static_cast<double>(jk_accumulate_steps))
      : loss;
  backprop_loss.backward();
  runtime_has_pending_grad_ = true;
  ++runtime_accum_counter_;

  result.loss = loss.detach().to(torch::kCPU);

  if (runtime_accum_counter_ < std::max(1, jk_accumulate_steps)) {
    return result;
  }

  std::vector<torch::Tensor> all_p;
  all_p.reserve(_encoder_net->parameters().size() + _projector_net->parameters().size());
  for (auto& p : _encoder_net->parameters()) all_p.push_back(p);
  for (auto& p : _projector_net->parameters()) all_p.push_back(p);

  std::vector<torch::Tensor> clip_vec;
  clip_vec.reserve(all_p.size());
  for (const auto& p : all_p) {
    const auto g = p.grad();
    if (g.defined()) clip_vec.push_back(p);
  }
  if (!clip_vec.empty()) {
    if (jk_clip_value > 0.0) {
      torch::nn::utils::clip_grad_value_(clip_vec, jk_clip_value);
    }
    if (jk_clip_norm > 0.0) {
      torch::nn::utils::clip_grad_norm_(clip_vec, jk_clip_norm);
    } else {
      const double clip_max = (runtime_iter_count_ < 1500) ? 5.0 : 1.0;
      torch::nn::utils::clip_grad_norm_(clip_vec, clip_max);
    }
  }

  bool grad_is_finite = true;
  for (const auto& p : all_p) {
    const auto g = p.grad();
    if (!g.defined()) continue;
    if (!torch::isfinite(g).all().template item<bool>()) {
      grad_is_finite = false;
      break;
    }
  }
  if (!grad_is_finite) {
    if (jk_skip_on_nan) {
      optimizer->zero_grad(jk_zero_grad_set_to_none);
      runtime_accum_counter_ = 0;
      runtime_has_pending_grad_ = false;
      runtime_pending_loss_sum_ = 0.0;
      runtime_pending_loss_count_ = 0;
      result.skipped = true;
      return result;
    }
    TORCH_CHECK(
        false,
        "[VICReg_4D::train_one_batch] non-finite gradients detected and skip_on_nan=false");
  }

  optimizer->step();
  if (lr_sched &&
      lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
    lr_sched->step();
  }

  const int effective_swa_start_iter =
      (swa_start_iter >= 0) ? swa_start_iter : jk_swa_start_iter;
  if (jk_vicreg_use_swa && runtime_iter_count_ >= effective_swa_start_iter) {
    _swa_encoder_net->update_parameters(_encoder_net);
  }

  runtime_last_committed_loss_mean_ =
      (runtime_pending_loss_count_ > 0)
          ? (runtime_pending_loss_sum_ /
             static_cast<double>(runtime_pending_loss_count_))
          : 0.0;
  runtime_pending_loss_sum_ = 0.0;
  runtime_pending_loss_count_ = 0;
  runtime_accum_counter_ = 0;
  runtime_has_pending_grad_ = false;
  ++runtime_iter_count_;
  result.optimizer_step_applied = true;
  return result;
}

bool VICReg_4D::finalize_pending_training_step(int swa_start_iter) {
  if (!runtime_has_pending_grad_ || runtime_accum_counter_ <= 0) return false;

  std::vector<torch::Tensor> all_p;
  all_p.reserve(_encoder_net->parameters().size() + _projector_net->parameters().size());
  for (auto& p : _encoder_net->parameters()) all_p.push_back(p);
  for (auto& p : _projector_net->parameters()) all_p.push_back(p);

  std::vector<torch::Tensor> clip_vec;
  clip_vec.reserve(all_p.size());
  for (const auto& p : all_p) {
    const auto g = p.grad();
    if (g.defined()) clip_vec.push_back(p);
  }
  if (!clip_vec.empty()) {
    if (jk_clip_value > 0.0) {
      torch::nn::utils::clip_grad_value_(clip_vec, jk_clip_value);
    }
    if (jk_clip_norm > 0.0) {
      torch::nn::utils::clip_grad_norm_(clip_vec, jk_clip_norm);
    } else {
      const double clip_max = (runtime_iter_count_ < 1500) ? 5.0 : 1.0;
      torch::nn::utils::clip_grad_norm_(clip_vec, clip_max);
    }
  }

  bool grad_is_finite = true;
  for (const auto& p : all_p) {
    const auto g = p.grad();
    if (!g.defined()) continue;
    if (!torch::isfinite(g).all().template item<bool>()) {
      grad_is_finite = false;
      break;
    }
  }
  if (!grad_is_finite) {
    if (jk_skip_on_nan) {
      optimizer->zero_grad(jk_zero_grad_set_to_none);
      runtime_accum_counter_ = 0;
      runtime_has_pending_grad_ = false;
      runtime_pending_loss_sum_ = 0.0;
      runtime_pending_loss_count_ = 0;
      return false;
    }
    TORCH_CHECK(
        false,
        "[VICReg_4D::finalize_pending_training_step] non-finite gradients detected and skip_on_nan=false");
  }

  optimizer->step();
  if (lr_sched &&
      lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
    lr_sched->step();
  }

  const int effective_swa_start_iter =
      (swa_start_iter >= 0) ? swa_start_iter : jk_swa_start_iter;
  if (jk_vicreg_use_swa && runtime_iter_count_ >= effective_swa_start_iter) {
    _swa_encoder_net->update_parameters(_encoder_net);
  }

  runtime_last_committed_loss_mean_ =
      (runtime_pending_loss_count_ > 0)
          ? (runtime_pending_loss_sum_ /
             static_cast<double>(runtime_pending_loss_count_))
          : 0.0;
  runtime_pending_loss_sum_ = 0.0;
  runtime_pending_loss_count_ = 0;
  runtime_accum_counter_ = 0;
  runtime_has_pending_grad_ = false;
  ++runtime_iter_count_;
  return true;
}

// -------------------- runtime helpers --------------------

void VICReg_4D::warm_up() {
  if (!device.is_cpu()) {
    int B = 1;
    TICK(warming_up_vicreg_4d_);
    {
      auto data = torch::ones({B, C, T, D}, torch::TensorOptions().dtype(dtype).device(device));
      auto mask = torch::full({B, C, T}, true, torch::TensorOptions().dtype(torch::kBool).device(device));
      encode_projected(data, mask);
      torch::cuda::synchronize();
    }
    PRINT_TOCK_ms(warming_up_vicreg_4d_);
  }
}

torch::Tensor VICReg_4D::encode(
  const torch::Tensor& data,
  const torch::Tensor& mask,
  bool use_swa,
  bool detach_to_cpu
) {
  auto& enc = (use_swa ? _swa_encoder_net->encoder() : *_encoder_net);

  enc.eval();

  TORCH_CHECK(data.dim() == 4,
      "[VICReg_4D::encode] data.dim()=" + std::to_string(data.dim()) +
      " (expected 4: [B,C,T,D])");

  TORCH_CHECK(data.size(1) == C && data.size(2) == T && data.size(3) == D,
      "[VICReg_4D::encode] data shape mismatch: "
      "got [C=" + std::to_string(data.size(1)) +
      ",T=" + std::to_string(data.size(2)) +
      ",D=" + std::to_string(data.size(3)) +
      "], expected [C=" + std::to_string(C) +
      ",T=" + std::to_string(T) +
      ",D=" + std::to_string(D) + "]");

  TORCH_CHECK(mask.dim() == 3,
      "[VICReg_4D::encode] mask.dim()=" + std::to_string(mask.dim()) +
      " (expected 3: [B,C,T])");

  TORCH_CHECK(mask.size(1) == C && mask.size(2) == T,
      "[VICReg_4D::encode] mask shape mismatch: "
      "got [C=" + std::to_string(mask.size(1)) +
      ",T=" + std::to_string(mask.size(2)) +
      "], expected [C=" + std::to_string(C) +
      ",T=" + std::to_string(T) + "]");

  torch::NoGradGuard ng;

  auto data_d = data.to(device);
  auto mask_d = mask.to(device);

  auto rep = enc.forward(data_d, mask_d);

  if (detach_to_cpu) rep = rep.detach().to(torch::kCPU);
  return rep;
}

torch::Tensor VICReg_4D::encode_projected(
  const torch::Tensor& data,
  const torch::Tensor& mask,
  bool use_swa,
  bool detach_to_cpu
) {
  auto z = encode(data, mask, use_swa, /*detach_to_cpu=*/false);
  _projector_net->eval();
  torch::NoGradGuard ng;
  auto p = _projector_net(z); // projector expects [B,T,E]
  if (detach_to_cpu) p = p.detach().to(torch::kCPU);
  return p;
}

// -------------------- save / load --------------------

void VICReg_4D::save(const std::string& path)
{
  torch::NoGradGuard nograd;

  const bool enc_was_training  = _encoder_net->is_training();
  const bool swa_was_training  = _swa_encoder_net->encoder().is_training();
  const bool proj_was_training = _projector_net->is_training();

  const torch::Device prev_device = device;
  const bool need_restore_device = !prev_device.is_cpu();
  if (need_restore_device) {
    this->to(torch::kCPU, dtype, /*non_blocking=*/false);
  }

  _encoder_net->eval();
  _swa_encoder_net->encoder().eval();
  _projector_net->eval();

  torch::serialize::OutputArchive root;

  { torch::serialize::OutputArchive a; _encoder_net->save(a); root.write("encoder_base", a); }
  { torch::serialize::OutputArchive a; _swa_encoder_net->encoder().save(a); root.write("encoder_swa", a); }
  { torch::serialize::OutputArchive a; _projector_net->save(a); root.write("projector", a); }

  if (optimizer) {
    torch::serialize::OutputArchive a; optimizer->save(a); root.write("adamw", a);
  }

  auto I64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/C",                       torch::tensor({C}, I64));
  root.write("meta/T",                       torch::tensor({T}, I64));
  root.write("meta/D",                       torch::tensor({D}, I64));
  root.write("meta/encoding_dims",           torch::tensor({encoding_dims}, I64));
  root.write("meta/channel_expansion_dim",   torch::tensor({channel_expansion_dim}, I64));
  root.write("meta/fused_feature_dim",       torch::tensor({fused_feature_dim}, I64));
  root.write("meta/encoder_hidden_dims",     torch::tensor({encoder_hidden_dims}, I64));
  root.write("meta/encoder_depth",           torch::tensor({encoder_depth}, I64));
  root.write("meta/optimizer_threshold_reset", torch::tensor({optimizer_threshold_reset}, I64));
  root.write("meta/enable_buffer_averaging", torch::tensor({static_cast<int64_t>(enable_buffer_averaging)}, I64));

  write_str(root, "meta/projector_mlp_spec", projector_mlp_spec);
  write_str(root, "meta/dtype",
            (dtype == torch::kFloat16 ? "f16" :
             dtype == torch::kFloat32 ? "f32" :
             dtype == torch::kFloat64 ? "f64" : "other"));
  write_str(root, "meta/device", "cpu");
  write_str(root, "meta/jk/component_name", cuwacunu::jkimyei::jk_setup(component_name, contract_hash).name);

  root.save_to(path);

  if (need_restore_device) {
    this->to(prev_device, dtype, /*non_blocking=*/false);
  }
  _encoder_net->train(enc_was_training);
  _swa_encoder_net->encoder().train(swa_was_training);
  _projector_net->train(proj_was_training);

  log_info("VICReg model saved to : %s \n", path.c_str());
}

void VICReg_4D::load(const std::string& path)
{
  torch::serialize::InputArchive root;
  root.load_from(path);

  // (re)build jk_component / optimizer from component_name if needed
  const auto component_name = read_str_strict(path, "meta/jk/component_name");
  if (!component_name.empty()) {
    if (!optimizer && cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_builder) {
      std::vector<at::Tensor> params;
      for (auto& p : this->parameters(/*recurse=*/true)) if (p.requires_grad()) params.push_back(p);
      optimizer = cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_builder->build(params);
    }
    if (cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sched_builder && optimizer) {
      lr_sched = cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sched_builder->build(*optimizer);
    }
    if (!loss_obj) {
      loss_obj = std::make_unique<VicRegLoss>(cuwacunu::jkimyei::jk_setup(component_name, contract_hash));
    }
  }

  {
    torch::serialize::InputArchive a; root.read("encoder_base", a); _encoder_net->load(a);
  }
  {
    torch::serialize::InputArchive a; root.read("encoder_swa", a); _swa_encoder_net->encoder().load(a);
  }
  {
    torch::serialize::InputArchive a; root.read("projector", a); _projector_net->load(a);
  }

  try {
    if (optimizer) {
      torch::serialize::InputArchive a; root.read("adamw", a);
      optimizer->load(a);
    }
  } catch (...) {
    log_warn("VICReg: optimizer state missing/incompatible; continuing without it.\n");
  }

  this->to(device, dtype, /*non_blocking=*/false);

  for (auto& p : this->parameters()) p.set_requires_grad(true);

  log_info("VICReg model loaded from : %s \n", path.c_str());
}

void VICReg_4D::load_jkimyei_training_policy(
    const cuwacunu::jkimyei::jk_component_t& jk_component) {
  TORCH_CHECK(
      !jk_component.resolved_component_id.empty(),
      "[VICReg_4D::load_jkimyei_training_policy] empty resolved_component_id for component '",
      component_name,
      "'");
  TORCH_CHECK(
      !jk_component.resolved_profile_id.empty(),
      "[VICReg_4D::load_jkimyei_training_policy] empty resolved_profile_id for component '",
      component_name,
      "'");
  TORCH_CHECK(
      !jk_component.resolved_profile_row_id.empty(),
      "[VICReg_4D::load_jkimyei_training_policy] empty resolved_profile_row_id for component '",
      component_name,
      "'");

  const auto component_row = jk_component.inst.retrive_row(
      "component_profiles_table",
      jk_component.resolved_profile_row_id);
  const auto gradient_row = jk_component.inst.retrive_row(
      "component_gradient_table",
      jk_component.resolved_profile_row_id);

  jk_vicreg_train = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(component_row, "vicreg_train"));
  jk_vicreg_use_swa = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(component_row, "vicreg_use_swa"));
  jk_vicreg_detach_to_cpu = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(component_row, "vicreg_detach_to_cpu"));
  jk_swa_start_iter = static_cast<int>(cuwacunu::camahjucunu::to_long(
      cuwacunu::camahjucunu::require_column(component_row, "swa_start_iter")));
  const int jk_optimizer_threshold_reset = static_cast<int>(cuwacunu::camahjucunu::to_long(
      cuwacunu::camahjucunu::require_column(component_row, "optimizer_threshold_reset")));
  if (optimizer_threshold_reset < 0) {
    optimizer_threshold_reset = jk_optimizer_threshold_reset;
  }

  const long accumulate_steps = cuwacunu::camahjucunu::to_long(
      cuwacunu::camahjucunu::require_column(gradient_row, "accumulate_steps"));
  TORCH_CHECK(
      accumulate_steps >= 1,
      "[VICReg_4D::load_jkimyei_training_policy] accumulate_steps must be >= 1");
  jk_accumulate_steps = static_cast<int>(accumulate_steps);

  jk_clip_norm = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(gradient_row, "clip_norm"));
  jk_clip_value = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(gradient_row, "clip_value"));
  TORCH_CHECK(
      jk_clip_norm >= 0.0,
      "[VICReg_4D::load_jkimyei_training_policy] clip_norm must be >= 0");
  TORCH_CHECK(
      jk_clip_value >= 0.0,
      "[VICReg_4D::load_jkimyei_training_policy] clip_value must be >= 0");

  jk_skip_on_nan = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(gradient_row, "skip_on_nan"));
  jk_zero_grad_set_to_none = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(gradient_row, "zero_grad_set_to_none"));
}

// -------------------- display --------------------

void VICReg_4D::display_model() const {
  const char* dtype_str =
    (dtype == torch::kInt8 ? "kInt8" :
    dtype == torch::kInt16 ? "kInt16" :
    dtype == torch::kInt32 ? "kInt32" :
    dtype == torch::kInt64 ? "kInt64" :
    dtype == torch::kFloat32 ? "Float32" :
    dtype == torch::kFloat16 ? "Float16" :
    dtype == torch::kFloat64 ? "Float64" : "Unknown");

  std::string dev = device.str();
  const char* device_str = dev.c_str();
  const char* mlp_spec_str = projector_mlp_spec.c_str();
  const char* swa_str = enable_buffer_averaging ? "true" : "false";

  // Read projector options as strings for display
  using cuwacunu::piaabo::dconfig::config_space_t;
  std::string norm_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_norm");
  std::string act_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_activation");
  std::string hbias_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_hidden_bias");
  std::string lbias_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_last_bias");
  std::string bnfp32_s = contract_space_t::get<std::string>(
      contract_hash, "VICReg", "projector_bn_in_fp32");

  const char* fmt =
    "\n%s \t[Representation Learning] VICReg_4D:  %s\n"
    "\t\t%s%-25s%s %s%-8s%s\n"    // component_name
    "\t\t%s%-25s%s %s%-8d%s\n"    // C
    "\t\t%s%-25s%s %s%-8d%s\n"    // T
    "\t\t%s%-25s%s %s%-8d%s\n"    // D
    "\t\t%s%-25s%s %s%-8s%s\n"    // Optimizer (string)
    "\t\t%s%-25s%s %s%-8s%s\n"    // LR schedule (string)
    "\t\t%s%-25s%s %s%-8s%s\n"    // Loss (string)
    "\t\t%s%-25s%s    %s%-8.4f%s\n"   // sim_coeff
    "\t\t%s%-25s%s    %s%-8.4f%s\n"   // std_coeff
    "\t\t%s%-25s%s    %s%-8.4f%s\n"   // cov_coeff
    "\t\t%s%-25s%s %s%-8d%s\n"    // encoding_dims
    "\t\t%s%-25s%s %s%-8d%s\n"    // channel_expansion_dim
    "\t\t%s%-25s%s %s%-8d%s\n"    // fused_feature_dim
    "\t\t%s%-25s%s %s%-8d%s\n"    // encoder_hidden_dims
    "\t\t%s%-25s%s %s%-8d%s\n"    // encoder_depth
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_mlp_spec
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_norm
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_activation
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_hidden_bias
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_last_bias
    "\t\t%s%-25s%s %s%-8s%s\n"    // projector_bn_in_fp32
    "\t\t%s%-25s%s %s%-8s%s\n"    // dtype
    "\t\t%s%-25s%s %s%-8s%s\n"    // device
    "\t\t%s%-25s%s %s%-8d%s\n"    // optimizer_threshold_reset
    "\t\t%s%-25s%s %s%-8s%s\n";   // SWA buffer avg

  log_info(fmt,
    ANSI_COLOR_Dim_Green,   ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Component:",                 ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  component_name.c_str(),         ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Channels  (C):",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  C,                              ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Timesteps (T):",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  T,                              ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Features  (D):",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  D,                              ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Optimizer:",                 ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_conf.id.c_str(),   ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "LR Scheduler:",              ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sch_conf.id.c_str(),   ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Loss:",                      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  cuwacunu::jkimyei::jk_setup(component_name, contract_hash).loss_conf.id.c_str(),  ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "    - Sim coeff (λ₁):",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  loss_obj->sim_coeff_,           ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "    - Std coeff (λ₂):",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  loss_obj->std_coeff_,           ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "    - Cov coeff (λ₃):",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  loss_obj->cov_coeff_,           ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Encoding dims:",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoding_dims,                  ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Channel expansion:",         ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  channel_expansion_dim,          ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Fused feature dim:",         ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  fused_feature_dim,              ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Encoder hidden dims:",       ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoder_hidden_dims,            ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Encoder depth:",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  encoder_depth,                  ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Proj MLP spec:",             ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  mlp_spec_str,                   ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector norm:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  norm_s.c_str(),                 ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector activation:",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  act_s.c_str(),                  ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector hidden bias:",     ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  hbias_s.c_str(),                ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector last bias:",       ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  lbias_s.c_str(),                ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector BN in FP32:",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  bnfp32_s.c_str(),               ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Data type:",                 ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  dtype_str,                      ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Device:",                    ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  device_str,                     ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Optimizer threshold reset:", ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_threshold_reset,      ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "SWA buffer avg:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  swa_str,                        ANSI_COLOR_RESET
  );
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
