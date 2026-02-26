// bnf_ast.h
#pragma once
#include "piaabo/bnf_compat/parser_types.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

// Forward declarations to avoid circular dependency with bnf_visitor.h
struct VisitorContext;
class ASTVisitor;

/* Base AST Node */
struct ASTNode {
  std::string name;
  size_t hash;
  virtual ~ASTNode() = default;
  virtual void accept(ASTVisitor& visitor, VisitorContext& context) const = 0;
  virtual std::string str(bool versbose = false) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class RootNode : public ASTNode {
public:
  std::string lhs_instruction;
  std::vector<ASTNodePtr> children;
  RootNode(const std::string& lhs_instruction, std::vector<ASTNodePtr> children)
    : lhs_instruction(lhs_instruction), children(std::move(children)) {
      name = lhs_instruction;
      hash = cuwacunu::piaabo::fnv1aHash(name);
    }
  void accept(ASTVisitor& visitor, VisitorContext& context) const override;
  std::string str(bool versbose = false) const override;
};

class IntermediaryNode : public ASTNode {
public:
  ProductionAlternative alt;
  std::vector<ASTNodePtr> children;
  IntermediaryNode(const ProductionAlternative& alt, std::vector<ASTNodePtr> children)
    : alt(alt), children(std::move(children)) {
      name = alt.lhs;
      hash = cuwacunu::piaabo::fnv1aHash(name);
  }
  IntermediaryNode(const ProductionAlternative& alt) /* empty children constructor */
    : alt(alt), children() {}
  void accept(ASTVisitor& visitor, VisitorContext& context) const override;
  std::string str(bool versbose = false) const override;
};

struct TerminalNode : public ASTNode {
  ProductionUnit unit;

  /* usual constructor */
  TerminalNode(const std::string& lhs, const ProductionUnit& unit_)
    : unit(unit_) {
    if(unit.type != ProductionUnit::Type::Terminal) {
      throw std::runtime_error(
        "AST TerminalNode should be instantiated only by Terminal ProductionUnits, found: " + unit.str(true)
      );
    }
    name = lhs;
    hash = cuwacunu::piaabo::fnv1aHash(name);
  }
  /* null terminal constructor */
  TerminalNode(const std::string& lhs) : unit(ProductionUnit(ProductionUnit::Type::Undetermined, "", 1, 1)) {
    name = lhs;
    hash = cuwacunu::piaabo::fnv1aHash(name);
  }
  
  void accept(ASTVisitor& visitor, VisitorContext& context) const override;
  std::string str(bool versbose = false) const override;
};

// Function to print AST using Visitor
void printAST(const ASTNode* node, bool verbose = false, int indent = 0,
              std::ostream& os = std::cout,
              const std::string& prefix = "", bool isLast = true);

// Functions to modify context
void push_context(VisitorContext& context, const ASTNode* node);
void pop_context(VisitorContext& context, const ASTNode* node);

// Function to compare ASTs
bool compareAST(const ASTNode* actual, const ASTNode* expected);

} // namespace bnf
} // namespace piaabo
} // namespace cuwacunu
