#include "camahjucunu/crypto_exchange/binance/binance_deserialization.h"

RUNTIME_WARNING("[binance_deserialization.cpp]() regex needs to be optimized, it is finiding all matches instead of stoping at the first occurance.\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() missing validations on the json objects for each desearialization\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() repeated json cleaning is unecesary\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() deserializations catch to fatal error, this needs revisitation\n");
RUNTIME_WARNING("[binance_deserialization.cpp]() some desearialization are missing the list functionality and some are missing the single item functionallity, this needs revisitation\n");


#define JSON_STRING_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"(.*?)\"")
#define JSON_BOOLEAN_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*(true|false)")
#define JSON_QUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*\"([-+]?\\d*\\.?\\d+)\"")
#define JSON_UNQUOTED_NUMBER_PATTERN(key) ("\"") + std::string(key) + ("\"\\s*:\\s*([-+]?\\d*\\.?\\d+)")

#define REMOVE_WHITESPACE(str) str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end())
#define REMOVE_QUOTES(str) str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == '\"' || c == '\''; }), str.end())

#define DESERIALIZE_OBJECT(OBJ, OBJ_TYPE, ITEM_TYPE, COLLECTION) do { \
  std::string result; \
  stream_ignore(iss, '{', 0x1 << 4, #OBJ_TYPE " : " #ITEM_TYPE " : not found"); \
  result = stream_get(iss, '}', #OBJ_TYPE " : " #ITEM_TYPE "structure is wrong"); \
  result += '}'; \
  OBJ.COLLECTION = ITEM_TYPE(result); \
} while (0)

#define DESERIALIZE_LIST_OF_OBJECTS(OBJ, OBJ_TYPE, ITEM_TYPE, COLLECTION) do { \
  std::string result; \
  OBJ.COLLECTION.clear(); \
  stream_ignore(iss, '[', 0x1 << 3, #OBJ_TYPE ": unexpected structure"); \
  if(iss.peek() == '{') { \
    do { \
      result = stream_get(iss, '}', #OBJ_TYPE ": (a) " #ITEM_TYPE " structure is wrong"); \
      result += '}'; \
      OBJ.COLLECTION.push_back(ITEM_TYPE(result)); \
      if(iss.peek() != ',') { break; } \
      stream_ignore(iss, ',', 0x1 << 0, #OBJ_TYPE ": (b) " #ITEM_TYPE " structure is wrong"); \
    } while(iss.good()); \
  } \
  if(OBJ.COLLECTION.size() == 0) { \
    log_warn("(" #OBJ_TYPE "<" #ITEM_TYPE ">)[deserialize] Empty or misunderstood json: %s. \n", json.c_str()); \
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
    log_warn("(" #OBJ_TYPE "<" #ITEM_TYPE ">)[deserialize] Empty or misunderstood json: %s. \n", json.c_str()); \
  } \
} while (0)

#define DESERIALIZE_LIST_OF_ENUMS(OBJ, OBJ_TYPE, ITEM_TYPE, COLLECTION) do { \
  std::string result; \
  OBJ.COLLECTION.clear(); \
  /* extract the array */ \
  stream_ignore(iss, '[', 0x1 << 3, #OBJ_TYPE ": unexpected array structure"); \
  result = stream_get(iss, ']', #OBJ_TYPE ": (a) " #ITEM_TYPE " array structure is wrong"); \
  REMOVE_QUOTES(result); \
  /* split the array */ \
  std::vector<std::string> parts = piaabo::split_string(result, ','); \
  /* process the individual enums */ \
  for(std::string pt : parts) { \
    OBJ.COLLECTION.push_back( \
      string_to_enum<ITEM_TYPE>( \
        pt \
      ) \
    ); \
  } \
  if(OBJ.COLLECTION.size() != parts.size()) { \
    log_warn("(" #OBJ_TYPE "<" #ITEM_TYPE ">)[deserialize] misunderstood json array: %s. \n", json.c_str()); \
  } \
  if(OBJ.COLLECTION.size() == 0) { \
    log_warn("(" #OBJ_TYPE "<" #ITEM_TYPE ">)[deserialize] empty json array: %s. \n", result.c_str()); \
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
T retriveKeyValue(const std::string& json, const char* key, const char* label);

/* Specialization for std::string */
template<>
std::string retriveKeyValue<std::string>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_STRING_PATTERN(key), json, key);
  
  if (match != "") {
    return match;
  }
  log_deserialization_unfound(key, label, json.c_str());

  return "";
}

/* Specialization for long */
template<>
long retriveKeyValue<long>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stol(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for int */
template<>
int retriveKeyValue<int>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_UNQUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stoi(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0;
}

/* Specialization for double */
template<>
double retriveKeyValue<double>(const std::string& json, const char* key, const char* label) {
  std::string match = validateKey_byRegex(JSON_QUOTED_NUMBER_PATTERN(key), json, key);
  
  if (match != "") {
    return std::stod(match);
  }
  log_deserialization_unfound(key, label, json.c_str());

  return 0.0f;
}

/* Specialization for bool */
template<>
bool retriveKeyValue<bool>(const std::string& json, const char* key, const char* label) {
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

void stream_ignore(std::istringstream& iss, const std::string& stop, size_t max_len, const char* label) {
  if (stop.empty()) {
    log_deserialization_unexpected("Stop string cannot be empty", label);
    return;
  }

  std::string buffer;
  char currentChar;
  size_t stop_length = stop.length();

  while (buffer.size() < max_len && iss.get(currentChar)) {
    buffer.push_back(currentChar);

    /* Keep only the last 'stop_length' characters in the buffer */
    if (buffer.size() > stop_length) {
      buffer.erase(0, buffer.size() - stop_length);
    }

    /* Check if the end of the buffer matches the stop string */
    if (buffer.size() == stop_length && buffer == stop) {
      return; /* Stop string found */
    }
  }

  /* If we exit the loop without finding the stop string */
  if (buffer.size() < stop_length || buffer != stop) {
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
  obj.id            = retriveKeyValue<long>(json, "id", "trade_t");
  obj.price         = retriveKeyValue<double>(json, "price", "trade_t");
  obj.qty           = retriveKeyValue<double>(json, "qty", "trade_t");
  obj.quoteQty      = retriveKeyValue<double>(json, "quoteQty", "trade_t");
  obj.time          = retriveKeyValue<long>(json, "time", "trade_t");
  obj.isBuyerMaker  = retriveKeyValue<bool>(json, "isBuyerMaker", "trade_t");
  obj.isBestMatch   = retriveKeyValue<bool>(json, "isBestMatch", "trade_t");
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
  
  obj.symbol              = retriveKeyValue<std::string>(json, "symbol", "tick_full_t");
  obj.priceChange         = retriveKeyValue<double>(json, "priceChange", "tick_full_t");
  obj.priceChangePercent  = retriveKeyValue<double>(json, "priceChangePercent", "tick_full_t");
  obj.weightedAvgPrice    = retriveKeyValue<double>(json, "weightedAvgPrice", "tick_full_t");
  obj.openPrice           = retriveKeyValue<double>(json, "openPrice", "tick_full_t");
  obj.highPrice           = retriveKeyValue<double>(json, "highPrice", "tick_full_t");
  obj.lowPrice            = retriveKeyValue<double>(json, "lowPrice", "tick_full_t");
  obj.lastPrice           = retriveKeyValue<double>(json, "lastPrice", "tick_full_t");
  obj.volume              = retriveKeyValue<double>(json, "volume", "tick_full_t");
  obj.quoteVolume         = retriveKeyValue<double>(json, "quoteVolume", "tick_full_t");
  obj.openTime            = retriveKeyValue<long>(json, "openTime", "tick_full_t");
  obj.closeTime           = retriveKeyValue<long>(json, "closeTime", "tick_full_t");
  obj.firstId             = retriveKeyValue<long>(json, "firstId", "tick_full_t");
  obj.lastId              = retriveKeyValue<long>(json, "lastId", "tick_full_t");
  obj.count               = retriveKeyValue<int>(json, "count", "tick_full_t");
  
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

  obj.symbol       = retriveKeyValue<std::string>(json, "symbol", "tick_mini_t");
  obj.openPrice    = retriveKeyValue<double>(json, "openPrice", "tick_mini_t");
  obj.highPrice    = retriveKeyValue<double>(json, "highPrice", "tick_mini_t");
  obj.lowPrice     = retriveKeyValue<double>(json, "lowPrice", "tick_mini_t");
  obj.lastPrice    = retriveKeyValue<double>(json, "lastPrice", "tick_mini_t");
  obj.volume       = retriveKeyValue<double>(json, "volume", "tick_mini_t");
  obj.quoteVolume  = retriveKeyValue<double>(json, "quoteVolume", "tick_mini_t");
  obj.openTime     = retriveKeyValue<long>(json, "openTime", "tick_mini_t");
  obj.closeTime    = retriveKeyValue<long>(json, "closeTime", "tick_mini_t");
  obj.firstId      = retriveKeyValue<long>(json, "firstId", "tick_mini_t");
  obj.lastId       = retriveKeyValue<long>(json, "lastId", "tick_mini_t");
  obj.count        = retriveKeyValue<int>(json, "count", "tick_mini_t");
}

void deserialize(price_t& obj, const std::string& json) {
  /* 
    {
      "symbol": "LTCBTC",
      "price": "4.00000200"
    }
  */
  obj.symbol     = retriveKeyValue<std::string>(json, "symbol", "price_t");
  obj.price      = retriveKeyValue<double>(json, "price", "price_t");
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
  obj.symbol    = retriveKeyValue<std::string>(json, "symbol", "bookPrice_t");
  obj.bidPrice  = retriveKeyValue<double>(json, "bidPrice", "bookPrice_t");
  obj.bidQty    = retriveKeyValue<double>(json, "bidQty", "bookPrice_t");
  obj.askPrice  = retriveKeyValue<double>(json, "askPrice", "bookPrice_t");
  obj.askQty    = retriveKeyValue<double>(json, "askQty", "bookPrice_t");
}

void deserialize(commissionRates_t& obj, const std::string& json) {
  /*
    {
      "maker": "0.00150000",
      "taker": "0.00150000",
      "buyer": "0.00000000",
      "seller": "0.00000000"
    }
  */
  obj.maker = retriveKeyValue<double>(json, "maker", "commissionRates_t");
  obj.taker = retriveKeyValue<double>(json, "taker", "commissionRates_t");
  obj.buyer = retriveKeyValue<double>(json, "buyer", "commissionRates_t");
  obj.seller = retriveKeyValue<double>(json, "seller", "commissionRates_t");
}

void deserialize(balance_t& obj, const std::string& json) {
  /*
    {
      "asset": "BTC",
      "free": "4723846.89208129",
      "locked": "0.00000000"
    }
  */
  obj.asset = retriveKeyValue<std::string>(json, "asset", "balance_t");
  obj.free = retriveKeyValue<double>(json, "free", "balance_t");
  obj.locked = retriveKeyValue<double>(json, "locked", "balance_t");
}

void deserialize(historicTrade_t& obj, const std::string& json) {
  /* historicTrade_t example:
    {
      "symbol": "BNBBTC",
      "id": 28457,
      "orderId": 100234,
      "orderListId": -1,
      "price": "4.00000100",
      "qty": "12.00000000",
      "quoteQty": "48.000012",
      "commission": "10.10000000",
      "commissionAsset": "BNB",
      "time": 1499865549590,
      "isBuyer": true,
      "isMaker": false,
      "isBestMatch": true
    }
  */
  obj.symbol          = retriveKeyValue<std::string>(json, "symbol", "historicTrade_t");
  obj.id              = retriveKeyValue<int>(json, "id", "historicTrade_t");
  obj.orderId         = retriveKeyValue<int>(json, "orderId", "historicTrade_t");
  obj.orderListId     = retriveKeyValue<int>(json, "orderListId", "historicTrade_t");
  obj.price           = retriveKeyValue<double>(json, "price", "historicTrade_t");
  obj.qty             = retriveKeyValue<double>(json, "qty", "historicTrade_t");
  obj.quoteQty        = retriveKeyValue<double>(json, "quoteQty", "historicTrade_t");
  obj.commission      = retriveKeyValue<double>(json, "commission", "historicTrade_t");
  obj.commissionAsset = retriveKeyValue<std::string>(json, "commissionAsset", "historicTrade_t");
  obj.time            = retriveKeyValue<long>(json, "time", "historicTrade_t");
  obj.isBuyer         = retriveKeyValue<bool>(json, "isBuyer", "historicTrade_t");
  obj.isMaker         = retriveKeyValue<bool>(json, "isMaker", "historicTrade_t");
  obj.isBestMatch     = retriveKeyValue<bool>(json, "isBestMatch", "historicTrade_t");
}
void deserialize(comission_discount_t& obj, const std::string& json) {
  /*
    {                               //Discount commission when paying in BNB
      "enabledForAccount": true,
      "enabledForSymbol": true,
      "discountAsset": "BNB",
      "discount": "0.75000000"      //Standard commission is reduced by this rate when paying commission in BNB.
    }
  */
  obj.enabledForAccount = retriveKeyValue<bool>(json, "enabledForAccount", "comission_discount_t");
  obj.enabledForSymbol  = retriveKeyValue<bool>(json, "enabledForSymbol", "comission_discount_t");
  obj.discountAsset     = retriveKeyValue<std::string>(json, "discountAsset", "comission_discount_t");
  obj.discount          = retriveKeyValue<double>(json, "discount", "comission_discount_t");
}
void deserialize(order_ack_resp_t& obj, const std::string& json) {
  /* example order_ack_resp_t
    {
      "symbol": "BTCUSDT",
      "orderId": 28,
      "orderListId": -1, // Unless an order list, value will be -1
      "clientOrderId": "6gCrw2kRUAF9CvJDGP16IP",
      "transactTime": 1507725176595
    }
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);
  
  obj.symbol        = retriveKeyValue<std::string>(json, "symbol", "order_ack_resp_t");
  obj.orderId       = retriveKeyValue<int>(json, "orderId", "order_ack_resp_t");
  obj.orderListId   = retriveKeyValue<int>(json, "orderListId", "order_ack_resp_t");
  obj.clientOrderId = retriveKeyValue<std::string>(json, "clientOrderId", "order_ack_resp_t");
  obj.transactTime  = retriveKeyValue<long>(json, "transactTime", "order_ack_resp_t");
}
void deserialize(order_result_resp_t& obj, const std::string& json) {
  /*
    {
      "symbol": "BTCUSDT",
      "orderId": 28,
      "orderListId": -1, // Unless an order list, value will be -1
      "clientOrderId": "6gCrw2kRUAF9CvJDGP16IP",
      "transactTime": 1507725176595,
      "price": "0.00000000",
      "origQty": "10.00000000",
      "executedQty": "10.00000000",
      "cummulativeQuoteQty": "10.00000000",
      "status": "FILLED",
      "timeInForce": "GTC",
      "type": "MARKET",
      "side": "SELL",
      "workingTime": 1507725176595,
      "selfTradePreventionMode": "NONE"
    }
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* deserialize */
  obj.symbol                  = retriveKeyValue<std::string>(mutableJson, "symbol", "order_result_resp_t");
  obj.orderId                 = retriveKeyValue<int>(mutableJson, "orderId", "order_result_resp_t");
  obj.orderListId             = retriveKeyValue<int>(mutableJson, "orderListId", "order_result_resp_t");
  obj.clientOrderId           = retriveKeyValue<std::string>(mutableJson, "clientOrderId", "order_result_resp_t");
  obj.transactTime            = retriveKeyValue<long>(mutableJson, "transactTime", "order_result_resp_t");
  obj.origQty                 = retriveKeyValue<double>(mutableJson, "origQty", "order_result_resp_t");
  obj.executedQty             = retriveKeyValue<double>(mutableJson, "executedQty", "order_result_resp_t");
  obj.cummulativeQuoteQty     = retriveKeyValue<double>(mutableJson, "cummulativeQuoteQty", "order_result_resp_t");
  obj.workingTime             = retriveKeyValue<long>(mutableJson, "workingTime", "order_result_resp_t");
  obj.status                  = string_to_enum<order_status_e>(retriveKeyValue<std::string>(mutableJson, "status", "order_result_resp_t"));
  obj.timeInForce             = string_to_enum<time_in_force_e>(retriveKeyValue<std::string>(mutableJson, "timeInForce", "order_result_resp_t"));
  obj.type                    = string_to_enum<order_type_e>(retriveKeyValue<std::string>(mutableJson, "type", "order_result_resp_t"));
  obj.side                    = string_to_enum<order_side_e>(retriveKeyValue<std::string>(mutableJson, "side", "order_result_resp_t"));
  obj.selfTradePreventionMode = string_to_enum<stp_modes_e>(retriveKeyValue<std::string>(mutableJson, "selfTradePreventionMode", "order_result_resp_t"));
}
void deserialize(order_fill_t& obj, const std::string& json) {
  /* order_fill_t example: 
    {
      "price": "4000.00000000",
      "qty": "1.00000000",
      "commission": "4.00000000",
      "commissionAsset": "USDT",
      "tradeId": 56
    }
  */
  obj.price           = retriveKeyValue<double>(json, "price", "order_fill_t");
  obj.qty             = retriveKeyValue<double>(json, "qty", "order_fill_t");
  obj.commission      = retriveKeyValue<double>(json, "commission", "order_fill_t");
  obj.commissionAsset = retriveKeyValue<std::string>(json, "commissionAsset", "order_fill_t");
  obj.tradeId         = retriveKeyValue<int>(json, "tradeId", "order_fill_t");
}
void deserialize(order_full_resp_t& obj, const std::string& json) {
  /* order_full_resp_t example: 
    {
      "symbol": "BTCUSDT",
      "orderId": 28,
      "orderListId": -1, // Unless an order list, value will be -1
      "clientOrderId": "6gCrw2kRUAF9CvJDGP16IP",
      "transactTime": 1507725176595,
      "price": "0.00000000",
      "origQty": "10.00000000",
      "executedQty": "10.00000000",
      "cummulativeQuoteQty": "10.00000000",
      "status": "FILLED",
      "timeInForce": "GTC",
      "type": "MARKET",
      "side": "SELL",
      "workingTime": 1507725176595,
      "selfTradePreventionMode": "NONE",
      "fills": [
        {
          "price": "4000.00000000",
          "qty": "1.00000000",
          "commission": "4.00000000",
          "commissionAsset": "USDT",
          "tradeId": 56
        },
        {
          "price": "3999.00000000",
          "qty": "5.00000000",
          "commission": "19.99500000",
          "commissionAsset": "USDT",
          "tradeId": 57
        }
      ]
    }
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);

  /* deserialize order result */
  deserialize(obj.result, mutableJson);

  /* deserialize the list of fills */
  iss.seekg(0);
  stream_ignore(iss, "fills", std::numeric_limits<std::streamsize>::max(), "order_full_resp_t: fills not found");
  DESERIALIZE_LIST_OF_OBJECTS(obj, order_full_resp_t, order_fill_t, fills);
}
void deserialize(order_sor_fill_t& obj, const std::string& json) {
  /* order_sor_fill_t example: 
    {
      "matchType": "ONE_PARTY_TRADE_REPORT",
      "price": "28000.00000000",
      "qty": "0.50000000",
      "commission": "0.00000000",
      "commissionAsset": "BTC",
      "tradeId": -1,
      "allocId": 0
    }
  */
  obj.matchType       = retriveKeyValue<std::string>(json, "matchType", "order_sor_fill_t");
  obj.price           = retriveKeyValue<double>(json, "price", "order_sor_fill_t");
  obj.qty             = retriveKeyValue<double>(json, "qty", "order_sor_fill_t");
  obj.commission      = retriveKeyValue<double>(json, "commission", "order_sor_fill_t");
  obj.commissionAsset = retriveKeyValue<std::string>(json, "commissionAsset", "order_sor_fill_t");
  obj.tradeId         = retriveKeyValue<int>(json, "tradeId", "order_sor_fill_t");
  obj.allocId         = retriveKeyValue<int>(json, "allocId", "order_sor_fill_t");
}
void deserialize(order_sor_full_resp_t& obj, const std::string& json) {
  /* order_sor_full_resp_t example: 
    {
      "symbol": "BTCUSDT",
      "orderId": 2,
      "orderListId": -1,
      "clientOrderId": "sBI1KM6nNtOfj5tccZSKly",
      "transactTime": 1689149087774,
      "price": "31000.00000000",
      "origQty": "0.50000000",
      "executedQty": "0.50000000",
      "cummulativeQuoteQty": "14000.00000000",
      "status": "FILLED",
      "timeInForce": "GTC",
      "type": "LIMIT",
      "side": "BUY",
      "workingTime": 1689149087774,
      "fills": [
        {
          "matchType": "ONE_PARTY_TRADE_REPORT",
          "price": "28000.00000000",
          "qty": "0.50000000",
          "commission": "0.00000000",
          "commissionAsset": "BTC",
          "tradeId": -1,
          "allocId": 0
        },
        {
          "matchType": "ONE_PARTY_TRADE_REPORT",
          "price": "28000.00000000",
          "qty": "0.50000000",
          "commission": "0.00000000",
          "commissionAsset": "BTC",
          "tradeId": -1,
          "allocId": 0
        }
      ],
      "workingFloor": "SOR",              
      "selfTradePreventionMode": "NONE",
      "usedSor": true
    }
  */
  
  /* Validate the input */
  /* ... */
  
  std::string mutableJson = json;
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);

  /* sor variables */
  obj.price        = retriveKeyValue<double>(mutableJson, "price", "order_sor_full_resp_t");
  obj.workingFloor = string_to_enum<allocation_type_e>(retriveKeyValue<std::string>(mutableJson, "workingFloor", "order_sor_full_resp_t"));
  obj.usedSor      = retriveKeyValue<bool>(mutableJson, "usedSor", "order_sor_full_resp_t");
  
  /* deserialize order result */
  deserialize(obj.result, mutableJson);
  
  /* deserialize the list of sor_fills */
  iss.seekg(0);
  stream_ignore(iss, "fills", std::numeric_limits<std::streamsize>::max(), "order_sor_full_resp_t: fills not found");
  DESERIALIZE_LIST_OF_OBJECTS(obj, order_sor_full_resp_t, order_sor_fill_t, fills);
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
  obj.serverTime = retriveKeyValue<long>(json, "serverTime", "time_ret_t");
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
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);

  /* initialize depth_ret_t */
  obj.lastUpdateId = retriveKeyValue<long>(json, "lastUpdateId", "depth_ret_t");
  
  /* validate the input */
  /* ... */
  
  {
    /* retrive the bids */
    stream_ignore(iss, 'b', std::numeric_limits<std::streamsize>::max(), "depth_ret_t: bids not found");
    DESERIALIZE_LIST_OF_LISTS(obj, depth_ret_t, price_qty_t, bids);
  }

  iss.seekg(0);

  {
    /* retrive the aks */
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
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
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
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
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
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);
  REMOVE_QUOTES(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
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

  obj.mins       = retriveKeyValue<int>(mutableJson, "mins", "avgPrice_ret_t");
  obj.price      = retriveKeyValue<double>(mutableJson, "price", "avgPrice_ret_t");
  obj.close_time = retriveKeyValue<long>(mutableJson, "closeTime", "avgPrice_ret_t");
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
  /* ticker_price_ret_t example:
    [
      {
        "symbol": "LTCBTC",
        "price": "4.00000200"
      }
    ]
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
  DESERIALIZE_LIST_OF_OBJECTS(obj, ticker_price_ret_t, price_t, prices);
}

void deserialize(ticker_bookTicker_ret_t& obj, const std::string& json) {
  /* ticker_bookTicker_ret_t example:
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
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
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
  /* account_information_ret_t example:
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
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);
  
  /* deserialize */
  obj.makerCommission            = retriveKeyValue<int>(mutableJson, "makerCommission", "account_information_ret_t");
  obj.takerCommission            = retriveKeyValue<int>(mutableJson, "takerCommission", "account_information_ret_t");
  obj.buyerCommission            = retriveKeyValue<int>(mutableJson, "buyerCommission", "account_information_ret_t");
  obj.sellerCommission           = retriveKeyValue<int>(mutableJson, "sellerCommission", "account_information_ret_t");
  obj.canTrade                   = retriveKeyValue<bool>(mutableJson, "canTrade", "account_information_ret_t");
  obj.canWithdraw                = retriveKeyValue<bool>(mutableJson, "canWithdraw", "account_information_ret_t");
  obj.canDeposit                 = retriveKeyValue<bool>(mutableJson, "canDeposit", "account_information_ret_t");
  obj.brokered                   = retriveKeyValue<bool>(mutableJson, "brokered", "account_information_ret_t");
  obj.requireSelfTradePrevention = retriveKeyValue<bool>(mutableJson, "requireSelfTradePrevention", "account_information_ret_t");
  obj.preventSor                 = retriveKeyValue<bool>(mutableJson, "preventSor", "account_information_ret_t");
  obj.updateTime                 = retriveKeyValue<long>(mutableJson, "updateTime", "account_information_ret_t");
  obj.uid                        = retriveKeyValue<long>(mutableJson, "uid", "account_information_ret_t");

  
  /* deserialize accountType */
  obj.accountType = string_to_enum<account_and_symbols_permissions_e>(
    retriveKeyValue<std::string>(mutableJson, "accountType", "account_information_ret_t")
  );

  /* deserialize commissionRates */
  iss.seekg(0);
  stream_ignore(iss, "commissionRates", std::numeric_limits<std::streamsize>::max(), "depth_ret_t: commissionRates not found");
  DESERIALIZE_OBJECT(obj, account_information_ret_t, commissionRates_t, commissionRates);

  
  /* deserialize balances */
  iss.seekg(0);
  stream_ignore(iss, "balances", std::numeric_limits<std::streamsize>::max(), "depth_ret_t: balances not found");
  DESERIALIZE_LIST_OF_OBJECTS(obj, account_information_ret_t, balance_t, balances);

  
  /* deserialize permissions */
  iss.seekg(0);
  stream_ignore(iss, "permissions", std::numeric_limits<std::streamsize>::max(), "depth_ret_t: permissions not found");
  DESERIALIZE_LIST_OF_ENUMS(obj, account_information_ret_t, account_and_symbols_permissions_e, permissions);
}

void deserialize(account_trade_list_ret_t& obj, const std::string& json) {
  /* account_trade_list_ret_t example:
    [
      {
        "symbol": "BNBBTC",
        "id": 28457,
        "orderId": 100234,
        "orderListId": -1,
        "price": "4.00000100",
        "qty": "12.00000000",
        "quoteQty": "48.000012",
        "commission": "10.10000000",
        "commissionAsset": "BNB",
        "time": 1499865549590,
        "isBuyer": true,
        "isMaker": false,
        "isBestMatch": true
      }
    ]
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);

  /* deserialize */
  DESERIALIZE_LIST_OF_OBJECTS(obj, account_trade_list_ret_t, historicTrade_t, trades);
}

void deserialize(query_commision_rates_ret_t& obj, const std::string& json) {
  /* query_commision_rates_ret_t example:
    {
      "symbol": "BTCUSDT",
      "standardCommission": {         //Commission rates on trades from the order.
        "maker": "0.00000010",
        "taker": "0.00000020",
        "buyer": "0.00000030",
        "seller": "0.00000040" 
      },
      "taxCommission": {              //Tax commission rates for trades from the order.
        "maker": "0.00000112",
        "taker": "0.00000114",
        "buyer": "0.00000118",
        "seller": "0.00000116" 
      },
      "discount": {                   //Discount commission when paying in BNB
        "enabledForAccount": true,
        "enabledForSymbol": true,
        "discountAsset": "BNB",
        "discount": "0.75000000"      //Standard commission is reduced by this rate when paying commission in BNB.
      }
    }
  */
  std::string mutableJson = json;
  
  /* Validate the input */
  /* ... */
  
  /* Clean json */
  REMOVE_WHITESPACE(mutableJson);

  /* Transform the string into a stream */
  std::istringstream iss(mutableJson);

  /* deserrialzie symbol */
  obj.symbol       = retriveKeyValue<std::string>(mutableJson, "symbol", "query_commision_rates_ret_t");

  /* deserialize standardCommission */
  iss.seekg(0);
  stream_ignore(iss, "standardCommission", std::numeric_limits<std::streamsize>::max(), "query_commision_rates_ret_t: standardCommission not found");
  DESERIALIZE_OBJECT(obj, account_information_ret_t, commissionRates_t, standardCommission);

  /* deserialize taxCommission */
  iss.seekg(0);
  stream_ignore(iss, "taxCommission", std::numeric_limits<std::streamsize>::max(), "query_commision_rates_ret_t: taxCommission not found");
  DESERIALIZE_OBJECT(obj, account_information_ret_t, commissionRates_t, taxCommission);

  /* deserialize discount */
  iss.seekg(0);
  stream_ignore(iss, "discount", std::numeric_limits<std::streamsize>::max(), "query_commision_rates_ret_t: discount not found");
  DESERIALIZE_OBJECT(obj, account_information_ret_t, comission_discount_t, discount);
}
} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */