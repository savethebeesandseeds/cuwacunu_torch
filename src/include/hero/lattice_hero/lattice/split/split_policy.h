// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/splits/source_split_catalog.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::hero::lattice::split {

struct lattice_split_t {
  std::string split_id{};
  cuwacunu::hero::lattice::exposure::exposure_split_role_t role{
      cuwacunu::hero::lattice::exposure::exposure_split_role_t::unknown};
  cuwacunu::hero::lattice::exposure::anchor_interval_t anchor_range{};
  cuwacunu::ujcamei::source::splits::source_split_t source_split_intent{};
  bool anchor_range_materialized{false};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t> allow_uses{};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t>
      protect_from_uses{};
  bool protect_requires_mutated_component{true};
};

struct lattice_split_protection_t {
  std::string split_id{};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t> allow_uses{};
  std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t>
      protect_from_uses{};
  bool protect_requires_mutated_component{true};
};

struct lattice_split_policy_overlay_t {
  std::string schema{"kikijyeba.lattice.split_policy.v1"};
  std::string policy_id{};
  std::string source_split_catalog_id{};
  std::string purge_left_context{"auto_from_Hx"};
  std::string purge_right_future{"auto_from_Hf"};
  std::vector<lattice_split_protection_t> protections{};
};

struct lattice_split_policy_t {
  std::string schema{"kikijyeba.lattice.split_policy.v1"};
  std::string policy_id{};
  std::string source_split_catalog_id{};
  std::string source_split_catalog_fingerprint{};
  std::string cursor_domain{"ujcamei.graph_anchor"};
  std::string purge_left_context{"auto_from_Hx"};
  std::string purge_right_future{"auto_from_Hf"};
  std::vector<lattice_split_t> splits{};

  [[nodiscard]] const lattice_split_t *
  find_split(const std::string &split_id) const {
    for (const auto &split : splits) {
      if (split.split_id == split_id) {
        return &split;
      }
    }
    return nullptr;
  }
};

namespace split_policy_detail {

namespace fs = std::filesystem;
namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string read_text_file_or_throw(const fs::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[lattice_split] unable to open file: " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

} // namespace split_policy_detail

[[nodiscard]] inline std::vector<
    cuwacunu::hero::lattice::exposure::exposure_use_t>
parse_optional_exposure_uses(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const std::string &key) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto raw = kv::optional(block, key, "");
  if (kv::trim(raw).empty()) {
    return {};
  }
  return cuwacunu::hero::lattice::exposure::parse_exposure_use_list(raw);
}

[[nodiscard]] inline cuwacunu::hero::lattice::exposure::exposure_split_role_t
exposure_role_from_source_role(
    cuwacunu::ujcamei::source::splits::source_split_role_t role) {
  using source_role_t = cuwacunu::ujcamei::source::splits::source_split_role_t;
  using exposure_role_t =
      cuwacunu::hero::lattice::exposure::exposure_split_role_t;
  switch (role) {
  case source_role_t::unknown:
    return exposure_role_t::unknown;
  case source_role_t::train:
    return exposure_role_t::train;
  case source_role_t::validation:
    return exposure_role_t::validation;
  case source_role_t::test:
    return exposure_role_t::test;
  }
  return exposure_role_t::unknown;
}

[[nodiscard]] inline lattice_split_policy_overlay_t
decode_lattice_split_policy_overlay_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_split_policy_overlay_t out{};
  bool saw_policy = false;
  std::vector<std::string> protected_split_ids;
  for (const auto &block : kv::parse_blocks(dsl_text)) {
    if (block.name == "LATTICE_SPLIT_POLICY") {
      if (saw_policy) {
        throw std::runtime_error(
            "[lattice_split] only one LATTICE_SPLIT_POLICY block is allowed");
      }
      saw_policy = true;
      out.policy_id = kv::required(block, "POLICY_ID");
      out.source_split_catalog_id =
          kv::required(block, "SOURCE_SPLIT_CATALOG_ID");
      out.purge_left_context =
          kv::optional(block, "PURGE_LEFT_CONTEXT", out.purge_left_context);
      out.purge_right_future =
          kv::optional(block, "PURGE_RIGHT_FUTURE", out.purge_right_future);
      continue;
    }
    if (block.name != "LATTICE_SPLIT_PROTECTION") {
      throw std::runtime_error("[lattice_split] unexpected block: " +
                               block.name);
    }
    lattice_split_protection_t protection{};
    protection.split_id = kv::required(block, "SPLIT_ID");
    protection.allow_uses = parse_optional_exposure_uses(block, "ALLOW_USES");
    protection.protect_from_uses =
        parse_optional_exposure_uses(block, "PROTECT_FROM_USES");
    protection.protect_requires_mutated_component = kv::parse_bool(
        kv::optional(block, "PROTECT_REQUIRES_MUTATED_COMPONENT", "true"));
    if (std::find(protected_split_ids.begin(), protected_split_ids.end(),
                  protection.split_id) != protected_split_ids.end()) {
      throw std::runtime_error(
          "[lattice_split] duplicate LATTICE_SPLIT_PROTECTION SPLIT_ID: " +
          protection.split_id);
    }
    protected_split_ids.push_back(protection.split_id);
    out.protections.push_back(std::move(protection));
  }
  if (!saw_policy) {
    throw std::runtime_error(
        "[lattice_split] LATTICE_SPLIT_POLICY block is required");
  }
  if (out.policy_id.empty()) {
    throw std::runtime_error("[lattice_split] POLICY_ID is required");
  }
  if (out.source_split_catalog_id.empty()) {
    throw std::runtime_error(
        "[lattice_split] SOURCE_SPLIT_CATALOG_ID is required");
  }
  return out;
}

[[nodiscard]] inline lattice_split_policy_t
bind_lattice_split_policy_to_source_catalog(
    const lattice_split_policy_overlay_t &overlay,
    const cuwacunu::ujcamei::source::splits::source_split_catalog_t
        &source_catalog) {
  if (overlay.source_split_catalog_id != source_catalog.catalog_id) {
    throw std::runtime_error(
        "[lattice_split] SOURCE_SPLIT_CATALOG_ID mismatch: policy references " +
        overlay.source_split_catalog_id + " but source catalog is " +
        source_catalog.catalog_id);
  }

  lattice_split_policy_t out{};
  out.policy_id = overlay.policy_id;
  out.source_split_catalog_id = source_catalog.catalog_id;
  out.source_split_catalog_fingerprint =
      cuwacunu::ujcamei::source::splits::source_split_catalog_fingerprint(
          source_catalog);
  out.cursor_domain = source_catalog.cursor_domain;
  out.purge_left_context = overlay.purge_left_context;
  out.purge_right_future = overlay.purge_right_future;
  out.splits.reserve(source_catalog.splits.size());
  for (const auto &source_split : source_catalog.splits) {
    lattice_split_t split{};
    split.split_id = source_split.split_id;
    split.role = exposure_role_from_source_role(source_split.role);
    split.source_split_intent = source_split;
    split.anchor_range_materialized =
        source_split.selector_kind ==
        cuwacunu::ujcamei::source::splits::source_split_selector_kind_t::
            anchor_index_range;
    if (split.anchor_range_materialized) {
      split.anchor_range = cuwacunu::hero::lattice::exposure::anchor_interval_t{
          .begin = source_split.anchor_range.begin,
          .end = source_split.anchor_range.end,
      };
    }
    out.splits.push_back(std::move(split));
  }

  for (const auto &protection : overlay.protections) {
    auto split = std::find_if(out.splits.begin(), out.splits.end(),
                              [&](const lattice_split_t &item) {
                                return item.split_id == protection.split_id;
                              });
    if (split == out.splits.end()) {
      throw std::runtime_error(
          "[lattice_split] LATTICE_SPLIT_PROTECTION references unknown source "
          "split: " +
          protection.split_id);
    }
    split->allow_uses = protection.allow_uses;
    split->protect_from_uses = protection.protect_from_uses;
    split->protect_requires_mutated_component =
        protection.protect_requires_mutated_component;
  }

  if (out.splits.empty()) {
    throw std::runtime_error(
        "[lattice_split] source split catalog has no splits");
  }
  return out;
}

[[nodiscard]] inline lattice_split_t
materialize_lattice_split(const lattice_split_t &split,
                          std::int64_t accepted_anchor_count) {
  lattice_split_t out = split;
  const auto materialized =
      cuwacunu::ujcamei::source::splits::materialize_source_split_range(
          split.source_split_intent, accepted_anchor_count);
  out.anchor_range = cuwacunu::hero::lattice::exposure::anchor_interval_t{
      .begin = materialized.begin,
      .end = materialized.end,
  };
  out.anchor_range_materialized = true;
  return out;
}

[[nodiscard]] inline lattice_split_policy_t
materialize_lattice_split_policy(const lattice_split_policy_t &policy,
                                 std::int64_t accepted_anchor_count) {
  lattice_split_policy_t out = policy;
  out.splits.clear();
  out.splits.reserve(policy.splits.size());
  for (const auto &split : policy.splits) {
    out.splits.push_back(
        materialize_lattice_split(split, accepted_anchor_count));
  }
  return out;
}

[[nodiscard]] inline lattice_split_policy_t
decode_lattice_split_policy_from_dsl(
    const std::string &policy_dsl_text,
    const cuwacunu::ujcamei::source::splits::source_split_catalog_t
        &source_catalog) {
  return bind_lattice_split_policy_to_source_catalog(
      decode_lattice_split_policy_overlay_from_dsl(policy_dsl_text),
      source_catalog);
}

[[nodiscard]] inline lattice_split_policy_t
load_lattice_split_policy_from_files(
    const std::filesystem::path &policy_path,
    const std::filesystem::path &source_catalog_path) {
  const auto source_catalog =
      cuwacunu::ujcamei::source::splits::load_source_split_catalog_from_file(
          source_catalog_path);
  return decode_lattice_split_policy_from_dsl(
      split_policy_detail::read_text_file_or_throw(policy_path),
      source_catalog);
}

inline void append_exposure_use_list(
    std::ostringstream &out,
    const std::vector<cuwacunu::hero::lattice::exposure::exposure_use_t>
        &uses) {
  std::vector<std::string> names;
  names.reserve(uses.size());
  for (const auto use : uses) {
    names.push_back(cuwacunu::hero::lattice::exposure::exposure_use_name(use));
  }
  std::sort(names.begin(), names.end());
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) {
      out << "|";
    }
    out << names[i];
  }
}

[[nodiscard]] inline std::string
canonical_lattice_split_policy_text(const lattice_split_policy_t &policy) {
  std::ostringstream out;
  out << "schema=" << policy.schema << "\n";
  out << "policy_id=" << policy.policy_id << "\n";
  out << "source_split_catalog_id=" << policy.source_split_catalog_id << "\n";
  out << "source_split_catalog_fingerprint="
      << policy.source_split_catalog_fingerprint << "\n";
  out << "cursor_domain=" << policy.cursor_domain << "\n";
  out << "purge_left_context=" << policy.purge_left_context << "\n";
  out << "purge_right_future=" << policy.purge_right_future << "\n";
  std::vector<lattice_split_t> splits = policy.splits;
  std::sort(splits.begin(), splits.end(),
            [](const lattice_split_t &a, const lattice_split_t &b) {
              return a.split_id < b.split_id;
            });
  for (const auto &split : splits) {
    const auto &source_intent = split.source_split_intent;
    out << "split=" << split.split_id << "|"
        << cuwacunu::hero::lattice::exposure::exposure_split_role_name(
               split.role);
    if (source_intent.selector_kind ==
        cuwacunu::ujcamei::source::splits::source_split_selector_kind_t::
            anchor_index_range) {
      out << "|" << source_intent.anchor_range.begin << "|"
          << source_intent.anchor_range.end;
    } else {
      out << "|"
          << cuwacunu::ujcamei::source::splits::source_split_selector_kind_name(
                 source_intent.selector_kind)
          << "|fraction="
          << cuwacunu::ujcamei::source::splits::source_split_detail::
                 fraction_text(source_intent.fraction_range.begin)
          << ":"
          << cuwacunu::ujcamei::source::splits::source_split_detail::
                 fraction_text(source_intent.fraction_range.end)
          << "|min_count=" << source_intent.min_count;
    }
    out << "|allow=";
    append_exposure_use_list(out, split.allow_uses);
    out << "|protect=";
    append_exposure_use_list(out, split.protect_from_uses);
    out << "|protect_requires_mutated_component="
        << (split.protect_requires_mutated_component ? "true" : "false")
        << "\n";
  }
  return out.str();
}

[[nodiscard]] inline std::string
lattice_split_policy_fingerprint(const lattice_split_policy_t &policy) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.split_policy.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, canonical_lattice_split_policy_text(policy));
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

} // namespace cuwacunu::hero::lattice::split
