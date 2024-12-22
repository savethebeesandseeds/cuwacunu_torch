/* dfiles.cpp */
#include "piaabo/dfiles.h"

RUNTIME_WARNING("(dfiles.cpp)[] binaryFile_to_vector has a note on imporving performance \n");

namespace cuwacunu {
namespace piaabo {
namespace dfiles {

std::string readFileToString(const std::string& filePath) {
	std::ifstream resultFile(filePath);
	if (!resultFile) { log_fatal("[readFileToString] Error: Unable to open file: %s...\n", filePath.c_str()); }

	std::ostringstream buffer;
	buffer << resultFile.rdbuf();
	return buffer.str();
}

std::ifstream readFileToStream(const std::string& filePath) {
	std::ifstream resultFile(filePath);
	if (!resultFile) { log_fatal("[readFileToStream] Error: Unable to open file: %s...\n", filePath.c_str()); }

	return resultFile;
}

size_t countLinesInFile(const std::string& file_path) {
  std::ifstream file = readFileToStream(file_path);
  size_t line_count = 0;
  std::string line;
  while (std::getline(file, line)) { ++line_count; }
  file.close();
  return line_count;
}

} /* namespace dfiles */
} /* namespace piaabo */
} /* namespace cuwacunu */