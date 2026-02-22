#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

#include "piaabo/dlogs.h"

#include "iinuji/iinuji_cmd/commands/iinuji.path.handlers.h"
#include "iinuji/iinuji_cmd/commands/iinuji.command.aliases.h"
#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void run_command(CmdState& st,
                        const std::string& raw,
                        const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& log_box) {
  const std::string cmd = raw;
  if (cmd.empty()) return;

  append_log(log_box, "$ " + cmd, "cmd", "#9ecfff");
  log_info("[iinuji_cmd.cmd] %s\n", cmd.c_str());

  std::istringstream iss(cmd);
  std::string a0;
  iss >> a0;
  a0 = to_lower_copy(a0);

  auto push_info = [&](const std::string& m) {
    append_log(log_box, m, "info", "#b8d8b8");
    log_info("[iinuji_cmd] %s\n", m.c_str());
  };
  auto push_warn = [&](const std::string& m) {
    append_log(log_box, m, "warn", "#ffd27f");
    log_warn("[iinuji_cmd] %s\n", m.c_str());
  };
  auto push_err = [&](const std::string& m) {
    append_log(log_box, m, "err", "#ff9ea1");
    log_err("[iinuji_cmd] %s\n", m.c_str());
  };
  auto append = [&](const std::string& text, const std::string& label, const std::string& color) {
    (void)color;
    append_log(log_box, text, label, color);
    log_info("[iinuji_cmd.%s] %s\n", label.empty() ? "log" : label.c_str(), text.c_str());
  };
  IinujiPathHandlers path_handlers{st};

  if (path_handlers.dispatch_text(cmd, push_info, push_warn, push_err, append)) return;

  const auto alias = command_aliases::resolve(cmd);
  if (alias.matched) {
    if (!path_handlers.dispatch_canonical_text(alias.canonical, push_info, push_warn, push_err, append)) {
      push_err("unsupported canonical alias target: " + alias.canonical);
    }
    return;
  }

  push_err("unknown command: " + a0);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
