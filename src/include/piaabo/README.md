# piaabo Specification

`piaabo` is the runtime foundation layer for cuwacunu. It provides shared infrastructure used by higher modules, not domain trading logic.

Primary namespace: `cuwacunu::piaabo`
Public include form: `#include "piaabo/..."`

## Core Contract

`piaabo` owns these cross-cutting guarantees:

- consistent diagnostics and fail-fast behavior
- typed utility/file/json primitives reused across modules
- explicit secret-memory and crypto interfaces
- runtime config loading and contract snapshot locking
- compatibility bridges (BNF parser, torch utilities, curl/websocket)

## dlogs Dual-Mode Contract (Important)

Header: `src/include/piaabo/dlogs.h`

`dlogs` has two output modes:

1. Standard mode (`DLOGS_USE_IOSTREAMS=0`, default)
- writes terminal output through stdio (`stdout`/`stderr`, configurable via `LOG_FILE`, `LOG_ERR_FILE`, `LOG_WARN_FILE`)

2. iinuji-enabled mode (`DLOGS_USE_IOSTREAMS=1`)
- writes through `std::cout` / `std::cerr`
- this exists because iinuji captures C++ stream buffers; this makes logs visible inside the iinuji shell streams (`.sys.stdout` / `.sys.stderr`)

Shared behavior in both modes:

- log macros capture entries into an in-memory ring buffer (`dlog_entry_t`) via `dlog_push`
- ANSI escapes are stripped before buffering
- `dlog_set_terminal_output_enabled(false)` suppresses terminal emission, but buffered capture continues
- iinuji command mode typically disables direct terminal emission and renders from the buffered snapshot (`dlog_snapshot`/`dlog_entry_t`) in its logs view
- `log_fatal` / `log_secure_fatal` throw runtime error
- `log_terminate_gracefully` exits process
- `log_sys_errno_*` logs and clears `errno`

## Module Contracts

### `dutils.h`
- common string/time/hash/conversion helpers
- `fnv1aHash` is the shared stable hash primitive used by other modules
- conversion helpers fail-fast on invalid values where specified (`string_cast` specializations)

### `dfiles.h`
- typed file utilities and CSV/binary conversion templates
- `csvFile_to_binary<T>` requires `T::from_csv(...)`
- `binaryFile_to_vector<T>` requires `T::from_binary(...)`
- does not perform cross-endianness normalization

### `djson_parsing.h`
- in-house JSON value model (`JsonValue`) and parser (`JsonParser`)
- includes a lightweight framing validator (`json_fast_validity_check`) used in streaming paths

### `dsecurity.h` and `dencryption.h`
- process/memory hardening (`mlock`, `prctl`, zeroization)
- secure credential holder (`SecureStronghold_t`, global `SecureVault`)
- OpenSSL-backed AEAD + Ed25519 signing interfaces

### `dconfig.h` (`config_space_t` + `contract_space_t`)
- runtime config access and typed retrieval
- immutable, hash-addressed contract snapshots with dependency fingerprinting
- intended to detect contract drift and fail fast when integrity constraints break

### `math_compat/statistics_space.h`
- running and rolling statistics primitives used for normalization workflows

### Compat surfaces
- `bnf_compat`: grammar + instruction parsing to AST
- `torch_compat`: torch device/dtype helpers, validation, distribution shims
- `https_compat`: curl global lifecycle + websocket session API

## External Dependencies

Used conditionally by header/module:

- OpenSSL (`dencryption`, `dsecurity`)
- libcurl (`https_compat`)
- libtorch (`torch_compat`)

## Legacy Ghosts

- `dlogs` remains macro-first and dual-sink for compatibility; a typed logging API is still pending.
- Fail-fast behavior is dominant across modules; structured recovery paths are sparse.
- `dconfig` still carries explicit removed-key migration guards for old contract/config shapes.
- `bnf_compat` has known parser debt (verification quality, unused flags in instruction parsing).
- `torch_compat` has unresolved RNG-seeding and float-vs-double consistency debt.
- `https_compat` still carries websocket assumptions (`id`-based JSON correlation, timeout/threading edge cases).
- `math_compat` assumes uniformly spaced samples and still lacks planned indicator/feature expansions.

This spec defines behavior-level contracts. Source remains authoritative for implementation details.
