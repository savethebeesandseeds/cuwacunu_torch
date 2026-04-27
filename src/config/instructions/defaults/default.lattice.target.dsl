/*
  Lattice target v1.

  This file declares desired evidence, not execution wiring. Runtime campaigns
  train/save components; Lattice later verifies that saved component evidence
  satisfies these target obligations.
*/
LATTICE_TARGET default_lattice_target_v1 {
  IMPORT_CONTRACT "./default.iitepi.contract.dsl" AS contract_default_iitepi;

  TARGET btcusdt_spot_vicreg_mdn_v1 {
    CONTRACT = contract_default_iitepi;

    COMPONENT_TARGET {
      w_rep = TAG vicreg.default;
      w_ev = TAG expected_value.mdn.default;
    };

    SOURCE {
      SYMBOL = BTCUSDT;
      RECORD_TYPE = kline;
      MARKET_TYPE = spot;
      VENUE = binance;
      BASE_ASSET = BTC;
      QUOTE_ASSET = USDT;
      TRAIN = 2020-01-01..2023-12-31;
      VALIDATE = 2024-01-01..2024-08-31;
      TEST = 2024-09-01..2024-12-31;
    };

    EFFORT {
      EPOCHS = 1024;
      BATCH_SIZE = 64;
      MAX_BATCHES_PER_EPOCH = 4000;
    };

    REQUIRE {
      TRAINED = true;
      VALIDATED = true;
      TESTED = true;
    };
  };
};
