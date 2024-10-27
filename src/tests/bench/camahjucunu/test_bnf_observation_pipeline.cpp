// main.cpp
#include <iostream>
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

int main() {
    try {
        std::string bnfGrammar = 
        "<instruction>         ::= <symbol_spec> [ <parameter_list> ] [ <file_id_list> ] ;\n"
        "<symbol_spec>         ::= \"<\" <identifier> \">\" ;\n"
        "<parameter_list>      ::= \"(\" <parameters> \")\" ;\n"
        "<parameters>          ::= <parameter> { \",\" <parameter> } | ;\n"
        "<parameter>           ::= <identifier> \"=\" <identifier> ;\n"
        "<file_id_list>        ::= \"[\" <file_ids> \"]\" ;\n"
        "<file_ids>            ::= <identifier> { \",\" <identifier> } | ;\n"
        "<identifier>          ::= <alphanumeric_string> ;\n"
        "<alphanumeric_string> ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;\n"
        "<letter_or_digit>     ::= <letter> | <digit> ;\n"
        "<letter>              ::= \"A\" | \"B\" | \"C\" | \"D\" | \"E\" | \"F\" | \"G\" | \"H\" | \"I\" | \"J\" | \"K\" | \"L\" | \"M\" | \"N\" | \"O\" | \"P\" | \"Q\" | \"R\" | \"S\" | \"T\" | \"U\" | \"V\" | \"W\" | \"X\" | \"Y\" | \"Z\" | \"a\" | \"b\" | \"c\" | \"d\" | \"e\" | \"f\" | \"g\" | \"h\" | \"i\" | \"j\" | \"k\" | \"l\" | \"m\" | \"n\" | \"o\" | \"p\" | \"q\" | \"r\" | \"s\" | \"t\" | \"u\" | \"v\" | \"w\" | \"x\" | \"y\" | \"z\" ;"
        "<digit>               ::= \"0\" | \"1\" | \"2\" | \"3\" | \"4\" | \"5\" | \"6\" | \"7\" | \"8\" | \"9\" ;";

        std::string instruction = "<BTCUSDT>(n=10)[1m,15m,1d]";

        // Initialize BNF lexer and parser with the BNF grammar
        cuwacunu::camahjucunu::BNF::GrammarLexer bnfLexer(bnfGrammar);
        cuwacunu::camahjucunu::BNF::GrammarParser bnfParser(bnfLexer);

        // Parse the BNF grammar
        bnfParser.parseGrammar();
        const auto& grammar = bnfParser.getGrammar();

        // Initialize instruction lexer and parser with the instruction input and parsed grammar
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer(instruction);
        cuwacunu::camahjucunu::BNF::InstructionParser parser(lexer, grammar);

        // Parse the instruction input
        auto actualAST = parser.parse();

        // Optionally, print the AST for debugging
        std::cout << "Parsed AST:\n";
        cuwacunu::camahjucunu::BNF::printAST(actualAST.get(), 2, std::cout);

        // Create an observationPipeline visitor and traverse the AST
        cuwacunu::camahjucunu::BNF::observationPipeline pipelineVisitor;
        actualAST->accept(pipelineVisitor);

        std::cout << "Parsing and execution successful!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
