#!/usr/bin/env bash

set -eu

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../../" && pwd -P)"
cd "$repo_root"

baseline_archive='.build/tests/representation_srr2_v1_production_baseline.tar'
candidate_patch='.build/tests/representation_srr2_v1_candidate.patch'

baseline_paths=(
  'src/config/README.md'
  'src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl.bnf'
  'src/config/man/wikimyei.config.man'
  'src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl'
  'src/include/jkimyei/training/inference/channel_graph_first_inference_launcher.h'
  'src/include/kikijyeba/protocol/config_bundle.h'
  'src/include/wikimyei/README.md'
  'src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/README.md'
  'src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h'
  'src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h'
  'src/tests/bench/jkimyei/training/channel_graph_first_launchers/test_jkimyei_channel_graph_first_launchers.cpp'
  'src/tests/bench/wikimyei/config/graph_first_specs/test_wikimyei_graph_first_specs.cpp'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/Makefile'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/test_wikimyei_mtf_jepa_mae_vicreg.cpp'
)

new_candidate_paths=(
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PLAN.md'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.sha256'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.md'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.sha256'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/production_structured_readout_parity_gate.h'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity.cpp'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/run_production_structured_readout_parity_mechanics.sh'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/test_production_structured_readout.cpp'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/test_production_structured_readout_parity_gate.cpp'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/test_production_structured_readout_parity_log_auditor.cpp'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/verify_production_structured_readout_parity_candidate_patch.sh'
  'src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/write_production_structured_readout_parity_build_receipt.sh'
)

[[ "$(sha256sum -- "$baseline_archive" | cut -d' ' -f1)" == \
   '22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd' ]]
[[ "$(stat -c '%s' -- "$baseline_archive")" == '829440' ]]

mapfile -t archived_paths < <(tar -tf "$baseline_archive")
[[ "${#archived_paths[@]}" -eq "${#baseline_paths[@]}" ]]
for index in "${!baseline_paths[@]}"; do
  [[ "${archived_paths[$index]}" == "${baseline_paths[$index]}" ]]
done

stage_root="$(mktemp -d)"
case "$stage_root" in
  /tmp/*) ;;
  *) exit 2 ;;
esac
trap 'rm -rf -- "$stage_root"' EXIT

mkdir -p "$stage_root/a" "$stage_root/b" "$stage_root/applied"
tar -xf "$baseline_archive" -C "$stage_root/a"
tar -xf "$baseline_archive" -C "$stage_root/b"

copy_live() {
  local relative="$1"
  [[ -f "$relative" && ! -L "$relative" ]]
  mkdir -p "$stage_root/b/$(dirname "$relative")"
  cp -p -- "$relative" "$stage_root/b/$relative"
}

for relative in "${baseline_paths[@]}"; do
  copy_live "$relative"
done
for relative in "${new_candidate_paths[@]}"; do
  copy_live "$relative"
done

set +e
(
  cd "$stage_root"
  git diff --no-index --binary --no-ext-diff --no-color --no-prefix -- a b
) >"$stage_root/generated.patch"
diff_status=$?
set -e
[[ "$diff_status" -eq 1 && -s "$stage_root/generated.patch" ]]

cp -a "$stage_root/a/." "$stage_root/applied/"
(
  cd "$stage_root/applied"
  git apply --binary -p1 --whitespace=nowarn "$stage_root/generated.patch"
)
diff -qr --no-dereference "$stage_root/applied" "$stage_root/b" >/dev/null

mode="${1:---verify}"
case "$mode" in
  --write)
    cp -- "$stage_root/generated.patch" "$candidate_patch.tmp"
    mv -f -- "$candidate_patch.tmp" "$candidate_patch"
    ;;
  --verify)
    cmp -s -- "$stage_root/generated.patch" "$candidate_patch"
    ;;
  *)
    exit 2
    ;;
esac

printf '%s\n' 'schema=wikimyei.mtf_jepa_mae_vicreg.srr2_candidate_patch.v1'
printf 'srr2.candidate_patch.path=%s\n' "$candidate_patch"
printf 'srr2.candidate_patch.sha256=%s\n' \
  "$(sha256sum -- "$candidate_patch" | cut -d' ' -f1)"
printf 'srr2.candidate_patch.bytes=%s\n' \
  "$(stat -c '%s' -- "$candidate_patch")"
printf 'srr2.candidate_patch.baseline_entry_count=%s\n' \
  "${#baseline_paths[@]}"
printf 'srr2.candidate_patch.new_entry_count=%s\n' \
  "${#new_candidate_paths[@]}"
printf '%s\n' 'srr2.candidate_patch.apply_exact=true'
printf '%s\n' 'srr2.candidate_patch.live_tree_exact=true'
printf '%s\n' 'srr2.candidate_patch.pass=true'
