// SPDX-License-Identifier: MIT
#pragma once

#include <sstream>
#include <string>
#include <string_view>

#include "piaabo/digest/sha256.h"

namespace cuwacunu::hero::marshal {

namespace digest_detail {

[[nodiscard]] inline std::string sha256_hex(std::string_view input) {
  return cuwacunu::piaabo::digest::sha256_hex(input);
}

} // namespace digest_detail

[[nodiscard]] inline std::string
marshal_digest_for_text(const std::string &domain, const std::string &text) {
  std::ostringstream input;
  input << "kikijyeba.marshal.digest.sha256.v1\n";
  input << "domain=" << domain.size() << ":" << domain << "\n";
  input << "text=" << text.size() << ":" << text << "\n";
  return digest_detail::sha256_hex(input.str());
}

[[nodiscard]] inline bool marshal_digest_is_strong_hex(const std::string &hex) {
  return cuwacunu::piaabo::digest::is_sha256_hex(hex);
}

} // namespace cuwacunu::hero::marshal
