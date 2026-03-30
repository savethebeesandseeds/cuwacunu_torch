// ./include/hero/hashimyei_hero/hashimyei_identity.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hero/hashimyei_hero/hashimyei_schema.h"

namespace cuwacunu {
namespace hashimyei {

inline constexpr std::size_t kHexIdentityCatalogSize = 16;

inline constexpr std::uint64_t kFnv64Offset = 14695981039346656037ull;
inline constexpr std::uint64_t kFnv64Prime = 1099511628211ull;

[[nodiscard]] inline std::uint64_t fnv1a64(std::string_view s) {
  std::uint64_t h = kFnv64Offset;
  for (unsigned char c : s) {
    h ^= static_cast<std::uint64_t>(c);
    h *= kFnv64Prime;
  }
  return h;
}

[[nodiscard]] inline std::string hex64(std::uint64_t v) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << std::nouppercase << v;
  return oss.str();
}

[[nodiscard]] inline std::string make_hex_hash_name(std::uint64_t ordinal) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::nouppercase << std::setfill('0')
      << std::setw(4) << ordinal;
  return oss.str();
}

enum class hashimyei_kind_e : std::uint8_t {
#define HASHIMYEI_KIND_ENTRY(ENUM_NAME, REQUIRES_SHA256) ENUM_NAME,
#include "hero/hashimyei_hero/hashimyei_kind.def"
#undef HASHIMYEI_KIND_ENTRY
};

[[nodiscard]] inline std::string hashimyei_kind_to_string(hashimyei_kind_e kind) {
  switch (kind) {
#define HASHIMYEI_KIND_ENTRY(ENUM_NAME, REQUIRES_SHA256) \
  case hashimyei_kind_e::ENUM_NAME:                      \
    return #ENUM_NAME;
#include "hero/hashimyei_hero/hashimyei_kind.def"
#undef HASHIMYEI_KIND_ENTRY
  }
  return "TSIEMENE";
}

[[nodiscard]] inline bool parse_hashimyei_kind(std::string_view text,
                                               hashimyei_kind_e* out) {
  if (!out) return false;
#define HASHIMYEI_KIND_ENTRY(ENUM_NAME, REQUIRES_SHA256) \
  if (text == #ENUM_NAME) {                              \
    *out = hashimyei_kind_e::ENUM_NAME;                  \
    return true;                                         \
  }
#include "hero/hashimyei_hero/hashimyei_kind.def"
#undef HASHIMYEI_KIND_ENTRY
  return false;
}

[[nodiscard]] inline bool hashimyei_kind_requires_sha256(hashimyei_kind_e kind) {
  switch (kind) {
#define HASHIMYEI_KIND_ENTRY(ENUM_NAME, REQUIRES_SHA256) \
  case hashimyei_kind_e::ENUM_NAME:                      \
    return REQUIRES_SHA256;
#include "hero/hashimyei_hero/hashimyei_kind.def"
#undef HASHIMYEI_KIND_ENTRY
  }
  return false;
}

[[nodiscard]] inline bool is_lower_hex_64(std::string_view text) {
  if (text.size() != 64) return false;
  for (const char c : text) {
    const bool digit = c >= '0' && c <= '9';
    const bool lower = c >= 'a' && c <= 'f';
    if (!digit && !lower) return false;
  }
  return true;
}

[[nodiscard]] inline bool normalize_hex_hash_name(
    std::string_view name, std::string* out_name,
    std::uint64_t* out_ordinal = nullptr) {
  if (!out_name && !out_ordinal) return false;
  if (name.size() < 3) return false;
  if (name[0] != '0' || (name[1] != 'x' && name[1] != 'X')) return false;

  std::uint64_t out = 0;
  for (std::size_t i = 2; i < name.size(); ++i) {
    const char c = name[i];
    std::uint64_t nibble = 0;
    if (c >= '0' && c <= '9') {
      nibble = static_cast<std::uint64_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      nibble = static_cast<std::uint64_t>(10 + (c - 'a'));
    } else if (c >= 'A' && c <= 'F') {
      nibble = static_cast<std::uint64_t>(10 + (c - 'A'));
    } else {
      return false;
    }
    if (out > (std::numeric_limits<std::uint64_t>::max() >> 4u)) return false;
    out = (out << 4u) | nibble;
  }

  if (out_ordinal) *out_ordinal = out;
  if (out_name) *out_name = make_hex_hash_name(out);
  return true;
}

inline void normalize_hex_hash_name_inplace(std::string* name) {
  if (!name) return;
  std::string normalized{};
  if (normalize_hex_hash_name(*name, &normalized)) {
    *name = std::move(normalized);
  }
}

[[nodiscard]] inline std::string normalize_hashimyei_canonical_path(
    std::string_view canonical_path) {
  std::string normalized(canonical_path);
  const std::size_t dot = normalized.rfind('.');
  if (dot == std::string::npos || dot + 1 >= normalized.size()) {
    return normalized;
  }

  std::string normalized_tail{};
  if (!normalize_hex_hash_name(
          std::string_view(normalized).substr(dot + 1), &normalized_tail)) {
    return normalized;
  }

  normalized.erase(dot + 1);
  normalized += normalized_tail;
  return normalized;
}

[[nodiscard]] inline bool parse_hex_hash_name_ordinal(std::string_view name,
                                                      std::uint64_t* out_ordinal) {
  return normalize_hex_hash_name(name, nullptr, out_ordinal);
}

struct hashimyei_t {
  std::string schema{kIdentitySchemaV2};
  hashimyei_kind_e kind{hashimyei_kind_e::TSIEMENE};
  std::string name{};
  std::uint64_t ordinal{0};
  std::string hash_sha256_hex{};
};

[[nodiscard]] inline bool validate_hashimyei(const hashimyei_t& id,
                                             std::string* error = nullptr) {
  if (error) error->clear();
  if (id.schema != kIdentitySchemaV2) {
    if (error) *error = std::string("identity.schema must be ") + kIdentitySchemaV2;
    return false;
  }
  if (id.name.empty()) {
    if (error) *error = "identity.name is required";
    return false;
  }
  std::uint64_t parsed = 0;
  if (!normalize_hex_hash_name(id.name, nullptr, &parsed)) {
    if (error) *error = "identity.name is not a valid 0x... hex ordinal";
    return false;
  }
  if (parsed != id.ordinal) {
    if (error) *error = "identity.ordinal does not match identity.name";
    return false;
  }
  if (hashimyei_kind_requires_sha256(id.kind)) {
    if (!is_lower_hex_64(id.hash_sha256_hex)) {
      if (error) {
        *error = "identity.hash_sha256_hex must be 64 lowercase hex chars";
      }
      return false;
    }
  } else if (!id.hash_sha256_hex.empty()) {
    if (error) {
      *error = "identity.hash_sha256_hex must be empty for this identity kind";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline hashimyei_t make_identity(hashimyei_kind_e kind,
                                               std::uint64_t ordinal,
                                               std::string hash_sha256_hex = {}) {
  hashimyei_t out{};
  out.kind = kind;
  out.ordinal = ordinal;
  out.name = make_hex_hash_name(ordinal);
  out.hash_sha256_hex = std::move(hash_sha256_hex);
  return out;
}

[[nodiscard]] inline bool is_hex_hash_name(std::string_view s) {
  std::uint64_t ordinal = 0;
  return normalize_hex_hash_name(s, nullptr, &ordinal);
}

[[nodiscard]] inline const std::vector<std::string>& known_hashimyeis() {
  static const std::vector<std::string> kNames = []() {
    std::vector<std::string> out;
    out.reserve(kHexIdentityCatalogSize);
    for (std::size_t i = 0; i < kHexIdentityCatalogSize; ++i) {
      out.push_back(make_hex_hash_name(i));
    }
    return out;
  }();
  return kNames;
}

class identity_provider_t final {
 public:
  [[nodiscard]] std::string assign(std::string_view key) {
    std::lock_guard<std::mutex> lk(mu_);

    const std::string key_s(key);
    const auto it_existing = key_to_name_.find(key_s);
    if (it_existing != key_to_name_.end()) return it_existing->second;

    const std::uint64_t seed0 = fnv1a64(key);
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(kHexIdentityCatalogSize); ++i) {
      const std::string candidate = make_hex_hash_name((seed0 + i) & 0x0full);
      const auto it = name_to_key_.find(candidate);
      if (it == name_to_key_.end() || it->second == key_s) {
        name_to_key_[candidate] = key_s;
        key_to_name_[key_s] = candidate;
        return candidate;
      }
    }

    // Overflow path for >16 live identities in one process: stay in hex form.
    std::uint64_t nonce = static_cast<std::uint64_t>(kHexIdentityCatalogSize);
    for (;;) {
      const std::string candidate = "0x" + hex64(seed0 + nonce).substr(8);
      const auto it = name_to_key_.find(candidate);
      if (it == name_to_key_.end() || it->second == key_s) {
        name_to_key_[candidate] = key_s;
        key_to_name_[key_s] = candidate;
        return candidate;
      }
      ++nonce;
    }
  }

 private:
  std::mutex mu_{};
  std::unordered_map<std::string, std::string> key_to_name_{};
  std::unordered_map<std::string, std::string> name_to_key_{};
};

[[nodiscard]] inline identity_provider_t& canonical_identity_provider() {
  static identity_provider_t provider{};
  return provider;
}

[[nodiscard]] inline bool split_model_hash_suffix(std::string_view fused_model,
                                                  std::string* out_model,
                                                  std::string* out_hashimyei) {
  if (!out_model || !out_hashimyei) return false;

  const std::size_t us = fused_model.rfind('_');
  if (us == std::string_view::npos || us == 0 || us + 1 >= fused_model.size()) return false;

  const std::string_view model = fused_model.substr(0, us);
  const std::string_view hash = fused_model.substr(us + 1);
  if (model.empty() || !is_hex_hash_name(hash)) return false;

  *out_model = std::string(model);
  std::string normalized_hash{};
  if (!normalize_hex_hash_name(hash, &normalized_hash)) return false;
  *out_hashimyei = std::move(normalized_hash);
  return true;
}

}  // namespace hashimyei
}  // namespace cuwacunu
