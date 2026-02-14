// ./include/tsiemene/ports.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string_view>

namespace tsiemene {

using PortId = std::uint32_t;

enum class PortDir : std::uint8_t { In, Out };
[[nodiscard]] constexpr bool is_in(PortDir d) noexcept { return d == PortDir::In; }
[[nodiscard]] constexpr bool is_out(PortDir d) noexcept { return d == PortDir::Out; }

// Minimal payload families.
enum class PayloadKind : std::uint8_t { Tensor, String };

// Schema starts as only kind; grows later without redesign.
struct Schema {
  PayloadKind kind{PayloadKind::Tensor};

  [[nodiscard]] static constexpr Schema Tensor() noexcept { return {PayloadKind::Tensor}; }
  [[nodiscard]] static constexpr Schema String() noexcept { return {PayloadKind::String}; }
};

// Port = static metadata.
struct Port {
  PortId id{};
  PortDir dir{PortDir::In};
  Schema schema{};

  // Optional exact-match label within same kind.
  // Empty means "any".
  std::string_view tag{};
  std::string_view doc{};
};

[[nodiscard]] constexpr Port port(PortId id,
                                  PortDir dir,
                                  Schema schema = {},
                                  std::string_view tag = {},
                                  std::string_view doc = {}) noexcept {
  return Port{.id = id, .dir = dir, .schema = schema, .tag = tag, .doc = doc};
}

struct PortIssue {
  std::string_view what{};
};

// Build-time compatibility:
// - Out -> In
// - same schema.kind
// - tag matches if both non-empty
[[nodiscard]] constexpr bool compatible(const Port& outp,
                                        const Port& inp,
                                        PortIssue* issue = nullptr) noexcept {
  if (!is_out(outp.dir) || !is_in(inp.dir)) {
    if (issue) issue->what = "direction mismatch (expected Out -> In)";
    return false;
  }
  if (outp.schema.kind != inp.schema.kind) {
    if (issue) issue->what = "kind mismatch";
    return false;
  }
  if (!outp.tag.empty() && !inp.tag.empty() && outp.tag != inp.tag) {
    if (issue) issue->what = "tag mismatch";
    return false;
  }
  return true;
}

} // namespace tsiemene
