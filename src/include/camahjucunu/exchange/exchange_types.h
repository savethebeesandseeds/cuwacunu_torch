#pragma once
#include <vector>
#include <variant>
#include <optional>
#include <sstream>
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_enums.h"

#define pairWrap(variable) cuwacunu::piaabo::dPair<const std::string, decltype(variable)>{#variable, variable}

#define pairWrap_variant(variable) cuwacunu::piaabo::dPair<const std::string, decltype(variable)>{std::holds_alternative<std::vector<std::string>>(variable) ? #variable"s" : "symbol" , variable}

RUNTIME_WARNING("(exchange_types.h)[] implement other types of orders.\n");

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

struct frame_response_t { uint http_status; std::string frame_id; };
ENFORCE_ARCHITECTURE_DESIGN(frame_response_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct ping_args_t                    { std::string jsonify(); };
struct time_args_t                    { std::string jsonify(); };
struct depth_args_t                   { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct trades_args_t                  { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct tradesHistorical_args_t        { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; std::optional<long> fromId = std::nullopt; };
struct klines_args_t                  { std::string jsonify(); std::string symbol; interval_type_e interval; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<std::string> timeZone = std::nullopt; std::optional<int> limit = std::nullopt; };
struct avgPrice_args_t                { std::string jsonify(); std::string symbol; };
struct ticker_args_t                  { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; std::optional<ticker_interval_e> windowSize = std::nullopt; std::optional<ticker_type_e> type = std::nullopt; };
struct tickerTradingDay_args_t        { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; std::optional<ticker_type_e> type = std::nullopt; std::optional<std::string> timeZone = std::nullopt; };
struct tickerPrice_args_t             { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; };
struct tickerBook_args_t              { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; };
ENFORCE_ARCHITECTURE_DESIGN(                   ping_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                   time_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                  depth_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                 trades_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       tradesHistorical_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                 klines_args_t);
ENFORCE_ARCHITECTURE_DESIGN(               avgPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       tickerTradingDay_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                 ticker_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickerPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             tickerBook_args_t);

/* order arguments */
struct orderStatus_args_t            { std::string jsonify(); std::string symbol; int orderId; std::string origClientOrderId; std::string apiKey; std::optional<long> recvWindow; std::string signature; long timestamp; };
struct orderMarket_args_t            { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
ENFORCE_ARCHITECTURE_DESIGN(           orderStatus_args_t);
ENFORCE_ARCHITECTURE_DESIGN(           orderMarket_args_t);

/* account arguments */
struct account_information_args_t     { std::string jsonify(); std::optional<bool> omitZeroBalances = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct account_order_history_args_t   { std::string jsonify(); std::string symbol; std::optional<int> orderId = std::nullopt; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<long> limit = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct account_trade_list_args_t      { std::string jsonify(); std::string symbol; std::optional<int> orderId = std::nullopt; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<int> fromId = std::nullopt; std::optional<int> limit = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct query_commission_rates_args_t  { std::string jsonify(); std::string symbol; std::string apiKey; std::string signature; long timestamp; };
ENFORCE_ARCHITECTURE_DESIGN(    account_information_args_t);
ENFORCE_ARCHITECTURE_DESIGN(  account_order_history_args_t);
ENFORCE_ARCHITECTURE_DESIGN(     account_trade_list_args_t);
ENFORCE_ARCHITECTURE_DESIGN( query_commission_rates_args_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secondary return structs */
struct price_qty_t           { double price; double qty; };
struct tick_full_t           { std::string symbol; double priceChange; double priceChangePercent; double weightedAvgPrice; double prevClosePrice; double lastPrice; double lastQty; double bidPrice; double bidQty; double askPrice; double askQty; double openPrice; double highPrice; double lowPrice; double volume; double quoteVolume; long openTime; long closeTime; long firstId; int lastId; int count; };
struct tick_mini_t           { std::string symbol; double lastPrice; double openPrice; double highPrice; double lowPrice; double volume; double quoteVolume; long openTime; long closeTime; long firstId; int lastId; int count; };
struct trade_t               { long id; double price; double qty; double quoteQty; long time; bool isBuyerMaker; bool isBestMatch; };
struct kline_t               { long open_time; double open_price; double high_price; double low_price; double close_price; double volume; long close_time; double quote_asset_volume; int number_of_trades; double taker_buy_base_volume; double taker_buy_quote_volume; };
struct price_t               { std::string symbol; double price; };
struct bookPrice_t           { std::string symbol; double bidPrice; double bidQty; double askPrice; double askQty; };
struct commissionRates_t     { double maker; double taker; double buyer; double seller; };
struct commission_discount_t { bool enabledForAccount; bool enabledForSymbol; std::string discountAsset; double discount; };
struct balance_t             { std::string asset; double free; double locked; };
struct historicTrade_t       { std::string symbol; int id; int orderId; int orderListId; double price; double qty; double quoteQty; double commission; std::string commissionAsset; long time; bool isBuyer; bool isMaker; bool isBestMatch; };
struct commissionDesc_t      { std::string symbol; commissionRates_t standardCommission; commissionRates_t taxCommission; commission_discount_t discount; };
ENFORCE_ARCHITECTURE_DESIGN(           price_qty_t);
ENFORCE_ARCHITECTURE_DESIGN(           tick_full_t);
ENFORCE_ARCHITECTURE_DESIGN(           tick_mini_t);
ENFORCE_ARCHITECTURE_DESIGN(               trade_t);
ENFORCE_ARCHITECTURE_DESIGN(               kline_t);
ENFORCE_ARCHITECTURE_DESIGN(               price_t);
ENFORCE_ARCHITECTURE_DESIGN(           bookPrice_t);
ENFORCE_ARCHITECTURE_DESIGN(     commissionRates_t);
ENFORCE_ARCHITECTURE_DESIGN( commission_discount_t);
ENFORCE_ARCHITECTURE_DESIGN(             balance_t);
ENFORCE_ARCHITECTURE_DESIGN(       historicTrade_t);
ENFORCE_ARCHITECTURE_DESIGN(      commissionDesc_t);

/* order returns */
struct orderStatus_ret_t { frame_response_t frame_rsp; orderStatus_ret_t(); orderStatus_ret_t(const std::string& json); std::string symbol; long orderId; int orderListId; std::string clientOrderId; double price; double origQty; double executedQty; double cummulativeQuoteQty; orderStatus_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; double stopPrice; double icebergQty; long time; long updateTime; bool isWorking; long workingTime; double origQuoteOrderQty; stp_modes_e selfTradePreventionMode; int preventedMatchId; double preventedQuantity; long trailingDelta; long trailingTime; int strategyId; int strategyType; };
struct order_fill_t       { double price; double qty; double commission; std::string commissionAsset; int tradeId; };
struct order_ack_ret_t    { frame_response_t frame_rsp; order_ack_ret_t(const std::string& json); std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; };
struct order_result_ret_t { frame_response_t frame_rsp; order_result_ret_t(); order_result_ret_t(const std::string& json); std::string symbol; int orderId; int orderListId; std::string clientOrderId; long transactTime; double origQty; double executedQty; double cummulativeQuoteQty; orderStatus_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; long workingTime; stp_modes_e selfTradePreventionMode; };
struct order_full_ret_t   { frame_response_t frame_rsp; order_full_ret_t(const std::string& json); order_result_ret_t result; std::vector<order_fill_t> fills; };
ENFORCE_ARCHITECTURE_DESIGN(   orderStatus_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(         order_fill_t);
ENFORCE_ARCHITECTURE_DESIGN(      order_ack_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(   order_result_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(     order_full_ret_t);
using orderMarket_ret_t   = std::variant<order_ack_ret_t, order_full_ret_t, order_result_ret_t>;

/* primary return structs */
struct ping_ret_t             { frame_response_t frame_rsp; ping_ret_t              (const std::string& json); };
struct time_ret_t             { frame_response_t frame_rsp; time_ret_t              (const std::string& json); long serverTime; };
struct depth_ret_t            { frame_response_t frame_rsp; depth_ret_t             (const std::string& json); long lastUpdateId; std::vector<price_qty_t> bids; std::vector<price_qty_t> asks; };
struct tradesRecent_ret_t     { frame_response_t frame_rsp; tradesRecent_ret_t      (const std::string& json); std::vector<trade_t> trades; };
struct tradesHistorical_ret_t { frame_response_t frame_rsp; tradesHistorical_ret_t  (const std::string& json); std::vector<trade_t> trades; };
struct klines_ret_t           { frame_response_t frame_rsp; klines_ret_t            (const std::string& json); std::vector<kline_t> klines; };
struct avgPrice_ret_t         { frame_response_t frame_rsp; avgPrice_ret_t          (const std::string& json); int mins; double price; long close_time; };
struct ticker_ret_t           { frame_response_t frame_rsp; ticker_ret_t            (const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t, std::vector<tick_full_t>, std::vector<tick_mini_t>> ticks; bool is_full; };
struct tickerTradingDay_ret_t { frame_response_t frame_rsp; tickerTradingDay_ret_t  (const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t, std::vector<tick_full_t>, std::vector<tick_mini_t>> ticks; bool is_full; };
struct tickerPrice_ret_t      { frame_response_t frame_rsp; tickerPrice_ret_t       (const std::string& json); std::variant<std::monostate, price_t, std::vector<price_t>> prices; };
struct tickerBook_ret_t       { frame_response_t frame_rsp; tickerBook_ret_t        (const std::string& json); std::variant<std::monostate, bookPrice_t, std::vector<bookPrice_t>> bookPrices; };
ENFORCE_ARCHITECTURE_DESIGN(                   ping_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                   time_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                  depth_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(           tradesRecent_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       tradesHistorical_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                 klines_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(               avgPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       tickerTradingDay_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                 ticker_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickerPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             tickerBook_ret_t);

/* account returns */

struct account_information_ret_t    { frame_response_t frame_rsp; account_information_ret_t(const std::string &json); int makerCommission; int takerCommission; int buyerCommission; int sellerCommission; commissionRates_t commissionRates; bool canTrade; bool canWithdraw; bool canDeposit; bool brokered; bool requireSelfTradePrevention; bool preventSor; long updateTime; account_and_symbols_permissions_e accountType; std::vector<balance_t> balances; std::vector<account_and_symbols_permissions_e> permissions; long uid; };
struct account_order_history_ret_t  { frame_response_t frame_rsp; account_order_history_ret_t(const std::string &json); std::vector<orderStatus_ret_t> orders; };
struct account_trade_list_ret_t     { frame_response_t frame_rsp; account_trade_list_ret_t(const std::string &json); std::vector<historicTrade_t> trades; };
struct query_commission_rates_ret_t { frame_response_t frame_rsp; query_commission_rates_ret_t(const std::string &json); std::vector<commissionDesc_t> commissionDesc; };
ENFORCE_ARCHITECTURE_DESIGN(    account_information_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(  account_order_history_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(     account_trade_list_ret_t);
ENFORCE_ARCHITECTURE_DESIGN( query_commission_rates_ret_t);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

/* Define macros for accessing these types within a std::variant */
#define GET_OBJECT(obj, obj_type) (std::get<obj_type>(obj))
#define GET_VECT_OBJECT(obj, obj_type) (std::get<std::vector<obj_type>>(obj))

#define GET_TICK_FULL(obj) (std::get<cuwacunu::camahjucunu::exchange::tick_full_t>(obj.ticks))
#define GET_TICK_MINI(obj) (std::get<cuwacunu::camahjucunu::exchange::tick_mini_t>(obj.ticks))

#define GET_VECT_TICK_FULL(obj) (std::get<std::vector<cuwacunu::camahjucunu::exchange::tick_full_t>>(obj.ticks))
#define GET_VECT_TICK_MINI(obj) (std::get<std::vector<cuwacunu::camahjucunu::exchange::tick_mini_t>>(obj.ticks))

// struct order_limit_args_t             { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_stop_loss_args_t         { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_stop_loss_limit_args_t   { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_take_profit_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_take_profit_limit_args_t { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; double stopPrice; long trailingDelta; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_limit_maker_args_t       { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; double quantity; std::optional<double> quoteOrderQty = std::nullopt; double price; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> stopPrice = std::nullopt; std::optional<long> trailingDelta = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// struct order_sor_args_t               { std::string jsonify(); std::string symbol; order_side_e side; order_type_e type; std::optional<time_in_force_e> timeInForce = std::nullopt; std::optional<double> quantity = std::nullopt; std::optional<double> price = std::nullopt; std::optional<std::string> newClientOrderId = std::nullopt; std::optional<int> strategyId = std::nullopt; std::optional<int> strategyType = std::nullopt; std::optional<double> icebergQty = std::nullopt; std::optional<order_response_type_e> newOrderRespType = std::nullopt; std::optional<stp_modes_e> selfTradePreventionMode = std::nullopt; std::optional<long> recvWindow = std::nullopt; long timestamp; };
// using order_args_t                    = std::variant<order_limit_args_t, orderMarket_args_t, order_stop_loss_args_t, order_stop_loss_limit_args_t, order_take_profit_args_t, order_take_profit_limit_args_t, order_limit_maker_args_t, order_sor_args_t>;
// ENFORCE_ARCHITECTURE_DESIGN(             order_limit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(         order_stop_loss_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(   order_stop_loss_limit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(       order_take_profit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN( order_take_profit_limit_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(       order_limit_maker_args_t);
// ENFORCE_ARCHITECTURE_DESIGN(               order_sor_args_t);

// struct order_sor_fill_t      { order_sor_fill_t(); order_sor_fill_t(const std::string& json); std::string matchType; double price; double qty; double commission; std::string commissionAsset; int tradeId; int allocId; };
// struct order_sor_full_ret_t { order_sor_full_ret_t(const std::string& json); order_result_ret_t result; double price; allocation_type_e workingFloor; bool usedSor; std::vector<order_sor_fill_t> fills; };
// ENFORCE_ARCHITECTURE_DESIGN(      order_sor_fill_t);
// ENFORCE_ARCHITECTURE_DESIGN( order_sor_full_ret_t);
// using order_limit_ret_t             = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_stop_loss_ret_t         = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_stop_loss_limit_ret_t   = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_take_profit_ret_t       = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_take_profit_limit_ret_t = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_limit_maker_ret_t       = std::variant<order_ack_ret_t, order_result_ret_t, order_full_ret_t>;
// using order_sor_ret_t               = std::variant<order_sor_full_ret_t>;
// using order_ret_t                   = std::variant<order_limit_ret_t, orderMarket_ret_t, order_stop_loss_ret_t, order_stop_loss_limit_ret_t, order_take_profit_ret_t, order_take_profit_limit_ret_t, order_limit_maker_ret_t, order_sor_ret_t>;
