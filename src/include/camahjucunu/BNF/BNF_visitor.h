// BNF_visitor.h
#pragma once
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

// Forward declarations of AST node types
struct RootNode;
struct IntermediaryNode;
struct TerminalNode;

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;
  virtual void visit(const RootNode* node) = 0;
  virtual void visit(const IntermediaryNode* node) = 0;
  virtual void visit(const TerminalNode* node) = 0;
  // Remove or add other node types as needed
};


} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
