#include "iinuji/iinuji_cmd/app.h"
#include "piaabo/log/dlogs.h"

namespace {

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

} // namespace

int main(int argc, char **argv) {
  return cuwacunu::iinuji::iinuji_cmd::run_cuwacunu_cmd(argc, argv);
}
