/*
  hero.runtime.probes.dsl
  ==========================
  Runtime probe catalog.

  Probes are visibility attachments for Runtime jobs. Enabling a probe
  here is not enough by itself: the selected Runtime wave must also include the
  debug atom in MODE. Probe streams are not Lattice facts, proof authority,
  dispatch authority, or replacements for reports/checkpoints/terminal facts.
*/
RUNTIME_PROBE {
  PROBE_ID = job_events;
  PROBE_KIND = job_events;
  ENABLED = true;
  RECORD_SCHEMA = kikijyeba.runtime.job_events.probe_record.v1;
  STREAM_LEAF = runtime.job_events.probe;
  EMIT_LIFECYCLE = true;
  EMIT_SCALAR_METRICS = true;
  EMIT_REPORT_METRICS = true;
  EMIT_ARTIFACTS = true;
  EMIT_WARNINGS = true;
};
