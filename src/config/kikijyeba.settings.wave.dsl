/*
  kikijyeba.settings.wave.dsl
  ===========================
  Runtime wave settings for the current graph-first procedure.

  A wave is the runtime instruction surface that says how the procedure is
  being launched. In v1 it does not define topology or worker assemblies; it
  names the wave, declares mode flags, optionally declares the graph-wide source
  range to stream, and declares the Ujcamei cursor family used by runtime `.lls`
  reports. Reusable profiles should generally leave SOURCE_RANGE as all and let
  Runtime Hero apply a launch-time wave_overlay for concrete anchor/source-key
  ranges.

  Fields:
    WAVE_ID:
      Human-readable runtime wave name. It is copied into reports so generated
      artifacts can be tied back to the launch intent.

    COMPATIBLE_PROTOCOLS:
      Optional comma-separated protocol allow-list for this launch profile.
      This guards against running a reusable wave profile under the wrong
      static protocol. It does not select the architecture; `.config` selects
      the active `kikijyeba.protocol.*.dsl`.

    TARGET:
      Selects the focal Wikimyei component family for this wave. Upstream
      dependencies may still execute as part of the protocol topology, but only
      the target component is allowed to mutate in MODE=train.

      Supported values:
        wikimyei.representation.encoding.vicreg
          Target the channel-preserving node representation. In MODE=train this
          mutates VICReg; in MODE=run it executes the channel
          representation path forward only.

        wikimyei.inference.expected_value.mdn
          Target ExpectedValue. The representation encoder runs frozen because
          MDN consumes `[B,N,C,De]` context. In MODE=train only MDN mutates.

    MODE:
      One or more mode atoms, joined with "|".

      Supported atoms:
        run
          Execute the target dependency closure forward-only. No target weights
          are updated.

        train
          Execute the target dependency closure and update only TARGET. Upstream
          dependencies run frozen. `run` and `train` are mutually exclusive.

        debug
          Emit component runtime `.lls` reports for the wave batch. These
          reports are produced by the components themselves and are grounded by
          the Ujcamei cursor below. Debug mode does not belong in `.jkimyei`.

      Examples:
        MODE = run;
        MODE = run | debug;
        MODE = train | debug;

    SOURCE_CURSOR_KIND:
      graph_anchor
        The only v1 cursor kind. It means reports are grounded by the
        graph-anchor batch cursor emitted by the Ujcamei graph source.

    SOURCE_CURSOR_SCOPE:
      wave_batch
        The only v1 cursor scope. It means the report describes the current
        wave batch, not a global/latest training state.

    SOURCE_RANGE:
      all
        Stream the full graph-anchor domain yielded by the graph source.

      anchor_index
        Stream a graph-wide half-open anchor-index interval:
          [ANCHOR_INDEX_BEGIN, ANCHOR_INDEX_END)

        This range is not a single batch. The stream generator pulses batches
        through the requested interval until the interval is exhausted. The
        interval is over the accepted graph-anchor cursor after all graph/source
        coverage checks, so it applies to the whole active graph, not one edge.

      source_key
        Stream a graph-wide half-open accepted-anchor key interval:
          [SOURCE_KEY_BEGIN, SOURCE_KEY_END)

        This is the stable authoring form for source-keyed records such as
        kline millisecond close times. Runtime resolves the key interval to the
        accepted graph-anchor index domain before execution, then reports both
        requested source-key bounds and resolved anchor-index bounds. The alias
        `anchor_key` and fields `ANCHOR_KEY_BEGIN/END` are accepted for graph
        anchor terminology, but `source_key` is the canonical emitted policy.

    SOURCE_ORDER:
      If omitted, MODE=train defaults to random_per_epoch and MODE=run defaults
      to sequential. A train wave may explicitly request sequential for
      reproducible/debug runs, but Runtime emits a SOURCE_ORDER warning because
      stochastic graph-anchor train loading is disabled.

      sequential
        Yield graph anchors in canonical accepted-anchor order.

      random_per_epoch
        Use Torch RandomSampler over graph-anchor indices once per stream
        epoch, seeded from the target component's training SEED. Each selected
        anchor still fetches all graph edges together, preserving graph
        synchronization.

    ANCHOR_INDEX_BEGIN / ANCHOR_INDEX_END:
      Required when SOURCE_RANGE = anchor_index. Both are zero-based positions
      in the accepted graph-anchor domain, and END must be greater than BEGIN.

    SOURCE_KEY_BEGIN / SOURCE_KEY_END:
      Required when SOURCE_RANGE = source_key. Bounds are integral graph-anchor
      source-key values and use half-open semantics over accepted anchors.
*/
WAVE_SETTINGS {
  WAVE_ID = cwu_02v_channel_validation_eval_mdn_1800_2050;
  COMPATIBLE_PROTOCOLS = cwu_02v;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = run|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
};
