// test_dsl_observation.cpp
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace {

std::string read_text_file(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

int main() {
  try {
    const std::string source_grammar_path =
        "/cuwacunu/src/config/dsl/observation_sources.bnf";
    const std::string source_dsl_path =
        "/cuwacunu/src/config/instructions/observation_sources.dsl";
    const std::string channel_grammar_path =
        "/cuwacunu/src/config/dsl/observation_channels.bnf";
    const std::string channel_dsl_path =
        "/cuwacunu/src/config/instructions/observation_channels.dsl";

    const std::string source_grammar = read_text_file(source_grammar_path);
    const std::string source_dsl = read_text_file(source_dsl_path);
    const std::string channel_grammar = read_text_file(channel_grammar_path);
    const std::string channel_dsl = read_text_file(channel_dsl_path);

    auto decoded = cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
        source_grammar,
        source_dsl,
        channel_grammar,
        channel_dsl);
    if (decoded.source_forms.empty()) {
      std::cerr << "[test_dsl_observation] source_forms is empty\n";
      return 1;
    }
    if (decoded.channel_forms.empty()) {
      std::cerr << "[test_dsl_observation] channel_forms is empty\n";
      return 1;
    }

    std::cout << "[test_dsl_observation] source_forms=" << decoded.source_forms.size()
              << " channel_forms=" << decoded.channel_forms.size() << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_dsl_observation] exception: " << e.what() << "\n";
    return 1;
  }
}
