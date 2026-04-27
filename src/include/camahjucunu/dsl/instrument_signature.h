#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

inline constexpr const char *kInstrumentSignatureAny = "ANY";

struct instrument_signature_t {
  std::string symbol{};
  std::string record_type{};
  std::string market_type{};
  std::string venue{};
  std::string base_asset{};
  std::string quote_asset{};
};

[[nodiscard]] inline std::string
instrument_signature_trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] inline bool
instrument_signature_is_any(std::string_view value) noexcept {
  return value == std::string_view(kInstrumentSignatureAny);
}

[[nodiscard]] inline instrument_signature_t instrument_signature_all_any() {
  return instrument_signature_t{
      .symbol = kInstrumentSignatureAny,
      .record_type = kInstrumentSignatureAny,
      .market_type = kInstrumentSignatureAny,
      .venue = kInstrumentSignatureAny,
      .base_asset = kInstrumentSignatureAny,
      .quote_asset = kInstrumentSignatureAny,
  };
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
instrument_signature_fields(const instrument_signature_t &signature) {
  return {
      {"SYMBOL", signature.symbol},
      {"RECORD_TYPE", signature.record_type},
      {"MARKET_TYPE", signature.market_type},
      {"VENUE", signature.venue},
      {"BASE_ASSET", signature.base_asset},
      {"QUOTE_ASSET", signature.quote_asset},
  };
}

[[nodiscard]] inline bool
instrument_signature_set_field(instrument_signature_t *signature,
                               std::string_view field, std::string value,
                               std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!signature) {
    if (error)
      *error = "missing instrument signature output";
    return false;
  }
  value = instrument_signature_trim_ascii(value);
  if (field == "SYMBOL") {
    signature->symbol = std::move(value);
    return true;
  }
  if (field == "RECORD_TYPE") {
    signature->record_type = std::move(value);
    return true;
  }
  if (field == "MARKET_TYPE") {
    signature->market_type = std::move(value);
    return true;
  }
  if (field == "VENUE") {
    signature->venue = std::move(value);
    return true;
  }
  if (field == "BASE_ASSET") {
    signature->base_asset = std::move(value);
    return true;
  }
  if (field == "QUOTE_ASSET") {
    signature->quote_asset = std::move(value);
    return true;
  }
  if (error)
    *error = "unknown instrument signature field: " + std::string(field);
  return false;
}

[[nodiscard]] inline bool
instrument_signature_is_complete(const instrument_signature_t &signature) {
  for (const auto &[_, value] : instrument_signature_fields(signature)) {
    if (instrument_signature_trim_ascii(value).empty())
      return false;
  }
  return true;
}

[[nodiscard]] inline bool
instrument_signature_contains_any(const instrument_signature_t &signature) {
  for (const auto &[_, value] : instrument_signature_fields(signature)) {
    if (instrument_signature_is_any(value))
      return true;
  }
  return false;
}

[[nodiscard]] inline bool
instrument_signature_validate(const instrument_signature_t &signature,
                              bool allow_any, std::string_view label,
                              std::string *error = nullptr) {
  if (error)
    error->clear();
  for (const auto &[field, value] : instrument_signature_fields(signature)) {
    const std::string trimmed = instrument_signature_trim_ascii(value);
    if (trimmed.empty()) {
      if (error) {
        *error = std::string(label) + " missing " + field;
      }
      return false;
    }
    if (!allow_any && instrument_signature_is_any(trimmed)) {
      if (error) {
        *error = std::string(label) + " must be exact; field " + field +
                 " cannot be ANY";
      }
      return false;
    }
  }

  if (!instrument_signature_is_any(signature.symbol)) {
    if (instrument_signature_is_any(signature.base_asset) ||
        instrument_signature_is_any(signature.quote_asset)) {
      if (error) {
        *error = std::string(label) +
                 " exact SYMBOL requires exact BASE_ASSET and QUOTE_ASSET";
      }
      return false;
    }
  }

  if (!instrument_signature_is_any(signature.symbol) &&
      !instrument_signature_is_any(signature.base_asset) &&
      !instrument_signature_is_any(signature.quote_asset)) {
    const std::string derived =
        signature.quote_asset == "NONE"
            ? signature.base_asset
            : signature.base_asset + signature.quote_asset;
    if (signature.symbol != derived) {
      if (error) {
        *error = std::string(label) +
                 " SYMBOL/BASE_ASSET/QUOTE_ASSET "
                 "mismatch: SYMBOL=" +
                 signature.symbol + " derived=" + derived;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
instrument_signature_accepts_runtime(const instrument_signature_t &accepted,
                                     const instrument_signature_t &runtime,
                                     std::string_view label,
                                     std::string *error = nullptr) {
  if (error)
    error->clear();
  std::string validation_error{};
  if (!instrument_signature_validate(accepted, /*allow_any=*/true, label,
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }
  if (!instrument_signature_validate(runtime, /*allow_any=*/false,
                                     "RUNTIME_INSTRUMENT_SIGNATURE",
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }

  const auto accepted_fields = instrument_signature_fields(accepted);
  const auto runtime_fields = instrument_signature_fields(runtime);
  for (std::size_t i = 0; i < accepted_fields.size(); ++i) {
    const auto &[field, accepted_value] = accepted_fields[i];
    const auto &[_, runtime_value] = runtime_fields[i];
    if (instrument_signature_is_any(accepted_value))
      continue;
    if (accepted_value == runtime_value)
      continue;
    if (error) {
      *error = std::string(label) +
               " does not accept runtime instrument: " + field +
               " expected=" + accepted_value + " runtime=" + runtime_value;
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool instrument_signatures_overlap(
    const instrument_signature_t &lhs, const instrument_signature_t &rhs,
    std::string_view lhs_label, std::string_view rhs_label,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  std::string validation_error{};
  if (!instrument_signature_validate(lhs, /*allow_any=*/true, lhs_label,
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }
  if (!instrument_signature_validate(rhs, /*allow_any=*/true, rhs_label,
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }

  const auto lhs_fields = instrument_signature_fields(lhs);
  const auto rhs_fields = instrument_signature_fields(rhs);
  for (std::size_t i = 0; i < lhs_fields.size(); ++i) {
    const auto &[field, lhs_value] = lhs_fields[i];
    const auto &[_, rhs_value] = rhs_fields[i];
    if (instrument_signature_is_any(lhs_value) ||
        instrument_signature_is_any(rhs_value) || lhs_value == rhs_value) {
      continue;
    }
    if (error) {
      *error = std::string(lhs_label) + " and " + std::string(rhs_label) +
               " have no compatible instrument overlap at " + field + ": " +
               lhs_value + " vs " + rhs_value;
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string
instrument_signature_compact_string(const instrument_signature_t &signature) {
  std::ostringstream oss;
  oss << "SYMBOL=" << signature.symbol
      << ",RECORD_TYPE=" << signature.record_type
      << ",MARKET_TYPE=" << signature.market_type
      << ",VENUE=" << signature.venue << ",BASE_ASSET=" << signature.base_asset
      << ",QUOTE_ASSET=" << signature.quote_asset;
  return oss.str();
}

} // namespace camahjucunu
} // namespace cuwacunu
