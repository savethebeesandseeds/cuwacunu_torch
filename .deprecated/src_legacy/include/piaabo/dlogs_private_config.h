// Logging & diagnostics utilities for cuwacunu.
// This header intentionally keeps the "macro logger" API used across the project,
// but centralizes it away from dutils.h.
//
// Build-time logging flags:
//
// 1) DLOGS_USE_IOSTREAMS (default: 0)
//    - 0: terminal output goes through stdio FILE* (fprintf/fflush).
//    - 1: terminal output goes through std::cout/std::cerr (streambuf-capturable).
//
// 2) DLOGS_CHECK_SYS_ERRNO_BEFORE_LOG (default: 0)
//    - 0: do not emit pending errno before each log call.
//    - 1: emit and clear errno before each log call.
//
// 3) DLOGS_ENABLE_METADATA (default: 0)
//    - 0: keep buffered log entries minimal.
//    - 1: capture extra metadata in dlog buffer entries:
//         file/function/line, callsite id, pid, monotonic_ns, canonical scope path.
//
// 4) DLOGS_INCLUDE_METADATA_IN_FORMAT (default: 0)
//    - 0: dlog_format_entry() prints compact lines.
//    - 1: dlog_format_entry() appends captured metadata fields.
//
// 5) Output stream overrides (optional):
//    - LOG_FILE      (default: stdout)
//    - LOG_DBG_FILE  (default: LOG_FILE)
//    - LOG_WARN_FILE (default: LOG_ERR_FILE)
//    - LOG_ERR_FILE  (default: stderr)
//    Redefine before including this header to reroute stdio output sinks.
//
// iinuji compatibility:
// iinuji captures std::cout/std::cerr by replacing their streambufs.
// For dlogs output to appear inside iinuji (.sys.stdout/.sys.stderr), build with:
//
//   -DDLOGS_USE_IOSTREAMS=1
//   -DDLOGS_CHECK_SYS_ERRNO_BEFORE_LOG=1
//
// Optional metadata capture for iinuji dlog buffer views:
//
//   -DDLOGS_ENABLE_METADATA=1
//   -DDLOGS_INCLUDE_METADATA_IN_FORMAT=0   (recommended default for UI readability)
//
// Runtime helpers:
//   - DLOG_CANONICAL_SCOPE("canonical.path"): attach ambient canonical path metadata.
//   - DLOG_BUFFER_CAPTURE_SCOPE(false): disable buffer storage in noisy scopes (still prints).

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdarg>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <deque>
#include <vector>
#include <cstdint>
#include <ctime>
#include <algorithm>
#include <cstddef>
#include <atomic>
#include <string_view>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#ifndef DLOGS_USE_IOSTREAMS
#define DLOGS_USE_IOSTREAMS 0
#endif

#ifndef DLOGS_CHECK_SYS_ERRNO_BEFORE_LOG
#define DLOGS_CHECK_SYS_ERRNO_BEFORE_LOG 0
#endif

#ifndef DLOGS_ENABLE_METADATA
#define DLOGS_ENABLE_METADATA 0
#endif

#ifndef DLOGS_INCLUDE_METADATA_IN_FORMAT
#define DLOGS_INCLUDE_METADATA_IN_FORMAT 0
#endif

// DLOGS_ENABLE_METADATA:
//   0 -> keep entry payload minimal.
//   1 -> capture source callsite and process/monotonic metadata in dlog buffer.
// DLOGS_INCLUDE_METADATA_IN_FORMAT:
//   0 -> keep formatted lines compact (default for terminal/logs panel).
//   1 -> append captured metadata to dlog_format_entry().

#ifndef LOG_FILE
#define LOG_FILE stdout
#endif

#ifndef LOG_ERR_FILE
#define LOG_ERR_FILE stderr
#endif

#ifndef LOG_DBG_FILE
#define LOG_DBG_FILE LOG_FILE
#endif

#ifndef LOG_WARN_FILE
#define LOG_WARN_FILE LOG_ERR_FILE
#endif

/* ANSI colors */
#ifndef ANSI_COLOR_RESET
#define ANSI_COLOR_RESET "\x1b[0m"
#endif
#ifndef ANSI_CLEAR_LINE
#define ANSI_CLEAR_LINE "\r\033[2K"
#endif

#ifndef ANSI_COLOR_ERROR
#define ANSI_COLOR_ERROR "\x1b[41m"
#endif
#ifndef ANSI_COLOR_FATAL
#define ANSI_COLOR_FATAL "\x1b[41m"
#endif
#ifndef ANSI_COLOR_SUCCESS
#define ANSI_COLOR_SUCCESS "\x1b[42m"
#endif
#ifndef ANSI_COLOR_WARNING
#define ANSI_COLOR_WARNING "\x1b[43m"
#endif
#ifndef ANSI_COLOR_WARNING2
#define ANSI_COLOR_WARNING2 "\x1b[48;2;255;165;0m"
#endif

#ifndef ANSI_COLOR_Black
#define ANSI_COLOR_Black "\x1b[30m"
#endif
#ifndef ANSI_COLOR_Red
#define ANSI_COLOR_Red "\x1b[31m"
#endif
#ifndef ANSI_COLOR_Green
#define ANSI_COLOR_Green "\x1b[32m"
#endif
#ifndef ANSI_COLOR_Yellow
#define ANSI_COLOR_Yellow "\x1b[33m"
#endif
#ifndef ANSI_COLOR_Blue
#define ANSI_COLOR_Blue "\x1b[34m"
#endif
#ifndef ANSI_COLOR_Magenta
#define ANSI_COLOR_Magenta "\x1b[35m"
#endif
#ifndef ANSI_COLOR_Cyan
#define ANSI_COLOR_Cyan "\x1b[36m"
#endif
#ifndef ANSI_COLOR_White
#define ANSI_COLOR_White "\x1b[37m"
#endif

#ifndef ANSI_COLOR_Bright_Grey
#define ANSI_COLOR_Bright_Grey "\x1b[90m"
#endif
#ifndef ANSI_COLOR_Bright_Red
#define ANSI_COLOR_Bright_Red "\x1b[91m"
#endif
#ifndef ANSI_COLOR_Bright_Green
#define ANSI_COLOR_Bright_Green "\x1b[92m"
#endif
#ifndef ANSI_COLOR_Bright_Yellow
#define ANSI_COLOR_Bright_Yellow "\x1b[93m"
#endif
#ifndef ANSI_COLOR_Bright_Blue
#define ANSI_COLOR_Bright_Blue "\x1b[94m"
#endif
#ifndef ANSI_COLOR_Bright_Magenta
#define ANSI_COLOR_Bright_Magenta "\x1b[95m"
#endif
#ifndef ANSI_COLOR_Bright_Cyan
#define ANSI_COLOR_Bright_Cyan "\x1b[96m"
#endif
#ifndef ANSI_COLOR_Bright_White
#define ANSI_COLOR_Bright_White "\x1b[97m"
#endif

#ifndef ANSI_COLOR_Dim_Gray
#define ANSI_COLOR_Dim_Gray "\x1b[2;90m"
#endif
#ifndef ANSI_COLOR_Dim_Red
#define ANSI_COLOR_Dim_Red "\x1b[2;91m"
#endif
#ifndef ANSI_COLOR_Dim_Green
#define ANSI_COLOR_Dim_Green "\x1b[2;92m"
#endif
#ifndef ANSI_COLOR_Dim_Yellow
#define ANSI_COLOR_Dim_Yellow "\x1b[2;93m"
#endif
#ifndef ANSI_COLOR_Dim_Blue
#define ANSI_COLOR_Dim_Blue "\x1b[2;94m"
#endif
#ifndef ANSI_COLOR_Dim_Magenta
#define ANSI_COLOR_Dim_Magenta "\x1b[2;95m"
#endif
#ifndef ANSI_COLOR_Dim_Cyan
#define ANSI_COLOR_Dim_Cyan "\x1b[2;96m"
#endif
#ifndef ANSI_COLOR_Dim_White
#define ANSI_COLOR_Dim_White "\x1b[2;97m"
#endif

/* Stringify + concat helpers (guarded to avoid macro redefinition noise). */
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif
#ifndef CONCAT_INTERNAL
#define CONCAT_INTERNAL(x, y) x##y
#endif
#ifndef CONCAT
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)
#endif

/* One global mutex for all logging. Prefer header-only inline variable (C++17+). */
#if __cplusplus >= 201703L
inline std::mutex log_mutex;
#else
extern std::mutex log_mutex;
#endif

/* Unique lock-guard name to avoid collisions when used multiple times in one scope. */
#ifndef LOCK_GUARD
#define LOCK_GUARD(mutex_expr) std::lock_guard<std::mutex> CONCAT(_cuwacunu_lock_, __COUNTER__)(mutex_expr)
#endif

#ifndef CLEAR_SYS_ERR
#define CLEAR_SYS_ERR() do { errno = 0; } while(false)
#endif

/* Run-time throw/exit helpers kept as macros for compatibility. */
#ifndef THROW_RUNTIME_ERROR
#define THROW_RUNTIME_ERROR() do { throw std::runtime_error("Runtime error occurred"); } while(false)
#endif
#ifndef THROW_RUNTIME_FINALIZATION
#define THROW_RUNTIME_FINALIZATION() do { std::exit(0); } while(false)
#endif
