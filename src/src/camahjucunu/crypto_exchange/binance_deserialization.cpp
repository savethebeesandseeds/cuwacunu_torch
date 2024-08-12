#include "camahjucunu/crypto_exchange/binance_deserialization.h"

RUNTIME_WARNING("[binance_deserialization.cpp]() regex needs to be optimized, it is finiding all matches instead of stoping at the first occurance.\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() validate the json objects on each desearialization\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() deserializations catch to fatal error, this needs revisitation\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() some desearialization are missing the list functionality and some are missing the single item functionallity, this needs revisitation\n");


#define JSON_STRING_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"(.*?)\"")
#define JSON_BOOLEAN_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*(true|false)")
#define JSON_QUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"([-+]?\\d*\\.?\\d+)\"")
#define JSON_UNQUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*([-+]?\\d*\\.?\\d+)")

#define REMOVE_WHITESPACE(str) str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end())
#define REMOVE_QUOTES(str) str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == '"' || c == '\''; }), str.end())

#define CLEAN_OBJECT() do { \
  std::string mutableJson = json; \
  \
  /* Validate the input */ \
  /* ... */ \
  \
  /* Clean json */ \
  REMOVE_WHITESPACE(mutableJson); \
  \
  /* Transform the string into a stream */ \
  iss = std::istringstream(mutableJson); \
  iss.seekg(0); \
} while (0)

#define DESERIALIZE_LIST_OF_OBJECTS(OBJ, OBJ_TYPE, ITEM_TYPE, COLLECTION) do { \
  std::string result; \
  OBJ.COLLECTION.clear(); \
  stream_ignore(iss, '[', 0x1 << 3, #OBJ_TYPE ": unexpected structure"); \
  if(iss.peek() == '{') { \
    do { \
      result = stream_get(iss, ']', #OBJ_TYPE ": (a) " #ITEM_TYPE " structure is wrong"); \
      result += ']'; \
      OBJ.COLLECTION.push_back(ITEM_TYPE(result)); \
      if(iss.peek() != ',') { break; } \
      stream_ignore(iss, ',', 0x1 << 0, #OBJ_TYPE ": (b) " #ITEM_TYPE " structure is wrong"); \
    } while(iss.good()); \
  } \
  if(OBJ.COLLECTION.size() == 0) { \
    log_warn("(" #OBJ_TYPE ")[deserialize] Empty or misunderstood json: %s. \n", json.c_str()); \
  } \
} while (0)

#define DESERIALIZE_LIST_OF_LISTS(OBJ, OBJ_TYPE, ITEM_TYPE, COLLECTION) do { \
  std::string result; \
  OBJ.COLLECTION.clear(); \
  stream_ignore(iss, '[', 0x1 << 3, #OBJ_TYPE ": unexpected structure"); \
  if(iss.peek() == '[') { \
    do { \
      result = stream_get(iss, ']', #OBJ_TYPE ": (a) " #ITEM_TYPE " structure is wrong"); \
      result += ']'; \
      OBJ.COLLECTION.push_back(ITEM_TYPE(result)); \
      if(iss.peek() != ',') { break; } \
      stream_ignore(iss, ',', 0x1 << 0, #OBJ_TYPE ": (b) " #ITEM_TYPE " structure is wrong"); \
    } while(iss.good()); \
  } \
  if(OBJ.COLLECTION.size() == 0) { \
    log_warn("(" #OBJ_TYPE ")[deserialize] Empty or misunderstood json: %s. \n", json.c_str()); \
  } \
} while (0)


namespace cuwacunu {
namespace camahjucunu {
namespace binance {

void log_deserialization_unexpected(const std::string& json, const char* label) {
  /* logging secure as there is no cerainty on the payload received */
  /* breaks to fatal, binance shodun't return an unexpected json */
  if(json != "") {
    log_secure_fatal("[binance_deserialization.cpp](%s) unexpected empty json\n", label);
  } else {
    log_secure_fatal("[binance_deserialization.cpp](%s) unexpected json:\n%s\n", label, json.c_str());
  }
}

void log_deserialization_unfound(const char* key, const char* label, const char* json) {
  /* logging secure as there is no cerainty on the payload received */
  log_secure_fatal("[binance_deserialization.cpp]() key [%s:%s] not found in json: %s\n", label, key, json);
}

/* validateKey_byRegex */
std::string validateKey_byRegex(std::string pattern, const std::string& json, const char* key) {
  std::smatch match;
  std::regex regexPattern(pattern);

  if (std::regex_search(json, match, regexPattern)) {
    if (match.size() == 2) {
      return match[1].str();
    }
  }

  return "";
}


/* Primary template function */
template<typename T>
T regexValue(const std::string& json, const char* key, const char* label);

/* Specialization for std::string */
template<>
std::string regexValue<std::string>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_STRING_PATTERN(key), json, key);
  
  if (match != "") {
    return match;
  }
  log_deserialization_unfound(key, label, json.c_str());

  return "";
}

/* Specialization for long */
template<>
long regexValue<long>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stol(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for int */
template<>
int regexValue<int>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stoi(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for double */
template<>
double regexValue<double>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_QUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stod(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0.0f;
}

/* Specialization for bool */
template<>
bool regexValue<bool>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_BOOLEAN_PATTERN(key), json, key);
  
  if (match != "") {
    return match == "true";
  }
  log_deserialization_unfound(key, label, json.c_str());

  return false;
}

/* stream ignore */
inline void stream_ignore(std::istringstream& iss, const char stop, size_t max_len, const char* label) {
  /* advance the stream to the stop */
  iss.ignore(max_len, stop);
  /* validate the operation was correct */
  if( iss.good() == false ) {
    /* finalize in error */
    log_deserialization_unexpected(iss.str(), label);
  }
}

inline std::string stream_get(std::istringstream& iss, const char stop, const char* label) {
  std::string result;
  /* get a string from the iss from the current position to the next stop */
  std::getline(iss, result, stop);   
  /* validate the operation was correct */
  if( iss.good() == false ) {
    /* finalize in error */
    log_deserialization_unexpected(iss.str(), label);
  }
  
  return result;
}

/* --- --- --- ---  --- --- --- --- */
/* --- secondary return structs --- */
/* --- --- --- ---  --- --- --- --- */
void deserialize(price_qty_t& obj, const std::string& json) {
  /*  price_qty_t json example: 
    [
      "4.00000000",     // PRICE
      "431.00000000"    // QTY
    ]
  */
  std::string result;

  /* transfor the string into a stream */
  std::istringstream iss(json);

  stream_ignore(iss, '[', 0x1 << 0, "price_qty_t: ('[') structure is wrong");

  /* deserialize the price */
  result = stream_get(iss, ',', "price_qty_t: unable to grab price");
  obj.price = std::stod(result);

  /* deserialize the qty */
  result = stream_get(iss, ']', "price_qty_t: unable to grab qty");  
  obj.qty = std::stod(result);
}

void deserialize(trade_t& obj, const std::string& json) {
  /*
    {
      "id": 28457,
      "price": "4.00000100",
      "qty": "12.00000000",
      "quoteQty": "48.000012",
      "time": 1499865549590,
      "isBuyerMaker": true,
      "isBestMatch": true
    }
  */
  obj.id            = regexValue<long>(json, "id", "trade_t");
  obj.price         = regexValue<double>(json, "price", "trade_t");
  obj.qty           = regexValue<double>(json, "qty", "trade_t");
  obj.quoteQty      = regexValue<double>(json, "quoteQty", "trade_t");
  obj.time          = regexValue<long>(json, "time", "trade_t");
  obj.isBuyerMaker  = regexValue<bool>(json, "isBuyerMaker", "trade_t");
  obj.isBestMatch   = regexValue<bool>(json, "isBestMatch", "trade_t");
}

void deserialize(kline_t& obj, const std::string& json) {
  /*
    [
      1499040000000,      // Kline open time
      "0.01634790",       // Open price
      "0.80000000",       // High price
      "0.01575800",       // Low price
      "0.01577100",       // Close price
      "148976.11427815",  // Volume
      1499644799999,      // Kline Close time
      "2434.19055334",    // Quote asset volume
      308,                // Number of trades
      "1756.87402397",    // Taker buy base asset volume
      "28.46694368",      // Taker buy quote asset volume
      "0"                 // Unused field, ignore.
    ]
  */
  std::string result;
  
  /* transfor the string into a stream */
  std::istringstream iss(json);

  stream_ignore(iss, '[', 0x1 << 0, "kline_t: ('[') structure is wrong");

  /* deserialize the open_time */
  result = stream_get(iss, ',', "kline_t: unable to grab open_time");
  obj.open_time = std::stol(result);
  
  /* deserialize the open_price */
  result = stream_get(iss, ',', "kline_t: unable to grab open_price");
  obj.open_price = std::stod(result);

  /* deserialize the high_price */
  result = stream_get(iss, ',', "kline_t: unable to grab high_price");
  obj.high_price = std::stod(result);

  /* deserialize the low_price */
  result = stream_get(iss, ',', "kline_t: unable to grab low_price");
  obj.low_price = std::stod(result);

  /* deserialize the close_price */
  result = stream_get(iss, ',', "kline_t: unable to grab close_price");
  obj.close_price = std::stod(result);

  /* deserialize the volume */
  result = stream_get(iss, ',', "kline_t: unable to grab volume");
  obj.volume = std::stod(result);

  /* deserialize the close_time */
  result = stream_get(iss, ',', "kline_t: unable to grab close_time");
  obj.close_time = std::stol(result);

  /* deserialize the quote_asset_volume */
  result = stream_get(iss, ',', "kline_t: unable to grab quote_asset_volume");
  obj.quote_asset_volume = std::stod(result);

  /* deserialize the number_of_trades */
  result = stream_get(iss, ',', "kline_t: unable to grab number_of_trades");
  obj.number_of_trades = std::stoi(result);

  /* deserialize the taker_buy_base_volume */
  result = stream_get(iss, ',', "kline_t: unable to grab taker_buy_base_volume");
  obj.taker_buy_base_volume = std::stod(result);

  /* deserialize the taker_buy_quote_volume */
  result = stream_get(iss, ',', "kline_t: unable to grab taker_buy_quote_volume");
  obj.taker_buy_quote_volume = std::stod(result);

}

void deserialize(tick_full_t& obj, const std::string& json) {
  /*
    {
      "symbol":             "BTCUSDT",
      "priceChange":        "-83.13000000",         // Absolute price change
      "priceChangePercent": "-0.317",               // Relative price change in percent
      "weightedAvgPrice":   "26234.58803036",       // quoteVolume / volume
      "openPrice":          "26304.80000000",
      "highPrice":          "26397.46000000",
      "lowPrice":           "26088.34000000",
      "lastPrice":          "26221.67000000",
      "volume":             "18495.35066000",       // Volume in base asset
      "quoteVolume":        "485217905.04210480",   // Volume in quote asset
      "openTime":           1695686400000,
      "closeTime":          1695772799999,
      "firstId":            3220151555,             // Trade ID of the first trade in the interval
      "lastId":             3220849281,             // Trade ID of the last trade in the interval
      "count":              697727                  // Number of trades in the interval
    }
  */
  
  obj.symbol              = regexValue<std::string>(json, "symbol", "tick_full_t");
  obj.priceChange         = regexValue<double>(json, "priceChange", "tick_full_t");
  obj.priceChangePercent  = regexValue<double>(json, "priceChangePercent", "tick_full_t");
  obj.weightedAvgPrice    = regexValue<double>(json, "weightedAvgPrice", "tick_full_t");
  obj.openPrice           = regexValue<double>(json, "openPrice", "tick_full_t");
  obj.highPrice           = regexValue<double>(json, "highPrice", "tick_full_t");
  obj.lowPrice            = regexValue<double>(json, "lowPrice", "tick_full_t");
  obj.lastPrice           = regexValue<double>(json, "lastPrice", "tick_full_t");
  obj.volume              = regexValue<double>(json, "volume", "tick_full_t");
  obj.quoteVolume         = regexValue<double>(json, "quoteVolume", "tick_full_t");
  obj.openTime            = regexValue<long>(json, "openTime", "tick_full_t");
  obj.closeTime           = regexValue<long>(json, "closeTime", "tick_full_t");
  obj.firstId             = regexValue<long>(json, "firstId", "tick_full_t");
  obj.lastId              = regexValue<long>(json, "lastId", "tick_full_t");
  obj.count               = regexValue<int>(json, "count", "tick_full_t");
  
}
void deserialize(tick_mini_t& obj, const std::string& json) {
  /*
    {
      "symbol":         "BTCUSDT",
      "openPrice":      "26304.80000000",
      "highPrice":      "26397.46000000",
      "lowPrice":       "26088.34000000",
      "lastPrice":      "26221.67000000",
      "volume":         "18495.35066000",       // Volume in base asset
      "quoteVolume":    "485217905.04210480",   // Volume in quote asset
      "openTime":       1695686400000,
      "closeTime":      1695772799999,
      "firstId":        3220151555,             // Trade ID of the first trade in the interval
      "lastId":         3220849281,             // Trade ID of the last trade in the interval
      "count":          697727                  // Number of trades in the interval
    }
  */

  obj.symbol       = regexValue<std::string>(json, "symbol", "tick_mini_t");
  obj.openPrice    = regexValue<double>(json, "openPrice", "tick_mini_t");
  obj.highPrice    = regexValue<double>(json, "highPrice", "tick_mini_t");
  obj.lowPrice     = regexValue<double>(json, "lowPrice", "tick_mini_t");
  obj.lastPrice    = regexValue<double>(json, "lastPrice", "tick_mini_t");
  obj.volume       = regexValue<double>(json, "volume", "tick_mini_t");
  obj.quoteVolume  = regexValue<double>(json, "quoteVolume", "tick_mini_t");
  obj.openTime     = regexValue<long>(json, "openTime", "tick_mini_t");
  obj.closeTime    = regexValue<long>(json, "closeTime", "tick_mini_t");
  obj.firstId      = regexValue<long>(json, "firstId", "tick_mini_t");
  obj.lastId       = regexValue<long>(json, "lastId", "tick_mini_t");
  obj.count        = regexValue<int>(json, "count", "tick_mini_t");
}

void deserialize(price_t& obj, const std::string& json) {
  /* 
    {
      "symbol": "LTCBTC",
      "price": "4.00000200"
    }
  */
  obj.symbol     = regexValue<std::string>(json, "symbol", "price_t");
  obj.price      = regexValue<double>(json, "price", "price_t");
}

void deserialize(bookPrice_t& obj, const std::string& json) {
  /* 
    {
      "symbol": "LTCBTC",
      "bidPrice": "4.00000000",
      "bidQty": "431.00000000",
      "askPrice": "4.00000200",
      "askQty": "9.00000000"
    }
  */
  obj.symbol    = regexValue<std::string>(json, "symbol", "bookPrice_t");
  obj.bidPrice  = regexValue<double>(json, "bidPrice", "bookPrice_t");
  obj.bidQty    = regexValue<double>(json, "bidQty", "bookPrice_t");
  obj.askPrice  = regexValue<double>(json, "askPrice", "bookPrice_t");
  obj.askQty    = regexValue<double>(json, "askQty", "bookPrice_t");
}

/* --- --- --- ------ --- --- --- */
/* --- primary return structs --- */
/* --- --- --- ------ --- --- --- */
void deserialize(ping_ret_t& obj, const std::string& json) {
  /* ping_ret_t json example:
    {}
  */
  /* validate the input */
  /* ... */
  if(json != "{}") {
    log_deserialization_unexpected(json, "ping_ret_t");
  }
}

void deserialize(time_ret_t& obj, const std::string& json) {
  /* time_ret_t json example: 
    {
      "serverTime": 1499827319559
    }
  */
  /* validate the input */
  /* ... */
  obj.serverTime = regexValue<long>(json, "serverTime", "time_ret_t");
}

void deserialize(depth_ret_t& obj, const std::string& json) {
  /* depth_ret_t json example: 
    {
      "lastUpdateId": 1027024,
      "bids": [
        [
          "4.00000000",     // PRICE
          "431.00000000"    // QTY
        ]
      ],
      "asks": [
        [
          "4.00000200",
          "12.00000000"
        ]
      ]
    }
  */
  std::istringstream iss;

  /* initialize depth_ret_t */
  obj.lastUpdateId = regexValue<long>(json, "lastUpdateId", "depth_ret_t");
  
  /* validate the input */
  /* ... */
  
  {
    /* retrive the bids */
    CLEAN_OBJECT();
    stream_ignore(iss, 'b', std::numeric_limits<std::streamsize>::max(), "depth_ret_t: bids not found");
    DESERIALIZE_LIST_OF_LISTS(obj, depth_ret_t, price_qty_t, bids);
  }

  {
    /* retrive the aks */
    CLEAN_OBJECT();
    stream_ignore(iss, 'k', std::numeric_limits<std::streamsize>::max(), "depth_ret_t: asks not found");
    DESERIALIZE_LIST_OF_LISTS(obj, depth_ret_t, price_qty_t, asks);
  }
}

void deserialize(trades_ret_t& obj, const std::string& json) {
  /*
    [
      {
        "id": 28457,
        "price": "4.00000100",
        "qty": "12.00000000",
        "quoteQty": "48.000012",
        "time": 1499865549590,
        "isBuyerMaker": true,
        "isBestMatch": true
      }
    ]
  */
  std::istringstream iss;
  CLEAN_OBJECT();
  DESERIALIZE_LIST_OF_OBJECTS(obj, trades_ret_t, trade_t, trades);
}

void deserialize(historicalTrades_ret_t& obj, const std::string& json) {
  /*
    [
      {
        "id": 28457,
        "price": "4.00000100",
        "qty": "12.00000000",
        "quoteQty": "48.000012",
        "time": 1499865549590,
        "isBuyerMaker": true,
        "isBestMatch": true
      }
    ]
  */
  std::istringstream iss;
  CLEAN_OBJECT();
  DESERIALIZE_LIST_OF_OBJECTS(obj, historicalTrades_ret_t, trade_t, trades);
}
void deserialize(klines_ret_t& obj, const std::string& json) {
  /* klines_ret_t example:
    [
      [
        1499040000000,      // Kline open time
        "0.01634790",       // Open price
        "0.80000000",       // High price
        "0.01575800",       // Low price
        "0.01577100",       // Close price
        "148976.11427815",  // Volume
        1499644799999,      // Kline Close time
        "2434.19055334",    // Quote asset volume
        308,                // Number of trades
        "1756.87402397",    // Taker buy base asset volume
        "28.46694368",      // Taker buy quote asset volume
        "0"                 // Unused field, ignore.
      ]
    ]
  */
  std::istringstream iss;
  DESERIALIZE_LIST_OF_LISTS(obj, klines_ret_t, kline_t, klines);
}
void deserialize(avgPrice_ret_t& obj, const std::string& json) {
  /* 
    {
      "mins": 5,                    // Average price interval (in minutes)
      "price": "9.35751834",        // Average price
      "closeTime": 1694061154503    // Last trade time
    }
  */
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);

  obj.mins       = regexValue<int>(mutableJson, "mins", "avgPrice_ret_t");
  obj.price      = regexValue<double>(mutableJson, "price", "avgPrice_ret_t");
  obj.close_time = regexValue<long>(mutableJson, "closeTime", "avgPrice_ret_t");
}
void deserialize(ticker_24hr_ret_t& obj, const std::string& json) {
  /* ticker_24hr_ret_t example: 
    {...} <-- can be tick_full_t or tick_mini_t
  */
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);

  /* determine the type of tick */
  obj.is_full = validateKey_byRegex(JSON_QUOTED_NUMBER_PATTERN("weightedAvgPrice"), mutableJson, "weightedAvgPrice") != "";

  if(obj.is_full) {
    /* variant for tick_full_t */
    obj.tick.emplace<tick_full_t>(mutableJson);
  } else {
    /* variant for tick_mini_t */
    obj.tick.emplace<tick_mini_t>(mutableJson);
  }
}

void deserialize(ticker_tradingDay_ret_t& obj, const std::string& json) {
  /* ticker_tradingDay_ret_t example: 
    {...} <-- can be tick_full_t or tick_mini_t
  */
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);

  /* determine the type of tick */
  obj.is_full = validateKey_byRegex(JSON_QUOTED_NUMBER_PATTERN("weightedAvgPrice"), mutableJson, "weightedAvgPrice") != "";

  if(obj.is_full) {
    /* variant for tick_full_t */
    obj.tick.emplace<tick_full_t>(mutableJson);
  } else {
    /* variant for tick_mini_t */
    obj.tick.emplace<tick_mini_t>(mutableJson);
  }
}

void deserialize(ticker_price_ret_t& obj, const std::string& json) {
  /* 
    [
      {
        "symbol": "LTCBTC",
        "price": "4.00000200"
      }
    ]
  */
  std::istringstream iss;
  CLEAN_OBJECT();
  DESERIALIZE_LIST_OF_OBJECTS(obj, ticker_price_ret_t, price_t, prices);
}

void deserialize(ticker_bookTicker_ret_t& obj, const std::string& json) {
  /* 
    [
      {
        "symbol": "LTCBTC",
        "bidPrice": "4.00000000",
        "bidQty": "431.00000000",
        "askPrice": "4.00000200",
        "askQty": "9.00000000"
      },
      {
        "symbol": "ETHBTC",
        "bidPrice": "0.07946700",
        "bidQty": "9.00000000",
        "askPrice": "100000.00000000",
        "askQty": "1000.00000000"
      }
    ]
  */
  std::istringstream iss;
  CLEAN_OBJECT();
  DESERIALIZE_LIST_OF_OBJECTS(obj, ticker_bookTicker_ret_t, bookPrice_t, bookPrices);
}

void deserialize(ticker_wind_ret_t& obj, const std::string& json) {
  /* ticker_wind_ret_t example: 
    {...} <-- can be tick_full_t or tick_mini_t
  */
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);

  /* determine the type of tick */
  obj.is_full = validateKey_byRegex(JSON_QUOTED_NUMBER_PATTERN("weightedAvgPrice"), mutableJson, "weightedAvgPrice") != "";

  if(obj.is_full) {
    /* variant for tick_full_t */
    obj.tick.emplace<tick_full_t>(mutableJson);
  } else {
    /* variant for tick_mini_t */
    obj.tick.emplace<tick_mini_t>(mutableJson);
  }
}

void deserialize(account_information_ret_t& obj, const std::string& json) {
  /*
    {
      "makerCommission": 15,
      "takerCommission": 15,
      "buyerCommission": 0,
      "sellerCommission": 0,
      "commissionRates": {
        "maker": "0.00150000",
        "taker": "0.00150000",
        "buyer": "0.00000000",
        "seller": "0.00000000"
      },
      "canTrade": true,
      "canWithdraw": true,
      "canDeposit": true,
      "brokered": false,
      "requireSelfTradePrevention": false,
      "preventSor": false,
      "updateTime": 123456789,
      "accountType": "SPOT",
      "balances": [
        {
          "asset": "BTC",
          "free": "4723846.89208129",
          "locked": "0.00000000"
        },
        {
          "asset": "LTC",
          "free": "4763368.68006011",
          "locked": "0.00000000"
        }
      ],
      "permissions": [
        "SPOT"
      ],
      "uid": 354937868
    }
  */

  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);

  // obj.mins       = regexValue<int>(mutableJson, "mins", "avgPrice_ret_t");

  // makerCommission
  // takerCommission
  // buyerCommission
  // sellerCommission
  // canTrade
  // canWithdraw
  // canDeposit
  // brokered
  // requireSelfTradePrevention
  // preventSor
  // updateTime
  // accountType
  // uid

  // commissionRates == {}

  // balances == []

  // permissions == []

}

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */