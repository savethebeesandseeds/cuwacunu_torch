#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cuwacunu {
namespace jkimyei {
namespace specs {

enum class value_kind_e : std::uint8_t {
  Bool,
  Int,
  Float,
  String,
  IntList,
  FloatList,
  StringList,
};

enum class component_type_e : std::uint8_t {
#define JK_COMPONENT_TYPE(kind_token, canonical_type, description) kind_token,
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_TYPE
};

struct component_descriptor_t {
  component_type_e type;
  std::string_view canonical_type;
  std::string_view description;
};

inline constexpr std::size_t kComponentCount =
0
#define JK_COMPONENT_TYPE(kind_token, canonical_type, description) + 1
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_TYPE
;

inline constexpr std::array<component_descriptor_t, kComponentCount> kComponents = {{
#define JK_COMPONENT_TYPE(kind_token, canonical_type, description) \
  component_descriptor_t{component_type_e::kind_token, canonical_type, description},
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_TYPE
}};

struct family_rule_t {
  component_type_e type;
  std::string_view family;
  bool required;
};

inline constexpr std::size_t kFamilyRuleCount =
0
#define JK_COMPONENT_REQUIRED_FAMILY(kind_token, family_token) + 1
#define JK_COMPONENT_FORBIDDEN_FAMILY(kind_token, family_token) + 1
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_REQUIRED_FAMILY
#undef JK_COMPONENT_FORBIDDEN_FAMILY
;

inline constexpr std::array<family_rule_t, kFamilyRuleCount> kFamilyRules = {{
#define JK_COMPONENT_REQUIRED_FAMILY(kind_token, family_token) \
  family_rule_t{component_type_e::kind_token, #family_token, true},
#define JK_COMPONENT_FORBIDDEN_FAMILY(kind_token, family_token) \
  family_rule_t{component_type_e::kind_token, #family_token, false},
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_REQUIRED_FAMILY
#undef JK_COMPONENT_FORBIDDEN_FAMILY
}};

struct typed_param_descriptor_t {
  std::string_view owner;
  std::string_view key;
  value_kind_e kind;
  bool required;
  std::string_view description;
};

inline constexpr std::size_t kTypedParamCount =
0
#define JK_OPTIMIZER_PARAM(type_token, key, value_kind, required, description) + 1
#define JK_SCHEDULER_PARAM(type_token, key, value_kind, required, description) + 1
#define JK_LOSS_PARAM(type_token, key, value_kind, required, description) + 1
#define JK_COMPONENT_PARAM(kind_token, key, value_kind, required, description) + 1
#define JK_REPRO_PARAM(key, value_kind, required, description) + 1
#define JK_NUMERIC_PARAM(key, value_kind, required, description) + 1
#define JK_GRADIENT_PARAM(key, value_kind, required, description) + 1
#define JK_CHECKPOINT_PARAM(key, value_kind, required, description) + 1
#define JK_METRIC_PARAM(key, value_kind, required, description) + 1
#define JK_DATA_PARAM(key, value_kind, required, description) + 1
#define JK_AUGMENTATION_PARAM(key, value_kind, required, description) + 1
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_OPTIMIZER_PARAM
#undef JK_SCHEDULER_PARAM
#undef JK_LOSS_PARAM
#undef JK_COMPONENT_PARAM
#undef JK_REPRO_PARAM
#undef JK_NUMERIC_PARAM
#undef JK_GRADIENT_PARAM
#undef JK_CHECKPOINT_PARAM
#undef JK_METRIC_PARAM
#undef JK_DATA_PARAM
#undef JK_AUGMENTATION_PARAM
;

inline constexpr std::array<typed_param_descriptor_t, kTypedParamCount> kTypedParams = {{
#define JK_OPTIMIZER_PARAM(type_token, key, value_kind, required, description) \
  typed_param_descriptor_t{"optimizer." #type_token, key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_SCHEDULER_PARAM(type_token, key, value_kind, required, description) \
  typed_param_descriptor_t{"scheduler." #type_token, key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_LOSS_PARAM(type_token, key, value_kind, required, description) \
  typed_param_descriptor_t{"loss." #type_token, key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_COMPONENT_PARAM(kind_token, key, value_kind, required, description) \
  typed_param_descriptor_t{"component." #kind_token, key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_REPRO_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"reproducibility", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_NUMERIC_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"numerics", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_GRADIENT_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"gradient", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_CHECKPOINT_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"checkpoint", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_METRIC_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"metrics", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_DATA_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"data_ref", key, value_kind_e::value_kind, ((required) != 0), description},
#define JK_AUGMENTATION_PARAM(key, value_kind, required, description) \
  typed_param_descriptor_t{"augmentation.curve", key, value_kind_e::value_kind, ((required) != 0), description},
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_OPTIMIZER_PARAM
#undef JK_SCHEDULER_PARAM
#undef JK_LOSS_PARAM
#undef JK_COMPONENT_PARAM
#undef JK_REPRO_PARAM
#undef JK_NUMERIC_PARAM
#undef JK_GRADIENT_PARAM
#undef JK_CHECKPOINT_PARAM
#undef JK_METRIC_PARAM
#undef JK_DATA_PARAM
#undef JK_AUGMENTATION_PARAM
}};

struct ini_selector_field_t {
  std::string_view key;
  std::string_view description;
};

inline constexpr std::size_t kIniSelectorFieldCount =
0
#define JK_INI_SELECTOR_FIELD(key, description) + 1
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_INI_SELECTOR_FIELD
;

inline constexpr std::array<ini_selector_field_t, kIniSelectorFieldCount> kIniSelectorFields = {{
#define JK_INI_SELECTOR_FIELD(key, description) ini_selector_field_t{key, description},
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_INI_SELECTOR_FIELD
}};

struct runtime_owned_field_t {
  std::string_view key;
  std::string_view description;
};

inline constexpr std::size_t kRuntimeOwnedFieldCount =
0
#define JK_RUNTIME_OWNED_FIELD(key, description) + 1
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_RUNTIME_OWNED_FIELD
;

inline constexpr std::array<runtime_owned_field_t, kRuntimeOwnedFieldCount> kRuntimeOwnedFields = {{
#define JK_RUNTIME_OWNED_FIELD(key, description) runtime_owned_field_t{key, description},
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_RUNTIME_OWNED_FIELD
}};

} // namespace specs
} // namespace jkimyei
} // namespace cuwacunu
