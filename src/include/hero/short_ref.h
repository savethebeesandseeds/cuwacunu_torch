// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace cuwacunu::hero::display {

inline constexpr std::size_t kDefaultDigestDisplayPrefixLength = 12;

[[nodiscard]] inline std::string
digest_prefix(std::string_view digest,
              std::size_t length = kDefaultDigestDisplayPrefixLength) {
  if (digest.empty() || length == 0U) {
    return {};
  }
  return std::string(digest.substr(0, std::min(length, digest.size())));
}

[[nodiscard]] inline std::string
digest_ref(std::string_view ref_prefix, std::string_view digest,
           std::size_t length = kDefaultDigestDisplayPrefixLength) {
  const auto prefix = digest_prefix(digest, length);
  if (prefix.empty()) {
    return {};
  }
  const std::string family_prefix =
      ref_prefix.empty() ? "ref" : std::string(ref_prefix);
  return family_prefix + "_" + prefix;
}

[[nodiscard]] inline std::string
fact_family_ref_prefix(std::string_view family) {
  if (family == "exposure") {
    return "exp";
  }
  if (family == "node_exposure") {
    return "node";
  }
  if (family == "checkpoint") {
    return "ckpt";
  }
  if (family == "source_receipt") {
    return "src";
  }
  if (family == "source_analytics") {
    return "san";
  }
  if (family == "target_transform") {
    return "ttf";
  }
  if (family == "forecast_baseline") {
    return "fbl";
  }
  if (family == "forecast_eval") {
    return "fev";
  }
  if (family == "observer_belief") {
    return "obl";
  }
  if (family == "allocation_engine") {
    return "alloc";
  }
  if (family == "replay_environment") {
    return "rep";
  }
  if (family == "policy_training") {
    return "ptf";
  }
  if (family == "selection_signal") {
    return "sel";
  }
  if (family == "representation_support") {
    return "rsf";
  }
  return "ref";
}

[[nodiscard]] inline std::string
fact_ref(std::string_view family, std::string_view digest,
         std::size_t length = kDefaultDigestDisplayPrefixLength) {
  return digest_ref(fact_family_ref_prefix(family), digest, length);
}

[[nodiscard]] inline std::string
checkpoint_ref(std::string_view digest,
               std::size_t length = kDefaultDigestDisplayPrefixLength) {
  return digest_ref("ckpt", digest, length);
}

[[nodiscard]] inline std::string
report_ref(std::string_view digest,
           std::size_t length = kDefaultDigestDisplayPrefixLength) {
  return digest_ref("rpt", digest, length);
}

[[nodiscard]] inline std::string
contract_ref(std::string_view digest,
             std::size_t length = kDefaultDigestDisplayPrefixLength) {
  return digest_ref("ctr", digest, length);
}

[[nodiscard]] inline std::string
certificate_ref(std::string_view digest,
                std::size_t length = kDefaultDigestDisplayPrefixLength) {
  return digest_ref("cert", digest, length);
}

} // namespace cuwacunu::hero::display
