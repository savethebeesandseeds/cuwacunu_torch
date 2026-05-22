# Kikijyeba

Kikijyeba means brain.

This pillar is for intent, coordination, objectives, and memory of what the
system is trying to become.

It is not Ujcamei: it does not own input/source.

It is not Wikimyei: it does not perform worker/model labor directly.

It should eventually help answer questions like:

- what active world are we working on,
- what workers are needed,
- what evidence is sufficient,
- what target or objective is being pursued,
- how source, expression, representation, inference, and training should be
  coordinated without reviving legacy orchestration dependencies.

Current migrated rooms:

- `kikijyeba.h`, umbrella include for the current runtime-planning surface.
- `protocol`, current graph-first protocol API. It compiles authored config
  files into a protocol contract, resolves source docking, and builds runtime
  objects.
- `runtime`, executable job layer. It runs a wave against a compiled protocol
  contract, producing a job manifest, job state, resolved wave plan, and a call
  into the existing Jkimyei representation or inference launcher.
- `settings`, current runtime wave settings. This is where run/debug mode,
  graph-wide source ranges, and Ujcamei cursor-reporting scope are declared for
  the active wave.
- `topology`, protocol-owned active-world topology. It includes graph topology,
  graph-order identity, Wikimyei registry checks, and dock bindings between
  active component assemblies.
- `lattice`, the symbolic lattice/memory language. Parser helpers and
  left-hand-side value declarations live directly in
  `cuwacunu::kikijyeba::lattice`; emitted runtime component reports live in
  `lattice/runtime_report`. Read-only lattice targets live in `lattice/target`
  and evaluate whether contract/component/source-cursor evidence is sufficient
  before suggesting the next wave.

Current C++ namespaces mirror those rooms:

- `cuwacunu::kikijyeba::protocol`
- `cuwacunu::kikijyeba::runtime`
- `cuwacunu::kikijyeba::settings`
- `cuwacunu::kikijyeba::topology`
- `cuwacunu::kikijyeba::topology::graph`
- `cuwacunu::kikijyeba::lattice`

Canonical runtime vocabulary:

- Protocol contract: the compiled, validated artifact that defines what can
  run. The current graph-first contract is built from Ujcamei source/retrieval
  config, Kikijyeba topology, Wikimyei assemblies and networks, dock bindings,
  and Jkimyei training/inference specs.
- Wave: one runtime instruction over a protocol contract. It chooses mode and
  graph-wide source cursor range. A wave does not redefine topology or worker
  assemblies.
- Lattice target: a desired state over contract-scoped evidence. V0 targets
  read manifests, state, reports, and checkpoints; they can recommend a next
  wave but cannot execute it.
- Job: one execution attempt of a wave against a protocol contract. Jobs write
  a manifest, mutable state, reports, and optional `.lls` sidecars.
- Dock binding: the compatibility binding between component dock variables
  such as `B`, `N`, `L`, `C`, `Hx`, `Hf`, `F`, `De`, `Df`, and `K`.
