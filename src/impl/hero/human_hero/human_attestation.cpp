// SPDX-License-Identifier: MIT
#include "hero/human_hero/human_attestation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/evp.h>

#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu {
namespace hero {
namespace human {

namespace {

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char ch : in) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (ch < 0x20) {
          out << "\\u00";
          constexpr char kHex[] = "0123456789abcdef";
          out << kHex[(ch >> 4) & 0x0f] << kHex[ch & 0x0f];
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
}

[[nodiscard]] std::string bool_json(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] bool is_hex_lower_string(std::string_view in) {
  if (in.empty()) return false;
  return std::all_of(in.begin(), in.end(), [](unsigned char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
  });
}

[[nodiscard]] std::string bytes_to_hex(const unsigned char* data,
                                       std::size_t size) {
  constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.resize(size * 2);
  for (std::size_t i = 0; i < size; ++i) {
    out[2 * i] = kHex[(data[i] >> 4) & 0x0f];
    out[2 * i + 1] = kHex[data[i] & 0x0f];
  }
  return out;
}

[[nodiscard]] bool base64_decode(std::string_view text,
                                 std::vector<unsigned char>* out,
                                 std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "base64 decode output pointer is null";
    return false;
  }
  out->clear();
  std::string cleaned{};
  cleaned.reserve(text.size());
  for (const unsigned char ch : text) {
    if (std::isspace(ch) != 0) continue;
    cleaned.push_back(static_cast<char>(ch));
  }
  if (cleaned.empty()) {
    if (error) *error = "base64 payload is empty";
    return false;
  }
  if ((cleaned.size() % 4) != 0) {
    if (error) *error = "base64 payload length is invalid";
    return false;
  }
  std::vector<unsigned char> decoded((cleaned.size() / 4) * 3 + 1, 0);
  const int rc = EVP_DecodeBlock(decoded.data(),
                                 reinterpret_cast<const unsigned char*>(cleaned.data()),
                                 static_cast<int>(cleaned.size()));
  if (rc < 0) {
    if (error) *error = "base64 decode failed";
    return false;
  }
  std::size_t decoded_size = static_cast<std::size_t>(rc);
  if (!cleaned.empty() && cleaned.back() == '=') --decoded_size;
  if (cleaned.size() >= 2 && cleaned[cleaned.size() - 2] == '=') --decoded_size;
  decoded.resize(decoded_size);
  *out = std::move(decoded);
  return true;
}

[[nodiscard]] bool hex_to_bytes(std::string_view hex,
                                std::vector<unsigned char>* out,
                                std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "hex decode output pointer is null";
    return false;
  }
  out->clear();
  const std::string token = trim_ascii(hex);
  if (token.empty() || (token.size() % 2) != 0) {
    if (error) *error = "signature hex must have even non-zero length";
    return false;
  }
  out->reserve(token.size() / 2);
  const auto hex_nibble = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
  };
  for (std::size_t i = 0; i < token.size(); i += 2) {
    const int hi = hex_nibble(token[i]);
    const int lo = hex_nibble(token[i + 1]);
    if (hi < 0 || lo < 0) {
      if (error) *error = "signature hex contains invalid characters";
      return false;
    }
    out->push_back(static_cast<unsigned char>((hi << 4) | lo));
  }
  return true;
}

struct binary_reader_t {
  const std::vector<unsigned char>& data;
  std::size_t pos{0};

  [[nodiscard]] bool read_u32(std::uint32_t* out) {
    if (!out || pos + 4 > data.size()) return false;
    *out = (static_cast<std::uint32_t>(data[pos]) << 24) |
           (static_cast<std::uint32_t>(data[pos + 1]) << 16) |
           (static_cast<std::uint32_t>(data[pos + 2]) << 8) |
           static_cast<std::uint32_t>(data[pos + 3]);
    pos += 4;
    return true;
  }

  [[nodiscard]] bool read_bytes(std::size_t n,
                                std::vector<unsigned char>* out) {
    if (!out || pos + n > data.size()) return false;
    out->assign(data.begin() + static_cast<std::ptrdiff_t>(pos),
                data.begin() + static_cast<std::ptrdiff_t>(pos + n));
    pos += n;
    return true;
  }

  [[nodiscard]] bool read_string(std::vector<unsigned char>* out) {
    std::uint32_t len = 0;
    if (!read_u32(&len)) return false;
    return read_bytes(static_cast<std::size_t>(len), out);
  }

  [[nodiscard]] bool read_string(std::string* out) {
    std::vector<unsigned char> bytes{};
    if (!read_string(&bytes)) return false;
    out->assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
  }
};

[[nodiscard]] bool principal_matches(std::string_view principals,
                                     std::string_view operator_id) {
  std::size_t start = 0;
  while (start <= principals.size()) {
    const std::size_t comma = principals.find(',', start);
    const std::string token = trim_ascii(
        principals.substr(start, comma == std::string_view::npos
                                      ? std::string_view::npos
                                      : comma - start));
    if (!token.empty() && token == operator_id) return true;
    if (comma == std::string_view::npos) break;
    start = comma + 1;
  }
  return false;
}

[[nodiscard]] bool public_key_fingerprint_from_raw_ed25519(
    const std::vector<unsigned char>& public_key_bytes, std::string* out_hex,
    std::string* error) {
  if (error) error->clear();
  if (!out_hex) {
    if (error) *error = "public key fingerprint output pointer is null";
    return false;
  }
  *out_hex = "";
  if (public_key_bytes.size() != 32) {
    if (error) *error = "ed25519 public key must be 32 bytes";
    return false;
  }
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_Digest(public_key_bytes.data(), public_key_bytes.size(), digest,
                 &digest_len, EVP_sha256(),
                 nullptr) != 1) {
    if (error) *error = "cannot hash public key fingerprint";
    return false;
  }
  *out_hex = bytes_to_hex(digest, digest_len);
  return true;
}

[[nodiscard]] bool load_openssh_ed25519_identity(
    const std::filesystem::path& path, std::vector<unsigned char>* out_seed,
    std::vector<unsigned char>* out_public_key, std::string* error) {
  if (error) error->clear();
  if (!out_seed || !out_public_key) {
    if (error) *error = "identity outputs are null";
    return false;
  }
  out_seed->clear();
  out_public_key->clear();

  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error)) return false;
  std::istringstream in(text);
  std::string line{};
  bool inside = false;
  std::string b64{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed == "-----BEGIN OPENSSH PRIVATE KEY-----") {
      inside = true;
      continue;
    }
    if (trimmed == "-----END OPENSSH PRIVATE KEY-----") break;
    if (inside) b64.append(trimmed);
  }
  if (b64.empty()) {
    if (error) *error = "cannot find OpenSSH private key block in " + path.string();
    return false;
  }

  std::vector<unsigned char> decoded{};
  if (!base64_decode(b64, &decoded, error)) return false;
  static constexpr unsigned char kMagic[] = "openssh-key-v1";
  const std::size_t kMagicSize = sizeof(kMagic);
  if (decoded.size() < kMagicSize ||
      std::memcmp(decoded.data(), kMagic, kMagicSize) != 0) {
    if (error) *error = "unsupported OpenSSH identity header in " + path.string();
    return false;
  }

  binary_reader_t reader{decoded, kMagicSize};
  std::string ciphername{};
  std::string kdfname{};
  std::vector<unsigned char> kdfoptions{};
  std::uint32_t key_count = 0;
  std::vector<unsigned char> public_blob{};
  std::vector<unsigned char> private_blob{};
  if (!reader.read_string(&ciphername) || !reader.read_string(&kdfname) ||
      !reader.read_string(&kdfoptions) || !reader.read_u32(&key_count) ||
      !reader.read_string(&public_blob) || !reader.read_string(&private_blob)) {
    if (error) *error = "malformed OpenSSH identity structure in " + path.string();
    return false;
  }
  if (ciphername != "none" || kdfname != "none") {
    if (error) *error = "encrypted OpenSSH identities are not supported";
    return false;
  }
  if (key_count != 1) {
    if (error) *error = "OpenSSH identity must contain exactly one key";
    return false;
  }

  binary_reader_t private_reader{private_blob, 0};
  std::uint32_t check_a = 0;
  std::uint32_t check_b = 0;
  std::string keytype{};
  std::vector<unsigned char> public_key{};
  std::vector<unsigned char> private_key{};
  std::string comment{};
  if (!private_reader.read_u32(&check_a) || !private_reader.read_u32(&check_b) ||
      !private_reader.read_string(&keytype) ||
      !private_reader.read_string(&public_key) ||
      !private_reader.read_string(&private_key) ||
      !private_reader.read_string(&comment)) {
    if (error) *error = "malformed OpenSSH private key body in " + path.string();
    return false;
  }
  if (check_a != check_b) {
    if (error) *error = "OpenSSH private key checksum mismatch";
    return false;
  }
  if (keytype != "ssh-ed25519") {
    if (error) *error = "only ssh-ed25519 identities are supported";
    return false;
  }
  if (public_key.size() != 32 || private_key.size() != 64) {
    if (error) *error = "unexpected ssh-ed25519 key sizes in " + path.string();
    return false;
  }
  if (!std::equal(public_key.begin(), public_key.end(),
                  private_key.begin() + 32)) {
    if (error) *error = "OpenSSH identity public/private key mismatch";
    return false;
  }
  out_seed->assign(private_key.begin(), private_key.begin() + 32);
  *out_public_key = std::move(public_key);
  (void)comment;
  (void)public_blob;
  return true;
}

[[nodiscard]] bool load_ed25519_public_key_for_operator(
    const std::filesystem::path& identities_path, std::string_view operator_id,
    std::vector<unsigned char>* out_public_key, std::string* error) {
  if (error) error->clear();
  if (!out_public_key) {
    if (error) *error = "public key output pointer is null";
    return false;
  }
  out_public_key->clear();

  std::ifstream in(identities_path);
  if (!in) {
    if (error) *error = "cannot open human operator identities: " + identities_path.string();
    return false;
  }
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty() || trimmed.front() == '#') continue;
    std::istringstream fields(trimmed);
    std::string principals{};
    std::string keytype{};
    std::string keydata{};
    if (!(fields >> principals >> keytype >> keydata)) continue;
    if (!principal_matches(principals, operator_id)) continue;
    if (keytype != "ssh-ed25519") {
      if (error) *error = "only ssh-ed25519 public keys are supported for operator identities";
      return false;
    }
    std::vector<unsigned char> blob{};
    if (!base64_decode(keydata, &blob, error)) return false;
    binary_reader_t reader{blob, 0};
    std::string blob_keytype{};
    std::vector<unsigned char> public_key{};
    if (!reader.read_string(&blob_keytype) || !reader.read_string(&public_key)) {
      if (error) *error = "malformed OpenSSH public key for operator " +
                          std::string(operator_id);
      return false;
    }
    if (blob_keytype != "ssh-ed25519" || public_key.size() != 32) {
      if (error) *error = "unexpected OpenSSH public key shape for operator " +
                          std::string(operator_id);
      return false;
    }
    *out_public_key = std::move(public_key);
    return true;
  }
  if (error) {
    *error = "operator_id not found in human operator identities: " +
             std::string(operator_id);
  }
  return false;
}

[[nodiscard]] bool parse_size_token(std::string_view raw, std::size_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(raw);
  if (token.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string& json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* out_end) {
  if (out) out->clear();
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string parsed;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = parsed;
      if (out_end) *out_end = pos;
      return true;
    }
    if (c == '\\') {
      if (pos >= json.size()) return false;
      const char esc = json[pos++];
      switch (esc) {
        case '"':
        case '\\':
        case '/':
          parsed.push_back(esc);
          break;
        case 'b':
          parsed.push_back('\b');
          break;
        case 'f':
          parsed.push_back('\f');
          break;
        case 'n':
          parsed.push_back('\n');
          break;
        case 'r':
          parsed.push_back('\r');
          break;
        case 't':
          parsed.push_back('\t');
          break;
        case 'u':
          if (pos + 4 > json.size()) return false;
          pos += 4;
          parsed.push_back('?');
          break;
        default:
          return false;
      }
      continue;
    }
    parsed.push_back(c);
  }
  return false;
}

[[nodiscard]] bool find_json_value_end(const std::string& json, std::size_t pos,
                                       std::size_t* out_end) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size()) return false;
  const char first = json[pos];
  if (first == '"') return parse_json_string_at(json, pos, nullptr, out_end);
  if (first == '{' || first == '[') {
    int depth = 0;
    bool in_string = false;
    for (std::size_t i = pos; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (c == '\\') {
          ++i;
          continue;
        }
        if (c == '"') in_string = false;
        continue;
      }
      if (c == '"') {
        in_string = true;
        continue;
      }
      if (c == '{' || c == '[') ++depth;
      if (c == '}' || c == ']') {
        --depth;
        if (depth == 0) {
          if (out_end) *out_end = i + 1;
          return true;
        }
      }
    }
    return false;
  }
  std::size_t i = pos;
  while (i < json.size()) {
    const char c = json[i];
    if (c == ',' || c == '}' || c == ']') break;
    ++i;
  }
  while (i > pos &&
         std::isspace(static_cast<unsigned char>(json[i - 1])) != 0) {
    --i;
  }
  if (i == pos) return false;
  if (out_end) *out_end = i;
  return true;
}

[[nodiscard]] bool extract_json_field_raw(const std::string& json,
                                          std::string_view key,
                                          std::string* out_raw) {
  if (out_raw) out_raw->clear();
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = 0;
  while (true) {
    pos = json.find(needle, pos);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size() || json[pos] != ':') continue;
    pos = skip_json_whitespace(json, pos + 1);
    std::size_t end = 0;
    if (!find_json_value_end(json, pos, &end)) return false;
    if (out_raw) *out_raw = json.substr(pos, end - pos);
    return true;
  }
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           std::string_view key, bool* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  if (raw == "true") {
    if (out) *out = true;
    return true;
  }
  if (raw == "false") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           std::string_view key,
                                           std::size_t* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  return parse_size_token(raw, out);
}

}  // namespace

bool validate_human_response_record(const human_response_record_t& record,
                                    std::string* error) {
  if (error) error->clear();
  if (trim_ascii(record.schema) != std::string(kHumanResponseSchemaV1)) {
    if (error) *error = "unsupported human response schema: " + record.schema;
    return false;
  }
  if (trim_ascii(record.loop_id).empty()) {
    if (error) *error = "human response missing loop_id";
    return false;
  }
  if (record.review_index == 0) {
    if (error) *error = "human response missing review_index";
    return false;
  }
  if (trim_ascii(record.request_sha256_hex).size() != 64 ||
      !is_hex_lower_string(trim_ascii(record.request_sha256_hex))) {
    if (error) *error = "human response request_sha256_hex must be 64 lowercase hex chars";
    return false;
  }
  if (trim_ascii(record.operator_id).empty()) {
    if (error) *error = "human response missing operator_id";
    return false;
  }
  if (record.responded_at_ms == 0) {
    if (error) *error = "human response missing responded_at_ms";
    return false;
  }
  const std::string control = trim_ascii(record.control_kind);
  if (control != "continue" && control != "stop") {
    if (error) *error = "human response control_kind must be continue or stop";
    return false;
  }
  if (trim_ascii(record.reason).empty()) {
    if (error) *error = "human response missing reason";
    return false;
  }
  if (trim_ascii(record.signer_public_key_fingerprint_sha256_hex).size() != 64 ||
      !is_hex_lower_string(
          trim_ascii(record.signer_public_key_fingerprint_sha256_hex))) {
    if (error) {
      *error =
          "human response signer_public_key_fingerprint_sha256_hex must be 64 lowercase hex chars";
    }
    return false;
  }
  if (control == "continue") {
    const std::string next = trim_ascii(record.next_action_kind);
    if (next != "default_plan" && next != "binding") {
      if (error) *error = "continue human response requires next_action_kind";
      return false;
    }
    if (next == "binding" && trim_ascii(record.target_binding_id).empty()) {
      if (error) *error = "binding human response requires target_binding_id";
      return false;
    }
  }
  return true;
}

std::string human_response_to_json(const human_response_record_t& record) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"loop_id\":" << json_quote(record.loop_id) << ","
      << "\"review_index\":" << record.review_index << ","
      << "\"request_sha256_hex\":" << json_quote(record.request_sha256_hex) << ","
      << "\"operator_id\":" << json_quote(record.operator_id) << ","
      << "\"responded_at_ms\":" << record.responded_at_ms << ","
      << "\"control_kind\":" << json_quote(record.control_kind) << ","
      << "\"next_action\":{"
      << "\"kind\":" << json_quote(record.next_action_kind) << ","
      << "\"target_binding_id\":" << json_quote(record.target_binding_id) << ","
      << "\"reset_runtime_state\":" << bool_json(record.reset_runtime_state)
      << "},"
      << "\"reason\":" << json_quote(record.reason) << ","
      << "\"memory_note\":" << json_quote(record.memory_note) << ","
      << "\"signer_public_key_fingerprint_sha256_hex\":"
      << json_quote(record.signer_public_key_fingerprint_sha256_hex)
      << "}";
  return out.str();
}

bool parse_human_response_json(const std::string& json,
                               human_response_record_t* out,
                               std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "human response output pointer is null";
    return false;
  }
  *out = human_response_record_t{};
  if (!extract_json_string_field(json, "schema", &out->schema)) {
    if (error) *error = "human response missing schema";
    return false;
  }
  if (!extract_json_string_field(json, "loop_id", &out->loop_id)) {
    if (error) *error = "human response missing loop_id";
    return false;
  }
  std::size_t review_index = 0;
  if (!extract_json_size_field(json, "review_index", &review_index)) {
    if (error) *error = "human response missing review_index";
    return false;
  }
  out->review_index = static_cast<std::uint64_t>(review_index);
  if (!extract_json_string_field(json, "request_sha256_hex",
                                 &out->request_sha256_hex)) {
    if (error) *error = "human response missing request_sha256_hex";
    return false;
  }
  if (!extract_json_string_field(json, "operator_id", &out->operator_id)) {
    if (error) *error = "human response missing operator_id";
    return false;
  }
  std::size_t responded_at_ms = 0;
  if (!extract_json_size_field(json, "responded_at_ms", &responded_at_ms)) {
    if (error) *error = "human response missing responded_at_ms";
    return false;
  }
  out->responded_at_ms = static_cast<std::uint64_t>(responded_at_ms);
  if (!extract_json_string_field(json, "control_kind", &out->control_kind)) {
    if (error) *error = "human response missing control_kind";
    return false;
  }
  std::string next_action_json{};
  if (extract_json_field_raw(json, "next_action", &next_action_json) &&
      trim_ascii(next_action_json) != "null") {
    (void)extract_json_string_field(next_action_json, "kind",
                                    &out->next_action_kind);
    (void)extract_json_string_field(next_action_json, "target_binding_id",
                                    &out->target_binding_id);
    (void)extract_json_bool_field(next_action_json, "reset_runtime_state",
                                  &out->reset_runtime_state);
  }
  if (!extract_json_string_field(json, "reason", &out->reason)) {
    if (error) *error = "human response missing reason";
    return false;
  }
  (void)extract_json_string_field(json, "memory_note", &out->memory_note);
  if (!extract_json_string_field(json,
                                 "signer_public_key_fingerprint_sha256_hex",
                                 &out->signer_public_key_fingerprint_sha256_hex)) {
    if (error) {
      *error =
          "human response missing signer_public_key_fingerprint_sha256_hex";
    }
    return false;
  }
  return validate_human_response_record(*out, error);
}

bool sha256_hex_string(std::string_view payload, std::string* out_hex,
                       std::string* error) {
  if (error) error->clear();
  if (!out_hex) {
    if (error) *error = "sha256 output pointer is null";
    return false;
  }
  *out_hex = "";
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_Digest(payload.data(), payload.size(), digest, &digest_len,
                 EVP_sha256(), nullptr) != 1) {
    if (error) *error = "EVP_Digest(SHA-256) failed";
    return false;
  }
  *out_hex = bytes_to_hex(digest, digest_len);
  return true;
}

bool sha256_hex_file(const std::filesystem::path& path, std::string* out_hex,
                     std::string* error) {
  if (error) error->clear();
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error)) return false;
  return sha256_hex_string(text, out_hex, error);
}

bool sign_human_response_json(
    const std::filesystem::path& ssh_identity_path,
    std::string_view response_json, std::string* out_signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error) {
  if (error) error->clear();
  if (!out_signature_hex || !out_public_key_fingerprint_sha256_hex) {
    if (error) *error = "human response signature outputs are null";
    return false;
  }
  *out_signature_hex = "";
  *out_public_key_fingerprint_sha256_hex = "";

  std::vector<unsigned char> private_seed{};
  std::vector<unsigned char> public_key{};
  if (!load_openssh_ed25519_identity(ssh_identity_path, &private_seed,
                                     &public_key, error)) {
    return false;
  }
  EVP_PKEY* pkey =
      EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
                                   private_seed.data(), private_seed.size());
  if (!pkey) {
    if (error) *error = "cannot create EVP_PKEY from ssh-ed25519 identity";
    return false;
  }

  bool ok = false;
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    if (error) *error = "cannot allocate EVP_MD_CTX for signing";
  } else if (EVP_DigestSignInit(mdctx, nullptr, nullptr, nullptr, pkey) != 1) {
    if (error) *error = "EVP_DigestSignInit failed for human response signing";
  } else {
    size_t sig_len = 0;
    if (EVP_DigestSign(mdctx, nullptr, &sig_len,
                       reinterpret_cast<const unsigned char*>(response_json.data()),
                       response_json.size()) != 1) {
      if (error) *error = "EVP_DigestSign size probe failed";
    } else {
      std::vector<unsigned char> signature(sig_len);
      if (EVP_DigestSign(mdctx, signature.data(), &sig_len,
                         reinterpret_cast<const unsigned char*>(response_json.data()),
                         response_json.size()) != 1) {
        if (error) *error = "EVP_DigestSign failed";
      } else {
        signature.resize(sig_len);
        *out_signature_hex = bytes_to_hex(signature.data(), signature.size());
        if (!public_key_fingerprint_from_raw_ed25519(
                public_key, out_public_key_fingerprint_sha256_hex, error)) {
          out_signature_hex->clear();
        } else {
          ok = true;
        }
      }
    }
  }
  if (mdctx) EVP_MD_CTX_free(mdctx);
  EVP_PKEY_free(pkey);
  return ok;
}

bool verify_human_response_json_signature(
    const std::filesystem::path& operator_identities_path,
    std::string_view operator_id, std::string_view response_json,
    std::string_view signature_hex,
    std::string* out_public_key_fingerprint_sha256_hex, std::string* error) {
  if (error) error->clear();
  if (!out_public_key_fingerprint_sha256_hex) {
    if (error) *error = "human response verify fingerprint output pointer is null";
    return false;
  }
  *out_public_key_fingerprint_sha256_hex = "";

  std::vector<unsigned char> signature{};
  if (!hex_to_bytes(signature_hex, &signature, error)) return false;

  std::vector<unsigned char> public_key{};
  if (!load_ed25519_public_key_for_operator(operator_identities_path,
                                            operator_id, &public_key, error)) {
    return false;
  }
  EVP_PKEY* pkey =
      EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                  public_key.data(), public_key.size());
  if (!pkey) {
    if (error) *error = "cannot create EVP_PKEY from operator public key";
    return false;
  }
  bool ok = false;
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    if (error) *error = "cannot allocate EVP_MD_CTX for verification";
  } else if (EVP_DigestVerifyInit(mdctx, nullptr, nullptr, nullptr, pkey) != 1) {
    if (error) *error = "EVP_DigestVerifyInit failed for human response verification";
  } else if (EVP_DigestVerify(
                 mdctx, signature.data(), signature.size(),
                 reinterpret_cast<const unsigned char*>(response_json.data()),
                 response_json.size()) != 1) {
    if (error) *error = "human response signature verification failed";
  } else if (!public_key_fingerprint_from_raw_ed25519(
                 public_key, out_public_key_fingerprint_sha256_hex, error)) {
    ok = false;
  } else {
    ok = true;
  }
  if (mdctx) EVP_MD_CTX_free(mdctx);
  EVP_PKEY_free(pkey);
  return ok;
}

}  // namespace human
}  // namespace hero
}  // namespace cuwacunu
