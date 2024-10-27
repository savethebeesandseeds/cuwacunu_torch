// observation_pipeline.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <stack>
#include "piaabo/dutils.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/* 
 * observationPipeline is a concrete visitor that traverses the AST to 
 * extract execution data (symbol, parameters, file IDs) and executes 
 * corresponding functions based on the parsed instructions.
 */

class observationPipeline : public ASTVisitor {
public:
    using ParameterList = std::unordered_map<std::string, std::string>;
    using FileIDList = std::vector<std::string>;

    // Map symbols to executable functions
    std::unordered_map<std::string, std::function<void(const ParameterList&, const FileIDList&)>> functions;

    // Parsed data
    std::string symbol;
    ParameterList parameters;
    FileIDList fileIDs;

    // Constructor: Registers executable functions
    observationPipeline();

    // Override visit methods for each AST node type
    void visit(const NonTerminalNode* node) override;
    void visit(const IdentifierNode* node) override;

private:
    // Stack to keep track of current traversal context
    std::stack<std::string> contextStack;

    // Helper function to execute the mapped function based on the symbol
    void executeFunction();

    // Helper function to get the current context
    std::string getCurrentContext() const;

    // Handler functions for specific non-terminals
    void handleInstruction(const NonTerminalNode* node);
    void handleSymbolSpec(const NonTerminalNode* node);
    void handleParameterList(const NonTerminalNode* node);
    void handleParameters(const NonTerminalNode* node);
    void handleParameter(const NonTerminalNode* node);
    void handleFileIdList(const NonTerminalNode* node);
    void handleFileIds(const NonTerminalNode* node);
};

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
