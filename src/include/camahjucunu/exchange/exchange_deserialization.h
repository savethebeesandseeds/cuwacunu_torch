#pragma once

#include "piaabo/json_parsing.h"
#include "piaabo/dutils.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/exchange/exchange_enums.h"
#include "camahjucunu/exchange/exchange_types.h"



/*  
    This module is made under the asumption that exchange will not 
    deliver unexpected responces. 
    
    In most unexpected cases, deserialzie methods will rise a fatal error.

    These methods return the id of the retrived transaction
*/

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*        string to struct functions           */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* specialization for deserialization of ping_ret_t */                                 /* ---- */
void deserialize(ping_ret_t& deserialized, const std::string& json);                   /* done */
/* specialization for deserialization of time_ret_t */                                 /* ---- */
void deserialize(time_ret_t& deserialized, const std::string &json);                   /* done */
/* specialization for deserialization of depth_ret_t */                                /* ---- */
void deserialize(depth_ret_t& deserialized, const std::string &json);                  /* done */
/* specialization for deserialization of tradesRecent_ret_t */                         /* ---- */
void deserialize(tradesRecent_ret_t& deserialized, const std::string &json);           /* done */
/* specialization for deserialization of tradesHistorical_ret_t */                     /* ---- */
void deserialize(tradesHistorical_ret_t& deserialized, const std::string &json);       /* done */
/* specialization for deserialization of klines_ret_t */                               /* ---- */
void deserialize(klines_ret_t& deserialized, const std::string &json);                 /* done */
/* specialization for deserialization of avgPrice_ret_t */                             /* ---- */
void deserialize(avgPrice_ret_t& deserialized, const std::string &json);               /* done */
/* specialization for deserialization of tickerTradingDay_ret_t */                     /* ---- */
void deserialize(tickerTradingDay_ret_t& deserialized, const std::string &json);       /* done */
/* specialization for deserialization of ticker_ret_t */                               /* ---- */
void deserialize(ticker_ret_t& deserialized, const std::string &json);                 /* done */
/* specialization for deserialization of tickerPrice_ret_t */                          /* ---- */
void deserialize(tickerPrice_ret_t& deserialized, const std::string &json);            /* done */
/* specialization for deserialization of tickerBook_ret_t */                           /* ---- */
void deserialize(tickerBook_ret_t& deserialized, const std::string &json);             /* done */
/* specialization for deserialization of account_information_ret_t */                  /* ---- */
void deserialize(account_information_ret_t& deserialized, const std::string &json);    /* done */
/* specialization for deserialization of account_order_history_ret_t */                /* ---- */
void deserialize(account_order_history_ret_t& deserialized, const std::string &json);  /* done */
/* specialization for deserialization of account_trade_list_ret_t */                   /* ---- */
void deserialize(account_trade_list_ret_t& deserialized, const std::string &json);     /* done */
/* specialization for deserialization of query_commission_rates_ret_t */               /* ---- */
void deserialize(query_commission_rates_ret_t& deserialized, const std::string &json); /* done */
/* specialization for deserialization of orderStatus_ret_t */                         /* ---- */
void deserialize(orderStatus_ret_t& deserialized, const std::string& json);           /* done */
/* specialization for deserialization of order_ack_ret_t */                            /* ---- */
void deserialize(order_ack_ret_t& deserialized, const std::string& json);              /* done */
/* specialization for deserialization of order_result_ret_t */                         /* ---- */
void deserialize(order_result_ret_t& deserialized, const std::string& json);           /* done */
/* specialization for deserialization of order_full_ret_t */                           /* ---- */
void deserialize(order_full_ret_t& deserialized, const std::string& json);             /* done */

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */