#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/evp.h>
#include <unistd.h>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "hero/lattice_hero/source_runtime_projection_runtime.h"
#include "hero/runtime_dev_loop.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iitepi/runtime_binding/runtime_binding.contract.init.h"
#include "iitepi/runtime_binding/runtime_binding.builder.h"
#include "iitepi/iitepi.h"
#include "source/dataloader/dataloader_component.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

const char* value_or_empty(const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

std::string trim_ascii_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::string sanitize_path_token_copy(std::string_view token) {
  std::string out;
  out.reserve(token.size());
  for (const unsigned char c : token) {
    const bool ok =
        (std::isalnum(c) != 0) || c == '_' || c == '-' || c == '.';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  if (out.empty()) return "default";
  return out;
}

std::string compact_runtime_dependency_path(std::string_view raw_path) {
  const std::string trimmed = trim_ascii_copy(raw_path);
  if (trimmed.empty()) return {};
  const std::string lowered = [&]() {
    std::string out = trimmed;
    for (char& ch : out) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
  }();
  const std::filesystem::path path(trimmed);
  const std::string filename = path.filename().string();
  if (filename.empty()) return path.lexically_normal().string();
  if (lowered.find("/.campaigns/") != std::string::npos) {
    return filename;
  }
  if (lowered.find("/tmp/iitepi.internal.") != std::string::npos &&
      filename.find(".runtime_binding.dsl") != std::string::npos) {
    return "iitepi.runtime_binding.dsl";
  }
  return path.lexically_normal().string();
}

std::string json_escape_copy(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (const unsigned char ch : text) {
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (ch < 0x20) {
          std::ostringstream hex;
          hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<unsigned>(ch);
          out += hex.str();
        } else {
          out.push_back(static_cast<char>(ch));
        }
        break;
    }
  }
  return out;
}

bool append_job_trace_line(const std::filesystem::path& trace_path,
                           std::string_view line) {
  if (trace_path.empty()) return true;
  std::string error{};
  if (cuwacunu::hero::runtime::append_text_file(trace_path, line, &error)) {
    return true;
  }
  log_warn("[cuwacunu_campaign] job trace append failed path=%s error=%s\n",
           trace_path.string().c_str(),
           value_or_empty(error));
  return false;
}

void refresh_runtime_job_device_metadata(
    const std::filesystem::path& campaigns_root,
    std::string_view job_cursor,
    const tsiemene::runtime_binding_device_resolution_t& runtime_device) {
  if (campaigns_root.empty()) return;
  const std::string trimmed_job_cursor = trim_ascii_copy(job_cursor);
  if (trimmed_job_cursor.empty()) return;

  std::string error{};
  cuwacunu::hero::runtime::runtime_job_record_t job{};
  if (!cuwacunu::hero::runtime::read_runtime_job_record(
          campaigns_root, trimmed_job_cursor, &job, &error)) {
    log_warn(
        "[cuwacunu_campaign] cannot refresh runtime job device metadata cursor=%s error=%s\n",
        value_or_empty(trimmed_job_cursor),
        value_or_empty(error));
    return;
  }

  job.requested_device = runtime_device.configured_device;
  job.resolved_device = runtime_device.device.str();
  job.device_source_section = runtime_device.source_section;
  job.device_contract_hash = runtime_device.contract_hash;
  job.device_error = runtime_device.error;
  job.cuda_required =
      runtime_device.resolved_from_config &&
      tsiemene::runtime_binding_requests_cuda_device_value(
          runtime_device.configured_device);
  if (!cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, job,
                                                         &error)) {
    log_warn(
        "[cuwacunu_campaign] cannot persist runtime job device metadata cursor=%s error=%s\n",
        value_or_empty(trimmed_job_cursor),
        value_or_empty(error));
  }
}

struct runtime_job_trace_sink_t {
  std::filesystem::path trace_path{};
};

std::string runtime_binding_trace_event_json(
    const tsiemene::runtime_binding_trace_event_t& event) {
  std::ostringstream out;
  out << "{"
      << "\"timestamp_ms\":" << event.timestamp_ms
      << ",\"phase\":\"" << json_escape_copy(event.phase) << "\""
      << ",\"campaign_hash\":\"" << json_escape_copy(event.campaign_hash) << "\""
      << ",\"binding_id\":\"" << json_escape_copy(event.binding_id) << "\""
      << ",\"contract_hash\":\"" << json_escape_copy(event.contract_hash) << "\""
      << ",\"wave_hash\":\"" << json_escape_copy(event.wave_hash) << "\""
      << ",\"contract_name\":\"" << json_escape_copy(event.contract_name) << "\""
      << ",\"component_name\":\"" << json_escape_copy(event.component_name)
      << "\""
      << ",\"resolved_record_type\":\""
      << json_escape_copy(event.resolved_record_type) << "\""
      << ",\"resolved_sampler\":\""
      << json_escape_copy(event.resolved_sampler) << "\""
      << ",\"run_id\":\"" << json_escape_copy(event.run_id) << "\""
      << ",\"source_runtime_cursor\":\""
      << json_escape_copy(event.source_runtime_cursor) << "\""
      << ",\"contract_index\":" << event.contract_index
      << ",\"contract_count\":" << event.contract_count
      << ",\"epochs\":" << event.epochs
      << ",\"epoch\":" << event.epoch
      << ",\"epoch_steps\":" << event.epoch_steps
      << ",\"total_steps\":" << event.total_steps
      << ",\"optimizer_steps\":" << event.optimizer_steps
      << ",\"trained_epochs\":" << event.trained_epochs
      << ",\"trained_samples\":" << event.trained_samples
      << ",\"skipped_batches\":" << event.skipped_batches
      << ",\"ok\":" << (event.ok ? "true" : "false")
      << ",\"error\":\"" << json_escape_copy(event.error) << "\""
      ;
  const auto append_finite_double =
      [&](const char* key, double value) {
        if (!std::isfinite(value)) return;
        out << ",\"" << key << "\":" << std::fixed << std::setprecision(12)
            << value;
      };
  append_finite_double("epoch_loss_mean", event.epoch_loss_mean);
  append_finite_double("last_committed_loss_mean",
                       event.last_committed_loss_mean);
  append_finite_double("loss_inv_mean", event.loss_inv_mean);
  append_finite_double("loss_var_mean", event.loss_var_mean);
  append_finite_double("loss_cov_raw_mean", event.loss_cov_raw_mean);
  append_finite_double("loss_cov_weighted_mean", event.loss_cov_weighted_mean);
  append_finite_double("learning_rate", event.learning_rate);
  out << "}\n";
  return out.str();
}

void emit_runtime_binding_trace_to_job(
    const tsiemene::runtime_binding_trace_event_t& event,
    void* user) {
  const auto* sink = static_cast<const runtime_job_trace_sink_t*>(user);
  if (!sink) return;
  (void)append_job_trace_line(sink->trace_path,
                              runtime_binding_trace_event_json(event));
}

bool sha256_hex_bytes(std::string_view payload, std::string* out_hex) {
  if (!out_hex) return false;
  constexpr std::size_t kSha256DigestBytes = 32;
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return false;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok || digest_len != kSha256DigestBytes) return false;

  static constexpr char kHex[] = "0123456789abcdef";
  out_hex->clear();
  out_hex->reserve(kSha256DigestBytes * 2);
  for (unsigned int i = 0; i < digest_len; ++i) {
    const unsigned char byte = digest[i];
    out_hex->push_back(kHex[(byte >> 4) & 0x0f]);
    out_hex->push_back(kHex[byte & 0x0f]);
  }
  return true;
}

bool parse_trailing_u64_suffix(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::size_t dot = text.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= text.size()) return false;
  std::uint64_t value = 0;
  const auto* begin = text.data() + dot + 1;
  const auto* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) return false;
  *out = value;
  return true;
}

cuwacunu::hashimyei::hashimyei_t make_hashimyei_identity(
    cuwacunu::hashimyei::hashimyei_kind_e kind,
    std::string_view seed,
    std::string hash_sha256_hex = {}) {
  return cuwacunu::hashimyei::make_identity(
      kind,
      cuwacunu::hashimyei::fnv1a64(seed),
      std::move(hash_sha256_hex));
}

bool build_wave_contract_binding_identity(
    std::string_view contract_hash,
    std::string_view wave_hash,
    std::string_view binding_id,
    cuwacunu::hero::hashimyei::wave_contract_binding_t* out) {
  if (!out) return false;
  out->contract = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT,
      contract_hash,
      std::string(contract_hash));
  out->wave = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE,
      wave_hash,
      std::string(wave_hash));
  out->binding_id = std::string(binding_id);

  std::ostringstream seed;
  seed << contract_hash << "|" << wave_hash << "|" << binding_id;
  std::string binding_hash{};
  if (!sha256_hex_bytes(seed.str(), &binding_hash) || binding_hash.empty()) {
    return false;
  }
  out->identity = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE_CONTRACT_BINDING,
      binding_hash,
      binding_hash);
  return true;
}

std::vector<cuwacunu::hero::hashimyei::component_instance_t>
runtime_component_instances_for_contract(
    const tsiemene::RuntimeBindingContract& contract) {
  std::vector<cuwacunu::hero::hashimyei::component_instance_t> out{};
  out.reserve(contract.spec.component_types.size());
  for (const auto& raw_type : contract.spec.component_types) {
    const std::string canonical_type = trim_ascii_copy(raw_type);
    if (canonical_type.empty()) continue;
    cuwacunu::hero::hashimyei::component_instance_t instance{};
    instance.family = canonical_type;
    if (canonical_type == contract.spec.representation_type &&
        !contract.spec.representation_hashimyei.empty()) {
      instance.hashimyei =
          trim_ascii_copy(contract.spec.representation_hashimyei);
      instance.canonical_path =
          canonical_type + "." + instance.hashimyei;
    } else {
      instance.canonical_path = canonical_type;
    }
    out.push_back(std::move(instance));
  }
  return out;
}

bool persist_runtime_run_manifests(
    const std::string& campaign_hash,
    const tsiemene::runtime_binding_run_record_t& run_record,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>&
        runtime_binding_itself,
    std::string* error) {
  if (error) error->clear();
  if (!runtime_binding_itself) {
    if (error) *error = "missing runtime binding record while saving run manifests";
    return false;
  }
  if (run_record.run_ids.empty()) {
    if (error) *error = "runtime binding run produced no run_ids";
    return false;
  }
  if (run_record.run_ids.size() != run_record.runtime_binding.contracts.size()) {
    if (error) {
      *error = "runtime binding run_ids/contracts size mismatch while saving "
               "run manifests";
    }
    return false;
  }

  const auto store_root = cuwacunu::hashimyei::store_root();
  const auto campaign_identity = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CAMPAIGN,
      campaign_hash);

  cuwacunu::hero::hashimyei::wave_contract_binding_t binding{};
  if (!build_wave_contract_binding_identity(
          run_record.contract_hash,
          run_record.wave_hash,
          run_record.binding_id,
          &binding)) {
    if (error) *error = "unable to derive wave_contract_binding identity";
    return false;
  }

  std::vector<cuwacunu::hero::hashimyei::dependency_file_t> dependency_files{};
  dependency_files.reserve(runtime_binding_itself->dependency_manifest.files.size());
  for (const auto& file : runtime_binding_itself->dependency_manifest.files) {
    dependency_files.push_back(
        {.canonical_path = compact_runtime_dependency_path(file.canonical_path),
         .sha256_hex = file.sha256_hex});
  }

  for (std::size_t i = 0; i < run_record.run_ids.size(); ++i) {
    const auto& run_id = run_record.run_ids[i];
    cuwacunu::hero::hashimyei::run_manifest_t manifest{};
    manifest.run_id = run_id;
    manifest.campaign_identity = campaign_identity;
    manifest.wave_contract_binding = binding;
    manifest.sampler = trim_ascii_copy(run_record.resolved_sampler);
    manifest.record_type = trim_ascii_copy(run_record.resolved_record_type);
    manifest.dependency_files = dependency_files;
    manifest.components =
        runtime_component_instances_for_contract(
            run_record.runtime_binding.contracts[i]);
    if (!parse_trailing_u64_suffix(run_id, &manifest.started_at_ms)) {
      manifest.started_at_ms = 0;
    }
    std::string save_error;
    if (!cuwacunu::hero::hashimyei::save_run_manifest(
            store_root, manifest, nullptr, &save_error)) {
      if (error) {
        *error = "save_run_manifest failed for " + run_id + ": " + save_error;
      }
      return false;
    }
  }
  return true;
}

std::filesystem::path source_runtime_projection_report_path(
    std::string_view canonical_path, std::string_view contract_hash,
    std::string_view binding_id, std::string_view run_id) {
  const std::string contract_token =
      cuwacunu::hashimyei::compact_contract_hash_path_token(contract_hash);
  return cuwacunu::hashimyei::canonical_path_directory(
             cuwacunu::hashimyei::store_root(), canonical_path) /
         "contracts" /
         (contract_token.empty() ? std::string("c.default") : contract_token) /
         "bindings" /
         sanitize_path_token_copy(binding_id) / "runs" /
         sanitize_path_token_copy(run_id) /
         std::string(
             cuwacunu::hero::wave::kSourceRuntimeProjectionLatestReportFilename);
}

bool persist_source_runtime_projection_reports(
    const tsiemene::runtime_binding_run_record_t& run_record,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>&
        runtime_binding_itself,
    std::string* error) {
  if (error) error->clear();
  if (!runtime_binding_itself) {
    if (error) {
      *error =
          "missing runtime binding record while saving source runtime projection";
    }
    return false;
  }
  if (run_record.run_ids.size() != run_record.runtime_binding.contracts.size()) {
    if (error) {
      *error = "runtime binding run_ids/contracts size mismatch while saving "
               "source runtime projection";
    }
    return false;
  }

  const auto& instruction = runtime_binding_itself->runtime_binding.decoded();
  const auto* bind = ::tsiemene::find_bind_by_id(instruction, run_record.binding_id);
  if (!bind) {
    if (error) {
      *error = "binding not found while saving source runtime projection: " +
               run_record.binding_id;
    }
    return false;
  }

  const auto wave_hash = cuwacunu::iitepi::runtime_binding_space_t::
      wave_hash_for_binding(run_record.campaign_hash, run_record.binding_id);
  const auto wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  if (!wave_itself) {
    if (error) {
      *error = "wave record not found while saving source runtime projection: " +
               wave_hash;
    }
    return false;
  }
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* wave = ::tsiemene::find_wave_by_id(wave_set, bind->wave_ref);
  if (!wave) {
    if (error) {
      *error = "binding references unknown WAVE id while saving source runtime "
               "projection: " +
               bind->wave_ref;
    }
    return false;
  }

  for (std::size_t i = 0; i < run_record.runtime_binding.contracts.size(); ++i) {
    const auto& contract = run_record.runtime_binding.contracts[i];
    const std::string contract_hash =
        contract.spec.contract_hash.empty() ? run_record.contract_hash
                                            : contract.spec.contract_hash;
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    if (!contract_itself) {
      if (error) {
        *error = "contract record not found while saving source runtime "
                 "projection: " +
                 contract_hash;
      }
      return false;
    }

    cuwacunu::camahjucunu::observation_spec_t observation{};
    std::string observation_error{};
    if (!::tsiemene::runtime_binding_builder::load_wave_dataloader_observation_payloads(
            contract_itself, wave_itself, *wave, nullptr, nullptr,
            &observation, &observation_error)) {
      if (error) {
        *error = "cannot resolve wave observation while saving source runtime "
                 "projection for contract[" +
                 std::to_string(i) + "]: " + observation_error;
      }
      return false;
    }

    const std::string record_type =
        contract.spec.sample_type.empty() ? run_record.resolved_record_type
                                          : contract.spec.sample_type;
    cuwacunu::hero::wave::source_runtime_projection_fragment_t fragment{};
    std::string fragment_error{};
    if (!cuwacunu::hero::wave::build_source_runtime_projection_fragment_for_wave(
            record_type, *wave, observation, &fragment, &fragment_error)) {
      if (error) {
        *error = "cannot build source runtime projection for contract[" +
                 std::to_string(i) + "]: " + fragment_error;
      }
      return false;
    }

    std::string canonical_source_type = trim_ascii_copy(contract.spec.source_type);
    if (canonical_source_type.empty()) {
      canonical_source_type = std::string(
          tsiemene::tsi_type_token(tsiemene::TsiTypeId::SourceDataloader));
    }
    std::string symbol{};
    if (!wave->sources.empty()) symbol = trim_ascii_copy(wave->sources.front().symbol);
    cuwacunu::hero::wave::source_runtime_projection_report_identity_t identity{};
    identity.canonical_path = canonical_source_type;
    identity.contract_hash = contract_hash;
    if (!wave->sources.empty()) {
      const auto& source_decl = wave->sources.front();
      identity.source_runtime_cursor =
          cuwacunu::source::dataloader::make_source_runtime_cursor(
              source_decl.symbol,
              source_decl.from,
              source_decl.to);
    }
    identity.source_label =
        cuwacunu::source::dataloader::make_source_label(symbol);
    identity.binding_id = run_record.binding_id;
    std::uint64_t run_started_at_ms = 0;
    if (parse_trailing_u64_suffix(run_record.run_ids[i], &run_started_at_ms) &&
        cuwacunu::piaabo::latent_lineage_state::pack_runtime_wave_cursor(
            run_started_at_ms, 0, 0, &identity.wave_cursor)) {
      identity.has_wave_cursor = true;
    }

    std::string report_payload{};
    std::string report_error{};
    if (!cuwacunu::hero::wave::emit_source_runtime_projection_runtime_report(
            fragment, identity, &report_payload, &report_error)) {
      if (error) {
        *error = "cannot serialize source runtime projection for contract[" +
                 std::to_string(i) + "]: " + report_error;
      }
      return false;
    }

    const auto output_path = source_runtime_projection_report_path(
        identity.canonical_path, contract_hash, identity.binding_id,
        run_record.run_ids[i]);
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            output_path, report_payload, &report_error)) {
      if (error) {
        *error = "cannot persist source runtime projection for contract[" +
                 std::to_string(i) + "] at " + output_path.string() + ": " +
                 report_error;
      }
      return false;
    }
  }
  return true;
}

void print_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--global-config <path>] [--binding <binding_id>]"
            << " [--campaign-dsl <path>] [--campaigns-root <path>]"
            << " [--job-cursor <cursor>] [--job-trace <path>]"
            << " [--reset-runtime-state]\n"
            << "Defaults:\n"
            << "  --global-config " << DEFAULT_GLOBAL_CONFIG_PATH << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string global_config_path = DEFAULT_GLOBAL_CONFIG_PATH;
  std::string binding_override;
  std::string campaign_dsl_override;
  std::string job_trace_path;
  std::string campaigns_root_path;
  std::string job_cursor;
  bool reset_runtime_state_flag = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--binding" && i + 1 < argc) {
      binding_override = argv[++i];
      continue;
    }
    if (arg == "--campaign-dsl" && i + 1 < argc) {
      campaign_dsl_override = argv[++i];
      continue;
    }
    if (arg == "--campaigns-root" && i + 1 < argc) {
      campaigns_root_path = argv[++i];
      continue;
    }
    if (arg == "--job-cursor" && i + 1 < argc) {
      job_cursor = argv[++i];
      continue;
    }
    if (arg == "--job-trace" && i + 1 < argc) {
      job_trace_path = argv[++i];
      continue;
    }
    if (arg == "--reset-runtime-state") {
      reset_runtime_state_flag = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  try {
    cuwacunu::iitepi::config_space_t::change_config_file(
        global_config_path.c_str());
    if (!campaign_dsl_override.empty()) {
      cuwacunu::iitepi::config_space_t::set_runtime_campaign_dsl_override(
          campaign_dsl_override);
    } else {
      cuwacunu::iitepi::config_space_t::clear_runtime_campaign_dsl_override();
    }
    cuwacunu::iitepi::config_space_t::update_config();

    if (reset_runtime_state_flag) {
      cuwacunu::hero::runtime_dev::runtime_reset_result_t reset{};
      std::string reset_error;
      if (!cuwacunu::hero::runtime_dev::reset_runtime_state_from_active_config(
              &reset, &reset_error)) {
        std::cerr << "[cuwacunu_campaign] runtime reset failed: " << reset_error
                  << "\n";
        return 1;
      }
      log_info(
          "[cuwacunu_campaign] runtime reset ok removed_store_roots=%zu removed_catalogs=%zu removed_entries=%llu\n",
          reset.removed_store_roots.size(),
          reset.removed_catalog_paths.size(),
          static_cast<unsigned long long>(reset.removed_store_entries));
      for (const auto& path : reset.removed_store_roots) {
        log_info("[cuwacunu_campaign] runtime reset store_root=%s\n",
                 path.string().c_str());
      }
      for (const auto& path : reset.removed_catalog_paths) {
        log_info("[cuwacunu_campaign] runtime reset catalog_path=%s\n",
                 path.string().c_str());
      }
    }

    const std::string configured_campaign_path =
        cuwacunu::iitepi::config_space_t::effective_campaign_dsl_path();
    if (configured_campaign_path.empty()) {
      std::cerr << "[cuwacunu_campaign] empty configured campaign path\n";
      return 1;
    }

    if (!binding_override.empty()) {
      cuwacunu::iitepi::runtime_binding_space_t::init(configured_campaign_path,
                                            binding_override);
    } else {
      cuwacunu::iitepi::runtime_binding_space_t::init(configured_campaign_path);
    }
    cuwacunu::iitepi::runtime_binding_space_t::assert_locked_runtime_intact_or_fail_fast();

    const std::string binding_id =
        cuwacunu::iitepi::runtime_binding_space_t::locked_binding_id();
    const std::filesystem::path trace_path =
        std::filesystem::path(job_trace_path);
    runtime_job_trace_sink_t trace_sink{.trace_path = trace_path};
    std::optional<tsiemene::runtime_binding_trace_scope_t> trace_scope{};
    if (!trace_path.empty()) {
      trace_scope.emplace(tsiemene::runtime_binding_trace_sink_t{
          .emit = emit_runtime_binding_trace_to_job, .user = &trace_sink});
      std::ostringstream trace_start;
      trace_start << "{\"timestamp_ms\":"
                  << cuwacunu::hero::runtime::now_ms_utc()
                  << ",\"phase\":\"worker_start\""
                  << ",\"binding_id\":\"" << json_escape_copy(binding_id) << "\""
                  << ",\"campaign_dsl\":\""
                  << json_escape_copy(configured_campaign_path) << "\""
                  << ",\"ok\":true}\n";
      (void)append_job_trace_line(trace_path, trace_start.str());
    }

    const std::string campaign_hash =
        cuwacunu::iitepi::runtime_binding_space_t::locked_runtime_binding_hash();
    log_info("[cuwacunu_campaign] campaign_hash=%s binding=%s campaign=%s\n",
             value_or_empty(campaign_hash), value_or_empty(binding_id),
             value_or_empty(configured_campaign_path));

    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    const auto runtime_device =
        tsiemene::resolve_runtime_binding_preferred_device_for_binding(
            binding_id, runtime_binding_itself);
    if (!runtime_device.error.empty()) {
      log_warn(
          "[cuwacunu_campaign] runtime device resolution fallback binding=%s error=%s fallback_device=%s\n",
          value_or_empty(binding_id),
          value_or_empty(runtime_device.error),
          runtime_device.device.str().c_str());
    } else if (runtime_device.resolved_from_config) {
      log_info(
          "[cuwacunu_campaign] runtime device resolved binding=%s contract_hash=%s source=%s configured=%s resolved=%s\n",
          value_or_empty(binding_id),
          value_or_empty(runtime_device.contract_hash),
          value_or_empty(runtime_device.source_section),
          value_or_empty(runtime_device.configured_device),
          runtime_device.device.str().c_str());
    } else {
      log_info(
          "[cuwacunu_campaign] runtime device unresolved in contract config binding=%s fallback_device=%s\n",
          value_or_empty(binding_id),
          runtime_device.device.str().c_str());
    }
    refresh_runtime_job_device_metadata(
        std::filesystem::path(campaigns_root_path), job_cursor, runtime_device);
    const bool cuda_required_for_binding =
        runtime_device.resolved_from_config &&
        tsiemene::runtime_binding_requests_cuda_device_value(
            runtime_device.configured_device);
    if (cuda_required_for_binding && runtime_device.device.is_cpu()) {
      const std::string detail =
          runtime_device.error.empty() ? std::string{}
                                       : " (" + runtime_device.error + ")";
      log_err(
          "[cuwacunu_campaign] refusing to run binding=%s on CPU because configured device '%s' explicitly requires CUDA%s\n",
          value_or_empty(binding_id),
          value_or_empty(runtime_device.configured_device),
          detail.c_str());
      return 1;
    }

    const auto run = cuwacunu::iitepi::run_runtime_binding(
        binding_id, runtime_device.device);
    if (!run.ok) {
      if (!trace_path.empty()) {
        std::ostringstream trace_fail;
        trace_fail << "{\"timestamp_ms\":"
                   << cuwacunu::hero::runtime::now_ms_utc()
                   << ",\"phase\":\"worker_end\""
                   << ",\"campaign_hash\":\""
                   << json_escape_copy(run.campaign_hash) << "\""
                   << ",\"binding_id\":\"" << json_escape_copy(run.binding_id)
                   << "\""
                   << ",\"total_steps\":" << run.total_steps
                   << ",\"ok\":false"
                   << ",\"error\":\"" << json_escape_copy(run.error) << "\"}\n";
        (void)append_job_trace_line(trace_path, trace_fail.str());
      }
      log_err(
          "[cuwacunu_campaign] run failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(run.error));
      log_err(
          "[cuwacunu_campaign] contract_hash=%s wave_hash=%s record_type=%s sampler=%s total_steps=%llu\n",
          value_or_empty(run.contract_hash),
          value_or_empty(run.wave_hash),
          value_or_empty(run.resolved_record_type),
          value_or_empty(run.resolved_sampler),
          static_cast<unsigned long long>(run.total_steps));
      return 1;
    }

    std::string manifest_error;
    if (!persist_runtime_run_manifests(
            campaign_hash, run, runtime_binding_itself, &manifest_error)) {
      if (!trace_path.empty()) {
        std::ostringstream trace_fail;
        trace_fail << "{\"timestamp_ms\":"
                   << cuwacunu::hero::runtime::now_ms_utc()
                   << ",\"phase\":\"run_manifests_persisted\""
                   << ",\"campaign_hash\":\"" << json_escape_copy(run.campaign_hash)
                   << "\""
                   << ",\"binding_id\":\"" << json_escape_copy(run.binding_id)
                   << "\""
                   << ",\"ok\":false"
                   << ",\"error\":\"" << json_escape_copy(manifest_error)
                   << "\"}\n";
        (void)append_job_trace_line(trace_path, trace_fail.str());
      }
      log_err(
          "[cuwacunu_campaign] run manifest persistence failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(manifest_error));
      return 1;
    }
    if (!persist_source_runtime_projection_reports(
            run, runtime_binding_itself, &manifest_error)) {
      if (!trace_path.empty()) {
        std::ostringstream trace_fail;
        trace_fail << "{\"timestamp_ms\":"
                   << cuwacunu::hero::runtime::now_ms_utc()
                   << ",\"phase\":\"source_runtime_projection_persisted\""
                   << ",\"campaign_hash\":\"" << json_escape_copy(run.campaign_hash)
                   << "\""
                   << ",\"binding_id\":\"" << json_escape_copy(run.binding_id)
                   << "\""
                   << ",\"ok\":false"
                   << ",\"error\":\"" << json_escape_copy(manifest_error)
                   << "\"}\n";
        (void)append_job_trace_line(trace_path, trace_fail.str());
      }
      log_err(
          "[cuwacunu_campaign] source runtime projection persistence failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(manifest_error));
      return 1;
    }
    if (!trace_path.empty()) {
      std::ostringstream trace_ok;
      trace_ok << "{\"timestamp_ms\":"
               << cuwacunu::hero::runtime::now_ms_utc()
               << ",\"phase\":\"run_manifests_persisted\""
               << ",\"campaign_hash\":\"" << json_escape_copy(run.campaign_hash)
               << "\""
               << ",\"binding_id\":\"" << json_escape_copy(run.binding_id) << "\""
               << ",\"ok\":true}\n";
      (void)append_job_trace_line(trace_path, trace_ok.str());
      trace_ok.str({});
      trace_ok.clear();
      trace_ok << "{\"timestamp_ms\":"
               << cuwacunu::hero::runtime::now_ms_utc()
               << ",\"phase\":\"source_runtime_projection_persisted\""
               << ",\"campaign_hash\":\"" << json_escape_copy(run.campaign_hash)
               << "\""
               << ",\"binding_id\":\"" << json_escape_copy(run.binding_id) << "\""
               << ",\"ok\":true}\n";
      (void)append_job_trace_line(trace_path, trace_ok.str());
      trace_ok.str({});
      trace_ok.clear();
      trace_ok << "{\"timestamp_ms\":"
               << cuwacunu::hero::runtime::now_ms_utc()
               << ",\"phase\":\"worker_end\""
               << ",\"campaign_hash\":\"" << json_escape_copy(run.campaign_hash)
               << "\""
               << ",\"binding_id\":\"" << json_escape_copy(run.binding_id) << "\""
               << ",\"total_steps\":" << run.total_steps
               << ",\"ok\":true}\n";
      (void)append_job_trace_line(trace_path, trace_ok.str());
    }

    log_info(
        "[cuwacunu_campaign] run ok campaign_hash=%s binding=%s contract_hash=%s wave_hash=%s record_type=%s sampler=%s contracts=%zu total_steps=%llu\n",
        value_or_empty(run.campaign_hash),
        value_or_empty(run.binding_id),
        value_or_empty(run.contract_hash),
        value_or_empty(run.wave_hash),
        value_or_empty(run.resolved_record_type),
        value_or_empty(run.resolved_sampler),
        run.contract_steps.size(),
        static_cast<unsigned long long>(run.total_steps));
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[cuwacunu_campaign] exception: " << e.what() << "\n";
    return 1;
  }
}
