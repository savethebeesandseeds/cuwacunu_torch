// test_dsl_parser_instruction_parser.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include "piaabo/bnf_compat/parser_types.h"
#include "piaabo/bnf_compat/ast.h"
#include "piaabo/bnf_compat/grammar_lexer.h"
#include "piaabo/bnf_compat/grammar_parser.h"
#include "piaabo/bnf_compat/instruction_lexer.h"
#include "piaabo/bnf_compat/instruction_parser.h"

// Structure representing a single test case
struct TestCase {
    std::string name;                                               // Descriptive name for the test case
    std::string grammar_spec;                                         // DSL grammar definition for the test case
    std::string input;                                              // DSL input string to parse
    std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> expectedAST; // Expected AST tree (for valid inputs)
    bool expectSuccess;                                             // True if parsing should succeed
    std::string expectedError;                                      // Expected error message (for invalid inputs)
};

// Helper function to capture AST as a string (for debugging purposes)
std::string getASTString(const cuwacunu::piaabo::bnf::ASTNode* ast) {
    std::stringstream ss;
    cuwacunu::piaabo::bnf::printAST(ast, true, 0, ss);
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
    std::cout << "\t Grammar: \n" << testCase.grammar_spec << "\n";
    std::cout << "\t Input: " << testCase.input << "\n";

    try {
        // Initialize DSL lexer and parser with the test-specific DSL grammar
        cuwacunu::piaabo::bnf::GrammarLexer grammarLexer(testCase.grammar_spec);
        cuwacunu::piaabo::bnf::GrammarParser grammarParser(grammarLexer);

        // Parse the DSL grammar
        grammarParser.parseGrammar();
        auto grammar = grammarParser.getGrammar();

        // Initialize instruction lexer and parser with the instruction input and parsed grammar
        cuwacunu::piaabo::bnf::InstructionLexer iLexer;
        cuwacunu::piaabo::bnf::InstructionParser iParser(iLexer, grammar);

        // Parse the instruction input
        auto actualAST = iParser.parse_Instruction(testCase.input);

        if (!testCase.expectSuccess) {
            std::cout << "[FAIL]: Expected failure but parsing succeeded.\n";
            return false;
        }

        // Compare the actual AST with the expected AST using compareAST
        if (cuwacunu::piaabo::bnf::compareAST(actualAST.get(), testCase.expectedAST.get())) {
            std::cout << "[PASS].\n";
            std::cout << "  Result AST:\n" << getASTString(actualAST.get());
            return true;
        }
        else {
            std::cout << "[FAIL]: AST does not match expected output.\n";
            std::cout << "  Actual AST:\n" << getASTString(actualAST.get());
            std::cout << "  Expected AST:\n" << getASTString(testCase.expectedAST.get());
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

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_EmptyOption();
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_NonEmptyOption();
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_Repetitions();
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_Recursions();

int main() {
    // Define test cases
    std::vector<TestCase> testCases;
    
    // Valid Test Case 1
    testCases.emplace_back(
        TestCase{
            "Simple Option Parsing",  // name
            "<instruction>          ::= <parameter_list> ;\n"
            "<parameter_list>       ::= \"(\" <alphanumeric_string> \")\" ;\n"
            "<alphanumeric_string>  ::= [<letter_or_digit>] ;\n"
            "<letter_or_digit>      ::= <letter> | <digit> ;\n"
            "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",        // Grammar
            "()",                                   // input
            buildExpectedAST_EmptyOption(), // expectedAST
            true,                                                          // expectSuccess
            ""                                                             // expectedError
        }
    );

    // Valid Test Case 2
    testCases.emplace_back(
        TestCase{
            "Simple Non Empty Option",  // name
            "<instruction>          ::= <parameter_list> ;\n"
            "<parameter_list>       ::= \"(\" <alphanumeric_string> \")\" ;\n"
            "<alphanumeric_string>  ::= [<letter_or_digit>] ;\n"
            "<letter_or_digit>      ::= <letter> | <digit> ;\n"
            "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",        // Grammar
            "(A)",                                   // input
            buildExpectedAST_NonEmptyOption(), // expectedAST
            true,                                                          // expectSuccess
            ""                                                             // expectedError
        }
    );

    // Valid Test Case 3
    testCases.emplace_back(
        TestCase{
            "Simple Repetition Parsing",  // name
            "<instruction>          ::= <parameter_list> ;\n"
            "<parameter_list>       ::= \"(\" <alphanumeric_string> \")\" ;\n"
            "<alphanumeric_string>  ::= {<letter_or_digit>} ;\n"
            "<letter_or_digit>      ::= <letter> | <digit> ;\n"
            "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",        // Grammar
            "(A1B2)",                                   // input
            buildExpectedAST_Repetitions(), // expectedAST
            true,                                                          // expectSuccess
            ""                                                             // expectedError
        }
    );

    // Valid Test Case 4
    testCases.emplace_back(
        TestCase{
            "Simple Recurssion Parsing",  // name
            "<instruction>          ::= <parameter_list> ;\n"
            "<parameter_list>       ::= \"(\" <alphanumeric_string> \")\" ;\n"
            "<alphanumeric_string>  ::= <letter_or_digit> \",\" <alphanumeric_string> | <letter_or_digit>  ;\n"
            "<letter_or_digit>      ::= <letter> | <digit> ;\n"
            "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",        // Grammar
            "(0,A,1)",                                   // input
            buildExpectedAST_Recursions(), // expectedAST
            true,                                                          // expectSuccess
            ""                                                             // expectedError
        }
    );

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
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildAlphanumericStringNode(const std::string& str);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildIdentifierNode(const std::string& identifierStr);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildParameterNode(const std::string& lhs, const std::string& rhs);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildParametersNode(const std::vector<std::pair<std::string, std::string>>& params, size_t index = 0);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildFileIdsNode(const std::vector<std::string>& ids, size_t index = 0);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildAlphanumericStringNode(const std::string& input, size_t& index);
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildLetterOrDigitNode(char c);


/* main auxiliary functions */

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_EmptyOption() {
    using namespace cuwacunu::piaabo::bnf;

    // --- RootNode: <instruction> ---

    // Build Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>("<parameter_list>", 
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // --- <alphanumeric_string> ---
    // Since the optional content is empty, we represent it accordingly.

    // Build the repetition node (empty)
    std::vector<ASTNodePtr> repetitionChildren; // Empty vector since there's nothing to repeat
    auto repetitionNode = std::make_unique<TerminalNode>("<alphanumeric_string>");

    // Build <alphanumeric_string> node
    std::vector<ASTNodePtr> alphanumStringChildren;
    alphanumStringChildren.push_back(std::move(repetitionNode));
    auto alphanumStringNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<alphanumeric_string>", {ProductionUnit(ProductionUnit::Type::Optional, "[<letter_or_digit>]")}),
        std::move(alphanumStringChildren)
    );

    // --- </alphanumeric_string> ---

    // Build Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>("<parameter_list>",
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
        ProductionAlternative("<parameter_list>", parameterListUnits),
        std::move(parameterListChildren)
    );

    // Build the <instruction> node
    std::vector<ASTNodePtr> instructionChildren;
    instructionChildren.push_back(std::move(parameterListNode));

    ProductionUnit instructionUnit = ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>");

    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<instruction>", instructionUnit),
        std::move(instructionChildren)
    );

    // Build the root node
    std::vector<ASTNodePtr> rootChildren;
    rootChildren.push_back(std::move(instructionNode));

    auto rootNode = std::make_unique<RootNode>(
        "<instruction>",
        std::move(rootChildren)
    );

    return rootNode;
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_NonEmptyOption() {
    using namespace cuwacunu::piaabo::bnf;

    // Build Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>("<parameter_list>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // Build Terminal "A"
    auto terminalA = std::make_unique<TerminalNode>("<letter>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A\"")
    );

    // Build <letter_or_digit> node
    std::vector<ASTNodePtr> letterOrDigitChildren;
    letterOrDigitChildren.push_back(std::move(terminalA));

    auto letterOrDigitNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<letter_or_digit>", {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter>")
        }),
        std::move(letterOrDigitChildren)
    );

    // Build [<letter_or_digit>] node (Optional)
    std::vector<ASTNodePtr> alphanumStringChildren;
    alphanumStringChildren.push_back(std::move(letterOrDigitNode));

    auto alphanumStringNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<alphanumeric_string>", {
            ProductionUnit(ProductionUnit::Type::Optional, "[<letter_or_digit>]")
        }),
        std::move(alphanumStringChildren)
    );

    // Build Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>("<parameter_list>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    );

    // Build <parameter_list> node
    std::vector<ASTNodePtr> parameterListChildren;
    parameterListChildren.push_back(std::move(terminalOpenParen));
    parameterListChildren.push_back(std::move(alphanumStringNode));
    parameterListChildren.push_back(std::move(terminalCloseParen));

    auto parameterListNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<parameter_list>", {
            ProductionUnit(ProductionUnit::Type::Terminal, "\"(\""),
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
            ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
        }),
        std::move(parameterListChildren)
    );

    // Build the <instruction> node
    std::vector<ASTNodePtr> instructionChildren;
    instructionChildren.push_back(std::move(parameterListNode));

    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<instruction>", {
            ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>")
        }),
        std::move(instructionChildren)
    );

    // Build the root node
    std::vector<ASTNodePtr> rootChildren;
    rootChildren.push_back(std::move(instructionNode));

    auto rootNode = std::make_unique<RootNode>(
        "<instruction>",
        std::move(rootChildren)
    );

    return rootNode;
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_Repetitions() {
    using namespace cuwacunu::piaabo::bnf;

    // --- RootNode: <instruction> ---

    // --- <instruction> ---
    std::vector<ASTNodePtr> instructionChildren;

    // --- <parameter_list> ---
    // Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>("<parameter_list>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // --- <alphanumeric_string> ---
    std::string alphanumStr = "A1B2";

    // Build the repetition node for {<letter_or_digit>}
    std::vector<ASTNodePtr> repetitionChildren;

    for (char c : alphanumStr) {
        // Build node
        auto new_TerminalNode = std::make_unique<TerminalNode>((isalpha(c) ? "<letter>" : "<digit>"),
            ProductionUnit(ProductionUnit::Type::Terminal, "\"" + std::string(1, c) + "\"")
        );

        // Build <leter_or_digit> node
        std::vector<ASTNodePtr> LetterOrDigitChildren;
        LetterOrDigitChildren.push_back(std::move(new_TerminalNode));
        auto OrNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative("<letter_or_digit>", {ProductionUnit(ProductionUnit::Type::NonTerminal, (isalpha(c) ? "<letter>" : "<digit>"))}),
            std::move(LetterOrDigitChildren)
        );

        repetitionChildren.push_back(std::move(OrNode));
    }

    // Build <alphanumeric_string> node
    auto alphanumStringNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<alphanumeric_string>", {ProductionUnit(ProductionUnit::Type::Repetition, "{<letter_or_digit>}")}),
        std::move(repetitionChildren)
    );

    // --- </alphanumeric_string> ---

    // Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>("<parameter_list>",
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
        ProductionAlternative("<parameter_list>", parameterListUnits),
        std::move(parameterListChildren)
    );

    // Add <parameter_list> to <instruction> children
    instructionChildren.push_back(std::move(parameterListNode));

    // Build the <instruction> node
    ProductionUnit instructionUnit = ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>");
    
    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<instruction>", instructionUnit),
        std::move(instructionChildren)
    );

    // Build the root node
    std::vector<ASTNodePtr> rootChildren;
    rootChildren.push_back(std::move(instructionNode));
    auto rootNode = std::make_unique<RootNode>(
        "<instruction>",
        std::move(rootChildren)
    );

    return rootNode;
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildExpectedAST_Recursions() {
    using namespace cuwacunu::piaabo::bnf;

    // --- RootNode: <instruction> ---

    // Build Terminal "("
    auto terminalOpenParen = std::make_unique<TerminalNode>("<parameter_list>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\"")
    );

    // Input tokens without parentheses and commas
    std::string input = "0,A,1";
    size_t index = 0;

    // Build <alphanumeric_string> node recursively
    auto alphanumericStringNode = buildAlphanumericStringNode(input, index);

    // Build Terminal ")"
    auto terminalCloseParen = std::make_unique<TerminalNode>("<parameter_list>",
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    );

    // Build <parameter_list> node
    std::vector<ASTNodePtr> parameterListChildren;
    parameterListChildren.push_back(std::move(terminalOpenParen));
    parameterListChildren.push_back(std::move(alphanumericStringNode));
    parameterListChildren.push_back(std::move(terminalCloseParen));

    std::vector<ProductionUnit> parameterListUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\"")
    };

    auto parameterListNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<parameter_list>", parameterListUnits),
        std::move(parameterListChildren)
    );

    // Build the <instruction> node
    std::vector<ASTNodePtr> instructionChildren;
    instructionChildren.push_back(std::move(parameterListNode));

    ProductionUnit instructionUnit = ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>");

    auto instructionNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("<instruction>", instructionUnit),
        std::move(instructionChildren)
    );

    // Build the root node
    std::vector<ASTNodePtr> rootChildren;
    rootChildren.push_back(std::move(instructionNode));

    auto rootNode = std::make_unique<RootNode>(
        "<instruction>",
        std::move(rootChildren)
    );

    return rootNode;
}






// Definitions of helper functions (remove default arguments in definitions)
std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildAlphanumericStringNode(const std::string& str) {
    using namespace cuwacunu::piaabo::bnf;

    if (str.empty()) {
        return nullptr;
    }

    char firstChar = str[0];
    std::string rest = str.substr(1);

    // Build <letter_or_digit> node
    std::string lexeme(1, firstChar);
    auto letterOrDigitNode = std::make_unique<TerminalNode>("<letter_or_digit>",
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
            ProductionAlternative("", units),
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
            ProductionAlternative("", units),
            std::move(children)
        );

        return alphanumericStringNode;
    }
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildIdentifierNode(const std::string& identifierStr) {
    using namespace cuwacunu::piaabo::bnf;

    auto alphanumericStringNode = buildAlphanumericStringNode(identifierStr);

    std::vector<ASTNodePtr> identifierChildren;
    identifierChildren.push_back(std::move(alphanumericStringNode));

    std::vector<ProductionUnit> units = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>")
    };
    auto identifierNode = std::make_unique<IntermediaryNode>(
        ProductionAlternative("", units),
        std::move(identifierChildren)
    );

    return identifierNode;
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildParameterNode(const std::string& lhs, const std::string& rhs) {
    using namespace cuwacunu::piaabo::bnf;

    auto lhsIdentifierNode = buildIdentifierNode(lhs);
    auto equalsNode = std::make_unique<TerminalNode>("<identifier>",
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
        ProductionAlternative("", units),
        std::move(parameterChildren)
    );

    return parameterNode;
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildParametersNode(const std::vector<std::pair<std::string, std::string>>& params, size_t index) {
    using namespace cuwacunu::piaabo::bnf;

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
            ProductionAlternative("", units),
            std::move(parametersChildren)
        );

        return parametersNode;
    } else {
        // Recursive case: <parameters> ::= <parameter> "," <parameters>
        auto commaNode = std::make_unique<TerminalNode>("<parameter>",
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
            ProductionAlternative("", units),
            std::move(parametersChildren)
        );

        return parametersNode;
    }
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildFileIdsNode(const std::vector<std::string>& ids, size_t index) {
    using namespace cuwacunu::piaabo::bnf;

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
            ProductionAlternative("", units),
            std::move(fileIdsChildren)
        );

        return fileIdsNode;
    } else {
        // Recursive case: <file_ids> ::= <identifier> "," <file_ids>
        auto commaNode = std::make_unique<TerminalNode>("<identifier>",
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
            ProductionAlternative("", units),
            std::move(fileIdsChildren)
        );

        return fileIdsNode;
    }
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildAlphanumericStringNode(const std::string& input, size_t& index) {
    using namespace cuwacunu::piaabo::bnf;

    if (index >= input.length()) {
        // Handle error or end of input
        return nullptr;
    }

    // Skip any commas at the current index
    while (index < input.length() && input[index] == ',') {
        index++;
    }

    if (index >= input.length()) {
        // No more tokens
        return nullptr;
    }

    // Build <letter_or_digit> node
    char currentChar = input[index];
    index++;

    auto letterOrDigitNode = buildLetterOrDigitNode(currentChar);

    if (index < input.length()) {
        if (input[index] == ',') {
            // Option 1: <letter_or_digit> "," <alphanumeric_string>
            // Consume ","
            auto commaTerminalNode = std::make_unique<TerminalNode>("<alphanumeric_string>",
                ProductionUnit(ProductionUnit::Type::Terminal, "\",\"")
            );
            index++; // Move past the comma

            // Recursively build <alphanumeric_string>
            auto recursiveAlphanumNode = buildAlphanumericStringNode(input, index);

            // Build current <alphanumeric_string> node
            std::vector<ASTNodePtr> alphanumChildren;
            alphanumChildren.push_back(std::move(letterOrDigitNode));
            alphanumChildren.push_back(std::move(commaTerminalNode));
            alphanumChildren.push_back(std::move(recursiveAlphanumNode));

            auto alphanumNode = std::make_unique<IntermediaryNode>(
                ProductionAlternative("<alphanumeric_string>", {
                    ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
                    ProductionUnit(ProductionUnit::Type::Terminal, "\",\""),
                    ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>")
                }),
                std::move(alphanumChildren)
            );

            return alphanumNode;
        } else {
            // Option 3: <letter_or_digit>
            // Build current <alphanumeric_string> node
            std::vector<ASTNodePtr> alphanumChildren;
            alphanumChildren.push_back(std::move(letterOrDigitNode));

            auto alphanumNode = std::make_unique<IntermediaryNode>(
                ProductionAlternative("<alphanumeric_string>", {
                    ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>")
                }),
                std::move(alphanumChildren)
            );

            return alphanumNode;
        }
    } else {
        // End of input, Option 3: <letter_or_digit>
        std::vector<ASTNodePtr> alphanumChildren;
        alphanumChildren.push_back(std::move(letterOrDigitNode));

        auto alphanumNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative("<alphanumeric_string>", {
                ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>")
            }),
            std::move(alphanumChildren)
        );

        return alphanumNode;
    }
}

std::unique_ptr<cuwacunu::piaabo::bnf::ASTNode> buildLetterOrDigitNode(char c) {
    using namespace cuwacunu::piaabo::bnf;

    std::string token(1, c);

    if (c == 'A' || c == 'B' || c == 'C') {
        // Build <letter> node
        auto letterTerminalNode = std::make_unique<TerminalNode>("<letter>",
            ProductionUnit(ProductionUnit::Type::Terminal, "\"" + token + "\"")
        );

        // Build <letter_or_digit> node
        std::vector<ASTNodePtr> letterOrDigitChildren;
        letterOrDigitChildren.push_back(std::move(letterTerminalNode));

        auto letterOrDigitNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative("<letter_or_digit>", {ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter>")}),
            std::move(letterOrDigitChildren)
        );

        return letterOrDigitNode;
    } else if (c == '0' || c == '1' || c == '2') {
        // Build <digit> node
        auto digitTerminalNode = std::make_unique<TerminalNode>("<digit>",
            ProductionUnit(ProductionUnit::Type::Terminal, "\"" + token + "\"")
        );
        
        // Build <letter_or_digit> node
        std::vector<ASTNodePtr> letterOrDigitChildren;
        letterOrDigitChildren.push_back(std::move(digitTerminalNode));

        auto letterOrDigitNode = std::make_unique<IntermediaryNode>(
            ProductionAlternative("<letter_or_digit>", {ProductionUnit(ProductionUnit::Type::NonTerminal, "<digit>")}),
            std::move(letterOrDigitChildren)
        );

        return letterOrDigitNode;
    } else {
        // Handle error or other cases
        return nullptr;
    }
}
