/*
  vicreg.solo.multi_symbol_eval objective-local campaign bundle.

  This objective imports the frozen optimized VICReg package and evaluates the
  already-trained promoted slot across discovered compatible symbols.

  Intentionally starts as an eval-only scaffold:
    Marshal Hero should author the concrete BIND and RUN entries on the first
    planning checkpoint.
    Auto-discover candidate symbols from local data inventory.
    Keep only symbols that satisfy the active optimized channel coverage and
    declared evaluation window.
    Build stable bind names from the discovered roster.
    Reuse the promoted optimized Hashimyei slot `0x00FF` for every evaluated
    symbol.
    Optional reference/control runs are allowed when justified, but no fixed
    symbol should be privileged by the scaffold itself.
    The placeholder bind and RUN below exist only to satisfy campaign decoding
    and should be replaced before the first real launch.
*/
CAMPAIGN {
  IMPORT_CONTRACT "../../optim/optim.vicreg.solo.iitepi.contract.dsl" AS contract_optim_vicreg_solo;

  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_scaffold_eval_template_replace_before_launch {
    __vicreg_hashimyei_slot = 0x00FF;
    __sampler = sequential;
    __workers = 0;
    __symbol = DISCOVER_SYMBOL_AND_REPLACE_ME;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_optim_vicreg_solo;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_scaffold_eval_template_replace_before_launch;
}
