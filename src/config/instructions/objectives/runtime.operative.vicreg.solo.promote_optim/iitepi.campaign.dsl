/*
  runtime.operative.vicreg.solo.promote_optim objective-local campaign bundle.

  This objective compares VICReg component revisions on the untouched benchmark
  window and then requests human approval before promoting anything to optim.
*/
CAMPAIGN {
  IMPORT_CONTRACT "iitepi.contract.dsl" AS contract_promote_vicreg_solo;

  FROM "iitepi.waves.dsl" IMPORT_WAVE eval_vicreg_promote_benchmark;

  BIND bind_promote_benchmark_candidate {
    __sampler = sequential;
    __workers = 0;
    __from = 01.09.2024;
    __to = 31.12.2024;
    __jk_profile_id = eval_payload_only;
    MOUNT {
      // replace this exact revision token while authoring one bind per candidate
      w_rep = EXACT 0x00FF;
    };
    CONTRACT = contract_promote_vicreg_solo;
    WAVE = eval_vicreg_promote_benchmark;
  };

  RUN bind_promote_benchmark_candidate;
}
