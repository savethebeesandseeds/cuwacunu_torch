# Ujcamei Source Contract

Source DSL specs and parser helpers live here. The current compatibility model
can still hold channel and graph rows because lower storage and validation code
have not fully split their input shape yet, but those rows are graph-first
Kikijyeba dock settings, not Ujcamei source identity.

The fresh namespace is `cuwacunu::ujcamei::source::contract`, with parser and
decoder helpers under `cuwacunu::ujcamei::source::contract::dsl`.

Header roles:

- `contract.h` is the stable source contract data model:
  `source_form_t`, `source_universe_t`, source analytics policy fields, and the
  explicitly compatibility-only merged `source_spec_t`.
- `runtime/decode.h` is the config/decode/runtime surface. New Ujcamei callers
  should request `source_universe_t`; `source_spec_t` decoding remains for
  compatibility with graph-first validation seams.
- `dsl/` contains parser support and split-DSL decoders. It is not the public
  source contract model.

Source rows carry `source_kind` provenance (`real`, `synthetic`, or `derived`);
graph/NodeLift validation uses this to prove real reverse-edge pairs.
Graph-first dock rows declare active channel sampling and directed instrument
edges. Kikijyeba composition resolves those dock rows against the Ujcamei
source universe before constructing dataloaders.

The active fresh config path is `src/config/.config`, which points at matching
dotted BNF/DSL filenames in `src/config`.
