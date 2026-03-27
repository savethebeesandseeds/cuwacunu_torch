#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "iitepi/runtime_binding_space_t.h"

namespace cuwacunu {
namespace iitepi {

using board_hash_t = runtime_binding_hash_t;
using board_file_fingerprint_t = runtime_binding_file_fingerprint_t;
using board_dependency_manifest_t = runtime_binding_dependency_manifest_t;

namespace board_record {

struct board_blob_t {
  std::shared_ptr<const runtime_binding_record_t> runtime_binding_record{};

  const cuwacunu::camahjucunu::runtime_binding_instruction_t& decoded() const {
    return runtime_binding_record->runtime_binding.decoded();
  }
};

}  // namespace board_record

struct board_record_t {
  std::shared_ptr<const runtime_binding_record_t> runtime_binding_record{};
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  std::string config_folder{};
  parsed_config_t config{};
  board_record::board_blob_t board{};
  board_dependency_manifest_t dependency_manifest{};

  template <class T = std::string>
  T get(const std::string& section,
        const std::string& key,
        std::optional<T> fallback = std::nullopt) const {
    return runtime_binding_record->get<T>(section, key, std::move(fallback));
  }

  template <class T = std::string>
  std::vector<T> get_arr(
      const std::string& section,
      const std::string& key,
      std::optional<std::vector<T>> fallback = std::nullopt) const {
    return runtime_binding_record->get_arr<T>(section, key, std::move(fallback));
  }
};

struct board_space_t {
  static board_hash_t register_board_file(const std::string& path) {
    return runtime_binding_space_t::register_runtime_binding_file(path);
  }

  static std::shared_ptr<const board_record_t> board_itself(
      const board_hash_t& hash) {
    const auto runtime_binding_record =
        runtime_binding_space_t::runtime_binding_itself(hash);

    auto board_record = std::make_shared<board_record_t>();
    board_record->runtime_binding_record = runtime_binding_record;
    board_record->config_file_path = runtime_binding_record->config_file_path;
    board_record->config_file_path_canonical =
        runtime_binding_record->config_file_path_canonical;
    board_record->config =
        runtime_binding_record->config;
    board_record->dependency_manifest =
        runtime_binding_record->dependency_manifest;
    board_record->board.runtime_binding_record = runtime_binding_record;

    const std::filesystem::path config_path(board_record->config_file_path);
    if (config_path.has_parent_path()) {
      board_record->config_folder = config_path.parent_path().string();
    }
    return board_record;
  }

  static std::string contract_hash_for_binding(const board_hash_t& hash,
                                               const std::string& binding_id) {
    return runtime_binding_space_t::contract_hash_for_binding(hash, binding_id);
  }

  static std::string wave_hash_for_binding(const board_hash_t& hash,
                                           const std::string& binding_id) {
    return runtime_binding_space_t::wave_hash_for_binding(hash, binding_id);
  }

  static void assert_intact_or_fail_fast(const board_hash_t& hash) {
    runtime_binding_space_t::assert_intact_or_fail_fast(hash);
  }

  static void assert_registry_intact_or_fail_fast() {
    runtime_binding_space_t::assert_registry_intact_or_fail_fast();
  }

  [[nodiscard]] static bool has_board(const board_hash_t& hash) noexcept {
    return runtime_binding_space_t::has_runtime_binding(hash);
  }

  [[nodiscard]] static std::vector<board_hash_t> registered_hashes() {
    return runtime_binding_space_t::registered_hashes();
  }
};

}  // namespace iitepi
}  // namespace cuwacunu
