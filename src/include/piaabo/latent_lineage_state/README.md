# Runtime `.lls` Standard

This directory defines the strict runtime `.lls` standard for cuwacunu and the
migration documents around it.

Scope:

- standalone runtime/report `.lls` artifacts
- embedded runtime `.lls` subdocuments stored inside joined Hero `report_lls`
- deterministic canonical emission rules under
  `cuwacunu::piaabo::latent_lineage_state`
- schema registry, audit notes, and migration backlog for later producer adoption

Out of scope in this pass:

- config and instruction `.dsl` files that also reuse latent-lineage syntax
- schema-specific report deserializers
- API-only latent-lineage serializers that do not persist standard runtime artifacts

## Implemented Runtime Boundary

The standardized runtime boundary now exists in code:

- strict runtime grammar: `src/config/bnf/runtime_lls.bnf`
- public runtime API: `src/include/piaabo/latent_lineage_state/runtime_lls.h`
- validator and canonical emitter:
  `src/impl/piaabo/latent_lineage_state/runtime_lls.cpp`
- shared legacy grammar basis still available in
  `src/config/bnf/latent_lineage_state.bnf`

The current `piaabo` surface is intentionally small:

- `runtime_lls_document_t`
- `runtime_lls_entry_t`
- `validate_runtime_lls_text(...)`
- `parse_runtime_lls_text(...)`
- `validate_runtime_lls_document(...)`
- `runtime_lls_document_to_kv_map(...)`
- `emit_runtime_lls_canonical(...)`
- typed entry builders for `str`, `bool`, `int`, `uint`, and `double`

## Standard Layers

### 1. Source Pre-Pass

Runtime `.lls` source is normalized before grammar validation:

- only `/* ... */` comments are allowed in source input
- block comments may appear before, after, or inside assignments
- block comments may carry human-only interpretation notes such as
  `/* lower is better */`
- comment stripping preserves line breaks
- `#`, `;`, and other legacy comment markers are rejected
- there is no quoting or escaping layer in runtime `.lls`
- raw string/domain payloads therefore must not contain literal `/*` or `*/`
- canonical emitted `.lls` remains comment-free; comments are source-side
  advisory text only and are not part of machine semantics

### 2. Strict Runtime Grammar

The runtime grammar is now BNF-enforceable through `runtime_lls.bnf`.

Syntactic rules:

- one assignment per logical line
- blank lines are allowed
- LF and CRLF line endings are accepted
- every assignment uses explicit typed LHS: `<key> [<domain>] : <type> = <value>`
- `<key>` is an identifier token
- `<type>` is one of `str`, `bool`, `int`, `uint`, or `double`
- values are stored as raw trimmed text and validated by declared type afterward

This strict grammar is the normative syntax for runtime `.lls`. The older
`latent_lineage_state.bnf` remains the broader shared syntax surface for other DSL
families.

### 3. Runtime Profile

The runtime profile adds validation rules above the grammar:

- exactly one schema-like key is required:
  top-level standalone payloads use `schema`
  embedded synthetic payloads may use a namespaced `*.schema` key
- the schema-like key must be the first assignment in source text
- the schema-like key must use declared type `str`
- schema values must use the versioned suffix form `owner.namespace.name.vN`
- duplicate keys are invalid
- supported declared types are `str`, `bool`, `int`, `uint`, and `double`
- declared domains must be empty, `(range)`, or `[enum]`
- declared domains are producer-declared interpretation/reference ranges
  or vocabularies; runtime validation checks only domain syntax, not whether a
  value numerically lies inside the declared range text
- runtime `.lls` does not assign open/closed interval semantics to the domain
  delimiters; range text is advisory and schema-owned
- `double` values must be finite
- raw `str` values must be single-line and grammar-representable with no escaping
- raw `str` values and declared domains must not contain `/*` or `*/`

Canonical emission rules:

- ASCII text with `\n` line endings
- no comments
- explicit declared types on every line
- booleans emit as `true` or `false`
- integers emit as base-10 ASCII
- unsigned integers emit as base-10 ASCII
- doubles emit as fixed-point ASCII with 12 fractional digits
- keys emit in deterministic order:
  schema-like key, `report_kind`, `tsi_type`, `canonical_path`, `hashimyei`,
  `contract_hash`, `wave_hash`, `binding_id`, `run_id`, then remaining keys in
  lexicographic order

Collection rule:

- runtime `.lls` is scalar-only at the type layer
- repeated/list-like payloads must be flattened into deterministic key families
  instead of using `arr[...]`

Document-level canonicalization is allowed to infer missing type/domain metadata
using the existing latent-lineage helper rules before emission.

## Deployed Runtime Boundary

Persisted runtime/report `.lls` surfaces are now expected to be emitted and read
through the strict runtime boundary.

Strict runtime `.lls` is now used for:

- persisted runtime artifact emission in the migrated `torch_compat`, source
  projection, lattice projection, Vicreg transfer-matrix, and Vicreg status paths
- persisted runtime artifact ingest in Lattice and Hashimyei
- schema extraction helpers for runtime analytics payloads

Persisted standalone runtime-artifact ingestion adds one narrower rule on top of
the shared parser:

- persisted standalone runtime `.lls` files must use top-level `schema`
- namespaced `*.schema` keys remain valid only for embedded synthetic subdocuments
  carried inside joined/query-time transports

Legacy relaxed readers still remain only where the payload is not part of the
strict persisted runtime-artifact contract:

- run manifests and component manifests
- synthetic joined `report_lls` transport
- cell-report and other non-runtime helper payloads

Synthetic joined `report_lls` transports may use `/* ... */` comment separators
for readability, but they remain transport text, not canonical runtime `.lls`
documents.

## Document Set

- `CURRENT_RUNTIME_AUDIT.md`: deployed producers, consumers, and mismatches
- `SCHEMA_REGISTRY.md`: normalized schema registry and status classification
- `MIGRATION_BACKLOG.md`: remaining producer/reader adoption work
- `src/config/man/runtime_lls.man`: compact runtime-profile rule sheet
