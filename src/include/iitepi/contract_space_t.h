#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Immutable hash-keyed runtime contract record registry
\*───────────────────────────────────────────────────────────────────────────*/

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace camahjucunu {
struct tsiemene_circuit_instruction_t;
struct observation_spec_t;
struct jkimyei_specs_t;
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace cuwacunu {
namespace iitepi {

extern std::mutex contract_config_mutex;
using contract_hash_t = std::string;

struct contract_record_t;

/*───────────────────────────────────────────────────────────────────────────*/
struct contract_space_t {
  /*—registry lifecycle—*/
  static contract_hash_t register_contract_file(const std::string& path);
  static std::shared_ptr<const contract_record_t> contract_itself(
      const contract_hash_t& hash);
  static void assert_intact_or_fail_fast(const contract_hash_t& hash);
  static void assert_registry_intact_or_fail_fast();
  [[nodiscard]] static bool has_contract(const contract_hash_t& hash) noexcept;
  [[nodiscard]] static std::vector<contract_hash_t> registered_hashes();
};

/*───────────────────────────────────────────────────────────────────────────*/
struct contract_file_fingerprint_t {
  std::string canonical_path{};
  std::uintmax_t file_size_bytes{};
  std::int64_t mtime_ticks{};
  std::string sha256_hex{};
};

struct contract_dependency_manifest_t {
  std::vector<contract_file_fingerprint_t> files{};
  std::string aggregate_sha256_hex{};
};

namespace contract_record {

struct dsl_blob_t {
  std::string grammar{};
  std::string dsl{};
};

struct circuit_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::tsiemene_circuit_instruction_t>
      decoded_cache_{};
};

struct observation_blob_t {
  dsl_blob_t sources{};
  dsl_blob_t channels{};

  const cuwacunu::camahjucunu::observation_spec_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::observation_spec_t>
      decoded_cache_{};
};

struct jkimyei_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::jkimyei_specs_t& decoded() const;

 private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::jkimyei_specs_t>
      decoded_cache_{};
};

struct canonical_path_blob_t {
  std::string grammar{};
};

}  // namespace contract_record

struct contract_record_t {
  std::string config_folder{};
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  std::map<std::string, parsed_config_section_t> module_sections{};
  std::map<std::string, std::string> module_section_paths{};

  contract_record::circuit_blob_t circuit{};
  contract_record::observation_blob_t observation{};
  contract_record::jkimyei_blob_t jkimyei{};
  contract_record::canonical_path_blob_t canonical_path{};

  contract_dependency_manifest_t dependency_manifest{};

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
  std::string raw(const std::string& section,
                  const std::string& key) const;

  template <class T>
  static T from_string(const std::string& s);
};

/*───────────────────────────────────────────────────────────────────────────*/
}  // namespace iitepi
}  // namespace cuwacunu
