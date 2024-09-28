#include "piaabo/dconfig.h"

RUNTIME_WARNING("(dconfig.h)[] remove comments in the config files.\n");

namespace cuwacunu {
namespace piaabo {
namespace dconfig {
std::mutex config_mutex;
std::string config_space_t::config_folder;
std::string config_space_t::learning_config_path;
std::string config_space_t::environment_config_path;
parsed_config_t config_space_t::learning_config;
parsed_config_t config_space_t::environment_config;

parsed_config_t config_space_t::read_config(std::string conf_path) {
  LOCK_GUARD(config_mutex);
  log_dbg("Reading config file [%s]\n", conf_path.c_str());

  std::ifstream configFile(conf_path);
  if (!configFile) {
    log_fatal("Unable to open config file [%s], try adding function update_config(\"path\\to\\config_folder/\") to the code base \n", conf_path.c_str());
  }

  std::string line;
  parsed_config_t dconfig;
  while (getline(configFile, line)) {
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
      std::string key = piaabo::trim_string(line.substr(0, pos));
      std::string value = piaabo::trim_string(line.substr(pos + 1));
      dconfig[key] = value;
    }
  }
  configFile.close();

  if(dconfig.empty()) {
    log_warn("Configuration file [%s] is empty. \n", conf_path.c_str());
  }

  return dconfig;
}


void config_space_t::update_config(const char *config_folder) {
  /* update paths */
  config_space_t::config_folder = std::string(config_folder);
  config_space_t::learning_config_path = config_space_t::config_folder + LEARNING_CONFIG_PATH;
  config_space_t::environment_config_path = config_space_t::config_folder + ENVIROMENT_CONFIG_PATH;

  /* update config */
  config_space_t::learning_config = read_config(config_space_t::learning_config_path.c_str());
  config_space_t::environment_config = read_config(config_space_t::environment_config_path.c_str());
}

static std::vector<std::string> active_symbols() {
  if (config_space_t::environment_config.find("ACTIVE_SYMBOLS") != config_space_t::environment_config.end()) {
    log_fatal("ACTIVE_SYMBOLS is not present in the [%s]\n", config_space_t::learning_config_path.c_str());
  }

  return piaabo::split_string(config_space_t::environment_config["ACTIVE_SYMBOLS"], ',');
}

void config_space_t::finit() {}

void config_space_t::init() {
  /* set up default paths */
  config_space_t::config_folder = std::string(DEFAULT_CONFIG_FOLDER);
  config_space_t::learning_config_path = config_space_t::config_folder + LEARNING_CONFIG_PATH;
  config_space_t::environment_config_path = config_space_t::config_folder + ENVIROMENT_CONFIG_PATH;

  std::atexit(config_space_t::finit);
}

/* initialize */
config_space_t::_init config_space_t::_initializer;
} /* namespace dconfig */
} /* namespace piaabo */
} /* namespace cuwacunu */

cuwacunu::piaabo::dconfig::parsed_config_t* environment_config = &cuwacunu::piaabo::dconfig::config_space_t::environment_config;
cuwacunu::piaabo::dconfig::parsed_config_t* learning_config = &cuwacunu::piaabo::dconfig::config_space_t::learning_config;
void (*update_config)(const char *config_folder) = &cuwacunu::piaabo::dconfig::config_space_t::update_config;