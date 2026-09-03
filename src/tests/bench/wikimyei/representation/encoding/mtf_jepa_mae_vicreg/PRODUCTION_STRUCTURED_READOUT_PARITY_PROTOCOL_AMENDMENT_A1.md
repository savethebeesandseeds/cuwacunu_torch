# SRR-2 protocol amendment A1 — deterministic command and log custody

**Sealed:** 2026-08-27, after implementation review and before any SRR-2
preflight or authoritative scientific row  
**Parent protocol:** `PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md`  
**Parent SHA-256:**
`742def90993850ab7ed381e860d60f5adbf1a258c2d9a7de0568bc0067af985e`  
**Parent bytes:** `20061`

## Reason

Preflight review found an operational contradiction in the immutable parent:
Section 15's exact command omitted the `CUBLAS_WORKSPACE_CONFIG=:4096:8`
setting required by the protocol's deterministic CUDA environment, while the
existing container does not define that variable. It also did not route the
complete experiment output to the canonical authoritative-log path required by
Sections 16 and 17.

No SRR-2 preflight capture, authoritative capture, target, probe, fit,
prediction, permutation, or bootstrap row had been requested when this defect
was found. The authoritative-attempt count remained zero. The parent protocol
and its sidecar remain byte-for-byte unchanged and authoritative for every
scientific population, threshold, gate, precedence rule, and exclusion.

## Narrow replacement

This amendment replaces only the command code block in parent Section 15. The
exact authoritative command is:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity --device cuda > .build/tests/representation_srr2_v1_authoritative.log 2>&1'
```

The required target-free preflight command, executed before the pre-run
manifest is sealed, is:

```text
docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity --experiment production-structured-readout-parity-preflight --device cuda > .build/tests/representation_srr2_v1_preflight.log 2>&1'
```

The pre-run manifest and both independent implementations must authenticate
the parent protocol, parent sidecar, this amendment, and this amendment's
sidecar. A missing or mismatched amendment is `invalid_mechanics`. No other
parent-protocol text is replaced or relaxed.
