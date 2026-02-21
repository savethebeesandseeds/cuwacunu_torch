// ./include/tsiemene/utils/directives.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string_view>

namespace tsiemene {

using DirectiveId = std::string_view;

// Canonical directive ids used across tsi nodes and runtime wiring.
namespace directive_id {
inline constexpr DirectiveId Payload = "@payload";
inline constexpr DirectiveId Loss    = "@loss";
inline constexpr DirectiveId Meta    = "@meta";
} // namespace directive_id

enum class DirectiveDir : std::uint8_t { In, Out };
[[nodiscard]] constexpr bool is_in(DirectiveDir d) noexcept { return d == DirectiveDir::In; }
[[nodiscard]] constexpr bool is_out(DirectiveDir d) noexcept { return d == DirectiveDir::Out; }

// Minimal payload families.
enum class PayloadKind : std::uint8_t { Tensor, String };

// Kind specifier (:tensor / :str).
struct KindSpec {
  PayloadKind kind{PayloadKind::Tensor};

  [[nodiscard]] static constexpr KindSpec Tensor() noexcept { return {PayloadKind::Tensor}; }
  [[nodiscard]] static constexpr KindSpec String() noexcept { return {PayloadKind::String}; }
};

[[nodiscard]] constexpr std::string_view kind_token(PayloadKind k) noexcept {
  switch (k) {
    case PayloadKind::Tensor: return ":tensor";
    case PayloadKind::String: return ":str";
  }
  return ":unknown";
}

// Directive = static metadata.
struct DirectiveSpec {
  DirectiveId id{};
  DirectiveDir dir{DirectiveDir::In};
  KindSpec kind{};
  std::string_view doc{};
};

[[nodiscard]] constexpr DirectiveSpec directive(DirectiveId id,
                                                DirectiveDir dir,
                                                KindSpec kind = {},
                                                std::string_view doc = {}) noexcept {
  return DirectiveSpec{.id = id, .dir = dir, .kind = kind, .doc = doc};
}

struct DirectiveIssue {
  std::string_view what{};
};

// Build-time compatibility:
// - Out -> In
// - same kind
// - directive id matches if both non-empty
[[nodiscard]] constexpr bool compatible(const DirectiveSpec& outp,
                                        const DirectiveSpec& inp,
                                        DirectiveIssue* issue = nullptr) noexcept {
  if (!is_out(outp.dir) || !is_in(inp.dir)) {
    if (issue) issue->what = "direction mismatch (expected Out -> In)";
    return false;
  }
  if (outp.kind.kind != inp.kind.kind) {
    if (issue) issue->what = "kind mismatch";
    return false;
  }
  if (!outp.id.empty() && !inp.id.empty() && outp.id != inp.id) {
    if (issue) issue->what = "directive mismatch";
    return false;
  }
  return true;
}

} // namespace tsiemene
