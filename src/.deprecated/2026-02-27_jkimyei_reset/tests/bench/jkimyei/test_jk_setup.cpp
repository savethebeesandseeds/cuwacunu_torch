// test_jk_setup.cpp (minimal)
// Runs a tiny training loop showing optimizer, LR scheduler, and loss.

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <torch/torch.h>

#include "piaabo/dconfig.h"
#include "jkimyei/training_setup/jk_setup.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"

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

static std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path);
  TORCH_CHECK(in.good(), "[test_jk_setup] failed to open: ", path.string());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static void write_text_file(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path);
  TORCH_CHECK(out.good(), "[test_jk_setup] failed to write: ", path.string());
  out << text;
  out.flush();
  TORCH_CHECK(out.good(), "[test_jk_setup] failed to flush: ", path.string());
}

static std::string replace_once(std::string text, const std::string& from, const std::string& to) {
  const std::size_t pos = text.find(from);
  TORCH_CHECK(pos != std::string::npos, "[test_jk_setup] replace token not found: ", from);
  text.replace(pos, from.size(), to);
  return text;
}

int main() {
  try {
    // Load jkimyei specs DSL from config
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    const std::string contract_hash =
        cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();
    std::string instruction =
        cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_dsl(
            contract_hash);

    TICK(jkimyeiSpecsPipeline_loadGrammar);
    auto trainPipe = cuwacunu::camahjucunu::dsl::jkimyeiSpecsPipeline(
        cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_grammar(
            contract_hash));
    PRINT_TOCK_ns(jkimyeiSpecsPipeline_loadGrammar);

    TICK(decode_Instruction);
    auto inst = trainPipe.decode(instruction);
    PRINT_TOCK_ns(decode_Instruction);
    (void)inst;

    // Contract isolation test:
    // Same component name under two contract hashes must resolve independently.
    const std::filesystem::path base_contract =
        "/cuwacunu/src/config/default.board.contract.config";
    const std::filesystem::path base_jk_specs =
        "/cuwacunu/src/config/instructions/jkimyei_specs.dsl";
    const std::filesystem::path tmp_dir = "/tmp/jk_setup_contract_isolation";
    const std::filesystem::path alt_contract =
        tmp_dir / "default.board.contract.isolation.config";
    std::filesystem::create_directories(tmp_dir);

    const std::string alt_specs_text = replace_once(
        read_text_file(base_jk_specs),
        "|  basic_test            |  Adam_1          |  MeanSquaredError_1    |  StepLR_1                      |",
        "|  basic_test            |  Adam_1          |  MeanSquaredError_1    |  ConstantLR_1                  |");
    write_text_file(
        alt_contract,
        read_text_file(base_contract) + "\n# jk_setup isolation hash variant\n");

    const std::string hash_a =
        cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
            base_contract.string());
    const std::string hash_b =
        cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
            alt_contract.string());
    TORCH_CHECK(hash_a != hash_b,
                "[test_jk_setup] expected distinct contract hashes for isolation test.");

    cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
        hash_b,
        "basic_test",
        "basic_test",
        alt_specs_text);

    auto& setup_a = cuwacunu::jkimyei::jk_setup("basic_test", hash_a);
    auto& setup_b = cuwacunu::jkimyei::jk_setup("basic_test", hash_b);
    TORCH_CHECK(setup_a.sch_conf.id == "StepLR_1",
                "[test_jk_setup] base contract scheduler mismatch.");
    TORCH_CHECK(setup_b.sch_conf.id == "ConstantLR_1",
                "[test_jk_setup] alt contract scheduler mismatch.");
    cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
        hash_b,
        "basic_test");

    // Build setup: optimizer + scheduler
    TICK(build_component);
    auto& setup = setup_a;
    PRINT_TOCK_ns(build_component);

    // Tiny model + synthetic regression data
    TinyReg net(4, 1);
    net->train();
    const int64_t N = 16;
    auto X = torch::randn({N, 4});
    auto y = X.sum(1, /*keepdim=*/true);            // regression target
    // If need logits: auto y_idx = (X.sum(1) > 0).to(torch::kLong); // classes

    // Build optimizer and LR scheduler
    auto opt_uptr = setup.opt_builder->build(net->parameters());
    auto& opt = *opt_uptr;
    auto sched_uptr = setup.sched_builder->build(opt);

    std::cout << "[init] lr=" << current_lr(opt) << "\n";

    // Minimal loop
    const int epochs = 1;
    for (int e = 1; e <= epochs; ++e) {
      auto pred = net->forward(X);                                  // [N,1]
      auto loss = torch::mse_loss(pred, y, at::Reduction::Mean);    // minimal regression loss

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
