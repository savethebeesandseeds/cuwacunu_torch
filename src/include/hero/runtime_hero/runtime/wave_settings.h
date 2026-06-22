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
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hero/lattice_hero/lattice/runtime_report/component_runtime_lls.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/splits/source_split_catalog.h"

namespace cuwacunu::hero::runtime::settings {

enum class wave_source_range_policy_t {
  all,
  anchor_index,
  source_key,
  fraction_range,
};

enum class wave_source_order_policy_t {
  sequential,
  random_per_epoch,
};

enum class wave_target_t {
  vicreg_representation,
  mtf_jepa_mae_vicreg_representation,
  inference_channel_mdn,
  graph_node_allocation_policy,
};

enum class wave_action_t {
  run,
  train,
};

struct wave_settings_t {
  std::string wave_id{};
  std::string protocol_id{};
  std::string authored_target{};
  wave_target_t target{wave_target_t::inference_channel_mdn};
  std::string mode_text{"run"};
  wave_action_t action{wave_action_t::run};
  bool debug{false};
  std::string source_cursor_id{};
  std::string source_split_id{};
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"wave_batch"};
  wave_source_range_policy_t source_range_policy{
      wave_source_range_policy_t::all};
  wave_source_order_policy_t source_order_policy{
      wave_source_order_policy_t::sequential};
  bool source_order_policy_explicit{false};
  std::string job_kind{};
  std::string policy_id{};
  std::string policy_kind{};
  std::string training_schedule_mode{};
  std::string train_split_id{};
  std::string validation_split_id{};
  std::string test_split_id{};
  bool live_execution_allowed{false};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
  std::optional<cuwacunu::ujcamei::source::splits::source_split_t>
      source_split_intent{std::nullopt};
};

struct wave_protocol_bindings_t {
  std::string protocol_id{"cwu_01v"};
  std::string representation_family{"wikimyei.representation.encoding.vicreg"};
  std::string inference_family{"wikimyei.inference.expected_value.mdn"};
  std::string allocation_policy_family{
      "wikimyei.policy.portfolio.spot_distributional_utility"};
  std::string policy_component_family{
      "wikimyei.policy.portfolio.graph_node_allocation"};
};

struct source_cursor_settings_t {
  std::string cursor_id{};
  std::string source_split_id{};
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"wave_batch"};
  wave_source_range_policy_t source_range_policy{
      wave_source_range_policy_t::all};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
  std::optional<std::int64_t> source_key_begin{std::nullopt};
  std::optional<std::int64_t> source_key_end{std::nullopt};
  std::optional<cuwacunu::ujcamei::source::splits::source_split_t>
      source_split_intent{std::nullopt};
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

[[nodiscard]] inline bool
wave_supports_protocol(const wave_settings_t &settings,
                       const std::string &protocol_id) {
  return wave_detail::kv::trim(settings.protocol_id) ==
         wave_detail::kv::trim(protocol_id);
}

[[nodiscard]] inline wave_protocol_bindings_t default_wave_protocol_bindings() {
  return wave_protocol_bindings_t{};
}

[[nodiscard]] inline std::string
resolve_wave_target_family(std::string value,
                           const wave_protocol_bindings_t &protocol) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(value));
  if (value == "wikimyei.representation.encoding" ||
      value == "representation_encoding" || value == "representation") {
    return protocol.representation_family;
  }
  if (value == "wikimyei.inference.expected_value" ||
      value == "expected_value_inference" || value == "inference") {
    return protocol.inference_family;
  }
  if (value == "wikimyei.policy.portfolio" || value == "portfolio_policy" ||
      value == "policy_portfolio") {
    return protocol.policy_component_family;
  }
  return value;
}

[[nodiscard]] inline wave_target_t
parse_wave_target(std::string value, const wave_protocol_bindings_t &protocol =
                                         default_wave_protocol_bindings()) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(
      resolve_wave_target_family(std::move(value), protocol)));
  if (value == "wikimyei.representation.encoding.vicreg") {
    return wave_target_t::vicreg_representation;
  }
  if (value == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg") {
    return wave_target_t::mtf_jepa_mae_vicreg_representation;
  }
  if (value == "wikimyei.inference.expected_value.mdn") {
    return wave_target_t::inference_channel_mdn;
  }
  if (value == "wikimyei.policy.portfolio.graph_node_allocation") {
    return wave_target_t::graph_node_allocation_policy;
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
  case wave_target_t::graph_node_allocation_policy:
    return "wikimyei.policy.portfolio.graph_node_allocation";
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
  if (value == "fraction_range") {
    return wave_source_range_policy_t::fraction_range;
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
  case wave_source_range_policy_t::fraction_range:
    return "fraction_range";
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

inline void reject_retired_cursor_key(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const std::string &key) {
  if (block.values.find(key) != block.values.end()) {
    throw std::runtime_error("[wave_settings] " + key +
                             " is retired; use SOURCE_KEY_BEGIN/END");
  }
}

[[nodiscard]] inline bool
parse_optional_bool(const cuwacunu::piaabo::parse::simple_kv::block_t &block,
                    const std::string &key, bool default_value) {
  const auto raw = wave_detail::kv::optional(block, key, "");
  const auto value = wave_detail::kv::lowercase(wave_detail::kv::trim(raw));
  if (value.empty()) {
    return default_value;
  }
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error("[wave_settings] " + key + " must be true or false");
}

inline void
validate_source_cursor_settings(const source_cursor_settings_t &settings) {
  if (wave_detail::kv::trim(settings.cursor_id).empty()) {
    throw std::runtime_error("[wave_settings] CURSOR_ID is required");
  }
  if (settings.source_cursor_kind != "graph_anchor") {
    throw std::runtime_error(
        "[wave_settings] v1 source cursor KIND must be graph_anchor");
  }
  if (settings.source_cursor_scope != "wave_batch") {
    throw std::runtime_error(
        "[wave_settings] v1 source cursor SCOPE must be wave_batch");
  }
  if (settings.source_range_policy == wave_source_range_policy_t::all) {
    if (settings.anchor_index_begin.has_value() ||
        settings.anchor_index_end.has_value() ||
        settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error("[wave_settings] cursor range bounds require "
                               "SOURCE_RANGE=anchor_index "
                               "or SOURCE_RANGE=source_key");
    }
    return;
  }
  if (settings.source_range_policy ==
      wave_source_range_policy_t::anchor_index) {
    if (settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error("[wave_settings] cursor SOURCE_KEY_BEGIN/END "
                               "require SOURCE_RANGE=source_key");
    }
    if (!settings.anchor_index_begin.has_value() ||
        !settings.anchor_index_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] cursor SOURCE_RANGE=anchor_index requires "
          "ANCHOR_INDEX_BEGIN and ANCHOR_INDEX_END");
    }
    if (*settings.anchor_index_end <= *settings.anchor_index_begin) {
      throw std::runtime_error(
          "[wave_settings] cursor ANCHOR_INDEX_END must be greater than "
          "ANCHOR_INDEX_BEGIN");
    }
    return;
  }
  if (settings.source_range_policy ==
      wave_source_range_policy_t::fraction_range) {
    if (settings.anchor_index_begin.has_value() ||
        settings.anchor_index_end.has_value() ||
        settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] fraction_range cursor must not carry concrete "
          "source bounds");
    }
    if (settings.source_split_id.empty() ||
        !settings.source_split_intent.has_value()) {
      throw std::runtime_error(
          "[wave_settings] SOURCE_RANGE=fraction_range requires SOURCE_SPLIT");
    }
    return;
  }
  if (settings.anchor_index_begin.has_value() ||
      settings.anchor_index_end.has_value()) {
    throw std::runtime_error("[wave_settings] cursor ANCHOR_INDEX_BEGIN/END "
                             "require SOURCE_RANGE=anchor_index");
  }
  if (!settings.source_key_begin.has_value() ||
      !settings.source_key_end.has_value()) {
    throw std::runtime_error(
        "[wave_settings] cursor SOURCE_RANGE=source_key requires "
        "SOURCE_KEY_BEGIN and SOURCE_KEY_END");
  }
  if (*settings.source_key_end <= *settings.source_key_begin) {
    throw std::runtime_error(
        "[wave_settings] cursor SOURCE_KEY_END must be greater than "
        "SOURCE_KEY_BEGIN");
  }
}

[[nodiscard]] inline source_cursor_settings_t
decode_source_cursor_settings_from_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  if (block.name != "UJCAMEI_SOURCE_CURSOR") {
    throw std::runtime_error(
        "[wave_settings] expected UJCAMEI_SOURCE_CURSOR block");
  }
  source_cursor_settings_t out{};
  out.cursor_id = kv::required(block, "CURSOR_ID");
  out.source_cursor_kind = kv::required(block, "SOURCE_CURSOR_KIND");
  out.source_cursor_scope = kv::required(block, "SOURCE_CURSOR_SCOPE");
  out.source_range_policy =
      parse_source_range_policy(kv::required(block, "SOURCE_RANGE"));
  reject_retired_cursor_key(block, "ANCHOR_KEY_BEGIN");
  reject_retired_cursor_key(block, "ANCHOR_KEY_END");
  out.anchor_index_begin = parse_optional_size(block, "ANCHOR_INDEX_BEGIN");
  out.anchor_index_end = parse_optional_size(block, "ANCHOR_INDEX_END");
  out.source_key_begin = parse_optional_i64(block, "SOURCE_KEY_BEGIN");
  out.source_key_end = parse_optional_i64(block, "SOURCE_KEY_END");
  validate_source_cursor_settings(out);
  return out;
}

[[nodiscard]] inline source_cursor_settings_t
source_cursor_settings_from_dsl(const std::string &dsl_text,
                                std::string cursor_id) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  cursor_id = kv::trim(std::move(cursor_id));
  if (cursor_id.empty()) {
    throw std::runtime_error("[wave_settings] SOURCE_CURSOR_ID is required");
  }
  const auto blocks = kv::parse_blocks(dsl_text);
  std::unordered_map<std::string, source_cursor_settings_t> cursors;
  for (const auto &block : blocks) {
    if (block.name != "UJCAMEI_SOURCE_CURSOR") {
      continue;
    }
    auto cursor = decode_source_cursor_settings_from_block(block);
    if (!cursors.emplace(cursor.cursor_id, cursor).second) {
      throw std::runtime_error("[wave_settings] duplicate CURSOR_ID: " +
                               cursor.cursor_id);
    }
  }
  const auto found = cursors.find(cursor_id);
  if (found == cursors.end()) {
    throw std::runtime_error("[wave_settings] SOURCE_CURSOR_ID not found: " +
                             cursor_id);
  }
  return found->second;
}

[[nodiscard]] inline std::string
source_cursor_id_from_split_id(const std::string &split_id) {
  const auto trimmed = wave_detail::kv::trim(split_id);
  if (trimmed.empty()) {
    throw std::runtime_error("[wave_settings] SOURCE_SPLIT is required");
  }
  return "split:" + trimmed;
}

[[nodiscard]] inline source_cursor_settings_t
source_cursor_settings_from_split_dsl(const std::string &split_dsl_text,
                                      std::string split_id) {
  namespace source_split = cuwacunu::ujcamei::source::splits;
  split_id = wave_detail::kv::trim(std::move(split_id));
  if (split_id.empty()) {
    throw std::runtime_error("[wave_settings] SOURCE_SPLIT is required");
  }
  if (wave_detail::kv::trim(split_dsl_text).empty()) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_SPLIT requires an Ujcamei source split "
        "catalog");
  }
  const auto split_catalog =
      source_split::decode_source_split_catalog_from_dsl(split_dsl_text);
  const auto *split = split_catalog.find_split(split_id);
  if (split == nullptr) {
    throw std::runtime_error("[wave_settings] SOURCE_SPLIT not found: " +
                             split_id);
  }
  if (split->anchor_range.begin < 0 || split->anchor_range.end < 0) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_SPLIT anchor range must be non-negative");
  }
  source_cursor_settings_t out{};
  out.cursor_id = source_cursor_id_from_split_id(split_id);
  out.source_split_id = split_id;
  out.source_cursor_kind = "graph_anchor";
  out.source_cursor_scope = "wave_batch";
  out.source_split_intent = *split;
  if (split->selector_kind ==
      source_split::source_split_selector_kind_t::anchor_index_range) {
    if (split->anchor_range.begin < 0 || split->anchor_range.end < 0) {
      throw std::runtime_error(
          "[wave_settings] SOURCE_SPLIT anchor range must be non-negative");
    }
    out.source_range_policy = wave_source_range_policy_t::anchor_index;
    out.anchor_index_begin =
        static_cast<std::size_t>(split->anchor_range.begin);
    out.anchor_index_end = static_cast<std::size_t>(split->anchor_range.end);
  } else {
    out.source_range_policy = wave_source_range_policy_t::fraction_range;
  }
  validate_source_cursor_settings(out);
  return out;
}

[[nodiscard]] inline const char *runtime_report_mode_name(
    cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t mode) {
  using mode_t = cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t;
  switch (mode) {
  case mode_t::normal:
    return "normal";
  case mode_t::debug:
    return "debug";
  }
  throw std::runtime_error("[wave_settings] unknown runtime report mode");
}

[[nodiscard]] inline cuwacunu::hero::lattice::runtime_report::
    runtime_report_mode_t
    runtime_report_mode_from_wave(const wave_settings_t &settings) {
  using mode_t = cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t;
  return settings.debug ? mode_t::debug : mode_t::normal;
}

inline void validate_wave_settings(const wave_settings_t &settings) {
  if (wave_detail::kv::trim(settings.wave_id).empty()) {
    throw std::runtime_error("[wave_settings] WAVE_ID is required");
  }
  if (wave_detail::kv::trim(settings.protocol_id).empty()) {
    throw std::runtime_error("[wave_settings] PROTOCOL is required");
  }
  if (wave_detail::kv::trim(settings.authored_target).empty()) {
    throw std::runtime_error("[wave_settings] TARGET is required");
  }
  if (wave_detail::kv::trim(settings.source_cursor_id).empty()) {
    throw std::runtime_error("[wave_settings] SOURCE_CURSOR_ID is required");
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

  if (settings.source_range_policy ==
      wave_source_range_policy_t::fraction_range) {
    if (settings.anchor_index_begin.has_value() ||
        settings.anchor_index_end.has_value() ||
        settings.source_key_begin.has_value() ||
        settings.source_key_end.has_value()) {
      throw std::runtime_error(
          "[wave_settings] SOURCE_RANGE=fraction_range must not carry concrete "
          "source bounds before materialization");
    }
    if (settings.source_split_id.empty() ||
        !settings.source_split_intent.has_value()) {
      throw std::runtime_error(
          "[wave_settings] SOURCE_RANGE=fraction_range requires SOURCE_SPLIT");
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

  if (settings.source_range_policy ==
      wave_source_range_policy_t::fraction_range) {
    if (!settings.source_split_intent.has_value()) {
      throw std::runtime_error(
          "[wave_settings] fraction_range wave range is missing SOURCE_SPLIT "
          "intent");
    }
    if (anchor_keys.size() >
        static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
      throw std::runtime_error(
          "[wave_settings] accepted graph-anchor domain exceeds int64 range");
    }
    const auto materialized =
        cuwacunu::ujcamei::source::splits::materialize_source_split_range(
            *settings.source_split_intent,
            static_cast<std::int64_t>(anchor_keys.size()));
    if (materialized.begin < 0 || materialized.end < 0) {
      throw std::runtime_error(
          "[wave_settings] fraction_range materialized negative bounds");
    }
    if (static_cast<std::size_t>(materialized.end) > anchor_keys.size()) {
      throw std::runtime_error(
          "[wave_settings] fraction_range materialized beyond accepted graph "
          "anchor domain");
    }
    return resolved_source_range_t{
        .anchor_index_begin = static_cast<std::size_t>(materialized.begin),
        .anchor_index_end = static_cast<std::size_t>(materialized.end),
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

[[nodiscard]] inline wave_settings_t decode_wave_settings_from_block(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const wave_protocol_bindings_t &protocol = default_wave_protocol_bindings(),
    const std::string &source_cursor_dsl_text = {},
    const std::string &split_dsl_text = {}) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  wave_settings_t out{};
  out.wave_id = kv::required(block, "WAVE_ID");
  out.protocol_id = kv::required(block, "PROTOCOL");
  if (kv::trim(out.protocol_id) != kv::trim(protocol.protocol_id)) {
    throw std::runtime_error(
        "[wave_settings] WAVE_SETTINGS.PROTOCOL " + out.protocol_id +
        " does not match active protocol " + protocol.protocol_id);
  }
  out.authored_target = kv::required(block, "TARGET");
  out.target = parse_wave_target(out.authored_target, protocol);
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
  const std::string authored_source_cursor_id =
      kv::optional(block, "SOURCE_CURSOR_ID", "");
  const std::string authored_source_split_id =
      kv::optional(block, "SOURCE_SPLIT", "");
  out.source_split_id = kv::trim(authored_source_split_id);
  if (!out.source_split_id.empty() &&
      !kv::trim(authored_source_cursor_id).empty()) {
    throw std::runtime_error(
        "[wave_settings] SOURCE_SPLIT derives source cursor identity; omit "
        "SOURCE_CURSOR_ID");
  }
  out.source_cursor_id =
      out.source_split_id.empty()
          ? kv::required(block, "SOURCE_CURSOR_ID")
          : source_cursor_id_from_split_id(out.source_split_id);
  const auto source_order_it = block.values.find("SOURCE_ORDER");
  out.source_order_policy_explicit = source_order_it != block.values.end() &&
                                     !kv::trim(source_order_it->second).empty();
  out.source_order_policy =
      out.source_order_policy_explicit
          ? parse_source_order_policy(source_order_it->second)
          : (out.action == wave_action_t::train
                 ? wave_source_order_policy_t::random_per_epoch
                 : wave_source_order_policy_t::sequential);
  const auto cursor = out.source_split_id.empty()
                          ? source_cursor_settings_from_dsl(
                                source_cursor_dsl_text, out.source_cursor_id)
                          : source_cursor_settings_from_split_dsl(
                                split_dsl_text, out.source_split_id);
  out.source_cursor_kind = cursor.source_cursor_kind;
  out.source_cursor_scope = cursor.source_cursor_scope;
  out.source_range_policy = cursor.source_range_policy;
  out.anchor_index_begin = cursor.anchor_index_begin;
  out.anchor_index_end = cursor.anchor_index_end;
  out.source_key_begin = cursor.source_key_begin;
  out.source_key_end = cursor.source_key_end;
  out.source_split_intent = cursor.source_split_intent;
  out.job_kind = kv::optional(block, "JOB_KIND", "");
  out.policy_id = kv::optional(block, "POLICY_ID", "");
  out.policy_kind = kv::optional(block, "POLICY_KIND", "");
  out.training_schedule_mode =
      kv::optional(block, "TRAINING_SCHEDULE_MODE", "");
  out.train_split_id = kv::optional(block, "TRAIN_SPLIT", "");
  out.validation_split_id = kv::optional(block, "VALIDATION_SPLIT", "");
  out.test_split_id = kv::optional(block, "TEST_SPLIT", "");
  out.live_execution_allowed =
      parse_optional_bool(block, "LIVE_EXECUTION_ALLOWED", false);
  validate_wave_settings(out);
  return out;
}

[[nodiscard]] inline wave_settings_t decode_wave_settings_from_dsl(
    const std::string &dsl_text, std::string selected_wave_id = {},
    const wave_protocol_bindings_t &protocol = default_wave_protocol_bindings(),
    const std::string &source_cursor_dsl_text = {},
    const std::string &split_dsl_text = {});

[[nodiscard]] inline cuwacunu::piaabo::parse::simple_kv::block_t
selected_wave_settings_block_from_dsl(const std::string &dsl_text,
                                      std::string selected_wave_id = {}) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  auto blocks = kv::parse_blocks(dsl_text);
  std::vector<std::pair<std::string, kv::block_t>> waves{};
  std::unordered_set<std::string> wave_ids{};
  for (const auto &block : blocks) {
    if (block.name == "WAVE_SELECTION") {
      throw std::runtime_error(
          "[wave_settings] WAVE_SELECTION is retired; select the active wave "
          "with config runtime_wave_id");
    }
    if (block.name != "WAVE_SETTINGS") {
      continue;
    }
    auto wave_id = kv::required(block, "WAVE_ID");
    if (!wave_ids.insert(wave_id).second) {
      throw std::runtime_error("[wave_settings] duplicate WAVE_ID: " + wave_id);
    }
    waves.push_back(std::make_pair(std::move(wave_id), block));
  }
  if (waves.empty()) {
    throw std::runtime_error("[wave_settings] missing WAVE_SETTINGS");
  }

  selected_wave_id = kv::trim(selected_wave_id);
  if (selected_wave_id.empty()) {
    if (waves.size() == 1) {
      return waves.front().second;
    }
    throw std::runtime_error(
        "[wave_settings] multiple WAVE_SETTINGS blocks require "
        "config runtime_wave_id");
  }

  for (const auto &wave : waves) {
    if (wave.first == selected_wave_id) {
      return wave.second;
    }
  }
  throw std::runtime_error("[wave_settings] selected WAVE_ID not found: " +
                           selected_wave_id);
}

[[nodiscard]] inline wave_settings_t
decode_wave_settings_from_dsl(const std::string &dsl_text,
                              std::string selected_wave_id,
                              const wave_protocol_bindings_t &protocol,
                              const std::string &source_cursor_dsl_text,
                              const std::string &split_dsl_text) {
  return decode_wave_settings_from_block(
      selected_wave_settings_block_from_dsl(dsl_text,
                                            std::move(selected_wave_id)),
      protocol, source_cursor_dsl_text, split_dsl_text);
}

} // namespace cuwacunu::hero::runtime::settings
