// test_bnf_instruction_parser.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

// Structure representing a single test case
struct TestCase {
    std::string name;                                               // Descriptive name for the test case
    std::string bnfGrammar;                                         // BNF grammar definition for the test case
    std::string input;                                              // DSL input string to parse
    std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> expectedAST; // Expected AST tree (for valid inputs)
    bool expectSuccess;                                             // True if parsing should succeed
    std::string expectedError;                                      // Expected error message (for invalid inputs)
};

// Helper function to capture AST as a string (for debugging purposes)
std::string getASTString(const cuwacunu::camahjucunu::BNF::ASTNode* ast) {
    std::stringstream ss;
    cuwacunu::camahjucunu::BNF::printAST(ast, 0, ss);
    return ss.str();
}
// Helper function to check if actual error contains expected error
bool contains(const std::string& actual, const std::string& expected) {
    return actual.find(expected) != std::string::npos;
}

// Helper function to run a single test case
bool runTestCase(const TestCase& testCase, int testNumber) {
    std::cout << "----------------------------------------\n";
    std::cout << "\t Test " << testNumber << ": " << testCase.name << "\n";
    std::cout << "\t Grammar: \n" << testCase.bnfGrammar << "\n";
    std::cout << "\t Input: " << testCase.input << "\n";

    try {
        // Initialize BNF lexer and parser with the test-specific BNF grammar
        cuwacunu::camahjucunu::BNF::GrammarLexer bnfLexer(testCase.bnfGrammar);
        cuwacunu::camahjucunu::BNF::GrammarParser bnfParser(bnfLexer);

        // Parse the BNF grammar
        bnfParser.parseGrammar();
        auto grammar = bnfParser.getGrammar();

        // Initialize instruction lexer and parser with the instruction input and parsed grammar
        cuwacunu::camahjucunu::BNF::InstructionLexer iLexer;
        cuwacunu::camahjucunu::BNF::InstructionParser iParser(iLexer, grammar);

        // Parse the instruction input
        auto actualAST = iParser.parse_Instruction(testCase.input);

        if (!testCase.expectSuccess) {
            std::cout << "[FAIL]: Expected failure but parsing succeeded.\n";
            return false;
        }

        // Compare the actual AST with the expected AST using compareAST
        if (cuwacunu::camahjucunu::BNF::compareAST(actualAST.get(), testCase.expectedAST.get())) {
            std::cout << "[PASS].\n";
            return true;
        }
        else {
            std::cout << "[FAIL]: AST does not match expected output.\n";
            std::cout << "  Expected AST:\n" << getASTString(testCase.expectedAST.get());
            std::cout << "  Actual AST:\n" << getASTString(actualAST.get());
            return false;
        }
    }
    catch (const std::exception& e) {
        if (testCase.expectSuccess) {
            std::cout << "[FAIL]: Expected success but caught an error.\n";
            std::cout << "  Error: " << e.what() << "\n";
            return false;
        }
        else {
            // Check if the caught error contains the expected error substring
            if (contains(e.what(), testCase.expectedError)) {
                std::cout << "[PASS] (Caught expected error).\n";
                return true;
            }
            else {
                std::cout << "[FAIL]: Caught unexpected error.\n";
                std::cout << "  Expected Error to contain: " << testCase.expectedError << "\n";
                std::cout << "  Actual Error: " << e.what() << "\n";
                return false;
            }
        }
    }
}

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildExpectedAST_MultipleParams_FileIDs();
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildExpectedAST_SingleParameter();

int main() {
    // Define test cases
    std::vector<TestCase> testCases;
    
    // Valid Test Case 1
    testCases.emplace_back(
        TestCase{
            "Simple parsing",  // name
            "<instruction>          ::= <parameter_list> ;\n"
            "<parameter_list>       ::= \"(\" <alphanumeric_string> \")\" ;\n"
            "<alphanumeric_string>  ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;\n"
            "<letter_or_digit>      ::= <letter> | <digit> ;\n"
            "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",        // Grammar
            "(A1B2)",                                   // input
            buildExpectedAST_SingleParameter(), // expectedAST
            true,                                                          // expectSuccess
            ""                                                             // expectedError
        }
    );

    // // Valid Test Case 2
    // testCases.emplace_back(
    //     TestCase{
    //         "Valid Input with Multiple Parameters and File IDs",  // name
    //         "<instruction>          ::= <symbol_spec> [<parameter_list>] [<file_id_list>] ;\n"
    //         "<symbol_spec>         ::= \"<\" <identifier> \">\" ;\n"
    //         "<parameter_list>      ::= \"(\" <parameters> \")\" ;\n"
    //         "<parameters>          ::= <parameter> | <parameter> \",\" <parameters> ;\n"
    //         "<parameter>           ::= <identifier> \"=\" <identifier> ;\n"
    //         "<file_id_list>        ::= \"[\" <file_ids> \"]\" ;\n"
    //         "<file_ids>            ::= <identifier> | <identifier> \",\" <file_ids> ;\n"
    //         "<identifier>          ::= <alphanumeric_string> ;\n"
    //         "<alphanumeric_string> ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;\n"
    //         "<letter_or_digit>     ::= <letter> | <digit> ;\n"
    //         "<letter>              ::= \"A\" | \"B\" | \"C\" ;\n"
    //         "<digit>               ::= \"0\" | \"1\" | \"2\" ;",       // bnfGrammar
    //         "<ABC>(A=10,B=20)[0,1,2]",                                   // input
    //         buildExpectedAST_MultipleParams_FileIDs(), // expectedAST
    //         true,                                                          // expectSuccess
    //         ""                                                             // expectedError
    //     }
    // );

    // Add more test cases as needed

    // Run test cases
    int passed = 0;
    int failed = 0;
    int testNumber = 1;

    for (const auto& testCase : testCases) {
        bool result = runTestCase(testCase, testNumber);
        if (result) {
            passed++;
        }
        else {
            failed++;
        }
        testNumber++;
    }

    // Summary
    std::cout << "----------------------------------------\n";
    std::cout << "Test Summary: " << passed << " Passed, " << failed << " Failed.\n";

    return (failed == 0) ? 0 : 1;
}

// Forward declarations of helper functions (declarations only)
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildAlphanumericStringNode(const std::string& str);
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildIdentifierNode(const std::string& identifierStr);
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildParameterNode(const std::string& lhs, const std::string& rhs);
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildParametersNode(const std::vector<std::pair<std::string, std::string>>& params, size_t index = 0);
std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildFileIdsNode(const std::vector<std::string>& ids, size_t index = 0);

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildExpectedAST_SingleParameter() {
    using namespace cuwacunu::camahjucunu::BNF;

    // --- <instruction> ---
    std::vector<ASTNodePtr> instructionChildren;

    // --- <parameter_list> ---
    // Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // --- <alphanumeric_string> ---
    std::vector<ASTNodePtr> alphanumStringChildren;

    // Example: Building "A1B2"
    // <letter_or_digit> ::= <letter> | <digit>

    // 'A'
    auto letterA = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A\"")
    );
    alphanumStringChildren.push_back(std::move(letterA));

    // '1'
    auto digit1 = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"1\"")
    );
    alphanumStringChildren.push_back(std::move(digit1));

    // 'B'
    auto letterB = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"B\"")
    );
    alphanumStringChildren.push_back(std::move(letterB));

    // '2'
    auto digit2 = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"2\"")
    );
    alphanumStringChildren.push_back(std::move(digit2));

    // Build <alphanumeric_string> node
    std::vector<ProductionUnit> alphanumStringUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>")
    };
    auto alphanumStringNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(alphanumStringUnits),
        std::move(alphanumStringChildren)
    );

    // --- </alphanumeric_string> ---

    // Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    );

    // Build <parameter_list> node
    std::vector<ASTNodePtr> parameterListChildren;
    parameterListChildren.push_back(std::move(terminalOpenParen));
    parameterListChildren.push_back(std::move(alphanumStringNode));
    parameterListChildren.push_back(std::move(terminalCloseParen));

    std::vector<ProductionUnit> parameterListUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    };
    auto parameterListNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(parameterListUnits),
        std::move(parameterListChildren)
    );

    // Add <parameter_list> to <instruction> children
    instructionChildren.push_back(std::move(parameterListNode));

    // Build the root <instruction> node
    std::vector<ProductionUnit> instructionUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>")
    };
    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(instructionUnits),
        std::move(instructionChildren)
    );

    return instructionNode;
}


std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildExpectedAST_MultipleParams_FileIDs() {
    using namespace cuwacunu::camahjucunu::BNF;

    // --- <instruction> ---
    std::vector<ASTNodePtr> instructionChildren;

    // --- <symbol_spec> ---
    std::vector<ASTNodePtr> symbolSpecChildren;

    // Terminal "<"
    auto terminalLessThan = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"<\"")
    );
    symbolSpecChildren.push_back(std::move(terminalLessThan));

    // <identifier> for "ACB"
    auto identifierNode = buildIdentifierNode("ACB");
    symbolSpecChildren.push_back(std::move(identifierNode));

    // Terminal ">"
    auto terminalGreaterThan = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\">\"")
    );
    symbolSpecChildren.push_back(std::move(terminalGreaterThan));

    // Build <symbol_spec> node
    std::vector<ProductionUnit> symbolSpecUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"<\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\">\"")
    };
    auto symbolSpecNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(symbolSpecUnits),
        std::move(symbolSpecChildren)
    );
    instructionChildren.push_back(std::move(symbolSpecNode));

    // --- [<parameter_list>] ---
    // Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // Build <parameters> node recursively
    std::vector<std::pair<std::string, std::string>> params = { {"A", "10"}, {"B", "20"} };
    auto parametersNode = buildParametersNode(params);

    // Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    );

    // Build <parameter_list> node
    std::vector<ASTNodePtr> parameterListChildren;
    parameterListChildren.push_back(std::move(terminalOpenParen));
    parameterListChildren.push_back(std::move(parametersNode));
    parameterListChildren.push_back(std::move(terminalCloseParen));

    std::vector<ProductionUnit> parameterListUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameters>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    };
    auto parameterListNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(parameterListUnits),
        std::move(parameterListChildren)
    );

    // Wrap in OptionalNode
    auto optionalParameterListNode = std::make_unique<OptionalNode>(
        ProductionUnit(ProductionUnit::Type::Optional, "[ <parameter_list> ]"),
        std::move(parameterListNode)
    );
    instructionChildren.push_back(std::move(optionalParameterListNode));

    // --- [<file_id_list>] ---
    // Terminal "["
    auto terminalOpenBracket = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"[\"")
    );

    // Build <file_ids> node recursively
    std::vector<std::string> fileIds = { "0", "1", "2" };
    auto fileIdsNode = buildFileIdsNode(fileIds);

    // Terminal "]"
    auto terminalCloseBracket = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"]\"")
    );

    // Build <file_id_list> node
    std::vector<ASTNodePtr> fileIdListChildren;
    fileIdListChildren.push_back(std::move(terminalOpenBracket));
    fileIdListChildren.push_back(std::move(fileIdsNode));
    fileIdListChildren.push_back(std::move(terminalCloseBracket));

    std::vector<ProductionUnit> fileIdListUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"[\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_ids>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"]\"")
    };
    auto fileIdListNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(fileIdListUnits),
        std::move(fileIdListChildren)
    );

    // Wrap in OptionalNode
    auto optionalFileIdListNode = std::make_unique<OptionalNode>(
        ProductionUnit(ProductionUnit::Type::Optional, "[ <file_id_list> ]"),
        std::move(fileIdListNode)
    );
    instructionChildren.push_back(std::move(optionalFileIdListNode));

    // Build the root <instruction> node
    std::vector<ProductionUnit> instructionUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<symbol_spec>"),
        // Since parameter_list and file_id_list are optional, we may or may not include them in the units
        ProductionUnit(ProductionUnit::Type::Optional, "[ <parameter_list> ]"),
        ProductionUnit(ProductionUnit::Type::Optional, "[ <file_id_list> ]")
    };
    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(instructionUnits),
        std::move(instructionChildren)
    );

    return instructionNode;
}

// Definitions of helper functions (remove default arguments in definitions)

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildAlphanumericStringNode(const std::string& str) {
    using namespace cuwacunu::camahjucunu::BNF;

    if (str.empty()) {
        return nullptr;
    }

    char firstChar = str[0];
    std::string rest = str.substr(1);

    // Build <letter_or_digit> node
    std::string lexeme(1, firstChar);
    auto letterOrDigitNode = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"" + lexeme + "\"")
    );

    if (rest.empty()) {
        // Base case: <alphanumeric_string> ::= <letter_or_digit>
        std::vector<ASTNodePtr> children;
        children.push_back(std::move(letterOrDigitNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>")
        };
        auto alphanumericStringNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(children)
        );

        return alphanumericStringNode;
    } else {
        // Recursive case: <alphanumeric_string> ::= <letter_or_digit> <alphanumeric_string>
        auto restNode = buildAlphanumericStringNode(rest);

        std::vector<ASTNodePtr> children;
        children.push_back(std::move(letterOrDigitNode));
        children.push_back(std::move(restNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>")
        };
        auto alphanumericStringNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(children)
        );

        return alphanumericStringNode;
    }
}

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildIdentifierNode(const std::string& identifierStr) {
    using namespace cuwacunu::camahjucunu::BNF;

    auto alphanumericStringNode = buildAlphanumericStringNode(identifierStr);

    std::vector<ASTNodePtr> identifierChildren;
    identifierChildren.push_back(std::move(alphanumericStringNode));

    std::vector<ProductionUnit> units = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>")
    };
    auto identifierNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(units),
        std::move(identifierChildren)
    );

    return identifierNode;
}

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildParameterNode(const std::string& lhs, const std::string& rhs) {
    using namespace cuwacunu::camahjucunu::BNF;

    auto lhsIdentifierNode = buildIdentifierNode(lhs);
    auto equalsNode = std::make_unique<TerminalNode>(
        ProductionUnit(ProductionUnit::Type::Terminal, "\"=\"")
    );
    auto rhsIdentifierNode = buildIdentifierNode(rhs);

    std::vector<ASTNodePtr> parameterChildren;
    parameterChildren.push_back(std::move(lhsIdentifierNode));
    parameterChildren.push_back(std::move(equalsNode));
    parameterChildren.push_back(std::move(rhsIdentifierNode));

    std::vector<ProductionUnit> units = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"=\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>")
    };
    auto parameterNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative(units),
        std::move(parameterChildren)
    );

    return parameterNode;
}

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildParametersNode(const std::vector<std::pair<std::string, std::string>>& params, size_t index) {
    using namespace cuwacunu::camahjucunu::BNF;

    if (index >= params.size()) {
        return nullptr;
    }

    auto parameterNode = buildParameterNode(params[index].first, params[index].second);

    if (index + 1 >= params.size()) {
        // Base case: only one parameter
        std::vector<ASTNodePtr> parametersChildren;
        parametersChildren.push_back(std::move(parameterNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter>")
        };
        auto parametersNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(parametersChildren)
        );

        return parametersNode;
    } else {
        // Recursive case: <parameters> ::= <parameter> "," <parameters>
        auto commaNode = std::make_unique<TerminalNode>(
            ProductionUnit(ProductionUnit::Type::Terminal, "\",\"")
        );
        auto restParametersNode = buildParametersNode(params, index + 1);

        std::vector<ASTNodePtr> parametersChildren;
        parametersChildren.push_back(std::move(parameterNode));
        parametersChildren.push_back(std::move(commaNode));
        parametersChildren.push_back(std::move(restParametersNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter>"),
            ProductionUnit(ProductionUnit::Type::Terminal, "\",\""),
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameters>")
        };
        auto parametersNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(parametersChildren)
        );

        return parametersNode;
    }
}

std::unique_ptr<cuwacunu::camahjucunu::BNF::ASTNode> buildFileIdsNode(const std::vector<std::string>& ids, size_t index) {
    using namespace cuwacunu::camahjucunu::BNF;

    if (index >= ids.size()) {
        return nullptr;
    }

    auto identifierNode = buildIdentifierNode(ids[index]);

    if (index + 1 >= ids.size()) {
        // Base case: only one identifier
        std::vector<ASTNodePtr> fileIdsChildren;
        fileIdsChildren.push_back(std::move(identifierNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>")
        };
        auto fileIdsNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(fileIdsChildren)
        );

        return fileIdsNode;
    } else {
        // Recursive case: <file_ids> ::= <identifier> "," <file_ids>
        auto commaNode = std::make_unique<TerminalNode>(
            ProductionUnit(ProductionUnit::Type::Terminal, "\",\"")
        );
        auto restFileIdsNode = buildFileIdsNode(ids, index + 1);

        std::vector<ASTNodePtr> fileIdsChildren;
        fileIdsChildren.push_back(std::move(identifierNode));
        fileIdsChildren.push_back(std::move(commaNode));
        fileIdsChildren.push_back(std::move(restFileIdsNode));

        std::vector<ProductionUnit> units = {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
            ProductionUnit(ProductionUnit::Type::Terminal, "\",\""),
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_ids>")
        };
        auto fileIdsNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative(units),
            std::move(fileIdsChildren)
        );

        return fileIdsNode;
    }
}
