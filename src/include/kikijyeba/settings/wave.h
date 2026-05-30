// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::settings {

enum class wave_source_range_policy_t {
  all,
  anchor_index,
  source_key,
};

enum class wave_source_order_policy_t {
  sequential,
  random_per_epoch,
};

enum class wave_target_t {
  vicreg_representation,
  mtf_jepa_mae_vicreg_representation,
  inference_channel_mdn,
};

enum class wave_action_t {
  run,
  train,
};

struct wave_settings_t {
  std::string wave_id{};
  std::vector<std::string> compatible_protocol_ids{};
  wave_target_t target{wave_target_t::inference_channel_mdn};
  std::string mode_text{"run"};
  wave_action_t action{wave_action_t::run};
  bool debug{false};
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"wave_batch"};
  wave_source_range_policy_t source_range_policy{
      wave_source_range_policy_t::all};
  wave_source_order_policy_t source_order_policy{
      wave_source_order_policy_t::sequential};
  bool source_order_policy_explicit{false};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
};

namespace wave_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::vector<std::string>
split_mode_atoms(std::string mode_text) {
  for (char &ch : mode_text) {
    if (ch == '|' || ch == '+' || ch == ',') {
      ch = ' ';
    }
  }
  std::istringstream in(mode_text);
  std::vector<std::string> atoms;
  std::string atom;
  while (in >> atom) {
    atoms.push_back(kv::lowercase(kv::trim(atom)));
  }
  return atoms;
}

} // namespace wave_detail

[[nodiscard]] inline std::vector<std::string>
parse_protocol_id_list(std::string value) {
  for (char &ch : value) {
    if (ch == '|' || ch == '+' || ch == ',') {
      ch = ' ';
    }
  }
  std::istringstream in(value);
  std::vector<std::string> out;
  std::string item;
  while (in >> item) {
    item = wave_detail::kv::trim(item);
    if (item.empty()) {
      continue;
    }
    if (std::find(out.begin(), out.end(), item) == out.end()) {
      out.push_back(std::move(item));
    }
  }
  return out;
}

[[nodiscard]] inline bool
wave_supports_protocol(const wave_settings_t &settings,
                       const std::string &protocol_id) {
  if (settings.compatible_protocol_ids.empty()) {
    return true;
  }
  return std::find(settings.compatible_protocol_ids.begin(),
                   settings.compatible_protocol_ids.end(),
                   wave_detail::kv::trim(protocol_id)) !=
         settings.compatible_protocol_ids.end();
}

[[nodiscard]] inline wave_target_t parse_wave_target(std::string value) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(value));
  if (value == "wikimyei.representation.encoding.vicreg" ||
      value == "vicreg_representation") {
    return wave_target_t::vicreg_representation;
  }
  if (value == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" ||
      value == "mtf_jepa_mae_vicreg_representation" ||
      value == "mtf_jvmae_representation") {
    return wave_target_t::mtf_jepa_mae_vicreg_representation;
  }
  if (value == "wikimyei.inference.expected_value.mdn" ||
      value == "inference_mdn" || value == "inference_channel_mdn" ||
      value == "mdn_expected_value_inference") {
    return wave_target_t::inference_channel_mdn;
  }
  throw std::runtime_error("[wave_settings] invalid TARGET: " + value);
}

[[nodiscard]] inline const char *wave_target_name(wave_target_t target) {
  switch (target) {
  case wave_target_t::vicreg_representation:
    return "wikimyei.representation.encoding.vicreg";
  case wave_target_t::mtf_jepa_mae_vicreg_representation:
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  case wave_target_t::inference_channel_mdn:
    return "wikimyei.inference.expected_value.mdn";
  }
  throw std::runtime_error("[wave_settings] unknown TARGET");
}

[[nodiscard]] inline const char *wave_action_name(wave_action_t action) {
  switch (action) {
  case wave_action_t::run:
    return "run";
  case wave_action_t::train:
    return "train";
  }
  throw std::runtime_error("[wave_settings] unknown MODE action");
}

[[nodiscard]] inline wave_source_range_policy_t
parse_source_range_policy(std::string value) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(value));
  if (value == "all") {
    return wave_source_range_policy_t::all;
  }
  if (value == "anchor_index") {
    return wave_source_range_policy_t::anchor_index;
  }
  if (value == "source_key" || value == "anchor_key") {
    return wave_source_range_policy_t::source_key;
  }
  throw std::runtime_error("[wave_settings] invalid SOURCE_RANGE: " + value);
}

[[nodiscard]] inline const char *
source_range_policy_name(wave_source_range_policy_t policy) {
  switch (policy) {
  case wave_source_range_policy_t::all:
    return "all";
  case wave_source_range_policy_t::anchor_index:
    return "anchor_index";
  case wave_source_range_policy_t::source_key:
    return "source_key";
  }
  throw std::runtime_error("[wave_settings] unknown SOURCE_RANGE policy");
}

[[nodiscard]] inline wave_source_order_policy_t
parse_source_order_policy(std::string value) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(value));
  if (value == "sequential") {
    return wave_source_order_policy_t::sequential;
  }
  if (value == "random_per_epoch") {
    return wave_source_order_policy_t::random_per_epoch;
  }
  throw std::runtime_error("[wave_settings] invalid SOURCE_ORDER: " + value);
}

[[nodiscard]] inline const char *
source_order_policy_name(wave_source_order_policy_t policy) {
  switch (policy) {
  case wave_source_order_policy_t::sequential:
    return "sequential";
  case wave_source_order_policy_t::random_per_epoch:
    return "random_per_epoch";
  }
  throw std::runtime_error("[wave_settings] unknown SOURCE_ORDER policy");
}

[[nodiscard]] inline bool
train_wave_uses_sequential_source_order(const wave_settings_t &settings) {
  return settings.action == wave_action_t::train &&
         settings.source_order_policy == wave_source_order_policy_t::sequential;
}

[[nodiscard]] inline const char *
source_order_warning_token(const wave_settings_t &settings) {
  if (train_wave_uses_sequential_source_order(settings)) {
    return settings.source_order_policy_explicit
               ? "train_wave_explicit_sequential_source_order"
               : "train_wave_effective_sequential_source_order";
  }
  return "";
}

[[nodiscard]] inline std::optional<std::size_t>
parse_optional_size(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                    const std::string &key) {
  const auto raw = wave_detail::kv::optional(block, key, "");
  if (wave_detail::kv::trim(raw).empty()) {
    return std::nullopt;
  }
  const auto value = wave_detail::kv::parse_i64(raw);
  if (value < 0) {
    throw std::runtime_error("[wave_settings] " + key +
                             " must be non-negative");
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] inline std::optional<std::int64_t>
parse_optional_i64(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                   const std::string &key) {
  const auto raw = wave_detail::kv::optional(block, key, "");
  if (wave_detail::kv::trim(raw).empty()) {
    return std::nullopt;
  }
  return wave_detail::kv::parse_i64(raw);
}

[[nodiscard]] inline std::optional<std::int64_t> parse_optional_i64_alias(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const std::string &primary_key, const std::string &alias_key) {
  const auto primary = parse_optional_i64(block, primary_key);
  const auto alias = parse_optional_i64(block, alias_key);
  if (primary.has_value() && alias.has_value() && *primary != *alias) {
    throw std::runtime_error("[wave_settings] " + primary_key + " and " +
                             alias_key + " disagree");
  }
  return primary.has_value() ? primary : alias;
}

[[nodiscard]] inline const char *runtime_report_mode_name(
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t mode) {
  using mode_t =
      cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t;
  switch (mode) {
  case mode_t::normal:
    return "normal";
  case mode_t::debug:
    return "debug";
  }
  throw std::runtime_error("[wave_settings] unknown runtime report mode");
}

[[nodiscard]] inline cuwacunu::kikijyeba::lattice::runtime_report::
    runtime_report_mode_t
    runtime_report_mode_from_wave(const wave_settings_t &settings) {
  using mode_t =
      cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t;
  return settings.debug ? mode_t::debug : mode_t::normal;
}

inline void validate_wave_settings(const wave_settings_t &settings) {
  if (wave_detail::kv::trim(settings.wave_id).empty()) {
    throw std::runtime_error("[wave_settings] WAVE_ID is required");
  }
  for (const auto &protocol_id : settings.compatible_protocol_ids) {
    if (wave_detail::kv::trim(protocol_id).empty() ||
        protocol_id != wave_detail::kv::trim(protocol_id)) {
      throw std::runtime_error(
          "[wave_settings] COMPATIBLE_PROTOCOLS entries must be trimmed and "
          "non-empty");
    }
  }
  if (settings.source_cursor_kind != "graph_anchor") {
    throw std::runtime_error(
        "[wave_settings] v1 SOURCE_CURSOR_KIND must be graph_anchor");
  }
  if (settings.source_cursor_scope != "wave_batch") {
    throw std::runtime_error(
        "[wave_settings] v1 SOURCE_CURSOR_SCOPE must be wave_batch");
  }
  if (settings.source_range_policy == wave_source_range_policy_t::all) {
    if (settings.anchor_index_begin.has_value() ||
        settings.anchor_index_end.has_value() ||
        settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] range bounds require SOURCE_RANGE=anchor_index or "
          "SOURCE_RANGE=source_key");
    }
    return;
  }

  if (settings.source_range_policy ==
      wave_source_range_policy_t::anchor_index) {
    if (settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error("[wave_settings] SOURCE_KEY_BEGIN/END require "
                               "SOURCE_RANGE=source_key");
    }
    if (!settings.anchor_index_begin.has_value() ||
        !settings.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] SOURCE_RANGE=anchor_index requires "
          "ANCHOR_INDEX_BEGIN and ANCHOR_INDEX_END");
    }
    if (*settings.anchor_index_end <= *settings.anchor_index_begin) {
      throw std::runtime_error(
          "[wave_settings] ANCHOR_INDEX_END must be greater than "
          "ANCHOR_INDEX_BEGIN");
    }
    return;
  }

  if (settings.anchor_index_begin.has_value() ||
      settings.anchor_index_end.has_value()) {
    throw std::runtime_error("[wave_settings] ANCHOR_INDEX_BEGIN/END require "
                             "SOURCE_RANGE=anchor_index");
  }
  if (!settings.source_key_begin.has_value() ||
      !settings.source_key_end.has_value()) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_RANGE=source_key requires SOURCE_KEY_BEGIN "
        "and SOURCE_KEY_END");
  }
  if (*settings.source_key_end <= *settings.source_key_begin) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_KEY_END must be greater than "
        "SOURCE_KEY_BEGIN");
  }
}

struct resolved_source_range_t {
  std::size_t anchor_index_begin{0};
  std::size_t anchor_index_end{0};
};

template <typename KeyT>
[[nodiscard]] inline KeyT checked_source_key_cast(std::int64_t value,
                                                  const std::string &label) {
  static_assert(std::is_integral_v<KeyT>,
                "source-key wave ranges require integral graph-anchor keys");
  if constexpr (!std::is_signed_v<KeyT>) {
    if (value < 0) {
      throw std::runtime_error("[wave_settings] " + label +
                               " is negative but graph-anchor keys are "
                               "unsigned");
    }
    if (static_cast<unsigned long long>(value) >
        static_cast<unsigned long long>(std::numeric_limits<KeyT>::max())) {
      throw std::runtime_error("[wave_settings] " + label +
                               " is outside graph-anchor key type range");
    }
  } else {
    const auto min_value =
        static_cast<std::int64_t>(std::numeric_limits<KeyT>::lowest());
    const auto max_value =
        static_cast<std::int64_t>(std::numeric_limits<KeyT>::max());
    if (value < min_value || value > max_value) {
      throw std::runtime_error("[wave_settings] " + label +
                               " is outside graph-anchor key type range");
    }
  }
  return static_cast<KeyT>(value);
}

template <typename KeyT>
[[nodiscard]] inline std::int64_t source_key_add_step_checked(KeyT value,
                                                              KeyT step) {
  const auto value_i = static_cast<std::int64_t>(value);
  const auto step_i = static_cast<std::int64_t>(step);
  if (step_i <= 0) {
    throw std::runtime_error(
        "[wave_settings] graph-anchor key step must be positive for "
        "source_key range resolution");
  }
  if (value_i > std::numeric_limits<std::int64_t>::max() - step_i) {
    throw std::runtime_error(
        "[wave_settings] graph-anchor key exclusive upper bound overflows");
  }
  return value_i + step_i;
}

template <typename CursorReportT>
[[nodiscard]] inline resolved_source_range_t
resolve_source_range_to_anchor_indices(const wave_settings_t &settings,
                                       const CursorReportT &cursor_report) {
  using key_t =
      typename std::decay_t<decltype(cursor_report.anchor_keys)>::value_type;
  const auto &anchor_keys = cursor_report.anchor_keys;
  if (anchor_keys.empty()) {
    throw std::runtime_error(
        "[wave_settings] cannot resolve wave range over empty graph-anchor "
        "domain");
  }
  if (!std::is_sorted(anchor_keys.begin(), anchor_keys.end())) {
    throw std::runtime_error(
        "[wave_settings] graph-anchor keys must be sorted for source_key "
        "range resolution");
  }
  if (std::adjacent_find(anchor_keys.begin(), anchor_keys.end()) !=
      anchor_keys.end()) {
    throw std::runtime_error(
        "[wave_settings] graph-anchor keys must be unique for source_key "
        "range resolution");
  }

  if (settings.source_range_policy == wave_source_range_policy_t::all) {
    return resolved_source_range_t{.anchor_index_begin = 0,
                                   .anchor_index_end = anchor_keys.size()};
  }

  if (settings.source_range_policy ==
      wave_source_range_policy_t::anchor_index) {
    if (!settings.anchor_index_begin.has_value() ||
        !settings.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] anchor_index wave range is missing bounds");
    }
    if (*settings.anchor_index_begin > *settings.anchor_index_end) {
      throw std::runtime_error(
          "[wave_settings] anchor_index wave range begin exceeds end");
    }
    if (*settings.anchor_index_end > anchor_keys.size()) {
      throw std::runtime_error(
          "[wave_settings] anchor_index wave range exceeds accepted graph "
          "anchor domain");
    }
    return resolved_source_range_t{
        .anchor_index_begin = *settings.anchor_index_begin,
        .anchor_index_end = *settings.anchor_index_end,
    };
  }

  if (!cursor_report.reference_key_step.has_value()) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_RANGE=source_key requires reference key step");
  }
  if (!settings.source_key_begin.has_value() ||
      !settings.source_key_end.has_value()) {
    throw std::runtime_error(
        "[wave_settings] source_key wave range is missing bounds");
  }
  const key_t begin_key = checked_source_key_cast<key_t>(
      *settings.source_key_begin, "SOURCE_KEY_BEGIN");
  const key_t end_key = checked_source_key_cast<key_t>(*settings.source_key_end,
                                                       "SOURCE_KEY_END");
  const auto first_key = anchor_keys.front();
  const auto last_key = anchor_keys.back();
  const auto last_exclusive =
      source_key_add_step_checked(last_key, *cursor_report.reference_key_step);
  if (begin_key < first_key) {
    throw std::runtime_error(
        "[wave_settings] source_key wave range begins before accepted graph "
        "anchor domain");
  }
  if (static_cast<std::int64_t>(end_key) > last_exclusive) {
    throw std::runtime_error(
        "[wave_settings] source_key wave range ends after accepted graph "
        "anchor domain");
  }

  const auto begin_it =
      std::lower_bound(anchor_keys.begin(), anchor_keys.end(), begin_key);
  const auto end_it =
      std::lower_bound(anchor_keys.begin(), anchor_keys.end(), end_key);
  const auto begin_index =
      static_cast<std::size_t>(std::distance(anchor_keys.begin(), begin_it));
  const auto end_index =
      static_cast<std::size_t>(std::distance(anchor_keys.begin(), end_it));
  if (begin_index >= end_index) {
    throw std::runtime_error(
        "[wave_settings] source_key wave range resolves to no accepted graph "
        "anchors");
  }
  return resolved_source_range_t{.anchor_index_begin = begin_index,
                                 .anchor_index_end = end_index};
}

[[nodiscard]] inline wave_settings_t
decode_wave_settings_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "WAVE_SETTINGS");
  wave_settings_t out{};
  out.wave_id = kv::required(block, "WAVE_ID");
  out.compatible_protocol_ids =
      parse_protocol_id_list(kv::optional(block, "COMPATIBLE_PROTOCOLS", ""));
  out.target = parse_wave_target(kv::required(block, "TARGET"));
  out.mode_text = kv::optional(block, "MODE", "run");
  bool saw_run = false;
  bool saw_train = false;
  out.debug = false;
  for (const auto &atom : wave_detail::split_mode_atoms(out.mode_text)) {
    if (atom == "run") {
      saw_run = true;
    } else if (atom == "train") {
      saw_train = true;
    } else if (atom == "debug") {
      out.debug = true;
    } else {
      throw std::runtime_error("[wave_settings] invalid MODE atom: " + atom);
    }
  }
  if (saw_run == saw_train) {
    throw std::runtime_error(
        "[wave_settings] MODE must include exactly one primary action: run or "
        "train; debug is only a modifier");
  }
  out.action = saw_train ? wave_action_t::train : wave_action_t::run;
  out.source_cursor_kind =
      kv::optional(block, "SOURCE_CURSOR_KIND", out.source_cursor_kind);
  out.source_cursor_scope =
      kv::optional(block, "SOURCE_CURSOR_SCOPE", out.source_cursor_scope);
  out.source_range_policy =
      parse_source_range_policy(kv::optional(block, "SOURCE_RANGE", "all"));
  const auto source_order_it = block.values.find("SOURCE_ORDER");
  out.source_order_policy_explicit = source_order_it != block.values.end() &&
                                     !kv::trim(source_order_it->second).empty();
  out.source_order_policy =
      out.source_order_policy_explicit
          ? parse_source_order_policy(source_order_it->second)
          : (out.action == wave_action_t::train
                 ? wave_source_order_policy_t::random_per_epoch
                 : wave_source_order_policy_t::sequential);
  out.anchor_index_begin = parse_optional_size(block, "ANCHOR_INDEX_BEGIN");
  out.anchor_index_end = parse_optional_size(block, "ANCHOR_INDEX_END");
  out.source_key_begin =
      parse_optional_i64_alias(block, "SOURCE_KEY_BEGIN", "ANCHOR_KEY_BEGIN");
  out.source_key_end =
      parse_optional_i64_alias(block, "SOURCE_KEY_END", "ANCHOR_KEY_END");
  validate_wave_settings(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::settings
