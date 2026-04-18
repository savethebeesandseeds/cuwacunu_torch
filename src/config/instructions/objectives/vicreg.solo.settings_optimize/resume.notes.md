# Resume Notes

This note carries forward the latest useful state into fresh
`vicreg.solo.settings_optimize` Marshal sessions.

Current carried-forward state:
- `optim/` remains frozen. This objective owns the local campaign, waves,
  contracts, and observation channels used for the medium-dock re-search.
- The active objective-local dock is `1d + 4h + 1h` in
  `tsi.source.dataloader.channels.dsl`.
- Medium-channel experiments intentionally use fresh `0x010x` exact hashimyei
  values so this search does not silently warm-start from old-channel
  artifacts that happen to share the same public dock shape.
- The default primary bind is seeded from `encoder_capacity_v1`; baseline and
  other projector/buffer variants remain available for clean comparisons.
- Marshal session `marshal.mnhkkyxw` completed intended-dock
  baseline calibration and currently carries forward
  `iitepi.contract.base.dsl` with exact hashimyei `0x0100` as the best-known
  candidate on `1d + 4h + 1h`.

What direct Runtime already established:
- The old sub-daily failures were caused by malformed raw kline `close_time`
  values, not by ordinary missing bars.
- The mask still matters, but only after a valid lattice exists. The original
  crash happened earlier, during cache/lattice construction.
- Dataloader hardening and regressions are now in place, so the active medium
  dock should be able to build caches and reach training.
- Direct Runtime campaign `campaign.mngxd8yh` completed the
  primary train bind for `encoder_capacity_v1`, saved weights to slot
  `0x0101`, and logged a small positive
  `forecast.linear.error_skill = 0.006250`.
- That successful post-hardening run used `3d + 1d + 1h`, which validated the
  repaired sub-daily dataloader path but is not the intended current dock.

Current operational caveat:
- The paired eval bind from `campaign.mngxd8yh` failed before
  payload evaluation because Hashimyei catalog refresh hit a live ingest lock.
- Treat that as an operational blocker, not as evidence that the dock or
  architecture failed scientifically.
- Marshal session `marshal.mnhhxg82` accidentally launched the
  wrong dock (`3d + 1d + 1h`) and was operator-paused quickly. Do not count
  that stopped run as scientific evidence for the current dock.

What Marshal already established on the intended dock:
- Marshal session `marshal.mnhkkyxw` used three launches on the
  intended `1d + 4h + 1h` dock and completed cleanly.
- `encoder_capacity_v1` finished train + held-out payload eval on the intended
  dock, but payload quality remained below null on the semantic evaluation
  surface.
- Baseline primary train plus held-out payload eval then completed cleanly for
  exact hashimyei `0x0100` with zero reported runtime anomalies.
- On the most important held-out error metrics, baseline beat
  `encoder_capacity_v1`: linear `error_skill = 0.000619988725` versus
  `-0.009266531867`, and MDN `error_skill = 0.307436274785` versus
  `-0.828638939952`.
- The remaining preauthored automatic hypotheses still belong to the weaker
  `encoder_capacity` family, so the session stopped because no further
  automatic launch was justified without a fresh operator continue request.

What not to repeat blindly:
- Do not spend early launches rediscovering the raw timestamp failure mode.
- Do not mutate `../../optim/` from this objective under ordinary
  objective-only authority.
- Do not read the last eval failure as model regression; it never reached the
  payload evaluation stage.

Suggested continuation:
- Continue from the existing best-known baseline result rather than restarting
  the weaker `encoder_capacity_v1` path blindly.
- If a new automatic launch is justified, prefer a small baseline-centered
  objective-local mutation over additional unchanged `encoder_capacity` family
  launches.
- If eval fails again on Hashimyei locking, triage the infrastructure issue
  first and avoid turning it into architecture evidence.
