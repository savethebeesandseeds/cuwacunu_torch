// test_bnf_ast.cpp
#include <iostream>
#include <vector>
#include <memory>
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

int main() {
    using namespace cuwacunu::camahjucunu::BNF;

    // Manually construct the AST for <ABC>(A=10)[1,15,1]

    // Construct symbolIdentifier with allowed letters
    auto symbolIdentifier = std::make_unique<IdentifierNode>("ABC");

    // Construct identifierNode
    std::vector<ASTNodePtr> identifierChildren;
    identifierChildren.emplace_back(std::move(symbolIdentifier));
    auto identifierNode = std::make_unique<NonTerminalNode>("<identifier>", std::move(identifierChildren));

    // Construct symbolSpecNode
    std::vector<ASTNodePtr> symbolSpecChildren;
    symbolSpecChildren.emplace_back(std::move(identifierNode));
    auto symbolSpecNode = std::make_unique<NonTerminalNode>("<symbol_spec>", std::move(symbolSpecChildren));

    // Construct parameterNode
    auto paramKey = std::make_unique<IdentifierNode>("A");
    auto paramValue = std::make_unique<IdentifierNode>("10");
    std::vector<ASTNodePtr> parameterChildren;
    parameterChildren.emplace_back(std::move(paramKey));
    parameterChildren.emplace_back(std::move(paramValue));
    auto parameterNode = std::make_unique<NonTerminalNode>("<parameter>", std::move(parameterChildren));

    // Construct parametersNode
    std::vector<ASTNodePtr> parametersChildren;
    parametersChildren.emplace_back(std::move(parameterNode));
    auto parametersNode = std::make_unique<NonTerminalNode>("<parameters>", std::move(parametersChildren));

    // Construct parameterListNode
    std::vector<ASTNodePtr> parameterListChildren;
    parameterListChildren.emplace_back(std::move(parametersNode));
    auto parameterListNode = std::make_unique<NonTerminalNode>("<parameter_list>", std::move(parameterListChildren));

    // Construct fileIdsNodes
    auto fileID1 = std::make_unique<IdentifierNode>("1");
    auto fileID2 = std::make_unique<IdentifierNode>("15");
    auto fileID3 = std::make_unique<IdentifierNode>("1");

    std::vector<ASTNodePtr> fileIdsChildren1;
    fileIdsChildren1.emplace_back(std::move(fileID1));
    auto fileIdsNode1 = std::make_unique<NonTerminalNode>("<identifier>", std::move(fileIdsChildren1));

    std::vector<ASTNodePtr> fileIdsChildren2;
    fileIdsChildren2.emplace_back(std::move(fileID2));
    auto fileIdsNode2 = std::make_unique<NonTerminalNode>("<identifier>", std::move(fileIdsChildren2));

    std::vector<ASTNodePtr> fileIdsChildren3;
    fileIdsChildren3.emplace_back(std::move(fileID3));
    auto fileIdsNode3 = std::make_unique<NonTerminalNode>("<identifier>", std::move(fileIdsChildren3));

    // Construct fileIdsNode
    std::vector<ASTNodePtr> fileIdsNodeChildren;
    fileIdsNodeChildren.emplace_back(std::move(fileIdsNode1));
    fileIdsNodeChildren.emplace_back(std::move(fileIdsNode2));
    fileIdsNodeChildren.emplace_back(std::move(fileIdsNode3));
    auto fileIdsNode = std::make_unique<NonTerminalNode>("<file_ids>", std::move(fileIdsNodeChildren));

    // Construct fileIdListNode
    std::vector<ASTNodePtr> fileIdListChildren;
    fileIdListChildren.emplace_back(std::move(fileIdsNode));
    auto fileIdListNode = std::make_unique<NonTerminalNode>("<file_id_list>", std::move(fileIdListChildren));

    // Construct instructionNode
    std::vector<ASTNodePtr> instructionChildren;
    instructionChildren.emplace_back(std::move(symbolSpecNode));
    instructionChildren.emplace_back(std::move(parameterListNode));
    instructionChildren.emplace_back(std::move(fileIdListNode));
    auto instructionNode = std::make_unique<NonTerminalNode>("<instruction>", std::move(instructionChildren));

    // Print the AST
    std::cout << "Parsed AST:\n";
    printAST(instructionNode.get(), 2, std::cout);

    // Create observationPipeline visitor and traverse the AST
    observationPipeline pipelineVisitor;
    instructionNode->accept(pipelineVisitor);

    // Verify results
    if (pipelineVisitor.symbol == "ABC" &&
        pipelineVisitor.parameters.size() == 1 &&
        pipelineVisitor.parameters.at("A") == "10" &&
        pipelineVisitor.fileIDs.size() == 3 &&
        pipelineVisitor.fileIDs.at(0) == "1" &&
        pipelineVisitor.fileIDs.at(1) == "15" &&
        pipelineVisitor.fileIDs.at(2) == "1") {
        std::cout << "Manual AST Test Passed.\n";
    } else {
        std::cout << "Manual AST Test Failed.\n";
    }

    return 0;
}
