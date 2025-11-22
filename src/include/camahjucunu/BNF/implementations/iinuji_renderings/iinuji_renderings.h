/* iinuji_renderings.h */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mutex>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

/* ────────────────────────────────────────────────────────────────────────────
   Instruction (decoded) types
   ──────────────────────────────────────────────────────────────────────────── */

namespace cuwacunu {
namespace camahjucunu {

struct iinuji_renderings_instruction_t {
  struct presenter_t {
    std::string name;
    std::map<std::string,std::string> kv;           // arbitrary k→v
  };

  struct arg_t {
    std::string name;                                // Arg2
    std::string path_from_Arg1;                      // e.g. Arg1.dataloader
    presenter_t presenter;                           // optional
    std::vector<std::pair<std::string,std::string>> triggers; // optional
  };

  struct shape_t {
    enum class kind_e { Curve, MaskScatter, Embedding, MdnBand, Text };
    kind_e kind{kind_e::Curve};
    std::map<std::string,std::string> kv;           // d,y,grid,symmetric,content,…
  };

  struct panel_t {
    std::string id;
    std::string type;                                // plot|embed|text|mdn|matrix|custom
    int x{0}, y{0}, w{1}, h{1};
    int z{0};
    float scale{1.0f};
    std::string bind_arg;                            // ArgN
    std::vector<shape_t> shapes;                     // draw …
  };

  struct screen_t {
    int fcode{0};                                    // F+<n>
    std::string title;                               // optional: via text shape or later extension
    std::vector<arg_t>   args;
    std::vector<panel_t> panels;
    std::string raw_text;                            // full instruction chunk for G+7
  };

  std::vector<screen_t> screens;

  // ephemeral parse state (used only during decode)
  struct _ps_t {
    screen_t*  scr{nullptr};
    arg_t*     arg{nullptr};
    panel_t*   pan{nullptr};
    shape_t*   shp{nullptr};
  } _ps;

  std::string str() const {
    std::ostringstream oss;
    auto quote_if_needed = [](const std::string& v) -> std::string {
      auto simple = [](char c){ return std::isalnum((unsigned char)c) || c=='_'||c=='-'||c=='.'||c==':'||c==','||c=='#'||c=='/'; };
      bool needs = v.empty() || std::any_of(v.begin(), v.end(), [&](char c){ return !simple(c) && c!=' '; }) || v.find(' ')!=std::string::npos;
      if (!needs) return v;
      std::string out = "\"";
      for (char c: v) out += (c=='"') ? "\\\"" : std::string(1,c);
      out += "\"";
      return out;
    };
    for (const auto& sc : screens) {
      oss << "screen F " << sc.fcode << "\n";
      for (const auto& a : sc.args) {
        oss << "  arg " << a.name << " path Arg1";
        if (!a.path_from_Arg1.empty()) oss << "." << a.path_from_Arg1;
        if (!a.presenter.name.empty()) {
          oss << " presented_by " << a.presenter.name;
          std::vector<std::pair<std::string,std::string>> pv(a.presenter.kv.begin(), a.presenter.kv.end());
          std::sort(pv.begin(), pv.end());
          for (auto& kv : pv) oss << " " << kv.first << " " << quote_if_needed(kv.second);
        }
        if (!a.triggers.empty()) {
          oss << " triggers";
          std::vector<std::pair<std::string,std::string>> tv(a.triggers.begin(), a.triggers.end());
          std::sort(tv.begin(), tv.end());
          for (auto& kv : tv) oss << " " << kv.first << " " << quote_if_needed(kv.second);
          oss << " endtriggers";
        }
        oss << "\n";
      }
      for (const auto& p : sc.panels) {
        oss << "  panel " << p.id << " " << p.type
            << " at " << p.x << " " << p.y << " " << p.w << " " << p.h;
        if (p.z) oss << " z " << p.z;
        if (p.scale != 1.f) oss << " scale " << p.scale;
        if (!p.bind_arg.empty()) oss << " bind " << p.bind_arg;
        oss << "\n";
        for (const auto& s : p.shapes) {
          std::string op;
          if (auto it = s.kv.find("op"); it != s.kv.end()) op = it->second;
          else {
            switch (s.kind) {
              case shape_t::kind_e::Curve:       op = "curve"; break;
              case shape_t::kind_e::MaskScatter: op = "mask_scatter"; break;
              case shape_t::kind_e::Embedding:   op = "embedding"; break;
              case shape_t::kind_e::MdnBand:     op = "mdn_band"; break;
              case shape_t::kind_e::Text:        op = "text"; break;
            }
          }
          oss << "    draw " << op;
          std::vector<std::pair<std::string,std::string>> kvs;
          kvs.reserve(s.kv.size());
          for (const auto& kv : s.kv) if (kv.first != "op") kvs.push_back(kv);
          std::sort(kvs.begin(), kvs.end());
          for (const auto& kv : kvs) oss << " " << kv.first << " " << quote_if_needed(kv.second);
          oss << "\n";
        }
        oss << "  endpanel\n";
      }
      oss << "endscreen\n";
    }
    return oss.str();
  }
};

/* ────────────────────────────────────────────────────────────────────────────
   Decoder / Visitor
   ──────────────────────────────────────────────────────────────────────────── */

namespace BNF {

class iinujiRenderings final : public ASTVisitor {
public:
  // Grammar is read from configuration (like the other implementations)
  //   config key: iinuji_renderings_bnf()
  std::string IINUJI_RENDERINGS_BNF_GRAMMAR =
      cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_bnf();

  iinujiRenderings();

  cuwacunu::camahjucunu::iinuji_renderings_instruction_t decode(std::string instruction);

  ProductionGrammar parseBnfGrammar();

  /* ASTVisitor */
  void visit(const RootNode* node,         VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node,     VisitorContext& context) override;

private:
  std::mutex       current_mutex;
  GrammarLexer     bnfLexer;
  GrammarParser    bnfParser;
  ProductionGrammar grammar;
  InstructionLexer  iLexer;
  InstructionParser iParser;

  static inline bool under(const VisitorContext& ctx, const char* rule) {
    for (auto* n : ctx.stack) if (n && n->name == rule) return true;
    return false;
  }
};

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
