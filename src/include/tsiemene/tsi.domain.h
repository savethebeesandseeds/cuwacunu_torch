// ./include/tsiemene/tsi.domain.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string_view>

namespace tsiemene {

enum class TsiDomain : std::uint8_t {
  Source = 0,
  Wikimyei = 1,
  Sink = 2,
};

[[nodiscard]] constexpr std::string_view domain_token(TsiDomain domain) noexcept {
  switch (domain) {
    case TsiDomain::Source: return "source";
    case TsiDomain::Wikimyei: return "wikimyei";
    case TsiDomain::Sink: return "sink";
  }
  return "unknown";
}

} // namespace tsiemene
