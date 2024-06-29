#include "piaabo/config_space.h"

RUNTIME_WARNING("(config_space.h)[] remove comments in the config files.\n");

namespace cuwacunu {
namespace piaabo {
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;
std::string config_space_t::config_folder;
std::string config_space_t::learning_config_path;
std::string config_space_t::environment_config_path;
parsed_config_t config_space_t::learning_config;
parsed_config_t config_space_t::environment_config;

parsed_config_t config_space_t::read_config(std::string conf_path) {
  pthread_mutex_lock(&config_mutex);
  log_dbg("Reading config file [%s]\n", conf_path.c_str());

  std::ifstream configFile(conf_path);
  if (!configFile) {
    pthread_mutex_unlock(&config_mutex);
    log_fatal("Unable to open config file [%s] \n", conf_path.c_str());
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
    pthread_mutex_unlock(&config_mutex);
    log_warn("Configuration file [%s] is empty. \n", conf_path.c_str());
  }

  pthread_mutex_unlock(&config_mutex);
  return dconfig;
}


void config_space_t::update() {
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
  std::atexit(config_space_t::finit);
  config_space_t::config_folder = std::string(CONFIG_FOLDER);
  config_space_t::learning_config_path = config_space_t::config_folder + LEARNING_CONFIG_PATH;
  config_space_t::environment_config_path = config_space_t::config_folder + ENVIROMENT_CONFIG_PATH;
  update();
}

/* initialize */
config_space_t::_init config_space_t::_initializer;
} /* namespace piaabo */
} /* namespace cuwacunu */

cuwacunu::piaabo::parsed_config_t* environment_config = &cuwacunu::piaabo::config_space_t::environment_config;
cuwacunu::piaabo::parsed_config_t* learning_config = &cuwacunu::piaabo::config_space_t::learning_config;
void (*update_config)() = &cuwacunu::piaabo::config_space_t::update;