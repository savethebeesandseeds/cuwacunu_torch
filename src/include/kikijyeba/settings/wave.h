// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::settings {

enum class wave_source_range_policy_t {
  all,
  anchor_index,
};

enum class wave_target_t {
  representation_vicreg,
  inference_mdn,
};

enum class wave_action_t {
  run,
  train,
};

struct wave_settings_t {
  std::string wave_id{};
  wave_target_t target{wave_target_t::inference_mdn};
  std::string mode_text{"run"};
  wave_action_t action{wave_action_t::run};
  bool debug{false};
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"wave_batch"};
  wave_source_range_policy_t source_range_policy{
      wave_source_range_policy_t::all};
  std::optional<std::size_t> anchor_index_begin{std::nullopt};
  std::optional<std::size_t> anchor_index_end{std::nullopt};
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

[[nodiscard]] inline wave_target_t parse_wave_target(std::string value) {
  value = wave_detail::kv::lowercase(wave_detail::kv::trim(value));
  if (value == "wikimyei.representation.encoding.vicreg" ||
      value == "representation_vicreg") {
    return wave_target_t::representation_vicreg;
  }
  if (value == "wikimyei.inference.expected_value.mdn" ||
      value == "inference_mdn") {
    return wave_target_t::inference_mdn;
  }
  throw std::runtime_error("[wave_settings] invalid TARGET: " + value);
}

[[nodiscard]] inline const char *wave_target_name(wave_target_t target) {
  switch (target) {
  case wave_target_t::representation_vicreg:
    return "wikimyei.representation.encoding.vicreg";
  case wave_target_t::inference_mdn:
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
  throw std::runtime_error("[wave_settings] invalid SOURCE_RANGE: " + value);
}

[[nodiscard]] inline const char *
source_range_policy_name(wave_source_range_policy_t policy) {
  switch (policy) {
  case wave_source_range_policy_t::all:
    return "all";
  case wave_source_range_policy_t::anchor_index:
    return "anchor_index";
  }
  throw std::runtime_error("[wave_settings] unknown SOURCE_RANGE policy");
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
        settings.anchor_index_end.has_value()) {
      throw std::runtime_error("[wave_settings] ANCHOR_INDEX_BEGIN/END require "
                               "SOURCE_RANGE=anchor_index");
    }
    return;
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
}

[[nodiscard]] inline wave_settings_t
decode_wave_settings_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "WAVE_SETTINGS");
  wave_settings_t out{};
  out.wave_id = kv::required(block, "WAVE_ID");
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
  out.anchor_index_begin = parse_optional_size(block, "ANCHOR_INDEX_BEGIN");
  out.anchor_index_end = parse_optional_size(block, "ANCHOR_INDEX_END");
  validate_wave_settings(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::settings
