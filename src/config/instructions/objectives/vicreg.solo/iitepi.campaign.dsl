/*
  vicreg.solo objective-local campaign bundle.

  Keep the whole study in one place. RUN defines the default launch target,
  while Runtime Hero may narrow execution to any declared BIND through
  hero.runtime.start_campaign(binding_id=...).
*/
CAMPAIGN {
  IMPORT_CONTRACT "iitepi.contract.base.dsl" AS contract_iitepi_contract_base;
  IMPORT_CONTRACT "iitepi.contract.small.dsl" AS contract_iitepi_contract_small;
  IMPORT_CONTRACT "iitepi.contract.wide.dsl" AS contract_iitepi_contract_wide;
  IMPORT_CONTRACT "iitepi.contract.deep.dsl" AS contract_iitepi_contract_deep;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_smoke;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_ablation_short;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_primary;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_train_vicreg_smoke {
    __hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_smoke_small {
    __hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_small;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_smoke_wide {
    __hashimyei_slot = 0x0002;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_wide;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_smoke_deep {
    __hashimyei_slot = 0x0003;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_deep;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_ablation_short {
    __hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_small {
    __hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_small;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_wide {
    __hashimyei_slot = 0x0002;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_wide;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_deep {
    __hashimyei_slot = 0x0003;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_deep;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_primary {
    __mode_flags = 3;
    __hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_small {
    __mode_flags = 3;
    __hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_small;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_wide {
    __mode_flags = 3;
    __hashimyei_slot = 0x0002;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_wide;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_deep {
    __mode_flags = 3;
    __hashimyei_slot = 0x0003;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_deep;
    WAVE = train_vicreg_primary;
  };

  BIND bind_eval_vicreg_payload {
    __hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_small {
    __hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_small;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_wide {
    __hashimyei_slot = 0x0002;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_wide;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_deep {
    __hashimyei_slot = 0x0003;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_deep;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_train_vicreg_primary;
  RUN bind_eval_vicreg_payload;
}
