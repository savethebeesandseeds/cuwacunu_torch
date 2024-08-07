#include "piaabo/dutils.h"
RUNTIME_WARNING("(dutils.cpp)[] #FIXME be aware to seed all random number generator seeds.\n");
RUNTIME_WARNING("(dutils.cpp)[] #FIXME Valgrind debug with libtorch suppresed warnings.\n");
RUNTIME_WARNING("(dutils.cpp)[] #FIXME revisate that all dependencies .d files are correct stated on the makefiles for each file, for instance dutils.cpp is missing as a dependency everywhere.\n");
RUNTIME_WARNING("(dutils.cpp)[] be aware of the floating point presition when printing doubles.\n");

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace cuwacunu {
namespace piaabo {
void sanitize_string(char* input, size_t max_len) {
  char sanitized_output[1024];
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

} /* namespace piaabo */
} /* namespace cuwacunu */