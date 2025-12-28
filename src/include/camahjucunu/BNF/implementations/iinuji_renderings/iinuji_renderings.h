/* iinuji_renderings.h */
#pragma once

#include <string>
#include <vector>
#include <iosfwd>

#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"  // cuwacunu::camahjucunu::BNF::ASTVisitor, VisitorContext

// Compile-time debug switch:
//   define IINUJI_RENDERINGS_DEBUG to 1 (e.g. via compiler flags)
//   to enable logging to std::cerr.
#ifndef IINUJI_RENDERINGS_DEBUG
#define IINUJI_RENDERINGS_DEBUG 0
#endif

namespace cuwacunu {
namespace camahjucunu {

// ------------------------------------------------------------------
// Simple 2D point with "unset" flag
// ------------------------------------------------------------------

struct iinuji_point_t {
  bool set = false;
  int  x   = 0;
  int  y   = 0;
};

// ------------------------------------------------------------------
// Event form binding: local_name : .path_name
//   __form x:.data,y:.value
// ------------------------------------------------------------------

struct iinuji_event_binding_t {
  std::string local_name = "<empty>";  // e.g. "x"
  std::string path_name = "<empty>";   // e.g. ".data"
};

// ------------------------------------------------------------------
// FIGURE
// ------------------------------------------------------------------

struct iinuji_figure_t {
  std::string kind_raw = "<empty>";    // "_label", "_horizontal_plot", "_input_box"

  iinuji_point_t coords;   // __coords
  iinuji_point_t shape;    // __shape
  bool           border      = false;
  double         tickness    = 1.0;   // __tickness

  bool           has_value   = false;
  std::string    value = "<empty>";              // __value

  bool           title_on    = false;
  std::string    title = "<empty>";             // __title

  bool has_capacity          = false;
  int  capacity              = 0;               // __capacity (lines), required for _buffer

  bool           legend_on   = false;
  std::string    legend = "<empty>";            // __legend

  std::string    type_raw = "<empty>";          // __type

  std::string    line_color = "<empty>";        // __line_color
  std::string    text_color = "<empty>";        // __text_color
  std::string    back_color = "<empty>";        // __back_color

  std::vector<std::string> triggers;   // __triggers event names

  std::string str(unsigned indent = 0) const;
};

// ------------------------------------------------------------------
// PANEL
// ------------------------------------------------------------------

struct iinuji_panel_t {
  std::string    kind_raw = "<empty>";   // "_rectangle"

  iinuji_point_t coords;     // __coords
  iinuji_point_t shape;      // __shape
  int            z_index  = 0;   // __z_index

  bool           title_on = false;
  std::string    title = "<empty>";          // __title

  bool           border   = false;   // __border

  std::string    line_color = "<empty>";        // __line_color
  std::string    text_color = "<empty>";        // __text_color
  std::string    back_color = "<empty>";        // __back_color
  double         tickness = 1.0;    // __tickness

  std::vector<iinuji_figure_t> figures;

  std::string str(unsigned indent = 0) const;
};

// ------------------------------------------------------------------
// EVENT
// ------------------------------------------------------------------

struct iinuji_event_t {
  std::string kind_raw = "<empty>";  // "_update" or "_action"
  std::string name = "<empty>";      // __name
  std::vector<iinuji_event_binding_t> bindings;  // __form

  std::string str(unsigned indent = 0) const;
};

// ------------------------------------------------------------------
// SCREEN
// ------------------------------------------------------------------

struct iinuji_screen_t {
  std::string kind_raw = "<empty>";   // "_screen"
  std::string name = "<empty>";       // __name

  std::string key_raw = "<empty>";    // "F+1"
  int         fcode = 0;  // numeric part (1 for F+1)

  std::string line_color = "<empty>"; // __line_color
  std::string text_color = "<empty>"; // __text_color
  std::string back_color = "<empty>"; // __back_color
  double      tickness = 1.0;  // __tickness
  bool        border   = false;

  std::vector<iinuji_panel_t> panels;
  std::vector<iinuji_event_t> events;

  // routes are not in the grammar yet; keep a placeholder
  std::vector<std::string> routes;

  std::string str(unsigned indent = 0) const;
};

// ------------------------------------------------------------------
// Whole instruction
// ------------------------------------------------------------------

struct iinuji_renderings_instruction_t {
  std::vector<iinuji_screen_t> screens;

  std::string str() const;
};

// ------------------------------------------------------------------
// Decoder: walks the BNF AST and fills iinuji_renderings_instruction_t
// ------------------------------------------------------------------

namespace BNF { struct ASTNode; }

class iinuji_renderings_decoder_t : public BNF::ASTVisitor {
public:
  // 'debug' parameter kept for compatibility; logging is controlled
  // solely by IINUJI_RENDERINGS_DEBUG.
  explicit iinuji_renderings_decoder_t(bool debug = false);

  // Decode from an already-parsed AST root (<instruction> node).
  iinuji_renderings_instruction_t decode(const BNF::ASTNode* root);

  // ASTVisitor interface
  void visit(const BNF::RootNode* node,        BNF::VisitorContext& context) override;
  void visit(const BNF::IntermediaryNode* node,BNF::VisitorContext& context) override;
  void visit(const BNF::TerminalNode* node,    BNF::VisitorContext& context) override;

  struct State;
};

std::ostream& operator<<(std::ostream& os, const iinuji_renderings_instruction_t& inst);

} // namespace camahjucunu
} // namespace cuwacunu
