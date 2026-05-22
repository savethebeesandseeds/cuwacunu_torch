/* runtime/decode.h */
#pragma once

#include <string>

#include "ujcamei/source/contract/contract.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {

struct source_registry_config_paths_t {
  std::string source_registry_dsl_bnf_path{};
  std::string source_registry_dsl_path{};
};

source_universe_t
decode_source_universe_from_split_dsl(std::string source_grammar,
                                      std::string source_instruction);
source_universe_t decode_source_universe_from_config(std::string config_path);
source_universe_t decode_source_universe_from_default_config();
source_spec_t decode_source_spec_from_split_dsl(std::string source_grammar,
                                                std::string source_instruction,
                                                std::string channel_grammar,
                                                std::string channel_instruction,
                                                std::string graph_grammar,
                                                std::string graph_instruction);
std::string default_source_config_path();
source_registry_config_paths_t
load_source_registry_config_paths_from_config(std::string config_path);
source_spec_t decode_source_spec_from_config(std::string config_path);
source_spec_t decode_source_spec_from_default_config();

struct source_runtime_t {
  static source_universe_t inst;
  static void update_from_config(std::string config_path = {});

private:
  static void init();
  static void finit();
  struct _init {
    _init() { source_runtime_t::init(); }
    ~_init() { source_runtime_t::finit(); }
  };
  static _init _initializer;
};

} /* namespace contract */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
