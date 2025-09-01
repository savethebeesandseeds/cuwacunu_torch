/* test_mixture_density_network.cpp */
#include <iostream>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network.h"

int main() {
  // torch::autograd::AnomalyMode::set_enabled(true);
  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);
  WARM_UP_CUDA();

  /* set the test variables */
  const char* config_folder = "/cuwacunu/src/config/";

  /* read the config */
  TICK(read_config_);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  PRINT_TOCK_ms(read_config_);
    
  // Reproducibility
  torch::manual_seed(48);

  // Dummy example: B=64, De=72 (rep), Dy=1
  const int64_t B  = 64;
  const int64_t De = 72;
  const int64_t Dy = 1;

  // IMPORTANT: our config/instructions must already be available to dconfig/jk_setup.
  // The strings below must match our .config target and <training_components>.instruction target.
  // e.g., config contains mixture_comps, features_hidden, residual_depth, dtype, device, grad_clip, optimizer_threshold_reset
  cuwacunu::wikimyei::mdn::MdnModel model(
      "VALUE_ESTIMATION",        /* target_conf  (.config) */
      "MDN_value_estimation",    /* target_setup (<training_components>.instruction) */
      De, Dy
  );

  model->eval(); // not strictly required, but avoids any dropout/bn surprises if later added

  // Make fake batch ON THE SAME DEVICE/DTYPE AS THE MODEL
  torch::Device device = torch::kCPU;
  torch::Dtype  dtype  = torch::kFloat32;
  if (!model->parameters().empty()) {
    device = model->parameters().front().device();
    dtype  = model->parameters().front().scalar_type();
  }
  auto opts = torch::TensorOptions().dtype(dtype).device(device);

  auto x = torch::randn({B, De}, opts);
  auto y = torch::randn({B, Dy}, opts);

  // Forward
  auto out  = model->forward(x);

  // Basic checks (won’t throw unless shapes are off)
  TORCH_CHECK(out.log_pi.sizes() == torch::IntArrayRef({B, out.mu.size(1)}), "log_pi shape mismatch");
  TORCH_CHECK(out.mu.dim() == 3 && out.sigma.dim() == 3, "mu/sigma must be [B,K,Dy]");

  // Loss & simple stats
  auto nll = cuwacunu::wikimyei::mdn::mdn_nll(out, y);
  std::cout << "NLL: " << nll.item<double>() << "\n";

  auto mean_pred = cuwacunu::wikimyei::mdn::mdn_expectation(out);   // [B, Dy]
  std::cout << "E[y|x] mean(abs): " << mean_pred.abs().mean().item<double>() << "\n";

  auto y_smpl = cuwacunu::wikimyei::mdn::mdn_sample_one_step(out);  // [B, Dy]
  std::cout << "Sample mean(abs): " << y_smpl.abs().mean().item<double>() << "\n";

  return 0;
}
