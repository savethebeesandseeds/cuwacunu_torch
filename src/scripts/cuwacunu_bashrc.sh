#!/usr/bin/env bash

# Repo-owned bashrc extension for cuwacunu shell behavior.

cuwacunu_prepend_path_once() {
  local dir="$1"
  case ":$PATH:" in
    *:"${dir}":*) ;;
    *) export PATH="${dir}:$PATH" ;;
  esac
}

cuwacunu_prompt_mark() {
  local helper="$1"
  if [ -x "${helper}" ]; then
    "${helper}" 2>/dev/null || true
  fi
}

cuwacunu_prepend_path_once /cuwacunu/.build/hero
cuwacunu_prepend_path_once /cuwacunu/.build/tools
cuwacunu_prepend_path_once /cuwacunu/.build/interface

alias codex-review='codex -s read-only'
alias codex-forge='codex -a never -s workspace-write'
alias codex-full-access='codex --dangerously-bypass-approvals-and-sandbox'

case $- in
  *i*)
    PS1='${debian_chroot:+($debian_chroot)}\u@\h$(cuwacunu_prompt_mark /cuwacunu/src/scripts/hero_human_prompt_mark.sh)$(cuwacunu_prompt_mark /cuwacunu/src/scripts/tsodao_sync_mark.sh):\w\$ '
    ;;
esac

unset -f cuwacunu_prepend_path_once
