/*
  runtime.operative.vicreg.solo.train objective-local campaign bundle.

  This cleaned campaign keeps execution focused:
  - train on 2020-2023
  - validate on 2024-01 to 2024-08
  - confirm on the untouched 2024-09 to 2024-12 window for the final
    promotion-biased round

  The imported optim bundle remains frozen. Objective-local iteration happens
  in the wave and jkimyei files, not inside ../../optim/.
*/
CAMPAIGN {
  IMPORT_CONTRACT "iitepi.contract.dsl" AS contract_train_vicreg_solo;

  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_primary;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload_untouched_test;
  FROM "iitepi.waves.dsl" IMPORT_WAVE train_vicreg_primary_no_swa_v1;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload_no_swa_v1;
  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_payload_untouched_test_no_swa_v1;

  BIND bind_train_vicreg_primary_btcusdt {
    __mode_flags = 3;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    __jk_profile_id = stable_pretrain_linear_only;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = train_vicreg_primary;
  };

  BIND bind_eval_vicreg_payload_btcusdt {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = eval_vicreg_payload;
  };

  /*
    Final confirmation bind. This last promotion-biased round spends the
    untouched window after reusable validation so the resulting candidate has a
    complete final evidence package.
  */
  BIND bind_eval_vicreg_payload_btcusdt_untouched_test {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.09.2024;
    __to = 31.12.2024;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = eval_vicreg_payload_untouched_test;
  };

  BIND bind_train_vicreg_primary_btcusdt_no_swa_v1 {
    __mode_flags = 3;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2020;
    __to = 31.12.2023;
    __jk_profile_id = stable_pretrain_no_swa_low_lr;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = train_vicreg_primary_no_swa_v1;
  };

  BIND bind_eval_vicreg_payload_btcusdt_no_swa_v1 {
    __jk_profile_id = eval_payload_no_swa_low_lr;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.01.2024;
    __to = 31.08.2024;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = eval_vicreg_payload_no_swa_v1;
  };

  BIND bind_eval_vicreg_payload_btcusdt_untouched_test_no_swa_v1 {
    __jk_profile_id = eval_payload_no_swa_low_lr;
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    __from = 01.09.2024;
    __to = 31.12.2024;
    MOUNT {
      w_rep = EXACT 0x0200;
    };
    CONTRACT = contract_train_vicreg_solo;
    WAVE = eval_vicreg_payload_untouched_test_no_swa_v1;
  };

  RUN bind_train_vicreg_primary_btcusdt;
  RUN bind_eval_vicreg_payload_btcusdt;
  RUN bind_eval_vicreg_payload_btcusdt_untouched_test;
}
