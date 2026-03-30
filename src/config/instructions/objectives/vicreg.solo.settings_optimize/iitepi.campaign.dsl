/*
  vicreg.solo objective-local campaign bundle.

  Keep the whole study in one place, but start from a calm baseline. The
  default campaign declares only the baseline contract plus four meaningful
  phases: smoke, short ablation, primary train, and payload evaluation.
  If future objective work needs additional contract variants or a different
  launch graph, add them deliberately instead of treating them as baseline.
*/
CAMPAIGN {
  IMPORT_CONTRACT "iitepi.contract.base.dsl" AS contract_iitepi_contract_base;
  IMPORT_CONTRACT "iitepi.contract.variant.encoder_capacity_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_v1;
  IMPORT_CONTRACT "iitepi.contract.variant.encoder_capacity_projector_compact_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_projector_compact_v1;
  IMPORT_CONTRACT "iitepi.contract.variant.encoder_capacity_projector_midwidth_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_projector_midwidth_v1;
  IMPORT_CONTRACT "iitepi.contract.variant.encoder_capacity_buffer_averaging_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_buffer_averaging_v1;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_smoke;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_ablation_short;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_primary;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_train_vicreg_smoke {
    __vicreg_hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_ablation_short {
    __vicreg_hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_v1 {
    __vicreg_hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_projector_compact_v1 {
    __vicreg_hashimyei_slot = 0x0002;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_projector_compact_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_projector_midwidth_v1 {
    __vicreg_hashimyei_slot = 0x0003;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_projector_midwidth_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_buffer_averaging_v1 {
    __vicreg_hashimyei_slot = 0x0004;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_buffer_averaging_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_primary {
    __mode_flags = 3;
    __vicreg_hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_encoder_capacity_v1 {
    __vicreg_hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = train_vicreg_primary;
  };

  BIND bind_eval_vicreg_payload {
    __vicreg_hashimyei_slot = 0x0000;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_encoder_capacity_v1 {
    __vicreg_hashimyei_slot = 0x0001;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_train_vicreg_primary_encoder_capacity_v1;
  RUN bind_eval_vicreg_payload_encoder_capacity_v1;
}
