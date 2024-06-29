#include "piaabo/dutils.h"
RUNTIME_WARNING("(dutils.cpp)[] #FIXME be aware to seed all random number generator seeds.\n");
RUNTIME_WARNING("(config.cpp)[] #FIXME Valgrind debug with libtorch suppresed warnings.\n");

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace cuwacunu {
namespace piaabo {
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
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < size; i++) {
    ss << std::setw(2) << static_cast<int>(data[i]);
  }
  return ss.str();
}
} /* namespace cuwacunu */
} /* namespace piaabo */