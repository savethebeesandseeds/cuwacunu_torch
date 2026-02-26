// test_dsl_parser_grammar_lexer.cpp
#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>

// Include your GrammarLexer and related headers
#include "piaabo/bnf_compat/grammar_lexer.h"

// Namespace shortcuts for convenience
using namespace cuwacunu::piaabo::bnf;

// Helper function to compare two ProductionUnits
bool unitsAreEqual(const ProductionUnit& a, const ProductionUnit& b) {
    return (a.type == b.type) && (a.lexeme == b.lexeme);
}

// Function to execute a single test case
bool runTest(const std::string& testName, const std::string& input, const std::vector<ProductionUnit>& expectedUnits) {
    GrammarLexer lexer(input);
    std::vector<ProductionUnit> actualUnits;
    ProductionUnit unit;

    try {
        do {
            unit = lexer.getNextUnit();
            actualUnits.push_back(unit);
        } while (unit.type != ProductionUnit::Type::EndOfFile);
    } catch (const std::exception& e) {
        std::cerr << "\t[FAIL] " << "Test '" << testName << "' failed with exception: " << e.what() << std::endl;
        return false;
    }

    if (actualUnits.size() != expectedUnits.size()) {
        for(auto unit: actualUnits) {
            std::cout << "\t" << unit.str() << std::endl; 
        }
        std::cerr << "\t[FAIL] " << "Test '" << testName << "' failed: Expected " << expectedUnits.size()
                  << " units, got " << actualUnits.size() << " units." << std::endl;
        return false;
    }

    for (size_t i = 0; i < expectedUnits.size(); ++i) {
        if (!unitsAreEqual(actualUnits[i], expectedUnits[i])) {
            std::cerr << "\t[FAIL] " << "Test '" << testName << "' failed at unit " << i << ":\n"
                      << "  Expected: " << static_cast<int>(expectedUnits[i].type) << " '" << expectedUnits[i].lexeme << "'\n"
                      << "  Got:      " << static_cast<int>(actualUnits[i].type) << " '" << actualUnits[i].lexeme << "'\n";
            return false;
        }
    }

    std::cout << "\t[PASS] Test '" << testName << std::endl;
    return true;
}

bool testBasicUnitization() {
    std::string input =
        "<instruction>          ::= <symbol_spec> <parameter_list> <file_id_list> ;\n"
        "\n"
        "<symbol_spec>          ::= \"<\" <identifier> \">\" ;\n"
        "\n"
        "<parameter_list>       ::= \"(\" <parameters> \")\" ;\n"
        "\n"
        "<parameters>           ::= <parameter> \",\" <parameters> | <parameter> ;\n"
        "\n"
        "<parameter>            ::= <identifier> \"=\" <identifier> ;\n"
        "\n"
        "<file_id_list>         ::= \"[\" <file_ids> \"]\" ;\n"
        "\n"
        "<file_ids>             ::= <identifier> \",\" <file_ids> | <identifier> ;\n"
        "\n"
        "<identifier>           ::= <alphanumeric_string> ;\n"
        "\n"
        "<alphanumeric_string>  ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;\n"
        "\n"
        "<letter_or_digit>      ::= <letter> | <digit> ;\n"
        "\n"
        "<letter>               ::= \"A\" | \"B\" | \"C\" ;\n"
        "\n"
        "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n";

    // Define the expected units as per your sample output
    std::vector<ProductionUnit> expectedUnits = {
        // <instruction> ::= <symbol_spec> <parameter_list> <file_id_list> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<instruction>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<symbol_spec>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_id_list>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <symbol_spec> ::= "<" <identifier> ">" ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<symbol_spec>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"<\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\">\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <parameter_list> ::= "(" <parameters> ")" ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter_list>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"(\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameters>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\")\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <parameters> ::= <parameter> "," <parameters> | <parameter> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameters>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\",\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameters>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <parameter> ::= <identifier> "=" <identifier> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<parameter>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"=\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <file_id_list> ::= "[" <file_ids> "]" ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_id_list>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"[\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_ids>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"]\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <file_ids> ::= <identifier> "," <file_ids> | <identifier> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_ids>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\",\""),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<file_ids>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <identifier> ::= <alphanumeric_string> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <alphanumeric_string> ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <letter_or_digit> ::= <letter> | <digit> ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<digit>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <letter> ::= "A" | "B" | "C" ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"B\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"C\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // <digit> ::= "0" | "1" | "2" ;
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<digit>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"0\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"1\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"2\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        // End of file
        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Basic Unitization", input, expectedUnits);
}

bool testUnitTypes() {
    std::string input =
        "<start> ::= \"A\" | \"B\" | \"C\" ;\n";

    std::vector<ProductionUnit> expectedUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<start>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"B\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"C\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),
        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Unit Types Test", input, expectedUnits);
}

bool testIdentifiersAndAlphanumerics() {
    std::string input =
        "<identifier> ::= <alphanumeric_string> ;\n"
        "<alphanumeric_string> ::= <letter_or_digit> | [<letter_or_digit>] <alphanumeric_string> ;\n";

    std::vector<ProductionUnit> expectedUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<identifier>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<letter_or_digit>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Optional, "[<letter_or_digit>]"),
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<alphanumeric_string>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),

        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Identifiers and Alphanumerics Test", input, expectedUnits);
}

bool testPunctuationParsing() {
    // Use only valid symbols: ::= ; | 
    std::string input =
        "::= ::= ::= ; | \n";

    std::vector<ProductionUnit> expectedUnits = {
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Punctuation Parsing Test", input, expectedUnits);
}

bool testTerminalParsing() {
    std::string input =
        "\"identifier\" \"string with spaces\" \"12345\" \"ABC\" \"A_B.C\" \"12345\" \"123.45\"";

    std::vector<ProductionUnit> expectedUnits = {
        ProductionUnit(ProductionUnit::Type::Terminal, "\"identifier\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"string with spaces\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"12345\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"ABC\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A_B.C\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"12345\""),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"123.45\""),
        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Terminal Parsing Test", input, expectedUnits);
}

bool testWhitespaceHandling() {
    std::string input =
        "   \t\n<start>\t::=\n\"A\" \t | \"B\" \n;  ";

    std::vector<ProductionUnit> expectedUnits = {
        ProductionUnit(ProductionUnit::Type::NonTerminal, "<start>"),
        ProductionUnit(ProductionUnit::Type::Punctuation, "::="),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"A\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, "|"),
        ProductionUnit(ProductionUnit::Type::Terminal, "\"B\""),
        ProductionUnit(ProductionUnit::Type::Punctuation, ";"),
        ProductionUnit(ProductionUnit::Type::EndOfFile, "")
    };

    return runTest("Whitespace Handling Test", input, expectedUnits);
}

bool testErrorHandling() {
    std::vector<std::pair<std::string, std::string>> errorInputs = {
        {"Unterminated non-terminal", "<start ::= \"A\" ;"},
        {"Unterminated terminal", "<start> ::= \"A ;"},
        {"Invalid symbol after ':'", "<start> ::= :x ;"},
        {"Unsupported '...'", "<start> ::= ... ;"},
        {"Optionals should enclose Non-Terminals", "<start> ::= [\"terminal\"] ;"}
    };

    bool allPassed = true;

    for (const auto& [description, input] : errorInputs) {
        GrammarLexer lexer(input);
        try {
            while (true) {
                ProductionUnit unit = lexer.getNextUnit();
                if (unit.type == ProductionUnit::Type::EndOfFile) break;
            }
            // If no exception was thrown, the test fails
            std::cerr << "\t[FAIL] " << "Error Handling Test failed: " << description << " did not throw an exception." << std::endl;
            allPassed = false;
        } catch (const std::runtime_error& e) {
            std::cout << "\t[PASS] " << "Error Handling Test passed for: " << description << " with exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "\t[FAIL] " << "Error Handling Test failed for: " << description << " with unknown exception." << std::endl;
            allPassed = false;
        }
    }

    return allPassed;
}

int main() {
    int passed = 0;
    int failed = 0;

    struct TestCase {
        std::string name;
        bool (*testFunc)();
    };

    std::vector<TestCase> tests = {
        {"Basic Unitization", testBasicUnitization},
        {"Unit Types Test", testUnitTypes},
        {"Identifiers and Alphanumerics Test", testIdentifiersAndAlphanumerics},
        {"Punctuation Parsing Test", testPunctuationParsing},
        {"Terminal Parsing Test", testTerminalParsing},
        {"Whitespace Handling Test", testWhitespaceHandling},
        {"Error Handling Test", testErrorHandling}
    };

    for (const auto& test : tests) {
        std::cout << "Running Test: " << test.name << "..." << std::endl;
        bool result = test.testFunc();
        if (result) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\nTest Summary: " << passed << " passed, " << failed << " failed." << std::endl;

    return (failed == 0) ? 0 : 1;
}
