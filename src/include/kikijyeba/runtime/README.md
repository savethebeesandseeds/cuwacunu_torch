# Kikijyeba Runtime

This folder contains the executable job layer.

Runtime does not implement source loading, component math, or optimizer loops.
It runs a wave against a compiled protocol contract. The runner writes a job
manifest, resolves the graph-anchor wave plan, delegates execution to the
Jkimyei representation or inference launchers, writes Runtime-owned terminal
facts, and writes a mutable job state.

The manifest records both sides of the launch:

- protocol contract identity: source/topology/assembly/dock/training
  compatibility fingerprint, excluding runtime model-state inputs and
  model-state admission flags,
- wave identity: target component family, mode, and resolved graph-wide Ujcamei
  cursor range.

Runtime artifacts are organized under one disposable root:

```text
/cuwacunu/.runtime/
  cuwacunu_exec/
    system/
      runtime_layout.v1.lls
      component_spawn_registry.v1.lls
    components/
      <component_family>/
        spawns/
          <component_spawn_id>_<component_spawn_fingerprint>/
            component_spawn.ref
            jobs/
              <wave_action>/
                <job_id>/
                  job.manifest
                  job.state
                  runtime.result.fact
                  runtime.checkpoint_io.fact
                  runtime.health_measurement.fact
    indexes/
      lattice_runtime_index.v1.lls
    cache/
```

`cuwacunu_exec` is the canonical execution root. Runtime Hero's `dev_nuke`
may dry-run or clear either this execution root or the whole disposable
`/cuwacunu/.runtime` tree when an operator policy enables it. Checked-in
policies keep actual reset disabled and keep backup snapshots off by default,
so cleanup does not create backup clutter under the runtime tree.

The folder path is for legibility and retrieval; it is not proof authority.
The authoritative identity remains in `job.manifest`, Runtime terminal facts,
checkpoint facts, and Lattice proof certificates. The spawn path segment keeps
the short `component_spawn_id` and the full component-spawn fingerprint visible
so repeated runs of the same configured component instance naturally group
together.

The component-spawn fingerprint uses the component family, active protocol
contract, graph order, and component assembly. It deliberately excludes the
wave id, action, mode, source cursor token, and requested/completed source
range; those are job/evidence/checkpoint lineage fields so train and eval waves
can operate on the same component spawn while Lattice still audits source
coverage and leakage.

Runtime roots that contain `system/runtime_layout.v1.lls` are treated as
component-layout roots. Runtime, Lattice, and Marshal job discovery scan
`components/` for those roots and do not accept old flat child job folders as
part of the execution root. Unmarked temporary fixture roots may still be
scanned recursively by tests, but the marked runtime layout is authoritative
for real execution evidence.

`job_runner_t` resolves the Jkimyei delegate from `TARGET` by default.
`wikimyei.representation.encoding.vicreg` runs the channel-preserving
representation launcher, while `wikimyei.inference.expected_value.mdn` runs
the channel-context MDN launcher. The latter runs VICReg as a frozen dependency
and mutates only Channel MDN in `MODE=train`. `MODE=run`
executes the target dependency closure without optimizer steps; `MODE=train`
mutates only the target component and runs upstream dependencies frozen.

V1 deliberately does not implement resume. Resume requests fail fast after
configuration entry and before pipeline materialization.

`hero.runtime.execute` accepts a Marshal `runtime_handoff` object for concrete
operator handoffs. The object binds schema/id/digest, target id, base config,
concrete wave fields, concrete checkpoint inputs, Runtime policy identity, and
dry-run/execute intent. Runtime rejects non-empty unresolved-symbol lists and
symbolic model-state selectors such as `latest_satisfying:*` before launching
`cuwacunu_exec`. When a handoff is accepted, Runtime passes the handoff id and
digest into the job runner so `job.manifest`, `runtime.result.fact`, and the
derived lattice exposure sidecar can echo the same identity.

Reusable wave profile files should keep protocol/target/mode/source-order intent
stable and avoid baked-in anchor indexes. Concrete launch ranges are supplied as
Runtime Hero `wave_overlay` fields or directly to `cuwacunu_exec` with
`--source-range`, `--anchor-index-begin/end`, or `--source-key-begin/end`.
Runtime applies the overlay in memory, validates the effective wave, and records
the resolved range in the manifest/state sidecars without editing the profile.

Every completed or failed job writes compact terminal sidecars:

```text
runtime.result.fact
runtime.checkpoint_io.fact
runtime.health_measurement.fact
```

These facts normalize the end-of-job result, checkpoint I/O, and model-health
measurements from `job.manifest`, `job.state`, and the component report. They
are Runtime evidence, not Lattice proof authority; Lattice still proves by
reading Runtime artifacts, and Marshal uses these facts as the preferred
operator-report surface with raw reports as fallback.

For channel MDN jobs, terminal facts also carry compact performance visibility
fields when the component report provides them: per-channel NLL,
per-target-feature NLL, per-channel/target-feature support counts, sigma health,
mixture entropy/usage, finite-parameter state, nonfinite-output count,
checkpoint I/O, and whether the run mutated model state. These are measurements
for reports and warnings; they are not performance gates by themselves.
