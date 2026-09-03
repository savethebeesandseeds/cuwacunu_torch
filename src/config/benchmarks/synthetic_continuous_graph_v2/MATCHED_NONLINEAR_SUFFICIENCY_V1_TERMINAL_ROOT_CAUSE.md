# Matched nonlinear sufficiency V1 terminal root cause

Status: terminal and immutable on 2026-08-13. This record explains why
`synthetic_v2_matched_nonlinear_sufficiency_development_v1` ended before any
fit and therefore produced no scientific result.

## Sealed failure authority

The controlling receipt is:

```text
path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_matched_nonlinear_sufficiency_development_v1/
     terminal.invalid.status
sha256=a83154f71bee7eda29d3de8e221e55c03b4a42742cdb485a01ebb00ac1f41237
schema_id=synthetic_v2_matched_nonlinear_sufficiency_terminal_invalid_v1
status=terminal_invalid
classification=invalid_pre_fit_raw_control_capture_contract_failure
```

It binds the consumed attempt and failure log exactly:

```text
path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_matched_nonlinear_sufficiency_development_v1/
     attempt.status
sha256=451615eb7807b6fcd2c538112339662bb024767a26475c91903c5dbe116350c1

path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_matched_nonlinear_sufficiency_development_v1/
     logs/capture.train.log
sha256=e6f97923483c8e1347d3d5d4c645cf059771453d892e7737176392c9371c29c5
```

The receipt also binds the V1 preregistration at
`cbbf1d837aa741ed157beb2fbab5b01d6c6e004376e865b1f71f2732b46fa348`,
the runner at
`b39e4812051af3f0e5d0492a7346ebaab5b38a88cf3a901e615967c770b7548c`,
and the raw-capture source at
`cb6f02b232887e4619f76b65e4388be57a45ff576f2d81f13fc89c665f747c1c`.
The failure was the first raw-train capture command, with reason code
`close_coordinate_mask_contract_mismatch` and message
`close-coordinate mask is not the exact right-aligned 4/10/30 contract`.
The same message is the terminal error line of the bound capture log.

## Exact causal chain

The failure was an error in the V1 control assertion, not evidence about the
data, representation, or nonlinear decoder.

1. **The retrieval DSL declares capacities.** In
   `src/config/benchmarks/synthetic_continuous_graph_v2/ujcamei.source.retrieval.channels.dsl`
   lines 10-12, the active 1w, 3d, and 1d channels have `input_length` 4, 10,
   and 30. The file SHA-256 is
   `36bcb2d4430f9e18673829bc4945ce04715d0f7749608177b8cd6a519fd58feb`.
   These numbers specify the maximum structural history supplied for each
   channel. They do not promise that every record or every recovered node
   coordinate inside that history is valid.

2. **The multi-channel dataset pads structure and preserves validity.** In
   `src/include/ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_dataset.h`
   lines 1153-1158, channel inputs are declared left-padded to the maximum
   input length. Lines 1183-1186 retain each channel's configured `np`, and
   lines 1234-1249 prepend zero feature rows and false mask rows when
   `np < max_input_length_`. The file SHA-256 is
   `c1c74de30a95d8a25c924438ea30e18e82a9bec96182be2843176e19a44005dd`.
   Consequently the common tensor has width 30, but the mask, not tensor
   position alone, is authoritative for validity.

3. **Log returns necessarily mask a record lacking predecessor context.** In
   `src/include/ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_datafile.h`
   lines 694-725, a valid record with no `previous_valid` is replaced by
   `null_instance` and counted as masked; an invalid record also resets
   predecessor context. The file SHA-256 is
   `87dff97f6d72ef4367b1aa2f2cb024cf310d8d66efcb121b0e31f2c7704bf862`.
   The kline implementation independently returns `null_instance` when the
   predecessor is absent at
   `src/impl/ujcamei/source/registry/types/registry_data.cpp` lines 603-615,
   SHA-256
   `1eae8a8c8ba02ddd51072892e6235219e5e07f767f52c1793dc9b97a8aa11376`.
   Therefore the first normalized record, and the first valid record after an
   invalid gap, can be structurally present while correctly mask-invalid.

4. **NodeLift validity additionally requires graph recoverability.** In
   `src/impl/wikimyei/expression/nodelift/srl/synthetic_reference_lift.cpp`
   lines 275-294, coordinate validity is the conjunction of the incoming edge
   mask, optional coordinate mask, and finiteness. Lines 303-307 initialize
   node features to zero and node masks to false. Lines 359-405 admit only
   graph components with enough valid edges, and lines 422-434 set a node
   coordinate mask true only after a successful solve. The file SHA-256 is
   `7d753e6e8e92454ca0a6e35dd8c0b56842918e34df4400d34a82b6ccbc2c014b`.
   Thus a configured source-history slot is not automatically a recoverable
   NodeLift node slot.

5. **V1 falsely promoted capacity into an exact validity pattern.** In
   `src/scripts/benchmarks/synthetic_continuous_graph_v2/raw_nodelift_edge_feature_probe_capture.cpp`
   lines 50-55, V1 fixed history width 30 and the capacity tuple 4/10/30.
   Lines 341-369 then required every close mask to equal
   `history >= 30 - capacity[channel]`, with exactly the capacity count true.
   This rejected any legitimate log-return or NodeLift-invalid cell inside the
   structural window. The first train capture encountered that legitimate
   distinction and failed closed.

The exact failing cell pattern was not needed and was not opened for this
root-cause record. The source semantics and sealed failure message are
sufficient: V1 equated configured history capacity with guaranteed
node-recoverable validity count, an implication the pipeline never makes.

## Scientific boundary

The terminal receipt records:

```text
raw captures completed=0
raw probe files published=0
validation capture started=false
evaluator invocations started=0
fits started=0
fits completed=0
optimizer steps=0
train metrics computed=false
validation metrics computed=false
scientific classification emitted=false
scientific result available=false
```

No representation forward, representation checkpoint, MDN, policy, certified
input, or final holdout was accessed. V1 is consumed, cannot resume, and cannot
retry. It says nothing about whether either MLP arm is predictable or whether
the frozen representation preserves the signal. Any corrected control must
use a new protocol identity and must treat the actual NodeLift mask as the
validity authority within the configured structural capacity.
