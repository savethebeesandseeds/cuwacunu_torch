# Iinuji HTML synthetic chart inspector

This directory contains the dependency-free HTTP surface for inspecting the
`synthetic_continuous_graph_v1` source charts. The browser assets live in
`src/resources/iinuji/html/synthetic_charts/` and use plain HTML, CSS,
JavaScript, and Canvas. They do not load a CDN, vendor bundle, model, or
checkpoint.

The page is a data diagnostic, not a forecasting result. It displays the exact
served kline-shaped rows, one-step log returns, return autocorrelation, a
previous-return sign baseline, and catalog-declared split boundaries. It also
compares intervals by row index and wall clock so duplicated OHLC paths and
time-dilated timestamps can be seen directly. The time-dilation warning is
shown only when the selected instrument's catalog metadata reports identical
cross-interval OHLC rows.

## Build and validate

Run all build and validation commands in the `unnamed_taoist` container:

```powershell
docker exec unnamed_taoist bash -lc "cd /cuwacunu && make -C src/main/interface build-iinuji-html"
docker exec unnamed_taoist bash -lc "cd /cuwacunu && make -C src/main/interface validate-iinuji-html"
```

The binary is written to `.build/interface/iinuji_html`. The validation target
checks the pinned benchmark inputs and all three static assets without opening
a listening socket.

## Run

For access from the host browser, the container must have TCP port `8765`
published when it is created, and the server must listen on the container's
non-loopback interface. Check the current mapping from PowerShell:

```powershell
docker port unnamed_taoist 8765/tcp
```

If no mapping is printed, Docker cannot add one to the already-running
container. Recreate `unnamed_taoist` through the project's existing launcher
with the equivalent of `127.0.0.1:8765:8765` added to its port configuration;
keep the host side loopback-only.

With the port published, run:

```powershell
docker exec unnamed_taoist bash -lc "cd /cuwacunu && make -C src/main/interface run-iinuji-html IINUJI_HTML_BIND=0.0.0.0 IINUJI_HTML_PORT=8765"
```

Then open `http://127.0.0.1:8765/`. The server stays in the foreground and can
be stopped with `Ctrl+C`. For an in-container-only check, leave the default bind
address (`127.0.0.1`) and request the routes from inside the container.

The binary also accepts direct options:

```text
--repo-root PATH
--bind ADDRESS
--port PORT
--validate-only
--max-requests N
```

## Read-only endpoints

- `GET /` and `GET /index.html` return the inspector.
- `GET /app.css` returns its stylesheet.
- `GET /app.js` returns its client code.
- `GET /api/v1/catalog` returns the allowlisted instruments, intervals,
  sources, split boundaries, input lengths, and served-data boundary.
- `GET /api/v1/chart/<instrument>/<interval>` returns points in the declared
  `point_fields` order: anchor, open time, close time, OHLC, volume, and
  one-step log return.
- `HEAD` is supported for the same fixed routes.

There are no directory listings, arbitrary file routes, query parameters, or
write endpoints. Instrument and interval identifiers must come from the
catalog.

## Sealed-data boundary

The API serves anchors only through `served_anchor_end_exclusive` and reports
`test_holdout_served=false`. The browser rejects any chart point at or beyond
that boundary. The `test_holdout` split is shown as a boundary notice from the
catalog, but its rows are not returned, plotted, compared, or included in any
descriptive statistic.

Changing instrument, interval, axis, or comparison does not train or evaluate
a model. The ACF and sign-baseline cards are simple calculations over already
served rows; they do not establish unseen-future predictability.
