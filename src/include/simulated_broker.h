#pragma once
#include "dtypes.h"

namespace cuwacunu {

class Broker {
private:
    static float ctime;
    static std::vector<currency_space_t> currencies;
    static std::vector<std::function<float()>> delta_price_lambdas;
    static void finit();
    static void init();
public:
    static void reset();
    static class _init {public:_init(){Broker::init();}}_initializer;
    static currency_space_t retrieve_currency(instrument_t inst);
    static float get_current_price(instrument_t inst);
    static float get_current_mean(instrument_t inst);
    static float get_current_std(instrument_t inst);
    static float get_current_max(instrument_t inst);
    static float get_current_min(instrument_t inst);
    static float exchange_rate(instrument_t base_symb, instrument_t target_symb);
    static float get_current_price(instrument_t target_symb, instrument_t base_symb);
    static float get_step_count();
    static void step();
    static void exchange(position_space_t& base_position, position_space_t& target_position, order_space_t& order);
    
};

} // namespace cuwacunu