// test_iinuji_cmd.cpp
#include "iinuji/iinuji_cmd/app.h"
#include "piaabo/dlogs.h"

namespace {

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

}  // namespace

int main() {
  return cuwacunu::iinuji::iinuji_cmd::run("/cuwacunu/src/config/.config");
}
