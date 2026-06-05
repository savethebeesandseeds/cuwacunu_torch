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

`wave_settings.h` decodes `kikijyeba.settings.wave.dsl`. It names the current
wave, selects the Wikimyei family to run with `TARGET`, declares mode flags such
as `run | debug`, and declares the graph-wide source range and Ujcamei cursor
family used to ground runtime `.lls` reports. A wave range is larger than a
batch: the stream generator yields graph-anchor batches until the requested
range is exhausted. `SOURCE_RANGE=source_key` is the stable authoring form for
key-valued sources; it resolves to the accepted graph-anchor index domain before
execution while preserving requested source-key bounds in runtime evidence.

`TARGET` is the focal component, not the whole execution chain. For example,
`TARGET=wikimyei.inference.expected_value.mdn` still runs NodeLift and a frozen
VICReg encoder because MDN depends on node encodings. `MODE=train` mutates only
the target; `MODE=run` applies no optimizer step. `debug` is only a report
modifier and may be combined with either primary mode. Wave settings are not
topology and are not training policy. Topology lives in `kikijyeba/topology`;
training policy lives in `.jkimyei` files consumed by Jkimyei.

V1 deliberately does not implement resume. Resume requests fail fast after
configuration entry and before pipeline materialization.

`hero.runtime.run operation=wave` accepts a Marshal `runtime_handoff` object for
concrete operator handoffs. The object binds schema/id/digest, target id, base
config, concrete wave fields, concrete checkpoint inputs, Runtime policy
identity, and dry-run/execute intent. Runtime rejects non-empty
unresolved-symbol lists and symbolic model-state selectors such as
`latest_satisfying:*` before launching `cuwacunu_exec`. When a handoff is
accepted, Runtime passes the handoff id and digest into the job runner so
`job.manifest`, `runtime.result.fact`, and the derived lattice exposure sidecar
can echo the same identity.

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
lattice.exposure.fact
lattice.source_analytics.fact
lattice.checkpoint.fact     # when a checkpoint exists
```

These facts normalize the end-of-job result, checkpoint I/O, and model-health
measurements from `job.manifest`, `job.state`, and the component report. They
are Runtime evidence, not Lattice proof authority; Lattice still proves by
reading Runtime artifacts, and Marshal uses these facts as the preferred
operator-report surface with raw reports as fallback.
The Runtime-emitted `lattice.source_analytics.fact` is deliberately narrow:
it binds source cursor, graph order, split/window, parent exposure digest,
anchor acceptance fraction, missingness fraction, duplicate-anchor count, and a
source-health level. If source data analytics `.lls` reports are already
job-local, Runtime also copies their entropy, information-density,
compression-ratio, and power-spectrum visibility metrics into the fact. It is
visibility evidence only and carries no readiness, coverage, leakage, or
contract-identity authority.

For channel MDN jobs, terminal facts also carry compact performance visibility
fields when the component report provides them: per-channel NLL,
per-target-feature NLL, per-channel/target-feature support counts, sigma health,
mixture entropy/usage, finite-parameter state, nonfinite-output count,
checkpoint I/O, and whether the run mutated model state. These are measurements
for reports and warnings; they are not performance gates by themselves.
For non-mutating channel MDN `MODE=run` jobs, Runtime also writes
target-transform, forecast-baseline, and forecast-eval Lattice facts. The
forecast-eval fact copies the report's stratified NLL and valid-target support
surfaces as inspection evidence only.

For non-dry-run channel MDN `MODE=run` jobs, Runtime also installs a
best-effort Kikijyeba replay artifact observer by default. The observer receives
each streamed MDN pulse, builds replay forecast records through
`wikimyei.observer.belief`, derives the matching graph-anchor future
realization batch from the representation stream, and writes pulse-local
artifacts under:

```text
artifacts/kikijyeba.environment.replay.v1/pulses/
```

The top-level `runtime_replay_batches.index` records every pulse packet. CLI
options may disable this sidecar writer or provide explicit replay base-reserve
and risky-node lists when the graph cannot infer a unique reserve node. Replay
artifact setup/write failures are recorded in `job.state` as
`replay_artifact_error`; they do not fail the MDN job. These artifacts are
replay evidence for `kikijyeba.environment.replay.v1`; they do not change MDN
inference outputs, optimizer steps, checkpoint behavior, or Lattice proof
authority. The environment read-side companion can consume a
completed Runtime job directory, load every indexed pulse packet, and expose the
packets as replay bundles for historical replay experiments. The Kikijyeba
environment driver can then run enabled replay policies against those bundles
and write a replay experiment report plus
`runtime_replay_experiments.index`. The report includes top-level
observation/action/execution time-law counters, future-observation and
realization-key violation counts, and projection-validation step counts derived
from the per-step replay evidence. This remains a post-job replay adapter and
does not change Runtime's MDN execution semantics. The executable surface is
`cuwacunu_exec --replay-from-job-dir <job_dir>`, which consumes an already
completed job directory instead of launching a new graph-first wave. Runtime
Hero exposes the same path as
`hero.runtime.run operation=replay requested_mode=plan|dry_run|execute`. The
tool accepts `job_id` or `job_dir`, checks the job is completed and has replay
batch evidence, then delegates to the executable with the requested replay
policy options and Cajtucu paper profile hints such as
`allow_synthetic_direct_edges` and `linear_transaction_cost_rate`. When a caller
sets `validation_rollout=true`, Runtime treats replay as validation-grade: it
requires positive `max_steps`, positive `max_parallel_jobs`, nonzero
`linear_transaction_cost_rate`, `execution_profile_digest`,
`policy_set_digest`, and rejects synthetic direct execution edges at the request
boundary. Execute mode also gates the produced replay report: validation replay
fails closed if the report is missing, has no completed episodes, lacks profile
or policy-set identity, contains `cajtucu_synthetic_market_step_count > 0`, or
contains `cajtucu_invalid_trace_count > 0`. The matching read path is named too:
`hero.runtime.inspect subject=artifact` can inspect `replay_batch_index`,
`replay_experiment_index`, and `replay_experiment_report`, and
`hero.runtime.inspect subject=job` summarizes those replay artifacts beside the
regular Runtime manifest/state/report summaries.

Policy-training is now represented as a Runtime contract, not as a trainer.
`hero.runtime.run operation=policy_training requested_mode=plan|dry_run`
validates a bounded trainable-policy handoff and returns a deterministic
`kikijyeba.runtime.policy_training_job_contract.v1` packet with a contract
digest. The contract binds policy identity, architecture/training-config
digests, disjoint train/validation/test range digests, normalization and replay
buffer ranges, environment/observation/action/reward/execution-profile digests,
causal walk-forward schedule mode/schema/digest, selection-policy digests,
parent replay-environment evidence, and finite episode/step/wall-clock bounds.
Runtime rejects contracts where normalization or replay-buffer inputs are not
training-bound, where train/validation/test ranges overlap by digest, where the
causal schedule does not bind a typed cursor-key ordering and ledger-derived
`no_future_snapshot_use` source, where target-label/reward/trajectory
availability extends beyond artifact `usable_from_key`, where
`offline_full_window_research` is presented as readiness evidence, or where live
execution is requested.
`requested_mode=execute` is explicitly refused in this checkpoint; PPO or any
future trainable policy needs a dedicated Runtime runner before checkpoints can
be mutated.

The readiness-grade schedule is `causal_walk_forward_training.v1`: each rollout
block must use a snapshot family whose artifacts declare `usable_from_key <=
block_cursor_begin`. Artifacts fitted on a block may be updated after that block
closes, but they cannot generate observations, beliefs, actions, rewards, or
policy-training trajectories inside that same block. The proof source is
`derived_from_artifact_fit_use_ledgers`; caller assertions are not enough.
Schedules must declare an explicit cursor-key ordering and reject opaque keys
without an ordering map. Target-label, reward, and trajectory availability must
be no later than the artifact `usable_from_key`. Full-window training is
quarantined as `offline_full_window_research`; it remains useful for diagnostics
but cannot satisfy policy-training readiness.
