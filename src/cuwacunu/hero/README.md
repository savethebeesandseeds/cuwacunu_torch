# HERO Local MCP

`hero_config_mcp` is a local stdin/stdout MCP executable dedicated to
configuration workflows. It is compatible with Codex MCP clients over `stdio`.

Code split:
- `hero_config_mcp.cpp`: CLI/bootstrap loop only.
- `hero_config_store.{h,cpp}`: config load/save/validate model.
- `hero_config_commands.{h,cpp}`: command router + protocol output.

Build:

```bash
make -C /cuwacunu/src/cuwacunu/hero -j12 all
```

Run:

```bash
/cuwacunu/src/cuwacunu/build/hero_config_mcp --repl
```

Install standalone helper into `src/config`:

```bash
make -C /cuwacunu/src/cuwacunu/hero -j12 install-hero-mcp
```

Protocol layers:
- `protocol_layer=STDIO`: implemented.
  - default runtime mode (`hero_config_mcp` without `--repl`) is JSON-RPC over stdio.
  - supports MCP `Content-Length` framed messages (Codex-compatible).
  - also accepts one-JSON-object-per-line requests for local debugging.
- `protocol_layer=HTTPS/SSE`: intentionally not implemented; startup fails fast.

JSON-RPC stdio methods:
- `initialize`
- `notifications/initialized` (notification; no response)
- `ping`
- `tools/list`
- `tools/call` with tool names `hero.status`, `hero.schema`, `hero.show`,
  `hero.get`, `hero.set`, `hero.validate`, `hero.diff`, `hero.dry_run`,
  `hero.backups`, `hero.rollback`, `hero.save`, `hero.reload`

`tools/call` expects MCP shape:

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "hero.get",
    "arguments": { "key": "protocol_layer" }
  }
}
```

Diff preview via MCP:

```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "method": "tools/call",
  "params": {
    "name": "hero.diff",
    "arguments": { "include_text": false }
  }
}
```

`hero.diff` / `hero.dry_run` return:
- `has_changes`: any save-visible change
- `diff_count`: semantic key/value/type changes
- `format_only`: `true` when only canonical formatting would change

Rollback via MCP:

```json
{
  "jsonrpc": "2.0",
  "id": 7,
  "method": "tools/call",
  "params": {
    "name": "hero.rollback",
    "arguments": { "backup": "hero.config.dsl.bak.1772860000000000.dsl" }
  }
}
```

`hero.rollback` behavior:
- if `backup` is omitted, restores the latest backup.
- validates backup DSL syntax before restore.
- snapshots current config first (when backups are enabled).

Codex integration:
- register `/cuwacunu/src/cuwacunu/build/hero_config_mcp` as a local MCP server
  using `stdio` transport in your Codex MCP settings.
- do not pass `--repl` for MCP usage.

Legacy terminal mode:
- enable with `--repl` or one-shot with `--once`.
- input: one command per line
- output: tab-delimited records ending with `end`
  - `ok\t<command>\t<message>`
  - `err\t<command>\t<message>`
  - `data\t<key>\t<value>`

Core commands:
- `help`
- `status`
- `schema`
- `show`
- `get <key>`
- `set <key> <value>`
- `validate`
- `diff` (or `dry_run`)
- `backups`
- `rollback [backup_filename]`
- `save`
- `reload`

Backup policy:
- before each `save`, current config is snapshot to `backup_dir`.
- retention is capped by `backup_max_entries` (oldest pruned first).

Deterministic policy:
- runtime behavior is deterministic-only.
- `hero.ask` and `hero.fix` return an explicit "disabled in deterministic mode" error.
