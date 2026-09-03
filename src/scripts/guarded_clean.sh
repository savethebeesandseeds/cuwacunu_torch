#!/usr/bin/env bash
set -euo pipefail

usage() {
  printf 'usage: %s <rm|shred|clean-tree|assert-output> <build-root> [path ...]\n' "$0" >&2
  exit 2
}

canon() {
  realpath -m -- "$1"
}

SCRIPT_PATH="$(realpath -e -- "${BASH_SOURCE[0]}")" || {
  printf '[CLEAN_GUARD] unable to resolve helper path: %s\n' \
    "${BASH_SOURCE[0]}" >&2
  exit 1
}
SCRIPT_DIR="${SCRIPT_PATH%/*}"
EXPECTED_PROJECT_ROOT="$(canon "$SCRIPT_DIR/../..")"
EXPECTED_BUILD_ROOT="$EXPECTED_PROJECT_ROOT/.build"
EXPECTED_EXTERNAL_ROOT="$EXPECTED_PROJECT_ROOT/.external"

validate_build_root() {
  local raw_root="$1"
  local resolved_root

  if [ "$raw_root" != "$EXPECTED_BUILD_ROOT" ]; then
    printf '[CLEAN_GUARD] refusing unexpected build root: %s (expected=%s)\n' \
      "$raw_root" "$EXPECTED_BUILD_ROOT" >&2
    return 1
  fi
  if [ -L "$EXPECTED_BUILD_ROOT" ]; then
    printf '[CLEAN_GUARD] refusing symlinked build root: %s\n' \
      "$EXPECTED_BUILD_ROOT" >&2
    return 1
  fi
  resolved_root="$(canon "$EXPECTED_BUILD_ROOT")"
  if [ "$resolved_root" != "$EXPECTED_BUILD_ROOT" ]; then
    printf '[CLEAN_GUARD] refusing misdirected build root: %s (resolved=%s)\n' \
      "$EXPECTED_BUILD_ROOT" "$resolved_root" >&2
    return 1
  fi
  case "$resolved_root" in
    "$EXPECTED_EXTERNAL_ROOT" | "$EXPECTED_EXTERNAL_ROOT"/*)
      printf '[CLEAN_GUARD] refusing external dependency tree as build root: %s\n' \
        "$resolved_root" >&2
      return 1
      ;;
  esac
  printf '%s\n' "$resolved_root"
}

guard_path() {
  local raw_path="$1"
  local abs_path

  abs_path="$(canon "$raw_path")"
  case "$abs_path" in
    "$EXPECTED_EXTERNAL_ROOT" | "$EXPECTED_EXTERNAL_ROOT"/*)
      printf '[CLEAN_GUARD] refusing external dependency path: %s\n' \
        "$abs_path" >&2
      return 1
      ;;
  esac
  case "$abs_path" in
    "$BUILD_ROOT_ABS" | "$BUILD_ROOT_ABS"/*)
      printf '%s\n' "$abs_path"
      ;;
    *)
      printf '[CLEAN_GUARD] refusing path outside build root: %s (root=%s)\n' \
        "$abs_path" "$BUILD_ROOT_ABS" >&2
      return 1
      ;;
  esac
}

delete_one() {
  local mode="$1"
  local raw_path="$2"
  local abs_path
  local link_count

  abs_path="$(guard_path "$raw_path")" || return 1
  if [ ! -e "$abs_path" ] && [ ! -L "$abs_path" ]; then
    return 0
  fi
  if [ -L "$abs_path" ]; then
    rm -f -- "$abs_path"
    return 0
  fi
  if [ -d "$abs_path" ]; then
    printf '[CLEAN_GUARD] refusing directory file-delete target: %s\n' "$abs_path" >&2
    return 1
  fi
  if [ "$mode" = "shred" ] && [ -f "$abs_path" ]; then
    link_count="$(stat -c '%h' -- "$abs_path")"
    if [ "$link_count" -ne 1 ]; then
      printf '[CLEAN_GUARD] refusing multiply-linked shred target: %s\n' \
        "$abs_path" >&2
      return 1
    fi
  fi

  case "$mode" in
    rm)
      rm -f -- "$abs_path"
      ;;
    shred)
      if [ -f "$abs_path" ] && [ -x "$abs_path" ]; then
        rm -f -- "$abs_path"
      else
        shred -u -- "$abs_path" 2>/dev/null || rm -f -- "$abs_path"
      fi
      ;;
    *)
      usage
      ;;
  esac
}

assert_output() {
  local raw_path="$1"
  local abs_path

  if [ -L "$raw_path" ]; then
    printf '[CLEAN_GUARD] refusing symlink output target: %s\n' "$raw_path" >&2
    return 1
  fi
  abs_path="$(guard_path "$raw_path")" || return 1
  if [ "$abs_path" = "$BUILD_ROOT_ABS" ]; then
    printf '[CLEAN_GUARD] refusing build-root output target: %s\n' "$abs_path" >&2
    return 1
  fi
  if [ -e "$abs_path" ] && [ ! -f "$abs_path" ]; then
    printf '[CLEAN_GUARD] refusing non-file output target: %s\n' "$abs_path" >&2
    return 1
  fi
  if [ -e "$abs_path" ] && [ "$(stat -c '%h' -- "$abs_path")" -ne 1 ]; then
    printf '[CLEAN_GUARD] refusing multiply-linked output target: %s\n' \
      "$abs_path" >&2
    return 1
  fi
  printf '%s\n' "$abs_path"
}

clean_tree() {
  local raw_dir="$1"
  local abs_dir

  abs_dir="$(guard_path "$raw_dir")" || return 1
  if [ ! -e "$abs_dir" ]; then
    return 0
  fi
  if [ ! -d "$abs_dir" ]; then
    printf '[CLEAN_GUARD] refusing non-directory tree clean target: %s\n' "$abs_dir" >&2
    return 1
  fi

  while IFS= read -r -d '' path; do
    delete_one shred "$path"
  done < <(find "$abs_dir" -type f -print0 2>/dev/null)

  find "$abs_dir" -depth -type d -empty -delete 2>/dev/null || true
}

[ "$#" -ge 2 ] || usage
MODE="$1"
shift
BUILD_ROOT_ABS="$(validate_build_root "$1")" || exit 1
shift

case "$BUILD_ROOT_ABS" in
  / | '')
    printf '[CLEAN_GUARD] refusing invalid build root: %s\n' "$BUILD_ROOT_ABS" >&2
    exit 1
    ;;
esac

case "$MODE" in
  rm | shred)
    for raw_path in "$@"; do
      delete_one "$MODE" "$raw_path"
    done
    ;;
  clean-tree)
    for raw_dir in "$@"; do
      clean_tree "$raw_dir"
    done
    ;;
  assert-output)
    for raw_path in "$@"; do
      assert_output "$raw_path" >/dev/null
    done
    ;;
  *)
    usage
    ;;
esac
