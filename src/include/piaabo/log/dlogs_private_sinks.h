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
