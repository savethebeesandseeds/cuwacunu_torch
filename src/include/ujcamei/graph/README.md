# Ujcamei Graph

Graph identity and active market graph metadata live here.

- `node.h` names active asset nodes and node indices.
- `edge.h` names directed instrument edges and endpoint roles.
- `graph.h` turns staged directed instruments into stable node/edge order and
  endpoint indices.

This room owns graph metadata: active nodes `V`, active directed edges `E`,
stable node/edge order, and endpoint maps `u(e)`/`v(e)`. It does not own source
contracts, representation models, or NodeLift math. Source instrument identity
lives in `ujcamei/source/instrument_signature.h`; graph edge helpers only adapt
it into a directed market edge when needed. Wikimyei workers should consume this
graph metadata instead of owning graph topology.
