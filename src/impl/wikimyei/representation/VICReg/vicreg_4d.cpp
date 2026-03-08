/* vicreg_4d.cpp */
#include <torch/torch.h>
#include <torch/nn/utils/clip_grad.h>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <string>
#include <string_view>

#include <torch/optim/adagrad.h>
#include <torch/optim/adam.h>
#include <torch/optim/adamw.h>
#include <torch/optim/rmsprop.h>
#include <torch/optim/sgd.h>

#include "wikimyei/representation/VICReg/vicreg_4d.h"
#include "piaabo/dconfig.h"
#include "piaabo/torch_compat/network_design_adapter.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "wikimyei/representation/VICReg/vicreg_4d_network_design.h"
#include "jkimyei/training_setup/jk_setup.h"

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .cpp
// ------------------------------------------------------------
DEV_WARNING("[vicreg_4d.cpp] In train+emit execution paths, representation payload encode plus VICReg two-view training currently implies ~2 encoder forwards per batch (payload + batched[aug1,aug2]).\n");
DEV_WARNING("[vicreg_4d.cpp] Future optimization candidate: stack [payload,aug1,aug2] in one encoder call when runtime semantics (SWA/eval-vs-train parity and deterministic emit policy) allow it.\n");

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

using cuwacunu::iitepi::contract_space_t;

// -------------------- local helpers (strict ckpt meta & config parsers) --------------------
namespace {

inline torch::Tensor read_tensor_strict(const std::string& path, const char* key) {
  // Always map archived tensors to CPU first so loading remains portable when
  // artifacts were produced on CUDA hosts.
  torch::serialize::InputArchive ar;
  ar.load_from(path, torch::Device(torch::kCPU));
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

inline torch::Tensor move_state_tensor_to_device(torch::Tensor t,
                                                 const torch::Device& dev) {
  if (!t.defined()) return t;
  if (t.device() == dev) return t;
  return t.to(dev, /*non_blocking=*/false);
}

inline void move_optimizer_state_to_device(torch::optim::Optimizer* optimizer,
                                           const torch::Device& dev) {
  if (!optimizer) return;
  for (auto& kv : optimizer->state()) {
    if (!kv.second) continue;

    if (auto* st = dynamic_cast<torch::optim::AdamWParamState*>(kv.second.get())) {
      st->exp_avg(move_state_tensor_to_device(st->exp_avg(), dev));
      st->exp_avg_sq(move_state_tensor_to_device(st->exp_avg_sq(), dev));
      st->max_exp_avg_sq(move_state_tensor_to_device(st->max_exp_avg_sq(), dev));
      continue;
    }
    if (auto* st = dynamic_cast<torch::optim::AdamParamState*>(kv.second.get())) {
      st->exp_avg(move_state_tensor_to_device(st->exp_avg(), dev));
      st->exp_avg_sq(move_state_tensor_to_device(st->exp_avg_sq(), dev));
      st->max_exp_avg_sq(move_state_tensor_to_device(st->max_exp_avg_sq(), dev));
      continue;
    }
    if (auto* st = dynamic_cast<torch::optim::RMSpropParamState*>(kv.second.get())) {
      st->square_avg(move_state_tensor_to_device(st->square_avg(), dev));
      st->momentum_buffer(move_state_tensor_to_device(st->momentum_buffer(), dev));
      st->grad_avg(move_state_tensor_to_device(st->grad_avg(), dev));
      continue;
    }
    if (auto* st = dynamic_cast<torch::optim::SGDParamState*>(kv.second.get())) {
      st->momentum_buffer(move_state_tensor_to_device(st->momentum_buffer(), dev));
      continue;
    }
    if (auto* st = dynamic_cast<torch::optim::AdagradParamState*>(kv.second.get())) {
      st->sum(move_state_tensor_to_device(st->sum(), dev));
      continue;
    }
  }
}

inline std::string trim_ascii_copy(std::string s) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
  while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) s.pop_back();
  return s;
}

cuwacunu::camahjucunu::jkimyei_specs_t::table_t select_vicreg_augmentations_for_profile(
    const cuwacunu::jkimyei::jk_component_t& jk_component) {
  using table_t = cuwacunu::camahjucunu::jkimyei_specs_t::table_t;
  table_t selected{};

  const auto table_it = jk_component.inst.tables.find("vicreg_augmentations");
  if (table_it == jk_component.inst.tables.end()) return selected;

  const std::string target_profile_row_id =
      trim_ascii_copy(jk_component.resolved_profile_row_id);
  const std::string target_component_id =
      trim_ascii_copy(jk_component.resolved_component_id);
  const std::string target_profile_id =
      trim_ascii_copy(jk_component.resolved_profile_id);

  const auto matches_profile_row_id = [&](const cuwacunu::camahjucunu::jkimyei_specs_t::row_t& row) {
    const auto it = row.find("profile_row_id");
    return it != row.end() &&
           trim_ascii_copy(it->second) == target_profile_row_id;
  };
  for (const auto& row : table_it->second) {
    if (matches_profile_row_id(row)) selected.push_back(row);
  }
  if (!selected.empty()) return selected;

  const auto matches_component_and_profile =
      [&](const cuwacunu::camahjucunu::jkimyei_specs_t::row_t& row) {
        const auto cid_it = row.find("component_id");
        const auto pid_it = row.find("profile_id");
        if (cid_it == row.end() || pid_it == row.end()) return false;
        return trim_ascii_copy(cid_it->second) == target_component_id &&
               trim_ascii_copy(pid_it->second) == target_profile_id;
      };
  for (const auto& row : table_it->second) {
    if (matches_component_and_profile(row)) selected.push_back(row);
  }
  if (!selected.empty()) return selected;

  throw std::runtime_error(
      "[VICReg_4D::select_vicreg_augmentations_for_profile] augmentations not found for component='" +
      target_component_id + "' profile='" + target_profile_id + "' row_id='" +
      target_profile_row_id + "'");
}

[[nodiscard]] bool has_non_ws_ascii(std::string_view text) {
  for (const char c : text) {
    if (!std::isspace(static_cast<unsigned char>(c))) return true;
  }
  return false;
}

[[nodiscard]] std::optional<std::string> read_str_optional(
    const std::string& path,
    const char* key) {
  torch::serialize::InputArchive ar;
  ar.load_from(path, torch::Device(torch::kCPU));
  torch::Tensor t;
  if (!ar.try_read(key, t)) return std::nullopt;
  t = t.to(torch::kCPU);
  std::string s;
  s.resize(static_cast<std::size_t>(t.numel()));
  if (t.numel() > 0) {
    std::memcpy(s.data(), t.data_ptr<int8_t>(), static_cast<std::size_t>(t.numel()));
  }
  return s;
}

[[nodiscard]] std::string norm_kind_to_string(NormKind kind) {
  switch (kind) {
    case NormKind::BatchNorm1d: return "BatchNorm1d";
    case NormKind::LayerNorm:   return "LayerNorm";
    case NormKind::None:        return "None";
    default:                    return "Unknown";
  }
}

[[nodiscard]] std::string act_kind_to_string(ActKind kind) {
  switch (kind) {
    case ActKind::ReLU: return "ReLU";
    case ActKind::SiLU: return "SiLU";
    default:            return "Unknown";
  }
}

[[nodiscard]] const char* bool_to_string(bool value) noexcept {
  return value ? "true" : "false";
}

[[nodiscard]] std::optional<ProjectorOptions>
read_projector_options_override_from_checkpoint(const std::string& path) {
  const auto norm =
      read_str_optional(path, "meta/projector_norm");
  const auto activation =
      read_str_optional(path, "meta/projector_activation");
  const auto hidden_bias =
      read_str_optional(path, "meta/projector_hidden_bias");
  const auto last_bias =
      read_str_optional(path, "meta/projector_last_bias");
  const auto bn_in_fp32 =
      read_str_optional(path, "meta/projector_bn_in_fp32");

  const bool has_any = norm.has_value() || activation.has_value() ||
                       hidden_bias.has_value() || last_bias.has_value() ||
                       bn_in_fp32.has_value();
  if (!has_any) return std::nullopt;
  if (!norm.has_value() || !activation.has_value() || !hidden_bias.has_value() ||
      !last_bias.has_value() || !bn_in_fp32.has_value()) {
    throw std::runtime_error(
        "[VICReg] checkpoint projector option metadata is partial; expected all "
        "of meta/projector_norm, meta/projector_activation, "
        "meta/projector_hidden_bias, meta/projector_last_bias, "
        "meta/projector_bn_in_fp32");
  }

  return ProjectorOptions{
      /* norm_kind       */ parse_norm_kind_strict(*norm),
      /* act_kind        */ parse_act_kind_strict(*activation),
      /* use_hidden_bias */ parse_bool_strict(*hidden_bias),
      /* use_last_bias   */ parse_bool_strict(*last_bias),
      /* bn_in_fp32      */ parse_bool_strict(*bn_in_fp32),
  };
}

struct resolved_network_design_t {
  vicreg_network_design_spec_t semantic{};
  cuwacunu::piaabo::torch_compat::vicreg_torch_adapter_spec_t torch_adapter{};
};

[[nodiscard]] resolved_network_design_t resolve_vicreg_network_design_or_throw(
    const cuwacunu::camahjucunu::network_design_instruction_t& ir,
    int C,
    int T,
    int D,
    const std::string& context) {
  resolved_network_design_t out{};
  std::string semantic_error;
  if (!resolve_vicreg_network_design(ir, &out.semantic, &semantic_error)) {
    throw std::runtime_error(
        context + " invalid VICReg network_design payload: " + semantic_error);
  }
  if (out.semantic.C != C || out.semantic.T != T || out.semantic.D != D) {
    throw std::runtime_error(
        context + " network_design INPUT shape mismatch: expected C/T/D=(" +
        std::to_string(C) + "," + std::to_string(T) + "," + std::to_string(D) +
        "), got (" + std::to_string(out.semantic.C) + "," +
        std::to_string(out.semantic.T) + "," + std::to_string(out.semantic.D) +
        ")");
  }

  std::string adapter_error;
  if (!cuwacunu::piaabo::torch_compat::build_vicreg_torch_adapter_spec(
          out.semantic,
          &out.torch_adapter,
          &adapter_error)) {
    throw std::runtime_error(
        context + " cannot map VICReg network_design to torch adapter: " +
        adapter_error);
  }
  return out;
}

VICReg_4D::init_from_contract_t resolve_init_from_contract_or_throw(
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    int C,
    int T,
    int D) {
  auto snapshot = contract_space_t::contract_itself(contract_hash);
  VICReg_4D::init_from_contract_t out{};
  out.encoding_dims = snapshot->get<int>("VICReg", "encoding_dims");
  out.channel_expansion_dim =
      snapshot->get<int>("VICReg", "channel_expansion_dim");
  out.fused_feature_dim = snapshot->get<int>("VICReg", "fused_feature_dim");
  out.encoder_hidden_dims = snapshot->get<int>("VICReg", "encoder_hidden_dims");
  out.encoder_depth = snapshot->get<int>("VICReg", "encoder_depth");
  out.projector_mlp_spec = snapshot->get<std::string>("VICReg", "projector_mlp_spec");
  out.dtype = cuwacunu::iitepi::config_dtype(contract_hash, "VICReg");
  out.device = cuwacunu::iitepi::config_device(contract_hash, "VICReg");
  out.optimizer_threshold_reset = -1;
  out.enable_buffer_averaging =
      snapshot->get<bool>("VICReg", "enable_buffer_averaging");
  if (!snapshot->vicreg_network_design.has_payload()) return out;

  const auto& ir = snapshot->vicreg_network_design.decoded();
  const auto resolved_design = resolve_vicreg_network_design_or_throw(
      ir, C, T, D, "[VICReg_4D]");
  const auto& semantic = resolved_design.semantic;

  out.encoding_dims = semantic.encoding_dims;
  out.channel_expansion_dim = semantic.channel_expansion_dim;
  out.fused_feature_dim = semantic.fused_feature_dim;
  out.encoder_hidden_dims = semantic.encoder_hidden_dims;
  out.encoder_depth = semantic.encoder_depth;
  out.projector_mlp_spec = resolved_design.torch_adapter.projector_mlp_spec;
  out.projector_options_override = resolved_design.torch_adapter.projector_options;
  out.network_design_network_id = semantic.network_id;
  out.network_design_join_policy = semantic.join_policy;
  out.network_design_grammar = snapshot->vicreg_network_design.grammar;
  out.network_design_dsl = snapshot->vicreg_network_design.dsl;

  log_dbg(
      "[VICReg_4D] network_design loaded for `%s` (join_policy=%s)\n",
      semantic.network_id.c_str(),
      semantic.join_policy.c_str());
  return out;
}

} // anon

// -------------------- constructors --------------------

VICReg_4D::VICReg_4D(
  const cuwacunu::iitepi::contract_hash_t& contract_hash_,
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
  bool enable_buffer_averaging_,
  std::optional<ProjectorOptions> projector_options_override_,
  std::string network_design_network_id_,
  std::string network_design_join_policy_,
  std::string network_design_grammar_,
  std::string network_design_dsl_
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
  network_design_network_id(std::move(network_design_network_id_)),
  network_design_join_policy(std::move(network_design_join_policy_)),
  network_design_grammar(std::move(network_design_grammar_)),
  network_design_dsl(std::move(network_design_dsl_)),

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
  aug(select_vicreg_augmentations_for_profile(
      cuwacunu::jkimyei::jk_setup(component_name_, contract_hash_))),
  trainable_params_(),
  optimizer(nullptr),
  lr_sched(nullptr),
  loss_obj(nullptr)
{
  ProjectorOptions popts{};
  if (projector_options_override_.has_value()) {
    popts = *projector_options_override_;
  } else {
    // ---- Read projector options from contract config and cast strictly ----
    auto norm_s =
        contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "projector_norm");
    auto act_s = contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "projector_activation");
    auto hbias_s = contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "projector_hidden_bias");
    auto lbias_s = contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "projector_last_bias");
    auto bnfp32_s = contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "projector_bn_in_fp32");

    popts = ProjectorOptions{
      /* norm_kind       */ parse_norm_kind_strict(norm_s),
      /* act_kind        */ parse_act_kind_strict(act_s),
      /* use_hidden_bias */ parse_bool_strict(hbias_s),
      /* use_last_bias   */ parse_bool_strict(lbias_s),
      /* bn_in_fp32      */ parse_bool_strict(bnfp32_s)
    };
  }
  effective_projector_options = popts;

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
  const cuwacunu::iitepi::contract_hash_t& contract_hash_,
  const std::string& component_name_,
  int C_, int T_, int D_,
  init_from_contract_t resolved_
)
: VICReg_4D(
    contract_hash_,
    component_name_,
    C_, T_, D_,
    resolved_.encoding_dims,
    resolved_.channel_expansion_dim,
    resolved_.fused_feature_dim,
    resolved_.encoder_hidden_dims,
    resolved_.encoder_depth,
    std::move(resolved_.projector_mlp_spec),
    resolved_.dtype,
    resolved_.device,
    resolved_.optimizer_threshold_reset,
    resolved_.enable_buffer_averaging,
    std::move(resolved_.projector_options_override),
    std::move(resolved_.network_design_network_id),
    std::move(resolved_.network_design_join_policy),
    std::move(resolved_.network_design_grammar),
    std::move(resolved_.network_design_dsl)
) {}

VICReg_4D::VICReg_4D(
  const cuwacunu::iitepi::contract_hash_t& contract_hash_,
  const std::string& component_name_, 
  int C_, int T_, int D_
)
: VICReg_4D(
    contract_hash_,
    component_name_,
    C_, T_, D_,
    resolve_init_from_contract_or_throw(contract_hash_, C_, T_, D_)
) {
  log_info("Initialized VICReg encoder from Configuration file...\n");
}

VICReg_4D::VICReg_4D(
                     const cuwacunu::iitepi::contract_hash_t& contract_hash_,
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
    static_cast<bool>(read_i64_strict(checkpoint_path, "meta/enable_buffer_averaging")),
    read_projector_options_override_from_checkpoint(checkpoint_path)
) {
  this->load(checkpoint_path);
}

void VICReg_4D::reset_runtime_training_state() {
  clear_pending_runtime_state_(/*reset_optimizer_steps=*/true);
}

void VICReg_4D::clear_pending_runtime_state_(bool reset_optimizer_steps) {
  if (optimizer) optimizer->zero_grad(training_policy.zero_grad_set_to_none);
  runtime_state.accum_counter = 0;
  runtime_state.has_pending_grad = false;
  runtime_state.pending_loss_sum = 0.0;
  runtime_state.pending_loss_count = 0;
  runtime_state.last_committed_loss_mean = 0.0;
  if (reset_optimizer_steps) runtime_state.optimizer_steps = 0;
}

std::vector<torch::Tensor> VICReg_4D::params_with_grad_() const {
  std::vector<torch::Tensor> out;
  out.reserve(trainable_params_.size());
  for (const auto& p : trainable_params_) {
    if (p.grad().defined()) out.push_back(p);
  }
  return out;
}

void VICReg_4D::apply_gradient_clipping_(
    const std::vector<torch::Tensor>& params_with_grad) const {
  if (params_with_grad.empty()) return;
  if (training_policy.clip_value > 0.0) {
    torch::nn::utils::clip_grad_value_(params_with_grad, training_policy.clip_value);
  }
  if (training_policy.clip_norm > 0.0) {
    torch::nn::utils::clip_grad_norm_(params_with_grad, training_policy.clip_norm);
  } else {
    const double clip_max = (runtime_state.optimizer_steps < 1500) ? 5.0 : 1.0;
    torch::nn::utils::clip_grad_norm_(params_with_grad, clip_max);
  }
}

bool VICReg_4D::grads_are_finite_(
    const std::vector<torch::Tensor>& params_with_grad) const {
  for (const auto& p : params_with_grad) {
    const auto g = p.grad();
    if (!g.defined()) continue;
    if (!torch::isfinite(g).all().template item<bool>()) return false;
  }
  return true;
}

bool VICReg_4D::commit_pending_training_step_(int swa_start_iter) {
  if (!runtime_state.has_pending_grad || runtime_state.accum_counter <= 0) return false;

  const auto params_with_grad = params_with_grad_();
  apply_gradient_clipping_(params_with_grad);

  const bool grad_is_finite = grads_are_finite_(params_with_grad);
  if (!grad_is_finite) {
    if (training_policy.skip_on_nan) {
      clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);
      return false;
    }
    TORCH_CHECK(
        false,
        "[VICReg_4D::commit_pending_training_step_] non-finite gradients detected and skip_on_nan=false");
  }

  optimizer->step();
  if (lr_sched &&
      lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
    lr_sched->step();
  }

  const int effective_swa_start_iter =
      (swa_start_iter >= 0) ? swa_start_iter : training_policy.swa_start_iter;
  if (training_policy.vicreg_use_swa && runtime_state.optimizer_steps >= effective_swa_start_iter) {
    _swa_encoder_net->update_parameters(_encoder_net);
  }

  runtime_state.last_committed_loss_mean =
      (runtime_state.pending_loss_count > 0)
          ? (runtime_state.pending_loss_sum /
             static_cast<double>(runtime_state.pending_loss_count))
          : 0.0;
  runtime_state.pending_loss_sum = 0.0;
  runtime_state.pending_loss_count = 0;
  runtime_state.accum_counter = 0;
  runtime_state.has_pending_grad = false;
  ++runtime_state.optimizer_steps;
  return true;
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

  if (!runtime_state.has_pending_grad) {
    optimizer->zero_grad(training_policy.zero_grad_set_to_none);
  }

  _encoder_net->train();
  _projector_net->train();
  _swa_encoder_net->encoder().train();

  auto data_d = data.to(device);
  auto mask_d = mask.to(device);

  auto [d1, m1] = aug.augment(data_d, mask_d);
  auto [d2, m2] = aug.augment(data_d, mask_d);

  if (verbose && (runtime_state.optimizer_steps % 100 == 0)) {
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

  // Low-risk optimization: execute both augmented views in one encoder forward,
  // then split back to preserve downstream VICReg semantics.
  auto d12 = torch::cat({d1, d2}, /*dim=*/0);
  auto m12 = torch::cat({m1, m2}, /*dim=*/0);
  auto k12 = _encoder_net(d12, m12);
  const auto b = d1.size(0);
  auto k1 = k12.narrow(/*dim=*/0, /*start=*/0, /*length=*/b);
  auto k2 = k12.narrow(/*dim=*/0, /*start=*/b, /*length=*/b);

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
  const double cov_boost = (runtime_state.optimizer_steps < cov_ramp_iters)
      ? (3.0 - 2.0 * (static_cast<double>(runtime_state.optimizer_steps) /
                      static_cast<double>(cov_ramp_iters)))
      : 1.0;
  auto loss = loss_obj->sim_coeff_ * terms.inv +
              loss_obj->std_coeff_ * terms.var +
              (loss_obj->cov_coeff_ * cov_boost) * terms.cov;

  const double loss_scalar = loss.template item<double>();
  if (verbose && (runtime_state.optimizer_steps % 500 == 0)) {
    log_info("[loss] optim=%.6f inv=%.6f var=%.6f cov=%.6f (cov_boost=%.2f)\n",
             loss_scalar,
             terms.inv.template item<double>(),
             terms.var.template item<double>(),
             terms.cov.template item<double>(),
             cov_boost);
  }

  if (!std::isfinite(loss_scalar)) {
    if (training_policy.skip_on_nan) {
      clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);
      result.skipped = true;
      return result;
    }
    TORCH_CHECK(
        false,
        "[VICReg_4D::train_one_batch] non-finite loss detected and skip_on_nan=false");
  }

  runtime_state.pending_loss_sum += loss_scalar;
  ++runtime_state.pending_loss_count;

  auto backprop_loss = (training_policy.accumulate_steps > 1)
      ? (loss / static_cast<double>(training_policy.accumulate_steps))
      : loss;
  backprop_loss.backward();
  runtime_state.has_pending_grad = true;
  ++runtime_state.accum_counter;

  result.loss = loss.detach().to(torch::kCPU);

  if (runtime_state.accum_counter < std::max(1, training_policy.accumulate_steps)) {
    return result;
  }

  result.optimizer_step_applied = commit_pending_training_step_(swa_start_iter);
  if (!result.optimizer_step_applied) result.skipped = true;
  return result;
}

bool VICReg_4D::finalize_pending_training_step(int swa_start_iter) {
  return commit_pending_training_step_(swa_start_iter);
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
  write_str(root, "meta/projector_norm",
            norm_kind_to_string(effective_projector_options.norm_kind));
  write_str(root, "meta/projector_activation",
            act_kind_to_string(effective_projector_options.act_kind));
  write_str(root, "meta/projector_hidden_bias",
            bool_to_string(effective_projector_options.use_hidden_bias));
  write_str(root, "meta/projector_last_bias",
            bool_to_string(effective_projector_options.use_last_bias));
  write_str(root, "meta/projector_bn_in_fp32",
            bool_to_string(effective_projector_options.bn_in_fp32));
  write_str(root, "meta/network_design_network_id", network_design_network_id);
  write_str(root, "meta/network_design_join_policy", network_design_join_policy);
  write_str(root, "meta/network_design_grammar", network_design_grammar);
  write_str(root, "meta/network_design_dsl", network_design_dsl);
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
  // Force CPU map-location on archive load to avoid CUDA runtime dependency
  // when checkpoint tensors were serialized from GPU.
  root.load_from(path, torch::Device(torch::kCPU));

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
  move_optimizer_state_to_device(optimizer.get(), device);

  if (const auto popts =
          read_projector_options_override_from_checkpoint(path);
      popts.has_value()) {
    effective_projector_options = *popts;
  }

  const auto checkpoint_network_design_dsl =
      read_str_optional(path, "meta/network_design_dsl");
  if (checkpoint_network_design_dsl.has_value() &&
      has_non_ws_ascii(*checkpoint_network_design_dsl)) {
    const auto checkpoint_network_design_grammar =
        read_str_optional(path, "meta/network_design_grammar");
    TORCH_CHECK(
        checkpoint_network_design_grammar.has_value() &&
            has_non_ws_ascii(*checkpoint_network_design_grammar),
        "[VICReg_4D::load] checkpoint contains meta/network_design_dsl but missing "
        "meta/network_design_grammar");
    const auto ir = cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
        *checkpoint_network_design_grammar,
        *checkpoint_network_design_dsl);
    const auto resolved_design = resolve_vicreg_network_design_or_throw(
        ir, C, T, D, "[VICReg_4D::load]");
    const auto& semantic = resolved_design.semantic;
    const auto& adapter = resolved_design.torch_adapter;
    TORCH_CHECK(
        semantic.encoding_dims == encoding_dims,
        "[VICReg_4D::load] network_design encoding_dims mismatch with checkpoint "
        "metadata/model");
    TORCH_CHECK(
        semantic.channel_expansion_dim == channel_expansion_dim,
        "[VICReg_4D::load] network_design channel_expansion_dim mismatch with "
        "checkpoint metadata/model");
    TORCH_CHECK(
        semantic.fused_feature_dim == fused_feature_dim,
        "[VICReg_4D::load] network_design fused_feature_dim mismatch with checkpoint "
        "metadata/model");
    TORCH_CHECK(
        semantic.encoder_hidden_dims == encoder_hidden_dims,
        "[VICReg_4D::load] network_design encoder_hidden_dims mismatch with checkpoint "
        "metadata/model");
    TORCH_CHECK(
        semantic.encoder_depth == encoder_depth,
        "[VICReg_4D::load] network_design encoder_depth mismatch with checkpoint "
        "metadata/model");
    TORCH_CHECK(
        adapter.projector_mlp_spec == projector_mlp_spec,
        "[VICReg_4D::load] network_design projector_mlp_spec mismatch with checkpoint "
        "metadata/model");
    TORCH_CHECK(
        adapter.projector_options.norm_kind ==
            effective_projector_options.norm_kind &&
            adapter.projector_options.act_kind ==
                effective_projector_options.act_kind &&
            adapter.projector_options.use_hidden_bias ==
                effective_projector_options.use_hidden_bias &&
            adapter.projector_options.use_last_bias ==
                effective_projector_options.use_last_bias &&
            adapter.projector_options.bn_in_fp32 ==
                effective_projector_options.bn_in_fp32,
        "[VICReg_4D::load] network_design projector options mismatch with "
        "checkpoint metadata/model");
    const bool checkpoint_has_analytics_policy =
        !ir.find_nodes_by_kind("NETWORK_ANALYTICS_POLICY").empty();
    bool runtime_has_analytics_policy = false;
    if (has_non_ws_ascii(network_design_grammar) &&
        has_non_ws_ascii(network_design_dsl)) {
      try {
        const auto runtime_ir =
            cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
                network_design_grammar, network_design_dsl);
        runtime_has_analytics_policy =
            !runtime_ir.find_nodes_by_kind("NETWORK_ANALYTICS_POLICY").empty();
      } catch (...) {
        runtime_has_analytics_policy = false;
      }
    }

    if (!checkpoint_has_analytics_policy && runtime_has_analytics_policy) {
      network_design_network_id = semantic.network_id;
      network_design_join_policy = semantic.join_policy;
      log_warn(
          "[VICReg_4D::load] checkpoint network_design lacks "
          "NETWORK_ANALYTICS_POLICY; preserving runtime contract "
          "network_design for analytics sidecars\n");
      log_info(
          "[VICReg_4D::load] validated checkpoint network_design `%s` "
          "(join_policy=%s)\n",
          network_design_network_id.c_str(),
          network_design_join_policy.c_str());
    } else {
      network_design_network_id = semantic.network_id;
      network_design_join_policy = semantic.join_policy;
      network_design_grammar = *checkpoint_network_design_grammar;
      network_design_dsl = *checkpoint_network_design_dsl;
      log_info(
          "[VICReg_4D::load] validated checkpoint network_design `%s` "
          "(join_policy=%s)\n",
          network_design_network_id.c_str(),
          network_design_join_policy.c_str());
    }
  } else {
    if (!has_non_ws_ascii(network_design_grammar) ||
        !has_non_ws_ascii(network_design_dsl)) {
      network_design_network_id.clear();
      network_design_join_policy.clear();
      network_design_grammar.clear();
      network_design_dsl.clear();
    } else {
      log_warn(
          "[VICReg_4D::load] checkpoint missing network_design metadata; "
          "preserving runtime contract network_design payload\n");
    }
  }

  for (auto& p : this->parameters()) p.set_requires_grad(true);

  log_info("VICReg model loaded from : %s \n", path.c_str());
}

void VICReg_4D::load_jkimyei_training_policy(
    const cuwacunu::jkimyei::jk_component_t& jk_component) {
  const auto trim_ascii_copy = [](std::string s) -> std::string {
    auto is_space = [](char ch) {
      return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    };
    std::size_t begin = 0;
    while (begin < s.size() && is_space(s[begin])) ++begin;
    std::size_t end = s.size();
    while (end > begin && is_space(s[end - 1])) --end;
    return s.substr(begin, end - begin);
  };
  const auto find_row_by_id = [&](const std::string& table_name,
                                  const std::string& row_id)
      -> const cuwacunu::camahjucunu::jkimyei_specs_t::row_t* {
    const auto table_it = jk_component.inst.tables.find(table_name);
    if (table_it == jk_component.inst.tables.end()) return nullptr;
    for (const auto& row : table_it->second) {
      const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
      if (rid_it == row.end()) continue;
      if (trim_ascii_copy(rid_it->second) == trim_ascii_copy(row_id)) return &row;
    }
    return nullptr;
  };
  const auto find_component_profile_row =
      [&]() -> const cuwacunu::camahjucunu::jkimyei_specs_t::row_t* {
    if (const auto* by_id = find_row_by_id("component_profiles_table",
                                           jk_component.resolved_profile_row_id)) {
      return by_id;
    }
    const auto table_it = jk_component.inst.tables.find("component_profiles_table");
    if (table_it == jk_component.inst.tables.end()) return nullptr;

    const std::string target_component = trim_ascii_copy(jk_component.resolved_component_id);
    const std::string target_profile = trim_ascii_copy(jk_component.resolved_profile_id);
    for (const auto& row : table_it->second) {
      const auto profile_it = row.find("profile_id");
      if (profile_it == row.end()) continue;
      if (trim_ascii_copy(profile_it->second) != target_profile) continue;
      const auto component_it = row.find("component_id");
      const auto type_it = row.find("component_type");
      const std::string component_value =
          (component_it == row.end()) ? std::string{} : trim_ascii_copy(component_it->second);
      const std::string type_value =
          (type_it == row.end()) ? std::string{} : trim_ascii_copy(type_it->second);
      if (component_value == target_component || type_value == target_component) {
        return &row;
      }
    }
    return nullptr;
  };

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
  const auto* component_row_ptr = find_component_profile_row();
  TORCH_CHECK(
      component_row_ptr != nullptr,
      "[VICReg_4D::load_jkimyei_training_policy] component profile row not found for component='",
      jk_component.resolved_component_id,
      "' profile='",
      jk_component.resolved_profile_id,
      "' requested_row_id='",
      jk_component.resolved_profile_row_id,
      "'");
  const auto profile_row_id = cuwacunu::camahjucunu::require_column(
      *component_row_ptr, ROW_ID_COLUMN_HEADER);
  const auto* gradient_row_ptr =
      find_row_by_id("component_gradient_table", profile_row_id);
  TORCH_CHECK(
      gradient_row_ptr != nullptr,
      "[VICReg_4D::load_jkimyei_training_policy] gradient row not found for row_id='",
      profile_row_id,
      "'");

  const auto& component_row = *component_row_ptr;
  const auto& gradient_row = *gradient_row_ptr;

  training_policy.vicreg_use_swa = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(component_row, "vicreg_use_swa"));
  training_policy.vicreg_detach_to_cpu = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(component_row, "vicreg_detach_to_cpu"));
  training_policy.swa_start_iter = static_cast<int>(cuwacunu::camahjucunu::to_long(
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
  training_policy.accumulate_steps = static_cast<int>(accumulate_steps);

  training_policy.clip_norm = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(gradient_row, "clip_norm"));
  training_policy.clip_value = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(gradient_row, "clip_value"));
  TORCH_CHECK(
      training_policy.clip_norm >= 0.0,
      "[VICReg_4D::load_jkimyei_training_policy] clip_norm must be >= 0");
  TORCH_CHECK(
      training_policy.clip_value >= 0.0,
      "[VICReg_4D::load_jkimyei_training_policy] clip_value must be >= 0");

  training_policy.skip_on_nan = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(gradient_row, "skip_on_nan"));
  training_policy.zero_grad_set_to_none = cuwacunu::camahjucunu::to_bool(
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
  const char* swa_str = bool_to_string(enable_buffer_averaging);
  const std::string norm_s =
      norm_kind_to_string(effective_projector_options.norm_kind);
  const std::string act_s =
      act_kind_to_string(effective_projector_options.act_kind);
  const char* hbias_s = bool_to_string(effective_projector_options.use_hidden_bias);
  const char* lbias_s = bool_to_string(effective_projector_options.use_last_bias);
  const char* bnfp32_s = bool_to_string(effective_projector_options.bn_in_fp32);
  const char* network_design_enabled_str =
      has_non_ws_ascii(network_design_dsl) ? "true" : "false";
  const char* network_design_id_str =
      has_non_ws_ascii(network_design_network_id)
          ? network_design_network_id.c_str()
          : "<none>";
  const char* network_design_join_policy_str =
      has_non_ws_ascii(network_design_join_policy)
          ? network_design_join_policy.c_str()
          : "<none>";

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
    "\t\t%s%-25s%s %s%-8s%s\n"    // network_design enabled
    "\t\t%s%-25s%s %s%-8s%s\n"    // network_design network id
    "\t\t%s%-25s%s %s%-8s%s\n"    // network_design join policy
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
    ANSI_COLOR_Bright_Grey, "Projector hidden bias:",     ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  hbias_s,                        ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector last bias:",       ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  lbias_s,                        ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Projector BN in FP32:",      ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  bnfp32_s,                       ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Network design:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  network_design_enabled_str,     ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Network design id:",         ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  network_design_id_str,          ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Join policy:",               ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  network_design_join_policy_str, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Data type:",                 ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  dtype_str,                      ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Device:",                    ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  device_str,                     ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Optimizer threshold reset:", ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  optimizer_threshold_reset,      ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "SWA buffer avg:",            ANSI_COLOR_RESET,  ANSI_COLOR_Bright_Blue,  swa_str,                        ANSI_COLOR_RESET
  );
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
