#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --project-root PATH --binary PATH --manifest PATH --depfile PATH --compiler PROGRAM --linker PROGRAM --compile-command TEXT --dependency-command TEXT --link-command TEXT --extra-inputs 'PATH ...'" >&2
  exit 64
}

project_root=
binary=
manifest=
depfile=
compiler=
linker=
compile_command=
dependency_command=
link_command=
extra_inputs=

while (($#)); do
  case "$1" in
  --project-root) project_root=${2-}; shift 2 ;;
  --binary) binary=${2-}; shift 2 ;;
  --manifest) manifest=${2-}; shift 2 ;;
  --depfile) depfile=${2-}; shift 2 ;;
  --compiler) compiler=${2-}; shift 2 ;;
  --linker) linker=${2-}; shift 2 ;;
  --compile-command) compile_command=${2-}; shift 2 ;;
  --dependency-command) dependency_command=${2-}; shift 2 ;;
  --link-command) link_command=${2-}; shift 2 ;;
  --extra-inputs) extra_inputs=${2-}; shift 2 ;;
  *) usage ;;
  esac
done

for value in project_root binary manifest depfile compiler linker; do
  [[ -n ${!value} ]] || usage
done

project_root=$(readlink -f "$project_root")
binary=$(readlink -f "$binary")
depfile=$(readlink -f "$depfile")
manifest_dir=$(dirname "$manifest")
manifest_name=$(basename "$manifest")
sidecar="$manifest.sha256"
sidecar_name=$(basename "$sidecar")
mkdir -p "$manifest_dir"
manifest_dir=$(readlink -f "$manifest_dir")
manifest="$manifest_dir/$manifest_name"
sidecar="$manifest_dir/$sidecar_name"

[[ -d $project_root && -x $binary && -s $depfile ]]

nonce="$$.$RANDOM"
raw_manifest="$manifest.tmp.raw.$nonce"
candidate_manifest="$manifest.tmp.$nonce"
candidate_sidecar="$sidecar.tmp.$nonce"
repo_paths="$manifest.tmp.repo_paths.$nonce"
repo_records="$manifest.tmp.repo_records.$nonce"
dso_output="$manifest.tmp.ldd.$nonce"
dso_paths="$manifest.tmp.dso_paths.$nonce"
dso_records="$manifest.tmp.dso_records.$nonce"
tool_records="$manifest.tmp.tool_records.$nonce"
cleanup() {
  rm -f "$raw_manifest" "$candidate_manifest" "$candidate_sidecar" \
    "$repo_paths" "$repo_records" "$dso_output" "$dso_paths" \
    "$dso_records" "$tool_records"
}
trap cleanup EXIT HUP INT TERM

resolve_program() {
  local requested=$1 located
  if [[ $requested == */* ]]; then
    located=$requested
  else
    located=$(command -v "$requested")
  fi
  readlink -f "$located"
}

compiler_path=$(resolve_program "$compiler")
linker_path=$(resolve_program "$linker")
cc1plus_path=$(resolve_program "$($compiler -print-prog-name=cc1plus)")
collect2_path=$(resolve_program "$($linker -print-prog-name=collect2)")
ld_path=$(resolve_program "$($linker -print-prog-name=ld)")
assembler_path=$(resolve_program "$($compiler -print-prog-name=as)")
make_path=$(resolve_program make)
sha256sum_path=$(resolve_program sha256sum)
ldd_path=$(resolve_program ldd)

: >"$tool_records"
for specification in \
  "compiler_driver:$compiler_path" \
  "linker_driver:$linker_path" \
  "cc1plus:$cc1plus_path" \
  "collect2:$collect2_path" \
  "ld:$ld_path" \
  "assembler:$assembler_path" \
  "make:$make_path" \
  "sha256sum:$sha256sum_path" \
  "ldd:$ldd_path"; do
  name=${specification%%:*}
  path=${specification#*:}
  [[ -x $path ]]
  digest=$(sha256sum "$path" | awk '{print $1}')
  printf 'tool:%s:path=%s\n' "$name" "$path" >>"$tool_records"
  printf 'tool:%s:sha256=%s\n' "$name" "$digest" >>"$tool_records"
done

# GNU compiler depfiles use backslash-newline continuation. Repository paths in
# this project contain no spaces, so one dependency token per line is lossless.
sed -e 's/\\$//' -e '1s/^[^:]*://' "$depfile" | tr ' ' '\n' | \
  sed -e '/^$/d' >"$repo_paths"
for input in $extra_inputs; do
  printf '%s\n' "$input" >>"$repo_paths"
done

: >"$repo_records"
while IFS= read -r dependency; do
  resolved=$(readlink -f "$dependency")
  [[ -f $resolved ]]
  case "$resolved" in
  "$project_root"/*)
    logical="\${REPO}/${resolved#"$project_root"/}"
    digest=$(sha256sum "$resolved" | awk '{print $1}')
    printf 'repo_dependency:%s=%s\n' "$logical" "$digest" >>"$repo_records"
    ;;
  esac
done <"$repo_paths"
LC_ALL=C sort -u -o "$repo_records" "$repo_records"
repo_count=$(wc -l <"$repo_records")
((repo_count > 0))

ldd "$binary" >"$dso_output"
if grep -Fq 'not found' "$dso_output"; then
  echo "VVA-1B binary has an unresolved DSO" >&2
  exit 66
fi
awk '$2 == "=>" && $3 ~ /^\// { print $3 } $1 ~ /^\// { print $1 }' \
  "$dso_output" >"$dso_paths"
LC_ALL=C sort -u -o "$dso_paths" "$dso_paths"
: >"$dso_records"
while IFS= read -r dso; do
  resolved=$(readlink -f "$dso")
  [[ -f $resolved ]]
  case "$resolved" in
  "$project_root"/*) logical="\${REPO}/${resolved#"$project_root"/}" ;;
  *) logical="host:$resolved" ;;
  esac
  digest=$(sha256sum "$resolved" | awk '{print $1}')
  printf 'dso:%s=%s\n' "$logical" "$digest" >>"$dso_records"
done <"$dso_paths"
LC_ALL=C sort -u -o "$dso_records" "$dso_records"
dso_count=$(wc -l <"$dso_records")
((dso_count > 0))

binary_digest=$(sha256sum "$binary" | awk '{print $1}')
compiler_version=$($compiler --version | sed -n '1p')
linker_version=$($linker --version | sed -n '1p')
{
  printf '%s\n' 'schema=vva1b.transitive_build_manifest.v1'
  printf '%s\n' 'workspace.logical_root=${REPO}'
  printf 'compiler.version=%s\n' "$compiler_version"
  printf 'linker_driver.version=%s\n' "$linker_version"
  printf 'compile.command=%s\n' "$compile_command"
  printf 'dependency_scan.command=%s\n' "$dependency_command"
  printf 'link.command=%s\n' "$link_command"
  cat "$tool_records"
  cat "$repo_records"
  printf 'repo_dependency.count=%s\n' "$repo_count"
  cat "$dso_records"
  printf 'dso.count=%s\n' "$dso_count"
  printf 'binary:${REPO}/%s=%s\n' "${binary#"$project_root"/}" \
    "$binary_digest"
} >"$raw_manifest"

sed "s#$project_root/#\${REPO}/#g" "$raw_manifest" >"$candidate_manifest"
[[ -s $candidate_manifest ]]
candidate_digest=$(sha256sum "$candidate_manifest" | awk '{print $1}')
printf '%s  %s\n' "$candidate_digest" "$manifest_name" >"$candidate_sidecar"

if [[ -e $sidecar && ! -f $manifest ]]; then
  echo "VVA-1B build manifest has a sidecar without its immutable data" >&2
  exit 75
fi
if [[ -e $manifest ]]; then
  if [[ ! -f $manifest ]]; then
    echo "VVA-1B build manifest final path is not a regular file" >&2
    exit 75
  fi
  if ! cmp -s "$candidate_manifest" "$manifest"; then
    echo "VVA-1B immutable build manifest differs from the current build" >&2
    exit 74
  fi
  if [[ ! -e $sidecar ]]; then
    # A crash may occur after the data link and before the checksum link. The
    # exact candidate comparison above proves the complete data is the current
    # build before the missing commit marker is reconstructed.
    ln "$candidate_sidecar" "$sidecar"
    rm -f "$candidate_sidecar"
  elif [[ ! -f $sidecar ]]; then
    echo "VVA-1B build manifest sidecar path is not a regular file" >&2
    exit 75
  fi
  (cd "$manifest_dir" && sha256sum -c "$sidecar_name")
  exit 0
fi

# The exclusive Makefile lock prevents another VVA-1B publisher. Both final
# names are populated only from fully written same-directory temporary files;
# the checksum sidecar is the commit marker.
ln "$candidate_manifest" "$manifest"
rm -f "$candidate_manifest"
ln "$candidate_sidecar" "$sidecar"
rm -f "$candidate_sidecar"
(cd "$manifest_dir" && sha256sum -c "$sidecar_name")
trap - EXIT HUP INT TERM
cleanup
