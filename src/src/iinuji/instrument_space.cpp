#include "iinuji/instrument_space.h"
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t could use many more indicators.\n");
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t evaluate if instrument_space_t should be in tensors.\n");
RUNTIME_WARNING("(instrument_space.cpp)[] #FIXME instrument_space_t might be absorb or mix with quote_space_t.\n");

... not working
namespace cuwacunu {
namespace iinuji {
/* instrument_space_t */
instrument_space_t::instrument_space_t(const instrument_e symb, float price)
  : symb(symb), 
    token(torch::tensor(CURRENCY_TOKENIZER[static_cast<int>(symb)], torch::kInt64).device(cuwacunu::kDevice)),
    price(torch::tensor({price}, torch::dtype(cuwacunu::kType).device(cuwacunu::kDevice))),
    stats(statistics_t(price)) {
      /* validate the tensors */
      validate_tensor(token, "[instrument_space_t] ---token.");
      validate_tensor(price, "[instrument_space_t] ---price.");
      /* validate token shape */
      assert_tensor_shape(token, (size_t) COUNT_INSTRUMENTS, "[instrument_space_t] ---token.");
    }
/* Copy Constructor */
instrument_space_t::instrument_space_t(const instrument_space_t& src)
  : symb(src.symb), 
    token(src.token),
    price(src.price.clone().detach()),
    stats(src.stats) {}
/* Step */
void instrument_space_t::step(float updated_price) {
  price = updated_price;   /* udpate the price */
  stats.update(price.item<float>());  /* update the stats */
}
/* Step by differential increment */
void instrument_space_t::delta_step(float delta_price) {
  step(price + delta_price);
}
} /* namespace iinuji */
} /* namespace cuwacunu */