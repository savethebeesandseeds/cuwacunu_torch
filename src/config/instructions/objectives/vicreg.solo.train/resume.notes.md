# Resume Notes

- Latest full long-horizon continuation on `0x00FF` completed `1024` train
  epochs and then reusable validation on `01.01.2024` to `31.08.2024`.
- The cleaned `stable_pretrain_linear_only` family is the best base we have in
  this objective bundle so far. Keep leaning on it rather than reviving heavier
  augmentation variants.
- This handoff is for one final promotion-biased pass, not for another search
  tree. Continue from the existing `0x00FF` weights.
- Use a conservative finishing policy: slightly cooler optimizer pressure,
  slightly gentler linear perturbation, then run reusable validation and the
  untouched final test in the same campaign.
- Representation reports are still worth saving, but the practical decision
  after this round is whether the resulting promoted bundle is good enough to
  hand to the downstream engine.
