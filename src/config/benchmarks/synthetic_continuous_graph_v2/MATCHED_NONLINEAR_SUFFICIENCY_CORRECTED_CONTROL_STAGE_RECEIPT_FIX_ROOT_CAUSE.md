# Matched nonlinear sufficiency corrected-control stage-receipt root cause

Status: fixed on 2026-08-14 from immutable receipt metadata and the frozen
predecessor runner, before any successor build, self-test, source access,
capture, evaluator invocation, fit, optimizer step, metric, or scientific
report.

This document explains why
synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1
ended immediately after consuming its attempt. The failure was a Bash
stage-receipt construction defect. It was not a capture, evaluator, model,
data, or scientific failure.

## Immutable predecessor bundle

The exact predecessor runner is:

    path=/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/run_matched_nonlinear_sufficiency_corrected_control_self_test_report_v1.sh
    sha256=9e6328a7385972abe14aea57def6307a6a0a982632fc3097d6bcc94df8677458

Its controlling preregistration is:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_PREREGISTRATION.md
    sha256=d22232ea9f3e71dce58c1c5a11beca32c028f6eb5961b5c2e62b7b949c2951f9

The immediate predecessor terminal is:

    path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/terminal.invalid.status
    sha256=71cc4730d172541d50fab4ae9d46bc0b55138070994fc873b89acdecb8d01ccf
    schema_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_terminal_invalid_v1
    status=terminal_invalid
    classification=invalid_post_attempt_execution
    failure_stage=post_attempt_setup
    failure_command_ordinal=0
    failure_reason_code=nonzero_exit
    worker_exit_code=1

The consumed attempt and diagnostic worker log are:

    attempt_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/attempt.status
    attempt_sha256=b59308962ab31837e2002f7122db8f2a5d9309e784caa1b2f119656cd5ab97df

    worker_log_path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_matched_nonlinear_sufficiency_corrected_control_self_test_report_development_v1/logs/worker.log
    worker_log_sha256=9ec0ea0d3728056a8dde8049b77819a372b1514d7b67320ff359886054ab6823

The prior self-test transport diagnosis remains independently fixed:

    path=/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MATCHED_NONLINEAR_SUFFICIENCY_CORRECTED_CONTROL_SELF_TEST_REPORT_ROOT_CAUSE_ADDENDUM.md
    sha256=ad9abce221bf4d93164ded532f20d668a7b5a4cf483c7b5a4759cf65a0d83ece

None of these artifacts is amended, replaced, retried, or reinterpreted by
this document.

## Exact state at failure

The terminal receipt fixes the state at the failure boundary:

    attempt_consumed=true
    same_protocol_resume_allowed=false
    same_protocol_retry_allowed=false
    new_protocol_required=true
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

The predecessor completed its frozen build and data-free self-test and then
published attempt ordinal 1. Its first post-attempt operation was stage setup.
No capture process, scientific evaluator, fit, or optimizer step began.

## Exact mechanical cause

The frozen predecessor runs with nounset enabled. Its emit_stage function used
one local builtin command to declare name and state while also computing the
scratch candidate:

    local name="$1" state="$2" candidate="${SCRATCH}/${name}.${state}.$$"

Bash expands every assignment word passed to that local builtin before the
builtin establishes the command's local assignments. Therefore the candidate
expansion reads name and state while they are still unset. With set -u, that
read terminates the worker with a nonzero exit.

The stage receipt ultimately targeted:

    ${STAGES}/${name}.${state}.status

but execution never reached publication of that path. In other words, the
one-command declaration/assignment used to construct the stage receipt
expanded its name/state-dependent path before name and state were initialized.
The failure occurred during post_attempt_setup at command ordinal 0, exactly
as the terminal records.

This is a shell evaluation-order error. It does not depend on source content,
captured rows, evaluator behavior, initialization seeds, optimization, or
observed metrics.

## Sole authorized correction

The successor may change only the declaration/assignment ordering in
emit_stage, apart from mechanically necessary successor protocol, schema,
runtime-root, document, and immutable-lineage identities. The corrected
function prefix is:

    local name state candidate
    [[ $# -ge 2 ]] || fail "emit_stage requires name and state"
    name="$1"
    state="$2"
    shift 2
    candidate="${SCRATCH}/${name}.${state}.$$"

The destination remains exactly:

    ${STAGES}/${name}.${state}.status

No receipt field, publication primitive, sealing rule, stage order, failure
trap, command ordinal, timeout, retry rule, or scientific behavior may change.
No broad shell refactor, helper rewrite, logger change, or opportunistic
cleanup is authorized.

## Scientific boundary

The predecessor attempt carries no scientific result. In particular, its
zero capture/evaluator/fit/step counts and maximum_anchor_read_upper_bound=none
prove that the failure cannot update any conclusion about raw-history
learnability, representation sufficiency, forecasting, or the data.

All capture code and settings, evaluator code and schema, data-free self-test,
source isolation, representation inputs, decoder, seeds, optimization,
metrics, gates, classifications, time limits, receipt validation, and
protected exclusions remain frozen by the predecessor preregistration at
SHA-256 d22232ea9f3e71dce58c1c5a11beca32c028f6eb5961b5c2e62b7b949c2951f9.
The evaluator payload schema remains:

    synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1

It is not renamed to the successor protocol identity.

The only authorized successor is the new one-shot protocol:

    synthetic_v2_matched_nonlinear_sufficiency_corrected_control_stage_receipt_fix_development_v1

It is not a resume or retry of the consumed predecessor attempt. Its separate
preregistration is the sole authority for execution.
