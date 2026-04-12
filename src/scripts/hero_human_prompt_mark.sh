#!/usr/bin/env bash
set -euo pipefail

pending_file=/cuwacunu/.runtime/.human_hero/request.pending.count
summary_pending_file=/cuwacunu/.runtime/.human_hero/summary.pending.count
pending_count=""
if [[ -r "${pending_file}" ]]; then
  IFS= read -r pending_count < "${pending_file}" || exit 0
fi
[[ -z "${pending_count}" || "${pending_count}" =~ ^[0-9]+$ ]] || exit 0

summary_pending_count=""
if [[ -r "${summary_pending_file}" ]]; then
  IFS= read -r summary_pending_count < "${summary_pending_file}" || exit 0
fi
[[ -z "${summary_pending_count}" || "${summary_pending_count}" =~ ^[0-9]+$ ]] || exit 0

pending_count=${pending_count:-0}
summary_pending_count=${summary_pending_count:-0}

if (( pending_count + summary_pending_count > 0 )); then
  printf '[*]'
fi
