// SPDX-License-Identifier: MIT

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "hero/marshal_hero/marshal/runtime_hero_handoff.h"

namespace {

using cuwacunu::hero::marshal::marshal_runtime_dry_run_request_t;
using cuwacunu::hero::marshal::materialize_runtime_handoff_object;

[[noreturn]] void usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " CONFIG TARGET_ID WAVE_TARGET WAVE_MODE SOURCE_RANGE BEGIN END "
               "DRY_RUN POLICY_PATH [KEY=PATH ...]\n";
  std::exit(2);
}

std::size_t parse_size(const std::string &text, const char *field) {
  std::size_t pos = 0;
  const unsigned long long value = std::stoull(text, &pos);
  if (pos != text.size()) {
    throw std::runtime_error(std::string("invalid ") + field + ": " + text);
  }
  return static_cast<std::size_t>(value);
}

bool parse_bool(const std::string &text) {
  if (text == "true" || text == "1") {
    return true;
  }
  if (text == "false" || text == "0") {
    return false;
  }
  throw std::runtime_error("DRY_RUN must be true|false|1|0");
}

std::pair<std::string, std::string> parse_kv(const std::string &text) {
  const std::size_t eq = text.find('=');
  if (eq == std::string::npos || eq == 0U || eq + 1U >= text.size()) {
    throw std::runtime_error("model input must be KEY=PATH: " + text);
  }
  return {text.substr(0, eq), text.substr(eq + 1U)};
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc < 10) {
      usage(argv[0]);
    }

    marshal_runtime_dry_run_request_t request{};
    request.config_path = std::filesystem::path(argv[1]).lexically_normal();
    request.target_id = argv[2];
    request.wave_target = argv[3];
    request.wave_mode = argv[4];
    request.source_range = argv[5];
    const std::size_t begin = parse_size(argv[6], "BEGIN");
    const std::size_t end = parse_size(argv[7], "END");
    const bool dry_run = parse_bool(argv[8]);
    const std::filesystem::path policy_path =
        std::filesystem::path(argv[9]).lexically_normal();

    if (begin >= end) {
      throw std::runtime_error("BEGIN must be less than END");
    }
    if (request.source_range == "anchor_index") {
      request.anchor_index_begin = begin;
      request.anchor_index_end = end;
    } else if (request.source_range == "source_key") {
      request.source_key_begin = static_cast<std::int64_t>(begin);
      request.source_key_end = static_cast<std::int64_t>(end);
    } else {
      throw std::runtime_error("SOURCE_RANGE must be anchor_index|source_key");
    }

    for (int i = 10; i < argc; ++i) {
      const auto [key, value] = parse_kv(argv[i]);
      request.model_state_inputs[key] =
          std::filesystem::path(value).lexically_normal().string();
    }

    std::string handoff_digest;
    const std::filesystem::path handoff_path =
        materialize_runtime_handoff_object(request, dry_run, policy_path,
                                           &handoff_digest);

    std::cout << "runtime_handoff_path=" << handoff_path.string() << "\n";
    std::cout << "runtime_handoff_digest=" << handoff_digest << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "emit_runtime_handoff: " << e.what() << "\n";
    return 1;
  }
}
