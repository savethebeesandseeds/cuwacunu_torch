/*
  kikijyeba.settings.wave.dsl
  ===========================
  Runtime wave settings for the current graph-first procedure.

  A wave is the runtime instruction surface that says how the procedure is
  being launched. In v1 it does not define topology or worker assemblies; it
  names the wave, declares mode flags, declares the graph-wide source range to
  stream, and declares the Ujcamei cursor family used by runtime `.lls`
  reports.

  Fields:
    WAVE_ID:
      Human-readable runtime wave name. It is copied into reports so generated
      artifacts can be tied back to the launch intent.

    TARGET:
      Selects the focal Wikimyei component family for this wave. Upstream
      dependencies may still execute as part of the protocol topology, but only
      the target component is allowed to mutate in MODE=train.

      Supported values:
        wikimyei.representation.encoding.vicreg
          Target the node representation. In MODE=train this mutates VICReg;
          in MODE=run it executes the representation path forward only.

        wikimyei.inference.expected_value.mdn
          Target node ExpectedValue. The representation encoder still runs as a
          frozen dependency because MDN needs node encodings. In MODE=train only
          the MDN heads mutate; in MODE=run no optimizer step is applied.

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

    ANCHOR_INDEX_BEGIN / ANCHOR_INDEX_END:
      Required when SOURCE_RANGE = anchor_index. Both are zero-based positions
      in the accepted graph-anchor domain, and END must be greater than BEGIN.
*/
WAVE_SETTINGS {
  WAVE_ID = cwu_01v_validation_eval_mdn_1800_2050;
  TARGET = wikimyei.inference.expected_value.mdn;
  MODE = run|debug;
  SOURCE_CURSOR_KIND = graph_anchor;
  SOURCE_CURSOR_SCOPE = wave_batch;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 1800;
  ANCHOR_INDEX_END = 2050;
};
