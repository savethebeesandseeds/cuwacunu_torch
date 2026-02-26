// ./include/tsiemene/tsi.wikimyei.representation.vicreg.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
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

// Your existing VICReg implementation:
#include "wikimyei/representation/VICReg/vicreg_4d.h"

namespace tsiemene {

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
                                  std::string component_name,
                                  int C, int T, int D,
                                  bool train = false,
                                  bool use_swa = true,
                                  bool detach_to_cpu = true)
      : id_(id),
        instance_name_(std::move(instance_name)),
        component_name_(std::move(component_name)),
        train_(train),
        use_swa_(use_swa),
        detach_to_cpu_(detach_to_cpu),
        model_(component_name_, C, T, D) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.wikimyei.representation.vicreg"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }
  [[nodiscard]] bool emits_loss_directive() const noexcept override { return true; }
  [[nodiscard]] bool supports_jkimyei_facet() const noexcept override { return true; }
  [[nodiscard]] bool supports_init_artifacts() const noexcept override { return true; }
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

  void set_train(bool on) noexcept { train_ = on; }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != PayloadKind::Tensor) return;

    {
      std::ostringstream oss;
      oss << "vicreg.in packed=" << tensor_shape_(in.signal.tensor)
          << " train=" << (train_ ? "true" : "false")
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

    // Training mode: you will drop your old fit-step logic here.
    // For now (so the plumbing tests), emit a placeholder loss.
    if (train_) {
      auto loss = torch::zeros({}, torch::TensorOptions().device(torch::kCPU).dtype(torch::kFloat32));
      const std::string loss_shape = tensor_shape_(loss);
      out.emit_tensor(wave, OUT_LOSS, std::move(loss));
      emit_meta_(wave, out, std::string("vicreg.out loss=") + loss_shape);
    }
  }

 private:
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
  oss << "0x" << std::hex << std::nouppercase << value;
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

  if (!seen_any) return "0x0";
  return format_wikimyei_hex_hash(max_seen + 1);
}

[[nodiscard]] inline bool write_wikimyei_representation_vicreg_artifact_payload(
    std::string canonical_action,
    wikimyei_representation_vicreg_init_record_t* out) {
  if (!out) return false;
  out->canonical_base = "tsi.wikimyei.representation.vicreg." + out->hashimyei;
  out->weights_file = out->artifact_directory / "weights.init.pt";

  std::string io_error;
  if (!ensure_wikimyei_vicreg_weights_placeholder(out->weights_file, &io_error)) {
    out->error = io_error;
    return false;
  }

  std::ostringstream metadata;
  metadata << "schema=tsi.wikimyei.representation.vicreg.init.v1\n";
  metadata << "canonical_action=" << canonical_action << "\n";
  metadata << "canonical_target=tsi.wikimyei.representation.vicreg\n";
  metadata << "family=representation\n";
  metadata << "model=vicreg\n";
  metadata << "hashimyei=" << out->hashimyei << "\n";
  metadata << "canonical_base=" << out->canonical_base << "\n";
  metadata << "weights_file=" << out->weights_file.filename().string() << "\n";

  out->metadata_encrypted = false;
  out->metadata_plaintext_fallback = false;
  out->metadata_warning.clear();

  std::string metadata_error;
  if (cuwacunu::hashimyei::write_encrypted_metadata(out->artifact_directory, metadata.str(), &metadata_error)) {
    out->metadata_encrypted = true;
  } else {
    out->metadata_warning = metadata_error;
    if (!write_wikimyei_text_file(out->artifact_directory / "metadata.txt", metadata.str(), &io_error)) {
      out->error =
          "cannot persist metadata (encrypted failed: " + metadata_error + "; plaintext failed: " + io_error + ")";
      return false;
    }
    out->metadata_plaintext_fallback = true;
  }

  out->ok = true;
  return true;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t persist_wikimyei_representation_vicreg_init() {
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
          "tsi.wikimyei.representation.vicreg.init()", &out)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t update_wikimyei_representation_vicreg_init(
    std::string hashimyei) {
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
          "tsi.wikimyei.representation.vicreg.edit()", &out)) {
    return out;
  }

  return out;
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
