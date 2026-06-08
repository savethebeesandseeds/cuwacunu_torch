# digest_hash_id_surface_cleanup.v1

## Objective

Clean up digest, hash, fingerprint, id, and short-reference surfaces across
Hero, Runtime, Lattice, Marshal, Kikijyeba, Cajtucu, Wikimyei, docs, and tests.

The goal is not to weaken evidence identity. The goal is to make the operator
surface humane while keeping canonical artifacts exact.

```text
Canonical evidence:
    full digest, exact, machine authority

Operator reference:
    short ref / unique prefix / job id / target id

Human summary:
    short ref only unless exact identity is required
```

This should be handled as a separate implementation session.

## Why This Matters

Recent PPO and policy-training lineage work exposed too many long hashes in
operator-visible output and assistant summaries. Those digests are useful for
Lattice proof, but they are awkward as a public vocabulary.

The current surface mixes several concepts:

```text
target_id
job_id
checkpoint_id
fact_digest
digest
digest_prefix
fact_digest_prefix
contract_digest
report_digest
checkpoint_digest
certificate_digest
fingerprint
hash
ref
```

Some of these are stable names, some are exact content identities, some are
shortened content identities, and some are proof fingerprints. The code should
make those categories explicit.

## Core Law

Do not shorten canonical evidence.

```text
Facts, reports, proof certificates, parent lineage, checkpoint identity,
and Lattice proof internals must keep full digests.
```

Do shorten or hide operator-facing display.

```text
CLI/MCP summaries, Marshal summaries, assistant summaries, previews, and docs
should use stable ids or short refs unless the exact digest is specifically
needed.
```

Short refs must fail closed.

```text
short ref matches exactly one known full digest -> accepted
short ref matches zero known full digests       -> not found
short ref matches multiple known full digests  -> ambiguous, require longer ref
```

## Current Observations

Lattice already has partial prefix support:

```text
hero.lattice.inspect subject=facts
    fact_digest
    digest
    digest_prefix
    fact_index
    index
```

Marshal has a slightly different public naming surface:

```text
hero.marshal.inspect subject=facts
    fact_digest
    fact_digest_prefix
    fact_index
```

This mismatch should be cleaned up. Prefer one canonical public spelling:

```text
fact_digest_prefix
```

Lattice may continue accepting `digest_prefix` only if we intentionally keep it
as an alias. If we choose no backward compatibility for this cleanup, remove the
old spelling from schema, parser, docs, and tests in the same session.

Runtime and Lattice also produce many full digest fields in JSON packets. Those
fields are valid machine payload, but the default human output should include
short refs and reserve full digests for explicit machine payload or verbose
mode.

## Orientation Audit Notes

The cleanup is broad. Treat the first implementable milestone as:

```text
Hero fact inspect naming and prefix safety
```

That milestone is complete when Lattice and Marshal fact inspection share public
selectors, prefix lookup fails closed, and docs/tests describe the same surface.
Defer general short-ref rollout, Runtime path layout, and broad human-summary
payload changes until that milestone is stable.

Initial source and MCP audit findings before this cleanup patch:

```text
hero-marshal MCP is registered in Codex config, but this Codex session exposed
only Lattice/Runtime/Config tool namespaces. Marshal behavior below comes from
source and tests.

Built Lattice MCP schema advertised:
    digest
    fact_digest
    digest_prefix
    fact_index
    index

Built Lattice MCP schema did not advertise:
    fact_digest_prefix

Lattice handle_fact_preview previously:
    accepts digest
    aliases fact_digest to digest only when digest is empty
    accepts digest_prefix
    aliases index to fact_index
    emits digest_filter
    emits digest_prefix_filter
    emits preview row field digest

Lattice prefix matching was starts_with filtering. It did not resolve
a prefix to exactly one full digest before returning rows. A prefix can match
multiple rows and still return a preview, so the fail-closed rule is not yet
implemented.

Marshal inspect source was ahead of Lattice for fact selectors:
    public schema advertises fact_digest, fact_digest_prefix, fact_index
    tests reject retired aliases digest, digest_prefix, index, family,
    and fact_family
    the Marshal adapter translated fact_digest_prefix to Lattice digest_prefix

Docs were stale in at least src/config/man/hero.marshal.man, which still said
digest, digest_prefix, and index can trigger preview even though Marshal tests
reject those aliases.

Lattice inspect uses one flat schema for subject=facts and subject=index.
digest_prefix is also used by subject=index mode=query, so removing it from the
global schema would affect index query unless Lattice gains subject-specific
validation or split public tools.
```

Implemented in `roadmap_and_operator_surface_cleanup.v1`:

```text
Lattice subject=facts mode=preview now:
    advertises fact_digest_prefix
    accepts fact_digest
    accepts fact_digest_prefix
    accepts fact_index
    rejects digest, digest_prefix, and index on the facts-preview path
    keeps digest and digest_prefix available for subject=index mode=query
    emits fact_digest_filter
    emits fact_digest_prefix_filter
    emits preview row field fact_digest
    rejects zero-match fact_digest_prefix with E_LATTICE_FACT_REF_NOT_FOUND
    rejects multi-match fact_digest_prefix with E_LATTICE_FACT_REF_AMBIGUOUS

Marshal inspect now:
    continues to expose fact_digest, fact_digest_prefix, and fact_index
    sends fact_digest_prefix through to Lattice without old-name translation
    summarizes fact_digest_filter and fact_digest_prefix_filter

Docs/tests updated:
    hero.lattice.man
    hero.marshal.man
    src/main/hero/README.md
    src/include/hero/lattice_hero/lattice/README.md
    src/include/hero/marshal_hero/marshal/README.md
    focused Runtime/MCP and Marshal tests
```

Later cross-module audit candidates:

```text
Jkimyei setup and analytics APIs still use contract_hash terminology. If these
are content identities over contracts, prefer contract_digest or
contract_fingerprint depending on semantics.

Wikimyei forecast artifacts use target_coords_hash and normalization_hash.
Kikijyeba replay code now also has target_coords_fingerprint. Normalize this
pair once the Hero fact-inspect cleanup is done.

Kikijyeba artifact_ref_t is a real operational reference container with
artifact_digest inside it. Do not rename it blindly; it is likely a legitimate
ref/digest split.
```

Runtime hash naming cleanup completed by the next cleanup patch:

```text
Runtime handoff JSON:
    base_config.hash -> base_config.digest
    runtime_policy.hash -> runtime_policy.digest
    checkpoint_artifact_hashes -> checkpoint_artifact_digests
    canonical handoff text uses base_config_digest/runtime_policy_digest
    Runtime validator accepts legacy hash fields for historical handoffs

Runtime terminal facts:
    source_report_hash -> source_report_digest
    config_bundle_hash -> config_bundle_id
    runtime_policy_hash -> runtime_policy_digest
    checkpoint_artifact_hash -> checkpoint_artifact_digest
    Lattice exposure/target readers accept legacy checkpoint_artifact_hash

Classified but deferred:
    contract_hash remains a Jkimyei setup/API cleanup candidate.
    target_coords_hash and normalization_hash remain a Wikimyei/Kikijyeba
    forecast-artifact cleanup candidate.

Intentionally unchanged:
    local hash variables inside FNV/digest implementations and path-layout
    digest prefixes are low-level implementation details, not public fields.
```

First implementation sequence completed by this cleanup patch:

```text
1. Add Lattice fact_digest_prefix support for subject=facts.
2. Decide alias policy explicitly:
   - rejected digest, digest_prefix, and index for subject=facts while retaining
     digest/digest_prefix for subject=index query
3. Add unique-prefix resolution for fact preview before selecting rows.
4. Emit canonical preview filter/row names:
     fact_digest_filter
     fact_digest_prefix_filter
     fact_digest
5. Update Marshal's Lattice adapter to send fact_digest_prefix once Lattice
   accepts it directly.
6. Update hero.lattice.man, hero.marshal.man, main Hero README, and focused
   tests in the same patch.
```

Validation for this completed sequence:

```text
git diff --check
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
```

Runtime/operator hash naming audit completed:

```text
Runtime/operator hash naming audit

Completion condition:
    classify each public/runtime-facing *_hash field found in the current audit
    candidates as digest, fingerprint, low-level hash, or operator ref; rename
    only the fields whose semantics and call sites are clear; update docs and
    focused tests for those renamed surfaces.

Renamed Runtime candidates:
    source_report_hash -> source_report_digest
    config_bundle_hash -> config_bundle_id
    runtime_policy_hash -> runtime_policy_digest
    checkpoint_artifact_hash -> checkpoint_artifact_digest

Deferred cross-module candidates:
    contract_hash
    target_coords_hash
    normalization_hash
```

## Terminology To Standardize

Use these meanings consistently.

```text
id
    Human-authored or stable operational name.
    Examples: target_id, job_id, policy_id, rollout_id.

digest
    Deterministic content identity.
    Used for exact lineage and proof.

fingerprint
    Deterministic identity over a contract, config, graph order, split policy,
    component assembly, or proof obligation.

hash
    Avoid in public surfaces unless referring to a low-level file hash.
    Prefer digest or fingerprint.

ref
    Operator display handle derived from an id or digest.
    Never proof authority by itself.

prefix
    A caller-supplied beginning of a full digest.
    Must resolve uniquely before use.
```

## Proposed Public Reference Model

Add short refs as display helpers.

```text
fact_ref       = <family-prefix>_<digest-prefix>
checkpoint_ref = ckpt_<digest-prefix>
report_ref     = rpt_<digest-prefix>
contract_ref   = ctr_<digest-prefix>
certificate_ref = cert_<digest-prefix>
```

Example:

```text
policy_training fact full digest:
    2a8eaae0c20d68bd

operator display:
    ptf_2a8eaae0c20d
```

Recommended default prefix length:

```text
12 hex chars for display
16 hex chars for generated stable paths where already used
full digest for canonical facts/proofs
```

The resolver must be able to ask for more characters when there is ambiguity.

Implemented in `short_ref_display_layer.v1`:

```text
Central helper:
    src/include/hero/short_ref.h
    display prefix length: 12
    fact family prefixes: exp, node, ckpt, src, san, ttf, fbl, fev, obl,
                          alloc, rep, ptf, sel, rsf
    non-fact helpers: ckpt_, rpt_, ctr_, cert_

Lattice display outputs:
    fact_preview rows include fact_ref and fact_digest_prefix next to full
    fact_digest
    fact_preview declares short_ref_scheme and display_digest_prefix_length
    artifact fact_preview_hint rows include fact_ref and fact_digest_prefix
    proof_certificate JSON includes certificate_ref next to certificate_digest
    checkpoint closure/evaluation JSON includes checkpoint_ref and
    root_checkpoint_ref next to full checkpoint digests

Marshal display outputs:
    preview_panel includes preview_fact_refs
    artifact_fact_preview_* summaries include artifact_fact_preview_refs

Safety boundary:
    fact_ref is display-only in this milestone. It is not accepted as a query
    selector or proof authority input.
```

Validation for `short_ref_display_layer.v1`:

```text
git diff --check
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
```

## Milestone Scope

### 1. Audit Existing Surfaces

Search the codebase for:

```text
fact_digest
digest_prefix
fact_digest_prefix
certificate_digest
target_spec_fingerprint
contract_digest
report_digest
checkpoint_digest
checkpoint_file_digest
checkpoint_id
component_spawn_fingerprint
runtime_terminal_fact_digest
runtime_checkpoint_io_fact_digest
artifact_hash
hash
_ref
_id
_fingerprint
```

Inspect at least these areas:

```text
src/impl/hero/lattice_hero/hero_lattice_tools.cpp
src/impl/hero/runtime_hero/hero_runtime_tools.cpp
src/include/hero/lattice_hero/lattice/exposure/exposure_ledger.h
src/include/hero/lattice_hero/lattice/target/lattice_target_evaluator.h
src/include/hero/marshal_hero/hero_marshal.def
src/include/hero/marshal_hero/marshal/tool_handler.h
src/include/hero/marshal_hero/marshal/target_driver.h
src/include/hero/runtime_hero/runtime/*
src/main/hero/README.md
src/config/man/hero.lattice.man
src/config/man/hero.marshal.man
src/config/man/hero.runtime.man
ROADMAP.md
```

Also inspect Kikijyeba, Cajtucu, Wikimyei, and jkimyei surfaces for digest/id
leakage into operator-visible reports:

```text
src/include/kikijyeba
src/include/cajtucu
src/include/wikimyei
src/include/jkimyei
src/config/*.dsl
src/config/*.net
src/config/*.jkimyei
src/config/man/*.man
```

### 2. Define Digest Taxonomy

Create a small shared vocabulary or README section that says which fields are:

```text
canonical_digest
proof_fingerprint
operator_id
operator_ref
legacy_alias
machine_only
human_default
```

This should cover:

```text
fact_digest
parent_*_fact_digest
report_digest
experiment_report_digest
checkpoint_digest
checkpoint_file_digest
contract_digest
target_spec_fingerprint
certificate_digest
component_spawn_fingerprint
graph_order_fingerprint
split_policy_fingerprint
source_cursor_token
```

### 3. Standardize Query Parameters

Pick canonical query names:

```text
fact_digest
fact_digest_prefix
fact_index
family
target_id
job_id
checkpoint_id
checkpoint_file_digest
```

Recommended cleanup:

```text
hero.lattice.inspect subject=facts
    accept fact_digest
    accept fact_digest_prefix
    accept fact_index
    stop exposing bare digest_prefix as the preferred public name

hero.marshal.inspect subject=facts
    use the same names as Lattice
```

If aliases are retained:

```text
alias digest -> fact_digest only for subject=facts
alias digest_prefix -> fact_digest_prefix only for subject=facts
output canonical field names only
docs mention aliases as transitional or internal
```

If no backward compatibility is desired:

```text
remove digest and digest_prefix from public schema
update tests and docs in the same patch
```

### 4. Add Short-Ref Generation

Add one central helper rather than ad hoc string truncation.

Possible helper names:

```cpp
digest_short_ref(family, full_digest)
digest_prefix(full_digest, length)
resolve_digest_prefix(family, prefix, ledger)
```

Rules:

```text
full digest empty -> no short ref
unknown family -> generic prefix
display prefix length configurable only at output layer
canonical digest serialization never calls short-ref helper
```

Suggested family prefixes:

```text
exp   exposure
node  node_exposure
ckpt  checkpoint
src   source_receipt
san   source_analytics
ttf   target_transform
fbl   forecast_baseline
fev   forecast_eval
obl   observer_belief
alloc allocation_engine
rep   replay_environment
ptf   policy_training
sel   selection_signal
rsf   representation_support
cert  proof_certificate
rpt   report
ctr   contract
```

### 5. Add Prefix Resolution

Implement unique-prefix resolution for fact previews.

Expected behavior:

```text
fact_digest=<full>          -> exact match
fact_digest_prefix=<prefix> -> unique prefix match
fact_ref=<family_prefix>_<prefix>, optional future -> unique prefix match
fact_index=<n>              -> row index match
```

Failure cases:

```text
no match:
    E_LATTICE_FACT_REF_NOT_FOUND

multiple matches:
    E_LATTICE_FACT_REF_AMBIGUOUS
    include candidate_count
    include minimum_required_prefix_length if cheap to compute
```

Do not let a prefix bypass identity checks. It should only resolve to a full
digest first. The normal proof code should then use the full digest.

### 6. Separate Human and Machine Payloads

For Hero JSON, decide which modes return full digest fields by default.

Recommended behavior:

```text
include_machine_payload=false:
    output job_id, target_id, fact_ref, digest_prefix, short summaries
    avoid large full digest arrays unless necessary

include_machine_payload=true:
    output full canonical digest fields
    output parent digest arrays
    output proof certificate details
```

Implemented in `machine_payload_boundary.v1`:

```text
Marshal fact/lineage panels:
    default lineage_panel keeps row counts, selected relations, and
    non-authority flags; raw relation/key/digest rows stay in machine_payload
    default preview_panel keeps counts and preview_fact_refs; concrete facts,
    identity_envelope payloads, parent digest arrays, and preview lineage rows
    stay in machine_payload

Marshal artifact summaries:
    default target panels and run-level target blockers keep
    artifact_fact_preview_refs and artifact_fact_preview_digest_count
    artifact_fact_preview_digests are emitted only in explicit machine payload

Machine payload guarantee:
    include_machine_payload=true still relays raw Lattice results for facts and
    target evidence, plus full target_statuses with artifact preview digests in
    operational reports
```

Existing proof/evaluate output may remain machine-heavy. If we change defaults,
do it carefully and update `.man` files and tests.

### 7. Normalize Assistant/Docs Language

Docs and assistant summaries should prefer:

```text
policy_training fact: ptf_2a8eaae0c20d
job: policy_training_ppo_v0_lineage_closure_1780882558
target: policy_training_artifact_ready
```

Only show full digests when:

```text
debugging exact mismatch
copying a machine command
recording a proof certificate
writing canonical facts
writing a regression test that asserts exact identity
```

### 8. Audit Generated Ids Built From Digests

Some Runtime ids and paths use digest prefixes:

```text
policy_training_ppo_v0_<contract_digest_prefix>
attempt_<contract_digest_prefix>_<timestamp>_<pid>
component_spawn_fingerprint=<contract_digest>
```

Inspect whether these should be:

```text
stable ids
operator refs
canonical fingerprints
path-safe shortened digests
```

Do not blindly change path layout. If path layout changes, define migration or
accept that old runtime jobs remain historical.

### 9. Clean Up "hash" Naming

Search for public fields that say `hash` but really mean digest.

Example current shape to inspect:

```text
checkpoint_artifact_digest
checkpoint_digest_reported
checkpoint_digest_verified
```

If `hash` is public/operator-visible and not intentionally low-level, rename to
`digest` in the same cleanup pass.

### 10. Update Docs And Man Pages

Update at least:

```text
src/config/man/hero.lattice.man
src/config/man/hero.marshal.man
src/config/man/hero.runtime.man
src/main/hero/README.md
src/include/hero/lattice_hero/lattice/README.md
src/include/hero/runtime_hero/runtime/README.md
ROADMAP.md
```

Mention:

```text
full digests are proof identity
short refs are operator convenience
prefixes are accepted only after unique resolution
target_id/job_id are preferred for normal operations
```

## Acceptance Criteria

This cleanup is accepted when:

```text
1. Canonical facts and proof certificates still write full digests.

2. Lattice and Marshal fact inspection share the same public parameter names:
   fact_digest, fact_digest_prefix, fact_index.

3. If aliases remain, outputs still use canonical names.

4. Short refs appear in preview/evaluate/inspect summaries where useful.

5. Short refs and digest prefixes fail closed on ambiguity.

6. target_id and job_id remain the preferred normal operator keys.

7. Lattice proof internals still resolve full digests before proving lineage.

8. No proof path accepts a short ref as authority.

9. Docs clearly separate id, digest, fingerprint, hash, ref, and prefix.

10. Tests cover:
    - exact digest lookup
    - unique prefix lookup
    - ambiguous prefix rejection
    - not-found prefix rejection
    - Marshal/Lattice schema parity
    - machine payload still exposes full digests
    - human summary uses short refs
```

## Focused Test Targets

Likely relevant tests:

```sh
make -C src/main/hero -j12 build-lattice-hero
make -C src/main/hero -j12 build-marshal-hero
make -C src/main/hero -j12 build-runtime-hero
make -C src/tests/bench/kikijyeba/lattice_exposure -j12 run-test_kikijyeba_lattice_exposure
make -C src/tests/bench/kikijyeba/lattice_target -j12 run-test_kikijyeba_lattice_target
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_runtime_wave_preview
```

Existing Marshal coverage for `hero.marshal.inspect subject=facts` lives in
`test_kikijyeba_marshal_dispatch.cpp`.

Always run:

```sh
git diff --check
```

## Risks

### Accidental Proof Weakening

The main risk is allowing a short prefix to become proof identity. Avoid this by
resolving prefixes to full digests before any proof or lineage check.

### Ambiguous Historical Runtime Roots

Large runtime roots may contain many facts. Short prefixes that were unique in a
small test may become ambiguous in real operator roots. Resolver errors must be
clear and non-mutating.

### Schema Churn

Renaming public fields can break tools. Decide explicitly whether this cleanup
allows no backward compatibility. If not, keep aliases only at parser boundary
and emit canonical names.

### Path Layout

Runtime paths use digest prefixes. Do not change path layout in the same patch
unless that is explicitly scoped.

## Out Of Scope

Do not change:

```text
cryptographic/content digest algorithm
canonical fact digest computation
Lattice certificate digest computation
policy-training lineage semantics
PPO behavior
Runtime execution behavior
live/paper authority flags
```

Do not convert canonical SHA-like strings to base36 unless there is a specific
storage or transport reason. Base36 shortens full digests only modestly and
adds conversion complexity. Prefix refs solve the human problem better.

## Suggested Next Session Prompt

```text
Implement digest_hash_id_surface_cleanup.v1 from CLEANUP_DIGEST.md.

The Lattice/Marshal fact-preview selector slice is already done. Continue with a
deeper audit of remaining public/runtime-facing hash, digest, fingerprint, id,
and ref fields. Keep full digests canonical in facts and proofs. Prefer short
refs only as display/query helpers with unique-prefix resolution. The Runtime
terminal-fact and handoff JSON hash naming slice is now done with compatibility
fallbacks. Continue with remaining cross-module candidates such as
contract_hash, target_coords_hash, normalization_hash, and any similarly named
operator-visible fields. Update docs/man pages and focused tests with each
renamed surface.
```
