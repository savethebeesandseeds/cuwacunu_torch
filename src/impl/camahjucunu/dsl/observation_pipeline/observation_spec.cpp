/* observation_spec.cpp */
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/observation_pipeline/observation_sources_decoder.h"
#include "camahjucunu/dsl/observation_pipeline/observation_channels_decoder.h"
#include "piaabo/dconfig.h"

RUNTIME_WARNING("(observation_spec.cpp)[] mutex on observation runtime might not be needed \n");
RUNTIME_WARNING("(observation_spec.cpp)[] observation runtime should include and expose the dataloaders, dataloaders should not be external variables \n");

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

/* ───────────────────── observation_spec_t methods ───────────────────── */

std::vector<observation_source_t> observation_spec_t::filter_source_forms(
  const std::string& target_instrument,
  const std::string& target_record_type,
  cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const
{
  std::vector<observation_source_t> result;
  for (const auto& form : source_forms) {
    if (form.instrument == target_instrument &&
        form.record_type == target_record_type &&
        form.interval == target_interval) {
      result.push_back(form);
    }
  }
  return result;
}

std::vector<float> observation_spec_t::retrieve_channel_weights() {
  std::vector<float> channel_weights;
  channel_weights.reserve(channel_forms.size());
  for (const auto& in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        channel_weights.push_back(static_cast<float>(std::stod(in_form.channel_weight)));
      } catch (...) {
        channel_weights.push_back(0.0f);
      }
    }
  }
  return channel_weights;
}

int64_t observation_spec_t::count_channels() {
  int64_t count = 0;
  for (const auto& in_form : channel_forms) {
    if (in_form.active == "true") ++count;
  }
  return count;
}

int64_t observation_spec_t::max_sequence_length() {
  int64_t max_seq = 0;
  for (const auto& in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v = static_cast<int64_t>(std::stoll(in_form.seq_length));
        if (v > max_seq) max_seq = v;
      } catch (...) {
      }
    }
  }
  return max_seq;
}

int64_t observation_spec_t::max_future_sequence_length() {
  int64_t max_fut_seq = 0;
  for (const auto& in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v = static_cast<int64_t>(std::stoll(in_form.future_seq_length));
        if (v > max_fut_seq) max_fut_seq = v;
      } catch (...) {
      }
    }
  }
  return max_fut_seq;
}

/* ───────────────────── _t lifecycle ───────────────────── */

void observation_runtime_t::init() {
  log_info("[observation_runtime_t] initialising\n");
  // Runtime callers must explicitly provide a contract hash for updates.
  inst = observation_spec_t{};
}

void observation_runtime_t::finit() {
  log_info("[observation_runtime_t] finalising\n");
}

void observation_runtime_t::update(const std::string& contract_hash) {
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
  inst = decode_observation_spec_from_contract(contract_hash);
}

namespace {

[[nodiscard]] bool has_non_ws(const std::string& s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

[[nodiscard]] std::string maybe_concat_instruction(std::string title,
                                                   const std::string& payload) {
  if (!has_non_ws(payload)) return {};
  std::ostringstream oss;
  oss << "/* " << title << " */\n" << payload << "\n";
  return oss.str();
}

} // namespace

std::string observation_spec_source_dump_from_contract(
    const std::string& contract_hash) {
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  const auto& source_instruction = contract_itself->observation.sources.dsl;
  const auto& channel_instruction = contract_itself->observation.channels.dsl;

  if (has_non_ws(source_instruction) && has_non_ws(channel_instruction)) {
    return maybe_concat_instruction("observation.sources", source_instruction) +
           maybe_concat_instruction("observation.channels", channel_instruction);
  }

  return "ERROR: split observation DSL is required. Missing one or more of:\n"
         "  [DSL].observation_sources_grammar_filename\n"
         "  [DSL].observation_sources_dsl_filename\n"
         "  [DSL].observation_channels_grammar_filename\n"
         "  [DSL].observation_channels_dsl_filename\n";
}

observation_spec_t decode_observation_spec_from_split_dsl(
    std::string source_grammar,
    std::string source_instruction,
    std::string channel_grammar,
    std::string channel_instruction) {
  if (has_non_ws(source_grammar) && has_non_ws(source_instruction) &&
      has_non_ws(channel_grammar) && has_non_ws(channel_instruction)) {
    cuwacunu::camahjucunu::dsl::observationSourcesDecoder sources_decoder(
        std::move(source_grammar));
    cuwacunu::camahjucunu::dsl::observationChannelsDecoder channels_decoder(
        std::move(channel_grammar));

    observation_spec_t sources_part = sources_decoder.decode(source_instruction);
    observation_spec_t channels_part = channels_decoder.decode(channel_instruction);

    observation_spec_t merged{};
    merged.source_forms = std::move(sources_part.source_forms);
    merged.channel_forms = std::move(channels_part.channel_forms);
    return merged;
  }

  throw std::runtime_error(
      "split observation DSL is required; legacy observation spec fallback has been removed");
}

observation_spec_t decode_observation_spec_from_contract(
    const std::string& contract_hash) {
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  return contract_itself->observation.decoded();
}

} // namespace camahjucunu
} // namespace cuwacunu
