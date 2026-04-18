#pragma once

#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/views/common/base.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string_view ui_screen_key(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "F1";
  case ScreenMode::Workbench:
    return "F2";
  case ScreenMode::Runtime:
    return "F3";
  case ScreenMode::Lattice:
    return "F4";
  case ScreenMode::ShellLogs:
    return "F8";
  case ScreenMode::Config:
    return "F9";
  }
  return "Fn";
}

inline std::string_view ui_screen_name(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "Home";
  case ScreenMode::Workbench:
    return "Workbench";
  case ScreenMode::Runtime:
    return "Runtime";
  case ScreenMode::Lattice:
    return "Lattice";
  case ScreenMode::ShellLogs:
    return "Shell Logs";
  case ScreenMode::Config:
    return "Config";
  }
  return "Screen";
}

inline std::string_view ui_screen_status_name(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "HOME";
  case ScreenMode::Workbench:
    return "WORKBENCH";
  case ScreenMode::Runtime:
    return "RUNTIME";
  case ScreenMode::Lattice:
    return "LATTICE";
  case ScreenMode::ShellLogs:
    return "SHELL LOGS";
  case ScreenMode::Config:
    return "CONFIG";
  }
  return "SCREEN";
}

inline std::string ui_screen_tab_text(ScreenMode screen) {
  return std::string(ui_screen_key(screen)) + " " +
         std::string(ui_screen_status_name(screen));
}

inline std::string ui_screen_hint(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return "F1 home";
  case ScreenMode::Workbench:
    return "F2 workbench";
  case ScreenMode::Runtime:
    return "F3 runtime";
  case ScreenMode::Lattice:
    return "F4 lattice";
  case ScreenMode::ShellLogs:
    return "F8 shell logs";
  case ScreenMode::Config:
    return "F9 config";
  }
  return "Fn screen";
}

inline std::string ui_focusable_panel_title(std::string title, bool focused) {
  return focused ? " " + std::move(title) + " [focus] "
                 : " " + std::move(title) + " ";
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
