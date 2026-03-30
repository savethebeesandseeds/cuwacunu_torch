#!/usr/bin/env bash

# Repo-owned bashrc extension for cuwacunu shell behavior.

cuwacunu_prepend_path_once() {
  local dir="$1"
  case ":$PATH:" in
    *:"${dir}":*) ;;
    *) export PATH="${dir}:$PATH" ;;
  esac
}

cuwacunu_prepend_path_once /cuwacunu/.build/hero
cuwacunu_prepend_path_once /cuwacunu/.build/tools

case $- in
  *i*)
    PS1='${debian_chroot:+($debian_chroot)}\u@\h$(/cuwacunu/src/scripts/hero_human_prompt_mark.sh)$(/cuwacunu/src/scripts/tsodao_sync_mark.sh):\w\$ '
    ;;
esac

unset -f cuwacunu_prepend_path_once
