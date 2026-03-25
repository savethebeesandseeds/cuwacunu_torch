#!/usr/bin/env bash
set -euo pipefail

API_URL="https://api.binance.com/api/v3/exchangeInfo?showPermissionSets=false"
BASE_FILTER=""
QUOTE_FILTER=""
STATUS_FILTER="TRADING"
SPOT_ONLY=1
SYMBOLS_ONLY=0

usage() {
  cat <<EOF
usage: $(basename "$0") [options]

Fetch Binance spot symbols and print a filtered symbol list.

options:
  --base ASSET         Filter by base asset.
  --quote ASSET        Filter by quote asset.
  --status STATUS      Filter by status. Default: ${STATUS_FILTER}
  --all-status         Disable status filtering.
  --include-non-spot   Include symbols without spot trading enabled.
  --symbols-only       Print only symbol names.
  --help               Show this help text.

examples:
  bash src/scripts/$(basename "$0") --quote USDT
  bash src/scripts/$(basename "$0") --base BTC --include-non-spot
  bash src/scripts/$(basename "$0") --base BTC --quote USDT --symbols-only
EOF
}

die() {
  printf '[list_binance_spot_symbols] error: %s\n' "$*" >&2
  exit 1
}

require_command() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || die "missing required command: $cmd"
}

extract_string_field() {
  local record="$1"
  local key="$2"
  printf '%s\n' "$record" | sed -n "s/.*\"${key}\":\"\\([^\"]*\\)\".*/\\1/p"
}

extract_bool_field() {
  local record="$1"
  local key="$2"
  printf '%s\n' "$record" | sed -n "s/.*\"${key}\":\\(true\\|false\\).*/\\1/p"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --base)
      [ "$#" -ge 2 ] || die "--base requires a value"
      BASE_FILTER="$2"
      shift 2
      ;;
    --quote)
      [ "$#" -ge 2 ] || die "--quote requires a value"
      QUOTE_FILTER="$2"
      shift 2
      ;;
    --status)
      [ "$#" -ge 2 ] || die "--status requires a value"
      STATUS_FILTER="$2"
      shift 2
      ;;
    --all-status)
      STATUS_FILTER=""
      shift
      ;;
    --include-non-spot)
      SPOT_ONLY=0
      shift
      ;;
    --symbols-only)
      SYMBOLS_ONLY=1
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

require_command curl
require_command sed
require_command grep
require_command sort
require_command tr

payload="$(curl -fsSL --retry 3 --connect-timeout 20 "$API_URL")" || die "failed to query $API_URL"
payload="$(printf '%s' "$payload" | tr -d '\n')"

rows="$(
  printf '%s' "$payload" \
    | sed 's/{\"symbol\"/\n{\"symbol\"/g' \
    | grep '^{"symbol"' \
    | while IFS= read -r record; do
        symbol="$(extract_string_field "$record" "symbol")"
        base_asset="$(extract_string_field "$record" "baseAsset")"
        quote_asset="$(extract_string_field "$record" "quoteAsset")"
        status="$(extract_string_field "$record" "status")"
        spot_allowed="$(extract_bool_field "$record" "isSpotTradingAllowed")"

        [ -n "$symbol" ] || continue
        if [ -n "$BASE_FILTER" ] && [ "$base_asset" != "$BASE_FILTER" ]; then
          continue
        fi
        if [ -n "$QUOTE_FILTER" ] && [ "$quote_asset" != "$QUOTE_FILTER" ]; then
          continue
        fi
        if [ -n "$STATUS_FILTER" ] && [ "$status" != "$STATUS_FILTER" ]; then
          continue
        fi
        if [ "$SPOT_ONLY" -eq 1 ] && [ "$spot_allowed" != "true" ]; then
          continue
        fi

        if [ "$SYMBOLS_ONLY" -eq 1 ]; then
          printf '%s\n' "$symbol"
        else
          printf '%s\t%s\t%s\t%s\t%s\n' \
            "$symbol" "$base_asset" "$quote_asset" "$status" "$spot_allowed"
        fi
      done \
    | sort
)"

if [ "$SYMBOLS_ONLY" -eq 1 ]; then
  printf '%s\n' "$rows"
else
  printf 'SYMBOL\tBASE\tQUOTE\tSTATUS\tSPOT_ALLOWED\n'
  printf '%s\n' "$rows"
fi
