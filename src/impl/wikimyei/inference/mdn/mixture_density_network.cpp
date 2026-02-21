/* mixture_density_network.cpp */
#include "wikimyei/inference/mdn/mixture_density_network.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// --- ctor ---
MdnModelImpl::MdnModelImpl(
    int64_t De_,
    int64_t Dy_,
    int64_t C_,
    int64_t Hf_,
    int64_t K_,
    int64_t H_,
    int64_t depth_,
    torch::Dtype dtype_,
    torch::Device device_,
    bool display_model_)
: De(De_),
  Dy(Dy_),
  C_axes(C_),
  Hf_axes(Hf_),
  K(K_),
  H(H_),
  depth(depth_),
  dtype(dtype_),
  device(device_) {
  TORCH_CHECK(C_axes > 0,  "[MdnModelImpl] C (channels) must be >= 1");
  TORCH_CHECK(Hf_axes > 0, "[MdnModelImpl] Hf (horizons) must be >= 1");
  TORCH_CHECK(De  > 0 && Dy > 0 && K > 0 && H > 0 && depth >= 0,
              "[MdnModelImpl] invalid dims: De,Dy,K,H must be >0; depth >=0");

  // Build trunk and heads
  BackboneOptions bopt{De, H, depth};
  backbone = register_module("backbone", Backbone(bopt));
  ch_heads = register_module("ch_heads", ChannelHeads(C_axes, Hf_axes, Dy, K, H));

  // Place module on requested device/dtype before any forward pass
  this->to(device, dtype, /*non_blocking=*/false);
  
  if(display_model_) { display_model(); }

  warm_up();
}

// --- temporal pooling ---
torch::Tensor MdnModelImpl::temporal_pool(const torch::Tensor& enc) {
  if (enc.dim() == 2) return enc;         // [B,De]
  if (enc.dim() == 3) return enc.mean(1); // [B,T',De] → pooled [B,De]
  TORCH_CHECK(false, "[MdnModelImpl::temporal_pool] encoding must be [B,De] or [B,T',De]");
}

// --- forward assuming [B,De] ---
MdnOut MdnModelImpl::forward(const torch::Tensor& x) {
  auto h = backbone->forward(x);        // [B,H]
  return ch_heads->forward(h);          // -> {log_pi,mu,sigma}
}

// --- forward from encoding [B,De] or [B,T',De] ---
MdnOut MdnModelImpl::forward_from_encoding(const torch::Tensor& encoding) {
  auto x = temporal_pool(encoding);     // [B,De]
  auto h = backbone->forward(x);        // [B,H]
  return ch_heads->forward(h);          // -> {log_pi,mu,sigma}
}

torch::Tensor MdnModelImpl::expectation_from_encoding(const torch::Tensor& encoding) {
  return mdn_expectation(forward_from_encoding(encoding));
}

// --- warm-up ---
void MdnModelImpl::warm_up() {
  if (device.is_cuda()) {
    torch::NoGradGuard ng;
    const bool was_training = this->is_training();
    this->eval();
    auto x = torch::zeros({2, De}, torch::TensorOptions().dtype(dtype).device(device));
    (void)forward(x);
    torch::cuda::synchronize();
    this->train(was_training);
  }
}

// --- placeholder inference ---
std::vector<torch::Tensor> MdnModelImpl::inference(const torch::Tensor& /*enc*/, const InferenceConfig& /*cfg*/) {
  return {};
}

// --- pretty print ---
void MdnModelImpl::display_model() const {
  std::string dev = device.str();
  const char* fmt =
    "\n%s \t[MDN-per-channel] %s\n"
    "\t\t%s%-25s%s %s%-8lld%s\n"  // De
    "\t\t%s%-25s%s %s%-8lld%s\n"  // Dy
    "\t\t%s%-25s%s %s%-8lld%s\n"  // K
    "\t\t%s%-25s%s %s%-8lld%s\n"  // feature_dim
    "\t\t%s%-25s%s %s%-8lld%s\n"  // depth
    "\t\t%s%-25s%s %s%-8lld%s\n"  // channels C
    "\t\t%s%-25s%s %s%-8lld%s\n"  // horizons Hf
    "\t\t%s%-25s%s %s%-8s%s\n";   // device

  log_info(fmt,
    ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Input dims (De):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, De, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Target dims (Dy):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, Dy, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Mixture comps (K):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, K, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Feature dim:",      ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, H, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Depth:",            ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, depth, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Channels (C):",     ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, C_axes, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Horizons (Hf):",    ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, Hf_axes, ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Grey, "Device:",           ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, dev.c_str(), ANSI_COLOR_RESET
  );
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
