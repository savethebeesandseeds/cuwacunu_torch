#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace camahjucunu {
struct tsiemene_wave_set_t;
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace cuwacunu {
namespace iitepi {

using wave_hash_t = std::string;
extern std::mutex wave_config_mutex;

struct wave_record_t;

struct wave_space_t {
  static wave_hash_t register_wave_file(const std::string& path);
  static std::shared_ptr<const wave_record_t> wave_itself(const wave_hash_t& hash);
  static void assert_intact_or_fail_fast(const wave_hash_t& hash);
  static void assert_registry_intact_or_fail_fast();
  [[nodiscard]] static bool has_wave(const wave_hash_t& hash) noexcept;
  [[nodiscard]] static std::vector<wave_hash_t> registered_hashes();
};

struct wave_file_fingerprint_t {
  std::string canonical_path{};
  std::uintmax_t file_size_bytes{};
  std::int64_t mtime_ticks{};
  std::string sha256_hex{};
};

struct wave_dependency_manifest_t {
  std::vector<wave_file_fingerprint_t> files{};
  std::string aggregate_sha256_hex{};
};

namespace wave_record {

struct dsl_blob_t {
  std::string grammar{};
  std::string dsl{};
};

struct wave_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::tsiemene_wave_set_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::tsiemene_wave_set_t> decoded_cache_{};
};

}  // namespace wave_record

struct wave_record_t {
  std::string config_folder{};
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  wave_record::wave_blob_t wave{};
  wave_dependency_manifest_t dependency_manifest{};

  template <class T = std::string>
  T get(const std::string& section,
        const std::string& key,
        std::optional<T> fallback = std::nullopt) const;

  template <class T = std::string>
  std::vector<T> get_arr(
      const std::string& section,
      const std::string& key,
      std::optional<std::vector<T>> fallback = std::nullopt) const;

 private:
  std::string raw(const std::string& section, const std::string& key) const;

  template <class T>
  static T from_string(const std::string& s);
};

}  // namespace iitepi
}  // namespace cuwacunu

