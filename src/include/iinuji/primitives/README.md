# Iinuji Primitives

Primitives are reusable UI building blocks. They own local widget state and
small behavior, but they do not know about command screens, Hero, config policy,
contracts, or application routing.

Every primitive should expose:

- a snake-case data model named `*_data_t`
- a factory option struct when construction has more than a trivial argument
- a `create_*` factory that sets `primitive_role_t` and `focus_mode_t`
- a key handler when the primitive accepts keyboard input
- render support in the matching `render/*_render.h` header when it draws as an
  object-tree widget

Keep primitive behavior predictable:

- use `standard_key_action()` instead of backend key constants
- use `primitive_key_result_t` or a derived result type for key handlers
- use `object_has_focus()` or `primitive_has_focus()` for focus checks
- keep ncurses-specific translation in `ncurses/`
- keep app-specific decisions outside `primitives/`

Compatibility aliases may remain while code is migrated, but new code should use
the snake-case data names and option-struct factories.
