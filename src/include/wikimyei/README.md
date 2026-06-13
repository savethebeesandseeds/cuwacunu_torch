# Wikimyei

Wikimyei is the worker pillar. The fresh graph-first path keeps only the worker
surfaces that are actively used by the node-centered pipeline:

- `assembly.h`
- `expression/nodelift/`
- `representation/encoding/vicreg/`
- `representation/encoding/mtf_jepa_mae_vicreg/`
- `representation/time_mae/`
- `inference/expected_value/mdn/`
- `observer/belief/`
- `observer/utility/`
- `policy/portfolio/`

Ujcamei is intentionally not nested under Wikimyei. Wikimyei consumes input or
source contracts; Ujcamei owns that boundary.

NodeLift belongs under `wikimyei.expression.nodelift`: it is not source input,
and it is not a learned representation. It is the deterministic expression that
turns edge evidence into node-state material for workers.

Every active Wikimyei should expose an assembly: the static compatibility
surface that declares its family, component id, trainability, consumed stream
docks, produced stream docks, and constraints. Assembly is not runtime state and
it is not training policy. Runtime evidence belongs to `.lls`; Jkimyei `.jkimyei`
files describe how a trainable assembly is trained.

Assembly docks may name symbolic dimensions such as `N`, `L`, `C`, `Hx`, `Hf`,
`F`, `De`, `Df`, and `K`. Those are not old global DSL variables. Kikijyeba
topology resolves them into a `dock_binding` from the loaded source, topology,
retrieval, and network specs, then records the resulting binding token in stream
plans and `.lls` runtime reports.

Fresh Wikimyei C++ namespaces mirror the room path:

- `cuwacunu::wikimyei::assembly`
- `cuwacunu::wikimyei::representation::encoding::vicreg`
- `cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg`
- `cuwacunu::wikimyei::inference::expected_value`
- `cuwacunu::wikimyei::inference::expected_value::mdn`
- `cuwacunu::wikimyei::observer`
- `cuwacunu::wikimyei::observer::belief`
- `cuwacunu::wikimyei::policy::portfolio`
- `cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility`

The active VICReg surface is the strict channel-preserving representation path:
feature-level NodeLift masks enter the channel node adapter, the encoder keeps
`C` through `[M,C,Hx,De]`, temporal reduction is mask-aware inside the
representation boundary, and graph export emits `[B,N,C,De]`. Legacy fused/node
VICReg headers have been removed from the active source tree. Historical lattice
receipts that mention the old node representation job remain audit data only.
The legacy node MDN path has been removed; the active MDN surface is the
channel-context MDN.

`representation/encoding/mtf_jepa_mae_vicreg/` is a separate experimental
representation family for multi-scale time/frequency JEPA-MAE pretraining with
VICReg-style stabilization. It does not replace the active VICReg production
path, and it keeps its stability head/loss local rather than including the
production VICReg implementation.
`representation/time_mae/` remains reserved for a standalone TimeMAE family.

The matching implementation subtree lives under `src/impl/wikimyei` and keeps
Makefiles rather than README files.

The observer path is the deterministic bridge from the current channel-context
MDN into portfolio-ready belief. `observer::belief` exposes its own deterministic
Wikimyei assembly, while `observer/utility/` contains the individual observer
helpers in the single `cuwacunu::wikimyei::observer` namespace. The observer
treats the MDN output as marginal future NodeLift potentials, not direct asset
returns. Valid channels are collapsed explicitly, the NodeLift potential-surface
utility exposes the future price-coordinate potential mixture, and the NodeLift
return-projection utility samples target nodes plus the projection reference node
before computing numeraire-relative log returns:
`phi_asset - phi_reference`. Only after that projection are values converted to
arithmetic-return scenarios. The builder then emits both a
`observer::belief::NodeLiftPotentialBelief` and a single-anchor
`observer::belief::AllocationBelief`.
`AllocationBelief::node_ids` carries the ordered target graph-node universe.
`BasePolicy` separates the accounting numeraire, settlement asset, and NodeLift
projection reference. V1 normally binds all three to the same USD-like node,
but the contract keeps the meanings separate.
`AllocationBelief` is marked projection-validation-required and live-capital
disabled by default; the projected-return bridge is therefore explicit research
state until empirical checks compare `phi_asset - phi_reference` against
realized tradable numeraire-denominated returns.
`observer::belief::build_single_anchor_allocation_belief` composes the
observer chain from `MdnOut`, graph metadata, target-node mapping, a projection
reference, and realized/shrunk potential correlation ordered as target nodes
then the projection reference. Graph node identity is carried by
`observer::belief::GraphNodeAxisBinding`: the dock says there is a graph-node
axis, while the binding says which node id lives at each graph coordinate.
Feature-coordinate meaning is carried separately through observer feature
semantics utilities, which resolve known kline dock coordinate spaces to the
Ujcamei kline feature registry and record a stable feature-semantics fingerprint
on allocation beliefs and forecast artifacts.
When upstream inference has `B > 1`,
`observer::belief::build_allocation_belief_batch` requires explicit anchor keys
and timestamps for every anchor and returns an `AllocationBeliefBatch`; live
portfolio optimization still selects one decision anchor explicitly.
`observer::belief::collate_allocation_belief_batch` provides a guarded batch view
for reports and backtests by stacking compatible single-anchor beliefs only when
the risky universe, base policy, graph order, units, horizon, return origin, and
scenario count match.
The observer value-projection utility owns affine de-normalization and support
checks for target coordinates, and the observer volatility utility blends
MDN-implied variance with realized variance before portfolio risk uses it. The
observer scenario-bank utility stores the base joint scenario matrix and
deterministic stress families for high-correlation, volatility-inflated,
left-tail-shifted, and tail-thickened variants. The belief builder returns that
scenario bank next to the single-anchor `AllocationBelief`, while
`AllocationBelief::scenarios` remains the base arithmetic-return scenario
matrix consumed by the allocator.
The observer forecast-artifact utility persists the single-anchor marginal
forecast and belief tensors as a Torch archive, including target-coordinate and
normalization fingerprints, so later realized returns can
drive surprise and calibration observers instead of relying on text reports. It
also persists scenario-bank stress families when supplied by the builder result.
`observer::belief::apply_surprise` and `observer::belief::apply_calibration`
copy those realized health signals back into a single-anchor
`AllocationBelief` and adjust confidence caps without mutating the return
forecast itself.
`observer::compute_nodelift_residual_quality` summarizes whether NodeLift
residual energy is small enough to trust potential-only projection, and
`observer::validate_projected_log_return` compares projected log-return
scenarios with realized log returns through MAE, bias, directional accuracy,
correlation, interval coverage, and per-node scores.

The first deterministic portfolio method and current reference baseline is
`policy::portfolio::spot_distributional_utility`, rooted on disk at
`policy/portfolio/spot_distributional_utility/`. Its deterministic Wikimyei
assembly consumes a post-projection `AllocationBelief` dock and produces an
allocation target dock. The method is a long-only, graph-node-weight, spot-only
V1 allocator over correlated arithmetic-return scenarios. It consumes
`AllocationBelief`, `PortfolioState`, `MarketState`, and
`PortfolioConstraints`, and falls back through
`policy::portfolio::spot_distributional_utility::allocation_numeraire_fallback` when
belief quality or scenario feasibility fails.
`PortfolioState` names the accounting numeraire graph node explicitly with
`accounting_numeraire_node_id`; the node is part of the same `node_ids` and
weight/unit tensors as every other target node.
`policy::portfolio::spot_distributional_utility::decision_step` is the thin
operational wrapper for that method: it evaluates method-local
`risk::risk_gate`, selects either the main allocator or method-local
`allocation_numeraire_fallback`, routes the resulting target through method-local
`execution::spot_rebalance_router`, and emits a monitoring report for the full
decision step.
Diagnostic and fallback allocators are now method-local support modules under
`policy/portfolio/spot_distributional_utility/utility/`. They do not define
additional policy methods; they share the same confidence, capacity,
accounting-numeraire, turnover, and tradability constraints as the main method.
The utility folder also
contains the method-local execution router, risk gate, ledger, reporter,
numeraire fallback, and solver helpers, so the method does not sprawl into
one-file support folders.

Execution remains a separate phase inside the one method boundary. The V1
`spot_distributional_utility::execution::spot_rebalance_router` consumes a valid
target portfolio, current portfolio state, and Kikijyeba `market_graph_t` edges,
then emits rebalance order intents against direct spot markets. It does not
mutate account state or pretend allocation and execution are the same step.
Its edge-market validator rejects malformed execution data before routing:
nonpositive edge prices, negative fees/spreads/slippage/notionals, non-bool edge
tradability masks, and max notionals below minimum notionals.

Accounting, monitoring, and risk are also method-local support phases. The V1
`spot_distributional_utility::accounting::portfolio_ledger` applies executed
spot fills to graph-node units, numeraire-valued fees, realized PnL,
unrealized PnL, and current weights.
`spot_distributional_utility::monitoring::belief_reporter` emits deterministic
decision reports that tie belief, portfolio state, target weights, rebalance
intents, ledger state, surprise, and calibration into one inspectable text
artifact.
`spot_distributional_utility::risk::risk_gate` evaluates belief validity, data
quality, tradable coverage, confidence, liquidity, drawdown, surprise,
calibration, and correlation shock thresholds before allocation/execution can
proceed, and can route directly into `allocation_numeraire_fallback`.
The observer data-quality utility is intentionally stricter than shape validation: it
fails closed on non-finite tensors, invalid confidence/liquidity/capacity
bounds, impossible arithmetic-return scenarios, non-PSD covariance, and invalid
correlation matrices.
Portfolio-side validators also fail closed before solver entry on invalid
current graph-node weights, negative units, bad market prices/costs/notionals,
and infeasible or negative constraints.
`TargetPortfolio` validation enforces long-only graph-node weights, accounting
numeraire membership, finite diagnostics scalars, and delta/turnover consistency
against current weights before execution routing creates order intents.

The bounded PPO V0 graph-node allocation policy component contract is
`policy::portfolio::graph_node_allocation`. It is rooted on disk at
`policy/portfolio/graph_node_allocation/` and backed by authored
`wikimyei.policy.portfolio.graph_node_allocation.{dsl,net,features.dsl,jkimyei}`
files.
This assembly consumes `kikijyeba.environment.policy_input.v1` rather than raw
MDN tensors or the full environment observation. Its feature manifest freezes
the actor-visible node/global/risk feature names, including the ordinary
accounting-numeraire node flag and compact cross-node risk block. The net
contract names shared node/global/risk encoders, mask-aware pooling, separate
policy/value heads, raw `node_weight_logits[A]`, and the `state_value` critic
head for PPO update evidence. The V0 Torch module lives at
`policy/portfolio/graph_node_allocation/torch_policy_module.h` and consumes the
manifest-bound `node_features[A,28]`, `global_features[6]`, and
`risk_features[10]` tensors. The same header owns the V0 checkpoint state
helpers for `module_state.pt`, which serialize the module parameters plus the
bounded PPO V0 head overlay used before full autograd PPO exists. Kikijyeba
adapts policy output into one unified `target_node_weights[A]` action. The
accounting numeraire remains an ordinary graph node in that action universe,
not a separate cash output. Runtime owns PPO execution and keeps it
replay/paper-only.
