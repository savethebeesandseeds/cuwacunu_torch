#include "piaabo/dutils.h"
#include "piaabo/config_space.h"
#include "piaabo/security.h"
#include "piaabo/encryption.h"
#include "camahjucunu/exchange/binance/binance_enums.h"
#include "camahjucunu/exchange/binance/binance_types.h"

int main() {
  /* test serialization */
  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sdepth_args_t%s] serialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    cuwacunu::camahjucunu::binance::depth_args_t  variable("value");
    variable.limit = 10;
    log_info("%s\n", variable.jsonify().c_str());
  }

  {
    log_dbg("Testing [cuwacunu::camahjucunu::binance::%sticker_24hr_args_t%s] serialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    cuwacunu::camahjucunu::binance::ticker_24hr_args_t  variable("value1");
    variable.type = cuwacunu::camahjucunu::binance::ticker_type_e::FULL;
    log_info("%s\n", variable.jsonify().c_str());
  }
  
  {
    cuwacunu::camahjucunu::binance::ticker_24hr_args_t  variable(std::vector<std::string>{"vaelu1", "value2"});
    variable.type = cuwacunu::camahjucunu::binance::ticker_type_e::FULL;
    log_info("%s\n", variable.jsonify().c_str());
  }
  
  return 0;
}