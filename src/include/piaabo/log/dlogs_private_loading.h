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
  std::cerr << ANSI_CLEAR_LINE
            << "[" << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET << "]: "
            << ANSI_COLOR_Bright_Blue << "DEBUG" << ANSI_COLOR_RESET << ": "
            << ss.str() << " ";
  std::cerr.flush();
#else
  std::fprintf(LOG_DBG_FILE, "%s[%s0x%s%s]: %sDEBUG%s: %s ",
    ANSI_CLEAR_LINE, ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET, ss.str().c_str());
  std::fflush(LOG_DBG_FILE);
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
  std::cerr << "\n\t ["
            << ANSI_COLOR_Cyan << "0x" << cuwacunu::piaabo::cthread_id() << ANSI_COLOR_RESET
            << "]: " << ANSI_COLOR_Bright_Blue << "DEBUG" << ANSI_COLOR_RESET << ": "
            << bar.color << "Execution time " << ANSI_COLOR_RESET
            << " [" << ANSI_COLOR_Yellow << bar.label << ANSI_COLOR_RESET << "] : "
            << GET_READABLE_TIME_ms(TOCK_ms(bar.tick)) << " \n";
  std::cerr.flush();
#else
  std::fprintf(LOG_DBG_FILE, "\n\t [%s0x%s%s]: %sDEBUG%s: %sExecution time %s [%s%s%s] : %s \n",
    ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET,
    ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET,
    bar.color.c_str(), ANSI_COLOR_RESET,
    ANSI_COLOR_Yellow, bar.label.c_str(), ANSI_COLOR_RESET,
    GET_READABLE_TIME_ms(TOCK_ms(bar.tick)).c_str());
  std::fflush(LOG_DBG_FILE);
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
