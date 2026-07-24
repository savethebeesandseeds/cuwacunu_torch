# Development-source isolation amendment

Status: fixed on 2026-07-16 after the raw-data and raw-history isolation
reports, after the first canonical representation training run, and while its
first production MDN run was still training on train core. The running MDN
had reached optimizer step 2,450 and train-only cumulative metrics had been
inspected. No frozen-feature train or validation capture, representation
affine result, canonical representation validation forecast, conditional
route, representation ablation, or certified-development feature row existed.

## Discovered exposure defect

The fresh-seed preregistration requires development diagnostics to reject the
canonical `data/raw` tree and use the physically truncated
`data/development_prefix` view. Audit of the completed representation job and
running MDN job found that their production config inherited the preparation
registry whose `SOURCE_ROOT` is `data/raw`. Their manifests consequently bind
canonical raw source paths and report 4,096 accepted anchors, even though the
explicit training cursor selected only `[0,2496)`.

The source-range restriction prevented final anchors from entering the
optimizer or a reported forecast metric. It did not satisfy the physical
source-isolation contract: Runtime constructed the full 4,096-anchor source
domain from files containing `[3328,4096)`. The first representation
checkpoint and its associated production MDN run are therefore quarantined.
They may be retained as implementation diagnostics but cannot supply a probe,
route decision, certified confirmation, or final claim.

V2's final interval is conservatively treated as operationally exposed. No
subsequent V2 result can be described as independent final-holdout evidence.
This does not prevent a clean development-only localization on the already
declared train, validation, and certified-development ranges.

## Clean development source

Before another model or probe run, a new preparation runner must atomically
construct a runtime-local source mirror from only the nine sealed
`data/development_prefix` CSV files. It must:

1. reject symlinks, non-regular inputs, canonical `data/raw` paths, and every
   source not under the sealed development prefix;
2. verify each source byte hash against the existing immutable fresh-seed
   closure before and after copying;
3. create a local registry whose only `SOURCE_ROOT` is the runtime mirror;
4. build raw and causal log-return caches only beside that mirror, before the
   mirror becomes read-only;
5. require strict CSV/raw-cache/normalized-cache freshness for all nine
   chains, then seal and hash the registry, CSVs, caches, configs, executable,
   cache guard, and build manifests in a new immutable closure receipt; and
6. prove through a zero-step production dry run that the authoritative source
   domain contains exactly 3,264 accepted anchors, has maximum anchor 3,263,
   and every source receipt names the runtime mirror and not `data/raw`.

The full sealed development prefix is the fixed source. No additional
train/validation-only truncation is introduced. Production `log_returns`
cache normalization is causal per record (current versus preceding valid
record), so later prefix rows do not alter earlier normalized records.

## Mandatory reruns and unchanged choices

The canonical representation and production MDN must be rerun from optimizer
step zero under new schema identifiers using the isolated-source closure.
Their architecture, policies, seed, optimizer, step counts, train range, and
all preregistered diagnostic choices remain unchanged. The invalid runs may
not be resumed, relabeled, or used as checkpoints. A byte-identical clean
checkpoint, if observed, is an execution-equivalence diagnostic rather than a
waiver of the rerun.

The staged frozen-feature and affine procedure begins only from the clean
representation and MDN receipts. It captures train and validation first and
uses the raw-96 validation gate as the already fixed conditional route. If
the ablation route is selected, every challenger also uses the same isolated
source closure. Any capture manifest containing `data/raw`, an accepted
anchor count other than 3,264, or an anchor beyond its authorized exact range
fails closed.

This amendment changes no data values, model, feature arm, augmentation,
seed, optimizer, step count, ridge, metric, threshold, split, tie rule, or
interpretation ladder. It corrects source transport and withdraws independent
final-holdout status from V2. A genuinely independent final evaluation now
requires a future fresh dataset and a new one-shot ledger.
