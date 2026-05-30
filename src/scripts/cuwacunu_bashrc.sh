#!/usr/bin/env bash

# Repo-owned bashrc extension for cuwacunu shell behavior.

cuwacunu_prepend_path_once() {
  local dir="$1"
  case ":$PATH:" in
    *:"${dir}":*) ;;
    *) export PATH="${dir}:$PATH" ;;
  esac
}

cuwacunu_configure_terminal_env() {
  case "${TERM:-}" in
    "" | dumb | unknown | xterm) export TERM=xterm-256color ;;
  esac

  case "${LANG:-}" in
    "" | C | POSIX) export LANG=C.UTF-8 ;;
  esac

  case "${LC_ALL:-}" in
    "" | C | POSIX) export LC_ALL=C.UTF-8 ;;
  esac

  export COLORTERM="${COLORTERM:-truecolor}"
  export FORCE_COLOR="${FORCE_COLOR:-1}"
}

cuwacunu_configure_terminal_env

cuwacunu_prepend_path_once /cuwacunu/.build/hero
cuwacunu_prepend_path_once /cuwacunu/.build/tools
cuwacunu_prepend_path_once /cuwacunu/.build/interface

alias codex-review='codex -s read-only'
alias codex-forge='codex -a never -s workspace-write'
alias codex-full-access='codex --dangerously-bypass-approvals-and-sandbox'

case $- in
  *i*)
    PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
    ;;
esac

unset -f cuwacunu_prepend_path_once
unset -f cuwacunu_configure_terminal_env
