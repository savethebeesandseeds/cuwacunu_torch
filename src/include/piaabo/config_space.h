#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <pthread.h>
#define CONFIG_FOLDER "../config"
#define LEARNING_CONFIG_PATH "/learning.config"
#define ENVIROMENT_CONFIG_PATH "/environment.config"

#include "piaabo/dutils.h"

namespace cuwacunu {
namespace piaabo {
using parsed_config_t = std::map<std::string, std::string>;
extern pthread_mutex_t config_mutex;

struct config_space_t {
public:
  static class _init {public:_init(){config_space_t::init();}}_initializer;
  /* struct variables */
  static std::string config_folder;
  static std::string learning_config_path;
  static std::string environment_config_path;
  static parsed_config_t learning_config;
  static parsed_config_t environment_config;

  /* utilities */
  static parsed_config_t read_config(std::string conf_path);
  static void update();
  static std::vector<std::string> active_symbols();
private:
  static void finit();
  static void init();
};
} /* namespace piaabo */
} /* namespace cuwacunu */
extern cuwacunu::piaabo::parsed_config_t* environment_config;
extern cuwacunu::piaabo::parsed_config_t* learning_config;
extern void (*update_config)();