# Ujcamei Source Registry

The source registry declares what source evidence exists. Its authored config is
`src/config/ujcamei.source.registry.dsl`, with grammar
`src/config/grammar/ujcamei.source.registry.dsl.bnf`.

The registry is source identity and availability: instrument, interval,
record-type, market metadata, source provenance, and file path. It does not
select active channels, graph topology, or training windows.

Nested registry modules:

- `instrument_signature.h`: exact/ANY source identity matching.
- `types/`: source record schemas, intervals, and kline feature registry.
