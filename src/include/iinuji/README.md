# Iinuji

Iinuji is the terminal UI toolkit used by operator-facing command interfaces.

This migrated surface is intentionally toolkit-only:

- object tree, layout, events, and shared focus state in `iinuji_types.h`
- toolkit-level key codes in `iinuji_keys.h`
- generic text/color-token helpers in `iinuji_utils.h`
- ncurses color allocation helpers in `iinuji_color.h`
- primitive widget models and factories in `iinuji_primitives.h` and
  `primitives/`
- layout/render glue in `iinuji_render.h` and `render/`
- ncurses bootstrap, renderer, and key translation adapters in `ncurses/`
- reusable widgets and drawing primitives in `primitives/`

The legacy `iinuji_cmd` shell is not part of this migration pass. Future command
interfaces should build on this toolkit without depending on old `iitepi`
contract state, legacy Hero screens, or Config Hero policy internals.
