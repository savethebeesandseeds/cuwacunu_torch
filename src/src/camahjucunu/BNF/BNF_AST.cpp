// BNF_AST.cpp
#include "camahjucunu/BNF/BNF_AST.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

void printAST(const ASTNode* node, int indent, std::ostream& os, const std::string& prefix, bool isLast) {
  if (!node) return;

  os << prefix;

  // Choose the connector based on whether this is the last child of its parent
  os << (isLast ? "└── " : "├── ");

  if (auto root = dynamic_cast<const RootNode*>(node)) {
    os << "RootNode: " << root->lhs_instruction << "\n";
    size_t childCount = root->children.size();
    for (size_t i = 0; i < childCount; ++i) {
      // Pass whether the current child is the last one
      printAST(root->children[i].get(), indent + 1, os, prefix + (isLast ? "    " : "│   "), i == childCount - 1);
    }
  }
  else if (auto intermediary = dynamic_cast<const IntermediaryNode*>(node)) {
    os << "IntermediaryNode: " << intermediary->alt.str() << "\n";
    size_t childCount = intermediary->children.size();
    for (size_t i = 0; i < childCount; ++i) {
      printAST(intermediary->children[i].get(), indent + 1, os, prefix + (isLast ? "    " : "│   "), i == childCount - 1);
    }
  }
  else if (auto optional = dynamic_cast<const OptionalNode*>(node)) {
    os << "OptionalNode: " << optional->unit.str() << "\n";
    if (optional->child) {
      printAST(optional->child.get(), indent + 1, os, prefix + (isLast ? "    " : "│   "), true);
    }
  }
  else if (auto terminal = dynamic_cast<const TerminalNode*>(node)) {
    os << "TerminalNode: " << terminal->unit.lexeme << "\n";
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
    if (actualIntermediary->alt.str() != expectedIntermediary->alt.str()) return false;
    if (actualIntermediary->children.size() != expectedIntermediary->children.size()) return false;
    for (size_t i = 0; i < actualIntermediary->children.size(); ++i) {
      if (!compareAST(actualIntermediary->children[i].get(), expectedIntermediary->children[i].get())) return false;
    }
    return true;
  }

  // Compare OptionalNode
  if (auto actualOptional = dynamic_cast<const OptionalNode*>(actual)) {
    auto expectedOptional = dynamic_cast<const OptionalNode*>(expected);
    if (!expectedOptional) return false;
    if (actualOptional->unit.str() != expectedOptional->unit.str()) return false;
    return compareAST(actualOptional->child.get(), expectedOptional->child.get());
  }

  // Compare TerminalNode
  if (auto actualTerminal = dynamic_cast<const TerminalNode*>(actual)) {
    auto expectedTerminal = dynamic_cast<const TerminalNode*>(expected);
    if (!expectedTerminal) return false;
    if (actualTerminal->unit.lexeme != expectedTerminal->unit.lexeme) return false;
    return true;
  }

  // Unknown node types are considered unequal
  return false;
}

void TerminalNode::accept(ASTVisitor& visitor) const {
  visitor.visit(this);
}

void IntermediaryNode::accept(ASTVisitor& visitor) const {
  visitor.visit(this);
  for (const auto& child : children) {
    child->accept(visitor);
  }
}

void OptionalNode::accept(ASTVisitor& visitor) const {
  visitor.visit(this);
  if (child) {
    child->accept(visitor);
  }
}

void RootNode::accept(ASTVisitor& visitor) const {
  visitor.visit(this);
  for (const auto& child : children) {
    child->accept(visitor);
  }
}


} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu