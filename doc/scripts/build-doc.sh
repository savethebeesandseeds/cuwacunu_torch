#!/usr/bin/env bash
# Secure LaTeX build wrapper for the Cuwacunu algorithm document.
set -Eeuo pipefail

DOC_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN_TEX="${DOC_MAIN:-algorithm.tex}"
DOC_BASENAME="${DOC_BASENAME:-cuwacunu-algorithm}"
ENGINE="${LATEX_ENGINE:-pdflatex}"
PASSES="${LATEX_PASSES:-3}"
BUILD_DIR="${DOC_ROOT}/build"
PUBLISH_DIR="${DOC_ROOT}/publish"

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --clean       Remove build and publish outputs.
  -h, --help    Show this help message.

Environment:
  LATEX_PASSES  Number of pdflatex passes. Default: ${PASSES}
  DOC_BASENAME  Publish PDF basename. Default: ${DOC_BASENAME}
EOF
}

clean_outputs() {
  rm -rf "${BUILD_DIR}" "${PUBLISH_DIR}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      clean_outputs
      exit 0
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ "${ENGINE}" != "pdflatex" ]]; then
  printf 'Refusing LATEX_ENGINE=%s; secure doc builds currently allow pdflatex only.\n' "${ENGINE}" >&2
  exit 2
fi

case "${MAIN_TEX}" in
  /*|*..*|*//*)
    printf 'Refusing unsafe DOC_MAIN path: %s\n' "${MAIN_TEX}" >&2
    exit 2
    ;;
esac

if ! [[ "${PASSES}" =~ ^[1-9][0-9]*$ ]]; then
  printf 'LATEX_PASSES must be a positive integer: %s\n' "${PASSES}" >&2
  exit 2
fi

if ! command -v "${ENGINE}" >/dev/null 2>&1; then
  printf 'Missing %s. Run setup.sh to install the document toolchain.\n' "${ENGINE}" >&2
  exit 127
fi

if [[ ! -f "${DOC_ROOT}/${MAIN_TEX}" ]]; then
  printf 'Missing main TeX file: %s\n' "${DOC_ROOT}/${MAIN_TEX}" >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}" "${PUBLISH_DIR}"

export openin_any=p
export openout_any=p
export shell_escape=f
export TEXMFOUTPUT="${BUILD_DIR}"
export BIBINPUTS="${DOC_ROOT}${BIBINPUTS:+:${BIBINPUTS}}:"

printf 'doc root      : %s\n' "${DOC_ROOT}"
printf 'latex engine  : %s\n' "${ENGINE}"
printf 'main source   : %s\n' "${MAIN_TEX}"
printf 'build dir     : %s\n' "${BUILD_DIR}"
printf 'publish dir   : %s\n' "${PUBLISH_DIR}"
printf 'secure flags  : -no-shell-escape openin_any=p openout_any=p shell_escape=f\n'

cd "${DOC_ROOT}"

for pass in $(seq 1 "${PASSES}"); do
  printf 'pdflatex pass %s/%s\n' "${pass}" "${PASSES}"
  "${ENGINE}" \
    -interaction=nonstopmode \
    -halt-on-error \
    -file-line-error \
    -no-shell-escape \
    -output-directory="${BUILD_DIR}" \
    "${MAIN_TEX}"

  if [[ "${pass}" -eq 1 ]] && grep -q '^\\citation{' "${BUILD_DIR}/${MAIN_TEX%.tex}.aux"; then
    if ! command -v bibtex >/dev/null 2>&1; then
      printf 'Missing bibtex. Run setup.sh to install the bibliography toolchain.\n' >&2
      exit 127
    fi
    printf 'bibtex pass\n'
    (
      cd "${BUILD_DIR}"
      bibtex "${MAIN_TEX%.tex}"
    )
  fi
done

PDF_SOURCE="${BUILD_DIR}/${MAIN_TEX%.tex}.pdf"
PDF_TARGET="${PUBLISH_DIR}/${DOC_BASENAME}.pdf"

if [[ ! -f "${PDF_SOURCE}" ]]; then
  printf 'Expected PDF was not produced: %s\n' "${PDF_SOURCE}" >&2
  exit 1
fi

cp "${PDF_SOURCE}" "${PDF_TARGET}"
printf 'Wrote %s\n' "${PDF_TARGET}"
