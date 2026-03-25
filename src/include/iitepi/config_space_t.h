#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Generic, thread-safe global configuration access
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

#define DEFAULT_GLOBAL_CONFIG_PATH "/cuwacunu/src/config/.config"
#define DEFAULT_IITEPI_CAMPAIGN_DSL_FILE \
  "instructions/objectives/vicreg.solo/iitepi.campaign.dsl"
#define GENERAL_DEFAULT_IITEPI_CAMPAIGN_DSL_KEY "default_iitepi_campaign_dsl_filename"

#include "piaabo/dfiles.h"
#include "piaabo/dutils.h"

namespace cuwacunu {
namespace iitepi {

enum class exchange_type_e { NONE, TEST, REAL };
using parsed_config_section_t = std::map<std::string, std::string>;
using parsed_config_t = std::map<std::string, parsed_config_section_t>;

extern std::mutex config_mutex;

/*───────────────────────────────────────────────────────────────────────────*/
class bad_config_access : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/*───────────────────────────────────────────────────────────────────────────*/
struct config_space_t {
  /*—static data—*/
  static exchange_type_e exchange_type;
  static std::string config_file_path;
  static std::string runtime_campaign_dsl_override_path;
  static parsed_config_t config;

  /*—initialisation—*/
  static void change_config_file(
      const char* config_path = DEFAULT_GLOBAL_CONFIG_PATH);
  static void update_config();    // re-read from disk
  static bool validate_config();  // throws on fatal errors
  static void set_runtime_campaign_dsl_override(const std::string& path);
  static void clear_runtime_campaign_dsl_override();
  static std::string effective_campaign_dsl_path();
  static std::string locked_campaign_hash();
  static std::string locked_campaign_path_canonical();
  static std::string locked_binding_id();

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
  static std::string Ed25519_pkey();
  static std::string hero_mode();
  static std::string hero_api_key_filename();
  static std::string locked_runtime_binding_hash();

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
}  // namespace iitepi
}  // namespace cuwacunu
