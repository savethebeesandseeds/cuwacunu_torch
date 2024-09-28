#pragma once
#include <vector>
#include <variant>
#include <sstream>
#include <optional>
#include <regex>
#include <cctype>  // For std::isspace
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_enums.h"
#include "camahjucunu/exchange/exchange_types.h"



/*  
    This module is made under the asumption that exchange will not 
    deliver unexpected responces. 
    
    In most unexpected cases, deserialzie methods will rise a fatal error.
*/



namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

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
/* specialization for deserialization of order_ack_ret_t */                                /* ---- */
void deserialize(order_ack_ret_t& obj, const std::string &json);                           /* done */
/* specialization for deserialization of order_result_ret_t */                             /* ---- */
void deserialize(order_result_ret_t& obj, const std::string &json);                        /* done */
/* specialization for deserialization of order_fill_t */                                    /* ---- */
void deserialize(order_fill_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of order_full_ret_t */                               /* ---- */
void deserialize(order_full_ret_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of order_sor_fill_t */                                /* ---- */
void deserialize(order_sor_fill_t& obj, const std::string &json);                           /* done */
/* specialization for deserialization of order_sor_full_ret_t */                           /* ---- */
void deserialize(order_sor_full_ret_t& obj, const std::string &json);                      /* done */
/* specialization for deserialization of commissionRates_t */                               /* ---- */
void deserialize(commissionRates_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of commission_discount_t */                            /* ---- */
void deserialize(commission_discount_t& obj, const std::string &json);                       /* done */
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
/* specialization for deserialization of tradesRecent_ret_t */                                    /* ---- */
void deserialize(tradesRecent_ret_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of tradesHistorical_ret_t */                          /* ---- */
void deserialize(tradesHistorical_ret_t& obj, const std::string &json);                     /* done */
/* specialization for deserialization of klines_ret_t */                                    /* ---- */
void deserialize(klines_ret_t& obj, const std::string &json);                               /* done */
/* specialization for deserialization of avgPrice_ret_t */                                  /* ---- */
void deserialize(avgPrice_ret_t& obj, const std::string &json);                             /* done */
/* specialization for deserialization of ticker24hr_ret_t */                               /* ---- */
void deserialize(ticker24hr_ret_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of tickerTradingDay_ret_t */                         /* ---- */
void deserialize(tickerTradingDay_ret_t& obj, const std::string &json);                    /* done */
/* specialization for deserialization of tickerPrice_ret_t */                              /* ---- */
void deserialize(tickerPrice_ret_t& obj, const std::string &json);                         /* done */
/* specialization for deserialization of tickerBook_ret_t */                         /* ---- */
void deserialize(tickerBook_ret_t& obj, const std::string &json);                    /* done */
/* specialization for deserialization of ticker_wind_ret_t */                               /* ---- */
void deserialize(ticker_wind_ret_t& obj, const std::string &json);                          /* done */
/* specialization for deserialization of account_information_ret_t */                       /* ---- */
void deserialize(account_information_ret_t& obj, const std::string &json);                  /* done */
/* specialization for deserialization of account_trade_list_ret_t */                        /* ---- */
void deserialize(account_trade_list_ret_t& obj, const std::string &json);                   /* done */
/* specialization for deserialization of query_commission_rates_ret_t */                     /* ---- */
void deserialize(query_commission_rates_ret_t& obj, const std::string &json);                /* done */

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */