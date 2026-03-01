/* canonical_path.cpp */
#include "camahjucunu/dsl/canonical_path/canonical_path.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hashimyei/hashimyei_identity.h"
#include "piaabo/dconfig.h"
#include "tsiemene/tsi.directive.registry.h"
#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace camahjucunu {

namespace {

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
  if (!std::isalnum(static_cast<unsigned char>(s.front())) && s.front() != '_') return false;
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

[[nodiscard]] std::string assign_hash_name(std::string_view key) {
  return cuwacunu::hashimyei::canonical_identity_provider().assign(key);
}

[[nodiscard]] std::string canonical_kind_token(std::string s) {
  s = lower_copy(trim_ascii_ws_copy(std::move(s)));
  if (s == "str" || s == ":str") return ":str";
  if (s == "tensor" || s == ":tensor") return ":tensor";
  return {};
}

[[nodiscard]] std::string canonical_directive_token(std::string s) {
  const auto id = tsiemene::parse_directive_id(std::move(s));
  if (id.has_value()) return std::string(*id);
  return {};
}

[[nodiscard]] std::optional<tsiemene::PayloadKind> payload_kind_from_token(std::string_view token) {
  if (token == ":str") return tsiemene::PayloadKind::String;
  if (token == ":tensor") return tsiemene::PayloadKind::Tensor;
  return std::nullopt;
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

[[nodiscard]] bool unpack_fused_wikimyei_model_hash(std::vector<std::string>* segs) {
  if (!segs || segs->size() != 4) return false;
  std::string model;
  std::string hash;
  if (!cuwacunu::hashimyei::split_model_hash_suffix((*segs)[3], &model, &hash)) return false;
  (*segs)[3] = std::move(model);
  segs->push_back(std::move(hash));
  return true;
}

[[nodiscard]] bool canonicalize_segments(std::vector<std::string>* segs,
                                         std::string* hashimyei,
                                         std::string* error) {
  if (!segs || !hashimyei) return false;
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

  if (segs->size() >= 3 &&
      (*segs)[0] == "tsi" &&
      (*segs)[1] == "wave" &&
      (*segs)[2] == "generator") {
    if (error) *error = "legacy alias 'tsi.wave.generator' is removed; use 'board.wave'";
    return false;
  }
  if (segs->size() >= 4 &&
      (*segs)[0] == "tsi" &&
      (*segs)[1] == "wikimyei" &&
      (*segs)[2] == "wave" &&
      (*segs)[3] == "generator") {
    if (error) *error = "legacy alias 'tsi.wikimyei.wave.generator' is removed; use 'board.wave'";
    return false;
  }
  if (segs->size() >= 3 &&
      (*segs)[0] == "tsi" &&
      (*segs)[1] == "wikimyei" &&
      (*segs)[2] == "source") {
    if (error) *error = "legacy namespace 'tsi.wikimyei.source.*' is removed; use 'tsi.source.*'";
    return false;
  }

  const bool root_is_tsi = (*segs)[0] == "tsi";
  const bool root_is_iinuji = (*segs)[0] == "iinuji";
  const bool root_is_board = (*segs)[0] == "board";
  if (!root_is_tsi && !root_is_iinuji && !root_is_board) {
    if (error) *error = "path root must be 'tsi', 'board', or 'iinuji'";
    return false;
  }
  if (segs->size() >= 2 && root_is_tsi && (*segs)[1] == "iinuji") {
    if (error) *error = "tsi.iinuji.* is not supported; use iinuji.*";
    return false;
  }
  if (segs->size() >= 2 && root_is_tsi && (*segs)[1] == "wave") {
    if (error) *error = "tsi.wave is not a TSI component anymore; use board.wave and source roots";
    return false;
  }
  if (!segs->empty() && segs->back() == "jkimyei") {
    if (error) *error = "legacy '.jkimyei' facet is removed; use '@jkimyei:<kind>'";
    return false;
  }

  if (segs->size() == 1) {
    return true;
  }

  if ((*segs)[1] == "wikimyei") {
    if (!root_is_tsi) {
      if (error) *error = "wikimyei paths must be rooted at tsi.wikimyei";
      return false;
    }
    if (segs->size() == 2 || segs->size() == 3) {
      return true;
    }
    if (segs->size() < 4) {
      if (error) *error = "tsi.wikimyei path requires family and model";
      return false;
    }
    if (segs->size() == 4) {
      if (!unpack_fused_wikimyei_model_hash(segs)) {
        if (error) {
          *error =
              "tsi.wikimyei path requires explicit hashimyei suffix "
              "(expected tsi.wikimyei.<family>.<model>.<hashimyei>)";
        }
        return false;
      }
    } else if (segs->size() != 5) {
      if (error) *error = "tsi.wikimyei path accepts family.model.hashimyei";
      return false;
    }

    *hashimyei = (*segs)[4];
    if (*hashimyei == "default") {
      if (error) {
        *error =
            "legacy hashimyei alias 'default' is removed; "
            "use explicit hex hashimyei id (for example 0x0000)";
      }
      return false;
    }
    if (!cuwacunu::hashimyei::is_hex_hash_name(*hashimyei)) {
      if (error) {
        *error =
            "invalid hashimyei id; expected explicit hex form 0x<hex>";
      }
      return false;
    }
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
  out.facet = canonical_facet_e::none;
  if (!canonicalize_segments(&segs, &out.hashimyei, &error)) {
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
    if (!out.segments.empty() && out.segments.front() == "tsi") {
      const auto type_id = tsiemene::parse_tsi_type_id(out.canonical_identity);
      const auto kind = payload_kind_from_token(out.kind);
      const auto directive = tsiemene::parse_directive_id(out.directive);
      if (type_id.has_value() && kind.has_value() && directive.has_value()) {
        if (!tsiemene::type_accepts_endpoint(*type_id, *directive, *kind)) {
          out.error = "endpoint directive/kind is not supported by tsi type";
          return out;
        }
      }
    }
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

namespace dsl {

canonicalPath::canonicalPath() {
  CANONICAL_PATH_GRAMMAR_TEXT.clear();
}

canonicalPath::canonicalPath(std::string grammar_text)
    : CANONICAL_PATH_GRAMMAR_TEXT(std::move(grammar_text)) {}

canonical_path_t canonicalPath::decode(std::string instruction) const {
  return decode_internal(std::move(instruction));
}

}  // namespace dsl

canonical_path_t decode_canonical_path(const std::string& text) {
  return dsl::canonicalPath().decode(text);
}

canonical_path_t decode_canonical_path(const std::string& text,
                                       const std::string& contract_hash) {
  if (contract_hash.empty()) {
    log_fatal("[canonical_path] missing contract hash for decode_canonical_path\n");
  }
  (void)cuwacunu::piaabo::dconfig::contract_space_t::snapshot(contract_hash);
  return dsl::canonicalPath().decode(text);
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
  return "NOTE(hashimyei): hex identity catalog active (0x0000..0x000f).";
}

}  // namespace camahjucunu
}  // namespace cuwacunu
