#include "piaabo/dconfig.h"

RUNTIME_WARNING("(dconfig.h)[] program does not read the configuration file automatically, this is to prevent unexpected defaulted config\n");

#define TEST_CONFIG_SECTION(prove, section_name, section_value_key) \
  do { \
    /* Check if the section exists */ \
    if(config_space_t::config.find(section_name) == config_space_t::config.end()) {\
      log_warn("Config file (%s) is missing section [%s] \n", \
        config_space_t::config_file_path.c_str(), section_name); \
      *prove &= false; \
    } \
    /* Check if the key exists within the section */ \
    else if(config_space_t::config[section_name].find(section_value_key) == config_space_t::config[section_name].end()) {\
      log_warn("Config file (%s) is missing field <%s = ...> in section [%s] \n", \
        config_space_t::config_file_path.c_str(), \
        section_value_key, section_name); \
      *prove &= false; \
    } \
  } while(0)

#define TEST_CONFIG_OPTION(prove, section_name, section_value_key, ...) \
  do { \
    /* Check if the section exists */ \
    if(config_space_t::config.find(section_name) == config_space_t::config.end()) {\
      log_warn("Config file (%s) is missing section [%s]\n", \
        config_space_t::config_file_path.c_str(), section_name); \
      *prove &= false; \
    } \
    /* Check if the key exists within the section */ \
    else if(config_space_t::config[section_name].find(section_value_key) == config_space_t::config[section_name].end()) {\
      log_warn("Config file (%s) is missing field <%s = ...> in section [%s]\n", \
        config_space_t::config_file_path.c_str(), \
        section_value_key, section_name); \
      *prove &= false; \
    } \
    /* Validate the value against allowed options */ \
    else { \
      std::string value = config_space_t::config[section_name][section_value_key]; \
      std::vector<std::string> allowed_options = { __VA_ARGS__ }; \
      if (std::find(allowed_options.begin(), allowed_options.end(), value) == allowed_options.end()) { \
        std::string allowed = join_strings(allowed_options); \
        log_warn("Config file (%s) has invalid instance [%s] <%s = %s>. Expected one of: [%s]\n", \
          config_space_t::config_file_path.c_str(), section_name, section_value_key, value.c_str(), allowed.c_str()); \
        *prove &= false; \
      } \
    } \
  } while(0)


namespace cuwacunu {
namespace piaabo {
namespace dconfig {
std::mutex config_mutex;
exchange_type_e config_space_t::exchange_type;
std::string config_space_t::config_folder;
std::string config_space_t::config_file_path;
parsed_config_t config_space_t::config;

/* util access methods */
// std::vector<std::string> config_space_t::active_symbols() {
//   if (config_space_t::environment_config.find("ACTIVE_SYMBOLS") != config_space_t::environment_config.end()) {
//     log_fatal("ACTIVE_SYMBOLS is not present in the [%s]\n", config_space_t::learning_config_path.c_str());
//   }

//   return piaabo::split_string(config_space_t::environment_config["ACTIVE_SYMBOLS"], ',');
// }

std::string config_space_t::websocket_url() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["websocket_url"];
}

std::string config_space_t::api_key() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["EXCHANGE_api_filename"];
}

std::string config_space_t::aes_salt() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["AES_salt"];
}

std::string config_space_t::Ed25519_pkey() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["Ed25519_pkey"];
}

/* instruction methods */
parsed_config_t config_space_t::read_config(const std::string& conf_path) {
  LOCK_GUARD(config_mutex);
  log_dbg("Reading config file (%s)\n", 
    conf_path.c_str());

  std::ifstream configFile(conf_path);
  if (!configFile) {
    log_fatal("Unable to open config file (%s). Ensure the file exists and is accessible.\n", 
      conf_path.c_str());
  }

  std::string line;
  parsed_config_t new_config;
  std::string current_section;

  while (std::getline(configFile, line)) {
    line = piaabo::trim_string(line);

    /* skip empty lines and comments */
    if (line.empty() || line[0] == '#' || line[0] == ';') {
      continue;
    }

    /* check if the line defines a section */
    if (line.front() == '[' && line.back() == ']') {
      current_section = line.substr(1, line.size() - 2);
      if(new_config.find(current_section) != new_config.end()) {
        new_config[current_section] = std::map<std::string, std::string>();
      }
      continue;
    }

    /* parse the key-value pair in the current context */
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
      std::string key = piaabo::trim_string(line.substr(0, pos));
      std::string value = piaabo::trim_string(line.substr(pos + 1));
      new_config[current_section][key] = value;
    }
  }

  configFile.close();

  return new_config;
}

void config_space_t::update_config() {
  /* update config */
  config_space_t::config = read_config(config_space_t::config_file_path.c_str());
  
  /* validate configuration */
  config_space_t::validate_config();

  /* if configuration is valid, set exchange_type*/
  if(config_space_t::exchange_type == exchange_type_e::NONE) {
    /* on the first run, exchange type is expected to be NONE */
    config_space_t::exchange_type = config_space_t::config["GENERAL"]["exchange_type"] == "REAL" ? exchange_type_e::REAL : exchange_type_e::TEST;
    /* log the configured setting */
    log_info("Configuration set (...)[GENERAL](exchange_type) = %s\n", 
      config_space_t::exchange_type == exchange_type_e::REAL ? "REAL" : "TEST");
  }
}

void config_space_t::change_config_file(const char *config_folder, const char *config_file) {
  /* update paths */
  config_space_t::config_folder = config_folder;
  config_space_t::config_file_path = config_space_t::config_folder + config_file;
  /* update config */
  config_space_t::update_config();
}

bool config_space_t::validate_config() {
  bool is_valid = true;
  /* test expected fields per section */
  { /* GENERAL */
    TEST_CONFIG_OPTION(&is_valid, "GENERAL", "exchange_type", "REAL", "TEST");
  }
  { /* LEARNING */
    TEST_CONFIG_SECTION(&is_valid, "LEARNING", "epochs");
    TEST_CONFIG_SECTION(&is_valid, "LEARNING", "max_episode_steps");
  }
  { /* ENVIRONMENT */
    TEST_CONFIG_SECTION(&is_valid, "ENVIRONMENT", "absolute_base_symbol");
  }
  { /* REAL_EXCHANGE */
    TEST_CONFIG_SECTION(&is_valid, "REAL_EXCHANGE", "AES_salt");
    TEST_CONFIG_SECTION(&is_valid, "REAL_EXCHANGE", "Ed25519_pkey");
    TEST_CONFIG_SECTION(&is_valid, "REAL_EXCHANGE", "EXCHANGE_api_filename");
    TEST_CONFIG_SECTION(&is_valid, "REAL_EXCHANGE", "websocket_url");
  }
  { /* TEST_EXCHANGE */
    TEST_CONFIG_SECTION(&is_valid, "TEST_EXCHANGE", "AES_salt");
    TEST_CONFIG_SECTION(&is_valid, "TEST_EXCHANGE", "Ed25519_pkey");
    TEST_CONFIG_SECTION(&is_valid, "TEST_EXCHANGE", "EXCHANGE_api_filename");
    TEST_CONFIG_SECTION(&is_valid, "TEST_EXCHANGE", "websocket_url");
  }
  /* faltal termination on non-valid */
  if(!is_valid) {
    log_terminate_gracefully("Incorrect configuration file (%s), program will be terminated. \n",
      config_space_t::config_file_path.c_str());
  }
  /* if configuration is found set, then it is expected this setting not to have change */
  if(config_space_t::exchange_type == exchange_type_e::REAL && config_space_t::config["GENERAL"]["exchange_type"] != "REAL") {
    log_terminate_gracefully("Changing (%s)[%s]<%s> at runtime is not permited, program will be terminated.\n",
      config_space_t::config_file_path.c_str(), "GENERAL", "exchange_type");
  }
  if(config_space_t::exchange_type == exchange_type_e::TEST && config_space_t::config["GENERAL"]["exchange_type"] != "TEST") {
    log_terminate_gracefully("Changing (%s)[%s]<%s> at runtime is not permited, program will be terminated.\n",
      config_space_t::config_file_path.c_str(), "GENERAL", "exchange_type");
  }

  return is_valid;
}

void config_space_t::finit() {}

void config_space_t::init() {
  /* set exchange type */
  config_space_t::exchange_type = exchange_type_e::NONE;
  // /* set up default paths */
  // config_space_t::change_config_file(); /* default */
  // /* update config */
  // config_space_t::update_config();
  /* set cleanup procedures */
  std::atexit(config_space_t::finit);
}

/* initialize */
config_space_t::_init config_space_t::_initializer;
} /* namespace dconfig */
} /* namespace piaabo */
} /* namespace cuwacunu */