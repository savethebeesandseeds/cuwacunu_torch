#include "piaabo/dutils.h"
RUNTIME_WARNING("(dutils.cpp)[] #FIXME be aware to seed all random number generator seeds, seed is for reproducibility, you actually dont want to seed or seed with a random seed.\n");
RUNTIME_WARNING("(dutils.cpp)[] #FIXME Valgrind debug with libtorch suppresed warnings.\n");
RUNTIME_WARNING("(dutils.cpp)[] #FIXME revisate that all dependencies .d files are correct stated on the makefiles for each file, for instance dutils.cpp is missing as a dependency everywhere.\n");
RUNTIME_WARNING("(dutils.cpp)[] be aware of the floating point presition when printing doubles.\n");

std::mutex log_mutex;

namespace cuwacunu {
namespace piaabo {


void sanitize_string(char* input, size_t max_len) {
  char sanitized_output[2048];
  size_t i = 0;
  size_t j = 0;
  while (input[i] && j < max_len - 1) {
    /* Escape potentially dangerous characters or sequences */
    if (strchr("`$\"\\", input[i])) {
      sanitized_output[j++] = '\\'; /* Add a backslash before the character */
    }
    sanitized_output[j++] = input[i++];
  }
  sanitized_output[j] = '\0';
  strncpy(input, sanitized_output, max_len - 1);
  input[max_len - 1] = '\0';
}

std::string trim_string(const std::string& str) {
  size_t start = 0;
  size_t end = str.size();
  while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {++start;}
  while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {--end;}

  return str.substr(start, end - start);
}

// Helper function to join strings with a delimiter
std::string join_strings(const std::vector<std::string>& vec, const std::string& delimiter) {
  if (vec.empty()) return "";
      
  std::ostringstream oss;
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i != 0)
      oss << delimiter;
    oss << vec[i];
  }
  return oss.str();
}

std::string join_strings_ch(const std::vector<std::string>& vec, const char delimiter) {
  if (vec.empty()) return "";
      
  std::ostringstream oss;
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i != 0)
      oss << delimiter;
    oss << vec[i];
  }
  return oss.str();
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream token_stream(str);

  while (std::getline(token_stream, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}

std::string to_hex_string(const unsigned char* data, size_t size) {
  /* this is one of those functions that i didn't figure out my self */
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < size; i++) {
    ss << std::setw(2) << static_cast<int>(data[i]);
  }
  return ss.str();
}

std::string to_hex_string(const std::string data) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : data) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  return oss.str();
}

void string_replace(std::string &str, const std::string& from, const std::string& to) {
  if(from.empty()) {
    return; // Prevent infinite loop if `from` is empty
  }
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // Move past the replaced part
  }
}

void string_replace(std::string &str, const char from, const char to) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == from) {
      str[i] = to;
    }
  }
}

void string_remove(std::string &str, const std::string& target) {
  return string_replace(str, target, "");
}

void string_remove(std::string &str, const char target) {
  size_t pos = 0; // Position to place the next non-target character
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] != target) {
      str[pos++] = str[i]; // Move non-target character to the 'pos' index
    }
    // If str[i] == target, do nothing (effectively removing it)
  }
  str.resize(pos); // Resize string to new length after removals
}


const char* cthread_id() {
  static std::string threadID;
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  threadID = oss.str();
  return threadID.c_str();
}

std::string generate_random_string(const std::string& format_str) {
  /* Define the set of characters used to replace 'x' in the format string. */
  const std::string chars =
    "ABCDEFGHIJKLMNOPQRST"
    "0123456789";

  /* Initialize random number generator */
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, static_cast<int>(chars.size()) - 1);

  std::string result;
  result.reserve(format_str.size());

  for (char c : format_str) {
    if (c == 'x' || c == 'X') {
      result += chars[dis(gen)];
    } 
    else { 
      result += c;
    }
  }

  return result;
}

std::string string_format(const char* format, ...) {
  /* Initialize a variable argument list */
  va_list args;
  va_start(args, format);

  /* Copy the argument list to determine the required buffer size */
  va_list args_copy;
  va_copy(args_copy, args);
  int length = std::vsnprintf(nullptr, 0, format, args_copy);
  va_end(args_copy);

  if (length < 0) {
    va_end(args);
    throw std::runtime_error("Error during string formatting.");
  }

  /* Allocate a buffer of the required size */
  std::unique_ptr<char[]> buffer(new char[length + 1]);

  /* Write the formatted string into the buffer */
  std::vsnprintf(buffer.get(), length + 1, format, args);
  va_end(args);

  /* Construct and return the std::string */
  return std::string(buffer.get(), length);
}

/* Convert readable string to Unix timestamp */
long stringToUnixTime(const std::string& timeString, const std::string& format) {
  std::tm tm = {};
  std::istringstream ss(timeString);
  ss >> std::get_time(&tm, format.c_str());  /* Parse the time string */

  if (ss.fail()) {
    throw std::runtime_error("Failed to parse time string.");
  }

  return std::mktime(&tm);  /* Convert to long (Unix timestamp) */
}

/* Convert Unix timestamp to readable string */
std::string unixTimeToString(long unixTime, const std::string& format) {
  std::tm* tm = std::localtime(&unixTime);  /* Convert to tm structure */
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), format.c_str(), tm);  /* Format as string */
  return std::string(buffer);
}

} /* namespace piaabo */
} /* namespace cuwacunu */