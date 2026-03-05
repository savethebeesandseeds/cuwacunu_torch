#!/usr/bin/env bash
set -euo pipefail

COLOR_DIM_YELLOW=$'\033[2;33m'
COLOR_GRAY=$'\033[90m'
COLOR_MAHOGANY_RED=$'\033[38;2;128;40;30m'
COLOR_RESET=$'\033[0m'

depfile="${1:?depfile path required}"
binary="${2:?binary path required}"
objfile="${3:?obj path required}"
name="${4:-$(basename "$binary")}"

if [[ ! -f "$depfile" ]]; then
  exit 0
fi

if [[ ! -e "$binary" && ! -e "$objfile" ]]; then
  exit 0
fi

# Depfiles from -MMD/-MP can span multiple lines with trailing backslashes.
deps="$(
  sed -e ':a' -e 'N' -e '$!ba' -e 's/\\\n/ /g' "$depfile" \
  | sed -E 's/^[^:]+:[[:space:]]*//'
)"

need_clear=0
for f in $deps; do
  [[ "$f" == "\\" ]] && continue
  [[ -e "$f" ]] || continue

  if [[ -e "$binary" && "$f" -nt "$binary" ]]; then
    need_clear=1
    break
  fi

  if [[ ! -e "$binary" && -e "$objfile" && "$f" -nt "$objfile" ]]; then
    need_clear=1
    break
  fi
done

if [[ "$need_clear" -eq 1 ]]; then
  rm -f "$binary" "$objfile"
  printf "%s[%sCLR:  %sSTALE%s]%s %s\n" \
    "$COLOR_DIM_YELLOW" \
    "$COLOR_GRAY" \
    "$COLOR_MAHOGANY_RED" \
    "$COLOR_DIM_YELLOW" \
    "$COLOR_RESET" \
    "$name"
fi
