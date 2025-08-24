/* instrument_space.h */
#pragma once

#include <torch/torch.h>
#include <string>
#include <vector>
#include <limits>        // std::numeric_limits
#include <stdexcept>     // std::invalid_argument
#include <utility>       // std::move
#include <compare>       // <=>  (C++20) – guarded
#include "piaabo/dutils.h"

#include "wikimyei/enviroment/capital_alocation_strategy/quote_space.h"
#include "wikimyei/enviroment/capital_alocation_strategy/observation_space.h"

RUNTIME_WARNING("(instrument_space.h)[] include all the desired here indicators.\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

// Forward declaration to avoid circular #include with quote_space.h
struct quote_space_t;

/* ────────────────────────────────────────────────────────────────
   A tradable instrument (currency, equity, future, …)
   Equality & ordering are defined by its name.                 */
struct instrument_space_t
{
  std::string target_symb;        /* e.g. "BTC", "USDT" */
  
  /* ---- ctor with basic validation -------------------------------- */
  explicit instrument_space_t(std::string name)
    : target_symb(std::move(name))
  {
    if (target_symb.empty())
      throw std::invalid_argument(
        "(instrument_space.h)[ctor] target_symb must not be empty");
  }

  /* ---- instrument name ----------------- */
  [[nodiscard]] std::string get_name() const { return target_symb; }
  
  /* ---- quote snapshot  ----------------- */
  [[nodiscard]] quote_space_t get_quote(const std::string& base_symb) {
    #FIXME ... implement
  }
  
  /* ---- equality --------------------------------------- */
  friend bool operator==(const instrument_space_t& lhs,
              const instrument_space_t& rhs) noexcept
  { return lhs.target_symb == rhs.target_symb; }
};
} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */

/* ------------------------------------------------------------------
   Hash specialisation so instrument_space_t can live in unordered_map
------------------------------------------------------------------- */
#include <functional>

template<>
struct std::hash<cuwacunu::wikimyei::capital_alocation_strategy::instrument_space_t>
{
  size_t operator()(
    const cuwacunu::wikimyei::capital_alocation_strategy::instrument_space_t& i)
    const noexcept
  {
    return std::hash<std::string>{}(i.instrument_name);
  }
};
