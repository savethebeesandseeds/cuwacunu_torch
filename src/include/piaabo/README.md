# Piaabo

Piaabo is the quiet utility layer.

It contains tools, not meanings. If a file knows about market source identity,
worker objectives, runtime intent, or graph semantics, it probably does not
belong in clean Piaabo.

## Rooms

- `core/`: small string, hash, conversion, and time helpers.
- `log/`: logging and log buffering.
- `io/`: files, paths, CSV, and binary helpers.
- `parse/`: JSON and BNF/instruction parsing helpers.
- `security/`: encryption, secure memory, and key handling.
- `math/`: generic statistics and numeric helpers.
- `network/`: curl/websocket/HTTP helpers.
- `tensor/`: generic Torch helpers and distribution shims.
- `db/`: small embedded database/storage helpers.

The fresh rule is simple: Piaabo helps; it does not rule.

## Namespace Shape

C++ namespaces mirror the rooms: `cuwacunu::piaabo::core`,
`cuwacunu::piaabo::parse::bnf`, `cuwacunu::piaabo::tensor::torch`, and so on.
If code has market source identity, worker objectives, runtime intent, or graph
semantics, put it under its real owner in Ujcamei, Wikimyei, Jkimyei, or
Kikijyeba.
