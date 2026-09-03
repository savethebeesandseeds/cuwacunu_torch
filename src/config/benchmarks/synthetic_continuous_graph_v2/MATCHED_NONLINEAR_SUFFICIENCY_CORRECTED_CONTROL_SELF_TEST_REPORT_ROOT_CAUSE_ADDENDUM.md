# Matched nonlinear sufficiency corrected-control self-test report root-cause addendum

Status: fixed on 2026-08-14 from immutable data-free failure artifacts, before
any successor binary build or execution. No successor self-test, source
capture, evaluator, fit, metric, or scientific report was opened or produced
to write this addendum.

This addendum explains why
synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1
ended before its attempt receipt. The failure was exclusively a self-test
report transport collision with static-global logging. It was not a failed
self-test case and it carries no evidence about the source data, raw control,
representation, evaluator, or forecastability.

## Immutable lineage

The original raw-control terminal remains the authority for the earlier
capacity-versus-validity contract error:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_development_v1/terminal.invalid.status
    sha256=a83154f71bee7eda29d3de8e221e55c03b4a42742cdb485a01ebb00ac1f41237
    classification=invalid_pre_fit_raw_control_capture_contract_failure

The immediately preceding corrected-control protocol is independently
terminal:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1/terminal.invalid.status
    sha256=ad63fa5dfbb4da59a8efaa32c5577dc2436ad2e795864b51158ef16a4890f0ca
    schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_terminal_invalid_v1
    status=terminal_invalid
    classification=invalid_data_free_self_test_failure
    failure_stage=self_test_validation
    failure_exit_code=1
    attempt_consumed=false
    same_protocol_retry_allowed=false
    new_protocol_required=true

Both terminal receipts remain byte-for-byte untouched. This addendum neither
amends nor un-retires either protocol.

The corrected-control scientific contract is:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_PREREGISTRATION.md
    sha256=54356d24de2cb11fedba9d7101b9ba97701d3c428b9d08f69ef9f208804a5719

The earlier raw-control diagnosis remains:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_V1_TERMINAL_ROOT_CAUSE.md
    sha256=7b3d8fb446c5585e24a0ecfdd9fee250f6781817c74d8d074a82411710dd6cf3

Nothing in this addendum changes either document.

## Sealed failing artifacts

The corrected terminal binds the exact capture implementation and binary:

    source_path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture_corrected_control.cpp
    source_sha256=f1483a0858c342b2477cc37e043bf5a894da369bd1e3ccb51ce04601710de2a8
    binary_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1/build/raw_nodelift_edge_feature_probe_capture_corrected_control
    binary_sha256=e2e244eb7139f145f37763804e9c52c9d1850f125e78701c527618ed7ee1c042

It also binds the failed transport artifacts:

    output_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1/self_test/self_test.output
    output_sha256=efed4a76d7ca7d46c4d3ce8ae5e3cb4be79fabf64f1f08707f28b1e9543c69e5
    output_line_count=10

    log_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1/self_test/self_test.log
    log_sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    log_size_bytes=0

The log hash is the SHA-256 of the empty byte string. The diagnostic messages
did not go to stderr.

## Exact observed report payload

The failed output contains ten LF-terminated records. One ANSI DEBUG record
precedes the machine report, the exact eight intended key/value records occupy
lines 2 through 9, and a second ANSI DEBUG record follows them. With only the
runtime thread token normalized, the two bracketing records are:

    [\x1b[36m0x<thread-id>\x1b[0m]: \x1b[94mDEBUG\x1b[0m: [source_runtime_t] initializing static-global source snapshot (single mutable cache updated by explicit runtime call)
    [\x1b[36m0x<thread-id>\x1b[0m]: \x1b[94mDEBUG\x1b[0m: [source_runtime_t] finalizing static-global source snapshot (last_config_path=<none>)

Here each \x1b denotes the single ESC byte, not four printable characters.
The actual thread token was the same nonempty decimal token on both records.

The eight intervening records are exactly:

    schema_id=synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_self_test_v1
    status=passed
    expected_case_count=8
    expected_cases=false_structural_padding,oldest_in_capacity_false,multiple_true_finite,raw96_placement_and_serialization,canonical_stream_minmax_hash,reject_outside_capacity,reject_nonzero_false,reject_nonfinite_true
    observed_canonical_output_sha256=dadad8ab786ad5205792f4a1aea4eb9bd154b82c10405d3c5cea36b4423dd5d9
    source_binary_binding_required_in_immutable_runner_receipt=true
    project_artifact_access=false
    status_line=corrected-control mask self-test passed

Thus every preregistered in-memory case passed and the canonical output hash
was produced. The terminal arose because the runner required the redirected
stdout file to contain exactly eight lines, while static logging made it ten.

## Exact causal chain

1. The linked source-runtime translation unit defines both
   source_runtime_t::inst and source_runtime_t::_initializer at
   src/impl/ujcamei/source/contract/runtime/decode.cpp lines 36-37.

2. The _initializer constructor calls source_runtime_t::init before main and
   its destructor calls source_runtime_t::finit during process teardown, as
   defined at src/include/ujcamei/source/contract/runtime/decode.h lines 35-46.
   The two exact log calls are at
   src/impl/ujcamei/source/contract/runtime/decode.cpp lines 532-544. The
   finalizer's last_config_path=<none> is affirmative evidence that this
   data-free invocation never updated or opened the source runtime.

3. The corrected capture build did not override the logging configuration.
   The compiled default is DLOGS_USE_IOSTREAMS=0,
   LOG_FILE=stdout, and LOG_DBG_FILE=LOG_FILE at
   src/include/piaabo/log/dlogs_private_config.h lines 76-109. The stdio
   log_dbg macro writes its colored prefix and payload to LOG_DBG_FILE at
   src/include/piaabo/log/dlogs_private_macros.h lines 58-68. Consequently
   both lifecycle diagnostics were stdout records, not stderr records.

4. The capture main at
   raw_nodelift_edge_feature_probe_capture_corrected_control.cpp lines 930-942
   performs no logger initialization. Suppression begun in main could not
   remove the already emitted pre-main record, and changing the project-wide
   logger or canonical archive would be broader than this failure.

5. The corrected runner redirected capture stdout directly to
   self_test.output and stderr to self_test.log. Its validator required exactly
   eight lines before checking the eight keys. The binary returned zero, but
   the ten-line redirected stdout therefore failed validation and was sealed
   terminally, as the preregistered failure policy required.

The capture's existing normal probe/report publication already uses exclusive
creation with O_EXCL and O_NOFOLLOW. The self-test report instead used stdout,
where it shared a transport with linked lifecycle diagnostics. That asymmetry,
not a scientific assertion, is the defect.

## Scientific boundary and successor authority

The corrected terminal records all of the following:

    isolated_development_source_access_started=false
    representation_probe_access=false
    raw_capture_invocations_started=0
    raw_capture_invocations_completed=0
    evaluator_invocations_started=0
    evaluator_invocations_completed=0
    fits_started=0
    fits_completed=0
    optimizer_steps=0
    scientific_result_available=false
    maximum_anchor_read_upper_bound=none
    certified_input_access=false
    final_holdout_access=false
    policy_access=false

Therefore the failure says nothing about either arm's learnability and cannot
change any model, seed, split, metric, gate, or interpretation.

Any successor must use a new identity. The only authorized correction is to
separate the eight-record self-test report from stdout by giving the frozen
binary an exclusive-create report path, while validating the two known
static-global stdout diagnostics independently. The successor identity is
synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1.
Its separate preregistration is the sole authority for that one-shot attempt.
