#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>

#include "HERO/hero_config/hero.config.h"
#include "main/hero/config_hero/hero_config_commands.h"
#include "main/hero/config_hero/hero_config_store.h"

namespace {

constexpr const char* kDefaultHeroConfigPath =
    "/cuwacunu/src/config/instructions/hero.config.dsl";

void print_cli_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--config <hero_config_dsl>] [--once <command>] [--repl]\n"
            << "  default mode: JSON-RPC over stdio\n"
            << "  --repl: legacy human command shell\n";
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
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

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string normalize_protocol_layer(std::string_view in) {
  return lowercase_copy(trim_ascii(in));
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path = kDefaultHeroConfigPath;
  std::string once_command;
  bool repl = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
      continue;
    }
    if (arg == "--once" && i + 1 < argc) {
      once_command = argv[++i];
      continue;
    }
    if (arg == "--repl") {
      repl = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_cli_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_cli_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(config_path);
  std::string load_error;
  if (!store.load(&load_error)) {
    std::cout << "err\tstartup\t" << load_error << "\n";
    std::cout << "end\n";
    std::cout.flush();
    return 2;
  }

  const std::string protocol_layer =
      normalize_protocol_layer(store.get_or_default("protocol_layer"));
  if (protocol_layer == "https/sse") {
    std::cerr << cuwacunu::hero::config::kProtocolLayerHttpsSseFailFastMessage
              << "\n";
    return 2;
  }
  if (!protocol_layer.empty() && protocol_layer != "stdio") {
    std::cerr << "Unsupported protocol_layer: " << protocol_layer
              << " (allowed: STDIO|HTTPS/SSE)\n";
    return 2;
  }

  int exit_code = 0;
  if (!once_command.empty()) {
    bool should_exit = false;
    const bool ok = cuwacunu::hero::mcp::execute_command_line(
        once_command, &store, &should_exit);
    exit_code = ok ? 0 : 1;
    return exit_code;
  }

  if (!repl) {
    cuwacunu::hero::mcp::run_jsonrpc_stdio_loop(&store);
    return 0;
  }

  cuwacunu::hero::mcp::emit_startup_banner(store);

  std::string line;
  while (true) {
    if (repl) {
      std::cout << "hero.config.mcp> ";
      std::cout.flush();
    }
    if (!std::getline(std::cin, line)) break;
    bool should_exit = false;
    const bool ok =
        cuwacunu::hero::mcp::execute_command_line(line, &store, &should_exit);
    if (!ok) exit_code = 1;
    if (should_exit) break;
  }

  return exit_code;
}
