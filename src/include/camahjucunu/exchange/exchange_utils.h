/* exchange_utils.h */
#pragma once
#include <tuple>
#include <vector>
#include <variant>
#include <optional>
#include <sstream>
#include <fstream>
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/dsecurity.h"
#include "piaabo/djson_parsing.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#define DOUBLE_SERIALIZATION_PRECISION 8
#define QUOTE_DOUBLES /* decimal numbers are required by exchange to be serialized as strings */
#undef QUOTE_DOUBLES_SIGNATURE /* decimal numbers are required by exchange to be signed as strings */

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         exchange global structs             */
/* --- --- --- --- --- --- --- --- --- --- --- */

struct frame_response_t { uint http_status; std::string frame_id; };
ENFORCE_ARCHITECTURE_DESIGN(frame_response_t);

struct loaded_data_response_t { std::map<interval_type_e, std::vector<double>> data; };
ENFORCE_ARCHITECTURE_DESIGN(loaded_data_response_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         serialization utils                 */
/* --- --- --- --- --- --- --- --- --- --- --- */

int64_t getUnixTimestampMillis();

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
#ifdef QUOTE_DOUBLES
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":" << "\"" << arg.second.value() << "\"" << ",";
    }
  } else {
    oss << "\"" << arg.first << "\":" << "\"" << arg.second << "\"" << ",";
  }
#else
  /* disable quoting doubles in serialization */
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "\"" << arg.first << "\":" << arg.second.value() << ",";
    }
  } else {
    oss << "\"" << arg.first << "\":" << arg.second << ",";
  }
#endif
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


/* --- --- --- --- --- --- --- --- --- --- --- */
/*        strcut fields signature functions    */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* Specialization for bool and std::optional<bool> */
template<typename T>
typename std::enable_if<std::is_same<T, bool>::value || std::is_same<T, std::optional<bool>>::value, std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << (arg.second.value() ? "true" : "false");
    }
  } else {
    oss << "&" << arg.first << "=" << (arg.second ? "true" : "false");
  }
  return oss.str();
}

/* Specialization for int and std::optional<int> */
template<typename T>
typename std::enable_if<(std::is_same<T, int>::value || std::is_same<T, std::optional<int>>::value) || (std::is_same<T, long>::value || std::is_same<T, std::optional<long>>::value), std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss << std::fixed;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << arg.second.value();
    }
  } else {
    oss << "&" << arg.first << "=" << arg.second;
  }
  return oss.str();
}

/* Specialization for double and std::optional<double> */
template<typename T>
typename std::enable_if<std::is_same<T, double>::value || std::is_same<T, std::optional<double>>::value, std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss.precision(DOUBLE_SERIALIZATION_PRECISION);
  oss << std::fixed;
#ifdef QUOTE_DOUBLES_SIGNATURE
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << arg.second.value();
    }
  } else {
    oss << "&" << arg.first << "=" << arg.second;
  }
#else 
  /* disable quoting doubles in serialization */
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << arg.second.value();
    }
  } else {
    oss << "&" << arg.first << "=" << arg.second;
  }
#endif
  return oss.str();
}

/* Specialization for strings and std::optional<string> */
template<typename T>
typename std::enable_if<std::is_same<T, std::string>::value || std::is_same<T, std::optional<std::string>>::value, std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << arg.second.value();
    }
  } else {
    oss << "&" << arg.first << "=" << arg.second;
  }
  return oss.str();
}

/* Specialization for any enum type and std::optional<enum> */
template<typename T>
typename std::enable_if<std::is_enum<T>::value || is_optional_enum<T>::value, std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  if constexpr (is_std_optional_v<T>) {
    if (arg.second.has_value()) {
      oss << "&" << arg.first << "=" << enum_to_string(arg.second.value());
    }
  } else {
    oss << "&" << arg.first << "=" << enum_to_string(arg.second);
  }
  return oss.str();
}

/* Specialization for std::variant<std::string, std::vector<std::string>> */
template<typename T>
typename std::enable_if<std::is_same<T, std::variant<std::string, std::vector<std::string>>>::value, std::string>::type
field_signature(const cuwacunu::piaabo::dPair<const std::string, T>& arg) {
  std::ostringstream oss;
  oss << "&" << arg.first << "=";

  std::visit([&oss](const auto& value) {  // Correctly name and reference the value
    using ValueType = std::decay_t<decltype(value)>;  // Determine the type of the visited value
    if constexpr (std::is_same_v<ValueType, std::string>) {
      oss << value;  // Correctly format the string
    } else if constexpr (std::is_same_v<ValueType, std::vector<std::string>>) {
      oss << "[";
      for (size_t i = 0; i < value.size(); ++i) {
        oss << value[i];  // Serialize each string in the vector
        if (i < value.size() - 1) oss << ",";  // Add commas between elements
      }
      oss << "]";
    }
  }, arg.second);

  return oss.str();
}


/* --- --- --- --- --- --- --- --- --- --- --- */
/*        auxiliary functions                  */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* Clean json for finalization */
void finalize_signature(std::string& signature);

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

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

#define pairWrap(variable) cuwacunu::piaabo::dPair<const std::string, decltype(variable)>{#variable, variable}
#define pairWrap_variant(variable) cuwacunu::piaabo::dPair<const std::string, decltype(variable)>{std::holds_alternative<std::vector<std::string>>(variable) ? #variable"s" : "symbol" , variable}

/* Define macros for accessing these types within a std::variant */
#define GET_OBJECT(obj, obj_type) (std::get<obj_type>(obj))
#define GET_VECT_OBJECT(obj, obj_type) (std::get<std::vector<obj_type>>(obj))

#define GET_TICK_FULL(obj) (std::get<cuwacunu::camahjucunu::exchange::tick_full_t>(obj.ticks))
#define GET_TICK_MINI(obj) (std::get<cuwacunu::camahjucunu::exchange::tick_mini_t>(obj.ticks))

#define GET_VECT_TICK_FULL(obj) (std::get<std::vector<cuwacunu::camahjucunu::exchange::tick_full_t>>(obj.ticks))
#define GET_VECT_TICK_MINI(obj) (std::get<std::vector<cuwacunu::camahjucunu::exchange::tick_mini_t>>(obj.ticks))

/* ------------------- General ------------------- */
#define INITIAL_PARSE(json_input, root_var, obj_var) \
    cuwacunu::piaabo::JsonValue root_var = \
        cuwacunu::piaabo::JsonParser(json_input).parse(); \
    cuwacunu::piaabo::ObjectType& obj_var = *(root_var.objectValue)

#define RETRIVE_OBJECT(obj_var, key, result_obj) \
  cuwacunu::piaabo::ObjectType& result_obj = *(obj_var[key].objectValue)

#define RETRIVE_ARRAY(obj_var, key, result_arr) \
  cuwacunu::piaabo::ArrayType& result_arr = *(obj_var[key].arrayValue)

#define RETRIVE_OBJECT_FROM_OBJECT(obj_var, key, result_obj) \
  RETRIVE_OBJECT((*obj_var.objectValue), key, result_obj)

/* ------------------- Value - Keys ------------------- */

#define ASSIGN_STRING_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member) \
  obj.member = root_obj[json_key].stringValue;

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member) \
  do { \
    try { \
      obj.member = std::stod(root_obj[json_key].stringValue.c_str()); \
    } catch (...) { } \
  } while (false)

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(obj, root_obj, json_key, member, cast_type) \
  obj.member = static_cast<cast_type>(root_obj[json_key].numberValue);

#define ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(obj, root_obj, json_key, member) \
  obj.member = root_obj[json_key].boolValue;

#define ASSIGN_ENUM_FIELD_FROM_JSON_STRING(obj, root_obj, json_key, member, T) \
  do { \
    try { \
      obj.member = string_to_enum<T>(root_obj[json_key].stringValue); \
    } catch (...) {} \
  } while (false)

/* ------------------- Values From Object ------------------- */

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_OBJECT(obj, root_obj, json_key, member, cast_type) \
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(obj, (*root_obj.objectValue), json_key, member, cast_type)

#define ASSIGN_STRING_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_BOOL_FIELD_FROM_JSON_BOOL_IN_OBJECT(obj, root_obj, json_key, member) \
  ASSIGN_BOOL_FIELD_FROM_JSON_BOOL(obj, (*root_obj.objectValue), json_key, member)

#define ASSIGN_ENUM_FIELD_FROM_JSON_STRING_IN_OBJECT(obj, root_obj, json_key, member, T) \
  ASSIGN_ENUM_FIELD_FROM_JSON_STRING(obj, (*root_obj.objectValue), json_key, member, T)

/* ------------------- Values From Array ------------------- */

#define ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER_IN_ARRAY(obj, root_arr, index, member, cast_type) \
  obj.member = static_cast<cast_type>((*root_arr.arrayValue)[index].numberValue);

#define ASSIGN_DOUBLE_FIELD_FROM_JSON_STRING_IN_ARRAY(obj, root_arr, index, member) \
  do { \
    try { \
      obj.member = std::stod((*root_arr.arrayValue)[index].stringValue.c_str()); \
    } catch (...) {} \
  } while (false)

#define ALLOCATE_VECT(obj, size) \
  obj.reserve(size); \
  obj.clear();

