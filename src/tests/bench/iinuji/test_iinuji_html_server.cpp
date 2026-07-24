// SPDX-License-Identifier: MIT

#include "iinuji/html/iinuji_http_server.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <unistd.h>

namespace {

bool require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    return false;
  }
  return true;
}

bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

std::size_t count_occurrences(std::string_view text, std::string_view needle) {
  std::size_t count = 0U;
  std::size_t cursor = 0U;
  while ((cursor = text.find(needle, cursor)) != std::string_view::npos) {
    ++count;
    cursor += needle.size();
  }
  return count;
}

template <typename Operation>
bool expect_http_error(Operation &&operation, int expected_status,
                       std::string_view label) {
  try {
    operation();
  } catch (const cuwacunu::iinuji::html::http_request_error &error) {
    return require(error.status() == expected_status,
                   std::string(label) + " returned the wrong HTTP status");
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << label
              << " threw the wrong exception: " << error.what() << '\n';
    return false;
  }
  std::cerr << "[FAIL] " << label << " unexpectedly succeeded\n";
  return false;
}

std::size_t count_point_tuples(std::string_view chart_json) {
  constexpr std::string_view marker = "\"points\":[";
  const auto marker_at = chart_json.find(marker);
  if (marker_at == std::string_view::npos) {
    return 0U;
  }
  std::size_t cursor = marker_at + marker.size();
  std::size_t count = 0U;
  int depth = 1;
  for (; cursor < chart_json.size() && depth > 0; ++cursor) {
    if (chart_json[cursor] == '[') {
      if (depth == 1) {
        ++count;
      }
      ++depth;
    } else if (chart_json[cursor] == ']') {
      --depth;
    }
  }
  return count;
}

} // namespace

int main() {
  using namespace cuwacunu::iinuji::html;
  bool ok = true;

  const auto parsed_get = parse_http_request(
      "GET /api/v1/catalog HTTP/1.1\r\nHost: localhost\r\n\r\n");
  ok = require(parsed_get.method == http_method_e::get &&
                   parsed_get.path == "/api/v1/catalog" &&
                   parsed_get.headers.at("host") == "localhost",
               "GET request parser should retain the allowlisted path") &&
       ok;

  const auto parsed_head = parse_http_request("HEAD /healthz HTTP/1.0\r\n\r\n");
  ok = require(parsed_head.method == http_method_e::head &&
                   parsed_head.path == "/healthz",
               "HEAD HTTP/1.0 should parse without Host") &&
       ok;

  const auto parsed_benchmark_query = parse_http_request(
      "GET /?benchmark=synthetic_continuous_graph_v2 HTTP/1.1\r\n"
      "Host: localhost\r\n\r\n");
  ok = require(parsed_benchmark_query.path == "/" &&
                   parsed_benchmark_query.query.at("benchmark") ==
                       "synthetic_continuous_graph_v2",
               "safe benchmark query should remain separate from the route") &&
       ok;

  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "POST /healthz HTTP/1.1\r\nHost: localhost\r\n\r\n");
           },
           405, "method rejection") &&
       ok;
  ok = expect_http_error(
           [] { (void)parse_http_request("GET /healthz HTTP/1.1\r\n\r\n"); },
           400, "missing Host rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "GET /healthz HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n");
           },
           400, "duplicate header rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "GET /healthz HTTP/1.1\r\nHost: localhost\r\n"
                 "Content-Length: nope\r\n\r\n");
           },
           400, "malformed Content-Length rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "GET /healthz HTTP/1.1\r\nHost: localhost\r\n"
                 "Content-Length: 1\r\n\r\nx");
           },
           400, "GET body rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "GET /../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
           },
           400, "dot-dot traversal rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request(
                 "GET /%2e%2e/etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
           },
           400, "percent-encoded traversal rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             (void)parse_http_request("GET /api/v1/catalog?all=true "
                                      "HTTP/1.1\r\nHost: localhost\r\n\r\n");
           },
           400, "query-string rejection") &&
       ok;
  ok = expect_http_error(
           [] {
             std::string oversized =
                 "GET /healthz HTTP/1.1\r\nHost: localhost\r\nX-Fill: ";
             oversized.append(kMaximumHttpRequestHeaderBytes, 'x');
             oversized.append("\r\n\r\n");
             (void)parse_http_request(oversized);
           },
           431, "8 KiB request cap") &&
       ok;

  try {
    const auto repo_root = discover_iinuji_repo_root();

    const auto symlink_fixture =
        std::filesystem::temp_directory_path() /
        ("iinuji-v2-prefix-symlink-" +
         std::to_string(static_cast<unsigned long long>(::getpid())));
    std::error_code fixture_error;
    std::filesystem::remove_all(symlink_fixture, fixture_error);
    const auto copy_fixture_file = [&](std::string_view relative) {
      const auto source = repo_root / std::filesystem::path(relative);
      const auto destination =
          symlink_fixture / std::filesystem::path(relative);
      std::filesystem::create_directories(destination.parent_path());
      std::filesystem::copy_file(source, destination,
                                 std::filesystem::copy_options::overwrite_existing);
    };
    copy_fixture_file(
        "src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/"
        "synthetic_periodic_chart_manifest.v1.report");
    copy_fixture_file(
        "src/config/benchmarks/synthetic_continuous_graph_v1/"
        "ujcamei.source.retrieval.channels.dsl");
    copy_fixture_file(
        "src/config/benchmarks/synthetic_continuous_graph_v1/"
        "ujcamei.source.splits.dsl");
    copy_fixture_file(
        "src/config/benchmarks/synthetic_continuous_graph_v2/artifacts/"
        "synthetic_quasiperiodic_chart_manifest.v2.report");
    const auto linked_prefix =
        symlink_fixture /
        "src/config/benchmarks/synthetic_continuous_graph_v2/data/"
        "development_prefix/SYN2ALPHASYN2USD/1d/"
        "SYN2ALPHASYN2USD-1d-all-years.csv";
    std::filesystem::create_directories(linked_prefix.parent_path());
    std::filesystem::create_symlink(
        repo_root /
            "src/config/benchmarks/synthetic_continuous_graph_v2/data/raw/"
            "SYN2ALPHASYN2USD/1d/SYN2ALPHASYN2USD-1d-all-years.csv",
        linked_prefix);
    bool symlink_rejected = false;
    try {
      const synthetic_chart_v2_repository_t unsafe_repository(symlink_fixture);
      (void)unsafe_repository;
    } catch (const synthetic_chart_error &) {
      symlink_rejected = true;
    }
    std::filesystem::remove_all(symlink_fixture, fixture_error);
    ok = require(symlink_rejected,
                 "v2 repository should reject a prefix symlink before it can "
                 "resolve to raw data") &&
         ok;

    const iinuji_http_application_t application(repo_root);
    const auto &repository = application.repository();
    const auto &v2_repository = application.v2_repository();

    ok = require(repository.served_point_count("SYNALPHASYNUSD", "1d") ==
                         kSyntheticServedAnchorEndExclusive &&
                     repository.maximum_served_raw_row() == 1117U,
                 "repository must stop before raw row 1118 / anchor 1088") &&
         ok;

    ok = require(
             v2_repository.served_point_count("SYN2ALPHASYN2USD", "1d") ==
                     3264U &&
                 v2_repository.served_point_count("SYN2BETASYN2USD", "3d") ==
                     1088U &&
                 v2_repository.served_point_count("SYN2GAMMASYN2USD", "1w") ==
                     467U &&
                 v2_repository.maximum_served_raw_row("SYN2ALPHASYN2USD",
                                                       "1d") == 3293U &&
                 v2_repository.maximum_served_raw_row("SYN2BETASYN2USD",
                                                       "3d") == 1097U &&
                 v2_repository.maximum_served_raw_row("SYN2GAMMASYN2USD",
                                                       "1w") == 470U,
             "v2 repository should bind exact development-prefix row counts") &&
         ok;

    const auto &v2_catalog = v2_repository.catalog_json();
    ok = require(
             contains(v2_catalog,
                      "\"schema_id\":\"iinuji.synthetic_chart_catalog.v2\"") &&
                 contains(v2_catalog,
                          "\"benchmark_id\":\"synthetic_continuous_graph_v2\"") &&
                 contains(v2_catalog,
                          "\"data_scope\":\"development_prefix_only\"") &&
                 contains(v2_catalog,
                          "\"development_prefix_row_counts\":{\"1d\":3294,"
                          "\"3d\":1098,\"1w\":471}") &&
                 contains(v2_catalog,
                          "\"served_anchor_end_exclusive\":3264") &&
                 contains(v2_catalog, "\"raw_source_served\":false") &&
                 contains(v2_catalog,
                          "\"id\":\"certified_development\",\"begin\":2880,"
                          "\"end_exclusive\":3264,\"served\":true") &&
                 contains(v2_catalog,
                          "\"id\":\"purge_3\",\"begin\":3264,"
                          "\"end_exclusive\":3328,\"served\":false") &&
                 contains(v2_catalog,
                          "\"id\":\"test_holdout\",\"begin\":3328,"
                          "\"end_exclusive\":4096,\"served\":false") &&
                 contains(v2_catalog, "/data/development_prefix/") &&
                 !contains(v2_catalog, "/data/raw/"),
             "v2 catalog should expose only the frozen development-prefix "
             "scope and exact boundaries") &&
         ok;

    const auto &v2_daily =
        v2_repository.chart_json("SYN2ALPHASYN2USD", "1d");
    const auto &v2_three_day =
        v2_repository.chart_json("SYN2ALPHASYN2USD", "3d");
    const auto &v2_weekly =
        v2_repository.chart_json("SYN2ALPHASYN2USD", "1w");
    ok = require(
             contains(v2_daily,
                      "\"schema_id\":\"iinuji.synthetic_chart.v2\"") &&
                 contains(v2_daily, "\"anchor_end_exclusive\":3264") &&
                 contains(v2_daily, "\"last_point_anchor\":3263") &&
                 contains(v2_daily, "\"raw_row_end_exclusive\":3294") &&
                 contains(v2_daily, "\"point_count\":3264") &&
                 contains(v2_daily, ",[3263,") &&
                 !contains(v2_daily, ",[3264,") &&
                 count_point_tuples(v2_daily) == 3264U &&
                 contains(v2_three_day, "\"last_point_anchor\":3261") &&
                 contains(v2_three_day,
                          "\"raw_row_end_exclusive\":1098") &&
                 count_point_tuples(v2_three_day) == 1088U &&
                 contains(v2_weekly, "\"last_point_anchor\":3262") &&
                 contains(v2_weekly, "\"raw_row_end_exclusive\":471") &&
                 count_point_tuples(v2_weekly) == 467U &&
                 contains(v2_daily, "/data/development_prefix/") &&
                 !contains(v2_daily, "/data/raw/") &&
                 !contains(v2_three_day, "/data/raw/") &&
                 !contains(v2_weekly, "/data/raw/"),
             "v2 chart payloads should use only exact prefix rows and never "
             "cross anchor 3264") &&
         ok;

    const auto &catalog = repository.catalog_json();
    ok =
        require(
            contains(catalog,
                     "\"schema_id\":\"iinuji.synthetic_chart_catalog.v1\"") &&
                contains(catalog, "\"row_count\":1200") &&
                contains(catalog, "\"accepted_anchor_count\":1170") &&
                contains(catalog, "\"served_anchor_end_exclusive\":1088") &&
                contains(catalog, "\"test_holdout_served\":false") &&
                contains(catalog, "\"id\":\"train\",\"begin\":0,"
                                  "\"end_exclusive\":730,\"served\":true") &&
                contains(catalog, "\"id\":\"embargo\",\"begin\":730,"
                                  "\"end_exclusive\":760,\"served\":true") &&
                contains(catalog, "\"id\":\"evaluation\",\"begin\":760,"
                                  "\"end_exclusive\":1088,\"served\":true") &&
                contains(catalog, "\"id\":\"test_holdout\",\"begin\":1088,"
                                  "\"end_exclusive\":1170,\"served\":false"),
            "catalog should expose the exact served and sealed split ranges") &&
        ok;
    ok = require(
             count_occurrences(catalog, "\"identical_by_row\":true") == 3U &&
                 count_occurrences(catalog, "\"maximum_absolute_delta\":0") ==
                     3U,
             "catalog should expose exact cross-interval OHLC duplication") &&
         ok;
    ok = require(contains(catalog, "\"id\":\"1d\",\"input_length\":30,"
                                   "\"step_days\":1") &&
                     contains(catalog, "\"id\":\"3d\",\"input_length\":10,"
                                       "\"step_days\":3") &&
                     contains(catalog, "\"id\":\"1w\",\"input_length\":4,"
                                       "\"step_days\":7"),
                 "catalog should reflect the active channels DSL") &&
         ok;

    const auto &chart = repository.chart_json("SYNALPHASYNUSD", "1d");
    ok = require(
             contains(chart, "\"schema_id\":\"iinuji.synthetic_chart.v1\"") &&
                 contains(chart, "\"anchor_begin\":0") &&
                 contains(chart, "\"anchor_end_exclusive\":1088") &&
                 contains(chart, "\"raw_row_begin\":30") &&
                 contains(chart, "\"raw_row_end_exclusive\":1118") &&
                 contains(chart, "\"point_count\":1088") &&
                 contains(chart, "\"test_holdout_served\":false") &&
                 contains(chart, "\"points\":[[0,1580428800000,") &&
                 contains(chart, ",[1087,") && !contains(chart, ",[1088,") &&
                 count_point_tuples(chart) == 1088U,
             "chart payload should map anchors to row 30+anchor and truncate "
             "holdout") &&
         ok;

    const http_request_t catalog_request{
        http_method_e::get, "/api/v1/catalog", "HTTP/1.1", {}};
    const auto catalog_response = application.route(catalog_request);
    ok = require(catalog_response.status == 200 &&
                     catalog_response.content_type ==
                         "application/json; charset=utf-8" &&
                     catalog_response.body == catalog,
                 "catalog route should return the validated cached document") &&
         ok;

    const auto benchmark_catalog_response = application.route(
        {http_method_e::get, "/api/v1/benchmarks", "HTTP/1.1", {}});
    ok = require(
             benchmark_catalog_response.status == 200 &&
                 benchmark_catalog_response.body ==
                     application.benchmark_catalog_json() &&
                 contains(benchmark_catalog_response.body,
                          "\"default_benchmark_id\":"
                          "\"synthetic_continuous_graph_v1\"") &&
                 count_occurrences(benchmark_catalog_response.body,
                                   "\"test_holdout_served\":false") == 2U,
             "benchmark catalog should preserve v1 default and offer sealed v2") &&
         ok;

    const http_request_t v2_catalog_request{
        http_method_e::get,
        "/api/v1/catalog",
        "HTTP/1.1",
        {},
        {{"benchmark", "synthetic_continuous_graph_v2"}}};
    const auto v2_catalog_response = application.route(v2_catalog_request);
    ok = require(v2_catalog_response.status == 200 &&
                     v2_catalog_response.body == v2_catalog,
                 "benchmark query should select the fixed v2 catalog") &&
         ok;

    const auto v2_chart_response = application.route(
        {http_method_e::get,
         "/api/v1/chart/SYN2BETASYN2USD/1w",
         "HTTP/1.1",
         {},
         {{"benchmark", "synthetic_continuous_graph_v2"}}});
    const auto v1_identity_under_v2 = application.route(
        {http_method_e::get,
         "/api/v1/chart/SYNBETASYNUSD/1w",
         "HTTP/1.1",
         {},
         {{"benchmark", "synthetic_continuous_graph_v2"}}});
    const auto unknown_benchmark = application.route(
        {http_method_e::get,
         "/api/v1/catalog",
         "HTTP/1.1",
         {},
         {{"benchmark", "synthetic_continuous_graph_v3"}}});
    ok = require(v2_chart_response.status == 200 &&
                     contains(v2_chart_response.body,
                              "\"instrument\":\"SYN2BETASYN2USD\"") &&
                     v1_identity_under_v2.status == 404 &&
                     unknown_benchmark.status == 404,
                 "benchmark-specific allowlists should fail closed") &&
         ok;

    const auto chart_response = application.route(
        {http_method_e::get, "/api/v1/chart/SYNBETASYNUSD/3d", "HTTP/1.1", {}});
    ok = require(chart_response.status == 200 &&
                     contains(chart_response.body,
                              "\"instrument\":\"SYNBETASYNUSD\"") &&
                     contains(chart_response.body, "\"interval\":\"3d\""),
                 "chart route should serve a fixed allowlisted "
                 "instrument/interval") &&
         ok;

    const auto unknown_instrument = application.route(
        {http_method_e::get, "/api/v1/chart/UNKNOWN/1d", "HTTP/1.1", {}});
    const auto unknown_interval =
        application.route({http_method_e::get,
                           "/api/v1/chart/SYNALPHASYNUSD/4h",
                           "HTTP/1.1",
                           {}});
    const auto nested_chart =
        application.route({http_method_e::get,
                           "/api/v1/chart/SYNALPHASYNUSD/1d/extra",
                           "HTTP/1.1",
                           {}});
    const auto arbitrary_file =
        application.route({http_method_e::get, "/etc/passwd", "HTTP/1.1", {}});
    ok = require(unknown_instrument.status == 404 &&
                     unknown_interval.status == 404 &&
                     nested_chart.status == 404 && arbitrary_file.status == 404,
                 "router should fail closed outside its exact allowlists") &&
         ok;

    const auto root_response =
        application.route({http_method_e::get, "/", "HTTP/1.1", {}});
    const auto index_response =
        application.route({http_method_e::get, "/index.html", "HTTP/1.1", {}});
    const auto css_response =
        application.route({http_method_e::get, "/app.css", "HTTP/1.1", {}});
    const auto js_response =
        application.route({http_method_e::get, "/app.js", "HTTP/1.1", {}});
    const auto health_response =
        application.route({http_method_e::get, "/healthz", "HTTP/1.1", {}});
    ok = require(root_response.status == 200 && index_response.status == 200 &&
                     root_response.body == index_response.body &&
                     css_response.status == 200 && js_response.status == 200 &&
                     health_response.status == 200 &&
                     contains(health_response.body, "\"status\":\"ok\""),
                 "fixed static and health routes should be available") &&
         ok;
    ok = require(contains(root_response.body, "id=\"benchmark-select\"") &&
                     contains(js_response.body, "/api/v1/benchmarks") &&
                     contains(js_response.body,
                              "development_prefix_only") &&
                     !contains(js_response.body, "data/raw"),
                 "page assets should make benchmark selection available "
                 "without embedding a raw v2 path") &&
         ok;

    const auto head_wire = serialize_http_response(chart_response, true);
    const auto separator = head_wire.find("\r\n\r\n");
    ok = require(separator != std::string::npos &&
                     separator + 4U == head_wire.size() &&
                     contains(head_wire,
                              "Content-Length: " +
                                  std::to_string(chart_response.body.size()) +
                                  "\r\n"),
                 "HEAD serialization should retain GET Content-Length without "
                 "body") &&
         ok;

    const auto wire = serialize_http_response(catalog_response, false);
    ok = require(
             contains(wire, "Content-Security-Policy: default-src 'none'") &&
                 contains(wire, "X-Content-Type-Options: nosniff") &&
                 contains(wire, "X-Frame-Options: DENY") &&
                 contains(wire, "Referrer-Policy: no-referrer") &&
                 contains(wire, "Cross-Origin-Resource-Policy: same-origin") &&
                 !contains(wire, "Access-Control-Allow-Origin"),
             "responses should carry restrictive security headers and no "
             "CORS") &&
         ok;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] repository/application validation threw: "
              << error.what() << '\n';
    ok = false;
  }

  if (!ok) {
    return 1;
  }
  std::cout
      << "[PASS] iinuji HTML parser, catalog, routes, security, and holdout"
         " truncation\n";
  return 0;
}
