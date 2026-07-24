# MDN retry-1 completion-inventory correction

Status: fixed after the sole authorized clean MDN retry completed 3,500
optimizer steps on 2026-07-21, but before its result receipt was published,
before its runtime was sealed, and before any downstream clean frozen-feature
capture, affine route, ablation selection, or certified-development result was
allowed to consume it. This is an operational inventory correction. It changes
no scientific choice, model byte, data byte, split, objective, metric, seed,
threshold, or route.

## Failure classification

The execution runner was
`src/scripts/benchmarks/synthetic_continuous_graph_v2/run_mdn_train_isolated_v2_retry1.sh`
at SHA-256
`93c477a6e4de3ddfbded94ca8f22db14cd7954800a319713c0a7961ccf2bb799`.
Its immutable `inputs.status` is SHA-256
`d1ee004e82f10352c988be425677e1fb3a98ebd7e7ef7cae603b3570857408f4`.
The runner launched a new step-zero retry under the already authorized clean
source, representation checkpoint, objective, seed 31, and train range
`[0,2496)`. Runtime completed 3,500 of 3,500 optimizer steps, wrote its final
checkpoint, report, terminal result fact, probes, and standard completion
facts, and returned success to the runner.

The runner then failed closed in its own post-execution `assert_exact_job_tree`
check. That check allowed nine legitimate outputs but omitted exactly these
seven standard Runtime outputs:

```text
job.state
lattice.checkpoint.fact
lattice.exposure.fact
lattice.source_analytics.fact
runtime.checkpoint_io.fact
runtime.component_training_update.fact
runtime.health_measurement.fact
```

Those seven files use the same Runtime schemas and completion roles already
present in the completed clean representation job. They are not unexpected
training products.

There is also a second, latent validator defect after that allowlist check. The
MDN report, `runtime.result.fact`, and `runtime.health_measurement.fact` each
encode the exact component-specific token `finite_parameter_check=1`; the old
runner incorrectly expects the string `true`. The independently emitted
`lattice.exposure.fact` uses its own exact token
`finite_parameter_check=true`. No token may be normalized or rewritten. Had
the allowlist check been corrected alone, the numeric/boolean encoding
mismatch would have caused the next fail-closed validation error.

No `result.status` was written and the runner stopped before its chmod sealing
phase. The existing job and log are therefore complete but writable
operational artifacts. This correction authorizes only validation, receipt
publication, and permission sealing of their existing bytes.

No training retry, re-execution, resume, checkpoint reload, metric recompute,
artifact rewrite, or in-place repair is authorized. Any byte or inventory drift
from the values below fails closed.

## Frozen pre-seal inventory

The retry runtime is
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1`.
Before `result.status` is added, its exact top-level entries are
`.execution.lock`, `components/`, `inputs.status`, `job/`,
`mdn_train.retry1.log`, `system/`, and
`synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config`.

The exact 16-file `job/` inventory, byte sizes, and SHA-256 values are:

```text
07b53bb36b240794a0845091d2559153a482e354102da3f8b3cc9bb7e74833db  14452       channel_inference.report
a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f  3228076     channel_inference.report.channel_mdn.pt
a7a3a1768cccd45271fdede4a80d611f9fea7bacd2331068e51f1ef75800e78f  3189        channel_inference.report.mdn.lls
bde33bb12ba2cd4d36ad12235da11169540d36026349cbb2ec57dd489f5bb3d8  2200        channel_inference.report.nodelift.lls
f24d73413bb2afb2e4a4403ec6f47d84e6c2935f5cb0b4a88fa06792a75a81ec  2125        channel_inference.report.representation.lls
e82caf605e2d554de93345ca4f9eef802be7216297a0d5f299bf2f57fdc3db27  7589        job.manifest
cb0ac2ad692f89ed763bffdbd2149f1f024f1f25325d9d44d4714b463ddbc484  4636        job.state
6f4bea543f338e7df93be90393c0fc9e3e1c0a7cc2651d83f47f698867d7d24b  3523        lattice.checkpoint.fact
8721e577e053f840920d29d922d9b0d08d90f28e68b253fd990dc41622f7b16d  9109        lattice.exposure.fact
6fb027764b12edf00d947b7c00ba7cc218e4477163b3570756fcc7e2a3fdeda9  3806        lattice.source_analytics.fact
c584f07973eb3d453039086a5a6eea1f073bcb0b7d2b9aaea76b6a7459ed432a  4129261000  representation_edge_features.probe
1cd37c868c32c7ab99e187eacdc2c9e4dffc4b77469717b9a3179f1a34def5df  1816        runtime.checkpoint_io.fact
c06d30a100f0eed3871b4a588d350b4e2065da3b2373318be95a0644be57d7bd  1162        runtime.component_training_update.fact
ed9eb3ad53fcb37b5974eac9a468f2ab214da9af5cd1e9fbf59952f0baa78958  2243        runtime.health_measurement.fact
cf0e89ddf0ea4e2207ed311e6af93a33364a8e9bf250126d23403b8e34afc53b  68622341    runtime.job_events.probe
15f815b4a10a783bba92de43b9c01892e4fddb942f0dce242a995c02a5e102dc  5930        runtime.result.fact
```

The exact non-job file hashes and byte sizes are:

```text
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  0      .execution.lock
d1ee004e82f10352c988be425677e1fb3a98ebd7e7ef7cae603b3570857408f4  5978   inputs.status
a42cfd073fc9e914bb5f6b73392267ecb9d256afb42c654e11266c049183511e  5307   synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config
4f74f73f1c153a8457ecb009a45f174425ccdabd6bd671c15779436f4af75479  12045  mdn_train.retry1.log
```

The exact component inventory contains one file:

```text
b882808f6942c01d8b49829b529b738cb359ad0d1ac27fa1243c1c189fb907d0  405  components/wikimyei.inference.expected_value.mdn/spawns/rpu_5ba58d2de0fb7dcb/component_spawn.ref
```

The exact system inventory contains two files:

```text
0415c82a0c4278022de2dba7c9a4ddb9b2fffe5e0d94ff566ca8e2da47d0d10d  705  system/component_spawn_registry.v1.lls
fb2198270b44590399e7ae2d7ee95190ca5a3b88d3495943afac3149b5f7e1bc  357  system/runtime_layout.v1.lls
```

Across the exact 23 pre-receipt files there are 4,201,197,994 bytes. The
SHA-256 of the canonical manifest whose lines are
`SHA256 SP SIZE SP RELATIVE_PATH LF`, ordered by relative path under the C
locale, is
`43b36342443cd37d78d2639618ea87f8c8c6f43dce24e0ef4a73cdc9a6399f50`.

All listed regular files have link count one. The pre-seal hierarchy contains
no symbolic link or special entry.

## Completion semantics

The completion sealer must validate, without changing content:

1. the exact execution-runner path and SHA and every exact-key field and hash
   in its immutable input receipt;
2. the clean isolated-source closure and cursor erratum, the completed clean
   representation result and checkpoint, the MDN objective, the original
   interrupted-attempt closure, and the interruption recovery amendment;
3. the derived configuration, exact source receipts, `[0,2496)` range, 3,261
   accepted/candidate anchors, zero source skips or duplicates, execution
   chain, stream plan, objective vector, empty MDN input checkpoint, and clean
   representation input checkpoint;
4. the manifest, report, checkpoint, terminal result, job state, all seven
   standard completion facts, all LLS files, both probes, component spawn
   metadata, and system inventories;
5. `status=completed`, 3,500 attempted/completed optimizer steps, zero skipped
   batches, the exact numeric token `finite_parameter_check=1` in the MDN
   report, terminal result, and health fact, the exact boolean token
   `finite_parameter_check=true` in the lattice exposure fact, finite reported
   loss and gradient-norm values, zero non-finite outputs, and successful final
   report/checkpoint writes; and
6. exact cross-file paths, identities, digests, generation lineage, source
   domain, and component identifiers.

`lattice.exposure.fact` records `gradients_finite=false` while also recording
finite numeric last and maximum pre-clip gradient norms. That value is bound
exactly and is not reinterpreted as evidence of NaN or infinity. Completion is
established by the exact component-specific terminal Runtime guards above,
finite loss/output measurements, and `nonfinite_output_count=0`.

Publication-time checkpoint metadata also remains exact. The checkpoint-I/O
fact leaves its artifact digest, status, byte-count, and digest-verification
fields blank even though it records `checkpoint_written=true` and
`checkpoint_write_count=70`. The lattice exposure fact records
`checkpoint_digest_verified=false` and `checkpoint_bytes=0`. These defaults
are not promoted or rewritten. The external frozen checkpoint SHA-256, report
`last_checkpoint_optimizer_step=3500`, completed job state, and terminal
result/checkpoint paths are the completion guards. Unavailable lattice/source
diagnostics may contain their emitted `nan` sentinel; the MDN report, health
fact, and terminal result numeric outputs contain no NaN or infinity.

The performance values in `runtime.result.fact` are retained observations.
This correction does not promote them to independent-final evidence and makes
no claim about whether they are good.

## Corrected receipt and sealing protocol

`seal_and_verify_mdn_retry1_completed_job.sh` is the sole completion sealer.
It publishes the originally preregistered result schema
`synthetic_v2_mdn_train_isolated_v2_retry1.result.v1` at `result.status` and
must include distinct execution-runner and completion-sealer path/SHA fields,
this correction's path/SHA, all frozen inventories, and the complete scientific
and recovery provenance chain. Its receipt keyset is exact.

The receipt candidate must be staged in the runtime parent, outside the
writable retry runtime, validated there, and made read-only. Before receipt
publication, the sealer makes every existing payload file mode `0444` and all
payload subdirectories mode `0555`, while retaining write permission only on
the retry runtime root. It then rehashes and revalidates the sealed payload.
Only after that check may it publish `result.status` with an atomic
same-filesystem hard link that fails if the destination exists. It removes the
external link and immediately makes the runtime root mode `0555`.

An existing receipt is accepted only if it is byte-identical to the newly
derived candidate and independently verifies. It is never overwritten. Such a
receipt may only drive completion of a pending permission-sealing pass over
unchanged, hash-matching bytes. This ordering permits idempotent recovery from
an interrupted chmod or publication pass without placing an authoritative
receipt over writable payload evidence. Final verification rejects any unknown
entry, symbolic link, special entry, external hard link, writable file, or
writable directory.

Every downstream consumer must invoke the completion sealer with `--verify`;
checking or hashing `result.status` alone is insufficient.

This correction cannot authorize a second retry. Canonical `data/raw`, V2
final evidence, policy execution, and independent-final claims remain
forbidden.
