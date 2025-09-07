/* observation_pipeline.cpp */
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

RUNTIME_WARNING("(observation_pipeline.cpp)[] mutex on observation pipeline might not be needed \n");
RUNTIME_WARNING("(observation_pipeline.cpp)[] observation pipeline object should include and expose the dataloaders, dataloaders should not be external variables \n");

#include <sstream>   // for debug prints if needed
#include <cstdint>   // int64_t
#include <cstdlib>   // std::stoll, std::stod
#include <utility>   // std::move

namespace cuwacunu {
namespace camahjucunu {

/* ───────────────────── observation_pipeline_t statics (move from header) ───────────────────── */
observation_instruction_t observation_pipeline_t::inst{};
observation_pipeline_t::_init observation_pipeline_t::_initializer{};

/* ───────────────────── observation_instruction_t methods ───────────────────── */

std::vector<instrument_form_t> observation_instruction_t::filter_instrument_forms(
  const std::string& target_instrument,
  const std::string& target_record_type,
  cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const
{
  std::vector<instrument_form_t> result;
  for (const auto& form : instrument_forms) {
    if (form.instrument == target_instrument &&
        form.record_type == target_record_type &&
        form.interval == target_interval) {
      result.push_back(form);
    }
  }
  return result;
}

std::vector<float> observation_instruction_t::retrieve_channel_weights() {
  std::vector<float> channel_weights;
  channel_weights.reserve(input_forms.size());
  for (const auto& in_form : input_forms) {
    if (in_form.active == "true") {
      try {
        channel_weights.push_back(static_cast<float>(std::stod(in_form.channel_weight))); // singular field name
      } catch (...) {
        // Malformed number → push 0.0f (or skip; choose your policy)
        channel_weights.push_back(0.0f);
      }
    }
  }
  return channel_weights;
}

int64_t observation_instruction_t::count_channels() {
  int64_t count = 0;
  for (const auto& in_form : input_forms) {
    if (in_form.active == "true") ++count;
  }
  return count;
}

int64_t observation_instruction_t::max_sequence_length() {
  int64_t max_seq = 0;
  for (const auto& in_form : input_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v = static_cast<int64_t>(std::stoll(in_form.seq_length));
        if (v > max_seq) max_seq = v;
      } catch (...) {
        // ignore malformed
      }
    }
  }
  return max_seq;
}

int64_t observation_instruction_t::max_future_sequence_length() {
  int64_t max_fut_seq = 0;
  for (const auto& in_form : input_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v = static_cast<int64_t>(std::stoll(in_form.future_seq_length));
        if (v > max_fut_seq) max_fut_seq = v;
      } catch (...) {
        // ignore malformed
      }
    }
  }
  return max_fut_seq;
}

/* ───────────────────── _t lifecycle ───────────────────── */

void observation_pipeline_t::init() {
  log_info("[observation_pipeline_t] initialising\n");
  update();  // Decode once at startup from config
}

void observation_pipeline_t::finit() {
  log_info("[observation_pipeline_t] finalising\n");
}

void observation_pipeline_t::update() {
  // Decode from the current config value
  auto instr_str = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
  cuwacunu::camahjucunu::BNF::observationPipeline decoder;  // fully qualified
  inst = decoder.decode(std::move(instr_str));
}

} // namespace camahjucunu
} // namespace cuwacunu


/* ───────────────────── BNF namespace implementations ───────────────────── */
namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

observationPipeline::observationPipeline()
  : bnfLexer(OBSERVATION_PIPELINE_BNF_GRAMMAR)
  , bnfParser(bnfLexer)
  , grammar(parseBnfGrammar())
  , iParser(iLexer, grammar)
{
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << OBSERVATION_PIPELINE_BNF_GRAMMAR << "\n";
#endif
}

cuwacunu::camahjucunu::observation_instruction_t
observationPipeline::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::cout << "Request to decode observationPipeline\n";
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
  cuwacunu::camahjucunu::observation_instruction_t current;
  VisitorContext context(static_cast<void*>(&current));

  /* decode and traverse the Abstract Syntax Tree */
  actualAST.get()->accept(*this, context);

  return current;
}

ProductionGrammar observationPipeline::parseBnfGrammar() {
  bnfParser.parseGrammar();
  return bnfParser.getGrammar();
}

void observationPipeline::visit(const RootNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("RootNode context: [%s]  ---> %s\n", oss.str().c_str(), node->lhs_instruction.c_str());
#endif
}

void observationPipeline::visit(const IntermediaryNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("IntermediaryNode context: [%s]  ---> %s\n", oss.str().c_str(), node->alt.str(true).c_str());
#endif

  /* Clear vectors when entering tables */
  if (context.stack.size() == 2 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table) {
    static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->instrument_forms.clear();
  }
  if (context.stack.size() == 2 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table) {
    static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->input_forms.clear();
  }

  /* Append new elements when entering forms */
  if (context.stack.size() == 3 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table &&
      context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->instrument_forms.emplace_back();
  }
  if (context.stack.size() == 3 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table &&
      context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->input_forms.emplace_back();
  }
}

void observationPipeline::visit(const TerminalNode* node, VisitorContext& context) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  for (auto item : context.stack) {
    oss << item->str(false) << ", ";
  }
  log_dbg("TerminalNode context: [%s]  ---> %s\n", oss.str().c_str(), node->unit.str(true).c_str());
#endif

  /* Assign instrument_forms fields */
  if (context.stack.size() > 3 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_instrument_table &&
      context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    cuwacunu::camahjucunu::instrument_form_t& element =
        static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->instrument_forms.back();

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_instrument &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_letter) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.instrument += aux;
    }

    if (context.stack.size() == 4 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_interval) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(aux);
    }

    if (context.stack.size() == 4 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_record_type) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.record_type += aux;
    }

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_norm_window &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.norm_window += aux;
    }

    if (context.stack.size() == 7 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_source &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_file_path &&
        context.stack[5]->hash == OBSERVATION_PIPELINE_HASH_literal) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.source += aux;
    }
  }

  /* Assign input_forms fields */
  if (context.stack.size() > 3 &&
      context.stack[0]->hash == OBSERVATION_PIPELINE_HASH_instruction &&
      context.stack[1]->hash == OBSERVATION_PIPELINE_HASH_input_table &&
      context.stack[2]->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    cuwacunu::camahjucunu::input_form_t& element =
        static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data)->input_forms.back();

    if (context.stack.size() == 4 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_interval) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(aux);
    }

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_active &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_boolean) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.active += aux;
    }

    if (context.stack.size() == 4 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_record_type) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.record_type += aux;
    }

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_seq_length &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.seq_length += aux;
    }

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_future_seq_length &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.future_seq_length += aux;
    }

    if (context.stack.size() == 5 &&
        context.stack[3]->hash == OBSERVATION_PIPELINE_HASH_channel_weight &&
        context.stack[4]->hash == OBSERVATION_PIPELINE_HASH_number) {
      std::string aux = node->unit.lexeme;
      cuwacunu::piaabo::string_remove(aux, '\"');
      element.channel_weight += aux;
    }
  }
}

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
