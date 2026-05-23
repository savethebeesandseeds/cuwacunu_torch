# Lattice Operational Readiness V3-J Schema Compatibility Audit

Generated on `2026-05-23`.

V3-J is a Hero MCP harness-safety gate. It is not a lattice proof condition,
does not change target satisfaction, and does not change Runtime Hero execution
policy.

## Guardrail

`src/include/hero/mcp_schema_compat.h` validates tool parameter schemas before
sourced Hero catalogs are emitted.

The compatibility rule is:

```text
- inputSchema root must be a JSON object
- inputSchema.type must be object
- top-level oneOf is rejected
- top-level anyOf is rejected
- top-level allOf is rejected
- top-level enum is rejected
- top-level not is rejected
```

Failures name:

```text
tool
schema_path
construct
message
```

## Regression

The focused bench test is:

```text
make -C src/tests/bench/kikijyeba/runtime run-test_hero_mcp_schema_compat -j12
```

It covers:

```text
- bad sample schema with top-level oneOf
- bad sample schema with top-level enum
- bad sample schema with non-object top-level type
- valid sample object schema
- generated Config Hero catalog
- generated Runtime Hero catalog
- generated Hashimyei Hero catalog
- generated Lattice Hero catalog
- hero.lattice.checkpoint_closure is present and schema-compatible
```

Observed result:

```text
passed
```

## Boundary

This audit prevents MCP harness breakage such as the prior
`hero_lattice_checkpoint_closure` invalid root schema failure. It does not grant
DB authority, does not write evidence, and does not execute waves.
