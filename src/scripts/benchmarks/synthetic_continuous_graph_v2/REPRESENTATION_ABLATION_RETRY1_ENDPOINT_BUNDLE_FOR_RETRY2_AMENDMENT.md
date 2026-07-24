# Representation ablation retry1 endpoint-evidence bundle amendment

Status: fixed after publication of the retry1 operational-interruption
closure. This amendment authorizes only an immutable, external copy of the
completed retry1 `endpoint_scale` evidence subtree. It changes no data, model,
augmentation, objective, seed, optimizer, range, metric, threshold,
comparator, tie rule, selection rule, or certification rule.

## Authority and scope

The source runtime is
`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry1`.
It remains immutable interruption evidence. It must not be edited, chmodded,
touched, resumed, re-entered, linked into a later experiment, or consumed
directly.

The prerequisite external interruption closure is
`synthetic_v2_representation_ablation_isolated_v2_retry1_interruption_closure_v1`.
Before inspecting the endpoint source, the endpoint-bundle verifier must run
that closure's frozen verifier in `--verify` mode and authenticate:

- interruption receipt SHA-256
  `e6c845233f3f434a9c46bead1b9fb825217492a5da7ae0a95174fc10b15e1117`;
- regular-file inventory SHA-256
  `c7cce9005bee5efaa5f85924624839afad57655b4bfdb3d0d4a774fd4bf60926`;
- directory inventory SHA-256
  `40c7b2cf3846f3c439c6ebfe8d86b2839c969996fe6ce211be3577e2d11613ee`;
- frozen closure verifier SHA-256
  `95478dbd60734116171c9bd2bc40890c8abd1ea59b0d60c1dccc4dcbdb1241a3`;
  and
- frozen interruption amendment SHA-256
  `e7d99698ee4b62254a274ebbf3d699b9e00ced0ff727279767e1d4976d594002`.

The endpoint-bundle verifier then acquires the retry1 `.development.lock`
through a read-only file descriptor, proves that the descriptor and canonical
path name the same device and inode, and holds and rechecks that identity
through two identical source snapshots and any publication.

## Exact endpoint evidence

The only copy source is the complete retained `arms/endpoint_scale` subtree.
Its exact allowlist contains twenty-one regular files and nine directories
when the subtree root `.` is counted, equivalently eight descendant
directories. Its total regular-file size is 32,731,999 bytes. Its canonical
content inventory SHA-256 is
`bd2f8d55b4e3e3a3a06bf14749b28ea0bec01ea9c07aaa1c1628e9ed4f59e13f`,
computed from the subtree root with
`find . -xdev -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum |
sha256sum`.

The exact tree includes the endpoint configuration, training status and log,
component-spawn evidence, complete Runtime job evidence, system registry, and
runtime layout. It includes no time-only or no-TF-alignment path. The endpoint
checkpoint file SHA-256 is
`09c286c5374e4769feb19644c3efa26aa081e37620f1eb5acf3bd9cf534b26ec`.
The Runtime's separately reported internal checkpoint digest is
`14398b461a259bc5`; these are distinct digest schemes and must never be
substituted for one another.

The verifier must authenticate exact hashes and sizes for every allowlisted
file and validate the cross-artifact completion semantics: job state, Runtime
result, representation report, job manifest, checkpoint and exposure facts,
source-analytics fact, component-update fact, health and checkpoint-I/O facts,
source ranges, optimizer counts, graph/protocol/config identities, component
spawn registry, runtime layout, and terminal event-stream evidence. The arm is
complete at 3,000 optimizer steps with seed 17 over `[0,2496)`, with finite
parameters, no input checkpoint, no forecast output, no policy execution, no
canonical `data/raw` access, no certified input, and no final-holdout access.

The terminal event stream contains eight exact artifact kind/path publication
records. Six of those records also contain exact digest/schema pairs, while
the delegated-report and checkpoint publications do not carry those fields.

## Copy and publication rule

The verifier publishes only the separate sibling runtime
`synthetic_v2_representation_ablation_isolated_v2_retry1_endpoint_bundle_for_retry2_v1`.
It copies each allowlisted source file with `cp --reflink=never` into
`source_snapshot`. It must verify source and destination hashes and bytes with
both `cmp` and SHA-256 before and after the copy, require source and destination
device/inode tuples to differ, and require link count one on both sides.

The bundle also contains the two exact source inventories, frozen copies of
this amendment and its verifier, and `endpoint_bundle.status`. Every bundle
file is mode 0444 and every bundle directory is mode 0555. Symbolic links,
special entries, unknown paths, cross-device source entries, hard links, and
overwrites are forbidden. Publication uses an atomic no-clobber
`mv -T -n` from a deterministic candidate below the benchmark runtime parent.
All scratch and candidate paths owned by the endpoint-bundle verifier are below
that runtime parent. It creates no endpoint payload, scratch file, or candidate
under `/tmp`.

The mandated prerequisite interruption-closure `--verify` invocation retains
its independently sealed policy: it may create only its small, ephemeral
inventory and receipt snapshots under `/tmp`, and it cleans them before
returning. Those prerequisite-verifier snapshots are not endpoint-bundle
payload or endpoint-bundle-owned scratch. No large payload is written under
`/tmp`.

## Retry2 boundary

This bundle is historical endpoint evidence only. Its publication does not
authorize direct checkpoint use, does not authorize consumption from the
bundle by retry2, and does not establish retry2 scientific or configuration
equivalence.

A separate retry2 stage verifier must independently:

1. authenticate this complete immutable bundle;
2. prove exact retry2 endpoint scientific and configuration equivalence;
3. make a second byte-identical, non-reflinked, non-hardlinked retry2-local
   copy with distinct source/destination device/inode identity;
4. bind that decision before either fresh challenger arm starts; and
5. restrict retry2 consumption to the retry2-local copy.

Without that separate verifier and receipt, the endpoint arm must restart from
optimizer step zero. Retry1 time-only and no-TF-alignment artifacts remain
ineligible for import or reuse under every path. Certified inputs, the final
holdout, canonical raw data, forecasting, and policy artifacts remain outside
this amendment.
