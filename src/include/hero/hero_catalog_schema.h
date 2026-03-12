#pragma once

#include <cstdint>
#include <string_view>

namespace cuwacunu {
namespace hero {
namespace schema {

inline constexpr const char kUnifiedCatalogModelSchemaV1[] =
    "hero.catalog.model.v1";

enum class logical_table_e : std::uint8_t {
#define HERO_LOGICAL_TABLE_ENTRY(ENUM_NAME, TEXT_NAME) ENUM_NAME,
#include "hero/hero_catalog_logical_table.def"
#undef HERO_LOGICAL_TABLE_ENTRY
};

[[nodiscard]] inline std::string_view logical_table_to_string(
    logical_table_e table) {
  switch (table) {
#define HERO_LOGICAL_TABLE_ENTRY(ENUM_NAME, TEXT_NAME) \
  case logical_table_e::ENUM_NAME:                     \
    return TEXT_NAME;
#include "hero/hero_catalog_logical_table.def"
#undef HERO_LOGICAL_TABLE_ENTRY
  }
  return "unknown";
}

[[nodiscard]] inline bool parse_logical_table(std::string_view text,
                                              logical_table_e* out) {
  if (!out) return false;
#define HERO_LOGICAL_TABLE_ENTRY(ENUM_NAME, TEXT_NAME) \
  if (text == TEXT_NAME) {                             \
    *out = logical_table_e::ENUM_NAME;                 \
    return true;                                       \
  }
#include "hero/hero_catalog_logical_table.def"
#undef HERO_LOGICAL_TABLE_ENTRY
  return false;
}

#define HERO_CATALOG_RECORD_KIND_ENTRY(ENUM_NAME, TEXT_NAME, LOGICAL_TABLE_ENUM) \
  inline constexpr const char kRecordKind##ENUM_NAME[] = TEXT_NAME;
#include "hero/hero_catalog_record_kind.def"
#undef HERO_CATALOG_RECORD_KIND_ENTRY

[[nodiscard]] inline logical_table_e logical_table_for_record_kind(
    std::string_view record_kind) {
#define HERO_CATALOG_RECORD_KIND_ENTRY(ENUM_NAME, TEXT_NAME, LOGICAL_TABLE_ENUM) \
  if (record_kind == TEXT_NAME) return logical_table_e::LOGICAL_TABLE_ENUM;
#include "hero/hero_catalog_record_kind.def"
#undef HERO_CATALOG_RECORD_KIND_ENTRY
  return logical_table_e::UNKNOWN;
}

[[nodiscard]] inline bool is_known_record_kind(std::string_view record_kind) {
  return logical_table_for_record_kind(record_kind) !=
         logical_table_e::UNKNOWN;
}

}  // namespace schema
}  // namespace hero
}  // namespace cuwacunu
