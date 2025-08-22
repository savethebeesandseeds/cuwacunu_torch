/* test_mixture_density_network.cpp */
#include <torch/torch.h>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network.h"

// =============================
// Test Mixture Density Networks
// =============================
int main() {
  // Dummy example: B=64, Dx=72 (rep), Dy=1, K=5
  const int64_t B = 64, Dx = 72, Dy = 1, K = 5;
  cuwacunu::wikimyei::mdn::MdnModelOptions opt{Dx, /*feature_dim=*/256, /*depth=*/3, Dy, K};
  auto model = cuwacunu::wikimyei::mdn::MdnModel(opt);

  // Fake batch
  auto x = torch::randn({B, Dx});
  auto y = torch::randn({B, Dy});

  auto out = model->forward(x);
  auto loss = cuwacunu::wikimyei::mdn::mdn_nll(out, y);
  std::cout << "NLL: " << loss.item<double>() << std::endl;

  auto mean_pred = cuwacunu::wikimyei::mdn::mdn_expectation(out);
  std::cout << "E[y|x] mean abs: " << mean_pred.abs().mean().item<double>() << std::endl;

  auto gen = torch::make_generator<torch::CPUGeneratorImpl>(123);
  auto y_smpl = cuwacunu::wikimyei::mdn::mdn_sample_one_step(out);
  std::cout << "Sample abs mean: " << y_smpl.abs().mean().item<double>() << std::endl;

  return 0;
}
