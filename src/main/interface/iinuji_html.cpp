// SPDX-License-Identifier: MIT

#include "iinuji/html/iinuji_http_server.h"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct options_t {
  std::string bind_address{"127.0.0.1"};
  std::uint16_t port{8765U};
  std::filesystem::path repo_root;
  std::size_t max_requests{0U};
  bool repo_root_set{false};
  bool validate_only{false};
  bool help{false};
};

[[nodiscard]] std::string_view require_value(int argc, char **argv, int &index,
                                             std::string_view option) {
  if (index + 1 >= argc) {
    throw std::invalid_argument(std::string(option) + " requires a value");
  }
  ++index;
  return argv[index];
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view text,
                                      std::string_view option) {
  if (text.empty()) {
    throw std::invalid_argument(std::string(option) + " requires a value");
  }
  std::uint64_t value = 0U;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value, 10);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument(std::string(option) +
                                " requires an unsigned integer");
  }
  return value;
}

[[nodiscard]] options_t parse_options(int argc, char **argv) {
  options_t options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--help" || argument == "-h") {
      options.help = true;
    } else if (argument == "--validate-only") {
      options.validate_only = true;
    } else if (argument == "--bind") {
      options.bind_address =
          std::string(require_value(argc, argv, index, argument));
    } else if (argument.starts_with("--bind=")) {
      options.bind_address = std::string(argument.substr(7U));
    } else if (argument == "--port") {
      const auto value =
          parse_u64(require_value(argc, argv, index, argument), argument);
      if (value == 0U || value > std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument("--port must be in [1,65535]");
      }
      options.port = static_cast<std::uint16_t>(value);
    } else if (argument.starts_with("--port=")) {
      const auto value = parse_u64(argument.substr(7U), "--port");
      if (value == 0U || value > std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument("--port must be in [1,65535]");
      }
      options.port = static_cast<std::uint16_t>(value);
    } else if (argument == "--repo-root") {
      options.repo_root =
          std::filesystem::path(require_value(argc, argv, index, argument));
      options.repo_root_set = true;
    } else if (argument.starts_with("--repo-root=")) {
      options.repo_root = std::filesystem::path(argument.substr(12U));
      options.repo_root_set = true;
    } else if (argument == "--max-requests") {
      const auto value =
          parse_u64(require_value(argc, argv, index, argument), argument);
      if (value > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("--max-requests is too large");
      }
      options.max_requests = static_cast<std::size_t>(value);
    } else if (argument.starts_with("--max-requests=")) {
      const auto value = parse_u64(argument.substr(15U), "--max-requests");
      if (value > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("--max-requests is too large");
      }
      options.max_requests = static_cast<std::size_t>(value);
    } else {
      throw std::invalid_argument("unknown option: " + std::string(argument));
    }
  }
  if (options.bind_address.empty()) {
    throw std::invalid_argument("--bind cannot be empty");
  }
  return options;
}

[[nodiscard]] std::filesystem::path discover_from_process(int argc,
                                                          char **argv) {
  try {
    return cuwacunu::iinuji::html::discover_iinuji_repo_root();
  } catch (const std::exception &) {
    if (argc > 0 && argv[0] != nullptr && argv[0][0] != '\0') {
      std::error_code error;
      const auto executable = std::filesystem::canonical(argv[0], error);
      if (!error) {
        return cuwacunu::iinuji::html::discover_iinuji_repo_root(
            executable.parent_path());
      }
    }
    throw;
  }
}

void print_usage(std::ostream &output) {
  output << "Usage: iinuji_html [options]\n"
         << "\n"
         << "Serve the fixed synthetic chart inspector over dependency-free "
            "HTTP.\n"
         << "\n"
         << "Options:\n"
         << "  --bind ADDRESS       Numeric bind address (default: 127.0.0.1)\n"
         << "  --port PORT          TCP port in [1,65535] (default: 8765)\n"
         << "  --repo-root PATH     Cuwacunu repository root (otherwise "
            "discovered)\n"
         << "  --validate-only      Validate all inputs/assets, then exit\n"
         << "  --max-requests N     Exit after N requests; 0 means unlimited\n"
         << "  -h, --help           Show this help\n";
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.help) {
      print_usage(std::cout);
      return 0;
    }

    const auto repo_root = options.repo_root_set
                               ? options.repo_root
                               : discover_from_process(argc, argv);
    auto application =
        std::make_shared<cuwacunu::iinuji::html::iinuji_http_application_t>(
            repo_root);
    if (options.validate_only) {
      std::cout << "iinuji_html validation=pass"
                << " benchmarks=2"
                << " default_benchmark="
                << cuwacunu::iinuji::html::kSyntheticBenchmarkV1Id
                << " v1_served_anchor_end_exclusive="
                << cuwacunu::iinuji::html::kSyntheticServedAnchorEndExclusive
                << " v2_served_anchor_end_exclusive="
                << cuwacunu::iinuji::html::kSyntheticV2ServedAnchorEndExclusive
                << " v2_data_scope=development_prefix_only"
                << " test_holdouts_served=false\n";
      return 0;
    }

    const cuwacunu::iinuji::html::iinuji_http_server_config_t config{
        options.bind_address, options.port, options.max_requests};
    const cuwacunu::iinuji::html::iinuji_http_server_t server(config,
                                                              application);
    const bool ipv6 = options.bind_address.find(':') != std::string::npos;
    std::cout << "iinuji_html listening=http://" << (ipv6 ? "[" : "")
              << options.bind_address << (ipv6 ? "]" : "") << ':'
              << options.port
              << " repo_root=" << application->repository().repo_root().string()
              << " benchmarks=2"
              << " default_benchmark="
              << cuwacunu::iinuji::html::kSyntheticBenchmarkV1Id
              << " v2_data_scope=development_prefix_only"
              << " test_holdouts_served=false\n";
    std::cout.flush();
    (void)server.run();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "iinuji_html error: " << error.what() << '\n';
    return 2;
  }
}
