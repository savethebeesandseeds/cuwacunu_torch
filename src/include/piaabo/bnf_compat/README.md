# Waajacu BNF Parser

## Overview
The **BNF Parser** is a C++ tool designed to parse grammars written in Backus-Naur form notation and interpret instruction statements based on those grammars. It generates Abstract Syntax Trees (ASTs) from valid inputs, offering basic functionality to create simple domain-specific languages (DSLs).

Please note that this project is an internal tool and may lack some modern features available in other BNF parsers. While functional for specific use cases, it is not intended to compete with more comprehensive solutions.


## Features
- **BNF Grammar Parsing**: Define and parse BNF grammars with ease.
- **Instruction Parsing**: Interpret instructions based on BNF rules and generate ASTs.
- **Supported Constructs**: Non-terminals, terminals, optional elements `[ ]`, repetitions `{}`, recurrences and alternatives `|`.
- **Robust Error Handling**: Clear error messages with line and column information.
- **Comprehensive Testing**: Includes lexer and parser test suites to ensure reliability.

## Supported BNF Constructs
- **Non-Terminals**: Enclosed in `< >`, e.g., `<instruction>`.
- **Terminals**: Enclosed in `" "`, e.g., `"+"`, `"A"`.
- **Production Rules**: Defined using `::=`, e.g., `<expr> ::= <term> "+" <expr> ;`.
- **Alternatives**: Separated by `|`, e.g., `<digit> ::= "0" | "1" | "2" ;`.
- **Optional Elements**: Enclosed in `[ ]`, e.g., `[ <parameter_list> ]`.
- **Repetitions**: Two elements, e.g. `<list> ::= {<item>} ;`
- **Recurrence**: Recurrence, e.g. `<list> ::= <item> "," <list> | <item>  ;`
- **End of Production**: Denoted by `;`.

Note: The order in defining the alternatives is important. 
this: `<list> ::= <item> "," | <list>  ;`
is different than: `<list> ::= <list> | <item> "," ;`

Examples:
```
<basic_asignment>      ::= <item> ;
<basic_sequence>       ::= "(" <item> ")" ;
<basic_optional>       ::= [<item>] ;
<basic_repetition>     ::= {<item>} ;
<basic_recurrence>     ::= <item> "," <basic_recurrence> | <item>  ;
```
```
<letter_or_digit>      ::= <letter> | <digit> ;
<letter>               ::= "A" | "B" | "C" ;
<digit>                ::= "0" | "1" | "2" ;
```


## Usage

### Extensive Example

**BNF Grammar:**
```
         Grammar: 
<instruction>     ::= "(" <string> ")" ;
<string>          ::= {<letter_or_digit>} ;
<letter_or_digit> ::= <letter> | <digit> ;
<letter>          ::= "A" | "B" | "C" ;
<digit>           ::= "0" | "1" | "2" ;
```
Given the following input as the instruction
```
         Input: (A1B2)
```
Would produce the be reresented as the following Abstract Syntax Tree.
```
RootNode: <instruction>
└── IntermediaryNode: Sequence: Terminal: "("  NonTerminal: <string>  Terminal: ")"  
        ├── TerminalNode: "("
        ├── IntermediaryNode: Single: Repetition: {<letter_or_digit>}  
        │   └── IntermediaryNode: Single: Repetition: {<letter_or_digit>}  
        │       ├── IntermediaryNode: Single: NonTerminal: <letter>  
        │       │   └── IntermediaryNode: Single: Terminal: "A"  
        │       │       └── TerminalNode: "A"
        │       ├── IntermediaryNode: Single: NonTerminal: <digit>  
        │       │   └── IntermediaryNode: Single: Terminal: "1"  
        │       │       └── TerminalNode: "1"
        │       ├── IntermediaryNode: Single: NonTerminal: <letter>  
        │       │   └── IntermediaryNode: Single: Terminal: "B"  
        │       │       └── TerminalNode: "B"
        │       └── IntermediaryNode: Single: NonTerminal: <digit>  
        │           └── IntermediaryNode: Single: Terminal: "2"  
        │               └── TerminalNode: "2"
        └── TerminalNode: ")"
```
