# Matched nonlinear sufficiency corrected-control stage-receipt fix preregistration

Status: fixed on 2026-08-14 before any successor runner execution, build,
self-test, source access, raw capture, evaluator invocation, fit, optimizer
step, metric, or scientific report.

    protocol_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1
    runtime_root=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1
    diagnostic_authority=development_only
    benchmark_acceptance_authority=false

This is a new one-shot protocol. It neither resumes nor retries the consumed
predecessor attempt. Its sole behavioral correction is the evaluation order
of local declarations and assignments in emit_stage. Every scientific and
data-free self-test setting is inherited unchanged.

## Immutable predecessor authority

The exact predecessor runner is:

    path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_corrected_control_self_test_report_v1.sh
    sha256=9e6328a7385972abe14aea57def6307a6a0a982632fc3097d6bcc94df8677458

The complete predecessor contract is:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_PREREGISTRATION.md
    sha256=d22232ea9f3e71dce58c1c5a11beca32c028f6eb5961b5c2e62b7b949c2951f9

The immediate predecessor terminal remains final and byte-for-byte untouched:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/terminal.invalid.status
    sha256=71cc4730d172541d50fab4ae9d46bc0b55138070994fc873b89acdecb8d01ccf
    status=terminal_invalid
    classification=invalid_post_attempt_execution
    failure_stage=post_attempt_setup
    failure_command_ordinal=0
    attempt_consumed=true
    same_protocol_resume_allowed=false
    same_protocol_retry_allowed=false
    new_protocol_required=true

The exact predecessor attempt and worker diagnostic are:

    attempt_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/attempt.status
    attempt_sha256=b59308962ab31837e2002f7122db8f2a5d9309e784caa1b2f119656cd5ab97df

    worker_log_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/logs/worker.log
    worker_log_sha256=9ec0ea0d3728056a8dde8049b77819a372b1514d7b67320ff359886054ab6823

The stage-receipt root-cause document is
MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_STAGE_RECEIPT_FIX_ROOT_CAUSE.md.
The successor runner must bind that document's final exact SHA-256, this
preregistration's final exact SHA-256, the predecessor runner, preregistration,
terminal, attempt, and worker log before any build or self-test.

## Frozen zero-science failure boundary

The predecessor terminal fixes:

    actual_raw_capture_invocations_started=0
    actual_raw_capture_invocations_completed=0
    actual_raw_capture_invocations_validated=0
    evaluator_invocations_started=0
    evaluator_invocations_completed=0
    evaluator_invocations_validated=0
    evaluator_report_attested=0
    fits_completed=0
    optimizer_steps=0
    maximum_anchor_read_upper_bound=none
    scientific_result_available=false
    representation_execution=false
    mdn_execution=false
    checkpoint_written=false
    certified_input_access=false
    final_holdout_access=false
    policy_access=false

These are hard lineage facts, not values that the successor may reinterpret.
No source or protected artifact was opened by the failed post-attempt setup.

## Sole permitted implementation delta

The frozen predecessor's emit_stage prefix is:

    local name="$1" state="$2" candidate="${SCRATCH}/${name}.${state}.$$"
    shift 2

Under set -u, name and state are expanded for candidate before the local
builtin initializes them. The successor must replace only that prefix with:

    local name state candidate
    [[ $# -ge 2 ]] || fail "emit_stage requires name and state"
    name="$1"
    state="$2"
    shift 2
    candidate="${SCRATCH}/${name}.${state}.$$"

The destination expression remains exactly:

    ${STAGES}/${name}.${state}.status

The stage payload, schema, state and stage fields, caller-supplied fields,
scratch containment, exclusive publication, fsync/sealing behavior, validation,
and call order remain unchanged. Beyond necessary successor protocol, schema,
runtime-root, document, and lineage bindings, every other runner byte must be
derived from the predecessor without behavioral change.

No alternative shell workaround, retry loop, resume path, broad refactor,
stage bypass, receipt weakening, or scientific modification is authorized.

## Frozen capture, evaluator, and self-test

The successor capture source and compile-only wrapper remain byte-identical:

    capture_source_path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.cpp
    capture_source_sha256=b489548b7a8fec72c7933f359b694e5852282e721108453b6e338e3ec73b2c62
    capture_build_script_path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.sh
    capture_build_script_sha256=5c168802d7618c9c144b41480e8b406a2c2b883c314ab7d3a08ee33cbaadd2b8

The scientific evaluator source and wrapper remain byte-identical:

    nonlinear_source_sha256=caddf0a96d13e9c425671a7067e48720f483de5ab40b933b5caf12b76ba99ef5
    nonlinear_build_script_sha256=73dbce2a5c7566f2dc24884bb0cc579ebdf66204d043bc434098b0ac4fb27816
    affine_source_sha256=45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939

The evaluator report schema remains exactly:

    evaluator_report_schema=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1

That value is the unchanged evaluator payload schema. It must remain separate
from the successor protocol_id and must not be renamed.

The data-free self-test remains exactly the predecessor contract. The capture
is invoked only as:

    --self-test --self-test-report ABS

The exact ordered eight-record report, two exact ANSI stdout lifecycle records
with decimal-thread-token-only normalization, normalized stdout evidence,
empty stderr, exclusive report creation, validation, sealing, hashes, sizes,
and immutable receipt bindings remain unchanged. No self-test case, expected
hash, path rule, output byte, or access exclusion may change.

The predecessor attempt binds the successfully established implementation
closure:

    capture_build_receipt_sha256=dd190ae567c17889c78746223b377a739cb8cfc66d140134b1962bf8ba894cb4
    nonlinear_build_receipt_sha256=9d40af56b88d3119e16faad1091edf42ba56fe29b43ff9582f2b7b804420454e
    self_test_receipt_sha256=a37e20e974d5e0ff44c43c4c2dee5ec44f82b22bd22fecebeb5896b688afe099

Those predecessor receipts are immutable lineage evidence, not successor
receipts. The successor uses its new private runtime root, rebuilds the frozen
sources with the frozen wrappers, reruns the unchanged data-free self-test
before attempt publication, and binds its own sealed binaries and receipts.

## Frozen scientific contract

Every scientific clause of
MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_PREREGISTRATION.md
at SHA-256
d22232ea9f3e71dce58c1c5a11beca32c028f6eb5961b5c2e62b7b949c2951f9
is incorporated unchanged. This summary is restrictive and cannot replace,
relax, or reinterpret that document.

The sealed representation inputs remain:

    train_range=[0,2496)
    train_probe_sha256=d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75
    train_rows=22464

    validation_range=[2560,2816)
    validation_probe_sha256=8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
    validation_rows=2304

The representation record schema and 96-value
base[0:32],quote[32:64],base_minus_quote[64:96] layout remain unchanged. The
frozen representation checkpoint identity remains
70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d,
but the representation is not constructed or executed.

Only the physically isolated development source may supply the corrected raw
control. The runner performs exactly two raw captures: train [0,2496) and
validation [2560,2816). Source order is sequential and maximum anchor read is
2815. The corrected capacity-versus-mask rule, actual-mask stream hashes,
min/max summaries, zero fill, right alignment, raw96 serialization, row key,
coordinate identity, target identity, and independent target check remain
unchanged.

The target remains:

    future[base_node,c,0,3] - future[quote_node,c,0,3]

There are exactly two arms, raw_history_96 and representation_raw96. Train-only
input standardization retains floor 1e-8. Train-only target standardization per
(edge_index,channel_index) retains floor 1e-8.

The identical decoder remains:

    input[96]
      -> Linear(96,128) -> GELU
      -> Linear(128,128) -> GELU
      -> Linear(128,9)
      -> select head channel_index * 3 + edge_index

The runner starts exactly one bounded evaluator invocation and exactly six CPU
float32 deterministic fits: two arms times seeds 31, 47, and 73. Each fit keeps
the byte-identical paired initialization and train-row sequence, unweighted
standardized-target MSE, Adam learning rate 1e-3, betas 0.9 and 0.999, epsilon
1e-8, zero weight decay, batch size 64, gradient-norm clipping at 5.0, and
exactly 3,500 optimizer steps. Successful completion therefore records six
fits and 21,000 optimizer steps. There is no search, retry, seed selection,
refit, early stopping, or checkpoint write.

Every metric, finite check, aggregation, median, per-channel result, pairwise
ordering, strong gate, partial-evidence threshold, and mechanical four-way
classification remains exactly as preregistered. No observed value may change
execution or authorize another fit.

## One-shot lifecycle and failure policy

The successor performs frozen preflight, builds, and self-test before
publishing its own attempt ordinal 1. It then starts one bounded worker.
Post-attempt lifecycle settings remain:

    planned_raw_capture_invocations=2
    planned_evaluator_invocations=1
    planned_fits=6
    soft_timeout_seconds=5350
    term_grace_seconds=10
    hard_timeout_seconds=5400
    retry_allowed=false
    resume_allowed=false

The stage sequence, command ordinals, progress accounting, trap behavior,
terminal receipt construction, manifest, development result receipt,
scientific report validation, and final sealing remain unchanged except for
successor-specific identities and the sole emit_stage declaration fix.

A preflight or prepare failure before the self-test fail-closed trap or the
scientific attempt exists is a non-attempt, non-terminal staged failure. The
same is true of an early self-test or run-development preflight failure before
its fail-closed trap is installed and before attempt publication. Such a
failure may be corrected only within the explicitly permitted staged
prepare/self-test lifecycle; it does not authorize science, a fit, or an
attempt.

Once the self-test fail-closed trap is installed, any self-test execution,
validation, or sealing failure terminalizes this protocol with
attempt_consumed=false. After the scientific attempt receipt exists, any
nonzero exit, timeout, signal, missing or invalid stage receipt,
capture/evaluator/fit mismatch, artifact mutation, validation failure, or
sealing failure terminalizes it with attempt_consumed=true. Once either a
terminal receipt or an attempt receipt exists, same-protocol retry and resume
remain forbidden. A terminal receipt fixes:

    same_protocol_resume_allowed=false
    same_protocol_retry_allowed=false
    new_protocol_required=true

There is exactly one attempt. Predecessor builds, self-tests, captures, or
attempts cannot be replayed as successor events.

## Protected exclusions

Canonical data/raw, certified development [2880,3264), final holdout
[3328,4096), policy data, policy artifacts, and every representation, MLP,
MDN, or policy checkpoint remain forbidden. No representation, MLP,
optimizer, MDN, or policy checkpoint may be written. No certified/final
access, representation retraining, production MDN work, policy work, serving
change, or deployment is authorized.

The successor runner must bind the final exact hashes of both documents in
this stage-receipt-fix pair before any execution. Unexpected pre-existing
successor state outside the permitted staged prepare/self-test lifecycle --
specifically an existing attempt, result, terminal receipt, or unreceipted
artifact -- is forbidden when requesting a fresh attempt. A hash mismatch,
path-containment failure, non-private directory, symlink, or hard-link anomaly
is likewise fail-closed. The exact private runtime root and lock, sealed build
receipts and binaries, and sealed self-test artifacts created and verified by
the prescribed prepare/self-test stages are permitted successor state.
Nothing permits mutation of a predecessor artifact.
