#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Immutable hash-keyed runtime contract snapshot registry
\*───────────────────────────────────────────────────────────────────────────*/

#include "piaabo/dconfig/config_space_t.h"

namespace cuwacunu {
namespace piaabo {
namespace dconfig {

extern std::mutex contract_config_mutex;
using contract_hash_t = std::string;

struct contract_snapshot_t;

/*───────────────────────────────────────────────────────────────────────────*/
struct contract_space_t {
  struct contract_instruction_sections_t {
    std::string wave_profile_id{};
    std::string observation_sources_dsl{};
    std::string observation_channels_dsl{};
    std::string jkimyei_specs_dsl{};
    std::string tsiemene_circuit_dsl{};
    std::string tsiemene_wave_dsl{};
  };

  /*—registry lifecycle—*/
  static contract_hash_t register_contract_file(const std::string& path);
  static const contract_snapshot_t& snapshot(const contract_hash_t& hash);

  /*—generic accessor—*/
  template <class T = std::string>
  static T get(const contract_hash_t& hash, const std::string& section,
               const std::string& key,
               std::optional<T> fallback = std::nullopt);

  /*—array accessor—*/
  template <class T = std::string>
  static std::vector<T> get_arr(const contract_hash_t& hash,
      const std::string& section, const std::string& key,
      std::optional<std::vector<T>> fallback = std::nullopt);

  /*—canonical contract resources—*/
  static std::string observation_sources_grammar(const contract_hash_t& hash);
  static std::string observation_sources_dsl(const contract_hash_t& hash);
  static std::string observation_channels_grammar(const contract_hash_t& hash);
  static std::string observation_channels_dsl(const contract_hash_t& hash);
  static std::string jkimyei_specs_grammar(const contract_hash_t& hash);
  static std::string jkimyei_specs_dsl(const contract_hash_t& hash);
  static std::string tsiemene_circuit_grammar(const contract_hash_t& hash);
  static std::string tsiemene_circuit_dsl(const contract_hash_t& hash);
  static std::string tsiemene_wave_grammar(const contract_hash_t& hash);
  static std::string tsiemene_wave_dsl(const contract_hash_t& hash);
  static std::string canonical_path_grammar(const contract_hash_t& hash);

  static contract_instruction_sections_t contract_instruction_sections(
      const contract_hash_t& hash);
  static void assert_intact_or_fail_fast(const contract_hash_t& hash);
  static void assert_registry_intact_or_fail_fast();

 private:
  /*—raw readers—*/
  static std::string raw(const contract_hash_t& hash, const std::string& section,
                         const std::string& key);

  template <class T>
  static T from_string(const std::string& s);
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

struct contract_snapshot_t {
  std::string config_folder{};
  std::string config_file_path{};
  std::string config_file_path_canonical{};
  parsed_config_t config{};
  std::map<std::string, parsed_config_section_t> module_sections{};
  std::map<std::string, std::string> module_section_paths{};
  std::map<std::string, std::string> dsl_asset_text_by_key{};
  contract_space_t::contract_instruction_sections_t contract_instruction_sections{};
  contract_dependency_manifest_t dependency_manifest{};
};

/*───────────────────────────────────────────────────────────────────────────*/
}  // namespace dconfig
}  // namespace piaabo
}  // namespace cuwacunu
