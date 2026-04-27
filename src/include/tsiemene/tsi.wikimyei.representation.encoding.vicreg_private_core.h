#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/instrument_signature.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.wikimyei.representation.h"
#include "wikimyei/evaluation/embedding_sequence_analytics/embedding_sequence_analytics.h"
#include "wikimyei/evaluation/transfer_evaluation_matrix/transfer_matrix_evaluation.h"

// Report-fragment metadata helpers:
#include "hero/hashimyei_hero/hashimyei_driver.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "iitepi/runtime_binding/runtime_binding.h"
#include "piaabo/latent_lineage_state/report_taxonomy.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "piaabo/torch_compat/network_analytics.h"

// Your existing VICReg implementation:
#include "wikimyei/representation/VICReg/vicreg_rank4.h"

namespace tsiemene {

using VicregTransferMatrixEvaluator =
    cuwacunu::wikimyei::evaluation::VicregTransferMatrixEvaluator;
using VicregEmbeddingSequenceEvaluator =
    cuwacunu::wikimyei::evaluation::VicregEmbeddingSequenceEvaluator;

struct vicreg_public_docking_shape_t {
  int C{0};
  int T{0};
  int D{0};
  int encoding_dims{0};
};

[[nodiscard]] inline vicreg_public_docking_shape_t
capture_vicreg_public_docking_shape_(
    const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &model) {
  vicreg_public_docking_shape_t out{};
  out.C = model.C;
  out.T = model.T;
  out.D = model.D;
  out.encoding_dims = model.encoding_dims;
  return out;
}

struct vicreg_train_summary_snapshot_t {
  std::uint64_t epoch_index{0};
  std::uint64_t optimizer_steps{0};
  std::uint64_t trained_epochs{0};
  std::uint64_t trained_samples{0};
  std::uint64_t skipped_batches{0};
  double epoch_loss_mean{0.0};
  double epoch_inv_mean{0.0};
  double epoch_var_mean{0.0};
  double epoch_cov_raw_mean{0.0};
  double epoch_cov_weighted_mean{0.0};
  double last_committed_loss_mean{0.0};
  double learning_rate{0.0};
};

struct wikimyei_representation_encoding_vicreg_init_record_t
    : TsiWikimyeiInitRecord {
  bool enable_embedding_sequence_analytics_sidecar{true};
  bool has_embedding_sequence_analytics{false};
  cuwacunu::piaabo::torch_compat::data_analytics_options_t
      embedding_sequence_analytics_options{};
  cuwacunu::piaabo::torch_compat::sequence_analytics_report_t
      embedding_sequence_analytics_report{};
  cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t
      embedding_sequence_symbolic_analytics_report{};
  std::filesystem::path embedding_sequence_analytics_file{};
  std::filesystem::path embedding_sequence_symbolic_analytics_file{};
  bool has_train_summary{false};
  vicreg_train_summary_snapshot_t train_summary_snapshot{};
  std::filesystem::path train_summary_file{};
};

using wikimyei_representation_encoding_vicreg_init_entry_t =
    TsiWikimyeiInitEntry;

[[nodiscard]] inline bool has_non_ws_ascii_(std::string_view text) {
  for (const unsigned char c : text) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

[[nodiscard]] inline std::string
effective_runtime_report_run_id_(std::string_view run_id,
                                 std::string_view hashimyei) {
  if (!run_id.empty())
    return std::string(run_id);
  if (hashimyei.empty())
    return "run.vicreg.unknown";
  return "run.vicreg." + std::string(hashimyei);
}

[[nodiscard]] inline std::string
sanitize_runtime_history_token_(std::string_view token) {
  std::string out;
  out.reserve(token.size());
  for (const unsigned char c : token) {
    const bool ok = (std::isalnum(c) != 0) || c == '_' || c == '-' || c == '.';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  if (out.empty())
    return "run.unknown";
  return out;
}

[[nodiscard]] inline std::filesystem::path
runtime_history_file_path_(const std::filesystem::path &stable_file,
                           std::string_view history_token) {
  const std::string sanitized = sanitize_runtime_history_token_(history_token);
  const std::string stem = stable_file.stem().string();
  std::string stem_prefix = stem;
  constexpr std::string_view kLatestSuffix = ".latest";
  if (stem_prefix.size() > kLatestSuffix.size() &&
      stem_prefix.compare(stem_prefix.size() - kLatestSuffix.size(),
                          kLatestSuffix.size(), kLatestSuffix) == 0) {
    stem_prefix.erase(stem_prefix.size() - kLatestSuffix.size());
  }
  return stable_file.parent_path() /
         (stem_prefix + "." + sanitized + stable_file.extension().string());
}

inline void append_runtime_string_entry_(runtime_lls_document_t *document,
                                         std::string_view key,
                                         std::string_view value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

inline void
append_runtime_string_entry_if_nonempty_(runtime_lls_document_t *document,
                                         std::string_view key,
                                         std::string_view value) {
  if (!document || value.empty())
    return;
  append_runtime_string_entry_(document, key, value);
}

inline void append_runtime_instrument_signature_entries_(
    runtime_lls_document_t *document, std::string_view prefix,
    const cuwacunu::camahjucunu::instrument_signature_t &signature) {
  if (!document)
    return;
  for (const auto &[field, value] :
       cuwacunu::camahjucunu::instrument_signature_fields(signature)) {
    append_runtime_string_entry_if_nonempty_(
        document, std::string(prefix) + "." + field, value);
  }
}

inline void append_runtime_component_evidence_entries_(
    runtime_lls_document_t *document, const TsiWikimyeiInitRecord *record) {
  if (!document || !record)
    return;
  append_runtime_string_entry_if_nonempty_(document, "component_tag",
                                           record->component_tag);
  append_runtime_string_entry_if_nonempty_(
      document, "component_compatibility_sha256_hex",
      record->component_compatibility_sha256_hex);
  append_runtime_string_entry_if_nonempty_(
      document, "docking_signature_sha256_hex",
      record->docking_signature_sha256_hex);
  append_runtime_instrument_signature_entries_(document, "instrument_signature",
                                               record->instrument_signature);
  append_runtime_instrument_signature_entries_(
      document, "runtime_instrument_signature",
      record->runtime_instrument_signature);
}

inline void append_runtime_int_entry_(runtime_lls_document_t *document,
                                      std::string_view key,
                                      std::int64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value));
}

inline void append_runtime_u64_entry_(runtime_lls_document_t *document,
                                      std::string_view key,
                                      std::uint64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, "(0,+inf)"));
}

inline void append_runtime_bool_entry_(runtime_lls_document_t *document,
                                       std::string_view key, bool value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          std::string(key), value));
}

inline void append_runtime_double_entry_(runtime_lls_document_t *document,
                                         std::string_view key, double value) {
  if (!document || !std::isfinite(value))
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value));
}

struct vicreg_network_analytics_plan_t {
  cuwacunu::piaabo::torch_compat::network_analytics_options_t options{};
  std::string network_label{};
};

[[nodiscard]] inline bool resolve_vicreg_network_analytics_plan_(
    const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &model,
    vicreg_network_analytics_plan_t *out, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "internal: null network analytics plan output";
    return false;
  }

  if (!has_non_ws_ascii_(model.network_design_grammar) ||
      !has_non_ws_ascii_(model.network_design_dsl)) {
    if (error) {
      *error = "missing network_design payload (expected explicit "
               "NETWORK_ANALYTICS_POLICY node)";
    }
    return false;
  }

  try {
    const auto design_ir =
        cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
            model.network_design_grammar, model.network_design_dsl);
    std::string options_error;
    if (!cuwacunu::piaabo::torch_compat::
            resolve_network_analytics_options_from_network_design(
                design_ir, &out->options, &options_error)) {
      if (error) {
        *error = options_error.empty()
                     ? "unable to resolve network analytics options"
                     : options_error;
      }
      return false;
    }
    out->network_label = model.network_design_network_id;
    if (!has_non_ws_ascii_(out->network_label) &&
        has_non_ws_ascii_(design_ir.network_id)) {
      out->network_label = design_ir.network_id;
    }
    if (!has_non_ws_ascii_(out->network_label)) {
      out->network_label = model.component_name;
    }
    return true;
  } catch (const std::exception &e) {
    if (error) {
      *error = std::string("failed to decode network_design payload (") +
               e.what() + ")";
    }
    return false;
  }
}

[[nodiscard]] inline bool write_vicreg_network_analytics_sidecar_(
    const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &model,
    const std::filesystem::path &weights_file, std::string_view canonical_path,
    std::string_view family_canonical_path, std::string_view report_fragment_id,
    std::string_view contract_hash, std::string_view binding_id,
    std::string_view run_id, std::string_view source_runtime_cursor,
    bool has_wave_cursor, std::uint64_t wave_cursor,
    std::filesystem::path *out_sidecar_file, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out_sidecar_file) {
    if (error)
      *error = "internal: null network analytics sidecar output path";
    return false;
  }
  (void)canonical_path;
  (void)contract_hash;
  (void)run_id;
  vicreg_network_analytics_plan_t plan{};
  std::string plan_error;
  if (!resolve_vicreg_network_analytics_plan_(model, &plan, &plan_error)) {
    if (error)
      *error = plan_error;
    return false;
  }
  auto network_report_identity =
      make_component_report_identity(std::string(family_canonical_path) + "." +
                                         std::string(report_fragment_id),
                                     binding_id,
                                     cuwacunu::piaabo::latent_lineage_state::
                                         report_taxonomy::kEmbeddingNetwork);
  network_report_identity.source_runtime_cursor =
      std::string(source_runtime_cursor);
  network_report_identity.has_wave_cursor = has_wave_cursor;
  network_report_identity.wave_cursor = wave_cursor;
  return cuwacunu::piaabo::torch_compat::
      write_network_analytics_sidecar_for_checkpoint(
          model, weights_file, out_sidecar_file, plan.options, error,
          network_report_identity);
}

[[nodiscard]] inline bool
load_wikimyei_representation_encoding_vicreg_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name, torch::Device preferred_device,
    vicreg_public_docking_shape_t expected_public_docking,
    std::unique_ptr<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4> *model_slot,
    std::string *error = nullptr);

[[nodiscard]] inline wikimyei_representation_encoding_vicreg_init_record_t
update_wikimyei_representation_encoding_vicreg_init(
    std::string hashimyei,
    cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *model = nullptr,
    bool enable_network_analytics_sidecar = true,
    bool enable_embedding_sequence_analytics_sidecar = true,
    std::string contract_hash = {}, std::string binding_id = {},
    std::string run_id = {}, std::string source_runtime_cursor = {},
    bool has_wave_cursor = false, std::uint64_t wave_cursor = 0,
    const cuwacunu::piaabo::torch_compat::sequence_analytics_report_t
        *embedding_sequence_analytics_report = nullptr,
    const cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t
        *embedding_sequence_symbolic_analytics_report = nullptr,
    const vicreg_train_summary_snapshot_t *train_summary_snapshot = nullptr,
    cuwacunu::piaabo::torch_compat::data_analytics_options_t
        embedding_sequence_analytics_options = {},
    const TsiWikimyeiRuntimeIoContext *runtime_io_context = nullptr);
