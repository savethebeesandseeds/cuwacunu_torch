# Kikijyeba Protocol

This room owns the current graph-first protocol API.

It compiles source, topology, expression, representation, inference, and
training specs into one validated protocol contract, then builds the runtime
objects needed by launchers. It does not own NodeLift math, representation
workers, inference heads, training loops, or Ujcamei source storage. Generic
topology primitives live in `topology/graph`; wave runtime settings live in
`settings`.

- `config_bundle.h` loads and cross-validates Ujcamei, Kikijyeba, Wikimyei,
  and Jkimyei specs from `.config` into
  `graph_first_protocol_contract_t`. The older
  `graph_first_config_bundle_t` name is retained as a compatibility alias.
- `source_dock.h` separates Ujcamei source/channel availability from protocol
  topology. Ujcamei sources and channel rows define retrievable source evidence;
  active graph rows are decoded by `kikijyeba/topology/graph` and resolved
  here with source availability. It also emits a source-resolution report that
  compares active nodes, selected edges, and available real source edges before
  any dataloader is materialized.
- `pipeline_builder.h` turns that bundle into graph-first runtime objects and
  dry-run reports, including graph-resolution counts such as available directed
  edges, missing ordered pairs, reverse pairs, connected components, and cycle
  dimension.
- `component_stream.h` defines the small value-based stream contract used by
  graph-first components: component family/id, assembly token, graph
  fingerprint, wave settings, requested graph-wide source range, Ujcamei cursor
  token, and optional runtime `.lls`.

Jkimyei launchers consume this protocol layer for training orchestration.

Contract/wave split:

- A protocol contract defines the runnable system: source universe, active
  source dock, topology graph, Wikimyei assemblies, dock binding, and
  Jkimyei specs.
- A wave runs against that contract: it chooses runtime mode and graph-wide
  Ujcamei cursor range.
- A job records one execution of a wave against a contract.
