# Ujcamei Source Contract

The source contract folder now owns the compatibility data model and runtime
decode surface. The current compatibility model can still hold retrieval-channel
and graph rows because lower storage and validation code have not fully split
their input shape yet. Retrieval-channel rows are Ujcamei retrieval policy.
Graph topology rows are typed and decoded under Kikijyeba protocol topology, not
Ujcamei source identity.

The fresh compatibility namespace is `cuwacunu::ujcamei::source::contract`.
Source registry decoding lives under
`cuwacunu::ujcamei::source::registry`. Retrieval channel decoding lives under
`cuwacunu::ujcamei::source::retrieval`.

Header roles:

- `contract.h` is the stable source contract data model:
  `source_form_t`, `source_universe_t`, source analytics policy fields, and the
  explicitly compatibility-only merged `source_spec_t`.
- `runtime/decode.h` is the config/decode/runtime surface. New Ujcamei callers
  should request `source_universe_t`; `source_spec_t` decoding remains for
  compatibility with graph-first validation seams.
- shared parser support lives in `ujcamei/source/contract/syntax`; source
  registry, retrieval-channel, and graph-topology decoders live in their owning
  folders.

Source rows carry `source_kind` provenance (`real`, `synthetic`, or `derived`);
graph/NodeLift validation uses this to prove real reverse-edge pairs. Ujcamei
retrieval-channel rows declare active temporal sampling and window lengths.
Current runtime execution supports only active `kline` retrieval channels;
non-kline source rows can remain authored in the source registry, but active
non-kline retrieval channels are rejected at decode time. Kikijyeba graph
topology rows declare active nodes and directed instrument edges. Kikijyeba
protocol resolves topology against the Ujcamei source and channel universe
before constructing dataloaders.

The active fresh config path is `src/config/.config`, which points at matching
dotted BNF/DSL filenames in `src/config`.
