/*
  default.iitepi.campaign.dsl
  ===========================
  Purpose:
    Unified top-level runtime dispatch language. A campaign imports contract
    files and wave files, defines reusable binds, then executes an ordered RUN
    sequence over those bind ids.

  Syntax:
    CAMPAIGN {
      IMPORT_CONTRACT_FILE "<contract_defaults_file>";
      IMPORT_WAVE_FILE "<wave_dsl_file>";

      BIND <binding_id> {
        // bind-local __variables scope only the selected wave graph
        // keep hard-static docking variables in the imported contract DSL
        __sampler = sequential;
        __workers = 0;
        __symbol = BTCUSDT;
        CONTRACT = <derived_contract_id>;
        WAVE = <wave_name_inside_imported_wave_files>;
      };

      RUN <binding_id>;
    }
*/
CAMPAIGN {
  IMPORT_CONTRACT_FILE "default.iitepi.contract.dsl";

  IMPORT_WAVE_FILE "default.iitepi.wave.dsl";

  BIND bind_train_vicreg_primary {
    // operational wave scope only; does not modify contract __variables
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    CONTRACT = contract_default_iitepi;
    WAVE = train_vicreg_primary;
  };

  RUN bind_train_vicreg_primary;
}
