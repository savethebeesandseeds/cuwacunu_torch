#include <iostream>
#include <string>

#include "piaabo/dconfig.h"

#include "iinuji/iinuji_cmd/app/overlays.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/config/view.h"
#include "iinuji/iinuji_cmd/views/inbox/commands.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"
#include "iinuji/iinuji_cmd/views/ui.h"

namespace {

bool require(bool cond, const std::string &msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

std::string first_line(const std::string &s) {
  const std::size_t pos = s.find('\n');
  return (pos == std::string::npos) ? s : s.substr(0, pos);
}

} // namespace

int main() {
  try {
    const char *global_config_path = "/cuwacunu/src/config/.config";
    cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
    cuwacunu::iitepi::config_space_t::update_config();

    using namespace cuwacunu::iinuji::iinuji_cmd;

    CmdState st{};
    st.inbox.app.global_config_path = global_config_path;
    st.inbox.app.hero_config_path =
        cuwacunu::hero::human_mcp::resolve_human_hero_dsl_path(
            global_config_path);
    st.inbox.app.self_binary_path =
        cuwacunu::hero::human_mcp::current_executable_path();
    std::string human_error{};
    (void)cuwacunu::hero::human_mcp::load_human_defaults(
        st.inbox.app.hero_config_path, global_config_path,
        &st.inbox.app.defaults, &human_error);
    (void)refresh_inbox_state(st);

    st.runtime.app.global_config_path = global_config_path;
    st.runtime.app.hero_config_path =
        cuwacunu::hero::runtime_mcp::resolve_runtime_hero_dsl_path(
            global_config_path);
    st.runtime.app.self_binary_path =
        cuwacunu::hero::runtime_mcp::current_executable_path();
    std::string runtime_error{};
    (void)cuwacunu::hero::runtime_mcp::load_runtime_defaults(
        st.runtime.app.hero_config_path, global_config_path,
        &st.runtime.app.defaults, &runtime_error);
    (void)refresh_runtime_state(st);

    st.config = load_config_view_from_state(st);
    clamp_selected_config_file(st);

    st.screen = ScreenMode::Home;
    const IinujiUi home_ui{st};
    const std::string home_status = home_ui.status_line();
    const std::string home_bottom = home_ui.bottom_line();

    st.screen = ScreenMode::Inbox;
    const IinujiUi ui{st};
    const std::string status = ui.status_line();
    const std::string bottom = ui.bottom_line();
    const std::string left = make_inbox_left(st);
    const std::string right = make_inbox_right(st);

    st.screen = ScreenMode::Runtime;
    const IinujiUi runtime_ui{st};
    const std::string runtime_bottom = runtime_ui.bottom_line();
    const std::string runtime_left = make_runtime_left(st);
    const std::string runtime_right = make_runtime_right(st);
    const auto config_panel = make_config_right_panel(st);

    std::vector<cuwacunu::iinuji::styled_text_line_t> ansi_meta_lines{};
    append_config_meta_line(ansi_meta_lines, "key", "valuevalue");
    const auto ansi_metrics =
        measure_config_styled_box(ansi_meta_lines, 6, 8, true, 0);

    std::string config_source_line{};
    for (const auto &line : config_panel.lines) {
      if (line.text.find("source") != std::string::npos &&
          (line.text.find("global default") != std::string::npos ||
           line.text.find("selected session") != std::string::npos)) {
        config_source_line = line.text;
        break;
      }
    }

    bool ok = true;
    ok = ok && require(home_status.find("[F1 HOME]") != std::string::npos,
                       "home status should highlight F1 HOME");
    ok = ok && require(home_status.find("F2 INBOX") != std::string::npos,
                       "home status should list F2 marshal");
    ok = ok && require(home_bottom.find("waajacu.com") != std::string::npos,
                       "home bottom should keep the waajacu.com note");
    ok = ok && require(status.find("[F2 INBOX]") != std::string::npos,
                       "status should highlight F2 INBOX");
    ok = ok && require(status.find("F3 RUNTIME") != std::string::npos,
                       "status should list F3 runtime");
    ok = ok && require(status.find("F4 LATTICE") != std::string::npos,
                       "status should list F4 lattice");
    ok = ok && require(status.find("Tab cycle inbox") == std::string::npos,
                       "status should keep the top bar focused on screen tabs");
    ok = ok && require(status.find("TRAIN") == std::string::npos,
                       "status should not mention removed training screen");
    ok = ok && require(left.find("> [Inbox]") != std::string::npos,
                       "inbox left should mark Inbox as the selected lane");
    ok = ok && require(left.find("Inbox") != std::string::npos,
                       "inbox left should include inbox heading");
    ok = ok && require(right.find("Inbox is clear.") != std::string::npos,
                       "inbox right should show the empty inbox detail");
    ok = ok && require(bottom.find("lane=Inbox") != std::string::npos,
                       "inbox bottom should show the lane summary");
    ok = ok &&
         require(runtime_left.find("Runtime Ledger") == std::string::npos,
                 "runtime left should not repeat the runtime panel header");
    ok =
        ok && require(runtime_left.find("focus=active anchor above") ==
                          std::string::npos,
                      "runtime left should not show the old filler focus line");
    ok = ok && require(runtime_right.find("Runtime Manifest Detail") ==
                           std::string::npos,
                       "runtime right should not repeat the mode header");
    ok = ok && require(runtime_right.find("content=header metadata above") ==
                           std::string::npos,
                       "runtime right should not show old log filler text");
    ok = ok && require(runtime_bottom.find("Tab cycles manifest/log/trace") !=
                           std::string::npos,
                       "runtime bottom should carry the passive runtime hint");
    ok = ok &&
         require(!config_source_line.empty(),
                 "config right panel should include the policy source line");
    ok = ok &&
         require(config_source_line.find("\x1b[") != std::string::npos,
                 "config policy source line should carry ANSI key coloring");
    ok = ok && require(ansi_metrics.total_rows == 3,
                       "config ANSI metrics should wrap by visible columns");
    {
      auto box = cuwacunu::iinuji::create_object("ansi-scroll-probe");
      auto tb = std::make_shared<cuwacunu::iinuji::textBox_data_t>(
          "", true, cuwacunu::iinuji::text_align_t::Left);
      tb->styled_lines = ansi_meta_lines;
      tb->content = ansi_meta_lines.front().text;
      box->data = tb;
      box->screen = cuwacunu::iinuji::Rect{0, 0, 6, 2};
      const auto caps = panel_scroll_caps(box);
      ok = ok && require(caps.v, "ANSI config meta lines should still report "
                                 "vertical scroll support");
    }

    std::cout << "home.status: " << home_status << "\n";
    std::cout << "home.bottom: " << home_bottom << "\n";
    std::cout << "status: " << status << "\n";
    std::cout << "bottom: " << bottom << "\n";
    std::cout << "human.left.first: " << first_line(left) << "\n";
    std::cout << "human.right.first: " << first_line(right) << "\n";
    std::cout << "runtime.bottom: " << runtime_bottom << "\n";
    std::cout << "runtime.left.first: " << first_line(runtime_left) << "\n";
    std::cout << "runtime.right.first: " << first_line(runtime_right) << "\n";

    if (!ok)
      return 1;
    std::cout << "[ok] iinuji cmd marshal landing smoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_iinuji_cmd_landing] exception: " << e.what() << "\n";
    return 1;
  }
}
