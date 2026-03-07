# HERO Local MCP

`hero_config_mcp` is a local stdin/stdout MCP-style executable dedicated to
configuration workflows.

HTTP/OpenAI transport is delegated to:
- `src/include/piaabo/https_compat/curl_toolkit/openai_responses_api.h`
- `src/impl/piaabo/https_compat/curl_toolkit/openai_responses_api.cpp`

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
- `protocol_layer=HTTPS/SSE`: intentionally not implemented; startup fails fast.

JSON-RPC stdio methods:
- `initialize`
- `ping`
- `tools/list`
- `tools/call` with tool names `hero.status`, `hero.schema`, `hero.show`,
  `hero.get`, `hero.set`, `hero.validate`, `hero.save`, `hero.reload`,
  `hero.ask`, `hero.fix`

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
- `save`
- `reload`
- `ask <prompt>` (OpenAI via curl)
- `fix <prompt>` (OpenAI via curl)

Backup policy:
- before each `save`, current config is snapshot to `backup_dir`.
- retention is capped by `backup_max_entries` (oldest pruned first).

Backend mode policy:
- `mode=openai` is active.
- `mode=selfhosted` fails fast with:
  `isufficient founds for self hosted model deployment, please change mode to openai.`
