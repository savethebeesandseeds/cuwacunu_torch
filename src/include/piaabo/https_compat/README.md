# https_compat Specification

`https_compat` is the curl-backed transport layer for websocket workflows.

Namespace: `cuwacunu::piaabo::curl`
Public include form: `#include "piaabo/https_compat/..."`

## Core Contract

Two layers are exposed:

1. Global curl utilities (`curl_utils.h`)
- controlled process-level init/cleanup
- session creation helper
- websocket frame send helper

2. Session API (`WebsocketAPI` in `curl_websocket_api.h`)
- session lifecycle (`ws_init`, `ws_finalize`)
- async TX queueing (`ws_write_*` methods)
- RX collection and frame-id based response retrieval

## Session Model Contract

From implementation behavior:

- `WebsocketAPI` is singleton-style with static process-global registries
- one session id maps to curl handles, RX/TX deques, mutex, worker threads, trigger condition variable
- TX runs in a detached flush loop; RX runs via curl callback loop

## Request/Response Matching Contract

- outbound writes enqueue frames and return a generated or caller-provided `frame_id`
- inbound callback accumulates payload chunks and validates complete JSON framing
- response correlation uses JSON field `id` extracted from received payload
- `ws_wait_server_response(...)` waits up to `WS_MAX_WAIT` for matching `frame_id`

## Failure and Lifecycle Contract

- unknown/invalid session ids are fail-fast (`log_fatal` path)
- repeated global init/cleanup calls are tolerated with warnings
- send failures are logged; callers own retry/recovery policy

## Dependency Contract

- requires libcurl
- depends on `piaabo/djson_parsing.h` for payload framing and `id` extraction

## Legacy Ghosts

- No-internet / curl-init failures are still treated as fatal in core session-creation paths.
- RX response correlation currently assumes JSON payloads containing an `id` field.
- Chunk reassembly is JSON-framing-based and flagged for edge-case review under mixed/interleaved traffic.
- Wait-loop behavior has known timeout/infinite-wait risk notes in source warnings.
- `ws_write_text` currently assumes ASCII-style byte mapping and needs explicit encoding strategy.

This spec defines behavior-level contracts; source remains authoritative for transport internals.
