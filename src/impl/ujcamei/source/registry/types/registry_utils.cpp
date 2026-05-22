#include "ujcamei/source/registry/types/utils.h"

// Serialization remains string/JSON based for compatibility with exchange
// payloads. DOUBLE_SERIALIZATION_PRECISION is the current shared precision
// contract for double fields.

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace registry {
namespace types {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         serialization utils                 */
/* --- --- --- --- --- --- --- --- --- --- --- */

int64_t getUnixTimestampMillis() {
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());

  return duration.count();
}

/* Clean signature for finalization */
void finalize_signature(std::string &signature) {
  if (!signature.empty() && signature[0] == '&') {
    signature.erase(0, 1); /* Remove the first character */
  }
}

/* Clean json for finalization */
void finalize_json(std::string &json) {
  piaabo::core::string_replace(json, ",]", "]");
  piaabo::core::string_replace(json, ",}", "}");
}

} /* namespace types */
} /* namespace registry */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
