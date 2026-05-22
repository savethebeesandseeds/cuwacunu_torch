# Kikijyeba Runtime

This folder contains the executable job layer.

Runtime does not implement source loading, component math, or optimizer loops.
It runs a wave against a compiled protocol contract. The runner writes a job
manifest, resolves the graph-anchor wave plan, delegates execution to the
Jkimyei representation or inference launchers, and writes a mutable job state.

The manifest records both sides of the launch:

- protocol contract identity: source/topology/assembly/dock/training
  compatibility fingerprint,
- wave identity: target component family, mode, and resolved graph-wide Ujcamei
  cursor range.

`job_runner_t` resolves the Jkimyei delegate from `TARGET` by default:
`wikimyei.representation.encoding.vicreg` runs the representation launcher,
while `wikimyei.inference.expected_value.mdn` runs the frozen-representation
node MDN launcher. `MODE=run` executes the target dependency closure without
optimizer steps; `MODE=train` mutates only the target component and runs
upstream dependencies frozen.

V1 deliberately does not implement resume. Resume requests fail fast after
configuration entry and before pipeline materialization.
