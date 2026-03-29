#!/usr/bin/env bash
set -euo pipefail

pending_file=/cuwacunu/.runtime/.human_hero/awaiting_human.pending.count
report_pending_file=/cuwacunu/.runtime/.human_hero/finished.pending.count
legacy_pending_file=/cuwacunu/.runtime/.human_hero/pending.count

if [[ ! -r "${pending_file}" ]]; then
  pending_file="${legacy_pending_file}"
fi
pending_count=""
if [[ -r "${pending_file}" ]]; then
  IFS= read -r pending_count < "${pending_file}" || exit 0
fi
[[ -z "${pending_count}" || "${pending_count}" =~ ^[0-9]+$ ]] || exit 0

report_pending_count=""
if [[ -r "${report_pending_file}" ]]; then
  IFS= read -r report_pending_count < "${report_pending_file}" || exit 0
fi
[[ -z "${report_pending_count}" || "${report_pending_count}" =~ ^[0-9]+$ ]] || exit 0

pending_count=${pending_count:-0}
report_pending_count=${report_pending_count:-0}

if (( pending_count + report_pending_count > 0 )); then
  printf '[*]'
fi
