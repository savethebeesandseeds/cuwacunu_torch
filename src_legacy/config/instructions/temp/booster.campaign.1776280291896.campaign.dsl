/*
  default.iitepi.campaign.dsl
  ===========================
  Purpose:
    Canonical top-level runtime dispatch example.
    A campaign imports contracts and waves, defines reusable binds, then
    executes an ordered RUN sequence over those bind ids. Runtime Hero may also
    select one declared bind explicitly, with RUN remaining the default launch
    plan.

    Vocabulary:
      CONTRACT = compatibility + realization package
      WAVE     = runtime policy + required slots
      BIND     = one runnable contract/wave pairing

  Syntax:
    CAMPAIGN {
      IMPORT_CONTRACT "<contract_defaults_file>" AS <contract_alias>;
      FROM "<wave_dsl_file>" IMPORT_WAVE <wave_id>;

      BIND <binding_id> {
        // bind-local __variables scope only the selected wave graph
        // keep hard-static compatibility values in the imported contract DSL
        __sampler = sequential;
        __workers = 0;
        __symbol = BTCUSDT;
        CONTRACT = <contract_alias>;
        WAVE = <imported_wave_id>;
      };

      RUN <binding_id>;
    }

    Marshal Hero now starts from a marshal objective DSL, which points to the
    campaign plus separate human-authored objective and guidance files.
    The defaults bundle ships `default.marshal.objective.dsl`,
    `default.marshal.objective.md`, and `default.runtime.operative.guidance.md`, but the
    plain default campaign remains executable on its own through Runtime Hero.
*/
CAMPAIGN {
  IMPORT_CONTRACT "default.iitepi.contract.dsl" AS contract_default_iitepi;

  FROM "default.iitepi.wave.dsl" IMPORT_WAVE train_vicreg_primary;

  BIND bind_train_vicreg_primary {
    // operational wave scope only; does not modify contract DOCK/ASSEMBLY values
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    CONTRACT = contract_default_iitepi;
    WAVE = train_vicreg_primary;
  };

  RUN bind_train_vicreg_primary;
}
