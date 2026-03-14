/*
  default.jkimyei.campaign.dsl
  ============================
  Purpose:
    Jkimyei campaign join layer. A campaign imports contract files and wave
    files, defines reusable wave-contract binds, then defines ordered campaigns
    over those bind ids.

  Syntax:
    ACTIVE_CAMPAIGN <campaign_id>;

    JKIMYEI_CAMPAIGN {
      IMPORT_CONTRACT_FILE "<contract_defaults_file>";
      IMPORT_WAVE_FILE "<wave_dsl_file>";

      BIND <binding_id> {
        __sampler = sequential;
        __workers = 0;
        __symbol = BTCUSDT;
        CONTRACT = <derived_contract_id>;
        WAVE = <wave_name_inside_imported_wave_files>;
      };

      CAMPAIGN <campaign_id> {
        BINDS = <bind_id>, <bind_id>, ...;
      };
    }

  Intent:
    - BIND is the reusable wave-contract execution unit.
    - CAMPAIGN is an ordered list of BIND refs.
    - No board abstraction appears here; this language is wave-contract level.
*/
ACTIVE_CAMPAIGN campaign_train_vicreg_primary;

JKIMYEI_CAMPAIGN {
  IMPORT_CONTRACT_FILE "default.iitepi.contract.dsl";

  IMPORT_WAVE_FILE "default.iitepi.wave.dsl";

  BIND bind_train_vicreg_primary {
    __sampler = sequential;
    __workers = 0;
    __symbol = BTCUSDT;
    CONTRACT = contract_default_iitepi;
    WAVE = train_vicreg_primary;
  };

  CAMPAIGN campaign_train_vicreg_primary {
    BINDS = bind_train_vicreg_primary;
  };
}
