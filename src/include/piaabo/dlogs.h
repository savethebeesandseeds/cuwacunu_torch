#pragma once

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
//    - LOG_WARN_FILE (default: stdout)
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

#ifndef LOG_WARN_FILE
#define LOG_WARN_FILE stdout
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

namespace cuwacunu {
namespace piaabo {

inline const char* cthread_id();

struct dlog_source_location_t {
  const char* file{nullptr};
  const char* function{nullptr};
  int line{0};
  const char* canonical_path{nullptr};
};

inline constexpr dlog_source_location_t dlog_source_location(
    const char* file,
    const char* function,
    int line,
    const char* canonical_path = nullptr) {
  return dlog_source_location_t{
      .file = file,
      .function = function,
      .line = line,
      .canonical_path = canonical_path};
}

struct dlog_entry_t {
  std::uint64_t seq{0};
  std::string timestamp{};
  std::string level{};
  std::string thread{};
  std::string message{};
#if DLOGS_ENABLE_METADATA
  std::uint64_t monotonic_ns{0};
  std::uint64_t callsite_id{0};
  std::uint32_t pid{0};
  std::uint32_t line{0};
  std::string source_file{};
  std::string source_function{};
  std::string source_path{};
#endif
};

inline std::mutex& dlog_buffer_mutex() {
  static std::mutex m;
  return m;
}

inline std::deque<dlog_entry_t>& dlog_buffer_storage() {
  static std::deque<dlog_entry_t> q;
  return q;
}

inline std::size_t& dlog_buffer_capacity_storage() {
  static std::size_t cap = 4096;
  return cap;
}

inline std::uint64_t& dlog_sequence_storage() {
  static std::uint64_t seq = 0;
  return seq;
}

inline std::string dlog_now_timestamp() {
  using clock = std::chrono::system_clock;
  const auto now = clock::now();
  const auto tt = clock::to_time_t(now);
  std::tm tmv{};
#if defined(_WIN32)
  localtime_s(&tmv, &tt);
#else
  localtime_r(&tt, &tmv);
#endif
  char datebuf[32];
  std::strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M:%S", &tmv);

  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) % 1000;
  char out[48];
  std::snprintf(out, sizeof(out), "%s.%03lld", datebuf, (long long)ms.count());
  return std::string(out);
}

inline std::string strip_ansi_escapes(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(in[i]);
    if (c == 0x1B) {
      // Skip CSI escape sequences (ESC [ ... final-byte).
      if (i + 1 < in.size() && in[i + 1] == '[') {
        i += 2;
        while (i < in.size()) {
          const unsigned char cc = static_cast<unsigned char>(in[i]);
          if (cc >= 0x40 && cc <= 0x7E) break;
          ++i;
        }
      }
      continue;
    }
    out.push_back(static_cast<char>(c));
  }
  return out;
}

inline void dlog_set_buffer_capacity(std::size_t cap) {
  LOCK_GUARD(dlog_buffer_mutex());
  auto& storage = dlog_buffer_storage();
  auto& current = dlog_buffer_capacity_storage();
  current = std::max<std::size_t>(1, cap);
  while (storage.size() > current) storage.pop_front();
}

inline std::size_t dlog_buffer_capacity() {
  LOCK_GUARD(dlog_buffer_mutex());
  return dlog_buffer_capacity_storage();
}

inline std::size_t dlog_buffer_size() {
  LOCK_GUARD(dlog_buffer_mutex());
  return dlog_buffer_storage().size();
}

inline void dlog_clear_buffer() {
  LOCK_GUARD(dlog_buffer_mutex());
  dlog_buffer_storage().clear();
}

inline std::atomic_bool& dlog_terminal_output_enabled_storage() {
  static std::atomic_bool enabled{true};
  return enabled;
}

inline void dlog_set_terminal_output_enabled(bool enabled) {
  dlog_terminal_output_enabled_storage().store(enabled, std::memory_order_relaxed);
}

inline bool dlog_terminal_output_enabled() {
  return dlog_terminal_output_enabled_storage().load(std::memory_order_relaxed);
}

inline bool& dlog_thread_buffer_capture_enabled_storage() {
  thread_local bool enabled = true;
  return enabled;
}

inline void dlog_set_thread_buffer_capture_enabled(bool enabled) {
  dlog_thread_buffer_capture_enabled_storage() = enabled;
}

inline bool dlog_thread_buffer_capture_enabled() {
  return dlog_thread_buffer_capture_enabled_storage();
}

class dlog_buffer_capture_scope final {
 public:
  explicit dlog_buffer_capture_scope(bool enabled)
      : prev_(dlog_thread_buffer_capture_enabled()) {
    dlog_set_thread_buffer_capture_enabled(enabled);
  }
  ~dlog_buffer_capture_scope() { dlog_set_thread_buffer_capture_enabled(prev_); }

  dlog_buffer_capture_scope(const dlog_buffer_capture_scope&) = delete;
  dlog_buffer_capture_scope& operator=(const dlog_buffer_capture_scope&) =
      delete;

 private:
  bool prev_{true};
};

inline std::vector<std::string>& dlog_canonical_scope_stack_storage() {
  thread_local std::vector<std::string> stack;
  return stack;
}

inline std::string dlog_current_canonical_scope_path() {
  const auto& stack = dlog_canonical_scope_stack_storage();
  if (stack.empty()) return {};
  return stack.back();
}

class dlog_canonical_scope final {
 public:
  explicit dlog_canonical_scope(std::string canonical_path) {
    if (!canonical_path.empty()) {
      dlog_canonical_scope_stack_storage().push_back(std::move(canonical_path));
      active_ = true;
    }
  }
  ~dlog_canonical_scope() {
    if (!active_) return;
    auto& stack = dlog_canonical_scope_stack_storage();
    if (!stack.empty()) stack.pop_back();
  }

  dlog_canonical_scope(const dlog_canonical_scope&) = delete;
  dlog_canonical_scope& operator=(const dlog_canonical_scope&) = delete;

 private:
  bool active_{false};
};

inline std::string dlog_normalize_path(std::string path) {
  for (char& ch : path) {
    if (ch == '\\') ch = '/';
  }
  return path;
}

inline std::uint64_t dlog_hash_callsite(std::string_view file,
                                        std::string_view function,
                                        std::uint32_t line) {
  std::uint64_t h = 14695981039346656037ULL;
  constexpr std::uint64_t p = 1099511628211ULL;
  const auto mix = [&](std::string_view v) {
    for (char c : v) {
      h ^= static_cast<unsigned char>(c);
      h *= p;
    }
  };
  mix(file);
  h ^= static_cast<std::uint64_t>(0xFFU);
  h *= p;
  mix(function);
  h ^= static_cast<std::uint64_t>(0xFEU);
  h *= p;
  h ^= static_cast<std::uint64_t>(line & 0xFFU);
  h *= p;
  h ^= static_cast<std::uint64_t>((line >> 8) & 0xFFU);
  h *= p;
  h ^= static_cast<std::uint64_t>((line >> 16) & 0xFFU);
  h *= p;
  h ^= static_cast<std::uint64_t>((line >> 24) & 0xFFU);
  h *= p;
  return h;
}

inline std::uint64_t dlog_monotonic_ns() {
  using steady = std::chrono::steady_clock;
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          steady::now().time_since_epoch())
          .count());
}

inline std::uint32_t dlog_process_id() {
#if defined(_WIN32)
  return static_cast<std::uint32_t>(_getpid());
#else
  return static_cast<std::uint32_t>(::getpid());
#endif
}

inline void dlog_attach_metadata(
    dlog_entry_t* entry,
    const dlog_source_location_t* source) {
  if (!entry) return;
#if DLOGS_ENABLE_METADATA
  entry->monotonic_ns = dlog_monotonic_ns();
  entry->pid = dlog_process_id();

  std::string file;
  std::string function;
  std::uint32_t line = 0;
  if (source) {
    if (source->file) file = dlog_normalize_path(source->file);
    if (source->function) function = source->function;
    if (source->line > 0) line = static_cast<std::uint32_t>(source->line);
  }
  entry->line = line;
  entry->source_file = file;
  entry->source_function = function;
  entry->callsite_id = dlog_hash_callsite(file, function, line);

  if (source && source->canonical_path && source->canonical_path[0] != '\0') {
    entry->source_path = dlog_normalize_path(source->canonical_path);
  } else {
    entry->source_path = dlog_current_canonical_scope_path();
  }
#else
  (void)source;
#endif
}

inline void dlog_push(const std::string& level, std::string message) {
  if (!dlog_thread_buffer_capture_enabled()) return;
  const std::string clean = strip_ansi_escapes(message);
  const std::string thread = cthread_id();

  LOCK_GUARD(dlog_buffer_mutex());
  auto& storage = dlog_buffer_storage();
  auto& seq = dlog_sequence_storage();
  const std::size_t cap = dlog_buffer_capacity_storage();

  std::size_t start = 0;
  bool pushed = false;
  while (start <= clean.size()) {
    const std::size_t end = clean.find('\n', start);
    std::string line = (end == std::string::npos)
                           ? clean.substr(start)
                           : clean.substr(start, end - start);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) {
      dlog_entry_t entry{};
      entry.seq = ++seq;
      entry.timestamp = dlog_now_timestamp();
      entry.level = level.empty() ? "INFO" : level;
      entry.thread = thread;
      entry.message = std::move(line);
      dlog_attach_metadata(&entry, nullptr);
      storage.push_back(std::move(entry));
      pushed = true;
      while (storage.size() > cap) storage.pop_front();
    }

    if (end == std::string::npos) break;
    start = end + 1;
  }

  if (!pushed) {
    dlog_entry_t entry{};
    entry.seq = ++seq;
    entry.timestamp = dlog_now_timestamp();
    entry.level = level.empty() ? "INFO" : level;
    entry.thread = thread;
    entry.message = "<empty>";
    dlog_attach_metadata(&entry, nullptr);
    storage.push_back(std::move(entry));
    while (storage.size() > cap) storage.pop_front();
  }
}

inline void dlog_push_with_source(const std::string& level,
                                  std::string message,
                                  const dlog_source_location_t& source) {
  if (!dlog_thread_buffer_capture_enabled()) return;
  const std::string clean = strip_ansi_escapes(message);
  const std::string thread = cthread_id();

  LOCK_GUARD(dlog_buffer_mutex());
  auto& storage = dlog_buffer_storage();
  auto& seq = dlog_sequence_storage();
  const std::size_t cap = dlog_buffer_capacity_storage();

  std::size_t start = 0;
  bool pushed = false;
  while (start <= clean.size()) {
    const std::size_t end = clean.find('\n', start);
    std::string line = (end == std::string::npos)
                           ? clean.substr(start)
                           : clean.substr(start, end - start);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) {
      dlog_entry_t entry{};
      entry.seq = ++seq;
      entry.timestamp = dlog_now_timestamp();
      entry.level = level.empty() ? "INFO" : level;
      entry.thread = thread;
      entry.message = std::move(line);
      dlog_attach_metadata(&entry, &source);
      storage.push_back(std::move(entry));
      pushed = true;
      while (storage.size() > cap) storage.pop_front();
    }

    if (end == std::string::npos) break;
    start = end + 1;
  }

  if (!pushed) {
    dlog_entry_t entry{};
    entry.seq = ++seq;
    entry.timestamp = dlog_now_timestamp();
    entry.level = level.empty() ? "INFO" : level;
    entry.thread = thread;
    entry.message = "<empty>";
    dlog_attach_metadata(&entry, &source);
    storage.push_back(std::move(entry));
    while (storage.size() > cap) storage.pop_front();
  }
}

inline std::string dlog_format_entry(const dlog_entry_t& e) {
  std::ostringstream oss;
  oss << "[" << e.timestamp << "] "
      << "[0x" << e.thread << "] "
      << "[" << e.level << "] "
      << e.message;
#if DLOGS_ENABLE_METADATA && DLOGS_INCLUDE_METADATA_IN_FORMAT
  if (!e.source_function.empty()) {
    oss << " "
        << "{fn=" << e.source_function;
    if (!e.source_file.empty()) {
      oss << " file=" << e.source_file;
      if (e.line > 0) oss << ":" << e.line;
    }
    if (!e.source_path.empty()) {
      oss << " path=" << e.source_path;
    }
    oss << " cs=0x" << std::hex << e.callsite_id << std::dec
        << " pid=" << e.pid
        << " mono_ns=" << e.monotonic_ns
        << "}";
  }
#endif
  return oss.str();
}

inline std::vector<dlog_entry_t> dlog_snapshot(std::size_t max_entries = 0) {
  LOCK_GUARD(dlog_buffer_mutex());
  const auto& storage = dlog_buffer_storage();
  if (max_entries == 0 || max_entries >= storage.size()) {
    return std::vector<dlog_entry_t>(storage.begin(), storage.end());
  }
  return std::vector<dlog_entry_t>(storage.end() - static_cast<std::ptrdiff_t>(max_entries),
                                   storage.end());
}

inline std::vector<std::string> dlog_snapshot_lines(std::size_t max_entries = 0) {
  const auto snap = dlog_snapshot(max_entries);
  std::vector<std::string> lines;
  lines.reserve(snap.size());
  for (const auto& e : snap) lines.push_back(dlog_format_entry(e));
  return lines;
}

/**
 * @brief Escapes a small set of shell/console-sensitive characters in-place.
 *
 * Characters escaped: \  "  $  `
 *
 * @param input   NUL-terminated string buffer to sanitize.
 * @param max_len Total capacity of @p input (including NUL terminator).
 *
 * @note This function truncates to fit within @p max_len.
 */
inline void sanitize_string(char* input, size_t max_len) {
  if (!input || max_len == 0) return;

  constexpr size_t kBufCap = 2048;
  const size_t cap = (max_len < kBufCap) ? max_len : kBufCap;

  char out[kBufCap];
  size_t j = 0;

  for (size_t i = 0; input[i] != '\0'; ++i) {
    const char c = input[i];
    const bool needs_escape = (c == '`' || c == '$' || c == '"' || c == '\\');

    if (needs_escape) {
      if (j + 2 >= cap) break;   // need room for '\' + char + '\0'
      out[j++] = '\\';
      out[j++] = c;
    } else {
      if (j + 1 >= cap) break;   // need room for char + '\0'
      out[j++] = c;
    }
  }

  out[j] = '\0';
  std::strncpy(input, out, cap - 1);
  input[cap - 1] = '\0';
}

/**
 * @brief Thread id as a stable C-string for the current thread.
 *
 * Uses thread_local storage to avoid data races between threads.
 */
inline const char* cthread_id() {
  thread_local std::string threadID;
  if (threadID.empty()) {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    threadID = oss.str();
  }
  return threadID.c_str();
}

/* compile-time basename extractor for both '/' and '\\' */
inline constexpr const char* path_basename(const char* path) noexcept {
  const char* base = path;
  for (const char* p = path; *p; ++p)
    if (*p == '/' || *p == '\\') base = p + 1;
  return base;
}

namespace detail {

inline void capture_formatted_log_v(const char* level,
                                    const char* fmt,
                                    std::va_list args);
inline void capture_formatted_log(const char* level, const char* fmt, ...);
inline void capture_formatted_log_source_v(const char* level,
                                           const dlog_source_location_t& source,
                                           const char* fmt,
                                           std::va_list args);
inline void capture_formatted_log_source(const char* level,
                                         const dlog_source_location_t& source,
                                         const char* fmt,
                                         ...);

inline void vsecure_log(FILE* out,
                        const char* level_label,
                        const char* level_color,
                        const char* fmt,
                        std::va_list args) {
  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) return;
  char temp[2048];

  int written = 0;
  if (level_label && level_color) {
    written = std::snprintf(
      temp, sizeof(temp),
      "[%s0x%s%s]: %s%s%s: ",
      ANSI_COLOR_Cyan, cthread_id(), ANSI_COLOR_RESET,
      level_color, level_label, ANSI_COLOR_RESET
    );
  } else {
    written = std::snprintf(
      temp, sizeof(temp),
      "[%s0x%s%s]: ",
      ANSI_COLOR_Cyan, cthread_id(), ANSI_COLOR_RESET
    );
  }

  if (written < 0) return;
  if (static_cast<size_t>(written) >= sizeof(temp)) written = static_cast<int>(sizeof(temp) - 1);

  const size_t prefix_len = static_cast<size_t>(written);
  const size_t avail = (prefix_len < sizeof(temp)) ? (sizeof(temp) - prefix_len) : 0;

  bool truncated = false;
  if (avail > 0) {
    const int r = std::vsnprintf(temp + prefix_len, avail, fmt, args);
    if (r < 0) {
      temp[prefix_len] = '\0';
    } else if (static_cast<size_t>(r) >= avail) {
      truncated = true;
    }
  } else {
    truncated = true;
  }

  sanitize_string(temp, sizeof(temp));
  const size_t len = std::strlen(temp);
  const bool needs_newline = (len == 0) ? true : (temp[len - 1] != '\n');

  std::fprintf(out, "%s%s%s",
    temp,
    truncated ? "...[message truncated]" : "",
    needs_newline ? "\n" : ""
  );
  std::fflush(out);
}

inline void secure_log(FILE* out,
                       const char* level_label,
                       const char* level_color,
                       const char* fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  std::va_list args_copy;
  va_copy(args_copy, args);
  capture_formatted_log_v(level_label, fmt, args_copy);
  va_end(args_copy);
  LOCK_GUARD(log_mutex);
  vsecure_log(out, level_label, level_color, fmt, args);
  va_end(args);
}

inline void secure_log_source(FILE* out,
                              const char* level_label,
                              const char* level_color,
                              const dlog_source_location_t& source,
                              const char* fmt,
                              ...) {
  std::va_list args;
  va_start(args, fmt);
  std::va_list args_copy;
  va_copy(args_copy, args);
  capture_formatted_log_source_v(level_label, source, fmt, args_copy);
  va_end(args_copy);
  LOCK_GUARD(log_mutex);
  vsecure_log(out, level_label, level_color, fmt, args);
  va_end(args);
}

/* ---------------- iostream helpers (for iinuji capture) ---------------- */

inline std::string vformat(const char* fmt, std::va_list args) {
  if (!fmt) return std::string();

  std::va_list ap1;
  va_copy(ap1, args);
  const int n = std::vsnprintf(nullptr, 0, fmt, ap1);
  va_end(ap1);

  if (n <= 0) return std::string();

  std::string buf;
  buf.resize((size_t)n + 1);

  std::va_list ap2;
  va_copy(ap2, args);
  (void)std::vsnprintf(buf.data(), buf.size(), fmt, ap2);
  va_end(ap2);

  // drop trailing NUL
  buf.pop_back();
  return buf;
}

inline void capture_formatted_log_v(const char* level,
                                    const char* fmt,
                                    std::va_list args) {
  if (!fmt) return;
  std::string msg = vformat(fmt, args);
  if (msg.empty()) return;
  cuwacunu::piaabo::dlog_push(level ? level : "INFO", std::move(msg));
}

inline void capture_formatted_log(const char* level, const char* fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  capture_formatted_log_v(level, fmt, args);
  va_end(args);
}

inline void capture_formatted_log_source_v(const char* level,
                                           const dlog_source_location_t& source,
                                           const char* fmt,
                                           std::va_list args) {
  if (!fmt) return;
  std::string msg = vformat(fmt, args);
  if (msg.empty()) return;
  cuwacunu::piaabo::dlog_push_with_source(level ? level : "INFO",
                                          std::move(msg),
                                          source);
}

inline void capture_formatted_log_source(const char* level,
                                         const dlog_source_location_t& source,
                                         const char* fmt,
                                         ...) {
  std::va_list args;
  va_start(args, fmt);
  capture_formatted_log_source_v(level, source, fmt, args);
  va_end(args);
}

inline void vstream_printf(std::ostream& out, const char* fmt, std::va_list args) {
  out << vformat(fmt, args);
}

inline void stream_printf(std::ostream& out, const char* fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  vstream_printf(out, fmt, args);
  va_end(args);
}

inline void vsecure_log_stream(std::ostream& out,
                               const char* level_label,
                               const char* level_color,
                               const char* fmt,
                               std::va_list args) {
  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) return;
  char temp[2048];

  int written = 0;
  if (level_label && level_color) {
    written = std::snprintf(
      temp, sizeof(temp),
      "[%s0x%s%s]: %s%s%s: ",
      ANSI_COLOR_Cyan, cthread_id(), ANSI_COLOR_RESET,
      level_color, level_label, ANSI_COLOR_RESET
    );
  } else {
    written = std::snprintf(
      temp, sizeof(temp),
      "[%s0x%s%s]: ",
      ANSI_COLOR_Cyan, cthread_id(), ANSI_COLOR_RESET
    );
  }

  if (written < 0) return;
  if (static_cast<size_t>(written) >= sizeof(temp)) written = static_cast<int>(sizeof(temp) - 1);

  const size_t prefix_len = static_cast<size_t>(written);
  const size_t avail = (prefix_len < sizeof(temp)) ? (sizeof(temp) - prefix_len) : 0;

  bool truncated = false;
  if (avail > 0) {
    const int r = std::vsnprintf(temp + prefix_len, avail, fmt, args);
    if (r < 0) {
      temp[prefix_len] = '\0';
    } else if (static_cast<size_t>(r) >= avail) {
      truncated = true;
    }
  } else {
    truncated = true;
  }

  sanitize_string(temp, sizeof(temp));
  const size_t len = std::strlen(temp);
  const bool needs_newline = (len == 0) ? true : (temp[len - 1] != '\n');

  out << temp;
  if (truncated) out << "...[message truncated]";
  if (needs_newline) out << "\n";
  out.flush();
}

inline void secure_log_stream(std::ostream& out,
                              const char* level_label,
                              const char* level_color,
                              const char* fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  std::va_list args_copy;
  va_copy(args_copy, args);
  capture_formatted_log_v(level_label, fmt, args_copy);
  va_end(args_copy);
  LOCK_GUARD(log_mutex);
  vsecure_log_stream(out, level_label, level_color, fmt, args);
  va_end(args);
}

inline void secure_log_stream_source(std::ostream& out,
                                     const char* level_label,
                                     const char* level_color,
                                     const dlog_source_location_t& source,
                                     const char* fmt,
                                     ...) {
  std::va_list args;
  va_start(args, fmt);
  std::va_list args_copy;
  va_copy(args_copy, args);
  capture_formatted_log_source_v(level_label, source, fmt, args_copy);
  va_end(args_copy);
  LOCK_GUARD(log_mutex);
  vsecure_log_stream(out, level_label, level_color, fmt, args);
  va_end(args);
}

} // namespace detail

/* ---- errno wrapper ----
 * WARNING: this clears errno when it logs it.
 */
inline void log_sys_errno_stream() {
  if (errno == 0) return;
  const int err = errno;
  cuwacunu::piaabo::dlog_push("SYS_ERRNO",
                              "[" + std::to_string(err) + "] " + std::strerror(err));
  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) {
    errno = 0;
    return;
  }
  LOCK_GUARD(log_mutex);
  std::cerr << "["
            << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET
            << "]: "
            << ANSI_COLOR_ERROR << "SYS ERRNO" << ANSI_COLOR_RESET
            << ": [" << err << "] " << std::strerror(err) << "\n";
  std::cerr.flush();
  errno = 0;
}

inline void log_sys_errno_stdio() {
  if (errno == 0) return;
  const int err = errno;
  cuwacunu::piaabo::dlog_push("SYS_ERRNO",
                              "[" + std::to_string(err) + "] " + std::strerror(err));
  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) {
    errno = 0;
    return;
  }
  LOCK_GUARD(log_mutex);
  std::fprintf(LOG_ERR_FILE, "[%s0x%s%s]: %sSYS ERRNO%s: [%d] %s\n",
               ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET,
               ANSI_COLOR_ERROR, ANSI_COLOR_RESET,
               err, std::strerror(err));
  std::fflush(LOG_ERR_FILE);
  errno = 0;
}

} // namespace piaabo
} // namespace cuwacunu

#if DLOGS_CHECK_SYS_ERRNO_BEFORE_LOG
#if DLOGS_USE_IOSTREAMS
#define wrap_log_sys_err() do { cuwacunu::piaabo::log_sys_errno_stream(); } while(false)
#else
#define wrap_log_sys_err() do { cuwacunu::piaabo::log_sys_errno_stdio(); } while(false)
#endif
#else
#define wrap_log_sys_err() do {} while(false)
#endif

#ifndef DLOG_SOURCE_LOC
#define DLOG_SOURCE_LOC() \
  cuwacunu::piaabo::dlog_source_location(__FILE__, __func__, __LINE__)
#endif

/* ---- basic log macros ---- */
#ifndef log_info
#if DLOGS_USE_IOSTREAMS
#define log_info(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("INFO", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cout << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: "; \
    cuwacunu::piaabo::detail::stream_printf(std::cout, __VA_ARGS__); \
    std::cout.flush(); \
  } \
} while(false)
#else
#define log_info(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("INFO", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_FILE, "[%s0x%s%s]: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET); \
    std::fprintf(LOG_FILE, __VA_ARGS__); \
    std::fflush(LOG_FILE); \
  } \
} while(false)
#endif
#endif

#ifndef log_dbg
#if DLOGS_USE_IOSTREAMS
#define log_dbg(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("DEBUG", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cerr << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: " \
              << ANSI_COLOR_Bright_Blue << "DEBUG" << ANSI_COLOR_RESET << ": "; \
    cuwacunu::piaabo::detail::stream_printf(std::cerr, __VA_ARGS__); \
    std::cerr.flush(); \
  } \
} while(false)
#else
#define log_dbg(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("DEBUG", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_ERR_FILE, "[%s0x%s%s]: %sDEBUG%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET); \
    std::fprintf(LOG_ERR_FILE, __VA_ARGS__); \
    std::fflush(LOG_ERR_FILE); \
  } \
} while(false)
#endif
#endif

#ifndef log_err
#if DLOGS_USE_IOSTREAMS
#define log_err(...) do {                                           \
  wrap_log_sys_err();                                               \
  cuwacunu::piaabo::detail::capture_formatted_log_source("ERROR", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) {            \
    LOCK_GUARD(log_mutex);                                           \
    std::ostringstream _log_oss;                                     \
    _log_oss << "[" << ANSI_COLOR_Cyan << "0x"                        \
             << cuwacunu::piaabo::cthread_id()                        \
             << ANSI_COLOR_RESET << "]: "                             \
             << ANSI_COLOR_ERROR << "ERROR"                           \
             << ANSI_COLOR_RESET << ": ";                             \
    cuwacunu::piaabo::detail::stream_printf(_log_oss, __VA_ARGS__);   \
    std::cerr << _log_oss.str();                                      \
    std::cerr.flush();                                                \
  }                                                                   \
} while(false)
#else
#define log_err(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("ERROR", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_ERR_FILE, "[%s0x%s%s]: %sERROR%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_ERROR, ANSI_COLOR_RESET); \
    std::fprintf(LOG_ERR_FILE, __VA_ARGS__); \
    std::fflush(LOG_ERR_FILE); \
  } \
} while(false)
#endif
#endif

#ifndef log_warn
#if DLOGS_USE_IOSTREAMS
#define log_warn(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("WARNING", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cout << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: " \
              << ANSI_COLOR_WARNING << "WARNING" << ANSI_COLOR_RESET << ": "; \
    cuwacunu::piaabo::detail::stream_printf(std::cout, __VA_ARGS__); \
    std::cout.flush(); \
  } \
} while(false)
#else
#define log_warn(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("WARNING", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_WARN_FILE, "[%s0x%s%s]: %sWARNING%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_WARNING, ANSI_COLOR_RESET); \
    std::fprintf(LOG_WARN_FILE, __VA_ARGS__); \
    std::fflush(LOG_WARN_FILE); \
  } \
} while(false)
#endif
#endif

#ifndef log_fatal
#if DLOGS_USE_IOSTREAMS
#define log_fatal(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("FATAL", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cerr << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: " \
              << ANSI_COLOR_FATAL << "FATAL" << ANSI_COLOR_RESET << ": "; \
    cuwacunu::piaabo::detail::stream_printf(std::cerr, __VA_ARGS__); \
    std::cerr.flush(); \
  } \
  THROW_RUNTIME_ERROR(); \
} while(false)
#else
#define log_fatal(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("FATAL", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_ERR_FILE, "[%s0x%s%s]: %sFATAL%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_FATAL, ANSI_COLOR_RESET); \
    std::fprintf(LOG_ERR_FILE, __VA_ARGS__); \
    std::fflush(LOG_ERR_FILE); \
  } \
  THROW_RUNTIME_ERROR(); \
} while(false)
#endif
#endif

#ifndef log_terminate_gracefully
#if DLOGS_USE_IOSTREAMS
#define log_terminate_gracefully(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("TERMINATION", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cout << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: " \
              << ANSI_COLOR_WARNING << "TERMINATION" << ANSI_COLOR_RESET << ": "; \
    cuwacunu::piaabo::detail::stream_printf(std::cout, __VA_ARGS__); \
    std::cout.flush(); \
  } \
  THROW_RUNTIME_FINALIZATION(); \
} while(false)
#else
#define log_terminate_gracefully(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("TERMINATION", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_WARN_FILE, "[%s0x%s%s]: %sTERMINATION%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_WARNING, ANSI_COLOR_RESET); \
    std::fprintf(LOG_WARN_FILE, __VA_ARGS__); \
    std::fflush(LOG_WARN_FILE); \
  } \
  THROW_RUNTIME_FINALIZATION(); \
} while(false)
#endif
#endif

#ifndef log_runtime_warning
#if DLOGS_USE_IOSTREAMS
#define log_runtime_warning(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("DEV_WARNING", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::cout << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: " \
              << ANSI_COLOR_WARNING2 << "DEV_WARNING" << ANSI_COLOR_RESET << ": "; \
    cuwacunu::piaabo::detail::stream_printf(std::cout, __VA_ARGS__); \
    std::cout.flush(); \
  } \
} while(false)
#else
#define log_runtime_warning(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::capture_formatted_log_source("DEV_WARNING", DLOG_SOURCE_LOC(), __VA_ARGS__); \
  if (cuwacunu::piaabo::dlog_terminal_output_enabled()) { \
    LOCK_GUARD(log_mutex); \
    std::fprintf(LOG_WARN_FILE, "[%s0x%s%s]: %sDEV_WARNING%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_WARNING2, ANSI_COLOR_RESET); \
    std::fprintf(LOG_WARN_FILE, __VA_ARGS__); \
    std::fflush(LOG_WARN_FILE); \
  } \
} while(false)
#endif
#endif

/* ---- secure log macros ---- */
#ifndef log_secure_dbg
#if DLOGS_USE_IOSTREAMS
#define log_secure_dbg(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_stream_source(std::cerr, "DEBUG", ANSI_COLOR_Bright_Blue, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#else
#define log_secure_dbg(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_source(LOG_ERR_FILE, "DEBUG", ANSI_COLOR_Bright_Blue, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#endif
#endif

#ifndef log_secure_info
#if DLOGS_USE_IOSTREAMS
#define log_secure_info(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_stream_source(std::cout, nullptr, nullptr, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#else
#define log_secure_info(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_source(LOG_FILE, nullptr, nullptr, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#endif
#endif

#ifndef log_secure_warn
#if DLOGS_USE_IOSTREAMS
#define log_secure_warn(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_stream_source(std::cout, "WARNING", ANSI_COLOR_WARNING, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#else
#define log_secure_warn(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_source(LOG_WARN_FILE, "WARNING", ANSI_COLOR_WARNING, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#endif
#endif

#ifndef log_secure_error
#if DLOGS_USE_IOSTREAMS
#define log_secure_error(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_stream_source(std::cerr, "ERROR", ANSI_COLOR_ERROR, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#else
#define log_secure_error(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_source(LOG_ERR_FILE, "ERROR", ANSI_COLOR_ERROR, DLOG_SOURCE_LOC(), __VA_ARGS__); \
} while(false)
#endif
#endif

#ifndef log_secure_fatal
#if DLOGS_USE_IOSTREAMS
#define log_secure_fatal(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_stream_source(std::cerr, "FATAL", ANSI_COLOR_FATAL, DLOG_SOURCE_LOC(), __VA_ARGS__); \
  THROW_RUNTIME_ERROR(); \
} while(false)
#else
#define log_secure_fatal(...) do { \
  wrap_log_sys_err(); \
  cuwacunu::piaabo::detail::secure_log_source(LOG_ERR_FILE, "FATAL", ANSI_COLOR_FATAL, DLOG_SOURCE_LOC(), __VA_ARGS__); \
  THROW_RUNTIME_ERROR(); \
} while(false)
#endif
#endif

namespace cuwacunu {
namespace piaabo {

// Stream-style logger for one-shot serial dumps:
//   LOG_INFO << "value=" << x;
class dlog_serial_stream final {
 public:
#if DLOGS_USE_IOSTREAMS
  dlog_serial_stream(const char* level_label,
                     const char* level_color,
                     std::ostream* out_stream,
                     dlog_source_location_t source)
      : level_label_(level_label),
        level_color_(level_color),
        source_(source),
        out_stream_(out_stream) {}
#else
  dlog_serial_stream(const char* level_label,
                     const char* level_color,
                     FILE* out_file,
                     dlog_source_location_t source)
      : level_label_(level_label),
        level_color_(level_color),
        source_(source),
        out_file_(out_file) {}
#endif

  dlog_serial_stream(const dlog_serial_stream&) = delete;
  dlog_serial_stream& operator=(const dlog_serial_stream&) = delete;
  dlog_serial_stream& operator=(dlog_serial_stream&&) = delete;

  dlog_serial_stream(dlog_serial_stream&& other) noexcept
      : level_label_(other.level_label_),
        level_color_(other.level_color_),
        source_(other.source_),
        buffer_(std::move(other.buffer_)),
#if DLOGS_USE_IOSTREAMS
        out_stream_(other.out_stream_),
#else
        out_file_(other.out_file_),
#endif
        flushed_(other.flushed_) {
    other.flushed_ = true;
  }

  ~dlog_serial_stream() { flush(); }

  template <typename T>
  dlog_serial_stream& operator<<(const T& value) {
    buffer_ << value;
    return *this;
  }

  dlog_serial_stream& operator<<(std::ostream& (*manip)(std::ostream&)) {
    manip(buffer_);
    return *this;
  }

  dlog_serial_stream& operator<<(std::ios& (*manip)(std::ios&)) {
    manip(buffer_);
    return *this;
  }

  dlog_serial_stream& operator<<(std::ios_base& (*manip)(std::ios_base&)) {
    manip(buffer_);
    return *this;
  }

 private:
  void flush() {
    if (flushed_) return;
    flushed_ = true;
    wrap_log_sys_err();
    const std::string message = buffer_.str();
#if DLOGS_USE_IOSTREAMS
    std::ostream& out = out_stream_ ? *out_stream_ : std::cout;
    cuwacunu::piaabo::detail::secure_log_stream_source(
        out, level_label_, level_color_, source_, "%s", message.c_str());
#else
    FILE* out = out_file_ ? out_file_ : LOG_FILE;
    cuwacunu::piaabo::detail::secure_log_source(
        out, level_label_, level_color_, source_, "%s", message.c_str());
#endif
  }

  const char* level_label_{nullptr};
  const char* level_color_{nullptr};
  dlog_source_location_t source_{};
  std::ostringstream buffer_{};
#if DLOGS_USE_IOSTREAMS
  std::ostream* out_stream_{nullptr};
#else
  FILE* out_file_{nullptr};
#endif
  bool flushed_{false};
};

#if DLOGS_USE_IOSTREAMS
inline dlog_serial_stream make_log_info_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream(nullptr, nullptr, &std::cout, source);
}
inline dlog_serial_stream make_log_debug_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("DEBUG", ANSI_COLOR_Bright_Blue, &std::cerr,
                            source);
}
inline dlog_serial_stream make_log_warn_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("WARNING", ANSI_COLOR_WARNING, &std::cout, source);
}
inline dlog_serial_stream make_log_error_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("ERROR", ANSI_COLOR_ERROR, &std::cerr, source);
}
#else
inline dlog_serial_stream make_log_info_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream(nullptr, nullptr, LOG_FILE, source);
}
inline dlog_serial_stream make_log_debug_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("DEBUG", ANSI_COLOR_Bright_Blue, LOG_ERR_FILE,
                            source);
}
inline dlog_serial_stream make_log_warn_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("WARNING", ANSI_COLOR_WARNING, LOG_WARN_FILE,
                            source);
}
inline dlog_serial_stream make_log_error_stream(
    dlog_source_location_t source = {}) {
  return dlog_serial_stream("ERROR", ANSI_COLOR_ERROR, LOG_ERR_FILE, source);
}
#endif

} // namespace piaabo
} // namespace cuwacunu

#ifndef LOG_INFO
#define LOG_INFO (::cuwacunu::piaabo::make_log_info_stream(DLOG_SOURCE_LOC()))
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG (::cuwacunu::piaabo::make_log_debug_stream(DLOG_SOURCE_LOC()))
#endif
#ifndef LOG_WARN
#define LOG_WARN (::cuwacunu::piaabo::make_log_warn_stream(DLOG_SOURCE_LOC()))
#endif
#ifndef LOG_ERROR
#define LOG_ERROR (::cuwacunu::piaabo::make_log_error_stream(DLOG_SOURCE_LOC()))
#endif

#ifndef DLOG_CANONICAL_SCOPE
#define DLOG_CANONICAL_SCOPE(PATH_EXPR) \
  ::cuwacunu::piaabo::dlog_canonical_scope CONCAT(_dlog_scope_, __COUNTER__)((PATH_EXPR))
#endif

#ifndef DLOG_BUFFER_CAPTURE_SCOPE
#define DLOG_BUFFER_CAPTURE_SCOPE(ENABLED_EXPR) \
  ::cuwacunu::piaabo::dlog_buffer_capture_scope CONCAT(_dlog_buf_scope_, __COUNTER__)((ENABLED_EXPR))
#endif

/* basename helper */
#ifndef FILEBASE
#define FILEBASE() (cuwacunu::piaabo::path_basename(__FILE__))
#endif

/* common fatal helper */
#ifndef RAISE_FATAL
#define RAISE_FATAL(FMT, ...) do { \
  log_secure_fatal("(%s)[%s:%d] Error: " FMT, FILEBASE(), __func__, __LINE__, ##__VA_ARGS__); \
} while (0)
#endif

/* --- timing helpers --- */
#ifndef TICK
#define TICK(STAMP_ID) auto STAMP_ID = std::chrono::high_resolution_clock::now()
#endif

#ifndef TOCK
#define TOCK(STAMP_ID) (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())
#endif

#ifndef TOCK_ms
#define TOCK_ms(STAMP_ID) (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())
#endif

#ifndef TOCK_ns
#define TOCK_ns(STAMP_ID) (std::chrono::duration<double, std::nano>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())
#endif

#ifndef GET_READABLE_TIME
#define GET_READABLE_TIME(sec) ([&]() -> std::string { \
  long long total_s = static_cast<long long>(sec); \
  int hours = static_cast<int>(total_s / 3600); \
  total_s %= 3600; \
  int minutes = static_cast<int>(total_s / 60); \
  total_s %= 60; \
  int seconds = static_cast<int>(total_s); \
  std::ostringstream oss; \
  if (hours > 0) oss << hours << "h."; \
  if (minutes > 0 || hours > 0) oss << minutes << "m."; \
  oss << seconds << "s."; \
  return oss.str(); \
}())
#endif

#ifndef GET_READABLE_TIME_ms
#define GET_READABLE_TIME_ms(ms) ([&]() -> std::string { \
  long long total_ms = static_cast<long long>(ms); \
  int hours = static_cast<int>(total_ms / 3'600'000); \
  total_ms %= 3'600'000; \
  int minutes = static_cast<int>(total_ms / 60'000); \
  total_ms %= 60'000; \
  int seconds = static_cast<int>(total_ms / 1'000); \
  total_ms %= 1'000; \
  std::ostringstream oss; \
  if (hours > 0) oss << hours << "h."; \
  if (minutes > 0 || hours > 0) oss << minutes << "m."; \
  if (seconds > 0 || minutes > 0 || hours > 0) oss << seconds << "s."; \
  oss << total_ms << "ms."; \
  return oss.str(); \
}())
#endif

#ifndef GET_READABLE_TIME_ns
#define GET_READABLE_TIME_ns(ns) ([&]() -> std::string { \
  long long total_ns = static_cast<long long>(ns); \
  int hours = static_cast<int>(total_ns / 3'600'000'000'000LL); \
  total_ns %= 3'600'000'000'000LL; \
  int minutes = static_cast<int>(total_ns / 60'000'000'000LL); \
  total_ns %= 60'000'000'000LL; \
  int seconds = static_cast<int>(total_ns / 1'000'000'000LL); \
  total_ns %= 1'000'000'000LL; \
  int milliseconds = static_cast<int>(total_ns / 1'000'000LL); \
  total_ns %= 1'000'000LL; \
  int microseconds = static_cast<int>(total_ns / 1'000LL); \
  total_ns %= 1'000LL; \
  std::ostringstream oss; \
  if (hours > 0) oss << hours << "h."; \
  if (minutes > 0 || hours > 0) oss << minutes << "m."; \
  if (seconds > 0 || minutes > 0 || hours > 0) oss << seconds << "s."; \
  if (milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0) oss << milliseconds << "ms."; \
  if (microseconds > 0 || milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0) oss << microseconds << "µs."; \
  oss << total_ns << "ns."; \
  return oss.str(); \
}())
#endif

#ifndef PRINT_TOCK
#define PRINT_TOCK(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME(TOCK(STAMP_ID)).c_str())
#endif

#ifndef PRINT_TOCK_ms
#define PRINT_TOCK_ms(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME_ms(TOCK_ms(STAMP_ID)).c_str())
#endif

#ifndef PRINT_TOCK_ns
#define PRINT_TOCK_ns(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME_ns(TOCK_ns(STAMP_ID)).c_str())
#endif

namespace cuwacunu {
namespace piaabo {

/**
 * Utilities for printing a loading bar.
 */
struct loading_bar_t {
  std::string label;
  std::string color;
  std::string character;
  int width = 0;
  double currentProgress = 0.0;
  double lastPercentage = -1.0;
  std::chrono::time_point<std::chrono::high_resolution_clock> tick;
};

inline void printLoadingBar(const loading_bar_t &bar) {
  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) return;
  std::stringstream ss;
  int filled = (bar.width * static_cast<int>(bar.currentProgress)) / 100;

  ss << bar.label << " [" << bar.color;
  for (int i = 0; i < filled; ++i) ss << bar.character;
  for (int i = filled; i < bar.width; ++i) ss << " ";
  ss << ANSI_COLOR_RESET << "] " << std::fixed << std::setprecision(2) << bar.currentProgress << "%";

  LOCK_GUARD(log_mutex);
#if DLOGS_USE_IOSTREAMS
  std::cout << ANSI_CLEAR_LINE
            << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: "
            << ss.str() << " ";
  std::cout.flush();
#else
  std::fprintf(LOG_FILE, "%s[%s0x%s%s]: %s ",
    ANSI_CLEAR_LINE, ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, ss.str().c_str());
  std::fflush(LOG_FILE);
#endif
}

inline void startLoadingBar(loading_bar_t &bar, const std::string &label, int width) {
  bar.label = label;
  bar.width = width;
  bar.character = "█";
  bar.currentProgress = 0;
  bar.lastPercentage = -1;
  if (bar.color.empty()) bar.color = ANSI_COLOR_Dim_Green;
  bar.tick = std::chrono::high_resolution_clock::now();
  printLoadingBar(bar);
}

inline void updateLoadingBar(loading_bar_t &bar, double percentage) {
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;

  if (percentage > bar.lastPercentage) {
    bar.currentProgress = percentage;
    bar.lastPercentage = percentage;
    printLoadingBar(bar);
  }
}

inline void finishLoadingBar(loading_bar_t &bar) {
  updateLoadingBar(bar, 100);

  if (!cuwacunu::piaabo::dlog_terminal_output_enabled()) return;
  LOCK_GUARD(log_mutex);
#if DLOGS_USE_IOSTREAMS
  std::cout << "\t " << bar.color
            << "Execution time " << ANSI_COLOR_RESET
            << " [" << ANSI_COLOR_Yellow << bar.label << ANSI_COLOR_RESET << "] : "
            << GET_READABLE_TIME_ms(TOCK_ms(bar.tick)) << " \n";
  std::cout.flush();
#else
  std::fprintf(LOG_FILE, "\t %sExecution time %s [%s%s%s] : %s \n",
    bar.color.c_str(), ANSI_COLOR_RESET,
    ANSI_COLOR_Yellow, bar.label.c_str(), ANSI_COLOR_RESET,
    GET_READABLE_TIME_ms(TOCK_ms(bar.tick)).c_str());
  std::fflush(LOG_FILE);
#endif
}

inline void resetLoadingBar(loading_bar_t &bar) {
  bar.currentProgress = 0;
  printLoadingBar(bar);
}

inline void setLoadingBarColor(loading_bar_t &bar, const std::string &colorCode) {
  bar.color = colorCode;
  printLoadingBar(bar);
}

inline void setLoadingBarCharacter(loading_bar_t &bar, std::string character) {
  bar.character = std::move(character);
  printLoadingBar(bar);
}

} // namespace piaabo
} // namespace cuwacunu

/* loading bar macros */
#ifndef START_LOADING_BAR
#define START_LOADING_BAR(var_ref, width, label) \
  cuwacunu::piaabo::loading_bar_t var_ref; \
  cuwacunu::piaabo::startLoadingBar(var_ref, label, width)
#endif
#ifndef UPDATE_LOADING_BAR
#define UPDATE_LOADING_BAR(var_ref, percentage) \
  cuwacunu::piaabo::updateLoadingBar(var_ref, percentage)
#endif
#ifndef FINISH_LOADING_BAR
#define FINISH_LOADING_BAR(var_ref) \
  cuwacunu::piaabo::finishLoadingBar(var_ref)
#endif
#ifndef RESET_LOADING_BAR
#define RESET_LOADING_BAR(var_ref) \
  cuwacunu::piaabo::resetLoadingBar(var_ref)
#endif
#ifndef SET_LOADING_BAR_COLOR
#define SET_LOADING_BAR_COLOR(var_ref, colorCode) \
  cuwacunu::piaabo::setLoadingBarColor(var_ref, colorCode)
#endif
#ifndef SET_LOADING_CHARACTER
#define SET_LOADING_CHARACTER(var_ref, character) \
  cuwacunu::piaabo::setLoadingBarCharacter(var_ref, character)
#endif

/* compile-time warnings and errors (GCC/Clang). */
#ifndef COMPILE_TIME_WARNING
#define COMPILE_TIME_WARNING(msg) _Pragma(STRINGIFY(GCC warning #msg))
#endif
#ifndef THROW_COMPILE_TIME_ERROR
#define THROW_COMPILE_TIME_ERROR(msg) _Pragma(STRINGIFY(GCC error #msg))
#endif

/* runtime warning helper */
struct RuntimeWarning {
  explicit RuntimeWarning(const char *msg) { log_runtime_warning("%s", msg); }
};
#ifndef DEV_WARNING
#define DEV_WARNING(msg) static RuntimeWarning CONCAT(_rw_, __COUNTER__) (msg)
#endif

/* assertions */
#ifndef ASSERT
#define ASSERT(condition, message) do { if (!(condition)) { log_secure_fatal("%s", (message)); } } while (false)
#endif
