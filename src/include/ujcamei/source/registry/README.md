# Ujcamei Source Registry

The source registry declares what source evidence exists. Its authored config is
`src/config/ujcamei.source.registry.dsl`, with grammar
`src/config/grammar/ujcamei.source.registry.dsl.bnf`.

The registry is source identity and availability: instrument, interval,
record-type, market metadata, source provenance, and file path. It does not
select active channels, graph topology, or training windows.

Repeated kline families can be authored with `KLINE_SOURCE_SET`. The decoder
expands each declared interval into the same `source_form_t` rows as the legacy
table. `SOURCE_DEFAULTS.SOURCE_ROOT` supplies the common root unless a
`KLINE_SOURCE_SET` overrides it, and `SOURCE_DEFAULTS.KLINE_INTERVALS` supplies
the shared interval list unless a set declares its own `INTERVALS`, using:

`SOURCE_ROOT/INSTRUMENT/INTERVAL/INSTRUMENT-INTERVAL-all-years.csv`

The explicit table remains supported for non-kline rows or one-off paths.

Nested registry modules:

- `instrument_signature.h`: exact/ANY source identity matching.
- `types/`: source record schemas, intervals, and kline feature registry.
