/* observation_pipeline.cpp */
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

RUNTIME_WARNING("(observation_pipeline.cpp)[] mutex on observation pipeline might not be needed \n");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

observationPipeline::observationPipeline() :
  bnfLexer(OBSERVATION_PIPELINE_BNF_GRAMMAR), 
  bnfParser(bnfLexer), 
  grammar(parseBnfGrammar()), 
  iParser(iLexer, grammar) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << OBSERVATION_PIPELINE_BNF_GRAMMAR << "\n";
#endif
}

observation_pipeline_instruction_t observationPipeline::decode(std::string instruction) {
  /* guard the thread to avoid multiple decoding in parallel */
  LOCK_GUARD(current_mutex);

  /* Parse the instruction text */
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);
  
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << "Parsed AST:\n";
  printAST(actualAST.get(), true, 2, std::cout);
#endif
  
  /* Parsed data */
  observation_pipeline_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));

  /* decode and transverse the Abstract Syntax Tree */
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar observationPipeline::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

void observationPipeline::visit(const RootNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
}

void observationPipeline::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif
  /* symbol */
  if( context.stack.size() == 2 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_symbol) {
    /* clear the contents of symbol */
    static_cast<observation_pipeline_instruction_t*>(context.user_data)->symbol.clear();
  }

  /* items */
  if( context.stack.size() == 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_sequence_item
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    /* append a new input_form_t element */
    static_cast<observation_pipeline_instruction_t*>(context.user_data)->items.emplace_back();
  }

}

void observationPipeline::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  /* debug */
  std::ostringstream oss;
  for(auto item: context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif
  /* symbol */
  if( context.stack.size() == 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_symbol 
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_letter) {
    /* append a letter to the symbol */
    std::string aux = node->unit.lexeme;
    cuwacunu::piaabo::string_remove(aux, '\"');
    static_cast<observation_pipeline_instruction_t*>(context.user_data)->symbol += aux;
  }

  /* items */
  if( context.stack.size() == 4 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_sequence_item
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
      /* set up the values for the last element of the items vector */
      input_form_t& element = static_cast<observation_pipeline_instruction_t*>(context.user_data)->items.back();
      
      /* get the lexeme without quotes */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');

      /* interval*/
      if(context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_interval) {

        element.interval = 
          cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(
            aux
          );
      }

      /* count */
      if(context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_count) {
        element.count = std::stoul(aux);
      }
  }
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
