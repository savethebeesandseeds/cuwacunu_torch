# HERO Config

Canonical HERO config surface lives in this folder.

- `hero.config.h`: typed profile catalog + embedded config text templates.
- `hero.config.def`: X-macro registry for profile ids, policy rules, and future learning targets.
- Runtime DSL reference file: `src/config/instructions/hero.config.dsl`
- Runtime MAN spec: `src/config/man/hero.config.man`

Current runtime profile set:
- `hero.config.ask`
- `hero.config.fix`

Declared future learning targets (not wired yet):
- `tsiemene/tsi.probe`
- `piaabo/torch_compat/network_analytics`
- `piaabo/math_compat/statistics_space`

Runtime policy:
- deterministic execution only.
- `hero.ask` and `hero.fix` are disabled in runtime and return explicit errors.
