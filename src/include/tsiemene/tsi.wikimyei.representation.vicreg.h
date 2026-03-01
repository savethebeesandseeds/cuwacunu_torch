// ./include/tsiemene/tsi.wikimyei.representation.vicreg.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <torch/torch.h>

#include "tsiemene/tsi.wikimyei.representation.h"

// Artifact metadata helpers:
#include "hashimyei/hashimyei_artifacts.h"
#include "hashimyei/hashimyei_driver.h"

// Your existing VICReg implementation:
#include "wikimyei/representation/VICReg/vicreg_4d.h"

namespace tsiemene {

[[nodiscard]] inline bool load_wikimyei_representation_vicreg_init_into_model(
    std::string_view hashimyei,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model,
    std::string* error = nullptr);

[[nodiscard]] inline TsiWikimyeiInitRecord
update_wikimyei_representation_vicreg_init(
    std::string hashimyei,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model = nullptr);

// packed: [B,C,T,D+1] where last slot is mask (0/1)
inline void unpack_vicreg_packed_batch(const torch::Tensor& packed,
                                       int D,
                                       torch::Tensor& data_out,
                                       torch::Tensor& mask_out) {
  TORCH_CHECK(packed.defined(), "[tsi.vicreg] packed batch undefined");
  TORCH_CHECK(packed.dim() == 4, "[tsi.vicreg] packed must be [B,C,T,D+1]");
  TORCH_CHECK(packed.size(3) == D + 1, "[tsi.vicreg] packed.size(3) must be D+1");

  data_out = packed.narrow(/*dim=*/3, /*start=*/0, /*length=*/D);
  mask_out = (packed.select(/*dim=*/3, /*index=*/D) > 0.5); // bool [B,C,T]
}

class TsiWikimyeiRepresentationVicreg final : public TsiWikimyeiRepresentation {
 public:
  static constexpr DirectiveId IN_STEP     = directive_id::Step;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_LOSS    = directive_id::Loss;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

  TsiWikimyeiRepresentationVicreg(TsiId id,
                                  std::string instance_name,
                                  std::string contract_hash,
                                  std::string component_name,
                                  int C, int T, int D,
                                  bool train = false,
                                  bool use_swa = true,
                                  bool detach_to_cpu = true)
      : id_(id),
        instance_name_(std::move(instance_name)),
        contract_hash_(std::move(contract_hash)),
        component_name_(std::move(component_name)),
        model_(contract_hash_, component_name_, C, T, D) {
    apply_runtime_policy_from_jkimyei_(train, use_swa, detach_to_cpu);
  }

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.wikimyei.representation.vicreg"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }
  [[nodiscard]] bool emits_loss_directive() const noexcept override { return true; }
  [[nodiscard]] bool supports_jkimyei_facet() const noexcept override { return true; }
  [[nodiscard]] bool supports_init_artifacts() const noexcept override { return true; }
  [[nodiscard]] bool runtime_autosave_artifacts() const noexcept override { return train_; }
  [[nodiscard]] bool runtime_load_from_hashimyei(
      std::string_view hashimyei,
      std::string* error = nullptr) override {
    return load_wikimyei_representation_vicreg_init_into_model(
        hashimyei, &model_, error);
  }
  [[nodiscard]] bool runtime_save_to_hashimyei(
      std::string_view hashimyei,
      std::string* error = nullptr) override {
    auto out = update_wikimyei_representation_vicreg_init(
        std::string(hashimyei), &model_);
    if (out.ok) return true;
    if (error) *error = out.error;
    return false;
  }
  [[nodiscard]] bool train_enabled() const noexcept { return train_; }
  [[nodiscard]] int optimizer_steps() const noexcept {
    return model_.runtime_optimizer_steps();
  }
  [[nodiscard]] std::string_view init_artifact_schema() const noexcept override {
    return "tsi.wikimyei.representation.vicreg.init.v1";
  }
  [[nodiscard]] std::string_view artifact_family() const noexcept override { return "representation"; }
  [[nodiscard]] std::string_view artifact_model() const noexcept override { return "vicreg"; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_STEP, DirectiveDir::In, KindSpec::Tensor(), "packed [B,C,T,D+1] (last=D is mask)"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Tensor(), "representation encoding"),
      directive(OUT_LOSS, DirectiveDir::Out, KindSpec::Tensor(), "loss scalar (only when train=true)"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 4);
  }

  void set_train(bool on) noexcept {
    train_ = on;
  }

  void step(const Wave& wave, Ingress in, BoardContext&, Emitter& out) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != PayloadKind::Tensor) return;

    {
      std::ostringstream oss;
      oss << "vicreg.in packed=" << tensor_shape_(in.signal.tensor)
          << " train=" << (train_ ? "true" : "false")
          << " use_swa=" << (use_swa_ ? "true" : "false")
          << " detach_to_cpu=" << (detach_to_cpu_ ? "true" : "false");
      emit_meta_(wave, out, oss.str());
    }

    torch::Tensor data, mask;
    unpack_vicreg_packed_batch(in.signal.tensor, model_.D, data, mask);

    data = data.to(model_.device);
    mask = mask.to(model_.device);

    // Always emit representation.
    auto repr = model_.encode(data, mask,
                              /*use_swa=*/use_swa_,
                              /*detach_to_cpu=*/detach_to_cpu_);
    if (detach_to_cpu_) repr = repr.cpu();
    const std::string repr_shape = tensor_shape_(repr);
    out.emit_tensor(wave, OUT_PAYLOAD, std::move(repr));
    emit_meta_(wave, out, std::string("vicreg.out payload=") + repr_shape);

    if (train_) {
      auto step_result = model_.train_one_batch(
          data, mask, model_.jk_swa_start_iter, /*verbose=*/false);
      if (step_result.loss.defined()) {
        TORCH_CHECK(
            torch::isfinite(step_result.loss).all().item<bool>(),
            "[tsi.vicreg] training loss is non-finite");
        auto loss = step_result.loss.detach();
        if (detach_to_cpu_) loss = loss.to(torch::kCPU);
        const std::string loss_shape = tensor_shape_(loss);
        out.emit_tensor(wave, OUT_LOSS, std::move(loss));
        emit_meta_(wave,
                   out,
                   std::string("vicreg.out loss=") + loss_shape +
                       " optimizer_step=" +
                       (step_result.optimizer_step_applied ? "true" : "false"));
      } else {
        emit_meta_(wave, out, "vicreg.out loss=skipped");
      }
    }
  }

  void on_epoch_end(BoardContext&) override {
    (void)model_.finalize_pending_training_step(model_.jk_swa_start_iter);
  }

  void reset(BoardContext&) override {
    // Keep training counters/state across epochs and only commit any
    // leftover accumulation tail before the next epoch starts.
    (void)model_.finalize_pending_training_step(model_.jk_swa_start_iter);
  }

 private:
  void apply_runtime_policy_from_jkimyei_(bool requested_train,
                                          bool requested_use_swa,
                                          bool requested_detach_to_cpu) {
    const bool jk_train = model_.jk_vicreg_train;
    const bool jk_use_swa = model_.jk_vicreg_use_swa;
    const bool jk_detach_to_cpu = model_.jk_vicreg_detach_to_cpu;

    if (requested_use_swa != jk_use_swa ||
        requested_detach_to_cpu != jk_detach_to_cpu) {
      log_warn(
          "[tsi.vicreg] runtime flags (%s/%s) differ from jkimyei policy (%s/%s) for component '%s'; using jkimyei policy for non-train flags\n",
          requested_use_swa ? "swa" : "base",
          requested_detach_to_cpu ? "detach" : "keep_device",
          jk_use_swa ? "swa" : "base",
          jk_detach_to_cpu ? "detach" : "keep_device",
          component_name_.c_str());
    }
    if (requested_train != jk_train) {
      log_warn(
          "[tsi.vicreg] wave TRAIN=%s overrides jkimyei vicreg_train=%s for component '%s'\n",
          requested_train ? "train" : "eval",
          jk_train ? "train" : "eval",
          component_name_.c_str());
    }

    train_ = requested_train;
    use_swa_ = jk_use_swa;
    detach_to_cpu_ = jk_detach_to_cpu;
  }

  static std::string tensor_shape_(const torch::Tensor& t) {
    if (!t.defined()) return ":tensor undefined";
    std::ostringstream oss;
    oss << ":tensor shape=[";
    const auto sizes = t.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0) oss << ",";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  void emit_meta_(const Wave& wave, Emitter& out, std::string msg) const {
    out.emit_string(wave, OUT_META, std::move(msg));
  }

  TsiId id_{};
  std::string instance_name_;
  std::string contract_hash_;
  std::string component_name_;

  bool train_{false};
  bool use_swa_{true};
  bool detach_to_cpu_{true};

  cuwacunu::wikimyei::vicreg_4d::VICReg_4D model_;
};

using wikimyei_representation_vicreg_init_record_t = TsiWikimyeiInitRecord;
using wikimyei_representation_vicreg_init_entry_t = TsiWikimyeiInitEntry;

[[nodiscard]] inline bool parse_wikimyei_hex_hash(std::string_view s, std::uint64_t* out) {
  if (!out) return false;
  if (s.size() < 3) return false;
  if (s[0] != '0') return false;
  if (s[1] != 'x' && s[1] != 'X') return false;

  std::uint64_t value = 0;
  for (std::size_t i = 2; i < s.size(); ++i) {
    const char c = s[i];
    std::uint64_t nibble = 0;
    if (c >= '0' && c <= '9') nibble = static_cast<std::uint64_t>(c - '0');
    else if (c >= 'a' && c <= 'f') nibble = static_cast<std::uint64_t>(10 + (c - 'a'));
    else if (c >= 'A' && c <= 'F') nibble = static_cast<std::uint64_t>(10 + (c - 'A'));
    else return false;

    if (value > (std::numeric_limits<std::uint64_t>::max() >> 4)) return false;
    value = (value << 4) | nibble;
  }

  *out = value;
  return true;
}

[[nodiscard]] inline std::string format_wikimyei_hex_hash(std::uint64_t value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::nouppercase << std::setfill('0')
      << std::setw(4) << value;
  return oss.str();
}

[[nodiscard]] inline std::filesystem::path wikimyei_representation_vicreg_store_root() {
  return cuwacunu::hashimyei::store_root() / "tsi.wikimyei" / "representation" / "vicreg";
}

[[nodiscard]] inline bool is_valid_wikimyei_representation_vicreg_hash(std::string_view hashimyei) {
  std::uint64_t parsed = 0;
  return parse_wikimyei_hex_hash(hashimyei, &parsed);
}

[[nodiscard]] inline std::vector<cuwacunu::hashimyei::artifact_identity_t>
list_wikimyei_representation_vicreg_artifacts() {
  return cuwacunu::hashimyei::discover_created_artifacts_for("representation", "vicreg");
}

[[nodiscard]] inline std::vector<wikimyei_representation_vicreg_init_entry_t>
list_wikimyei_representation_vicreg_init_entries() {
  const auto artifacts = list_wikimyei_representation_vicreg_artifacts();
  std::vector<wikimyei_representation_vicreg_init_entry_t> out;
  out.reserve(artifacts.size());
  for (const auto& item : artifacts) {
    wikimyei_representation_vicreg_init_entry_t e{};
    e.hashimyei = item.hashimyei;
    e.canonical_base = item.canonical_base;
    e.artifact_directory = item.directory;
    e.weights_count = item.weight_files.size();
    out.push_back(std::move(e));
  }
  return out;
}

[[nodiscard]] inline bool write_wikimyei_text_file(const std::filesystem::path& path,
                                                   const std::string& text,
                                                   std::string* error) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open file for write: " + path.string();
    return false;
  }
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!out) {
    if (error) *error = "cannot write file contents: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline bool ensure_wikimyei_vicreg_weights_placeholder(
    const std::filesystem::path& weights_file,
    std::string* error) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (fs::exists(weights_file, ec) && fs::is_regular_file(weights_file, ec)) return true;
  const std::string payload =
      "schema=tsi.wikimyei.representation.vicreg.weights.init.v1\n"
      "state=placeholder\n";
  return write_wikimyei_text_file(weights_file, payload, error);
}

[[nodiscard]] inline std::string next_wikimyei_representation_vicreg_hash(
    const std::filesystem::path& artifacts_root) {
  namespace fs = std::filesystem;
  std::error_code ec;
  std::uint64_t max_seen = 0;
  bool seen_any = false;

  if (fs::exists(artifacts_root, ec) && fs::is_directory(artifacts_root, ec)) {
    for (const auto& entry : fs::directory_iterator(artifacts_root, ec)) {
      if (ec) break;
      if (!entry.is_directory()) continue;

      const std::string hash = entry.path().filename().string();
      std::uint64_t parsed = 0;
      if (!parse_wikimyei_hex_hash(hash, &parsed)) continue;
      if (!seen_any || parsed > max_seen) max_seen = parsed;
      seen_any = true;
    }
  }

  if (!seen_any) return "0x0000";
  return format_wikimyei_hex_hash(max_seen + 1);
}

inline constexpr std::string_view kWikimyeiVicregCanonicalType = "tsi.wikimyei.representation.vicreg";
inline constexpr std::string_view kWikimyeiVicregFamily = "representation";
inline constexpr std::string_view kWikimyeiVicregModel = "vicreg";

[[nodiscard]] inline bool save_wikimyei_representation_vicreg_artifact_with_driver(
    const cuwacunu::hashimyei::artifact_action_context_t& action,
    std::string* error) {
  namespace fs = std::filesystem;
  if (error) error->clear();

  if (!is_valid_wikimyei_representation_vicreg_hash(action.artifact_id)) {
    if (error) *error = "invalid wikimyei hashimyei id: " + action.artifact_id;
    return false;
  }

  std::error_code ec;
  fs::create_directories(action.artifact_directory, ec);
  if (ec) {
    if (error) *error = "cannot create wikimyei artifact directory: " + action.artifact_directory.string();
    return false;
  }

  const fs::path weights_file = action.artifact_directory / "weights.init.pt";
  if (action.object_handle) {
    // Contract: object_handle points to cuwacunu::wikimyei::vicreg_4d::VICReg_4D.
    auto* model = static_cast<cuwacunu::wikimyei::vicreg_4d::VICReg_4D*>(action.object_handle);
    try {
      model->save(weights_file.string());
    } catch (const std::exception& e) {
      if (error) *error = "vicreg save failed: " + std::string(e.what());
      return false;
    }
  } else {
    std::string io_error;
    if (!ensure_wikimyei_vicreg_weights_placeholder(weights_file, &io_error)) {
      if (error) *error = io_error;
      return false;
    }
  }

  const std::string canonical_base = std::string(kWikimyeiVicregCanonicalType) + "." + action.artifact_id;
  const std::string canonical_action = action.canonical_action.empty()
      ? "tsi.wikimyei.representation.vicreg.init()"
      : action.canonical_action;

  std::ostringstream metadata;
  metadata << "schema=tsi.wikimyei.representation.vicreg.init.v1\n";
  metadata << "canonical_action=" << canonical_action << "\n";
  metadata << "canonical_target=tsi.wikimyei.representation.vicreg\n";
  metadata << "family=" << kWikimyeiVicregFamily << "\n";
  metadata << "model=" << kWikimyeiVicregModel << "\n";
  metadata << "hashimyei=" << action.artifact_id << "\n";
  metadata << "canonical_base=" << canonical_base << "\n";
  metadata << "weights_file=" << weights_file.filename().string() << "\n";

  bool metadata_encrypted = false;
  bool metadata_plaintext_fallback = false;
  std::string metadata_warning;

  std::string metadata_error;
  if (cuwacunu::hashimyei::write_encrypted_metadata(action.artifact_directory, metadata.str(), &metadata_error)) {
    metadata_encrypted = true;
  } else {
    metadata_warning = metadata_error;
    std::string io_error;
    if (!write_wikimyei_text_file(action.artifact_directory / "metadata.txt", metadata.str(), &io_error)) {
      if (error) {
        *error =
            "cannot persist metadata (encrypted failed: " + metadata_error + "; plaintext failed: " + io_error + ")";
      }
      return false;
    }
    metadata_plaintext_fallback = true;
  }

  cuwacunu::hashimyei::artifact_manifest_t manifest{};
  manifest.canonical_type = std::string(kWikimyeiVicregCanonicalType);
  manifest.family = std::string(kWikimyeiVicregFamily);
  manifest.model = std::string(kWikimyeiVicregModel);
  manifest.artifact_id = action.artifact_id;
  {
    std::error_code size_ec;
    const auto weights_size = fs::file_size(weights_file, size_ec);
    manifest.files.push_back(cuwacunu::hashimyei::artifact_manifest_file_t{
        .path = weights_file.filename().string(),
        .size = size_ec ? 0 : weights_size,
    });
  }
  const auto append_manifest_file_if_present =
      [&](const fs::path& path) {
        std::error_code file_ec;
        if (!fs::exists(path, file_ec) || !fs::is_regular_file(path, file_ec)) return;
        const auto size = fs::file_size(path, file_ec);
        manifest.files.push_back(cuwacunu::hashimyei::artifact_manifest_file_t{
            .path = path.filename().string(),
            .size = file_ec ? 0 : size,
        });
      };
  append_manifest_file_if_present(action.artifact_directory / "metadata.enc");
  append_manifest_file_if_present(action.artifact_directory / "metadata.txt");

  std::string manifest_error;
  if (!cuwacunu::hashimyei::write_artifact_manifest(action.artifact_directory, manifest, &manifest_error)) {
    if (error) *error = "cannot persist artifact manifest: " + manifest_error;
    return false;
  }

  auto* out = static_cast<wikimyei_representation_vicreg_init_record_t*>(action.user_data);
  if (out) {
    out->canonical_base = canonical_base;
    out->weights_file = weights_file;
    out->metadata_encrypted = metadata_encrypted;
    out->metadata_plaintext_fallback = metadata_plaintext_fallback;
    out->metadata_warning = metadata_warning;
  }
  return true;
}

[[nodiscard]] inline bool load_wikimyei_representation_vicreg_artifact_with_driver(
    const cuwacunu::hashimyei::artifact_action_context_t& action,
    std::string* error) {
  namespace fs = std::filesystem;
  if (error) error->clear();

  if (!is_valid_wikimyei_representation_vicreg_hash(action.artifact_id)) {
    if (error) *error = "invalid wikimyei hashimyei id: " + action.artifact_id;
    return false;
  }

  const fs::path weights_file = action.artifact_directory / "weights.init.pt";
  std::error_code ec;
  if (!fs::exists(weights_file, ec) || !fs::is_regular_file(weights_file, ec)) {
    if (error) *error = "vicreg artifact weights file not found: " + weights_file.string();
    return false;
  }

  if (cuwacunu::hashimyei::artifact_manifest_exists(action.artifact_directory)) {
    cuwacunu::hashimyei::artifact_manifest_t manifest{};
    std::string manifest_error;
    if (!cuwacunu::hashimyei::read_artifact_manifest(action.artifact_directory, &manifest, &manifest_error)) {
      if (error) *error = "cannot read artifact manifest: " + manifest_error;
      return false;
    }
    if (manifest.canonical_type != std::string(kWikimyeiVicregCanonicalType)) {
      if (error) *error = "artifact manifest canonical_type mismatch: " + manifest.canonical_type;
      return false;
    }
    if (!action.family.empty() && manifest.family != action.family) {
      if (error) *error = "artifact manifest family mismatch: " + manifest.family;
      return false;
    }
    if (!action.model.empty() && manifest.model != action.model) {
      if (error) *error = "artifact manifest model mismatch: " + manifest.model;
      return false;
    }
    if (manifest.artifact_id != action.artifact_id) {
      if (error) *error = "artifact manifest hashimyei mismatch: " + manifest.artifact_id;
      return false;
    }
    if (!cuwacunu::hashimyei::artifact_manifest_has_file(manifest, weights_file.filename().string())) {
      if (error) *error = "artifact manifest missing weights file entry: " + weights_file.filename().string();
      return false;
    }
  }

  if (action.object_handle) {
    // Contract: object_handle points to cuwacunu::wikimyei::vicreg_4d::VICReg_4D.
    auto* model = static_cast<cuwacunu::wikimyei::vicreg_4d::VICReg_4D*>(action.object_handle);
    try {
      model->load(weights_file.string());
    } catch (const std::exception& e) {
      if (error) *error = "vicreg load failed: " + std::string(e.what());
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool ensure_wikimyei_representation_vicreg_driver_registered(
    std::string* error = nullptr) {
  if (error) error->clear();
  if (cuwacunu::hashimyei::has_artifact_driver(kWikimyeiVicregCanonicalType)) return true;

  cuwacunu::hashimyei::artifact_driver_t driver{};
  driver.canonical_type = std::string(kWikimyeiVicregCanonicalType);
  driver.family = std::string(kWikimyeiVicregFamily);
  driver.model = std::string(kWikimyeiVicregModel);
  driver.save = save_wikimyei_representation_vicreg_artifact_with_driver;
  driver.load = load_wikimyei_representation_vicreg_artifact_with_driver;

  if (cuwacunu::hashimyei::register_artifact_driver(std::move(driver), error)) return true;
  // Registration may race in multi-entry contexts; treat "already registered" as success.
  return cuwacunu::hashimyei::has_artifact_driver(kWikimyeiVicregCanonicalType);
}

[[nodiscard]] inline bool write_wikimyei_representation_vicreg_artifact_payload(
    std::string canonical_action,
    wikimyei_representation_vicreg_init_record_t* out,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model = nullptr) {
  if (!out) return false;

  out->metadata_encrypted = false;
  out->metadata_plaintext_fallback = false;
  out->metadata_warning.clear();
  out->canonical_base.clear();
  out->weights_file.clear();

  std::string registration_error;
  if (!ensure_wikimyei_representation_vicreg_driver_registered(&registration_error)) {
    out->error = "failed to register vicreg artifact driver: " + registration_error;
    return false;
  }

  cuwacunu::hashimyei::artifact_action_context_t action{};
  action.canonical_type = std::string(kWikimyeiVicregCanonicalType);
  action.family = std::string(kWikimyeiVicregFamily);
  action.model = std::string(kWikimyeiVicregModel);
  action.artifact_id = out->hashimyei;
  action.artifact_directory = out->artifact_directory;
  action.canonical_action = std::move(canonical_action);
  action.object_handle = model;
  action.user_data = out;

  std::string dispatch_error;
  if (!cuwacunu::hashimyei::dispatch_artifact_save(action.canonical_type, action, &dispatch_error)) {
    out->error = dispatch_error;
    return false;
  }

  out->ok = true;
  return true;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t persist_wikimyei_representation_vicreg_init(
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model = nullptr) {
  namespace fs = std::filesystem;
  wikimyei_representation_vicreg_init_record_t out{};

  out.store_root = wikimyei_representation_vicreg_store_root();

  std::error_code ec;
  fs::create_directories(out.store_root, ec);
  if (ec) {
    out.error = "cannot create wikimyei artifact root: " + out.store_root.string();
    return out;
  }

  out.hashimyei = next_wikimyei_representation_vicreg_hash(out.store_root);
  out.artifact_directory = out.store_root / out.hashimyei;

  fs::create_directories(out.artifact_directory, ec);
  if (ec) {
    out.error = "cannot create wikimyei artifact directory: " + out.artifact_directory.string();
    return out;
  }

  if (!write_wikimyei_representation_vicreg_artifact_payload(
          "tsi.wikimyei.representation.vicreg.init()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t update_wikimyei_representation_vicreg_init(
    std::string hashimyei,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model) {
  namespace fs = std::filesystem;
  wikimyei_representation_vicreg_init_record_t out{};
  out.store_root = wikimyei_representation_vicreg_store_root();

  std::uint64_t parsed = 0;
  if (!parse_wikimyei_hex_hash(hashimyei, &parsed)) {
    out.error = "invalid wikimyei hashimyei id: " + hashimyei;
    return out;
  }

  out.hashimyei = std::move(hashimyei);
  out.artifact_directory = out.store_root / out.hashimyei;

  std::error_code ec;
  if (!fs::exists(out.artifact_directory, ec) || !fs::is_directory(out.artifact_directory, ec)) {
    out.error = "wikimyei artifact not found: " + out.artifact_directory.string();
    return out;
  }

  if (!write_wikimyei_representation_vicreg_artifact_payload(
          "tsi.wikimyei.representation.vicreg.edit()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline bool load_wikimyei_representation_vicreg_init_into_model(
    std::string_view hashimyei,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D* model,
    std::string* error) {
  namespace fs = std::filesystem;
  if (error) error->clear();
  if (!model) {
    if (error) *error = "vicreg model handle is null";
    return false;
  }

  std::uint64_t parsed = 0;
  if (!parse_wikimyei_hex_hash(hashimyei, &parsed)) {
    if (error) *error = "invalid wikimyei hashimyei id: " + std::string(hashimyei);
    return false;
  }

  const fs::path artifact_directory = wikimyei_representation_vicreg_store_root() / std::string(hashimyei);
  std::error_code ec;
  if (!fs::exists(artifact_directory, ec) || !fs::is_directory(artifact_directory, ec)) {
    if (error) *error = "wikimyei artifact not found: " + artifact_directory.string();
    return false;
  }

  std::string registration_error;
  if (!ensure_wikimyei_representation_vicreg_driver_registered(&registration_error)) {
    if (error) *error = "failed to register vicreg artifact driver: " + registration_error;
    return false;
  }

  cuwacunu::hashimyei::artifact_action_context_t action{};
  action.canonical_type = std::string(kWikimyeiVicregCanonicalType);
  action.family = std::string(kWikimyeiVicregFamily);
  action.model = std::string(kWikimyeiVicregModel);
  action.artifact_id = std::string(hashimyei);
  action.artifact_directory = artifact_directory;
  action.canonical_action = "tsi.wikimyei.representation.vicreg.load()";
  action.object_handle = model;

  std::string dispatch_error;
  if (!cuwacunu::hashimyei::dispatch_artifact_load(action.canonical_type, action, &dispatch_error)) {
    if (error) *error = dispatch_error;
    return false;
  }
  return true;
}

[[nodiscard]] inline bool delete_wikimyei_representation_vicreg_init(
    std::string_view hashimyei,
    std::uintmax_t* removed_count = nullptr,
    std::string* error = nullptr) {
  namespace fs = std::filesystem;
  if (removed_count) *removed_count = 0;
  if (error) error->clear();

  std::uint64_t parsed = 0;
  if (!parse_wikimyei_hex_hash(hashimyei, &parsed)) {
    if (error) *error = "invalid wikimyei hashimyei id";
    return false;
  }

  const fs::path target = wikimyei_representation_vicreg_store_root() / std::string(hashimyei);
  std::error_code ec;
  if (!fs::exists(target, ec) || !fs::is_directory(target, ec)) {
    if (error) *error = "wikimyei artifact not found: " + target.string();
    return false;
  }

  const std::uintmax_t removed = fs::remove_all(target, ec);
  if (ec) {
    if (error) *error = "failed to delete wikimyei artifact: " + target.string();
    return false;
  }

  if (removed_count) *removed_count = removed;
  return true;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t
invoke_wikimyei_representation_vicreg_init_from_config() {
  return persist_wikimyei_representation_vicreg_init();
}

} // namespace tsiemene
