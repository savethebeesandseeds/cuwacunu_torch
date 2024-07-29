#pragma once
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/crypto_exchange/binance_types.h"
#include "camahjucunu/crypto_exchange/binance_enums.h"

RUNTIME_WARNING("(binance_mech.h)[] remmember the server endpoint is asynchonous.\n");
RUNTIME_WARNING("(binance_mech.h)[] implement a count on the api usage limit.\n");
namespace cuwacunu {
namespace camahjucunu {
using namespace binance;

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         virtual exchange structure          */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct exchange_mech_t {
  virtual ping_ret_t                    ping                    (                    ping_args_t args )  const = 0;
  virtual ping_ret_t                    time                    (                    ping_args_t args )  const = 0;
  virtual depth_ret_t                   depth                   (                   depth_args_t args )  const = 0;
  virtual trades_ret_t                  trades                  (                  trades_args_t args )  const = 0;
  virtual historicalTrades_ret_t        historicalTrades        (        historicalTrades_args_t args )  const = 0;
  virtual klines_ret_t                  klines                  (                  klines_args_t args )  const = 0;
  virtual avgPrice_ret_t                avgPrice                (                avgPrice_args_t args )  const = 0;
  virtual ticker_24hr_ret_t             ticker_24hr             (             ticker_24hr_args_t args )  const = 0;
  virtual ticker_tradingDay_ret_t       ticker_tradingDay       (       ticker_tradingDay_args_t args )  const = 0;
  virtual ticker_price_ret_t            ticker_price            (            ticker_price_args_t args )  const = 0;
  virtual ticker_bookTicker_ret_t       ticker_bookTicker       (       ticker_bookTicker_args_t args )  const = 0;
  virtual ticker_wind_ret_t             ticker_wind             (             ticker_wind_args_t args )  const = 0;
  virtual order_limit_ret_t             order_limit             (             order_limit_args_t args )  const = 0;
  virtual order_market_ret_t            order_market            (            order_market_args_t args )  const = 0;
  virtual order_stop_loss_ret_t         order_stop_loss         (         order_stop_loss_args_t args )  const = 0;
  virtual order_stop_loss_limit_ret_t   order_stop_loss_limit   (   order_stop_loss_limit_args_t args )  const = 0;
  virtual order_take_profit_ret_t       order_take_profit       (       order_take_profit_args_t args )  const = 0;
  virtual order_take_profit_limit_ret_t order_take_profit_limit ( order_take_profit_limit_args_t args )  const = 0;
  virtual order_limit_maker_ret_t       order_limit_maker       (       order_limit_maker_args_t args )  const = 0;
  virtual order_sor_ret_t               order_sor               (               order_sor_args_t args )  const = 0;
  virtual account_information_ret_t     account_information     (     account_information_args_t args )  const = 0;
  virtual account_trade_list_ret_t      account_trade_list      (      account_trade_list_args_t args )  const = 0;
  virtual query_commision_rates_ret_t   query_commision_rates   (   query_commision_rates_args_t args )  const = 0;
  virtual ~exchange_mech_t              () {} /* destructor */
};

} /* namespace camahjucunu */
} /* namespace cuwacunu */