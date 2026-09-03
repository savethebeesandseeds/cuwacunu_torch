# Matched nonlinear sufficiency corrected-control self-test report preregistration

Status: fixed on 2026-08-14 before any successor binary build, self-test
execution, source access, corrected raw capture, evaluator invocation, fit,
metric, or scientific report.

    protocol_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1
    runtime_root=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1
    diagnostic_authority=development_only
    benchmark_acceptance_authority=false

This is a new one-shot protocol. It does not resume or retry either predecessor.
Its sole permitted change is transport of the already-passing, data-free
self-test report. All scientific inputs, source semantics, raw feature
construction, splits, evaluator code and schema, model settings, seeds,
optimizer schedule, metrics, gates, classifications, lifecycle bounds, and
protected exclusions are inherited unchanged from the corrected-control
preregistration.

## Immutable predecessor authority

The original raw-control predecessor remains terminal and byte-for-byte
untouched:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_development_v1/terminal.invalid.status
    sha256=a83154f71bee7eda29d3de8e221e55c03b4a42742cdb485a01ebb00ac1f41237
    classification=invalid_pre_fit_raw_control_capture_contract_failure
    scientific_result_available=false
    same_protocol_retry_allowed=false
    new_protocol_required=true

The corrected-control predecessor also remains terminal and byte-for-byte
untouched:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1/terminal.invalid.status
    sha256=ad63fa5dfbb4da59a8efaa32c5577dc2436ad2e795864b51158ef16a4890f0ca
    classification=invalid_data_free_self_test_failure
    failure_stage=self_test_validation
    attempt_consumed=false
    fits_completed=0
    optimizer_steps=0
    scientific_result_available=false
    same_protocol_retry_allowed=false
    new_protocol_required=true

The complete inherited scientific contract is:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_PREREGISTRATION.md
    sha256=54356d24de2cb11fedba9d7101b9ba97701d3c428b9d08f69ef9f208804a5719

The original raw-control root cause remains:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_V1_TERMINAL_ROOT_CAUSE.md
    sha256=7b3d8fb446c5585e24a0ecfdd9fee250f6781817c74d8d074a82411710dd6cf3

The self-test transport root-cause addendum is
MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_ROOT_CAUSE_ADDENDUM.md.
The successor runner must bind its final exact SHA-256, this preregistration's
final exact SHA-256, both predecessor terminals, and the two older documents
above before any build or self-test.

The corrected terminal additionally binds the failed artifacts that justify
this transport-only successor:

    failed_capture_source_sha256=f1483a0858c342b2477cc37e043bf5a894da369bd1e3ccb51ce04601710de2a8
    failed_capture_binary_sha256=e2e244eb7139f145f37763804e9c52c9d1850f125e78701c527618ed7ee1c042
    failed_self_test_output_sha256=efed4a76d7ca7d46c4d3ce8ae5e3cb4be79fabf64f1f08707f28b1e9543c69e5
    failed_self_test_log_sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

The failed output contained the exact eight passed-report records bracketed by
two known ANSI DEBUG lifecycle records. The failed log was empty. This is
transport evidence only and cannot authorize a scientific change.

## Frozen successor transport implementation

The successor capture source is:

    path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.cpp
    sha256=b489548b7a8fec72c7933f359b694e5852282e721108453b6e338e3ec73b2c62

Its compile-only build wrapper is:

    path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_raw_nodelift_edge_feature_probe_capture_corrected_control_self_test_report.sh
    sha256=5c168802d7618c9c144b41480e8b406a2c2b883c314ab7d3a08ee33cbaadd2b8

Both files must remain byte-identical. The runner must verify both hashes before
build, bind them into the immutable build receipt, and bind the resulting
binary identity before self-test. No substitute source, wrapper, or binary is
authorized.

## Sole permitted implementation delta

Only these capture and runner changes are authorized:

1. The capture binary accepts exactly:

       --self-test --self-test-report ABS

   in that order, with no additional argument. --self-test alone,
   --self-test-report outside this exact form, a relative path, an empty path,
   a lexically non-clean path, any symlinked path component, a missing or
   symlinked parent directory, or an existing leaf must fail.

2. The existing in-memory self-test cases and all their assertions remain
   byte-for-contract unchanged. After every case passes, the binary renders the
   eight fixed report records into memory. It then creates the absent report
   itself with:

       O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW
       mode=0600

   It writes all bytes with EINTR handling, fsyncs, closes, and commits only
   after successful I/O. Any exception or failed I/O before commit removes the
   uncommitted leaf. It never truncates, appends to, follows, replaces, or
   derives the report from a runner-created file.

3. The eight report records no longer go to stdout. No self-test assertion,
   canonical serialization, expected case, or observed canonical hash changes.
   Normal capture mode, its CLI, raw probe/report schemas, NodeLift-mask
   validation, raw96 construction, and exclusive-create publication remain
   unchanged.

4. The successor runner passes the exact report path, captures stdout and
   stderr separately, validates all three artifacts after process exit, and
   seals them read-only. It must not grep, filter, extract, or reserialize the
   report from stdout.

No project logger, source-runtime lifecycle, common archive behavior,
representation probe, isolated source, capture science, evaluator source, or
scientific contract may change under this protocol.

## Exact data-free self-test contract

The report path is fixed:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/self_test/self_test.report

The parent self_test directory must be newly created, private, canonical, and
non-symlinked. The report leaf must be absent when the binary starts.

The report is exactly these eight ASCII, LF-terminated records in this order,
with no BOM, carriage return, blank line, prefix, suffix, or extra byte:

    schema_id=synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_self_test_v1
    status=passed
    expected_case_count=8
    expected_cases=false_structural_padding,oldest_in_capacity_false,multiple_true_finite,raw96_placement_and_serialization,canonical_stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,reject_nonfinite_true
    observed_canonical_output_sha256=dadad8ab786ad5205792f4a1aea4eb9bd154b82c10405d3c5cea36b4423dd5d9
    source_binary_binding_required_in_immutable_runner_receipt=true
    project_artifact_access=false
    status_line=corrected-control mask self-test passed

All eight inherited cases must execute:

- false structural padding;
- the false oldest in-capacity cell;
- multiple true finite cells;
- exact raw96 placement and serialization;
- canonical stream ordering, min/max counts, and SHA-256;
- rejection outside configured capacity;
- rejection of a nonzero false-masked cell; and
- rejection of a non-finite true-masked cell.

The self-test must not open a source, representation probe, checkpoint, model,
policy, certified input, final holdout, or any other scientific artifact.

### Exact stdout normalization

Raw stdout is captured at:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/self_test/self_test.stdout

It must contain exactly two LF-terminated ANSI DEBUG records and no other byte.
Before comparison, the validator performs only this normalization:

1. Require both records to have the exact ANSI framing shown below.
2. Require each thread token to be one or more ASCII decimal digits.
3. Require the two thread tokens to be byte-identical.
4. Replace only those two tokens with the literal ASCII text <thread-id>.
   Do not strip ANSI, trim whitespace, change newlines, or normalize any other
   byte.

After that single replacement, stdout must be exactly:

    [\x1b[36m0x<thread-id>\x1b[0m]: \x1b[94mDEBUG\x1b[0m: [source_runtime_t] initializing static-global source snapshot (single mutable cache updated by explicit runtime call)
    [\x1b[36m0x<thread-id>\x1b[0m]: \x1b[94mDEBUG\x1b[0m: [source_runtime_t] finalizing static-global source snapshot (last_config_path=<none>)

Each \x1b in this notation represents one ESC byte. The finalizer's
last_config_path=<none> is mandatory. A missing, additional, reordered, or
changed record; a different thread token; a different ANSI byte; or any other
stdout content is terminally invalid.

The normalized stdout evidence is fixed at:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/self_test/self_test.stdout.normalized

Only after the raw stdout passes exact byte validation may the runner create
this absent leaf with no-clobber publication. It contains exactly the two
records above with the shared decimal token replaced by literal <thread-id>;
all ANSI and every other byte remain unchanged.

Raw stderr is captured at:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/self_test/self_test.stderr

It must be exactly zero bytes. The report, raw stdout, normalized stdout
evidence, and empty stderr are separate artifacts. Their exact SHA-256 values,
paths, sizes, and validation result must be bound into the immutable self-test
receipt together with the frozen capture source, binary, and build receipt.

The self-test occurs before source access and before attempt publication. Any
CLI, path, creation, process-exit, timeout, signal, report, stdout, stderr,
case, hash, sealing, receipt, or verification failure publishes a terminal
invalid receipt with attempt_consumed=false and all scientific counts zero.
The successor is then retired and may not resume or retry.

## Inherited scientific contract

Every scientific clause of
MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_PREREGISTRATION.md at SHA-256
54356d24de2cb11fedba9d7101b9ba97701d3c428b9d08f69ef9f208804a5719
is incorporated unchanged. The following summary is restrictive, not a
replacement or relaxation of that document.

The two sealed representation inputs remain byte-identical:

    train_range=[0,2496)
    train_probe_sha256=d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75
    train_rows=22464

    validation_range=[2560,2816)
    validation_probe_sha256=8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
    validation_rows=2304

The representation record schema and 96-value
base[0:32],quote[32:64],base_minus_quote[64:96] layout remain unchanged.
The frozen representation checkpoint identity remains
70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d,
but the representation is not constructed or executed.

Only the physically isolated development source may supply the corrected raw
control. The runner performs exactly two raw captures: train [0,2496) and
validation [2560,2816). Source order remains sequential and maximum anchor
read is 2815. The corrected capacity-versus-mask rule, actual-mask stream
hashes, min/max summaries, zero fill, right alignment, raw96 serialization,
row key, coordinate identity, target identity, and independent target check
remain unchanged.

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

The runner starts exactly one bounded evaluator invocation. It performs exactly
six CPU float32 deterministic fits: two arms times seeds 31, 47, and 73. Each
fit uses the unchanged byte-identical initialization pairing and train-row
sequence, unweighted standardized-target MSE, Adam with learning rate 1e-3,
betas 0.9 and 0.999, epsilon 1e-8, zero weight decay, batch size 64,
gradient-norm clipping at 5.0, and exactly 3,500 optimizer steps. Successful
completion therefore records six fits and 21,000 optimizer steps. There is no
dropout, normalization, residual branch, mixture distribution, auxiliary loss,
identity input, early stopping, seed selection, search, retry, refit, or
checkpoint write. Validation is first evaluated after each fit's final step.

The whole post-attempt lifecycle retains the hard upper bound of 5,400 seconds.
There is no replay of either capture or the evaluator.

All mandatory metrics, finite checks, aggregation, medians, per-channel
results, pairwise ordering, strong gates, partial-evidence thresholds, and
mechanical four-way classification remain exactly as preregistered. In
particular, no observed value may change execution, select a seed, tune a
threshold, or authorize another fit.

## Protocol identity versus evaluator schema

The successor protocol and runtime root use only:

    protocol_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1

Runner-owned build, self-test, attempt, stage, terminal, manifest, and
development receipts must carry successor-specific schema IDs and separately
carry that protocol_id.

The scientific evaluator source remains byte-for-byte unchanged. Its report
schema remains:

    evaluator_report_schema=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1

That string is an immutable evaluator payload schema, not the successor
protocol identity. The successor runner must validate it as the evaluator
schema while binding the evaluator report to the new protocol in runner-owned
receipts. It must not require evaluator schema_id to equal protocol_id and must
not edit the evaluator merely to rename its schema.

The raw record schema remains
kikijyeba.synthetic.raw_nodelift_edge_feature_probe.corrected_control.v1 and
the capture-report schema remains
synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_capture_v1.
The self-test report schema also remains the exact value fixed above.

## Protected exclusions and one-shot lifecycle

Canonical data/raw, certified development [2880,3264), final holdout
[3328,4096), policy data, policy artifacts, and all representation, MLP, MDN,
or policy checkpoints remain forbidden. No representation, MLP, optimizer,
MDN, or policy checkpoint may be written. No certified/final access,
representation retraining, production MDN work, policy work, serving change,
or deployment is authorized by this development-only protocol.

The successor runtime root must be pristine, canonical, non-symlinked, private,
and protected by one exclusive execution lock. All source, script, archive,
binary, preregistration, addendum, predecessor-terminal, representation-probe,
isolated-source/config, build-receipt, and self-test identities must be
verified before attempt publication. The attempt is published atomically once
only, after the self-test receipt passes complete re-verification.

After attempt publication, any nonzero exit, timeout, signal, integrity or hash
failure, unexpected artifact, row/coordinate/target mismatch, mask-contract
failure, capture failure, evaluator failure, non-finite fit, wrong invocation,
fit, or optimizer-step count, protected access, or incomplete report
automatically publishes an immutable terminal-invalid receipt. It must bind
the failing stage and command ordinal, artifact and log hashes, actual
capture/evaluator/fit/step counts, maximum possible anchor read, and every
protected-access flag.

No failure may resume or retry under this identity. Success requires exactly
one passed immutable self-test receipt, two completed immutable corrected raw
captures, one completed evaluator invocation, exactly six finite fits, exact
metrics and classifications, a complete hash manifest, and an immutable
development.status followed by byte/hash verification. The outcome remains
development-only and has no benchmark-acceptance authority.
