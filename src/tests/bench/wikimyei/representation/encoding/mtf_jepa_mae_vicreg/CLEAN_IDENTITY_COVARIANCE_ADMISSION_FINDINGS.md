# LSA-0 — Clean-Identity Covariance Admission Findings

Executed 2026-09-05. Decision: **`not_admitted`**. Mechanics/custody: **PASS**.

The registered audit does not justify spending the matched three-seed training
budget on covariance removal. No encoder optimizer or EMA updates were performed.
This is a directional budget decision, not proof that covariance is harmless.

## Question and result

At the certified FSPA-4 start and authenticated GPV affine-projector endpoints,
does covariance exert a distinctive adverse direction on protected structure?
The same clean-identical global-pool objective was replayed at both states,
averaging the frozen SSL row schedules at steps 0, 255, and 511.

The delayed gradient interaction is real: covariance is tiny at initialization,
but comparable to variance at the endpoints. Endpoint gradients nearly oppose
each other, so the full gradient is much smaller than either component.

| Seed | State | Weighted variance trunk norm | Weighted covariance trunk norm | Covariance/full norm ratio | Covariance–variance cosine |
|---:|---|---:|---:|---:|---:|
| 17 | FSPA-4 | 0.029677 | 0.000013192 | 0.000445 | -0.899454 |
| 31 | FSPA-4 | 0.023651 | 0.000008362 | 0.000354 | -0.888764 |
| 47 | FSPA-4 | 0.024011 | 0.000006449 | 0.000269 | -0.855418 |
| 17 | GPV endpoint | 0.254719 | 0.263929 | 10.334000 | -0.995780 |
| 31 | GPV endpoint | 0.238512 | 0.237279 | 6.203171 | -0.987087 |
| 47 | GPV endpoint | 0.314940 | 0.324490 | 5.921657 | -0.985755 |

However, covariance's virtual direction increased normalized order separation
and channel contrast in all three endpoint seeds. Variance's direction decreased
both. For the larger precommitted displacement (0.001 of trunk parameter norm):

| Seed | Covariance: order change | Variance: order change | Covariance: channel change | Variance: channel change |
|---:|---:|---:|---:|---:|
| 17 | +1.640% | -1.590% | +1.836% | -1.824% |
| 31 | +1.408% | -1.283% | +1.287% | -1.164% |
| 47 | +1.010% | -1.002% | +1.669% | -1.605% |

Participation responses were mixed. Only seed 47, endpoint channel 1, met the
complete adverse predicate at both radii: covariance reduced participation by
1.200% and 2.585%, while variance increased it by 0.977% and 1.737%; the full
direction also reduced participation. That is **1/3**, below the sealed **2/3**
requirement for the same diagnostic at one state. Every other diagnostic/state
had zero qualifying seeds. The initial state also failed the non-negligible
weighted covariance/full norm-ratio requirement.

## Mechanical validation

- All six archive hashes, endpoint metadata, frozen dataset/splits, historical
  GPV source/log, and the documented post-GPV view-pairing seam were authenticated.
- Separate affine projected views were exactly equal; invariance loss and all
  invariance gradients were exactly zero.
- Weighted component gradients reconstructed the full gradient in all six cases.
  Maximum absolute residual: `5.23869e-9`; maximum relative L2 residual:
  `1.20329e-6`, within the sealed `5e-5` / `1e-4` tolerances.
- Repeated baseline diagnostics were exact. Realized virtual displacements
  matched their prescribed norms. Restoring the disposable trunk recovered the
  entire original state, verifying fixed projector, target, other parameters,
  and buffers.
- Reference parameters, buffers, target state, empty gradient slots, evaluation
  mode, and CPU/CUDA RNG were preserved. No optimizer was constructed and no
  checkpoint was written. Historical Adam state is not in the GPV archives and
  was not compared. Confirmation remained unopened.
- The final targeted build and one complete audit run passed. The earlier build
  was compile-only; no new measurements preceded the sealed protocol.

## Interpretation and next boundary

This audit weakens the proposed explanation that covariance removal would rescue
the clean affine global-pool chassis: its observed endpoint direction protects
the two separation proxies while opposing variance. It does not establish what
an Adam training trajectory would do. There are no new quality-probe AULCs here;
these label-free structural proxies cannot certify order/family safety, predict
downstream gains, or exclude intermediate-state and optimizer effects.

LSA-1 remains unopened. Do not respond with covariance weight ladders or more
projector/variance searches. The next proposed boundary is LWM-0: design and audit
a genuinely future, history-conditioned online-target objective with explicit
noncollapse protection and authenticated disjoint raw support. That contract
remains unimplemented and unregistered. FSPA-4 plus the sparse structured readout
remains the scientific anchor; the representation training problem is unresolved.

## Reproduction and retained outputs

From `/cuwacunu` inside the existing `unnamed_taoist` container:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -f Makefile.lsa0 -j12 lsa0
.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_covariance_admission
```

The ordinary Makefile, production defaults, and historical GPV artifacts were
not changed. The opt-in executable has no training dispatch.

- [Sealed protocol](CLEAN_IDENTITY_COVARIANCE_ADMISSION_PROTOCOL.md), SHA-256
  `37ab8774e3c4fa62c9d2886319237d7e0601bf6086a0c622ca621d54dfed4f31`.
- [All 180 virtual metric records](CLEAN_IDENTITY_COVARIANCE_ADMISSION_RESULTS.csv),
  SHA-256 `fb122f2332e6052b9aa0d48fefe9fecaebbc27ae7fb70a2c0038a9d2d558b206`.
- [All 24 component gradient records](CLEAN_IDENTITY_COVARIANCE_ADMISSION_GRADIENTS.csv),
  SHA-256 `c6db3a2bbb35a0c4dd9e8a29f45dae6165302c81820b9a9f6da30b43a96cee57`.
- Machine-local full log: `.build/tests/representation_lsa0_v1.log`, SHA-256
  `1a94f1157fbc17f9a15d82f65474db48ec795244df288a3f21c4308c8564d935`.
- Source SHA-256: `dd95c5514c379ee5febf96eb97d20b58272513a6caf2d1dbe457e8648af8fdec`.
- Executed binary SHA-256: `cff6b22ca18ce41f1de95f6e0e21c6a56fc55e5ad7769a96bc6697af787b5b98`.

The source, protocol, findings and CSV files are portable; `.build` remains ignored.
