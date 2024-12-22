#pragma once
#include <stdexcept>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <cctype>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstdarg>
#include <ctime>
#include <string_view>
#include <cstdint>

#define LOG_FILE stdout
#define LOG_ERR_FILE stderr
#define LOG_WARN_FILE stdout

#define ANSI_COLOR_RESET "\x1b[0m" 
#define ANSI_CLEAR_LINE "\r\033[2K"
#define ANSI_COLOR_ERROR "\x1b[41m" 
#define ANSI_COLOR_FATAL "\x1b[41m" 
#define ANSI_COLOR_SUCCESS "\x1b[42m" 
#define ANSI_COLOR_WARNING "\x1b[43m" 
#define ANSI_COLOR_WARNING2 "\x1b[48;2;255;165;0m" 
#define ANSI_COLOR_Black "\x1b[30m" 
#define ANSI_COLOR_Red "\x1b[31m"     
#define ANSI_COLOR_Green "\x1b[32m"   
#define ANSI_COLOR_Yellow "\x1b[33m" 
#define ANSI_COLOR_Blue "\x1b[34m"  
#define ANSI_COLOR_Magenta "\x1b[35m" 
#define ANSI_COLOR_Cyan "\x1b[36m"    
#define ANSI_COLOR_White "\x1b[37m" 

#define ANSI_COLOR_Bright_Black_Grey "\x1b[90m"
#define ANSI_COLOR_Bright_Red "\x1b[91m" 
#define ANSI_COLOR_Bright_Green "\x1b[92m" 
#define ANSI_COLOR_Bright_Yellow "\x1b[93m"     
#define ANSI_COLOR_Bright_Blue "\x1b[94m" 
#define ANSI_COLOR_Bright_Magenta "\x1b[95m" 
#define ANSI_COLOR_Bright_Cyan "\x1b[96m"       
#define ANSI_COLOR_Bright_White "\x1b[97m"

#define ANSI_COLOR_Dim_Black_Grey "\x1b[2;90m"
#define ANSI_COLOR_Dim_Red "\x1b[2;91m"
#define ANSI_COLOR_Dim_Green "\x1b[2;92m"
#define ANSI_COLOR_Dim_Yellow "\x1b[2;93m"
#define ANSI_COLOR_Dim_Blue "\x1b[2;94m"
#define ANSI_COLOR_Dim_Magenta "\x1b[2;95m"
#define ANSI_COLOR_Dim_Cyan "\x1b[2;96m"
#define ANSI_COLOR_Dim_White "\x1b[2;97m"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define FOR_ALL(arr, elem) for(auto& elem : arr)

extern std::mutex log_mutex;

namespace cuwacunu {
namespace piaabo {
/* utilt types */
template<typename T1, typename T2>
struct dPair {
  T1 first;
  T2 second;
};
/* util methods */

/**
 * @brief Escapes specific characters in a string to sanitize it.
 *
 * This function processes the input C-string `input` and escapes potentially
 * dangerous characters (`\`, `"`, `$`, `` ` ``) by prefixing them with a backslash.
 * The sanitized string is written back to `input`, ensuring it does not exceed `max_len`.
 *
 * @param input Pointer to the null-terminated string to be sanitized.
 * @param max_len The maximum allowed length for the sanitized string, including the null terminator.
 *
 * @note
 * - The function uses a fixed-size buffer of 1024 characters for intermediate storage.
 * - Ensure that `max_len` does not exceed the size of `sanitized_output` to prevent buffer overflow.
 */
void sanitize_string(char* input, size_t max_len); /* Function to sanitize input against console injection */

/**
 * @brief Removes leading and trailing whitespace from a string.
 *
 * This function takes an input string and returns a new string with all
 * leading and trailing whitespace characters removed. It handles spaces,
 * tabs, newlines, and other standard whitespace characters.
 *
 * @param str The input string to be trimmed.
 * @return A new `std::string` with leading and trailing whitespace removed.
 *
 * @example
 * ```cpp
 * std::string original = "   Hello, World!   ";
 * std::string trimmed = trim_string(original);
 * // trimmed is "Hello, World!"
 * ```
 */
std::string trim_string(const std::string& str);

/**
 * @brief Joins a vector of strings together by a delimiter
 *
 * @param vec The input vector of strings to be joined.
 * @param delimiter The delimiter string to put in between.
 * @return A new `std::string` with all the elements in vec joined by delimier.
 *
 * @example
 * ```cpp
 * std::string vec = {"Hello","World!"};
 * std::string delimiter = ", ";
 * // returns "Hello, World!"
 */
std::string join_strings(const std::vector<std::string>& vec, const std::string& delimiter = ", ");
std::string join_strings_ch(const std::vector<std::string>& vec, const char delimiter = ',');

/**
 * @brief Splits a string into tokens based on a specified delimiter.
 *
 * This function takes an input string and a delimiter character, then
 * separates the string into multiple substrings wherever the delimiter
 * appears. The resulting tokens are stored in a vector of strings.
 *
 * @param str The input string to be split.
 * @param delimiter The character used to determine where to split the string.
 * @return A vector containing the split substrings.
 *
 * @example
 * ```cpp
 * std::string data = "apple,banana,cherry";
 * std::vector<std::string> fruits = split_string(data, ',');
 * // fruits contains {"apple", "banana", "cherry"}
 * ```
 */
std::vector<std::string> split_string(const std::string& str, char delimiter);

/**
 * @brief Converts a byte array to its hexadecimal string representation.
 *
 * This function takes a pointer to an array of unsigned characters (`data`) and its size,
 * then converts each byte to a two-character hexadecimal string and concatenates them.
 *
 * @param data Pointer to the byte array to be converted.
 * @param size The number of bytes in the array.
 * @return A `std::string` containing the hexadecimal representation of the input data.
 *
 * @example
 * ```cpp
 * unsigned char bytes[] = { 0xDE, 0xAD, 0xBE, 0xEF };
 * std::string hexStr = to_hex_string(bytes, 4);
 * // hexStr will be "deadbeef"
 * ```
 */
std::string to_hex_string(const unsigned char* data, size_t size);
std::string to_hex_string(const std::string data);

/**
 * @brief Replaces all occurrences of a substring with another string within a given string.
 *
 * @param str The target string where replacements will be made.
 * @param from The substring to search for and replace.
 * @param to The string to replace each occurrence of `from` with.
 *
 * @note If `from` is empty, the function exits early to prevent an infinite loop.
 */
void string_replace(std::string &str, const std::string& from, const std::string& to);
void string_replace(std::string &str, const char from, const char to);

/**
 * @brief Removes all occurrences of a substring with another string within a given string.
 *
 * @param str The target string where replacements will be made.
 * @param target The substring to search for and remove.
 *
 * @note If `from` is empty, the function exits early to prevent an infinite loop.
 */

void string_remove(std::string &str, const std::string& target);
void string_remove(std::string &str, const char target);

/**
 * @brief Retrieves the current thread's ID as a C-style string.
 *
 * This function obtains the ID of the calling thread, converts it to a string,
 * and returns a pointer to the internal C-string representation.
 * 
 * @return A `const char*` pointing to the thread ID string.
 * 
 * @note The returned pointer is valid until the function is called again.
 */
const char* cthread_id();

/**
 * @brief Generates a randomized string based on a given format.
 *
 * This function takes a format string where each occurrence of the character 'x'
 * is replaced with a randomly selected character from the predefined set
 * "ABCDEFGHIJKLMNOPQRST0123456789". All other characters in the format string
 * remain unchanged.
 *
 * @param format_str The template string containing 'x' placeholders to be replaced.
 *                   Example: "ID-xx-2024" might become "ID-A5-2024".
 *                   Example: "xxx-xxx.xxx" might become "AB0-1A2.7DE".
 *
 * @return A new string with 'x' characters replaced by random characters from the set.
 */
std::string generate_random_string(const std::string& format_str);

/**
 * @brief Formats a string using a printf-style format string and returns it as a std::string.
 *
 * This function takes a format string and a variable number of arguments, formats them
 * similarly to how `sprintf` works, and returns the result as a `std::string`. It safely
 * handles dynamic memory allocation to accommodate the formatted string of any length.
 *
 * @param format The printf-style format string.
 * @param ...    Variable arguments corresponding to the format specifiers in the format string.
 *
 * @return A `std::string` containing the formatted text.
 *
 * @throws std::runtime_error If an error occurs during formatting.
 *
 * @example
 * ```cpp
 * int value = 42;
 * double pi = 3.14159;
 * std::string formatted = string_format("Integer: %d, Double: %.2f", value, pi);
 * // formatted == "Integer: 42, Double: 3.14"
 * ```
 */
std::string string_format(const char* format, ...);

/**
 * @brief Converts a human-readable time string to a Unix timestamp (time_t).
 * 
 * This function parses a time string according to the specified format 
 * and returns the corresponding Unix timestamp. It uses std::get_time 
 * to extract the time information and std::mktime to generate the time_t value.
 * 
 * @param timeString The input time string (e.g., "2024-10-12 14:30:00").
 * @param format The format of the input string following strftime-style format 
 *               (default is "%Y-%m-%d %H:%M:%S").
 * 
 * @return A Unix timestamp (time_t) representing the input time string.
 * 
 * @throws std::runtime_error if the time string cannot be parsed.
 */
long stringToUnixTime(const std::string& timeString, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief Converts a Unix timestamp (time_t) to a human-readable time string.
 * 
 * This function converts a given Unix timestamp into a readable string 
 * following the provided format. It uses std::localtime to generate a 
 * tm structure and std::strftime to format the time as a string.
 * 
 * @param unixTime The input Unix timestamp to be converted in seconds.
 * @param format The desired output format using strftime-style format 
 *               (default is "%Y-%m-%d %H:%M:%S").
 * 
 * @return A string representing the formatted time.
 */
std::string unixTimeToString(long unixTime, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief Computes the FNV-1a hash for a given string.
 * 
 * This function implements the 64-bit FNV-1a hashing algorithm, which is a fast, 
 * non-cryptographic hash function with good distribution properties. It is commonly 
 * used for hash tables and simple checksum tasks.
 * 
 * @param str The input string as a std::string_view.
 * @return uint64_t The computed 64-bit hash value.
 * 
 * @note This function is constexpr, meaning the hash can be computed at compile-time 
 *       for constant input strings.
 * 
 * @example
 * constexpr uint64_t hashValue = fnv1aHash("Hello, World!");
 */
constexpr uint64_t FNV_prime = 1099511628211u;
constexpr uint64_t FNV_offset_basis = 14695981039346656037u;
constexpr uint64_t fnv1aHash(std::string_view str) {
  uint64_t hash = FNV_offset_basis;
  for (char c : str) {
    hash ^= static_cast<uint64_t>(c);  /* XOR the byte */
    hash *= FNV_prime;  /* Multiply by the FNV prime */
  }
  return hash;
}

} /* namespace piaabo */
} /* namespace cuwacunu */

#define DEFINE_HASH(name_hash, value) constexpr uint64_t name_hash = cuwacunu::piaabo::fnv1aHash(value);

#define LOCK_GUARD(v_mutex) std::lock_guard<std::mutex> lock(v_mutex)

#define CLEAR_SYS_ERR() { errno = 0; }

#define FORMAT_STRING(fmt, ...) \
  ([&]() -> std::string { \
    int size = std::snprintf(nullptr, 0, fmt, __VA_ARGS__); \
    std::unique_ptr<char[]> buf(new char[size + 1]); \
    std::snprintf(buf.get(), size + 1, fmt, __VA_ARGS__); \
    return std::string(buf.get(), buf.get() + size); \
  }())

/* This log functionality checks if there is a pendding log for the error trasported by the errno.h lib, 
 * WARNING! This functionaly sets the errno=0 if the errno is futher required, this might be not a desired behaviour.
 */
#define wrap_log_sys_err() {\
  if(errno != 0) {\
    LOCK_GUARD(log_mutex);\
    fprintf(LOG_ERR_FILE,"[%s0x%s%s]: %sSYS ERRNO%s: ",\
      ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
      ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
    fprintf(LOG_ERR_FILE,"[%d] %s\n", errno, strerror(errno));\
    fflush(LOG_ERR_FILE);\
    errno = 0;\
  }\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_info(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  fprintf(LOG_FILE,"[%s0x%s%s]: ",\
    ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET);\
  fprintf(LOG_FILE,__VA_ARGS__);\
  fflush(LOG_FILE);\
} while(false)
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_dbg(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%s%s]: %sDEBUG%s: ",\
    ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
    ANSI_COLOR_Bright_Blue,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
} while(false)
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_err(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%s%s]: %sERROR%s: ",\
    ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
    ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
} while(false)
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_fatal(...) do {\
  wrap_log_sys_err();\
  { \
    LOCK_GUARD(log_mutex);\
    fprintf(LOG_ERR_FILE,"[%s0x%s%s]: %sFATAL%s: ",\
      ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
      ANSI_COLOR_FATAL,ANSI_COLOR_RESET);\
    fprintf(LOG_ERR_FILE,__VA_ARGS__);\
    fflush(LOG_ERR_FILE);\
  } \
  THROW_RUNTIME_ERROR();\
} while(false)
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_terminate_gracefully(...) do {\
  wrap_log_sys_err();\
  { \
    LOCK_GUARD(log_mutex);\
    fprintf(LOG_WARN_FILE,"[%s0x%s%s]: %sTERMINATION%s: ",\
      ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
      ANSI_COLOR_WARNING,ANSI_COLOR_RESET);\
    fprintf(LOG_WARN_FILE,__VA_ARGS__);\
    fflush(LOG_WARN_FILE);\
  } \
  THROW_RUNTIME_FINALIZATION();\
} while(false)
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_warn(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  fprintf(LOG_WARN_FILE,"[%s0x%s%s]: %sWARNING%s: ",\
    ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
    ANSI_COLOR_WARNING,ANSI_COLOR_RESET);\
  fprintf(LOG_WARN_FILE,__VA_ARGS__);\
  fflush(LOG_WARN_FILE);\
} while(false)

#define log_runtime_warning(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  fprintf(LOG_WARN_FILE,"[%s0x%s%s]: %sDEV_WARNING%s: ",\
    ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET,\
    ANSI_COLOR_WARNING2,ANSI_COLOR_RESET);\
  fprintf(LOG_WARN_FILE,__VA_ARGS__);\
  fflush(LOG_WARN_FILE);\
} while(false)

/* Secure logging macro */
#define log_secure_dbg(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  char temp[2048];\
  snprintf(temp, sizeof(temp), "[%s0x%s%s]: %sDEBUG%s: ", \
    ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
    ANSI_COLOR_Bright_Blue, ANSI_COLOR_RESET);\
  char formatted_message[2048];\
  snprintf(formatted_message, sizeof(formatted_message), __VA_ARGS__);\
  size_t temp_len = strlen(temp);\
  snprintf(temp + temp_len, sizeof(temp) - temp_len, "%s", formatted_message);\
  cuwacunu::piaabo::sanitize_string(temp, sizeof(temp));\
  fprintf(LOG_ERR_FILE, "%s%s%s", temp, strlen(temp)>=(2048 - 1) ? "...[message truncated]" : "", temp[strlen(temp)-1] != '\n' ? "\n" : "");\
  fflush(LOG_ERR_FILE);\
} while(false)

/* Secure logging macro */
#define log_secure_info(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  char temp[2048];\
  snprintf(temp, sizeof(temp), "[%s0x%s%s]: ", \
    ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET);\
  char formatted_message[2048];\
  snprintf(formatted_message, sizeof(formatted_message), __VA_ARGS__);\
  size_t temp_len = strlen(temp);\
  snprintf(temp + temp_len, sizeof(temp) - temp_len, "%s", formatted_message);\
  cuwacunu::piaabo::sanitize_string(temp, sizeof(temp));\
  fprintf(LOG_ERR_FILE, "%s%s%s", temp, strlen(temp)>=(2048 - 1) ? "...[message truncated]" : "", temp[strlen(temp)-1] != '\n' ? "\n" : "");\
  fflush(LOG_ERR_FILE);\
} while(false)

/* Secure logging macro */
#define log_secure_warn(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  char temp[2048];\
  snprintf(temp, sizeof(temp), "[%s0x%s%s]: %sWARNING%s: ", \
    ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
    ANSI_COLOR_WARNING, ANSI_COLOR_RESET);\
  char formatted_message[2048];\
  snprintf(formatted_message, sizeof(formatted_message), __VA_ARGS__);\
  size_t temp_len = strlen(temp);\
  snprintf(temp + temp_len, sizeof(temp) - temp_len, "%s", formatted_message);\
  cuwacunu::piaabo::sanitize_string(temp, sizeof(temp));\
  fprintf(LOG_ERR_FILE, "%s%s%s", temp, strlen(temp)>=(2048 - 1) ? "...[message truncated]" : "", temp[strlen(temp)-1] != '\n' ? "\n" : "");\
  fflush(LOG_ERR_FILE);\
} while(false)

/* Secure logging macro */
#define log_secure_error(...) do {\
  wrap_log_sys_err();\
  LOCK_GUARD(log_mutex);\
  char temp[2048];\
  snprintf(temp, sizeof(temp), "[%s0x%s%s]: %sERROR%s: ", \
    ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
    ANSI_COLOR_ERROR, ANSI_COLOR_RESET);\
  char formatted_message[2048];\
  snprintf(formatted_message, sizeof(formatted_message), __VA_ARGS__);\
  size_t temp_len = strlen(temp);\
  snprintf(temp + temp_len, sizeof(temp) - temp_len, "%s", formatted_message);\
  cuwacunu::piaabo::sanitize_string(temp, sizeof(temp));\
  fprintf(LOG_ERR_FILE, "%s%s%s", temp, strlen(temp)>=(2048 - 1) ? "...[message truncated]" : "", temp[strlen(temp)-1] != '\n' ? "\n" : "");\
  fflush(LOG_ERR_FILE);\
} while(false)

/* Secure logging macro */
#define log_secure_fatal(...) do {\
  wrap_log_sys_err();\
  { \
    LOCK_GUARD(log_mutex);\
    char temp[2048];\
    snprintf(temp, sizeof(temp), "[%s0x%s%s]: %sFATAL%s: ", \
      ANSI_COLOR_Cyan, cuwacunu::piaabo::cthread_id(), ANSI_COLOR_RESET, \
      ANSI_COLOR_FATAL, ANSI_COLOR_RESET);\
    char formatted_message[2048];\
    snprintf(formatted_message, sizeof(formatted_message), __VA_ARGS__);\
    size_t temp_len = strlen(temp);\
    snprintf(temp + temp_len, sizeof(temp) - temp_len, "%s", formatted_message);\
    cuwacunu::piaabo::sanitize_string(temp, sizeof(temp));\
    fprintf(LOG_ERR_FILE, "%s%s%s", temp, strlen(temp)>=(2048 - 1) ? "...[message truncated]" : "", temp[strlen(temp)-1] != '\n' ? "\n" : "");\
    fflush(LOG_ERR_FILE);\
  } \
  THROW_RUNTIME_ERROR();\
} while(false)

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
/* Macro to capture the current high-resolution time point */
#define TICK(STAMP_ID) \
  auto STAMP_ID = std::chrono::high_resolution_clock::now()

/* Macro to calculate the elapsed time in seconds since the given time point */
#define TOCK(STAMP_ID) \
  (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())

/* Macro to calculate the elapsed time in milliseconds since the given time point */
#define TOCK_ms(STAMP_ID) \
  (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())

/* Macro to calculate the elapsed time in nanoseconds since the given time point */
#define TOCK_ns(STAMP_ID) \
  (std::chrono::duration<double, std::nano>(std::chrono::high_resolution_clock::now() - (STAMP_ID)).count())

#define GET_READABLE_TIME(ms) \
  ([&]() -> std::string { \
    long long total_s = static_cast<long long>(ms); \
    int hours = total_s / 3600; \
    total_s %= 3600; \
    int minutes = total_s / 60; \
    total_s %= 60; \
    int seconds = total_s / 1; \
    total_s %= 1; \
    std::ostringstream oss; \
    if (hours > 0) \
      oss << hours << "h."; \
    if (minutes > 0 || hours > 0) \
      oss << minutes << "m."; \
    oss << seconds << "s."; \
    return oss.str(); \
  })()

#define GET_READABLE_TIME_ms(ms) \
  ([&]() -> std::string { \
    long long total_ms = static_cast<long long>(ms); \
    int hours = total_ms / 3'600'000; \
    total_ms %= 3'600'000; \
    int minutes = total_ms / 60'000; \
    total_ms %= 60'000; \
    int seconds = total_ms / 1'000; \
    total_ms %= 1'000; \
    std::ostringstream oss; \
    if (hours > 0) \
      oss << hours << "h."; \
    if (minutes > 0 || hours > 0) \
      oss << minutes << "m."; \
    if (seconds > 0 || minutes > 0 || hours > 0) \
      oss << seconds << "s."; \
    oss << total_ms << "ms."; \
    return oss.str(); \
  })()

#define GET_READABLE_TIME_ns(ns) \
  ([&]() -> std::string { \
    long long total_ns = static_cast<long long>(ns); \
    int hours = total_ns / 3'600'000'000'000; \
    total_ns %= 3'600'000'000'000; \
    int minutes = total_ns / 60'000'000'000; \
    total_ns %= 60'000'000'000; \
    int seconds = total_ns / 1'000'000'000; \
    total_ns %= 1'000'000'000; \
    int milliseconds = total_ns / 1'000'000; \
    total_ns %= 1'000'000; \
    int microseconds = total_ns / 1'000; \
    total_ns %= 1'000; \
    std::ostringstream oss; \
    if (hours > 0) \
      oss << hours << "h."; \
    if (minutes > 0 || hours > 0) \
      oss << minutes << "m."; \
    if (seconds > 0 || minutes > 0 || hours > 0) \
      oss << seconds << "s."; \
    if (milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0) \
      oss << milliseconds << "ms."; \
    if (microseconds > 0 || milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0) \
      oss << microseconds << "µs."; \
    oss << total_ns << "ns."; \
    return oss.str(); \
  })()

#define PRINT_TOCK(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME(TOCK(STAMP_ID)).c_str());
  
#define PRINT_TOCK_ms(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME_ms(TOCK_ms(STAMP_ID)).c_str());
  
#define PRINT_TOCK_ns(STAMP_ID) \
  log_info("%s \t Execution time %s [%s%s%s] : %s \n", \
    ANSI_COLOR_Green, ANSI_COLOR_RESET, \
    ANSI_COLOR_Yellow, #STAMP_ID, ANSI_COLOR_RESET, \
    GET_READABLE_TIME_ns(TOCK_ns(STAMP_ID)).c_str());

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
namespace cuwacunu {
namespace piaabo {
  /**
 * Utilities for printing a loading bar
 */
struct loading_bar_t {
  std::string label;
  std::string color;
  std::string character;
  int width;
  double currentProgress;
  double lastPercentage;
  std::chrono::time_point<std::chrono::high_resolution_clock> tick;
};
inline void printLoadingBar(const loading_bar_t &bar) {
  std::stringstream ss;
  int filled = (bar.width * static_cast<int>(bar.currentProgress)) / 100;
  ss << bar.label << " [" << bar.color;
  for (int i = 0; i < filled; ++i) {
    ss << bar.character;
  }
  for (int i = filled; i < bar.width; ++i) {
    ss << " ";
  }
  ss << ANSI_COLOR_RESET << "] " << std::fixed << std::setprecision(2) << bar.currentProgress << "%";
  /* log the loading bar instant */
  {
    LOCK_GUARD(log_mutex);\
    fprintf(LOG_FILE,"%s[%s0x%s%s]: %s ",
      ANSI_CLEAR_LINE,ANSI_COLOR_Cyan,cuwacunu::piaabo::cthread_id(),ANSI_COLOR_RESET, ss.str().c_str());
    fflush(LOG_FILE);
  }
}
inline void startLoadingBar(loading_bar_t &bar, const std::string &label, int width) {
  bar.label = label;
  bar.width = width;
  bar.character = "█";
  bar.currentProgress = 0;
  bar.lastPercentage = -1;
  if (bar.color.empty()) {
    bar.color = ANSI_COLOR_Dim_Green;
  }
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
  fprintf(LOG_FILE, "\t %sExecution time %s [%s%s%s] : %s \n",
    bar.color.c_str(), ANSI_COLOR_RESET,
    ANSI_COLOR_Yellow, bar.label.c_str(), ANSI_COLOR_RESET,
    GET_READABLE_TIME_ms(TOCK_ms(bar.tick)).c_str());
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
  bar.character = character;
  printLoadingBar(bar);
}
} /* namespace piaabo */
} /* namespace cuwacunu */

/* loading bar macros */
#define START_LOADING_BAR(var_ref, width, label) \
  cuwacunu::piaabo::loading_bar_t var_ref; \
  cuwacunu::piaabo::startLoadingBar(var_ref, label, width);
#define UPDATE_LOADING_BAR(var_ref, percentage) \
  cuwacunu::piaabo::updateLoadingBar(var_ref, percentage);
#define FINISH_LOADING_BAR(var_ref) \
  cuwacunu::piaabo::finishLoadingBar(var_ref);
#define RESET_LOADING_BAR(var_ref) \
  cuwacunu::piaabo::resetLoadingBar(var_ref);
#define SET_LOADING_BAR_COLOR(var_ref, colorCode) \
  cuwacunu::piaabo::setLoadingBarColor(var_ref, colorCode);
#define SET_LOADING_CHARACTER(var_ref, character) \
  cuwacunu::piaabo::setLoadingBarCharacter(var_ref, character);
/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
/* utilities */
#define STRINGIFY(x) #x
#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)
/* compile-time warnings and errors */
#define COMPILE_TIME_WARNING(msg) _Pragma(STRINGIFY(GCC warning #msg))
#define THROW_COMPILE_TIME_ERROR(msg) _Pragma(STRINGIFY(GCC error #msg))
/* run-time warnings and errors */
struct RuntimeWarning { RuntimeWarning(const char *msg) { log_runtime_warning(msg); }};
#define RUNTIME_WARNING(msg) static RuntimeWarning CONCAT(rw_, __COUNTER__) (msg)
#define THROW_RUNTIME_ERROR() { throw std::runtime_error("Runtime error occurred"); }
#define THROW_RUNTIME_FINALIZATION() { std::exit(0); }
/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
#define ASSERT(condition, message) do {if (!(condition)) {log_secure_fatal(message);} } while (false)

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
namespace cuwacunu {
namespace piaabo {

/**
 * @brief Converts a string to a specified type.
 * 
 * @tparam T The target type to convert to.
 * @param value The string value to be converted.
 * @return The converted value of type `T`.
 * @throws std::invalid_argument if the conversion fails or if the input is invalid.
 */
template <typename T>
T string_cast(const std::string& value);
template <typename T>
T string_cast(const std::string& value) {
  static_assert(!std::is_same<T, T>::value, "Specialization not implemented for string_cast");
}

template <>
inline std::string string_cast<std::string>(const std::string& value) {
  return value;
}

template <>
inline double string_cast<double>(const std::string& value) {
  try {
    return std::stod(value);
  } catch (const std::exception& e) {
    log_fatal("(dutils)[string_cast] Error converting '%s' to double.", value.c_str());
  }
}

template <>
inline int string_cast<int>(const std::string& value) {
  try {
    return std::stoi(value);
  } catch (const std::exception& e) {
    log_fatal("(dutils)[string_cast] Error converting '%s' to int.", value.c_str());
  }
}

template <>
inline size_t string_cast<size_t>(const std::string& value) {
  try {
    return std::stoul(value);
  } catch (const std::exception& e) {
    log_fatal("(dutils)[string_cast] Error converting '%s' to size_t.", value.c_str());
  }
}

template <>
inline bool string_cast<bool>(const std::string& value) {
  if (value == "true") return true;
  if (value == "false") return false;
  log_fatal("(dutils)[string_cast] Invalid boolean value: '%s'", value.c_str());
}

template <>
inline std::vector<double> string_cast<std::vector<double>>(const std::string& value) {
  std::vector<double> result;
  std::istringstream stream(value);
  std::string token;
  while (std::getline(stream, token, ',')) { result.push_back(string_cast<double>(token)); }
  return result;
}

template <>
inline std::vector<int> string_cast<std::vector<int>>(const std::string& value) {
  std::vector<int> result;
  std::istringstream stream(value);
  std::string token;
  while (std::getline(stream, token, ',')) { result.push_back(string_cast<int>(token)); }
  return result;
}

} /* namespace piaabo */
} /* namespace cuwacunu */
