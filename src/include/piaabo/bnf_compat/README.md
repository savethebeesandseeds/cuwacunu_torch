# bnf_compat Specification

`bnf_compat` is the internal parser stack for grammar-defined instruction DSLs.

Namespace: `cuwacunu::piaabo::bnf`
Public include form: `#include "piaabo/bnf_compat/..."`

## Core Contract

The module defines a 2-stage parse pipeline:

- Grammar stage: text -> `GrammarLexer` -> `GrammarParser` -> `ProductionGrammar`
- Instruction stage: instruction text + `ProductionGrammar` -> `InstructionParser` -> AST (`ASTNodePtr`)

## Grammar Model Contract

Public model objects:

- `ProductionUnit`
- `ProductionAlternative`
- `ProductionRule`
- `ProductionGrammar`

Accepted unit families in grammar parsing:

- terminal literals
- non-terminals (`<...>`)
- optionals (`[<...>]`)
- repetitions (`{<...>}`)
- punctuation (`::=`, `|`, `;`)

Current parser constraints from source behavior:

- first production must start with `<instruction> ::= ...`
- optional and repetition wrappers are parsed as wrappers around non-terminals
- comment lines are recognized when `;` appears at column 1

## Instruction Parse Contract

Public entrypoint:

- `InstructionParser::parse_Instruction(const std::string&)`

Matching behavior:

- alternatives are attempted and the parser keeps the match that consumes the longest input
- terminal matching is exact character-by-character
- repetition parse currently succeeds only when at least one repetition item is matched
- parsing failure throws `std::runtime_error` with failure context

AST contract:

- root type: `RootNode`
- intermediate type: `IntermediaryNode`
- leaves: `TerminalNode`
- visitor extension point: `ASTVisitor` + `VisitorContext`
- node `hash` is derived from `fnv1aHash(name)`

## Validation and Failure Contract

- grammar integrity check is exposed as `verityGrammar(...)`
- malformed grammar/instruction paths throw exceptions (no silent recovery)

## Legacy Ghosts

- Grammar verification quality is flagged in source as needing improvement.
- `InstructionParser::parse_ProductionAlternative` currently does not consume `ProductionAlternative::Flags` semantics.
- Parser performance is explicitly flagged for improvement in instruction parsing hot paths.
- AST `hash` naming/semantics have a known mismatch warning (`fnv1aHash(name)` used, but internal warning notes hash semantics debt).

This spec defines behavior and interfaces; source remains authoritative for parser internals.
