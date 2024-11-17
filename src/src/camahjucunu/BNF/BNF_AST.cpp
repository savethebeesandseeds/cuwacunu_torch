// BNF_AST.cpp
#include "camahjucunu/BNF/BNF_AST.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

void printAST(const ASTNode* node, bool verbose, int indent, std::ostream& os, const std::string& prefix, bool isLast) {
  if (!node) return;

  os << prefix;

  // Choose the connector based on whether this is the last child of its parent
  os << (isLast ? "└── " : "├── ");

  if (auto root = dynamic_cast<const RootNode*>(node)) {
    os << root->str(verbose) << ANSI_COLOR_RESET << " : " << ANSI_COLOR_Dim_Green << root->name << ANSI_COLOR_RESET << " : " << root->lhs_instruction << "\n";
    size_t childCount = root->children.size();
    for (size_t i = 0; i < childCount; ++i) {
      // Pass whether the current child is the last one
      printAST(root->children[i].get(), verbose, indent + 1, os, prefix + (isLast ? "    " : "│   "), i == childCount - 1);
    }
  }
  else if (auto intermediary = dynamic_cast<const IntermediaryNode*>(node)) {
    os << intermediary->str(verbose) << ANSI_COLOR_RESET << " : " <<  ANSI_COLOR_Dim_Green << intermediary->name << ANSI_COLOR_RESET << " : " << intermediary->alt.str(verbose) << "\n";
    size_t childCount = intermediary->children.size();
    for (size_t i = 0; i < childCount; ++i) {
      printAST(intermediary->children[i].get(), verbose, indent + 1, os, prefix + (isLast ? "    " : "│   "), i == childCount - 1);
    }
  }
  else if (auto terminal = dynamic_cast<const TerminalNode*>(node)) {
    os << terminal->str(verbose) << ANSI_COLOR_RESET << " : " <<  terminal->unit.str(verbose) << "\n";
  }
  else {
    os << "Unknown AST Node\n";
  }
}

bool compareAST(const ASTNode* actual, const ASTNode* expected) {
  if (actual == nullptr && expected == nullptr) return true;
  if ((actual == nullptr) != (expected == nullptr)) return false;

  // Compare RootNode
  if (auto actualRoot = dynamic_cast<const RootNode*>(actual)) {
    auto expectedRoot = dynamic_cast<const RootNode*>(expected);
    if (!expectedRoot) return false;
    if (actualRoot->name != expectedRoot->name) return false;
    if (actualRoot->lhs_instruction != expectedRoot->lhs_instruction) return false;
    if (actualRoot->children.size() != expectedRoot->children.size()) return false;
    for (size_t i = 0; i < actualRoot->children.size(); ++i) {
      if (!compareAST(actualRoot->children[i].get(), expectedRoot->children[i].get())) return false;
    }
    return true;
  }

  // Compare IntermediaryNode
  if (auto actualIntermediary = dynamic_cast<const IntermediaryNode*>(actual)) {
    auto expectedIntermediary = dynamic_cast<const IntermediaryNode*>(expected);
    if (!expectedIntermediary) return false;
    if (actualIntermediary->name != expectedIntermediary->name) return false;
    if (actualIntermediary->alt.str() != expectedIntermediary->alt.str()) return false;
    if (actualIntermediary->children.size() != expectedIntermediary->children.size()) return false;
    for (size_t i = 0; i < actualIntermediary->children.size(); ++i) {
      if (!compareAST(actualIntermediary->children[i].get(), expectedIntermediary->children[i].get())) return false;
    }
    return true;
  }

  // Compare TerminalNode
  if (auto actualTerminal = dynamic_cast<const TerminalNode*>(actual)) {
    auto expectedTerminal = dynamic_cast<const TerminalNode*>(expected);
    if (!expectedTerminal) return false;
    if (actualTerminal->name != expectedTerminal->name) return false;
    if (actualTerminal->unit.lexeme != expectedTerminal->unit.lexeme) return false;
    return true;
  }

  // Unknown node types are considered unequal
  return false;
}

void push_context(VisitorContext& context, const ASTNode* node) {
  if (context.stack.empty() || context.stack.back()->hash() != node->hash()) {
    context.stack.push_back(node);
  }
}

void pop_context(VisitorContext& context, const ASTNode* node) {
  if (!context.stack.empty() && context.stack.back()->hash() == node->hash()) {
    context.stack.pop_back();
  }
}

std::string TerminalNode::str(bool versbose) const {
  if(versbose) {
    return ANSI_COLOR_Dim_Yellow + std::string("TerminalNode.") + ANSI_COLOR_Dim_Black_Grey + this->name + std::string(ANSI_COLOR_RESET) + "." + this->unit.str(versbose);
  }
  return ANSI_COLOR_Dim_Black_Grey + this->name + ANSI_COLOR_RESET;
};
std::string IntermediaryNode::str(bool versbose) const {
  if(versbose) {
    return ANSI_COLOR_Dim_Yellow + std::string("IntermediaryNode.") + ANSI_COLOR_Dim_Black_Grey + this->name + std::string(ANSI_COLOR_RESET) + std::string(ANSI_COLOR_Bright_Green) + ".(" + ANSI_COLOR_RESET + this->alt.str(versbose) + std::string(ANSI_COLOR_Bright_Green) + ")" + ANSI_COLOR_RESET;
  }
  return ANSI_COLOR_Dim_Black_Grey + this->name + ANSI_COLOR_RESET;
};
std::string RootNode::str(bool versbose) const {
  if(versbose) {
    return ANSI_COLOR_Dim_Yellow + std::string("RootNode.") + ANSI_COLOR_Dim_Black_Grey + this->name + std::string(ANSI_COLOR_RESET);
  }
  return ANSI_COLOR_Dim_Black_Grey + this->name + ANSI_COLOR_RESET;
};

std::string TerminalNode::hash() const { /* these are unique but are no hashes, hope i'd be forgiven */
  return this->name;
};
std::string IntermediaryNode::hash() const { /* these are unique but are no hashes, hope i'd be forgiven */
  return this->name;
};
std::string RootNode::hash() const { /* these are unique but are no hashes, hope i'd be forgiven */
  return this->name;
};


void TerminalNode::accept(ASTVisitor& visitor, VisitorContext& context) const {
  /* push context */
  push_context(context, this);
  /* visit */
  visitor.visit(this, context);
  /* push back context */
  pop_context(context, this);
}

void IntermediaryNode::accept(ASTVisitor& visitor, VisitorContext& context) const {
  /* push context */
  push_context(context, this);
  /* visit */
  visitor.visit(this, context);
  /* accept nodes */
  for (const auto& child : children) {
    /* push context */
    push_context(context, this);
    /* accept children */
    child->accept(visitor, context);
    /* push back context */
    pop_context(context, this);
  }
  /* push back context */
  pop_context(context, this);
}

void RootNode::accept(ASTVisitor& visitor, VisitorContext& context) const {
  /* push context */
  push_context(context, this);
  /* visit */
  visitor.visit(this, context);
  /* accept nodes */
  for (const auto& child : children) {
    /* push context */
    push_context(context, this);
    /* accept children */
    child->accept(visitor, context);
    /* push back context */
    pop_context(context, this);
  }
  /* push back context */
  pop_context(context, this);
}


} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu