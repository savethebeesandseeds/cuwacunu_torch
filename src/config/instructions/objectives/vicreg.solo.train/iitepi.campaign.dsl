/*
  vicreg.solo.train objective-local campaign bundle.

  This objective imports the frozen optimized VICReg package and stages one
  train-plus-eval pass on BTCUSDT, materializing and then reviewing the
  promoted slot `0x00ff`.

  Super may relaunch this same plan multiple times when post-launch evidence
  justifies more effort. The imported optim bundle remains frozen; additional
  training effort comes from repeated launches against the same promoted slot,
  not from mutating ../../optim/.
*/
CAMPAIGN {
  IMPORT_CONTRACT "../../optim/optim.vicreg.solo.iitepi.contract.dsl" AS contract_optim_vicreg_solo;

  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_primary;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_train_vicreg_primary_btcusdt {
    __mode_flags = 3;
    __vicreg_hashimyei_slot = 0x00ff;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_optim_vicreg_solo;
    WAVE = train_vicreg_primary;
  };

  BIND bind_eval_vicreg_payload_btcusdt {
    __vicreg_hashimyei_slot = 0x00ff;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_optim_vicreg_solo;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_train_vicreg_primary_btcusdt;
  RUN bind_eval_vicreg_payload_btcusdt;
}
