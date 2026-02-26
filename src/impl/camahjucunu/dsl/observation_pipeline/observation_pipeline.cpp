/* observation_pipeline.cpp */
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/dsl/observation_pipeline/sources/observation_sources_pipeline.h"
#include "camahjucunu/dsl/observation_pipeline/channels/observation_channels_pipeline.h"

RUNTIME_WARNING("(observation_pipeline.cpp)[] mutex on observation pipeline might not be needed \n");
RUNTIME_WARNING("(observation_pipeline.cpp)[] observation pipeline object should include and expose the dataloaders, dataloaders should not be external variables \n");

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace cuwacunu {
namespace camahjucunu {

/* ───────────────────── observation_pipeline_t statics ───────────────────── */
observation_instruction_t observation_pipeline_t::inst{};
observation_pipeline_t::_init observation_pipeline_t::_initializer{};

/* ───────────────────── observation_instruction_t methods ───────────────────── */

std::vector<source_form_t> observation_instruction_t::filter_source_forms(
  const std::string& target_instrument,
  const std::string& target_record_type,
  cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const
{
  std::vector<source_form_t> result;
  for (const auto& form : source_forms) {
    if (form.instrument == target_instrument &&
        form.record_type == target_record_type &&
        form.interval == target_interval) {
      result.push_back(form);
    }
  }
  return result;
}

std::vector<float> observation_instruction_t::retrieve_channel_weights() {
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

int64_t observation_instruction_t::count_channels() {
  int64_t count = 0;
  for (const auto& in_form : channel_forms) {
    if (in_form.active == "true") ++count;
  }
  return count;
}

int64_t observation_instruction_t::max_sequence_length() {
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

int64_t observation_instruction_t::max_future_sequence_length() {
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

void observation_pipeline_t::init() {
  log_info("[observation_pipeline_t] initialising\n");
  update();
}

void observation_pipeline_t::finit() {
  log_info("[observation_pipeline_t] finalising\n");
}

void observation_pipeline_t::update() {
  inst = decode_observation_instruction_from_config();
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

std::string observation_instruction_source_dump_from_config() {
  const auto source_instruction =
      cuwacunu::piaabo::dconfig::contract_space_t::observation_sources_dsl();
  const auto channel_instruction =
      cuwacunu::piaabo::dconfig::contract_space_t::observation_channels_dsl();

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

observation_instruction_t decode_observation_instruction_from_split_dsl(
    std::string source_grammar,
    std::string source_instruction,
    std::string channel_grammar,
    std::string channel_instruction) {
  if (has_non_ws(source_grammar) && has_non_ws(source_instruction) &&
      has_non_ws(channel_grammar) && has_non_ws(channel_instruction)) {
    cuwacunu::camahjucunu::dsl::observationSourcesPipeline sources_decoder(
        std::move(source_grammar));
    cuwacunu::camahjucunu::dsl::observationChannelsPipeline channels_decoder(
        std::move(channel_grammar));

    observation_instruction_t sources_part = sources_decoder.decode(source_instruction);
    observation_instruction_t channels_part = channels_decoder.decode(channel_instruction);

    observation_instruction_t merged{};
    merged.source_forms = std::move(sources_part.source_forms);
    merged.channel_forms = std::move(channels_part.channel_forms);
    return merged;
  }

  throw std::runtime_error(
      "split observation DSL is required; legacy observation_pipeline fallback has been removed");
}

observation_instruction_t decode_observation_instruction_from_config() {
  return decode_observation_instruction_from_split_dsl(
      cuwacunu::piaabo::dconfig::contract_space_t::observation_sources_grammar(),
      cuwacunu::piaabo::dconfig::contract_space_t::observation_sources_dsl(),
      cuwacunu::piaabo::dconfig::contract_space_t::observation_channels_grammar(),
      cuwacunu::piaabo::dconfig::contract_space_t::observation_channels_dsl());
}

} // namespace camahjucunu
} // namespace cuwacunu
