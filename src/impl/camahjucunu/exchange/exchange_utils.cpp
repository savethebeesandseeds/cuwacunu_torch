#include "camahjucunu/exchange/exchange_utils.h"

RUNTIME_WARNING("(exchange_utils.cpp)[serialization] parsing args and resp to and from json is somehow slow. Leaving this until further upgrade.\n");
RUNTIME_WARNING("(exchange_utils.cpp)[serialization] validate DOUBLE_SERIALIZATION_PRECISION\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         serialization utils                 */
/* --- --- --- --- --- --- --- --- --- --- --- */

int64_t getUnixTimestampMillis() {
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

  return duration.count();
}

/* Clean signature for finalization */
void finalize_signature(std::string& signature) {
  if (!signature.empty() && signature[0] == '&') {
    signature.erase(0, 1); /* Remove the first character */
  }
}

/* Clean json for finalization */
void finalize_json(std::string& json) {
  piaabo::string_replace(json, ",]", "]");
  piaabo::string_replace(json, ",}", "}");
}

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
