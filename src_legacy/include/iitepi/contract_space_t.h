#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Immutable hash-keyed runtime contract record registry
\*───────────────────────────────────────────────────────────────────────────*/

#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "camahjucunu/dsl/instrument_signature.h"
#include "iitepi/config_space_t.h"
#include "iitepi/network_analytics.h"

namespace cuwacunu {
namespace camahjucunu {
struct tsiemene_circuit_instruction_t;
struct network_design_instruction_t;
} // namespace camahjucunu
} // namespace cuwacunu

namespace cuwacunu {
namespace iitepi {

extern std::mutex contract_config_mutex;
using contract_hash_t = std::string;

struct contract_record_t;

/*───────────────────────────────────────────────────────────────────────────*/
struct contract_space_t {
  /*—registry lifecycle—*/
  static contract_hash_t register_contract_file(const std::string &path);
  static std::shared_ptr<const contract_record_t>
  contract_itself(const contract_hash_t &hash);
  [[nodiscard]] static bool
  validate_contract_file_isolated(const std::string &path,
                                  const std::string &global_config_path = {},
                                  std::string *error = nullptr);
  static void network_topology_analytics(const contract_hash_t &hash,
                                         std::ostream *out = nullptr,
                                         bool beautify = false);
  static void network_parameter_analytics(const contract_hash_t &hash,
                                          std::ostream *out = nullptr,
                                          bool beautify = false);
  static void network_analytics(
      const contract_hash_t &hash, std::ostream *out = nullptr,
      bool beautify = false,
      network_analytics_mode_e mode = network_analytics_mode_e::Topology);
  static void assert_intact_or_fail_fast(const contract_hash_t &hash);
  static void assert_registry_intact_or_fail_fast();
  [[nodiscard]] static std::string sha256_hex_for_file(const std::string &path);
  [[nodiscard]] static bool has_contract(const contract_hash_t &hash) noexcept;
  [[nodiscard]] static std::vector<contract_hash_t> registered_hashes();

private:
  static void lifecycle_init();
  static void lifecycle_finit();
  struct _init {
    _init() { contract_space_t::lifecycle_init(); }
    ~_init() { contract_space_t::lifecycle_finit(); }
  };
  static _init _initializer;
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
  std::string dependency_manifest_aggregate_sha256_hex{};
};

struct contract_component_binding_t {
  std::string binding_id{};
  std::string canonical_path{};
  std::string tsi_type{};
  std::string hashimyei{};
  std::string component_tag{};
  std::string component_compatibility_sha256_hex{};
  cuwacunu::camahjucunu::instrument_signature_t instrument_signature{};
  std::string tsi_dsl_path{};
  std::string tsi_dsl_sha256_hex{};
};

struct contract_module_signature_entry_t {
  std::string module_id{};
  std::string component_tag{};
  std::string module_dsl_path{};
  std::string module_dsl_sha256_hex{};
};

struct contract_variable_assignment_t {
  std::string name{};
  std::string value{};
};

struct contract_instrument_signature_assignment_t {
  std::string binding_id{};
  cuwacunu::camahjucunu::instrument_signature_t signature{};
};

struct contract_component_compatibility_signature_t {
  std::string binding_id{};
  std::string family{};
  std::string component_tag{};
  cuwacunu::camahjucunu::instrument_signature_t instrument_signature{};
  std::string sha256_hex{};
};

struct contract_docking_surface_entry_t {
  std::string surface_id{};
  std::string canonical_path{};
  std::string sha256_hex{};
};

struct contract_docking_signature_t {
  std::string schema{"iitepi.contract.docking_signature.v2"};
  std::vector<std::string> compatible_circuits{};
  std::vector<contract_variable_assignment_t> variable_assignments{};
  std::vector<contract_instrument_signature_assignment_t>
      instrument_signatures{};
  std::vector<contract_docking_surface_entry_t> surfaces{};
  std::string sha256_hex{};
};

struct contract_signature_t {
  std::string circuit_dsl_sha256_hex{};
  std::string docking_signature_sha256_hex{};
  std::vector<contract_component_binding_t> bindings{};
  std::vector<contract_module_signature_entry_t> module_dsl_entries{};
  std::vector<contract_variable_assignment_t> variable_assignments{};
};

namespace contract_record {

struct dsl_blob_t {
  std::string grammar{};
  std::string dsl{};
};

struct circuit_blob_t : dsl_blob_t {
  const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &decoded() const;

private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::tsiemene_circuit_instruction_t>
      decoded_cache_{};
};

struct canonical_path_blob_t {
  std::string grammar{};
};

struct network_design_blob_t : dsl_blob_t {
  [[nodiscard]] bool has_payload() const noexcept { return !dsl.empty(); }
  const cuwacunu::camahjucunu::network_design_instruction_t &decoded() const;

private:
  mutable std::once_flag decode_once_{};
  mutable std::shared_ptr<cuwacunu::camahjucunu::network_design_instruction_t>
      decoded_cache_{};
};

} // namespace contract_record

struct contract_record_t {
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  std::map<std::string, parsed_config_section_t> module_sections{};
  std::map<std::string, std::string> module_section_paths{};

  contract_record::circuit_blob_t circuit{};
  contract_record::canonical_path_blob_t canonical_path{};
  contract_record::network_design_blob_t vicreg_network_design{};
  contract_record::network_design_blob_t expected_value_network_design{};
  std::vector<contract_instrument_signature_assignment_t>
      instrument_signatures{};
  std::vector<contract_component_compatibility_signature_t>
      component_compatibility_signatures{};
  contract_docking_signature_t docking_signature{};
  contract_signature_t signature{};

  contract_dependency_manifest_t dependency_manifest{};

  template <class T = std::string>
  T get(const std::string &section, const std::string &key,
        std::optional<T> fallback = std::nullopt) const;

  template <class T = std::string>
  std::vector<T>
  get_arr(const std::string &section, const std::string &key,
          std::optional<std::vector<T>> fallback = std::nullopt) const;

private:
  std::string raw(const std::string &section, const std::string &key) const;

  template <class T> static T from_string(const std::string &s);
};

[[nodiscard]] inline const contract_component_compatibility_signature_t *
find_component_compatibility_signature(const contract_record_t &record,
                                       std::string_view binding_id) noexcept {
  for (const auto &signature : record.component_compatibility_signatures) {
    if (signature.binding_id == binding_id)
      return &signature;
  }
  return nullptr;
}

[[nodiscard]] bool is_valid_component_tag_token(std::string_view tag) noexcept;
[[nodiscard]] std::string derive_hashimyei_from_component_compatibility_sha256(
    std::string_view component_compatibility_sha256_hex);
[[nodiscard]] std::optional<contract_component_binding_t>
realize_contract_component_binding_for_runtime(
    const contract_record_t &record, std::string_view binding_id,
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    std::string *error = nullptr);

/*───────────────────────────────────────────────────────────────────────────*/
} // namespace iitepi
} // namespace cuwacunu
