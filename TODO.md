# Architecture TODO

This file tracks source-level architecture work. The `/doc` thread owns the
notation and conceptual write-up; this file should stay short enough to be used
as an implementation compass.

## Current Position

We have paused source architecture work until the node/edge document is stable.
The current source state is prepared for the larger representation/inference
model, but the iitepi remake should not start until the doc gives us the target
shape.

The intended direction remains:

- Train one shared representation wikimyei over a compatible source domain.
- Train inference wikimyei, such as MDN expected-value heads, per active
  instrument or base/quote edge when needed.
- Keep source/runtime compatibility explicit through instrument signatures.
- Avoid legacy compatibility paths and silent fallback defaults during refactors.

## Done Enough For Now

- `INSTRUMENT_SIGNATURE` is the contract compatibility vocabulary.
- Waves declare exact `RUNTIME_INSTRUMENT_SIGNATURE` selections.
- Runtime validates wave signatures against every selected contract component.
- Source rows carry exact instrument facts:
  `(SYMBOL, RECORD_TYPE, MARKET_TYPE, VENUE, BASE_ASSET, QUOTE_ASSET)`.
- Source/dataloader selection is signature-first, not symbol-only.
- `INSTRUMENT_SCOPE` was removed.
- Component DSLs carry a unique `TAG`.
- Normal Hashimyei selection is contract/wave-derived; campaigns no longer
  mount component revisions directly.
- `ANY` signature fields are compatibility wildcards and are omitted from the
  component storage identity hash.
- Runtime wikimyei report fragments now repeat target-verification evidence:
  component `TAG`, component compatibility hash, docking signature hash,
  accepted instrument signature, and exact runtime instrument signature.
- A first strict `default.lattice.target.dsl` exists and decodes into target
  obligations.

## Parked Until The Doc Settles

Do not start the large iitepi/circuit/tsiemene refactor yet.

When the node/edge doc is finished, resume here:

### 1. Decide The New Runtime Shape

Use the doc to decide how source code should represent:

- asset nodes,
- traded instrument edges,
- representation wikimyei domains,
- inference wikimyei per edge/base-quote pair,
- wave or target attachment in the new model.

### 2. Lattice Target Verification

Teach Lattice to evaluate `.lattice.target.dsl` obligations against persisted
Hashimyei/Lattice evidence:

- match component `TAG`, compatibility hash, docking hash, and source signature,
- report each obligation as missing, stale, insufficient, or compliant,
- surface the exact report fragments used as evidence,
- keep active instruments authored in target files, not inferred from `.data`.

Only implement this before the iitepi remake if it still matches the doc's
node/edge target language.

### 3. Finish Evidence Semantics

Review whether target compliance needs more persisted evidence:

- source schema or feature-shape identity,
- representation geometry,
- representation checkpoint linkage for inference heads,
- training/loss semantics needed for load safety.

## Large Pending Reframe

### Redo Iitepi Around Nodes And Edges

The current iitepi/circuit/tsiemene model is likely not the final execution
abstraction. We are moving toward a graph model:

- nodes are assets or latent asset states,
- directed edges are traded instruments,
- representation wikimyei attach to compatible source domains,
- inference wikimyei attach to active instrument edges or base/quote pairs,
- waves and component attachment will be redesigned around this node/edge split.

This is intentionally large and should wait until the doc thread finishes the
conceptual map. For now, keep peripheral systems ready: signatures, derived
component identity, source facts, and lattice evidence.

## Suggested Order

1. Finish and review the node/edge document.
2. Translate the doc into a source-level iitepi remake plan.
3. Decide whether Lattice target verification should use current wikimyei
   evidence language or the new node/edge language.
4. Fill any remaining evidence semantics needed by that verifier.
5. Replace circuit/tsiemene semantics with the new node/edge runtime model.
