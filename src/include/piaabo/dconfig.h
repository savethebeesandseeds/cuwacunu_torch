#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <mutex>
#define DEFAULT_CONFIG_FOLDER "/cuwacunu/src/config/"
#define DEAFAULT_CONFIG_FILE ".config"

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"

namespace cuwacunu {
namespace piaabo {
namespace dconfig {

enum class exchange_type_e {
  REAL, TEST, NONE
};

using parsed_config_section_t = std::map<std::string, std::string>;
using parsed_config_t = std::map<std::string, parsed_config_section_t>;
extern std::mutex config_mutex;

struct config_space_t {
public:
  static class _init {public:_init(){config_space_t::init();}}_initializer;
  /* struct variables */
  static exchange_type_e exchange_type;
  static std::string config_folder;
  static std::string config_file_path;
  static parsed_config_t config;


  /* utilities */
  static parsed_config_t read_config(const std::string& conf_path);
  static void update_config();
  static void change_config_file(const char *config_folder = DEFAULT_CONFIG_FOLDER, const char *config_file = DEAFAULT_CONFIG_FILE);
  static bool validate_config();

  /* access methods */
  static std::string websocket_url();
  static std::string api_key();
  static std::string aes_salt();
  static std::string Ed25519_pkey();

  static std::string observation_pipeline_bnf();
  static std::string observation_pipeline_instruction();

private:
  static void finit();
  static void init();
};
} /* namespace dconfig */
} /* namespace piaabo */
} /* namespace cuwacunu */