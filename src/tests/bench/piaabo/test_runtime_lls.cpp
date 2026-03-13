#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace {

using cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_entry;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry;
using cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry;
using cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text;
using cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;
using cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map;
using cuwacunu::piaabo::latent_lineage_state::validate_runtime_lls_document;
using cuwacunu::piaabo::latent_lineage_state::validate_runtime_lls_text;

void expect_valid_text(const std::string& text) {
  std::string error;
  const bool ok = validate_runtime_lls_text(text, &error);
  if (!ok || !error.empty()) {
    std::cerr << "expected valid runtime .lls text\n"
              << "text:\n"
              << text
              << "error: " << error << "\n";
    std::abort();
  }
}

void expect_invalid_text(const std::string& text,
                         const std::string& expected_fragment) {
  std::string error;
  const bool ok = validate_runtime_lls_text(text, &error);
  if (ok) {
    std::cerr << "expected invalid runtime .lls text\n"
              << "text:\n"
              << text;
    std::abort();
  }
  if (error.find(expected_fragment) == std::string::npos) {
    std::cerr << "unexpected runtime .lls validation error\n"
              << "text:\n"
              << text
              << "expected fragment: " << expected_fragment << "\n"
              << "actual error: " << error << "\n";
    std::abort();
  }
}

void expect_invalid_document(const runtime_lls_document_t& document,
                             const std::string& expected_fragment) {
  std::string error;
  const bool ok = validate_runtime_lls_document(document, &error);
  if (ok) {
    std::cerr << "expected invalid runtime .lls document\n";
    std::abort();
  }
  if (error.find(expected_fragment) == std::string::npos) {
    std::cerr << "unexpected runtime .lls document validation error\n"
              << "expected fragment: " << expected_fragment << "\n"
              << "actual error: " << error << "\n";
    std::abort();
  }
}

}  // namespace

int main() {
  expect_valid_text(
      "/* header */\n"
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "metric.value(-inf,+inf):double = 1.250000000000\n"
      "flag:bool = true\n");

  expect_valid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "metric.value(-inf,+inf):double = 1.0 /* middle */\n"
      "flag:bool = true\n");

  expect_valid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "message:str = hello /* tail */\n"
      "count(-inf,+inf):int = 7\n");

  expect_valid_text(
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n"
      "source.runtime.symbol:str = BTCUSDT\n"
      "run_id:str = run_001\n");

  expect_valid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "wave_cursor:uint = 9223372036854775808\n");

  expect_invalid_text(
      "# legacy header\n"
      "schema:str = piaabo.test.runtime_lls.v1\n",
      "only /* ... */ comments");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "count:int = 1;\n",
      "only /* ... */ comments");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "count = 1\n",
      "parse failure");

  expect_invalid_text(
      "metric:int = 1\n",
      "missing schema");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls\n",
      ".v<integer> suffix");

  expect_invalid_text(
      "metric:int = 1\n"
      "schema:str = piaabo.test.runtime_lls.v1\n",
      "first assignment");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "schema:str = piaabo.test.runtime_lls.v1\n",
      "duplicate key");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "wave_cursor:uint = -1\n",
      "invalid uint value");

  expect_invalid_text(
      "schema:str = piaabo.test.runtime_lls.v1\n"
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n",
      "duplicate schema entry");

  runtime_lls_document_t document{};
  document.entries.push_back(make_runtime_lls_int_entry("count", 7));
  document.entries.push_back(
      make_runtime_lls_string_entry("schema", "piaabo.test.runtime_lls.v1"));
  document.entries.push_back(make_runtime_lls_bool_entry("flag", true));
  document.entries.push_back(make_runtime_lls_double_entry("loss", 1.5));
  document.entries.push_back(
      make_runtime_lls_uint_entry("wave_cursor", 9223372036854775808ULL));

  std::string error;
  assert(validate_runtime_lls_document(document, &error));
  assert(error.empty());

  const std::string emitted = emit_runtime_lls_canonical(document);
  assert(emitted.find("schema:str = piaabo.test.runtime_lls.v1\n") == 0);
  assert(emitted.find("flag:bool = true\n") != std::string::npos);
  assert(emitted.find("count(-inf,+inf):int = 7\n") != std::string::npos);
  assert(emitted.find("loss(-inf,+inf):double = 1.500000000000\n") !=
         std::string::npos);
  assert(emitted.find(
             "wave_cursor(-inf,+inf):uint = 9223372036854775808\n") !=
         std::string::npos);

  std::string validate_error;
  assert(validate_runtime_lls_text(emitted, &validate_error));
  assert(validate_error.empty());

  runtime_lls_document_t parsed{};
  std::string parse_error;
  assert(parse_runtime_lls_text(
      "source.runtime.projection.schema:str = wave.source.runtime.projection.v2\n"
      "source.runtime.symbol:str = BTCUSDT\n"
      "run_id:str = run_001\n",
      &parsed,
      &parse_error));
  assert(parse_error.empty());
  std::unordered_map<std::string, std::string> kv{};
  assert(runtime_lls_document_to_kv_map(parsed, &kv, &parse_error));
  assert(parse_error.empty());
  assert(kv["source.runtime.projection.schema"] ==
         "wave.source.runtime.projection.v2");
  assert(kv["source.runtime.symbol"] == "BTCUSDT");
  assert(kv["run_id"] == "run_001");

  runtime_lls_document_t invalid_comment_document{};
  invalid_comment_document.entries.push_back(
      make_runtime_lls_string_entry("schema", "piaabo.test.runtime_lls.v1"));
  invalid_comment_document.entries.push_back(
      make_runtime_lls_string_entry("message", "hello /* hidden */ world"));
  expect_invalid_document(invalid_comment_document, "comment delimiter");

  runtime_lls_document_t invalid_type_document{};
  invalid_type_document.entries.push_back(
      make_runtime_lls_string_entry("schema", "piaabo.test.runtime_lls.v1"));
  invalid_type_document.entries.push_back(
      make_runtime_lls_entry("dims", "1,2,3", "arr[int]"));
  expect_invalid_document(invalid_type_document, "unsupported declared_type");

  runtime_lls_document_t invalid_domain_document{};
  invalid_domain_document.entries.push_back(
      make_runtime_lls_string_entry("schema", "piaabo.test.runtime_lls.v1"));
  invalid_domain_document.entries.push_back(
      make_runtime_lls_entry("metric", "1", "int", "(bad#domain)"));
  expect_invalid_document(invalid_domain_document, "invalid declared_domain");

  return 0;
}
