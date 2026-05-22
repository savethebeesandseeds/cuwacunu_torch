// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "piaabo/parse/simple_kv_block.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::lattice::split {

struct lattice_split_t {
  std::string split_id{};
  cuwacunu::kikijyeba::lattice::exposure::exposure_split_role_t role{
      cuwacunu::kikijyeba::lattice::exposure::exposure_split_role_t::unknown};
  cuwacunu::kikijyeba::lattice::exposure::anchor_interval_t anchor_range{};
  std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
      allow_uses{};
  std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
      protect_from_uses{};
  bool protect_requires_mutated_component{true};
};

struct lattice_split_policy_t {
  std::string schema{"kikijyeba.lattice.splits.v1"};
  std::string policy_id{};
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

[[nodiscard]] inline std::int64_t
parse_nonnegative_i64(const kv::block_t &block, const std::string &key) {
  const auto value = kv::parse_i64(kv::required(block, key));
  if (value < 0) {
    throw std::runtime_error("[lattice_split] " + key +
                             " must be non-negative");
  }
  return value;
}

} // namespace split_policy_detail

[[nodiscard]] inline std::vector<
    cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
parse_optional_exposure_uses(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const std::string &key) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto raw = kv::optional(block, key, "");
  if (kv::trim(raw).empty()) {
    return {};
  }
  return cuwacunu::kikijyeba::lattice::exposure::parse_exposure_use_list(raw);
}

[[nodiscard]] inline lattice_split_policy_t
decode_lattice_split_policy_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  lattice_split_policy_t out{};
  bool saw_policy = false;
  std::vector<std::string> split_ids;
  for (const auto &block : kv::parse_blocks(dsl_text)) {
    if (block.name == "LATTICE_SPLIT_POLICY") {
      if (saw_policy) {
        throw std::runtime_error(
            "[lattice_split] only one LATTICE_SPLIT_POLICY block is allowed");
      }
      saw_policy = true;
      out.policy_id = kv::required(block, "POLICY_ID");
      out.cursor_domain =
          kv::optional(block, "CURSOR_DOMAIN", out.cursor_domain);
      out.purge_left_context =
          kv::optional(block, "PURGE_LEFT_CONTEXT", out.purge_left_context);
      out.purge_right_future =
          kv::optional(block, "PURGE_RIGHT_FUTURE", out.purge_right_future);
      continue;
    }
    if (block.name != "LATTICE_SPLIT") {
      throw std::runtime_error("[lattice_split] unexpected block: " +
                               block.name);
    }
    lattice_split_t split{};
    split.split_id = kv::required(block, "SPLIT_ID");
    split.role =
        cuwacunu::kikijyeba::lattice::exposure::parse_exposure_split_role(
            kv::required(block, "ROLE"));
    split.anchor_range.begin =
        split_policy_detail::parse_nonnegative_i64(block, "ANCHOR_INDEX_BEGIN");
    split.anchor_range.end =
        split_policy_detail::parse_nonnegative_i64(block, "ANCHOR_INDEX_END");
    split.allow_uses = parse_optional_exposure_uses(block, "ALLOW_USES");
    split.protect_from_uses =
        parse_optional_exposure_uses(block, "PROTECT_FROM_USES");
    split.protect_requires_mutated_component = kv::parse_bool(
        kv::optional(block, "PROTECT_REQUIRES_MUTATED_COMPONENT", "true"));
    if (split.anchor_range.end <= split.anchor_range.begin) {
      throw std::runtime_error(
          "[lattice_split] ANCHOR_INDEX_END must be greater than "
          "ANCHOR_INDEX_BEGIN for " +
          split.split_id);
    }
    if (std::find(split_ids.begin(), split_ids.end(), split.split_id) !=
        split_ids.end()) {
      throw std::runtime_error("[lattice_split] duplicate SPLIT_ID: " +
                               split.split_id);
    }
    split_ids.push_back(split.split_id);
    out.splits.push_back(std::move(split));
  }
  if (!saw_policy) {
    throw std::runtime_error(
        "[lattice_split] LATTICE_SPLIT_POLICY block is required");
  }
  if (out.policy_id.empty()) {
    throw std::runtime_error("[lattice_split] POLICY_ID is required");
  }
  if (out.cursor_domain != "ujcamei.graph_anchor") {
    throw std::runtime_error(
        "[lattice_split] v1 only supports CURSOR_DOMAIN=ujcamei.graph_anchor");
  }
  if (out.splits.empty()) {
    throw std::runtime_error("[lattice_split] at least one LATTICE_SPLIT block "
                             "is required");
  }
  return out;
}

[[nodiscard]] inline lattice_split_policy_t
load_lattice_split_policy_from_file(const std::filesystem::path &path) {
  return decode_lattice_split_policy_from_dsl(
      split_policy_detail::read_text_file_or_throw(path));
}

inline void append_exposure_use_list(
    std::ostringstream &out,
    const std::vector<cuwacunu::kikijyeba::lattice::exposure::exposure_use_t>
        &uses) {
  std::vector<std::string> names;
  names.reserve(uses.size());
  for (const auto use : uses) {
    names.push_back(
        cuwacunu::kikijyeba::lattice::exposure::exposure_use_name(use));
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
  out << "cursor_domain=" << policy.cursor_domain << "\n";
  out << "purge_left_context=" << policy.purge_left_context << "\n";
  out << "purge_right_future=" << policy.purge_right_future << "\n";
  std::vector<lattice_split_t> splits = policy.splits;
  std::sort(splits.begin(), splits.end(),
            [](const lattice_split_t &a, const lattice_split_t &b) {
              return a.split_id < b.split_id;
            });
  for (const auto &split : splits) {
    out << "split=" << split.split_id << "|"
        << cuwacunu::kikijyeba::lattice::exposure::exposure_split_role_name(
               split.role)
        << "|" << split.anchor_range.begin << "|" << split.anchor_range.end;
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
      hash, "kikijyeba.lattice.splits.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, canonical_lattice_split_policy_text(policy));
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

} // namespace cuwacunu::kikijyeba::lattice::split
