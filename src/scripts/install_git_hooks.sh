#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"
GIT_CONFIG_PATH="${REPO_ROOT}/.git/config"

note() {
  printf '[install_git_hooks] %s\n' "$*"
}

die() {
  printf '[install_git_hooks][error] %s\n' "$*" >&2
  exit 1
}

install_with_git() {
  git -C "${REPO_ROOT}" config core.hooksPath .githooks
  note "set core.hooksPath=.githooks via git config"
}

install_by_editing_config() {
  [ -f "${GIT_CONFIG_PATH}" ] || die "missing ${GIT_CONFIG_PATH}"

  local tmp
  tmp="$(mktemp "${TMPDIR:-/tmp}/git_config_hooks.XXXXXX")"
  awk '
    function print_hook() {
      print "\thooksPath = .githooks";
    }
    function trim_local(s) {
      sub(/^[[:space:]]+/, "", s);
      sub(/[[:space:]]+$/, "", s);
      return s;
    }
    BEGIN {
      in_core = 0;
      saw_core = 0;
      wrote_hook = 0;
    }
    {
      if ($0 ~ /^\[core\][[:space:]]*$/) {
        if (in_core && !wrote_hook) print_hook();
        print;
        in_core = 1;
        saw_core = 1;
        next;
      }
      if ($0 ~ /^\[[^]]+\][[:space:]]*$/) {
        if (in_core && !wrote_hook) print_hook();
        in_core = 0;
        print;
        next;
      }
      if (in_core) {
        line = $0;
        sub(/[[:space:]]*[#;].*$/, "", line);
        pos = index(line, "=");
        if (pos > 0) {
          lhs = trim_local(substr(line, 1, pos - 1));
          if (lhs == "hooksPath") {
            print_hook();
            wrote_hook = 1;
            next;
          }
        }
      }
      print;
    }
    END {
      if (!saw_core) {
        print "[core]";
        print_hook();
      } else if (in_core && !wrote_hook) {
        print_hook();
      }
    }
  ' "${GIT_CONFIG_PATH}" > "${tmp}"
  mv "${tmp}" "${GIT_CONFIG_PATH}"
  note "set core.hooksPath=.githooks by editing ${GIT_CONFIG_PATH}"
}

main() {
  if command -v git >/dev/null 2>&1; then
    install_with_git
  else
    install_by_editing_config
  fi
}

main "$@"
