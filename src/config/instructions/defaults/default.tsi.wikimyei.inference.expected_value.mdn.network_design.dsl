/*
  default.tsi.wikimyei.inference.expected_value.mdn.network_design.dsl
  ============================================================
  Purpose:
    Framework-agnostic network design payload for the MDN-backed
    ExpectedValue Wikimyei.

  Notes:
    - This file is the authored source for ExpectedValue architecture.
    - The thin `default.tsi.wikimyei.inference.expected_value.mdn.dsl`
      wrapper owns runtime placement, checkpoint path, and payload bindings.
    - Contract-scoped `__embedding_dims` and
      `__future_target_feature_indices` form the
      public docking bridge from VICReg representation output into the MDN.
    - `ENCODING.temporal_reducer` is explicit by design. The MDN only accepts
      already-reduced `[B,De]`; ExpectedValue applies this policy while it still
      has the source observation mask.
*/

NETWORK "tsi.wikimyei.inference.expected_value.mdn" {
  ASSEMBLY_TAG = mdn_expected_value;

  node enc@ENCODING {
    De:int = % __embedding_dims ? 72 %;
    temporal_reducer:str = last_valid;
  }

  node target@FUTURE_TARGET {
    target_feature_indices:arr[int] = % __future_target_feature_indices ? 3 %;
  }

  node mdn@MDN {
    mixture_comps:int = 10;
    features_hidden:int = 12;
    residual_depth:int = 3;
  }

  export expectation = mdn;
}
