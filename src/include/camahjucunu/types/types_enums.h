/* types_enums.h */
#pragma once
#include <string>
#include <string_view>
#include <type_traits>
#include <optional>
#include "piaabo/dutils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* Generalities */
template <typename T> 
  struct EnumTraits;

template <typename T>
struct is_optional_enum : std::false_type {};

template <typename E>
struct is_optional_enum<std::optional<E>> : std::is_enum<E> {};

template <typename T>
[[nodiscard]] inline std::string enum_to_string(T enumValue) {
  static_assert(std::is_enum_v<T>, "enum_to_string<T> requires T to be an enum type");
  return EnumTraits<T>::toString(enumValue);
}

template <typename T>
[[nodiscard]] inline T string_to_enum(const std::string& str) {
  static_assert(std::is_enum_v<T>, "string_to_enum<T> requires T to be an enum type");
  return EnumTraits<T>::fromString(str);
}

// Convenience overload for callers with std::string_view; preserves your EnumTraits policy.
template <typename T>
[[nodiscard]] inline T string_to_enum(std::string_view sv) {
  static_assert(std::is_enum_v<T>, "string_to_enum<T> requires T to be an enum type");
  return EnumTraits<T>::fromString(std::string(sv));
}
/* interval_type_e */
enum class interval_type_e {
  /* utilities */ utility_constant, utility_sine, utility_triangular, 
  /* seconds */   interval_1s, 
  /* minutes */   interval_1m, interval_3m, interval_5m, interval_15m, interval_30m,
  /* hours */     interval_1h, interval_2h, interval_4h, interval_6h, interval_8h, interval_12h,
  /* days */      interval_1d, interval_3d,
  /* weeks */     interval_1w,
  /* month */     interval_1M
};
template <>
struct EnumTraits<interval_type_e> {
  static std::string toString(interval_type_e value) {
    switch (value) {
      case interval_type_e::utility_constant:   return "constant";
      case interval_type_e::utility_sine:       return "sine";
      case interval_type_e::utility_triangular: return "triangular";
      case interval_type_e::interval_1s:   return "1s";
      case interval_type_e::interval_1m:   return "1m";
      case interval_type_e::interval_3m:   return "3m";
      case interval_type_e::interval_5m:   return "5m";
      case interval_type_e::interval_15m:  return "15m";
      case interval_type_e::interval_30m:  return "30m";
      case interval_type_e::interval_1h:   return "1h";
      case interval_type_e::interval_2h:   return "2h";
      case interval_type_e::interval_4h:   return "4h";
      case interval_type_e::interval_6h:   return "6h";
      case interval_type_e::interval_8h:   return "8h";
      case interval_type_e::interval_12h:  return "12h";
      case interval_type_e::interval_1d:   return "1d";
      case interval_type_e::interval_3d:   return "3d";
      case interval_type_e::interval_1w:   return "1w";
      case interval_type_e::interval_1M:   return "1M";
      default: log_fatal("Unknown interval_type_e requested to convert into str.\n"); return "Unknown interval_type_e";
    }
  }
  static interval_type_e fromString(const std::string& str) {
    if (str == "constant")    return interval_type_e::utility_constant;
    if (str == "sine")        return interval_type_e::utility_sine;
    if (str == "triangular")  return interval_type_e::utility_triangular;
    if (str == "1s")     return interval_type_e::interval_1s;
    if (str == "1m")     return interval_type_e::interval_1m;
    if (str == "3m")     return interval_type_e::interval_3m;
    if (str == "5m")     return interval_type_e::interval_5m;
    if (str == "15m")    return interval_type_e::interval_15m;
    if (str == "30m")    return interval_type_e::interval_30m;
    if (str == "1h")     return interval_type_e::interval_1h;
    if (str == "2h")     return interval_type_e::interval_2h;
    if (str == "4h")     return interval_type_e::interval_4h;
    if (str == "6h")     return interval_type_e::interval_6h;
    if (str == "8h")     return interval_type_e::interval_8h;
    if (str == "12h")    return interval_type_e::interval_12h;
    if (str == "1d")     return interval_type_e::interval_1d;
    if (str == "3d")     return interval_type_e::interval_3d;
    if (str == "1w")     return interval_type_e::interval_1w;
    if (str == "1M")     return interval_type_e::interval_1M;
    log_fatal("Unknown string requested to convert into interval_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for interval_type_e");
  }
};

/* ticker_interval_e */
enum class ticker_interval_e {
  /* minutes */   interval_1m, interval_3m, interval_5m, interval_15m, interval_30m, 
  /* hours */     interval_1h, interval_2h, interval_3h, interval_4h, interval_5h, interval_6h, interval_7h, interval_8h, interval_9h, interval_10h, interval_11h, interval_12h, 
  /* days */      interval_1d, interval_2d, interval_3d, interval_4d, interval_5d, interval_6d, interval_7d
};
template <>
struct EnumTraits<ticker_interval_e> {
  static std::string toString(ticker_interval_e value) {
    switch (value) {
      case ticker_interval_e::interval_1m:    return "1m";
      case ticker_interval_e::interval_3m:    return "3m";
      case ticker_interval_e::interval_5m:    return "5m";
      case ticker_interval_e::interval_15m:   return "15m";
      case ticker_interval_e::interval_30m:   return "30m";
      case ticker_interval_e::interval_1h:    return "1h";
      case ticker_interval_e::interval_2h:    return "2h";
      case ticker_interval_e::interval_3h:    return "3h";
      case ticker_interval_e::interval_4h:    return "4h";
      case ticker_interval_e::interval_5h:    return "5h";
      case ticker_interval_e::interval_6h:    return "6h";
      case ticker_interval_e::interval_7h:    return "7h";
      case ticker_interval_e::interval_8h:    return "8h";
      case ticker_interval_e::interval_9h:    return "9h";
      case ticker_interval_e::interval_10h:   return "10h";
      case ticker_interval_e::interval_11h:   return "11h";
      case ticker_interval_e::interval_12h:   return "12h";
      case ticker_interval_e::interval_1d:    return "1d";
      case ticker_interval_e::interval_2d:    return "2d";
      case ticker_interval_e::interval_3d:    return "3d";
      case ticker_interval_e::interval_4d:    return "4d";
      case ticker_interval_e::interval_5d:    return "5d";
      case ticker_interval_e::interval_6d:    return "6d";
      case ticker_interval_e::interval_7d:    return "7d";
      default: log_fatal("Unknown ticker_interval_e requested to convert into str.\n"); return "Unknown ticker_interval_e";
    }
  }
  static ticker_interval_e fromString(const std::string& str) {
    if (str == "1m")    return ticker_interval_e::interval_1m;
    if (str == "3m")    return ticker_interval_e::interval_3m;
    if (str == "5m")    return ticker_interval_e::interval_5m;
    if (str == "15m")   return ticker_interval_e::interval_15m;
    if (str == "30m")   return ticker_interval_e::interval_30m;
    if (str == "1h")    return ticker_interval_e::interval_1h;
    if (str == "2h")    return ticker_interval_e::interval_2h;
    if (str == "3h")    return ticker_interval_e::interval_3h;
    if (str == "4h")    return ticker_interval_e::interval_4h;
    if (str == "5h")    return ticker_interval_e::interval_5h;
    if (str == "6h")    return ticker_interval_e::interval_6h;
    if (str == "7h")    return ticker_interval_e::interval_7h;
    if (str == "8h")    return ticker_interval_e::interval_8h;
    if (str == "9h")    return ticker_interval_e::interval_9h;
    if (str == "10h")   return ticker_interval_e::interval_10h;
    if (str == "11h")   return ticker_interval_e::interval_11h;
    if (str == "12h")   return ticker_interval_e::interval_12h;
    if (str == "1d")    return ticker_interval_e::interval_1d;
    if (str == "2d")    return ticker_interval_e::interval_2d;
    if (str == "3d")    return ticker_interval_e::interval_3d;
    if (str == "4d")    return ticker_interval_e::interval_4d;
    if (str == "5d")    return ticker_interval_e::interval_5d;
    if (str == "6d")    return ticker_interval_e::interval_6d;
    if (str == "7d")    return ticker_interval_e::interval_7d;
    log_fatal("Unknown string requested to convert into ticker_interval_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for ticker_interval_e");
  }
};

/* ticker_type_e */
enum class ticker_type_e {
  FULL,
  MINI
};
template <>
struct EnumTraits<ticker_type_e> {
  static std::string toString(ticker_type_e value) {
    switch (value) {
      case ticker_type_e::FULL:   return "FULL";
      case ticker_type_e::MINI:   return "MINI";
      default: log_fatal("Unknown ticker_type_e requested to convert into str.\n"); return "Unknown ticker_type_e";
    }
  }
  static ticker_type_e fromString(const std::string& str) {
    if (str == "FULL")    return ticker_type_e::FULL;
    if (str == "MINI")    return ticker_type_e::MINI;
    log_fatal("Unknown string requested to convert into ticker_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for ticker_type_e");
  }
};


/* Symbol status (status) */
enum class symbol_status_e {
  PRE_TRADING,    /* Status before the trading session begins */
  TRADING,        /* Status during the active trading session */
  POST_TRADING,   /* Status after the trading session ends */
  END_OF_DAY,     /* Status indicating the end of the trading day */
  HALT,           /* Status indicating that trading is temporarily halted */
  AUCTION_MATCH,  /* Status during an auction match period */
  BREAK           /* Status indicating a break in trading */
};
template <>
struct EnumTraits<symbol_status_e> {
  static std::string toString(symbol_status_e value) {
    switch (value) {
      case symbol_status_e::PRE_TRADING:    return "PRE_TRADING";
      case symbol_status_e::TRADING:        return "TRADING";
      case symbol_status_e::POST_TRADING:   return "POST_TRADING";
      case symbol_status_e::END_OF_DAY:     return "END_OF_DAY";
      case symbol_status_e::HALT:           return "HALT";
      case symbol_status_e::AUCTION_MATCH:  return "AUCTION_MATCH";
      case symbol_status_e::BREAK:          return "BREAK";
      default: log_fatal("Unknown symbol_status_e requested to convert into str.\n"); return "Unknown symbol_status_e";
    }
  }
  static symbol_status_e fromString(const std::string& str) {
    if (str == "PRE_TRADING")         return symbol_status_e::PRE_TRADING;
    if (str == "TRADING")             return symbol_status_e::TRADING;
    if (str == "POST_TRADING")        return symbol_status_e::POST_TRADING;
    if (str == "END_OF_DAY")          return symbol_status_e::END_OF_DAY;
    if (str == "HALT")                return symbol_status_e::HALT;
    if (str == "AUCTION_MATCH")       return symbol_status_e::AUCTION_MATCH;
    if (str == "BREAK")               return symbol_status_e::BREAK;
    log_fatal("Unknown string requested to convert into symbol_status_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for symbol_status_e");
  }
};

/* Account and Symbol Permissions (permissions) */
enum class account_and_symbols_permissions_e {
  SPOT,
  MARGIN,
  LEVERAGED,
  TRD_GRP_002,
  TRD_GRP_003,
  TRD_GRP_004,
  TRD_GRP_005,
  TRD_GRP_006,
  TRD_GRP_007,
  TRD_GRP_008,
  TRD_GRP_009,
  TRD_GRP_010,
  TRD_GRP_011,
  TRD_GRP_012,
  TRD_GRP_013,
  TRD_GRP_014,
  TRD_GRP_015,
  TRD_GRP_016,
  TRD_GRP_017,
  TRD_GRP_018,
  TRD_GRP_019,
  TRD_GRP_020,
  TRD_GRP_021,
  TRD_GRP_022,
  TRD_GRP_023,
  TRD_GRP_024,
  TRD_GRP_025
};
template <>
struct EnumTraits<account_and_symbols_permissions_e> {
  static std::string toString(account_and_symbols_permissions_e value) {
    switch (value) {
      case account_and_symbols_permissions_e::SPOT:        return "SPOT";
      case account_and_symbols_permissions_e::MARGIN:      return "MARGIN";
      case account_and_symbols_permissions_e::LEVERAGED:   return "LEVERAGED";
      case account_and_symbols_permissions_e::TRD_GRP_002: return "TRD_GRP_002";
      case account_and_symbols_permissions_e::TRD_GRP_003: return "TRD_GRP_003";
      case account_and_symbols_permissions_e::TRD_GRP_004: return "TRD_GRP_004";
      case account_and_symbols_permissions_e::TRD_GRP_005: return "TRD_GRP_005";
      case account_and_symbols_permissions_e::TRD_GRP_006: return "TRD_GRP_006";
      case account_and_symbols_permissions_e::TRD_GRP_007: return "TRD_GRP_007";
      case account_and_symbols_permissions_e::TRD_GRP_008: return "TRD_GRP_008";
      case account_and_symbols_permissions_e::TRD_GRP_009: return "TRD_GRP_009";
      case account_and_symbols_permissions_e::TRD_GRP_010: return "TRD_GRP_010";
      case account_and_symbols_permissions_e::TRD_GRP_011: return "TRD_GRP_011";
      case account_and_symbols_permissions_e::TRD_GRP_012: return "TRD_GRP_012";
      case account_and_symbols_permissions_e::TRD_GRP_013: return "TRD_GRP_013";
      case account_and_symbols_permissions_e::TRD_GRP_014: return "TRD_GRP_014";
      case account_and_symbols_permissions_e::TRD_GRP_015: return "TRD_GRP_015";
      case account_and_symbols_permissions_e::TRD_GRP_016: return "TRD_GRP_016";
      case account_and_symbols_permissions_e::TRD_GRP_017: return "TRD_GRP_017";
      case account_and_symbols_permissions_e::TRD_GRP_018: return "TRD_GRP_018";
      case account_and_symbols_permissions_e::TRD_GRP_019: return "TRD_GRP_019";
      case account_and_symbols_permissions_e::TRD_GRP_020: return "TRD_GRP_020";
      case account_and_symbols_permissions_e::TRD_GRP_021: return "TRD_GRP_021";
      case account_and_symbols_permissions_e::TRD_GRP_022: return "TRD_GRP_022";
      case account_and_symbols_permissions_e::TRD_GRP_023: return "TRD_GRP_023";
      case account_and_symbols_permissions_e::TRD_GRP_024: return "TRD_GRP_024";
      case account_and_symbols_permissions_e::TRD_GRP_025: return "TRD_GRP_025";
      default: log_fatal("Unknown account_and_symbols_permissions_e requested to convert into str.\n"); return "Unknown account_and_symbols_permissions_e";
    }
  }
  static account_and_symbols_permissions_e fromString(const std::string& str) {
    if (str == "SPOT")        return account_and_symbols_permissions_e::SPOT;
    if (str == "MARGIN")      return account_and_symbols_permissions_e::MARGIN;
    if (str == "LEVERAGED")   return account_and_symbols_permissions_e::LEVERAGED;
    if (str == "TRD_GRP_002") return account_and_symbols_permissions_e::TRD_GRP_002;
    if (str == "TRD_GRP_003") return account_and_symbols_permissions_e::TRD_GRP_003;
    if (str == "TRD_GRP_004") return account_and_symbols_permissions_e::TRD_GRP_004;
    if (str == "TRD_GRP_005") return account_and_symbols_permissions_e::TRD_GRP_005;
    if (str == "TRD_GRP_006") return account_and_symbols_permissions_e::TRD_GRP_006;
    if (str == "TRD_GRP_007") return account_and_symbols_permissions_e::TRD_GRP_007;
    if (str == "TRD_GRP_008") return account_and_symbols_permissions_e::TRD_GRP_008;
    if (str == "TRD_GRP_009") return account_and_symbols_permissions_e::TRD_GRP_009;
    if (str == "TRD_GRP_010") return account_and_symbols_permissions_e::TRD_GRP_010;
    if (str == "TRD_GRP_011") return account_and_symbols_permissions_e::TRD_GRP_011;
    if (str == "TRD_GRP_012") return account_and_symbols_permissions_e::TRD_GRP_012;
    if (str == "TRD_GRP_013") return account_and_symbols_permissions_e::TRD_GRP_013;
    if (str == "TRD_GRP_014") return account_and_symbols_permissions_e::TRD_GRP_014;
    if (str == "TRD_GRP_015") return account_and_symbols_permissions_e::TRD_GRP_015;
    if (str == "TRD_GRP_016") return account_and_symbols_permissions_e::TRD_GRP_016;
    if (str == "TRD_GRP_017") return account_and_symbols_permissions_e::TRD_GRP_017;
    if (str == "TRD_GRP_018") return account_and_symbols_permissions_e::TRD_GRP_018;
    if (str == "TRD_GRP_019") return account_and_symbols_permissions_e::TRD_GRP_019;
    if (str == "TRD_GRP_020") return account_and_symbols_permissions_e::TRD_GRP_020;
    if (str == "TRD_GRP_021") return account_and_symbols_permissions_e::TRD_GRP_021;
    if (str == "TRD_GRP_022") return account_and_symbols_permissions_e::TRD_GRP_022;
    if (str == "TRD_GRP_023") return account_and_symbols_permissions_e::TRD_GRP_023;
    if (str == "TRD_GRP_024") return account_and_symbols_permissions_e::TRD_GRP_024;
    if (str == "TRD_GRP_025") return account_and_symbols_permissions_e::TRD_GRP_025;
    log_fatal("Unknown string requested to convert into account_and_symbols_permissions_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for account_and_symbols_permissions_e");
  }
};

/* Order status (status) */
enum class orderStatus_e {
  NEW,                /* The order has been accepted by the engine. */
  PENDING_NEW,        /* The order is in a pending phase until the working order of an order list has been fully filled. */
  PARTIALLY_FILLED,   /* A part of the order has been filled. */
  FILLED,             /* The order has been completed. */
  CANCELED,           /* The order has been canceled by the user. */
  PENDING_CANCEL,     /* Currently unused */
  REJECTED,           /* The order was not accepted by the engine and not processed. */
  EXPIRED,            /* The order was canceled according to the order type's rules (e.g. LIMIT FOK orders with no fill, LIMIT IOC or MARKET orders that partially fill) or by the exchange, (e.g. orders canceled during liquidation, orders canceled during maintenance) */
  EXPIRED_IN_MATCH    /* The order was expired by the exchange due to STP. (e.g. an order with EXPIRE_TAKER will match with existing orders on the book with the same account or same tradeGroupId) */
};
template <>
struct EnumTraits<orderStatus_e> {
  static std::string toString(orderStatus_e value) {
    switch (value) {
      case orderStatus_e::NEW:               return "NEW";
      case orderStatus_e::PENDING_NEW:       return "PENDING_NEW";
      case orderStatus_e::PARTIALLY_FILLED:  return "PARTIALLY_FILLED";
      case orderStatus_e::FILLED:            return "FILLED";
      case orderStatus_e::CANCELED:          return "CANCELED";
      case orderStatus_e::PENDING_CANCEL:    return "PENDING_CANCEL";
      case orderStatus_e::REJECTED:          return "REJECTED";
      case orderStatus_e::EXPIRED:           return "EXPIRED";
      case orderStatus_e::EXPIRED_IN_MATCH:  return "EXPIRED_IN_MATCH";
      default: log_fatal("Unknown orderStatus_e requested to convert into str.\n"); return "Unknown orderStatus_e";
    }
  }
  static orderStatus_e fromString(const std::string& str) {
    if (str == "NEW")                 return orderStatus_e::NEW;
    if (str == "PENDING_NEW")         return orderStatus_e::PENDING_NEW;
    if (str == "PARTIALLY_FILLED")    return orderStatus_e::PARTIALLY_FILLED;
    if (str == "FILLED")              return orderStatus_e::FILLED;
    if (str == "CANCELED")            return orderStatus_e::CANCELED;
    if (str == "PENDING_CANCEL")      return orderStatus_e::PENDING_CANCEL;
    if (str == "REJECTED")            return orderStatus_e::REJECTED;
    if (str == "EXPIRED")             return orderStatus_e::EXPIRED;
    if (str == "EXPIRED_IN_MATCH")    return orderStatus_e::EXPIRED_IN_MATCH;
    log_fatal("Unknown string requested to convert into orderStatus_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for orderStatus_e");
  }
};

/* Order List Status (listStatusType) */
enum class order_list_status_e {
  RESPONSE,	      /* This is used when the ListStatus is responding to a failed action. (E.g. order list placement or cancellation) */
  EXEC_STARTED,	  /* The order list has been placed or there is an update to the order list status. */
  ALL_DONE	      /* The order list has finished executing and thus is no longer active. */
};
template <>
struct EnumTraits<order_list_status_e> {
  static std::string toString(order_list_status_e value) {
    switch (value) {
      case order_list_status_e::RESPONSE:      return "RESPONSE";
      case order_list_status_e::EXEC_STARTED:  return "EXEC_STARTED";
      case order_list_status_e::ALL_DONE:      return "ALL_DONE";
      default: log_fatal("Unknown order_list_status_e requested to convert into str.\n"); return "Unknown order_list_status_e";
    }
  }
  static order_list_status_e fromString(const std::string& str) {
    if (str == "RESPONSE")        return order_list_status_e::RESPONSE;
    if (str == "EXEC_STARTED")    return order_list_status_e::EXEC_STARTED;
    if (str == "ALL_DONE")        return order_list_status_e::ALL_DONE;
    log_fatal("Unknown string requested to convert into order_list_status_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for order_list_status_e");
  }
};

/* Order List Order Status (listOrderStatus) */
enum class order_list_orderStatus_e {
  EXECUTING,    /* Either an order list has been placed or there is an update to the status of the list. */
  ALL_DONE,     /* An order list has completed execution and thus no longer active. */
  REJECT        /* The List Status is responding to a failed action either during order placement or order canceled. */
};
template <>
struct EnumTraits<order_list_orderStatus_e> {
  static std::string toString(order_list_orderStatus_e value) {
    switch (value) {
      case order_list_orderStatus_e::EXECUTING:  return "EXECUTING";
      case order_list_orderStatus_e::ALL_DONE:   return "ALL_DONE";
      case order_list_orderStatus_e::REJECT:     return "REJECT";
      default: log_fatal("Unknown order_list_orderStatus_e requested to convert into str.\n"); return "Unknown order_list_orderStatus_e";
    }
  }
  static order_list_orderStatus_e fromString(const std::string& str) {
    if (str == "EXECUTING")   return order_list_orderStatus_e::EXECUTING;
    if (str == "ALL_DONE")    return order_list_orderStatus_e::ALL_DONE;
    if (str == "REJECT")      return order_list_orderStatus_e::REJECT;
    log_fatal("Unknown string requested to convert into order_list_orderStatus_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for order_list_orderStatus_e");
  }
};

/* ContingencyType */
enum class contingency_type_e {
  OCO, /* One Cancels the Other: When one order is executed, the other is automatically canceled. */
  OTO  /* One Triggers the Other: When the primary order is executed, it triggers the placement of the secondary order. */
};
template <>
struct EnumTraits<contingency_type_e> {
  static std::string toString(contingency_type_e value) {
    switch (value) {
      case contingency_type_e::OCO: return "OCO";
      case contingency_type_e::OTO: return "OTO";
      default: log_fatal("Unknown contingency_type_e requested to convert into str.\n"); return "Unknown contingency_type_e";
    }
  }
  static contingency_type_e fromString(const std::string& str) {
    if (str == "OCO") return contingency_type_e::OCO;
    if (str == "OTO") return contingency_type_e::OTO;
    log_fatal("Unknown string requested to convert into contingency_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for contingency_type_e");
  }
};

/* AllocationType */
enum class allocation_type_e {
  SOR
};
template <>
struct EnumTraits<allocation_type_e> {
  static std::string toString(allocation_type_e value) {
    switch (value) {
      case allocation_type_e::SOR:  return "SOR";
      default: log_fatal("Unknown allocation_type_e requested to convert into str.\n"); return "Unknown allocation_type_e";
    }
  }
  static allocation_type_e fromString(const std::string& str) {
    if (str == "SOR")   return allocation_type_e::SOR;
    log_fatal("Unknown string requested to convert into allocation_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for allocation_type_e");
  }
};

/* Order types (orderTypes, type) */
enum class order_type_e {
  LIMIT,
  MARKET,
  STOP_LOSS,
  STOP_LOSS_LIMIT,
  TAKE_PROFIT,
  TAKE_PROFIT_LIMIT,
  LIMIT_MAKER
};
template <>
struct EnumTraits<order_type_e> {
  static std::string toString(order_type_e value) {
    switch (value) {
      case order_type_e::LIMIT:               return "LIMIT";
      case order_type_e::MARKET:              return "MARKET";
      case order_type_e::STOP_LOSS:           return "STOP_LOSS";
      case order_type_e::STOP_LOSS_LIMIT:     return "STOP_LOSS_LIMIT";
      case order_type_e::TAKE_PROFIT:         return "TAKE_PROFIT";
      case order_type_e::TAKE_PROFIT_LIMIT:   return "TAKE_PROFIT_LIMIT";
      case order_type_e::LIMIT_MAKER:         return "LIMIT_MAKER";
      default: log_fatal("Unknown order_type_e requested to convert into str.\n"); return "Unknown order_type_e";
    }
  }
  static order_type_e fromString(const std::string& str) {
    if (str == "LIMIT")               return order_type_e::LIMIT;
    if (str == "MARKET")              return order_type_e::MARKET;
    if (str == "STOP_LOSS")           return order_type_e::STOP_LOSS;
    if (str == "STOP_LOSS_LIMIT")     return order_type_e::STOP_LOSS_LIMIT;
    if (str == "TAKE_PROFIT")         return order_type_e::TAKE_PROFIT;
    if (str == "TAKE_PROFIT_LIMIT")   return order_type_e::TAKE_PROFIT_LIMIT;
    if (str == "LIMIT_MAKER")         return order_type_e::LIMIT_MAKER;
    log_fatal("Unknown string requested to convert into order_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for order_type_e");
  }
};

/* Order Response Type (newOrderRespType) */
enum class order_response_type_e {
  ACK,
  RESULT,
  FULL
};
template <>
struct EnumTraits<order_response_type_e> {
  static std::string toString(order_response_type_e value) {
    switch (value) {
      case order_response_type_e::ACK:     return "ACK";
      case order_response_type_e::RESULT:  return "RESULT";
      case order_response_type_e::FULL:    return "FULL";
      default: log_fatal("Unknown order_response_type_e requested to convert into str.\n"); return "Unknown order_response_type_e";
    }
  }
  static order_response_type_e fromString(const std::string& str) {
    if (str == "ACK")     return order_response_type_e::ACK;
    if (str == "RESULT")  return order_response_type_e::RESULT;
    if (str == "FULL")    return order_response_type_e::FULL;
    log_fatal("Unknown string requested to convert into order_response_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for order_response_type_e");
  }
};

/* Working Floor */
enum class working_floor_e {
  EXCHANGE,
  SOR
};
template <>
struct EnumTraits<working_floor_e> {
  static std::string toString(working_floor_e value) {
    switch (value) {
      case working_floor_e::EXCHANGE: return "EXCHANGE";
      case working_floor_e::SOR:      return "SOR";
      default: log_fatal("Unknown working_floor_e requested to convert into str.\n"); return "Unknown working_floor_e";
    }
  }
  static working_floor_e fromString(const std::string& str) {
    if (str == "EXCHANGE")  return working_floor_e::EXCHANGE;
    if (str == "SOR")       return working_floor_e::SOR;
    log_fatal("Unknown string requested to convert into working_floor_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for working_floor_e");
  }
};

/* Order side (side) */
enum class order_side_e {
  BUY,
  SELL
};
template <>
struct EnumTraits<order_side_e> {
  static std::string toString(order_side_e value) {
    switch (value) {
      case order_side_e::BUY:   return "BUY";
      case order_side_e::SELL:  return "SELL";
      default: log_fatal("Unknown order_side_e requested to convert into str.\n"); return "Unknown order_side_e";
    }
  }
  static order_side_e fromString(const std::string& str) {
    if (str == "BUY")   return order_side_e::BUY;
    if (str == "SELL")  return order_side_e::SELL;
    log_fatal("Unknown string requested to convert into order_side_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for order_side_e");
  }
};

/* Time in force (timeInForce): This sets how long an order will be active before expiration. */
enum class time_in_force_e {
  GTC, /* Good Til Canceled. An order will be on the book unless the order is canceled. */
  IOC, /* Immediate Or Cancel. An order will try to fill the order as much as it can before the order expires. */
  FOK  /* Fill or Kill. An order will expire if the full order cannot be filled upon execution. */
};
template <>
struct EnumTraits<time_in_force_e> {
  static std::string toString(time_in_force_e value) {
    switch (value) {
      case time_in_force_e::GTC: return "GTC";
      case time_in_force_e::IOC: return "IOC";
      case time_in_force_e::FOK: return "FOK";
      default: log_fatal("Unknown time_in_force_e requested to convert into str.\n"); return "Unknown time_in_force_e";
    }
  }
  static time_in_force_e fromString(const std::string& str) {
    if (str == "GTC")   return time_in_force_e::GTC;
    if (str == "IOC")   return time_in_force_e::IOC;
    if (str == "FOK")   return time_in_force_e::FOK;
    log_fatal("Unknown string requested to convert into time_in_force_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for time_in_force_e");
  }
};

/* Rate limiters (rateLimitType) */
enum class rate_limiters_e {
  REQUEST_WEIGHT,   /* { "rateLimitType": "REQUEST_WEIGHT", "interval": "MINUTE", "intervalNum": 1, "limit": 6000 } */
  ORDERS,           /* { "rateLimitType": "ORDERS", "interval": "SECOND", "intervalNum": 1, "limit": 10 } */
  RAW_REQUESTS      /* { "rateLimitType": "RAW_REQUESTS", "interval": "MINUTE", "intervalNum": 5, "limit": 61000 } */
};
template <>
struct EnumTraits<rate_limiters_e> {
  static std::string toString(rate_limiters_e value) {
    switch (value) {
      case rate_limiters_e::REQUEST_WEIGHT: return "REQUEST_WEIGHT";
      case rate_limiters_e::ORDERS:         return "ORDERS";
      case rate_limiters_e::RAW_REQUESTS:   return "RAW_REQUESTS";
      default: log_fatal("Unknown rate_limiters_e requested to convert into str.\n"); return "Unknown rate_limiters_e";
    }
  }
  static rate_limiters_e fromString(const std::string& str) {
    if (str == "REQUEST_WEIGHT")    return rate_limiters_e::REQUEST_WEIGHT;
    if (str == "ORDERS")            return rate_limiters_e::ORDERS;
    if (str == "RAW_REQUESTS")      return rate_limiters_e::RAW_REQUESTS;
    log_fatal("Unknown string requested to convert into rate_limiters_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for rate_limiters_e");
  }
};

/* Rate limit intervals (interval) */
enum class rate_limit_intervals_e {
  SECOND,
  MINUTE,
  DAY
};
template <>
struct EnumTraits<rate_limit_intervals_e> {
  static std::string toString(rate_limit_intervals_e value) {
    switch (value) {
      case rate_limit_intervals_e::SECOND:  return "SECOND";
      case rate_limit_intervals_e::MINUTE:  return "MINUTE";
      case rate_limit_intervals_e::DAY:     return "DAY";
      default: log_fatal("Unknown rate_limit_intervals_e requested to convert into str.\n"); return "Unknown rate_limit_intervals_e";
    }
  }
  static rate_limit_intervals_e fromString(const std::string& str) {
    if (str == "SECOND") return rate_limit_intervals_e::SECOND;
    if (str == "MINUTE") return rate_limit_intervals_e::MINUTE;
    if (str == "DAY") return rate_limit_intervals_e::DAY;
    log_fatal("Unknown string requested to convert into rate_limit_intervals_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for rate_limit_intervals_e");
  }
};

/* STP Modes */
enum class stp_modes_e {
  NONE,
  EXPIRE_MAKER,
  EXPIRE_TAKER,
  EXPIRE_BOTH,
  Previous
};
template <>
struct EnumTraits<stp_modes_e> {
  static std::string toString(stp_modes_e value) {
    switch (value) {
      case stp_modes_e::NONE:           return "NONE";
      case stp_modes_e::EXPIRE_MAKER:   return "EXPIRE_MAKER";
      case stp_modes_e::EXPIRE_TAKER:   return "EXPIRE_TAKER";
      case stp_modes_e::EXPIRE_BOTH:    return "EXPIRE_BOTH";
      case stp_modes_e::Previous:       return "Previous";
      default: log_fatal("Unknown stp_modes_e requested to convert into str.\n"); return "Unknown stp_modes_e";
    }
  }
  static stp_modes_e fromString(const std::string& str) {
    if (str == "NONE")          return stp_modes_e::NONE;
    if (str == "EXPIRE_MAKER")  return stp_modes_e::EXPIRE_MAKER;
    if (str == "EXPIRE_TAKER")  return stp_modes_e::EXPIRE_TAKER;
    if (str == "EXPIRE_BOTH")   return stp_modes_e::EXPIRE_BOTH;
    if (str == "Previous")      return stp_modes_e::Previous;
    log_fatal("Unknown string requested to convert into stp_modes_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for stp_modes_e");
  }
};

/* Enpoint security types */
enum class endpoint_security_type_e {
  NONE,	      /* Endpoint can be accessed freely.*/
  TRADE,	    /* Endpoint requires sending a valid API-Key and signature.*/
  USER_DATA,	/* Endpoint requires sending a valid API-Key and signature.*/
  USER_STREAM	/* Endpoint requires sending a valid API-Key.*/
};
template <>
struct EnumTraits<endpoint_security_type_e> {
  static std::string toString(endpoint_security_type_e value) {
    switch (value) {
      case endpoint_security_type_e::NONE:        return "NONE";
      case endpoint_security_type_e::TRADE:       return "TRADE";
      case endpoint_security_type_e::USER_DATA:   return "USER_DATA";
      case endpoint_security_type_e::USER_STREAM: return "USER_STREAM";
      default: log_fatal("Unknown endpoint_security_type_e requested to convert into str.\n"); return "Unknown endpoint_security_type_e";
    }
  }
  static endpoint_security_type_e fromString(const std::string& str) {
    if (str == "NONE")        return endpoint_security_type_e::NONE;
    if (str == "TRADE")       return endpoint_security_type_e::TRADE;
    if (str == "USER_DATA")   return endpoint_security_type_e::USER_DATA;
    if (str == "USER_STREAM") return endpoint_security_type_e::USER_STREAM;
    log_fatal("Unknown string requested to convert into endpoint_security_type_e: %s\n", str.c_str());
    // throw std::invalid_argument("Unknown string for endpoint_security_type_e");
  }
};

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */