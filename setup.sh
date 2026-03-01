#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Cuwacunu reproducible setup script
#
# What this script installs/configures (inside a Debian-based environment):
# - Base toolchain and system deps via apt:
#   ca-certificates, build-essential, libssl-dev, ncurses dev libs, gnupg2,
#   valgrind, gdb, ccache, mold, locales, curl, libcurl4-openssl-dev
# - curl websocket capability check (ws/wss), with optional source build fallback
# - NVIDIA CUDA apt repository + CUDA Toolkit (default series: 12-1)
# - cuDNN runtime/dev packages from a local NVIDIA .deb repo package
# - Project build targets from /cuwacunu/src (default target: "modules")
#
# Required staged artifacts (for reproducibility):
# - Extract libtorch under: /cuwacunu/external/libtorch
# - Place cuDNN local repo package under: /cuwacunu/external/cudnn/
#   default file:
#   cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb
#
# Typical usage:
# - ./setup.sh
# - ./setup.sh --verbose
# - ./setup.sh --with-curl-source
# - ./setup.sh --skip-build --skip-cuda --skip-cudnn
# - MAKE_TARGETS="piaabo camahjucunu" ./setup.sh --verbose
#
# Notes:
# - Run as root (or with sudo available).
# - Use --dry-run to preview all steps without executing commands.
# -----------------------------------------------------------------------------
set -Eeuo pipefail

SCRIPT_NAME="$(basename "$0")"
REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
EXTERNAL_DIR="${EXTERNAL_DIR:-$REPO_ROOT/external}"
DATA_DIR="${DATA_DIR:-$REPO_ROOT/data}"
SRC_DIR="${SRC_DIR:-$REPO_ROOT/src}"

CUDA_VERSION="${CUDA_VERSION:-12.1}"
CUDA_APT_SERIES="${CUDA_APT_SERIES:-12-1}"
CUDA_REPO_KEY_URL="${CUDA_REPO_KEY_URL:-https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/3bf863cc.pub}"
CUDA_REPO_URL="${CUDA_REPO_URL:-https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/}"

CUDNN_DEB_FILE="${CUDNN_DEB_FILE:-cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb}"
CUDNN_LOCAL_REPO_DIR="${CUDNN_LOCAL_REPO_DIR:-cudnn-local-repo-debian11-8.9.7.29}"
CUDNN_KEYRING_FILE="${CUDNN_KEYRING_FILE:-cudnn-local-9EAC560A-keyring.gpg}"

CURL_SOURCE_VERSION="${CURL_SOURCE_VERSION:-8.9.1}"
CURL_SOURCE_ARCHIVE="curl-${CURL_SOURCE_VERSION}.tar.gz"
MAKE_TARGETS="${MAKE_TARGETS:-modules}"

VERBOSE=0
DRY_RUN=0
WITH_CURL_SOURCE=0
RUN_CUDA=1
RUN_CUDNN=1
RUN_BUILD=1
SUDO=()

if [[ -t 1 ]]; then
  C_RESET=$'\033[0m'
  C_BOLD=$'\033[1m'
  C_RED=$'\033[1;31m'
  C_GREEN=$'\033[1;32m'
  C_YELLOW=$'\033[1;33m'
  C_BLUE=$'\033[1;34m'
  C_MAGENTA=$'\033[1;35m'
  C_CYAN=$'\033[1;36m'
  C_WHITE=$'\033[1;37m'
else
  C_RESET=''
  C_BOLD=''
  C_RED=''
  C_GREEN=''
  C_YELLOW=''
  C_BLUE=''
  C_MAGENTA=''
  C_CYAN=''
  C_WHITE=''
fi

LOG_DIR="${TMPDIR:-/tmp}/cuwacunu-setup-logs"
mkdir -p "${LOG_DIR}"
LOG_FILE="${LOG_DIR}/setup-$(date +%Y%m%d-%H%M%S).log"
: > "${LOG_FILE}"

usage() {
  cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Options:
  -v, --verbose           Show command output while running.
  -n, --dry-run           Print steps without executing commands.
      --with-curl-source  Build curl from source if distro curl lacks ws/wss.
      --skip-cuda         Skip CUDA repository and toolkit installation.
      --skip-cudnn        Skip cuDNN local package installation.
      --skip-build        Skip project compilation.
  -h, --help              Show this help message.

Environment overrides:
  EXTERNAL_DIR            Default: ${EXTERNAL_DIR}
  DATA_DIR                Default: ${DATA_DIR}
  SRC_DIR                 Default: ${SRC_DIR}
  CUDA_VERSION            Default: ${CUDA_VERSION}
  CUDA_APT_SERIES         Default: ${CUDA_APT_SERIES}
  CURL_SOURCE_VERSION     Default: ${CURL_SOURCE_VERSION}
  CUDNN_DEB_FILE          Default: ${CUDNN_DEB_FILE}
  MAKE_TARGETS            Default: "${MAKE_TARGETS}"

Examples:
  ${SCRIPT_NAME}
  ${SCRIPT_NAME} --verbose
  ${SCRIPT_NAME} --with-curl-source --skip-build
  MAKE_TARGETS="piaabo camahjucunu" ${SCRIPT_NAME} -v
EOF
}

banner() {
  printf "\n%b%s%b\n" "${C_MAGENTA}${C_BOLD}" "============================================================" "${C_RESET}"
  printf "%b%s%b\n" "${C_MAGENTA}${C_BOLD}" "  CUWACUNU REPRODUCIBLE ENVIRONMENT SETUP" "${C_RESET}"
  printf "%b%s%b\n\n" "${C_MAGENTA}${C_BOLD}" "============================================================" "${C_RESET}"
}

section() {
  printf "\n%b%s%b\n" "${C_CYAN}${C_BOLD}" "==> $*" "${C_RESET}"
}

info() {
  printf "%b[%s]%b %s\n" "${C_BLUE}" "INFO" "${C_RESET}" "$*"
}

success() {
  printf "%b[%s]%b %s\n" "${C_GREEN}" " OK " "${C_RESET}" "$*"
}

warn() {
  printf "%b[%s]%b %s\n" "${C_YELLOW}" "WARN" "${C_RESET}" "$*"
}

fail() {
  printf "%b[%s]%b %s\n" "${C_RED}" "FAIL" "${C_RESET}" "$*" >&2
}

render_cmd() {
  printf '%q ' "$@"
}

on_error() {
  local exit_code=$?
  local line="$1"
  local cmd="$2"

  fail "Setup failed at line ${line}: ${cmd}"
  if (( VERBOSE == 0 )); then
    warn "See full log: ${LOG_FILE}"
    warn "Last 40 log lines:"
    tail -n 40 "${LOG_FILE}" >&2 || true
  fi
  exit "${exit_code}"
}

trap 'on_error "${LINENO}" "${BASH_COMMAND}"' ERR

run_cmd() {
  local description="$1"
  shift
  local cmd_text
  cmd_text="$(render_cmd "$@")"

  info "${description}"
  if (( DRY_RUN )); then
    printf "%b[%s]%b %s\n" "${C_YELLOW}" "DRY " "${C_RESET}" "${cmd_text}"
    return 0
  fi

  if (( VERBOSE )); then
    printf "%b[%s]%b %s\n" "${C_WHITE}" "RUN " "${C_RESET}" "${cmd_text}"
    "$@"
  else
    "$@" >>"${LOG_FILE}" 2>&1
  fi
}

append_line_once() {
  local line="$1"
  local file="$2"

  if (( DRY_RUN )); then
    info "Would ensure in ${file}: ${line}"
    return 0
  fi

  touch "${file}"
  if grep -Fqx "${line}" "${file}"; then
    info "Already set in ${file}: ${line}"
    return 0
  fi

  printf '%s\n' "${line}" >>"${file}"
  success "Added to ${file}: ${line}"
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -v|--verbose)
        VERBOSE=1
        shift
        ;;
      -n|--dry-run)
        DRY_RUN=1
        shift
        ;;
      --with-curl-source)
        WITH_CURL_SOURCE=1
        shift
        ;;
      --skip-cuda)
        RUN_CUDA=0
        shift
        ;;
      --skip-cudnn)
        RUN_CUDNN=0
        shift
        ;;
      --skip-build)
        RUN_BUILD=0
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        fail "Unknown option: $1"
        usage
        exit 1
        ;;
    esac
  done
}

require_root_or_sudo() {
  if (( EUID == 0 )); then
    success "Running with root privileges."
    return 0
  fi

  if command -v sudo >/dev/null 2>&1; then
    SUDO=(sudo)
    warn "Running as non-root user. Commands needing privileges will use sudo."
    return 0
  fi

  fail "This script needs root privileges (or sudo available)."
  exit 1
}

ensure_directories() {
  section "Directory Preparation"
  run_cmd "Creating ${EXTERNAL_DIR}, ${EXTERNAL_DIR}/cudnn, and ${DATA_DIR}" \
    mkdir -p "${EXTERNAL_DIR}" "${EXTERNAL_DIR}/cudnn" "${DATA_DIR}"
}

install_base_requirements() {
  section "Base Apt Requirements"
  run_cmd "Updating apt metadata" "${SUDO[@]}" apt-get update
  run_cmd "Upgrading installed packages" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
  run_cmd "Installing base build/debug dependencies" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends \
    ca-certificates build-essential libssl-dev libncurses5-dev libncursesw5-dev \
    gnupg2 valgrind gdb ccache mold locales curl libcurl4-openssl-dev
  run_cmd "Generating locale en_US.UTF-8" "${SUDO[@]}" locale-gen en_US.UTF-8
  run_cmd "Setting LANG=en_US.UTF-8" "${SUDO[@]}" update-locale LANG=en_US.UTF-8
}

curl_supports_websockets() {
  local protocols
  protocols="$(curl --version | awk '/^Protocols:/ {for (i=2; i<=NF; ++i) print $i}')"
  grep -qx "ws" <<<"${protocols}" && grep -qx "wss" <<<"${protocols}"
}

verify_curl() {
  section "curl Validation"
  run_cmd "Checking curl path" which curl
  run_cmd "Inspecting curl version" curl --version

  if (( DRY_RUN )); then
    info "Dry run: skipping websocket protocol validation."
    return 0
  fi

  if curl_supports_websockets; then
    success "curl reports websocket protocol support (ws/wss)."
  else
    warn "curl does not report ws/wss support."
    if (( WITH_CURL_SOURCE )); then
      install_curl_from_source
    else
      warn "Use --with-curl-source to compile curl with websocket support."
    fi
  fi
}

install_curl_from_source() {
  section "curl Build From Source"

  local archive_path="${EXTERNAL_DIR}/${CURL_SOURCE_ARCHIVE}"
  local source_dir="${EXTERNAL_DIR}/curl-${CURL_SOURCE_VERSION}"

  if (( DRY_RUN )); then
    run_cmd "Removing distro curl packages before source install" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive \
      apt-get remove -y curl libcurl4-openssl-dev libcurl4 libcurl3-gnutls
    run_cmd "Downloading ${CURL_SOURCE_ARCHIVE}" curl -fsSL -o "${archive_path}" \
      "https://curl.se/download/${CURL_SOURCE_ARCHIVE}"
    run_cmd "Extracting ${CURL_SOURCE_ARCHIVE}" tar -xzf "${archive_path}" -C "${EXTERNAL_DIR}"
    run_cmd "Configuring curl with OpenSSL + websocket support" bash -lc "cd \"${source_dir}\" && ./configure --with-openssl --enable-websockets"
    run_cmd "Building curl from source" bash -lc "cd \"${source_dir}\" && make -j\$(nproc)"
    run_cmd "Installing curl from source" "${SUDO[@]}" bash -lc "cd \"${source_dir}\" && make install"
    run_cmd "Refreshing shared library cache" "${SUDO[@]}" ldconfig
    info "Dry run: skipping curl ws/wss post-validation."
    return 0
  fi

  run_cmd "Removing distro curl packages before source install" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive \
    apt-get remove -y curl libcurl4-openssl-dev libcurl4 libcurl3-gnutls

  if [[ ! -f "${archive_path}" ]]; then
    run_cmd "Downloading ${CURL_SOURCE_ARCHIVE}" curl -fsSL -o "${archive_path}" \
      "https://curl.se/download/${CURL_SOURCE_ARCHIVE}"
  else
    info "Found existing archive: ${archive_path}"
  fi

  if [[ ! -d "${source_dir}" ]]; then
    run_cmd "Extracting ${CURL_SOURCE_ARCHIVE}" tar -xzf "${archive_path}" -C "${EXTERNAL_DIR}"
  else
    info "Found existing source directory: ${source_dir}"
  fi

  pushd "${source_dir}" >/dev/null
  run_cmd "Configuring curl with OpenSSL + websocket support" ./configure --with-openssl --enable-websockets
  run_cmd "Building curl from source" make -j"$(nproc)"
  run_cmd "Installing curl from source" "${SUDO[@]}" make install
  popd >/dev/null

  run_cmd "Refreshing shared library cache" "${SUDO[@]}" ldconfig
  append_line_once 'export PATH="/usr/local/bin:$PATH"' "${HOME}/.bashrc"
  run_cmd "Inspecting curl version after source install" curl --version

  if curl_supports_websockets; then
    success "curl source build provides ws/wss support."
  else
    fail "curl still does not report ws/wss support after source install."
    exit 1
  fi
}

configure_nvidia_repo() {
  section "NVIDIA CUDA Apt Repository"

  local key_tmp
  local list_tmp
  key_tmp="$(mktemp)"
  list_tmp="$(mktemp)"

  run_cmd "Downloading NVIDIA CUDA apt key" curl -fsSL -o "${key_tmp}" "${CUDA_REPO_KEY_URL}"
  run_cmd "Installing NVIDIA keyring" "${SUDO[@]}" gpg --dearmor --yes \
    --output /usr/share/keyrings/nvidia-cuda-archive-keyring.gpg "${key_tmp}"

  printf 'deb [signed-by=/usr/share/keyrings/nvidia-cuda-archive-keyring.gpg] %s /\n' "${CUDA_REPO_URL}" >"${list_tmp}"
  run_cmd "Writing /etc/apt/sources.list.d/cuda.list" "${SUDO[@]}" install -m 644 "${list_tmp}" /etc/apt/sources.list.d/cuda.list
  run_cmd "Refreshing apt metadata after adding NVIDIA repo" "${SUDO[@]}" apt-get update

  rm -f "${key_tmp}" "${list_tmp}"
}

install_cuda() {
  section "CUDA Toolkit Installation"
  run_cmd "Installing cuda-toolkit-${CUDA_APT_SERIES}" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends "cuda-toolkit-${CUDA_APT_SERIES}"

  append_line_once "export CUDA_VERSION=${CUDA_VERSION}" "${HOME}/.bashrc"
  append_line_once 'export PATH=/usr/local/cuda-$CUDA_VERSION/bin:$PATH' "${HOME}/.bashrc"
  append_line_once 'export LD_LIBRARY_PATH=/usr/local/cuda-$CUDA_VERSION/lib64:$LD_LIBRARY_PATH' "${HOME}/.bashrc"

  if (( DRY_RUN )); then
    info "Dry run: skipping nvcc runtime validation."
    return 0
  fi

  if command -v nvcc >/dev/null 2>&1; then
    run_cmd "Validating CUDA via nvcc --version" nvcc --version
  elif [[ -x "/usr/local/cuda-${CUDA_VERSION}/bin/nvcc" ]]; then
    run_cmd "Validating CUDA via /usr/local/cuda-${CUDA_VERSION}/bin/nvcc --version" \
      "/usr/local/cuda-${CUDA_VERSION}/bin/nvcc" --version
  else
    warn "nvcc was not found in PATH. Open a new shell or source ~/.bashrc."
  fi
}

install_cudnn() {
  section "cuDNN Installation"

  local cudnn_deb_path="${EXTERNAL_DIR}/cudnn/${CUDNN_DEB_FILE}"
  local cudnn_key_src="/var/${CUDNN_LOCAL_REPO_DIR}/${CUDNN_KEYRING_FILE}"

  if (( DRY_RUN )); then
    run_cmd "Installing local cuDNN repository package" "${SUDO[@]}" dpkg -i "${cudnn_deb_path}"
    run_cmd "Installing cuDNN apt keyring" "${SUDO[@]}" install -m 644 "${cudnn_key_src}" "/usr/share/keyrings/${CUDNN_KEYRING_FILE}"
    run_cmd "Refreshing apt metadata for cuDNN repo" "${SUDO[@]}" apt-get update
    run_cmd "Installing libcudnn8" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libcudnn8
    run_cmd "Installing libcudnn8-dev" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libcudnn8-dev
    append_line_once 'export CUDNN_VERSION=8' "${HOME}/.bashrc"
    append_line_once 'export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH' "${HOME}/.bashrc"
    info "Dry run: skipping local cuDNN file existence checks."
    return 0
  fi

  if [[ ! -f "${cudnn_deb_path}" ]]; then
    fail "Missing ${cudnn_deb_path}"
    fail "Download the cuDNN Debian package and place it in ${EXTERNAL_DIR}/cudnn first."
    exit 1
  fi

  run_cmd "Installing local cuDNN repository package" "${SUDO[@]}" dpkg -i "${cudnn_deb_path}"

  if [[ ! -f "${cudnn_key_src}" ]]; then
    fail "Expected cuDNN keyring not found: ${cudnn_key_src}"
    fail "Check CUDNN_LOCAL_REPO_DIR / CUDNN_KEYRING_FILE values for your package version."
    exit 1
  fi

  run_cmd "Installing cuDNN apt keyring" "${SUDO[@]}" install -m 644 "${cudnn_key_src}" "/usr/share/keyrings/${CUDNN_KEYRING_FILE}"
  run_cmd "Refreshing apt metadata for cuDNN repo" "${SUDO[@]}" apt-get update
  run_cmd "Installing libcudnn8" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libcudnn8
  run_cmd "Installing libcudnn8-dev" "${SUDO[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libcudnn8-dev

  append_line_once 'export CUDNN_VERSION=8' "${HOME}/.bashrc"
  append_line_once 'export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH' "${HOME}/.bashrc"
  run_cmd "Validating cuDNN packages in dpkg" dpkg -l libcudnn8 libcudnn8-dev
}

build_project() {
  section "Project Build"

  if [[ ! -d "${SRC_DIR}" ]]; then
    fail "Missing source directory: ${SRC_DIR}"
    exit 1
  fi

  pushd "${SRC_DIR}" >/dev/null
  for target in ${MAKE_TARGETS}; do
    run_cmd "Running make ${target}" make "${target}"
  done
  popd >/dev/null
}

cleanup_apt_lists() {
  section "Apt Cache Cleanup"
  run_cmd "Cleaning /var/lib/apt/lists" "${SUDO[@]}" find /var/lib/apt/lists -mindepth 1 -delete
}

main() {
  parse_args "$@"
  banner

  info "Repository root : ${REPO_ROOT}"
  info "External folder : ${EXTERNAL_DIR}"
  info "Data folder     : ${DATA_DIR}"
  info "Build targets   : ${MAKE_TARGETS}"
  info "Verbose mode    : ${VERBOSE}"
  info "Dry run         : ${DRY_RUN}"
  if (( VERBOSE == 0 )); then
    info "Command output is captured in ${LOG_FILE}"
  fi

  require_root_or_sudo
  ensure_directories
  install_base_requirements
  verify_curl

  if (( RUN_CUDA )); then
    configure_nvidia_repo
    install_cuda
  else
    warn "Skipping CUDA installation (--skip-cuda)."
  fi

  if (( RUN_CUDNN )); then
    install_cudnn
  else
    warn "Skipping cuDNN installation (--skip-cudnn)."
  fi

  if (( RUN_BUILD )); then
    build_project
  else
    warn "Skipping build step (--skip-build)."
  fi

  cleanup_apt_lists

  section "Setup Complete"
  success "Provisioning flow finished."
  info "Open a new shell or run: source ~/.bashrc"
  info "Useful check: nvidia-smi"
  if (( VERBOSE == 0 )); then
    info "Execution log: ${LOG_FILE}"
  fi
}

main "$@"
