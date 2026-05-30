#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <ncursesw/ncurses.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/action_registry.h"
#include "iinuji/iinuji_cmd/home_showcase.h"
#include "iinuji/iinuji_cmd/views.h"
#include "iinuji/iinuji_keys.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "iinuji/primitives/choice_menu.h"
#include "iinuji/primitives/editor.h"
#include "iinuji/primitives/grid_container.h"
#include "iinuji/primitives/image.h"
#include "iinuji/primitives/panel.h"
#include "iinuji/primitives/plot_box.h"
#include "iinuji/primitives/text_box.h"

#ifndef BUTTON_SHIFT
#define BUTTON_SHIFT 0
#endif
#ifndef BUTTON_CTRL
#define BUTTON_CTRL 0
#endif
#ifndef BUTTON_ALT
#define BUTTON_ALT 0
#endif
#ifndef BUTTON1_CLICKED
#define BUTTON1_CLICKED 0
#endif
#ifndef BUTTON1_PRESSED
#define BUTTON1_PRESSED 0
#endif
#ifndef BUTTON1_DOUBLE_CLICKED
#define BUTTON1_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON1_TRIPLE_CLICKED
#define BUTTON1_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON4_PRESSED
#define BUTTON4_PRESSED 0
#endif
#ifndef BUTTON4_CLICKED
#define BUTTON4_CLICKED 0
#endif
#ifndef BUTTON4_DOUBLE_CLICKED
#define BUTTON4_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON4_TRIPLE_CLICKED
#define BUTTON4_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON5_PRESSED
#define BUTTON5_PRESSED 0
#endif
#ifndef BUTTON5_CLICKED
#define BUTTON5_CLICKED 0
#endif
#ifndef BUTTON5_DOUBLE_CLICKED
#define BUTTON5_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON5_TRIPLE_CLICKED
#define BUTTON5_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON6_PRESSED
#define BUTTON6_PRESSED 0
#endif
#ifndef BUTTON7_PRESSED
#define BUTTON7_PRESSED 0
#endif

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class SplashSnapshotMode : std::uint8_t {
  None = 0,
  Bootstrap = 1,
  Farewell = 2,
};

enum class HomeVisualSnapshotMode : std::uint8_t {
  None = 0,
  Full = 1,
  Image = 2,
  Animation = 3,
};

struct CmdOptions {
  std::string invoked_command_name{"cuwacunu_cmd"};
  std::filesystem::path global_config_path{"/cuwacunu/src/config/.config"};
  ScreenMode initial_screen{ScreenMode::Home};
  SplashSnapshotMode splash_snapshot{SplashSnapshotMode::None};
  HomeVisualSnapshotMode visual_snapshot_mode{HomeVisualSnapshotMode::None};
  bool snapshot{false};
  bool menu{false};
  bool actions{false};
  bool command_catalog{false};
  bool visual_snapshot{false};
  bool zoom{false};
  bool help{false};
  bool version{false};
  std::vector<std::string> commands{};
};

inline std::string invoked_command_name_from_argv0(const char *argv0) {
  if (argv0 == nullptr || std::string_view{argv0}.empty())
    return "cuwacunu_cmd";
  const std::filesystem::path path{argv0};
  const std::string filename = path.filename().string();
  if (filename == "cuwacunu_cmd" || filename == "cuwacunu.cmd" ||
      filename == "iinuji_cmd")
    return filename;
  return "cuwacunu_cmd";
}

inline bool parse_screen_name(const std::string &name, ScreenMode &screen) {
  const std::string normalized = normalize_command(name);
  if (normalized == "home" || normalized == "iinuji.screen.home" ||
      normalized == "1" || normalized == "f1") {
    screen = ScreenMode::Home;
    return true;
  }
  if (normalized == "workbench" || normalized == "work" || normalized == "w" ||
      normalized == "marshal" || normalized == "inbox" ||
      normalized == "human" || normalized == "iinuji.screen.workbench" ||
      normalized == "iinuji.screen.marshal" ||
      normalized == "iinuji.screen.inbox" ||
      normalized == "iinuji.screen.human" || normalized == "2" ||
      normalized == "f2") {
    screen = ScreenMode::Workbench;
    return true;
  }
  if (normalized == "runtime" || normalized == "run" || normalized == "rt" ||
      normalized == "iinuji.screen.runtime" || normalized == "3" ||
      normalized == "f3") {
    screen = ScreenMode::Runtime;
    return true;
  }
  if (normalized == "lattice" || normalized == "lat" || normalized == "proof" ||
      normalized == "iinuji.screen.lattice" || normalized == "4" ||
      normalized == "f4") {
    screen = ScreenMode::Lattice;
    return true;
  }
  if (normalized == "logs" || normalized == "log" || normalized == "shell" ||
      normalized == "shell logs" || normalized == "shell log" ||
      normalized == "shelllogs" || normalized == "shell-logs" ||
      normalized == "shell-log" || normalized == "shell_logs" ||
      normalized == "iinuji.screen.logs" ||
      normalized == "iinuji.screen.shell logs" ||
      normalized == "iinuji.screen.shelllogs" ||
      normalized == "iinuji.screen.shell-logs" ||
      normalized == "iinuji.screen.shell_logs" || normalized == "events" ||
      normalized == "8" || normalized == "f8") {
    screen = ScreenMode::Logs;
    return true;
  }
  if (normalized == "config" || normalized == "cfg" ||
      normalized == "iinuji.screen.config" || normalized == "9" ||
      normalized == "f9") {
    screen = ScreenMode::Config;
    return true;
  }
  return false;
}

inline bool parse_splash_snapshot_name(const std::string &name,
                                       SplashSnapshotMode &mode) {
  const std::string normalized = normalize_command(name);
  if (normalized == "bootstrap" || normalized == "boot" ||
      normalized == "loading" || normalized == "load") {
    mode = SplashSnapshotMode::Bootstrap;
    return true;
  }
  if (normalized == "farewell" || normalized == "closing" ||
      normalized == "close" || normalized == "good luck" ||
      normalized == "good-luck" || normalized == "good_luck" ||
      normalized == "goodluck") {
    mode = SplashSnapshotMode::Farewell;
    return true;
  }
  return false;
}

inline std::string lowercase_ascii(std::string value) {
  for (char &ch : value)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return value;
}

inline bool positional_arg_looks_like_global_config_path(std::string_view arg) {
  if (arg.empty())
    return false;
  const std::filesystem::path path{std::string(arg)};
  const std::string filename = lowercase_ascii(path.filename().string());
  const std::string value = lowercase_ascii(std::string(arg));
  return filename == ".config" ||
         (value.size() >= 7u && value.rfind(".config") == value.size() - 7u) ||
         (value.size() >= 5u && value.rfind(".conf") == value.size() - 5u);
}

inline bool
positional_arg_looks_like_managed_config_path(std::string_view arg) {
  if (arg.empty() || positional_arg_looks_like_global_config_path(arg))
    return false;

  const std::filesystem::path path{std::string(arg)};
  const std::string extension = lowercase_ascii(path.extension().string());
  if (managed_config_path_extension(extension))
    return true;

  std::error_code ec{};
  if (std::filesystem::exists(path, ec))
    return true;
  return arg.find('/') != std::string_view::npos ||
         arg.find('\\') != std::string_view::npos;
}

inline void apply_positional_option(CmdOptions &opts, const std::string &arg) {
  ScreenMode screen = opts.initial_screen;
  if (parse_screen_name(arg, screen)) {
    opts.initial_screen = screen;
    return;
  }
  if (positional_arg_looks_like_global_config_path(arg)) {
    opts.global_config_path = arg;
    return;
  }
  if (positional_arg_looks_like_managed_config_path(arg)) {
    opts.commands.push_back("config file " + arg);
    return;
  }
  opts.commands.push_back(arg);
}

inline CmdOptions parse_options(int argc, char **argv) {
  CmdOptions opts{};
  opts.invoked_command_name =
      invoked_command_name_from_argv0(argc > 0 ? argv[0] : nullptr);
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i] == nullptr ? std::string{} : argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.help = true;
    } else if (arg == "--version" || arg == "-V" || arg == "-v") {
      opts.version = true;
    } else if (arg == "--menu") {
      opts.snapshot = true;
      opts.menu = true;
    } else if (arg == "--actions" || arg == "--palette") {
      opts.snapshot = true;
      opts.actions = true;
    } else if (arg == "--commands" || arg == "--catalog") {
      opts.snapshot = true;
      opts.command_catalog = true;
    } else if (arg == "--visual" || arg == "--home-visual" ||
               arg == "--waajacamaya") {
      opts.snapshot = true;
      opts.visual_snapshot = true;
      opts.visual_snapshot_mode = HomeVisualSnapshotMode::Full;
    } else if (arg == "--image") {
      opts.snapshot = true;
      opts.visual_snapshot = true;
      opts.visual_snapshot_mode = HomeVisualSnapshotMode::Image;
    } else if (arg == "--animation") {
      opts.snapshot = true;
      opts.visual_snapshot = true;
      opts.visual_snapshot_mode = HomeVisualSnapshotMode::Animation;
    } else if (arg == "--zoom" || arg == "--full") {
      opts.zoom = true;
    } else if ((arg == "--run" || arg == "--command") && i + 1 < argc) {
      opts.commands.push_back(argv[++i] == nullptr ? std::string{} : argv[i]);
    } else if (arg.rfind("--run=", 0) == 0) {
      opts.commands.push_back(arg.substr(6u));
    } else if (arg.rfind("--command=", 0) == 0) {
      opts.commands.push_back(arg.substr(10u));
    } else if (arg == "--splash") {
      opts.snapshot = true;
      SplashSnapshotMode parsed = SplashSnapshotMode::Bootstrap;
      if (i + 1 < argc && argv[i + 1] != nullptr) {
        const std::string next = argv[i + 1];
        if (i + 2 < argc && argv[i + 2] != nullptr &&
            parse_splash_snapshot_name(next + " " + std::string(argv[i + 2]),
                                       parsed)) {
          opts.splash_snapshot = parsed;
          i += 2;
        } else if (parse_splash_snapshot_name(next, parsed)) {
          opts.splash_snapshot = parsed;
          ++i;
        } else {
          opts.splash_snapshot = SplashSnapshotMode::Bootstrap;
        }
      } else {
        opts.splash_snapshot = SplashSnapshotMode::Bootstrap;
      }
    } else if (arg == "--bootstrap" || arg == "--boot") {
      opts.snapshot = true;
      opts.splash_snapshot = SplashSnapshotMode::Bootstrap;
    } else if (arg == "--farewell" || arg == "--closing") {
      opts.snapshot = true;
      opts.splash_snapshot = SplashSnapshotMode::Farewell;
    } else if (arg.rfind("--splash=", 0) == 0) {
      opts.snapshot = true;
      SplashSnapshotMode parsed = SplashSnapshotMode::Bootstrap;
      if (parse_splash_snapshot_name(arg.substr(9u), parsed))
        opts.splash_snapshot = parsed;
      else
        opts.splash_snapshot = SplashSnapshotMode::Bootstrap;
    } else if (arg == "--snapshot") {
      opts.snapshot = true;
    } else if (arg == "--global-config" && i + 1 < argc) {
      opts.global_config_path = argv[++i];
    } else if (arg.rfind("--global-config=", 0) == 0) {
      opts.global_config_path = arg.substr(16u);
    } else if (arg == "--screen" && i + 1 < argc) {
      ScreenMode parsed = opts.initial_screen;
      if (parse_screen_name(argv[++i], parsed))
        opts.initial_screen = parsed;
    } else if (arg.rfind("--screen=", 0) == 0) {
      ScreenMode parsed = opts.initial_screen;
      if (parse_screen_name(arg.substr(9u), parsed))
        opts.initial_screen = parsed;
    } else if (!arg.empty() && arg.front() != '-') {
      apply_positional_option(opts, arg);
    }
  }
  return opts;
}

inline void append_cli_help_section_header(std::ostream &out,
                                           std::string_view title) {
  out << detail_section_rule() << "\n"
      << title << "\n"
      << detail_section_rule() << "\n";
}

inline void print_help(std::ostream &out) {
  out << "usage: cuwacunu_cmd [PATH|SCREEN|COMMAND] "
         "[--help|--version] "
         "[--global-config PATH|=PATH] [--screen NAME|=NAME] "
         "[--run|--command COMMAND] "
         "[--snapshot] "
         "[--menu|--actions|--palette|--commands|--catalog|--visual|"
         "--home-visual|--image|--animation|--waajacamaya|"
         "--splash[=bootstrap|boot|loading|farewell|closing|close|good-luck]|"
         "--bootstrap|--boot|--farewell|--closing] "
         "[--zoom|--full]\n";
  out << "       cuwacunu.cmd and iinuji_cmd also open this command shell.\n";
  out << "\n";
  append_cli_help_section_header(out, "Screens");
  out << "  F1 home        animated Home showcase\n";
  out << "  F2 workbench   core workspace\n";
  out << "  F3 runtime     Runtime device/job evidence\n";
  out << "  F4 lattice     Lattice target proof preview\n";
  out << "  F8 shell logs  shell diagnostics and command/status feed\n";
  out << "  F9 config      managed Config catalog preview\n";
  out << "\n";
  append_cli_help_section_header(out, "Options");
  out << "  --help, -h                   print this command guide\n";
  out << "  --version, -V/-v             print command identity and aliases\n";
  out << "  --global-config PATH|=PATH   read GUI asset and log defaults\n";
  out << "  --screen NAME|=NAME          start on a screen: home, workbench, "
         "runtime, lattice, shell logs, config\n";
  out << "                               aliases: work/w, run/rt, lat/proof, "
         "log/shell/events, cfg\n";
  out << "  --run COMMAND                dispatch a command before rendering "
         "or "
         "entering the TUI\n";
  out << "  --command COMMAND            alias for --run\n";
  out << "  --snapshot                   render a non-interactive terminal "
         "snapshot\n";
  out << "                               in prompt commands, --snapshot "
         "renders "
         "menu/actions/visual/splash/full/screen utilities\n";
  out << "  --menu                       render the menu overlay and exit\n";
  out << "  --actions, --palette         render the action palette and exit\n";
  out << "  --commands, --catalog        print the active command catalog\n";
  out << "  --visual, --home-visual      render Home image/APNG preview "
         "frames\n";
  out << "  --image                      render Home still image preview\n";
  out << "  --animation                  render Home APNG motion preview\n";
  out << "  --waajacamaya                render the Home visual preview\n";
  out << "  --splash[=bootstrap|boot|loading|farewell|closing|close|"
         "good-luck] render "
         "splash asset diagnostics\n";
  out << "  --splash bootstrap|boot|loading|farewell|closing|close|"
         "good-luck spaced form of splash diagnostics\n";
  out << "  --bootstrap, --boot          render bootstrap splash diagnostics\n";
  out << "  --farewell, --closing        render closing splash diagnostics\n";
  out << "  --zoom, --full               start supported screens in full panel "
         "mode\n";
  out << "  PATH|SCREEN|COMMAND          positional shortcut for path-like "
         "config files, screen names, or commands\n";
  out << "                               .config/.conf paths are global config "
         "paths; managed config paths open Config\n";
  out << "\n";
  append_cli_help_section_header(out, "Examples");
  out << "  cuwacunu_cmd runtime --snapshot\n";
  out << "  cuwacunu.cmd runtime --snapshot\n";
  out << "  iinuji_cmd runtime --snapshot\n";
  out << "  cuwacunu_cmd \"show shell logs\" --snapshot\n";
  out << "  cuwacunu.cmd \"show shell logs\" --snapshot\n";
  out << "  cuwacunu_cmd --snapshot --screen home\n";
  out << "  cuwacunu.cmd --snapshot --screen home\n";
  out << "  cuwacunu_cmd --visual\n";
  out << "  cuwacunu.cmd --visual\n";
  out << "  cuwacunu_cmd --version\n";
  out << "  cuwacunu.cmd --version\n";
  out << "  cuwacunu_cmd --home-visual\n";
  out << "  cuwacunu.cmd --home-visual\n";
  out << "  cuwacunu_cmd --image\n";
  out << "  cuwacunu.cmd --image\n";
  out << "  cuwacunu_cmd --waajacamaya\n";
  out << "  cuwacunu.cmd --waajacamaya\n";
  out << "  iinuji_cmd --visual\n";
  out << "  cuwacunu_cmd --animation\n";
  out << "  cuwacunu.cmd --animation\n";
  out << "  cuwacunu_cmd --bootstrap\n";
  out << "  cuwacunu_cmd --boot\n";
  out << "  cuwacunu.cmd --bootstrap\n";
  out << "  cuwacunu_cmd --splash=boot\n";
  out << "  cuwacunu.cmd --splash=boot\n";
  out << "  cuwacunu_cmd --splash loading\n";
  out << "  cuwacunu.cmd --splash loading\n";
  out << "  iinuji_cmd --splash loading\n";
  out << "  cuwacunu_cmd --splash farewell\n";
  out << "  cuwacunu.cmd --splash farewell\n";
  out << "  cuwacunu_cmd --splash=farewell\n";
  out << "  cuwacunu.cmd --splash=farewell\n";
  out << "  cuwacunu_cmd --splash close\n";
  out << "  cuwacunu.cmd --splash close\n";
  out << "  cuwacunu_cmd --splash=good-luck\n";
  out << "  cuwacunu.cmd --splash=good-luck\n";
  out << "  cuwacunu_cmd --splash good luck\n";
  out << "  cuwacunu.cmd --splash good luck\n";
  out << "  iinuji_cmd --closing\n";
  out << "  cuwacunu_cmd --commands\n";
  out << "  cuwacunu.cmd --commands\n";
  out << "  cuwacunu_cmd --catalog\n";
  out << "  cuwacunu.cmd --catalog\n";
  out << "  iinuji_cmd --commands\n";
  out << "  cuwacunu_cmd --menu --run runtime\n";
  out << "  cuwacunu.cmd --menu --run runtime\n";
  out << "  cuwacunu_cmd --snapshot --menu\n";
  out << "  cuwacunu.cmd --snapshot --menu\n";
  out << "  cuwacunu_cmd --snapshot --actions\n";
  out << "  cuwacunu.cmd --snapshot --actions\n";
  out << "  cuwacunu_cmd --snapshot --catalog\n";
  out << "  cuwacunu.cmd --snapshot --catalog\n";
  out << "  cuwacunu_cmd --snapshot --visual\n";
  out << "  cuwacunu.cmd --snapshot --visual\n";
  out << "  cuwacunu_cmd --snapshot --image\n";
  out << "  cuwacunu.cmd --snapshot --image\n";
  out << "  cuwacunu_cmd --snapshot --animation\n";
  out << "  cuwacunu.cmd --snapshot --animation\n";
  out << "  cuwacunu_cmd --snapshot --splash loading\n";
  out << "  cuwacunu.cmd --snapshot --splash loading\n";
  out << "  cuwacunu_cmd --snapshot --splash close\n";
  out << "  cuwacunu.cmd --snapshot --splash close\n";
  out << "  cuwacunu_cmd --snapshot --splash good-luck\n";
  out << "  cuwacunu.cmd --snapshot --splash good-luck\n";
  out << "  cuwacunu_cmd --snapshot --screen runtime\n";
  out << "  cuwacunu.cmd --snapshot --screen runtime\n";
  out << "  cuwacunu_cmd --snapshot --full\n";
  out << "  cuwacunu.cmd --snapshot --full\n";
  out << "  cuwacunu_cmd --command \"show config\" --snapshot\n";
  out << "  cuwacunu.cmd --command \"show config\" --snapshot\n";
  out << "  cuwacunu_cmd src/config/hero.runtime.dsl --snapshot\n";
  out << "  cuwacunu.cmd src/config/hero.runtime.dsl --snapshot\n";
  out << "  cuwacunu_cmd --palette --command \"show shell logs\"\n";
  out << "  cuwacunu.cmd --palette --command \"show shell logs\"\n";
  out << "  cuwacunu_cmd --actions --run \"show shell logs\"\n";
  out << "  cuwacunu.cmd --actions --run \"show shell logs\"\n";
  out << "  cuwacunu_cmd --snapshot --run show --run \"logs source show\"\n";
  out << "  cuwacunu.cmd --snapshot --run show --run \"logs source show\"\n";
  out << "\n";
  append_cli_help_section_header(out, "Shortcut Commands");
  out << "  h/H / ? / help / menu        open menu overlay; lowercase h "
         "browses "
         "rows on browser screens\n";
  out << "  a / actions / palette / commands open action palette\n";
  out << "  home / f1 / 1                switch to F1 Home\n";
  out << "  workbench / work / w / f2 / 2 switch to F2 Workbench\n";
  out << "  runtime / run / rt / f3 / 3  switch to F3 Runtime\n";
  out << "  lattice / lat / proof / f4 / 4 switch to F4 Lattice\n";
  out << "  logs / log / shell / events / f8 / 8 switch to F8 Shell Logs\n";
  out << "  config / cfg / f9 / 9        switch to F9 Config\n";
  out << "  refresh / r                  refresh snapshots\n";
  out << "  version / about              show command identity and aliases\n";
  out << "  splash / farewell            Home splash visual diagnostics\n";
  out << "  help/menu up/down/left/right scroll menu overlay\n";
  out << "  help/menu page/home/end      page or edge-scroll menu overlay\n";
  out << "  actions up/down              move action palette selection\n";
  out << "  actions page/home/end        page or edge-select action palette\n";
  out << "  logs setting prev/next       move Shell Logs setting cursor\n";
  out << "  logs setting left/right      change selected Shell Logs setting\n";
  out << "  logs up/down                 scroll Shell Logs stream\n";
  out << "  logs page up/down            page Shell Logs stream\n";
  out << "  logs home/end                jump oldest/newest Shell Logs rows\n";
  out << "  quit / q / exit / x          exit command shell\n";
  out << "\n";
  append_cli_help_section_header(out, "Screen Keys");
  out << "  Runtime Enter menu; a actions for the selected item\n";
  out << "  Lattice Enter                show selected target proof\n";
  out << "  Config Enter / e             open selected file preview\n";
  out << "  Shell Logs m / p             metadata lens and mouse capture\n";
  out << "  Shell Logs Left / Right      change selected log setting\n";
  out << "  Shell Logs Home / End        jump oldest or newest log rows\n";
  out << "\n";
  append_cli_help_section_header(out, "Command Paths");
  out << "  iinuji.help()                open menu overlay\n";
  out << "  iinuji.help.close()          close menu overlay\n";
  out << "  iinuji.help.scroll.*()       menu overlay scroll utilities\n";
  out << "  iinuji.screen.*()            screen selection paths\n";
  out << "  iinuji.show.*()              structured show summaries\n";
  out << "  iinuji.version()             command identity and aliases\n";
  out << "  iinuji.home.visual()         return to the Home visual showcase\n";
  out << "  iinuji.home.animation()      Home animation alias\n";
  out << "  iinuji.home.splash()         show bootstrap splash diagnostics\n";
  out << "  iinuji.splash()              bootstrap splash alias\n";
  out << "  iinuji.home.farewell()       show closing splash diagnostics\n";
  out << "  iinuji.farewell()            closing splash alias\n";
  out << "  iinuji.actions()             open action palette\n";
  out << "  iinuji.commands()            commands namespace for action "
         "palette\n";
  out << "  iinuji.actions.run()         run selected palette action\n";
  out << "  iinuji.commands.run.selected() run selected palette action\n";
  out << "  iinuji.actions.select.*()    action palette movement utilities\n";
  out << "  iinuji.commands.select.*()   commands namespace movement "
         "utilities\n";
  out << "  iinuji.actions.close()       close action palette\n";
  out << "  iinuji.commands.close()      close action palette via commands\n";
  out << "  iinuji.runtime.*()           Runtime focus/detail/item utilities\n";
  out << "  iinuji.runtime.row.prev()    previous Runtime focus lane\n";
  out << "  iinuji.runtime.row.next()    next Runtime focus lane\n";
  out << "  iinuji.runtime.item.prev()   previous Runtime row\n";
  out << "  iinuji.runtime.item.next()   next Runtime row\n";
  out << "  iinuji.lattice.target.*()    Lattice target navigation\n";
  out << "  iinuji.config.*()            Config catalog utilities\n";
  out << "  iinuji.config.file.*()       Config catalog navigation\n";
  out << "  iinuji.logs.clear()          clear Shell Logs feed\n";
  out << "  iinuji.logs.scroll.up()      scroll Shell Logs stream\n";
  out << "  iinuji.logs.scroll.down()    scroll Shell Logs stream\n";
  out << "  iinuji.logs.scroll.page.up() page Shell Logs stream up\n";
  out << "  iinuji.logs.scroll.page.down() page Shell Logs stream down\n";
  out << "  iinuji.logs.scroll.home()    jump Shell Logs to oldest\n";
  out << "  iinuji.logs.scroll.end()     jump Shell Logs to newest\n";
  out << "  iinuji.logs.settings.*()     Shell Logs lenses and toggles\n";
  out << "  iinuji.logs.settings.select.prev() move Shell Logs setting "
         "cursor\n";
  out << "  iinuji.logs.settings.select.next() move Shell Logs setting "
         "cursor\n";
  out << "  iinuji.logs.settings.change.prev() change selected Shell Logs "
         "setting\n";
  out << "  iinuji.logs.settings.change.next() change selected Shell Logs "
         "setting\n";
  out << "  iinuji.logs.settings.source.next() cycle Shell Logs source lens\n";
  out << "  iinuji.logs.settings.source.any() set Shell Logs source lens\n";
  out << "  iinuji.logs.settings.source.all() set Shell Logs source lens\n";
  out << "  iinuji.logs.settings.source.refresh() set Shell Logs source "
         "lens\n";
  out << "  iinuji.logs.settings.source.action() set Shell Logs source lens\n";
  out << "  iinuji.logs.settings.source.command() set Shell Logs source "
         "lens\n";
  out << "  iinuji.logs.settings.source.show() set Shell Logs source lens\n";
  out << "  iinuji.logs.settings.source.status() set Shell Logs source "
         "lens\n";
  out << "  iinuji.logs.settings.date.toggle() toggle Shell Logs "
         "timestamps\n";
  out << "  iinuji.logs.settings.follow.toggle() toggle Shell Logs follow "
         "mode\n";
  out << "  iinuji.logs.settings.level.next() cycle Shell Logs severity "
         "lens\n";
  out << "  iinuji.logs.settings.level.debug() direct severity lens\n";
  out << "  iinuji.logs.settings.level.info() direct severity lens\n";
  out << "  iinuji.logs.settings.level.warning() direct severity lens\n";
  out << "  iinuji.logs.settings.level.error() direct severity lens\n";
  out << "  iinuji.logs.settings.level.fatal() direct severity lens\n";
  out << "  iinuji.logs.settings.metadata.filter.next() cycle metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.filter.any() direct metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.filter.any_meta() direct metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.filter.function() direct metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.filter.path() direct metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.filter.callsite() direct metadata "
         "lenses\n";
  out << "  iinuji.logs.settings.metadata.toggle() toggle Shell Logs "
         "metadata\n";
  out << "  iinuji.logs.settings.thread.toggle() toggle Shell Logs thread "
         "column\n";
  out << "  iinuji.logs.settings.color.toggle() toggle Shell Logs color "
         "emphasis\n";
  out << "  iinuji.logs.settings.mouse.capture.toggle() toggle Shell Logs "
         "mouse capture\n";
  out << "  iinuji.workspace.*()         full/split panel controls\n";
  out << "  iinuji.workspace.zoom.toggle() toggle full/split workspace\n";
  out << "  iinuji.workspace.split()     restore split workspace\n";
  out << "  iinuji.quit() / iinuji.exit() exit command shell\n";
  out << "\n";
  append_cli_help_section_header(out, "GUI Config Keys");
  out << "  GUI.iinuji_loading_logo_path\n";
  out << "  GUI.iinuji_closing_logo_path\n";
  out << "  GUI.iinuji_home_animation_path\n";
}

inline void print_version(std::ostream &out,
                          std::string_view command_name = "cuwacunu_cmd") {
  const auto lines = command_identity_lines(command_name);
  if (lines.empty())
    return;
  out << lines.front() << "\n\n";
  append_cli_help_section_header(out, "Identity");
  for (std::size_t i = 1u; i < lines.size(); ++i)
    out << lines[i] << "\n";
}

inline std::shared_ptr<text_box_data_t>
text_data(const std::shared_ptr<iinuji_object_t> &object) {
  if (!object)
    return nullptr;
  return std::dynamic_pointer_cast<text_box_data_t>(object->data);
}

inline std::shared_ptr<editor_box_data_t>
editor_data(const std::shared_ptr<iinuji_object_t> &object) {
  if (!object)
    return nullptr;
  return std::dynamic_pointer_cast<editor_box_data_t>(object->data);
}

inline std::shared_ptr<image_box_data_t>
image_data(const std::shared_ptr<iinuji_object_t> &object) {
  if (!object)
    return nullptr;
  return std::dynamic_pointer_cast<image_box_data_t>(object->data);
}

inline std::shared_ptr<plot_box_data_t>
plot_data(const std::shared_ptr<iinuji_object_t> &object) {
  if (!object)
    return nullptr;
  return std::dynamic_pointer_cast<plot_box_data_t>(object->data);
}

inline void set_text(const std::shared_ptr<iinuji_object_t> &object,
                     std::string content) {
  if (auto data = text_data(object)) {
    data->content = std::move(content);
    data->clear_styled_lines();
  }
}

inline void set_styled_lines(const std::shared_ptr<iinuji_object_t> &object,
                             std::vector<styled_text_line_t> lines,
                             bool wrap = true, bool reset_scroll = true) {
  if (auto data = text_data(object)) {
    std::ostringstream content;
    for (std::size_t i = 0; i < lines.size(); ++i) {
      if (i > 0)
        content << "\n";
      content << styled_text_line_text(lines[i]);
    }
    data->content = content.str();
    data->styled_lines = std::move(lines);
    data->wrap = wrap;
    if (reset_scroll) {
      data->scroll_y = 0;
      data->scroll_x = 0;
    }
  }
}

inline void set_image(const std::shared_ptr<iinuji_object_t> &object,
                      rgba_image_t image) {
  if (auto data = image_data(object))
    data->image = std::move(image);
}

struct CmdUiAssetPaths {
  std::filesystem::path loading_logo{
      "/cuwacunu/src/resources/waajacamaya_hello.png"};
  std::filesystem::path closing_logo{"/cuwacunu/src/resources/waajacamaya.png"};
  std::filesystem::path home_animation{
      "/cuwacunu/src/resources/waajacamaya.apng"};
};

inline std::string strip_config_value(std::string value) {
  value = strip_trailing_semicolon(std::move(value));
  if (value.size() >= 2u && ((value.front() == '"' && value.back() == '"') ||
                             (value.front() == '\'' && value.back() == '\''))) {
    value = value.substr(1u, value.size() - 2u);
  }
  return value;
}

inline std::filesystem::path
resolve_config_path_value(const std::filesystem::path &global_config_path,
                          std::string value) {
  value = strip_config_value(std::move(value));
  if (value.empty())
    return {};
  std::filesystem::path path{value};
  if (path.is_absolute() || global_config_path.empty())
    return path;
  return global_config_path.parent_path() / path;
}

inline std::optional<std::string>
read_global_config_value(const std::filesystem::path &global_config_path,
                         std::string_view section, std::string_view key) {
  std::ifstream in(global_config_path);
  if (!in)
    return std::nullopt;

  std::string current_section{};
  std::string line{};
  while (std::getline(in, line)) {
    std::string trimmed = trim(std::move(line));
    if (trimmed.empty() || trimmed.front() == '#')
      continue;
    if (trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim(trimmed.substr(1u, trimmed.size() - 2u));
      continue;
    }
    if (current_section != section)
      continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string lhs = trim(trimmed.substr(0u, eq));
    if (lhs != key)
      continue;
    return strip_config_value(trimmed.substr(eq + 1u));
  }
  return std::nullopt;
}

struct CmdUiLogDefaults {
  std::size_t buffer_capacity{6000};
  bool show_timestamp{false};
  bool show_thread{false};
  bool show_metadata{true};
  bool show_color{true};
  bool auto_follow{true};
  bool mouse_capture{true};
  LogsMetadataFilter metadata_filter{LogsMetadataFilter::Any};
};

inline bool parse_config_bool_value(std::string value, bool &out) {
  const std::string token = normalize_command(strip_config_value(value));
  if (token == "true" || token == "1" || token == "yes" || token == "on") {
    out = true;
    return true;
  }
  if (token == "false" || token == "0" || token == "no" || token == "off") {
    out = false;
    return true;
  }
  return false;
}

inline bool parse_config_size_value(std::string value, std::size_t &out) {
  value = trim(strip_config_value(std::move(value)));
  if (value.empty())
    return false;
  std::size_t parsed = 0;
  for (unsigned char ch : value) {
    if (!std::isdigit(ch))
      return false;
    const std::size_t digit = static_cast<std::size_t>(ch - '0');
    if (parsed > (std::numeric_limits<std::size_t>::max() - digit) / 10u)
      return false;
    parsed = (parsed * 10u) + digit;
  }
  if (parsed == 0u)
    return false;
  out = parsed;
  return true;
}

inline LogsMetadataFilter
parse_logs_metadata_filter_value(std::string value,
                                 LogsMetadataFilter fallback) {
  const std::string token = normalize_command(strip_config_value(value));
  if (token == "any")
    return LogsMetadataFilter::Any;
  if (token == "meta+" || token == "meta" || token == "any_meta" ||
      token == "with_any_metadata")
    return LogsMetadataFilter::WithAnyMetadata;
  if (token == "fn+" || token == "function" || token == "with_function")
    return LogsMetadataFilter::WithFunction;
  if (token == "path+" || token == "path" || token == "with_path")
    return LogsMetadataFilter::WithPath;
  if (token == "callsite+" || token == "callsite" || token == "with_callsite")
    return LogsMetadataFilter::WithCallsite;
  return fallback;
}

inline CmdUiLogDefaults
resolve_cmd_ui_log_defaults(const std::filesystem::path &global_config_path) {
  CmdUiLogDefaults defaults{};
  if (const auto value = read_global_config_value(
          global_config_path, "GUI", "iinuji_logs_buffer_capacity")) {
    (void)parse_config_size_value(*value, defaults.buffer_capacity);
  }
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_logs_show_date")) {
    (void)parse_config_bool_value(*value, defaults.show_timestamp);
  }
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_logs_show_thread")) {
    (void)parse_config_bool_value(*value, defaults.show_thread);
  }
  if (const auto value = read_global_config_value(
          global_config_path, "GUI", "iinuji_logs_show_metadata")) {
    (void)parse_config_bool_value(*value, defaults.show_metadata);
  }
  if (const auto value = read_global_config_value(
          global_config_path, "GUI", "iinuji_logs_metadata_filter")) {
    defaults.metadata_filter =
        parse_logs_metadata_filter_value(*value, defaults.metadata_filter);
  }
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_logs_show_color")) {
    (void)parse_config_bool_value(*value, defaults.show_color);
  }
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_logs_auto_follow")) {
    (void)parse_config_bool_value(*value, defaults.auto_follow);
  }
  if (const auto value = read_global_config_value(
          global_config_path, "GUI", "iinuji_logs_mouse_capture")) {
    (void)parse_config_bool_value(*value, defaults.mouse_capture);
  }
  return defaults;
}

inline void
apply_cmd_ui_log_defaults(CmdState &state,
                          const std::filesystem::path &global_config_path) {
  const CmdUiLogDefaults defaults =
      resolve_cmd_ui_log_defaults(global_config_path);
  state.logs.buffer_capacity = defaults.buffer_capacity;
  state.logs.show_timestamp = defaults.show_timestamp;
  state.logs.show_thread = defaults.show_thread;
  state.logs.show_metadata = defaults.show_metadata;
  state.logs.show_color = defaults.show_color;
  state.logs.auto_follow = defaults.auto_follow;
  state.logs.mouse_capture = defaults.mouse_capture;
  state.logs.metadata_filter = defaults.metadata_filter;
  if (state.log.size() > state.logs.buffer_capacity) {
    const auto over = static_cast<std::vector<LogEntry>::difference_type>(
        state.log.size() - state.logs.buffer_capacity);
    state.log.erase(state.log.begin(), state.log.begin() + over);
  }
}

inline CmdUiAssetPaths
resolve_cmd_ui_asset_paths(const std::filesystem::path &global_config_path) {
  CmdUiAssetPaths paths{};
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_loading_logo_path")) {
    if (const auto path = resolve_config_path_value(global_config_path, *value);
        !path.empty()) {
      paths.loading_logo = path;
      paths.closing_logo = path;
    }
  }
  if (const auto value = read_global_config_value(global_config_path, "GUI",
                                                  "iinuji_closing_logo_path")) {
    if (const auto path = resolve_config_path_value(global_config_path, *value);
        !path.empty())
      paths.closing_logo = path;
  }
  if (const auto value = read_global_config_value(
          global_config_path, "GUI", "iinuji_home_animation_path")) {
    if (const auto path = resolve_config_path_value(global_config_path, *value);
        !path.empty())
      paths.home_animation = path;
  }
  return paths;
}

inline void apply_home_visual_state(CmdState &state,
                                    const CmdUiAssetPaths &paths,
                                    const HomeShowcaseAssets &assets) {
  state.home_visual.loading_logo_path = paths.loading_logo;
  state.home_visual.closing_logo_path = paths.closing_logo;
  state.home_visual.home_animation_path = paths.home_animation;
  state.home_visual.loading_logo_error = assets.loading_logo_error;
  state.home_visual.home_animation_error = assets.home_animation_error;
  state.home_visual.loading_logo_ok = assets.loading_logo_ok;
  state.home_visual.animation_ok = assets.home_animation_ok;
  state.home_visual.animation_dynamic = assets.home_animation_is_dynamic;
  state.home_visual.animation_frames =
      assets.home_animation_ok ? assets.home_animation.frames.size() : 0u;
  state.home_visual.animation_width = 0;
  state.home_visual.animation_height = 0;
  if (assets.home_animation_ok && !assets.home_animation.frames.empty()) {
    const rgba_image_t rendered_frame = composite_home_wordmark_over_image(
        assets.home_animation.frames.front(), assets.home_wordmark_image);
    if (rendered_frame.valid()) {
      state.home_visual.animation_width = rendered_frame.width;
      state.home_visual.animation_height = rendered_frame.height;
    } else {
      state.home_visual.animation_width = assets.home_animation.width;
      state.home_visual.animation_height = assets.home_animation.height;
    }
  }
}

inline void
refresh_home_visual_state(CmdState &state,
                          const std::filesystem::path &global_config_path) {
  const CmdUiAssetPaths paths = resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets assets = load_home_showcase_assets(
      paths.loading_logo.string(), paths.home_animation.string());
  apply_home_visual_state(state, paths, assets);
}

inline std::string get_text(const std::shared_ptr<iinuji_object_t> &object) {
  if (auto data = text_data(object))
    return data->content;
  return {};
}

inline constexpr int kAnimatedHomeFrameStepMs = 33;
inline constexpr int kEscapeSequenceTimeoutMs = 12;
inline constexpr int kFarewellHoldMs = 1100;
inline constexpr double kHomeImageDockFraction = 0.56;
inline constexpr double kRuntimeDevicePlotDockFraction = 0.40;
inline constexpr int kRuntimeDevicePlotPadRight = 10;

inline bool animation_has_multiple_frames(const rgba_animation_t &animation) {
  return animation.valid() && animation.frames.size() > 1u;
}

inline int desired_input_timeout_for_screen(ScreenMode screen,
                                            bool animate_home = false) {
  if (screen == ScreenMode::Home)
    return animate_home ? kAnimatedHomeFrameStepMs : 250;
  if (screen == ScreenMode::Logs)
    return 50;
  return 250;
}

inline std::uint64_t
idle_refresh_period_ms_for_screen(ScreenMode screen,
                                  bool animate_home = false) {
  switch (screen) {
  case ScreenMode::Home:
    return animate_home ? static_cast<std::uint64_t>(kAnimatedHomeFrameStepMs)
                        : 5000u;
  case ScreenMode::Workbench:
    return 5000u;
  case ScreenMode::Runtime:
    return 2000u;
  case ScreenMode::Lattice:
    return 1200u;
  case ScreenMode::Logs:
    return 250u;
  case ScreenMode::Config:
    return 2000u;
  }
  return 5000u;
}

inline bool refresh_visible_screen_on_idle(CmdState &state,
                                           bool animate_home = false) {
  switch (state.screen) {
  case ScreenMode::Home:
    return animate_home;
  case ScreenMode::Workbench:
  case ScreenMode::Runtime:
  case ScreenMode::Logs:
  case ScreenMode::Config:
    return false;
  case ScreenMode::Lattice:
    return refresh_lattice_runtime_evidence_counts(state);
  }
  return false;
}

inline bool should_animate_home_frame(const CmdState &state,
                                      bool animate_home) {
  return animate_home && state.screen == ScreenMode::Home && !state.help_view &&
         !state.action_menu_open && !cmd_choice_menu_open(state) &&
         !cmd_content_popup_open(state);
}

inline constexpr std::uint64_t kRuntimeDeviceRefreshPeriodMs = 700u;

inline bool
should_poll_runtime_device_snapshot(const CmdState &state,
                                    std::uint64_t frame_now_ms,
                                    std::uint64_t last_interaction_ms) {
  (void)frame_now_ms;
  (void)last_interaction_ms;
  return state.screen == ScreenMode::Runtime &&
         state.runtime.focus == RuntimeFocus::Devices && !state.help_view &&
         !state.action_menu_open && !cmd_choice_menu_open(state) &&
         !cmd_content_popup_open(state) && !state.command_mode &&
         !state.command_focused;
}

inline void reset_command_history_cursor(CmdState &state) {
  state.command_history_cursor = -1;
  state.command_history_draft.clear();
}

inline bool is_command_text_key(int key) { return key >= 32 && key <= 126; }

inline void enter_command_mode(CmdState &state) {
  state.command_mode = true;
  state.command_focused = true;
  state.status = "command mode";
}

inline void leave_command_mode(CmdState &state) {
  state.command_mode = false;
  state.command_focused = false;
  reset_command_history_cursor(state);
}

inline void remember_command(CmdState &state, const std::string &command) {
  const std::string normalized = trim(command);
  if (normalized.empty())
    return;
  if (state.command_history.empty() ||
      state.command_history.back() != normalized) {
    state.command_history.push_back(normalized);
    if (state.command_history.size() > 80)
      state.command_history.erase(state.command_history.begin(),
                                  state.command_history.begin() + 20);
  }
  reset_command_history_cursor(state);
}

inline void scroll_text(const std::shared_ptr<iinuji_object_t> &object,
                        int key) {
  auto text = text_data(object);
  auto editor = editor_data(object);
  if (!text && !editor)
    return;
  auto scroll_by = [&](int dy, int dx = 0) {
    if (text) {
      text->scroll_by(dy, dx);
      return;
    }
    editor->top_line = std::max(0, editor->top_line + dy);
    editor->left_col = std::max(0, editor->left_col + dx);
  };
  switch (standard_key_action(key)) {
  case key_action_t::MoveUp:
    scroll_by(-1);
    break;
  case key_action_t::MoveDown:
    scroll_by(1);
    break;
  case key_action_t::MoveLeft:
    scroll_by(0, -4);
    break;
  case key_action_t::MoveRight:
    scroll_by(0, 4);
    break;
  case key_action_t::PageUp:
    scroll_by(-8);
    break;
  case key_action_t::PageDown:
    scroll_by(8);
    break;
  case key_action_t::Home:
    if (text)
      text->scroll_y = 0;
    else
      editor->top_line = 0;
    break;
  case key_action_t::End:
    if (text)
      text->scroll_y = std::numeric_limits<int>::max();
    else
      editor->top_line = std::numeric_limits<int>::max();
    break;
  default:
    break;
  }
}

struct text_scroll_metrics_t {
  bool can_scroll_y{false};
  bool can_scroll_x{false};
};

inline std::vector<std::string>
text_box_visible_lines(const std::shared_ptr<text_box_data_t> &data) {
  std::vector<std::string> lines{};
  if (!data)
    return lines;
  if (!data->styled_lines.empty()) {
    lines.reserve(data->styled_lines.size());
    for (const auto &line : data->styled_lines)
      lines.push_back(styled_text_line_text(line));
    return lines;
  }
  return split_lines_keep_empty(data->content);
}

inline text_scroll_metrics_t
text_box_scroll_metrics(const std::shared_ptr<iinuji_object_t> &object) {
  text_scroll_metrics_t metrics{};
  const auto data = text_data(object);
  if (!object || !data)
    return metrics;

  const Rect rect = content_rect(*object);
  int text_w = std::max(0, rect.w);
  int text_h = std::max(0, rect.h);
  if (text_w <= 0 || text_h <= 0)
    return metrics;

  const std::vector<std::string> lines = text_box_visible_lines(data);
  int max_line_width = 0;
  for (const auto &line : lines)
    max_line_width = std::max(max_line_width, utf8_display_width(line));

  if (data->wrap) {
    std::size_t wrapped_lines = 0u;
    const int safe_w = std::max(1, text_w);
    for (const auto &line : lines) {
      const int width = utf8_display_width(line);
      wrapped_lines +=
          static_cast<std::size_t>(std::max(1, (width + safe_w - 1) / safe_w));
    }
    metrics.can_scroll_y = wrapped_lines > static_cast<std::size_t>(text_h);
    return metrics;
  }

  int reserve_h = 0;
  int reserve_v = 0;
  for (int i = 0; i < 3; ++i) {
    text_w = std::max(0, rect.w - reserve_v);
    text_h = std::max(0, rect.h - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return metrics;
    const bool need_h = rect.h > 1 && max_line_width > text_w;
    const int next_h = need_h ? 1 : 0;
    const bool need_v =
        static_cast<int>(lines.size()) > std::max(0, rect.h - next_h);
    const int next_v = need_v ? 1 : 0;
    if (next_h == reserve_h && next_v == reserve_v)
      break;
    reserve_h = next_h;
    reserve_v = next_v;
  }

  text_w = std::max(0, rect.w - reserve_v);
  text_h = std::max(0, rect.h - reserve_h);
  metrics.can_scroll_x = max_line_width > text_w;
  metrics.can_scroll_y = static_cast<int>(lines.size()) > text_h;
  return metrics;
}

inline void
adapt_wheel_scroll_axis(const std::shared_ptr<iinuji_object_t> &object, int &dy,
                        int &dx) {
  if (dx != 0 || dy == 0)
    return;
  auto data = text_data(object);
  if (!data)
    return;
  const text_scroll_metrics_t metrics = text_box_scroll_metrics(object);
  if (metrics.can_scroll_y || !metrics.can_scroll_x)
    return;

  data->scroll_y = 0;
  const int steps = std::max(1, (std::abs(dy) + 2) / 3);
  dx = (dy < 0 ? -8 : 8) * steps;
  dy = 0;
}

inline bool scroll_object_by(const std::shared_ptr<iinuji_object_t> &object,
                             int dy, int dx = 0) {
  adapt_wheel_scroll_axis(object, dy, dx);
  auto data = text_data(object);
  if (data) {
    data->scroll_by(dy, dx);
    return true;
  }
  auto editor = editor_data(object);
  if (editor) {
    editor->top_line = std::max(0, editor->top_line + dy);
    editor->left_col = std::max(0, editor->left_col + dx);
    return true;
  }
  return false;
}

inline bool scroll_text_by(const std::shared_ptr<iinuji_object_t> &object,
                           int dy, int dx = 0) {
  return scroll_object_by(object, dy, dx);
}

inline bool open_runtime_selected_menu(CmdState &state) {
  if (state.screen != ScreenMode::Runtime)
    return false;

  open_choice_menu(state, CmdChoiceMenuKind::RuntimeSelection);

  if (state.runtime.focus == RuntimeFocus::Jobs) {
    if (state.runtime.jobs.empty()) {
      state.status = "runtime jobs menu";
      return true;
    }
    state.status = "runtime job menu";
    return true;
  }

  state.status = state.runtime.selected_device_row == 0u ? "runtime host menu"
                                                         : "runtime gpu menu";
  return true;
}

inline void scroll_object_to(const std::shared_ptr<iinuji_object_t> &object,
                             int scroll_y, int scroll_x) {
  auto data = text_data(object);
  if (data) {
    data->scroll_y = std::max(0, scroll_y);
    data->scroll_x = std::max(0, scroll_x);
    return;
  }
  auto editor = editor_data(object);
  if (editor) {
    editor->top_line = std::max(0, scroll_y);
    editor->left_col = std::max(0, scroll_x);
  }
}

inline void scroll_text_to(const std::shared_ptr<iinuji_object_t> &object,
                           int scroll_y, int scroll_x) {
  scroll_object_to(object, scroll_y, scroll_x);
}

inline std::shared_ptr<iinuji_object_t>
make_panel_text(const std::string &panel_id, const std::string &text_id,
                const iinuji_style_t &panel_style,
                const iinuji_style_t &text_style) {
  auto panel = create_panel(panel_id, iinuji_layout_t{}, panel_style);
  auto body = create_text_box(text_id, "", true, text_align_t::Left,
                              iinuji_layout_t{}, text_style);
  panel_body_object(panel)->add_child(body);
  return panel;
}

struct CmdUi {
  std::shared_ptr<iinuji_object_t> root{};
  std::shared_ptr<iinuji_object_t> title{};
  std::shared_ptr<iinuji_object_t> nav{};
  std::shared_ptr<iinuji_object_t> workspace{};
  std::shared_ptr<iinuji_object_t> main_panel{};
  std::shared_ptr<iinuji_object_t> home_image{};
  std::shared_ptr<iinuji_object_t> main{};
  std::shared_ptr<iinuji_object_t> split_panel{};
  std::shared_ptr<iinuji_object_t> split_nav_panel{};
  std::shared_ptr<iinuji_object_t> split_nav{};
  std::shared_ptr<iinuji_object_t> split_worklist_panel{};
  std::shared_ptr<iinuji_object_t> split_worklist{};
  std::shared_ptr<iinuji_object_t> detail_panel{};
  std::shared_ptr<iinuji_object_t> detail{};
  std::shared_ptr<iinuji_object_t> config_editor{};
  std::shared_ptr<iinuji_object_t> runtime_device_plot{};
  std::shared_ptr<iinuji_object_t> status{};
  std::shared_ptr<iinuji_object_t> command_panel{};
  std::shared_ptr<iinuji_object_t> command_grid{};
  std::shared_ptr<iinuji_object_t> command_prompt{};
  std::shared_ptr<iinuji_object_t> input{};
  std::shared_ptr<iinuji_object_t> command_hint{};
  std::shared_ptr<iinuji_object_t> menu_overlay{};
  std::shared_ptr<iinuji_object_t> menu_commands{};
  std::shared_ptr<iinuji_object_t> menu_comments{};
  std::shared_ptr<iinuji_object_t> choice_shadow{};
  std::shared_ptr<iinuji_object_t> choice_overlay{};
  std::shared_ptr<iinuji_object_t> choice_commands{};
  std::shared_ptr<iinuji_object_t> choice_comments{};
  std::shared_ptr<iinuji_object_t> popup_shadow{};
  std::shared_ptr<iinuji_object_t> popup_overlay{};
  std::shared_ptr<iinuji_object_t> popup_text{};
  rgba_animation_t home_animation{};
  rgba_image_t home_static_image{};
  rgba_image_t closing_static_image{};
  std::uint64_t home_animation_origin_ms{0};
  std::filesystem::path config_editor_path{};
  std::size_t config_editor_selected_file{
      std::numeric_limits<std::size_t>::max()};
  int last_mouse_x{0};
  int last_mouse_y{0};
};

inline bool command_input_is_active(const CmdState &state, const CmdUi &ui) {
  return state.command_focused || state.command_mode ||
         !get_text(ui.input).empty();
}

inline void sync_command_input_focus(CmdState &state, CmdUi &ui) {
  const bool active = command_input_is_active(state, ui);
  state.command_focused = active;
  if (ui.input) {
    ui.input->focus_mode = focus_mode_t::Input;
    ui.input->focusable = true;
    ui.input->focused = active;
  }
  if (ui.command_hint) {
    ui.command_hint->visible = !active;
    set_text(ui.command_hint,
             active ? std::string{} : std::string{"tab or click to focus"});
  }
  if (ui.command_grid && ui.command_grid->grid) {
    ui.command_grid->grid->cols =
        active ? std::vector<len_spec>{len_spec::px(5), len_spec::frac(1.0),
                                       len_spec::px(0)}
               : std::vector<len_spec>{len_spec::px(5), len_spec::frac(1.0),
                                       len_spec::px(22)};
  }
}

inline void focus_command_input(CmdState &state, CmdUi &ui) {
  state.command_focused = true;
  sync_command_input_focus(state, ui);
  state.status = "command focus";
}

struct SplashUi {
  std::shared_ptr<iinuji_object_t> root{};
  std::shared_ptr<iinuji_object_t> title{};
  std::shared_ptr<iinuji_object_t> image{};
  std::shared_ptr<iinuji_object_t> detail{};
  std::shared_ptr<iinuji_object_t> status{};
};

struct CmdScreenTheme {
  std::string_view title_fg{};
  std::string_view title_bg{};
  std::string_view border{};
  std::string_view nav_fg{};
  std::string_view main_fg{};
  std::string_view detail_fg{};
  std::string_view status_fg{};
  std::string_view status_bg{};
  std::string_view command_fg{};
};

inline CmdScreenTheme screen_theme(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return CmdScreenTheme{
        .title_fg = "#F1FFF4",
        .title_bg = "#173122",
        .border = "#2D6A44",
        .nav_fg = "#CDEFD5",
        .main_fg = "#DAF5E0",
        .detail_fg = "#D6EBD9",
        .status_fg = "#B7D2C0",
        .status_bg = "#101014",
        .command_fg = "#E8F7EC",
    };
  case ScreenMode::Workbench:
    return CmdScreenTheme{
        .title_fg = "#FFF3D9",
        .title_bg = "#2A1C11",
        .border = "#8D5C33",
        .nav_fg = "#F3DFC3",
        .main_fg = "#F3DFC3",
        .detail_fg = "#F0DEC4",
        .status_fg = "#E5C89A",
        .status_bg = "#101014",
        .command_fg = "#FAEEDB",
    };
  case ScreenMode::Runtime:
    return CmdScreenTheme{
        .title_fg = "#EAF3FF",
        .title_bg = "#162338",
        .border = "#4C7BB4",
        .nav_fg = "#BFD8FF",
        .main_fg = "#D8E7FF",
        .detail_fg = "#C8DCFF",
        .status_fg = "#AFC5E6",
        .status_bg = "#101014",
        .command_fg = "#DCEAFF",
    };
  case ScreenMode::Lattice:
    return CmdScreenTheme{
        .title_fg = "#F4ECFF",
        .title_bg = "#241C30",
        .border = "#7A5AA0",
        .nav_fg = "#E4D2FF",
        .main_fg = "#EFE1FF",
        .detail_fg = "#E5D4F6",
        .status_fg = "#CFB9E8",
        .status_bg = "#101014",
        .command_fg = "#F4E8FF",
    };
  case ScreenMode::Logs:
    return CmdScreenTheme{
        .title_fg = "#FFF7DA",
        .title_bg = "#272313",
        .border = "#BDA34A",
        .nav_fg = "#F6DF8E",
        .main_fg = "#F1E4AF",
        .detail_fg = "#DED2A0",
        .status_fg = "#D4C174",
        .status_bg = "#101014",
        .command_fg = "#FFF1BC",
    };
  case ScreenMode::Config:
    return CmdScreenTheme{
        .title_fg = "#EDEDED",
        .title_bg = "#202028",
        .border = "#6C6C75",
        .nav_fg = "#D0D0D0",
        .main_fg = "#D0D0D0",
        .detail_fg = "#C8C8CE",
        .status_fg = "#B8B8BF",
        .status_bg = "#101014",
        .command_fg = "#E8E8E8",
    };
  }
  return screen_theme(ScreenMode::Config);
}

inline std::string screen_main_panel_title(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return " waajacamaya ";
  case ScreenMode::Workbench:
    return " workbench ";
  case ScreenMode::Runtime:
    return " runtime ";
  case ScreenMode::Lattice:
    return " lattice ";
  case ScreenMode::Logs:
    return " shell logs  [^] [v] ";
  case ScreenMode::Config:
    return " config ";
  }
  return " view ";
}

inline std::string screen_detail_panel_title(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return " disclosures ";
  case ScreenMode::Workbench:
    return "";
  case ScreenMode::Runtime:
    return " telemetry ";
  case ScreenMode::Lattice:
    return " evidence ";
  case ScreenMode::Logs:
    return " controls ";
  case ScreenMode::Config:
    return " preview ";
  }
  return " context ";
}

inline std::string runtime_detail_panel_title(const CmdState &state) {
  return state.runtime.focus == RuntimeFocus::Jobs ? " details "
                                                   : " telemetry ";
}

inline bool screen_uses_split_left_panels(ScreenMode screen) {
  return screen == ScreenMode::Runtime || screen == ScreenMode::Lattice ||
         screen == ScreenMode::Config;
}

inline std::string screen_split_nav_panel_title(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Runtime:
    return " navigator ";
  case ScreenMode::Lattice:
    return " targets ";
  case ScreenMode::Config:
    return " catalog ";
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Logs:
    break;
  }
  return " navigation ";
}

inline std::string screen_split_worklist_panel_title(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Runtime:
    return " worklist ";
  case ScreenMode::Lattice:
    return " target list ";
  case ScreenMode::Config:
    return " files ";
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Logs:
    break;
  }
  return " worklist ";
}

inline std::string panel_title_with_zoom(std::string title,
                                         const std::string &label) {
  if (!title.empty() && title.back() == ' ')
    title.pop_back();
  title += " ";
  title += label;
  title += " ";
  return title;
}

inline std::string command_frame_title(std::string_view command_name) {
  std::string command = trim(std::string(command_name));
  if (command.empty())
    command = "cuwacunu_cmd";
  return " " + command + " ";
}

inline void apply_frame_style(const std::shared_ptr<iinuji_object_t> &object,
                              std::string_view label_color,
                              std::string_view background_color,
                              std::string_view border_color) {
  if (!object)
    return;
  object->style.label_color = std::string(label_color);
  object->style.background_color = std::string(background_color);
  object->style.border_color = std::string(border_color);
}

inline void apply_text_style(const std::shared_ptr<iinuji_object_t> &object,
                             std::string_view label_color,
                             std::string_view background_color) {
  if (!object)
    return;
  object->style.label_color = std::string(label_color);
  object->style.background_color = std::string(background_color);
}

inline void apply_screen_theme(CmdUi &ui, const CmdState &state) {
  const CmdScreenTheme theme = screen_theme(state.screen);
  const std::string bg{"#101014"};

  apply_frame_style(ui.title, theme.title_fg, theme.title_bg, theme.border);
  if (ui.title)
    ui.title->style.title = command_frame_title(state.command_name);
  apply_text_style(ui.nav, theme.nav_fg, bg);
  apply_frame_style(ui.main_panel, theme.main_fg, bg, theme.border);
  apply_frame_style(ui.split_nav_panel, theme.main_fg, bg, theme.border);
  apply_frame_style(ui.split_worklist_panel, theme.main_fg, bg, theme.border);
  apply_frame_style(ui.detail_panel, theme.detail_fg, bg, theme.border);
  apply_text_style(ui.main, theme.main_fg, bg);
  apply_text_style(ui.split_nav, theme.main_fg, bg);
  apply_text_style(ui.split_worklist, theme.main_fg, bg);
  apply_text_style(ui.detail, theme.detail_fg, bg);
  apply_text_style(ui.config_editor, theme.detail_fg, bg);
  apply_text_style(ui.runtime_device_plot, theme.detail_fg, bg);
  if (ui.runtime_device_plot)
    ui.runtime_device_plot->style.border_color = "#3B4452";
  apply_text_style(ui.home_image, theme.main_fg, bg);
  apply_frame_style(ui.status, theme.status_fg, theme.status_bg, theme.border);
  apply_frame_style(ui.command_panel, theme.command_fg, bg, theme.border);
  apply_text_style(ui.command_prompt, theme.command_fg, bg);
  apply_text_style(ui.input, theme.command_fg, bg);
  apply_text_style(ui.command_hint, "#7C828C", bg);

  if (ui.menu_overlay) {
    ui.menu_overlay->style.label_color = std::string(theme.command_fg);
    ui.menu_overlay->style.border_color = std::string(theme.border);
    ui.menu_overlay->style.background_color = "#11151C";
  }
  if (ui.choice_overlay) {
    ui.choice_overlay->style.label_color = std::string(theme.command_fg);
    ui.choice_overlay->style.border_color = std::string(theme.border);
    ui.choice_overlay->style.background_color = "#11151C";
  }
  apply_text_style(ui.choice_commands, "#A7D4FF", "#11151C");
  apply_text_style(ui.choice_comments, "#95A1B5", "#11151C");
  if (ui.popup_overlay) {
    ui.popup_overlay->style.label_color = std::string(theme.command_fg);
    ui.popup_overlay->style.border_color = std::string(theme.border);
    ui.popup_overlay->style.background_color = "#11151C";
  }
  apply_text_style(ui.popup_text, std::string(theme.detail_fg), "#11151C");
}

inline bool recall_command_history(CmdState &state, CmdUi &ui, int delta) {
  if (state.command_history.empty())
    return false;

  const int size = static_cast<int>(state.command_history.size());
  int next = state.command_history_cursor;
  if (next < 0) {
    state.command_history_draft = get_text(ui.input);
    next = delta < 0 ? size - 1 : 0;
  } else {
    next += delta;
  }

  if (next < 0)
    next = 0;
  if (next >= size) {
    set_text(ui.input, state.command_history_draft);
    reset_command_history_cursor(state);
    state.status = "command history: draft";
    return true;
  }

  state.command_history_cursor = next;
  set_text(ui.input, state.command_history[static_cast<std::size_t>(next)]);
  state.status = "command history: " + std::to_string(next + 1) + "/" +
                 std::to_string(size);
  return true;
}

inline std::vector<len_spec>
workspace_split_columns_for_screen(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return {len_spec::frac(0.42), len_spec::frac(0.58)};
  case ScreenMode::Logs:
    return {len_spec::frac(0.68), len_spec::frac(0.32)};
  case ScreenMode::Config:
    return {len_spec::frac(0.60), len_spec::frac(0.40)};
  case ScreenMode::Workbench:
  case ScreenMode::Runtime:
  case ScreenMode::Lattice:
    return {len_spec::frac(0.44), len_spec::frac(0.56)};
  }
  return {len_spec::frac(0.44), len_spec::frac(0.56)};
}

inline CmdUi create_ui(
    const std::filesystem::path &global_config_path = std::filesystem::path{
        "/cuwacunu/src/config/.config"}) {
  CmdUi ui{};
  const iinuji_style_t root_style{"#D8D8D8", "#101014"};
  const iinuji_style_t title_style{"#EDEDED",
                                   "#202028",
                                   true,
                                   "#6C6C75",
                                   false,
                                   false,
                                   command_frame_title("cuwacunu_cmd")};
  const iinuji_style_t nav_style{"#CFCFCF", "#101014"};
  const iinuji_style_t panel_style{"#D0D0D0", "#101014", true,    "#5E5E68",
                                   false,     false,     " view "};
  const iinuji_style_t detail_style{"#C8C8CE", "#101014", true,       "#5E5E68",
                                    false,     false,     " context "};
  const iinuji_style_t status_style{"#B3A99B", "#1E1B18", true,      "#4E473E",
                                    false,     false,     " status "};
  const iinuji_style_t command_style{
      "#E8E8E8", "#101014", true, "#5E5E68", false, false, " command "};
  const iinuji_style_t prompt_style{"#E8E8E8", "#101014", false, "#101014",
                                    true};
  const iinuji_style_t input_style{"#E8E8E8", "#101014", false, "#101014"};

  ui.root = create_grid_container(
      "cmd.root",
      {len_spec::px(3), len_spec::px(0), len_spec::frac(1.0), len_spec::px(3),
       len_spec::px(3)},
      {len_spec::frac(1.0)}, 0, 0, iinuji_layout_t{}, root_style);

  ui.title = create_text_box("cmd.title", "", false, text_align_t::Left,
                             iinuji_layout_t{}, title_style);
  place_in_grid(ui.title, 0, 0);
  ui.root->add_child(ui.title);

  ui.nav = create_text_box("cmd.tabs", "", false, text_align_t::Left,
                           iinuji_layout_t{}, nav_style);
  place_in_grid(ui.nav, 1, 0);
  ui.root->add_child(ui.nav);

  ui.workspace = create_grid_container(
      "cmd.workspace", {len_spec::frac(1.0)},
      workspace_split_columns_for_screen(ScreenMode::Home), 1, 1,
      iinuji_layout_t{}, root_style);
  place_in_grid(ui.workspace, 2, 0);
  ui.root->add_child(ui.workspace);

  ui.main_panel = make_panel_text("cmd.main.panel", "cmd.main", panel_style,
                                  iinuji_style_t{"#D0D0D0", "#101014"});
  place_in_grid(ui.main_panel, 0, 0);
  auto main_body = panel_body_object(ui.main_panel);
  ui.main = main_body->children.front();
  ui.main->layout.mode = layout_mode_t::Dock;
  ui.main->layout.dock = dock_t::Fill;

  const CmdUiAssetPaths asset_paths =
      resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets home_assets = load_home_showcase_assets(
      asset_paths.loading_logo.string(), asset_paths.home_animation.string());
  ui.home_animation = make_composited_home_animation(home_assets);
  if (ui.home_animation.valid()) {
    ui.home_static_image = ui.home_animation.frames.front();
  } else if (home_assets.loading_logo_ok) {
    ui.home_static_image = make_static_home_showcase_image(home_assets);
  } else {
    ui.home_static_image = make_fallback_home_wordmark_image();
  }
  if (asset_paths.closing_logo == asset_paths.loading_logo &&
      home_assets.loading_logo_ok) {
    ui.closing_static_image = home_assets.loading_logo_image;
  } else {
    std::string closing_error{};
    if (!decode_png_rgba_file(asset_paths.closing_logo.string(),
                              ui.closing_static_image, closing_error)) {
      ui.closing_static_image = home_assets.loading_logo_ok
                                    ? home_assets.loading_logo_image
                                    : ui.home_static_image;
    }
  }

  image_box_opts_t home_image_opts{};
  home_image_opts.image = ui.home_static_image;
  home_image_opts.render_options = make_home_showcase_render_options();
  home_image_opts.style = iinuji_style_t{"#D0D0D0", "#101014"};
  ui.home_image =
      create_image_box("cmd.home.image", std::move(home_image_opts));
  ui.home_image->layout.mode = layout_mode_t::Dock;
  ui.home_image->layout.dock = dock_t::Top;
  ui.home_image->layout.dock_size = len_spec::frac(kHomeImageDockFraction);
  main_body->add_child(ui.home_image);
  ui.workspace->add_child(ui.main_panel);

  ui.split_panel = create_grid_container(
      "cmd.split", {len_spec::px(9), len_spec::frac(1.0)},
      {len_spec::frac(1.0)}, 0, 0, iinuji_layout_t{}, root_style);
  ui.split_panel->visible = false;
  place_in_grid(ui.split_panel, 0, 0);
  ui.split_nav_panel =
      make_panel_text("cmd.split.nav.panel", "cmd.split.nav", panel_style,
                      iinuji_style_t{"#D0D0D0", "#101014"});
  place_in_grid(ui.split_nav_panel, 0, 0);
  ui.split_nav = panel_body_object(ui.split_nav_panel)->children.front();
  ui.split_panel->add_child(ui.split_nav_panel);
  ui.split_worklist_panel =
      make_panel_text("cmd.split.worklist.panel", "cmd.split.worklist",
                      panel_style, iinuji_style_t{"#D0D0D0", "#101014"});
  place_in_grid(ui.split_worklist_panel, 1, 0);
  ui.split_worklist =
      panel_body_object(ui.split_worklist_panel)->children.front();
  ui.split_panel->add_child(ui.split_worklist_panel);
  ui.workspace->add_child(ui.split_panel);

  ui.detail_panel =
      make_panel_text("cmd.detail.panel", "cmd.detail", detail_style,
                      iinuji_style_t{"#C8C8CE", "#101014"});
  place_in_grid(ui.detail_panel, 0, 1);
  ui.detail = panel_body_object(ui.detail_panel)->children.front();
  ui.detail->layout.mode = layout_mode_t::Dock;
  ui.detail->layout.dock = dock_t::Fill;
  editor_box_opts_t config_editor_opts{};
  config_editor_opts.read_only = true;
  config_editor_opts.layout.mode = layout_mode_t::Dock;
  config_editor_opts.layout.dock = dock_t::Fill;
  config_editor_opts.style = iinuji_style_t{"#C8C8CE", "#101014"};
  ui.config_editor =
      create_editor_box("cmd.config.editor", std::move(config_editor_opts));
  if (auto editor = editor_data(ui.config_editor)) {
    editor->show_header = true;
    editor->show_footer = true;
    editor->show_line_numbers = true;
    editor->status = "read only";
  }
  ui.config_editor->visible = false;
  panel_body_object(ui.detail_panel)->add_child(ui.config_editor);
  const auto runtime_plot_opts = [] {
    plot_box_create_opts_t opts{};
    opts.layout.mode = layout_mode_t::Dock;
    opts.layout.dock = dock_t::Bottom;
    opts.layout.dock_size = len_spec::frac(kRuntimeDevicePlotDockFraction);
    opts.layout.pad_right = kRuntimeDevicePlotPadRight;
    opts.style = iinuji_style_t{"#C8C8CE", "#101014"};
    opts.plot.draw_axes = true;
    opts.plot.draw_grid = true;
    opts.plot.baseline0 = false;
    opts.plot.margin_left = 5;
    opts.plot.margin_right = 6;
    opts.plot.margin_top = 1;
    opts.plot.margin_bot = 0;
    opts.plot.x_ticks = 0;
    opts.plot.y_ticks = 3;
    opts.plot.draw_y_tick_labels = true;
    opts.plot.draw_y2_tick_labels = true;
    opts.plot.draw_x_tick_labels = false;
    opts.plot.y_min = 0.0;
    return opts;
  };
  ui.runtime_device_plot =
      create_plot_box("cmd.runtime.device.plot", runtime_plot_opts());
  ui.runtime_device_plot->visible = false;
  panel_body_object(ui.detail_panel)->add_child(ui.runtime_device_plot);
  ui.workspace->add_child(ui.detail_panel);

  ui.status = create_text_box("cmd.status", "", true, text_align_t::Left,
                              iinuji_layout_t{}, status_style);
  place_in_grid(ui.status, 3, 0);
  ui.root->add_child(ui.status);

  ui.command_panel =
      create_panel("cmd.command.panel", iinuji_layout_t{}, command_style);
  place_in_grid(ui.command_panel, 4, 0);
  ui.command_grid = create_grid_container(
      "cmd.command.grid", {len_spec::frac(1.0)},
      {len_spec::px(5), len_spec::frac(1.0), len_spec::px(22)}, 0, 1,
      iinuji_layout_t{}, input_style);
  ui.command_grid->layout.mode = layout_mode_t::Dock;
  ui.command_grid->layout.dock = dock_t::Fill;
  ui.command_prompt =
      create_text_box("cmd.command.prompt", "", false, text_align_t::Left,
                      iinuji_layout_t{}, prompt_style);
  place_in_grid(ui.command_prompt, 0, 0);
  ui.command_grid->add_child(ui.command_prompt);
  ui.input = create_input_line("cmd.input", "", iinuji_layout_t{}, input_style);
  place_in_grid(ui.input, 0, 1);
  ui.command_grid->add_child(ui.input);
  ui.command_hint = create_text_box(
      "cmd.command.hint", "tab or click to focus", false, text_align_t::Right,
      iinuji_layout_t{}, iinuji_style_t{"#7C828C", "#101014"});
  place_in_grid(ui.command_hint, 0, 2);
  ui.command_grid->add_child(ui.command_hint);
  panel_body_object(ui.command_panel)->add_child(ui.command_grid);
  ui.root->add_child(ui.command_panel);

  ui.menu_overlay =
      create_panel("cmd.menu.overlay", iinuji_layout_t{},
                   iinuji_style_t{"#E8EDF5", "#11151C", true, "#FFD26E", false,
                                  false, " menu  Esc closes "});
  place_in_grid(ui.menu_overlay, 2, 0);
  ui.menu_overlay->visible = false;
  ui.menu_overlay->z_index = 100;
  auto overlay_grid = create_grid_container(
      "cmd.menu.grid", {len_spec::frac(1.0)},
      {len_spec::frac(0.46), len_spec::frac(0.54)}, 0, 1, iinuji_layout_t{},
      iinuji_style_t{"#E8EDF5", "#11151C"});
  overlay_grid->layout.mode = layout_mode_t::Dock;
  overlay_grid->layout.dock = dock_t::Fill;
  ui.menu_commands =
      create_text_box("cmd.menu.commands", "", false, text_align_t::Left,
                      iinuji_layout_t{}, iinuji_style_t{"#A7D4FF", "#11151C"});
  ui.menu_comments =
      create_text_box("cmd.menu.comments", "", false, text_align_t::Left,
                      iinuji_layout_t{}, iinuji_style_t{"#95A1B5", "#11151C"});
  place_in_grid(ui.menu_commands, 0, 0);
  place_in_grid(ui.menu_comments, 0, 1);
  overlay_grid->add_child(ui.menu_commands);
  overlay_grid->add_child(ui.menu_comments);
  panel_body_object(ui.menu_overlay)->add_child(overlay_grid);
  ui.root->add_child(ui.menu_overlay);

  iinuji_layout_t choice_layout{};
  choice_layout.mode = layout_mode_t::Normalized;
  choice_layout.x = 0.20;
  choice_layout.y = 0.27;
  choice_layout.width = 0.60;
  choice_layout.height = 0.44;
  choice_layout.normalized = true;
  iinuji_layout_t choice_shadow_layout = choice_layout;
  choice_shadow_layout.x += 0.006;
  choice_shadow_layout.y += 0.020;
  ui.choice_shadow = create_panel("cmd.choice.shadow", choice_shadow_layout,
                                  iinuji_style_t{"#050608", "#050608"});
  ui.choice_shadow->visible = false;
  ui.choice_shadow->z_index = 119;
  ui.root->add_child(ui.choice_shadow);
  ui.choice_overlay =
      create_panel("cmd.choice.overlay", choice_layout,
                   iinuji_style_t{"#E8EDF5", "#11151C", true, "#FFD26E", false,
                                  false, " menu  Esc closes "});
  ui.choice_overlay->visible = false;
  ui.choice_overlay->z_index = 120;
  auto choice_grid = create_grid_container(
      "cmd.choice.grid", {len_spec::frac(1.0)},
      {len_spec::frac(0.42), len_spec::frac(0.58)}, 0, 1, iinuji_layout_t{},
      iinuji_style_t{"#E8EDF5", "#11151C"});
  choice_grid->layout.mode = layout_mode_t::Dock;
  choice_grid->layout.dock = dock_t::Fill;
  ui.choice_commands =
      create_text_box("cmd.choice.commands", "", false, text_align_t::Left,
                      iinuji_layout_t{}, iinuji_style_t{"#A7D4FF", "#11151C"});
  ui.choice_comments =
      create_text_box("cmd.choice.comments", "", false, text_align_t::Left,
                      iinuji_layout_t{}, iinuji_style_t{"#95A1B5", "#11151C"});
  place_in_grid(ui.choice_commands, 0, 0);
  place_in_grid(ui.choice_comments, 0, 1);
  choice_grid->add_child(ui.choice_commands);
  choice_grid->add_child(ui.choice_comments);
  panel_body_object(ui.choice_overlay)->add_child(choice_grid);
  ui.root->add_child(ui.choice_overlay);

  iinuji_layout_t popup_layout{};
  popup_layout.mode = layout_mode_t::Normalized;
  popup_layout.x = 0.17;
  popup_layout.y = 0.20;
  popup_layout.width = 0.66;
  popup_layout.height = 0.58;
  popup_layout.normalized = true;
  iinuji_layout_t popup_shadow_layout = popup_layout;
  popup_shadow_layout.x += 0.006;
  popup_shadow_layout.y += 0.020;
  ui.popup_shadow = create_panel("cmd.popup.shadow", popup_shadow_layout,
                                 iinuji_style_t{"#050608", "#050608"});
  ui.popup_shadow->visible = false;
  ui.popup_shadow->z_index = 129;
  ui.root->add_child(ui.popup_shadow);
  ui.popup_overlay =
      create_panel("cmd.popup.overlay", popup_layout,
                   iinuji_style_t{"#E8EDF5", "#11151C", true, "#FFD26E", false,
                                  false, " detail  Esc closes "});
  ui.popup_overlay->visible = false;
  ui.popup_overlay->z_index = 130;
  ui.popup_text =
      create_text_box("cmd.popup.text", "", true, text_align_t::Left,
                      iinuji_layout_t{}, iinuji_style_t{"#C8DCFF", "#11151C"});
  ui.popup_text->layout.mode = layout_mode_t::Dock;
  ui.popup_text->layout.dock = dock_t::Fill;
  panel_body_object(ui.popup_overlay)->add_child(ui.popup_text);
  ui.root->add_child(ui.popup_overlay);
  return ui;
}

inline SplashUi create_splash_ui(const rgba_image_t &image,
                                 const std::string &context_title) {
  SplashUi splash{};
  const iinuji_style_t root_style{"#D8D8D8", "#101014"};
  const iinuji_style_t title_style{"#F1FFF4",
                                   "#173122",
                                   true,
                                   "#2D6A44",
                                   false,
                                   false,
                                   command_frame_title("cuwacunu_cmd")};
  const iinuji_style_t view_style{
      "#DAF5E0", "#101014", true, "#2D6A44", false, false, " waajacamaya "};
  const iinuji_style_t context_style{
      "#D6EBD9", "#101014", true, "#2D6A44", false, false, context_title};
  const iinuji_style_t status_style{"#B7D2C0", "#101014", true,      "#2D6A44",
                                    false,     false,     " status "};

  splash.root = create_grid_container(
      "cmd.splash.root",
      {len_spec::px(3), len_spec::frac(1.0), len_spec::px(3)},
      {len_spec::frac(1.0)}, 0, 0, iinuji_layout_t{}, root_style);
  splash.title =
      create_text_box("cmd.splash.title", "", false, text_align_t::Left,
                      iinuji_layout_t{}, title_style);
  place_in_grid(splash.title, 0, 0);
  splash.root->add_child(splash.title);

  auto workspace = create_grid_container(
      "cmd.splash.workspace", {len_spec::frac(1.0)},
      workspace_split_columns_for_screen(ScreenMode::Home), 1, 1,
      iinuji_layout_t{}, root_style);
  place_in_grid(workspace, 1, 0);
  splash.root->add_child(workspace);

  auto image_panel =
      create_panel("cmd.splash.image.panel", iinuji_layout_t{}, view_style);
  place_in_grid(image_panel, 0, 0);
  image_box_opts_t image_opts{};
  image_opts.image = image;
  image_opts.render_options = make_home_showcase_render_options();
  image_opts.style = iinuji_style_t{"#DAF5E0", "#101014"};
  splash.image = create_image_box("cmd.splash.image", std::move(image_opts));
  splash.image->layout.mode = layout_mode_t::Dock;
  splash.image->layout.dock = dock_t::Fill;
  panel_body_object(image_panel)->add_child(splash.image);
  workspace->add_child(image_panel);

  auto detail_panel =
      make_panel_text("cmd.splash.detail.panel", "cmd.splash.detail",
                      context_style, iinuji_style_t{"#D6EBD9", "#101014"});
  place_in_grid(detail_panel, 0, 1);
  splash.detail = panel_body_object(detail_panel)->children.front();
  workspace->add_child(detail_panel);

  splash.status =
      create_text_box("cmd.splash.status", "", false, text_align_t::Left,
                      iinuji_layout_t{}, status_style);
  place_in_grid(splash.status, 2, 0);
  splash.root->add_child(splash.status);
  return splash;
}

struct SplashStageDescriptor {
  std::string_view label{};
  std::string_view detail{};
};

inline const std::array<SplashStageDescriptor, 6> &boot_splash_stages() {
  static const std::array<SplashStageDescriptor, 6> stages{{
      {"GUI assets", "load waajacamaya image, APNG, and farewell logo"},
      {"Config catalog", "scan managed config files and active policy roots"},
      {"Runtime policy", "read Runtime Hero policy and execution root"},
      {"Runtime inventory", "collect device telemetry, jobs, and artifacts"},
      {"Lattice targets", "collect target ids and readiness fact counters"},
      {"Workbench", "keep the F-key rail around F2 Workbench"},
  }};
  return stages;
}

inline const std::array<SplashStageDescriptor, 4> &farewell_splash_stages() {
  static const std::array<SplashStageDescriptor, 4> stages{{
      {"Freeze frame", "hold the final waajacamaya panel briefly"},
      {"Close overlays", "settle menus, command prompt, and mouse capture"},
      {"Flush renderer", "return the terminal to the shell cleanly"},
      {"Good luck", "waajacu™"},
  }};
  return stages;
}

inline std::string bootstrap_splash_status_text() {
  return "waajacu™ | bootstrap in progress";
}

inline std::string farewell_splash_status_text() {
  return "waajacu™ | closing terminal";
}

template <std::size_t N>
inline void
append_splash_stages(std::ostringstream &out,
                     const std::array<SplashStageDescriptor, N> &stages,
                     double progress_ratio) {
  const double ratio = std::clamp(progress_ratio, 0.0, 1.0);
  const std::size_t stage_count = stages.size();
  const std::size_t active_index =
      ratio >= 1.0 ? stage_count
                   : std::min(stage_count - 1u,
                              static_cast<std::size_t>(ratio * stage_count));
  for (std::size_t i = 0; i < stages.size(); ++i) {
    const bool done = ratio >= 1.0 || i < active_index;
    const bool active = ratio < 1.0 && i == active_index;
    out << (done ? "[ok] " : (active ? " ●   " : "[  ] ")) << stages[i].label
        << " - " << stages[i].detail << "\n";
  }
}

inline std::string splash_progress_bar(double progress_ratio,
                                       std::size_t width = 28u) {
  progress_ratio = std::clamp(progress_ratio, 0.0, 1.0);
  const std::size_t filled =
      std::min(width, static_cast<std::size_t>(std::lround(
                          progress_ratio * static_cast<double>(width))));
  std::string out{"["};
  append_repeated(out, "█", static_cast<int>(filled));
  append_repeated(out, "░", static_cast<int>(width - filled));
  out += "] ";
  out += std::to_string(static_cast<int>(std::lround(progress_ratio * 100.0)));
  out += "%";
  return out;
}

inline std::string make_splash_detail_body_text(const std::string &phase,
                                                std::uint64_t tick,
                                                double progress_ratio) {
  std::ostringstream out;
  out << animated_rule(tick, 42) << "\n";
  out << "progress " << splash_progress_bar(progress_ratio) << "\n\n";
  if (normalize_command(phase) == "farewell")
    append_splash_stages(out, farewell_splash_stages(), progress_ratio);
  else
    append_splash_stages(out, boot_splash_stages(), progress_ratio);
  out << "\n";
  return out.str();
}

inline std::string make_splash_detail_text(const std::string &phase,
                                           std::uint64_t tick,
                                           double progress_ratio = -1.0) {
  if (progress_ratio < 0.0)
    progress_ratio = static_cast<double>(tick % 48u) / 47.0;
  std::ostringstream out;
  out << phase << "\n\n";
  out << make_splash_detail_body_text(phase, tick, progress_ratio);
  for (ScreenMode screen : screen_order()) {
    out << screen_key_label(screen) << " " << screen_badge_label(screen)
        << "\n";
  }
  return out.str();
}

inline bool splash_quit_key(int key) {
  return standard_key_action(key) == key_action_t::Close || key == 'q' ||
         key == 'Q';
}

inline void render_splash_frame(NcursesApp &app, SplashUi &splash) {
  int height = 0;
  int width = 0;
  app.renderer().size(height, width);
  app.renderer().clear();
  layout_tree(splash.root, Rect{0, 0, width, height});
  render_tree(splash.root);
  app.renderer().flush();
}

inline void run_visual_splash(NcursesApp &app, const CmdUi &ui, CmdState &state,
                              const std::string &title,
                              const std::string &phase,
                              const std::string &status, std::uint64_t hold_ms,
                              const rgba_image_t *fallback_override = nullptr,
                              bool animate = true) {
  const rgba_image_t &candidate =
      fallback_override != nullptr ? *fallback_override : ui.home_static_image;
  const rgba_image_t fallback =
      candidate.valid() ? candidate : make_fallback_home_wordmark_image();
  SplashUi splash = create_splash_ui(fallback, " " + phase + " ");
  if (splash.title)
    splash.title->style.title = command_frame_title(state.command_name);
  const std::uint64_t started_ms = now_ms();

  while (true) {
    const std::uint64_t elapsed_ms = now_ms() - started_ms;
    const std::uint64_t tick = now_ms() / 160u;
    if (animate && ui.home_animation.valid()) {
      set_image(splash.image, animation_detail::frame_for_elapsed_ms(
                                  ui.home_animation, elapsed_ms));
    }
    const double progress_ratio =
        hold_ms == 0u ? 1.0
                      : std::clamp(static_cast<double>(elapsed_ms) /
                                       static_cast<double>(hold_ms),
                                   0.0, 1.0);
    set_text(splash.title, title);
    set_text(splash.detail,
             make_splash_detail_text(phase, tick, progress_ratio));
    set_text(splash.status, status);
    render_splash_frame(app, splash);

    const int key = app.read_key();
    if (splash_quit_key(key)) {
      state.quit = true;
      state.status = "quit";
      return;
    }
    if (elapsed_ms >= hold_ms)
      return;
  }
}

inline double bootstrap_visual_ratio(double refresh_ratio) {
  return std::clamp(0.16 + (std::clamp(refresh_ratio, 0.0, 1.0) * 0.80), 0.0,
                    0.98);
}

inline void
render_bootstrap_progress_frame(NcursesApp &app, SplashUi &splash, CmdUi &ui,
                                const SnapshotRefreshProgress &progress,
                                std::uint64_t started_ms) {
  const std::uint64_t elapsed_ms = now_ms() - started_ms;
  const std::uint64_t tick = now_ms() / 160u;
  if (ui.home_animation.valid()) {
    set_image(splash.image, animation_detail::frame_for_elapsed_ms(
                                ui.home_animation, elapsed_ms));
  }
  set_text(splash.title, "loading " + std::string(progress.label));
  set_text(
      splash.detail,
      make_splash_detail_text(
          "bootstrap", tick,
          progress.complete ? 1.0 : bootstrap_visual_ratio(progress.ratio)));
  set_text(splash.status, bootstrap_splash_status_text());
  render_splash_frame(app, splash);
}

inline void run_bootstrap_refresh_splash(NcursesApp &app, CmdUi &ui,
                                         CmdState &state,
                                         std::uint64_t min_hold_ms = 700u) {
  const rgba_image_t fallback = ui.home_static_image.valid()
                                    ? ui.home_static_image
                                    : make_fallback_home_wordmark_image();
  SplashUi splash = create_splash_ui(fallback, " bootstrap ");
  if (splash.title)
    splash.title->style.title = command_frame_title(state.command_name);
  const std::uint64_t started_ms = now_ms();

  const SnapshotRefreshProgress asset_progress{
      .completed_stages = 0u,
      .total_stages = boot_splash_stages().size(),
      .label = "GUI assets",
      .detail = "load waajacamaya image, APNG, and farewell logo",
      .ratio = 0.0,
      .complete = false,
  };
  render_bootstrap_progress_frame(app, splash, ui, asset_progress, started_ms);

  refresh_snapshots_with_progress(
      state, [&](const SnapshotRefreshProgress &progress) {
        render_bootstrap_progress_frame(app, splash, ui, progress, started_ms);
      });

  SnapshotRefreshProgress ready_progress{
      .completed_stages = boot_splash_stages().size(),
      .total_stages = boot_splash_stages().size(),
      .label = "Workbench",
      .detail = "F2 Workbench with F-key rail, menus, logs, and command line",
      .ratio = 1.0,
      .complete = true,
  };

  while (now_ms() - started_ms < min_hold_ms) {
    render_bootstrap_progress_frame(app, splash, ui, ready_progress,
                                    started_ms);
    const int key = app.read_key();
    if (splash_quit_key(key)) {
      state.quit = true;
      state.status = "quit";
      return;
    }
  }
}

inline void update_home_image(CmdUi &ui, const CmdState &state) {
  if (!ui.home_image)
    return;
  ui.home_image->visible = state.screen == ScreenMode::Home;
  if (state.screen != ScreenMode::Home)
    return;
  if (state.home_visual.presentation == HomePresentationMode::FarewellSplash) {
    set_image(ui.home_image, ui.closing_static_image);
    return;
  }
  if (!ui.home_animation.valid()) {
    set_image(ui.home_image, ui.home_static_image);
    return;
  }
  const std::uint64_t now = now_ms();
  if (ui.home_animation_origin_ms == 0u)
    ui.home_animation_origin_ms = now;
  set_image(ui.home_image,
            animation_detail::frame_for_elapsed_ms(
                ui.home_animation, now - ui.home_animation_origin_ms));
}

inline void update_workspace_zoom(CmdUi &ui, const CmdState &state) {
  if (!ui.workspace || !ui.workspace->grid)
    return;

  const bool supports_zoom = workspace_screen_supports_zoom(state.screen);
  const bool zoomed = workspace_is_current_screen_zoomed(state);
  const bool zoom_left = workspace_current_screen_uses_left_panel_zoom(state);
  const bool split_left = screen_uses_split_left_panels(state.screen);
  const bool config_editor_full_workspace =
      state.screen == ScreenMode::Config && state.config.file_editor_open;
  const bool left_visible =
      !config_editor_full_workspace &&
      (!supports_zoom || !zoomed ||
       workspace_current_screen_uses_left_panel_zoom(state));

  if (config_editor_full_workspace) {
    ui.workspace->grid->cols = {len_spec::px(0), len_spec::frac(1.0)};
    if (ui.detail_panel)
      ui.detail_panel->visible = true;
  } else if (!supports_zoom || !zoomed) {
    ui.workspace->grid->cols = workspace_split_columns_for_screen(state.screen);
    if (ui.detail_panel)
      ui.detail_panel->visible = true;
  } else if (zoom_left) {
    ui.workspace->grid->cols = {len_spec::frac(1.0), len_spec::px(0)};
    if (ui.detail_panel)
      ui.detail_panel->visible = false;
  } else {
    ui.workspace->grid->cols = {len_spec::px(0), len_spec::frac(1.0)};
    if (ui.detail_panel)
      ui.detail_panel->visible = true;
  }

  if (ui.main_panel)
    ui.main_panel->visible = left_visible && !split_left;
  if (ui.split_panel)
    ui.split_panel->visible = left_visible && split_left;
  if (ui.split_panel && ui.split_panel->grid) {
    if (state.screen == ScreenMode::Runtime) {
      ui.split_panel->grid->rows = {len_spec::px(15), len_spec::frac(1.0)};
    } else if (state.screen == ScreenMode::Config ||
               state.screen == ScreenMode::Lattice) {
      ui.split_panel->grid->rows = {len_spec::px(7), len_spec::frac(1.0)};
    } else {
      ui.split_panel->grid->rows = {len_spec::frac(0.34), len_spec::frac(0.66)};
    }
  }

  const std::string main_title = screen_main_panel_title(state.screen);
  const std::string detail_title =
      state.screen == ScreenMode::Runtime
          ? runtime_detail_panel_title(state)
          : screen_detail_panel_title(state.screen);
  if (ui.main_panel)
    ui.main_panel->style.title = main_title;
  if (ui.split_nav_panel)
    ui.split_nav_panel->style.title =
        screen_split_nav_panel_title(state.screen);
  if (ui.split_worklist_panel)
    ui.split_worklist_panel->style.title =
        screen_split_worklist_panel_title(state.screen);
  if (ui.detail_panel)
    ui.detail_panel->style.title = detail_title;
  if (!supports_zoom)
    return;

  const std::string label = zoomed ? "[split]" : "[full]";
  if (zoom_left && ui.main_panel)
    ui.main_panel->style.title = panel_title_with_zoom(main_title, label);
  if (zoom_left && split_left && ui.split_worklist_panel)
    ui.split_worklist_panel->style.title = panel_title_with_zoom(
        screen_split_worklist_panel_title(state.screen), label);
  if (!zoom_left && ui.detail_panel)
    ui.detail_panel->style.title = panel_title_with_zoom(detail_title, label);
}

inline std::string config_editor_panel_title(const CmdState &state) {
  std::string title = " read-only file ";
  if (!workspace_screen_supports_zoom(state.screen))
    return title;
  const std::string label =
      workspace_is_current_screen_zoomed(state) ? "[split]" : "[full]";
  if (!workspace_current_screen_uses_left_panel_zoom(state))
    title = panel_title_with_zoom(title, label);
  return title;
}

inline std::string read_config_editor_text(const ConfigFileSummary &file) {
  std::ifstream in(file.path, std::ios::binary);
  if (!in)
    return "unavailable: " + file.path.string() + "\n";

  constexpr std::size_t kMaxEditorBytes = 512u * 1024u;
  std::ostringstream out;
  std::array<char, 4096> buffer{};
  std::size_t total = 0;
  while (in && total < kMaxEditorBytes) {
    const std::size_t remaining = kMaxEditorBytes - total;
    in.read(buffer.data(), static_cast<std::streamsize>(std::min<std::size_t>(
                               buffer.size(), remaining)));
    const std::streamsize count = in.gcount();
    if (count <= 0)
      break;
    out.write(buffer.data(), count);
    total += static_cast<std::size_t>(count);
  }
  if (in.good())
    out << "\n... truncated at " << kMaxEditorBytes << " bytes\n";
  return out.str();
}

inline bool config_editor_should_be_visible(const CmdState &state,
                                            const CmdUi &ui) {
  return state.screen == ScreenMode::Config && state.config.file_editor_open &&
         ui.config_editor && selected_config_file(state) != nullptr;
}

inline std::shared_ptr<iinuji_object_t>
active_detail_scroll_object(const CmdState &state, const CmdUi &ui) {
  if (config_editor_should_be_visible(state, ui))
    return ui.config_editor;
  return ui.detail;
}

inline void sync_config_editor_ui(CmdUi &ui, CmdState &state) {
  const bool show_editor = config_editor_should_be_visible(state, ui);
  if (ui.detail)
    ui.detail->visible = !show_editor;
  if (ui.config_editor)
    ui.config_editor->visible = show_editor;
  if (!show_editor) {
    ui.config_editor_path.clear();
    ui.config_editor_selected_file = std::numeric_limits<std::size_t>::max();
    return;
  }

  const ConfigFileSummary *file = selected_config_file(state);
  if (file == nullptr)
    return;
  if (ui.detail_panel)
    ui.detail_panel->style.title = config_editor_panel_title(state);

  auto editor = editor_data(ui.config_editor);
  if (!editor)
    return;
  if (ui.config_editor_path == file->path &&
      ui.config_editor_selected_file == state.config.selected_file) {
    editor->read_only = true;
    primitives::editor_configure_syntax_from_path(*editor);
    return;
  }

  editor->path = file->path.string();
  editor->read_only = true;
  editor->dirty = false;
  editor->status = "read only";
  primitives::editor_set_text(*editor, read_config_editor_text(*file));
  primitives::editor_configure_syntax_from_path(*editor);
  editor->dirty = false;
  ui.config_editor_path = file->path;
  ui.config_editor_selected_file = state.config.selected_file;
}

inline bool runtime_device_plot_should_be_visible(const CmdState &state,
                                                  const CmdUi &ui) {
  return state.screen == ScreenMode::Runtime &&
         state.runtime.focus == RuntimeFocus::Devices &&
         state.runtime.detail_mode == RuntimeDetailMode::Manifest &&
         ui.runtime_device_plot != nullptr &&
         !config_editor_should_be_visible(state, ui);
}

inline std::vector<std::pair<double, double>>
runtime_plot_points_from_percent_values(const std::vector<int> &values) {
  std::vector<std::pair<double, double>> points{};
  points.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    const int pct = values[i];
    if (pct < 0)
      continue;
    points.emplace_back(static_cast<double>(i),
                        static_cast<double>(std::clamp(pct, 0, 100)));
  }
  return points;
}

inline double
runtime_plot_y_max_for_percent_values(const std::vector<int> &values) {
  double max_value = 1.0;
  for (const int pct : values) {
    if (pct >= 0)
      max_value =
          std::max(max_value, static_cast<double>(std::clamp(pct, 0, 100)));
  }
  return max_value;
}

inline plot_series_cfg_t runtime_device_plot_series(std::string color,
                                                    bool use_y2_axis = false) {
  plot_series_cfg_t cfg{};
  cfg.color_fg = std::move(color);
  cfg.mode = plot_mode_t::Line;
  cfg.fill_vertical_if_same_x = false;
  cfg.use_y2_axis = use_y2_axis;
  return cfg;
}

inline bool update_runtime_device_combined_plot(
    const std::shared_ptr<iinuji_object_t> &object,
    const std::vector<int> &primary_values, std::string primary_color,
    const std::vector<int> &secondary_values, std::string secondary_color,
    std::string primary_label, std::string secondary_label) {
  if (!object)
    return false;
  object->visible = false;
  auto plot = plot_data(object);
  if (!plot)
    return false;

  std::vector<std::vector<std::pair<double, double>>> series{};
  std::vector<plot_series_cfg_t> cfg{};
  auto primary_points = runtime_plot_points_from_percent_values(primary_values);
  if (primary_points.size() >= 2u) {
    series.push_back(std::move(primary_points));
    cfg.push_back(runtime_device_plot_series(std::move(primary_color)));
  }
  auto secondary_points =
      runtime_plot_points_from_percent_values(secondary_values);
  if (secondary_points.size() >= 2u) {
    series.push_back(std::move(secondary_points));
    cfg.push_back(runtime_device_plot_series(std::move(secondary_color), true));
  }
  if (series.empty())
    return false;

  object->layout.mode = layout_mode_t::Dock;
  object->layout.dock = dock_t::Bottom;
  object->layout.dock_size = len_spec::frac(kRuntimeDevicePlotDockFraction);
  object->layout.pad_right = kRuntimeDevicePlotPadRight;
  object->style.label_color = "#C8C8CE";
  object->style.border_color = "#3B4452";

  std::size_t x_count = 1u;
  x_count = std::max(x_count, primary_values.size());
  x_count = std::max(x_count, secondary_values.size());
  plot->series = std::move(series);
  plot->series_cfg = std::move(cfg);
  plot->opts.x_min = 0.0;
  plot->opts.x_max =
      static_cast<double>(std::max<std::size_t>(1u, x_count - 1u));
  plot->opts.y_min = 0.0;
  plot->opts.y_max = runtime_plot_y_max_for_percent_values(primary_values);
  plot->opts.y2_min = 0.0;
  plot->opts.y2_max = runtime_plot_y_max_for_percent_values(secondary_values);
  plot->opts.draw_axes = true;
  plot->opts.draw_grid = true;
  plot->opts.baseline0 = false;
  plot->opts.x_ticks = 0;
  plot->opts.y_ticks = 3;
  plot->opts.draw_y_tick_labels = true;
  plot->opts.draw_y2_tick_labels = true;
  plot->opts.draw_x_tick_labels = false;
  plot->opts.margin_left = 5;
  plot->opts.margin_right = 6;
  plot->opts.margin_top = 1;
  plot->opts.margin_bot = 0;
  plot->opts.x_label.clear();
  plot->opts.y_label = std::move(primary_label);
  plot->opts.y2_label = std::move(secondary_label);
  object->visible = true;
  return true;
}

inline void update_runtime_device_plot_ui(CmdUi &ui, const CmdState &state) {
  const bool want_plot = runtime_device_plot_should_be_visible(state, ui);
  if (ui.runtime_device_plot)
    ui.runtime_device_plot->visible = false;
  if (ui.detail) {
    ui.detail->layout.mode = layout_mode_t::Dock;
    ui.detail->layout.dock = dock_t::Fill;
    ui.detail->layout.dock_size = len_spec::frac(1.0);
  }
  if (!want_plot)
    return;

  std::vector<int> primary_values{};
  std::vector<int> secondary_values{};
  std::string primary_color{};
  std::string secondary_color{"#67c1ff"};
  std::string primary_label{};
  std::string secondary_label{};
  if (state.runtime.selected_device_row == 0 ||
      state.runtime.device.gpus.empty()) {
    primary_values = host_cpu_history_values(state.runtime.device_history);
    secondary_values = host_memory_history_values(state.runtime.device_history);
    primary_color = "#f0d060";
    primary_label = "cpu%";
    secondary_label = "ram%";
  } else {
    const std::size_t gpu_index = state.runtime.selected_device_row - 1u;
    if (gpu_index >= state.runtime.device.gpus.size()) {
      primary_values = host_cpu_history_values(state.runtime.device_history);
      secondary_values =
          host_memory_history_values(state.runtime.device_history);
      primary_color = "#f0d060";
      primary_label = "cpu%";
      secondary_label = "ram%";
    } else {
      const RuntimeGpuSnapshot &gpu = state.runtime.device.gpus[gpu_index];
      primary_values =
          gpu_util_history_values(state.runtime.device_history, gpu.uuid);
      secondary_values =
          gpu_memory_history_values(state.runtime.device_history, gpu.uuid);
      primary_color = "#7fe08a";
      primary_label = "util%";
      secondary_label = "vram%";
    }
  }

  (void)update_runtime_device_combined_plot(
      ui.runtime_device_plot, secondary_values, secondary_color, primary_values,
      primary_color, secondary_label, primary_label);
}

inline void clamp_action_menu_selection(CmdState &state,
                                        const ActionRegistry &actions);

inline MenuOverlayColumns
make_action_menu_overlay_columns(const CmdState &state,
                                 const ActionRegistry &actions);

inline constexpr int kActionMenuHeaderRows = 14;
inline constexpr int kActionMenuVisibleReserve = 10;
inline constexpr int kMenuCloseRow = 1;
inline constexpr int kMenuActionsRow = 2;
inline constexpr int kMenuScreenFirstRow = 8;
inline constexpr int kRuntimeFocusSelectorRow = 2;
inline constexpr int kRuntimeDetailSelectorRow = 3;
inline constexpr int kRuntimeDeviceHeaderRow = 7;
inline constexpr int kConfigBrowserFirstRow = 9;
inline constexpr int kLatticeBrowserFirstRow = 10;
inline constexpr int kSplitWorklistFirstRow = 1;

inline std::vector<styled_text_line_t>
make_split_nav_styled_lines(const CmdState &state) {
  switch (state.screen) {
  case ScreenMode::Runtime:
    return style_content_text(make_runtime_nav_text(state));
  case ScreenMode::Lattice:
    return style_content_text(make_lattice_nav_text(state));
  case ScreenMode::Config:
    return style_content_text(make_config_nav_text(state));
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Logs:
    break;
  }
  return {};
}

inline std::vector<styled_text_line_t>
make_split_worklist_styled_lines(const CmdState &state) {
  switch (state.screen) {
  case ScreenMode::Runtime:
    return style_content_text(make_runtime_worklist_text(state));
  case ScreenMode::Lattice:
    return style_content_text(make_lattice_worklist_text(state));
  case ScreenMode::Config:
    return style_content_text(make_config_worklist_text(state));
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Logs:
    break;
  }
  return {};
}

enum class RuntimeChoiceAction : std::uint8_t {
  RuntimePolicy = 0,
  RuntimeJobManifest = 1,
  RuntimeJobState = 2,
  RuntimeJobReport = 3,
  RuntimeJobTrace = 4,
  RefreshRuntime = 5,
};

struct RuntimeChoiceMenuRow {
  primitives::choice_menu_option_t option{};
  RuntimeChoiceAction action{RuntimeChoiceAction::RuntimePolicy};
};

inline std::string runtime_choice_device_subject(const CmdState &state) {
  if (state.runtime.selected_device_row == 0u ||
      state.runtime.device.gpus.empty()) {
    return "host " + display_or(state.runtime.device.host_name);
  }
  const std::size_t gpu_index = state.runtime.selected_device_row - 1u;
  if (gpu_index >= state.runtime.device.gpus.size())
    return "gpu";
  const RuntimeGpuSnapshot &gpu = state.runtime.device.gpus[gpu_index];
  return "gpu " + display_or(gpu.index) + " " + display_or(gpu.name);
}

inline std::string runtime_choice_job_subject(const CmdState &state) {
  const RuntimeJobSummary *job = selected_job(state);
  if (job == nullptr)
    return "jobs";
  return "job " + display_or(job->job_id);
}

inline std::string runtime_choice_subject(const CmdState &state) {
  return state.runtime.focus == RuntimeFocus::Jobs
             ? runtime_choice_job_subject(state)
             : runtime_choice_device_subject(state);
}

inline RuntimeChoiceMenuRow runtime_choice_row(std::string label,
                                               std::string detail,
                                               RuntimeChoiceAction action) {
  RuntimeChoiceMenuRow row{};
  row.option.label = std::move(label);
  row.option.detail = std::move(detail);
  row.action = action;
  return row;
}

inline std::vector<RuntimeChoiceMenuRow>
make_runtime_choice_menu_rows(const CmdState &state) {
  std::vector<RuntimeChoiceMenuRow> rows{};
  rows.reserve(6u);

  if (state.runtime.focus == RuntimeFocus::Jobs) {
    if (selected_job(state) != nullptr) {
      rows.push_back(runtime_choice_row(
          "open job.manifest", "job summary, artifacts, report preview",
          RuntimeChoiceAction::RuntimeJobManifest));
      rows.push_back(runtime_choice_row("open job.state",
                                        "job state and report tail",
                                        RuntimeChoiceAction::RuntimeJobState));
      rows.push_back(runtime_choice_row("open report",
                                        "report preview if recorded",
                                        RuntimeChoiceAction::RuntimeJobReport));
      rows.push_back(runtime_choice_row("open trace",
                                        "trace preview or manifest fallback",
                                        RuntimeChoiceAction::RuntimeJobTrace));
    }
    rows.push_back(runtime_choice_row("refresh job artifacts",
                                      "reload runtime jobs and telemetry",
                                      RuntimeChoiceAction::RefreshRuntime));
  } else {
    const bool host = state.runtime.selected_device_row == 0u ||
                      state.runtime.device.gpus.empty();
    rows.push_back(runtime_choice_row("inspect runtime policy",
                                      "open policy and active wave popup",
                                      RuntimeChoiceAction::RuntimePolicy));
    rows.push_back(
        runtime_choice_row(host ? "refresh host sample" : "refresh GPU sample",
                           "reload telemetry without changing the active view",
                           RuntimeChoiceAction::RefreshRuntime));
  }

  return rows;
}

inline primitives::choice_menu_model_t
make_runtime_choice_menu_model(const CmdState &state) {
  primitives::choice_menu_model_t model{};
  model.title = "RUNTIME MENU";
  model.selected = state.choice_menu.selected;
  model.scroll_y = state.choice_menu.scroll_y;
  model.visible_reserve = 8u;
  model.label_width = 30u;
  model.detail_width = 42u;
  const std::string subject = compact_text(runtime_choice_subject(state), 42u);
  model.options.push_back(primitives::choice_menu_option_t{
      .label = "selected",
      .detail = subject,
      .enabled = false,
      .label_emphasis = text_line_emphasis_t::Accent,
      .detail_emphasis = text_line_emphasis_t::Info,
  });
  for (auto &row : make_runtime_choice_menu_rows(state))
    model.options.push_back(std::move(row.option));
  return model;
}

inline std::size_t runtime_choice_action_offset() { return 1u; }

inline void clamp_choice_menu_selection(CmdState &state) {
  if (!cmd_choice_menu_open(state))
    return;
  std::size_t total = 0u;
  switch (state.choice_menu.kind) {
  case CmdChoiceMenuKind::RuntimeSelection:
    total = make_runtime_choice_menu_model(state).options.size();
    break;
  case CmdChoiceMenuKind::None:
    break;
  }
  state.choice_menu.selected = primitives::choice_menu_clamped_selection(
      state.choice_menu.selected, total);
  if (state.choice_menu.kind == CmdChoiceMenuKind::RuntimeSelection &&
      state.choice_menu.selected == 0u && total > 1u) {
    state.choice_menu.selected = 1u;
  }
  state.choice_menu.scroll_y = primitives::choice_menu_scroll_for_selection(
      state.choice_menu.selected, state.choice_menu.scroll_y, total, 10u);
}

inline primitives::choice_menu_columns_t
make_choice_menu_overlay_columns(CmdState state) {
  clamp_choice_menu_selection(state);
  switch (state.choice_menu.kind) {
  case CmdChoiceMenuKind::RuntimeSelection:
    return primitives::choice_menu_columns(
        make_runtime_choice_menu_model(state));
  case CmdChoiceMenuKind::None:
    break;
  }
  return {};
}

inline std::string
action_menu_selected_position_text(const CmdState &state,
                                   const ActionRegistry &actions) {
  const std::size_t total = actions.actions().size();
  if (total == 0u)
    return "0/0";
  const std::size_t selected = std::min(state.action_menu_selected, total - 1u);
  return std::to_string(selected + 1u) + "/" + std::to_string(total);
}

inline std::string choice_menu_selected_position_text(CmdState state) {
  if (!cmd_choice_menu_open(state))
    return "0/0";
  clamp_choice_menu_selection(state);
  const auto columns = make_choice_menu_overlay_columns(state);
  const std::size_t option_total =
      columns.labels.size() >
              static_cast<std::size_t>(primitives::kChoiceMenuHeaderRows)
          ? columns.labels.size() -
                static_cast<std::size_t>(primitives::kChoiceMenuHeaderRows)
          : 0u;
  const std::size_t total = option_total > runtime_choice_action_offset()
                                ? option_total - runtime_choice_action_offset()
                                : option_total;
  if (total == 0u)
    return "0/0";
  const std::size_t selected =
      state.choice_menu.selected >= runtime_choice_action_offset()
          ? state.choice_menu.selected - runtime_choice_action_offset()
          : 0u;
  return std::to_string(std::min(selected + 1u, total)) + "/" +
         std::to_string(total);
}

inline CmdContentPopupKind
runtime_choice_popup_kind(RuntimeChoiceAction action) {
  switch (action) {
  case RuntimeChoiceAction::RuntimePolicy:
    return CmdContentPopupKind::RuntimePolicy;
  case RuntimeChoiceAction::RuntimeJobManifest:
    return CmdContentPopupKind::RuntimeJobManifest;
  case RuntimeChoiceAction::RuntimeJobState:
    return CmdContentPopupKind::RuntimeJobState;
  case RuntimeChoiceAction::RuntimeJobReport:
    return CmdContentPopupKind::RuntimeJobReport;
  case RuntimeChoiceAction::RuntimeJobTrace:
    return CmdContentPopupKind::RuntimeJobTrace;
  case RuntimeChoiceAction::RefreshRuntime:
    break;
  }
  return CmdContentPopupKind::None;
}

inline std::string content_popup_title(const CmdState &state) {
  switch (state.content_popup.kind) {
  case CmdContentPopupKind::RuntimePolicy:
    return " runtime policy  Esc close  [x] ";
  case CmdContentPopupKind::RuntimeJobManifest:
    return " job.manifest  Esc close  [x] ";
  case CmdContentPopupKind::RuntimeJobState:
    return " job.state  Esc close  [x] ";
  case CmdContentPopupKind::RuntimeJobReport:
    return " report  Esc close  [x] ";
  case CmdContentPopupKind::RuntimeJobTrace:
    return " trace  Esc close  [x] ";
  case CmdContentPopupKind::None:
    break;
  }
  return " detail  Esc close  [x] ";
}

inline void append_runtime_popup_file(std::ostringstream &out,
                                      const std::filesystem::path &path,
                                      std::string_view label,
                                      int max_lines = 80) {
  append_detail_section_header(out, std::string(label));
  if (path.empty()) {
    out << "not recorded\n";
    return;
  }
  out << path.string() << "\n";
  out << "status: " << runtime_artifact_status(path) << "\n\n";
  append_runtime_file_preview(out, path, max_lines);
}

inline std::string make_content_popup_text(const CmdState &state) {
  std::ostringstream out;
  switch (state.content_popup.kind) {
  case CmdContentPopupKind::RuntimePolicy: {
    CmdState policy_state = state;
    policy_state.runtime.focus = RuntimeFocus::Devices;
    return make_runtime_trace_detail_text(policy_state);
  }
  case CmdContentPopupKind::RuntimeJobManifest:
  case CmdContentPopupKind::RuntimeJobState:
  case CmdContentPopupKind::RuntimeJobReport:
  case CmdContentPopupKind::RuntimeJobTrace: {
    const RuntimeJobSummary *job = selected_job(state);
    if (job == nullptr) {
      out << "no runtime job selected\n";
      return out.str();
    }
    if (state.content_popup.kind == CmdContentPopupKind::RuntimeJobManifest) {
      append_runtime_popup_file(out, job->job_dir / "job.manifest",
                                "job.manifest");
      return out.str();
    }
    if (state.content_popup.kind == CmdContentPopupKind::RuntimeJobState) {
      append_runtime_popup_file(out, job->job_dir / "job.state", "job.state");
      return out.str();
    }
    if (state.content_popup.kind == CmdContentPopupKind::RuntimeJobReport) {
      append_runtime_popup_file(out, runtime_report_preview_path(*job),
                                "report");
      return out.str();
    }
    append_runtime_popup_file(out, runtime_trace_preview_path(*job), "trace");
    return out.str();
  }
  case CmdContentPopupKind::None:
    break;
  }
  return {};
}

inline void update_ui(CmdUi &ui, CmdState &state,
                      const ActionRegistry &actions) {
  clamp_action_menu_selection(state, actions);
  clamp_choice_menu_selection(state);
  apply_screen_theme(ui, state);
  update_workspace_zoom(ui, state);
  update_home_image(ui, state);
  set_styled_lines(ui.title, make_tab_styled_lines(state), false);
  set_styled_lines(ui.nav, make_tab_styled_lines(state), false);
  set_styled_lines(ui.main, make_main_styled_lines(state), true, false);
  if (screen_uses_split_left_panels(state.screen)) {
    set_styled_lines(ui.split_nav, make_split_nav_styled_lines(state), true,
                     false);
    const bool wrap_split_worklist = state.screen != ScreenMode::Runtime;
    set_styled_lines(ui.split_worklist, make_split_worklist_styled_lines(state),
                     wrap_split_worklist, false);
  }
  set_styled_lines(ui.detail, make_detail_styled_lines(state), true, false);
  sync_config_editor_ui(ui, state);
  update_runtime_device_plot_ui(ui, state);
  if (state.screen == ScreenMode::Logs)
    scroll_text_to(ui.main, state.logs.scroll_y, state.logs.scroll_x);
  set_styled_lines(ui.status, make_bottom_status_styled_lines(state), false);
  set_text(ui.command_prompt, command_prompt_status(state) + " ");
  sync_command_input_focus(state, ui);
  const bool choice_open = cmd_choice_menu_open(state);
  const bool popup_open = cmd_content_popup_open(state);
  const bool choice_visible = choice_open && !popup_open;
  if (ui.menu_overlay) {
    ui.menu_overlay->visible = state.help_view || state.action_menu_open;
    if (state.action_menu_open) {
      ui.menu_overlay->style.title =
          " actions " + action_menu_selected_position_text(state, actions) +
          "  Enter/iinuji.actions.run  "
          "j/k move  "
          "Esc/iinuji.actions.close  [x] ";
      const auto columns = make_action_menu_overlay_columns(state, actions);
      set_styled_lines(ui.menu_commands, columns.commands, false);
      set_styled_lines(ui.menu_comments, columns.comments, false);
    } else {
      ui.menu_overlay->style.title = " menu  Esc closes  [x] ";
      const auto columns = make_menu_overlay_columns(state);
      set_styled_lines(ui.menu_commands, columns.commands, false);
      set_styled_lines(ui.menu_comments, columns.comments, false);
    }
    const int scroll_y = state.action_menu_open ? state.action_menu_scroll_y
                                                : state.help_scroll_y;
    const int scroll_x = state.action_menu_open ? 0 : state.help_scroll_x;
    scroll_text_to(ui.menu_commands, scroll_y, scroll_x);
    scroll_text_to(ui.menu_comments, scroll_y, scroll_x);
  }
  if (ui.choice_shadow)
    ui.choice_shadow->visible = choice_visible;
  if (ui.choice_overlay) {
    ui.choice_overlay->visible = choice_visible;
    if (choice_visible) {
      ui.choice_overlay->style.title =
          " runtime item " + choice_menu_selected_position_text(state) +
          "  Enter choose  j/k move  Esc close  [x] ";
      const auto columns = make_choice_menu_overlay_columns(state);
      set_styled_lines(ui.choice_commands, columns.labels, false);
      set_styled_lines(ui.choice_comments, columns.details, false);
      scroll_text_to(ui.choice_commands, state.choice_menu.scroll_y, 0);
      scroll_text_to(ui.choice_comments, state.choice_menu.scroll_y, 0);
    }
  }
  if (ui.popup_shadow)
    ui.popup_shadow->visible = popup_open;
  if (ui.popup_overlay) {
    ui.popup_overlay->visible = popup_open;
    if (popup_open) {
      ui.popup_overlay->style.title = content_popup_title(state);
      set_styled_lines(ui.popup_text,
                       style_content_text(make_content_popup_text(state)), true,
                       false);
      scroll_text_to(ui.popup_text, state.content_popup.scroll_y,
                     state.content_popup.scroll_x);
    }
  }
}

inline std::string operator_status_log_scope(ScreenMode screen) {
  std::string scope = screen_label(screen);
  std::transform(scope.begin(), scope.end(), scope.begin(),
                 [](unsigned char ch) {
                   return ch == ' ' ? '_' : static_cast<char>(std::tolower(ch));
                 });
  return scope;
}

inline void mirror_operator_status_to_logs(CmdState &state) {
  if (state.screen == ScreenMode::Logs)
    return;
  const std::string status = trim(state.status);
  if (status.empty())
    return;

  const std::string scope = operator_status_log_scope(state.screen);
  if (scope == state.last_status_log_scope &&
      status == state.last_status_log_text) {
    return;
  }

  state.last_status_log_scope = scope;
  state.last_status_log_text = status;
  append_log(state, "status", scope + " | " + status);
}

inline bool scroll_help_overlay(CmdState &state, int key) {
  if (!state.help_view)
    return false;
  std::string status;
  key_action_t action = standard_key_action(key);
  if (key == 'k' || key == 'K')
    action = key_action_t::MoveUp;
  else if (key == 'j' || key == 'J')
    action = key_action_t::MoveDown;
  else if (key == 'h' || key == 'H' || key == '[')
    action = key_action_t::MoveLeft;
  else if (key == 'l' || key == 'L' || key == ']')
    action = key_action_t::MoveRight;

  switch (action) {
  case key_action_t::MoveUp:
    state.help_scroll_y = std::max(0, state.help_scroll_y - 3);
    status = "help scroll=up";
    break;
  case key_action_t::MoveDown:
    state.help_scroll_y += 3;
    status = "help scroll=down";
    break;
  case key_action_t::MoveLeft:
    state.help_scroll_x = std::max(0, state.help_scroll_x - 16);
    status = "help scroll=left";
    break;
  case key_action_t::MoveRight:
    state.help_scroll_x += 16;
    status = "help scroll=right";
    break;
  case key_action_t::PageUp:
    state.help_scroll_y = std::max(0, state.help_scroll_y - 20);
    status = "help scroll=page-up";
    break;
  case key_action_t::PageDown:
    state.help_scroll_y += 20;
    status = "help scroll=page-down";
    break;
  case key_action_t::Home:
    state.help_scroll_y = 0;
    state.help_scroll_x = 0;
    status = "help scroll=home";
    break;
  case key_action_t::End:
    state.help_scroll_y = std::numeric_limits<int>::max();
    status = "help scroll=end";
    break;
  default:
    return false;
  }
  state.status = std::move(status);
  return true;
}

inline std::string action_aliases_text(const Action &action) {
  std::ostringstream out;
  for (std::size_t i = 0; i < action.aliases.size(); ++i) {
    if (i > 0)
      out << " | ";
    out << action.aliases[i];
  }
  return out.str();
}

inline std::string action_category_label(ActionId id) {
  switch (id) {
  case ActionId::ShowHome:
  case ActionId::ShowWorkbench:
  case ActionId::ShowRuntime:
  case ActionId::ShowLattice:
  case ActionId::ShowConfig:
  case ActionId::ShowLogs:
    return "screen";
  case ActionId::HomeVisual:
  case ActionId::HomeSplash:
  case ActionId::HomeFarewell:
  case ActionId::ShowCurrent:
  case ActionId::ShowIdentity:
  case ActionId::ShowHomeSummary:
  case ActionId::ShowWorkbenchSummary:
  case ActionId::ShowRuntimeSummary:
  case ActionId::ShowLatticeSummary:
  case ActionId::ShowLogsSummary:
  case ActionId::ShowConfigSummary:
    return "show";
  case ActionId::Help:
  case ActionId::CloseHelp:
  case ActionId::HelpScrollUp:
  case ActionId::HelpScrollDown:
  case ActionId::HelpScrollLeft:
  case ActionId::HelpScrollRight:
  case ActionId::HelpScrollPageUp:
  case ActionId::HelpScrollPageDown:
  case ActionId::HelpScrollHome:
  case ActionId::HelpScrollEnd:
  case ActionId::ActionMenu:
  case ActionId::ActionMenuClose:
  case ActionId::ActionMenuSelectPrev:
  case ActionId::ActionMenuSelectNext:
  case ActionId::ActionMenuSelectPageUp:
  case ActionId::ActionMenuSelectPageDown:
  case ActionId::ActionMenuSelectHome:
  case ActionId::ActionMenuSelectEnd:
    return "menu";
  case ActionId::Refresh:
  case ActionId::WorkbenchRefresh:
  case ActionId::RuntimeRefresh:
  case ActionId::LatticeRefresh:
  case ActionId::ConfigReload:
    return "refresh";
  case ActionId::ClearLogs:
  case ActionId::LogsScrollUp:
  case ActionId::LogsScrollDown:
  case ActionId::LogsScrollPageUp:
  case ActionId::LogsScrollPageDown:
  case ActionId::LogsScrollHome:
  case ActionId::LogsScrollEnd:
  case ActionId::ToggleLogsFollow:
  case ActionId::ToggleLogsTimestamp:
  case ActionId::ToggleLogsThread:
  case ActionId::ToggleLogsMetadata:
  case ActionId::ToggleLogsColor:
  case ActionId::ToggleLogsMouseCapture:
  case ActionId::CycleLogsSourceFilter:
  case ActionId::SetLogsSourceAll:
  case ActionId::SetLogsSourceRefresh:
  case ActionId::SetLogsSourceAction:
  case ActionId::SetLogsSourceCommand:
  case ActionId::SetLogsSourceShow:
  case ActionId::SetLogsSourceStatus:
  case ActionId::CycleLogsSeverityFilter:
  case ActionId::CycleLogsMetadataFilter:
  case ActionId::SetLogsSeverityDebug:
  case ActionId::SetLogsSeverityInfo:
  case ActionId::SetLogsSeverityWarning:
  case ActionId::SetLogsSeverityError:
  case ActionId::SetLogsSeverityFatal:
  case ActionId::SetLogsMetadataAny:
  case ActionId::SetLogsMetadataAnyMeta:
  case ActionId::SetLogsMetadataFunction:
  case ActionId::SetLogsMetadataPath:
  case ActionId::SetLogsMetadataCallsite:
  case ActionId::LogsSelectSettingPrev:
  case ActionId::LogsSelectSettingNext:
  case ActionId::LogsAdjustSettingPrev:
  case ActionId::LogsAdjustSettingNext:
    return "logs";
  case ActionId::RuntimeFocusDevices:
  case ActionId::RuntimeFocusJobs:
  case ActionId::RuntimeFocusPrev:
  case ActionId::RuntimeFocusNext:
  case ActionId::RuntimeDetailNext:
  case ActionId::RuntimeDetailManifest:
  case ActionId::RuntimeDetailLog:
  case ActionId::RuntimeDetailTrace:
  case ActionId::RuntimeItemPrev:
  case ActionId::RuntimeItemNext:
    return "runtime";
  case ActionId::ConfigFiles:
  case ActionId::ConfigShow:
  case ActionId::ConfigFileShow:
  case ActionId::ConfigFilePrev:
  case ActionId::ConfigFileNext:
  case ActionId::ConfigFileFirst:
  case ActionId::ConfigFileLast:
    return "config";
  case ActionId::LatticeTargetPrev:
  case ActionId::LatticeTargetNext:
  case ActionId::LatticeTargetFirst:
  case ActionId::LatticeTargetLast:
    return "lattice";
  case ActionId::ToggleWorkspaceZoom:
  case ActionId::RestoreWorkspaceSplit:
    return "workspace";
  case ActionId::Quit:
  case ActionId::Exit:
    return "app";
  }
  return "action";
}

inline std::string action_menu_detail_text(const CmdState &state,
                                           const Action &action) {
  if (action.id == ActionId::ShowHome)
    return "[screen] home | iinuji.screen.home()";
  if (action.id == ActionId::HomeVisual)
    return "--visual | --image | --animation | iinuji.home.visual()";
  if (action.id == ActionId::HomeSplash)
    return "[show] --splash/--boot | loading | iinuji.splash()";
  if (action.id == ActionId::HomeFarewell)
    return "[show] --farewell/--closing | iinuji.farewell()";
  if (action.id == ActionId::ShowLogs)
    return "[screen] shell logs | logs | f8";
  if (action.id == ActionId::ShowWorkbench)
    return "[screen] workbench | iinuji.screen.workbench()";
  if (action.id == ActionId::ShowLatticeSummary)
    return "[show] selected target proof | iinuji.show.lattice()";
  if (action.id == ActionId::ShowIdentity)
    return "[show] --version | iinuji.version()";
  if (action.id == ActionId::ConfigFileShow)
    return "[config] selected file preview | iinuji.config.file.show()";

  std::string detail = "[" + action_category_label(action.id) + "]";
  auto append_part = [&](const std::string &part) -> bool {
    if (part.empty())
      return false;
    const std::string candidate =
        detail + (detail.back() == ']' ? " " : " | ") + part;
    if (candidate.size() > kMenuOverlayCommentWidth)
      return false;
    detail = candidate;
    return true;
  };

  std::string canonical{};
  for (const auto &alias : action.aliases) {
    if (alias.rfind("iinuji.", 0) == 0) {
      canonical = alias;
      break;
    }
  }

  std::size_t shown_aliases = 0u;
  if (!action.aliases.empty()) {
    if (append_part(action.aliases.front()))
      ++shown_aliases;
  } else if (append_part("internal action")) {
    ++shown_aliases;
  }
  if (!canonical.empty() && canonical != action.aliases.front() &&
      append_part(canonical)) {
    ++shown_aliases;
  }

  const std::size_t hidden_aliases = action.aliases.size() > shown_aliases
                                         ? action.aliases.size() - shown_aliases
                                         : 0u;
  if (hidden_aliases > 0u) {
    (void)append_part("+" + std::to_string(hidden_aliases) + " aliases");
  }
  if ((action.id == ActionId::ToggleWorkspaceZoom ||
       action.id == ActionId::RestoreWorkspaceSplit) &&
      !workspace_screen_supports_zoom(state.screen)) {
    (void)append_part("unavailable here");
  }
  return detail;
}

inline std::string
action_menu_selected_group_text(const std::vector<Action> &rows,
                                std::size_t selected) {
  if (rows.empty())
    return "-";
  selected = std::min(selected, rows.size() - 1u);
  const std::string category = action_category_label(rows[selected].id);
  std::size_t ordinal = 0u;
  std::size_t total = 0u;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (action_category_label(rows[i].id) != category)
      continue;
    ++total;
    if (i == selected)
      ordinal = total;
  }
  return category + " " + std::to_string(ordinal) + "/" + std::to_string(total);
}

inline MenuOverlayColumns
make_action_menu_overlay_columns(const CmdState &state,
                                 const ActionRegistry &actions) {
  MenuOverlayColumns columns{};
  const auto &rows = actions.actions();
  const std::size_t selected =
      rows.empty() ? 0u
                   : std::min(state.action_menu_selected, rows.size() - 1u);

  push_menu_header(columns, "ACTION MENU");
  push_menu_header(columns, "CONTROLS");
  const std::string selected_text =
      rows.empty()
          ? "0/0"
          : std::to_string(selected + 1u) + "/" + std::to_string(rows.size());
  push_menu_row(columns, "selected", selected_text,
                text_line_emphasis_t::Accent, text_line_emphasis_t::Info);
  push_menu_row(columns, "group",
                action_menu_selected_group_text(rows, selected),
                text_line_emphasis_t::Accent, text_line_emphasis_t::Info);
  push_menu_row(columns, "Enter", "run selected action",
                text_line_emphasis_t::Warning);
  push_menu_row(columns, "iinuji.actions.run()", "run selected action");
  push_menu_row(columns, "Esc / [x]", "close action menu",
                text_line_emphasis_t::Warning);
  push_menu_row(columns, "iinuji.actions.close()", "close action menu");
  push_menu_row(columns, "Up/Down or j/k",
                "move selection; PgUp/PgDn page; Home/End edge");
  push_menu_row(columns, "iinuji.actions.select.*()", "move action selection");
  push_menu_header(columns, "GROUPS");
  push_menu_row(columns, "groups", "screen/show/menu/refresh/logs/runtime");
  push_menu_row(columns, "groups+", "config/lattice/workspace/app");
  push_menu_header(columns, "ACTIONS");

  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto &action = rows[i];
    const bool is_selected = i == selected;

    const std::string detail = action_menu_detail_text(state, action);
    push_menu_row(columns, (is_selected ? "> " : "  ") + action.label, detail,
                  is_selected ? text_line_emphasis_t::Warning
                              : text_line_emphasis_t::None,
                  is_selected ? text_line_emphasis_t::Warning
                              : text_line_emphasis_t::Info,
                  is_selected ? "#2A2418" : std::string{});
  }

  return columns;
}

inline void clamp_action_menu_selection(CmdState &state,
                                        const ActionRegistry &actions) {
  const auto size = actions.actions().size();
  if (size == 0u) {
    state.action_menu_selected = 0u;
    state.action_menu_scroll_y = 0;
    return;
  }
  if (state.action_menu_selected >= size)
    state.action_menu_selected = size - 1u;

  const int visible =
      std::min<int>(kActionMenuVisibleReserve, static_cast<int>(size));
  const int selected = static_cast<int>(state.action_menu_selected);
  const int max_top =
      std::max(0, static_cast<int>(size) - std::max(1, visible));
  int top = state.action_menu_scroll_y > kActionMenuHeaderRows
                ? state.action_menu_scroll_y - kActionMenuHeaderRows
                : 0;
  top = std::clamp(top, 0, max_top);
  if (selected < top || selected >= top + visible)
    top = std::clamp(selected, 0, max_top);
  state.action_menu_scroll_y = top == 0 ? 0 : kActionMenuHeaderRows + top;
}

inline bool move_action_menu_selection(CmdState &state,
                                       const ActionRegistry &actions,
                                       int delta) {
  const auto size = actions.actions().size();
  if (size == 0u)
    return false;
  const int max_index = static_cast<int>(size) - 1;
  const int current = static_cast<int>(state.action_menu_selected);
  const int next = std::clamp(current + delta, 0, max_index);
  state.action_menu_selected = static_cast<std::size_t>(next);
  clamp_action_menu_selection(state, actions);
  state.status = "action menu";
  return true;
}

inline bool dispatch_selected_action_menu_item(const ActionRegistry &actions,
                                               CmdState &state) {
  const auto &rows = actions.actions();
  if (rows.empty())
    return false;
  clamp_action_menu_selection(state, actions);
  const ActionId selected_id = rows[state.action_menu_selected].id;
  state.action_menu_open = false;
  state.action_menu_scroll_y = 0;
  return actions.dispatch(selected_id, state);
}

inline bool handle_action_menu_key(int key, const ActionRegistry &actions,
                                   CmdState &state) {
  if (!state.action_menu_open)
    return false;

  const key_action_t action = standard_key_action(key);
  if (key == 'k' || key == 'K')
    return move_action_menu_selection(state, actions, -1);
  if (key == 'j' || key == 'J')
    return move_action_menu_selection(state, actions, 1);
  if (action == key_action_t::Cancel) {
    state.action_menu_open = false;
    state.action_menu_scroll_y = 0;
    state.status = screen_label(state.screen);
    return true;
  }
  if (is_enter_key(key))
    return dispatch_selected_action_menu_item(actions, state);
  if (action == key_action_t::MoveUp)
    return move_action_menu_selection(state, actions, -1);
  if (action == key_action_t::MoveDown)
    return move_action_menu_selection(state, actions, 1);
  if (action == key_action_t::PageUp)
    return move_action_menu_selection(state, actions, -8);
  if (action == key_action_t::PageDown)
    return move_action_menu_selection(state, actions, 8);
  if (action == key_action_t::Home)
    return move_action_menu_selection(
        state, actions, -static_cast<int>(actions.actions().size()));
  if (action == key_action_t::End)
    return move_action_menu_selection(
        state, actions, static_cast<int>(actions.actions().size()));
  return true;
}

inline std::size_t first_choice_menu_enabled_index(const CmdState &state) {
  switch (state.choice_menu.kind) {
  case CmdChoiceMenuKind::RuntimeSelection:
    return runtime_choice_action_offset();
  case CmdChoiceMenuKind::None:
    break;
  }
  return 0u;
}

inline std::size_t choice_menu_option_count(const CmdState &state) {
  switch (state.choice_menu.kind) {
  case CmdChoiceMenuKind::RuntimeSelection:
    return make_runtime_choice_menu_model(state).options.size();
  case CmdChoiceMenuKind::None:
    break;
  }
  return 0u;
}

inline bool move_choice_menu_selection(CmdState &state, int delta) {
  if (!cmd_choice_menu_open(state))
    return false;
  const std::size_t total = choice_menu_option_count(state);
  if (total == 0u)
    return false;
  const std::size_t first =
      std::min(first_choice_menu_enabled_index(state), total - 1u);
  const int min_index = static_cast<int>(first);
  const int max_index = static_cast<int>(total - 1u);
  const int current = static_cast<int>(
      std::clamp(state.choice_menu.selected, first, total - 1u));
  state.choice_menu.selected = static_cast<std::size_t>(
      std::clamp(current + delta, min_index, max_index));
  clamp_choice_menu_selection(state);
  state.status = "runtime menu";
  return true;
}

inline bool choose_runtime_menu_action(const ActionRegistry &actions,
                                       CmdState &state,
                                       RuntimeChoiceAction action) {
  const CmdContentPopupKind popup_kind = runtime_choice_popup_kind(action);
  if (popup_kind != CmdContentPopupKind::None) {
    open_content_popup(state, popup_kind, true);
    state.status = "runtime popup";
    return true;
  }

  close_choice_menu(state);
  close_content_popup(state);
  switch (action) {
  case RuntimeChoiceAction::RuntimePolicy:
  case RuntimeChoiceAction::RuntimeJobManifest:
  case RuntimeChoiceAction::RuntimeJobState:
  case RuntimeChoiceAction::RuntimeJobReport:
  case RuntimeChoiceAction::RuntimeJobTrace:
    return false;
  case RuntimeChoiceAction::RefreshRuntime:
    return actions.dispatch(ActionId::RuntimeRefresh, state);
  }
  return false;
}

inline bool dispatch_selected_choice_menu_item(const ActionRegistry &actions,
                                               CmdState &state) {
  if (!cmd_choice_menu_open(state))
    return false;
  switch (state.choice_menu.kind) {
  case CmdChoiceMenuKind::RuntimeSelection: {
    const auto rows = make_runtime_choice_menu_rows(state);
    if (rows.empty())
      return false;
    clamp_choice_menu_selection(state);
    const std::size_t offset = runtime_choice_action_offset();
    if (state.choice_menu.selected < offset)
      return false;
    const std::size_t index = state.choice_menu.selected - offset;
    if (index >= rows.size())
      return false;
    return choose_runtime_menu_action(actions, state, rows[index].action);
  }
  case CmdChoiceMenuKind::None:
    break;
  }
  return false;
}

inline bool handle_choice_menu_key(int key, const ActionRegistry &actions,
                                   CmdState &state) {
  if (!cmd_choice_menu_open(state))
    return false;

  const key_action_t action = standard_key_action(key);
  if (key == 'k' || key == 'K')
    return move_choice_menu_selection(state, -1);
  if (key == 'j' || key == 'J')
    return move_choice_menu_selection(state, 1);
  if (action == key_action_t::Cancel) {
    close_choice_menu(state);
    state.status = screen_label(state.screen);
    return true;
  }
  if (is_enter_key(key))
    return dispatch_selected_choice_menu_item(actions, state);
  if (action == key_action_t::MoveUp)
    return move_choice_menu_selection(state, -1);
  if (action == key_action_t::MoveDown)
    return move_choice_menu_selection(state, 1);
  if (action == key_action_t::PageUp)
    return move_choice_menu_selection(state, -8);
  if (action == key_action_t::PageDown)
    return move_choice_menu_selection(state, 8);
  if (action == key_action_t::Home)
    return move_choice_menu_selection(
        state, -static_cast<int>(choice_menu_option_count(state)));
  if (action == key_action_t::End)
    return move_choice_menu_selection(
        state, static_cast<int>(choice_menu_option_count(state)));
  return true;
}

inline bool scroll_content_popup_by(CmdState &state, int dy, int dx = 0) {
  if (!cmd_content_popup_open(state) || (dy == 0 && dx == 0))
    return false;
  state.content_popup.scroll_y = std::max(0, state.content_popup.scroll_y + dy);
  state.content_popup.scroll_x = std::max(0, state.content_popup.scroll_x + dx);
  state.status = "popup scroll";
  return true;
}

inline bool handle_content_popup_key(int key, CmdState &state) {
  if (!cmd_content_popup_open(state))
    return false;

  const key_action_t action = standard_key_action(key);
  if (action == key_action_t::Cancel) {
    close_content_popup(state);
    state.status = cmd_choice_menu_open(state) ? "runtime menu"
                                               : screen_label(state.screen);
    return true;
  }
  if (action == key_action_t::MoveUp || key == 'k' || key == 'K')
    return scroll_content_popup_by(state, -1);
  if (action == key_action_t::MoveDown || key == 'j' || key == 'J')
    return scroll_content_popup_by(state, 1);
  if (action == key_action_t::PageUp)
    return scroll_content_popup_by(state, -8);
  if (action == key_action_t::PageDown)
    return scroll_content_popup_by(state, 8);
  if (action == key_action_t::Home) {
    state.content_popup.scroll_y = 0;
    state.content_popup.scroll_x = 0;
    state.status = "popup scroll";
    return true;
  }
  if (action == key_action_t::End)
    return scroll_content_popup_by(state, 10000);
  return true;
}

inline bool overlay_close_key(int key) { return key == 'x' || key == 'X'; }

inline bool
dispatch_selected_logs_setting_adjustment(const ActionRegistry &actions,
                                          CmdState &state, int delta = 1) {
  return actions.dispatch(delta < 0 ? ActionId::LogsAdjustSettingPrev
                                    : ActionId::LogsAdjustSettingNext,
                          state);
}

inline bool dispatch_function_key(int key, const ActionRegistry &actions,
                                  CmdState &state) {
  if (key == KEY_F(1))
    return actions.dispatch(ActionId::ShowHome, state);
  if (key == KEY_F(2))
    return actions.dispatch(ActionId::ShowWorkbench, state);
  if (key == KEY_F(3))
    return actions.dispatch(ActionId::ShowRuntime, state);
  if (key == KEY_F(4))
    return actions.dispatch(ActionId::ShowLattice, state);
  if (key == KEY_F(8))
    return actions.dispatch(ActionId::ShowLogs, state);
  if (key == KEY_F(9))
    return actions.dispatch(ActionId::ShowConfig, state);
  return false;
}

inline int function_key_from_csi_number(int number) {
  switch (number) {
  case 11:
    return KEY_F(1);
  case 12:
    return KEY_F(2);
  case 13:
    return KEY_F(3);
  case 14:
    return KEY_F(4);
  case 15:
    return KEY_F(5);
  case 17:
    return KEY_F(6);
  case 18:
    return KEY_F(7);
  case 19:
    return KEY_F(8);
  case 20:
    return KEY_F(9);
  default:
    return key_escape;
  }
}

inline int function_key_from_linux_console_suffix(int ch) {
  switch (ch) {
  case 'A':
    return KEY_F(1);
  case 'B':
    return KEY_F(2);
  case 'C':
    return KEY_F(3);
  case 'D':
    return KEY_F(4);
  case 'E':
    return KEY_F(5);
  default:
    return key_escape;
  }
}

inline int key_from_csi_final(int ch) {
  switch (ch) {
  case 'A':
    return key_up;
  case 'B':
    return key_down;
  case 'C':
    return key_right;
  case 'D':
    return key_left;
  case 'H':
    return key_home;
  case 'F':
    return key_end;
  case 'Z':
    return key_shift_tab;
  default:
    return key_escape;
  }
}

inline int read_csi_number_sequence(NcursesApp &app, int first_digit) {
  int number = 0;
  int ch = first_digit;
  while (ch >= '0' && ch <= '9') {
    number = (number * 10) + (ch - '0');
    ch = app.read_raw_key();
    if (ch == ERR)
      return key_escape;
  }

  if (ch == ';') {
    do {
      ch = app.read_raw_key();
      if (ch == ERR)
        return key_escape;
    } while (!((ch >= 'A' && ch <= 'Z') || ch == '~'));
  }

  if (ch == '~') {
    switch (number) {
    case 1:
    case 7:
      return key_home;
    case 3:
      return key_delete;
    case 4:
    case 8:
      return key_end;
    case 5:
      return key_page_up;
    case 6:
      return key_page_down;
    default:
      return function_key_from_csi_number(number);
    }
  }

  return key_from_csi_final(ch);
}

class ScopedInputTimeout {
public:
  ScopedInputTimeout(int temporary_timeout_ms, int restore_timeout_ms)
      : restore_timeout_ms_(restore_timeout_ms) {
    ::wtimeout(stdscr, temporary_timeout_ms);
  }

  ~ScopedInputTimeout() { ::wtimeout(stdscr, restore_timeout_ms_); }

  ScopedInputTimeout(const ScopedInputTimeout &) = delete;
  ScopedInputTimeout &operator=(const ScopedInputTimeout &) = delete;

private:
  int restore_timeout_ms_;
};

inline int read_terminal_escape_sequence_key(NcursesApp &app,
                                             int restore_timeout_ms) {
  const ScopedInputTimeout scoped_timeout(kEscapeSequenceTimeoutMs,
                                          restore_timeout_ms);
  const int prefix = app.read_raw_key();
  if (prefix == ERR)
    return key_escape;

  if (prefix == '[') {
    const int ch = app.read_raw_key();
    if (ch == ERR)
      return key_escape;
    if (ch == '[') {
      const int suffix = app.read_raw_key();
      if (suffix == ERR)
        return key_escape;
      return function_key_from_linux_console_suffix(suffix);
    }
    if (ch >= '0' && ch <= '9')
      return read_csi_number_sequence(app, ch);
    return key_from_csi_final(ch);
  }

  if (prefix == 'O') {
    const int ch = app.read_raw_key();
    if (ch == ERR)
      return key_escape;
    switch (ch) {
    case 'H':
      return key_home;
    case 'F':
      return key_end;
    case 'P':
      return KEY_F(1);
    case 'Q':
      return KEY_F(2);
    case 'R':
      return KEY_F(3);
    case 'S':
      return KEY_F(4);
    default:
      return key_escape;
    }
  }

  return key_escape;
}

inline int read_command_key(NcursesApp &app, int current_timeout_ms) {
  const int key = app.read_key();
  if (key != key_escape)
    return key;
  return read_terminal_escape_sequence_key(app, current_timeout_ms);
}

inline void apply_logs_mouse_capture(const CmdState &state) {
  mousemask(state.logs.mouse_capture ? ALL_MOUSE_EVENTS : 0, nullptr);
  mouseinterval(0);
}

inline bool mouse_has(MEVENT event, mmask_t mask) {
  return mask != 0 && (event.bstate & mask) != 0;
}

inline bool mouse_left_click(MEVENT event) {
  return mouse_has(event, BUTTON1_PRESSED) ||
         mouse_has(event, BUTTON1_CLICKED) ||
         mouse_has(event, BUTTON1_DOUBLE_CLICKED) ||
         mouse_has(event, BUTTON1_TRIPLE_CLICKED);
}

inline bool mouse_wheel_up(MEVENT event) {
  return mouse_has(event, BUTTON4_PRESSED) ||
         mouse_has(event, BUTTON4_CLICKED) ||
         mouse_has(event, BUTTON4_DOUBLE_CLICKED) ||
         mouse_has(event, BUTTON4_TRIPLE_CLICKED);
}

inline bool mouse_wheel_down(MEVENT event) {
  return mouse_has(event, BUTTON5_PRESSED) ||
         mouse_has(event, BUTTON5_CLICKED) ||
         mouse_has(event, BUTTON5_DOUBLE_CLICKED) ||
         mouse_has(event, BUTTON5_TRIPLE_CLICKED);
}

inline bool mouse_horizontal_modifier(MEVENT event) {
  return mouse_has(event, BUTTON_SHIFT) || mouse_has(event, BUTTON_CTRL) ||
         mouse_has(event, BUTTON_ALT);
}

inline bool
object_contains_point(const std::shared_ptr<iinuji_object_t> &object, int x,
                      int y) {
  return object != nullptr && object->visible &&
         pt_in_rect(object->screen, x, y);
}

inline bool command_panel_hit(const CmdUi &ui, int x, int y) {
  return object_contains_point(ui.command_panel, x, y) ||
         object_contains_point(ui.command_prompt, x, y) ||
         object_contains_point(ui.input, x, y) ||
         object_contains_point(ui.command_hint, x, y);
}

inline bool overlay_close_point(const std::shared_ptr<iinuji_object_t> &object,
                                int x, int y) {
  if (object == nullptr || !object->visible)
    return false;
  const Rect &screen = object->screen;
  return y == screen.y && x >= screen.x + std::max(0, screen.w - 5) &&
         x < screen.x + screen.w;
}

inline ActionId screen_action_id(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Home:
    return ActionId::ShowHome;
  case ScreenMode::Workbench:
    return ActionId::ShowWorkbench;
  case ScreenMode::Runtime:
    return ActionId::ShowRuntime;
  case ScreenMode::Lattice:
    return ActionId::ShowLattice;
  case ScreenMode::Logs:
    return ActionId::ShowLogs;
  case ScreenMode::Config:
    return ActionId::ShowConfig;
  }
  return ActionId::ShowHome;
}

inline ActionId refresh_action_id(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Workbench:
    return ActionId::WorkbenchRefresh;
  case ScreenMode::Runtime:
    return ActionId::RuntimeRefresh;
  case ScreenMode::Lattice:
    return ActionId::LatticeRefresh;
  case ScreenMode::Config:
    return ActionId::ConfigReload;
  case ScreenMode::Home:
  case ScreenMode::Logs:
    break;
  }
  return ActionId::Refresh;
}

inline std::string nav_rail_label_for_screen(const CmdState &state,
                                             ScreenMode screen) {
  return screen_tab_display_text(state, screen);
}

inline bool dispatch_nav_rail_click(const ActionRegistry &actions,
                                    CmdState &state, const CmdUi &ui, int x,
                                    int y) {
  if (state.help_view || state.action_menu_open ||
      cmd_choice_menu_open(state) || cmd_content_popup_open(state))
    return false;

  std::shared_ptr<iinuji_object_t> rail = nullptr;
  Rect content{};
  for (const auto &candidate : {ui.title, ui.nav}) {
    if (!object_contains_point(candidate, x, y))
      continue;
    const Rect candidate_content = content_rect(*candidate);
    if (!pt_in_rect(candidate_content, x, y))
      continue;
    rail = candidate;
    content = candidate_content;
    break;
  }
  if (!rail)
    return false;

  int cursor = content.x;
  bool first = true;
  for (ScreenMode screen : screen_order()) {
    if (!first)
      ++cursor;
    first = false;

    const std::string label = nav_rail_label_for_screen(state, screen);
    const std::string core = screen_tab_core_text(screen);
    const int end = cursor + static_cast<int>(label.size());
    const std::size_t core_offset = label.find(core);
    const int hit_start =
        (state.screen == screen || core_offset == std::string::npos)
            ? cursor
            : cursor + static_cast<int>(core_offset);
    const int hit_end =
        (state.screen == screen || core_offset == std::string::npos)
            ? end
            : hit_start + static_cast<int>(core.size());
    if (x >= hit_start && x < hit_end)
      return actions.dispatch(screen_action_id(screen), state);
    cursor = end;
  }

  return false;
}

inline bool active_split_left_visible(const CmdState &state, const CmdUi &ui) {
  return screen_uses_split_left_panels(state.screen) && ui.split_panel &&
         ui.split_panel->visible;
}

inline std::shared_ptr<iinuji_object_t> active_left_panel(const CmdState &state,
                                                          const CmdUi &ui) {
  if (active_split_left_visible(state, ui))
    return ui.split_panel;
  return ui.main_panel;
}

inline std::shared_ptr<iinuji_object_t>
active_left_worklist_panel(const CmdState &state, const CmdUi &ui) {
  if (active_split_left_visible(state, ui))
    return ui.split_worklist_panel;
  return ui.main_panel;
}

inline std::shared_ptr<iinuji_object_t>
active_left_nav_text(const CmdState &state, const CmdUi &ui) {
  if (active_split_left_visible(state, ui) && ui.split_nav)
    return ui.split_nav;
  return ui.main;
}

inline std::shared_ptr<iinuji_object_t>
active_left_worklist_text(const CmdState &state, const CmdUi &ui) {
  if (active_split_left_visible(state, ui) && ui.split_worklist)
    return ui.split_worklist;
  return ui.main;
}

inline bool logs_panel_top_edge_hit(const CmdState &state, const CmdUi &ui,
                                    int x, int y) {
  if (state.screen != ScreenMode::Logs || state.help_view ||
      state.action_menu_open || cmd_choice_menu_open(state) ||
      cmd_content_popup_open(state) || ui.main_panel == nullptr ||
      !ui.main_panel->visible) {
    return false;
  }
  const Rect &screen = ui.main_panel->screen;
  return y == screen.y && x >= screen.x && x < screen.x + screen.w;
}

inline bool logs_panel_bottom_edge_hit(const CmdState &state, const CmdUi &ui,
                                       int x, int y) {
  if (state.screen != ScreenMode::Logs || state.help_view ||
      state.action_menu_open || cmd_choice_menu_open(state) ||
      cmd_content_popup_open(state) || ui.main_panel == nullptr ||
      !ui.main_panel->visible) {
    return false;
  }
  const Rect &screen = ui.main_panel->screen;
  return y == screen.y + std::max(0, screen.h - 1) && x >= screen.x &&
         x < screen.x + screen.w;
}

inline bool browser_panel_top_edge_hit(const CmdState &state, const CmdUi &ui,
                                       int x, int y) {
  if ((state.screen != ScreenMode::Config &&
       state.screen != ScreenMode::Lattice) ||
      state.help_view || state.action_menu_open ||
      cmd_choice_menu_open(state) || cmd_content_popup_open(state)) {
    return false;
  }
  const auto panel = active_left_worklist_panel(state, ui);
  if (panel == nullptr || !panel->visible)
    return false;
  const Rect &screen = panel->screen;
  return y == screen.y && x >= screen.x && x < screen.x + screen.w;
}

inline bool browser_panel_bottom_edge_hit(const CmdState &state,
                                          const CmdUi &ui, int x, int y) {
  if ((state.screen != ScreenMode::Config &&
       state.screen != ScreenMode::Lattice) ||
      state.help_view || state.action_menu_open ||
      cmd_choice_menu_open(state) || cmd_content_popup_open(state)) {
    return false;
  }
  const auto panel = active_left_worklist_panel(state, ui);
  if (panel == nullptr || !panel->visible)
    return false;
  const Rect &screen = panel->screen;
  return y == screen.y + std::max(0, screen.h - 1) && x >= screen.x &&
         x < screen.x + screen.w;
}

inline const std::shared_ptr<iinuji_object_t> &
workspace_zoom_control_panel(const CmdState &state, const CmdUi &ui) {
  return workspace_current_screen_uses_left_panel_zoom(state) ? ui.main_panel
                                                              : ui.detail_panel;
}

inline bool workspace_zoom_control_hit(const CmdState &state, const CmdUi &ui,
                                       int x, int y) {
  if (!workspace_screen_supports_zoom(state.screen) || state.help_view ||
      state.action_menu_open || cmd_choice_menu_open(state) ||
      cmd_content_popup_open(state)) {
    return false;
  }
  const auto &panel = workspace_zoom_control_panel(state, ui);
  if (panel == nullptr || !panel->visible)
    return false;
  const Rect &screen = panel->screen;
  const std::size_t label_size =
      workspace_is_current_screen_zoomed(state) ? 7u : 6u;
  const int x0 =
      screen.x + std::max(0, screen.w - static_cast<int>(label_size) - 2);
  return y == screen.y && x >= x0 && x < screen.x + screen.w;
}

inline void persist_logs_scroll_from_main(CmdState &state, const CmdUi &ui) {
  if (state.screen != ScreenMode::Logs)
    return;
  if (const auto data = text_data(ui.main)) {
    state.logs.scroll_y = data->scroll_y;
    state.logs.scroll_x = data->scroll_x;
  }
}

inline bool scroll_help_overlay_by(CmdState &state, int dy, int dx) {
  if (!state.help_view || (dy == 0 && dx == 0))
    return false;
  state.help_scroll_y = std::max(0, state.help_scroll_y + dy);
  state.help_scroll_x = std::max(0, state.help_scroll_x + dx);
  if (dy < 0)
    state.status = "help scroll=up";
  else if (dy > 0)
    state.status = "help scroll=down";
  else if (dx < 0)
    state.status = "help scroll=left";
  else
    state.status = "help scroll=right";
  return true;
}

inline std::string logs_scroll_status_for_key(int key) {
  switch (standard_key_action(key)) {
  case key_action_t::MoveUp:
    return "logs scroll=up";
  case key_action_t::MoveDown:
    return "logs scroll=down";
  case key_action_t::PageUp:
    return "logs scroll=page-up";
  case key_action_t::PageDown:
    return "logs scroll=page-down";
  case key_action_t::Home:
    return "logs scroll=home";
  case key_action_t::End:
    return "logs scroll=end";
  default:
    return "logs scroll";
  }
}

inline bool scroll_action_menu_by(const ActionRegistry &actions,
                                  CmdState &state, int dy) {
  if (!state.action_menu_open || dy == 0)
    return false;
  const int step = dy > 0 ? 3 : -3;
  return move_action_menu_selection(state, actions, step);
}

inline bool scroll_choice_menu_by(CmdState &state, int dy) {
  if (!cmd_choice_menu_open(state) || dy == 0)
    return false;
  const int step = dy > 0 ? 3 : -3;
  return move_choice_menu_selection(state, step);
}

inline bool menu_point_inside_action_rows(const CmdUi &ui, int x, int y) {
  return object_contains_point(ui.menu_commands, x, y) ||
         object_contains_point(ui.menu_comments, x, y);
}

inline bool choice_point_inside_action_rows(const CmdUi &ui, int x, int y) {
  return object_contains_point(ui.choice_commands, x, y) ||
         object_contains_point(ui.choice_comments, x, y);
}

inline bool action_menu_index_at_point(const ActionRegistry &actions,
                                       const CmdState &state, const CmdUi &ui,
                                       int x, int y, std::size_t &index) {
  if (!state.action_menu_open || !menu_point_inside_action_rows(ui, x, y))
    return false;

  const auto &anchor = object_contains_point(ui.menu_commands, x, y)
                           ? ui.menu_commands
                           : ui.menu_comments;
  const int rendered_row = y - anchor->screen.y;
  const int content_row = rendered_row + state.action_menu_scroll_y;
  if (content_row < kActionMenuHeaderRows)
    return false;

  const std::size_t candidate =
      static_cast<std::size_t>(content_row - kActionMenuHeaderRows);
  if (candidate >= actions.actions().size())
    return false;

  index = candidate;
  return true;
}

inline bool dispatch_action_menu_click(const ActionRegistry &actions,
                                       CmdState &state, const CmdUi &ui, int x,
                                       int y, bool run_selected) {
  std::size_t index = 0;
  if (!action_menu_index_at_point(actions, state, ui, x, y, index))
    return false;

  state.action_menu_selected = index;
  clamp_action_menu_selection(state, actions);
  if (run_selected)
    return dispatch_selected_action_menu_item(actions, state);
  state.status = "action menu";
  return true;
}

inline bool choice_menu_index_at_point(const CmdState &state, const CmdUi &ui,
                                       int x, int y, std::size_t &index) {
  if (!cmd_choice_menu_open(state) ||
      !choice_point_inside_action_rows(ui, x, y))
    return false;

  const auto &anchor = object_contains_point(ui.choice_commands, x, y)
                           ? ui.choice_commands
                           : ui.choice_comments;
  const int rendered_row = y - anchor->screen.y;
  const int content_row = rendered_row + state.choice_menu.scroll_y;
  if (content_row < primitives::kChoiceMenuHeaderRows)
    return false;

  const std::size_t candidate =
      static_cast<std::size_t>(content_row - primitives::kChoiceMenuHeaderRows);
  if (candidate < first_choice_menu_enabled_index(state))
    return false;
  if (candidate >= choice_menu_option_count(state))
    return false;

  index = candidate;
  return true;
}

inline bool dispatch_choice_menu_click(const ActionRegistry &actions,
                                       CmdState &state, const CmdUi &ui, int x,
                                       int y, bool run_selected) {
  std::size_t index = 0;
  if (!choice_menu_index_at_point(state, ui, x, y, index))
    return false;

  state.choice_menu.selected = index;
  clamp_choice_menu_selection(state);
  if (run_selected)
    return dispatch_selected_choice_menu_item(actions, state);
  state.status = "runtime menu";
  return true;
}

inline bool menu_overlay_row_at_point(const CmdState &state, const CmdUi &ui,
                                      int x, int y, int &row) {
  if (!state.help_view || state.action_menu_open)
    return false;
  const auto &anchor = object_contains_point(ui.menu_commands, x, y)
                           ? ui.menu_commands
                           : ui.menu_comments;
  if (!object_contains_point(anchor, x, y))
    return false;
  row = y - anchor->screen.y + state.help_scroll_y;
  return true;
}

inline bool dispatch_menu_alias_action(const ActionRegistry &actions,
                                       CmdState &state, ActionId id) {
  state.help_view = false;
  state.action_menu_open = false;
  return actions.dispatch(id, state);
}

inline bool dispatch_menu_alias_command(const ActionRegistry &actions,
                                        CmdState &state, std::string command) {
  state.help_view = false;
  state.action_menu_open = false;
  return actions.dispatch(std::move(command), state);
}

inline int menu_section_header_row(const CmdState &state,
                                   std::string_view header) {
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  for (std::size_t i = 0; i < columns.commands.size(); ++i) {
    if (trim(columns.commands[i].text) == header)
      return static_cast<int>(i);
  }
  return static_cast<int>(columns.commands.size());
}

inline int menu_command_alias_end_row(const CmdState &state) {
  return menu_section_header_row(state, "CANONICAL PATHS");
}

inline std::optional<std::string>
menu_canonical_command_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "CANONICAL PATHS") + 1;
  const int end = menu_section_header_row(state, "COMMAND LINE");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline bool menu_canonical_pattern_row(std::string_view command) {
  return command.empty() || command.find('*') != std::string_view::npos ||
         command.find("TOKEN") != std::string_view::npos;
}

inline std::optional<std::string>
menu_command_alias_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "COMMAND ALIASES") + 1;
  if (row < first)
    return std::nullopt;
  if (row >= menu_command_alias_end_row(state))
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

template <std::size_t N>
inline bool
menu_command_is_one_of(std::string_view command,
                       const std::array<std::string_view, N> &commands) {
  return std::find(commands.begin(), commands.end(), command) != commands.end();
}

inline std::optional<ActionId>
home_utility_action_for_menu_text(std::string_view command) {
  static constexpr std::array<std::string_view, 13> kVisualCommands{{
      "home | visual | waajacamaya",
      "Home showcase",
      "visual / waajacamaya",
      "cuwacunu.cmd --visual",
      "cuwacunu.cmd --visual / --home-visual",
      "cuwacunu.cmd --home-visual",
      "cuwacunu.cmd --image",
      "cuwacunu.cmd --animation",
      "cuwacunu_cmd --visual",
      "cuwacunu_cmd --visual / --home-visual",
      "cuwacunu_cmd --home-visual",
      "cuwacunu_cmd --image",
      "cuwacunu_cmd --animation",
  }};
  static constexpr std::array<std::string_view, 18> kBootstrapCommands{{
      "splash | bootstrap",
      "splash / bootstrap",
      "cuwacunu.cmd --splash",
      "cuwacunu.cmd --splash=bootstrap",
      "cuwacunu.cmd --splash bootstrap",
      "cuwacunu.cmd --splash=loading",
      "cuwacunu.cmd --splash loading",
      "cuwacunu.cmd --bootstrap",
      "cuwacunu.cmd --bootstrap / --boot",
      "cuwacunu.cmd --boot",
      "cuwacunu_cmd --splash",
      "cuwacunu_cmd --splash=bootstrap",
      "cuwacunu_cmd --splash bootstrap",
      "cuwacunu_cmd --splash=loading",
      "cuwacunu_cmd --splash loading",
      "cuwacunu_cmd --bootstrap",
      "cuwacunu_cmd --bootstrap / --boot",
      "cuwacunu_cmd --boot",
  }};
  static constexpr std::array<std::string_view, 22> kFarewellCommands{{
      "farewell | closing",
      "farewell / closing",
      "cuwacunu.cmd --farewell",
      "cuwacunu.cmd --farewell / --closing",
      "cuwacunu.cmd --splash=farewell",
      "cuwacunu.cmd --splash farewell",
      "cuwacunu.cmd --splash=close",
      "cuwacunu.cmd --splash close",
      "cuwacunu.cmd --splash=closing",
      "cuwacunu.cmd --splash closing",
      "cuwacunu.cmd --splash=good-luck",
      "cuwacunu.cmd --splash good-luck",
      "cuwacunu_cmd --farewell",
      "cuwacunu_cmd --farewell / --closing",
      "cuwacunu_cmd --splash=farewell",
      "cuwacunu_cmd --splash farewell",
      "cuwacunu_cmd --splash=close",
      "cuwacunu_cmd --splash close",
      "cuwacunu_cmd --splash=closing",
      "cuwacunu_cmd --splash closing",
      "cuwacunu_cmd --splash=good-luck",
      "cuwacunu_cmd --splash good-luck",
  }};

  if (menu_command_is_one_of(command, kVisualCommands) ||
      command == "cuwacunu.cmd --waajacamaya" ||
      command == "cuwacunu_cmd --waajacamaya")
    return ActionId::HomeVisual;
  if (menu_command_is_one_of(command, kBootstrapCommands))
    return ActionId::HomeSplash;
  if (menu_command_is_one_of(command, kFarewellCommands) ||
      command == "cuwacunu.cmd --closing" ||
      command == "cuwacunu_cmd --closing")
    return ActionId::HomeFarewell;
  return std::nullopt;
}

inline std::optional<ActionId>
menu_command_alias_action_for_text(std::string_view command) {
  if (const auto home_action = home_utility_action_for_menu_text(command))
    return home_action;

  using AliasRoute = std::pair<std::string_view, ActionId>;
  static constexpr auto routes = std::to_array<AliasRoute>({
      {"help | h | ? | menu", ActionId::Help},
      {"version | about", ActionId::ShowIdentity},
      {"home | f1 | 1", ActionId::ShowHome},
      {"workbench | work | w | f2 | 2", ActionId::ShowWorkbench},
      {"runtime | run | rt | f3 | 3", ActionId::ShowRuntime},
      {"lattice | lat | proof | f4 | 4", ActionId::ShowLattice},
      {"logs | log | shell logs | events | f8 | 8", ActionId::ShowLogs},
      {"config | cfg | f9 | 9", ActionId::ShowConfig},
      {"show | iinuji.show()", ActionId::ShowCurrent},
      {"show home | iinuji.show.home()", ActionId::ShowHomeSummary},
      {"home show | show home", ActionId::ShowHomeSummary},
      {"show workbench | iinuji.show.workbench()",
       ActionId::ShowWorkbenchSummary},
      {"workbench show | show workbench", ActionId::ShowWorkbenchSummary},
      {"show runtime | iinuji.show.runtime()", ActionId::ShowRuntimeSummary},
      {"runtime show | show runtime", ActionId::ShowRuntimeSummary},
      {"show lattice | iinuji.show.lattice()", ActionId::ShowLatticeSummary},
      {"show target | show proof", ActionId::ShowLatticeSummary},
      {"lattice show | proof show", ActionId::ShowLatticeSummary},
      {"show shell logs | iinuji.show.logs()", ActionId::ShowLogsSummary},
      {"logs show | shell logs show", ActionId::ShowLogsSummary},
      {"show config | iinuji.show.config()", ActionId::ShowConfigSummary},
      {"config show | show config", ActionId::ConfigShow},
      {"refresh | reload | r", ActionId::Refresh},
      {"help down | help page down", ActionId::HelpScrollDown},
      {"menu down | menu page down", ActionId::HelpScrollDown},
      {"clear logs | logs clear | clear", ActionId::ClearLogs},
      {"logs filter | source next", ActionId::CycleLogsSourceFilter},
      {"logs level | severity", ActionId::CycleLogsSeverityFilter},
      {"logs meta | metadata filter", ActionId::CycleLogsMetadataFilter},
      {"logs time | timestamp", ActionId::ToggleLogsTimestamp},
      {"logs follow | follow", ActionId::ToggleLogsFollow},
      {"a | actions | palette | commands", ActionId::ActionMenu},
      {"actions down / palette down / commands down",
       ActionId::ActionMenuSelectNext},
      {"runtime down / up", ActionId::RuntimeItemNext},
      {"runtime right / left", ActionId::RuntimeFocusNext},
  });
  for (const auto &[text, action] : routes) {
    if (command == text)
      return action;
  }
  return std::nullopt;
}

inline std::optional<int> menu_current_screen_first_row(const CmdState &state) {
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  for (std::size_t i = 0; i < columns.commands.size(); ++i) {
    if (trim(columns.commands[i].text) == "CURRENT SCREEN")
      return static_cast<int>(i + 1u);
  }
  return std::nullopt;
}

inline std::optional<std::string>
menu_current_screen_command_text_at(const CmdState &state, int row) {
  const auto first = menu_current_screen_first_row(state);
  if (!first.has_value() || row < *first)
    return std::nullopt;
  const int end = menu_section_header_row(state, "MENU SCROLL UTILITIES");
  if (row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline bool menu_command_row_label_hit(const CmdUi &ui, int x, int y,
                                       std::string_view command_text,
                                       std::string_view label) {
  if (!object_contains_point(ui.menu_commands, x, y))
    return false;
  const std::size_t offset = command_text.find(label);
  if (offset == std::string_view::npos)
    return false;
  const int start = ui.menu_commands->screen.x + static_cast<int>(offset);
  const int end = start + static_cast<int>(label.size());
  return x >= start && x < end;
}

inline bool menu_command_row_last_label_hit(const CmdUi &ui, int x, int y,
                                            std::string_view command_text,
                                            std::string_view label) {
  if (!object_contains_point(ui.menu_commands, x, y))
    return false;
  const std::size_t offset = command_text.rfind(label);
  if (offset == std::string_view::npos)
    return false;
  const int start = ui.menu_commands->screen.x + static_cast<int>(offset);
  const int end = start + static_cast<int>(label.size());
  return x >= start && x < end;
}

inline bool dispatch_menu_current_screen_row(const ActionRegistry &actions,
                                             CmdState &state, const CmdUi &ui,
                                             int x, int y, int row) {
  const auto command = menu_current_screen_command_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };
  const auto scroll_detail = [&](int dy) {
    if (!scroll_text_by(ui.detail, dy))
      return false;
    state.help_view = false;
    state.action_menu_open = false;
    state.status = "context scroll";
    return true;
  };

  if (*command == "version | about")
    return dispatch(ActionId::ShowIdentity);

  switch (state.screen) {
  case ScreenMode::Home:
    if (*command == "F2 / F3 / F4 / F8 / F9") {
      if (hit("F2"))
        return dispatch(ActionId::ShowWorkbench);
      if (hit("F3"))
        return dispatch(ActionId::ShowRuntime);
      if (hit("F4"))
        return dispatch(ActionId::ShowLattice);
      if (hit("F8"))
        return dispatch(ActionId::ShowLogs);
      if (hit("F9"))
        return dispatch(ActionId::ShowConfig);
    }
    if (const auto home_action = home_utility_action_for_menu_text(*command))
      return dispatch(*home_action);
    return false;

  case ScreenMode::Workbench:
    if (*command == "F3 / F4 / F8 / F9") {
      if (hit("F3"))
        return dispatch(ActionId::ShowRuntime);
      if (hit("F4"))
        return dispatch(ActionId::ShowLattice);
      if (hit("F8"))
        return dispatch(ActionId::ShowLogs);
      if (hit("F9"))
        return dispatch(ActionId::ShowConfig);
      return dispatch(ActionId::ShowRuntime);
    }
    return false;

  case ScreenMode::Runtime:
    if (*command == "Left / Right") {
      if (hit("Left"))
        return dispatch(ActionId::RuntimeFocusPrev);
      if (hit("Right"))
        return dispatch(ActionId::RuntimeFocusNext);
      return dispatch(ActionId::RuntimeFocusNext);
    }
    if (*command == "d / J") {
      if (hit("d"))
        return dispatch(ActionId::RuntimeFocusDevices);
      if (hit("J"))
        return dispatch(ActionId::RuntimeFocusJobs);
      return dispatch(ActionId::RuntimeFocusDevices);
    }
    if (*command == "k / j / h / l") {
      if (hit("k") || hit("h"))
        return dispatch(ActionId::RuntimeItemPrev);
      return dispatch(ActionId::RuntimeItemNext);
    }
    if (*command == "Enter menu | a actions") {
      if (hit("a") || hit("actions"))
        return dispatch(ActionId::ActionMenu);
      state.help_view = false;
      return open_runtime_selected_menu(state);
    }
    if (*command == "Tab" || *command == "Tab / Shift-Tab" ||
        *command == "Runtime detail") {
      return dispatch(ActionId::RuntimeDetailNext);
    }
    if (*command == "manifest / log / trace" ||
        *command == "runtime detail.*") {
      if (hit("manifest"))
        return dispatch(ActionId::RuntimeDetailManifest);
      if (hit("log"))
        return dispatch(ActionId::RuntimeDetailLog);
      if (hit("trace"))
        return dispatch(ActionId::RuntimeDetailTrace);
      return dispatch(ActionId::RuntimeDetailNext);
    }
    if (*command == "Up / Down") {
      if (hit("Up"))
        return dispatch(ActionId::RuntimeItemPrev);
      if (hit("Down"))
        return dispatch(ActionId::RuntimeItemNext);
      return dispatch(ActionId::RuntimeItemNext);
    }
    if (*command == "PgUp / PgDn")
      return scroll_detail(hit("PgUp") ? -8 : 8);
    if (*command == "f / z")
      return dispatch(ActionId::ToggleWorkspaceZoom);
    if (*command == "r")
      return dispatch(ActionId::RuntimeRefresh);
    return false;

  case ScreenMode::Lattice:
    if (*command == "Up / Down / k / j") {
      if (hit("Up"))
        return dispatch(ActionId::LatticeTargetPrev);
      if (hit("Down") || hit("j"))
        return dispatch(ActionId::LatticeTargetNext);
      if (hit("k"))
        return dispatch(ActionId::LatticeTargetPrev);
      return dispatch(ActionId::LatticeTargetNext);
    }
    if (*command == "h / l / [ / ]") {
      if (hit("h") || hit("["))
        return dispatch(ActionId::LatticeTargetPrev);
      return dispatch(ActionId::LatticeTargetNext);
    }
    if (*command == "Home / End") {
      if (hit("Home"))
        return dispatch(ActionId::LatticeTargetFirst);
      if (hit("End"))
        return dispatch(ActionId::LatticeTargetLast);
      return dispatch(ActionId::LatticeTargetLast);
    }
    if (*command == "Enter")
      return dispatch(ActionId::ShowLatticeSummary);
    if (*command == "PgUp / PgDn")
      return scroll_detail(hit("PgUp") ? -8 : 8);
    if (*command == "r")
      return dispatch(ActionId::LatticeRefresh);
    return false;

  case ScreenMode::Logs:
    if (*command == "PgUp / PgDn") {
      if (hit("PgUp"))
        return dispatch(ActionId::LogsScrollPageUp);
      if (hit("PgDn"))
        return dispatch(ActionId::LogsScrollPageDown);
      return dispatch(ActionId::LogsScrollPageDown);
    }
    if (*command == "Home / End") {
      if (hit("Home"))
        return dispatch(ActionId::LogsScrollHome);
      if (hit("End"))
        return dispatch(ActionId::LogsScrollEnd);
      return dispatch(ActionId::LogsScrollEnd);
    }
    if (*command == "s / v / m / t / l") {
      if (hit("s"))
        return dispatch(ActionId::CycleLogsSourceFilter);
      if (hit("v"))
        return dispatch(ActionId::CycleLogsSeverityFilter);
      if (hit("m"))
        return dispatch(ActionId::CycleLogsMetadataFilter);
      if (hit("t"))
        return dispatch(ActionId::ToggleLogsTimestamp);
      if (hit("l"))
        return dispatch(ActionId::ToggleLogsFollow);
      return dispatch(ActionId::CycleLogsSeverityFilter);
    }
    if (*command == "u / e / o / p") {
      if (hit("u"))
        return dispatch(ActionId::ToggleLogsThread);
      if (hit("e"))
        return dispatch(ActionId::ToggleLogsMetadata);
      if (hit("o"))
        return dispatch(ActionId::ToggleLogsColor);
      if (hit("p"))
        return dispatch(ActionId::ToggleLogsMouseCapture);
      return dispatch(ActionId::ToggleLogsMetadata);
    }
    if (*command == "f / z")
      return dispatch(ActionId::ToggleWorkspaceZoom);
    if (*command == "clear logs")
      return dispatch(ActionId::ClearLogs);
    if (*command == "Up / Down settings") {
      if (hit("Up"))
        return dispatch(ActionId::LogsSelectSettingPrev);
      if (hit("Down"))
        return dispatch(ActionId::LogsSelectSettingNext);
      return dispatch(ActionId::LogsSelectSettingNext);
    }
    if (*command == "Left / Right settings") {
      if (hit("Left"))
        return dispatch(ActionId::LogsAdjustSettingPrev);
      if (hit("Right"))
        return dispatch(ActionId::LogsAdjustSettingNext);
      return dispatch(ActionId::LogsAdjustSettingNext);
    }
    return false;

  case ScreenMode::Config:
    if (*command == "Up / Down / k / j") {
      if (hit("Up"))
        return dispatch(ActionId::ConfigFilePrev);
      if (hit("Down") || hit("j"))
        return dispatch(ActionId::ConfigFileNext);
      if (hit("k"))
        return dispatch(ActionId::ConfigFilePrev);
      return dispatch(ActionId::ConfigFileNext);
    }
    if (*command == "h / l / [ / ]") {
      if (hit("h") || hit("["))
        return dispatch(ActionId::ConfigFilePrev);
      return dispatch(ActionId::ConfigFileNext);
    }
    if (*command == "Home / End") {
      if (hit("Home"))
        return dispatch(ActionId::ConfigFileFirst);
      if (hit("End"))
        return dispatch(ActionId::ConfigFileLast);
      return dispatch(ActionId::ConfigFileLast);
    }
    if (*command == "Enter" || *command == "Enter / e")
      return dispatch(ActionId::ConfigFileShow);
    if (*command == "PgUp / PgDn")
      return scroll_detail(hit("PgUp") ? -8 : 8);
    if (*command == "files / show config") {
      if (hit("show config"))
        return dispatch(ActionId::ConfigShow);
      return dispatch(ActionId::ConfigFiles);
    }
    if (*command == "show file")
      return dispatch(ActionId::ConfigFileShow);
    if (*command == "r")
      return dispatch(ActionId::ConfigReload);
    return false;
  }

  return false;
}

inline bool dispatch_menu_command_alias_row(const ActionRegistry &actions,
                                            CmdState &state, const CmdUi &ui,
                                            int x, int y, int row) {
  const auto command = menu_command_alias_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "logs metadata | thread") {
    if (hit("thread"))
      return dispatch(ActionId::ToggleLogsThread);
    return dispatch(ActionId::ToggleLogsMetadata);
  }
  if (*command == "logs color | mouse") {
    if (hit("mouse"))
      return dispatch(ActionId::ToggleLogsMouseCapture);
    return dispatch(ActionId::ToggleLogsColor);
  }
  if (*command == "help down | help page down") {
    if (hit("page down"))
      return dispatch(ActionId::HelpScrollPageDown);
    return dispatch(ActionId::HelpScrollDown);
  }
  if (*command == "menu down | menu page down") {
    if (hit("page down"))
      return dispatch(ActionId::HelpScrollPageDown);
    return dispatch(ActionId::HelpScrollDown);
  }
  if (*command == "logs down | logs page down") {
    if (hit("page down"))
      return dispatch(ActionId::LogsScrollPageDown);
    return dispatch(ActionId::LogsScrollDown);
  }
  if (*command == "runtime down / up") {
    if (hit("up"))
      return dispatch(ActionId::RuntimeItemPrev);
    return dispatch(ActionId::RuntimeItemNext);
  }
  if (*command == "runtime right / left") {
    if (hit("left"))
      return dispatch(ActionId::RuntimeFocusPrev);
    return dispatch(ActionId::RuntimeFocusNext);
  }
  if (*command == "lattice down / up") {
    if (hit("up"))
      return dispatch(ActionId::LatticeTargetPrev);
    return dispatch(ActionId::LatticeTargetNext);
  }
  if (*command == "lattice target next / prev") {
    if (hit("prev"))
      return dispatch(ActionId::LatticeTargetPrev);
    return dispatch(ActionId::LatticeTargetNext);
  }
  if (*command == "lattice target n1 / id")
    return dispatch_menu_alias_command(actions, state, "lattice target n1");
  if (*command == "lattice index(n=1)")
    return dispatch_menu_alias_command(actions, state,
                                       "iinuji.lattice.target.index(n=1)");
  if (*command == "config down / up") {
    if (hit("up"))
      return dispatch(ActionId::ConfigFilePrev);
    return dispatch(ActionId::ConfigFileNext);
  }
  if (*command == "config file next / prev") {
    if (hit("prev"))
      return dispatch(ActionId::ConfigFilePrev);
    return dispatch(ActionId::ConfigFileNext);
  }
  if (*command == "config file n1 / id")
    return dispatch_menu_alias_command(actions, state, "config file n1");
  if (*command == "config index(n=1)")
    return dispatch_menu_alias_command(actions, state,
                                       "iinuji.config.file.index(n=1)");
  if (*command == "files | show file") {
    if (hit("show file"))
      return dispatch(ActionId::ConfigFileShow);
    return dispatch(ActionId::ConfigFiles);
  }
  if (*command == "zoom | full | split") {
    if (hit("split"))
      return dispatch(ActionId::RestoreWorkspaceSplit);
    return dispatch(ActionId::ToggleWorkspaceZoom);
  }
  if (*command == "quit | exit | q | x") {
    if (hit("exit") || menu_command_row_last_label_hit(ui, x, y, *command, "x"))
      return dispatch(ActionId::Exit);
    return dispatch(ActionId::Quit);
  }
  if (*command == "actions run") {
    state.help_view = false;
    return actions.dispatch("actions run", state);
  }

  const auto action = menu_command_alias_action_for_text(*command);
  if (!action.has_value())
    return false;
  return dispatch(*action);
}

inline bool dispatch_menu_canonical_command_row(const ActionRegistry &actions,
                                                CmdState &state,
                                                const CmdUi &ui, int x, int y,
                                                int row) {
  const auto command = menu_canonical_command_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "iinuji.screen.*()")
    return dispatch(screen_action_id(state.screen));
  if (*command == "iinuji.show.*()")
    return dispatch(ActionId::ShowCurrent);
  if (*command == "iinuji.actions.*()" || *command == "iinuji.commands.*()")
    return dispatch(ActionId::ActionMenu);
  if (*command == "iinuji.workspace.*()")
    return dispatch(ActionId::ToggleWorkspaceZoom);
  if (*command == "iinuji.runtime.row.*()")
    return dispatch(ActionId::RuntimeFocusNext);
  if (*command == "iinuji.runtime.item.*()")
    return dispatch(ActionId::RuntimeItemNext);
  if (*command == "iinuji.logs.settings.*()")
    return dispatch(ActionId::LogsSelectSettingNext);
  if (*command == "iinuji.logs.settings.source.*()")
    return dispatch(ActionId::CycleLogsSourceFilter);
  if (*command == "iinuji.logs.settings.level.*()")
    return dispatch(ActionId::CycleLogsSeverityFilter);
  if (*command == "iinuji.logs.settings.metadata.*()")
    return dispatch(ActionId::CycleLogsMetadataFilter);
  if (*command == "iinuji.lattice.target.*()")
    return dispatch(ActionId::LatticeTargetNext);
  if (*command == "iinuji.config.file.*()")
    return dispatch(ActionId::ConfigFileNext);
  if (menu_canonical_pattern_row(*command))
    return false;

  if (*command == "iinuji.quit() / iinuji.exit()") {
    state.help_view = false;
    state.action_menu_open = false;
    return actions.dispatch(
        hit("iinuji.exit()") ? "iinuji.exit()" : "iinuji.quit()", state);
  }

  return dispatch_menu_alias_command(actions, state, *command);
}

inline std::optional<std::string>
menu_command_line_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "COMMAND LINE") + 1;
  const int end = menu_section_header_row(state, "CURRENT SCREEN");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline std::optional<std::string>
menu_shell_snapshot_command_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "SHELL SNAPSHOTS") + 1;
  const int end = menu_section_header_row(state, "IDENTITY");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline std::optional<std::string>
menu_shell_log_utility_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "SHELL LOG UTILITIES") + 1;
  const int end = menu_section_header_row(state, "SHELL SNAPSHOTS");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline std::optional<std::string>
menu_help_scroll_utility_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "MENU SCROLL UTILITIES") + 1;
  const int end = menu_section_header_row(state, "ACTION PALETTE UTILITIES");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline std::optional<std::string>
menu_action_palette_utility_text_at(const CmdState &state, int row) {
  const int first =
      menu_section_header_row(state, "ACTION PALETTE UTILITIES") + 1;
  const int end = menu_section_header_row(state, "RUNTIME UTILITIES");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline std::optional<std::string>
menu_runtime_utility_text_at(const CmdState &state, int row) {
  const int first = menu_section_header_row(state, "RUNTIME UTILITIES") + 1;
  const int end = menu_section_header_row(state, "SHELL LOG UTILITIES");
  if (row < first || row >= end)
    return std::nullopt;
  const MenuOverlayColumns columns = make_menu_overlay_columns(state);
  if (row >= static_cast<int>(columns.commands.size()))
    return std::nullopt;
  return trim(columns.commands[static_cast<std::size_t>(row)].text);
}

inline bool dispatch_menu_command_line_row(const ActionRegistry &actions,
                                           CmdState &state, int row) {
  const auto command = menu_command_line_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;
  if (*command == ":" || *command == "Enter" || *command == "Ctrl-U" ||
      *command == "Backspace" || *command == "Ctrl-U / Backspace" ||
      *command == "Up / Down")
    return false;
  if (*command == "cuwacunu.cmd --actions / --palette")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu.cmd --actions");
  if (*command == "cuwacunu_cmd --actions / --palette")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu_cmd --actions");
  if (*command == "cuwacunu.cmd --commands / --catalog")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu.cmd --catalog");
  if (*command == "cuwacunu_cmd --commands / --catalog")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu_cmd --catalog");
  if (*command == "cuwacunu_cmd --help / --version")
    return dispatch_menu_alias_command(actions, state, "cuwacunu_cmd --help");
  if (*command == "cuwacunu.cmd --help / --version")
    return dispatch_menu_alias_command(actions, state, "cuwacunu.cmd --help");
  if (*command == "cuwacunu_cmd --run/--command \"show\"")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu_cmd --run \"show\"");
  if (*command == "cuwacunu.cmd --run/--command \"show\"")
    return dispatch_menu_alias_command(actions, state,
                                       "cuwacunu.cmd --run \"show\"");
  return dispatch_menu_alias_command(actions, state, *command);
}

inline bool
dispatch_menu_action_palette_utility_row(const ActionRegistry &actions,
                                         CmdState &state, const CmdUi &ui,
                                         int x, int y, int row) {
  const auto command = menu_action_palette_utility_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "actions up | down") {
    if (hit("down"))
      return dispatch(ActionId::ActionMenuSelectNext);
    return dispatch(ActionId::ActionMenuSelectPrev);
  }
  if (*command == "actions page up | page down") {
    if (hit("page down"))
      return dispatch(ActionId::ActionMenuSelectPageDown);
    return dispatch(ActionId::ActionMenuSelectPageUp);
  }
  if (*command == "actions home | end") {
    if (hit("end"))
      return dispatch(ActionId::ActionMenuSelectEnd);
    return dispatch(ActionId::ActionMenuSelectHome);
  }
  if (*command == "actions close | run") {
    if (hit("run")) {
      state.help_view = false;
      state.action_menu_open = false;
      return actions.dispatch("actions run", state);
    }
    return dispatch(ActionId::ActionMenuClose);
  }
  return false;
}

inline bool dispatch_menu_runtime_utility_row(const ActionRegistry &actions,
                                              CmdState &state, const CmdUi &ui,
                                              int x, int y, int row) {
  const auto command = menu_runtime_utility_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "runtime devices | jobs") {
    if (hit("jobs"))
      return dispatch(ActionId::RuntimeFocusJobs);
    return dispatch(ActionId::RuntimeFocusDevices);
  }
  if (*command == "runtime manifest | log | trace") {
    if (hit("trace"))
      return dispatch(ActionId::RuntimeDetailTrace);
    if (hit("log"))
      return dispatch(ActionId::RuntimeDetailLog);
    return dispatch(ActionId::RuntimeDetailManifest);
  }
  return false;
}

inline bool dispatch_menu_shell_snapshot_row(const ActionRegistry &actions,
                                             CmdState &state, int row) {
  const auto command = menu_shell_snapshot_command_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;
  return dispatch_menu_alias_command(actions, state, *command);
}

inline bool dispatch_menu_shell_log_utility_row(const ActionRegistry &actions,
                                                CmdState &state,
                                                const CmdUi &ui, int x, int y,
                                                int row) {
  const auto command = menu_shell_log_utility_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "logs source all | refresh") {
    if (hit("refresh"))
      return dispatch(ActionId::SetLogsSourceRefresh);
    return dispatch(ActionId::SetLogsSourceAll);
  }
  if (*command == "logs setting prev | next") {
    if (hit("next"))
      return dispatch(ActionId::LogsSelectSettingNext);
    return dispatch(ActionId::LogsSelectSettingPrev);
  }
  if (*command == "logs setting left | right") {
    if (hit("right"))
      return dispatch(ActionId::LogsAdjustSettingNext);
    return dispatch(ActionId::LogsAdjustSettingPrev);
  }
  if (*command == "logs up | down") {
    if (hit("down"))
      return dispatch(ActionId::LogsScrollDown);
    return dispatch(ActionId::LogsScrollUp);
  }
  if (*command == "logs page up | page down") {
    if (hit("page down"))
      return dispatch(ActionId::LogsScrollPageDown);
    return dispatch(ActionId::LogsScrollPageUp);
  }
  if (*command == "logs home | end") {
    if (hit("end"))
      return dispatch(ActionId::LogsScrollEnd);
    return dispatch(ActionId::LogsScrollHome);
  }
  if (*command == "logs source action | command") {
    if (hit("command"))
      return dispatch(ActionId::SetLogsSourceCommand);
    return dispatch(ActionId::SetLogsSourceAction);
  }
  if (*command == "logs source show | status") {
    if (hit("status"))
      return dispatch(ActionId::SetLogsSourceStatus);
    return dispatch(ActionId::SetLogsSourceShow);
  }
  if (*command == "logs debug | info | warning") {
    if (hit("warning"))
      return dispatch(ActionId::SetLogsSeverityWarning);
    if (hit("info"))
      return dispatch(ActionId::SetLogsSeverityInfo);
    return dispatch(ActionId::SetLogsSeverityDebug);
  }
  if (*command == "logs error | fatal") {
    if (hit("fatal"))
      return dispatch(ActionId::SetLogsSeverityFatal);
    return dispatch(ActionId::SetLogsSeverityError);
  }
  if (*command == "logs meta any | any_meta") {
    if (hit("any_meta"))
      return dispatch(ActionId::SetLogsMetadataAnyMeta);
    return dispatch(ActionId::SetLogsMetadataAny);
  }
  if (*command == "logs meta function | path") {
    if (hit("path"))
      return dispatch(ActionId::SetLogsMetadataPath);
    return dispatch(ActionId::SetLogsMetadataFunction);
  }
  if (*command == "logs meta callsite") {
    return dispatch(ActionId::SetLogsMetadataCallsite);
  }
  return false;
}

inline bool dispatch_menu_help_scroll_utility_row(const ActionRegistry &actions,
                                                  CmdState &state,
                                                  const CmdUi &ui, int x, int y,
                                                  int row) {
  const auto command = menu_help_scroll_utility_text_at(state, row);
  if (!command.has_value() || command->empty())
    return false;

  const auto hit = [&](std::string_view label) {
    return menu_command_row_label_hit(ui, x, y, *command, label);
  };
  const auto dispatch = [&](ActionId id) {
    return dispatch_menu_alias_action(actions, state, id);
  };

  if (*command == "help/menu up | down") {
    if (hit("down"))
      return dispatch(ActionId::HelpScrollDown);
    return dispatch(ActionId::HelpScrollUp);
  }
  if (*command == "help/menu page up | page down") {
    if (hit("page down"))
      return dispatch(ActionId::HelpScrollPageDown);
    return dispatch(ActionId::HelpScrollPageUp);
  }
  if (*command == "help/menu left | right") {
    if (hit("right"))
      return dispatch(ActionId::HelpScrollRight);
    return dispatch(ActionId::HelpScrollLeft);
  }
  if (*command == "help/menu home | end") {
    if (hit("end"))
      return dispatch(ActionId::HelpScrollEnd);
    return dispatch(ActionId::HelpScrollHome);
  }
  return false;
}

inline bool dispatch_menu_overlay_click(const ActionRegistry &actions,
                                        CmdState &state, const CmdUi &ui, int x,
                                        int y) {
  int row = 0;
  if (!menu_overlay_row_at_point(state, ui, x, y, row))
    return false;

  if (row == kMenuCloseRow)
    return actions.dispatch(ActionId::CloseHelp, state);
  if (row == kMenuActionsRow)
    return actions.dispatch(ActionId::ActionMenu, state);

  const int screen_index = row - kMenuScreenFirstRow;
  const auto &screens = screen_order();
  if (screen_index >= 0 && screen_index < static_cast<int>(screens.size())) {
    return actions.dispatch(
        screen_action_id(screens[static_cast<std::size_t>(screen_index)]),
        state);
  }

  if (menu_command_alias_text_at(state, row).has_value()) {
    return dispatch_menu_command_alias_row(actions, state, ui, x, y, row);
  }

  if (dispatch_menu_canonical_command_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_command_line_row(actions, state, row))
    return true;

  if (dispatch_menu_current_screen_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_help_scroll_utility_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_action_palette_utility_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_runtime_utility_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_shell_log_utility_row(actions, state, ui, x, y, row))
    return true;

  if (dispatch_menu_shell_snapshot_row(actions, state, row))
    return true;

  return false;
}

inline bool
text_content_row_at_point(const std::shared_ptr<iinuji_object_t> &box, int x,
                          int y, int &row) {
  if (!object_contains_point(box, x, y))
    return false;
  const auto data = text_data(box);
  if (!data)
    return false;
  row = y - box->screen.y + data->scroll_y;
  return true;
}

inline int text_row_after_heading(const std::shared_ptr<iinuji_object_t> &box,
                                  std::string_view heading, int fallback) {
  const auto data = text_data(box);
  if (!data)
    return fallback;
  const auto lines = data->styled_lines.empty()
                         ? split_lines_keep_empty(data->content)
                         : std::vector<std::string>{};
  const auto line_count =
      data->styled_lines.empty() ? lines.size() : data->styled_lines.size();
  for (std::size_t i = 0; i < line_count; ++i) {
    const std::string line =
        data->styled_lines.empty() ? lines[i] : data->styled_lines[i].text;
    if (trim_for_style(line) == heading)
      return static_cast<int>(i + 1u);
  }
  return fallback;
}

inline int
runtime_device_header_row(const std::shared_ptr<iinuji_object_t> &box,
                          int fallback) {
  const auto data = text_data(box);
  if (!data)
    return fallback;
  const auto lines = data->styled_lines.empty()
                         ? split_lines_keep_empty(data->content)
                         : std::vector<std::string>{};
  const std::size_t line_count =
      data->styled_lines.empty() ? lines.size() : data->styled_lines.size();
  for (std::size_t i = 0; i < line_count; ++i) {
    const std::string line =
        data->styled_lines.empty() ? lines[i] : data->styled_lines[i].text;
    const std::string trimmed = trim_for_style(line);
    if (style_starts_with(trimmed, "> Devices") ||
        style_starts_with(trimmed, "Devices"))
      return static_cast<int>(i);
  }
  return fallback;
}

inline bool dispatch_content_rail_click(const ActionRegistry &actions,
                                        CmdState &state, CmdUi &ui, int x,
                                        int y) {
  (void)actions;
  (void)state;
  (void)ui;
  (void)x;
  (void)y;
  return false;
}

inline bool
dispatch_runtime_row_click(CmdState &state, int row,
                           int device_header_row = kRuntimeDeviceHeaderRow) {
  if (state.screen != ScreenMode::Runtime)
    return false;

  int cursor = device_header_row;
  if (row == cursor) {
    set_runtime_focus(state, RuntimeFocus::Devices);
    append_log(state, "action", state.status);
    return true;
  }

  ++cursor;
  if (row == cursor) {
    set_runtime_focus(state, RuntimeFocus::Devices);
    state.runtime.selected_device_row = 0;
    state.status = "runtime device host";
    append_log(state, "action", state.status);
    return true;
  }

  if (state.runtime.device.gpus.empty()) {
    ++cursor;
    if (row == cursor) {
      set_runtime_focus(state, RuntimeFocus::Devices);
      state.status = "runtime devices";
      append_log(state, "action", state.status);
      return true;
    }
  } else {
    for (std::size_t i = 0; i < state.runtime.device.gpus.size(); ++i) {
      ++cursor;
      if (row == cursor) {
        set_runtime_focus(state, RuntimeFocus::Devices);
        state.runtime.selected_device_row = i + 1u;
        state.status = "runtime gpu " + std::to_string(i + 1u) + "/" +
                       std::to_string(state.runtime.device.gpus.size());
        append_log(state, "action", state.status);
        return true;
      }
    }
  }

  const int jobs_header_row = cursor + 2;
  if (row == jobs_header_row) {
    set_runtime_focus(state, RuntimeFocus::Jobs);
    append_log(state, "action", state.status);
    return true;
  }

  if (state.runtime.jobs.empty()) {
    if (row == jobs_header_row + 1) {
      set_runtime_focus(state, RuntimeFocus::Jobs);
      append_log(state, "action", state.status);
      return true;
    }
    return false;
  }

  int job_row = jobs_header_row + 1;
  for (std::size_t i = 0; i < state.runtime.jobs.size(); ++i) {
    if (row == job_row) {
      set_runtime_focus(state, RuntimeFocus::Jobs);
      state.runtime.selected_job = i;
      state.status = "runtime job " + std::to_string(i + 1u) + "/" +
                     std::to_string(state.runtime.jobs.size());
      append_log(state, "action", state.status);
      return true;
    }
    ++job_row;
  }

  return false;
}

inline int logs_lens_first_setting_row(const CmdState &state) {
  const auto lines =
      split_styled_content_lines(make_logs_control_deck_text(state));
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (lines[i] != "Lens")
      continue;
    std::size_t row = i + 1u;
    if (row < lines.size() && lines[row] == detail_section_rule())
      ++row;
    return static_cast<int>(row);
  }
  return 0;
}

inline bool dispatch_logs_lens_click(const ActionRegistry &actions,
                                     CmdState &state, const CmdUi &ui, int x,
                                     int y) {
  if (state.screen != ScreenMode::Logs || state.help_view ||
      state.action_menu_open || cmd_choice_menu_open(state) ||
      cmd_content_popup_open(state)) {
    return false;
  }

  int row = 0;
  if (!text_content_row_at_point(ui.detail, x, y, row))
    return false;

  const int first = logs_lens_first_setting_row(state);
  const int setting = row - first;
  if (setting < 0 || setting >= static_cast<int>(logs_settings_count()))
    return false;

  state.logs.selected_setting = static_cast<std::size_t>(setting);
  return dispatch_selected_logs_setting_adjustment(actions, state);
}

inline std::string runtime_selector_label(const CmdState &state,
                                          RuntimeFocus focus) {
  std::string label = runtime_focus_label(focus);
  if (state.runtime.focus == focus)
    label = "[" + label + "]";
  return label;
}

inline std::string runtime_selector_label(const CmdState &state,
                                          RuntimeDetailMode mode) {
  std::string label = runtime_detail_mode_label(mode);
  if (state.runtime.detail_mode == mode)
    label = "[" + label + "]";
  return label;
}

inline bool dispatch_runtime_selector_click(const ActionRegistry &actions,
                                            CmdState &state, const CmdUi &ui,
                                            int x, int y) {
  if (state.screen != ScreenMode::Runtime || state.help_view ||
      state.action_menu_open || cmd_choice_menu_open(state) ||
      cmd_content_popup_open(state)) {
    return false;
  }

  int row = 0;
  const auto nav_text = active_left_nav_text(state, ui);
  if (!text_content_row_at_point(nav_text, x, y, row))
    return false;

  if (row == kRuntimeFocusSelectorRow) {
    int cursor = nav_text->screen.x + 7;
    const auto hit_focus = [&](RuntimeFocus focus, ActionId id) {
      const std::string label = runtime_selector_label(state, focus);
      const int end = cursor + static_cast<int>(label.size());
      const bool hit = x >= cursor && x < end;
      cursor = end + 1;
      return hit ? actions.dispatch(id, state) : false;
    };
    if (hit_focus(RuntimeFocus::Devices, ActionId::RuntimeFocusDevices))
      return true;
    if (hit_focus(RuntimeFocus::Jobs, ActionId::RuntimeFocusJobs))
      return true;
    return false;
  }

  if (row == kRuntimeDetailSelectorRow) {
    int cursor = nav_text->screen.x + 8;
    const auto hit_detail = [&](RuntimeDetailMode mode, ActionId id) {
      const std::string label = runtime_selector_label(state, mode);
      const int end = cursor + static_cast<int>(label.size());
      const bool hit = x >= cursor && x < end;
      cursor = end + 1;
      return hit ? actions.dispatch(id, state) : false;
    };
    if (hit_detail(RuntimeDetailMode::Manifest,
                   ActionId::RuntimeDetailManifest))
      return true;
    if (hit_detail(RuntimeDetailMode::Log, ActionId::RuntimeDetailLog))
      return true;
    if (hit_detail(RuntimeDetailMode::Trace, ActionId::RuntimeDetailTrace))
      return true;
    return false;
  }

  return false;
}

inline bool dispatch_browser_row_click(CmdState &state, const CmdUi &ui, int x,
                                       int y) {
  if (state.help_view || state.action_menu_open ||
      cmd_choice_menu_open(state) || cmd_content_popup_open(state))
    return false;

  int row = 0;
  const auto worklist_text = active_left_worklist_text(state, ui);
  if (!text_content_row_at_point(worklist_text, x, y, row))
    return false;

  const bool split = active_split_left_visible(state, ui);
  const int device_header_row = runtime_device_header_row(
      worklist_text, split ? kSplitWorklistFirstRow : kRuntimeDeviceHeaderRow);
  if (dispatch_runtime_row_click(state, row, device_header_row))
    return true;

  if (state.screen == ScreenMode::Config) {
    const int first_row = text_row_after_heading(
        worklist_text, "Worklist",
        split ? kSplitWorklistFirstRow : kConfigBrowserFirstRow);
    if (row < first_row)
      return false;
    const std::size_t index = static_cast<std::size_t>(row - first_row);
    if (index >= state.config.files.size())
      return false;
    state.config.selected_file = index;
    state.config.file_editor_open = false;
    state.status = selected_config_file_status(state);
    append_log(state, "action", state.status);
    return true;
  }

  if (state.screen == ScreenMode::Lattice) {
    const int first_row = text_row_after_heading(
        worklist_text, "Worklist",
        split ? kSplitWorklistFirstRow : kLatticeBrowserFirstRow);
    if (row < first_row)
      return false;
    const std::size_t index = static_cast<std::size_t>(row - first_row);
    if (index >= state.lattice.target_ids.size())
      return false;
    state.lattice.selected_target = index;
    state.status = "lattice target " + std::to_string(index + 1u) + "/" +
                   std::to_string(state.lattice.target_ids.size());
    append_log(state, "action", state.status);
    return true;
  }

  return false;
}

inline bool scroll_hovered_workspace_panel(CmdState &state, CmdUi &ui, int dy,
                                           int dx) {
  if (dy == 0 && dx == 0)
    return false;
  const bool target_detail =
      object_contains_point(ui.detail_panel, ui.last_mouse_x, ui.last_mouse_y);
  if (target_detail) {
    state.status = "context scroll";
    return scroll_object_by(active_detail_scroll_object(state, ui), dy, dx);
  }
  if (active_split_left_visible(state, ui)) {
    if (object_contains_point(ui.split_nav_panel, ui.last_mouse_x,
                              ui.last_mouse_y)) {
      if (scroll_object_by(ui.split_nav, dy, dx)) {
        state.status = "navigator scroll";
        return true;
      }
    }
    if (object_contains_point(ui.split_worklist_panel, ui.last_mouse_x,
                              ui.last_mouse_y)) {
      if (scroll_object_by(ui.split_worklist, dy, dx)) {
        state.status = "view scroll";
        return true;
      }
    }
  }
  const auto left_panel = active_left_panel(state, ui);
  const bool target_main =
      object_contains_point(left_panel, ui.last_mouse_x, ui.last_mouse_y);
  if (target_main || state.screen == ScreenMode::Logs) {
    const auto target_text = active_left_worklist_text(state, ui);
    if (scroll_object_by(target_text, dy, dx)) {
      persist_logs_scroll_from_main(state, ui);
      if (state.screen == ScreenMode::Logs) {
        if (dy < 0)
          state.logs.auto_follow = false;
        if (dy > 0)
          state.logs.auto_follow = true;
        state.status = dy < 0 ? "logs scroll=up" : "logs scroll=down";
      } else {
        state.status = "view scroll";
      }
      return true;
    }
  }
  return false;
}

inline std::shared_ptr<iinuji_object_t>
keyboard_scroll_target(const CmdState &state, const CmdUi &ui) {
  if (workspace_is_current_screen_zoomed(state)) {
    return workspace_current_screen_uses_left_panel_zoom(state) ? ui.main
                                                                : ui.detail;
  }

  switch (state.screen) {
  case ScreenMode::Runtime:
  case ScreenMode::Lattice:
  case ScreenMode::Config:
    if (ui.detail_panel && ui.detail_panel->visible)
      return active_detail_scroll_object(state, ui);
    return ui.main;
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Logs:
    return ui.main;
  }
  return ui.main;
}

inline bool scroll_workspace_by_key(CmdState &state, CmdUi &ui, int key) {
  const auto target = keyboard_scroll_target(state, ui);
  if (!target)
    return false;
  scroll_text(target, key);
  const bool target_is_main = target == ui.main;
  const bool target_is_detail =
      target == ui.detail || target == ui.config_editor;
  if (state.screen == ScreenMode::Logs && target_is_main) {
    persist_logs_scroll_from_main(state, ui);
    if (standard_key_action(key) == key_action_t::PageUp ||
        standard_key_action(key) == key_action_t::Home)
      state.logs.auto_follow = false;
    if (standard_key_action(key) == key_action_t::End)
      state.logs.auto_follow = true;
    state.status = logs_scroll_status_for_key(key);
  } else if (target_is_detail) {
    state.status = "context scroll";
  } else {
    state.status = "view scroll";
  }
  return true;
}

inline bool handle_mouse_key(const ActionRegistry &actions, CmdState &state,
                             CmdUi &ui) {
  if (!state.logs.mouse_capture)
    return false;

  MEVENT event{};
  if (getmouse(&event) != OK)
    return false;

  ui.last_mouse_x = event.x;
  ui.last_mouse_y = event.y;

  if ((state.help_view || state.action_menu_open ||
       cmd_choice_menu_open(state) || cmd_content_popup_open(state)) &&
      mouse_left_click(event)) {
    const bool command_hit = command_panel_hit(ui, event.x, event.y);
    const auto overlay = cmd_content_popup_open(state)
                             ? ui.popup_overlay
                             : (cmd_choice_menu_open(state) ? ui.choice_overlay
                                                            : ui.menu_overlay);
    if (overlay_close_point(overlay, event.x, event.y) ||
        !object_contains_point(overlay, event.x, event.y)) {
      if (cmd_content_popup_open(state)) {
        close_content_popup(state);
        state.status = cmd_choice_menu_open(state) ? "runtime menu"
                                                   : screen_label(state.screen);
        return true;
      }
      state.help_view = false;
      state.action_menu_open = false;
      close_choice_menu(state);
      close_content_popup(state);
      state.status = screen_label(state.screen);
      if (command_hit)
        focus_command_input(state, ui);
      return true;
    }
  }

  if (mouse_left_click(event) && command_panel_hit(ui, event.x, event.y)) {
    focus_command_input(state, ui);
    return true;
  }

  if (mouse_left_click(event) &&
      dispatch_action_menu_click(
          actions, state, ui, event.x, event.y,
          mouse_has(event, BUTTON1_DOUBLE_CLICKED) ||
              mouse_has(event, BUTTON1_TRIPLE_CLICKED))) {
    return true;
  }

  if (mouse_left_click(event) &&
      dispatch_choice_menu_click(
          actions, state, ui, event.x, event.y,
          mouse_has(event, BUTTON1_DOUBLE_CLICKED) ||
              mouse_has(event, BUTTON1_TRIPLE_CLICKED))) {
    return true;
  }

  if (mouse_left_click(event) && cmd_choice_menu_open(state) &&
      object_contains_point(ui.choice_overlay, event.x, event.y)) {
    return true;
  }

  if (mouse_left_click(event) && cmd_content_popup_open(state) &&
      object_contains_point(ui.popup_overlay, event.x, event.y)) {
    return true;
  }

  if (mouse_left_click(event) &&
      dispatch_menu_overlay_click(actions, state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      dispatch_nav_rail_click(actions, state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      dispatch_content_rail_click(actions, state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      dispatch_browser_row_click(state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      dispatch_logs_lens_click(actions, state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      dispatch_runtime_selector_click(actions, state, ui, event.x, event.y))
    return true;

  if (mouse_left_click(event) &&
      workspace_zoom_control_hit(state, ui, event.x, event.y)) {
    actions.dispatch(ActionId::ToggleWorkspaceZoom, state);
    return true;
  }

  if (mouse_left_click(event) &&
      logs_panel_top_edge_hit(state, ui, event.x, event.y)) {
    actions.dispatch(ActionId::LogsScrollHome, state);
    return true;
  }
  if (mouse_left_click(event) &&
      logs_panel_bottom_edge_hit(state, ui, event.x, event.y)) {
    actions.dispatch(ActionId::LogsScrollEnd, state);
    return true;
  }

  if (mouse_left_click(event) &&
      browser_panel_top_edge_hit(state, ui, event.x, event.y)) {
    if (state.screen == ScreenMode::Config) {
      actions.dispatch(ActionId::ConfigFileFirst, state);
    } else if (state.screen == ScreenMode::Lattice) {
      actions.dispatch(ActionId::LatticeTargetFirst, state);
    }
    scroll_text_to(active_left_worklist_text(state, ui), 0, 0);
    return true;
  }
  if (mouse_left_click(event) &&
      browser_panel_bottom_edge_hit(state, ui, event.x, event.y)) {
    if (state.screen == ScreenMode::Config) {
      actions.dispatch(ActionId::ConfigFileLast, state);
      scroll_text_to(active_left_worklist_text(state, ui),
                     static_cast<int>(state.config.selected_file > 4
                                          ? state.config.selected_file - 4
                                          : 0),
                     0);
    } else if (state.screen == ScreenMode::Lattice) {
      actions.dispatch(ActionId::LatticeTargetLast, state);
      scroll_text_to(active_left_worklist_text(state, ui),
                     static_cast<int>(state.lattice.selected_target > 4
                                          ? state.lattice.selected_target - 4
                                          : 0),
                     0);
    }
    return true;
  }

  const int wheel =
      mouse_wheel_up(event) ? -1 : (mouse_wheel_down(event) ? 1 : 0);
  if (wheel == 0)
    return mouse_left_click(event);

  const bool horizontal = mouse_horizontal_modifier(event);
  const int dy = horizontal ? 0 : wheel * 3;
  const int dx = horizontal ? wheel * 8 : 0;

  if (state.help_view)
    return scroll_help_overlay_by(state, dy, dx);
  if (state.action_menu_open)
    return scroll_action_menu_by(actions, state, dy);
  if (cmd_choice_menu_open(state))
    return scroll_choice_menu_by(state, dy);
  if (cmd_content_popup_open(state))
    return scroll_content_popup_by(state, dy, dx);
  return scroll_hovered_workspace_panel(state, ui, dy, dx);
}

inline bool handle_config_editor_key(CmdState &state, CmdUi &ui, int key) {
  if (!config_editor_should_be_visible(state, ui))
    return false;

  const key_action_t action = standard_key_action(key);
  if (action == key_action_t::Cancel || action == key_action_t::Close) {
    state.config.file_editor_open = false;
    state.status = screen_label(ScreenMode::Config);
    return true;
  }

  primitives::editor_keymap_options_t opts{};
  opts.allow_save = false;
  opts.allow_reload = false;
  opts.allow_history = false;
  opts.allow_newline = false;
  opts.allow_tab_indent = false;
  opts.allow_printable_insert = false;
  opts.escape_requests_close = true;
  const auto result =
      primitives::handle_editor_key(ui.config_editor, key, opts);
  if (result.close_requested) {
    state.config.file_editor_open = false;
    state.status = screen_label(ScreenMode::Config);
    return true;
  }
  if (result.handled) {
    state.status = result.viewport_changed ? "cursor" : result.status;
    if (state.status.empty())
      state.status = "read only";
    return true;
  }
  state.status = "read only";
  return true;
}

inline void handle_key(int key, const ActionRegistry &actions, CmdState &state,
                       CmdUi &ui) {
  if (key == ERR)
    return;
  if (key == KEY_MOUSE && handle_mouse_key(actions, state, ui))
    return;
  if (dispatch_function_key(key, actions, state))
    return;

  const std::string input = get_text(ui.input);
  const bool command_entry = command_input_is_active(state, ui);
  const key_action_t key_action = standard_key_action(key);

  if (standard_key_action(key) == key_action_t::Close) {
    actions.dispatch(ActionId::Quit, state);
    return;
  }

  if (!command_entry && overlay_close_key(key) &&
      (state.help_view || state.action_menu_open ||
       cmd_choice_menu_open(state) || cmd_content_popup_open(state))) {
    if (cmd_content_popup_open(state)) {
      close_content_popup(state);
      state.status = cmd_choice_menu_open(state) ? "runtime menu"
                                                 : screen_label(state.screen);
      return;
    }
    state.help_view = false;
    state.action_menu_open = false;
    state.action_menu_scroll_y = 0;
    close_choice_menu(state);
    close_content_popup(state);
    state.status = screen_label(state.screen);
    return;
  }

  if (cmd_content_popup_open(state)) {
    handle_content_popup_key(key, state);
    return;
  }

  if (state.action_menu_open) {
    handle_action_menu_key(key, actions, state);
    return;
  }

  if (cmd_choice_menu_open(state)) {
    handle_choice_menu_key(key, actions, state);
    return;
  }

  if (!command_entry && key == '\t') {
    focus_command_input(state, ui);
    return;
  }

  if (!command_entry && key == ':') {
    enter_command_mode(state);
    sync_command_input_focus(state, ui);
    return;
  }

  if (command_entry) {
    if (key_action == key_action_t::Cancel) {
      set_text(ui.input, "");
      leave_command_mode(state);
      sync_command_input_focus(state, ui);
      state.status = screen_label(state.screen);
      return;
    }
    if ((key_action == key_action_t::MoveUp ||
         key_action == key_action_t::MoveDown) &&
        (!state.command_history.empty() || state.command_history_cursor >= 0)) {
      recall_command_history(state, ui,
                             key_action == key_action_t::MoveUp ? -1 : 1);
      return;
    }
    if (key == 21) {
      set_text(ui.input, "");
      reset_command_history_cursor(state);
      state.status = "command cleared";
      return;
    }
    if (is_enter_key(key)) {
      if (!input.empty()) {
        remember_command(state, input);
        if (state.help_view)
          state.help_view = false;
        actions.dispatch(input, state);
      } else {
        state.status = screen_label(state.screen);
      }
      set_text(ui.input, "");
      leave_command_mode(state);
      sync_command_input_focus(state, ui);
      return;
    }

    const auto result = handle_text_box_key(ui.input, key);
    if (result.content_changed) {
      enter_command_mode(state);
      sync_command_input_focus(state, ui);
      reset_command_history_cursor(state);
      state.status = "editing";
    }
    return;
  }

  if (state.help_view && standard_key_action(key) == key_action_t::Cancel &&
      input.empty()) {
    state.help_view = false;
    state.status = screen_label(state.screen);
    return;
  }
  if (!state.help_view && input.empty() &&
      standard_key_action(key) == key_action_t::Cancel &&
      workspace_is_current_screen_zoomed(state)) {
    actions.dispatch(ActionId::RestoreWorkspaceSplit, state);
    return;
  }
  if (input.empty() && scroll_help_overlay(state, key))
    return;
  const bool h_key_is_screen_navigation = state.screen == ScreenMode::Runtime ||
                                          state.screen == ScreenMode::Lattice ||
                                          state.screen == ScreenMode::Config;
  if ((key == '?' || key == 'H' ||
       (key == 'h' && !h_key_is_screen_navigation)) &&
      input.empty()) {
    actions.dispatch(ActionId::Help, state);
    return;
  }
  if ((key == 'a' || key == 'A') && input.empty()) {
    actions.dispatch(ActionId::ActionMenu, state);
    return;
  }
  if ((key == 'q' || key == 'Q') && input.empty()) {
    actions.dispatch(ActionId::Quit, state);
    return;
  }
  if ((key == 'x' || key == 'X') && input.empty()) {
    actions.dispatch(ActionId::Exit, state);
    return;
  }
  if ((standard_key_action(key) == key_action_t::Reload || key == 'r' ||
       key == 'R') &&
      input.empty()) {
    actions.dispatch(refresh_action_id(state.screen), state);
    return;
  }
  if (input.empty() && (key == 'z' || key == 'Z' ||
                        ((key == 'f' || key == 'F') &&
                         workspace_screen_supports_zoom(state.screen)))) {
    actions.dispatch(ActionId::ToggleWorkspaceZoom, state);
    return;
  }
  if (key == 21) {
    set_text(ui.input, "");
    reset_command_history_cursor(state);
    state.status = "command cleared";
    return;
  }

  if (state.screen == ScreenMode::Logs && input.empty()) {
    const key_action_t action = standard_key_action(key);
    if (action == key_action_t::MoveUp) {
      actions.dispatch(ActionId::LogsSelectSettingPrev, state);
      return;
    }
    if (action == key_action_t::MoveDown) {
      actions.dispatch(ActionId::LogsSelectSettingNext, state);
      return;
    }
    if (action == key_action_t::MoveLeft || action == key_action_t::MoveRight) {
      dispatch_selected_logs_setting_adjustment(
          actions, state, action == key_action_t::MoveLeft ? -1 : 1);
      return;
    }
    if (key == 's' || key == 'S') {
      actions.dispatch(ActionId::CycleLogsSourceFilter, state);
      return;
    }
    if (key == 'v' || key == 'V') {
      actions.dispatch(ActionId::CycleLogsSeverityFilter, state);
      return;
    }
    if (key == 'm' || key == 'M') {
      actions.dispatch(ActionId::CycleLogsMetadataFilter, state);
      return;
    }
    if (key == 't' || key == 'T') {
      actions.dispatch(ActionId::ToggleLogsTimestamp, state);
      return;
    }
    if (key == 'u' || key == 'U') {
      actions.dispatch(ActionId::ToggleLogsThread, state);
      return;
    }
    if (key == 'e' || key == 'E') {
      actions.dispatch(ActionId::ToggleLogsMetadata, state);
      return;
    }
    if (key == 'o' || key == 'O') {
      actions.dispatch(ActionId::ToggleLogsColor, state);
      return;
    }
    if (key == 'l' || key == 'L') {
      actions.dispatch(ActionId::ToggleLogsFollow, state);
      return;
    }
    if (key == 'p' || key == 'P') {
      actions.dispatch(ActionId::ToggleLogsMouseCapture, state);
      return;
    }
    if (key == 'c' || key == 'C') {
      actions.dispatch(ActionId::ClearLogs, state);
      return;
    }
  }

  if (state.screen == ScreenMode::Runtime && input.empty()) {
    const key_action_t action = standard_key_action(key);
    if (action == key_action_t::MoveLeft) {
      actions.dispatch(ActionId::RuntimeFocusPrev, state);
      return;
    }
    if (action == key_action_t::MoveRight) {
      actions.dispatch(ActionId::RuntimeFocusNext, state);
      return;
    }
    if (action == key_action_t::Tab) {
      actions.dispatch(ActionId::RuntimeDetailNext, state);
      return;
    }
    if (key == 'd' || key == 'D') {
      actions.dispatch(ActionId::RuntimeFocusDevices, state);
      return;
    }
    if (key == 'J') {
      actions.dispatch(ActionId::RuntimeFocusJobs, state);
      return;
    }
    if (is_enter_key(key)) {
      open_runtime_selected_menu(state);
      return;
    }
    if (action == key_action_t::MoveUp || key == 'k' || key == 'K' ||
        key == 'h' || key == '[') {
      actions.dispatch(ActionId::RuntimeItemPrev, state);
      return;
    }
    if (action == key_action_t::MoveDown || key == 'j' || key == 'l' ||
        key == 'L' || key == ']') {
      actions.dispatch(ActionId::RuntimeItemNext, state);
      return;
    }
  }

  if (state.screen == ScreenMode::Config && input.empty()) {
    if (handle_config_editor_key(state, ui, key))
      return;
    const key_action_t action = standard_key_action(key);
    const auto worklist_text = active_left_worklist_text(state, ui);
    if (is_enter_key(key) || key == 'e' || key == 'E') {
      actions.dispatch(ActionId::ConfigFileShow, state);
      return;
    }
    if (action == key_action_t::MoveUp || key == 'k' || key == 'K' ||
        key == 'h' || key == '[') {
      actions.dispatch(ActionId::ConfigFilePrev, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.config.selected_file > 4
                                          ? state.config.selected_file - 4
                                          : 0),
                     0);
      return;
    }
    if (action == key_action_t::MoveDown || key == 'j' || key == 'J' ||
        key == 'l' || key == 'L' || key == ']') {
      actions.dispatch(ActionId::ConfigFileNext, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.config.selected_file > 4
                                          ? state.config.selected_file - 4
                                          : 0),
                     0);
      return;
    }
    if (action == key_action_t::Home) {
      actions.dispatch(ActionId::ConfigFileFirst, state);
      scroll_text_to(worklist_text, 0, 0);
      return;
    }
    if (action == key_action_t::End) {
      actions.dispatch(ActionId::ConfigFileLast, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.config.selected_file > 4
                                          ? state.config.selected_file - 4
                                          : 0),
                     0);
      return;
    }
  }

  if (state.screen == ScreenMode::Lattice && input.empty()) {
    const key_action_t action = standard_key_action(key);
    const auto worklist_text = active_left_worklist_text(state, ui);
    if (is_enter_key(key)) {
      actions.dispatch(ActionId::ShowLatticeSummary, state);
      return;
    }
    if (action == key_action_t::MoveUp || key == 'k' || key == 'K' ||
        key == 'h' || key == '[') {
      actions.dispatch(ActionId::LatticeTargetPrev, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.lattice.selected_target > 4
                                          ? state.lattice.selected_target - 4
                                          : 0),
                     0);
      return;
    }
    if (action == key_action_t::MoveDown || key == 'j' || key == 'J' ||
        key == 'l' || key == 'L' || key == ']') {
      actions.dispatch(ActionId::LatticeTargetNext, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.lattice.selected_target > 4
                                          ? state.lattice.selected_target - 4
                                          : 0),
                     0);
      return;
    }
    if (action == key_action_t::Home) {
      actions.dispatch(ActionId::LatticeTargetFirst, state);
      scroll_text_to(worklist_text, 0, 0);
      return;
    }
    if (action == key_action_t::End) {
      actions.dispatch(ActionId::LatticeTargetLast, state);
      scroll_text_to(worklist_text,
                     static_cast<int>(state.lattice.selected_target > 4
                                          ? state.lattice.selected_target - 4
                                          : 0),
                     0);
      return;
    }
  }

  if (standard_key_action(key) == key_action_t::PageUp ||
      standard_key_action(key) == key_action_t::PageDown ||
      standard_key_action(key) == key_action_t::Home ||
      standard_key_action(key) == key_action_t::End) {
    scroll_workspace_by_key(state, ui, key);
    return;
  }

  if (is_enter_key(key)) {
    if (!input.empty()) {
      remember_command(state, input);
      actions.dispatch(input, state);
      set_text(ui.input, "");
      leave_command_mode(state);
      sync_command_input_focus(state, ui);
    }
    return;
  }

  const auto result = handle_text_box_key(ui.input, key);
  if (result.content_changed) {
    if (is_command_text_key(key)) {
      state.command_mode = true;
      state.command_focused = true;
    }
    sync_command_input_focus(state, ui);
    reset_command_history_cursor(state);
    state.status = "editing";
  }
}

inline bool apply_option_commands(CmdState &state, const CmdOptions &opts,
                                  const ActionRegistry &actions);

inline void apply_option_presentation(CmdState &state, const CmdOptions &opts,
                                      const ActionRegistry *actions = nullptr);

inline int run_interactive(CmdState &state, const CmdOptions &opts) {
  ActionRegistry actions = create_default_actions();
  CmdUi ui = create_ui(state.global_config_path);
  auto iinuji_state = initialize_iinuji_state(ui.root, false);
  (void)set_focused_object(*iinuji_state, ui.input);
  const bool home_animation_dynamic =
      animation_has_multiple_frames(ui.home_animation);

  NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms =
      desired_input_timeout_for_screen(state.screen, home_animation_dynamic);
  NcursesApp app(app_opts);
  apply_logs_mouse_capture(state);
  int current_input_timeout = app_opts.input_timeout_ms;
  std::uint64_t last_idle_refresh_ms = now_ms();
  std::uint64_t last_interaction_ms = last_idle_refresh_ms;
  bool dirty = true;

  run_bootstrap_refresh_splash(app, ui, state);
  (void)apply_option_commands(state, opts, actions);
  apply_option_presentation(state, opts, &actions);
  dirty = true;

  while (!state.quit) {
    const std::uint64_t frame_now_ms = now_ms();
    if (should_animate_home_frame(state, home_animation_dynamic)) {
      state.animation_tick = frame_now_ms / 180u;
      dirty = true;
    }
    const std::uint64_t idle_refresh_ms =
        idle_refresh_period_ms_for_screen(state.screen, home_animation_dynamic);
    if (frame_now_ms - last_idle_refresh_ms >= idle_refresh_ms) {
      dirty = refresh_visible_screen_on_idle(state, home_animation_dynamic) ||
              dirty;
      last_idle_refresh_ms = frame_now_ms;
    }
    if (should_poll_runtime_device_snapshot(state, frame_now_ms,
                                            last_interaction_ms)) {
      dirty = maybe_refresh_runtime_device_snapshot(
                  state, kRuntimeDeviceRefreshPeriodMs) ||
              dirty;
    }
    const int desired_timeout =
        desired_input_timeout_for_screen(state.screen, home_animation_dynamic);
    if (desired_timeout != current_input_timeout) {
      ::wtimeout(stdscr, desired_timeout);
      current_input_timeout = desired_timeout;
    }
    if (dirty) {
      mirror_operator_status_to_logs(state);
      update_ui(ui, state, actions);
      int height = 0;
      int width = 0;
      app.renderer().size(height, width);
      app.renderer().clear();
      layout_tree(ui.root, Rect{0, 0, width, height});
      render_tree(ui.root);
      app.renderer().flush();
      dirty = false;
    }
    const int key = read_command_key(app, current_input_timeout);
    if (key != ERR) {
      handle_key(key, actions, state, ui);
      apply_logs_mouse_capture(state);
      last_interaction_ms = now_ms();
      dirty = true;
    }
  }
  run_visual_splash(app, ui, state, "Good luck!", "farewell",
                    farewell_splash_status_text(), kFarewellHoldMs,
                    &ui.closing_static_image, false);
  return 0;
}

inline std::string
make_overlay_columns_snapshot_text(const MenuOverlayColumns &columns) {
  std::ostringstream out;
  const std::size_t rows =
      std::max(columns.commands.size(), columns.comments.size());
  for (std::size_t i = 0; i < rows; ++i) {
    const bool is_header =
        i < columns.commands.size() && i < columns.comments.size() &&
        columns.commands[i].emphasis == text_line_emphasis_t::Accent &&
        columns.comments[i].text.empty() && !columns.commands[i].text.empty();
    const bool next_is_header =
        i + 1u < columns.commands.size() && i + 1u < columns.comments.size() &&
        columns.commands[i + 1u].emphasis == text_line_emphasis_t::Accent &&
        columns.comments[i + 1u].text.empty() &&
        !columns.commands[i + 1u].text.empty();
    const bool frame_header = is_header && !next_is_header;
    const std::string command =
        i < columns.commands.size() ? columns.commands[i].text : std::string{};
    const std::string comment =
        i < columns.comments.size() ? columns.comments[i].text : std::string{};
    if (frame_header)
      out << detail_section_rule() << "\n";
    out << command;
    if (!comment.empty()) {
      if (command.size() < kMenuOverlayCommandWidth)
        out << std::string(kMenuOverlayCommandWidth - command.size(), ' ');
      else
        out << " ";
      out << comment;
    }
    out << "\n";
    if (frame_header)
      out << detail_section_rule() << "\n";
  }
  return out.str();
}

inline std::string action_menu_visible_range_text(std::size_t total,
                                                  std::size_t top,
                                                  std::size_t visible) {
  if (total == 0u)
    return "0-0/0";
  const std::size_t last = std::min(total, top + visible);
  return std::to_string(top + 1u) + "-" + std::to_string(last) + "/" +
         std::to_string(total);
}

inline std::string action_menu_scrollbar_text(std::size_t total,
                                              std::size_t top,
                                              std::size_t visible) {
  constexpr std::size_t kWidth = 10u;
  if (total == 0u)
    return "░░░░░░░░░░";
  if (total <= visible)
    return "██████████";

  const std::size_t thumb =
      std::max<std::size_t>(1u, (visible * kWidth + total - 1u) / total);
  const std::size_t max_top = total > visible ? total - visible : 0u;
  const std::size_t span = kWidth > thumb ? kWidth - thumb : 0u;
  const std::size_t thumb_top =
      max_top == 0u ? 0u
                    : std::min(span, (top * span + max_top / 2u) / max_top);

  std::string out{};
  for (std::size_t i = 0; i < kWidth; ++i)
    out += (i >= thumb_top && i < thumb_top + thumb) ? "█" : "░";
  return out;
}

inline MenuOverlayColumns
make_action_menu_snapshot_overlay_columns(CmdState state,
                                          const ActionRegistry &actions) {
  clamp_action_menu_selection(state, actions);
  const MenuOverlayColumns all =
      make_action_menu_overlay_columns(state, actions);
  const std::size_t total = actions.actions().size();
  if (total == 0u)
    return all;

  const std::size_t visible =
      std::min<std::size_t>(kActionMenuVisibleReserve, total);
  const std::size_t selected = std::min(state.action_menu_selected, total - 1u);
  const std::size_t max_top = total > visible ? total - visible : 0u;
  std::size_t top = 0u;
  if (state.action_menu_scroll_y > kActionMenuHeaderRows) {
    top = static_cast<std::size_t>(state.action_menu_scroll_y -
                                   kActionMenuHeaderRows);
  }
  top = std::min(top, max_top);
  if (selected < top)
    top = selected;
  else if (selected >= top + visible)
    top = selected + 1u - visible;
  top = std::min(top, max_top);
  const std::size_t last = std::min(total, top + visible);

  MenuOverlayColumns out{};
  const std::size_t header_rows =
      std::min<std::size_t>(kActionMenuHeaderRows, all.commands.size());
  for (std::size_t i = 0; i < header_rows; ++i) {
    out.commands.push_back(all.commands[i]);
    out.comments.push_back(i < all.comments.size() ? all.comments[i]
                                                   : styled_text_line_t{});
  }

  const std::string range = action_menu_visible_range_text(total, top, visible);
  push_menu_row(out, "visible",
                range + " shown | " +
                    action_menu_scrollbar_text(total, top, visible),
                text_line_emphasis_t::Accent, text_line_emphasis_t::Info);

  for (std::size_t i = top; i < last; ++i) {
    const std::size_t source_row = kActionMenuHeaderRows + i;
    if (source_row < all.commands.size())
      out.commands.push_back(all.commands[source_row]);
    if (source_row < all.comments.size())
      out.comments.push_back(all.comments[source_row]);
  }
  return out;
}

inline std::string
make_action_menu_snapshot_text(CmdState state, const ActionRegistry &actions) {
  if (state.action_menu_open) {
    state.help_view = false;
  } else {
    actions.open_action_menu(state);
  }
  clamp_action_menu_selection(state, actions);
  std::ostringstream out;
  append_snapshot_shell_chrome(out, state);
  append_snapshot_status(out, state);
  out << "---\n";
  out << "Action palette\n\n";
  out << make_overlay_columns_snapshot_text(
      make_action_menu_snapshot_overlay_columns(state, actions));
  out << "---\n";
  out << make_command_line_snapshot_text(state);
  return out.str();
}

inline std::string action_primary_path(const Action &action) {
  for (const auto &alias : action.aliases) {
    if (alias.rfind("iinuji.", 0) == 0)
      return alias;
  }
  return action.aliases.empty() ? action.label : action.aliases.front();
}

inline bool action_alias_is_removed_f2_hero_name(ActionId id,
                                                 std::string_view alias) {
  if (id != ActionId::ShowWorkbench && id != ActionId::ShowWorkbenchSummary)
    return false;
  return alias.find("marshal") != std::string_view::npos ||
         alias.find("inbox") != std::string_view::npos ||
         alias.find("human") != std::string_view::npos;
}

inline std::string action_short_aliases_text(const Action &action,
                                             std::size_t max_aliases = 0u) {
  std::ostringstream out;
  std::size_t shown = 0;
  for (const auto &alias : action.aliases) {
    if (alias.rfind("iinuji.", 0) == 0)
      continue;
    if (action_alias_is_removed_f2_hero_name(action.id, alias))
      continue;
    if (shown > 0)
      out << " | ";
    out << alias;
    ++shown;
    if (max_aliases > 0u && shown >= max_aliases)
      break;
  }
  if (shown == 0)
    return "-";
  return out.str();
}

inline std::string action_secondary_paths_text(const Action &action,
                                               const std::string &primary) {
  std::ostringstream out;
  std::size_t shown = 0;
  for (const auto &alias : action.aliases) {
    if (alias.rfind("iinuji.", 0) != 0 || alias == primary)
      continue;
    if (action_alias_is_removed_f2_hero_name(action.id, alias))
      continue;
    if (shown > 0)
      out << " | ";
    out << alias;
    ++shown;
  }
  if (shown == 0)
    return "-";
  return out.str();
}

inline const std::array<std::string_view, 10> &
command_catalog_category_order() {
  static constexpr std::array<std::string_view, 10> order{
      "screen",  "show",   "menu",    "refresh",   "logs",
      "runtime", "config", "lattice", "workspace", "app",
  };
  return order;
}

inline void append_command_catalog_action(std::ostringstream &out,
                                          const Action &action) {
  const std::string primary = action_primary_path(action);
  out << "  " << primary << "\n";
  out << "    label: " << action.label << "\n";
  out << "    aliases: " << action_short_aliases_text(action) << "\n";
  const std::string secondary_paths =
      action_secondary_paths_text(action, primary);
  if (secondary_paths != "-")
    out << "    paths: " << secondary_paths << "\n";
}

inline void append_command_catalog_section_header(std::ostringstream &out,
                                                  std::string_view section) {
  std::string title{"["};
  title += section;
  title += "]";
  append_detail_section_header(out, title);
}

inline void append_command_catalog_action_run_paths(std::ostringstream &out) {
  out << "  iinuji.actions.run()\n";
  out << "    label: run selected palette action\n";
  out << "    aliases: actions run | action run | run action | commands run | "
         "palette run | run palette\n";
  out << "  iinuji.commands.run.selected()\n";
  out << "    label: run selected palette action\n";
  out << "    aliases: iinuji.commands.run()\n";
}

inline void
append_command_catalog_shell_snapshot_paths(std::ostringstream &out) {
  out << "  cuwacunu_cmd --snapshot --menu\n";
  out << "    label: render menu overlay snapshot\n";
  out << "    aliases: cuwacunu_cmd --menu --snapshot\n";
  out << "  cuwacunu.cmd --snapshot --menu\n";
  out << "    label: render menu overlay snapshot\n";
  out << "    aliases: cuwacunu.cmd --menu --snapshot\n";
  out << "  cuwacunu_cmd --snapshot --actions\n";
  out << "    label: render action palette snapshot\n";
  out << "    aliases: cuwacunu_cmd --actions --snapshot | "
         "cuwacunu_cmd --snapshot --palette\n";
  out << "  cuwacunu.cmd --snapshot --actions\n";
  out << "    label: render action palette snapshot\n";
  out << "    aliases: cuwacunu.cmd --actions --snapshot | "
         "cuwacunu.cmd --snapshot --palette\n";
  out << "  cuwacunu_cmd --snapshot --catalog\n";
  out << "    label: render command catalog snapshot\n";
  out << "    aliases: cuwacunu_cmd --catalog --snapshot | "
         "cuwacunu_cmd --commands --snapshot | "
         "cuwacunu_cmd --snapshot --commands\n";
  out << "  cuwacunu.cmd --snapshot --catalog\n";
  out << "    label: render command catalog snapshot\n";
  out << "    aliases: cuwacunu.cmd --catalog --snapshot | "
         "cuwacunu.cmd --commands --snapshot | "
         "cuwacunu.cmd --snapshot --commands\n";
  out << "  cuwacunu_cmd --snapshot --visual\n";
  out << "    label: render Home waajacamaya visual snapshot\n";
  out << "    aliases: cuwacunu_cmd --visual --snapshot | "
         "cuwacunu_cmd --snapshot --home-visual | "
         "cuwacunu_cmd --snapshot --waajacamaya\n";
  out << "  cuwacunu.cmd --snapshot --visual\n";
  out << "    label: render Home waajacamaya visual snapshot\n";
  out << "    aliases: cuwacunu.cmd --visual --snapshot | "
         "cuwacunu.cmd --snapshot --home-visual | "
         "cuwacunu.cmd --snapshot --waajacamaya\n";
  out << "  cuwacunu_cmd --snapshot --image\n";
  out << "    label: render Home still image snapshot\n";
  out << "    aliases: cuwacunu_cmd --image --snapshot\n";
  out << "  cuwacunu.cmd --snapshot --image\n";
  out << "    label: render Home still image snapshot\n";
  out << "    aliases: cuwacunu.cmd --image --snapshot\n";
  out << "  cuwacunu_cmd --snapshot --animation\n";
  out << "    label: render Home APNG motion snapshot\n";
  out << "    aliases: cuwacunu_cmd --animation --snapshot\n";
  out << "  cuwacunu.cmd --snapshot --animation\n";
  out << "    label: render Home APNG motion snapshot\n";
  out << "    aliases: cuwacunu.cmd --animation --snapshot\n";
  out << "  cuwacunu_cmd --snapshot --splash loading\n";
  out << "    label: render bootstrap splash snapshot\n";
  out << "    aliases: cuwacunu_cmd --splash loading --snapshot | "
         "cuwacunu_cmd --snapshot --bootstrap | cuwacunu_cmd --snapshot "
         "--boot\n";
  out << "  cuwacunu.cmd --snapshot --splash loading\n";
  out << "    label: render bootstrap splash snapshot\n";
  out << "    aliases: cuwacunu.cmd --splash loading --snapshot | "
         "cuwacunu.cmd --snapshot --bootstrap | cuwacunu.cmd --snapshot "
         "--boot\n";
  out << "  cuwacunu_cmd --snapshot --splash close\n";
  out << "    label: render closing splash snapshot\n";
  out << "    aliases: cuwacunu_cmd --splash close --snapshot | "
         "cuwacunu_cmd --snapshot --farewell | cuwacunu_cmd --snapshot "
         "--closing\n";
  out << "  cuwacunu.cmd --snapshot --splash close\n";
  out << "    label: render closing splash snapshot\n";
  out << "    aliases: cuwacunu.cmd --splash close --snapshot | "
         "cuwacunu.cmd --snapshot --farewell | cuwacunu.cmd --snapshot "
         "--closing\n";
  out << "  cuwacunu_cmd --snapshot --splash good-luck\n";
  out << "    label: render Good luck splash snapshot\n";
  out << "    aliases: cuwacunu_cmd --splash good-luck --snapshot | "
         "cuwacunu_cmd --snapshot --splash=good-luck\n";
  out << "  cuwacunu.cmd --snapshot --splash good-luck\n";
  out << "    label: render Good luck splash snapshot\n";
  out << "    aliases: cuwacunu.cmd --splash good-luck --snapshot | "
         "cuwacunu.cmd --snapshot --splash=good-luck\n";
  out << "  cuwacunu_cmd --snapshot --screen runtime\n";
  out << "    label: render Runtime screen snapshot\n";
  out << "    aliases: cuwacunu_cmd --snapshot --screen=runtime\n";
  out << "  cuwacunu.cmd --snapshot --screen runtime\n";
  out << "    label: render Runtime screen snapshot\n";
  out << "    aliases: cuwacunu.cmd --snapshot --screen=runtime\n";
  out << "  cuwacunu_cmd --snapshot --full\n";
  out << "    label: render full-panel snapshot\n";
  out << "    aliases: cuwacunu_cmd --full --snapshot | "
         "cuwacunu_cmd --snapshot --zoom\n";
  out << "  cuwacunu.cmd --snapshot --full\n";
  out << "    label: render full-panel snapshot\n";
  out << "    aliases: cuwacunu.cmd --full --snapshot | "
         "cuwacunu.cmd --snapshot --zoom\n";
  out << "  cuwacunu_cmd hero.runtime.dsl --snapshot\n";
  out << "    label: open managed Config file by path token through shell "
         "snapshot\n";
  out << "    aliases: cuwacunu_cmd src/config/hero.runtime.dsl --snapshot | "
         "src/config/hero.runtime.dsl\n";
  out << "  cuwacunu.cmd hero.runtime.dsl --snapshot\n";
  out << "    label: open managed Config file by path token through shell "
         "snapshot\n";
  out << "    aliases: cuwacunu.cmd src/config/hero.runtime.dsl --snapshot | "
         "src/config/hero.runtime.dsl\n";
}

inline void append_command_catalog_current_screen(std::ostringstream &out,
                                                  const CmdState &state) {
  append_command_catalog_section_header(out, "current");
  out << "  screen: " << screen_key_label(state.screen) << " "
      << screen_badge_label(state.screen) << "\n";
  out << "  state: " << trim_for_style(make_operator_state_line(state)) << "\n";
  out << "  hint: " << trim_for_style(make_operator_hint_line(state)) << "\n";
  out << "  shortcuts:\n";
  for (const std::string &line :
       split_styled_content_lines(make_nav_current_tools_text(state))) {
    const std::string trimmed = trim_for_style(line);
    if (trimmed.empty() || trimmed == "Current")
      continue;
    out << "    " << trimmed << "\n";
  }
  out << "\n";
}

inline std::string
make_command_catalog_snapshot_text(const ActionRegistry &actions,
                                   const CmdState *state = nullptr) {
  std::ostringstream out;
  out << "Command catalog\n";
  out << "Generated from the active action registry and shell utility "
         "entries.\n\n";
  if (state != nullptr)
    append_command_catalog_current_screen(out, *state);

  bool first_category = true;
  for (const std::string_view category : command_catalog_category_order()) {
    bool wrote_header = false;
    for (const auto &action : actions.actions()) {
      if (action_category_label(action.id) != category)
        continue;
      if (!wrote_header) {
        if (!first_category)
          out << "\n";
        first_category = false;
        wrote_header = true;
        append_command_catalog_section_header(out, category);
      }
      append_command_catalog_action(out, action);
    }
    if (category == "menu") {
      if (!wrote_header) {
        if (!first_category)
          out << "\n";
        first_category = false;
        wrote_header = true;
        append_command_catalog_section_header(out, category);
      }
      append_command_catalog_action_run_paths(out);
      append_command_catalog_shell_snapshot_paths(out);
    }
  }

  out << "\n";
  append_command_catalog_section_header(out, "dynamic");
  out << "  iinuji.config.file.index.n1()\n";
  out << "    label: select config file by one-based index\n";
  out << "    aliases: iinuji.config.file.index.1() | "
         "iinuji.config.file.index.N() | config file 1 | file n1 | "
         "iinuji.config.file.index(n=1)\n";
  out << "  iinuji.config.file.id.TOKEN()\n";
  out << "    label: select config file by id token\n";
  out << "    aliases: iinuji.config.file.id.default() | config file "
         "default | file default | iinuji.config.file.id(value=default)\n";
  out << "  iinuji.lattice.target.index.n1()\n";
  out << "    label: select lattice target by one-based index\n";
  out << "    aliases: iinuji.lattice.target.index.1() | "
         "iinuji.lattice.target.index.N() | lattice target 1 | target n1 | "
         "iinuji.lattice.target.index(n=1)\n";
  out << "  iinuji.lattice.target.id.TOKEN()\n";
  out << "    label: select lattice target by id token\n";
  out << "    aliases: iinuji.lattice.target.id.default() | lattice target "
         "default | target default | iinuji.lattice.target.id(value=default)\n";

  return out.str();
}

inline void
append_splash_asset_report(std::ostringstream &out,
                           const std::filesystem::path &global_config_path) {
  const CmdUiAssetPaths paths = resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets assets = load_home_showcase_assets(
      paths.loading_logo.string(), paths.home_animation.string());

  append_detail_section_header(out, "Asset report");
  out << "  loading logo: " << paths.loading_logo.string() << "\n";
  out << "    status: " << (assets.loading_logo_ok ? "loaded" : "unavailable")
      << "\n";
  if (!assets.loading_logo_ok && !assets.loading_logo_error.empty())
    out << "    error: " << assets.loading_logo_error << "\n";
  out << "  closing logo: " << paths.closing_logo.string() << "\n";
  if (paths.closing_logo == paths.loading_logo) {
    out << "    status: shares loading logo\n";
  } else {
    rgba_image_t closing_image{};
    std::string closing_error{};
    const bool closing_ok = decode_png_rgba_file(paths.closing_logo.string(),
                                                 closing_image, closing_error);
    out << "    status: " << (closing_ok ? "loaded" : "unavailable") << "\n";
    if (!closing_ok && !closing_error.empty())
      out << "    error: " << closing_error << "\n";
  }
  out << "  home animation: " << paths.home_animation.string() << "\n";
  out << "    status: " << (assets.home_animation_ok ? "loaded" : "unavailable")
      << "\n";
  if (assets.home_animation_ok) {
    out << "    frames: " << assets.home_animation.frames.size()
        << " dynamic=" << yes_no(assets.home_animation_is_dynamic)
        << " size=" << assets.home_animation.width << "x"
        << assets.home_animation.height << "\n";
  } else if (!assets.home_animation_error.empty()) {
    out << "    error: " << assets.home_animation_error << "\n";
  }
  out << "\n";
}

inline std::string render_image_preview_text(const rgba_image_t &image,
                                             int width, int height,
                                             image_grayscale_options_t options);

inline std::string splash_snapshot_mode_label(SplashSnapshotMode mode) {
  return mode == SplashSnapshotMode::Farewell ? "farewell" : "bootstrap";
}

inline rgba_image_t splash_snapshot_preview_image(
    SplashSnapshotMode mode, const std::filesystem::path &global_config_path,
    std::string &source, std::size_t &frame_index, std::size_t &frame_count) {
  const CmdUiAssetPaths paths = resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets assets = load_home_showcase_assets(
      paths.loading_logo.string(), paths.home_animation.string());

  frame_index = 0u;
  frame_count = 1u;

  if (mode == SplashSnapshotMode::Farewell) {
    rgba_image_t closing_image{};
    std::string closing_error{};
    if (paths.closing_logo == paths.loading_logo && assets.loading_logo_ok) {
      source = "shared loading logo";
      return assets.loading_logo_image;
    }
    if (decode_png_rgba_file(paths.closing_logo.string(), closing_image,
                             closing_error)) {
      source = "closing logo";
      return closing_image;
    }
    if (assets.loading_logo_ok) {
      source = "loading logo fallback";
      return assets.loading_logo_image;
    }
    source = "fallback wordmark";
    return make_fallback_home_wordmark_image();
  }

  const rgba_animation_t animation = make_composited_home_animation(assets);
  if (animation.valid()) {
    frame_count = animation.frames.size();
    frame_index = frame_count > 1u ? frame_count / 2u : 0u;
    source = frame_count > 1u ? "home animation frame" : "home animation still";
    return animation.frames[frame_index];
  }
  if (assets.loading_logo_ok) {
    source = "loading logo + wordmark";
    return make_static_home_showcase_image(assets);
  }
  source = "fallback wordmark";
  return make_fallback_home_wordmark_image();
}

inline void
append_splash_image_preview(std::ostringstream &out, SplashSnapshotMode mode,
                            const std::filesystem::path &global_config_path) {
  std::string source{};
  std::size_t frame_index = 0u;
  std::size_t frame_count = 1u;
  const rgba_image_t preview = splash_snapshot_preview_image(
      mode, global_config_path, source, frame_index, frame_count);

  append_detail_section_header(out, "Preview image");
  out << "  mode: " << splash_snapshot_mode_label(mode) << "\n";
  out << "  source: " << source << "\n";
  if (frame_count > 1u)
    out << "  frame: " << (frame_index + 1u) << "/" << frame_count << "\n";
  out << "  render: braille preview 44x12\n\n";
  out << render_image_preview_text(preview, 44, 12,
                                   make_home_showcase_render_options());
  out << "\n";
}

inline std::string utf8_from_codepoint(char32_t cp) {
  std::string out{};
  if (cp <= 0x7Fu) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FFu) {
    out.push_back(static_cast<char>(0xC0u | ((cp >> 6u) & 0x1Fu)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  } else if (cp <= 0xFFFFu) {
    out.push_back(static_cast<char>(0xE0u | ((cp >> 12u) & 0x0Fu)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 6u) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  } else {
    out.push_back(static_cast<char>(0xF0u | ((cp >> 18u) & 0x07u)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 12u) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | ((cp >> 6u) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  }
  return out;
}

class TextImagePreviewRenderer final : public IRend {
public:
  TextImagePreviewRenderer(int height, int width)
      : height_(std::max(1, height)), width_(std::max(1, width)),
        cells_(
            static_cast<std::size_t>(height_),
            std::vector<std::string>(static_cast<std::size_t>(width_), " ")) {}

  void size(int &height, int &width) override {
    height = height_;
    width = width_;
  }

  void clear() override {
    for (auto &row : cells_)
      std::fill(row.begin(), row.end(), " ");
  }

  void flush() override {}

  void putText(int y, int x, const std::string &text, int max_width = -1,
               short = 0, bool = false, bool = false) override {
    if (y < 0 || y >= height_ || x >= width_)
      return;
    const std::size_t limit =
        max_width < 0 ? text.size()
                      : utf8_prefix_bytes_for_width(text, max_width);
    int col = std::max(0, x);
    for (std::size_t offset = 0; offset < limit && col < width_;) {
      const std::size_t bytes = utf8_codepoint_bytes(text, offset);
      if (bytes == 0)
        break;
      if (x >= 0)
        cells_[static_cast<std::size_t>(y)][static_cast<std::size_t>(col)] =
            text.substr(offset, bytes);
      offset += bytes;
      ++col;
    }
  }

  void putGlyph(int y, int x, wchar_t ch, short = 0) override {
    if (y < 0 || y >= height_ || x < 0 || x >= width_)
      return;
    cells_[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] =
        utf8_from_codepoint(static_cast<char32_t>(ch));
  }

  void fillRect(int y, int x, int height, int width, short = 0) override {
    for (int row = 0; row < height; ++row) {
      const int dst_y = y + row;
      if (dst_y < 0 || dst_y >= height_)
        continue;
      for (int col = 0; col < width; ++col) {
        const int dst_x = x + col;
        if (dst_x < 0 || dst_x >= width_)
          continue;
        cells_[static_cast<std::size_t>(dst_y)]
              [static_cast<std::size_t>(dst_x)] = " ";
      }
    }
  }

  std::string str() const {
    std::ostringstream out;
    for (const auto &row : cells_) {
      std::size_t end = row.size();
      while (end > 0u && row[end - 1u] == " ")
        --end;
      for (std::size_t i = 0; i < end; ++i)
        out << row[i];
      out << "\n";
    }
    return out.str();
  }

private:
  int height_{0};
  int width_{0};
  std::vector<std::vector<std::string>> cells_{};
};

inline std::string render_image_preview_text(
    const rgba_image_t &image, int width = 44, int height = 12,
    image_grayscale_options_t options = make_home_showcase_render_options()) {
  if (!image.valid())
    return {};
  options.use_color = false;
  TextImagePreviewRenderer renderer{height, width};
  IRend *old_renderer = set_renderer(&renderer);
  renderer.clear();
  render_image_grayscale(image, 0, 0, width, height, options);
  set_renderer(old_renderer);
  return renderer.str();
}

inline std::vector<std::size_t>
home_motion_strip_indices(std::size_t frame_count,
                          std::size_t max_motion_frames = 4u) {
  std::vector<std::size_t> indices{};
  if (frame_count <= 1u || max_motion_frames == 0u)
    return indices;

  const std::size_t sample_count =
      std::min(max_motion_frames, frame_count - 1u);
  indices.reserve(sample_count);
  for (std::size_t step = 1u; step <= sample_count; ++step) {
    const std::size_t index =
        std::min(frame_count - 1u, (step * (frame_count - 1u)) / sample_count);
    if (indices.empty() || indices.back() != index)
      indices.push_back(index);
  }
  return indices;
}

inline void append_home_motion_strip(std::ostringstream &out,
                                     const rgba_animation_t &animation,
                                     int width = 36, int height = 9) {
  const auto indices = home_motion_strip_indices(animation.frames.size());
  if (indices.empty())
    return;

  out << "\n";
  append_detail_section_header(out, "Motion strip");
  out << "motion strip " << indices.size() << " frames\n";
  for (const std::size_t index : indices) {
    out << "preview frame " << (index + 1u) << "/" << animation.frames.size()
        << "\n";
    out << render_image_preview_text(animation.frames[index], width, height);
  }
}

inline std::string make_home_visual_snapshot_text(
    const std::filesystem::path &global_config_path =
        std::filesystem::path{"/cuwacunu/src/config/.config"},
    HomeVisualSnapshotMode mode = HomeVisualSnapshotMode::Full) {
  if (mode == HomeVisualSnapshotMode::None)
    mode = HomeVisualSnapshotMode::Full;
  const CmdUiAssetPaths paths = resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets assets = load_home_showcase_assets(
      paths.loading_logo.string(), paths.home_animation.string());
  const rgba_animation_t animation = make_composited_home_animation(assets);

  rgba_image_t fallback{};
  if (animation.valid()) {
    fallback = animation.frames.front();
  } else if (assets.loading_logo_ok) {
    fallback = make_static_home_showcase_image(assets);
  } else {
    fallback = make_fallback_home_wordmark_image();
  }

  const bool dynamic = animation.valid() && animation.frames.size() > 1u;
  const std::size_t frame_count =
      animation.valid() ? animation.frames.size() : std::size_t{1u};

  std::ostringstream out;
  switch (mode) {
  case HomeVisualSnapshotMode::Image:
    out << "Home image snapshot\n\n";
    append_detail_section_header(out, "Home image");
    out << "primitive: image_box still + Cuwacunu wordmark\n";
    break;
  case HomeVisualSnapshotMode::Animation:
    out << "Home animation snapshot\n\n";
    append_detail_section_header(out, "Home animation");
    out << "primitive: APNG/GIF/PNG animation frames + image_box\n";
    break;
  case HomeVisualSnapshotMode::Full:
  case HomeVisualSnapshotMode::None:
    out << "Home visual snapshot\n\n";
    append_detail_section_header(out, "Home visual");
    out << "primitive: image_box + APNG/GIF/PNG animation frames\n";
    break;
  }
  out << "status: "
      << (animation.valid()
              ? "animation loaded"
              : (assets.loading_logo_ok ? "static loading logo + wordmark"
                                        : "fallback wordmark"))
      << "\n";
  out << "frames: " << frame_count << " dynamic=" << yes_no(dynamic);
  if (animation.valid())
    out << " total_ms=" << animation.total_duration_ms()
        << " size=" << animation.width << "x" << animation.height;
  out << "\n\n";

  append_detail_section_header(out, "Assets");
  out << "loading logo: " << paths.loading_logo.string() << "\n";
  out << "home animation: " << paths.home_animation.string() << "\n\n";

  append_detail_section_header(out, mode == HomeVisualSnapshotMode::Animation
                                        ? "Animation preview"
                                        : "Preview image");
  out << "render: braille preview 44x12\n\n";

  if (mode == HomeVisualSnapshotMode::Image)
    out << "still frame\n";
  else
    out << "preview frame 1/" << frame_count << "\n";
  out << render_image_preview_text(fallback);

  if (dynamic && mode != HomeVisualSnapshotMode::Image)
    append_home_motion_strip(out, animation);

  return out.str();
}

inline std::string make_home_snapshot_visual_preview_text(
    const std::filesystem::path &global_config_path =
        std::filesystem::path{"/cuwacunu/src/config/.config"},
    HomePresentationMode presentation = HomePresentationMode::Showcase) {
  if (presentation == HomePresentationMode::BootstrapSplash ||
      presentation == HomePresentationMode::FarewellSplash) {
    const SplashSnapshotMode mode =
        presentation == HomePresentationMode::FarewellSplash
            ? SplashSnapshotMode::Farewell
            : SplashSnapshotMode::Bootstrap;
    std::ostringstream out;
    append_detail_section_header(out, "Splash preview");
    out << "  primitive: image_box splash frame\n";
    out << "  status: " << splash_snapshot_mode_label(mode) << " visual\n\n";
    append_splash_image_preview(out, mode, global_config_path);
    return out.str();
  }

  const CmdUiAssetPaths paths = resolve_cmd_ui_asset_paths(global_config_path);
  const HomeShowcaseAssets assets = load_home_showcase_assets(
      paths.loading_logo.string(), paths.home_animation.string());
  const rgba_animation_t animation = make_composited_home_animation(assets);

  rgba_image_t fallback{};
  if (animation.valid()) {
    fallback = animation.frames.front();
  } else if (assets.loading_logo_ok) {
    fallback = make_static_home_showcase_image(assets);
  } else {
    fallback = make_fallback_home_wordmark_image();
  }

  const bool dynamic = animation.valid() && animation.frames.size() > 1u;
  const std::size_t frame_count =
      animation.valid() ? animation.frames.size() : std::size_t{1u};

  std::ostringstream out;
  append_detail_section_header(out, "Visual preview");
  out << "  primitive: image_box + APNG/GIF/PNG animation frames\n";
  out << "  status: "
      << (animation.valid()
              ? "animation loaded"
              : (assets.loading_logo_ok ? "static loading logo + wordmark"
                                        : "fallback wordmark"))
      << "\n";
  out << "  frames: " << frame_count << " dynamic=" << yes_no(dynamic) << "\n";
  append_detail_section_header(out, "Preview image");
  out << "  preview frame 1/" << frame_count << "\n";
  out << render_image_preview_text(fallback, 36, 9);
  if (dynamic) {
    const std::size_t mid_index = animation.frames.size() / 2u;
    append_detail_section_header(out, "Motion frame");
    out << "  motion frame " << (mid_index + 1u) << "/" << frame_count << "\n";
    out << render_image_preview_text(animation.frames[mid_index], 36, 9);
  }
  return out.str();
}

inline std::string make_snapshot_text_with_visuals(
    const CmdState &state,
    const std::filesystem::path &global_config_path = std::filesystem::path{
        "/cuwacunu/src/config/.config"}) {
  if (state.screen != ScreenMode::Home)
    return make_snapshot_text(state);

  std::ostringstream out;
  append_snapshot_shell_chrome(out, state);
  append_snapshot_status(out, state);
  out << "---\n";
  out << "waajacamaya\n";
  out << make_home_snapshot_visual_preview_text(global_config_path,
                                                state.home_visual.presentation);
  out << make_main_text(state) << "\n";
  out << "---\n";
  out << make_detail_text(state) << "\n---\n";
  out << make_command_line_snapshot_text(state);
  return out.str();
}

inline std::string make_splash_snapshot_text(
    SplashSnapshotMode mode,
    const std::filesystem::path &global_config_path =
        std::filesystem::path{"/cuwacunu/src/config/.config"},
    const std::string &command_name = "cuwacunu_cmd") {
  if (mode == SplashSnapshotMode::None)
    mode = SplashSnapshotMode::Bootstrap;
  const bool farewell = mode == SplashSnapshotMode::Farewell;
  const std::string title =
      farewell ? "Good luck!"
               : "loading " + display_or(command_name, "cuwacunu_cmd");
  const std::string phase = farewell ? "farewell" : "bootstrap";
  const std::string status =
      farewell ? farewell_splash_status_text() : bootstrap_splash_status_text();
  const double progress = farewell ? 1.0 : 0.50;

  std::ostringstream out;
  out << title << "\n";
  out << "Splash snapshot\n\n";
  append_detail_section_header(out, "waajacamaya");
  out << "  image panel: waajacamaya + Cuwacunu wordmark\n";
  out << "  image primitive: iinuji image_box\n";
  out << "  animation primitive: APNG/GIF/PNG frames on interactive Home and "
         "bootstrap\n\n";
  append_splash_asset_report(out, global_config_path);
  append_splash_image_preview(out, mode, global_config_path);
  append_detail_section_header(out, phase);
  out << make_splash_detail_body_text(phase, 0, progress);
  append_detail_section_header(out, "Screens");
  for (ScreenMode screen : screen_order()) {
    out << screen_key_label(screen) << " " << screen_badge_label(screen)
        << "\n";
  }
  out << "\n";
  append_detail_section_header(out, "Status");
  out << " " << status << "\n";
  return out.str();
}

inline std::string make_menu_overlay_snapshot_text(CmdState state) {
  state.help_view = true;
  state.action_menu_open = false;

  std::ostringstream out;
  append_snapshot_shell_chrome(out, state);
  append_snapshot_status(out, state);
  out << "---\n";
  out << "Menu overlay\n\n";
  out << make_overlay_columns_snapshot_text(make_menu_overlay_columns(state));
  out << "---\n";
  out << make_command_line_snapshot_text(state);
  return out.str();
}

inline bool apply_option_commands(CmdState &state, const CmdOptions &opts,
                                  const ActionRegistry &actions) {
  bool ok = true;
  for (const auto &command : opts.commands) {
    if (trim(command).empty())
      continue;
    remember_command(state, command);
    if (!actions.dispatch(command, state))
      ok = false;
  }
  return ok;
}

inline void apply_option_presentation(CmdState &state, const CmdOptions &opts,
                                      const ActionRegistry *actions) {
  if (opts.zoom)
    (void)workspace_toggle_current_screen_zoom(state);

  if (opts.actions) {
    if (actions != nullptr && !state.action_menu_open) {
      actions->open_action_menu(state);
    } else {
      state.help_view = false;
      state.action_menu_open = true;
      if (actions != nullptr) {
        clamp_action_menu_selection(state, *actions);
      } else {
        state.action_menu_scroll_y = 0;
      }
    }
    return;
  }
  if (opts.menu) {
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.help_scroll_x = 0;
  }
}

inline int run_cuwacunu_cmd(int argc, char **argv) {
  const CmdOptions opts = parse_options(argc, argv);
  if (opts.help) {
    print_help(std::cout);
    return 0;
  }
  if (opts.version) {
    print_version(std::cout, opts.invoked_command_name);
    return 0;
  }

  CmdState state{};
  state.command_name = opts.invoked_command_name;
  state.global_config_path = opts.global_config_path;
  state.screen = opts.initial_screen;
  apply_cmd_ui_log_defaults(state, state.global_config_path);
  refresh_home_visual_state(state, state.global_config_path);
  const ActionRegistry actions = create_default_actions();

  if (opts.snapshot) {
    refresh_snapshots(state);
    const bool commands_ok = apply_option_commands(state, opts, actions);
    apply_option_presentation(state, opts, &actions);
    if (opts.visual_snapshot) {
      std::cout << make_home_visual_snapshot_text(state.global_config_path,
                                                  opts.visual_snapshot_mode);
      return commands_ok ? 0 : 2;
    }
    if (opts.splash_snapshot != SplashSnapshotMode::None) {
      std::cout << make_splash_snapshot_text(
          opts.splash_snapshot, state.global_config_path, state.command_name);
      return commands_ok ? 0 : 2;
    }
    if (state.action_menu_open) {
      std::cout << make_action_menu_snapshot_text(state, actions);
      return commands_ok ? 0 : 2;
    }
    if (opts.command_catalog) {
      std::cout << make_command_catalog_snapshot_text(actions, &state);
      return commands_ok ? 0 : 2;
    }
    if (state.help_view) {
      std::cout << make_menu_overlay_snapshot_text(state);
      return commands_ok ? 0 : 2;
    }
    std::cout << make_snapshot_text_with_visuals(state,
                                                 state.global_config_path);
    return commands_ok ? 0 : 2;
  }
  return run_interactive(state, opts);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
