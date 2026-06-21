// SPDX-License-Identifier: MIT
#pragma once

#include "kikijyeba/protocol/protocol_variant.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace cuwacunu::tests::kikijyeba::protocol_fixture {

[[nodiscard]] inline std::filesystem::path canonical_cwu_02v_protocol_path() {
  return std::filesystem::path{
      "/cuwacunu/src/config/kikijyeba.protocol.cwu_02v.dsl"};
}

[[nodiscard]] inline std::string
read_fixture_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to read canonical fixture text: " +
                             path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::string canonical_cwu_02v_protocol_text() {
  return read_fixture_text(canonical_cwu_02v_protocol_path());
}

[[nodiscard]] inline cuwacunu::kikijyeba::protocol::
    protocol_no_lookahead_contract_t
    canonical_cwu_02v_no_lookahead_contract() {
  namespace protocol = cuwacunu::kikijyeba::protocol;
  const auto variant = protocol::decode_protocol_variant_from_dsl(
      canonical_cwu_02v_protocol_text());
  if (variant.protocol_id != "cwu_02v") {
    throw std::runtime_error(
        "canonical cwu_02v protocol fixture has protocol " +
        variant.protocol_id);
  }
  if (!protocol::protocol_no_lookahead_contract_declared(
          variant.no_lookahead_contract)) {
    throw std::runtime_error(
        "canonical cwu_02v protocol fixture lacks NO_LOOKAHEAD_CONTRACT");
  }
  return variant.no_lookahead_contract;
}

[[nodiscard]] inline std::string canonical_cwu_02v_no_lookahead_contract_id() {
  return canonical_cwu_02v_no_lookahead_contract().contract_id;
}

[[nodiscard]] inline std::string
canonical_cwu_02v_no_lookahead_contract_digest() {
  return canonical_cwu_02v_no_lookahead_contract().contract_digest;
}

[[nodiscard]] inline std::string
canonical_cwu_02v_no_lookahead_contract_block_text() {
  const auto text = canonical_cwu_02v_protocol_text();
  const auto start = text.find("NO_LOOKAHEAD_CONTRACT");
  if (start == std::string::npos) {
    throw std::runtime_error(
        "canonical cwu_02v protocol fixture lacks NO_LOOKAHEAD_CONTRACT text");
  }
  const auto open = text.find('{', start);
  if (open == std::string::npos) {
    throw std::runtime_error(
        "canonical cwu_02v NO_LOOKAHEAD_CONTRACT lacks opening brace");
  }
  int depth = 0;
  std::size_t close = std::string::npos;
  for (std::size_t i = open; i < text.size(); ++i) {
    if (text[i] == '{') {
      ++depth;
    } else if (text[i] == '}') {
      --depth;
      if (depth == 0) {
        close = i;
        break;
      }
    }
  }
  if (close == std::string::npos) {
    throw std::runtime_error(
        "canonical cwu_02v NO_LOOKAHEAD_CONTRACT lacks closing brace");
  }
  const auto semi = text.find(';', close);
  if (semi == std::string::npos) {
    throw std::runtime_error(
        "canonical cwu_02v NO_LOOKAHEAD_CONTRACT lacks terminator");
  }
  return text.substr(start, semi - start + 1) + "\n";
}

} // namespace cuwacunu::tests::kikijyeba::protocol_fixture
