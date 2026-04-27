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
    std::fprintf(LOG_DBG_FILE, "[%s0x%s%s]: %sDEBUG%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET); \
    std::fprintf(LOG_DBG_FILE, __VA_ARGS__); \
    std::fflush(LOG_DBG_FILE); \
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
  cuwacunu::piaabo::detail::secure_log_source(LOG_DBG_FILE, "DEBUG", ANSI_COLOR_Bright_Blue, DLOG_SOURCE_LOC(), __VA_ARGS__); \
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
