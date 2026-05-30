#pragma once

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "camahjucunu/dsl/parser_types.h"

namespace cuwacunu {
namespace camahjucunu {

struct latent_lineage_state_entry_t {
  std::string key{};
  std::string declared_domain{};
  std::string declared_type{};
  std::string value{};
  std::size_t line{0};
};

struct latent_lineage_state_instruction_t {
  std::vector<latent_lineage_state_entry_t> entries{};
  std::string str() const;
  std::map<std::string, std::string> to_map() const;
};

namespace dsl {

class latentLineageStatePipeline {
 public:
  explicit latentLineageStatePipeline(std::string grammar_text);
  latent_lineage_state_instruction_t decode(std::string instruction);

 private:
  ProductionGrammar parseGrammarDefinition();

  std::mutex current_mutex_{};
  std::string grammar_text_{};
  GrammarLexer grammar_lexer_;
  GrammarParser grammar_parser_;
  ProductionGrammar grammar_;
  InstructionLexer instruction_lexer_;
  InstructionParser instruction_parser_;
};

latent_lineage_state_instruction_t decode_latent_lineage_state_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
