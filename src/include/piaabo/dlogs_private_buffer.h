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

