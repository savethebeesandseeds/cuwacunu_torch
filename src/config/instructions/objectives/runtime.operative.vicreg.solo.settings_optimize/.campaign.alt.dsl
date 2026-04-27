/*
  .campaign.dsl
  Objective-local campaign bundle.

  Keep the whole study in one place, but start from a calm baseline. The
  default campaign declares only the baseline contract plus four meaningful
  phases: smoke, short ablation, primary train, and payload evaluation.
  If future objective work needs additional contract variants or a different
  launch graph, add them deliberately instead of treating them as baseline.

  Contract variants own architecture differences. Runtime derives component
  Hashimyei storage identity from each selected contract compatibility surface,
  so this search does not need campaign-owned revision tokens.
*/
CAMPAIGN {
  IMPORT_CONTRACT ".contract.base.dsl" AS contract_iitepi_contract_base;
  IMPORT_CONTRACT ".contract.variant.baseline_buffer_averaging_v1.dsl" AS contract_iitepi_contract_variant_baseline_buffer_averaging_v1;
  IMPORT_CONTRACT ".contract.variant.encoder_capacity_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_v1;
  IMPORT_CONTRACT ".contract.variant.encoder_capacity_projector_compact_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_projector_compact_v1;
  IMPORT_CONTRACT ".contract.variant.encoder_capacity_projector_midwidth_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_projector_midwidth_v1;
  IMPORT_CONTRACT ".contract.variant.encoder_capacity_buffer_averaging_v1.dsl" AS contract_iitepi_contract_variant_encoder_capacity_buffer_averaging_v1;
  FROM ".waves.dsl" IMPORT_WAVE train_vicreg_smoke;
  FROM ".waves.dsl" IMPORT_WAVE train_vicreg_ablation_short;
  FROM ".waves.dsl" IMPORT_WAVE train_vicreg_primary;
  FROM ".waves.dsl" IMPORT_WAVE eval_vicreg_payload;

  BIND bind_train_vicreg_smoke {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.01.2020;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_smoke;
  };

  BIND bind_train_vicreg_ablation_short {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_projector_compact_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_projector_compact_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_projector_midwidth_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_projector_midwidth_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_ablation_short_encoder_capacity_buffer_averaging_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2022;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_buffer_averaging_v1;
    WAVE = train_vicreg_ablation_short;
  };

  BIND bind_train_vicreg_primary {
    __mode_flags = 3;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_baseline_buffer_averaging_v1 {
    __mode_flags = 3;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_variant_baseline_buffer_averaging_v1;
    WAVE = train_vicreg_primary;
  };

  BIND bind_train_vicreg_primary_encoder_capacity_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = train_vicreg_primary;
  };

  BIND bind_eval_vicreg_payload {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_base;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_baseline_buffer_averaging_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_variant_baseline_buffer_averaging_v1;
    WAVE = eval_vicreg_payload;
  };

  BIND bind_eval_vicreg_payload_encoder_capacity_v1 {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __base_asset = BTC;
    __quote_asset = USDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    CONTRACT = contract_iitepi_contract_variant_encoder_capacity_v1;
    WAVE = eval_vicreg_payload;
  };

  RUN bind_train_vicreg_primary_baseline_buffer_averaging_v1;
  RUN bind_eval_vicreg_payload_baseline_buffer_averaging_v1;
}
