#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Generic, thread-safe configuration access
\*───────────────────────────────────────────────────────────────────────────*/
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#define DEFAULT_CONFIG_FOLDER "/cuwacunu/src/config/"
#define DEFAULT_CONFIG_FILE ".config"
#define DEFAULT_CONTRACT_CONFIG_FILE "default.board.contract.config"
#define GENERAL_BOARD_CONTRACT_CONFIG_KEY "board_contract_config_filename"

#include "piaabo/dfiles.h"
#include "piaabo/dutils.h"

namespace cuwacunu {
namespace piaabo {
namespace dconfig {

enum class exchange_type_e { NONE, TEST, REAL };
using parsed_config_section_t = std::map<std::string, std::string>;
using parsed_config_t = std::map<std::string, parsed_config_section_t>;

extern std::mutex config_mutex;
extern std::mutex contract_config_mutex;

/*───────────────────────────────────────────────────────────────────────────*/
class bad_config_access : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/*───────────────────────────────────────────────────────────────────────────*/
struct config_space_t {
  /*—static data—*/
  static exchange_type_e exchange_type;
  static std::string config_folder;
  static std::string config_file_path;
  static parsed_config_t config;

  /*—initialisation—*/
  static void change_config_file(const char* folder = DEFAULT_CONFIG_FOLDER,
                                 const char* file = DEFAULT_CONFIG_FILE);
  static void update_config();    // re-read from disk
  static bool validate_config();  // throws on fatal errors

  /*—generic accessor—*/
  template <class T = std::string>
  static T get(const std::string& section, const std::string& key,
               std::optional<T> fallback = std::nullopt);

  /*—array accessor—*/
  template <class T = std::string>
  static std::vector<T> get_arr(
      const std::string& section, const std::string& key,
      std::optional<std::vector<T>> fallback = std::nullopt);

  /*—helpers for special resources—*/
  static std::string websocket_url();
  static std::string api_key();
  static std::string aes_salt();
  static std::string Ed25519_pkey();

 private:
  /*—raw readers—*/
  static parsed_config_t read_config(const std::string& path);

  static std::string raw(const std::string& section, const std::string& key);

  template <class T>
  static T from_string(const std::string& s);

  static void init();
  static void finit();
  struct _init {
    _init() { config_space_t::init(); }
  };
  static _init _initializer;
};

/*───────────────────────────────────────────────────────────────────────────*/
struct contract_space_t {
  struct contract_instruction_sections_t {
    std::string observation_sources_dsl{};
    std::string observation_channels_dsl{};
    std::string jkimyei_specs_dsl{};
    std::string tsiemene_circuit_dsl{};
  };

  /*—static data—*/
  static std::string config_folder;
  static std::string config_file_path;
  static parsed_config_t config;

  /*—initialisation—*/
  // Deprecated for runtime use: bootstrap-only once contract runtime lock is active.
  static void change_contract_file(const char* folder = DEFAULT_CONFIG_FOLDER,
                                   const char* file = nullptr);
  // Deprecated for runtime use: bootstrap-only once contract runtime lock is active.
  static void update_config();    // re-read from disk
  static bool validate_config();  // throws on fatal errors

  /*—generic accessor—*/
  template <class T = std::string>
  static T get(const std::string& section, const std::string& key,
               std::optional<T> fallback = std::nullopt);

  /*—array accessor—*/
  template <class T = std::string>
  static std::vector<T> get_arr(
      const std::string& section, const std::string& key,
      std::optional<std::vector<T>> fallback = std::nullopt);

  /*—canonical contract resources—*/
  static std::string observation_sources_grammar();
  static std::string observation_sources_dsl();
  static std::string observation_channels_grammar();
  static std::string observation_channels_dsl();
  static std::string jkimyei_specs_grammar();
  static std::string jkimyei_specs_dsl();
  static std::string tsiemene_circuit_grammar();
  static std::string tsiemene_circuit_dsl();
  static std::string canonical_path_grammar();

  static contract_instruction_sections_t contract_instruction_sections();

 private:
  /*—raw readers—*/
  static parsed_config_t read_config(const std::string& path);

  static std::string raw(const std::string& section, const std::string& key);

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
  parsed_config_t config{};
  std::map<std::string, parsed_config_section_t> module_sections{};
  std::map<std::string, std::string> module_section_paths{};
  std::map<std::string, std::string> dsl_asset_text_by_key{};
  contract_space_t::contract_instruction_sections_t contract_instruction_sections{};
  contract_dependency_manifest_t dependency_manifest{};
};

struct contract_runtime_t {
  static std::shared_ptr<const contract_snapshot_t> active();
  static bool has_active();
  static bool is_locked();

  static void bootstrap_from_global_config();
  static void assert_intact_or_fail_fast();
};

/*───────────────────────────────────────────────────────────────────────────*/
}  // namespace dconfig
}  // namespace piaabo
}  // namespace cuwacunu
