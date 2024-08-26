#pragma once
#include <vector>
#include <variant>
#include <optional>
#include <sstream>
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/exchange/binance/binance_enums.h"

#define pairWrap(variable) cuwacunu::piaabo::dPair<const std::string, decltype(variable)>{#variable, variable}

namespace cuwacunu {
namespace camahjucunu {
namespace binance {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_args_t                    { std::string jsonify(); };
struct time_args_t                    { std::string jsonify(); };
struct depth_args_t                   { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct trades_args_t                  { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct historicalTrades_args_t        { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; std::optional<long> fromId = std::nullopt; };
struct klines_args_t                  { std::string jsonify(); std::string symbol; interval_type_e interval; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<std::string> timeZone = std::nullopt; std::optional<int> limit = std::nullopt; };
struct avgPrice_args_t                { std::string jsonify(); std::string symbol; };
struct ticker_24hr_args_t             { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbols; std::optional<ticker_type_e> type = std::nullopt; };
struct ticker_tradingDay_args_t       { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbols; std::optional<std::string> timeZone = std::nullopt; std::optional<ticker_type_e> type = std::nullopt; };
struct ticker_price_args_t            { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbols; };
struct ticker_bookTicker_args_t       { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbols; };
struct ticker_wind_args_t             { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbols; std::optional<interval_type_e> windowSize = std::nullopt; std::optional<ticker_type_e> type = std::nullopt; };
using ticker_args_t                   = std::variant<ticker_24hr_args_t,ticker_tradingDay_args_t,ticker_price_args_t,ticker_bookTicker_args_t,ticker_wind_args_t>;
struct order_limit_args_t             { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_market_args_t            { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_stop_loss_args_t         { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_stop_loss_limit_args_t   { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_take_profit_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_take_profit_limit_args_t { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_limit_maker_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct order_sor_args_t               { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
using order_args_t                    = std::variant<order_limit_args_t, order_market_args_t, order_stop_loss_args_t, order_stop_loss_limit_args_t, order_take_profit_args_t, order_take_profit_limit_args_t, order_limit_maker_args_t, order_sor_args_t>;
struct account_information_args_t     { std::string jsonify(); std::optional<bool> omitZeroBalances = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct account_trade_list_args_t      { std::string jsonify(); std::string symbol; std::optional<long> orderId = std::nullopt; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<long> fromId = std::nullopt; std::optional<int> limit = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
struct query_commision_rates_args_t   { std::string jsonify(); std::string symbol; };

ENFORCE_ARCHITECTURE_DESIGN(                    ping_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                    time_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                   depth_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                  trades_args_t);
ENFORCE_ARCHITECTURE_DESIGN(        historicalTrades_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                  klines_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                avgPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_24hr_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_tradingDay_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            ticker_price_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_bookTicker_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_wind_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             order_limit_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            order_market_args_t);
ENFORCE_ARCHITECTURE_DESIGN(         order_stop_loss_args_t);
ENFORCE_ARCHITECTURE_DESIGN(   order_stop_loss_limit_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       order_take_profit_args_t);
ENFORCE_ARCHITECTURE_DESIGN( order_take_profit_limit_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       order_limit_maker_args_t);
ENFORCE_ARCHITECTURE_DESIGN(               order_sor_args_t);
ENFORCE_ARCHITECTURE_DESIGN(     account_information_args_t);
ENFORCE_ARCHITECTURE_DESIGN(      account_trade_list_args_t);
ENFORCE_ARCHITECTURE_DESIGN(   query_commision_rates_args_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secondary return structs */
struct price_qty_t           { price_qty_t(const std::string& json); double price; double qty; };
struct tick_full_t           { tick_full_t(const std::string& json); std::string symbol; double pricexchange; double pricexchangePercent; double weightedAvgPrice; double prevClosePrice; double lastPrice; double lastQty; double bidPrice; double bidQty; double askPrice; double askQty; double openPrice; double highPrice; double lowPrice; double volume; double quoteVolume; long openTime; long closeTime; long firstId; long lastId; int count; };
struct tick_mini_t           { tick_mini_t(const std::string& json); std::string symbol; double lastPrice; double openPrice; double highPrice; double lowPrice; double volume; double quoteVolume; long openTime; long closeTime; long firstId; long lastId; int count; };
struct trade_t               { trade_t(const std::string& json); long id; double price; double qty; double quoteQty; long time; bool isBuyerMaker; bool isBestMatch; };
struct kline_t               { kline_t(const std::string& json); long open_time; double open_price; double high_price; double low_price; double close_price; double volume; long close_time; double quote_asset_volume; int number_of_trades; double taker_buy_base_volume; double taker_buy_quote_volume; };
struct price_t               { price_t(const std::string& json); std::string symbol; double price; };
struct bookPrice_t           { bookPrice_t(const std::string& json); std::string symbol; double bidPrice; double bidQty; double askPrice; double askQty; };
struct commissionRates_t     { commissionRates_t(); commissionRates_t(const std::string& json); double maker; double taker; double buyer; double seller; };
struct comission_discount_t  { comission_discount_t(); comission_discount_t(const std::string& json); bool enabledForAccount; bool enabledForSymbol; std::string discountAsset; double discount; };
struct balance_t             { balance_t(const std::string& json); std::string asset; double free; double locked; };
struct historicTrade_t       { historicTrade_t(const std::string& json); std::string symbol; int id; int orderId; int orderListId; double price; double qty; double quoteQty; double commission; std::string commissionAsset; long time; bool isBuyer; bool isMaker; bool isBestMatch; };
struct order_ack_resp_t      { order_ack_resp_t(const std::string& json); std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; };
struct order_result_resp_t   { order_result_resp_t(); order_result_resp_t(const std::string& json);  std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; double origQty; double executedQty; double cummulativeQuoteQty; order_status_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; long workingTime; stp_modes_e selfTradePreventionMode; };
struct order_fill_t          { order_fill_t(); order_fill_t(const std::string& json); double price; double qty; double commission; std::string commissionAsset; int tradeId; };
struct order_full_resp_t     { order_full_resp_t(const std::string& json); order_result_resp_t result; std::vector<order_fill_t> fills; };
struct order_sor_fill_t      { order_sor_fill_t(); order_sor_fill_t(const std::string& json); std::string matchType; double price; double qty; double commission; std::string commissionAsset; int tradeId; int allocId; };
struct order_sor_full_resp_t { order_sor_full_resp_t(const std::string& json); order_result_resp_t result; double price; allocation_type_e workingFloor; bool usedSor; std::vector<order_sor_fill_t> fills; };

ENFORCE_ARCHITECTURE_DESIGN(          price_qty_t);
ENFORCE_ARCHITECTURE_DESIGN(          tick_full_t);
ENFORCE_ARCHITECTURE_DESIGN(          tick_mini_t);
ENFORCE_ARCHITECTURE_DESIGN(              trade_t);
ENFORCE_ARCHITECTURE_DESIGN(              kline_t);
ENFORCE_ARCHITECTURE_DESIGN(              price_t);
ENFORCE_ARCHITECTURE_DESIGN(          bookPrice_t);
ENFORCE_ARCHITECTURE_DESIGN(     order_ack_resp_t);
ENFORCE_ARCHITECTURE_DESIGN(  order_result_resp_t);
ENFORCE_ARCHITECTURE_DESIGN(         order_fill_t);
ENFORCE_ARCHITECTURE_DESIGN(    order_full_resp_t);
ENFORCE_ARCHITECTURE_DESIGN(    commissionRates_t);
ENFORCE_ARCHITECTURE_DESIGN( comission_discount_t);
ENFORCE_ARCHITECTURE_DESIGN(            balance_t);
ENFORCE_ARCHITECTURE_DESIGN(      historicTrade_t);

/* primary return structs */
struct ping_ret_t                   { ping_ret_t(const std::string& json); };
struct time_ret_t                   { time_ret_t(const std::string& json); long serverTime; };
struct depth_ret_t                  { depth_ret_t(const std::string& json); long lastUpdateId; std::vector<price_qty_t> bids; std::vector<price_qty_t> asks; };
struct trades_ret_t                 { trades_ret_t(const std::string& json); std::vector<trade_t> trades; };
struct historicalTrades_ret_t       { historicalTrades_ret_t(const std::string& json); std::vector<trade_t> trades; };
struct klines_ret_t                 { klines_ret_t(const std::string& json); std::vector<kline_t> klines; };
struct avgPrice_ret_t               { avgPrice_ret_t(const std::string& json); int mins; double price; long close_time; };
struct ticker_24hr_ret_t            { ticker_24hr_ret_t(const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t> tick; bool is_full;};
struct ticker_tradingDay_ret_t      { ticker_tradingDay_ret_t(const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t> tick; bool is_full; };
struct ticker_price_ret_t           { ticker_price_ret_t(const std::string& json); std::vector<price_t> prices; };
struct ticker_bookTicker_ret_t      { ticker_bookTicker_ret_t(const std::string& json); std::vector<bookPrice_t> bookPrices; };
struct ticker_wind_ret_t            { ticker_wind_ret_t(const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t> tick; bool is_full; };
using ticker_ret_t                  = std::variant<ticker_24hr_ret_t, ticker_tradingDay_ret_t, ticker_price_ret_t, ticker_bookTicker_ret_t, ticker_wind_ret_t>;
using order_limit_ret_t             = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_market_ret_t            = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_stop_loss_ret_t         = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_stop_loss_limit_ret_t   = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_take_profit_ret_t       = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_take_profit_limit_ret_t = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_limit_maker_ret_t       = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_sor_ret_t               = std::variant<order_sor_full_resp_t>;
using order_ret_t                   = std::variant<order_limit_ret_t, order_market_ret_t, order_stop_loss_ret_t, order_stop_loss_limit_ret_t, order_take_profit_ret_t, order_take_profit_limit_ret_t, order_limit_maker_ret_t, order_sor_ret_t>;
struct account_information_ret_t    { account_information_ret_t(const std::string &json); int makerCommission; int takerCommission; int buyerCommission; int sellerCommission; commissionRates_t commissionRates; bool canTrade; bool canWithdraw; bool canDeposit; bool brokered; bool requireSelfTradePrevention; bool preventSor; long updateTime; account_and_symbols_permissions_e accountType; std::vector<balance_t> balances; std::vector<account_and_symbols_permissions_e> permissions; long uid; };
struct account_trade_list_ret_t     { account_trade_list_ret_t(const std::string &json); std::vector<historicTrade_t> trades; };
struct query_commision_rates_ret_t  { query_commision_rates_ret_t(const std::string &json); std::string symbol; commissionRates_t standardCommission; commissionRates_t taxCommission; comission_discount_t discount; };

ENFORCE_ARCHITECTURE_DESIGN(                    ping_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                    time_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                   depth_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                  trades_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(        historicalTrades_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                  klines_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                avgPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_24hr_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_tradingDay_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(            ticker_price_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_bookTicker_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_wind_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(     account_information_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(      account_trade_list_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(   query_commision_rates_ret_t);

} /* namespace binance */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

/* Define macros for accessing these types within a std::variant */
#define GET_TICK_FULL(obj) (std::get<cuwacunu::camahjucunu::binance::tick_full_t>(obj.tick))
#define GET_TICK_MINI(obj) (std::get<cuwacunu::camahjucunu::binance::tick_mini_t>(obj.tick))
