/* observation_spec.cpp */
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/observation_pipeline/observation_channels_decoder.h"
#include "camahjucunu/dsl/observation_pipeline/observation_sources_decoder.h"
#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/observation_dsl_paths.h"
#include "iitepi/runtime_binding/runtime_binding_space_t.h"
#include "iitepi/wave_space_t.h"
#include "piaabo/dconfig.h"

DEV_WARNING("(observation_spec.cpp)[] mutex on observation runtime might not "
            "be needed \n");
DEV_WARNING(
    "(observation_spec.cpp)[] observation runtime should include and expose "
    "the dataloaders, dataloaders should not be external variables \n");

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {

/* ───────────────────── observation_runtime_t statics ───────────────────── */
observation_spec_t observation_runtime_t::inst{};
observation_runtime_t::_init observation_runtime_t::_initializer{};

namespace {

std::string &observation_runtime_last_contract_hash_() {
  static std::string hash;
  return hash;
}

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

} // namespace

/* ───────────────────── observation_spec_t methods ───────────────────── */

std::vector<observation_source_t> observation_spec_t::filter_source_forms(
    const instrument_signature_t &target_signature,
    cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const {
  std::vector<observation_source_t> result;
  for (const auto &form : source_forms) {
    if (form.interval != target_interval)
      continue;
    if (instrument_signature_compact_string(form.instrument_signature()) ==
        instrument_signature_compact_string(target_signature)) {
      result.push_back(form);
    }
  }
  return result;
}

bool observation_spec_t::active_sources_match_runtime_signature(
    const instrument_signature_t &runtime_signature, std::string *error) const {
  if (error)
    error->clear();

  std::string validation_error{};
  if (!instrument_signature_validate(runtime_signature, /*allow_any=*/false,
                                     "RUNTIME_INSTRUMENT_SIGNATURE",
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }

  bool saw_active = false;
  for (const auto &channel : channel_forms) {
    if (trim_ascii_ws_copy(channel.active) != "true")
      continue;
    saw_active = true;
    if (trim_ascii_ws_copy(channel.record_type) !=
        runtime_signature.record_type) {
      if (error) {
        *error = "active observation channel record_type '" +
                 trim_ascii_ws_copy(channel.record_type) +
                 "' does not match RUNTIME_INSTRUMENT_SIGNATURE.RECORD_TYPE " +
                 runtime_signature.record_type;
      }
      return false;
    }
    const auto matching_sources =
        filter_source_forms(runtime_signature, channel.interval);
    if (matching_sources.empty()) {
      if (error) {
        *error = "no observation source row for runtime instrument " +
                 instrument_signature_compact_string(runtime_signature) +
                 " on an active interval";
      }
      return false;
    }
  }
  if (!saw_active) {
    if (error)
      *error = "observation channel DSL has no active rows";
    return false;
  }
  return true;
}

std::vector<float> observation_spec_t::retrieve_channel_weights() {
  std::vector<float> channel_weights;
  channel_weights.reserve(channel_forms.size());
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        channel_weights.push_back(
            static_cast<float>(std::stod(in_form.channel_weight)));
      } catch (...) {
        channel_weights.push_back(0.0f);
      }
    }
  }
  return channel_weights;
}

int64_t observation_spec_t::count_channels() {
  int64_t count = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true")
      ++count;
  }
  return count;
}

int64_t observation_spec_t::max_sequence_length() {
  int64_t max_seq = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v = static_cast<int64_t>(std::stoll(in_form.seq_length));
        if (v > max_seq)
          max_seq = v;
      } catch (...) {
      }
    }
  }
  return max_seq;
}

int64_t observation_spec_t::max_future_sequence_length() {
  int64_t max_fut_seq = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v =
            static_cast<int64_t>(std::stoll(in_form.future_seq_length));
        if (v > max_fut_seq)
          max_fut_seq = v;
      } catch (...) {
      }
    }
  }
  return max_fut_seq;
}

/* ───────────────────── _t lifecycle ───────────────────── */

void observation_runtime_t::init() {
  log_dbg(
      "[observation_runtime_t] initializing static-global observation snapshot "
      "(single mutable cache updated by explicit contract hash)\n");
  // Keep startup side-effect free; runtime state is materialized via update().
}

void observation_runtime_t::finit() {
  const auto &last_contract_hash = observation_runtime_last_contract_hash_();
  const char *last_hash =
      last_contract_hash.empty() ? "<none>" : last_contract_hash.c_str();
  log_dbg(
      "[observation_runtime_t] finalizing static-global observation snapshot "
      "(last_contract_hash=%s)\n",
      last_hash);
}

void observation_runtime_t::update(const std::string &contract_hash) {
  bool has_non_ws_hash = false;
  for (const unsigned char c : contract_hash) {
    if (!std::isspace(c)) {
      has_non_ws_hash = true;
      break;
    }
  }
  if (!has_non_ws_hash) {
    throw std::runtime_error(
        "observation_runtime_t::update requires non-empty contract hash");
  }
  observation_runtime_last_contract_hash_() = contract_hash;
  inst = decode_observation_spec_from_contract(contract_hash);
}

namespace {

[[nodiscard]] bool has_non_ws(const std::string &s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::string resolve_path_from_folder(const std::string &folder,
                                                   std::string path) {
  path = trim_ascii_ws_copy(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.string();
  if (folder.empty())
    return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] std::string maybe_concat_instruction(std::string title,
                                                   const std::string &payload) {
  if (!has_non_ws(payload))
    return {};
  std::ostringstream oss;
  oss << "/* " << title << " */\n" << payload << "\n";
  return oss.str();
}

struct resolved_observation_payload_t {
  std::string source_instruction{};
  std::string channel_instruction{};
  std::string source_grammar{};
  std::string channel_grammar{};
};

[[nodiscard]] const cuwacunu::camahjucunu::iitepi_wave_t *
find_wave_by_id_or_null(
    const cuwacunu::camahjucunu::iitepi_wave_set_t &wave_set,
    const std::string &wave_id) {
  for (const auto &wave : wave_set.waves) {
    if (wave.name == wave_id)
      return &wave;
  }
  return nullptr;
}

[[nodiscard]] resolved_observation_payload_t
resolve_observation_payload_for_contract_or_throw(
    const std::string &contract_hash) {
  if (!has_non_ws(contract_hash)) {
    throw std::runtime_error("decode_observation_spec_from_contract requires "
                             "non-empty contract hash");
  }

  const std::string runtime_binding_hash =
      cuwacunu::iitepi::config_space_t::locked_runtime_binding_hash();
  const std::string binding_id =
      cuwacunu::iitepi::config_space_t::locked_binding_id();
  if (!has_non_ws(runtime_binding_hash) || !has_non_ws(binding_id)) {
    throw std::runtime_error(
        "cannot resolve active runtime binding while loading observation DSL");
  }

  const std::string bound_contract_hash =
      cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
          runtime_binding_hash, binding_id);
  if (bound_contract_hash != contract_hash) {
    throw std::runtime_error("decode_observation_spec_from_contract received "
                             "contract hash that does not "
                             "match the active runtime binding");
  }

  const auto runtime_binding_itself =
      cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
          runtime_binding_hash);
  const auto &runtime_binding_instruction =
      runtime_binding_itself->runtime_binding.decoded();
  const cuwacunu::camahjucunu::runtime_binding_bind_decl_t *bind = nullptr;
  for (const auto &b : runtime_binding_instruction.binds) {
    if (b.id == binding_id) {
      bind = &b;
      break;
    }
  }
  if (!bind) {
    throw std::runtime_error("active runtime binding id not found in decoded "
                             "runtime-binding instruction");
  }

  const std::string wave_hash =
      cuwacunu::iitepi::runtime_binding_space_t::wave_hash_for_binding(
          runtime_binding_hash, binding_id);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  const auto &wave_set = wave_itself->wave.decoded();
  const auto *selected_wave = find_wave_by_id_or_null(wave_set, bind->wave_ref);
  if (!selected_wave) {
    throw std::runtime_error("bound wave id not found in decoded wave DSL");
  }
  if (selected_wave->sources.size() != 1) {
    throw std::runtime_error(
        "runtime currently requires exactly one SOURCE block in selected wave");
  }

  cuwacunu::iitepi::observation_dsl_path_resolution_t observation_paths{};
  std::string path_error{};
  if (!cuwacunu::iitepi::resolve_observation_dsl_paths(
          contract_itself, wave_itself, *selected_wave, &observation_paths,
          &path_error)) {
    throw std::runtime_error(path_error);
  }
  if (!has_non_ws(observation_paths.sources_path) ||
      !std::filesystem::exists(observation_paths.sources_path) ||
      !std::filesystem::is_regular_file(observation_paths.sources_path)) {
    throw std::runtime_error(
        "invalid observation sources DSL for selected wave: " +
        observation_paths.sources_path);
  }
  if (!has_non_ws(observation_paths.channels_path) ||
      !std::filesystem::exists(observation_paths.channels_path) ||
      !std::filesystem::is_regular_file(observation_paths.channels_path)) {
    throw std::runtime_error(
        "invalid observation channels DSL for selected wave: " +
        observation_paths.channels_path);
  }

  const std::string source_grammar_path = resolve_path_from_folder(
      std::filesystem::path(cuwacunu::iitepi::config_space_t::config_file_path)
          .parent_path()
          .string(),
      unquote_if_wrapped(cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "observation_sources_grammar_filename")));
  const std::string channel_grammar_path = resolve_path_from_folder(
      std::filesystem::path(cuwacunu::iitepi::config_space_t::config_file_path)
          .parent_path()
          .string(),
      unquote_if_wrapped(cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "observation_channels_grammar_filename")));
  if (!has_non_ws(source_grammar_path) ||
      !std::filesystem::exists(source_grammar_path) ||
      !std::filesystem::is_regular_file(source_grammar_path)) {
    throw std::runtime_error(
        "invalid [BNF].observation_sources_grammar_filename path");
  }
  if (!has_non_ws(channel_grammar_path) ||
      !std::filesystem::exists(channel_grammar_path) ||
      !std::filesystem::is_regular_file(channel_grammar_path)) {
    throw std::runtime_error(
        "invalid [BNF].observation_channels_grammar_filename path");
  }

  resolved_observation_payload_t out{};
  out.source_instruction = cuwacunu::piaabo::dfiles::readFileToString(
      observation_paths.sources_path);
  out.channel_instruction = cuwacunu::piaabo::dfiles::readFileToString(
      observation_paths.channels_path);
  out.source_grammar =
      cuwacunu::piaabo::dfiles::readFileToString(source_grammar_path);
  out.channel_grammar =
      cuwacunu::piaabo::dfiles::readFileToString(channel_grammar_path);
  if (!has_non_ws(out.source_instruction) ||
      !has_non_ws(out.channel_instruction)) {
    throw std::runtime_error("selected wave observation DSL payload is empty");
  }
  if (!has_non_ws(out.source_grammar) || !has_non_ws(out.channel_grammar)) {
    throw std::runtime_error("observation grammar payload is empty");
  }
  return out;
}

} // namespace

std::string
observation_spec_source_dump_from_contract(const std::string &contract_hash) {
  try {
    const auto payload =
        resolve_observation_payload_for_contract_or_throw(contract_hash);
    return maybe_concat_instruction("observation.sources",
                                    payload.source_instruction) +
           maybe_concat_instruction("observation.channels",
                                    payload.channel_instruction);
  } catch (const std::exception &e) {
    return std::string(
               "ERROR: failed to resolve contract/wave observation DSL: ") +
           e.what() + "\n";
  }
}

observation_spec_t decode_observation_spec_from_split_dsl(
    std::string source_grammar, std::string source_instruction,
    std::string channel_grammar, std::string channel_instruction) {
  if (has_non_ws(source_grammar) && has_non_ws(source_instruction) &&
      has_non_ws(channel_grammar) && has_non_ws(channel_instruction)) {
    cuwacunu::camahjucunu::dsl::observationSourcesDecoder sources_decoder(
        std::move(source_grammar));
    cuwacunu::camahjucunu::dsl::observationChannelsDecoder channels_decoder(
        std::move(channel_grammar));

    observation_spec_t sources_part =
        sources_decoder.decode(source_instruction);
    observation_spec_t channels_part =
        channels_decoder.decode(channel_instruction);

    observation_spec_t merged{};
    merged.source_forms = std::move(sources_part.source_forms);
    merged.channel_forms = std::move(channels_part.channel_forms);
    merged.csv_bootstrap_deltas = sources_part.csv_bootstrap_deltas;
    merged.csv_step_abs_tol = sources_part.csv_step_abs_tol;
    merged.csv_step_rel_tol = sources_part.csv_step_rel_tol;
    merged.data_analytics_policy = sources_part.data_analytics_policy;
    if (!merged.data_analytics_policy.declared) {
      throw std::runtime_error(
          "DATA_ANALYTICS_POLICY block is required in sources DSL");
    }
    if (merged.data_analytics_policy.max_samples <= 0 ||
        merged.data_analytics_policy.max_features <= 0) {
      throw std::runtime_error(
          "DATA_ANALYTICS_POLICY requires MAX_SAMPLES and MAX_FEATURES > 0");
    }
    if (merged.data_analytics_policy.mask_epsilon < 0.0L) {
      throw std::runtime_error(
          "DATA_ANALYTICS_POLICY.MASK_EPSILON must be >= 0");
    }
    if (!(merged.data_analytics_policy.standardize_epsilon > 0.0L)) {
      throw std::runtime_error(
          "DATA_ANALYTICS_POLICY.STANDARDIZE_EPSILON must be > 0");
    }
    for (const auto &source_form : merged.source_forms) {
      std::string signature_error{};
      if (!instrument_signature_validate(
              source_form.instrument_signature(), /*allow_any=*/false,
              "source row " + source_form.instrument, &signature_error)) {
        throw std::runtime_error(signature_error);
      }
    }
    return merged;
  }

  throw std::runtime_error("split observation DSL is required; legacy "
                           "observation spec fallback has been removed");
}

observation_spec_t
decode_observation_spec_from_contract(const std::string &contract_hash) {
  const auto payload =
      resolve_observation_payload_for_contract_or_throw(contract_hash);
  return decode_observation_spec_from_split_dsl(
      payload.source_grammar, payload.source_instruction,
      payload.channel_grammar, payload.channel_instruction);
}

} // namespace camahjucunu
} // namespace cuwacunu
