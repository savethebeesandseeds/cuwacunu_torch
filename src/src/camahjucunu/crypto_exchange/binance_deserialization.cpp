#include "camahjucunu/crypto_exchange/binance_deserialization.h"

RUNTIME_WARNING("[binance_deserialization.cpp]() regex needs to be optimized, it is finiding all matches instead of stoping at the first occurance.\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() validate the json objects on each desearialization\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() deserializations catch to fatal error, this needs revisitation\n");


#define JSON_STRING_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"(.*?)\"")
#define JSON_BOOLEAN_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*(true|false)")
#define JSON_QUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"([-+]?\\d*\\.?\\d+)\"")
#define JSON_UNQUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*([-+]?\\d*\\.?\\d+)")

#define REMOVE_WHITESPACE(str) str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end())
#define REMOVE_QUOTES(str) str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == '"' || c == '\''; }), str.end())

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

/* regexValidate */
std::string regexValidate(std::string pattern, const std::string& json, const char* key) {
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
  std::string match = regexValidate(JSON_STRING_PATTERN(key), json, key);
  
  if (match != "") {
    return match;
  }
  log_deserialization_unfound(key, label, json.c_str());

  return "";
}

/* Specialization for long */
template<>
long regexValue<long>(const std::string& json, const char* key, const char* label) {
  std::string match = regexValidate(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stol(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for int */
template<>
int regexValue<int>(const std::string& json, const char* key, const char* label) {
  std::string match = regexValidate(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stoi(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for double */
template<>
double regexValue<double>(const std::string& json, const char* key, const char* label) {
  std::string match = regexValidate(JSON_QUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stod(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0.0f;
}

/* Specialization for bool */
template<>
bool regexValue<bool>(const std::string& json, const char* key, const char* label) {
  std::string match = regexValidate(JSON_BOOLEAN_PATTERN(key), json, key);
  
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
  std::string result;
  std::string mutableJson = json;

  /* initialize depth_ret_t */
  obj.lastUpdateId = regexValue<long>(json, "lastUpdateId", "depth_ret_t");
  obj.bids.clear();
  obj.asks.clear();
  
  /* validate the input */
  /* ... */

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* transfor the string into a stream */
  std::istringstream iss(mutableJson);

  
  /* advance to the bids (b) */
  stream_ignore(iss, 'b', std::numeric_limits<std::streamsize>::max(), "depth_ret_t: bids not found");
  stream_ignore(iss, '[', 0x1 << 3, "depth_ret_t: (a) bids structure is wrong");
  
  /* validate if bids is not empty */
  if(iss.peek() == '[') {
    do {
      /* retrive the price_qty_t part */
      result = stream_get(iss, ']', "depth_ret_t: (b) bids structure is wrong");
      result += ']';

      /* deserialize next price_qty_t */
      obj.bids.push_back(price_qty_t(result));

      /* try to get the next */
      if(iss.peek() != ',') { break; }

      /* advance to the next price_qty_t in the bids list */
      stream_ignore(iss, ',', 0x1 << 0, "depth_ret_t: (c) bids structure is wrong");

    } while(iss.good());
  }

  /* Reset stream to initial state for the next ignore */
  iss.seekg(0);

  /* advance to the asks (k) */
  stream_ignore(iss, 'k', std::numeric_limits<std::streamsize>::max(), "depth_ret_t: asks not found");
  
  stream_ignore(iss, '[', 0x1 << 3, "depth_ret_t: (a) asks structure is wrong");
  
  /* validate if asks is not empty */
  if(iss.peek() == '[') {
    do {
      /* retrive the price_qty_t part */
      result = stream_get(iss, ']', "depth_ret_t: (b) asks structure is wrong");
      result += ']';

      /* deserialize next price_qty_t */
      obj.asks.push_back(price_qty_t(result));

      /* try to get the next */
      if(iss.peek() != ',') { break; }

      /* advance to the next price_qty_t in the asks list */
      stream_ignore(iss, ',', 0x1 << 0, "depth_ret_t: (c) asks structure is wrong");

    } while(iss.good());
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
  std::string result;
  std::string mutableJson = json;

  /* validate the input */
  /* ... */

  /* initialize the obj */
  obj.trades.clear();

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* transfor the string into a stream */
  std::istringstream iss(mutableJson);

  /* loop over the */
  stream_ignore(iss, '[', 0x1 << 3, "trades_ret_t: unexpected structure");
  
  /* validate if trades_ret_t is not empty */
  if(iss.peek() == '{') {
    do {
      /* retrive the price_qty_t part */
      result = stream_get(iss, '}', "trades_ret_t: (a) trade_t structure is wrong");
      result += '}';

      /* deserialize next price_qty_t */
      obj.trades.push_back(trade_t(result));

      /* try to get the next */
      if(iss.peek() != ',') { break; }

      /* advance to the next price_qty_t in the asks list */
      stream_ignore(iss, ',', 0x1 << 0, "trades_ret_t: (b) trade_t structure is wrong");

    } while(iss.good());
  }
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
  std::string result;
  std::string mutableJson = json;

  /* validate the input */
  /* ... */

  /* initialize the obj */
  obj.trades.clear();

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* transfor the string into a stream */
  std::istringstream iss(mutableJson);

  /* loop over the */
  stream_ignore(iss, '[', 0x1 << 3, "trades_ret_t: unexpected structure");
  
  /* validate if trades_ret_t is not empty */
  if(iss.peek() == '{') {
    do {
      /* retrive the price_qty_t part */
      result = stream_get(iss, '}', "trades_ret_t: (a) trade_t structure is wrong");
      result += '}';

      /* deserialize next price_qty_t */
      obj.trades.push_back(trade_t(result));

      /* try to get the next */
      if(iss.peek() != ',') { break; }

      /* advance to the next price_qty_t in the asks list */
      stream_ignore(iss, ',', 0x1 << 0, "trades_ret_t: (b) trade_t structure is wrong");

    } while(iss.good());
  }
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
  std::string result;
  std::string mutableJson = json;

  /* validate the input */
  /* ... */

  /* initialize the obj */
  obj.klines.clear();

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* transfor the string into a stream */
  std::istringstream iss(mutableJson);

  /* loop over the */
  stream_ignore(iss, '[', 0x1 << 3, "klines_ret_t: unexpected structure");
  
  /* validate if klines_ret_t is not empty */
  if(iss.peek() == '[') {
    do {
      /* retrive the price_qty_t part */
      result = stream_get(iss, ']', "klines_ret_t: (a) kline_t structure is wrong");
      result += ']';

      /* deserialize next price_qty_t */
      obj.klines.push_back(kline_t(result));

      /* try to get the next */
      if(iss.peek() != ',') { break; }

      /* advance to the next price_qty_t in the asks list */
      stream_ignore(iss, ',', 0x1 << 0, "klines_ret_t: (b) kline_t structure is wrong");

    } while(iss.good());
  }
}
void deserialize(avgPrice_ret_t& obj, const std::string& json) {
  /* 
    {
      "mins": 5,                    // Average price interval (in minutes)
      "price": "9.35751834",        // Average price
      "closeTime": 1694061154503    // Last trade time
    }
  */
  std::string result;
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  obj.mins       = regexValue<int>(mutableJson, "mins", "avgPrice_ret_t");
  obj.price      = regexValue<double>(mutableJson, "price", "avgPrice_ret_t");
  obj.close_time = regexValue<long>(mutableJson, "closeTime", "avgPrice_ret_t");
}
void deserialize(ticker_24hr_ret_t& obj, const std::string& json) {
  /* ticker_24hr_ret_t example: 
    {...} <-- can be mini or full
  */
  std::string result;
  std::string mutableJson = json;

  /* claen json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  if(regexValidate(JSON_QUOTED_NUMBER_PATTERN("weightedAvgPrice"), mutableJson, "weightedAvgPrice") != "") {
    /* variant for tick_full_t */
    obj.tick.emplace<tick_full_t>(mutableJson);
  } else {
    /* variant for tick_mini_t */
    obj.tick.emplace<tick_mini_t>(mutableJson);
  }
}

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */