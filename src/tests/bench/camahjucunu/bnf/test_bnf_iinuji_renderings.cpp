/* test_bnf_iinuji_renderings.cpp */

#include <iostream>
#include <string>

#include "piaabo/dconfig.h"
#include "piaabo/dutils.h"

#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"


int main() {
  using namespace cuwacunu;
  using namespace camahjucunu;
  using namespace camahjucunu::BNF;

  try {
    // ------------------------------------------------------------------
    // 1) Load grammar and instruction from config
    // ------------------------------------------------------------------
    const char* config_folder = "/cuwacunu/src/config/";
    piaabo::dconfig::config_space_t::change_config_file(config_folder);
    piaabo::dconfig::config_space_t::update_config();

    std::string LANGUAGE =
      piaabo::dconfig::config_space_t::iinuji_renderings_bnf();
    std::string INPUT =
      piaabo::dconfig::config_space_t::iinuji_renderings_instruction();

    std::cout << "[test_bnf_iinuji_renderings] Loaded grammar length: "
              << LANGUAGE.size() << "\n";
    std::cout << "[test_bnf_iinuji_renderings] Loaded instruction length: "
              << INPUT.size() << "\n\n";

    // ------------------------------------------------------------------
    // 2) Build ProductionGrammar from BNF grammar text
    // ------------------------------------------------------------------
    GrammarLexer glex(LANGUAGE);
    GrammarParser gparser(glex);
    gparser.parseGrammar();

    // Make a copy so we can pass a non‑const ProductionGrammar to InstructionParser
    ProductionGrammar grammar = gparser.getGrammar();

    // std::cout << "========== Grammar (ProductionGrammar::str) ==========\n";
    // std::cout << grammar.str(0) << "\n\n";

    // ------------------------------------------------------------------
    // 3) Parse the instruction into an AST
    // ------------------------------------------------------------------
    InstructionLexer ilex(INPUT);
    InstructionParser iparser(ilex, grammar);

    // IMPORTANT: keep the ASTNodePtr alive for as long as we use the AST!
    ASTNodePtr root = iparser.parse_Instruction(INPUT);

    if (!root) {
      std::cerr << "ERROR: InstructionParser returned a null AST root.\n";
      return 1;
    }

    // std::cout << "=================== Parsed AST ===================\n";
    // printAST(root.get(), /*verbose=*/true);
    // std::cout << "\n";

    // ------------------------------------------------------------------
    // 4) Decode AST into iinuji_renderings_instruction_t
    // ------------------------------------------------------------------
    iinuji_renderings_decoder_t decoder;
    iinuji_renderings_instruction_t inst = decoder.decode(root.get());

    // ------------------------------------------------------------------
    // 5) Detailed dump of decoded structure
    // ------------------------------------------------------------------
    std::cout << "========== Parsed iinuji_renderings_instruction ==========\n";
    std::cout << "Number of screens: " << inst.screens.size() << "\n\n";

    // for (std::size_t si = 0; si < inst.screens.size(); ++si) {
    //   const auto& sc = inst.screens[si];
    //   std::cout << "SCREEN #" << si << "\n";
    //   std::cout << "  kind_raw  : "
    //             << (sc.kind_raw.empty() ? "_screen" : sc.kind_raw) << "\n";
    //   std::cout << "  name      : " << sc.name << "\n";
    //   std::cout << "  key_raw   : " << sc.key_raw << "\n";
    //   std::cout << "  fcode     : " << sc.fcode << "\n";
    //   std::cout << "  panels    : " << sc.panels.size() << "\n";
    //   std::cout << "  events    : " << sc.events.size() << "\n";
    //   std::cout << "  routes    : " << sc.routes.size() << "\n";

    //   if (!sc.line_color.empty())
    //     std::cout << "  line_color: " << sc.line_color << "\n";
    //   if (!sc.text_color.empty())
    //     std::cout << "  text_color: " << sc.text_color << "\n";
    //   if (!sc.back_color.empty())
    //     std::cout << "  back_color: " << sc.back_color << "\n";
    //   if (sc.tickness != 1.0)
    //     std::cout << "  tickness  : " << sc.tickness << "\n";
    //   if (sc.border)
    //     std::cout << "  border    : true\n";

    //   std::cout << "\n";

    //   // ----------------- PANELS -----------------
    //   for (std::size_t pi = 0; pi < sc.panels.size(); ++pi) {
    //     const auto& p = sc.panels[pi];
    //     std::cout << "  PANEL #" << pi << "\n";
    //     std::cout << "    kind_raw : "
    //               << (p.kind_raw.empty() ? "_rectangle" : p.kind_raw) << "\n";

    //     if (p.coords.set) {
    //       std::cout << "    coords   : (" << p.coords.x << "," << p.coords.y << ")\n";
    //     } else {
    //       std::cout << "    coords   : (unset)\n";
    //     }

    //     if (p.shape.set) {
    //       std::cout << "    shape    : (" << p.shape.x << "," << p.shape.y << ")\n";
    //     } else {
    //       std::cout << "    shape    : (unset)\n";
    //     }

    //     std::cout << "    z_index  : " << p.z_index << "\n";

    //     if (p.title_on)
    //       std::cout << "    title    : on  \"" << p.title << "\"\n";
    //     else if (!p.title.empty())
    //       std::cout << "    title    : off \"" << p.title << "\"\n";

    //     std::cout << "    border   : " << (p.border ? "true" : "false") << "\n";

    //     if (!p.line_color.empty())
    //       std::cout << "    line_color: " << p.line_color << "\n";
    //     if (!p.text_color.empty())
    //       std::cout << "    text_color: " << p.text_color << "\n";
    //     if (!p.back_color.empty())
    //       std::cout << "    back_color: " << p.back_color << "\n";
    //     if (p.tickness != 1.0)
    //       std::cout << "    tickness  : " << p.tickness << "\n";

    //     std::cout << "    figures  : " << p.figures.size() << "\n\n";

    //     // -------------- FIGURES ----------------
    //     for (std::size_t fi = 0; fi < p.figures.size(); ++fi) {
    //       const auto& f = p.figures[fi];
    //       std::cout << "    FIGURE #" << fi << "\n";
    //       std::cout << "      kind_raw : " << f.kind_raw << "\n";

    //       if (f.coords.set) {
    //         std::cout << "      coords   : (" << f.coords.x << "," << f.coords.y << ")\n";
    //       } else {
    //         std::cout << "      coords   : (unset)\n";
    //       }

    //       if (f.shape.set) {
    //         std::cout << "      shape    : (" << f.shape.x << "," << f.shape.y << ")\n";
    //       } else {
    //         std::cout << "      shape    : (unset)\n";
    //       }

    //       if (f.has_value)
    //         std::cout << "      value    : \"" << f.value << "\"\n";

    //       if (f.title_on)
    //         std::cout << "      title    : on  \"" << f.title << "\"\n";
    //       else if (!f.title.empty())
    //         std::cout << "      title    : off \"" << f.title << "\"\n";

    //       if (f.legend_on)
    //         std::cout << "      legend   : on  \"" << f.legend << "\"\n";
    //       else if (!f.legend.empty())
    //         std::cout << "      legend   : off \"" << f.legend << "\"\n";

    //       if (!f.type_raw.empty())
    //         std::cout << "      type_raw : " << f.type_raw << "\n";

    //       std::cout << "      border   : " << (f.border ? "true" : "false") << "\n";

    //       if (!f.line_color.empty())
    //         std::cout << "      line_color: " << f.line_color << "\n";
    //       if (!f.text_color.empty())
    //         std::cout << "      text_color: " << f.text_color << "\n";
    //       if (!f.back_color.empty())
    //         std::cout << "      back_color: " << f.back_color << "\n";
    //       if (f.tickness != 1.0)
    //         std::cout << "      tickness  : " << f.tickness << "\n";

    //       if (!f.triggers.empty()) {
    //         std::cout << "      triggers : ";
    //         for (std::size_t ti = 0; ti < f.triggers.size(); ++ti) {
    //           if (ti) std::cout << ", ";
    //           std::cout << f.triggers[ti];
    //         }
    //         std::cout << "\n";
    //       }

    //       std::cout << "\n";
    //     }
    //   }

    //   // --------------- EVENTS ------------------
    //   if (!sc.events.empty()) {
    //     std::cout << "  EVENTS\n";
    //     for (std::size_t ei = 0; ei < sc.events.size(); ++ei) {
    //       const auto& e = sc.events[ei];
    //       std::cout << "    EVENT #" << ei << "\n";
    //       std::cout << "      kind_raw : " << e.kind_raw << "\n";
    //       std::cout << "      name     : " << e.name << "\n";
    //       if (!e.bindings.empty()) {
    //         std::cout << "      bindings : ";
    //         for (std::size_t bi = 0; bi < e.bindings.size(); ++bi) {
    //           if (bi) std::cout << ", ";
    //           std::cout << e.bindings[bi].local_name
    //                     << ":" << e.bindings[bi].path_name;
    //         }
    //         std::cout << "\n";
    //       }
    //     }
    //     std::cout << "\n";
    //   }

    //   // --------------- ROUTES (placeholder) ------------------
    //   if (!sc.routes.empty()) {
    //     std::cout << "  ROUTES\n";
    //     for (std::size_t ri = 0; ri < sc.routes.size(); ++ri) {
    //       const auto& r = sc.routes[ri];
    //       std::cout << "    route #" << ri << " \"" << r << "\"\n";
    //     }
    //     std::cout << "\n";
    //   }

    //   std::cout << "----------------------------------------------------------\n\n";
    // }

    // // ------------------------------------------------------------------
    // // 6) Compact pretty‑printed summary using inst.str()
    // // ------------------------------------------------------------------
    // std::cout << "=========== Pretty-printed summary (inst.str()) ==========\n";
    // std::cout << inst.str() << "\n";

  } catch (const std::exception& ex) {
    std::cerr << "EXCEPTION: " << ex.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception\n";
    return 1;
  }

  return 0;
}
