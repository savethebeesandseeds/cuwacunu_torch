/* expected_value.cpp */
/**
 * @file expected_value.cpp
 * @brief ExpectedValue: non-template implementation of the MDN-backed
 *        E[F|X] wrapper with safe checkpointing.
 */

#include "wikimyei/inference/expected_value/expected_value.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio> // std::remove, std::rename
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <utility>

#include "piaabo/torch_compat/data_analytics.h"

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .cpp
// ------------------------------------------------------------
DEV_WARNING("[expected_value.cpp] select_targets uses from_blob()+clone for "
            "indices (tiny extra alloc, safe).\n");
DEV_WARNING("[expected_value.cpp] Channel EMA weights use 1/(ema+eps) with "
            "clamp_max to limit volatility.\n");
DEV_WARNING("[expected_value.cpp] Optimizer state is skipped on CUDA during "
            "save; loader tolerates its absence.\n");
DEV_WARNING("[expected_value.cpp] Checkpoint uses SAFE state-dict "
            "(params/buffers only); avoids JIT pickler & undefined buffers.\n");
DEV_WARNING("[expected_value.cpp] Checkpoint save writes to .tmp and requires "
            "successful rename to final path.\n");

namespace cuwacunu {
namespace wikimyei {

// -------------------- safe state-dict helpers ----------------
namespace {
constexpr int64_t kExpectedValueCheckpointFormatVersion = 6;
constexpr const char *kExpectedValueHeadManifestSchema =
    "expected_value.head_manifest.v1";
constexpr const char *kExpectedValueExportSemantics =
    "target_space_expectation";
constexpr const char *kExpectedValueLossMode = "pseudo_likelihood";
constexpr const char *kExpectedValueTrainingRecipe =
    "representation_frozen_value_head";
constexpr const char *kExpectedValueCovarianceFamily =
    "diagonal_gaussian_components";
constexpr const char *kExpectedValueSigmaParameterization =
    "safe_softplus_output_loss_clamp";
constexpr const char *kExpectedValueOutputLayout =
    "log_pi[B,C,Hf,K];mu_sigma[B,C,Hf,K,Df]";
constexpr const char *kExpectedValueFutureMaskSemantics = "1_valid_0_ignore";
constexpr double kExpectedValueManifestFloatTolerance = 1e-9;

struct ev_module_state_counts_t {
  int64_t parameters{0};
  int64_t buffers{0};
};

inline std::string ev_trim_lower_ascii(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  s = s.substr(b, e - b);
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

inline std::string ev_strip_torch_type_prefixes(std::string v) {
  if (v.rfind("torch::", 0) == 0)
    v = v.substr(7);
  if (v.rfind("at::", 0) == 0)
    v = v.substr(4);
  if (v.rfind("k", 0) == 0 && v.size() > 1 &&
      std::isalpha(static_cast<unsigned char>(v[1]))) {
    v = v.substr(1);
  }
  return v;
}

inline bool ev_requests_cuda_device(std::string configured_device) {
  std::string v = ev_trim_lower_ascii(std::move(configured_device));
  if (v == "gpu" || v.rfind("gpu:", 0) == 0)
    return true;
  v = ev_strip_torch_type_prefixes(std::move(v));
  return v == "cuda" || v.rfind("cuda:", 0) == 0;
}

inline torch::Device ev_resolve_expected_value_device_or_throw(
    const cuwacunu::iitepi::contract_hash_t &contract_hash) {
  const std::string configured_device =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>("EXPECTED_VALUE", "device");
  const torch::Device resolved_device =
      cuwacunu::iitepi::config_device(contract_hash, "EXPECTED_VALUE");
  TORCH_CHECK(
      !(resolved_device.is_cpu() && ev_requests_cuda_device(configured_device)),
      "[ExpectedValue](ctor) configured device '", configured_device,
      "' explicitly requires CUDA, but resolved device is CPU. Refusing to "
      "create the MDN expected-value head on CPU; inspect earlier "
      "[torch_utils][CPU_FALLBACK_ACTIVE] warnings for the CUDA probe failure "
      "details.");
  return resolved_device;
}

inline torch::Tensor ev_move_state_tensor_to_device(torch::Tensor t,
                                                    const torch::Device &dev) {
  if (!t.defined())
    return t;
  if (t.device() == dev)
    return t;
  return t.to(dev, /*non_blocking=*/false);
}

inline void
ev_move_optimizer_state_to_device(torch::optim::Optimizer *optimizer,
                                  const torch::Device &dev) {
  if (!optimizer)
    return;
  for (auto &kv : optimizer->state()) {
    if (auto *st =
            dynamic_cast<torch::optim::AdamWParamState *>(kv.second.get())) {
      st->exp_avg(ev_move_state_tensor_to_device(st->exp_avg(), dev));
      st->exp_avg_sq(ev_move_state_tensor_to_device(st->exp_avg_sq(), dev));
      st->max_exp_avg_sq(
          ev_move_state_tensor_to_device(st->max_exp_avg_sq(), dev));
      continue;
    }
    if (auto *st =
            dynamic_cast<torch::optim::AdamParamState *>(kv.second.get())) {
      st->exp_avg(ev_move_state_tensor_to_device(st->exp_avg(), dev));
      st->exp_avg_sq(ev_move_state_tensor_to_device(st->exp_avg_sq(), dev));
      st->max_exp_avg_sq(
          ev_move_state_tensor_to_device(st->max_exp_avg_sq(), dev));
      continue;
    }
    if (auto *st =
            dynamic_cast<torch::optim::RMSpropParamState *>(kv.second.get())) {
      st->square_avg(ev_move_state_tensor_to_device(st->square_avg(), dev));
      st->momentum_buffer(
          ev_move_state_tensor_to_device(st->momentum_buffer(), dev));
      st->grad_avg(ev_move_state_tensor_to_device(st->grad_avg(), dev));
      continue;
    }
    if (auto *st =
            dynamic_cast<torch::optim::SGDParamState *>(kv.second.get())) {
      st->momentum_buffer(
          ev_move_state_tensor_to_device(st->momentum_buffer(), dev));
      continue;
    }
    if (auto *st =
            dynamic_cast<torch::optim::AdagradParamState *>(kv.second.get())) {
      st->sum(ev_move_state_tensor_to_device(st->sum(), dev));
      continue;
    }
  }
}

// write CPU clone of a tensor under key
inline void ev_write_tensor(torch::serialize::OutputArchive &ar,
                            const std::string &key, const torch::Tensor &t) {
  ar.write(key, t.detach().to(torch::kCPU));
}

// try read tensor; return true on success
inline bool ev_try_read_tensor(torch::serialize::InputArchive &ar,
                               const std::string &key, torch::Tensor &out) {
  try {
    ar.read(key, out);
    return out.defined();
  } catch (...) {
    return false;
  }
}

inline void ev_write_string(torch::serialize::OutputArchive &ar,
                            const std::string &key, const std::string &value) {
  auto t = torch::empty({static_cast<int64_t>(value.size())}, torch::kChar);
  if (t.numel() > 0) {
    std::memcpy(t.data_ptr<int8_t>(), value.data(), value.size());
  }
  ar.write(key, t);
}

inline bool ev_try_read_string(torch::serialize::InputArchive &ar,
                               const std::string &key, std::string &out) {
  torch::Tensor t;
  if (!ev_try_read_tensor(ar, key, t))
    return false;
  t = t.to(torch::kCPU);
  TORCH_CHECK(t.dim() == 1, "[ExpectedValue::ckpt] key '", key,
              "' must be a 1-D byte tensor.");
  out.resize(static_cast<size_t>(t.numel()));
  if (t.numel() > 0) {
    std::memcpy(out.data(), t.data_ptr<int8_t>(),
                static_cast<size_t>(t.numel()));
  }
  return true;
}

inline int64_t ev_require_scalar_i64(torch::serialize::InputArchive &ar,
                                     const std::string &key) {
  torch::Tensor t;
  TORCH_CHECK(ev_try_read_tensor(ar, key, t),
              "[ExpectedValue::ckpt] missing required key '", key, "'.");
  t = t.to(torch::kCPU);
  TORCH_CHECK(t.numel() >= 1, "[ExpectedValue::ckpt] key '", key,
              "' must contain a scalar.");
  return t.item<int64_t>();
}

inline double ev_require_scalar_double(torch::serialize::InputArchive &ar,
                                       const std::string &key) {
  torch::Tensor t;
  TORCH_CHECK(ev_try_read_tensor(ar, key, t),
              "[ExpectedValue::ckpt] missing required key '", key, "'.");
  t = t.to(torch::kCPU);
  TORCH_CHECK(t.numel() >= 1, "[ExpectedValue::ckpt] key '", key,
              "' must contain a scalar.");
  return t.item<double>();
}

inline std::string ev_require_string(torch::serialize::InputArchive &ar,
                                     const std::string &key) {
  std::string value;
  TORCH_CHECK(ev_try_read_string(ar, key, value),
              "[ExpectedValue::ckpt] missing required key '", key, "'.");
  return value;
}

inline void ev_write_i64_vector(torch::serialize::OutputArchive &ar,
                                const std::string &key,
                                const std::vector<int64_t> &values) {
  auto t = torch::empty({static_cast<int64_t>(values.size())},
                        torch::TensorOptions().dtype(torch::kInt64));
  auto *data = t.data_ptr<int64_t>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    data[i] = values[i];
  }
  ar.write(key, t);
}

inline std::vector<int64_t>
ev_require_i64_vector(torch::serialize::InputArchive &ar,
                      const std::string &key) {
  torch::Tensor t;
  TORCH_CHECK(ev_try_read_tensor(ar, key, t),
              "[ExpectedValue::ckpt] missing required key '", key, "'.");
  t = t.to(torch::kCPU).to(torch::kInt64);
  TORCH_CHECK(t.dim() == 1, "[ExpectedValue::ckpt] key '", key,
              "' must be a 1-D int64 tensor.");
  std::vector<int64_t> out(static_cast<std::size_t>(t.numel()));
  const auto *data = t.data_ptr<int64_t>();
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = data[i];
  }
  return out;
}

inline std::string ev_i64_vector_string(const std::vector<int64_t> &values) {
  std::ostringstream oss;
  oss << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i)
      oss << ",";
    oss << values[i];
  }
  oss << "]";
  return oss.str();
}

inline bool ev_ends_with(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

inline std::string ev_feature_alias_for_normalization(std::string alias,
                                                      std::string norm) {
  norm = ev_trim_lower_ascii(std::move(norm));
  if (norm != "log_returns")
    return alias;
  if (ev_ends_with(alias, "_price")) {
    alias.resize(alias.size() - std::string_view("_price").size());
    return alias + ".log_return";
  }
  if (alias == "price")
    return "price.log_return";
  return alias + ".log1p";
}

inline std::string ev_target_feature_aliases_string(
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const std::vector<int64_t> &target_feature_indices) {
  try {
    const auto observation =
        cuwacunu::camahjucunu::decode_observation_spec_from_contract(
            contract_hash);

    bool have_active = false;
    bool mixed_record_type = false;
    bool mixed_normalization = false;
    std::string record_type{};
    std::string normalization_policy{};
    for (const auto &channel : observation.channel_forms) {
      const std::string active = ev_trim_lower_ascii(channel.active);
      if (active != "true" && active != "1" && active != "yes" &&
          active != "on") {
        continue;
      }
      have_active = true;
      const std::string channel_record_type =
          ev_trim_lower_ascii(channel.record_type);
      const std::string channel_normalization =
          ev_trim_lower_ascii(channel.normalization_policy);
      if (record_type.empty()) {
        record_type = channel_record_type;
      } else if (record_type != channel_record_type) {
        mixed_record_type = true;
      }
      if (normalization_policy.empty()) {
        normalization_policy = channel_normalization;
      } else if (normalization_policy != channel_normalization) {
        mixed_normalization = true;
      }
    }

    if (!have_active || mixed_record_type || mixed_normalization ||
        record_type.empty()) {
      return "unavailable";
    }

    const auto &feature_names =
        cuwacunu::piaabo::torch_compat::data_feature_names_for_record_type(
            record_type);
    if (feature_names.empty())
      return "unavailable";

    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < target_feature_indices.size(); ++i) {
      if (i != 0)
        oss << ", ";
      const int64_t index = target_feature_indices[i];
      if (index >= 0 &&
          static_cast<std::size_t>(index) < feature_names.size()) {
        oss << ev_feature_alias_for_normalization(
            feature_names[static_cast<std::size_t>(index)],
            normalization_policy);
      } else {
        oss << "#" << index;
      }
    }
    oss << "]";
    return oss.str();
  } catch (...) {
    return "unavailable";
  }
}

inline bool ev_manifest_double_equal(double saved, double runtime) {
  const double scale =
      std::max(1.0, std::max(std::abs(saved), std::abs(runtime)));
  return std::abs(saved - runtime) <=
         kExpectedValueManifestFloatTolerance * scale;
}

// Save only defined params/buffers (CPU clones), no JIT/module pickling.
inline ev_module_state_counts_t
ev_save_module_state(torch::serialize::OutputArchive &ar,
                     const torch::nn::Module &m, const std::string &base) {
  ev_module_state_counts_t counts{};
  for (const auto &np : m.named_parameters(/*recurse=*/true)) {
    ev_write_tensor(ar, base + "/param/" + np.key(), np.value());
    ++counts.parameters;
  }
  for (const auto &nb : m.named_buffers(/*recurse=*/true)) {
    if (nb.value().defined()) {
      ev_write_tensor(ar, base + "/buffer/" + nb.key(), nb.value());
      ++counts.buffers;
    } else {
      log_warn("[ExpectedValue::ckpt] skipping undefined buffer '%s'.\n",
               nb.key().c_str());
    }
  }
  return counts;
}

inline void ev_require_same_shape(const torch::Tensor &saved,
                                  const torch::Tensor &runtime,
                                  const std::string &key) {
  TORCH_CHECK(saved.sizes() == runtime.sizes(),
              "[ExpectedValue::ckpt] shape mismatch for '", key,
              "' checkpoint=", saved.sizes(), " runtime=", runtime.sizes());
}

// Strict load by name; missing params/buffers fail the checkpoint restore.
inline ev_module_state_counts_t
ev_load_module_state(torch::serialize::InputArchive &ar, torch::nn::Module &m,
                     const std::string &base) {
  torch::NoGradGuard _ng;
  ev_module_state_counts_t counts{};
  for (auto &np : m.named_parameters(/*recurse=*/true)) {
    const auto &name = np.key();
    auto &p = np.value();
    const std::string key = base + "/param/" + name;
    torch::Tensor t;
    TORCH_CHECK(ev_try_read_tensor(ar, key, t),
                "[ExpectedValue::ckpt] missing required model parameter '",
                name, "'.");
    ev_require_same_shape(t, p, key);
    p.detach().copy_(t.to(p.device(), p.dtype()));
    ++counts.parameters;
  }
  for (auto &nb : m.named_buffers(/*recurse=*/true)) {
    const auto &name = nb.key();
    auto &b = nb.value();
    if (!b.defined())
      continue;
    const std::string key = base + "/buffer/" + name;
    torch::Tensor t;
    TORCH_CHECK(ev_try_read_tensor(ar, key, t),
                "[ExpectedValue::ckpt] missing required model buffer '", name,
                "'.");
    ev_require_same_shape(t, b, key);
    b.detach().copy_(t.to(b.device(), b.dtype()));
    ++counts.buffers;
  }
  return counts;
}
} // anonymous namespace

// ---------- ctor ----------
ExpectedValue::ExpectedValue(
    const cuwacunu::iitepi::contract_hash_t &contract_hash_,
    const std::string &component_name_)
    : contract_hash(contract_hash_), component_name(component_name_) {
  const auto contract =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);

  // Static weights and target feature indices from the bound contract snapshot.
  future_feature_width_ = contract->get<int64_t>("DOCK", "__obs_feature_dim");
  TORCH_CHECK(future_feature_width_ > 0,
              "[ExpectedValue](ctor) __obs_feature_dim must be positive");
  future_feature_schema_hash_ = contract->docking_signature.sha256_hex;
  TORCH_CHECK(!future_feature_schema_hash_.empty(),
              "[ExpectedValue](ctor) future feature schema hash is empty");
  auto observation_instruction =
      cuwacunu::camahjucunu::decode_observation_spec_from_contract(
          contract_hash);
  static_channel_weights_ = observation_instruction.retrieve_channel_weights();
  static_feature_weights_ =
      contract->get_arr<float>("EXPECTED_VALUE", "target_weights");
  grad_clip = contract->get<double>("EXPECTED_VALUE", "grad_clip");
  optimizer_threshold_reset =
      contract->get<int>("EXPECTED_VALUE", "optimizer_threshold_reset");
  target_feature_indices_ =
      contract->get_arr<int64_t>("EXPECTED_VALUE", "target_feature_indices");
  {
    auto sorted_indices = target_feature_indices_;
    std::sort(sorted_indices.begin(), sorted_indices.end());
    TORCH_CHECK(!sorted_indices.empty(),
                "[ExpectedValue](ctor) target_feature_indices must be "
                "non-empty");
    TORCH_CHECK(sorted_indices.front() >= 0,
                "[ExpectedValue](ctor) target_feature_indices must be "
                "non-negative");
    TORCH_CHECK(
        std::adjacent_find(sorted_indices.begin(), sorted_indices.end()) ==
            sorted_indices.end(),
        "[ExpectedValue](ctor) target_feature_indices must be unique");
    TORCH_CHECK(sorted_indices.back() < future_feature_width_,
                "[ExpectedValue](ctor) target_feature_indices must fit future "
                "feature width");
  }
  temporal_reducer_ = parse_temporal_reducer_(contract->get<std::string>(
      "EXPECTED_VALUE", "encoding_temporal_reducer", "last_valid"));
  telemetry_every_ =
      std::max(0, contract->get<int>("EXPECTED_VALUE", "telemetry_every", 0));

  // Model
  const torch::Dtype configured_dtype =
      cuwacunu::iitepi::config_dtype(contract_hash, "EXPECTED_VALUE");
  const torch::Device configured_device =
      ev_resolve_expected_value_device_or_throw(contract_hash);
  semantic_model = cuwacunu::wikimyei::mdn::MdnModel(
      contract->get<int>("EXPECTED_VALUE", "encoding_dims"),   // De
      static_cast<int64_t>(target_feature_indices_.size()),    // Df
      observation_instruction.count_channels(),                // C
      observation_instruction.max_future_sequence_length(),    // Hf
      contract->get<int>("EXPECTED_VALUE", "mixture_comps"),   // K
      contract->get<int>("EXPECTED_VALUE", "features_hidden"), // H
      contract->get<int>("EXPECTED_VALUE", "residual_depth"),  // depth
      configured_dtype, configured_device,
      /* display_model */ false);
  semantic_model->assert_resident_on_device_or_throw("[ExpectedValue](ctor)");

  // Params
  trainable_params_.reserve(64);
  for (auto &p : semantic_model->parameters(/*recurse=*/true)) {
    if (p.requires_grad())
      trainable_params_.push_back(p);
  }

  // Builders
  TORCH_CHECK(
      cuwacunu::jkimyei::jk_setup(component_name, contract_hash).opt_builder,
      "[ExpectedValue](ctor) opt_builder is null");
  TORCH_CHECK(
      cuwacunu::jkimyei::jk_setup(component_name, contract_hash).sched_builder,
      "[ExpectedValue](ctor) sched_builder is null");

  optimizer = cuwacunu::jkimyei::jk_setup(component_name, contract_hash)
                  .opt_builder->build(trainable_params_);
  lr_sched = cuwacunu::jkimyei::jk_setup(component_name, contract_hash)
                 .sched_builder->build(*optimizer);
  loss_obj = std::make_unique<cuwacunu::wikimyei::mdn::MdnNLLLoss>(
      cuwacunu::jkimyei::jk_setup(component_name, contract_hash));
  load_jkimyei_training_policy();

  display_model(/* display_semantic */ true);
}

void ExpectedValue::load_jkimyei_training_policy() {
  const auto &jk_component =
      cuwacunu::jkimyei::jk_setup(component_name, contract_hash);
  const auto find_row_by_id = [&](const std::string &table_name,
                                  const std::string &row_id)
      -> const cuwacunu::camahjucunu::jkimyei_specs_t::row_t * {
    const auto table_it = jk_component.inst.tables.find(table_name);
    if (table_it == jk_component.inst.tables.end())
      return nullptr;
    for (const auto &row : table_it->second) {
      const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
      if (rid_it == row.end())
        continue;
      if (cuwacunu::camahjucunu::trim_copy(rid_it->second) == row_id)
        return &row;
    }
    return nullptr;
  };

  const auto *gradient_row = find_row_by_id(
      "component_gradient_table", jk_component.resolved_profile_row_id);
  TORCH_CHECK(gradient_row != nullptr,
              "[ExpectedValue::load_jkimyei_training_policy] gradient row not "
              "found for row_id='",
              jk_component.resolved_profile_row_id, "'");

  const long accumulate_steps = cuwacunu::camahjucunu::to_long(
      cuwacunu::camahjucunu::require_column(*gradient_row, "accumulate_steps"));
  TORCH_CHECK(accumulate_steps >= 1, "[ExpectedValue::load_jkimyei_training_"
                                     "policy] accumulate_steps must be >= 1");
  training_policy.accumulate_steps = static_cast<int>(accumulate_steps);

  training_policy.clip_norm = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(*gradient_row, "clip_norm"));
  training_policy.clip_value = cuwacunu::camahjucunu::to_double(
      cuwacunu::camahjucunu::require_column(*gradient_row, "clip_value"));
  TORCH_CHECK(
      training_policy.clip_norm >= 0.0,
      "[ExpectedValue::load_jkimyei_training_policy] clip_norm must be >= 0");
  TORCH_CHECK(
      training_policy.clip_value >= 0.0,
      "[ExpectedValue::load_jkimyei_training_policy] clip_value must be >= 0");
  grad_clip = training_policy.clip_norm;

  training_policy.skip_on_nan = cuwacunu::camahjucunu::to_bool(
      cuwacunu::camahjucunu::require_column(*gradient_row, "skip_on_nan"));
  training_policy.zero_grad_set_to_none =
      cuwacunu::camahjucunu::to_bool(cuwacunu::camahjucunu::require_column(
          *gradient_row, "zero_grad_set_to_none"));
}

const char *ExpectedValue::temporal_reducer_name_(TemporalReducer reducer) {
  switch (reducer) {
  case TemporalReducer::LastValid:
    return "last_valid";
  case TemporalReducer::MaskedMean:
    return "masked_mean";
  case TemporalReducer::Mean:
    return "mean";
  }
  return "unknown";
}

ExpectedValue::TemporalReducer
ExpectedValue::parse_temporal_reducer_(std::string token) {
  token = cuwacunu::camahjucunu::trim_copy(std::move(token));
  std::transform(
      token.begin(), token.end(), token.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  std::replace(token.begin(), token.end(), '-', '_');

  if (token.empty() || token == "last_valid" || token == "lastvalid")
    return TemporalReducer::LastValid;
  if (token == "masked_mean" || token == "maskedmean")
    return TemporalReducer::MaskedMean;
  if (token == "mean")
    return TemporalReducer::Mean;

  TORCH_CHECK(false, "[ExpectedValue] unsupported encoding_temporal_reducer='",
              token, "' (expected one of: last_valid, masked_mean, mean).");
}

torch::Tensor ExpectedValue::encoding_sequence_mask_from_source_mask_(
    const torch::Tensor &source_mask, int64_t batch_size,
    int64_t sequence_length, torch::Device dev) const {
  TORCH_CHECK(source_mask.defined(),
              "[ExpectedValue::reduce_encoding] temporal reducer '",
              temporal_reducer_name_(temporal_reducer_),
              "' requires source mask for rank-3 encodings.");

  auto mask = source_mask.to(dev, torch::kFloat32, /*non_blocking=*/true,
                             /*copy=*/false);
  if (mask.dim() == 3) {
    TORCH_CHECK(mask.size(0) == batch_size,
                "[ExpectedValue::reduce_encoding] source mask batch mismatch");
    TORCH_CHECK(mask.size(2) == sequence_length,
                "[ExpectedValue::reduce_encoding] source mask time mismatch");
    mask = mask.gt(0).any(/*dim=*/1); // [B,C,Hx] -> [B,Hx]
  } else if (mask.dim() == 2) {
    TORCH_CHECK(mask.size(0) == batch_size,
                "[ExpectedValue::reduce_encoding] source mask batch mismatch");
    TORCH_CHECK(mask.size(1) == sequence_length,
                "[ExpectedValue::reduce_encoding] source mask time mismatch");
    mask = mask.gt(0);
  } else {
    TORCH_CHECK(false, "[ExpectedValue::reduce_encoding] source mask must be "
                       "[B,C,Hx] or [B,Hx] for rank-3 encodings.");
  }
  return mask.to(torch::kFloat32);
}

torch::Tensor
ExpectedValue::reduce_encoding_(const torch::Tensor &encoding,
                                const torch::Tensor &source_mask) const {
  TORCH_CHECK(encoding.defined(),
              "[ExpectedValue::reduce_encoding] encoding is undefined");
  TORCH_CHECK(encoding.dim() == 2 || encoding.dim() == 3,
              "[ExpectedValue::reduce_encoding] encoding must be [B,De] or "
              "[B,Hx,De]");

  auto enc = encoding.to(semantic_model->device, semantic_model->dtype,
                         /*non_blocking=*/true, /*copy=*/false);
  if (enc.dim() == 2) {
    TORCH_CHECK(enc.size(1) == semantic_model->De,
                "[ExpectedValue::reduce_encoding] encoding width mismatch");
    return enc;
  }

  const int64_t B = enc.size(0);
  const int64_t T = enc.size(1);
  const int64_t De = enc.size(2);
  TORCH_CHECK(B > 0 && T > 0 && De == semantic_model->De,
              "[ExpectedValue::reduce_encoding] rank-3 encoding shape "
              "mismatch");

  if (temporal_reducer_ == TemporalReducer::Mean) {
    return enc.mean(/*dim=*/1);
  }

  auto seq_mask = encoding_sequence_mask_from_source_mask_(
      source_mask, B, T, semantic_model->device);
  const auto valid = seq_mask.gt(0);
  const auto valid_counts = valid.to(torch::kLong).sum(/*dim=*/1);
  TORCH_CHECK(valid_counts.gt(0).all().template item<bool>(),
              "[ExpectedValue::reduce_encoding] temporal reducer '",
              temporal_reducer_name_(temporal_reducer_),
              "' received an all-masked encoding sequence.");

  if (temporal_reducer_ == TemporalReducer::LastValid) {
    auto positions = torch::arange(T, torch::TensorOptions()
                                          .dtype(torch::kLong)
                                          .device(semantic_model->device));
    auto last_idx = std::get<0>(positions.view({1, T})
                                    .expand({B, T})
                                    .masked_fill(valid.logical_not(), -1)
                                    .max(/*dim=*/1));
    auto idx = last_idx.view({B, 1, 1}).expand({B, 1, De});
    return enc.gather(/*dim=*/1, idx).squeeze(/*dim=*/1);
  }

  if (temporal_reducer_ == TemporalReducer::MaskedMean) {
    auto weights = valid.to(enc.dtype()).unsqueeze(/*dim=*/-1);
    auto denom = weights.sum(/*dim=*/1).clamp_min(1.0);
    return (enc * weights).sum(/*dim=*/1) / denom;
  }

  TORCH_CHECK(false, "[ExpectedValue::reduce_encoding] unsupported temporal "
                     "reducer.");
}

void ExpectedValue::validate_future_feature_width_(
    const torch::Tensor &future_features, const char *caller) const {
  TORCH_CHECK(future_features.defined(), caller,
              " future_features is undefined");
  TORCH_CHECK(future_features.dim() == 4, caller,
              " future_features must be [B,C,Hf,Dx]");
  const int64_t D = future_features.size(3);
  TORCH_CHECK(D > 0, caller, " future feature width must be positive");
  for (const auto index : target_feature_indices_) {
    TORCH_CHECK(index >= 0 && index < D, caller, " target feature index ",
                index, " out of range for future feature width Dx=", D);
  }
}

mdn::MdnOut
ExpectedValue::forward_from_encoding(const torch::Tensor &encoding) {
  return forward_from_encoding(encoding, torch::Tensor());
}

mdn::MdnOut
ExpectedValue::forward_from_encoding(const torch::Tensor &encoding,
                                     const torch::Tensor &source_mask) {
  return semantic_model->forward_from_encoding(
      reduce_encoding_(encoding, source_mask));
}

torch::Tensor ExpectedValue::predict_expected_value_from_encoding(
    const torch::Tensor &encoding) {
  return predict_expected_value_from_encoding(encoding, torch::Tensor());
}

torch::Tensor ExpectedValue::predict_expected_value_from_encoding(
    const torch::Tensor &encoding, const torch::Tensor &source_mask) {
  return expected_value_from_out(forward_from_encoding(encoding, source_mask));
}

void ExpectedValue::clear_pending_runtime_state_(bool reset_optimizer_steps) {
  if (optimizer)
    optimizer->zero_grad(training_policy.zero_grad_set_to_none);
  runtime_state.accum_counter = 0;
  runtime_state.has_pending_grad = false;
  runtime_state.pending_sample_count = 0;
  runtime_state.pending_loss_sum = 0.0;
  runtime_state.pending_loss_count = 0;
  if (reset_optimizer_steps) {
    runtime_state.optimizer_steps = 0;
    runtime_state.trained_samples = 0;
    runtime_state.skipped_batches = 0;
    runtime_state.last_committed_loss_mean = 0.0;
  }
}

void ExpectedValue::reset_runtime_training_state() {
  clear_pending_runtime_state_(/*reset_optimizer_steps=*/true);
}

std::vector<torch::Tensor> ExpectedValue::params_with_grad_() const {
  std::vector<torch::Tensor> out;
  out.reserve(trainable_params_.size());
  for (const auto &p : trainable_params_) {
    if (p.grad().defined())
      out.push_back(p);
  }
  return out;
}

void ExpectedValue::apply_gradient_clipping_(
    const std::vector<torch::Tensor> &params_with_grad) const {
  if (params_with_grad.empty())
    return;
  if (training_policy.clip_value > 0.0) {
    torch::nn::utils::clip_grad_value_(params_with_grad,
                                       training_policy.clip_value);
  }
  if (training_policy.clip_norm > 0.0) {
    torch::nn::utils::clip_grad_norm_(params_with_grad,
                                      training_policy.clip_norm);
  }
}

bool ExpectedValue::grads_are_finite_(
    const std::vector<torch::Tensor> &params_with_grad) const {
  for (const auto &p : params_with_grad) {
    const auto g = p.grad();
    if (!g.defined())
      continue;
    if (!torch::isfinite(g).all().template item<bool>())
      return false;
  }
  return true;
}

bool ExpectedValue::commit_pending_training_step_() {
  if (!runtime_state.has_pending_grad || runtime_state.accum_counter <= 0)
    return false;

  const auto params_with_grad = params_with_grad_();
  const bool grad_is_finite = grads_are_finite_(params_with_grad);
  if (!grad_is_finite) {
    if (training_policy.skip_on_nan) {
      clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);
      ++runtime_state.skipped_batches;
      return false;
    }
    TORCH_CHECK(false, "[ExpectedValue::commit_pending_training_step_] "
                       "non-finite gradients detected and skip_on_nan=false");
  }
  apply_gradient_clipping_(params_with_grad);
  TORCH_CHECK(grads_are_finite_(params_with_grad),
              "[ExpectedValue::commit_pending_training_step_] non-finite "
              "gradients detected after clipping");

  optimizer->step();
  if (optimizer_threshold_reset >= 0) {
    cuwacunu::jkimyei::optim::clamp_adam_step(
        *optimizer, static_cast<int64_t>(optimizer_threshold_reset));
  }
  if (lr_sched &&
      lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
    lr_sched->step();
    ++scheduler_batch_steps_;
  }

  runtime_state.last_committed_loss_mean =
      (runtime_state.pending_loss_count > 0)
          ? (runtime_state.pending_loss_sum /
             static_cast<double>(runtime_state.pending_loss_count))
          : 0.0;
  runtime_state.trained_samples += runtime_state.pending_sample_count;
  runtime_state.pending_sample_count = 0;
  runtime_state.pending_loss_sum = 0.0;
  runtime_state.pending_loss_count = 0;
  runtime_state.accum_counter = 0;
  runtime_state.has_pending_grad = false;
  ++runtime_state.optimizer_steps;
  return true;
}

bool ExpectedValue::finalize_pending_training_step() {
  return commit_pending_training_step_();
}

ExpectedValue::train_step_result_t ExpectedValue::train_one_batch(
    const torch::Tensor &encoding, const torch::Tensor &future_features,
    const torch::Tensor &future_mask, const torch::Tensor &source_mask) {
  train_step_result_t result{};
  TORCH_CHECK(optimizer, "[ExpectedValue::train_one_batch] optimizer is null");
  TORCH_CHECK(loss_obj, "[ExpectedValue::train_one_batch] loss object is null");
  TORCH_CHECK(encoding.defined(),
              "[ExpectedValue::train_one_batch] encoding is undefined");
  TORCH_CHECK(future_features.defined() && future_mask.defined(),
              "[ExpectedValue::train_one_batch] future_features and "
              "future_mask must be defined");

  if (!runtime_state.has_pending_grad) {
    optimizer->zero_grad(training_policy.zero_grad_set_to_none);
  }

  semantic_model->train();
  auto enc = encoding.to(semantic_model->device, semantic_model->dtype,
                         /*non_blocking=*/true, /*copy=*/false);
  auto smask = source_mask.defined()
                   ? source_mask.to(semantic_model->device, torch::kFloat32,
                                    /*non_blocking=*/true, /*copy=*/false)
                   : torch::Tensor();
  auto fut = future_features.to(semantic_model->device, semantic_model->dtype,
                                /*non_blocking=*/true, /*copy=*/false);
  auto fmask = future_mask.to(semantic_model->device, torch::kFloat32,
                              /*non_blocking=*/true, /*copy=*/false);

  TORCH_CHECK(
      fut.dim() == 4,
      "[ExpectedValue::train_one_batch] future_features must be [B,C,Hf,Dx]");
  TORCH_CHECK(fmask.dim() == 3,
              "[ExpectedValue::train_one_batch] future_mask must be [B,C,Hf]");
  TORCH_CHECK(enc.dim() == 2 || enc.dim() == 3,
              "[ExpectedValue::train_one_batch] encoding must be [B,De] or "
              "[B,Hx,De]");
  TORCH_CHECK(
      fut.size(0) == enc.size(0),
      "[ExpectedValue::train_one_batch] batch size mismatch encoding/future");

  validate_future_feature_width_(fut, "[ExpectedValue::train_one_batch]");
  auto f = select_targets(fut, target_feature_indices_);
  auto out = forward_from_encoding(enc, smask);

  const int64_t C = fmask.size(1);
  const int64_t Hf = fmask.size(2);
  const int64_t Df = f.size(3);
  auto w_ch =
      build_channel_weights(C, semantic_model->device, semantic_model->dtype);
  auto w_tau =
      build_horizon_weights(Hf, semantic_model->device, semantic_model->dtype);
  auto w_dim =
      build_feature_weights(Df, semantic_model->device, semantic_model->dtype);

  auto loss = loss_obj->compute(out, f, fmask, w_ch, w_tau, w_dim);
  const double loss_scalar = loss.item().toDouble();
  if (!std::isfinite(loss_scalar)) {
    if (training_policy.skip_on_nan) {
      clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);
      ++runtime_state.skipped_batches;
      result.skipped = true;
      return result;
    }
    TORCH_CHECK(false, "[ExpectedValue::train_one_batch] non-finite loss "
                       "detected and skip_on_nan=false");
  }

  const int64_t next_iter = total_iters_trained_ + 1;
  const bool log_now =
      (telemetry_every_ > 0) && (next_iter % telemetry_every_ == 0);
  if (use_channel_ema_weights_ || log_now) {
    torch::NoGradGuard telemetry_ng;
    cuwacunu::wikimyei::mdn::MdnNllOptions o{
        loss_obj->eps_, loss_obj->sigma_min_, loss_obj->sigma_max_};
    auto nll_map = cuwacunu::wikimyei::mdn::mdn_nll_map(out, f, fmask, o);
    auto ch_mean =
        cuwacunu::wikimyei::mdn::mdn_masked_mean_per_channel(nll_map, fmask);
    if (use_channel_ema_weights_)
      update_channel_ema(ch_mean);
    auto hz_mean =
        cuwacunu::wikimyei::mdn::mdn_masked_mean_per_horizon(nll_map, fmask);
    last_per_channel_nll_ = ch_mean.detach();
    last_per_horizon_nll_ = hz_mean.detach();

    if (log_now) {
      auto ch_cpu = ch_mean.detach().to(torch::kCPU);
      auto hz_cpu = hz_mean.detach().to(torch::kCPU);
      log_dbg("[ExpectedValue::telemetry it=%lld chNLL[min=%.5f max=%.5f] "
              "hzNLL[min=%.5f max=%.5f]\n",
              static_cast<long long>(next_iter), ch_cpu.min().item().toDouble(),
              ch_cpu.max().item().toDouble(), hz_cpu.min().item().toDouble(),
              hz_cpu.max().item().toDouble());
    }
  }

  runtime_state.pending_loss_sum += loss_scalar;
  ++runtime_state.pending_loss_count;
  if (fut.size(0) > 0) {
    runtime_state.pending_sample_count +=
        static_cast<std::uint64_t>(fut.size(0));
  }

  auto backprop_loss =
      (training_policy.accumulate_steps > 1)
          ? (loss / static_cast<double>(training_policy.accumulate_steps))
          : loss;
  backprop_loss.backward();
  runtime_state.has_pending_grad = true;
  ++runtime_state.accum_counter;
  ++total_iters_trained_;

  result.loss = loss.detach().to(torch::kCPU);
  if (runtime_state.accum_counter <
      std::max(1, training_policy.accumulate_steps)) {
    return result;
  }

  result.optimizer_step_applied = commit_pending_training_step_();
  if (!result.optimizer_step_applied)
    result.skipped = true;
  return result;
}

// ---------- helpers: targets & weights ----------
torch::Tensor ExpectedValue::select_targets(
    const torch::Tensor &future_features,
    const std::vector<int64_t> &target_feature_indices) {
  TORCH_CHECK(future_features.defined(),
              "[ExpectedValue::select_targets] future_features undefined");
  TORCH_CHECK(future_features.dim() == 4,
              "[ExpectedValue::select_targets] expecting [B,C,Hf,Dx]");
  const auto B = future_features.size(0);
  const auto C = future_features.size(1);
  const auto Hf = future_features.size(2);
  const auto D = future_features.size(3);
  TORCH_CHECK(!target_feature_indices.empty(),
              "[ExpectedValue::select_targets] empty target_feature_indices");
  for (auto index : target_feature_indices) {
    TORCH_CHECK(index >= 0 && index < D,
                "[ExpectedValue::select_targets] target feature index out of "
                "range");
  }
  const auto Df = static_cast<int64_t>(target_feature_indices.size());
  auto idx = torch::tensor(
                 target_feature_indices,
                 torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU))
                 .to(future_features.device());

  auto flat = future_features.reshape({B * C * Hf, D});
  auto idx2 = idx.unsqueeze(0).expand({B * C * Hf, Df});
  auto f_sel = flat.gather(/*dim=*/1, idx2);
  return f_sel.view({B, C, Hf, Df});
}

torch::Tensor ExpectedValue::build_horizon_weights(int64_t Hf,
                                                   torch::Device dev,
                                                   torch::Dtype dt) const {
  if (Hf <= 0)
    return torch::Tensor();
  std::vector<float> w(static_cast<size_t>(Hf), 1.f);
  switch (horizon_policy_) {
  case HorizonPolicy::Uniform:
    break;
  case HorizonPolicy::NearTerm: {
    float g = gamma_near_;
    for (size_t t = 0; t < w.size(); ++t)
      w[t] = std::pow(g, static_cast<float>(t));
    break;
  }
  case HorizonPolicy::VeryNearTerm: {
    float g = gamma_very_;
    for (size_t t = 0; t < w.size(); ++t)
      w[t] = std::pow(g, static_cast<float>(t));
    break;
  }
  }
  auto wt = torch::tensor(w, torch::TensorOptions().dtype(torch::kFloat32));
  if (wt.dtype() != dt)
    wt = wt.to(dt);
  return wt.to(dev);
}

torch::Tensor ExpectedValue::build_channel_weights(int64_t C, torch::Device dev,
                                                   torch::Dtype dt) {
  if (C <= 0)
    return torch::Tensor();
  auto w = torch::ones({C}, torch::TensorOptions().dtype(dt).device(dev));

  if (!static_channel_weights_.empty()) {
    TORCH_CHECK(static_cast<int64_t>(static_channel_weights_.size()) == C,
                "[ExpectedValue] static_channel_weights size must equal C");
    auto ws = torch::tensor(static_channel_weights_,
                            torch::TensorOptions().dtype(torch::kFloat32));
    if (ws.dtype() != dt)
      ws = ws.to(dt);
    w = w * ws.to(dev);
  }
  if (use_channel_ema_weights_) {
    auto w_ema = channel_weights_from_ema(C);
    if (w_ema.dtype() != dt)
      w_ema = w_ema.to(dt);
    w = w * w_ema;
  }
  return w;
}

torch::Tensor ExpectedValue::build_feature_weights(int64_t Df,
                                                   torch::Device dev,
                                                   torch::Dtype dt) const {
  if (Df <= 0)
    return torch::Tensor();
  if (!static_feature_weights_.empty()) {
    TORCH_CHECK(static_cast<int64_t>(static_feature_weights_.size()) == Df,
                "[ExpectedValue] static_feature_weights size must equal Df");
    auto wd = torch::tensor(static_feature_weights_,
                            torch::TensorOptions().dtype(torch::kFloat32));
    if (wd.dtype() != dt)
      wd = wd.to(dt);
    return wd.to(dev);
  }
  return torch::ones({Df}, torch::TensorOptions().dtype(dt).device(dev));
}

torch::Tensor ExpectedValue::channel_weights_from_ema(int64_t C) {
  if (!use_channel_ema_weights_) {
    return torch::ones(
        {C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
  }
  if (!channel_ema_.defined() || channel_ema_.numel() != C) {
    channel_ema_ = torch::ones(
        {C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
  }
  const float eps = 1e-6f;
  auto w = 1.0f / (channel_ema_ + eps);
  return w.clamp_max(10.0f); // limit volatility
}

void ExpectedValue::update_channel_ema(const torch::Tensor &ch_mean_loss) {
  if (!use_channel_ema_weights_)
    return;
  if (!channel_ema_.defined()) {
    channel_ema_ = ch_mean_loss.detach();
    return;
  }
  channel_ema_.mul_(ema_alpha_)
      .add_(ch_mean_loss.detach() * (1.0 - ema_alpha_));
}

const char *ExpectedValue::scheduler_mode_name(
    cuwacunu::jkimyei::LRSchedulerAny::Mode mode) {
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
// Checkpointing (SAFE, strict v6)
// ==========================
bool ExpectedValue::save_checkpoint(
    const std::string &path, bool write_network_analytics_sidecar) const {
  const std::string tmp = path + ".tmp";
  try {
    torch::serialize::OutputArchive ar;
    semantic_model->assert_resident_on_device_or_throw(
        "[ExpectedValue::ckpt save]");

    // --- model state (safe, tensor-by-tensor) ---
    const auto model_counts =
        ev_save_module_state(ar, *semantic_model, "model");

    // --- strict metadata ---
    ar.write("format_version",
             torch::tensor({kExpectedValueCheckpointFormatVersion},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ev_write_string(ar, "meta/contract_hash", contract_hash);
    ev_write_string(ar, "meta/component_name", component_name);
    ev_write_string(ar, "meta/encoding_temporal_reducer",
                    temporal_reducer_name_(temporal_reducer_));
    ev_write_string(ar, "meta/head_manifest_schema",
                    kExpectedValueHeadManifestSchema);
    ev_write_string(ar, "meta/export_semantics", kExpectedValueExportSemantics);
    ev_write_string(ar, "meta/loss_mode", kExpectedValueLossMode);
    ev_write_string(ar, "meta/training_recipe", kExpectedValueTrainingRecipe);
    ev_write_string(ar, "meta/covariance_family",
                    kExpectedValueCovarianceFamily);
    ev_write_string(ar, "meta/sigma_parameterization",
                    kExpectedValueSigmaParameterization);
    ev_write_string(ar, "meta/output_layout", kExpectedValueOutputLayout);
    ev_write_string(ar, "meta/future_mask_semantics",
                    kExpectedValueFutureMaskSemantics);
    ev_write_string(ar, "meta/scheduler_mode",
                    lr_sched ? scheduler_mode_name(lr_sched->mode)
                             : std::string("None"));
    ar.write("meta/scheduler_batch_steps",
             torch::tensor({scheduler_batch_steps_},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/scheduler_epoch_steps",
             torch::tensor({scheduler_epoch_steps_},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/model_parameter_count",
             torch::tensor({model_counts.parameters},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/model_buffer_count",
             torch::tensor({model_counts.buffers},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/future_feature_width",
             torch::tensor({future_feature_width_},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ev_write_string(ar, "meta/future_feature_schema_hash",
                    future_feature_schema_hash_);
    ar.write(
        "meta/target_width",
        torch::tensor({static_cast<int64_t>(target_feature_indices_.size())},
                      torch::TensorOptions().dtype(torch::kInt64)));
    ev_write_i64_vector(ar, "meta/target_feature_indices",
                        target_feature_indices_);
    ar.write("meta/encoding_dims",
             torch::tensor({semantic_model->De},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/channel_count",
             torch::tensor({semantic_model->C_axes},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/horizon_count",
             torch::tensor({semantic_model->Hf_axes},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/mixture_comps",
             torch::tensor({semantic_model->K},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/features_hidden",
             torch::tensor({semantic_model->H},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/residual_depth",
             torch::tensor({semantic_model->depth},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("meta/loss_eps",
             torch::tensor({loss_obj ? loss_obj->eps_ : 0.0},
                           torch::TensorOptions().dtype(torch::kFloat64)));
    ar.write("meta/loss_sigma_min",
             torch::tensor({loss_obj ? loss_obj->sigma_min_ : 0.0},
                           torch::TensorOptions().dtype(torch::kFloat64)));
    ar.write("meta/loss_sigma_max",
             torch::tensor({loss_obj ? loss_obj->sigma_max_ : 0.0},
                           torch::TensorOptions().dtype(torch::kFloat64)));
    ar.write(
        "meta/target_weight_count",
        torch::tensor({static_cast<int64_t>(static_feature_weights_.size())},
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
        } catch (const c10::Error &e) {
          log_warn("[ExpectedValue::ckpt] optimizer->save failed; skipping. "
                   "Err=%s\n",
                   e.what());
        }
      } else {
        log_warn(
            "[ExpectedValue::ckpt] skipping optimizer state save (on CUDA).\n");
      }
    }
    ar.write("has_optimizer",
             torch::tensor({wrote_opt},
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
    ar.write("scheduler_serialized",
             torch::tensor({scheduler_serialized},
                           torch::TensorOptions().dtype(torch::kInt64)));

    // --- scalars ---
    ar.write("best_metric", torch::tensor({best_metric_}));
    ar.write("best_epoch",
             torch::tensor({int64_t(best_epoch_)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("total_iters_trained",
             torch::tensor({int64_t(total_iters_trained_)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("total_epochs_trained",
             torch::tensor({int64_t(total_epochs_trained_)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("runtime_optimizer_steps",
             torch::tensor({int64_t(runtime_state.optimizer_steps)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("runtime_trained_samples",
             torch::tensor({int64_t(runtime_state.trained_samples)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("runtime_skipped_batches",
             torch::tensor({int64_t(runtime_state.skipped_batches)},
                           torch::TensorOptions().dtype(torch::kInt64)));
    ar.write("runtime_last_committed_loss_mean",
             torch::tensor({runtime_state.last_committed_loss_mean}));

    // --- telemetry (write ONLY if defined; no placeholders) ---
    if (channel_ema_.defined())
      ar.write("channel_ema", channel_ema_.detach().to(torch::kCPU));
    if (last_per_channel_nll_.defined())
      ar.write("last_per_channel_nll",
               last_per_channel_nll_.detach().to(torch::kCPU));
    if (last_per_horizon_nll_.defined())
      ar.write("last_per_horizon_nll",
               last_per_horizon_nll_.detach().to(torch::kCPU));

    // --- strict atomic write: write tmp, verify, rename, verify ---
    ar.save_to(tmp);
    std::error_code ec;
    if (!std::filesystem::exists(tmp, ec) || ec) {
      log_err("[ExpectedValue::ckpt] save failed: temporary checkpoint was not "
              "created (%s)\n",
              tmp.c_str());
      std::remove(tmp.c_str());
      return false;
    }
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
      log_err(
          "[ExpectedValue::ckpt] save failed: rename(%s -> %s) failed: %s\n",
          tmp.c_str(), path.c_str(), ec.message().c_str());
      std::remove(tmp.c_str());
      return false;
    }
    if (!std::filesystem::exists(path, ec) || ec) {
      log_err("[ExpectedValue::ckpt] save failed: final checkpoint path "
              "missing after rename (%s)\n",
              path.c_str());
      return false;
    }

    if (write_network_analytics_sidecar) {
      std::filesystem::path network_analytics_path{};
      std::string network_analytics_error;
      if (cuwacunu::piaabo::torch_compat::
              write_network_analytics_sidecar_for_checkpoint(
                  *semantic_model, path, &network_analytics_path, {},
                  &network_analytics_error)) {
        log_dbg("[ExpectedValue::network_analytics saved → %s\n",
                network_analytics_path.string().c_str());
      } else {
        log_warn(
            "[ExpectedValue::network_analytics] report skipped for '%s': %s\n",
            path.c_str(), network_analytics_error.c_str());
      }
    }

    log_info("%s[ExpectedValue::ckpt]%s saved → %s\n", ANSI_COLOR_Bright_Green,
             ANSI_COLOR_RESET, path.c_str());
    return true;

  } catch (const c10::Error &e) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed: %s\n", e.what());
    return false;
  } catch (const std::exception &e) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed: %s\n", e.what());
    return false;
  } catch (...) {
    std::remove(tmp.c_str());
    log_err("[ExpectedValue::ckpt] save failed (unknown exception)\n");
    return false;
  }
}

bool ExpectedValue::load_checkpoint(const std::string &path) {
  try {
    torch::serialize::InputArchive ar;
    // Checkpoints store CPU tensor clones; keep archive hydration CPU-bound,
    // then copy into the already constructed runtime device explicitly.
    ar.load_from(path, torch::Device(torch::kCPU));

    const int64_t format_version = ev_require_scalar_i64(ar, "format_version");
    TORCH_CHECK(format_version == kExpectedValueCheckpointFormatVersion,
                "[ExpectedValue::ckpt] unsupported checkpoint format_version=",
                format_version, " (expected ",
                kExpectedValueCheckpointFormatVersion, ").");

    const std::string saved_contract_hash =
        ev_require_string(ar, "meta/contract_hash");
    const std::string saved_component_name =
        ev_require_string(ar, "meta/component_name");
    const std::string saved_scheduler_mode =
        ev_require_string(ar, "meta/scheduler_mode");
    const int64_t saved_scheduler_batch_steps =
        ev_require_scalar_i64(ar, "meta/scheduler_batch_steps");
    const int64_t saved_scheduler_epoch_steps =
        ev_require_scalar_i64(ar, "meta/scheduler_epoch_steps");
    const int64_t saved_model_parameter_count =
        ev_require_scalar_i64(ar, "meta/model_parameter_count");
    const int64_t saved_model_buffer_count =
        ev_require_scalar_i64(ar, "meta/model_buffer_count");

    TORCH_CHECK(!saved_contract_hash.empty(),
                "[ExpectedValue::ckpt] meta/contract_hash must be non-empty.");
    TORCH_CHECK(!saved_component_name.empty(),
                "[ExpectedValue::ckpt] meta/component_name must be non-empty.");
    TORCH_CHECK(!saved_scheduler_mode.empty(),
                "[ExpectedValue::ckpt] meta/scheduler_mode must be non-empty.");
    TORCH_CHECK(
        saved_scheduler_batch_steps >= 0,
        "[ExpectedValue::ckpt] meta/scheduler_batch_steps must be >= 0.");
    TORCH_CHECK(
        saved_scheduler_epoch_steps >= 0,
        "[ExpectedValue::ckpt] meta/scheduler_epoch_steps must be >= 0.");
    TORCH_CHECK(
        saved_model_parameter_count >= 0,
        "[ExpectedValue::ckpt] meta/model_parameter_count must be >= 0.");
    TORCH_CHECK(saved_model_buffer_count >= 0,
                "[ExpectedValue::ckpt] meta/model_buffer_count must be >= 0.");
    TORCH_CHECK(saved_contract_hash == contract_hash,
                "[ExpectedValue::ckpt] contract hash mismatch (ckpt='",
                saved_contract_hash, "', runtime='", contract_hash, "').");
    TORCH_CHECK(saved_component_name == component_name,
                "[ExpectedValue::ckpt] component mismatch (ckpt='",
                saved_component_name, "', runtime='", component_name, "').");

    const std::string saved_temporal_reducer =
        ev_require_string(ar, "meta/encoding_temporal_reducer");
    const std::string runtime_temporal_reducer =
        temporal_reducer_name_(temporal_reducer_);
    TORCH_CHECK(saved_temporal_reducer == runtime_temporal_reducer,
                "[ExpectedValue::ckpt] encoding temporal reducer mismatch "
                "(ckpt='",
                saved_temporal_reducer, "', runtime='",
                runtime_temporal_reducer, "').");

    const std::string runtime_scheduler_mode =
        lr_sched ? scheduler_mode_name(lr_sched->mode) : "None";
    TORCH_CHECK(saved_scheduler_mode == runtime_scheduler_mode,
                "[ExpectedValue::ckpt] scheduler mode mismatch (ckpt='",
                saved_scheduler_mode, "', runtime='", runtime_scheduler_mode,
                "').");

    const std::string saved_manifest_schema =
        ev_require_string(ar, "meta/head_manifest_schema");
    const std::string saved_export_semantics =
        ev_require_string(ar, "meta/export_semantics");
    const std::string saved_loss_mode = ev_require_string(ar, "meta/loss_mode");
    const std::string saved_training_recipe =
        ev_require_string(ar, "meta/training_recipe");
    const std::string saved_covariance_family =
        ev_require_string(ar, "meta/covariance_family");
    const std::string saved_sigma_parameterization =
        ev_require_string(ar, "meta/sigma_parameterization");
    const std::string saved_output_layout =
        ev_require_string(ar, "meta/output_layout");
    const std::string saved_future_mask_semantics =
        ev_require_string(ar, "meta/future_mask_semantics");
    const int64_t saved_future_feature_width =
        ev_require_scalar_i64(ar, "meta/future_feature_width");
    const std::string saved_future_feature_schema_hash =
        ev_require_string(ar, "meta/future_feature_schema_hash");
    const int64_t saved_target_width =
        ev_require_scalar_i64(ar, "meta/target_width");
    const std::vector<int64_t> saved_target_feature_indices =
        ev_require_i64_vector(ar, "meta/target_feature_indices");
    const int64_t saved_encoding_dims =
        ev_require_scalar_i64(ar, "meta/encoding_dims");
    const int64_t saved_channel_count =
        ev_require_scalar_i64(ar, "meta/channel_count");
    const int64_t saved_horizon_count =
        ev_require_scalar_i64(ar, "meta/horizon_count");
    const int64_t saved_mixture_comps =
        ev_require_scalar_i64(ar, "meta/mixture_comps");
    const int64_t saved_features_hidden =
        ev_require_scalar_i64(ar, "meta/features_hidden");
    const int64_t saved_residual_depth =
        ev_require_scalar_i64(ar, "meta/residual_depth");
    const double saved_loss_eps = ev_require_scalar_double(ar, "meta/loss_eps");
    const double saved_loss_sigma_min =
        ev_require_scalar_double(ar, "meta/loss_sigma_min");
    const double saved_loss_sigma_max =
        ev_require_scalar_double(ar, "meta/loss_sigma_max");
    const int64_t saved_target_weight_count =
        ev_require_scalar_i64(ar, "meta/target_weight_count");

    TORCH_CHECK(saved_manifest_schema == kExpectedValueHeadManifestSchema,
                "[ExpectedValue::ckpt] head manifest schema mismatch (ckpt='",
                saved_manifest_schema, "', runtime='",
                kExpectedValueHeadManifestSchema, "').");
    TORCH_CHECK(saved_export_semantics == kExpectedValueExportSemantics,
                "[ExpectedValue::ckpt] export semantics mismatch (ckpt='",
                saved_export_semantics, "', runtime='",
                kExpectedValueExportSemantics, "').");
    TORCH_CHECK(saved_loss_mode == kExpectedValueLossMode,
                "[ExpectedValue::ckpt] loss mode mismatch (ckpt='",
                saved_loss_mode, "', runtime='", kExpectedValueLossMode, "').");
    TORCH_CHECK(saved_training_recipe == kExpectedValueTrainingRecipe,
                "[ExpectedValue::ckpt] training recipe mismatch (ckpt='",
                saved_training_recipe, "', runtime='",
                kExpectedValueTrainingRecipe, "').");
    TORCH_CHECK(saved_covariance_family == kExpectedValueCovarianceFamily,
                "[ExpectedValue::ckpt] covariance family mismatch (ckpt='",
                saved_covariance_family, "', runtime='",
                kExpectedValueCovarianceFamily, "').");
    TORCH_CHECK(saved_sigma_parameterization ==
                    kExpectedValueSigmaParameterization,
                "[ExpectedValue::ckpt] sigma parameterization mismatch "
                "(ckpt='",
                saved_sigma_parameterization, "', runtime='",
                kExpectedValueSigmaParameterization, "').");
    TORCH_CHECK(saved_output_layout == kExpectedValueOutputLayout,
                "[ExpectedValue::ckpt] output layout mismatch (ckpt='",
                saved_output_layout, "', runtime='", kExpectedValueOutputLayout,
                "').");
    TORCH_CHECK(saved_future_mask_semantics ==
                    kExpectedValueFutureMaskSemantics,
                "[ExpectedValue::ckpt] future mask semantics mismatch "
                "(ckpt='",
                saved_future_mask_semantics, "', runtime='",
                kExpectedValueFutureMaskSemantics, "').");
    TORCH_CHECK(saved_future_feature_width == future_feature_width_,
                "[ExpectedValue::ckpt] future feature width mismatch (ckpt=",
                saved_future_feature_width, ", runtime=", future_feature_width_,
                ").");
    TORCH_CHECK(saved_future_feature_schema_hash == future_feature_schema_hash_,
                "[ExpectedValue::ckpt] future feature schema hash mismatch "
                "(ckpt='",
                saved_future_feature_schema_hash, "', runtime='",
                future_feature_schema_hash_, "').");
    TORCH_CHECK(saved_target_width ==
                    static_cast<int64_t>(target_feature_indices_.size()),
                "[ExpectedValue::ckpt] target width mismatch (ckpt=",
                saved_target_width,
                ", runtime=", target_feature_indices_.size(), ").");
    TORCH_CHECK(saved_target_feature_indices == target_feature_indices_,
                "[ExpectedValue::ckpt] target feature indices mismatch (ckpt=",
                ev_i64_vector_string(saved_target_feature_indices),
                ", runtime=", ev_i64_vector_string(target_feature_indices_),
                ").");
    TORCH_CHECK(saved_encoding_dims == semantic_model->De,
                "[ExpectedValue::ckpt] encoding dims mismatch (ckpt=",
                saved_encoding_dims, ", runtime=", semantic_model->De, ").");
    TORCH_CHECK(saved_channel_count == semantic_model->C_axes,
                "[ExpectedValue::ckpt] channel layout mismatch (ckpt=",
                saved_channel_count, ", runtime=", semantic_model->C_axes,
                ").");
    TORCH_CHECK(saved_horizon_count == semantic_model->Hf_axes,
                "[ExpectedValue::ckpt] horizon layout mismatch (ckpt=",
                saved_horizon_count, ", runtime=", semantic_model->Hf_axes,
                ").");
    TORCH_CHECK(saved_mixture_comps == semantic_model->K,
                "[ExpectedValue::ckpt] mixture count mismatch (ckpt=",
                saved_mixture_comps, ", runtime=", semantic_model->K, ").");
    TORCH_CHECK(saved_features_hidden == semantic_model->H,
                "[ExpectedValue::ckpt] hidden width mismatch (ckpt=",
                saved_features_hidden, ", runtime=", semantic_model->H, ").");
    TORCH_CHECK(saved_residual_depth == semantic_model->depth,
                "[ExpectedValue::ckpt] residual depth mismatch (ckpt=",
                saved_residual_depth, ", runtime=", semantic_model->depth,
                ").");
    const double runtime_loss_eps = (loss_obj ? loss_obj->eps_ : 0.0);
    const double runtime_loss_sigma_min =
        (loss_obj ? loss_obj->sigma_min_ : 0.0);
    const double runtime_loss_sigma_max =
        (loss_obj ? loss_obj->sigma_max_ : 0.0);
    TORCH_CHECK(ev_manifest_double_equal(saved_loss_eps, runtime_loss_eps),
                "[ExpectedValue::ckpt] loss eps mismatch (ckpt=",
                saved_loss_eps, ", runtime=", runtime_loss_eps, ").");
    TORCH_CHECK(
        ev_manifest_double_equal(saved_loss_sigma_min, runtime_loss_sigma_min),
        "[ExpectedValue::ckpt] loss sigma_min mismatch (ckpt=",
        saved_loss_sigma_min, ", runtime=", runtime_loss_sigma_min, ").");
    TORCH_CHECK(
        ev_manifest_double_equal(saved_loss_sigma_max, runtime_loss_sigma_max),
        "[ExpectedValue::ckpt] loss sigma_max mismatch (ckpt=",
        saved_loss_sigma_max, ", runtime=", runtime_loss_sigma_max, ").");
    TORCH_CHECK(saved_target_weight_count ==
                    static_cast<int64_t>(static_feature_weights_.size()),
                "[ExpectedValue::ckpt] target weight count mismatch (ckpt=",
                saved_target_weight_count,
                ", runtime=", static_feature_weights_.size(), ").");

    // --- model state (safe, tensor-by-tensor) ---
    const auto loaded_model_counts =
        ev_load_module_state(ar, *semantic_model, "model");
    TORCH_CHECK(loaded_model_counts.parameters == saved_model_parameter_count,
                "[ExpectedValue::ckpt] model parameter count mismatch (ckpt=",
                saved_model_parameter_count,
                ", runtime=", loaded_model_counts.parameters, ").");
    TORCH_CHECK(loaded_model_counts.buffers == saved_model_buffer_count,
                "[ExpectedValue::ckpt] model buffer count mismatch (ckpt=",
                saved_model_buffer_count,
                ", runtime=", loaded_model_counts.buffers, ").");
    semantic_model->to(semantic_model->device, semantic_model->dtype,
                       /*non_blocking=*/false);

    // --- optimizer (optional) ---
    const bool expect_opt = (ev_require_scalar_i64(ar, "has_optimizer") != 0);
    if (optimizer && expect_opt) {
      torch::serialize::InputArchive oa;
      bool has_optimizer_state = true;
      try {
        ar.read("optimizer", oa);
      } catch (...) {
        has_optimizer_state = false;
      }
      TORCH_CHECK(has_optimizer_state, "[ExpectedValue::ckpt] optimizer state "
                                       "declared in checkpoint but missing.");
      optimizer->load(oa);
    }
    ev_move_optimizer_state_to_device(optimizer.get(), semantic_model->device);

    // --- scalars ---
    best_metric_ = ev_require_scalar_double(ar, "best_metric");
    best_epoch_ = static_cast<int>(ev_require_scalar_i64(ar, "best_epoch"));
    total_iters_trained_ = ev_require_scalar_i64(ar, "total_iters_trained");
    total_epochs_trained_ = ev_require_scalar_i64(ar, "total_epochs_trained");
    runtime_state.optimizer_steps =
        static_cast<int>(ev_require_scalar_i64(ar, "runtime_optimizer_steps"));
    runtime_state.trained_samples = static_cast<std::uint64_t>(
        ev_require_scalar_i64(ar, "runtime_trained_samples"));
    runtime_state.skipped_batches = static_cast<std::uint64_t>(
        ev_require_scalar_i64(ar, "runtime_skipped_batches"));
    runtime_state.last_committed_loss_mean =
        ev_require_scalar_double(ar, "runtime_last_committed_loss_mean");
    runtime_state.accum_counter = 0;
    runtime_state.has_pending_grad = false;
    runtime_state.pending_sample_count = 0;
    runtime_state.pending_loss_sum = 0.0;
    runtime_state.pending_loss_count = 0;
    if (optimizer)
      optimizer->zero_grad(training_policy.zero_grad_set_to_none);
    scheduler_batch_steps_ = saved_scheduler_batch_steps;
    scheduler_epoch_steps_ = saved_scheduler_epoch_steps;

    // --- telemetry (optional) ---
    {
      torch::Tensor t;
      if (ar_try_read_tensor(ar, "channel_ema", t) && t.defined())
        channel_ema_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_channel_nll", t) && t.defined())
        last_per_channel_nll_ = t.to(semantic_model->device);
      if (ar_try_read_tensor(ar, "last_per_horizon_nll", t) && t.defined())
        last_per_horizon_nll_ = t.to(semantic_model->device);
    }

    // --- scheduler strict restore ---
    const bool sched_serialized =
        (ev_require_scalar_i64(ar, "scheduler_serialized") != 0);
    if (lr_sched) {
      const auto mode = lr_sched->mode;
      if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
        TORCH_CHECK(scheduler_epoch_steps_ == 0,
                    "[ExpectedValue::ckpt] invalid scheduler counters for "
                    "PerBatch mode.");
      } else {
        TORCH_CHECK(scheduler_batch_steps_ == 0,
                    "[ExpectedValue::ckpt] invalid scheduler counters for "
                    "non-PerBatch mode.");
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
                    "[ExpectedValue::ckpt] scheduler marked serialized but "
                    "archive is missing.");
        TORCH_CHECK(try_load_scheduler(*lr_sched, sch_ar),
                    "[ExpectedValue::ckpt] scheduler archive present but "
                    "scheduler does not support load().");
      } else {
        if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch) {
          for (int64_t i = 0; i < scheduler_batch_steps_; ++i)
            lr_sched->step();
        } else if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch) {
          for (int64_t e = 0; e < scheduler_epoch_steps_; ++e)
            lr_sched->step();
        } else {
          TORCH_CHECK(scheduler_epoch_steps_ == 0,
                      "[ExpectedValue::ckpt] PerEpochWithMetric checkpoints "
                      "require serialized scheduler state.");
        }
      }
    }

    semantic_model->verify_ready_on_device_or_throw(
        "[ExpectedValue::ckpt load]");

    log_info("%s[ExpectedValue::ckpt]%s loaded ← %s (best=%.6f:at.%d, "
             "iters=%lld epochs=%lld, sch[b=%lld,e=%lld])\n",
             ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET, path.c_str(),
             best_metric_, best_epoch_,
             static_cast<long long>(total_iters_trained_),
             static_cast<long long>(total_epochs_trained_),
             static_cast<long long>(scheduler_batch_steps_),
             static_cast<long long>(scheduler_epoch_steps_));
    return true;

  } catch (const c10::Error &e) {
    log_err("[ExpectedValue::ckpt] load failed: %s\n", e.what());
    return false;
  } catch (const std::exception &e) {
    log_err("[ExpectedValue::ckpt] load failed: %s\n", e.what());
    return false;
  } catch (...) {
    log_err("[ExpectedValue::ckpt] load failed (unknown exception)\n");
    return false;
  }
}

bool ExpectedValue::ar_try_read_tensor(torch::serialize::InputArchive &ar,
                                       const char *key, torch::Tensor &out) {
  try {
    torch::Tensor t;
    ar.read(key, t);
    out = t;
    return true;
  } catch (...) {
    return false;
  }
}

// ---------- pretty print ----------
void ExpectedValue::display_model(bool display_semantic = false) const {
  // 1) Setup & IDs (never pass raw nullptrs to %s)
  const auto &setup =
      cuwacunu::jkimyei::jk_setup(component_name, contract_hash);
  const std::string opt_id =
      setup.opt_conf.id.empty() ? std::string("<unset>") : setup.opt_conf.id;
  const std::string sch_id =
      setup.sch_conf.id.empty() ? std::string("<unset>") : setup.sch_conf.id;
  const std::string loss_id =
      setup.loss_conf.id.empty() ? std::string("<unset>") : setup.loss_conf.id;

  // 2) Current LR
  double lr_now = 0.0;
  if (optimizer) {
    lr_now = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);
  }

  // 3) Horizon policy text
  const char *horizon_policy_str = "Uniform";
  switch (horizon_policy_) {
  case HorizonPolicy::Uniform:
    horizon_policy_str = "Uniform";
    break;
  case HorizonPolicy::NearTerm:
    horizon_policy_str = "NearTerm";
    break;
  case HorizonPolicy::VeryNearTerm:
    horizon_policy_str = "VeryNearTerm";
    break;
  }

  // 4) Static weights preview
  const auto C = semantic_model ? semantic_model->C_axes : 0;
  const auto Df = semantic_model ? semantic_model->Df : 0;

  auto preview_vec = [](const std::vector<float> &v, size_t n = 4) {
    if (v.empty())
      return std::string("none");
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(4);
    oss << "[";
    for (size_t i = 0; i < v.size() && i < n; ++i) {
      if (i)
        oss << ", ";
      oss << v[i];
    }
    if (v.size() > n)
      oss << ", ...";
    oss << "]";
    return oss.str();
  };
  auto preview_targets = [](const std::vector<int64_t> &v, size_t n = 6) {
    if (v.empty())
      return std::string("[]");
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < v.size() && i < n; ++i) {
      if (i)
        oss << ", ";
      oss << v[i];
    }
    if (v.size() > n)
      oss << ", ...";
    oss << "]";
    return oss.str();
  };

  // 5) EMA stats
  bool ema_on = use_channel_ema_weights_;
  double ema_alpha_show = ema_alpha_;
  double ema_min = 0.0, ema_max = 0.0;
  bool ema_has_values = channel_ema_.defined() && channel_ema_.numel() > 0;
  if (ema_has_values) {
    auto ema_cpu = channel_ema_.detach().to(torch::kCPU);
    ema_min = ema_cpu.min().item<double>();
    ema_max = ema_cpu.max().item<double>();
  }

  // 6) Loss params (from your MdnNLLLoss)
  const double loss_eps = (loss_obj ? loss_obj->eps_ : 0.0);
  const double s_min = (loss_obj ? loss_obj->sigma_min_ : 0.0);
  const double s_max = (loss_obj ? loss_obj->sigma_max_ : 0.0);
  const char *red_name = "mean"; // current default; update if you add options
  const char *temporal_reducer_name = temporal_reducer_name_(temporal_reducer_);

  // 7) Tiny helper for colors (wrap and reset)
  auto colorize = [](const char *color, const std::string &txt) -> std::string {
    return std::string(color) + txt + ANSI_COLOR_RESET;
  };
  auto k = [&](const char *key) {
    return colorize(ANSI_COLOR_Bright_Grey, key);
  };
  auto vS = [&](const std::string &s) {
    return colorize(ANSI_COLOR_Bright_Blue, s);
  };
  auto vD = [&](double d, int prec = 3) {
    std::ostringstream o;
    o.setf(std::ios::fixed);
    o << std::setprecision(prec) << d;
    return colorize(ANSI_COLOR_Bright_Blue, o.str());
  };
  auto vI = [&](long long i) {
    return colorize(ANSI_COLOR_Bright_Blue, std::to_string(i));
  };
  const std::string target_feature_aliases =
      ev_target_feature_aliases_string(contract_hash, target_feature_indices_);

  // 8) Compose a single, safe string
  std::ostringstream out;
  out << "\n"
      << ANSI_COLOR_Dim_Green << "\t[Value Estimator]" << ANSI_COLOR_RESET
      << "\n\t\t" << k("Optimizer:") << "                " << vS(opt_id)
      << "\n\t\t" << k("LR Scheduler:") << "             " << vS(sch_id)
      << "\n\t\t    " << k("- lr:") << "                 " << vD(lr_now, 3)
      << "\n\t\t" << k("Loss:") << "                     " << vS(loss_id)
      << "\n\t\t    " << k("- eps:") << "                " << vD(loss_eps, 2)
      << "\n\t\t    " << k("- sigma_min:") << "          " << vD(s_min, 2)
      << "\n\t\t    " << k("- sigma_max:") << "          " << vD(s_max, 2)
      << "\n\t\t    " << k("- reduction:") << "          " << vS(red_name)
      << "\n\t\t" << k("Horizon policy:") << "           "
      << vS(horizon_policy_str) << "\n\t\t    " << k("- γ_near:")
      << "             " << vD(gamma_near_, 3) << "\n\t\t    " << k("- γ_very:")
      << "             " << vD(gamma_very_, 3) << "\n\t\t" << k("Channels (C):")
      << "             " << vI(static_cast<long long>(C)) << "\n\t\t"
      << k("Encoding reducer:") << "         " << vS(temporal_reducer_name)
      << "\n\t\t    " << k("- Static ch weights:") << "  "
      << vS(preview_vec(static_channel_weights_)) << "\n\t\t"
      << k("Future feature width:") << "     "
      << vI(static_cast<long long>(future_feature_width_)) << "\n\t\t"
      << k("Future width (Df):") << "        " << vI(static_cast<long long>(Df))
      << "\n\t\t" << k("Target feature indices:") << " "
      << vS(preview_targets(target_feature_indices_)) << "\n\t\t"
      << k("Target feature aliases:") << " " << vS(target_feature_aliases)
      << "\n\t\t    " << k("- Static feat weights:") << " "
      << vS(preview_vec(static_feature_weights_)) << "\n\t\t"
      << k("Head manifest:") << "           "
      << vS(kExpectedValueHeadManifestSchema) << "\n\t\t    "
      << k("- loss mode:") << "          " << vS(kExpectedValueLossMode)
      << "\n\t\t    " << k("- training:") << "           "
      << vS(kExpectedValueTrainingRecipe) << "\n\t\t" << k("Channel EMA:")
      << "              " << vS(ema_on ? "ON" : "OFF") << "\n\t\t    "
      << k("- α:") << "                  " << vD(ema_alpha_show, 3)
      << "\n\t\t    " << k("- min:") << "                "
      << vD(ema_has_values ? ema_min : 0.0, 4) << "\n\t\t    " << k("- max:")
      << "                " << vD(ema_has_values ? ema_max : 0.0, 4) << "\n\t\t"
      << k("Grad clip:") << "                " << vD(grad_clip, 3) << "\n\t\t"
      << k("opt_threshold_reset:") << "      "
      << vI(static_cast<long long>(optimizer_threshold_reset)) << "\n\t\t"
      << k("Telemetry every:") << "          " << vI(telemetry_every_)
      << "\n\t\t" << k("Progress:") << "\n\t\t    " << k("- epochs:")
      << "             " << vI(static_cast<long long>(total_epochs_trained_))
      << "\n\t\t    " << k("- iters:") << "              "
      << vI(static_cast<long long>(total_iters_trained_)) << "\n\t\t    "
      << k("- best:") << "               " << vD(best_metric_, 6) << k(".at:")
      << vI(best_epoch_) << "\n";

  // 9) Print once, safely.
  log_dbg("%s", out.str().c_str());

  // 10) First, show the underlying MDN model spec.
  if (semantic_model && display_semantic) {
    semantic_model->display_model();
  }
}

} // namespace wikimyei
} // namespace cuwacunu
