CAMPAIGN {
  IMPORT_CONTRACT_FILE "binding.contract.dsl";

  IMPORT_WAVE_FILE "binding.wave.dsl";

  BIND bind_train_vicreg_primary {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    CONTRACT = contract_binding;
    WAVE = train_vicreg_primary;
  };

  RUN bind_train_vicreg_primary;
}
