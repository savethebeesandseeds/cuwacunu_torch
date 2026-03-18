#pragma once

#include <iosfwd>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iitepi/config_space_t.h"
#include "iitepi/network_analytics_mode.h"

namespace cuwacunu {
namespace camahjucunu {
struct runtime_binding_instruction_t;
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace cuwacunu {
namespace iitepi {

using runtime_binding_hash_t = std::string;
extern std::mutex runtime_binding_config_mutex;

struct runtime_binding_record_t;

struct runtime_binding_space_t {
  static void init();
  static void init(const std::string& campaign_file_path);
  static void init(const std::string& campaign_file_path,
                   const std::string& binding_id);
  [[nodiscard]] static bool is_initialized() noexcept;
  static runtime_binding_hash_t locked_runtime_binding_hash();
  static std::string locked_campaign_path_canonical();
  static std::string locked_binding_id();
  static runtime_binding_hash_t register_runtime_binding_file(const std::string& path);
  static std::shared_ptr<const runtime_binding_record_t> runtime_binding_itself(const runtime_binding_hash_t& hash);
  static std::string contract_hash_for_binding(const runtime_binding_hash_t& hash,
                                               const std::string& binding_id);
  static std::string wave_hash_for_binding(const runtime_binding_hash_t& hash,
                                           const std::string& binding_id);
  static void network_topology_analytics(std::ostream* out = nullptr,
                                         bool beautify = false);
  static void network_parameter_analytics(std::ostream* out = nullptr,
                                          bool beautify = false);
  static void network_analytics(std::ostream* out = nullptr,
                                bool beautify = false,
                                network_analytics_mode_e mode =
                                    network_analytics_mode_e::Topology);
  static void assert_locked_runtime_intact_or_fail_fast();
  static void assert_intact_or_fail_fast(const runtime_binding_hash_t& hash);
  static void assert_registry_intact_or_fail_fast();
  [[nodiscard]] static bool has_runtime_binding(const runtime_binding_hash_t& hash) noexcept;
  [[nodiscard]] static std::vector<runtime_binding_hash_t> registered_hashes();

 private:
  static void lifecycle_init();
  static void lifecycle_finit();
  struct _init {
    _init()  { runtime_binding_space_t::lifecycle_init(); }
    ~_init() { runtime_binding_space_t::lifecycle_finit(); }
  };
  static _init _initializer;
};

struct runtime_binding_file_fingerprint_t {
  std::string canonical_path{};
  std::uintmax_t file_size_bytes{};
  std::int64_t mtime_ticks{};
  std::string sha256_hex{};
};

struct runtime_binding_dependency_manifest_t {
  std::vector<runtime_binding_file_fingerprint_t> files{};
  std::string dependency_manifest_aggregate_sha256_hex{};
};

namespace runtime_binding_record {

struct dsl_blob_t {
  std::string grammar{};
  std::string dsl{};
};

struct runtime_binding_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::runtime_binding_instruction_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::runtime_binding_instruction_t>
      decoded_cache_{};
};

}  // namespace runtime_binding_record

struct runtime_binding_record_t {
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  runtime_binding_record::runtime_binding_blob_t runtime_binding{};
  runtime_binding_dependency_manifest_t dependency_manifest{};

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
