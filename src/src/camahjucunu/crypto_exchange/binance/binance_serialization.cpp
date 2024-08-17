#include "camahjucunu/crypto_exchange/binance/binance_serialization.h"


RUNTIME_WARNING("(binance_serialization.cpp)[] parsing args and resp to and from json is somehow slow. Leaving this until further upgrade.\n");
RUNTIME_WARNING("(binance_serialization.cpp)[] validate DOUBLE_SERIALIZATION_PRECISION\n");

namespace cuwacunu {
namespace camahjucunu {
namespace binance {

/* Clean json for finalization */
void finalize_json(std::string& json) {
  piaabo::string_replace(json, ",]", "]");
  piaabo::string_replace(json, ",}", "}");
}

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */