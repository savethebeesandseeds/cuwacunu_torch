/* tsiemene_circuit.cpp */
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace camahjucunu {

std::optional<tsiemene::DirectiveId> parse_directive_ref(std::string s);
std::optional<tsiemene::PayloadKind> parse_kind_ref(std::string s);

namespace {

std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::optional<std::uint64_t> parse_u64_strict_(std::string_view s) {
  std::uint64_t out = 0;
  const auto* b = s.data();
  const auto* e = s.data() + s.size();
  const auto r = std::from_chars(b, e, out);
  if (r.ec != std::errc{} || r.ptr != e) return std::nullopt;
  return out;
}

[[nodiscard]] std::optional<std::int64_t> parse_i64_strict_(std::string_view s) {
  std::int64_t out = 0;
  const auto* b = s.data();
  const auto* e = s.data() + s.size();
  const auto r = std::from_chars(b, e, out);
  if (r.ec != std::errc{} || r.ptr != e) return std::nullopt;
  return out;
}

[[nodiscard]] std::string to_lower_ascii_(std::string s) {
  for (char& c : s) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return s;
}

[[nodiscard]] constexpr bool is_leap_year_(int y) noexcept {
  return ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
}

[[nodiscard]] constexpr int days_in_month_(int y, int m) noexcept {
  constexpr int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 0;
  if (m == 2 && is_leap_year_(y)) return 29;
  return kDays[m - 1];
}

// Whole UTC days since unix epoch (1970-01-01).
[[nodiscard]] constexpr std::int64_t days_from_civil_utc_(int y, unsigned m, unsigned d) noexcept {
  y -= (m <= 2) ? 1 : 0;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned mp = (m > 2) ? (m - 3) : (m + 9);
  const unsigned doy = (153 * mp + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<std::int64_t>(era) * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

[[nodiscard]] std::optional<std::int64_t>
parse_ddmmyyyy_to_unix_ms_(std::string_view ddmmyyyy, bool end_of_day) {
  const std::size_t p1 = ddmmyyyy.find('.');
  if (p1 == std::string_view::npos) return std::nullopt;
  const std::size_t p2 = ddmmyyyy.find('.', p1 + 1);
  if (p2 == std::string_view::npos) return std::nullopt;

  const auto d = parse_i64_strict_(ddmmyyyy.substr(0, p1));
  const auto m = parse_i64_strict_(ddmmyyyy.substr(p1 + 1, p2 - p1 - 1));
  const auto y = parse_i64_strict_(ddmmyyyy.substr(p2 + 1));
  if (!d || !m || !y) return std::nullopt;
  if (*y < 1970 || *m < 1 || *m > 12) return std::nullopt;

  const int dim = days_in_month_(static_cast<int>(*y), static_cast<int>(*m));
  if (*d < 1 || *d > dim) return std::nullopt;

  constexpr std::int64_t kMsPerDay = 24LL * 60LL * 60LL * 1000LL;
  const std::int64_t day_index = days_from_civil_utc_(
      static_cast<int>(*y),
      static_cast<unsigned>(*m),
      static_cast<unsigned>(*d));
  if (day_index < 0) return std::nullopt;

  const std::int64_t day_start_ms = day_index * kMsPerDay;
  return day_start_ms + (end_of_day ? (kMsPerDay - 1) : 0);
}

[[nodiscard]] std::string extract_symbol_from_command_(std::string cmd) {
  cmd = trim_ascii_ws_copy(std::move(cmd));
  if (cmd.empty()) return {};
  if (cmd.rfind("batches=", 0) == 0) return {};
  const std::size_t lb = cmd.find('[');
  if (lb == std::string::npos) return cmd;
  return trim_ascii_ws_copy(cmd.substr(0, lb));
}

bool parse_wave_envelope_(std::string payload,
                          tsiemene_wave_invoke_t* out,
                          std::string* error) {
  if (!out) return false;
  *out = tsiemene_wave_invoke_t{};

  payload = trim_ascii_ws_copy(std::move(payload));
  if (payload.empty()) {
    if (error) *error = "empty circuit invoke payload";
    return false;
  }

  constexpr std::string_view kPrefix = "wave@";
  if (payload.rfind(std::string(kPrefix), 0) != 0) {
    out->source_command = payload;
    out->source_symbol = extract_symbol_from_command_(out->source_command);
    return true;
  }

  const std::string rest = payload.substr(kPrefix.size());
  const std::size_t sep = rest.find('@');
  if (sep == std::string::npos) {
    if (error) {
      *error = "wave invoke envelope missing source command separator '@': " + payload;
    }
    return false;
  }

  const std::string meta = trim_ascii_ws_copy(rest.substr(0, sep));
  const std::string source_command = trim_ascii_ws_copy(rest.substr(sep + 1));
  if (source_command.empty()) {
    if (error) *error = "wave invoke envelope has empty source command";
    return false;
  }

  bool has_wave_i = false;
  bool has_from = false;
  bool has_to = false;
  std::int64_t span_from_ms = 0;
  std::int64_t span_to_ms = 0;

  std::size_t i = 0;
  while (i < meta.size()) {
    std::size_t j = i;
    while (j < meta.size() && meta[j] != ',') ++j;
    std::string item = trim_ascii_ws_copy(meta.substr(i, j - i));
    if (item.empty()) {
      if (error) *error = "empty wave invoke metadata token";
      return false;
    }

    const std::size_t c = item.find(':');
    if (c == std::string::npos) {
      if (error) *error = "invalid wave invoke metadata token (expected key:value): " + item;
      return false;
    }
    std::string key = to_lower_ascii_(trim_ascii_ws_copy(item.substr(0, c)));
    std::string val = trim_ascii_ws_copy(item.substr(c + 1));
    if (key.empty() || val.empty()) {
      if (error) *error = "invalid empty key/value in wave invoke metadata token: " + item;
      return false;
    }

    if (key == "symbol") {
      out->source_symbol = val;
    } else if (key == "episode") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) *error = "invalid episode value in wave invoke metadata: " + val;
        return false;
      }
      out->episode = *v;
    } else if (key == "batch") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) *error = "invalid batch value in wave invoke metadata: " + val;
        return false;
      }
      out->batch = *v;
    } else if (key == "i") {
      const auto v = parse_u64_strict_(val);
      if (!v) {
        if (error) *error = "invalid i value in wave invoke metadata: " + val;
        return false;
      }
      out->wave_i = *v;
      has_wave_i = true;
    } else if (key == "from") {
      const auto v = parse_ddmmyyyy_to_unix_ms_(val, /*end_of_day=*/false);
      if (!v) {
        if (error) *error = "invalid from date (expected dd.mm.yyyy): " + val;
        return false;
      }
      span_from_ms = *v;
      has_from = true;
    } else if (key == "to") {
      const auto v = parse_ddmmyyyy_to_unix_ms_(val, /*end_of_day=*/true);
      if (!v) {
        if (error) *error = "invalid to date (expected dd.mm.yyyy): " + val;
        return false;
      }
      span_to_ms = *v;
      has_to = true;
    } else if (key == "from_ms") {
      const auto v = parse_i64_strict_(val);
      if (!v) {
        if (error) *error = "invalid from_ms value in wave invoke metadata: " + val;
        return false;
      }
      if (*v < 0) {
        if (error) *error = "from_ms must be >= 0 in wave invoke metadata: " + val;
        return false;
      }
      span_from_ms = *v;
      has_from = true;
    } else if (key == "to_ms") {
      const auto v = parse_i64_strict_(val);
      if (!v) {
        if (error) *error = "invalid to_ms value in wave invoke metadata: " + val;
        return false;
      }
      if (*v < 0) {
        if (error) *error = "to_ms must be >= 0 in wave invoke metadata: " + val;
        return false;
      }
      span_to_ms = *v;
      has_to = true;
    } else {
      if (error) *error = "unknown wave invoke metadata key: " + key;
      return false;
    }

    i = (j < meta.size()) ? (j + 1) : j;
  }

  if (has_from != has_to) {
    if (error) {
      *error = "wave invoke metadata requires both from/to (or from_ms/to_ms) when one is provided";
    }
    return false;
  }

  out->source_command = source_command;
  if (out->source_symbol.empty()) {
    out->source_symbol = extract_symbol_from_command_(source_command);
  }
  if (!has_wave_i) out->wave_i = out->batch;
  if (has_from && has_to) {
    out->has_time_span = true;
    out->span_begin_ms = std::min(span_from_ms, span_to_ms);
    out->span_end_ms = std::max(span_from_ms, span_to_ms);
  }
  return true;
}

bool build_alias_type_map(
    const tsiemene_circuit_decl_t& circuit,
    std::unordered_map<std::string, tsiemene::TsiTypeId>* out,
    std::string* error) {
  if (!out) return false;
  out->clear();
  out->reserve(circuit.instances.size());
  std::array<std::size_t, tsiemene::kTsiTypeRegistry.size()> instance_counts{};

  for (const auto& inst : circuit.instances) {
    const std::string alias = trim_ascii_ws_copy(inst.alias);
    const std::string type = trim_ascii_ws_copy(inst.tsi_type);
    if (alias.empty()) {
      if (error) *error = "empty instance alias";
      return false;
    }
    if (type.empty()) {
      if (error) *error = "empty tsi_type for alias: " + alias;
      return false;
    }

    const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(type);
    if (!type_path.ok) {
      if (error) *error = "invalid tsi_type canonical path for alias " + alias + ": " + type_path.error;
      return false;
    }
    if (type_path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
      if (error) *error = "tsi_type must be canonical node path for alias " + alias + ": " + type_path.canonical;
      return false;
    }

    const auto type_id = tsiemene::parse_tsi_type_id(type_path.canonical_identity);
    if (!type_id) {
      if (error) *error = "unsupported tsi_type for alias " + alias + ": " + type_path.canonical_identity;
      return false;
    }
    const std::size_t type_index = tsiemene::tsi_type_index(*type_id);
    ++instance_counts[type_index];
    if (tsiemene::is_unique_instance_type(*type_id) && instance_counts[type_index] > 1) {
      if (error) {
        *error = "tsi_type must be unique per circuit: " +
                 std::string(tsiemene::tsi_type_token(*type_id)) +
                 " (alias: " + alias + ")";
      }
      return false;
    }
    if (!out->emplace(alias, *type_id).second) {
      if (error) *error = "duplicated instance alias: " + alias;
      return false;
    }
  }
  return true;
}

bool resolve_hop_decl_with_types(
    const tsiemene_hop_decl_t& hop,
    const std::unordered_map<std::string, tsiemene::TsiTypeId>& alias_to_type,
    tsiemene_resolved_hop_t* out,
    std::string* error) {
  if (!out) return false;

  const std::string from_instance = trim_ascii_ws_copy(hop.from.instance);
  const std::string to_instance = trim_ascii_ws_copy(hop.to.instance);
  const std::string from_dir_text = trim_ascii_ws_copy(hop.from.directive);
  const std::string from_kind_text = trim_ascii_ws_copy(hop.from.kind);
  const std::string to_dir_text = trim_ascii_ws_copy(hop.to.directive);
  const std::string to_kind_text = trim_ascii_ws_copy(hop.to.kind);

  const auto from_it = alias_to_type.find(from_instance);
  if (from_it == alias_to_type.end()) {
    if (error) *error = "hop references unknown instance alias: " + from_instance;
    return false;
  }
  const auto to_it = alias_to_type.find(to_instance);
  if (to_it == alias_to_type.end()) {
    if (error) *error = "hop references unknown instance alias: " + to_instance;
    return false;
  }

  const auto from_dir = parse_directive_ref(from_dir_text);
  const auto from_kind = parse_kind_ref(from_kind_text);
  if (!from_dir || !from_kind) {
    if (error) {
      *error = "invalid directive/kind in hop: " +
               from_instance + "@" + from_dir_text + ":" + from_kind_text +
               " -> " + to_instance;
    }
    return false;
  }

  if (!tsiemene::type_emits_output(from_it->second, *from_dir, *from_kind)) {
    if (error) {
      *error = "hop source endpoint is not an output of source tsi type: " +
               from_instance + std::string(*from_dir) + std::string(tsiemene::kind_token(*from_kind)) +
               " for type " + std::string(tsiemene::tsi_type_token(from_it->second));
    }
    return false;
  }

  if (to_dir_text.empty()) {
    if (error) {
      *error = "missing target input directive in hop: " +
               from_instance + std::string(*from_dir) + std::string(tsiemene::kind_token(*from_kind)) +
               " -> " +
               to_instance;
    }
    return false;
  }
  if (!to_kind_text.empty()) {
    if (error) {
      *error = "target kind cast is not allowed in hop: " +
               from_instance + std::string(*from_dir) + std::string(tsiemene::kind_token(*from_kind)) +
               " -> " + to_instance + "@" + to_dir_text + ":" + to_kind_text +
               " (use target inbound directive only; kind is inferred from source)";
    }
    return false;
  }
  const auto to_dir = parse_directive_ref(to_dir_text);
  if (!to_dir) {
    if (error) {
      *error = "invalid target directive in hop: " + to_instance + "@" + to_dir_text;
    }
    return false;
  }

  if (!tsiemene::type_is_compatible(to_it->second, *to_dir, *from_kind)) {
    if (error) {
      *error = "hop target endpoint is not an input of target tsi type: " +
               to_instance + std::string(*to_dir) + std::string(tsiemene::kind_token(*from_kind)) +
               " for type " + std::string(tsiemene::tsi_type_token(to_it->second));
    }
    return false;
  }

  out->from.instance = from_instance;
  out->from.directive = *from_dir;
  out->from.kind = *from_kind;

  out->to.instance = to_instance;
  out->to.directive = *to_dir;
  out->to.kind = *from_kind;
  return true;
}

} /* namespace */

std::optional<tsiemene::DirectiveId> parse_directive_ref(std::string s) {
  return tsiemene::parse_directive_id(std::move(s));
}

std::optional<tsiemene::PayloadKind> parse_kind_ref(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (s == "tensor" || s == ":tensor") return tsiemene::PayloadKind::Tensor;
  if (s == "str" || s == ":str") return tsiemene::PayloadKind::String;
  return std::nullopt;
}

bool parse_circuit_invoke_wave(const tsiemene_circuit_decl_t& circuit,
                               tsiemene_wave_invoke_t* out,
                               std::string* error) {
  return parse_wave_envelope_(circuit.invoke_payload, out, error);
}

std::string circuit_invoke_command(const tsiemene_circuit_decl_t& circuit) {
  tsiemene_wave_invoke_t parsed{};
  std::string local_error;
  if (!parse_circuit_invoke_wave(circuit, &parsed, &local_error)) return {};
  return parsed.source_command;
}

std::string circuit_invoke_symbol(const tsiemene_circuit_decl_t& circuit) {
  tsiemene_wave_invoke_t parsed{};
  std::string local_error;
  if (!parse_circuit_invoke_wave(circuit, &parsed, &local_error)) return {};
  return parsed.source_symbol;
}

bool resolve_hops(
    const tsiemene_circuit_decl_t& circuit,
    std::vector<tsiemene_resolved_hop_t>* out_hops,
    std::string* error) {
  if (!out_hops) return false;
  std::unordered_map<std::string, tsiemene::TsiTypeId> alias_to_type;
  if (!build_alias_type_map(circuit, &alias_to_type, error)) return false;

  out_hops->clear();
  out_hops->reserve(circuit.hops.size());

  for (const auto& h : circuit.hops) {
    tsiemene_resolved_hop_t resolved{};
    if (!resolve_hop_decl_with_types(h, alias_to_type, &resolved, error)) return false;
    out_hops->push_back(std::move(resolved));
  }

  return true;
}

bool resolve_hop_decl(const tsiemene_hop_decl_t& hop,
                      tsiemene_resolved_hop_t* out,
                      std::string* error) {
  if (!out) return false;

  const std::string from_dir_text = trim_ascii_ws_copy(hop.from.directive);
  const std::string from_kind_text = trim_ascii_ws_copy(hop.from.kind);
  const std::string to_dir_text = trim_ascii_ws_copy(hop.to.directive);
  const std::string to_kind_text = trim_ascii_ws_copy(hop.to.kind);

  const auto from_dir = parse_directive_ref(hop.from.directive);
  const auto from_kind = parse_kind_ref(hop.from.kind);
  if (!from_dir || !from_kind) {
    if (error) {
      *error = "invalid directive/kind in hop: " +
               hop.from.instance + "@" + from_dir_text + ":" + from_kind_text +
               " -> " + hop.to.instance;
    }
    return false;
  }

  if (to_dir_text.empty()) {
    if (error) {
      *error = "missing target input directive in hop: " +
               hop.from.instance + "@" + from_dir_text + ":" + from_kind_text +
               " -> " + hop.to.instance;
    }
    return false;
  }
  if (!to_kind_text.empty()) {
    if (error) {
      *error = "target kind cast is not allowed in hop: " +
               hop.from.instance + "@" + from_dir_text + ":" + from_kind_text + " -> " +
               hop.to.instance + "@" + to_dir_text + ":" + to_kind_text;
    }
    return false;
  }
  const auto to_dir = parse_directive_ref(to_dir_text);
  if (!to_dir) {
    if (error) {
      *error = "invalid target directive in hop: " + hop.to.instance + "@" + to_dir_text;
    }
    return false;
  }

  out->from.instance = hop.from.instance;
  out->from.directive = *from_dir;
  out->from.kind = *from_kind;

  out->to.instance = hop.to.instance;
  out->to.directive = *to_dir;
  out->to.kind = *from_kind;
  return true;
}

bool validate_circuit_decl(const tsiemene_circuit_decl_t& circuit,
                           std::string* error) {
  const std::string circuit_name = trim_ascii_ws_copy(circuit.name);
  if (circuit_name.empty()) {
    if (error) *error = "empty circuit name";
    return false;
  }
  if (trim_ascii_ws_copy(circuit.invoke_name).empty()) {
    if (error) *error = "empty circuit invoke name";
    return false;
  }
  if (trim_ascii_ws_copy(circuit.invoke_payload).empty()) {
    if (error) *error = "empty circuit invoke payload";
    return false;
  }

  tsiemene_wave_invoke_t parsed_invoke{};
  std::string invoke_error;
  if (!parse_circuit_invoke_wave(circuit, &parsed_invoke, &invoke_error)) {
    if (error) *error = "invalid circuit invoke payload: " + invoke_error;
    return false;
  }
  if (parsed_invoke.source_command.empty()) {
    if (error) *error = "empty source command in circuit invoke payload";
    return false;
  }
  if (circuit.instances.empty()) {
    if (error) *error = "circuit has no instance declarations";
    return false;
  }
  if (circuit.hops.empty()) {
    if (error) *error = "circuit has no hop declarations";
    return false;
  }

  std::unordered_map<std::string, tsiemene::TsiTypeId> alias_to_type;
  if (!build_alias_type_map(circuit, &alias_to_type, error)) return false;

  std::vector<tsiemene_resolved_hop_t> resolved_hops;
  if (!resolve_hops(circuit, &resolved_hops, error)) return false;

  std::unordered_map<std::string, std::vector<std::string>> adj;
  std::unordered_map<std::string, std::size_t> in_degree;
  std::unordered_map<std::string, std::size_t> out_degree;
  std::unordered_set<std::string> referenced;
  referenced.reserve(circuit.instances.size());

  for (const auto& h : resolved_hops) {
    if (alias_to_type.find(h.from.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.from.instance;
      return false;
    }
    if (alias_to_type.find(h.to.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.to.instance;
      return false;
    }
    referenced.insert(h.from.instance);
    referenced.insert(h.to.instance);
    adj[h.from.instance].push_back(h.to.instance);
    (void)adj[h.to.instance];
    ++in_degree[h.to.instance];
    (void)in_degree[h.from.instance];
    ++out_degree[h.from.instance];
    (void)out_degree[h.to.instance];
  }

  if (referenced.empty()) {
    if (error) *error = "no valid hop endpoints";
    return false;
  }

  for (const auto& [alias, _] : alias_to_type) {
    if (!referenced.count(alias)) {
      if (error) *error = "orphan instance not referenced by any hop: " + alias;
      return false;
    }
  }

  std::vector<std::string> roots;
  roots.reserve(referenced.size());
  for (const auto& alias : referenced) {
    const auto in_it = in_degree.find(alias);
    const std::size_t id = (in_it == in_degree.end()) ? 0 : in_it->second;
    if (id == 0) roots.push_back(alias);
  }

  if (roots.empty()) {
    if (error) *error = "circuit has no root instance";
    return false;
  }
  if (roots.size() != 1) {
    if (error) *error = "circuit must have exactly one root instance";
    return false;
  }

  std::unordered_map<std::string, int> color;
  std::unordered_set<std::string> reachable;
  bool cycle = false;

  std::function<void(const std::string&)> dfs = [&](const std::string& u) {
    if (cycle) return;
    color[u] = 1;
    reachable.insert(u);
    const auto it = adj.find(u);
    if (it != adj.end()) {
      for (const std::string& v : it->second) {
        const int state = color[v];
        if (state == 1) {
          cycle = true;
          return;
        }
        if (state == 0) dfs(v);
        if (cycle) return;
      }
    }
    color[u] = 2;
  };

  dfs(roots[0]);
  if (cycle) {
    if (error) *error = "cycle detected in circuit hops";
    return false;
  }

  if (reachable.size() != referenced.size()) {
    if (error) *error = "unreachable instance from circuit root";
    return false;
  }

  for (const auto& alias : referenced) {
    const auto od_it = out_degree.find(alias);
    const std::size_t od = (od_it == out_degree.end()) ? 0 : od_it->second;
    if (od != 0) continue;
    const auto type_it = alias_to_type.find(alias);
    if (type_it == alias_to_type.end()) {
      if (error) *error = "internal semantic error resolving type for alias: " + alias;
      return false;
    }
    if (!tsiemene::is_sink_type(type_it->second)) {
      if (error) {
        *error = "terminal instance must be sink type: " + alias + "=" +
                 std::string(tsiemene::tsi_type_token(type_it->second));
      }
      return false;
    }
  }

  return true;
}

bool validate_circuit_instruction(const tsiemene_circuit_instruction_t& circuit_instruction,
                                  std::string* error) {
  if (circuit_instruction.circuits.empty()) {
    if (error) *error = "circuit instruction has no circuits";
    return false;
  }

  std::unordered_set<std::string> circuit_names;
  std::unordered_set<std::string> invoke_names;
  circuit_names.reserve(circuit_instruction.circuits.size());
  invoke_names.reserve(circuit_instruction.circuits.size());

  for (std::size_t i = 0; i < circuit_instruction.circuits.size(); ++i) {
    const auto& c = circuit_instruction.circuits[i];
    const std::string cname = trim_ascii_ws_copy(c.name);
    const std::string iname = trim_ascii_ws_copy(c.invoke_name);
    if (!circuit_names.insert(cname).second) {
      if (error) *error = "duplicated circuit name: " + cname;
      return false;
    }
    if (!invoke_names.insert(iname).second) {
      if (error) *error = "duplicated circuit invoke name: " + iname;
      return false;
    }

    std::string local_error;
    if (!validate_circuit_decl(c, &local_error)) {
      if (error) *error = "circuit[" + std::to_string(i) + "] " + local_error;
      return false;
    }
  }

  return true;
}

std::string tsiemene_circuit_instruction_t::str() const {
  std::ostringstream oss;
  oss << "tsiemene_circuit_instruction_t: circuits=" << circuits.size() << "\n";
  for (std::size_t i = 0; i < circuits.size(); ++i) {
    const auto& c = circuits[i];
    oss << "  [" << i << "] " << c.name
        << " instances=" << c.instances.size()
        << " hops=" << c.hops.size()
        << " invoke=" << c.invoke_name
        << "(\"" << c.invoke_payload << "\")\n";
  }
  return oss.str();
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

namespace {

std::string unescape_like_parser(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (std::size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
        case 'n': result += '\n'; ++i; break;
        case 'r': result += '\r'; ++i; break;
        case 't': result += '\t'; ++i; break;
        case '\\': result += '\\'; ++i; break;
        case '"': result += '"'; ++i; break;
        case '\'': result += '\''; ++i; break;
        default:
          result += '\\';
          result += str[i + 1];
          ++i;
          break;
      }
    } else {
      result += str[i];
    }
  }
  return result;
}

std::string terminal_text_from_unit(const ProductionUnit& unit) {
  std::string lex = unit.lexeme;
  if (lex.size() >= 2 &&
      ((lex.front() == '"' && lex.back() == '"') ||
       (lex.front() == '\'' && lex.back() == '\''))) {
    lex = lex.substr(1, lex.size() - 2);
  }
  return unescape_like_parser(lex);
}

void append_all_terminals(const ASTNode* node, std::string& out) {
  if (!node) return;

  if (const auto* term = dynamic_cast<const TerminalNode*>(node)) {
    if (term->unit.type == ProductionUnit::Type::Terminal) {
      out += terminal_text_from_unit(term->unit);
    }
    return;
  }

  if (const auto* root = dynamic_cast<const RootNode*>(node)) {
    for (const auto& ch : root->children) append_all_terminals(ch.get(), out);
    return;
  }

  if (const auto* mid = dynamic_cast<const IntermediaryNode*>(node)) {
    for (const auto& ch : mid->children) append_all_terminals(ch.get(), out);
    return;
  }
}

std::string flatten_node_text(const ASTNode* node) {
  std::string out;
  append_all_terminals(node, out);
  return out;
}

std::string trim_ascii_ws(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

std::string normalize_line(std::string s) {
  for (char& c : s) {
    if (c == '\r' || c == '\n') c = ' ';
  }
  return trim_ascii_ws(std::move(s));
}

const ASTNode* find_direct_child_by_hash(const IntermediaryNode* parent, std::size_t wanted_hash) {
  if (!parent) return nullptr;
  for (const auto& ch : parent->children) {
    if (ch && ch->hash == wanted_hash) return ch.get();
  }
  return nullptr;
}

bool parse_endpoint_text(const std::string& endpoint_text,
                         cuwacunu::camahjucunu::tsiemene_endpoint_t* out,
                         bool require_kind) {
  if (!out) return false;
  std::string line = normalize_line(endpoint_text);
  const std::size_t at = line.find('@');
  if (at == std::string::npos || at == 0 || at + 1 >= line.size()) {
    return false;
  }
  if (require_kind) {
    const std::size_t colon = line.rfind(':');
    if (colon == std::string::npos || colon <= at + 1 || colon + 1 >= line.size()) {
      return false;
    }
    out->instance = trim_ascii_ws(line.substr(0, at));
    out->directive = trim_ascii_ws(line.substr(at + 1, colon - (at + 1)));
    out->kind = trim_ascii_ws(line.substr(colon + 1));
    return !(out->instance.empty() || out->directive.empty() || out->kind.empty());
  }

  if (line.find(':', at + 1) != std::string::npos) return false;
  out->instance = trim_ascii_ws(line.substr(0, at));
  out->directive = trim_ascii_ws(line.substr(at + 1));
  out->kind.clear();
  return !(out->instance.empty() || out->directive.empty());
}

bool parse_instance_decl_text(const std::string& decl_text, cuwacunu::camahjucunu::tsiemene_instance_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) return false;
  out->alias = trim_ascii_ws(line.substr(0, eq));
  out->tsi_type = trim_ascii_ws(line.substr(eq + 1));
  return !(out->alias.empty() || out->tsi_type.empty());
}

bool parse_hop_decl_text(const std::string& decl_text, cuwacunu::camahjucunu::tsiemene_hop_decl_t* out) {
  if (!out) return false;
  std::string line = normalize_line(decl_text);
  const std::size_t arrow = line.find("->");
  if (arrow == std::string::npos || arrow == 0 || arrow + 2 >= line.size()) return false;
  const std::string lhs = trim_ascii_ws(line.substr(0, arrow));
  const std::string rhs = trim_ascii_ws(line.substr(arrow + 2));
  if (!parse_endpoint_text(lhs, &out->from, /*require_kind=*/true)) return false;
  if (!parse_endpoint_text(rhs, &out->to, /*require_kind=*/false)) return false;
  return true;
}

std::string expand_implicit_hop_targets(std::string instruction) {
  // Target shorthand inference is removed; keep decode path passthrough.
  return instruction;
}

bool parse_circuit_header_text(const std::string& header_text, std::string* out_name) {
  if (!out_name) return false;
  std::string line = normalize_line(header_text);
  const std::size_t eq = line.find('=');
  if (eq == std::string::npos || eq == 0) return false;
  *out_name = trim_ascii_ws(line.substr(0, eq));
  return !out_name->empty();
}

bool parse_circuit_invoke_text(const std::string& invoke_text, std::string* out_name, std::string* out_payload) {
  if (!out_name || !out_payload) return false;
  std::string line = normalize_line(invoke_text);
  if (!line.empty() && line.back() == ';') {
    line.pop_back();
    line = trim_ascii_ws(std::move(line));
  }
  const std::size_t lp = line.find('(');
  const std::size_t rp = line.rfind(')');
  if (lp == std::string::npos || rp == std::string::npos || lp == 0 || rp <= lp) return false;
  *out_name = trim_ascii_ws(line.substr(0, lp));
  *out_payload = trim_ascii_ws(line.substr(lp + 1, rp - (lp + 1)));
  return !out_name->empty();
}

bool parse_instruction_text_fallback(
    const std::string& instruction,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t* out,
    std::string* error = nullptr) {
  if (!out) return false;
  if (error) error->clear();
  out->circuits.clear();

  std::istringstream iss(instruction);
  std::string raw_line;
  std::size_t line_no = 0;
  bool in_circuit_block = false;
  std::optional<std::size_t> pending_invoke_index{};
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t current{};

  auto fail = [&](std::size_t at_line,
                  std::string_view reason,
                  std::string_view line_text = {}) {
    if (error) {
      std::ostringstream oss;
      oss << "fallback parser: " << reason;
      if (at_line > 0) oss << " at line " << at_line;
      if (!line_text.empty()) oss << ": " << line_text;
      *error = oss.str();
    }
    return false;
  };

  auto assign_invoke = [&](std::string invoke_name, std::string invoke_payload) {
    if (pending_invoke_index.has_value() && *pending_invoke_index < out->circuits.size()) {
      auto& c = out->circuits[*pending_invoke_index];
      c.invoke_name = std::move(invoke_name);
      c.invoke_payload = std::move(invoke_payload);
      pending_invoke_index.reset();
      return true;
    }
    for (auto& c : out->circuits) {
      if (c.name == invoke_name) {
        c.invoke_name = std::move(invoke_name);
        c.invoke_payload = std::move(invoke_payload);
        return true;
      }
    }
    return false;
  };

  while (std::getline(iss, raw_line)) {
    ++line_no;
    std::string line = trim_ascii_ws(raw_line);
    if (line.empty()) continue;
    if (line.rfind("//", 0) == 0 || line.rfind('#', 0) == 0) continue;

    if (in_circuit_block) {
      if (line == "}") {
        out->circuits.push_back(std::move(current));
        current = {};
        in_circuit_block = false;
        pending_invoke_index = out->circuits.empty() ? std::optional<std::size_t>{}
                                                     : std::optional<std::size_t>{out->circuits.size() - 1u};
        continue;
      }

      cuwacunu::camahjucunu::tsiemene_hop_decl_t hop{};
      if (parse_hop_decl_text(line, &hop)) {
        current.hops.push_back(std::move(hop));
        continue;
      }

      cuwacunu::camahjucunu::tsiemene_instance_decl_t inst{};
      if (parse_instance_decl_text(line, &inst)) {
        current.instances.push_back(std::move(inst));
        continue;
      }

      return fail(line_no, "unrecognized statement in circuit block", line);
    }

    std::string circuit_name;
    if (parse_circuit_header_text(line, &circuit_name)) {
      current = {};
      current.name = std::move(circuit_name);
      in_circuit_block = true;
      pending_invoke_index.reset();
      continue;
    }

    std::string invoke_name;
    std::string invoke_payload;
    if (parse_circuit_invoke_text(line, &invoke_name, &invoke_payload)) {
      if (!assign_invoke(std::move(invoke_name), std::move(invoke_payload))) {
        return fail(line_no, "invoke target does not match any declared circuit", line);
      }
      continue;
    }

    return fail(line_no, "unrecognized statement at board scope", line);
  }

  if (in_circuit_block) return fail(line_no, "unterminated circuit block");
  for (auto& c : out->circuits) {
    if (c.name.empty()) continue;
    if (c.invoke_name.empty()) c.invoke_name = c.name;
  }
  if (out->circuits.empty()) return fail(0, "no circuits decoded");
  return true;
}

cuwacunu::camahjucunu::tsiemene_circuit_decl_t parse_circuit_node(const IntermediaryNode* node) {
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t out{};
  if (!node) return out;

  if (const ASTNode* n_header = find_direct_child_by_hash(node, TSIEMENE_CIRCUIT_HASH_circuit_header)) {
    const auto* im = dynamic_cast<const IntermediaryNode*>(n_header);
    if (im) {
      if (const ASTNode* n_name = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_circuit_name)) {
        out.name = trim_ascii_ws(flatten_node_text(n_name));
      }
    }
    if (out.name.empty()) {
      parse_circuit_header_text(flatten_node_text(n_header), &out.name);
    }
  }

  for (const auto& ch : node->children) {
    const auto* im = dynamic_cast<const IntermediaryNode*>(ch.get());
    if (!im) continue;

    if (im->hash == TSIEMENE_CIRCUIT_HASH_instance_decl) {
      cuwacunu::camahjucunu::tsiemene_instance_decl_t inst{};
      const ASTNode* n_alias = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_instance_alias);
      const ASTNode* n_type = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_tsi_type);
      if (n_alias && n_type) {
        inst.alias = trim_ascii_ws(flatten_node_text(n_alias));
        inst.tsi_type = trim_ascii_ws(flatten_node_text(n_type));
      } else {
        parse_instance_decl_text(flatten_node_text(im), &inst);
      }
      if (!inst.alias.empty() && !inst.tsi_type.empty()) {
        out.instances.push_back(std::move(inst));
      }
      continue;
    }

    if (im->hash == TSIEMENE_CIRCUIT_HASH_hop_decl) {
      cuwacunu::camahjucunu::tsiemene_hop_decl_t hop{};
      const ASTNode* n_from = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_endpoint_from);
      const ASTNode* n_to = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_endpoint_to);
      bool ok = false;
      if (n_from && n_to) {
        ok = parse_endpoint_text(flatten_node_text(n_from), &hop.from, /*require_kind=*/true) &&
             parse_endpoint_text(flatten_node_text(n_to), &hop.to, /*require_kind=*/false);
      }
      if (!ok) ok = parse_hop_decl_text(flatten_node_text(im), &hop);
      if (ok) out.hops.push_back(std::move(hop));
      continue;
    }

    if (im->hash == TSIEMENE_CIRCUIT_HASH_circuit_invoke) {
      const ASTNode* n_name = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_invoke_name);
      const ASTNode* n_payload = find_direct_child_by_hash(im, TSIEMENE_CIRCUIT_HASH_invoke_payload);
      if (n_name && n_payload) {
        out.invoke_name = trim_ascii_ws(flatten_node_text(n_name));
        out.invoke_payload = trim_ascii_ws(flatten_node_text(n_payload));
      } else {
        parse_circuit_invoke_text(flatten_node_text(im), &out.invoke_name, &out.invoke_payload);
      }
      continue;
    }
  }

  if (out.name.empty()) out.name = out.invoke_name;
  if (out.invoke_name.empty()) out.invoke_name = out.name;
  return out;
}

} /* namespace */

tsiemeneCircuits::tsiemeneCircuits()
  : grammarLexer(TSIEMENE_CIRCUIT_GRAMMAR_TEXT)
  , grammarParser(grammarLexer)
  , grammar(parseGrammarDefinition())
  , iParser(iLexer, grammar)
{
#ifdef TSIEMENE_CIRCUIT_DEBUG
  log_info("%s\n", TSIEMENE_CIRCUIT_GRAMMAR_TEXT);
#endif
}

cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
tsiemeneCircuits::decode(std::string instruction) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  log_info("Request to decode tsiemeneCircuits\n");
#endif
  LOCK_GUARD(current_mutex);

  instruction = expand_implicit_hop_targets(std::move(instruction));

  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
#endif

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));
  actualAST.get()->accept(*this, context);

  bool needs_fallback = current.circuits.empty();
  if (!needs_fallback) {
    for (const auto& c : current.circuits) {
      if (c.hops.empty()) {
        needs_fallback = true;
        break;
      }
    }
  }
  if (needs_fallback) {
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t fallback{};
    std::string fallback_error;
    if (parse_instruction_text_fallback(instruction, &fallback, &fallback_error) &&
        !fallback.circuits.empty()) {
      current = std::move(fallback);
    }
  }
  return current;
}

ProductionGrammar tsiemeneCircuits::parseGrammarDefinition() {
  grammarParser.parseGrammar();
  return grammarParser.getGrammar();
}

void tsiemeneCircuits::visit(const RootNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
  (void)node;
  (void)context;
}

void tsiemeneCircuits::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif
  auto* out = static_cast<cuwacunu::camahjucunu::tsiemene_circuit_instruction_t*>(context.user_data);
  if (!out) return;

  if (node->hash == TSIEMENE_CIRCUIT_HASH_instruction) {
    out->circuits.clear();
    return;
  }
  if (node->hash == TSIEMENE_CIRCUIT_HASH_circuit) {
    auto circuit = parse_circuit_node(node);
    if (!circuit.name.empty()) {
      out->circuits.push_back(std::move(circuit));
    }
    return;
  }
}

void tsiemeneCircuits::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef TSIEMENE_CIRCUIT_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif
  (void)node;
  (void)context;
}

} /* namespace dsl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
