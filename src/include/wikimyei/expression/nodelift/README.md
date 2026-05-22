# Wikimyei Expression NodeLift

`wikimyei.expression.nodelift`

NodeLift is the deterministic expression that lifts edge observations into
node-state material while preserving edge residuals.

It is a Wikimyei expression because it turns Ujcamei evidence into the form that
Wikimyei workers can represent, train on, and reason from.

The implementation should follow the document:

- price coordinates by signed incidence recovery,
- synthetic reference gauge,
- coordinatewise node masks,
- price residual sidecars,
- activity coordinates by endpoint aggregation.

`srl/stream/srl_graph_adapter.h` provides the narrow bridge from
`kikijyeba::topology::graph::market_graph_t` into the current SRL graph metadata.
`srl/stream/node_lifted_stream.h` composes Ujcamei graph-anchor edge batches
with the SRL transform. Ujcamei still owns source formation and graph-collated
edge batches; NodeLift only consumes the stable graph order and endpoint
indices.
