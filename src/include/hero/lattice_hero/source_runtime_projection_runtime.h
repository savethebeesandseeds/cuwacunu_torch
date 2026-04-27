// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/types/types_data.h"
#include "hero/lattice_hero/source_runtime_projection.h"

namespace cuwacunu {
namespace hero {
namespace wave {

namespace source_runtime_projection_runtime_detail {

[[nodiscard]] inline std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

template <typename Datatype_t>
[[nodiscard]] inline bool resolve_effective_domain_bounds_for_datatype(
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    double *out_domain_from_ms, double *out_domain_to_ms, std::string *error) {
  if (error)
    error->clear();
  if (!out_domain_from_ms || !out_domain_to_ms) {
    if (error)
      *error = "domain output pointers are null";
    return false;
  }
  *out_domain_from_ms = 0.0;
  *out_domain_to_ms = 0.0;

  try {
    auto dataset =
        cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<
            Datatype_t>(runtime_signature, observation,
                        /*force_rebuild_cache=*/false);
    const double domain_left = static_cast<double>(dataset.leftmost_key_value_);
    const double domain_right =
        static_cast<double>(dataset.rightmost_key_value_);
    const double step = static_cast<double>(dataset.key_value_step_);
    if (!std::isfinite(domain_left) || !std::isfinite(domain_right) ||
        !std::isfinite(step)) {
      if (error)
        *error = "effective source bounds are non-finite";
      return false;
    }
    if (!(step > 0.0)) {
      if (error)
        *error = "effective source key step must be > 0";
      return false;
    }
    *out_domain_from_ms = domain_left;
    *out_domain_to_ms = domain_right + step;
    return true;
  } catch (const std::exception &e) {
    if (error) {
      *error =
          std::string("failed to resolve effective source bounds: ") + e.what();
    }
    return false;
  } catch (...) {
    if (error)
      *error = "failed to resolve effective source bounds";
    return false;
  }
}

[[nodiscard]] inline bool resolve_effective_domain_bounds_for_record_type(
    std::string_view record_type,
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    double *out_domain_from_ms, double *out_domain_to_ms, std::string *error) {
  if (error)
    error->clear();
  const std::string rt =
      lowercase_copy(source_runtime_projection_detail::trim_ascii(record_type));
  if (rt == "kline") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::kline_t>(
        runtime_signature, observation, out_domain_from_ms, out_domain_to_ms,
        error);
  }
  if (rt == "trade") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::trade_t>(
        runtime_signature, observation, out_domain_from_ms, out_domain_to_ms,
        error);
  }
  if (rt == "basic") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::basic_t>(
        runtime_signature, observation, out_domain_from_ms, out_domain_to_ms,
        error);
  }
  if (error) {
    *error = "unsupported record_type for source-runtime projection: " + rt;
  }
  return false;
}

[[nodiscard]] inline bool parse_bool_ascii_token(std::string_view token,
                                                 bool *out) {
  if (!out)
    return false;
  const std::string lowered =
      lowercase_copy(source_runtime_projection_detail::trim_ascii(token));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] inline bool collect_channel_states_for_record_type(
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    std::string_view record_type,
    std::vector<cuwacunu::hero::wave::source_runtime_channel_state_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "channel state output pointer is null";
    return false;
  }
  out->clear();

  const std::string target_record_type =
      lowercase_copy(source_runtime_projection_detail::trim_ascii(record_type));
  for (const auto &ch : observation.channel_forms) {
    const std::string row_record_type = lowercase_copy(
        source_runtime_projection_detail::trim_ascii(ch.record_type));
    if (row_record_type != target_record_type)
      continue;

    bool active = false;
    if (!parse_bool_ascii_token(ch.active, &active)) {
      if (error) {
        *error = "invalid channel active token for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) +
                 "'";
      }
      return false;
    }

    out->push_back(cuwacunu::hero::wave::source_runtime_channel_state_t{
        .interval =
            cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval),
        .active = active,
    });
  }

  if (out->empty()) {
    if (error) {
      *error =
          "no channel rows found for record_type '" + target_record_type + "'";
    }
    return false;
  }
  return true;
}

} // namespace source_runtime_projection_runtime_detail

[[nodiscard]] inline bool build_source_runtime_projection_fragment_for_wave(
    std::string_view record_type,
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    cuwacunu::hero::wave::source_runtime_projection_fragment_t *out,
    std::string *error) {
  using namespace source_runtime_projection_runtime_detail;

  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "source projection fragment output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::wave::source_runtime_projection_fragment_t{};

  if (wave.sources.size() != 1U) {
    if (error) {
      *error = "runtime requires exactly one SOURCE block for projection";
    }
    return false;
  }
  const auto &source_decl = wave.sources.front();
  const std::string symbol = source_runtime_projection_detail::trim_ascii(
      source_decl.runtime_instrument_signature.symbol);
  if (symbol.empty()) {
    if (error)
      *error = "SOURCE.RUNTIME.RUNTIME_INSTRUMENT_SIGNATURE.SYMBOL is empty";
    return false;
  }

  double domain_from_ms = 0.0;
  double domain_to_ms = 0.0;
  if (!resolve_effective_domain_bounds_for_record_type(
          record_type, source_decl.runtime_instrument_signature, observation,
          &domain_from_ms, &domain_to_ms, error)) {
    return false;
  }

  std::vector<cuwacunu::hero::wave::source_runtime_channel_state_t>
      channel_states{};
  if (!collect_channel_states_for_record_type(observation, record_type,
                                              &channel_states, error)) {
    return false;
  }

  cuwacunu::hero::wave::source_runtime_projection_input_t input{};
  input.symbol = symbol;
  input.from_date_ddmmyyyy = source_decl.from;
  input.to_date_ddmmyyyy = source_decl.to;
  input.domain_from_ms = domain_from_ms;
  input.domain_to_ms = domain_to_ms;
  input.channel_states = std::move(channel_states);
  return cuwacunu::hero::wave::build_source_runtime_projection_fragment(
      input, out, error);
}

} // namespace wave
} // namespace hero
} // namespace cuwacunu
