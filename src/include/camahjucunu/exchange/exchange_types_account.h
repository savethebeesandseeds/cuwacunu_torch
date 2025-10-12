/* exchange_types_account.h */
#pragma once
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_trade.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
struct account_information_args_t       { void add_signature(); std::string jsonify(); std::optional<bool> omitZeroBalances = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct account_order_history_args_t     { void add_signature(); std::string jsonify(); std::string symbol; std::optional<int> orderId = std::nullopt; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<long> limit = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct account_trade_list_args_t        { void add_signature(); std::string jsonify(); std::string symbol; std::optional<int> orderId = std::nullopt; std::optional<long> startTime = std::nullopt; std::optional<long> endTime = std::nullopt; std::optional<int> fromId = std::nullopt; std::optional<int> limit = std::nullopt; std::optional<long> recvWindow = std::nullopt; std::string apiKey; std::string signature; long timestamp; };
struct account_commission_rates_args_t  { void add_signature(); std::string jsonify(); std::string symbol; std::string apiKey; std::string signature; long timestamp; };
ENFORCE_ARCHITECTURE_DESIGN(      account_information_args_t);
ENFORCE_ARCHITECTURE_DESIGN(    account_order_history_args_t);
ENFORCE_ARCHITECTURE_DESIGN(       account_trade_list_args_t);
ENFORCE_ARCHITECTURE_DESIGN( account_commission_rates_args_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */

/* secondary return structs */
struct balance_t             { std::string asset; double free; double locked; };
struct historicTrade_t       { std::string symbol; int id; int orderId; int orderListId; double price; double qty; double quoteQty; double commission; std::string commissionAsset; long time; bool isBuyer; bool isMaker; bool isBestMatch; };
struct commissionRates_t     { double maker; double taker; double buyer; double seller; };
struct commission_discount_t { bool enabledForAccount; bool enabledForSymbol; std::string discountAsset; double discount; };
struct commissionDesc_t      { std::string symbol; commissionRates_t standardCommission; commissionRates_t taxCommission; commission_discount_t discount; };
ENFORCE_ARCHITECTURE_DESIGN(             balance_t);
ENFORCE_ARCHITECTURE_DESIGN(       historicTrade_t);
ENFORCE_ARCHITECTURE_DESIGN(     commissionRates_t);
ENFORCE_ARCHITECTURE_DESIGN( commission_discount_t);
ENFORCE_ARCHITECTURE_DESIGN(      commissionDesc_t);

/* primary return structs */
struct account_information_ret_t      { frame_response_t frame_rsp; account_information_ret_t(const std::string &json); int makerCommission; int takerCommission; int buyerCommission; int sellerCommission; commissionRates_t commissionRates; bool canTrade; bool canWithdraw; bool canDeposit; bool brokered; bool requireSelfTradePrevention; bool preventSor; long updateTime; account_and_symbols_permissions_e accountType; std::vector<balance_t> balances; std::vector<account_and_symbols_permissions_e> permissions; long uid; };
struct account_order_history_ret_t    { frame_response_t frame_rsp; account_order_history_ret_t(const std::string &json); std::vector<orderStatus_ret_t> orders; };
struct account_trade_list_ret_t       { frame_response_t frame_rsp; account_trade_list_ret_t(const std::string &json); std::vector<historicTrade_t> trades; };
struct account_commission_rates_ret_t { frame_response_t frame_rsp; account_commission_rates_ret_t(const std::string &json); commissionDesc_t commissionDesc; };
ENFORCE_ARCHITECTURE_DESIGN(      account_information_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(    account_order_history_ret_t);
ENFORCE_ARCHITECTURE_DESIGN(       account_trade_list_ret_t);
ENFORCE_ARCHITECTURE_DESIGN( account_commission_rates_ret_t);

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(account_information_ret_t& deserialized, const std::string &json);
void deserialize(account_order_history_ret_t& deserialized, const std::string &json);
void deserialize(account_trade_list_ret_t& deserialized, const std::string &json);
void deserialize(account_commission_rates_ret_t& deserialized, const std::string &json);

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
