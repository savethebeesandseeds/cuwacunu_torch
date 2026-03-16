#pragma once

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace family_rank {

inline constexpr std::string_view kFamilyRankSchemaV1 = "hero.family.rank.v1";

struct assignment_t {
  std::uint64_t rank{0};
  std::string hashimyei{};
  std::string canonical_path{};
  std::string component_id{};
};

struct state_t {
  std::string schema{std::string(kFamilyRankSchemaV1)};
  std::string family{};
  std::string contract_hash{};
  std::string source_view_kind{};
  std::string source_view_transport_sha256{};
  std::uint64_t updated_at_ms{0};
  std::vector<assignment_t> assignments{};
};

[[nodiscard]] inline std::string trim_ascii_copy(std::string_view text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] inline bool parse_u64(std::string_view text,
                                    std::uint64_t* out) noexcept {
  if (!out) return false;
  const std::string trimmed = trim_ascii_copy(text);
  if (trimmed.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), parsed);
  if (ec != std::errc{} || ptr != trimmed.data() + trimmed.size()) return false;
  *out = parsed;
  return true;
}

[[nodiscard]] inline std::string padded_index(std::size_t value) {
  std::ostringstream out;
  out.width(4);
  out.fill('0');
  out << value;
  return out.str();
}

[[nodiscard]] inline std::string scope_key(std::string_view family,
                                           std::string_view contract_hash) {
  return trim_ascii_copy(family) + "|" + trim_ascii_copy(contract_hash);
}

[[nodiscard]] inline std::optional<std::uint64_t> rank_for_hashimyei(
    const state_t& state, std::string_view hashimyei) {
  const std::string query = trim_ascii_copy(hashimyei);
  if (query.empty()) return std::nullopt;
  for (const auto& assignment : state.assignments) {
    if (assignment.hashimyei == query) return assignment.rank;
  }
  return std::nullopt;
}

[[nodiscard]] inline const assignment_t* find_assignment(
    const state_t& state, std::string_view hashimyei) {
  const std::string query = trim_ascii_copy(hashimyei);
  if (query.empty()) return nullptr;
  for (const auto& assignment : state.assignments) {
    if (assignment.hashimyei == query) return &assignment;
  }
  return nullptr;
}

[[nodiscard]] inline bool parse_state_from_kv(
    const std::unordered_map<std::string, std::string>& kv, state_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "family rank output pointer is null";
    return false;
  }
  *out = state_t{};

  const auto schema_it = kv.find("schema");
  if (schema_it == kv.end() ||
      trim_ascii_copy(schema_it->second) != std::string(kFamilyRankSchemaV1)) {
    if (error) *error = "family rank schema mismatch";
    return false;
  }
  out->schema = std::string(kFamilyRankSchemaV1);

  const auto family_it = kv.find("family");
  if (family_it == kv.end()) {
    if (error) *error = "family rank payload missing family";
    return false;
  }
  out->family = trim_ascii_copy(family_it->second);
  if (out->family.empty()) {
    if (error) *error = "family rank payload family is empty";
    return false;
  }

  const auto contract_it = kv.find("contract_hash");
  if (contract_it == kv.end()) {
    if (error) *error = "family rank payload missing contract_hash";
    return false;
  }
  out->contract_hash = trim_ascii_copy(contract_it->second);
  if (out->contract_hash.empty()) {
    if (error) *error = "family rank payload contract_hash is empty";
    return false;
  }
  if (const auto it = kv.find("source_view_kind"); it != kv.end()) {
    out->source_view_kind = trim_ascii_copy(it->second);
  }
  if (const auto it = kv.find("source_view_transport_sha256"); it != kv.end()) {
    out->source_view_transport_sha256 = trim_ascii_copy(it->second);
  }
  if (const auto it = kv.find("updated_at_ms"); it != kv.end()) {
    (void)parse_u64(it->second, &out->updated_at_ms);
  }

  std::uint64_t assignment_count = 0;
  if (const auto it = kv.find("assignment_count"); it != kv.end()) {
    if (!parse_u64(it->second, &assignment_count)) {
      if (error) *error = "family rank assignment_count is invalid";
      return false;
    }
  }

  out->assignments.clear();
  out->assignments.reserve(static_cast<std::size_t>(assignment_count));
  std::unordered_set<std::string> seen_hashimyeis{};
  for (std::uint64_t i = 0; i < assignment_count; ++i) {
    const std::string prefix = "assignment_" + padded_index(static_cast<std::size_t>(i));
    assignment_t assignment{};
    assignment.rank = i;
    if (const auto it = kv.find(prefix + ".rank"); it != kv.end()) {
      if (!parse_u64(it->second, &assignment.rank)) {
        if (error) *error = "family rank assignment rank is invalid";
        return false;
      }
    }
    if (const auto it = kv.find(prefix + ".hashimyei"); it != kv.end()) {
      assignment.hashimyei = trim_ascii_copy(it->second);
    }
    if (assignment.hashimyei.empty()) {
      if (error) *error = "family rank assignment hashimyei is empty";
      return false;
    }
    if (!seen_hashimyeis.insert(assignment.hashimyei).second) {
      if (error) *error = "family rank payload has duplicate hashimyei";
      return false;
    }
    if (const auto it = kv.find(prefix + ".canonical_path"); it != kv.end()) {
      assignment.canonical_path = trim_ascii_copy(it->second);
    }
    if (const auto it = kv.find(prefix + ".component_id"); it != kv.end()) {
      assignment.component_id = trim_ascii_copy(it->second);
    }
    out->assignments.push_back(std::move(assignment));
  }

  std::sort(out->assignments.begin(), out->assignments.end(),
            [](const assignment_t& a, const assignment_t& b) {
              if (a.rank != b.rank) return a.rank < b.rank;
              return a.hashimyei < b.hashimyei;
            });
  for (std::size_t i = 0; i < out->assignments.size(); ++i) {
    if (out->assignments[i].rank != static_cast<std::uint64_t>(i)) {
      if (error) *error = "family rank assignments must be contiguous from zero";
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline std::string emit_state_payload(const state_t& state) {
  using cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical;
  using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry;

  state_t normalized = state;
  if (normalized.schema.empty()) {
    normalized.schema = std::string(kFamilyRankSchemaV1);
  }
  std::sort(normalized.assignments.begin(), normalized.assignments.end(),
            [](const assignment_t& a, const assignment_t& b) {
              if (a.rank != b.rank) return a.rank < b.rank;
              return a.hashimyei < b.hashimyei;
            });

  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  document.entries.push_back(
      make_runtime_lls_string_entry("schema", normalized.schema));
  document.entries.push_back(
      make_runtime_lls_string_entry("family", normalized.family));
  document.entries.push_back(
      make_runtime_lls_string_entry("contract_hash", normalized.contract_hash));
  if (!normalized.source_view_kind.empty()) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "source_view_kind", normalized.source_view_kind));
  }
  if (!normalized.source_view_transport_sha256.empty()) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "source_view_transport_sha256",
        normalized.source_view_transport_sha256));
  }
  document.entries.push_back(make_runtime_lls_string_entry(
      "updated_at_ms", std::to_string(normalized.updated_at_ms)));
  document.entries.push_back(make_runtime_lls_string_entry(
      "assignment_count", std::to_string(normalized.assignments.size())));

  for (std::size_t i = 0; i < normalized.assignments.size(); ++i) {
    const assignment_t& assignment = normalized.assignments[i];
    const std::string prefix = "assignment_" + padded_index(i);
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + ".rank", std::to_string(assignment.rank)));
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + ".hashimyei", assignment.hashimyei));
    if (!assignment.canonical_path.empty()) {
      document.entries.push_back(make_runtime_lls_string_entry(
          prefix + ".canonical_path", assignment.canonical_path));
    }
    if (!assignment.component_id.empty()) {
      document.entries.push_back(make_runtime_lls_string_entry(
          prefix + ".component_id", assignment.component_id));
    }
  }

  return emit_runtime_lls_canonical(document);
}

[[nodiscard]] inline bool ordering_matches(const state_t& lhs,
                                           const state_t& rhs) {
  if (trim_ascii_copy(lhs.family) != trim_ascii_copy(rhs.family) ||
      trim_ascii_copy(lhs.contract_hash) != trim_ascii_copy(rhs.contract_hash) ||
      lhs.assignments.size() != rhs.assignments.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.assignments.size(); ++i) {
    if (lhs.assignments[i].rank != rhs.assignments[i].rank ||
        trim_ascii_copy(lhs.assignments[i].hashimyei) !=
            trim_ascii_copy(rhs.assignments[i].hashimyei)) {
      return false;
    }
  }
  return true;
}

}  // namespace family_rank
}  // namespace hero
}  // namespace cuwacunu
