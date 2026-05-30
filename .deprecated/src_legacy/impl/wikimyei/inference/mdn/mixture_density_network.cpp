/* mixture_density_network.cpp */
#include "wikimyei/inference/mdn/mixture_density_network.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {
namespace {

struct module_training_mode_guard_t {
  torch::nn::Module &module;
  const bool was_training;

  explicit module_training_mode_guard_t(torch::nn::Module &module_)
      : module(module_), was_training(module_.is_training()) {}

  ~module_training_mode_guard_t() { module.train(was_training); }
};

inline bool tensor_matches_device_(const torch::Tensor &tensor,
                                   const torch::Device &expected) {
  if (!tensor.defined())
    return true;
  const auto actual = tensor.device();
  if (actual.type() != expected.type())
    return false;
  if (!expected.has_index())
    return true;
  return actual.has_index() && actual.index() == expected.index();
}

inline void assert_tensor_device_(const torch::Tensor &tensor,
                                  const torch::Device &expected,
                                  const char *context, const char *name) {
  TORCH_CHECK(tensor_matches_device_(tensor, expected), context, " tensor '",
              name, "' is on ",
              tensor.defined() ? tensor.device().str()
                               : std::string("undefined"),
              " but expected ", expected.str());
}

} // namespace

// --- ctor ---
MdnModelImpl::MdnModelImpl(int64_t De_, int64_t Df_, int64_t C_, int64_t Hf_,
                           int64_t K_, int64_t H_, int64_t depth_,
                           torch::Dtype dtype_, torch::Device device_,
                           bool display_model_)
    : De(De_), Df(Df_), C_axes(C_), Hf_axes(Hf_), K(K_), H(H_), depth(depth_),
      dtype(dtype_), device(device_) {
  TORCH_CHECK(C_axes > 0, "[MdnModelImpl] C (channels) must be >= 1");
  TORCH_CHECK(Hf_axes > 0, "[MdnModelImpl] Hf (horizons) must be >= 1");
  TORCH_CHECK(De > 0 && Df > 0 && K > 0 && H > 0 && depth >= 0,
              "[MdnModelImpl] invalid dims: De,Df,K,H must be >0; depth >=0");

  // Build trunk and heads
  BackboneOptions bopt{De, H, depth};
  backbone = register_module("backbone", Backbone(bopt));
  ch_heads =
      register_module("ch_heads", ChannelHeads(C_axes, Hf_axes, Df, K, H));

  // Place module on requested device/dtype before any forward pass
  this->to(device, dtype, /*non_blocking=*/false);
  verify_ready_on_device_or_throw("[MdnModelImpl::ctor]");

  if (display_model_) {
    display_model();
  }

  warm_up();
}

// --- temporal pooling ---
torch::Tensor MdnModelImpl::temporal_pool(const torch::Tensor &enc) {
  if (enc.dim() == 2)
    return enc; // [B,De]
  TORCH_CHECK(false, "[MdnModelImpl::temporal_pool] encoding must be [B,De]. "
                     "Rank-3 [B,Hx,De] encodings must be reduced by an "
                     "explicit ExpectedValue temporal reducer first.");
}

// --- forward assuming [B,De] ---
MdnOut MdnModelImpl::forward(const torch::Tensor &x) {
  auto h = backbone->forward(x); // [B,H]
  return ch_heads->forward(h);   // -> {log_pi,mu,sigma}
}

// --- forward from encoding [B,De] ---
MdnOut MdnModelImpl::forward_from_encoding(const torch::Tensor &encoding) {
  auto x = temporal_pool(encoding); // [B,De]
  auto h = backbone->forward(x);    // [B,H]
  return ch_heads->forward(h);      // -> {log_pi,mu,sigma}
}

torch::Tensor
MdnModelImpl::expectation_from_encoding(const torch::Tensor &encoding) {
  return mdn_expectation(forward_from_encoding(encoding));
}

// --- warm-up ---
void MdnModelImpl::warm_up() {
  if (device.is_cuda()) {
    torch::NoGradGuard ng;
    module_training_mode_guard_t training_mode_guard(*this);
    this->eval();
    auto x = torch::zeros({2, De},
                          torch::TensorOptions().dtype(dtype).device(device));
    (void)forward(x);
    torch::cuda::synchronize();
  }
}

void MdnModelImpl::assert_resident_on_device_or_throw(
    const char *context) const {
  for (const auto &named_param : named_parameters(/*recurse=*/true)) {
    TORCH_CHECK(tensor_matches_device_(named_param.value(), device), context,
                " parameter '", named_param.key(), "' is on ",
                named_param.value().device().str(), " but expected ",
                device.str());
  }
  for (const auto &named_buffer : named_buffers(/*recurse=*/true)) {
    TORCH_CHECK(tensor_matches_device_(named_buffer.value(), device), context,
                " buffer '", named_buffer.key(), "' is on ",
                named_buffer.value().device().str(), " but expected ",
                device.str());
  }
}

void MdnModelImpl::verify_ready_on_device_or_throw(const char *context) {
  assert_resident_on_device_or_throw(context);
  if (!device.is_cuda())
    return;

  torch::NoGradGuard ng;
  module_training_mode_guard_t training_mode_guard(*this);
  this->eval();

  auto x =
      torch::zeros({2, De}, torch::TensorOptions().dtype(dtype).device(device));
  const auto out = forward_from_encoding(x);
  assert_tensor_device_(out.log_pi, device, context, "log_pi");
  assert_tensor_device_(out.mu, device, context, "mu");
  assert_tensor_device_(out.sigma, device, context, "sigma");
  const auto expectation = mdn_expectation(out);
  assert_tensor_device_(expectation, device, context, "expectation");
  torch::cuda::synchronize();
}

// --- placeholder inference ---
std::vector<torch::Tensor>
MdnModelImpl::inference(const torch::Tensor & /*enc*/,
                        const InferenceConfig & /*cfg*/) {
  return {};
}

// --- pretty print ---
void MdnModelImpl::display_model() const {
  std::string dev = device.str();
  const char *fmt = "\n%s \t[MDN-per-channel] %s\n"
                    "\t\t%s%-25s%s %s%-8lld%s\n" // De
                    "\t\t%s%-25s%s %s%-8lld%s\n" // Df
                    "\t\t%s%-25s%s %s%-8lld%s\n" // K
                    "\t\t%s%-25s%s %s%-8lld%s\n" // feature_dim
                    "\t\t%s%-25s%s %s%-8lld%s\n" // depth
                    "\t\t%s%-25s%s %s%-8lld%s\n" // channels C
                    "\t\t%s%-25s%s %s%-8lld%s\n" // horizons Hf
                    "\t\t%s%-25s%s %s%-8s%s\n";  // device

  log_dbg(fmt, ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, ANSI_COLOR_Bright_Grey,
          "Input dims (De):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, De,
          ANSI_COLOR_RESET, ANSI_COLOR_Bright_Grey,
          "Future width (Df):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, Df,
          ANSI_COLOR_RESET, ANSI_COLOR_Bright_Grey,
          "Mixture comps (K):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, K,
          ANSI_COLOR_RESET, ANSI_COLOR_Bright_Grey,
          "Feature dim:", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, H,
          ANSI_COLOR_RESET, ANSI_COLOR_Bright_Grey, "Depth:", ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Blue, depth, ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Grey, "Channels (C):", ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Blue, C_axes, ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Grey, "Horizons (Hf):", ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Blue, Hf_axes, ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Grey, "Device:", ANSI_COLOR_RESET,
          ANSI_COLOR_Bright_Blue, dev.c_str(), ANSI_COLOR_RESET);
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
