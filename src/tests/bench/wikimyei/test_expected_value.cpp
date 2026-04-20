/* test_expected_value.cpp */
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <torch/torch.h>

#include "piaabo/dconfig.h"

#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"
#include "camahjucunu/types/types_utils.h"

#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "iitepi/runtime_binding/runtime_binding_space_t.h"

#include "wikimyei/inference/expected_value/expected_value.h"
#include "wikimyei/representation/VICReg/vicreg_4d.h"

static std::string read_text_file(const std::filesystem::path &path) {
  std::ifstream in(path);
  TORCH_CHECK(in.good(),
              "[test_expected_value] failed to open: ", path.string());
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

static void write_text_file(const std::filesystem::path &path,
                            const std::string &text) {
  std::ofstream out(path);
  TORCH_CHECK(out.good(),
              "[test_expected_value] failed to write: ", path.string());
  out << text;
  out.flush();
  TORCH_CHECK(out.good(),
              "[test_expected_value] failed to flush: ", path.string());
}

static std::string replace_once(std::string text, const std::string &from,
                                const std::string &to) {
  const std::size_t pos = text.find(from);
  TORCH_CHECK(pos != std::string::npos,
              "[test_expected_value] replace token not found: ", from);
  text.replace(pos, from.size(), to);
  return text;
}

static std::string replace_all_optional(std::string text,
                                        const std::string &from,
                                        const std::string &to) {
  std::size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos) {
    text.replace(pos, from.size(), to);
    pos += to.size();
  }
  return text;
}

static std::string extract_quoted_after(const std::string &text,
                                        const std::string &marker) {
  const std::size_t marker_pos = text.rfind(marker);
  TORCH_CHECK(marker_pos != std::string::npos,
              "[test_expected_value] marker not found: ", marker);
  const std::size_t first_quote = text.find('"', marker_pos);
  TORCH_CHECK(
      first_quote != std::string::npos,
      "[test_expected_value] opening quote not found after marker: ", marker);
  const std::size_t second_quote = text.find('"', first_quote + 1);
  TORCH_CHECK(
      second_quote != std::string::npos,
      "[test_expected_value] closing quote not found after marker: ", marker);
  return text.substr(first_quote + 1, second_quote - first_quote - 1);
}

static std::string replace_section_after(std::string text,
                                         const std::string &marker,
                                         const std::string &section_name,
                                         const std::string &replacement) {
  const std::size_t marker_pos = text.find(marker);
  TORCH_CHECK(marker_pos != std::string::npos,
              "[test_expected_value] marker not found: ", marker);
  const std::string section_header = "    [" + section_name + "]";
  const std::size_t section_pos = text.find(section_header, marker_pos);
  TORCH_CHECK(section_pos != std::string::npos,
              "[test_expected_value] section not found: ", section_name);
  const std::size_t next_section =
      text.find("\n    [", section_pos + section_header.size());
  const std::size_t section_end =
      (next_section == std::string::npos) ? text.size() : next_section;
  text.replace(section_pos, section_end - section_pos, replacement);
  return text;
}

static std::string rewrite_device_to_cpu(std::string text) {
  std::size_t key_pos = text.find("device:str");
  if (key_pos == std::string::npos) {
    key_pos = text.find("device[");
  }
  TORCH_CHECK(key_pos != std::string::npos,
              "[test_expected_value] missing device typed row in "
              "latent_lineage_state DSL text.");
  const std::size_t line_start =
      (text.rfind('\n', key_pos) == std::string::npos)
          ? 0
          : (text.rfind('\n', key_pos) + 1);
  const std::size_t line_end = text.find('\n', key_pos);
  const std::size_t replace_len = (line_end == std::string::npos)
                                      ? (text.size() - line_start)
                                      : (line_end - line_start);
  text.replace(line_start, replace_len,
               "device:str = cpu # cpu | cuda:0 | gpu");
  return text;
}

static std::string rewrite_config_key_line(std::string text,
                                           const std::string &key,
                                           const std::string &replacement) {
  const std::size_t key_pos = text.find(key);
  TORCH_CHECK(key_pos != std::string::npos,
              "[test_expected_value] missing config key row: ", key);
  const std::size_t row_pos =
      (key_pos < text.size() && text[key_pos] == '\n') ? key_pos + 1 : key_pos;
  const std::size_t line_start =
      (text.rfind('\n', row_pos) == std::string::npos)
          ? 0
          : (text.rfind('\n', row_pos) + 1);
  const std::size_t line_end = text.find('\n', row_pos);
  const std::size_t replace_len = (line_end == std::string::npos)
                                      ? (text.size() - line_start)
                                      : (line_end - line_start);
  text.replace(line_start, replace_len, replacement);
  return text;
}

static void write_archive_string(torch::serialize::OutputArchive &archive,
                                 const std::string &key,
                                 const std::string &value) {
  auto tensor = torch::empty({static_cast<int64_t>(value.size())},
                             torch::TensorOptions().dtype(torch::kChar));
  if (tensor.numel() > 0) {
    std::memcpy(tensor.data_ptr<int8_t>(), value.data(), value.size());
  }
  archive.write(key, tensor);
}

int main() {
  // torch::autograd::AnomalyMode::set_enabled(true);
  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);
  WARM_UP_CUDA();

  /* set the test variables */
  const char *global_config_path = "/cuwacunu/src/config/.config";

  /* read the config */
  TICK(read_config_);
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
  cuwacunu::iitepi::config_space_t::update_config();
  PRINT_TOCK_ms(read_config_);

  const std::filesystem::path base_campaign =
      cuwacunu::iitepi::config_space_t::effective_campaign_dsl_path();
  const std::string base_campaign_text = read_text_file(base_campaign);
  const std::string base_contract_ref =
      extract_quoted_after(base_campaign_text, "IMPORT_CONTRACT");
  const std::string base_wave_ref =
      extract_quoted_after(base_campaign_text, "FROM");
  const std::filesystem::path base_contract_path =
      (base_campaign.parent_path() / base_contract_ref).lexically_normal();
  const std::filesystem::path base_wave_path =
      (base_campaign.parent_path() / base_wave_ref).lexically_normal();
  const std::string base_contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          base_contract_path.string());
  const auto base_contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(base_contract_hash);
  const std::string resolved_contract_path =
      base_contract_itself->config_file_path;
  const std::filesystem::path contract_dir =
      std::filesystem::path(resolved_contract_path).parent_path();
  const std::string base_vicreg_contract_ref =
      base_contract_itself->get<std::string>("ASSEMBLY",
                                             "__vicreg_config_dsl_file");
  const std::string base_expected_value_contract_ref =
      base_contract_itself->get<std::string>(
          "ASSEMBLY", "__expected_value_config_dsl_file");
  const std::string base_vicreg_dsl =
      (contract_dir / base_vicreg_contract_ref).lexically_normal().string();
  const std::string base_expected_value_dsl =
      (contract_dir / base_expected_value_contract_ref)
          .lexically_normal()
          .string();

  const std::filesystem::path cpu_contract_dir =
      "/tmp/test_expected_value_contract_cpu";
  std::filesystem::create_directories(cpu_contract_dir);
  const std::filesystem::path cpu_vicreg_dsl =
      cpu_contract_dir / "wikimyei_vicreg.cpu.dsl";
  const std::filesystem::path cpu_expected_value_dsl =
      cpu_contract_dir / "wikimyei_expected_value.cpu.dsl";
  const std::filesystem::path cpu_contract =
      cpu_contract_dir / "default.runtime_binding.contract.cpu.config";
  const std::filesystem::path cpu_campaign =
      cpu_contract_dir / "default.iitepi.campaign.cpu.dsl";

  std::string cpu_vicreg_text = rewrite_device_to_cpu(
      read_text_file(std::filesystem::path(base_vicreg_dsl)));
  cpu_vicreg_text = rewrite_config_key_line(
      std::move(cpu_vicreg_text), "\nnetwork_design_dsl_file",
      "network_design_dsl_file:str = "
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.representation.vicreg.network_design.dsl");
  cpu_vicreg_text = rewrite_config_key_line(
      std::move(cpu_vicreg_text), "\njkimyei_dsl_file",
      "jkimyei_dsl_file:str = "
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl");
  write_text_file(cpu_vicreg_dsl, cpu_vicreg_text);

  std::string cpu_expected_value_text = rewrite_device_to_cpu(
      read_text_file(std::filesystem::path(base_expected_value_dsl)));
  cpu_expected_value_text = rewrite_config_key_line(
      std::move(cpu_expected_value_text), "\nnetwork_design_dsl_file",
      "network_design_dsl_file:str = "
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.inference.mdn.expected_value.network_design.dsl");
  cpu_expected_value_text = rewrite_config_key_line(
      std::move(cpu_expected_value_text), "\njkimyei_dsl_file",
      "jkimyei_dsl_file:str = "
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl");
  write_text_file(cpu_expected_value_dsl, cpu_expected_value_text);
  std::string cpu_contract_text = read_text_file(resolved_contract_path);
  cpu_contract_text = replace_once(cpu_contract_text, base_vicreg_contract_ref,
                                   cpu_vicreg_dsl.string());
  cpu_contract_text =
      replace_once(cpu_contract_text, base_expected_value_contract_ref,
                   cpu_expected_value_dsl.string());
  cpu_contract_text = replace_all_optional(
      std::move(cpu_contract_text), "default.tsi.source.dataloader.sources.dsl",
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.source.dataloader.sources.dsl");
  cpu_contract_text =
      replace_all_optional(std::move(cpu_contract_text),
                           "default.tsi.source.dataloader.channels.dsl",
                           "/cuwacunu/src/config/instructions/defaults/"
                           "default.tsi.source.dataloader.channels.dsl");
  cpu_contract_text = replace_all_optional(
      std::move(cpu_contract_text), "default.iitepi.circuit.dsl",
      "/cuwacunu/src/config/instructions/defaults/default.iitepi.circuit.dsl");
  cpu_contract_text = replace_all_optional(
      std::move(cpu_contract_text),
      "default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl",
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl");
  cpu_contract_text = replace_all_optional(
      std::move(cpu_contract_text),
      "default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl",
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl");
  cpu_contract_text = replace_all_optional(
      std::move(cpu_contract_text),
      "/cuwacunu/src/config/instructions/defaults//cuwacunu/src/config/"
      "instructions/defaults/",
      "/cuwacunu/src/config/instructions/defaults/");
  write_text_file(cpu_contract, cpu_contract_text);

  std::string cpu_campaign_text = replace_once(
      base_campaign_text, base_contract_ref, cpu_contract.string());
  cpu_campaign_text = replace_once(std::move(cpu_campaign_text), base_wave_ref,
                                   base_wave_path.string());
  write_text_file(cpu_campaign, cpu_campaign_text);
  cuwacunu::iitepi::config_space_t::set_runtime_campaign_dsl_override(
      cpu_campaign.string());

  const auto contract_hash =
      cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
          cuwacunu::iitepi::config_space_t::locked_campaign_hash(),
          cuwacunu::iitepi::config_space_t::locked_binding_id());
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(contract_hash);

  // Reproducibility
  torch::manual_seed(48);

  // -----------------------------------------------------
  // Create the Dataloader
  // -----------------------------------------------------
  torch::manual_seed(
      cuwacunu::iitepi::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* types definition */
  std::string INSTRUMENT = "BTCUSDT"; // "UTILITIES"
  using Datatype_t = cuwacunu::camahjucunu::exchange::
      kline_t; // cuwacunu::camahjucunu::exchange::basic_t;
  using Dataset_t =
      cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Datasample_t = cuwacunu::camahjucunu::data::observation_sample_t;
  using Sampler_t = torch::data::samplers::
      SequentialSampler; // using Sampler_t =
                         // torch::data::samplers::RandomSampler;

  TICK(create_dataloader_);
  auto raw_dataloader = cuwacunu::camahjucunu::data::make_obs_mm_dataloader<
      Datatype_t, Sampler_t>(INSTRUMENT, contract_hash);
  PRINT_TOCK_ms(create_dataloader_);

  // -----------------------------------------------------
  // Instantiate VICReg_4d (config-driven ctor, CPU-safe)
  // -----------------------------------------------------
  TICK(load_representation_model_);
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D representation_model(
      contract_hash, "tsi.wikimyei.representation.vicreg",
      static_cast<int>(raw_dataloader.C_), static_cast<int>(raw_dataloader.T_),
      static_cast<int>(raw_dataloader.D_));
  PRINT_TOCK_ms(load_representation_model_);

  // -----------------------------------------------------
  // Instantiate representation Dataloader
  // -----------------------------------------------------
  TICK(extend_dataloader_with_enbedings_);
  auto representation_dataloader =
      representation_model.make_representation_dataloader<
          Dataset_t, Datasample_t, Datatype_t, Sampler_t>(
          raw_dataloader, /*use_swa=*/true, /* debug */ false);
  PRINT_TOCK_ms(extend_dataloader_with_enbedings_);

  // -----------------------------------------------------
  // Instantiate MDN (from configuration)
  // -----------------------------------------------------
  TICK(create_expected_value_model_);
  cuwacunu::wikimyei::ExpectedValue expected_value_network(
      contract_hash, "tsi.wikimyei.inference.mdn");
  PRINT_TOCK_ms(create_expected_value_model_);

  // -----------------------------------------------------
  // Training
  // -----------------------------------------------------
  const int configured_telemetry_every =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<int>("EXPECTED_VALUE", "telemetry_every");
  const int configured_epochs =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<int>("EXPECTED_VALUE", "n_epochs", -1);
  const int configured_iters =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<int>("EXPECTED_VALUE", "n_iters", -1);
  const int smoke_telemetry_every = 1;
  const int smoke_epochs = 1;
  const int smoke_iters = 1;
  std::cout << "[smoke] EXPECTED_VALUE training limited to n_epochs="
            << smoke_epochs << " n_iters=" << smoke_iters << " (configured "
            << configured_epochs << "/" << configured_iters
            << ", telemetry_every=" << configured_telemetry_every << ")\n";

  expected_value_network.set_telemetry_every(smoke_telemetry_every);
  TICK(fit_expected_value_);
  expected_value_network.fit(representation_dataloader,
                             /* n_epochs */ smoke_epochs,
                             /* n_iters */ smoke_iters,
                             /* verbose */ false);
  TORCH_CHECK(
      expected_value_network.scheduler_batch_steps_ == 0,
      "[test_expected_value] PerEpoch scheduler should not step per batch.");
  TORCH_CHECK(
      expected_value_network.scheduler_epoch_steps_ == 1,
      "[test_expected_value] PerEpoch scheduler should step once per epoch.");
  PRINT_TOCK_ms(fit_expected_value_);

  const std::filesystem::path base_jk_specs =
      "/cuwacunu/src/config/instructions/defaults/"
      "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl";
  const std::string base_specs_text = read_text_file(base_jk_specs);

  // Scheduler mode semantics: PerBatch
  const std::string per_batch_component_name =
      "tsi.wikimyei.inference.mdn.perbatch_counter";
  const std::string per_batch_specs = replace_section_after(
      base_specs_text, "COMPONENT \"tsi.wikimyei.inference.mdn\"",
      "LR_SCHEDULER",
      "    [LR_SCHEDULER]\n"
      "      .type: \"OneCycleLR\"\n"
      "      .max_lr: 0.001\n"
      "      .total_steps: 1");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash, per_batch_component_name, "tsi.wikimyei.inference.mdn",
      per_batch_specs);
  {
    cuwacunu::wikimyei::ExpectedValue per_batch_ev(contract_hash,
                                                   per_batch_component_name);
    per_batch_ev.fit(representation_dataloader,
                     /*n_epochs=*/1,
                     /*n_iters=*/1,
                     /*verbose=*/false);
    TORCH_CHECK(per_batch_ev.scheduler_batch_steps_ == 1,
                "[test_expected_value] PerBatch scheduler should step once for "
                "one batch.");
    TORCH_CHECK(
        per_batch_ev.scheduler_epoch_steps_ == 0,
        "[test_expected_value] PerBatch scheduler should not step per epoch.");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash, per_batch_component_name);

  // Scheduler mode semantics: PerEpochWithMetric
  const std::string metric_component_name =
      "tsi.wikimyei.inference.mdn.metric_counter";
  const std::string metric_specs = replace_section_after(
      base_specs_text, "COMPONENT \"tsi.wikimyei.inference.mdn\"",
      "LR_SCHEDULER",
      "    [LR_SCHEDULER]\n"
      "      .type: \"ReduceLROnPlateau\"\n"
      "      .mode: \"min\"\n"
      "      .factor: 0.5\n"
      "      .patience: 1\n"
      "      .threshold: 0.0\n"
      "      .threshold_mode: \"rel\"\n"
      "      .cooldown: 0\n"
      "      .min_lr: 0.0\n"
      "      .eps: 1e-8");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash, metric_component_name, "tsi.wikimyei.inference.mdn",
      metric_specs);
  {
    cuwacunu::wikimyei::ExpectedValue metric_ev(contract_hash,
                                                metric_component_name);
    metric_ev.fit(representation_dataloader,
                  /*n_epochs=*/1,
                  /*n_iters=*/1,
                  /*verbose=*/false);
    TORCH_CHECK(metric_ev.scheduler_batch_steps_ == 0,
                "[test_expected_value] PerEpochWithMetric scheduler should not "
                "step per batch.");
    TORCH_CHECK(metric_ev.scheduler_epoch_steps_ == 1,
                "[test_expected_value] PerEpochWithMetric scheduler should "
                "step once per epoch.");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash, metric_component_name);

  // Optimizer threshold clamp semantics for Adam/AdamW step counters.
  {
    cuwacunu::wikimyei::ExpectedValue clamp_ev(contract_hash,
                                               "tsi.wikimyei.inference.mdn");
    clamp_ev.optimizer_threshold_reset = 0;
    clamp_ev.fit(representation_dataloader,
                 /*n_epochs=*/1,
                 /*n_iters=*/1,
                 /*verbose=*/false);
    bool saw_adamw_state = false;
    for (auto &kv : clamp_ev.optimizer->state()) {
      if (auto *s =
              dynamic_cast<torch::optim::AdamWParamState *>(kv.second.get())) {
        saw_adamw_state = true;
        TORCH_CHECK(s->step() <= 0, "[test_expected_value] expected AdamW step "
                                    "counter clamp at threshold 0.");
      }
    }
    TORCH_CHECK(
        saw_adamw_state,
        "[test_expected_value] expected AdamW optimizer state entries.");
  }
  // -----------------------------------------------------
  // Save
  // -----------------------------------------------------
  TICK(save_expected_value_network_);
  const std::string ckpt_path =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>("EXPECTED_VALUE", "model_path");
  TORCH_CHECK(expected_value_network.save_checkpoint(ckpt_path),
              "[test_expected_value] save_checkpoint should succeed");
  PRINT_TOCK_ms(save_expected_value_network_);

  // -----------------------------------------------------
  // Load
  // -----------------------------------------------------
  TICK(load_expected_value_network_);
  cuwacunu::wikimyei::ExpectedValue loaded_expected_value_network(
      contract_hash, "tsi.wikimyei.inference.mdn");
  TORCH_CHECK(loaded_expected_value_network.load_checkpoint(ckpt_path),
              "[test_expected_value] load_checkpoint should succeed");
  PRINT_TOCK_ms(load_expected_value_network_);

  // -----------------------------------------------------
  // Strict checkpoint v4 negative tests
  // -----------------------------------------------------
  const std::filesystem::path tmp_dir = "/tmp/test_expected_value_ckpt";
  std::filesystem::create_directories(tmp_dir);
  TORCH_CHECK(!expected_value_network.save_checkpoint(tmp_dir.string()),
              "[test_expected_value] save should fail when destination path is "
              "a directory");

  // Reject legacy/no-version checkpoint.
  const std::filesystem::path no_version_ckpt =
      tmp_dir / "expected_value_no_version.ckpt";
  {
    torch::serialize::OutputArchive legacy;
    legacy.write("best_metric", torch::tensor({0.0}));
    legacy.save_to(no_version_ckpt.string());
  }
  {
    cuwacunu::wikimyei::ExpectedValue strict_loader(
        contract_hash, "tsi.wikimyei.inference.mdn");
    TORCH_CHECK(!strict_loader.load_checkpoint(no_version_ckpt.string()),
                "[test_expected_value] load should reject checkpoint missing "
                "format_version");
  }

  // Reject checkpoint contract hash mismatch.
  const std::filesystem::path mismatch_meta_ckpt =
      tmp_dir / "expected_value_contract_mismatch.ckpt";
  {
    torch::serialize::OutputArchive mismatch_archive;
    mismatch_archive.write(
        "format_version",
        torch::tensor({int64_t(4)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    write_archive_string(mismatch_archive, "meta/contract_hash",
                         "mismatched_contract_hash");
    write_archive_string(mismatch_archive, "meta/component_name",
                         "tsi.wikimyei.inference.mdn");
    write_archive_string(mismatch_archive, "meta/scheduler_mode", "PerEpoch");
    mismatch_archive.write(
        "meta/scheduler_batch_steps",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "meta/scheduler_epoch_steps",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "meta/model_parameter_count",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "meta/model_buffer_count",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "has_optimizer",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write("best_metric", torch::tensor({0.0}));
    mismatch_archive.write(
        "best_epoch", torch::tensor({int64_t(0)}, torch::TensorOptions().dtype(
                                                      torch::kInt64)));
    mismatch_archive.write(
        "total_iters_trained",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "total_epochs_trained",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "runtime_optimizer_steps",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "runtime_trained_samples",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write(
        "runtime_skipped_batches",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.write("runtime_last_committed_loss_mean",
                           torch::tensor({0.0}));
    mismatch_archive.write(
        "scheduler_serialized",
        torch::tensor({int64_t(0)},
                      torch::TensorOptions().dtype(torch::kInt64)));
    mismatch_archive.save_to(mismatch_meta_ckpt.string());
  }
  {
    cuwacunu::wikimyei::ExpectedValue mismatch_loader(
        contract_hash, "tsi.wikimyei.inference.mdn");
    TORCH_CHECK(
        !mismatch_loader.load_checkpoint(mismatch_meta_ckpt.string()),
        "[test_expected_value] load should reject contract hash mismatch");
  }

  // Reject scheduler mode mismatch.
  const std::filesystem::path mismatch_ckpt =
      tmp_dir / "expected_value_scheduler_mismatch.ckpt";
  const std::string per_batch_specs_for_mismatch = replace_section_after(
      base_specs_text, "COMPONENT \"tsi.wikimyei.inference.mdn\"",
      "LR_SCHEDULER",
      "    [LR_SCHEDULER]\n"
      "      .type: \"OneCycleLR\"\n"
      "      .max_lr: 0.001\n"
      "      .total_steps: 1");
  cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
      contract_hash, "tsi.wikimyei.inference.mdn", "tsi.wikimyei.inference.mdn",
      per_batch_specs_for_mismatch);
  {
    cuwacunu::wikimyei::ExpectedValue per_batch_mode_model(
        contract_hash, "tsi.wikimyei.inference.mdn");
    TORCH_CHECK(per_batch_mode_model.save_checkpoint(mismatch_ckpt.string()),
                "[test_expected_value] expected save to succeed for scheduler "
                "mismatch fixture");
  }
  cuwacunu::jkimyei::jk_setup_t::registry.clear_component_instruction_override(
      contract_hash, "tsi.wikimyei.inference.mdn");
  {
    cuwacunu::wikimyei::ExpectedValue per_epoch_loader(
        contract_hash, "tsi.wikimyei.inference.mdn");
    TORCH_CHECK(
        !per_epoch_loader.load_checkpoint(mismatch_ckpt.string()),
        "[test_expected_value] load should reject scheduler mode mismatch");
  }

  // -----------------------------------------------------
  // Dashboards: fetch latest vectors (CPU tensors)
  // -----------------------------------------------------
  TICK(estimation_network_dashboards_);
  auto ch = expected_value_network.get_last_per_channel_nll(); // [C] on CPU
  auto hz = expected_value_network.get_last_per_horizon_nll(); // [Hf] on CPU
  PRINT_TOCK_ms(estimation_network_dashboards_);

  return 0;
}
