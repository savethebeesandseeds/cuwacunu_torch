#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SRC_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd -- "$SRC_ROOT/.." && pwd)"

GLOBAL_CONFIG_PATH="$SRC_ROOT/config/.config"
HUMAN_CONFIG_PATH=""
SUPER_CONFIG_PATH=""
MODE="init"
OPERATOR_ID_OVERRIDE=""
ASSUME_YES=0
STEP_COUNTER=0

COLOR_RESET=""
COLOR_BOLD=""
COLOR_DIM=""
COLOR_RED=""
COLOR_GREEN=""
COLOR_YELLOW=""
COLOR_BLUE=""
COLOR_CYAN=""

setup_colors() {
  if [[ -n "${NO_COLOR:-}" ]]; then
    return 0
  fi
  if [[ ! -t 1 && ! -t 2 ]]; then
    return 0
  fi
  COLOR_RESET=$'\033[0m'
  COLOR_BOLD=$'\033[1m'
  COLOR_DIM=$'\033[2m'
  COLOR_RED=$'\033[31m'
  COLOR_GREEN=$'\033[32m'
  COLOR_YELLOW=$'\033[33m'
  COLOR_BLUE=$'\033[34m'
  COLOR_CYAN=$'\033[36m'
}

usage() {
  cat <<EOF
usage: $(basename "$0") [options]

Bootstrap or validate the Human Hero operator identity contract.

This script manages the current Human Hero signing surface:
- operator_id in default.hero.human.dsl
- unencrypted OpenSSH ssh-ed25519 private key at operator_signing_ssh_identity
- matching public-key registration in human_operator_identities

options:
  --global-config PATH   Global HERO config. Default: $GLOBAL_CONFIG_PATH
  --human-config PATH    Human Hero defaults DSL override.
  --super-config PATH    Super Hero defaults DSL override.
  --operator-id ID       Explicit operator id to persist before setup.
  --yes                  Assume yes for interactive replacement prompts.
  --validate             Validate only; do not mutate files.
  --help                 Show this help text.

examples:
  bash src/scripts/$(basename "$0")
  bash src/scripts/$(basename "$0") --operator-id alice@workstation
  bash src/scripts/$(basename "$0") --yes
  bash src/scripts/$(basename "$0") --validate
EOF
}

die() {
  printf '%s[setup_human_operator][error]%s %s\n' \
    "$COLOR_RED$COLOR_BOLD" "$COLOR_RESET" "$*" >&2
  exit 1
}

note() {
  printf '%s[setup_human_operator]%s %s\n' \
    "$COLOR_BLUE$COLOR_BOLD" "$COLOR_RESET" "$*" >&2
}

warn() {
  printf '%s[setup_human_operator][warning]%s %s\n' \
    "$COLOR_YELLOW$COLOR_BOLD" "$COLOR_RESET" "$*" >&2
}

success() {
  printf '%s[setup_human_operator][ok]%s %s\n' \
    "$COLOR_GREEN$COLOR_BOLD" "$COLOR_RESET" "$*" >&2
}

section() {
  printf '\n%s%s== %s ==%s\n' \
    "$COLOR_CYAN" "$COLOR_BOLD" "$*" "$COLOR_RESET" >&2
}

step() {
  STEP_COUNTER=$((STEP_COUNTER + 1))
  printf '%s%s[%02d]%s %s\n' \
    "$COLOR_BLUE" "$COLOR_BOLD" "$STEP_COUNTER" "$COLOR_RESET" "$*" >&2
}

show_pair() {
  local key="$1"
  local value="$2"
  printf '%s  - %-18s%s %s\n' \
    "$COLOR_DIM" "$key" "$COLOR_RESET" "$value" >&2
}

require_command() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || die "missing required command: $cmd"
}

canon_dir() {
  local path="$1"
  realpath -m -- "$path"
}

resolve_near() {
  local base_dir="$1"
  local raw_path="$2"
  if [[ -z "$raw_path" ]]; then
    return 0
  fi
  if [[ "$raw_path" = /* ]]; then
    realpath -m -- "$raw_path"
  else
    realpath -m -- "$base_dir/$raw_path"
  fi
}

trim() {
  local value="$1"
  value="${value#"${value%%[![:space:]]*}"}"
  value="${value%"${value##*[![:space:]]}"}"
  printf '%s' "$value"
}

strip_inline_comment() {
  local value="$1"
  value="${value%%#*}"
  value="${value%%;*}"
  trim "$value"
}

read_ini_value() {
  local ini_path="$1"
  local section="$2"
  local key="$3"
  awk -v wanted_section="$section" -v wanted_key="$key" '
    function trim_local(s) {
      sub(/^[[:space:]]+/, "", s);
      sub(/[[:space:]]+$/, "", s);
      return s;
    }
    {
      line = $0;
      sub(/[[:space:]]*[#;].*$/, "", line);
      line = trim_local(line);
      if (line == "") next;
      if (line ~ /^\[[^]]+\]$/) {
        current = trim_local(substr(line, 2, length(line) - 2));
        next;
      }
      if (current != wanted_section) next;
      pos = index(line, "=");
      if (pos == 0) next;
      lhs = trim_local(substr(line, 1, pos - 1));
      rhs = trim_local(substr(line, pos + 1));
      if (lhs == wanted_key) {
        print rhs;
        exit;
      }
    }
  ' "$ini_path"
}

read_dsl_value() {
  local dsl_path="$1"
  local key="$2"
  awk -v wanted_key="$key" '
    function trim_local(s) {
      sub(/^[[:space:]]+/, "", s);
      sub(/[[:space:]]+$/, "", s);
      return s;
    }
    BEGIN { in_block = 0; }
    {
      line = $0;
      if (in_block) {
        if (line ~ /\*\//) {
          sub(/^.*\*\//, "", line);
          in_block = 0;
        } else {
          next;
        }
      }
      if (line ~ /\/\*/) {
        if (line !~ /\*\//) {
          sub(/\/\*.*$/, "", line);
          in_block = 1;
        } else {
          gsub(/\/\*([^*]|\*+[^/])*\*\//, "", line);
        }
      }
      sub(/[[:space:]]*[#;].*$/, "", line);
      line = trim_local(line);
      if (line == "") next;
      pos = index(line, "=");
      if (pos == 0) next;
      lhs = trim_local(substr(line, 1, pos - 1));
      rhs = trim_local(substr(line, pos + 1));
      gsub(/\[[^]]*\]/, "", lhs);
      gsub(/\([^)]*\)/, "", lhs);
      sub(/:.*/, "", lhs);
      if (lhs == wanted_key) {
        if ((rhs ~ /^".*"$/) || (rhs ~ /^'\''.*'\''$/)) {
          rhs = substr(rhs, 2, length(rhs) - 2);
        }
        print rhs;
        exit;
      }
    }
  ' "$dsl_path"
}

write_operator_id() {
  local dsl_path="$1"
  local operator_id="$2"
  local tmp
  tmp="$(mktemp "${TMPDIR:-/tmp}/human_operator_dsl.XXXXXX")"
  awk -v operator_id="$operator_id" '
    function trim_local(s) {
      sub(/^[[:space:]]+/, "", s);
      sub(/[[:space:]]+$/, "", s);
      return s;
    }
    BEGIN { replaced = 0; }
    {
      line = $0;
      work = line;
      sub(/[[:space:]]*[#;].*$/, "", work);
      work = trim_local(work);
      pos = index(work, "=");
      if (!replaced && pos > 0) {
        lhs = trim_local(substr(work, 1, pos - 1));
        gsub(/\[[^]]*\]/, "", lhs);
        gsub(/\([^)]*\)/, "", lhs);
        sub(/:.*/, "", lhs);
        if (lhs == "operator_id") {
          print "operator_id:str = " operator_id " # managed by setup_human_operator.sh";
          replaced = 1;
          next;
        }
      }
      print line;
    }
    END {
      if (!replaced) {
        print "operator_id:str = " operator_id " # managed by setup_human_operator.sh";
      }
    }
  ' "$dsl_path" >"$tmp"
  mv -f -- "$tmp" "$dsl_path"
}

derive_operator_id() {
  local user_name host_name
  user_name="${USER:-}"
  if [[ -z "$user_name" ]]; then
    user_name="$(id -un 2>/dev/null || true)"
  fi
  [[ -n "$user_name" ]] || user_name="operator"
  host_name="$(hostname 2>/dev/null || true)"
  [[ -n "$host_name" ]] || host_name="localhost"
  printf '%s@%s' "$user_name" "$host_name"
}

prompt_replace_identity() {
  local identity_path="$1"
  if [[ "$ASSUME_YES" -eq 1 ]]; then
    return 0
  fi
  if [[ ! -t 0 || ! -t 1 ]]; then
    warn "existing identity found at $identity_path; non-interactive shell detected, keeping current identity"
    return 1
  fi

  printf '%s[setup_human_operator]%s existing identity found at %s\n' \
    "$COLOR_YELLOW$COLOR_BOLD" "$COLOR_RESET" "$identity_path" >&2
  while true; do
    printf '%sReplace it with a new ssh-ed25519 identity? [y/N]: %s' \
      "$COLOR_BOLD" "$COLOR_RESET" >&2
    local reply=""
    IFS= read -r reply || return 1
    reply="$(trim "$reply")"
    case "${reply,,}" in
      y|yes) return 0 ;;
      ""|n|no) return 1 ;;
      *) warn "please answer yes or no" ;;
    esac
  done
}

ensure_operator_id() {
  local dsl_path="$1"
  local current_value="$2"
  if [[ -n "$OPERATOR_ID_OVERRIDE" ]]; then
    write_operator_id "$dsl_path" "$OPERATOR_ID_OVERRIDE"
    printf '%s' "$OPERATOR_ID_OVERRIDE"
    return 0
  fi
  if [[ -z "$current_value" || "$current_value" == "CHANGE_ME_OPERATOR" ]]; then
    local derived
    derived="$(derive_operator_id)"
    note "bootstrapping operator_id to $derived"
    write_operator_id "$dsl_path" "$derived"
    printf '%s' "$derived"
    return 0
  fi
  printf '%s' "$current_value"
}

extract_public_key() {
  local identity_path="$1"
  require_command ssh-keygen
  ssh-keygen -y -f "$identity_path" 2>/dev/null | tr -d '\r' | sed -n '1p'
}

register_public_key() {
  local identities_path="$1"
  local operator_id="$2"
  local public_key="$3"
  local tmp
  mkdir -p -- "$(dirname -- "$identities_path")"
  tmp="$(mktemp "${TMPDIR:-/tmp}/human_operator_identities.XXXXXX")"
  if [[ -f "$identities_path" ]]; then
    awk -v operator_id="$operator_id" -v public_key="$public_key" '
      BEGIN { replaced = 0; }
      /^[[:space:]]*$/ { print; next; }
      /^[[:space:]]*#/ { print; next; }
      {
        split($0, parts, /[[:space:]]+/);
        if (parts[1] == operator_id) {
          if (!replaced) {
            print operator_id " " public_key;
            replaced = 1;
          }
          next;
        }
        print;
      }
      END {
        if (!replaced) print operator_id " " public_key;
      }
    ' "$identities_path" >"$tmp"
  else
    printf '%s %s\n' "$operator_id" "$public_key" >"$tmp"
  fi
  mv -f -- "$tmp" "$identities_path"
}

validate_operator_setup() {
  local operator_id="$1"
  local identity_path="$2"
  local identities_path="$3"
  [[ -n "$operator_id" ]] || die "operator_id is empty"
  [[ "$operator_id" != "CHANGE_ME_OPERATOR" ]] || die "operator_id is still CHANGE_ME_OPERATOR"
  [[ -f "$identity_path" ]] || die "missing operator signing identity: $identity_path"
  [[ -r "$identity_path" ]] || die "operator signing identity is not readable: $identity_path"
  [[ -f "$identities_path" ]] || die "missing human operator identities file: $identities_path"
  [[ -r "$identities_path" ]] || die "human operator identities file is not readable: $identities_path"
  local public_key registered_line
  public_key="$(extract_public_key "$identity_path")"
  [[ -n "$public_key" ]] || die "failed to derive public key from $identity_path"
  registered_line="$(awk -v operator_id="$operator_id" '
    /^[[:space:]]*$/ { next }
    /^[[:space:]]*#/ { next }
    {
      split($0, parts, /[[:space:]]+/);
      if (parts[1] == operator_id) {
        print $0;
        exit;
      }
    }
  ' "$identities_path")"
  [[ -n "$registered_line" ]] || die "no human_operator_identities entry found for $operator_id"
  local expected_line="$operator_id $public_key"
  [[ "$registered_line" == "$expected_line" ]] || {
    printf '%s[setup_human_operator]%s expected registration: %s\n' \
      "$COLOR_YELLOW$COLOR_BOLD" "$COLOR_RESET" "$expected_line" >&2
    printf '%s[setup_human_operator]%s actual registration:   %s\n' \
      "$COLOR_YELLOW$COLOR_BOLD" "$COLOR_RESET" "$registered_line" >&2
    die "registered public key does not match $identity_path"
  }
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --global-config)
      [[ $# -ge 2 ]] || die "--global-config requires a value"
      GLOBAL_CONFIG_PATH="$2"
      shift 2
      ;;
    --human-config)
      [[ $# -ge 2 ]] || die "--human-config requires a value"
      HUMAN_CONFIG_PATH="$2"
      shift 2
      ;;
    --super-config)
      [[ $# -ge 2 ]] || die "--super-config requires a value"
      SUPER_CONFIG_PATH="$2"
      shift 2
      ;;
    --operator-id)
      [[ $# -ge 2 ]] || die "--operator-id requires a value"
      OPERATOR_ID_OVERRIDE="$2"
      shift 2
      ;;
    --yes)
      ASSUME_YES=1
      shift
      ;;
    --validate)
      MODE="validate"
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1"
      ;;
  esac
done

setup_colors
section "Human Operator Setup"
show_pair "mode" "$MODE"
show_pair "global config" "$GLOBAL_CONFIG_PATH"

require_command awk
require_command sed
require_command realpath
require_command hostname
require_command id

GLOBAL_CONFIG_PATH="$(realpath -m -- "$GLOBAL_CONFIG_PATH")"
[[ -f "$GLOBAL_CONFIG_PATH" ]] || die "missing global config: $GLOBAL_CONFIG_PATH"
GLOBAL_CONFIG_DIR="$(dirname -- "$GLOBAL_CONFIG_PATH")"

step "Resolving Human Hero configuration"
if [[ -z "$HUMAN_CONFIG_PATH" ]]; then
  human_config_raw="$(read_ini_value "$GLOBAL_CONFIG_PATH" "REAL_HERO" "human_hero_dsl_filename")"
  [[ -n "$human_config_raw" ]] || die "missing [REAL_HERO].human_hero_dsl_filename in $GLOBAL_CONFIG_PATH"
  HUMAN_CONFIG_PATH="$(resolve_near "$GLOBAL_CONFIG_DIR" "$human_config_raw")"
else
  HUMAN_CONFIG_PATH="$(realpath -m -- "$HUMAN_CONFIG_PATH")"
fi

if [[ -z "$SUPER_CONFIG_PATH" ]]; then
  super_config_raw="$(read_ini_value "$GLOBAL_CONFIG_PATH" "REAL_HERO" "super_hero_dsl_filename")"
  [[ -n "$super_config_raw" ]] || die "missing [REAL_HERO].super_hero_dsl_filename in $GLOBAL_CONFIG_PATH"
  SUPER_CONFIG_PATH="$(resolve_near "$GLOBAL_CONFIG_DIR" "$super_config_raw")"
else
  SUPER_CONFIG_PATH="$(realpath -m -- "$SUPER_CONFIG_PATH")"
fi

[[ -f "$HUMAN_CONFIG_PATH" ]] || die "missing Human Hero defaults DSL: $HUMAN_CONFIG_PATH"
[[ -f "$SUPER_CONFIG_PATH" ]] || die "missing Super Hero defaults DSL: $SUPER_CONFIG_PATH"

HUMAN_CONFIG_DIR="$(dirname -- "$HUMAN_CONFIG_PATH")"
SUPER_CONFIG_DIR="$(dirname -- "$SUPER_CONFIG_PATH")"
show_pair "human config" "$HUMAN_CONFIG_PATH"
show_pair "super config" "$SUPER_CONFIG_PATH"

operator_id="$(trim "$(read_dsl_value "$HUMAN_CONFIG_PATH" "operator_id")")"
identity_raw="$(trim "$(read_dsl_value "$HUMAN_CONFIG_PATH" "operator_signing_ssh_identity")")"
identities_raw="$(trim "$(read_dsl_value "$SUPER_CONFIG_PATH" "human_operator_identities")")"

[[ -n "$identity_raw" ]] || die "missing operator_signing_ssh_identity in $HUMAN_CONFIG_PATH"
[[ -n "$identities_raw" ]] || die "missing human_operator_identities in $SUPER_CONFIG_PATH"

IDENTITY_PATH="$(resolve_near "$HUMAN_CONFIG_DIR" "$identity_raw")"
IDENTITIES_PATH="$(resolve_near "$SUPER_CONFIG_DIR" "$identities_raw")"
show_pair "identity path" "$IDENTITY_PATH"
show_pair "identities file" "$IDENTITIES_PATH"

if [[ "$MODE" == "init" ]]; then
  step "Bootstrapping operator identity"
  operator_id="$(ensure_operator_id "$HUMAN_CONFIG_PATH" "$operator_id")"
  show_pair "operator id" "$operator_id"
  mkdir -p -- "$(dirname -- "$IDENTITY_PATH")"
  mkdir -p -- "$(dirname -- "$IDENTITIES_PATH")"
  if [[ -f "$IDENTITY_PATH" ]]; then
    step "Existing operator key detected"
    if prompt_replace_identity "$IDENTITY_PATH"; then
      require_command ssh-keygen
      note "replacing operator identity at $IDENTITY_PATH"
      rm -f -- "$IDENTITY_PATH" "${IDENTITY_PATH}.pub"
    else
      note "keeping existing operator identity"
    fi
  fi
  if [[ ! -f "$IDENTITY_PATH" ]]; then
    step "Generating ssh-ed25519 operator identity"
    require_command ssh-keygen
    note "generating unencrypted ssh-ed25519 operator identity at $IDENTITY_PATH"
    ssh-keygen -q -t ed25519 -N '' -C "$operator_id" -f "$IDENTITY_PATH"
  else
    step "Using existing ssh-ed25519 operator identity"
  fi
  chmod 600 -- "$IDENTITY_PATH" 2>/dev/null || true
  step "Registering public key"
  public_key="$(extract_public_key "$IDENTITY_PATH")"
  [[ -n "$public_key" ]] || die "failed to derive public key from $IDENTITY_PATH"
  printf '%s\n' "$public_key" >"${IDENTITY_PATH}.pub"
  chmod 644 -- "${IDENTITY_PATH}.pub" 2>/dev/null || true
  register_public_key "$IDENTITIES_PATH" "$operator_id" "$public_key"
  chmod 644 -- "$IDENTITIES_PATH" 2>/dev/null || true
fi

step "Validating operator setup"
validate_operator_setup "$operator_id" "$IDENTITY_PATH" "$IDENTITIES_PATH"

section "Summary"
show_pair "operator_id" "$operator_id"
show_pair "identity_path" "$IDENTITY_PATH"
show_pair "identities_path" "$IDENTITIES_PATH"
success "operator setup is valid"
printf 'operator_id=%s\n' "$operator_id"
printf 'identity_path=%s\n' "$IDENTITY_PATH"
printf 'identities_path=%s\n' "$IDENTITIES_PATH"
printf 'status=ok\n'
