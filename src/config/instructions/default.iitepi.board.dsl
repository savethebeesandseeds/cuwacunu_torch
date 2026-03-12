/*
  default.iitepi.board.dsl
  ==================
  Purpose:
    Board-level join layer. A board imports contract files and wave files, then
    defines executable bindings. Topology is not authored here.

  Syntax:
    BOARD {
      IMPORT_CONTRACT_FILE "<contract_defaults_file>";
      IMPORT_WAVE_FILE "<wave_dsl_file>";

      BIND <binding_id> {
        CONTRACT = <derived_contract_id>;
        WAVE = <wave_name_inside_imported_wave_files>;
      };
    }

  Contract id derivation:
    - Import path basename `default.iitepi.contract.dsl` maps to
      `contract_default_iitepi`.
    - Bind CONTRACT must reference this derived id.

  Wave selection semantics:
    - BIND.WAVE is the wave id inside imported wave DSL files.
    - If the same wave id exists in more than one imported wave file, binding is
      rejected as ambiguous.
*/

BOARD {
  IMPORT_CONTRACT_FILE "default.iitepi.contract.dsl";

  IMPORT_WAVE_FILE "default.iitepi.wave.dsl";

  BIND bind_train_vicreg {
    CONTRACT = contract_default_iitepi;
    WAVE = train_vicreg_primary;
  };
}
