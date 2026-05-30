/*
  .campaign.dsl
  Objective-local campaign bundle.

  This objective imports the frozen optimized VICReg package and evaluates the
  already-trained promoted component revision across discovered compatible
  symbols.

  Intentionally starts as an eval-only scaffold:
    Marshal Hero should author the concrete BIND and RUN entries on the first
    planning checkpoint.
    Auto-discover candidate symbols from local data inventory.
    Keep only symbols that satisfy the active optimized channel coverage and
    declared evaluation window.
    Build stable bind names from the discovered roster.
    Reuse the optimized contract component identity for every evaluated symbol;
    the concrete storage Hashimyei is derived from contract compatibility.
    Optional reference/control runs are allowed when justified, but no fixed
    symbol should be privileged by the scaffold itself.
    The placeholder bind and RUN below exist only to satisfy campaign decoding
    and should be replaced before the first real launch.
*/
CAMPAIGN {
  IMPORT_CONTRACT "../../optim/optim.vicreg.solo.iitepi.contract.dsl" AS contract_optim_vicreg_solo;

  FROM ".waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_scaffold_eval_template_replace_before_launch {
    __sampler = sequential;
    __workers = 0;
    __symbol = DISCOVER_SYMBOL_AND_REPLACE_ME;
    __base_asset = DISCOVER_BASE_ASSET_AND_REPLACE_ME;
    __quote_asset = DISCOVER_QUOTE_ASSET_AND_REPLACE_ME;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_optim_vicreg_solo;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_scaffold_eval_template_replace_before_launch;
}
