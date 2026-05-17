/*
  .campaign.dsl
  =============
  Purpose:
    Objective-local Runtime scaffold for Marshal smoke sessions.

    This file exists so an operator can explicitly test Marshal-triggered
    Runtime launch/stop behavior. It is not a claim that VICReg training is the
    subject of this objective; it simply reuses the currently available default
    wave inventory for a bounded smoke run when the operator requests one.
*/
CAMPAIGN {
  IMPORT_CONTRACT "../../defaults/default.iitepi.contract.dsl" AS contract_default_iitepi;

  FROM "../../defaults/default.iitepi.wave.dsl" IMPORT_WAVE train_vicreg_primary;

  BIND bind_smoke_runtime_launch {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    CONTRACT = contract_default_iitepi;
    WAVE = train_vicreg_primary;
  };

  RUN bind_smoke_runtime_launch;
}
