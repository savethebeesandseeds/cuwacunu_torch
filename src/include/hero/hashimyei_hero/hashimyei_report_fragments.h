// ./include/hero/hashimyei_hero/hashimyei_report_fragments.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <openssl/rand.h>

#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "piaabo/dconfig.h"
#include "piaabo/dencryption.h"
#include "piaabo/dsecurity.h"

namespace cuwacunu {
namespace hashimyei {

inline constexpr std::string_view kReportFragmentManifestFilename = "manifest.v2.kv";
inline constexpr std::string_view kReportFragmentManifestSchema = "hashimyei.report_fragment.manifest.v2";
inline constexpr std::string_view kCatalogFilename = "hashimyei_catalog.idydb";
inline constexpr std::string_view kHashimyeiMetaDirname = "_meta";
inline constexpr std::string_view kRuntimeHashimyeiDirname = ".hashimyei";

struct report_fragment_metadata_t {
  bool present{false};
  bool decrypted{false};
  std::string text{};
  std::string error{};
};

struct report_fragment_identity_t {
  std::string family{};
  std::string model{};
  std::string hashimyei{};
  std::string canonical_path{};
  std::filesystem::path directory{};
  std::vector<std::filesystem::path> weight_files{};
  report_fragment_metadata_t metadata{};
};

struct report_fragment_manifest_file_t {
  std::string path{};
  std::uintmax_t size{0};
};

struct report_fragment_manifest_t {
  std::string schema{std::string(kReportFragmentManifestSchema)};
  std::string family_canonical_path{};
  std::string family{};
  std::string model{};
  std::string report_fragment_id{};
  std::vector<report_fragment_manifest_file_t> files{};
};

[[nodiscard]] inline std::string trim_ascii(std::string value) {
  std::size_t b = 0;
  while (b < value.size() &&
         std::isspace(static_cast<unsigned char>(value[b])) != 0) {
    ++b;
  }
  std::size_t e = value.size();
  while (e > b &&
         std::isspace(static_cast<unsigned char>(value[e - 1])) != 0) {
    --e;
  }
  return value.substr(b, e - b);
}

[[nodiscard]] inline std::string manifest_file_key(std::size_t index,
                                                   std::string_view suffix) {
  std::ostringstream oss;
  oss << "file_" << std::setfill('0') << std::setw(4) << index << "_" << suffix;
  return oss.str();
}

[[nodiscard]] inline bool parse_uintmax(std::string_view text,
                                        std::uintmax_t* out_value) {
  if (!out_value) return false;
  const std::string trimmed = trim_ascii(std::string(text));
  if (trimmed.empty()) return false;
  try {
    *out_value = static_cast<std::uintmax_t>(std::stoull(trimmed));
    return true;
  } catch (...) {
    return false;
  }
}

[[nodiscard]] inline std::vector<report_fragment_manifest_file_t> normalized_manifest_files(
    const std::vector<report_fragment_manifest_file_t>& files) {
  std::map<std::string, std::uintmax_t> merged{};
  for (const auto& file : files) {
    if (file.path.empty()) continue;
    const auto it = merged.find(file.path);
    if (it == merged.end() || file.size > it->second) {
      merged[file.path] = file.size;
    }
  }

  std::vector<report_fragment_manifest_file_t> out;
  out.reserve(merged.size());
  for (const auto& [path, size] : merged) {
    out.push_back(report_fragment_manifest_file_t{path, size});
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> split_dot(std::string_view s) {
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

[[nodiscard]] inline std::filesystem::path canonical_path_directory(
    const std::filesystem::path& root, std::string_view canonical_path) {
  std::filesystem::path out = root;
  const auto segments = split_dot(canonical_path);
  for (const auto& segment : segments) {
    const std::string token = trim_ascii(segment);
    if (token.empty()) continue;
    out /= token;
  }
  return out;
}

[[nodiscard]] inline std::filesystem::path meta_root(
    const std::filesystem::path& root) {
  return root / std::string(kHashimyeiMetaDirname);
}

[[nodiscard]] inline std::filesystem::path catalog_directory(
    const std::filesystem::path& root) {
  return meta_root(root) / "catalog";
}

[[nodiscard]] inline std::filesystem::path runs_root(
    const std::filesystem::path& root) {
  return meta_root(root) / "runs";
}

[[nodiscard]] inline std::filesystem::path report_fragment_manifest_path(const std::filesystem::path& report_fragment_dir) {
  return report_fragment_dir / std::string(kReportFragmentManifestFilename);
}

[[nodiscard]] inline bool report_fragment_manifest_exists(const std::filesystem::path& report_fragment_dir) {
  std::error_code ec;
  const auto p = report_fragment_manifest_path(report_fragment_dir);
  return std::filesystem::exists(p, ec) && std::filesystem::is_regular_file(p, ec);
}

[[nodiscard]] inline bool write_report_fragment_manifest(const std::filesystem::path& report_fragment_dir,
                                                  const report_fragment_manifest_t& manifest,
                                                  std::string* error = nullptr) {
  namespace fs = std::filesystem;
  if (error) error->clear();

  if (manifest.family_canonical_path.empty() ||
      manifest.family.empty() ||
      manifest.model.empty() ||
      manifest.report_fragment_id.empty()) {
    if (error) *error = "report_fragment manifest missing family_canonical_path/family/model/report_fragment_id";
    return false;
  }

  std::error_code ec;
  fs::create_directories(report_fragment_dir, ec);
  if (ec) {
    if (error) *error = "cannot create report_fragment directory: " + report_fragment_dir.string();
    return false;
  }

  const std::vector<report_fragment_manifest_file_t> files =
      normalized_manifest_files(manifest.files);
  const auto target_path = report_fragment_manifest_path(report_fragment_dir);
  std::ofstream out(target_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open manifest for write: " + target_path.string();
    return false;
  }

  out << "schema=" << kReportFragmentManifestSchema << "\n";
  out << "family_canonical_path=" << manifest.family_canonical_path << "\n";
  out << "family=" << manifest.family << "\n";
  out << "model=" << manifest.model << "\n";
  out << "report_fragment_id=" << manifest.report_fragment_id << "\n";
  out << "file_count=" << files.size() << "\n";
  for (std::size_t i = 0; i < files.size(); ++i) {
    out << manifest_file_key(i, "path") << "=" << files[i].path << "\n";
    out << manifest_file_key(i, "size") << "=" << files[i].size << "\n";
  }
  if (!out) {
    if (error) *error = "cannot write manifest contents: " + target_path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline bool read_report_fragment_manifest(const std::filesystem::path& report_fragment_dir,
                                                 report_fragment_manifest_t* out,
                                                 std::string* error = nullptr) {
  if (!out) return false;
  if (error) error->clear();
  *out = report_fragment_manifest_t{};

  const auto manifest_path = report_fragment_manifest_path(report_fragment_dir);
  std::ifstream in(manifest_path, std::ios::binary);
  if (!in) {
    if (error) *error = "manifest file not found: " + manifest_path.string();
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = trim_ascii(line.substr(0, eq));
    const std::string value = line.substr(eq + 1);
    if (!key.empty()) kv[key] = value;
  }

  if (!in.eof() && in.fail()) {
    if (error) *error = "cannot parse manifest: " + manifest_path.string();
    return false;
  }

  const auto read_kv = [&](const char* key, std::string* out_value) {
    if (!out_value) return;
    const auto it = kv.find(key);
    if (it == kv.end()) return;
    *out_value = it->second;
  };

  read_kv("schema", &out->schema);
  if (out->schema.empty()) out->schema = std::string(kReportFragmentManifestSchema);
  if (out->schema != kReportFragmentManifestSchema) {
    if (error) *error = "unsupported report_fragment manifest schema: " + out->schema;
    return false;
  }

  read_kv("family_canonical_path", &out->family_canonical_path);
  read_kv("family", &out->family);
  read_kv("model", &out->model);
  read_kv("report_fragment_id", &out->report_fragment_id);

  const auto file_count_it = kv.find("file_count");
  if (file_count_it == kv.end()) {
    if (error) *error = "manifest missing file_count";
    return false;
  }

  std::uintmax_t file_count_u = 0;
  if (!parse_uintmax(file_count_it->second, &file_count_u)) {
    if (error) *error = "manifest has invalid file_count";
    return false;
  }
  if (file_count_u > 1000000u) {
    if (error) *error = "manifest file_count too large";
    return false;
  }
  const std::size_t file_count = static_cast<std::size_t>(file_count_u);

  out->files.clear();
  out->files.reserve(file_count);
  for (std::size_t i = 0; i < file_count; ++i) {
    const std::string path_key = manifest_file_key(i, "path");
    const std::string size_key = manifest_file_key(i, "size");

    const auto it_path = kv.find(path_key);
    const auto it_size = kv.find(size_key);
    if (it_path == kv.end() || it_size == kv.end()) {
      if (error) *error = "manifest missing indexed file entry";
      return false;
    }

    std::uintmax_t size = 0;
    if (!parse_uintmax(it_size->second, &size)) {
      if (error) *error = "manifest has invalid indexed file size";
      return false;
    }
    if (it_path->second.empty()) {
      if (error) *error = "manifest has empty indexed file path";
      return false;
    }

    out->files.push_back(report_fragment_manifest_file_t{it_path->second, size});
  }
  out->files = normalized_manifest_files(out->files);

  if (out->family_canonical_path.empty() ||
      out->family.empty() ||
      out->model.empty() ||
      out->report_fragment_id.empty()) {
    if (error) *error = "manifest missing family_canonical_path/family/model/report_fragment_id";
    return false;
  }
  return true;
}

[[nodiscard]] inline bool report_fragment_manifest_has_file(const report_fragment_manifest_t& manifest,
                                                     std::string_view path) {
  for (const auto& file : manifest.files) {
    if (file.path == path) return true;
  }
  return false;
}

[[nodiscard]] inline bool is_valid_atom(std::string_view s) {
  if (s.empty()) return false;
  const auto ok_char = [](char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || c == '_';
  };
  const unsigned char first = static_cast<unsigned char>(s.front());
  if (!std::isalnum(first) && s.front() != '_') return false;
  for (char c : s) {
    if (!ok_char(c)) return false;
  }
  return true;
}

[[nodiscard]] inline std::string metadata_secret() {
  const char* env = std::getenv("CUWACUNU_HASHIMYEI_META_SECRET");
  if (env != nullptr && env[0] != '\0') {
    const std::string env_value = trim_ascii(std::string(env));
    if (!env_value.empty()) return env_value;
  }

  try {
    const std::string configured = trim_ascii(cuwacunu::iitepi::config_space_t::get<std::string>(
        "GENERAL", "hashimyei_metadata_secret", std::string{}));
    if (!configured.empty()) return configured;
  } catch (...) {
  }

  return {};
}

[[nodiscard]] inline std::filesystem::path runtime_root() {
  const std::string configured = trim_ascii(cuwacunu::iitepi::config_space_t::get<std::string>(
      "GENERAL", "runtime_root", std::string{}));
  if (!configured.empty()) return std::filesystem::path(configured);
  throw std::runtime_error(
      "missing GENERAL.runtime_root in active global config");
}

[[nodiscard]] inline std::filesystem::path store_root() {
  const char* env = std::getenv("CUWACUNU_HASHIMYEI_STORE_ROOT");
  if (env != nullptr && env[0] != '\0') {
    const std::string env_value = trim_ascii(std::string(env));
    if (!env_value.empty()) return std::filesystem::path(env_value);
  }
  return (runtime_root() / std::string(kRuntimeHashimyeiDirname)).lexically_normal();
}

[[nodiscard]] inline std::filesystem::path catalog_db_path(const std::filesystem::path& root) {
  return catalog_directory(root) / std::string(kCatalogFilename);
}

[[nodiscard]] inline std::filesystem::path catalog_db_path() {
  return catalog_db_path(store_root());
}

[[nodiscard]] inline bool starts_with_weights_filename(const std::filesystem::path& p) {
  const std::string name = p.filename().string();
  return name.rfind("weights", 0) == 0;
}

[[nodiscard]] inline bool read_file_binary(const std::filesystem::path& p,
                                           std::vector<unsigned char>* out,
                                           std::string* error) {
  if (!out) return false;
  out->clear();
  std::ifstream in(p, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file: " + p.string();
    return false;
  }
  in.seekg(0, std::ios::end);
  const std::streamoff sz = in.tellg();
  in.seekg(0, std::ios::beg);
  if (sz < 0) {
    if (error) *error = "cannot read file size: " + p.string();
    return false;
  }
  out->resize(static_cast<std::size_t>(sz));
  if (!out->empty()) {
    in.read(reinterpret_cast<char*>(out->data()), static_cast<std::streamsize>(out->size()));
    if (!in) {
      if (error) *error = "cannot read file contents: " + p.string();
      out->clear();
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool write_file_binary(const std::filesystem::path& p,
                                            const unsigned char* data,
                                            std::size_t size,
                                            std::string* error) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open file for write: " + p.string();
    return false;
  }
  if (size > 0) {
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!out) {
      if (error) *error = "cannot write file contents: " + p.string();
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool decrypt_metadata_text(const std::vector<unsigned char>& encrypted_blob,
                                                std::string* out_plaintext,
                                                std::string* error) {
  if (!out_plaintext) return false;
  out_plaintext->clear();

  if (encrypted_blob.empty()) {
    if (error) *error = "empty encrypted metadata";
    return false;
  }

  const std::string secret = metadata_secret();
  if (secret.empty()) {
    if (error) *error = "metadata secret missing (set GENERAL.hashimyei_metadata_secret or CUWACUNU_HASHIMYEI_META_SECRET)";
    return false;
  }

  std::size_t plain_len = 0;
  unsigned char* plaintext = nullptr;

  if (!cuwacunu::piaabo::dencryption::is_aead_blob(encrypted_blob.data(), encrypted_blob.size())) {
    if (error) *error = "metadata blob format is not AEAD";
    return false;
  }

  plaintext = cuwacunu::piaabo::dencryption::aead_decrypt_blob(
      encrypted_blob.data(), encrypted_blob.size(), secret.c_str(), plain_len);

  if (plaintext == nullptr) {
    if (error) *error = "metadata decryption failed";
    return false;
  }

  out_plaintext->assign(reinterpret_cast<const char*>(plaintext), plain_len);

  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(plaintext, plain_len);
  return true;
}

[[nodiscard]] inline bool encrypt_metadata_text(const std::string& plaintext,
                                                std::vector<unsigned char>* out_encrypted,
                                                std::string* error) {
  if (!out_encrypted) return false;
  out_encrypted->clear();

  const std::string secret = metadata_secret();
  if (secret.empty()) {
    if (error) *error = "metadata secret missing (set GENERAL.hashimyei_metadata_secret or CUWACUNU_HASHIMYEI_META_SECRET)";
    return false;
  }

  std::size_t encrypted_len = 0;
  unsigned char* encrypted = nullptr;

  encrypted = cuwacunu::piaabo::dencryption::aead_encrypt_blob(
      reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size(), secret.c_str(),
      encrypted_len);

  if (encrypted == nullptr) {
    if (error) *error = "metadata encryption failed";
    return false;
  }

  out_encrypted->assign(encrypted, encrypted + encrypted_len);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted, encrypted_len);
  return true;
}

[[nodiscard]] inline bool write_encrypted_metadata(const std::filesystem::path& report_fragment_dir,
                                                   const std::string& metadata_text,
                                                   std::string* error = nullptr) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(report_fragment_dir, ec);
  if (ec) {
    if (error) *error = "cannot create report_fragment directory: " + report_fragment_dir.string();
    return false;
  }

  std::vector<unsigned char> encrypted;
  std::string crypt_error;
  if (!encrypt_metadata_text(metadata_text, &encrypted, &crypt_error)) {
    if (error) *error = crypt_error;
    return false;
  }

  if (!write_file_binary(report_fragment_dir / "metadata.enc", encrypted.data(), encrypted.size(), error)) {
    return false;
  }
  return true;
}

[[nodiscard]] inline report_fragment_metadata_t load_report_fragment_metadata(const std::filesystem::path& report_fragment_dir) {
  report_fragment_metadata_t out{};
  const auto enc_path = report_fragment_dir / "metadata.enc";

  if (!std::filesystem::exists(enc_path)) {
    return out;
  }

  out.present = true;

  std::vector<unsigned char> encrypted;
  std::string read_error;
  if (!read_file_binary(enc_path, &encrypted, &read_error)) {
    out.error = read_error;
    return out;
  }

  std::string plain;
  std::string decrypt_error;
  if (!decrypt_metadata_text(encrypted, &plain, &decrypt_error)) {
    out.error = decrypt_error;
    return out;
  }

  out.decrypted = true;
  out.text = std::move(plain);
  return out;
}

[[nodiscard]] inline std::vector<report_fragment_identity_t> discover_created_report_fragments_for(
    std::string_view family,
    std::string_view model) {
  namespace fs = std::filesystem;
  std::vector<report_fragment_identity_t> out;
  if (!is_valid_atom(family) || !is_valid_atom(model)) return out;

  const fs::path base = canonical_path_directory(
      store_root(), "tsi.wikimyei." + std::string(family) + "." +
                        std::string(model));
  std::error_code ec;
  if (!fs::exists(base, ec) || !fs::is_directory(base, ec)) return out;

  for (const auto& entry : fs::directory_iterator(base, ec)) {
    if (ec) break;
    if (!entry.is_directory()) continue;
    const std::string hash = entry.path().filename().string();
    if (!is_hex_hash_name(hash)) continue;

    report_fragment_identity_t item{};
    item.family = std::string(family);
    item.model = std::string(model);
    item.hashimyei = hash;
    item.canonical_path = "tsi.wikimyei." + item.family + "." + item.model + "." + item.hashimyei;
    item.directory = entry.path();

    const bool has_manifest = report_fragment_manifest_exists(entry.path());
    for (const auto& f : fs::directory_iterator(entry.path(), ec)) {
      if (ec) break;
      if (!f.is_regular_file()) continue;
      if (!starts_with_weights_filename(f.path().filename())) continue;
      item.weight_files.push_back(f.path());
    }
    if (item.weight_files.empty() && !has_manifest) continue;

    std::sort(item.weight_files.begin(), item.weight_files.end());
    item.metadata = load_report_fragment_metadata(entry.path());
    out.push_back(std::move(item));
  }

  std::sort(out.begin(), out.end(), [](const report_fragment_identity_t& a, const report_fragment_identity_t& b) {
    return a.hashimyei < b.hashimyei;
  });
  return out;
}

[[nodiscard]] inline std::vector<report_fragment_identity_t> discover_created_report_fragments_for_type(
    std::string_view tsi_wikimyei_type) {
  const auto segs = split_dot(tsi_wikimyei_type);
  if (segs.size() != 4) return {};
  if (segs[0] != "tsi" || segs[1] != "wikimyei") return {};
  return discover_created_report_fragments_for(segs[2], segs[3]);
}

}  // namespace hashimyei
}  // namespace cuwacunu
