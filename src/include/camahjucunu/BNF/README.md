# BNF Parser

## Overview
The **BNF Parser** is a C++ tool designed to parse grammars written in Backus-Naur Form (BNF) and interpret instruction statements based on these grammars. It generates Abstract Syntax Trees (ASTs) from valid inputs, offering basic functionality to create simple domain-specific languages (DSLs). 

Please note that this project is an internal tool and may lack some modern features available in other BNF parsers. While functional for specific use cases, it is not intended to compete with more comprehensive solutions.


## Features
- **BNF Grammar Parsing**: Define and parse BNF grammars with ease.
- **Instruction Parsing**: Interpret instructions based on BNF rules and generate ASTs.
- **Supported Constructs**: Non-terminals, terminals, optional elements `[ ]`, and alternatives `|`.
- **Robust Error Handling**: Clear error messages with line and column information.
- **Comprehensive Testing**: Includes lexer and parser test suites to ensure reliability.

## Supported BNF Constructs
- **Non-Terminals**: Enclosed in `< >`, e.g., `<instruction>`.
- **Terminals**: Enclosed in `" "`, e.g., `"+"`, `"A"`.
- **Production Rules**: Defined using `::=`, e.g., `<expr> ::= <term> "+" <expr> ;`.
- **Alternatives**: Separated by `|`, e.g., `<digit> ::= "0" | "1" | "2" ;`.
- **Optional Elements**: Enclosed in `[ ]`, e.g., `[ <parameter_list> ]`.
- **Repetitions**: Two elements, e.g. `<list> ::= {<items>} ;`
- **Repetitions**: Recurrence, e.g. `<list> ::= <item> "," | <list>  ;`
- **End of Production**: Denoted by `;`.

Note: The order in defining the alternatives is important. 
this: `<list> ::= <item> "," | <list>  ;`
is different than: `<list> ::= <list> | <item> "," ;`

## Usage

### Creating .bnf Files
Define your grammar using BNF syntax. Example:

```bnf
<expression> ::= <number> | <expression> "+" <number> ;

<number> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
```

### Creating Instruction Statements
Write instructions that conform to your BNF grammar.

Example: 
```
1 + 2
```
Example: 
```
3 + 4 + 5
```

But if you change the grammar definition file, you can achieve other different instructions. Please see BNF (Backus-Naur Form) for more on this. 

### Extensive Example

**BNF Grammar:**
```
<instruction> ::= <variable_1> [ <list_1> ] [ <list_2> ] ;

<variable_1> ::= "<" <identifier> ">" ;

<list_1> ::= "(" <parameters> ")" ;

<parameters> ::= { "," <parameter> } ;

<parameter> ::= <identifier> "=" <identifier> ;

<list_2> ::= "[" <list_2_types> "]" ;

<list_2_types> ::= <identifier> | <identifier> "," <list_2_types> ;

<identifier> ::= <alphanumeric_string> ;

<alphanumeric_string> ::= <letter_or_digit> | <letter_or_digit> <alphanumeric_string> ;

<letter_or_digit> ::= <letter> | <digit> ;

<letter> ::= "A" | "B" | "C" | "D" ;

<digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" ;
```
### Instruction Input:
```
<ACB>(A=10,B=20)[0,1,2,D1,AAA1]
```
### Parsed AST (abstract syntax tree):
```
<instruction>
  <variable_1>
    <identifier> ACB
  <list_1>
    <parameters>
      <parameter>
        <identifier> A
        <identifier> 10
      <parameter>
        <identifier> B
        <identifier> 20
  <list_2>
    <list_2_types>
      <identifier> 0
      <identifier> 1
      <identifier> 2
      <identifier> D1
      <identifier> AAAA1
```

### Contributing

Contributions are welcome! Please fork the repository, create a new branch for your feature or bugfix, and submit a pull request.

### License

This project is licensed under the MIT License.
