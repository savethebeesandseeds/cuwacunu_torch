#include "dconfig.h"
#include "dutils.h"
#include "simulated_broker.h"

namespace cuwacunu {
float Broker::ctime;
std::vector<currency_space_t> Broker::currencies;
std::vector<std::function<float()>> Broker::delta_price_lambdas;

void Broker::finit() {}
void Broker::init() {
  std::atexit(Broker::finit);
  Broker::ctime = 0;
  Broker::delta_price_lambdas = {
    []() -> float { return 0.0; },  /* update delta for CONST */
    []() -> float {  /* update delta for SINE diff(sin) = cos */
      return std::cos(2 * M_PI * (0.25) * Broker::ctime); } 
  };
  FOR_ALL_INSTRUMENTS(inst) {
    /* initialize currency price in 5 to avoid negative numbers (as the price it's a sine wave) */
    Broker::currencies.push_back(currency_space_t(inst, 5.0));
  }
}
void Broker::reset() {
  Broker::init();
}
currency_space_t Broker::retrieve_currency(instrument_t inst) {   /* in ABSOLUTE_BASE_SYMB */
  return Broker::currencies.at((size_t)inst);
}
/* get_current_price */ float Broker::get_current_price(instrument_t inst) { return Broker::retrieve_currency(inst).price.item<float>(); }
/* get_current_mean */  float Broker::get_current_mean (instrument_t inst) { return Broker::retrieve_currency(inst).stats.mean();}
/* get_current_std */   float Broker::get_current_std  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.stddev(); }
/* get_current_max */   float Broker::get_current_max  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.max(); }
/* get_current_min */   float Broker::get_current_min  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.min(); }
/* get exchange_rate */ float Broker::exchange_rate    (instrument_t base_symb, instrument_t target_symb) {
/* the exchange rate is expressed is how much of the target currency you can buy with one unit of the base currency */
  return Broker::get_current_price(target_symb) / Broker::get_current_price(base_symb);
}
/* get_current_price */ float Broker::get_current_price(instrument_t target_symb, instrument_t base_symb) { return Broker::exchange_rate(base_symb, target_symb); }
/* get_step_count */    float Broker::get_step_count() { return ctime; }
/* perform a step */    void Broker::step() {
  ++ctime;
  FOR_ALL_INSTRUMENTS(inst) {
    float dv = delta_price_lambdas[(int)inst]();
    Broker::currencies[(size_t)inst].price += dv;   /* udpate the price */
    Broker::currencies[(size_t)inst].stats.update(
      Broker::currencies[(size_t)inst].price.item<float>());  /* update the stats */
  }
}
/* make a transation, exchange an order */
void Broker::exchange(position_space_t& base_position, position_space_t& target_position, order_space_t& order) {
  if(!order.liquidated) {
    if(order.target_price <= Broker::get_current_price(order.target_symb, order.base_symb)) {
      float exch_rte = Broker::exchange_rate(base_position.symb, target_position.symb);
      float required_base_amount = (order.target_amount * exch_rte) * (1 + BROKER_COMMISSION_RATE);
      if(required_base_amount > base_position.amount) { /* not enough base amount, then buy the max */
        required_base_amount = base_position.amount;
      }
      base_position.amount -= required_base_amount;
      target_position.amount += required_base_amount / exch_rte;
      order.liquidated = true;
    }
  }
}
/* initialize */
Broker::_init Broker::_initializer;

/* WARNING: a singleton desing for the broker might bottle-neck parallel training */

} // namespace cuwacunu



// namespace cuwacunu {
// class Broker {
// private:
//   static float ctime;
//   static std::vector<currency_space_t> currencies;
//   static std::vector<std::function<float()>> delta_price_lambdas;
// public:
//   static class _init {public:_init(){Broker::init();}}_initializer;
//   static void init() {
//     Broker::ctime = 0;
//     delta_price_lambdas = {
//       []() -> float { return 0.0; },           /* update delta for CONST */
//       []() -> float { return std::cos(2 * M_PI * (0.25) * Broker::ctime); }  /* update delta for SINE diff(sin) = cos */
//     };
//     FOR_ALL_INSTRUMENTS(inst) {
//       /* initialize currency price in 5 to avoid negative numbers (as the price it's a sine wave) */
//       Broker::currencies.push_back(currency_space_t(inst, 5.0));
//     }
//   }
//   static currency_space_t retrieve_currency(instrument_t inst) {   /* in ABSOLUTE_BASE_SYMB */
//     return Broker::currencies.at((size_t)inst);
//   }
//   /* get_current_price */ static float get_current_price(instrument_t inst) { return Broker::retrieve_currency(inst).price; }
//   /* get_current_mean */  static float get_current_mean (instrument_t inst) { return Broker::retrieve_currency(inst).stats.mean();}
//   /* get_current_std */   static float get_current_std  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.std(); }
//   /* get_current_max */   static float get_current_max  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.max(); }
//   /* get_current_min */   static float get_current_min  (instrument_t inst) { return Broker::retrieve_currency(inst).stats.min(); }
//   /* get exchange_rate */ static float exchange_rate (instrument_t base_symb, instrument_t target_symb) {
//     /* the exchange rate is expressed is how much of the target currency you can buy with one unit of the base currency */
//     return Broker::get_current_price(target_symb) / Broker::get_current_price(base_symb);
//   }
//   /* get_current_price */ static float get_current_price(instrument_t target_symb, instrument_t base_symb) { return Broker::exchange_rate(base_symb, target_symb); }
//   /* get_step_count */    static float get_step_count() { return ctime; }
//   /* perform a step */    static void step() {
//     ++ctime;
//     FOR_ALL_INSTRUMENTS(inst) {
//       float dv = delta_price_lambdas[(int)inst]();
//       Broker::currencies[(size_t)inst].diff = dv;  /* replace the diff */
//       Broker::currencies[(size_t)inst].price += dv;   /* udpate the price */
//       Broker::currencies[(size_t)inst].stats.update(
//       Broker::currencies[(size_t)inst].price);  /* update the stats */
//     }
//   }
//   /* make a transation, exchange an order */
//   static void exchange(position_space_t& base_position, position_space_t& target_position, order_space_t& order) {
//     if(!order.liquidated) {
//       if(order.target_price <= Broker::get_current_price(order.target_symb, order.base_symb)) {
//         float exch_rte = Broker::exchange_rate(base_position.symb, target_position.symb);
//         float required_base_amount = (order.target_amount * exch_rte) * (1 + BROKER_COMMISSION_RATE);
//         if(required_base_amount > base_position.amount) { /* not enough base amount, then buy the max */
//           required_base_amount = base_position.amount;
//         }
//         base_position.amount -= required_base_amount;
//         target_position.amount += required_base_amount / exch_rte;
//         order.liquidated = true;
//       }
//     }
//   }
// };
// Broker::_init Broker::_initializer;