// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace latent_lineage_state {

inline constexpr std::string_view kRuntimeLlsGrammarPath =
    "/cuwacunu/src/config/bnf/runtime_lls.bnf";

struct runtime_lls_entry_t {
  std::string key{};
  std::string declared_domain{};
  std::string declared_type{};
  std::string value{};
};

struct runtime_lls_document_t {
  std::vector<runtime_lls_entry_t> entries{};
};

[[nodiscard]] std::string runtime_lls_grammar_text();

[[nodiscard]] bool validate_runtime_lls_text(
    std::string_view text,
    std::string* error = nullptr);

[[nodiscard]] bool parse_runtime_lls_text(
    std::string_view text,
    runtime_lls_document_t* out,
    std::string* error = nullptr);

[[nodiscard]] bool validate_runtime_lls_document(
    const runtime_lls_document_t& document,
    std::string* error = nullptr);

[[nodiscard]] bool runtime_lls_document_to_kv_map(
    const runtime_lls_document_t& document,
    std::unordered_map<std::string, std::string>* out,
    std::string* error = nullptr);

[[nodiscard]] std::string emit_runtime_lls_canonical(
    const runtime_lls_document_t& document);

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_entry(
    std::string key,
    std::string value,
    std::string declared_type,
    std::string declared_domain = {});

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_string_entry(
    std::string key,
    std::string value);

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_bool_entry(
    std::string key,
    bool value);

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_int_entry(
    std::string key,
    std::int64_t value,
    std::string declared_domain = "(-inf,+inf)");

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_uint_entry(
    std::string key,
    std::uint64_t value,
    std::string declared_domain = "(-inf,+inf)");

[[nodiscard]] runtime_lls_entry_t make_runtime_lls_double_entry(
    std::string key,
    double value,
    std::string declared_domain = "(-inf,+inf)");

}  // namespace latent_lineage_state
}  // namespace piaabo
}  // namespace cuwacunu
