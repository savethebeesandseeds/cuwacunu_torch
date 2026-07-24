# Iinuji

Iinuji is the interface toolkit used by cuwacunu command surfaces. Its primary
surface remains the terminal UI, with a bounded HTML surface for read-only
scientific inspection.

This surface contains both the reusable toolkit primitives and the
`cuwacunu_cmd` command shell:

- object tree, layout, events, and shared focus state in `iinuji_types.h`
- toolkit-level key codes in `iinuji_keys.h`
- generic text/color-token helpers in `iinuji_utils.h`
- ncurses color allocation helpers in `iinuji_color.h`
- primitive widget models and factories in `iinuji_primitives.h` and
  `primitives/`
- layout/render glue in `iinuji_render.h` and `render/`
- ncurses bootstrap, renderer, and key translation adapters in `ncurses/`
- reusable widgets and drawing primitives in `primitives/`
- command UI state, routing, views, and Home assets in `iinuji_cmd/`
- the small fixed-route HTTP backend in `html/`
- offline HTML assets under `src/resources/iinuji/html/`

## Command Shell

The primary installed command is `cuwacunu_cmd`. Additional shell aliases
`cuwacunu.cmd` and `iinuji_cmd` point to the same interface binary when
installed with:

```bash
make -C src/main/interface AUTO_PREP=0 install-cuwacunu-cmd
```

The restored top-level screen order is:

- `F1` Home: animated waajacamaya/Cuwacunu showcase and splash diagnostics.
- `F2` Workbench: core workspace; content is modeled separately from the
  restored chrome.
- `F3` Runtime: runtime device, job, manifest, log, and trace evidence.
- `F4` Lattice: target catalog and proof preview.
- `F8` Shell Logs: command/status feed and log control deck.
- `F9` Config: managed config catalog and read-only preview.

Use the Workbench name for F2 in visible navigation, menus, help, and catalog
output. Historical F2 aliases remain internal to command routing.

## HTML Synthetic Chart Inspector

`iinuji_html` serves the canonical `synthetic_continuous_graph_v1` CSVs through
a fixed, read-only API and an offline chart interface. It is intentionally not
a general file server and does not attempt to port `cuwacunu_cmd` to the DOM.

Build and validate it with:

```bash
make -C src/main/interface AUTO_PREP=0 build-iinuji-html
make -C src/main/interface AUTO_PREP=0 validate-iinuji-html
```

The default server binds to `127.0.0.1:8765`. The API omits the benchmark test
holdout and accepts only the known instrument and interval identifiers. See
[`html/README.md`](./html/README.md) for routes and Docker browser access.
