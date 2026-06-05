# Kikijyeba

Kikijyeba means brain.

This pillar is for protocol, topology, and environment/world contracts.

It is not Ujcamei: it does not own input/source.

It is not Wikimyei: it does not perform worker/model labor directly.

Hero Runtime, Hero Lattice, and Hero Marshal own execution, proof/inspection,
and coordination surfaces. They may read Kikijyeba contracts, but they do not
live inside Kikijyeba.

Current migrated rooms:

- `kikijyeba.h`, umbrella include for the current protocol/topology surface.
- `environment`, the historical replay operating-world contract. V1 keeps
  episodes, observations, target-weight actions, simulated execution, rewards,
  and replay evidence in Kikijyeba while policy math remains in Wikimyei.
  Replay bundles are the spawn unit: one accepted Ujcamei/component-stream
  cursor range becomes one independent replay-world instance. Built-in baseline
  policy adapters and the deterministic Wikimyei allocation adapter emit the
  same target-weight action shape, so they can be compared under the same
  world/reward/report grammar. Runtime can write best-effort replay artifacts
  for completed MDN run jobs, and `cuwacunu_exec --replay-from-job-dir
  <job_dir>` can run post-job replay experiments without replacing normal
  Runtime execution. Replay reports name
  `direct_edge_realized_return_truth_v1` when realized asset/base returns come
  from direct graph-edge close log returns. Include
  `kikijyeba/environment/environment.h` directly; it is intentionally not
  pulled through the top-level umbrella because it depends on Torch-backed
  portfolio and observer types.
- `protocol`, current graph-first protocol API. It compiles authored config
  files into a protocol contract, resolves source docking, and builds runtime
  objects.
- `topology`, protocol-owned active-world topology. It includes graph topology,
  graph-order identity, Wikimyei registry checks, and dock bindings between
  active component assemblies.

Hero-owned rooms now live under `src/include/hero`:

- `hero/runtime_hero`, Runtime Hero public entry headers; executable wave/job
  contracts live under `hero/runtime_hero/runtime`.
- `hero/lattice_hero`, Lattice Hero public entry headers; read-only proof,
  exposure, split, and report contracts live under `hero/lattice_hero/lattice`.
- `hero/marshal_hero`, Marshal Hero public entry headers; bounded coordination,
  handoff, rollout, and inspection contracts live under
  `hero/marshal_hero/marshal`.
- `hero/config_hero`, Config Hero public entry headers; config store contracts
  live under `hero/config_hero/config`.

Current C++ namespaces:

- `cuwacunu::kikijyeba::protocol`
- `cuwacunu::kikijyeba::environment`
- `cuwacunu::kikijyeba::topology`
- `cuwacunu::kikijyeba::topology::graph`

Canonical runtime vocabulary:

- Protocol contract: the compiled, validated artifact that defines what can
  run. The current graph-first contract is built from Ujcamei source/retrieval
  config, Kikijyeba topology, Wikimyei assemblies and networks, dock bindings,
  and Jkimyei training/inference specs.
- Dock binding: the compatibility binding between component dock variables such
  as `B`, `N`, `L`, `C`, `Hx`, `Hf`, `F`, `De`, `Df`, and `K`.
