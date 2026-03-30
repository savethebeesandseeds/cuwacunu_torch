#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"
BUILD_BIN="${REPO_ROOT}/.build/tools/tsodao"

if [ -x "${BUILD_BIN}" ]; then
  exec "${BUILD_BIN}" prompt-mark
fi

exit 0
