# MDN retry-1 seal concurrency erratum

Status: fixed after the completed MDN retry runtime was sealed on 2026-07-21,
but before any downstream clean frozen-feature capture, affine route, ablation
selection, certified-development result, or consumer was authorized to use its
`result.status`. This is an operational provenance erratum. It changes no
training byte, model byte, source byte, split, objective, metric, seed,
threshold, result value, or scientific interpretation.

## Incident

The sole MDN retry completed normally. Its post-execution repair was governed
by `MDN_RETRY1_COMPLETION_INVENTORY_CORRECTION.md`. The first completion-sealer
candidate that received static approval had SHA-256
`3ccefd6952c0e2d1e283334f95d42cb8ee47f92412e635a152bd8631c1054c1c`.
The parent launched that candidate in `--seal` mode. The process table observed
the launch at `STIME=14:15` UTC with minute precision.

While that already-running shell was hashing the 4.2 GB payload, the sealer
source file received one final crash-window hardening edit. The edit moved
`chmod 0555 -- "${runtime_root}"` immediately before, rather than immediately
after, the post-publication `verify_exact_receipt` call. The final on-disk
sealer has SHA-256
`88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0`.
No other byte differs between the two sealer sources.

Bash had parsed the publisher function before the edit. The running process
therefore retained the `3cce...` publication order in memory, while its
receipt emitter dynamically hashed the live script path after the edit and
recorded `88b...`. Terminating the supervising tool cell did not stop the
already-started `docker exec`; it continued and completed the seal.

The published result is:

```text
path=.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1/result.status
schema=synthetic_v2_mdn_train_isolated_v2_retry1.result.v1
sha256=d9eeddb89be7f2313083f4ea375bbf8c7f4168c95d15c4dbc216eadd009c1d93
bytes=13863
mode=0444
links=1
embedded_completion_sealer_sha256=88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0
```

Its embedded sealer SHA identifies the disk source read during receipt
emission, not the function body already loaded by the initial publisher. The
receipt is therefore not sufficient provenance on its own even though its
payload bindings are correct.

The logical path of the initial publisher and the live final sealer are the
same repository path:
`src/scripts/benchmarks/synthetic_continuous_graph_v2/seal_and_verify_mdn_retry1_completed_job.sh`.
That path now contains only the final `88b...` bytes. The initial `3cce...`
identity is historical/logical; it must never be represented as a current
path/hash pair. The layered closure instead creates an immutable, explicitly
labeled reconstruction. It swaps the single three-line publication sequence
in the live `88b...` source, requires exactly one match, and requires the
resulting reconstructed bytes to hash exactly to `3cce...`. The reconstruction
is evidence only and is never executable authority.

## Bound scientific payload

The completion-inventory correction is SHA-256
`ab4ceb9a7d1e6d55c6830b3263abfbfe60225399283ef63673176c11d4ebc5d9`.
The pre-receipt 23-file content inventory master remains
`43b36342443cd37d78d2639618ea87f8c8c6f43dce24e0ef4a73cdc9a6399f50`.
The completed MDN checkpoint is SHA-256
`a0a01cf4074aaf96526dfa387677dadfe4a27086eab68063dd13969e5660ab4f`.
The execution runner remains SHA-256
`93c477a6e4de3ddfbded94ca8f22db14cd7954800a319713c0a7961ccf2bb799`.

After the initial publisher exited, the retry runtime contained exactly the
original 23 payload files plus `result.status`; all 24 regular files were mode
`0444`, all seven directories were mode `0555`, all regular files had link
count one, the deterministic external staging name was absent, and no entry
was writable. The publication-order delta did not alter any payload or receipt
byte. It only makes the initial publisher identity in the receipt incomplete.

The initial `3cce...` order still sealed all payload files and subdirectories
before receipt publication. Its additional receipt verification occurred in
the short interval after the external candidate name was unlinked and before
the already-planned root `0555` chmod. The final tree is closed, but the
`88b...` implementation must be genuinely adopted once, idempotently, under
the layered closure so the authoritative executor and receipt-emission source
are the same frozen bytes.

## Frozen forensic timeline

The following values are observations made before the authoritative `88b...`
adoption. They are preserved even though idempotent chmod calls may later
change ctimes. Birth times are unavailable on this mounted filesystem.

```text
final_sealer_path=src/scripts/benchmarks/synthetic_continuous_graph_v2/seal_and_verify_mdn_retry1_completed_job.sh
final_sealer_sha256=88b215f1e598907b209d9eeae45623696cd92767915bf343c5f792a8ecf655a0
final_sealer_inode=130885864170461293
final_sealer_device=66
final_sealer_mode=0755
final_sealer_links=1
final_sealer_bytes=75702
final_sealer_mtime=2026-07-21 14:17:23.739143400 +0000
final_sealer_ctime=2026-07-21 14:17:23.739143400 +0000

result_inode=79094468456190506
result_device=66
result_mode=0444
result_links=1
result_bytes=13863
result_mtime=2026-07-21 14:18:19.686483500 +0000
result_ctime=2026-07-21 14:19:34.648242700 +0000

runtime_root_inode=9851624185335778
runtime_root_device=66
runtime_root_mode=0555
runtime_root_mtime=2026-07-21 14:19:34.643710800 +0000
runtime_root_ctime=2026-07-21 14:19:34.693483500 +0000

checkpoint_inode=78250043525577835
checkpoint_device=66
checkpoint_mode=0444
checkpoint_links=1
checkpoint_bytes=3228076
checkpoint_mtime=2026-07-21 12:32:40.727424400 +0000
checkpoint_ctime=2026-07-21 14:18:19.786856900 +0000
```

At `2026-07-21T14:30:04.623927172Z`, a self-excluding procfs scan found no
process whose command line contained the completion-sealer name or
`cuwacunu_exec`. This is external operational evidence, not a Runtime process
exit record. The launch identity and tool-cell termination are likewise
external orchestration observations. They are not model evidence.

## Layered closure and one authoritative adoption

`seal_and_verify_mdn_retry1_completion_concurrency_closure.sh` is the sole
layered closure wrapper. Its closure runtime is the separate sibling:

```text
.runtime/benchmarks/synthetic_continuous_graph_v2/
  synthetic_v2_mdn_train_isolated_v2_retry1_completion_concurrency_closure_v1/
```

It must never edit, replace, unlink, or regenerate any sealed retry content
byte, path, or inode. The final sealer's idempotent `chmod` calls are the sole
authorized adoption mutation: they preserve those identities and modes but
may advance ctimes. Before adoption, the live final sealer and closure wrapper
must both be executable and non-writable at mode `0555`; this erratum and the
reconstructed publisher snapshot must be mode `0444`. The wrapper captures
its own SHA-256 once at process start, never substitutes a later live-path
hash, and binds that fixed identity in every artifact and the closure receipt.
It must recheck the same start SHA-256 plus both source files' modes, link
counts, and inodes immediately before and after every child invocation and
before publication. Its own closure lock is separate; it must not hold the
retry execution lock while invoking the final sealer.

Before the child call, the wrapper atomically freezes:

1. the exact incident observations above plus the then-current frozen source
   metadata in `pre_adoption_forensic.status`;
2. the never-executable reconstructed `3cce...` source snapshot at
   `reconstructed_initial_publisher.3cce.sh` inside the closure runtime;
3. a C-locale, relative-path-ordered content identity inventory covering all
   24 sealed retry files with SHA-256, byte size, mode, and link count; and
4. an immutable `attempt.started` marker identifying exactly one adoption
   attempt.

Once `attempt.started` exists, absence of a valid `success.status` fails
closed. The wrapper must not silently repeat an ambiguous child invocation.
It captures the final `88b...` sealer's idempotent `--seal` output, then one
explicit `--verify` output. It freezes a post-adoption content inventory and
requires it to be byte-identical to the pre-adoption inventory. Ctimes are
intentionally excluded from content identity because idempotent chmod may
change them; the original forensic ctimes remain in this document and the
pre-adoption snapshot.

Only after successful adoption, verification, source-identity rechecks, and
pre/post content equality may the wrapper publish
`completion_concurrency.status` with schema
`synthetic_v2_mdn_train_isolated_v2_retry1.completion_concurrency_closure.v1`.
The receipt is staged outside the closure runtime, made read-only, and
published with an atomic no-clobber same-filesystem hard link. All closure
files become mode `0444`, and the closure directory becomes mode `0555`.
Crash residues are recovered only when their exact bytes and inode relation
are proven; unknown or mismatching residues fail closed.

Every downstream consumer must invoke this wrapper with `--verify` and bind
the layered closure receipt. The wrapper's verification calls the final
`88b...` sealer's `--verify`, rechecks the current 24-file content identity,
and separately requires the exact seven-directory tree at mode `0555`. The
closure runtime itself is exactly one directory and ten mode-`0444` regular
files, including `.closure.lock`, with no symbolic links, special files,
writable entries, or external hard links. The MDN `result.status`, old
execution runner, initial `3cce...` publisher
identity, or reconstructed snapshot is never sufficient consumer authority.

Canonical `data/raw`, V2 final evidence, policy execution, additional
training, checkpoint resume, and independent-final claims remain forbidden.
