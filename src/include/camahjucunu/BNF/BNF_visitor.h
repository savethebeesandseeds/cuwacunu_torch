// BNF_visitor.h
#pragma once
#include <vector>
#include "BNF_AST.h"

/* 
 * ASTVisitor is an abstract base class that defines the Visitor interface 
 * for traversing the Abstract Syntax Tree (AST). It declares pure virtual 
 * `visit` methods for each concrete AST node type. Concrete visitor 
 * classes will inherit from ASTVisitor and implement these methods to 
 * perform specific operations on the AST nodes, such as execution, 
 * transformation, or analysis.
 */

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

class ProductionUnits;

struct ASTNode;
struct RootNode;
struct IntermediaryNode;
struct TerminalNode;

struct VisitorContext {
  void* user_data;
  std::vector<const ASTNode*> stack;
  VisitorContext(void* user_data) : user_data(user_data) {};
};

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;
  virtual void visit(const RootNode* node, VisitorContext& context) = 0;
  virtual void visit(const IntermediaryNode* node, VisitorContext& context) = 0;
  virtual void visit(const TerminalNode* node, VisitorContext& context) = 0;
};


} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
