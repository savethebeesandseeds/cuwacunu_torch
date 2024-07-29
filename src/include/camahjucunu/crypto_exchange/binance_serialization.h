#pragma once
#include <vector>
#include <variant>
#include <sstream>
#include <optional>
#include <type_traits>
#include "piaabo/dutils.h"
#include "camahjucunu/crypto_exchange/binance_enums.h"

#define DOUBLE_SERIALIZATION_PRECISION 10

namespace cuwacunu {
namespace camahjucunu {
namespace binance {
template<typename T>
struct is_std_optional : std::false_type {};

template<typename T>
struct is_std_optional<std::optional<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_optional_v = is_std_optional<T>::value;

/* --- --- --- --- --- --- --- --- --- --- --- */
/*        strcut to string functions           */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* Specialization for bool and std::optional<bool> */
template<typename T>
typename std::enable_if<std::is_same<T, bool>::value || std::is_same<T, std::optional<bool>>::value, std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":" << (arg.second.value() ? "true," : "false,");
    }
  } else {
    oss << "\"" << arg.first << "\":" << (arg.second ? "true," : "false,");
  }
  return oss.str();
}

/* Specialization for int and std::optional<int> */
template<typename T>
typename std::enable_if<(std::is_same<T, int>::value || std::is_same<T, std::optional<int>>::value) || (std::is_same<T, long>::value || std::is_same<T, std::optional<long>>::value), std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss << std::fixed;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":" << arg.second.value() << ",";
    }
  } else {
    oss << "\"" << arg.first << "\":" << arg.second << ",";
  }
  return oss.str();
}

/* Specialization for double and std::optional<double> */
template<typename T>
typename std::enable_if<std::is_same<T, double>::value || std::is_same<T, std::optional<double>>::value, std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss.precision(DOUBLE_SERIALIZATION_PRECISION);
  oss << std::fixed;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":" << arg.second.value() << ",";
    }
  } else {
    oss << "\"" << arg.first << "\":" << arg.second << ",";
  }
  return oss.str();
}

/* Specialization for strings and std::optional<string> */
template<typename T>
typename std::enable_if<std::is_same<T, std::string>::value || std::is_same<T, std::optional<std::string>>::value, std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":\"" << arg.second.value() << "\",";
    }
  } else {
    oss << "\"" << arg.first << "\":\"" << arg.second << "\",";
  }
  return oss.str();
}

/* Specialization for any enum type and std::optional<enum> */
template<typename T>
typename std::enable_if<std::is_enum<T>::value || is_optional_enum<T>::value, std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":\"" << enum_to_string(arg.second.value()) << "\",";
    }
  } else {
    oss << "\"" << arg.first << "\":\"" << enum_to_string(arg.second) << "\",";
  }
  return oss.str();
}

/* Specialization for std::variant<std::string, std::vector<std::string>> */
template<typename T>
typename std::enable_if<std::is_same<T, std::variant<std::string, std::vector<std::string>>>::value, std::string>::type
serialize(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss << "\"" << arg.first << "\":";

  std::visit([&oss](const auto& value) {  // Correctly name and reference the value
    using ValueType = std::decay_t<decltype(value)>;  // Determine the type of the visited value
    if constexpr (std::is_same_v<ValueType, std::string>) {
      oss << "\"" << value << "\"";  // Correctly format the string
    } else if constexpr (std::is_same_v<ValueType, std::vector<std::string>>) {
      oss << "[";
      for (size_t i = 0; i < value.size(); ++i) {
        oss << "\"" << value[i] << "\"";  // Serialize each string in the vector
        if (i < value.size() - 1) oss << ", ";  // Add commas between elements
      }
      oss << "]";
    }
  }, arg.second);

  oss << ",";
  return oss.str();
}

/* Clean json for finalization */
void finalize_json(std::string& json);

/*Variadic template to handle multiple arguments */
template<typename... Args>
std::string jsonify_as_object(Args... args) {
  std::ostringstream oss;
  (oss << ... << serialize(args)); /* Fold expression over the comma operator */
  std::string ret_val = "{" + oss.str() + "}";
  finalize_json(ret_val);
  return ret_val;
}

/*Variadic template to handle multiple arguments */
template<typename... Args>
std::string jsonify_as_array(Args... args) {
  std::ostringstream oss;
  (oss << ... << serialize(args)); /* Fold expression over the comma operator */
  std::string ret_val = "[" + oss.str() + "]";
  finalize_json(ret_val);
  return ret_val;
}

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */