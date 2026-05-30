#pragma once

namespace cuwacunu {
namespace hashimyei {

#define HASHIMYEI_CONST_STR(NAME, VALUE) inline constexpr const char NAME[] = VALUE;
#include "hero/hashimyei_hero/hashimyei_schema.def"
#undef HASHIMYEI_CONST_STR

}  // namespace hashimyei
}  // namespace cuwacunu
