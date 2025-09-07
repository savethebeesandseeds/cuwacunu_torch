// test_jk_setup.cpp (minimal)
// Runs a tiny training loop showing optimizer, LR scheduler, and loss.

#include <iostream>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "jkimyei/training_setup/jk_setup.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

static inline double current_lr(const torch::optim::Optimizer& opt) {
  const auto& groups = opt.param_groups();
  return groups.empty() ? 0.0 : groups[0].options().get_lr();
}

struct TinyRegImpl : torch::nn::Module {
  torch::nn::Linear fc{nullptr};
  TinyRegImpl(int in=4, int out=1) : fc(register_module("fc", torch::nn::Linear(in, out))) {}
  torch::Tensor forward(const torch::Tensor& x) { return fc(x); }
};
TORCH_MODULE(TinyReg);

int main() {
  try {
    // Load training instruction from your config
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction();

    TICK(trainingPipeline_loadGrammar);
    auto trainPipe = cuwacunu::camahjucunu::BNF::trainingPipeline();
    PRINT_TOCK_ns(trainingPipeline_loadGrammar);

    TICK(decode_Instruction);
    auto inst = trainPipe.decode(instruction);
    PRINT_TOCK_ns(decode_Instruction);

    // Build setup: loss + optimizer + scheduler
    TICK(build_component);
    auto setup = cuwacunu::jkimyei::build_training_setup_component(inst, "basic_test");
    PRINT_TOCK_ns(build_component);

    // Tiny model + synthetic regression data
    TinyReg net(4, 1);
    net->train();
    const int64_t N = 128;
    auto X = torch::randn({N, 4});
    auto y = X.sum(1, /*keepdim=*/true);            // regression target
    // If you ever need logits: auto y_idx = (X.sum(1) > 0).to(torch::kLong); // classes

    // Build optimizer and LR scheduler
    auto opt_uptr = setup.opt_builder->build(net->parameters());
    auto& opt = *opt_uptr;
    auto sched_uptr = setup.sched_builder->build(opt);

    std::cout << "[init] lr=" << current_lr(opt) << "\n";

    // Minimal loop
    const int epochs = 10;
    for (int e = 1; e <= epochs; ++e) {
      auto pred = net->forward(X);                                  // [N,1]
      cuwacunu::jkimyei::OutView out = cuwacunu::jkimyei::OutView::from_pred(pred);
      auto loss = (*setup.loss)(out, y);                             // uses your configured loss

      opt.zero_grad();
      loss.backward();
      opt.step();
      sched_uptr->step();

      std::cout << "[epoch " << e << "] loss=" << loss.item<double>()
                << "  lr=" << current_lr(opt) << "\n";
    }

    std::cout << "Done.\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 2;
  } catch (...) {
    std::cerr << "Unknown exception\n";
    return 3;
  }
}
