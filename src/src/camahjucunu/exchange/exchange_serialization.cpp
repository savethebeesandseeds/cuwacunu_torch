#include "camahjucunu/exchange/exchange_serialization.h"


RUNTIME_WARNING("(exchange_serialization.cpp)[] parsing args and resp to and from json is somehow slow. Leaving this until further upgrade.\n");
RUNTIME_WARNING("(exchange_serialization.cpp)[] validate DOUBLE_SERIALIZATION_PRECISION\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* Clean json for finalization */
void finalize_json(std::string& json) {
  piaabo::string_replace(json, ",]", "]");
  piaabo::string_replace(json, ",}", "}");
}

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */