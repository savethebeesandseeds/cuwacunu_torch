#pragma once

// General-purpose utilities (string, hashing, time conversion, etc).
// Logging/diagnostics have been split into: piaabo/dlogs.h
#include "piaabo/dlogs.h"

#include <cctype>
#include <cstdint>
#include <ctime>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef FOR_ALL
#define FOR_ALL(arr, elem) for (auto& elem : (arr))
#endif

namespace cuwacunu {
namespace piaabo {

/* util types */
template<typename T1, typename T2>
struct dPair {
  T1 first;
  T2 second;
};

/* util methods */
std::string trim_string(const std::string& str);

std::string join_strings(const std::vector<std::string>& vec, const std::string& delimiter = ", ");
std::string join_strings_ch(const std::vector<std::string>& vec, const char delimiter = ',');

std::vector<std::string> split_string(const std::string& str, char delimiter);

std::string to_hex_string(const unsigned char* data, size_t size);
std::string to_hex_string(const std::string data);

void string_replace(std::string &str, const std::string& from, const std::string& to);
void string_replace(std::string &str, const char from, const char to);

void string_remove(std::string &str, const std::string& target);
void string_remove(std::string &str, const char target);

std::string generate_random_string(const std::string& format_str);

std::string string_format(const char* format, ...);

long stringToUnixTime(const std::string& timeString, const std::string& format = "%Y-%m-%d %H:%M:%S");
std::string unixTimeToString(long unixTime, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief Computes the 64-bit FNV-1a hash for a given string.
 */
constexpr uint64_t FNV_prime = 1099511628211ULL;
constexpr uint64_t FNV_offset_basis = 14695981039346656037ULL;
constexpr uint64_t fnv1aHash(std::string_view str) {
  uint64_t hash = FNV_offset_basis;
  for (char c : str) {
    hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
    hash *= FNV_prime;
  }
  return hash;
}

} /* namespace piaabo */
} /* namespace cuwacunu */

#ifndef DEFINE_HASH
#define DEFINE_HASH(name_hash, value) constexpr uint64_t name_hash = cuwacunu::piaabo::fnv1aHash(value)
#endif

/* A convenient (printf-like) formatting helper that returns std::string. */
#ifndef FORMAT_STRING
#define FORMAT_STRING(fmt, ...) \
  ([&]() -> std::string { \
    int size = std::snprintf(nullptr, 0, fmt, __VA_ARGS__); \
    std::unique_ptr<char[]> buf(new char[size + 1]); \
    std::snprintf(buf.get(), size + 1, fmt, __VA_ARGS__); \
    return std::string(buf.get(), buf.get() + size); \
  }())
#endif

namespace cuwacunu {
namespace piaabo {

/**
 * @brief Converts a string to a specified type.
 *
 * Note: Specializations call log_fatal on errors (throws).
 */
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
  } catch (...) {
    log_fatal("(dutils)[string_cast] Error converting '%s' to double.", value.c_str());
  }
}

template <>
inline int string_cast<int>(const std::string& value) {
  try {
    return std::stoi(value);
  } catch (...) {
    log_fatal("(dutils)[string_cast] Error converting '%s' to int.", value.c_str());
  }
}

template <>
inline size_t string_cast<size_t>(const std::string& value) {
  try {
    return std::stoul(value);
  } catch (...) {
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
