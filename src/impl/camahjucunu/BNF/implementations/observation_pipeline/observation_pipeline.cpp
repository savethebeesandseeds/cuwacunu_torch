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
        // Malformed number → push 0.0f (or skip; choose policy)
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
  log_info("%s\n", OBSERVATION_PIPELINE_BNF_GRAMMAR);
#endif
}

cuwacunu::camahjucunu::observation_instruction_t
observationPipeline::decode(std::string instruction) {
#ifdef OBSERVARION_PIPELINE_DEBUG
  log_info("Request to decode observationPipeline\n");
#endif
  /* guard the thread to avoid multiple decoding in parallel */
  LOCK_GUARD(current_mutex);

  /* Parse the instruction text */
  ASTNodePtr actualAST = iParser.parse_Instruction(instruction);

#ifdef OBSERVARION_PIPELINE_DEBUG
  std::ostringstream oss;
  printAST(actualAST.get(), true, 2, oss);
  log_info("Parsed AST:\n%s\n", oss.str().c_str());
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

// -------------------------
// Helpers: decode text from AST nodes (local to this .cpp)
// -------------------------
static std::string unescape_like_parser(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
        case 'n': result += '\n'; ++i; break;
        case 'r': result += '\r'; ++i; break;
        case 't': result += '\t'; ++i; break;
        case '\\': result += '\\'; ++i; break;
        case '"': result += '"'; ++i; break;
        case '\'': result += '\''; ++i; break;
        default:
          result += '\\';
          result += str[i + 1];
          ++i;
          break;
      }
    } else {
      result += str[i];
    }
  }
  return result;
}

static std::string terminal_text_from_unit(const ProductionUnit& unit) {
  std::string lex = unit.lexeme;

  // Strip surrounding quotes if present
  if (lex.size() >= 2) {
    if ((lex.front() == '"'  && lex.back() == '"') ||
        (lex.front() == '\'' && lex.back() == '\'')) {
      lex = lex.substr(1, lex.size() - 2);
    }
  }

  // Interpret escapes like \\ and \n
  return unescape_like_parser(lex);
}

static std::string trim_spaces_tabs(std::string s) {
  auto is_ws = [](unsigned char c) { return c == ' ' || c == '\t'; };

  size_t start = 0;
  while (start < s.size() && is_ws(static_cast<unsigned char>(s[start]))) start++;

  size_t end = s.size();
  while (end > start && is_ws(static_cast<unsigned char>(s[end - 1]))) end--;

  return s.substr(start, end - start);
}

static void appendAllTerminals(const ASTNode* node, std::string& out) {
  if (!node) return;

  if (auto term = dynamic_cast<const TerminalNode*>(node)) {
    if (term->unit.type == ProductionUnit::Type::Terminal) {
      out += terminal_text_from_unit(term->unit);
    }
    return;
  }

  if (auto root = dynamic_cast<const RootNode*>(node)) {
    for (const auto& ch : root->children) appendAllTerminals(ch.get(), out);
    return;
  }

  if (auto mid = dynamic_cast<const IntermediaryNode*>(node)) {
    for (const auto& ch : mid->children) appendAllTerminals(ch.get(), out);
    return;
  }
}

static std::string flattenNodeText(const ASTNode* node) {
  std::string out;
  appendAllTerminals(node, out);
  return out;
}

static const ASTNode* findDirectChildByHash(const IntermediaryNode* parent, size_t wanted_hash) {
  if (!parent) return nullptr;
  for (const auto& ch : parent->children) {
    if (ch && ch->hash == wanted_hash) return ch.get();
  }
  return nullptr;
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

  auto* out = static_cast<cuwacunu::camahjucunu::observation_instruction_t*>(context.user_data);
  if (!out) return;

  // Clear when entering each table
  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_table) {
    out->instrument_forms.clear();
    return;
  }
  if (node->hash == OBSERVATION_PIPELINE_HASH_input_table) {
    out->input_forms.clear();
    return;
  }

  // Extract a full instrument_form row when we reach it
  if (node->hash == OBSERVATION_PIPELINE_HASH_instrument_form) {
    cuwacunu::camahjucunu::instrument_form_t f{}; // value-init interval too

    const ASTNode* n_instrument  = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_instrument);
    const ASTNode* n_interval    = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_interval);
    const ASTNode* n_record_type = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_record_type);
    const ASTNode* n_norm_window = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_norm_window);
    const ASTNode* n_source      = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_source);

    f.instrument  = trim_spaces_tabs(flattenNodeText(n_instrument));
    std::string interval_s = trim_spaces_tabs(flattenNodeText(n_interval));
    f.record_type = trim_spaces_tabs(flattenNodeText(n_record_type));
    f.norm_window = trim_spaces_tabs(flattenNodeText(n_norm_window));
    f.source      = trim_spaces_tabs(flattenNodeText(n_source)); // IMPORTANT: trims table padding

    // Convert interval string -> enum (guarded)
    try {
      f.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(interval_s);
    } catch (...) {
      // leave default value if conversion fails
    }

    out->instrument_forms.push_back(std::move(f));
    return;
  }

  // Extract a full input_form row when we reach it
  if (node->hash == OBSERVATION_PIPELINE_HASH_input_form) {
    cuwacunu::camahjucunu::input_form_t f{}; // value-init interval too

    const ASTNode* n_interval          = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_interval);
    const ASTNode* n_active            = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_active);
    const ASTNode* n_record_type       = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_record_type);
    const ASTNode* n_seq_length        = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_seq_length);
    const ASTNode* n_future_seq_length = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_future_seq_length);
    const ASTNode* n_channel_weight    = findDirectChildByHash(node, OBSERVATION_PIPELINE_HASH_channel_weight);

    std::string interval_s   = trim_spaces_tabs(flattenNodeText(n_interval));
    f.active                 = trim_spaces_tabs(flattenNodeText(n_active));
    f.record_type            = trim_spaces_tabs(flattenNodeText(n_record_type));
    f.seq_length             = trim_spaces_tabs(flattenNodeText(n_seq_length));
    f.future_seq_length      = trim_spaces_tabs(flattenNodeText(n_future_seq_length));
    f.channel_weight         = trim_spaces_tabs(flattenNodeText(n_channel_weight));

    try {
      f.interval = cuwacunu::camahjucunu::exchange::string_to_enum<cuwacunu::camahjucunu::exchange::interval_type_e>(interval_s);
    } catch (...) {
      // leave default value
    }

    out->input_forms.push_back(std::move(f));
    return;
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

  (void)node;
  (void)context;
  // NOTE:
  // We decode rows at the <instrument_form>/<input_form> IntermediaryNode level now.
  // This avoids relying on context.stack size/index assumptions.
}

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
