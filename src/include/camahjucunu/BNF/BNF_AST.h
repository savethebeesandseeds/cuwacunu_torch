// BNF_AST.h
#pragma once
#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_visitor.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/* Base AST Node */
struct ASTNode {
  virtual ~ASTNode() = default;
  virtual void accept(ASTVisitor& visitor) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class RootNode : public ASTNode {
public:
  std::string lhs_instruction;
  std::vector<ASTNodePtr> children;
  RootNode(const std::string& lhs_instruction, std::vector<ASTNodePtr> children)
    : lhs_instruction(lhs_instruction), children(std::move(children)) {}
  void accept(ASTVisitor& visitor) const override;
};

class IntermediaryNode : public ASTNode {
public:
  ProductionAlternative alt;
  std::vector<ASTNodePtr> children;
  IntermediaryNode(const ProductionAlternative& alt, std::vector<ASTNodePtr> children)
    : alt(alt), children(std::move(children)) {}
  IntermediaryNode(const ProductionAlternative& alt) /* empty children constructor */
    : alt(alt), children() {}
  void accept(ASTVisitor& visitor) const override;
};

struct TerminalNode : public ASTNode {
  ProductionUnit unit;

  TerminalNode(const ProductionUnit& unit_)
    : unit(unit_) {
      if(unit.type != ProductionUnit::Type::Terminal) {
        throw std::runtime_error(
          "AST TerminalNode should be instantiated only by Terminal ProductionUnits, found: " + unit.str()
        );
      }
    }

  void accept(ASTVisitor& visitor) const override;
};

// Function to print AST using Visitor
void printAST(const ASTNode* node, int indent = 0, std::ostream& os = std::cout, const std::string& prefix = "", bool isLast = true);

// Function to compare ASTs
bool compareAST(const ASTNode* actual, const ASTNode* expected);

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
