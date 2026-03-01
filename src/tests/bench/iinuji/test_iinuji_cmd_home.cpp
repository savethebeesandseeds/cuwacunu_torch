#include <iostream>
#include <string>

#include "piaabo/dconfig.h"

#include "iinuji/iinuji_cmd/commands/iinuji.screen.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/home/view.h"
#include "iinuji/iinuji_cmd/views/ui.h"

namespace {

bool require(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

std::string first_line(const std::string& s) {
  const std::size_t pos = s.find('\n');
  return (pos == std::string::npos) ? s : s.substr(0, pos);
}

}  // namespace

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    cuwacunu::iinuji::iinuji_cmd::CmdState st{};
    st.config = cuwacunu::iinuji::iinuji_cmd::load_config_view_from_config();
    cuwacunu::iinuji::iinuji_cmd::clamp_selected_tab(st);

    st.board = cuwacunu::iinuji::iinuji_cmd::load_board_from_config();
    cuwacunu::iinuji::iinuji_cmd::clamp_selected_circuit(st);

    st.data = cuwacunu::iinuji::iinuji_cmd::load_data_view_from_config(&st.board);
    cuwacunu::iinuji::iinuji_cmd::clamp_selected_data_channel(st);
    cuwacunu::iinuji::iinuji_cmd::clamp_data_plot_mode(st);
    cuwacunu::iinuji::iinuji_cmd::clamp_data_plot_x_axis(st);
    cuwacunu::iinuji::iinuji_cmd::clamp_data_nav_focus(st);
    cuwacunu::iinuji::iinuji_cmd::clamp_selected_tsi_tab(st);

    cuwacunu::iinuji::iinuji_cmd::IinujiScreen{st}.home();

    const cuwacunu::iinuji::iinuji_cmd::IinujiUi ui{st};
    const std::string status = ui.status_line();
    const std::string left = cuwacunu::iinuji::iinuji_cmd::IinujiHomeView{st}.left();
    const std::string right = cuwacunu::iinuji::iinuji_cmd::IinujiHomeView::right();

    bool ok = true;
    ok = ok && require(status.find("[F1 HOME]") != std::string::npos, "status should highlight F1 HOME");
    ok = ok && require(status.find("[F2 BOARD]") == std::string::npos, "status should not highlight F2 on home");
    ok = ok && require(status.find("F3 TRAIN") != std::string::npos, "status should list F3 training");
    ok = ok && require(left.find("CUWACUNU command terminal") != std::string::npos,
                       "home left should include terminal heading");
    ok = ok && require(right.find("commands") != std::string::npos,
                       "home right should include commands heading");

    std::cout << "status: " << status << "\n";
    std::cout << "home.left.first: " << first_line(left) << "\n";
    std::cout << "home.right.first: " << first_line(right) << "\n";
    std::cout << "[round-home] NOTE(hashimyei): hex identity catalog active (0x0000..0x000f).\n";

    if (!ok) return 1;
    std::cout << "[ok] iinuji cmd home F1 smoke passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_iinuji_cmd_home] exception: " << e.what() << "\n";
    return 1;
  }
}
