#pragma once
#include <variant>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/crypto_echange_enums.h"
THROW_RUNTIME_WARNING("(crypto_exchange_mech.h)[] it is convinient for this file to have c++20, everything on the libtorch uses c++17\n");
THROW_RUNTIME_WARNING("(crypto_exchange_mech.h)[] parsing args and resp is somehow slow. Leaving this until further upgrade.\n");
namespace cuwacunu {
namespace camahjucunu {
namespace binance {

/*
  template<typename T> struct default_value;
  template<> struct default_value<int>          { static constexpr int value = -1; };
  template<> struct default_value<float>        { static constexpr float value = -1.0f; };
  template<> struct default_value<long>         { static constexpr long value = -1L; };
  template<> struct default_value<std::string>  { static constexpr const char* value = "N/A"; };
  template <typename T> T _() { return default_value<T>::value; }
*/

/* arguments to mech functions */
struct ping_args_t                    { };
struct time_args_t                    { };
struct depth_args_t                   { std::string symbol; int limit = 100; };
struct trades_args_t                  { std::string symbol; int limit = 500; };
struct historicalTrades_args_t        { std::string symbol; int limit = 500; long fromId = 0 };
struct klines_args_t                  { std::string symbol; interval_type_e interval; long startTime = 0; long endTime = 0; std::string timeZone = "0"; int limit = 0; };
struct avgPrice_args_t                { std::string symbol; };
struct ticker_24hr_args_t             { std::string symbol = ""; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct tickers_24hr_args_t            { std::vector<std::string> symbols = {}; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct ticker_tradingDay_args_t       { std::string symbol = ""; std::string timeZone = "0"; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct tickers_tradingDay_args_t      { std::vector<std::string> symbols = {}; std::string timeZone = "0"; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct ticker_price_args_t            { std::string symbol = ""; };
struct tickers_price_args_t           { std::vector<std::string> symbols = {}; };
struct ticker_bookTicker_args_t       { std::string symbol = ""; };
struct tickers_bookTicker_args_t      { std::vector<std::string> symbols = {}; };
struct ticker_wind_args_t             { std::string symbol = ""; interval_type_e windowSize = interval_type_e::interval_1d; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct tickers_wind_args_t            { std::vector<std::string> symbols = {}; interval_type_e windowSize = interval_type_e::interval_1d; ticker_type_e type = ticker_type_e::ticker_FULL; };
struct order_limit_args_t             { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_market_args_t            { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_stop_loss_args_t         { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_stop_loss_limit_args_t   { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_take_profit_args_t       { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_take_profit_limit_args_t { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_limit_maker_args_t       { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float quoteOrderQty; float price; std::string newClientOrderId; int strategyId; int strategyType; float stopPrice; long trailingDelta; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct order_sor_args_t               { std::string symbol; order_side_e side; order_type_e type; time_in_force_e timeInForce; float quantity; float price; std::string newClientOrderId; int strategyId; int strategyType; float icebergQty; order_response_type_e newOrderRespType; stp_modes_e selfTradePreventionMode; long recvWindow; long timestamp; };
struct account_information_args_t     { bool omitZeroBalances = false; long recvWindow; long timestamp; };
struct account_trade_list_args_t      { std::string symbol; long orderId; long startTime; long endTime; long fromId; int limit; long recvWindow; long timestamp; };
struct query_commision_rates_args_t   { std::string symbol; };

ENFORCE_ARCHITECTURE_DESIGN(                    ping_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                    time_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                   depth_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                  trades_args_t);
ENFORCE_ARCHITECTURE_DESIGN(        historicalTrades_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                  klines_args_t);
ENFORCE_ARCHITECTURE_DESIGN(                avgPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_24hr_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickers_24hr_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_tradingDay_args_t);
ENFORCE_ARCHITECTURE_DESIGN(      tickers_tradingDay_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            ticker_price_args_t);
ENFORCE_ARCHITECTURE_DESIGN(           tickers_price_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_bookTicker_args_t);
ENFORCE_ARCHITECTURE_DESIGN(      tickers_bookTicker_args_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_wind_args_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickers_wind_args_t);
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

bool ping_args_validation                    (                    ping_args_t args);
bool time_args_validation                    (                    time_args_t args);
bool depth_args_validation                   (                   depth_args_t args);
bool trades_args_validation                  (                  trades_args_t args);
bool historicalTrades_args_validation        (        historicalTrades_args_t args);
bool klines_args_validation                  (                  klines_args_t args);
bool avgPrice_args_validation                (                avgPrice_args_t args);
bool ticker_24hr_args_validation             (             ticker_24hr_args_t args);
bool tickers_24hr_args_validation            (            tickers_24hr_args_t args);
bool ticker_tradingDay_args_validation       (       ticker_tradingDay_args_t args);
bool tickers_tradingDay_args_validation      (      tickers_tradingDay_args_t args);
bool ticker_price_args_validation            (            ticker_price_args_t args);
bool tickers_price_args_validation           (           tickers_price_args_t args);
bool ticker_bookTicker_args_validation       (       ticker_bookTicker_args_t args);
bool tickers_bookTicker_args_validation      (      tickers_bookTicker_args_t args);
bool ticker_wind_args_validation             (             ticker_wind_args_t args);
bool tickers_wind_args_validation            (            tickers_wind_args_t args);
bool order_limit_args_validation             (             order_limit_args_t args);
bool order_market_args_validation            (            order_market_args_t args);
bool order_stop_loss_args_validation         (         order_stop_loss_args_t args);
bool order_stop_loss_limit_args_validation   (   order_stop_loss_limit_args_t args);
bool order_take_profit_args_validation       (       order_take_profit_args_t args);
bool order_take_profit_limit_args_validation ( order_take_profit_limit_args_t args);
bool order_limit_maker_args_validation       (       order_limit_maker_args_t args);
bool order_sor_args_validation               (               order_sor_args_t args);
bool account_information_args_validation     (     account_information_args_t args);
bool account_trade_list_args_validation      (      account_trade_list_args_t args);
bool query_commision_rates_args_validation   (   query_commision_rates_args_t args);

/* secondary return structs */
struct price_qty_t           { float price; float qty;};
struct tick_full_t           { std::string symbol; float priceChange; float priceChangePercent; float weightedAvgPrice; float prevClosePrice; float lastPrice; float lastQty; float bidPrice; float bidQty; float askPrice; float askQty; float openPrice; float highPrice; float lowPrice; float volume; float quoteVolume; long openTime; long closeTime; long firstId; long lastId; int count; };
struct tick_mini_t           { std::string symbol; float lastPrice; float openPrice; float highPrice; float lowPrice; float volume; float quoteVolume; long openTime; long closeTime; long firstId; long lastId; int count; };
struct trade_t               { long id; float price; float qty; float quoteQty; long time; bool isBuyerMaker; bool isBestMatch; };
struct kline_t               { long open_time; float open_price; float high_price; float low_price; float close_price; float volume; long close_time; float quote_asset_volume; int number_of_trades; float taker_buy_base_volume; float taker_buy_quote_volume; };
struct price_t               { std::string symbol; float price; };
struct bookPrice_t           { std::string symbol; float bidPrice; float bigQty; float askPrice; float askQty; };
struct order_ack_resp_t      { std::string symbol; int orderId; int orderListId; std::string clientOrderId; double transactTime; };
struct order_result_resp_t   { std::string symbol; int orderId; int orderListId; std::string clientOrderId; double transactTime; float origQty; float executedQty; float cummulativeQuoteQty; order_status_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; double workingTime; stp_modes_e selfTradePreventionMode; };
struct order_fill_t          { float price; float qty; float commission; std::string commissionAsset; int tradeId; };
struct order_full_resp_t     { order_result_resp_t result; std::vector<order_fill_t> fills; };
struct order_sor_fill_t      { std::string matchType; float price; float qty; float commission; std::string commissionAsset; int tradeId; int allocId; };
struct order_sor_full_resp_t { std::string symbol; int orderId; int orderListId; std::string clientOrderId; double transactTime; float price; float origQty; float executedQty; float cummulativeQuoteQty; order_status_e status; time_in_force_e timeInForce; order_type_e type; order_side_e side; double workingTime; std::vector<order_sor_fill_t> fills; allocation_type_e workingFloor; stp_modes_e selfTradePreventionMode; bool usedSor; };
struct comission_t           { float maker; float taker; float buyer; float seller; };
struct comission_discount_t  { bool enabledForAccount; bool enabledForSymbol; std::string discountAsset; float discount; };
struct balance_t             { std::string asset; float free; float locked; };
struct account_data_t        { int makerCommission; int takerCommission; int buyerCommission; int sellerCommission; comission_t commissionRates; bool canTrade; bool canWithdraw; bool canDeposit; bool brokered; bool requireSelfTradePrevention; bool preventSor; double updateTime; account_and_symbols_permissions_e accountType; std::vector<> balances; account_and_symbols_permissions_e permissions; double uid; };
struct historicTrade_t       { std::string symbol; int id; int orderId; int orderListId; float price; float qty; float quoteQty; float commission; float commissionAsset; double time; bool isBuyer; bool isMaker; bool isBestMatch; }

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
ENFORCE_ARCHITECTURE_DESIGN(          comission_t);
ENFORCE_ARCHITECTURE_DESIGN( comission_discount_t);
ENFORCE_ARCHITECTURE_DESIGN(            balance_t);
ENFORCE_ARCHITECTURE_DESIGN(       account_data_t);
ENFORCE_ARCHITECTURE_DESIGN(      historicTrade_t);

/* primary return structs */
struct ping_ret_t                   {  };
struct time_ret_t                   { long serverTime };
struct depth_ret_t                  { long lastUpdateId, std::vector<price_qty_t> bids; std::vector<price_qty> asks; };
struct trades_ret_t                 { std::vector<trade_t> trades; };
struct historicalTrades_ret_t       { std::vector<trade_t> trades; };
struct klines_ret_t                 { std::vector<klines_t> klines; };
struct avgPrice_ret_t               { int mins; float price; long close_time; };
struct ticker_24hr_ret_t            { tick_full_t tick; };
struct tickers_24hrs_ret_t          { std::vector<tick_full_t> ticks; };
struct ticker_tradingDay_ret_t      { tick_full_t tick; };
struct tickers_tradingDay_ret_t     { std::vector<tick_full_t> ticks; };
struct ticker_price_ret_t           { price_t price; };
struct tickers_price_ret_t          { std::vector<price_t> prices };
struct ticker_bookTicker_ret_t      { bookPrice_t bookPrice; };
struct tickers_bookTicker_ret_t     { std::vector<bookPrice_t> bookPrices; };
struct ticker_wind_ret_t            { tick_full_t tick; };
struct tickers_wind_ret_t           { std::vector<tick_full_t> ticks; };
using order_limit_ret_t             = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_market_ret_t            = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_stop_loss_ret_t         = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_stop_loss_limit_ret_t   = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_take_profit_ret_t       = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_take_profit_limit_ret_t = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_limit_maker_ret_t       = std::variant<order_ack_resp_t, order_result_resp_t, order_full_resp_t>;
using order_sor_ret_t               = std::variant<order_sor_full_resp_t>;
struct account_information_ret_t    { account_data_t account_data; };
struct account_trade_list_ret_t     { std::vector<historicTrade_t> trades; };
struct query_commision_rates_ret_t  { std::string symbol; comission_t standardCommissionForOrder; comission_t taxCommissionForOrder; comission_discount_t discount };

ENFORCE_ARCHITECTURE_DESIGN(                    ping_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                    time_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                   depth_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                  trades_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(        historicalTrades_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                  klines_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(                avgPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_24hr_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickers_24hr_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_tradingDay_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(      tickers_tradingDay_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(            ticker_price_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(           tickers_price_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_bookTicker_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(      tickers_bookTicker_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(             ticker_wind_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(            tickers_wind_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(     account_information_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(      account_trade_list_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(   query_commision_rates_ret_t);

bool ping_ret_validation                    (                    ping_ret_t ret);
bool time_ret_validation                    (                    time_ret_t ret);
bool depth_ret_validation                   (                   depth_ret_t ret);
bool trades_ret_validation                  (                  trades_ret_t ret);
bool historicalTrades_ret_validation        (        historicalTrades_ret_t ret);
bool klines_ret_validation                  (                  klines_ret_t ret);
bool avgPrice_ret_validation                (                avgPrice_ret_t ret);
bool ticker_24hr_ret_validation             (             ticker_24hr_ret_t ret);
bool tickers_24hr_ret_validation            (            tickers_24hr_ret_t ret);
bool ticker_tradingDay_ret_validation       (       ticker_tradingDay_ret_t ret);
bool tickers_tradingDay_ret_validation      (      tickers_tradingDay_ret_t ret);
bool ticker_price_ret_validation            (            ticker_price_ret_t ret);
bool tickers_price_ret_validation           (           tickers_price_ret_t ret);
bool ticker_bookTicker_ret_validation       (       ticker_bookTicker_ret_t ret);
bool tickers_bookTicker_ret_validation      (      tickers_bookTicker_ret_t ret);
bool ticker_wind_ret_validation             (             ticker_wind_ret_t ret);
bool tickers_wind_ret_validation            (            tickers_wind_ret_t ret);
bool order_limit_ret_validation             (             order_limit_ret_t ret);
bool order_market_ret_validation            (            order_market_ret_t ret);
bool order_stop_loss_ret_validation         (         order_stop_loss_ret_t ret);
bool order_stop_loss_limit_ret_validation   (   order_stop_loss_limit_ret_t ret);
bool order_take_profit_ret_validation       (       order_take_profit_ret_t ret);
bool order_take_profit_limit_ret_validation ( order_take_profit_limit_ret_t ret);
bool order_limit_maker_ret_validation       (       order_limit_maker_ret_t ret);
bool order_sor_ret_validation               (               order_sor_ret_t ret);
bool account_information_ret_validation     (     account_information_ret_t ret);
bool account_trade_list_ret_validation      (      account_trade_list_ret_t ret);
bool query_commision_rates_ret_validation   (   query_commision_rates_ret_t ret);

struct exchange_mech_t {
  virtual ping_ret_t                    ping                    (                    ping_args_t args )  const = 0;
  virtual ping_ret_t                    time                    (                    ping_args_t args )  const = 0;
  virtual depth_ret_t                   depth                   (                   depth_args_t args )  const = 0;
  virtual trades_ret_t                  trades                  (                  trades_args_t args )  const = 0;
  virtual historicalTrades_ret_t        historicalTrades        (        historicalTrades_args_t args )  const = 0;
  virtual klines_ret_t                  klines                  (                  klines_args_t args )  const = 0;
  virtual avgPrice_ret_t                avgPrice                (                avgPrice_args_t args )  const = 0;
  virtual ticker_24hr_ret_t             ticker_24hr             (             ticker_24hr_args_t args )  const = 0;
  virtual tickers_24hr_ret_t            tickers_24hr            (            tickers_24hr_args_t args )  const = 0;
  virtual ticker_tradingDay_ret_t       ticker_tradingDay       (       ticker_tradingDay_args_t args )  const = 0;
  virtual tickers_tradingDay_ret_t      tickers_tradingDay      (      tickers_tradingDay_args_t args )  const = 0;
  virtual ticker_price_ret_t            ticker_price            (            ticker_price_args_t args )  const = 0;
  virtual tickers_price_ret_t           tickers_price           (           tickers_price_args_t args )  const = 0;
  virtual ticker_bookTicker_ret_t       ticker_bookTicker       (       ticker_bookTicker_args_t args )  const = 0;
  virtual tickers_bookTicker_ret_t      tickers_bookTicker      (      tickers_bookTicker_args_t args )  const = 0;
  virtual ticker_wind_ret_t             ticker_wind             (             ticker_wind_args_t args )  const = 0;
  virtual tickers_wind_ret_t            tickers_wind            (            tickers_wind_args_t args )  const = 0;
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

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */