/*
  iitepi_board.dsl
  ==================
  Purpose:
    Board-level join layer. A board imports contract files and wave files, then
    defines executable bindings. Topology is not authored here.

  Syntax:
    BOARD {
      IMPORT_CONTRACT_FILE "<contract_config_file>";
      IMPORT_WAVE_FILE "<wave_config_file>";

      BIND <binding_id> {
        CONTRACT = <derived_contract_id>;
        WAVE = <wave_name_inside_imported_wave_files>;
      };
    }

  Contract id derivation:
    - Import path basename `default.board.contract.config` maps to
      `contract_default`.
    - Bind CONTRACT must reference this derived id.

  Wave selection semantics:
    - BIND.WAVE is the wave id inside imported wave DSL files.
    - If the same wave id exists in more than one imported wave file, binding is
      rejected as ambiguous.
*/

BOARD {
  IMPORT_CONTRACT_FILE "default.board.contract.config";

  IMPORT_WAVE_FILE "default.wave.config";

  BIND bind_train_vicreg {
    CONTRACT = contract_default;
    WAVE = train_vicreg_primary;
  };

  BIND bind_run_vicreg {
    CONTRACT = contract_default;
    WAVE = run_vicreg_embedding;
  };
}
