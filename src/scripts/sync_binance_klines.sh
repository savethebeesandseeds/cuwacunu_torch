#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
RAW_ROOT="${CUWACUNU_RAW_ROOT:-$REPO_ROOT/.data/raw}"
BASE_URL="https://data.binance.vision/data/spot/monthly/klines"

DEFAULT_INTERVALS=(1m 3m 5m 15m 30m 1h 2h 4h 6h 8h 12h 1d 3d 1w)
VICREG_SOLO_INTERVALS=(1w 3d 1d)

SYMBOLS=()
INTERVALS=("${DEFAULT_INTERVALS[@]}")
CUSTOM_INTERVALS=0
FROM_YM="2020-01"
TO_YM="$(date -u +%Y-%m)"
DRY_RUN=0
REBUILD_ONLY=0
ZIP_EXTRACTOR=""

usage() {
  cat <<EOF
usage: $(basename "$0") --symbol SYMBOL[,SYMBOL...] [options]

Download Binance spot monthly klines into:
  $RAW_ROOT/<SYMBOL>/<INTERVAL>/<YYYY>/<MM>/

The script verifies .CHECKSUM files, extracts monthly CSV files, and rebuilds:
  <SYMBOL>-<INTERVAL>-<YYYY>.csv
  <SYMBOL>-<INTERVAL>-all-years.csv

options:
  --symbol SYMBOLS      Comma-separated symbol list. Repeatable.
  --interval INTERVALS  Comma-separated interval list. Repeatable.
  --vicreg-solo         Use the current vicreg.solo active intervals: 1w,3d,1d.
  --from YYYY-MM        First monthly archive to consider. Default: ${FROM_YM}
  --to YYYY-MM          Last monthly archive to consider. Default: ${TO_YM}
  --raw-root PATH       Override raw data root. Default: ${RAW_ROOT}
  --rebuild-only        Skip downloads and only rebuild yearly/all-years CSVs.
  --dry-run             Print planned work without changing files.
  --help                Show this help text.

examples:
  bash src/scripts/$(basename "$0") --symbol BTCUSDT --vicreg-solo --from 2020-01 --to 2024-08
  bash src/scripts/$(basename "$0") --symbol ETHUSDT --interval 1d,3d,1w --from 2020-01 --to 2024-08
  bash src/scripts/$(basename "$0") --symbol BTCUSDT --rebuild-only
EOF
}

log() {
  printf '[sync_binance_klines] %s\n' "$*"
}

die() {
  printf '[sync_binance_klines] error: %s\n' "$*" >&2
  exit 1
}

require_command() {
  local cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || die "missing required command: $cmd"
}

select_zip_extractor() {
  if command -v unzip >/dev/null 2>&1; then
    ZIP_EXTRACTOR="unzip"
    return 0
  fi
  if command -v perl >/dev/null 2>&1 &&
      perl -MIO::Uncompress::Unzip -e 1 >/dev/null 2>&1; then
    ZIP_EXTRACTOR="perl"
    return 0
  fi
  return 1
}

trim_ascii() {
  local value="$1"
  value="${value#"${value%%[![:space:]]*}"}"
  value="${value%"${value##*[![:space:]]}"}"
  printf '%s' "$value"
}

append_csv_values() {
  local -n out_ref="$1"
  local raw="$2"
  local chunk trimmed
  IFS=',' read -r -a chunks <<< "$raw"
  for chunk in "${chunks[@]}"; do
    trimmed="$(trim_ascii "$chunk")"
    [ -n "$trimmed" ] || continue
    out_ref+=("$trimmed")
  done
}

validate_ym() {
  local ym="$1"
  if [[ ! "$ym" =~ ^([0-9]{4})-([0-9]{2})$ ]]; then
    return 1
  fi
  local month="${BASH_REMATCH[2]}"
  ((10#$month >= 1 && 10#$month <= 12))
}

ym_to_index() {
  local ym="$1"
  validate_ym "$ym" || die "invalid YYYY-MM value: $ym"
  local year="${BASH_REMATCH[1]}"
  local month="${BASH_REMATCH[2]}"
  printf '%d\n' $((10#$year * 12 + 10#$month - 1))
}

index_to_year_month() {
  local idx="$1"
  local year=$((idx / 12))
  local month=$((idx % 12 + 1))
  printf '%04d %02d\n' "$year" "$month"
}

remote_http_code() {
  local url="$1"
  curl -sS -L -o /dev/null -w '%{http_code}' -I "$url" || true
}

download_file() {
  local url="$1"
  local destination="$2"
  local tmp_path="${destination}.tmp.$$"
  if [ "$DRY_RUN" -eq 1 ]; then
    log "[dry-run] download $url -> $destination"
    return 0
  fi
  rm -f -- "$tmp_path"
  if ! curl -fsSL --retry 3 --connect-timeout 20 -o "$tmp_path" "$url"; then
    rm -f -- "$tmp_path"
    return 1
  fi
  mv -- "$tmp_path" "$destination"
}

ensure_remote_file() {
  local url="$1"
  local destination="$2"
  if [ -f "$destination" ]; then
    return 0
  fi

  local http_code
  http_code="$(remote_http_code "$url")"
  case "$http_code" in
    200)
      log "downloading $url"
      download_file "$url" "$destination"
      ;;
    404)
      log "remote archive missing: $url"
      return 2
      ;;
    '')
      die "failed to probe $url"
      ;;
    *)
      die "unexpected HTTP ${http_code} for $url"
      ;;
  esac
}

verify_checksum() {
  local zip_path="$1"
  local checksum_path="$2"
  local expected actual

  expected="$(awk '{print $1}' "$checksum_path")"
  [ -n "$expected" ] || return 1

  actual="$(sha256sum "$zip_path" | awk '{print $1}')"
  [ "$expected" = "$actual" ]
}

extract_month_archive() {
  local zip_path="$1"
  local csv_path="$2"
  local destination_dir="$3"
  if [ "$DRY_RUN" -eq 1 ]; then
    log "[dry-run] unzip $zip_path -> $destination_dir"
    return 0
  fi

  case "$ZIP_EXTRACTOR" in
    unzip)
      unzip -oq "$zip_path" -d "$destination_dir"
      ;;
    perl)
      perl -MIO::Uncompress::Unzip=unzip,\$UnzipError -e '
        use strict;
        use warnings;
        my ($zip_path, $csv_path) = @ARGV;
        my $z = IO::Uncompress::Unzip->new($zip_path)
          or die "unzip failed for $zip_path: $UnzipError\n";
        open my $out, q(>), $csv_path
          or die "open failed for $csv_path: $!\n";
        my $buffer = q();
        while (($z->read($buffer)) > 0) {
          print {$out} $buffer or die "write failed for $csv_path: $!\n";
        }
        my $status = $z->read($buffer);
        if (!defined $status) {
          die "read failed for $zip_path: $UnzipError\n";
        }
        close $out or die "close failed for $csv_path: $!\n";
      ' "$zip_path" "$csv_path"
      ;;
    *)
      die "no ZIP extractor selected"
      ;;
  esac
}

sync_month() {
  local symbol="$1"
  local interval="$2"
  local year="$3"
  local month="$4"

  local month_dir="$RAW_ROOT/$symbol/$interval/$year/$month"
  local stem="${symbol}-${interval}-${year}-${month}"
  local zip_url="${BASE_URL}/${symbol}/${interval}/${stem}.zip"
  local checksum_url="${zip_url}.CHECKSUM"
  local zip_path="${month_dir}/${stem}.zip"
  local checksum_path="${zip_path}.CHECKSUM"
  local csv_path="${month_dir}/${stem}.csv"
  local downloaded_any=0
  local rc=0

  mkdir -p -- "$month_dir"

  if [ ! -f "$zip_path" ]; then
    rc=0
    ensure_remote_file "$zip_url" "$zip_path" || rc=$?
    case "$rc" in
      0) downloaded_any=1 ;;
      2) return 0 ;;
      *) return "$rc" ;;
    esac
  fi

  if [ ! -f "$checksum_path" ]; then
    rc=0
    ensure_remote_file "$checksum_url" "$checksum_path" || rc=$?
    case "$rc" in
      0) downloaded_any=1 ;;
      2)
        if [ "$downloaded_any" -eq 1 ] && [ "$DRY_RUN" -eq 0 ]; then
          rm -f -- "$zip_path"
        fi
        die "missing checksum for $zip_url"
        ;;
      *)
        return "$rc"
        ;;
    esac
  fi

  if [ "$DRY_RUN" -eq 0 ] && ! verify_checksum "$zip_path" "$checksum_path"; then
    log "checksum mismatch for $zip_path, refreshing archive"
    rm -f -- "$zip_path" "$checksum_path"
    ensure_remote_file "$zip_url" "$zip_path" || die "failed to refresh $zip_url"
    ensure_remote_file "$checksum_url" "$checksum_path" || die "failed to refresh ${checksum_url}"
    verify_checksum "$zip_path" "$checksum_path" || die "checksum mismatch persists for $zip_path"
    downloaded_any=1
  fi

  if [ "$downloaded_any" -eq 1 ] || [ ! -f "$csv_path" ]; then
    extract_month_archive "$zip_path" "$csv_path" "$month_dir"
  fi
}

rebuild_interval_rollups() {
  local symbol="$1"
  local interval="$2"
  local interval_dir="$RAW_ROOT/$symbol/$interval"
  local year_dir year yearly_path
  local yearly_paths=()

  if [ ! -d "$interval_dir" ]; then
    log "no local interval directory for ${symbol}/${interval}, skipping rebuild"
    return 0
  fi

  shopt -s nullglob
  for year_dir in "$interval_dir"/[0-9][0-9][0-9][0-9]; do
    [ -d "$year_dir" ] || continue
    year="$(basename "$year_dir")"
    local month_csvs=( "$year_dir"/??/"${symbol}-${interval}-${year}"-??.csv )
    if [ "${#month_csvs[@]}" -eq 0 ]; then
      continue
    fi

    yearly_path="${year_dir}/${symbol}-${interval}-${year}.csv"
    if [ "$DRY_RUN" -eq 1 ]; then
      log "[dry-run] rebuild $yearly_path from ${#month_csvs[@]} monthly files"
    else
      cat "${month_csvs[@]}" > "$yearly_path"
    fi
    yearly_paths+=("$yearly_path")
  done
  shopt -u nullglob

  if [ "${#yearly_paths[@]}" -eq 0 ]; then
    log "no monthly CSV files found for ${symbol}/${interval}, skipping all-years rebuild"
    return 0
  fi

  local all_years_path="${interval_dir}/${symbol}-${interval}-all-years.csv"
  if [ "$DRY_RUN" -eq 1 ]; then
    log "[dry-run] rebuild $all_years_path from ${#yearly_paths[@]} yearly files"
  else
    cat "${yearly_paths[@]}" > "$all_years_path"
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --symbol)
      [ "$#" -ge 2 ] || die "--symbol requires a value"
      append_csv_values SYMBOLS "$2"
      shift 2
      ;;
    --interval)
      [ "$#" -ge 2 ] || die "--interval requires a value"
      if [ "$CUSTOM_INTERVALS" -eq 0 ]; then
        INTERVALS=()
        CUSTOM_INTERVALS=1
      fi
      append_csv_values INTERVALS "$2"
      shift 2
      ;;
    --vicreg-solo)
      INTERVALS=("${VICREG_SOLO_INTERVALS[@]}")
      CUSTOM_INTERVALS=1
      shift
      ;;
    --from)
      [ "$#" -ge 2 ] || die "--from requires a value"
      FROM_YM="$2"
      shift 2
      ;;
    --to)
      [ "$#" -ge 2 ] || die "--to requires a value"
      TO_YM="$2"
      shift 2
      ;;
    --raw-root)
      [ "$#" -ge 2 ] || die "--raw-root requires a value"
      RAW_ROOT="$2"
      shift 2
      ;;
    --rebuild-only)
      REBUILD_ONLY=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
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

[ "${#SYMBOLS[@]}" -gt 0 ] || {
  usage
  exit 2
}
[ "${#INTERVALS[@]}" -gt 0 ] || die "at least one interval is required"

require_command curl
require_command awk
require_command sed
require_command date

if [ "$REBUILD_ONLY" -eq 0 ]; then
  if [ "$DRY_RUN" -eq 0 ]; then
    require_command sha256sum
    select_zip_extractor || die "missing ZIP extractor: need unzip or perl IO::Uncompress::Unzip"
  fi
fi

validate_ym "$FROM_YM" || die "invalid --from value: $FROM_YM"
validate_ym "$TO_YM" || die "invalid --to value: $TO_YM"

from_idx="$(ym_to_index "$FROM_YM")"
to_idx="$(ym_to_index "$TO_YM")"
if [ "$from_idx" -gt "$to_idx" ]; then
  die "--from must be earlier than or equal to --to"
fi

for symbol in "${SYMBOLS[@]}"; do
  for interval in "${INTERVALS[@]}"; do
    log "syncing ${symbol}/${interval} from ${FROM_YM} to ${TO_YM}"
    if [ "$REBUILD_ONLY" -eq 0 ]; then
      for ((idx = from_idx; idx <= to_idx; ++idx)); do
        read -r year month <<< "$(index_to_year_month "$idx")"
        sync_month "$symbol" "$interval" "$year" "$month"
      done
    fi
    rebuild_interval_rollups "$symbol" "$interval"
  done
done

log "done"
