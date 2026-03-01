// ./include/hashimyei/hashimyei_identity.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

[[nodiscard]] inline bool is_hex_hash_name(std::string_view s) {
  if (s.size() < 3) return false;
  if (s[0] != '0') return false;
  if (s[1] != 'x' && s[1] != 'X') return false;
  for (std::size_t i = 2; i < s.size(); ++i) {
    const char c = s[i];
    const bool digit = (c >= '0' && c <= '9');
    const bool lower = (c >= 'a' && c <= 'f');
    const bool upper = (c >= 'A' && c <= 'F');
    if (!digit && !lower && !upper) return false;
  }
  return true;
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
  *out_hashimyei = std::string(hash);
  return true;
}

}  // namespace hashimyei
}  // namespace cuwacunu
