/* test_expected_value.cpp */
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <torch/torch.h>

#include "piaabo/dconfig.h"

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

#include "wikimyei/representation/VICReg/vicreg_4d.h"
#include "wikimyei/inference/expected_value/expected_value.h"

static std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path);
  TORCH_CHECK(in.good(), "[test_expected_value] failed to open: ", path.string());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static void write_text_file(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path);
  TORCH_CHECK(out.good(), "[test_expected_value] failed to write: ", path.string());
  out << text;
  out.flush();
  TORCH_CHECK(out.good(), "[test_expected_value] failed to flush: ", path.string());
}

static std::string replace_once(std::string text, const std::string& from, const std::string& to) {
  const std::size_t pos = text.find(from);
  TORCH_CHECK(pos != std::string::npos, "[test_expected_value] replace token not found: ", from);
  text.replace(pos, from.size(), to);
  return text;
}

static std::string rewrite_device_to_cpu(std::string text) {
  const std::size_t key_pos = text.find("device:str");
  TORCH_CHECK(key_pos != std::string::npos,
              "[test_expected_value] missing device:str row in ini text.");
  const std::size_t line_start =
      (text.rfind('\n', key_pos) == std::string::npos)
          ? 0
          : (text.rfind('\n', key_pos) + 1);
  const std::size_t line_end = text.find('\n', key_pos);
  const std::size_t replace_len =
      (line_end == std::string::npos) ? (text.size() - line_start)
                                      : (line_end - line_start);
  text.replace(line_start, replace_len, "device:str = cpu # cpu | cuda:0 | gpu");
  return text;
}

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

  const std::string configured_contract_path = cuwacunu::piaabo::dconfig::config_space_t::get<
      std::string>("GENERAL", GENERAL_BOARD_CONTRACT_CONFIG_KEY);
  const std::filesystem::path contract_path(configured_contract_path);
  const std::string resolved_contract_path = contract_path.is_absolute()
      ? contract_path.string()
      : (std::filesystem::path(cuwacunu::piaabo::dconfig::config_space_t::config_folder) /
         contract_path)
            .string();
  const auto base_contract_hash =
      cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
          resolved_contract_path);
  const std::string base_vicreg_ini =
      cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
          base_contract_hash, "SPECS", "vicreg_config_filename");
  const std::string base_value_ini =
      cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
          base_contract_hash, "SPECS", "value_estimation_config_filename");

  const std::filesystem::path cpu_contract_dir = "/tmp/test_expected_value_contract_cpu";
  std::filesystem::create_directories(cpu_contract_dir);
  const std::filesystem::path cpu_vicreg_ini = cpu_contract_dir / "wikimyei_vicreg.cpu.ini";
  const std::filesystem::path cpu_value_ini = cpu_contract_dir / "wikimyei_value_estimation.cpu.ini";
  const std::filesystem::path cpu_contract = cpu_contract_dir / "default.board.contract.cpu.config";

  write_text_file(
      cpu_vicreg_ini,
      rewrite_device_to_cpu(read_text_file(std::filesystem::path(base_vicreg_ini))));
  write_text_file(
      cpu_value_ini,
      rewrite_device_to_cpu(read_text_file(std::filesystem::path(base_value_ini))));
  std::string cpu_contract_text = read_text_file(resolved_contract_path);
  cpu_contract_text =
      replace_once(cpu_contract_text, base_vicreg_ini, cpu_vicreg_ini.string());
  cpu_contract_text =
      replace_once(cpu_contract_text, base_value_ini, cpu_value_ini.string());
  write_text_file(cpu_contract, cpu_contract_text);

  const auto contract_hash =
      cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
          cpu_contract.string());
  cuwacunu::piaabo::dconfig::contract_space_t::assert_intact_or_fail_fast(
      contract_hash);
    
  // Reproducibility
  torch::manual_seed(48);

  // -----------------------------------------------------
  // Create the Dataloader
  // -----------------------------------------------------
  torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* types definition */
  std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
  using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
  using Dataset_t = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Datasample_t = cuwacunu::camahjucunu::data::observation_sample_t;
  using Sampler_t = torch::data::samplers::SequentialSampler; // using Sampler_t = torch::data::samplers::RandomSampler;

  TICK(create_dataloader_);
  auto raw_dataloader = cuwacunu::camahjucunu::data::make_obs_mm_dataloader
    <Datatype_t, Sampler_t>(INSTRUMENT, contract_hash);
  PRINT_TOCK_ms(create_dataloader_);

  // -----------------------------------------------------
  // Instantiate VICReg_4d (config-driven ctor, CPU-safe)
  // -----------------------------------------------------
  TICK(load_representation_model_);
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D representation_model(
    contract_hash,
    "VICReg_representation",
    static_cast<int>(raw_dataloader.C_),
    static_cast<int>(raw_dataloader.T_),
    static_cast<int>(raw_dataloader.D_));
  PRINT_TOCK_ms(load_representation_model_);

  // -----------------------------------------------------
  // Instantiate representation Dataloader
  // -----------------------------------------------------
  TICK(extend_dataloader_with_enbedings_);
  auto representation_dataloader =
    representation_model.make_representation_dataloader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>
      (raw_dataloader, /*use_swa=*/true, /* debug */ false);
  PRINT_TOCK_ms(extend_dataloader_with_enbedings_);
  
  // -----------------------------------------------------
  // Instantiate MDN (from configuration)
  // -----------------------------------------------------
  TICK(create_expected_value_model_);
  cuwacunu::wikimyei::ExpectedValue value_estimation_network(
      contract_hash, "MDN_value_estimation");
  PRINT_TOCK_ms(create_expected_value_model_);

  // -----------------------------------------------------
  // Training
  // -----------------------------------------------------
  const int configured_telemetry_every =
      cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
          contract_hash, "VALUE_ESTIMATION", "telemetry_every");
  const int configured_epochs =
      cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
          contract_hash, "VALUE_ESTIMATION", "n_epochs");
  const int configured_iters =
      cuwacunu::piaabo::dconfig::contract_space_t::get<int>(
          contract_hash, "VALUE_ESTIMATION", "n_iters");
  const int smoke_telemetry_every = 1;
  const int smoke_epochs = 1;
  const int smoke_iters = 1;
  std::cout << "[smoke] VALUE_ESTIMATION training limited to n_epochs=" << smoke_epochs
            << " n_iters=" << smoke_iters
            << " (configured " << configured_epochs << "/" << configured_iters
            << ", telemetry_every=" << configured_telemetry_every << ")\n";

  value_estimation_network.set_telemetry_every(
    smoke_telemetry_every
  );
  TICK(fit_value_estimation_);
  value_estimation_network.fit(representation_dataloader, 
    /* n_epochs */  smoke_epochs,
    /* n_iters */   smoke_iters,
    /* verbose */   false
  );
  TORCH_CHECK(value_estimation_network.scheduler_batch_steps_ == 0,
              "[test_expected_value] PerEpoch scheduler should not step per batch.");
  TORCH_CHECK(value_estimation_network.scheduler_epoch_steps_ == 1,
              "[test_expected_value] PerEpoch scheduler should step once per epoch.");
  PRINT_TOCK_ms(fit_value_estimation_);

  const std::filesystem::path base_jk_specs = "/cuwacunu/src/config/instructions/jkimyei_specs.dsl";
  const std::string base_specs_text = read_text_file(base_jk_specs);

  // Scheduler mode semantics: PerBatch
  const std::string per_batch_component_name = "MDN_value_estimation_perbatch_counter";
  const std::string per_batch_specs = replace_once(
      base_specs_text,
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  ConstantLR_1                  |",
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  OneCycleLR_1                  |");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash,
      per_batch_component_name,
      "MDN_value_estimation",
      per_batch_specs);
  {
    cuwacunu::wikimyei::ExpectedValue per_batch_ev(
        contract_hash, per_batch_component_name);
    per_batch_ev.fit(representation_dataloader,
                     /*n_epochs=*/1,
                     /*n_iters=*/1,
                     /*verbose=*/false);
    TORCH_CHECK(per_batch_ev.scheduler_batch_steps_ == 1,
                "[test_expected_value] PerBatch scheduler should step once for one batch.");
    TORCH_CHECK(per_batch_ev.scheduler_epoch_steps_ == 0,
                "[test_expected_value] PerBatch scheduler should not step per epoch.");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash,
      per_batch_component_name);

  // Scheduler mode semantics: PerEpochWithMetric
  const std::string metric_component_name = "MDN_value_estimation_metric_counter";
  const std::string metric_specs = replace_once(
      base_specs_text,
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  ConstantLR_1                  |",
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  ReduceLROnPlateau_1           |");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash,
      metric_component_name,
      "MDN_value_estimation",
      metric_specs);
  {
    cuwacunu::wikimyei::ExpectedValue metric_ev(
        contract_hash, metric_component_name);
    metric_ev.fit(representation_dataloader,
                  /*n_epochs=*/1,
                  /*n_iters=*/1,
                  /*verbose=*/false);
    TORCH_CHECK(metric_ev.scheduler_batch_steps_ == 0,
                "[test_expected_value] PerEpochWithMetric scheduler should not step per batch.");
    TORCH_CHECK(metric_ev.scheduler_epoch_steps_ == 1,
                "[test_expected_value] PerEpochWithMetric scheduler should step once per epoch.");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash,
      metric_component_name);

  // Optimizer threshold clamp semantics for Adam/AdamW step counters.
  {
    cuwacunu::wikimyei::ExpectedValue clamp_ev(
        contract_hash, "MDN_value_estimation");
    clamp_ev.optimizer_threshold_reset = 0;
    clamp_ev.fit(representation_dataloader,
                 /*n_epochs=*/1,
                 /*n_iters=*/1,
                 /*verbose=*/false);
    bool saw_adamw_state = false;
    for (auto& kv : clamp_ev.optimizer->state()) {
      if (auto* s = dynamic_cast<torch::optim::AdamWParamState*>(kv.second.get())) {
        saw_adamw_state = true;
        TORCH_CHECK(s->step() <= 0,
                    "[test_expected_value] expected AdamW step counter clamp at threshold 0.");
      }
    }
    TORCH_CHECK(saw_adamw_state,
                "[test_expected_value] expected AdamW optimizer state entries.");
  }
  // -----------------------------------------------------
  // Save
  // -----------------------------------------------------
  TICK(save_value_estimation_network_);
  const std::string ckpt_path =
      cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
          contract_hash, "VALUE_ESTIMATION", "model_path");
  TORCH_CHECK(value_estimation_network.save_checkpoint(ckpt_path),
              "[test_expected_value] save_checkpoint should succeed");
  PRINT_TOCK_ms(save_value_estimation_network_);

  // -----------------------------------------------------
  // Load
  // -----------------------------------------------------
  TICK(load_value_estimation_network_);
  cuwacunu::wikimyei::ExpectedValue loaded_value_estimation_network(
      contract_hash, "MDN_value_estimation");
  TORCH_CHECK(loaded_value_estimation_network.load_checkpoint(ckpt_path),
              "[test_expected_value] load_checkpoint should succeed");
  PRINT_TOCK_ms(load_value_estimation_network_);

  // -----------------------------------------------------
  // Strict checkpoint v2 negative tests
  // -----------------------------------------------------
  const std::filesystem::path tmp_dir = "/tmp/test_expected_value_ckpt";
  std::filesystem::create_directories(tmp_dir);
  TORCH_CHECK(!value_estimation_network.save_checkpoint(tmp_dir.string()),
              "[test_expected_value] save should fail when destination path is a directory");

  // Reject legacy/no-version checkpoint.
  const std::filesystem::path no_version_ckpt = tmp_dir / "expected_value_no_version.ckpt";
  {
    torch::serialize::OutputArchive legacy;
    legacy.write("best_metric", torch::tensor({0.0}));
    legacy.save_to(no_version_ckpt.string());
  }
  {
    cuwacunu::wikimyei::ExpectedValue strict_loader(contract_hash, "MDN_value_estimation");
    TORCH_CHECK(!strict_loader.load_checkpoint(no_version_ckpt.string()),
                "[test_expected_value] load should reject checkpoint missing format_version");
  }

  // Reject contract hash mismatch.
  const std::filesystem::path base_contract = cpu_contract;
  const std::filesystem::path mismatch_contract = tmp_dir / "default.board.contract.contract_mismatch.config";
  write_text_file(
      mismatch_contract,
      read_text_file(base_contract) + "\n# contract_mismatch_variant\n");
  const std::string mismatch_contract_hash =
      cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
          mismatch_contract.string());
  {
    cuwacunu::wikimyei::ExpectedValue mismatch_loader(
        mismatch_contract_hash, "MDN_value_estimation");
    TORCH_CHECK(!mismatch_loader.load_checkpoint(ckpt_path),
                "[test_expected_value] load should reject contract hash mismatch");
  }

  // Reject scheduler mode mismatch.
  const std::filesystem::path mismatch_ckpt = tmp_dir / "expected_value_scheduler_mismatch.ckpt";
  const std::string per_batch_specs_for_mismatch = replace_once(
      base_specs_text,
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  ConstantLR_1                  |",
      "|  MDN_value_estimation  |  AdamW_1         |  NLLLoss_1             |  OneCycleLR_1                  |");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash,
      "MDN_value_estimation",
      "MDN_value_estimation",
      per_batch_specs_for_mismatch);
  {
    cuwacunu::wikimyei::ExpectedValue per_batch_mode_model(
        contract_hash, "MDN_value_estimation");
    TORCH_CHECK(per_batch_mode_model.save_checkpoint(mismatch_ckpt.string()),
                "[test_expected_value] expected save to succeed for scheduler mismatch fixture");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash,
      "MDN_value_estimation");
  {
    cuwacunu::wikimyei::ExpectedValue per_epoch_loader(
        contract_hash, "MDN_value_estimation");
    TORCH_CHECK(!per_epoch_loader.load_checkpoint(mismatch_ckpt.string()),
                "[test_expected_value] load should reject scheduler mode mismatch");
  }

  // -----------------------------------------------------
  // Dashboards: fetch latest vectors (CPU tensors)
  // -----------------------------------------------------
  TICK(estimation_network_dashboards_);
  auto ch = value_estimation_network.get_last_per_channel_nll();  // [C] on CPU
  auto hz = value_estimation_network.get_last_per_horizon_nll();  // [Hf] on CPU
  PRINT_TOCK_ms(estimation_network_dashboards_);

  return 0;
}
