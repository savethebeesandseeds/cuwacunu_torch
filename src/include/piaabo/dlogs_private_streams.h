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
  return dlog_serial_stream("DEBUG", ANSI_COLOR_Bright_Blue, LOG_DBG_FILE,
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
