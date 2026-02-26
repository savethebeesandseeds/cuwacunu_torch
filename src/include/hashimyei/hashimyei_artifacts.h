// ./include/hashimyei/hashimyei_artifacts.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/rand.h>

#include "piaabo/dconfig.h"
#include "piaabo/dencryption.h"
#include "piaabo/dsecurity.h"

namespace cuwacunu {
namespace hashimyei {

struct artifact_metadata_t {
  bool present{false};
  bool decrypted{false};
  std::string text{};
  std::string error{};
};

struct artifact_identity_t {
  std::string family{};
  std::string model{};
  std::string hashimyei{};
  std::string canonical_base{};
  std::filesystem::path directory{};
  std::vector<std::filesystem::path> weight_files{};
  artifact_metadata_t metadata{};
};

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
  if (env != nullptr && env[0] != '\0') return std::string(env);
  return cuwacunu::piaabo::dconfig::config_space_t::get<std::string>(
      "GENERAL",
      "hashimyei_metadata_secret");
}

[[nodiscard]] inline std::filesystem::path store_root() {
  const std::string configured = cuwacunu::piaabo::dconfig::config_space_t::get<std::string>(
      "GENERAL",
      "hashimyei_store_root");
  return std::filesystem::path(configured);
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

[[nodiscard]] inline bool decrypt_metadata_text(const std::vector<unsigned char>& encrypted,
                                                const std::vector<unsigned char>& salt,
                                                std::string* out_plaintext,
                                                std::string* error) {
  if (!out_plaintext) return false;
  out_plaintext->clear();

  if (salt.size() != AES_SALT_LEN) {
    if (error) *error = "invalid metadata salt size";
    return false;
  }
  if (encrypted.empty()) {
    if (error) *error = "empty encrypted metadata";
    return false;
  }

  const std::string secret = metadata_secret();
  if (secret.empty()) {
    if (error) *error = "metadata secret missing (set GENERAL.hashimyei_metadata_secret or CUWACUNU_HASHIMYEI_META_SECRET)";
    return false;
  }

  auto* key = cuwacunu::piaabo::dsecurity::secure_allocate<unsigned char>(AES_KEY_LEN);
  auto* iv = cuwacunu::piaabo::dsecurity::secure_allocate<unsigned char>(AES_IV_LEN);
  std::size_t plain_len = 0;
  unsigned char* plaintext = nullptr;

  cuwacunu::piaabo::dencryption::derive_key_iv(
      secret.c_str(),
      key,
      iv,
      salt.data());
  plaintext = cuwacunu::piaabo::dencryption::aes_decrypt(
      encrypted.data(),
      encrypted.size(),
      key,
      iv,
      plain_len);

  if (plaintext == nullptr) {
    if (error) *error = "metadata decryption failed";
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(key, AES_KEY_LEN);
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(iv, AES_IV_LEN);
    return false;
  }

  out_plaintext->assign(reinterpret_cast<const char*>(plaintext), plain_len);

  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(plaintext, plain_len);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(key, AES_KEY_LEN);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(iv, AES_IV_LEN);
  return true;
}

[[nodiscard]] inline bool encrypt_metadata_text(const std::string& plaintext,
                                                std::vector<unsigned char>* out_encrypted,
                                                std::vector<unsigned char>* out_salt,
                                                std::string* error) {
  if (!out_encrypted || !out_salt) return false;
  out_encrypted->clear();
  out_salt->clear();

  const std::string secret = metadata_secret();
  if (secret.empty()) {
    if (error) *error = "metadata secret missing (set GENERAL.hashimyei_metadata_secret or CUWACUNU_HASHIMYEI_META_SECRET)";
    return false;
  }

  out_salt->resize(AES_SALT_LEN);
  if (RAND_bytes(out_salt->data(), AES_SALT_LEN) != 1) {
    if (error) *error = "cannot generate metadata salt";
    out_salt->clear();
    return false;
  }

  auto* key = cuwacunu::piaabo::dsecurity::secure_allocate<unsigned char>(AES_KEY_LEN);
  auto* iv = cuwacunu::piaabo::dsecurity::secure_allocate<unsigned char>(AES_IV_LEN);
  std::size_t encrypted_len = 0;
  unsigned char* encrypted = nullptr;

  cuwacunu::piaabo::dencryption::derive_key_iv(
      secret.c_str(),
      key,
      iv,
      out_salt->data());
  encrypted = cuwacunu::piaabo::dencryption::aes_encrypt(
      reinterpret_cast<const unsigned char*>(plaintext.data()),
      plaintext.size(),
      key,
      iv,
      encrypted_len);

  if (encrypted == nullptr) {
    if (error) *error = "metadata encryption failed";
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(key, AES_KEY_LEN);
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(iv, AES_IV_LEN);
    out_salt->clear();
    return false;
  }

  out_encrypted->assign(encrypted, encrypted + encrypted_len);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted, encrypted_len);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(key, AES_KEY_LEN);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(iv, AES_IV_LEN);
  return true;
}

[[nodiscard]] inline bool write_encrypted_metadata(const std::filesystem::path& artifact_dir,
                                                   const std::string& metadata_text,
                                                   std::string* error = nullptr) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(artifact_dir, ec);
  if (ec) {
    if (error) *error = "cannot create artifact directory: " + artifact_dir.string();
    return false;
  }

  std::vector<unsigned char> encrypted;
  std::vector<unsigned char> salt;
  std::string crypt_error;
  if (!encrypt_metadata_text(metadata_text, &encrypted, &salt, &crypt_error)) {
    if (error) *error = crypt_error;
    return false;
  }

  if (!write_file_binary(artifact_dir / "metadata.enc", encrypted.data(), encrypted.size(), error)) {
    return false;
  }
  if (!write_file_binary(artifact_dir / "metadata.salt", salt.data(), salt.size(), error)) {
    return false;
  }
  return true;
}

[[nodiscard]] inline artifact_metadata_t load_artifact_metadata(const std::filesystem::path& artifact_dir) {
  artifact_metadata_t out{};
  const auto enc_path = artifact_dir / "metadata.enc";
  const auto salt_path = artifact_dir / "metadata.salt";

  if (!std::filesystem::exists(enc_path) || !std::filesystem::exists(salt_path)) {
    return out;
  }

  out.present = true;

  std::vector<unsigned char> encrypted;
  std::vector<unsigned char> salt;
  std::string read_error;
  if (!read_file_binary(enc_path, &encrypted, &read_error)) {
    out.error = read_error;
    return out;
  }
  if (!read_file_binary(salt_path, &salt, &read_error)) {
    out.error = read_error;
    return out;
  }

  std::string plain;
  std::string decrypt_error;
  if (!decrypt_metadata_text(encrypted, salt, &plain, &decrypt_error)) {
    out.error = decrypt_error;
    return out;
  }

  out.decrypted = true;
  out.text = std::move(plain);
  return out;
}

[[nodiscard]] inline std::vector<artifact_identity_t> discover_created_artifacts_for(
    std::string_view family,
    std::string_view model) {
  namespace fs = std::filesystem;
  std::vector<artifact_identity_t> out;
  if (!is_valid_atom(family) || !is_valid_atom(model)) return out;

  const fs::path base = store_root() / "tsi.wikimyei" / std::string(family) / std::string(model);
  std::error_code ec;
  if (!fs::exists(base, ec) || !fs::is_directory(base, ec)) return out;

  for (const auto& entry : fs::directory_iterator(base, ec)) {
    if (ec) break;
    if (!entry.is_directory()) continue;
    const std::string hash = entry.path().filename().string();
    if (!is_valid_atom(hash)) continue;

    artifact_identity_t item{};
    item.family = std::string(family);
    item.model = std::string(model);
    item.hashimyei = hash;
    item.canonical_base = "tsi.wikimyei." + item.family + "." + item.model + "." + item.hashimyei;
    item.directory = entry.path();

    for (const auto& f : fs::directory_iterator(entry.path(), ec)) {
      if (ec) break;
      if (!f.is_regular_file()) continue;
      if (!starts_with_weights_filename(f.path().filename())) continue;
      item.weight_files.push_back(f.path());
    }
    if (item.weight_files.empty()) continue;

    std::sort(item.weight_files.begin(), item.weight_files.end());
    item.metadata = load_artifact_metadata(entry.path());
    out.push_back(std::move(item));
  }

  std::sort(out.begin(), out.end(), [](const artifact_identity_t& a, const artifact_identity_t& b) {
    return a.hashimyei < b.hashimyei;
  });
  return out;
}

[[nodiscard]] inline std::vector<artifact_identity_t> discover_created_artifacts_for_type(
    std::string_view tsi_wikimyei_type) {
  const auto segs = split_dot(tsi_wikimyei_type);
  if (segs.size() != 4) return {};
  if (segs[0] != "tsi" || segs[1] != "wikimyei") return {};
  return discover_created_artifacts_for(segs[2], segs[3]);
}

}  // namespace hashimyei
}  // namespace cuwacunu
