#pragma once
#include "camahjucunu/exchange/abstract/abstract_mech_data.h"
#include "camahjucunu/exchange/binance/binance_utils.h"

RUNTIME_WARNING("(binance_mech_data.h)[] be aware, volumetric data provided by binance is only volumen inside binance, we should want to incorporate the total volume instead.\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
namespace mech {
namespace binance {
struct binance_mech_data_t: public abstract_mech_data_t {
/* variables */
public:
  bool owns_session;
  cuwacunu::camahjucunu::curl::ws_session_id_t session_id;
public:
  /* constructor */
  binance_mech_data_t(cuwacunu::camahjucunu::curl::ws_session_id_t _session_id = NULL_CURL_SESSION);
  ~binance_mech_data_t();

  /* methods */
  std::optional<depth_ret_t> depth(depth_args_t args, bool await) const override;
  std::optional<klines_ret_t> klines(klines_args_t args, bool await) const override;
  std::optional<avgPrice_ret_t> avgPrice(avgPrice_args_t args, bool await) const override;
  std::optional<ticker_ret_t> ticker(ticker_args_t args, bool await) const override;
  std::optional<tickerTradingDay_ret_t> ticker_tradingDay(tickerTradingDay_args_t args, bool await) const override;
  std::optional<tickerPrice_ret_t> tickerPrice(tickerPrice_args_t args, bool await) const override;
  std::optional<tickerBook_ret_t> tickerBook(tickerBook_args_t args, bool await) const override;
  std::optional<tradesRecent_ret_t> tradesRecent(tradesRecent_args_t args, bool await) const override;
  std::optional<tradesHistorical_ret_t> tradesHistorical(tradesHistorical_args_t args, bool await) const override;
};

} /* namespace binance */
} /* namespace mech */
} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
