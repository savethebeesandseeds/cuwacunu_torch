#pragma once
/*───────────────────────────────────────────────────────────────────────────*\
  Generic, thread-safe configuration access
\*───────────────────────────────────────────────────────────────────────────*/
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <algorithm>

#define DEFAULT_CONFIG_FOLDER "/cuwacunu/src/config/"
#define DEFAULT_CONFIG_FILE   ".config"

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"

namespace cuwacunu {
namespace piaabo {
namespace dconfig {

enum class exchange_type_e { NONE, TEST, REAL };
using parsed_config_section_t = std::map<std::string, std::string>;
using parsed_config_t         = std::map<std::string, parsed_config_section_t>;

extern std::mutex config_mutex;

/*───────────────────────────────────────────────────────────────────────────*/
class bad_config_access : public std::runtime_error {
  public: using std::runtime_error::runtime_error;
};

/*───────────────────────────────────────────────────────────────────────────*/
struct config_space_t {
  /*—static data—*/
  static exchange_type_e  exchange_type;
  static std::string      config_folder;
  static std::string      config_file_path;
  static parsed_config_t  config;

  /*—initialisation—*/
  static void change_config_file(const char* folder = DEFAULT_CONFIG_FOLDER, const char* file   = DEFAULT_CONFIG_FILE);
  static void update_config();    // re-read from disk
  static bool validate_config();  // throws on fatal errors

  /*—generic accessor—*/
  template<class T = std::string>
  static T get(const std::string& section, const std::string& key, std::optional<T> fallback = std::nullopt);
  
  /*—array accessor—*/
  template<class T = std::string>
  static std::vector<T> get_arr(const std::string& section, const std::string& key, std::optional<std::vector<T>> fallback = std::nullopt);

  /*—helpers for special resources—*/
  static std::string websocket_url();
  static std::string api_key();
  static std::string aes_salt();
  static std::string Ed25519_pkey();

  static std::string observation_pipeline_bnf();
  static std::string observation_pipeline_instruction();
  static std::string training_components_bnf();
  static std::string training_components_instruction();
  static std::string tsiemene_board_bnf();
  static std::string tsiemene_board_instruction();

private:
  /*—raw readers—*/
  static parsed_config_t read_config(const std::string& path);

  static std::string raw(const std::string& section, const std::string& key);

  template<class T> static T from_string(const std::string& s);

  static void init();
  static void finit();
  struct _init { _init() { config_space_t::init(); } };
  static _init _initializer;
};

/*───────────────────────────────────────────────────────────────────────────*/
} // namespace dconfig
} // namespace piaabo
} // namespace cuwacunu
