# Kikijyeba Protocol Topology

Protocol topology identity and active market graph metadata live here.

- `node.h` names active asset nodes and node indices.
- `edge.h` names directed instrument edges and endpoint roles.
- `graph.h` turns staged directed instruments into stable node/edge order and
  endpoint indices.
- `graph_topology_spec.h` defines the decoded topology policy and active
  node/edge forms.
- `graph_topology_decoder.h` decodes the graph topology DSL into the active
  protocol graph used by graph-first runtime builders.

This room owns graph topology metadata for a protocol: active nodes `V`, active
directed edges `E`, stable node/edge order, endpoint maps `u(e)`/`v(e)`, and
graph-order identity. It does not own source registries, source retrieval,
representation models, inference heads, or NodeLift math. Source instrument
identity lives in `ujcamei/source/registry/instrument_signature.h`; topology
edge helpers only adapt it into a directed market edge when needed.

Ujcamei retrieval code consumes this topology when constructing graph-anchor
batches, but Ujcamei does not define protocol graph topology. Wikimyei workers
should consume this graph metadata instead of owning graph topology.
