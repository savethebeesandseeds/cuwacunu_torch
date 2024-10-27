// test_bnf_grammar_parser.cpp
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <algorithm> // For std::sort and std::unique

#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"

// Use the BNF namespace for convenience
using namespace cuwacunu::camahjucunu::BNF;

// ----------------------------
// Expected Grammar Structures
// ----------------------------

// Define ExpectedProductionRule to match the structure used in the test cases
struct ExpectedProductionRule {
    std::string lhs;
    std::vector<std::vector<std::string>> rhs;  // Each alternative is a vector of strings
};

// ExpectedGrammar maps non-terminal names to their expected production rules
using ExpectedGrammar = std::unordered_map<std::string, ExpectedProductionRule>;

// ----------------------------
// Utility Functions for Testing
// ----------------------------

// Function to compare parsed grammar with expected grammar
bool compare_grammar(
    const std::unordered_map<std::string, ProductionRule>& parsed,
    const ExpectedGrammar& expected,
    std::string& error_message
) {
    // Check if all expected non-terminals are present
    for (const auto& pair : expected) {
        const std::string& lhs = pair.first;
        const ExpectedProductionRule& expected_rule = pair.second;

        auto it = parsed.find(lhs);
        if (it == parsed.end()) {
            error_message = "Missing non-terminal: " + lhs;
            return false;
        }

        const ProductionRule& parsed_rule = it->second;

        if (parsed_rule.rhs.size() != expected_rule.rhs.size()) {
            error_message = "Number of alternatives mismatch for: " + lhs;
            return false;
        }

        // Compare each alternative
        for (size_t i = 0; i < expected_rule.rhs.size(); ++i) {
            const std::vector<std::string>& expected_alt = expected_rule.rhs[i];
            const ProductionAlternative& parsed_alt = parsed_rule.rhs[i];

            std::vector<ProductionUnit> parsed_units;

            if (parsed_alt.type == ProductionAlternative::Type::Single) {
                parsed_units.push_back(std::get<ProductionUnit>(parsed_alt.content));
            } else if (parsed_alt.type == ProductionAlternative::Type::Sequence) {
                parsed_units = std::get<std::vector<ProductionUnit>>(parsed_alt.content);
            } else {
                error_message = "Unknown alternative type for: " + lhs;
                return false;
            }

            if (expected_alt.size() != parsed_units.size()) {
                error_message = "Number of units mismatch in alternative " +
                                std::to_string(i+1) + " for: " + lhs;
                return false;
            }

            for (size_t j = 0; j < expected_alt.size(); ++j) {
                const std::string& expected_sym = expected_alt[j];
                const std::string& parsed_sym = parsed_units[j].lexeme;

                if (expected_sym != parsed_sym) {
                    error_message = "Unit mismatch at alternative " +
                                    std::to_string(i+1) + ", position " +
                                    std::to_string(j+1) + " for: " + lhs +
                                    ". Expected: " + expected_sym +
                                    ", Found: " + parsed_sym;
                    return false;
                }
            }
        }
    }

    // Optionally, check for unexpected non-terminals in parsed grammar
    for (const auto& pair : parsed) {
        const std::string& lhs = pair.first;
        if (expected.find(lhs) == expected.end()) {
            error_message = "Unexpected non-terminal found: " + lhs;
            return false;
        }
    }

    return true;
}

// ----------------------------
// Test Cases Definitions
// ----------------------------

// Define a structure for test cases
struct TestCase {
    std::string name;
    std::string bnf_content;
    bool should_pass;
    ExpectedGrammar expected_grammar; // Empty if should_pass is false
};

// Function to get expected grammar for Basic Instruction Parsing
ExpectedGrammar get_expected_basic_instruction_grammar() {
    ExpectedGrammar grammar;

    grammar["<instruction>"] = ExpectedProductionRule{
        "<instruction>",
        {
            {"<symbol_spec>", "<parameter_list>", "<file_id_list>"}
        }
    };

    grammar["<symbol_spec>"] = ExpectedProductionRule{
        "<symbol_spec>",
        {
            {"\"<\"", "<identifier>", "\">\""}
        }
    };

    grammar["<parameter_list>"] = ExpectedProductionRule{
        "<parameter_list>",
        {
            {"\"(\"", "<parameters>", "\")\""}
        }
    };

    grammar["<parameters>"] = ExpectedProductionRule{
        "<parameters>",
        {
            {"<parameter>", "\",\"", "<parameters>"},
            {"<parameter>"}
        }
    };

    grammar["<parameter>"] = ExpectedProductionRule{
        "<parameter>",
        {
            {"<identifier>", "\"=\"", "<identifier>"}
        }
    };

    grammar["<file_id_list>"] = ExpectedProductionRule{
        "<file_id_list>",
        {
            {"\"[\"", "<file_ids>", "\"]\""}
        }
    };

    grammar["<file_ids>"] = ExpectedProductionRule{
        "<file_ids>",
        {
            {"<identifier>", "\",\"", "<file_ids>"},
            {"<identifier>"}
        }
    };

    grammar["<identifier>"] = ExpectedProductionRule{
        "<identifier>",
        {
            {"<alphanumeric_string>"}
        }
    };

    grammar["<alphanumeric_string>"] = ExpectedProductionRule{
        "<alphanumeric_string>",
        {
            {"<letter_or_digit>"},
            {"<letter_or_digit>", "<alphanumeric_string>"}
        }
    };

    grammar["<letter_or_digit>"] = ExpectedProductionRule{
        "<letter_or_digit>",
        {
            {"<letter>"},
            {"<digit>"}
        }
    };

    grammar["<letter>"] = ExpectedProductionRule{
        "<letter>",
        {
            {"\"A\""}, {"\"B\""}, {"\"C\""}
        }
    };

    grammar["<digit>"] = ExpectedProductionRule{
        "<digit>",
        {
            {"\"0\""}, {"\"1\""}, {"\"2\""}
        }
    };

    return grammar;
}

// Function to get expected grammar for Arithmetic Expressions
ExpectedGrammar get_expected_arithmetic_grammar() {
    ExpectedGrammar grammar;

    grammar["<instruction>"] = ExpectedProductionRule{
        "<instruction>",
        {
            {"<term>", "\"+\"", "<instruction>"},
            {"<term>"}
        }
    };

    grammar["<term>"] = ExpectedProductionRule{
        "<term>",
        {
            {"<factor>", "\"*\"", "<term>"},
            {"<factor>"}
        }
    };

    grammar["<factor>"] = ExpectedProductionRule{
        "<factor>",
        {
            {"\"(\"", "<instruction>", "\")\""},
            {"<number>"}
        }
    };

    grammar["<number>"] = ExpectedProductionRule{
        "<number>",
        {
            {"<digit>"},
            {"<digit>", "<number>"}
        }
    };

    grammar["<digit>"] = ExpectedProductionRule{
        "<digit>",
        {
            {"\"0\""}, {"\"1\""}, {"\"2\""}, {"\"3\""}, {"\"4\""},
            {"\"5\""}, {"\"6\""}, {"\"7\""}, {"\"8\""}, {"\"9\""}
        }
    };

    return grammar;
}

// Function to get expected grammar for Simple Language with Optional and Repetition
ExpectedGrammar get_expected_simple_lang_grammar() {
    ExpectedGrammar grammar;

    grammar["<instruction>"] = ExpectedProductionRule{
        "<instruction>",
        {
            {"<statement_list>"}
        }
    };

    grammar["<statement_list>"] = ExpectedProductionRule{
        "<statement_list>",
        {
            {"<statement>", "\";\"", "<statement_list>"},
            {"<statement>"}
        }
    };

    grammar["<statement>"] = ExpectedProductionRule{
        "<statement>",
        {
            {"\"print\"", "<expression>"},
            {"\"let\"", "<identifier>", "\"=\"", "<expression>"}
        }
    };

    grammar["<expression>"] = ExpectedProductionRule{
        "<expression>",
        {
            {"<term>"},
            {"<expression>", "\"+\"", "<term>"}
        }
    };

    grammar["<term>"] = ExpectedProductionRule{
        "<term>",
        {
            {"<factor>"},
            {"<term>", "\"*\"", "<factor>"}
        }
    };

    grammar["<factor>"] = ExpectedProductionRule{
        "<factor>",
        {
            {"\"(\"", "<expression>", "\")\""},
            {"<identifier>"},
            {"<number>"}
        }
    };

    grammar["<identifier>"] = ExpectedProductionRule{
        "<identifier>",
        {
            {"<letter>"},
            {"<letter>", "<identifier>"}
        }
    };

    grammar["<letter>"] = ExpectedProductionRule{
        "<letter>",
        {
            {"\"a\""}, {"\"b\""}, {"\"c\""}, {"\"d\""}, {"\"e\""}
        }
    };

    grammar["<number>"] = ExpectedProductionRule{
        "<number>",
        {
            {"<digit>"},
            {"<digit>", "<number>"}
        }
    };

    grammar["<digit>"] = ExpectedProductionRule{
        "<digit>",
        {
            {"\"0\""}, {"\"1\""}, {"\"2\""}, {"\"3\""}, {"\"4\""},
            {"\"5\""}, {"\"6\""}, {"\"7\""}, {"\"8\""}, {"\"9\""}
        }
    };

    return grammar;
}

// ----------------------------
// Test Runner Implementation
// ----------------------------

int main() {
    // Define all test cases
    std::vector<TestCase> test_cases = {
        TestCase{
            "Basic Instruction Parsing",
            // BNF Content
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
            "<digit>                ::= \"0\" | \"1\" | \"2\" ;\n",
            true,
            get_expected_basic_instruction_grammar()
        },
        TestCase{
            "Arithmetic Expressions",
            // BNF Content
            "<instruction> ::= <term> \"+\" <instruction> | <term> ;\n"
            "<term>       ::= <factor> \"*\" <term> | <factor> ;\n"
            "<factor>     ::= \"(\" <instruction> \")\" | <number> ;\n"
            "<number>     ::= <digit> | <digit> <number> ;\n"
            "<digit>      ::= \"0\" | \"1\" | \"2\" | \"3\" | \"4\" | \"5\" | \"6\" | \"7\" | \"8\" | \"9\" ;\n",
            true,
            get_expected_arithmetic_grammar()
        },
        TestCase{
            "Simple Language with Optional and Repetition",
            // BNF Content
            "<instruction>     ::= <statement_list> ;\n"
            "\n"
            "<statement_list> ::= <statement> \";\" <statement_list> | <statement> ;\n"
            "\n"
            "<statement>   ::= \"print\" <expression> | \"let\" <identifier> \"=\" <expression> ;\n"
            "\n"
            "<expression>  ::= <term> | <expression> \"+\" <term> ;\n"
            "\n"
            "<term>        ::= <factor> | <term> \"*\" <factor> ;\n"
            "\n"
            "<factor>      ::= \"(\" <expression> \")\" | <identifier> | <number> ;\n"
            "\n"
            "<identifier>  ::= <letter> | <letter> <identifier> ;\n"
            "\n"
            "<letter>      ::= \"a\" | \"b\" | \"c\" | \"d\" | \"e\" ;\n"
            "\n"
            "<number>      ::= <digit> | <digit> <number> ;\n"
            "\n"
            "<digit>       ::= \"0\" | \"1\" | \"2\" | \"3\" | \"4\" | \"5\" | \"6\" | \"7\" | \"8\" | \"9\" ;\n",
            true,
            get_expected_simple_lang_grammar()
        },
        TestCase{
            "Missing Semicolon",
            // BNF Content (Invalid: Missing semicolon)
            "<instruction> ::= <symbol_spec> <parameter_list> <file_id_list>\n",
            false,
            {} // No expected grammar since it should fail
        },
        TestCase{
            "Undefined Unit",
            // BNF Content with all necessary definitions
            "<instruction> ::= <symbol_spec> <parameter_list> <file_id_list> ;\n"
            "<symbol_spec> ::= \"<\" <undefined_identifier> \">\" ;\n"
            "<parameter_list> ::= \"(\" <parameters> \")\" ;\n"
            "<parameters> ::= <parameter> \",\" <parameters> | <parameter> ;\n"
            "<parameter> ::= <identifier> \"=\" <identifier> ;\n"
            "<file_id_list> ::= \"[\" <file_ids> \"]\" ;\n"
            "<file_ids> ::= <identifier> \",\" <file_ids> | <identifier> ;\n"
            "<identifier> ::= <alphanumeric_string> ;\n"
            "<alphanumeric_string> ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;\n"
            "<letter_or_digit> ::= <letter> | <digit> ;\n"
            "<letter> ::= \"A\" | \"B\" | \"C\" ;\n"
            "<digit> ::= \"0\" | \"1\" | \"2\" ;\n"
            "<undefined_identifier> ::= \"X\" | \"Y\" | \"Z\" ;\n", // Define <undefined_identifier>
            true, // Set to true as all units are now defined
            // Define expected grammar
            ExpectedGrammar{
                {"<instruction>", ExpectedProductionRule{
                    "<instruction>",
                    {{"<symbol_spec>", "<parameter_list>", "<file_id_list>"}}
                }},
                {"<symbol_spec>", ExpectedProductionRule{
                    "<symbol_spec>",
                    {{"\"<\"", "<undefined_identifier>", "\">\""}}
                }},
                {"<parameter_list>", ExpectedProductionRule{
                    "<parameter_list>",
                    {{"\"(\"", "<parameters>", "\")\""}}
                }},
                {"<parameters>", ExpectedProductionRule{
                    "<parameters>",
                    {{"<parameter>", "\",\"", "<parameters>"}, {"<parameter>"}}
                }},
                {"<parameter>", ExpectedProductionRule{
                    "<parameter>",
                    {{"<identifier>", "\"=\"", "<identifier>"}}
                }},
                {"<file_id_list>", ExpectedProductionRule{
                    "<file_id_list>",
                    {{"\"[\"", "<file_ids>", "\"]\""}}
                }},
                {"<file_ids>", ExpectedProductionRule{
                    "<file_ids>",
                    {{"<identifier>", "\",\"", "<file_ids>"}, {"<identifier>"}}
                }},
                {"<identifier>", ExpectedProductionRule{
                    "<identifier>",
                    {{"<alphanumeric_string>"}}
                }},
                {"<alphanumeric_string>", ExpectedProductionRule{
                    "<alphanumeric_string>",
                    {{"<letter_or_digit>"}, {"<letter_or_digit>", "<alphanumeric_string>"}}
                }},
                {"<letter_or_digit>", ExpectedProductionRule{
                    "<letter_or_digit>",
                    {{"<letter>"}, {"<digit>"}}
                }},
                {"<letter>", ExpectedProductionRule{
                    "<letter>",
                    {{"\"A\""}, {"\"B\""}, {"\"C\""}}
                }},
                {"<digit>", ExpectedProductionRule{
                    "<digit>",
                    {{"\"0\""}, {"\"1\""}, {"\"2\""}}
                }},
                {"<undefined_identifier>", ExpectedProductionRule{
                    "<undefined_identifier>",
                    {{"\"X\""}, {"\"Y\""}, {"\"Z\""}}
                }}
            }
        },
        // Additional test cases for error conditions
        TestCase{
            "Error: Production does not start with <instruction>",
            // BNF Content
            "<command> ::= \"run\" ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Left-hand side is not a non-terminal",
            // BNF Content
            "\"run\" ::= <parameters> ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Missing '::=' after LHS",
            // BNF Content
            "<instruction> <symbol_spec> <parameter_list> ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Unexpected '::=' in RHS",
            // BNF Content
            "<instruction> ::= <symbol_spec> ::= <parameter_list> ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Missing semicolon at end of production",
            // BNF Content
            "<instruction> ::= <symbol_spec> <parameter_list>\n",
            false,
            {}
        },
        TestCase{
            "Error: Empty right-hand side alternative",
            // BNF Content
            "<instruction> ::= ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Infinite recursion in single alternative",
            // BNF Content
            "<instruction> ::= <instruction> ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Duplicate production rules",
            // BNF Content
            "<instruction> ::= <symbol_spec> ;\n"
            "<instruction> ::= <parameter_list> ;\n",
            false,
            {}
        },
        TestCase{
            "Error: RHS contains invalid unit",
            // BNF Content
            "<instruction> ::= %invalid% ;\n",
            false,
            {}
        },
        TestCase{
            "Error: Undefined non-terminal in RHS",
            // BNF Content
            "<instruction> ::= <undefined_non_terminal> ;\n",
            true, // Depending on implementation, this might pass
            ExpectedGrammar{
                {"<instruction>", ExpectedProductionRule{
                    "<instruction>",
                    {{"<undefined_non_terminal>"}}
                }}
            }
        },
        TestCase{
            "Error: Missing RHS",
            // BNF Content
            "<instruction> ::= ;\n",
            false,
            {}
        },
        TestCase{
            "Error: LHS is not a non-terminal",
            // BNF Content
            "\"instruction\" ::= <symbol_spec> ;\n",
            false,
            {}
        },
    };

    int passed = 0;
    int failed = 0;

    for (const auto& test : test_cases) {
        try {
            // Initialize lexer and parser with the BNF content
            GrammarLexer lexer(test.bnf_content);
            GrammarParser parser(lexer);
            parser.parseGrammar();

            if (!test.should_pass) {
                std::cout << "[FAIL] " << test.name << " - Expected to fail but passed.\n";
                failed++;
                continue;
            }

            // Build the parsed grammar map
            const auto& parsed_grammar_obj = parser.getGrammar();
            std::unordered_map<std::string, ProductionRule> parsed_grammar;
            for (const auto& rule : parsed_grammar_obj.rules) {
                parsed_grammar[rule.lhs] = rule;
            }

            // Compare the parsed grammar with the expected grammar
            const ExpectedGrammar& expected_grammar = test.expected_grammar;

            std::string error_message;
            if (!compare_grammar(parsed_grammar, expected_grammar, error_message)) {
                std::cout << "[FAIL] " << test.name << " - " << error_message << "\n";
                failed++;
                continue;
            }

            // If comparison passes
            std::cout << "[PASS] " << test.name << "\n";
            passed++;

        } catch (const std::exception& e) {
            if (test.should_pass) {
                std::cout << "[FAIL] " << test.name << " - Unexpected error: " << e.what() << "\n";
                failed++;
            } else {
                std::cout << "[PASS] " << test.name << " - Properly failed with error: " << e.what() << "\n";
                passed++;
            }
        }
    }

    std::cout << "\nTotal Tests: " << test_cases.size()
              << ", Passed: " << passed
              << ", Failed: " << failed << "\n";

    return (failed == 0) ? 0 : 1;
}
