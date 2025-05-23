#pragma once
#include "piaabo/math_compat/statistics_space.h"
#include "camahjucunu/exchange/exchange_utils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*      arguments structures       */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct depth_args_t       { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct tradesRecent_args_t      { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; };
struct tradesHistorical_args_t    { std::string jsonify(); std::string symbol; std::optional<int> limit = std::nullopt; std::optional<long> fromId = std::nullopt; };
struct klines_args_t          { std::string jsonify(); std::string symbol; interval_type_e interval; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<std::string> timeZone = std::nullopt; std::optional<int> limit = std::nullopt; };
struct avgPrice_args_t        { std::string jsonify(); std::string symbol; };
struct ticker_args_t          { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; std::optional<ticker_interval_e> windowSize = std::nullopt; std::optional<ticker_type_e> type = std::nullopt; };
struct tickerTradingDay_args_t    { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; std::optional<ticker_type_e> type = std::nullopt; std::optional<std::string> timeZone = std::nullopt; };
struct tickerPrice_args_t    { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; };
struct tickerBook_args_t        { std::string jsonify(); std::variant<std::string, std::vector<std::string>> symbol; };
ENFORCE_ARCHITECTURE_DESIGN(          depth_args_t);
ENFORCE_ARCHITECTURE_DESIGN(   tradesRecent_args_t);
ENFORCE_ARCHITECTURE_DESIGN( tradesHistorical_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       klines_args_t);
ENFORCE_ARCHITECTURE_DESIGN(     avgPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       ticker_args_t);
ENFORCE_ARCHITECTURE_DESIGN( tickerTradingDay_args_t);
ENFORCE_ARCHITECTURE_DESIGN(    tickerPrice_args_t);
ENFORCE_ARCHITECTURE_DESIGN(     tickerBook_args_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*     expected return structures    */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secondary return structs */
struct price_qty_t       { double price; double qty; };

struct tick_full_t       { std::string symbol; double priceChange; double priceChangePercent; double weightedAvgPrice; double prevClosePrice; 
                           double lastPrice; double lastQty; double bidPrice; double bidQty; double askPrice; double askQty; double openPrice; 
                           double highPrice; double lowPrice; double volume; double quoteVolume; long openTime; long closeTime; long firstId; int lastId; int count; };

struct tick_mini_t       { std::string symbol; double lastPrice; double openPrice; double highPrice; double lowPrice; double volume; 
                          double quoteVolume; long openTime; long closeTime; long firstId; int lastId; int count; };


#pragma pack(push, 1) /* ensure binary conversion compatibility and memory efficiency */
struct trade_t         {
  using key_type_t = int64_t;
  /* Methods */
  static constexpr std::size_t key_offset() { return offsetof(trade_t, time); }
  key_type_t key_value();
  static trade_t null_instance(key_type_t key_value = INT64_MIN);
  static trade_t from_binary(const char* data);
  static trade_t from_csv(const std::string& line, char delimiter = ',', size_t line_number = 0); 
  static statistics_pack_t<trade_t> initialize_statistics_pack(unsigned int window_size = 100);
  std::vector<double> tensor_features() const;
  void to_csv(std::ostream& os, char delimiter = ',') const;
  bool is_valid() const;
  /* Values */
  int64_t id; double price; double qty; double quoteQty; int64_t time; bool isBuyerMaker; bool isBestMatch;
};
#pragma pack(pop)


#pragma pack(push, 1) /* ensure binary conversion compatibility and memory efficiency */
struct kline_t         {
  using key_type_t = int64_t;
  /* Methods */
  static constexpr std::size_t key_offset() { return offsetof(kline_t, close_time); }
  key_type_t key_value();
  static kline_t from_binary(const char* data);
  static kline_t null_instance(key_type_t key_value = INT64_MIN);
  static kline_t from_csv(const std::string& line, char delimiter = ',', size_t line_number = 0); 
  static statistics_pack_t<kline_t> initialize_statistics_pack(unsigned int window_size = 100);
  std::vector<double> tensor_features() const; 
  void to_csv(std::ostream& os, char delimiter = ',') const;
  bool is_valid() const;
  /* Values */
  int64_t open_time; double open_price; double high_price; double low_price; double close_price; double volume; int64_t close_time; 
  double quote_asset_volume; int32_t number_of_trades; double taker_buy_base_volume; double taker_buy_quote_volume;
};
#pragma pack(pop)

#pragma pack(push, 1) /* ensure binary conversion compatibility and memory efficiency */
struct basic_t         {
  using key_type_t = double;
  /* Methods */
  static constexpr std::size_t key_offset() { return offsetof(basic_t, time); }
  key_type_t key_value();
  static basic_t from_binary(const char* data);
  static basic_t null_instance(key_type_t key_value = std::numeric_limits<double>::min());
  static basic_t from_csv(const std::string& line, char delimiter = ',', size_t line_number = 0); 
  static statistics_pack_t<basic_t> initialize_statistics_pack(unsigned int window_size = 100);
  std::vector<double> tensor_features() const; 
  void to_csv(std::ostream& os, char delimiter = ',') const;
  bool is_valid() const;
  /* Values */
  double time; double value;
};
#pragma pack(pop)

struct price_t         { std::string symbol; double price; };

struct bookPrice_t     { std::string symbol; double bidPrice; double bidQty; double askPrice; double askQty; };

ENFORCE_ARCHITECTURE_DESIGN(  price_qty_t);
ENFORCE_ARCHITECTURE_DESIGN(  tick_full_t);
ENFORCE_ARCHITECTURE_DESIGN(  tick_mini_t);
ENFORCE_ARCHITECTURE_DESIGN(      trade_t);
ENFORCE_ARCHITECTURE_DESIGN(      kline_t);
ENFORCE_ARCHITECTURE_DESIGN(      price_t);
ENFORCE_ARCHITECTURE_DESIGN(  bookPrice_t);

/* primary return structs */
struct depth_ret_t            { frame_response_t frame_rsp; depth_ret_t             (const std::string& json); long lastUpdateId; std::vector<price_qty_t> bids; std::vector<price_qty_t> asks; };
struct tradesRecent_ret_t     { frame_response_t frame_rsp; tradesRecent_ret_t      (const std::string& json); std::vector<trade_t> trades; };
struct tradesHistorical_ret_t { frame_response_t frame_rsp; tradesHistorical_ret_t  (const std::string& json); std::vector<trade_t> trades; };
struct klines_ret_t           { frame_response_t frame_rsp; klines_ret_t            (const std::string& json); std::vector<kline_t> klines; };
struct avgPrice_ret_t         { frame_response_t frame_rsp; avgPrice_ret_t          (const std::string& json); int mins; double price; long close_time; };
struct ticker_ret_t           { frame_response_t frame_rsp; ticker_ret_t            (const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t, std::vector<tick_full_t>, std::vector<tick_mini_t>> ticks; bool is_full; };
struct tickerTradingDay_ret_t { frame_response_t frame_rsp; tickerTradingDay_ret_t  (const std::string& json); std::variant<std::monostate, tick_full_t, tick_mini_t, std::vector<tick_full_t>, std::vector<tick_mini_t>> ticks; bool is_full; };
struct tickerPrice_ret_t      { frame_response_t frame_rsp; tickerPrice_ret_t       (const std::string& json); std::variant<std::monostate, price_t, std::vector<price_t>> prices; };
struct tickerBook_ret_t       { frame_response_t frame_rsp; tickerBook_ret_t        (const std::string& json); std::variant<std::monostate, bookPrice_t, std::vector<bookPrice_t>> bookPrices; };
ENFORCE_ARCHITECTURE_DESIGN(            depth_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(     tradesRecent_ret_t);
ENFORCE_ARCHITECTURE_DESIGN( tradesHistorical_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(           klines_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(         avgPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(           ticker_ret_t);
ENFORCE_ARCHITECTURE_DESIGN( tickerTradingDay_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(      tickerPrice_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       tickerBook_ret_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*     deserialize specializations     */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(depth_ret_t& deserialized, const std::string &json);
void deserialize(tradesRecent_ret_t& deserialized, const std::string &json);
void deserialize(tradesHistorical_ret_t& deserialized, const std::string &json);
void deserialize(klines_ret_t& deserialized, const std::string &json);
void deserialize(avgPrice_ret_t& deserialized, const std::string &json);
void deserialize(tickerTradingDay_ret_t& deserialized, const std::string &json);
void deserialize(ticker_ret_t& deserialized, const std::string &json);
void deserialize(tickerPrice_ret_t& deserialized, const std::string &json);
void deserialize(tickerBook_ret_t& deserialized, const std::string &json);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
