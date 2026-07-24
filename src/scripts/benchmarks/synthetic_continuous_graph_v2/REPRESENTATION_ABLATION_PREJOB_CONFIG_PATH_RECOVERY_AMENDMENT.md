# Representation ablation pre-job config-path recovery amendment

Status: fixed on 2026-07-22 after the first challenger launch failed, but
before Runtime created a challenger job and before any challenger checkpoint,
probe, selection, certified artifact, or scientific result existed. This is an
operational recovery amendment. It changes no data, model, objective, seed,
optimizer, range, metric, threshold, or selection rule.

## Failed attempt authority

The retained failed runtime is
`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2`.
It is historical evidence and must not be edited, sealed in place, resumed,
copied as a retry configuration source, or used as scientific evidence.

The failure closure binds these identities:

- frozen runner SHA-256
  `af1824c7386f0c15a2ce1a0afb2f3b2a4d0fe2858e7b39cb10edd2ef52f3adda`;
- endpoint training log SHA-256
  `4be9a577f7e07dc1cc20aa37f932b83a2f7f83cd52e08a123a5419681b607dc9`;
- complete regular-file inventory digest
  `a467215c0fd46eba9a5a6957f161396adae30b4b06995ad803606b73826b830d`,
  computed from C-locale-sorted `sha256sum` lines for every regular file,
  using `./`-relative names from the failed runtime root;
- Runtime executable SHA-256
  `9f09e6ec8fa22177def737672a229a13de64954b5de928881480dbd8ff506aff`;
- grammar-derivation source SHA-256
  `d1bc0c64eaff3b404e7d6cc537ec131a7c445827f64f7d2501c62edfe9b8a2c4`;
- graph-first config-bundle source SHA-256
  `7f7fae337ec3132258b14e731bca240605b61c05527365c6700b2ff77d277feb`;
- job-runner source SHA-256
  `aaecc932ea72a98f24c791b94d0782e2f6805fa8e4a4e3a93cd5538375508120`;
- MTF launcher source SHA-256
  `5c1ed715c5926be0ceb2b4553006138145ba6a138641509d32c098d0428a4502`;
  and
- canonical MTF net grammar SHA-256
  `26f1d105ec04945024ac91806fc4206e21d81429c3a190782b7159af69d2e0a3`.

## Observed failure boundary

The endpoint log contains exactly one graph-first open failure for:

```text
/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2/arms/endpoint_scale/config/grammar/representation.net.bnf
```

The endpoint `training/` directory has zero descendants. The exact retained
tree has twelve directories and twenty-three regular files. It has no symbolic
links, special entries, unexpected hard links, `job.manifest`,
`runtime.result.fact`, checkpoint, probe, challenger training status,
selection, development completion, result, or certified artifact. These are
observed absence facts. They do not assert an unrecorded process exit status or
process-liveness history.

The pinned loader sources explain the boundary. An omitted `foo_bnf_path` is
derived as `<resolved foo_path parent>/grammar/<foo filename>.bnf`. Each arm
renamed its absolute local net to `config/representation.net` without adding
`wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path`. Runtime therefore
derived the missing path recorded in the log. Wave settings were parsed first;
the full graph-first bundle did not finish loading. Runtime job creation and
MTF model and optimizer construction occur after that load. Consequently,
`job_created=false`, `optimizer_constructed=false`, and `optimizer_steps=0`
are source-backed boundary inferences, not fields emitted by a Runtime result.

## External immutable closure

Before a retry, the one-shot verifier
`seal_and_verify_representation_ablation_prejob_failure_closure_v1.sh` must
publish `failure_closure.status` under the separate sibling runtime
`synthetic_v2_representation_ablation_isolated_v2_prejob_failure_closure_v1`.
It must hold the old development lock read-only, verify the exact allowlisted
tree, bytes, modes, sizes, link counts, devices, inodes, directory identities,
failure token, empty training directory, and forbidden absences, and then
publish deterministic file and directory inventories plus its receipt using an
atomic no-clobber directory rename.

The verifier may write only to its separate closure runtime. It must not chmod,
touch, rename, link into, or publish a receipt inside the failed runtime. An
existing closure is accepted only when its exact tree, frozen sources,
inventories, receipt, and the unchanged failed runtime all verify.

## Retry rule

One restart-from-zero recovery is authorized only under the distinct schema
`synthetic_v2_representation_ablation_isolated_v2_retry1` and its sibling
runtime. It must regenerate every arm train and capture config with paths into
that new runtime; old arm configs contain absolute references to the failed
runtime and may not be reused.

Every generated train and capture config must explicitly bind:

```text
wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path = /cuwacunu/src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf
```

The retry must bind this amendment, the failure-closure verifier and receipt,
the canonical grammar identity, and a statically resolved complete effective
grammar closure before training. That closure must be immutable before any
Runtime invocation; Runtime `--dry-run` is not the config-path gate. The failed
endpoint launch is not a challenger result and must not enter selection or
certification.
