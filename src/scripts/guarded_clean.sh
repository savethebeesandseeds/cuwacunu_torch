#!/usr/bin/env bash
set -euo pipefail

usage() {
  printf 'usage: %s <rm|shred|clean-tree> <build-root> [path ...]\n' "$0" >&2
  exit 2
}

canon() {
  realpath -m -- "$1"
}

guard_path() {
  local raw_path="$1"
  local abs_path

  abs_path="$(canon "$raw_path")"
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
BUILD_ROOT_ABS="$(canon "$1")"
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
  *)
    usage
    ;;
esac
