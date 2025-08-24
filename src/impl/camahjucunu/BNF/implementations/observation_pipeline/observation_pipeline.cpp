/* observation_pipeline.cpp */
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

RUNTIME_WARNING("(observation_pipeline.cpp)[] mutex on observation pipeline might not be needed \n");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

std::vector<instrument_form_t> observation_instruction_t::filter_instrument_forms(
  const std::string& target_instrument,
  const std::string& target_record_type, 
  cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const
{
  std::vector<instrument_form_t> result;
  for (const auto& form : instrument_forms) {
    if (form.instrument == target_instrument && form.record_type == target_record_type && form.interval == target_interval) {
      result.push_back(form);
    }
  }
  return result;
}


observationPipeline::observationPipeline() :
  bnfLexer(OBSERVATION_PIPELINE_BNF_GRAMMAR), 
  bnfParser(bnfLexer), 
  grammar(parseBnfGrammar()), 
  iParser(iLexer, grammar) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << OBSERVATION_PIPELINE_BNF_GRAMMAR << "\n";
#endif
}

observation_instruction_t observationPipeline::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << "Request to decode observationPipeline" << "\n";
#endif
  /* guard the thread to avoid multiple decoding in parallel */
  LOCK_GUARD(current_mutex);
  /* Parse the instruction text */
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);
  
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << "Parsed AST:\n";
  printAST(actualAST.get(), true, 2, std::cout);
#endif
  
  /* Parsed data */
  observation_instruction_t current;
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

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------      Clear Vectors      ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  /* clear instrument_forms */
  if( context.stack.size() == 2 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table) {
    /* clear the contents of instrument_forms */
    static_cast<observation_instruction_t*>(context.user_data)->instrument_forms.clear();
  }

  /* clear input_forms */
  if( context.stack.size() == 2 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table) {
    /* clear the contents of input_forms */
    static_cast<observation_instruction_t*>(context.user_data)->input_forms.clear();
  }

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------    Append New Element   ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  /* append instrument_forms */
  if(  context.stack.size() == 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    /* append a new element */
    static_cast<observation_instruction_t*>(context.user_data)->instrument_forms.emplace_back();
  }

  /* append input_forms */
  if(  context.stack.size() == 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    /* append a new element */
    static_cast<observation_instruction_t*>(context.user_data)->input_forms.emplace_back();
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

  /* ----------------------- ----------------------- ----------------------- */
  /* -----------------------  Assign Vector Fields   ----------------------- */
  /* ----------------------- ----------------------- ----------------------- */
  /* assign instrument_forms fields */
  if(  context.stack.size() > 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    /* retrive the last element */
    instrument_form_t& element = static_cast<observation_instruction_t*>(context.user_data)->instrument_forms.back();
    
    /* instrument */
    if(  context.stack.size() == 5 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_instrument
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_letter) {
      /* assign "instrument" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.instrument += aux;
    }

    /* interval */
    if(  context.stack.size() == 4 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_interval) {
      /* assign "interval" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(aux);
    }

    /* record_type */
    if(  context.stack.size() == 4 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_record_type) {
      /* assign "record_type" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.record_type += aux;
    }

    /* norm_window */
    if(  context.stack.size() == 5 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_norm_window
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      /* assign "norm_window" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.norm_window += aux;
    }

    /* source */
    if(  context.stack.size() == 7 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_source
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_file_path
      && context.stack[5]->hash == OBSERVATION_PIPELINE_HASH_literal) {
      /* assign "source" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.source += aux;
    }
  }

  /* assign input_forms fields */
  if(  context.stack.size() > 3 
    && context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction 
    && context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table
    && context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    /* retrive the last element */
    input_form_t& element = static_cast<observation_instruction_t*>(context.user_data)->input_forms.back();
    
    /* interval */
    if(  context.stack.size() == 4 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_interval) {
      /* assign "interval" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(aux);
    }

    /* active */
    if(  context.stack.size() == 5 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_active
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_boolean) {
      /* assign "active" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.active += aux;
    }

    /* record_type */
    if(  context.stack.size() == 4 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_record_type) {
      /* assign "record_type" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.record_type += aux;
    }

    /* seq_length */
    if(  context.stack.size() == 5 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_seq_length
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      /* assign "seq_length" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.seq_length += aux;
    }

    /* future_seq_length */
    if(  context.stack.size() == 5 
      && context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_future_seq_length
      && context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      /* assign "future_seq_length" of the last element in the vector */
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.future_seq_length += aux;
    }
  }
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
