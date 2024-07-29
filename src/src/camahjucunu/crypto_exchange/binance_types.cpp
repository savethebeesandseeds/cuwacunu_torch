#include "camahjucunu/crypto_exchange/binance_types.h"
#include "camahjucunu/crypto_exchange/binance_serialization.h"
#include "camahjucunu/crypto_exchange/binance_deserialization.h"

RUNTIME_WARNING("(binance_types.cpp)[] it is convinient (due to std::variant and std::optional) for this file to have c++20, everything on the libtorch uses c++17\n");

namespace cuwacunu {
namespace camahjucunu {
namespace binance {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string                    ping_args_t::jsonify() { return "{}"; }
std::string                    time_args_t::jsonify() { return "{}"; }
std::string                   depth_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit)); }
std::string                  trades_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit)); }
std::string        historicalTrades_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(limit), pairWrap(fromId)); }
std::string                  klines_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(interval), pairWrap(startTime), pairWrap(endTime), pairWrap(timeZone), pairWrap(limit)); }
std::string                avgPrice_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol)); }
std::string             ticker_24hr_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(type)); }
std::string       ticker_tradingDay_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(timeZone), pairWrap(type)); }
std::string            ticker_price_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols)); }
std::string       ticker_bookTicker_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols)); }
std::string             ticker_wind_args_t::jsonify() { return jsonify_as_object(pairWrap(symbols), pairWrap(windowSize), pairWrap(type)); }
std::string             order_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string            order_market_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string         order_stop_loss_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string   order_stop_loss_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string       order_take_profit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string order_take_profit_limit_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string       order_limit_maker_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(quoteOrderQty), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(stopPrice), pairWrap(trailingDelta), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string               order_sor_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(side), pairWrap(type), pairWrap(timeInForce), pairWrap(quantity), pairWrap(price), pairWrap(newClientOrderId), pairWrap(strategyId), pairWrap(strategyType), pairWrap(icebergQty), pairWrap(newOrderRespType), pairWrap(selfTradePreventionMode), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string     account_information_args_t::jsonify() { return jsonify_as_object(pairWrap(omitZeroBalances), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string      account_trade_list_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol), pairWrap(orderId), pairWrap(startTime), pairWrap(endTime), pairWrap(fromId), pairWrap(limit), pairWrap(recvWindow), pairWrap(timestamp)); }
std::string   query_commision_rates_args_t::jsonify() { return jsonify_as_object(pairWrap(symbol)); }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
/* secondary return structs */
price_qty_t::price_qty_t(const std::string &json) { deserialize(*this, json); };
trade_t::trade_t(const std::string &json) { deserialize(*this, json); };
kline_t::kline_t(const std::string &json) { deserialize(*this, json); };
tick_full_t::tick_full_t(const std::string &json) { deserialize(*this, json); };


/* primary return structs */
ping_ret_t::ping_ret_t(const std::string &json) { deserialize(*this, json); };
time_ret_t::time_ret_t(const std::string &json) { deserialize(*this, json); };
depth_ret_t::depth_ret_t(const std::string &json) { deserialize(*this, json); };
trades_ret_t::trades_ret_t(const std::string &json) { deserialize(*this, json); };
historicalTrades_ret_t::historicalTrades_ret_t(const std::string &json) { deserialize(*this, json); };
klines_ret_t::klines_ret_t(const std::string &json) { deserialize(*this, json); };
avgPrice_ret_t::avgPrice_ret_t(const std::string &json) { deserialize(*this, json); };
ticker_24hr_ret_t::ticker_24hr_ret_t(const std::string &json) { deserialize(*this, json); };
} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */