#pragma once
#include <vector>
#include <variant>
#include <sstream>
#include <optional>
#include <regex>
#include <cctype>  // For std::isspace
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/exchange/binance/binance_enums.h"
#include "camahjucunu/exchange/binance/binance_types.h"



/*  
    This module is made under the asumption that Binance will not 
    deliver unexpected responces. 
    
    In most unexpected cases, deserialzie methods will rise a fatal error.
*/



namespace cuwacunu {
namespace camahjucunu {
namespace binance {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*        string to struct functions           */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secundary return structs */

/* specialization for deserialization of price_qty_t */                                     /* ---- */
void deserialize(price_qty_t& obj, const std::string &json);                                /* done */
/* specialization for deserialization of tick_full_t */                                     /* ---- */
void deserialize(tick_full_t& obj, const std::string &json);                                /* done */
/* specialization for deserialization of tick_mini_t */                                     /* ---- */
void deserialize(tick_mini_t& obj, const std::string &json);                                /* done */
/* specialization for deserialization of trade_t */                                         /* ---- */
void deserialize(trade_t& obj, const std::string &json);                                    /* done */
/* specialization for deserialization of kline_t */                                         /* ---- */
void deserialize(kline_t& obj, const std::string &json);                                    /* done */
/* specialization for deserialization of price_t */                                         /* ---- */
void deserialize(price_t& obj, const std::string &json);                                    /* done */
/* specialization for deserialization of bookPrice_t */                                     /* ---- */
void deserialize(bookPrice_t& obj, const std::string &json);                                /* done */
/* specialization for deserialization of order_ack_resp_t */                                /* ---- */
void deserialize(order_ack_resp_t& obj, const std::string &json);                           /* done */
/* specialization for deserialization of order_result_resp_t */                             /* ---- */
void deserialize(order_result_resp_t& obj, const std::string &json);                        /* done */
/* specialization for deserialization of order_fill_t */                                    /* ---- */
void deserialize(order_fill_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of order_full_resp_t */                               /* ---- */
void deserialize(order_full_resp_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of order_sor_fill_t */                                /* ---- */
void deserialize(order_sor_fill_t& obj, const std::string &json);                           /* done */
/* specialization for deserialization of order_sor_full_resp_t */                           /* ---- */
void deserialize(order_sor_full_resp_t& obj, const std::string &json);                      /* done */
/* specialization for deserialization of commissionRates_t */                               /* ---- */
void deserialize(commissionRates_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of comission_discount_t */                            /* ---- */
void deserialize(comission_discount_t& obj, const std::string &json);                       /* done */
/* specialization for deserialization of balance_t */                                       /* ---- */
void deserialize(balance_t& obj, const std::string &json);                                  /* done */
/* specialization for deserialization of historicTrade_t */                                 /* ---- */
void deserialize(historicTrade_t& obj, const std::string &json);                            /* done */

/* primary return structs*/

/* specialization for deserialization of ping_ret_t */                                      /* ---- */
void deserialize(ping_ret_t& obj, const std::string &json);                                 /* done */
/* specialization for deserialization of time_ret_t */                                      /* ---- */
void deserialize(time_ret_t& obj, const std::string &json);                                 /* done */
/* specialization for deserialization of depth_ret_t */                                     /* ---- */
void deserialize(depth_ret_t& obj, const std::string &json);                                /* done */
/* specialization for deserialization of trades_ret_t */                                    /* ---- */
void deserialize(trades_ret_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of historicalTrades_ret_t */                          /* ---- */
void deserialize(historicalTrades_ret_t& obj, const std::string &json);                     /* done */
/* specialization for deserialization of klines_ret_t */                                    /* ---- */
void deserialize(klines_ret_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of avgPrice_ret_t */                                  /* ---- */
void deserialize(avgPrice_ret_t& obj, const std::string &json);                             /* done */
/* specialization for deserialization of ticker_24hr_ret_t */                               /* ---- */
void deserialize(ticker_24hr_ret_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of ticker_tradingDay_ret_t */                         /* ---- */
void deserialize(ticker_tradingDay_ret_t& obj, const std::string &json);                    /* done */
/* specialization for deserialization of ticker_price_ret_t */                              /* ---- */
void deserialize(ticker_price_ret_t& obj, const std::string &json);                         /* done */
/* specialization for deserialization of ticker_bookTicker_ret_t */                         /* ---- */
void deserialize(ticker_bookTicker_ret_t& obj, const std::string &json);                    /* done */
/* specialization for deserialization of ticker_wind_ret_t */                               /* ---- */
void deserialize(ticker_wind_ret_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of account_information_ret_t */                       /* ---- */
void deserialize(account_information_ret_t& obj, const std::string &json);                  /* done */
/* specialization for deserialization of account_trade_list_ret_t */                        /* ---- */
void deserialize(account_trade_list_ret_t& obj, const std::string &json);                   /* done */
/* specialization for deserialization of query_commision_rates_ret_t */                     /* ---- */
void deserialize(query_commision_rates_ret_t& obj, const std::string &json);                /* done */

} /* namespace binance */
} /* namespace cuwacunu */
} /* namespace camahjucunu */