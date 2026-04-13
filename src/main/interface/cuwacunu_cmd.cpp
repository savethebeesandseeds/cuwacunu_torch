#include <cstdio>

#include "iinuji/iinuji_cmd/app.h"
#include "piaabo/dlogs.h"

namespace {

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 2) {
    std::fprintf(stderr, "usage: %s [global_config_path]\n", argv[0]);
    return 2;
  }
  const char* global_config_path =
      (argc == 2 && argv[1] != nullptr && argv[1][0] != '\0')
          ? argv[1]
          : DEFAULT_GLOBAL_CONFIG_PATH;
  return cuwacunu::iinuji::iinuji_cmd::run(global_config_path);
}
