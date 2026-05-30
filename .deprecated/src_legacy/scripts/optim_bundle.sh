#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"
BUILD_BIN="${REPO_ROOT}/.build/tools/tsodao"

if [ -x "${BUILD_BIN}" ]; then
  exec "${BUILD_BIN}" "$@"
fi

printf 'optim_bundle.sh: missing tsodao binary at %s; run `make -C %s/src/main/tools -j12 build-tsodao`\n' \
  "${BUILD_BIN}" "${REPO_ROOT}" >&2
exit 1
