#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/common.h"
#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class circuit_draw_style_t : std::uint8_t {
  Default = 0,
  NodeSource,
  NodeWikimyei,
  NodeSink,
  NodeRoot,
  NodeAlias,
  NodeType,
  EdgePayload,
  EdgeMeta,
  EdgeLoss,
  EdgeControl,
  EdgeWarn,
  EdgeError,
  EdgeOther,
};

inline bool is_edge_style(circuit_draw_style_t s) {
  return s == circuit_draw_style_t::EdgePayload ||
         s == circuit_draw_style_t::EdgeMeta ||
         s == circuit_draw_style_t::EdgeLoss ||
         s == circuit_draw_style_t::EdgeControl ||
         s == circuit_draw_style_t::EdgeWarn ||
         s == circuit_draw_style_t::EdgeError ||
         s == circuit_draw_style_t::EdgeOther;
}

inline circuit_draw_style_t merge_draw_style(circuit_draw_style_t old_style,
                                             circuit_draw_style_t new_style) {
  if (new_style == circuit_draw_style_t::Default) return old_style;
  if (old_style == circuit_draw_style_t::Default) return new_style;
  if (old_style == new_style) return old_style;
  if (is_edge_style(old_style) && is_edge_style(new_style)) return circuit_draw_style_t::EdgeOther;
  if (is_edge_style(new_style)) return new_style;
  return old_style;
}

inline const char* draw_style_ansi_open(circuit_draw_style_t s) {
  switch (s) {
    case circuit_draw_style_t::NodeSource: return "\x1b[38;2;97;169;217m";
    case circuit_draw_style_t::NodeWikimyei: return "\x1b[38;2;212;174;102m";
    case circuit_draw_style_t::NodeSink: return "\x1b[38;2;123;179;131m";
    case circuit_draw_style_t::NodeRoot: return "\x1b[1;38;2;236;205;120m";
    case circuit_draw_style_t::NodeAlias: return "\x1b[1;38;2;227;234;244m";
    case circuit_draw_style_t::NodeType: return "\x1b[2;38;2;151;160;174m";
    case circuit_draw_style_t::EdgePayload: return "\x1b[38;2;111;161;248m";
    case circuit_draw_style_t::EdgeMeta: return "\x1b[38;2;169;143;214m";
    case circuit_draw_style_t::EdgeLoss: return "\x1b[38;2;214;106;106m";
    case circuit_draw_style_t::EdgeControl: return "\x1b[38;2;113;183;160m";
    case circuit_draw_style_t::EdgeWarn: return "\x1b[38;2;209;161;81m";
    case circuit_draw_style_t::EdgeError: return "\x1b[38;2;198;95;95m";
    case circuit_draw_style_t::EdgeOther: return "\x1b[38;2;179;179;186m";
    case circuit_draw_style_t::Default: return nullptr;
  }
  return nullptr;
}

inline std::string join_lines_ansi(
    const std::vector<std::string>& lines,
    const std::vector<std::vector<circuit_draw_style_t>>& styles) {
  std::ostringstream oss;
  for (std::size_t y = 0; y < lines.size(); ++y) {
    const auto& row = lines[y];
    int right = static_cast<int>(row.size()) - 1;
    while (right >= 0 && row[static_cast<std::size_t>(right)] == ' ') --right;

    circuit_draw_style_t current = circuit_draw_style_t::Default;
    for (int x = 0; x <= right; ++x) {
      const auto style = styles[y][static_cast<std::size_t>(x)];
      if (style != current) {
        if (current != circuit_draw_style_t::Default) oss << "\x1b[0m";
        if (const char* open = draw_style_ansi_open(style)) oss << open;
        current = style;
      }
      oss << row[static_cast<std::size_t>(x)];
    }
    if (current != circuit_draw_style_t::Default) oss << "\x1b[0m";
    if (y + 1 < lines.size()) oss << '\n';
  }
  return oss.str();
}

inline std::string compact_tsi_type_label(std::string_view full, std::size_t keep_parts = 3u) {
  std::string_view canonical = full;
  if (canonical.size() > 4 && canonical.substr(0, 4) == "tsi.") {
    canonical.remove_prefix(4);
  }
  std::vector<std::string_view> parts;
  std::size_t b = 0;
  while (b <= canonical.size()) {
    const std::size_t p = canonical.find('.', b);
    if (p == std::string_view::npos) {
      parts.push_back(canonical.substr(b));
      break;
    }
    parts.push_back(canonical.substr(b, p - b));
    b = p + 1;
  }
  if (parts.empty()) return std::string(canonical);
  if (keep_parts == 0u) keep_parts = 1u;
  const std::size_t keep = std::min(keep_parts, parts.size());
  const std::size_t start = parts.size() - keep;
  std::ostringstream oss;
  for (std::size_t i = start; i < parts.size(); ++i) {
    if (i > start) oss << '.';
    oss << parts[i];
  }
  return oss.str();
}

inline circuit_draw_style_t node_style_from_tsi_type(std::string_view tsi_type) {
  const auto type_id = tsiemene::parse_tsi_type_id(tsi_type);
  if (!type_id) return circuit_draw_style_t::NodeWikimyei;
  switch (tsiemene::tsi_type_domain(*type_id)) {
    case tsiemene::TsiDomain::Source: return circuit_draw_style_t::NodeSource;
    case tsiemene::TsiDomain::Wikimyei: return circuit_draw_style_t::NodeWikimyei;
    case tsiemene::TsiDomain::Sink: return circuit_draw_style_t::NodeSink;
  }
  return circuit_draw_style_t::NodeWikimyei;
}

inline circuit_draw_style_t edge_style_from_directive(tsiemene::DirectiveId d) {
  if (d == tsiemene::directive_id::Payload || d == tsiemene::directive_id::Future) {
    return circuit_draw_style_t::EdgePayload;
  }
  if (d == tsiemene::directive_id::Meta) return circuit_draw_style_t::EdgeMeta;
  if (d == tsiemene::directive_id::Loss) return circuit_draw_style_t::EdgeLoss;
  if (d == tsiemene::directive_id::Warn) return circuit_draw_style_t::EdgeWarn;
  if (d == tsiemene::directive_id::Error) return circuit_draw_style_t::EdgeError;
  if (d == tsiemene::directive_id::Info || d == tsiemene::directive_id::Debug ||
      d == tsiemene::directive_id::Step || d == tsiemene::directive_id::Init ||
      d == tsiemene::directive_id::Jkimyei || d == tsiemene::directive_id::Weights) {
    return circuit_draw_style_t::EdgeControl;
  }
  return circuit_draw_style_t::EdgeOther;
}

inline std::string short_directive_token(tsiemene::DirectiveId d) {
  std::string s(d);
  if (!s.empty() && s.front() == '@') s.erase(s.begin());
  return s;
}

inline std::string hop_label(const tsiemene_resolved_hop_t& h) {
  const std::string out = short_directive_token(h.from.directive);
  const std::string in = short_directive_token(h.to.directive);
  if (out == in) return out;
  return out + ">" + in;
}

inline std::string make_edge_legend_text(const std::vector<tsiemene_resolved_hop_t>& hops) {
  if (hops.empty()) return {};

  std::vector<tsiemene::DirectiveId> ordered{};
  ordered.reserve(hops.size());
  auto push_unique = [&](tsiemene::DirectiveId d) {
    for (const auto& x : ordered) {
      if (x == d) return;
    }
    ordered.push_back(d);
  };

  static constexpr std::array<tsiemene::DirectiveId, 12> kPref = {
      tsiemene::directive_id::Payload,
      tsiemene::directive_id::Future,
      tsiemene::directive_id::Meta,
      tsiemene::directive_id::Loss,
      tsiemene::directive_id::Info,
      tsiemene::directive_id::Warn,
      tsiemene::directive_id::Debug,
      tsiemene::directive_id::Error,
      tsiemene::directive_id::Step,
      tsiemene::directive_id::Init,
      tsiemene::directive_id::Jkimyei,
      tsiemene::directive_id::Weights,
  };
  for (const auto pref : kPref) {
    for (const auto& h : hops) {
      if (h.from.directive == pref) {
        push_unique(pref);
        break;
      }
    }
  }
  for (const auto& h : hops) push_unique(h.from.directive);

  if (ordered.empty()) return {};

  std::ostringstream oss;
  oss << "\n";
  for (const auto d : ordered) {
    const auto style = edge_style_from_directive(d);
    if (const char* open = draw_style_ansi_open(style)) oss << open;
    oss << "+--- " << d;
    oss << "\x1b[0m\n";
  }
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
