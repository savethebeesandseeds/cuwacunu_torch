#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace camahjucunu {
struct tsiemene_board_instruction_t;
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace cuwacunu {
namespace iitepi {

using board_hash_t = std::string;
extern std::mutex board_config_mutex;

struct board_record_t;

struct board_space_t {
  static void init();
  static void init(const std::string& board_file_path,
                   const std::string& board_binding_id);
  [[nodiscard]] static bool is_initialized() noexcept;
  static board_hash_t locked_board_hash();
  static std::string locked_board_path_canonical();
  static std::string locked_board_binding_id();
  static board_hash_t register_board_file(const std::string& path);
  static std::shared_ptr<const board_record_t> board_itself(const board_hash_t& hash);
  static std::string contract_hash_for_binding(const board_hash_t& hash,
                                               const std::string& binding_id);
  static std::string wave_hash_for_binding(const board_hash_t& hash,
                                           const std::string& binding_id);
  static void assert_locked_runtime_intact_or_fail_fast();
  static void assert_intact_or_fail_fast(const board_hash_t& hash);
  static void assert_registry_intact_or_fail_fast();
  [[nodiscard]] static bool has_board(const board_hash_t& hash) noexcept;
  [[nodiscard]] static std::vector<board_hash_t> registered_hashes();
};

struct board_file_fingerprint_t {
  std::string canonical_path{};
  std::uintmax_t file_size_bytes{};
  std::int64_t mtime_ticks{};
  std::string sha256_hex{};
};

struct board_dependency_manifest_t {
  std::vector<board_file_fingerprint_t> files{};
  std::string aggregate_sha256_hex{};
};

namespace board_record {

struct dsl_blob_t {
  std::string grammar{};
  std::string dsl{};
};

struct board_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::tsiemene_board_instruction_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::tsiemene_board_instruction_t>
      decoded_cache_{};
};

}  // namespace board_record

struct board_record_t {
  std::string config_folder{};
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  board_record::board_blob_t board{};
  board_dependency_manifest_t dependency_manifest{};

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
