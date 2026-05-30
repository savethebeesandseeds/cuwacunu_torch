# math_compat Specification

`math_compat` provides incremental statistics primitives used for normalization workflows.

Namespace: `cuwacunu`
Public include form: `#include "piaabo/math_compat/statistics_space.h"`

## Core Contract

This module exposes two statistics engines plus a field-pack adapter:

1. `statistics_space_t`
- unbounded (infinite-window) running statistics
- Welford-style incremental update for mean/variance stability

2. `statistics_space_n_t`
- fixed-size rolling window statistics
- tracks window values, min/max, mean, variance, stddev

3. `statistics_pack_t<T>`
- applies rolling normalization across multiple fields of a user type `T`
- uses accessor lambdas (`FieldAccessor<T>`) to read/write field values

## `statistics_space_t` Contract

- initialized with first value (`count == 1` after construction)
- `update(x)` updates mean/variance/min/max incrementally
- variance is sample variance (`n-1` denominator)
- `normalize(x)` returns z-score; returns `0.0` when stddev is zero

## `statistics_space_n_t` Contract

- `update(x)` appends value and evicts oldest when window exceeds `window_size`
- `ready()` is true only after at least `window_size` updates
- `count()` returns total processed samples (`ctx`), not current window length
- min/max are derived from maintained ordered multiset
- variance is sample variance over current window (`n-1` denominator)
- `normalize(x)` returns z-score; returns `0.0` when stddev is zero

## `statistics_pack_t<T>` Contract

Required shape for `T`:

- must be copyable for `normalize` output
- must expose `is_valid()` used as update/normalize gate

Behavior:

- `update(data_point)` updates each tracked field only when `data_point.is_valid()` is true
- `normalize(data_point)` returns normalized copy when valid; otherwise returns unchanged copy
- `ready()` is true only when all field statistics are ready

## Assumptions and Limits

- time spacing is assumed uniform (no delta-time weighting)
- no robust/outlier-resistant statistics are implemented here
- no exception-based error model in normal operations

## Legacy Ghosts

- Source warnings explicitly note missing delta-time-aware statistics support.
- Current statistics are designed for equally spaced samples and can mislead on irregularly sampled data.
- Planned indicator extensions (for example RSI/MACD) are referenced in source warnings but not implemented.
- Additional feature-space statistics/correlation expansions are noted as pending.

This spec defines behavior-level contracts; source remains authoritative for numeric implementation details.
