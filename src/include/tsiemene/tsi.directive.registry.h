// ./include/tsiemene/tsi.directive.registry.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tsiemene {

using DirectiveId = std::string_view;
using MethodId = std::string_view;

// Canonical directive ids used across tsi nodes and runtime wiring.
namespace directive_id {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) inline constexpr DirectiveId ID = TOKEN;
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) inline constexpr DirectiveId ID = TOKEN;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_DIRECTIVE
} // namespace directive_id

// Canonical callable method ids in tsi paths.
namespace method_id {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY) inline constexpr MethodId ID = TOKEN;
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY) inline constexpr MethodId ID = TOKEN;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
} // namespace method_id

[[nodiscard]] inline std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

[[nodiscard]] inline std::optional<DirectiveId> parse_directive_id(std::string token) {
  token = lower_ascii_copy(trim_ascii_ws_copy(std::move(token)));
  if (token.empty()) return std::nullopt;
  if (token.front() != '@') token.insert(token.begin(), '@');

#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) \
  if (token == TOKEN) return directive_id::ID;
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) \
  if (token == TOKEN) return directive_id::ID;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_DIRECTIVE
  return std::nullopt;
}

[[nodiscard]] inline std::optional<MethodId> parse_method_id(std::string token) {
  token = lower_ascii_copy(trim_ascii_ws_copy(std::move(token)));
  if (token.empty()) return std::nullopt;

#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY) \
  if (token == TOKEN) return method_id::ID;
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY) \
  if (token == TOKEN) return method_id::ID;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  return std::nullopt;
}

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
  return true;
}

} // namespace tsiemene
