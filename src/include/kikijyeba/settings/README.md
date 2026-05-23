# Kikijyeba Settings

This room holds small runtime settings for the active procedure.

The first surface is `wave.h`, decoded from `kikijyeba.settings.wave.dsl`.
It names the current wave, selects the Wikimyei family to run with
`TARGET`, declares mode flags such as `run | debug`, and declares the
graph-wide source range and Ujcamei cursor family used to ground runtime `.lls`
reports. A wave range is larger than a batch: the stream generator yields
graph-anchor batches until the requested range is exhausted.

`TARGET` is the focal component, not the whole execution chain. For example,
`TARGET=wikimyei.inference.expected_value.mdn` still runs NodeLift and a frozen
VICReg encoder because MDN depends on node encodings. `MODE=train` mutates only
the target; `MODE=run` applies no optimizer step. `debug` is only a report
modifier and may be combined with either primary mode.

The channel-preserving path uses
`TARGET=wikimyei.representation.encoding.vicreg` for VICReg and
`TARGET=wikimyei.inference.expected_value.mdn` for the strict channel
MDN. Channel MDN keeps VICReg frozen in `MODE=train`.

Wave settings are not topology and are not training policy. Topology lives in
`kikijyeba/topology/graph`; training policy lives in `.jkimyei` files consumed by
Jkimyei.
