#!/usr/bin/env bash

set -eu

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../../" && pwd -P)"
cd "$repo_root"

printf '%s\n' 'schema=wikimyei.mtf_jepa_mae_vicreg.srr2_build_receipt.v1'
printf '%s\n' "build_command=docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg srr2-screen'"

record_binary() {
  local label="$1"
  local path="$2"
  [[ -f "$path" && -x "$path" ]]
  local bytes
  local digest
  bytes="$(stat -c '%s' -- "$path")"
  digest="$(sha256sum -- "$path")"
  digest="${digest%% *}"
  printf 'srr2.build.%s.path=%s\n' "$label" "$path"
  printf 'srr2.build.%s.bytes=%s\n' "$label" "$bytes"
  printf 'srr2.build.%s.sha256=%s\n' "$label" "$digest"
}

record_binary production .build/tests/test_production_structured_readout
record_binary shadow .build/tests/test_structured_readout_shadow
record_binary gate .build/tests/test_production_structured_readout_parity_gate
record_binary auditor .build/tests/test_production_structured_readout_parity_log_auditor
record_binary parity .build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity
record_binary core .build/tests/test_wikimyei_mtf_jepa_mae_vicreg
record_binary contracts .build/tests/test_wikimyei_mtf_jepa_mae_vicreg_contracts
record_binary config .build/tests/test_wikimyei_graph_first_specs
record_binary adapter .build/tests/test_jkimyei_channel_graph_first_launchers

printf '%s\n' 'srr2.build.binary_count=9'
printf '%s\n' 'srr2.build.pass=true'
