#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dsecurity.h"
#include "piaabo/dencryption.h"
#include "camahjucunu/exchange/exchange_enums.h"
#include "camahjucunu/exchange/exchange_types.h"

int main() {
  /* test serialization */
  {
    log_dbg("Testing [cuwacunu::camahjucunu::exchange::%sdepth_args_t%s] serialization \n", ANSI_COLOR_Yellow, ANSI_COLOR_RESET);
    cuwacunu::camahjucunu::exchange::depth_args_t  variable("value");
    variable.limit = 10;
    log_info("%s\n", variable.jsonify().c_str());
  }
  
  return 0;
}