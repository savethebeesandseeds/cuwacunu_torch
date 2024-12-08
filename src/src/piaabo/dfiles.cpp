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



} /* namespace dfiles */
} /* namespace piaabo */
} /* namespace cuwacunu */