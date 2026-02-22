/* canonical_path.cpp */
#include "camahjucunu/BNF/implementations/canonical_path/canonical_path.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "piaabo/dconfig.h"

namespace cuwacunu {
namespace camahjucunu {

namespace {

constexpr std::uint64_t kFnv64Offset = 14695981039346656037ull;
constexpr std::uint64_t kFnv64Prime = 1099511628211ull;
constexpr std::array<std::string_view, 4> kHashFamilies = {
    "lion", "agile", "dephi", "glowie"
};
constexpr std::array<std::string_view, 16> kHashQualia = {
    "_the_anchor", "_the_berry", "_the_crawer", "_the_drifter",
    "_the_echo", "_the_flrying", "_the_gliph", "_the_halo",
    "_the_ibuggy", "_the_jogger", "_the_knotch", "_the_locuas",
    "_the_mantic", "_the_n2courius", "_the_obionekenobi", "_the_pioneer"
};

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string lower_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

[[nodiscard]] bool starts_with(std::string_view s, std::string_view pfx) {
  return s.size() >= pfx.size() && s.substr(0, pfx.size()) == pfx;
}

[[nodiscard]] bool is_atom_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

[[nodiscard]] bool is_valid_atom(std::string_view s) {
  if (s.empty()) return false;
  if (!std::isalpha(static_cast<unsigned char>(s.front())) && s.front() != '_') return false;
  for (char c : s) {
    if (!is_atom_char(c)) return false;
  }
  return true;
}

[[nodiscard]] std::string sanitize_atom(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    out.push_back(is_atom_char(c) ? c : '_');
  }
  if (out.empty()) out = "unknown";
  if (!std::isalpha(static_cast<unsigned char>(out.front())) && out.front() != '_') {
    out.insert(out.begin(), '_');
  }
  return out;
}

[[nodiscard]] std::string sanitize_value(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    const bool keep = std::isalnum(static_cast<unsigned char>(c))
                      || c == '_' || c == '.' || c == '-' || c == ':' || c == '/' || c == '@';
    out.push_back(keep ? c : '_');
  }
  return out.empty() ? std::string("empty") : out;
}

[[nodiscard]] std::vector<std::string> split_dot(std::string_view s) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (pos <= s.size()) {
    const std::size_t dot = s.find('.', pos);
    if (dot == std::string_view::npos) {
      out.emplace_back(s.substr(pos));
      break;
    }
    out.emplace_back(s.substr(pos, dot - pos));
    pos = dot + 1;
  }
  return out;
}

[[nodiscard]] std::string join_dot(const std::vector<std::string>& parts) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) oss << '.';
    oss << parts[i];
  }
  return oss.str();
}

[[nodiscard]] std::uint64_t fnv1a64(std::string_view s) {
  std::uint64_t h = kFnv64Offset;
  for (unsigned char c : s) {
    h ^= static_cast<std::uint64_t>(c);
    h *= kFnv64Prime;
  }
  return h;
}

[[nodiscard]] std::string hex64(std::uint64_t v) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << v;
  return oss.str();
}

struct hash_name_registry_t {
  std::mutex mu{};
  std::unordered_map<std::string, std::string> key_to_name{};
  std::unordered_map<std::string, std::string> name_to_key{};
};

[[nodiscard]] hash_name_registry_t& hash_name_registry() {
  static hash_name_registry_t reg{};
  return reg;
}

[[nodiscard]] std::string make_hash_name_from_seed(std::uint64_t seed) {
  const std::uint64_t idx = seed & 0x3full;  // 6-bit bucket -> 64 names
  const std::size_t fam = static_cast<std::size_t>((idx >> 4) & 0x3u);
  const std::size_t qua = static_cast<std::size_t>(idx & 0x0fu);
  return std::string(kHashFamilies[fam]) + std::string(kHashQualia[qua]);
}

[[nodiscard]] std::string assign_hash_name(std::string_view key) {
  auto& reg = hash_name_registry();
  std::lock_guard<std::mutex> lk(reg.mu);

  const std::string key_s(key);
  const auto it_existing = reg.key_to_name.find(key_s);
  if (it_existing != reg.key_to_name.end()) return it_existing->second;

  const std::uint64_t seed0 = fnv1a64(key);
  for (std::uint64_t i = 0; i < 64; ++i) {
    const std::string candidate = make_hash_name_from_seed(seed0 + i);  // hash(.self + 0x0001) probing
    const auto it = reg.name_to_key.find(candidate);
    if (it == reg.name_to_key.end() || it->second == key_s) {
      reg.name_to_key[candidate] = key_s;
      reg.key_to_name[key_s] = candidate;
      return candidate;
    }
  }

  // Fallback beyond 64 concurrent aliases: keep mnemonic base and append short nonce.
  std::uint64_t nonce = 64;
  for (;;) {
    const std::string candidate =
        make_hash_name_from_seed(seed0) + "_x" + hex64(seed0 + nonce).substr(12);
    const auto it = reg.name_to_key.find(candidate);
    if (it == reg.name_to_key.end() || it->second == key_s) {
      reg.name_to_key[candidate] = key_s;
      reg.key_to_name[key_s] = candidate;
      return candidate;
    }
    ++nonce;
  }
}

[[nodiscard]] std::string canonical_kind_token(std::string s) {
  s = lower_copy(trim_ascii_ws_copy(std::move(s)));
  if (s == "str" || s == ":str") return ":str";
  if (s == "tensor" || s == ":tensor") return ":tensor";
  return {};
}

[[nodiscard]] std::string canonical_directive_token(std::string s) {
  s = lower_copy(trim_ascii_ws_copy(std::move(s)));
  if (!s.empty() && s.front() != '@') s.insert(s.begin(), '@');
  if (s == "@payload" || s == "@loss" || s == "@meta") return s;
  return {};
}

struct parsed_endpoint_t {
  bool present{false};
  std::string directive{};
  std::string kind{};
};

[[nodiscard]] std::optional<parsed_endpoint_t> parse_endpoint_suffix(std::string text, std::string* error) {
  text = trim_ascii_ws_copy(std::move(text));
  if (text.empty()) return parsed_endpoint_t{};

  const std::size_t colon = text.rfind(':');
  if (colon == std::string::npos || colon == 0 || colon + 1 >= text.size()) {
    if (error) *error = "endpoint requires @directive:kind";
    return std::nullopt;
  }

  parsed_endpoint_t out{};
  out.present = true;
  out.directive = canonical_directive_token(text.substr(0, colon));
  out.kind = canonical_kind_token(text.substr(colon + 1));
  if (out.directive.empty()) {
    if (error) *error = "invalid directive in endpoint suffix";
    return std::nullopt;
  }
  if (out.kind.empty()) {
    if (error) *error = "invalid kind in endpoint suffix";
    return std::nullopt;
  }
  return out;
}

struct parsed_core_t {
  std::string path_text{};
  std::string args_text{};
  std::string endpoint_text{};
  bool has_call{false};
};

[[nodiscard]] std::optional<parsed_core_t> split_core(std::string input, std::string* error) {
  input = trim_ascii_ws_copy(std::move(input));
  if (input.empty()) {
    if (error) *error = "empty path expression";
    return std::nullopt;
  }

  std::size_t at = std::string::npos;
  int depth = 0;
  for (std::size_t i = 0; i < input.size(); ++i) {
    const char c = input[i];
    if (c == '(') ++depth;
    else if (c == ')') {
      --depth;
      if (depth < 0) {
        if (error) *error = "unbalanced ')'";
        return std::nullopt;
      }
    } else if (c == '@' && depth == 0) {
      at = i;
    }
  }
  if (depth != 0) {
    if (error) *error = "unbalanced parentheses";
    return std::nullopt;
  }

  parsed_core_t out{};
  std::string core = input;
  if (at != std::string::npos) {
    out.endpoint_text = trim_ascii_ws_copy(input.substr(at + 1));
    core = trim_ascii_ws_copy(input.substr(0, at));
  }

  const std::size_t lp = core.find('(');
  if (lp == std::string::npos) {
    out.path_text = trim_ascii_ws_copy(std::move(core));
    if (out.path_text.empty()) {
      if (error) *error = "missing base path";
      return std::nullopt;
    }
    return out;
  }

  int call_depth = 0;
  std::size_t rp = std::string::npos;
  for (std::size_t i = lp; i < core.size(); ++i) {
    if (core[i] == '(') ++call_depth;
    else if (core[i] == ')') {
      --call_depth;
      if (call_depth == 0) {
        rp = i;
        break;
      }
    }
  }
  if (rp == std::string::npos) {
    if (error) *error = "missing ')' for call suffix";
    return std::nullopt;
  }

  const std::string trailing = trim_ascii_ws_copy(core.substr(rp + 1));
  if (!trailing.empty()) {
    if (error) *error = "unexpected trailing text after call";
    return std::nullopt;
  }

  out.has_call = true;
  out.path_text = trim_ascii_ws_copy(core.substr(0, lp));
  out.args_text = trim_ascii_ws_copy(core.substr(lp + 1, rp - lp - 1));
  if (out.path_text.empty()) {
    if (error) *error = "missing callable path";
    return std::nullopt;
  }
  return out;
}

[[nodiscard]] bool parse_args(std::string text, std::vector<canonical_path_arg_t>* out, std::string* error) {
  if (!out) return false;
  out->clear();
  text = trim_ascii_ws_copy(std::move(text));
  if (text.empty()) return true;

  std::size_t pos = 0;
  while (pos <= text.size()) {
    const std::size_t comma = text.find(',', pos);
    const std::string token = trim_ascii_ws_copy(
        comma == std::string::npos ? text.substr(pos) : text.substr(pos, comma - pos));
    if (!token.empty()) {
      const std::size_t eq = token.find('=');
      canonical_path_arg_t arg{};
      if (eq == std::string::npos) {
        arg.key = trim_ascii_ws_copy(token);
      } else {
        arg.key = trim_ascii_ws_copy(token.substr(0, eq));
        arg.value = trim_ascii_ws_copy(token.substr(eq + 1));
      }
      if (!is_valid_atom(arg.key)) {
        if (error) *error = "invalid argument key: " + arg.key;
        return false;
      }
      out->push_back(std::move(arg));
    }
    if (comma == std::string::npos) break;
    pos = comma + 1;
  }
  return true;
}

[[nodiscard]] std::string canonical_args(const std::vector<canonical_path_arg_t>& args) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i > 0) oss << ',';
    oss << args[i].key;
    if (!args[i].value.empty()) {
      oss << '=' << args[i].value;
    }
  }
  return oss.str();
}

[[nodiscard]] bool is_trainable_jkimyei_base(std::string_view base_type) {
  static const std::unordered_set<std::string> kTrainable = {
      "tsi.wikimyei.representation.vicreg",
  };
  return kTrainable.find(std::string(base_type)) != kTrainable.end();
}

[[nodiscard]] bool canonicalize_segments(std::vector<std::string>* segs,
                                         canonical_facet_e* facet,
                                         std::string* hashimyei,
                                         std::string* error) {
  if (!segs || !facet || !hashimyei) return false;
  if (segs->empty()) {
    if (error) *error = "missing path segments";
    return false;
  }

  for (const auto& s : *segs) {
    if (!is_valid_atom(s)) {
      if (error) *error = "invalid path segment: " + s;
      return false;
    }
  }

  const bool root_is_tsi = (*segs)[0] == "tsi";
  const bool root_is_iinuji = (*segs)[0] == "iinuji";
  if (!root_is_tsi && !root_is_iinuji) {
    if (error) *error = "path root must be 'tsi' or 'iinuji'";
    return false;
  }
  if (segs->size() >= 2 && root_is_tsi && (*segs)[1] == "iinuji") {
    if (error) *error = "tsi.iinuji.* is not supported; use iinuji.*";
    return false;
  }

  *facet = canonical_facet_e::none;
  if (!segs->empty() && segs->back() == "jkimyei") {
    *facet = canonical_facet_e::jkimyei;
    segs->pop_back();
  }

  if (segs->size() < 2) {
    if (error) *error = "path requires at least <root>.<namespace>";
    return false;
  }

  if ((*segs)[1] == "wikimyei") {
    if (!root_is_tsi) {
      if (error) *error = "wikimyei paths must be rooted at tsi.wikimyei";
      return false;
    }
    if (segs->size() < 4) {
      if (error) *error = "tsi.wikimyei path requires family and model";
      return false;
    }
    if (segs->size() == 4) {
      segs->push_back("default");
    } else if (segs->size() != 5) {
      if (error) *error = "tsi.wikimyei path accepts family.model.hashimyei";
      return false;
    }

    *hashimyei = (*segs)[4];
    if (*hashimyei == "default") {
      const std::string base_key =
          (*segs)[0] + "." + (*segs)[1] + "." + (*segs)[2] + "." + (*segs)[3] + ".self";
      *hashimyei = assign_hash_name(base_key);
      (*segs)[4] = *hashimyei;
    }
    if (*facet == canonical_facet_e::jkimyei) {
      const std::string base = (*segs)[0] + "." + (*segs)[1] + "." + (*segs)[2] + "." + (*segs)[3];
      if (!is_trainable_jkimyei_base(base)) {
        if (error) *error = "jkimyei facet only valid for trainable tsi.wikimyei types";
        return false;
      }
    }
  } else {
    if (*facet == canonical_facet_e::jkimyei) {
      if (error) *error = "jkimyei facet only valid for tsi.wikimyei paths";
      return false;
    }
  }

  if (*facet == canonical_facet_e::jkimyei) {
    segs->push_back("jkimyei");
  }
  return true;
}

[[nodiscard]] canonical_path_t decode_internal(std::string text) {
  canonical_path_t out{};
  out.raw = std::move(text);

  std::string error;
  const auto core = split_core(out.raw, &error);
  if (!core.has_value()) {
    out.error = error;
    return out;
  }

  auto segs = split_dot(core->path_text);
  if (!canonicalize_segments(&segs, &out.facet, &out.hashimyei, &error)) {
    out.error = error;
    return out;
  }

  out.segments = segs;

  if (core->has_call) {
    if (!parse_args(core->args_text, &out.args, &error)) {
      out.error = error;
      return out;
    }
    out.path_kind = canonical_path_kind_e::Call;
  } else {
    out.path_kind = canonical_path_kind_e::Node;
  }

  const auto endpoint = parse_endpoint_suffix(core->endpoint_text, &error);
  if (!endpoint.has_value()) {
    out.error = error;
    return out;
  }
  if (endpoint->present) {
    out.directive = endpoint->directive;
    out.kind = endpoint->kind;
    if (out.path_kind != canonical_path_kind_e::Call) {
      out.path_kind = canonical_path_kind_e::Endpoint;
    }
  }

  std::ostringstream identity;
  identity << join_dot(out.segments);
  if (core->has_call) {
    identity << "(" << canonical_args(out.args) << ")";
  }
  out.canonical_identity = identity.str();

  if (endpoint->present) {
    out.canonical_endpoint = out.canonical_identity + out.directive + out.kind;
    out.canonical = out.canonical_endpoint;
  } else {
    out.canonical_endpoint.clear();
    out.canonical = out.canonical_identity;
  }

  out.identity_hash_name = assign_hash_name(out.canonical_identity + ".self");
  if (!out.canonical_endpoint.empty()) {
    out.endpoint_hash_name = assign_hash_name(out.canonical_endpoint + ".self");
  }

  out.ok = true;
  return out;
}

}  // namespace

namespace BNF {

canonicalPath::canonicalPath() {
  try {
    CANONICAL_PATH_BNF_GRAMMAR = cuwacunu::piaabo::dconfig::config_space_t::canonical_path_bnf();
  } catch (...) {
    CANONICAL_PATH_BNF_GRAMMAR.clear();
  }
}

canonicalPath::canonicalPath(std::string grammar_text)
    : CANONICAL_PATH_BNF_GRAMMAR(std::move(grammar_text)) {}

canonical_path_t canonicalPath::decode(std::string instruction) const {
  return decode_internal(std::move(instruction));
}

}  // namespace BNF

canonical_path_t decode_canonical_path(const std::string& text) {
  return BNF::canonicalPath().decode(text);
}

bool validate_canonical_path(const canonical_path_t& path, std::string* error) {
  if (!path.ok) {
    if (error) *error = path.error.empty() ? "invalid path" : path.error;
    return false;
  }
  if (path.canonical_identity.empty()) {
    if (error) *error = "missing canonical identity";
    return false;
  }
  if (path.identity_hash_name.empty()) {
    if (error) *error = "missing identity hash";
    return false;
  }
  if (!path.canonical_endpoint.empty() && path.endpoint_hash_name.empty()) {
    if (error) *error = "missing endpoint hash";
    return false;
  }
  return true;
}

std::string canonicalize_canonical_path(const canonical_path_t& path) {
  if (!path.ok) return {};
  return path.canonical;
}

canonical_path_t decode_primitive_endpoint_text(const std::string& text) {
  const std::string t = trim_ascii_ws_copy(text);
  if (starts_with(t, "iinuji.") || starts_with(t, "tsi.")) {
    return decode_canonical_path(t);
  }

  const std::size_t at = t.find('@');
  if (at == std::string::npos) {
    const std::string alias = sanitize_atom(t);
    return decode_canonical_path("iinuji.primitive.endpoint." + alias);
  }

  const std::string alias = sanitize_atom(trim_ascii_ws_copy(t.substr(0, at)));
  const std::string endpoint = trim_ascii_ws_copy(t.substr(at + 1));
  return decode_canonical_path("iinuji.primitive.endpoint." + alias + "@" + endpoint);
}

canonical_path_t decode_primitive_command_text(const std::string& text) {
  const std::string t = trim_ascii_ws_copy(text);
  if (starts_with(t, "iinuji.") || starts_with(t, "tsi.")) {
    return decode_canonical_path(t);
  }

  std::istringstream iss(t);
  std::string a0, a1, a2;
  iss >> a0 >> a1 >> a2;
  a0 = lower_copy(a0);
  a1 = lower_copy(a1);
  a2 = lower_copy(a2);

  if (a0.empty()) return decode_canonical_path(t);
  if (a0 == "reload") return decode_canonical_path("iinuji.refresh()");

  if (a0 == "data" && a1 == "plot") {
    const std::string mode = sanitize_value(a2.empty() ? "seq" : a2);
    return decode_canonical_path("iinuji.view.data.plot(mode=" + mode + ")");
  }
  if (a0 == "plot") {
    const std::string view = sanitize_value(a1.empty() ? "toggle" : a1);
    return decode_canonical_path("iinuji.view.data.plot(view=" + view + ")");
  }
  if (a0 == "data") return decode_canonical_path("iinuji.view.data()");
  if (a0 == "tsi") return decode_canonical_path("iinuji.view.tsi()");

  return decode_canonical_path(
      "iinuji.primitive.command(raw=" + sanitize_value(t) + ")");
}

std::string hashimyei_round_note() {
  return "NOTE(hashimyei): revisit hash function design space (word-combo/fun encodings).";
}

}  // namespace camahjucunu
}  // namespace cuwacunu
