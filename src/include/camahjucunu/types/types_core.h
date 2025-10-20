/* types_core.h */
#pragma once
#include <cstdint>
#include <type_traits>

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {

/* Fixed-width aliases (portable across platforms) */
using i8  = std::int8_t;
using u8  = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;

/* Milliseconds since Unix epoch (alias for clarity) */
using ms_t = std::int64_t;

/* GNU/Clang packed attribute (no-op on other compilers) */
#if defined(__GNUC__) || defined(__clang__)
  #define GNU_PACKED [[gnu::packed]]
#else
  #define GNU_PACKED
#endif

} /* namespace exchange */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
