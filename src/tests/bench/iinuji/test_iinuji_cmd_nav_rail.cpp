#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/app.h"

namespace {

bool require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    return false;
  }
  return true;
}

bool contains_non_ascii(const std::string &text) {
  for (unsigned char ch : text) {
    if (ch >= 0x80u)
      return true;
  }
  return false;
}

bool approx(double actual, double expected) {
  return std::abs(actual - expected) < 0.000001;
}

bool require_frac_len(const cuwacunu::iinuji::len_spec &spec, double expected,
                      const std::string &message) {
  return require(spec.u == cuwacunu::iinuji::unit_t::Frac &&
                     approx(spec.v, expected),
                 message);
}

bool require_px_len(const cuwacunu::iinuji::len_spec &spec, int expected,
                    const std::string &message) {
  return require(spec.u == cuwacunu::iinuji::unit_t::Px &&
                     approx(spec.v, static_cast<double>(expected)),
                 message);
}

std::optional<int>
menu_command_row_index(const cuwacunu::iinuji::iinuji_cmd::CmdState &state,
                       std::string_view command) {
  const auto columns =
      cuwacunu::iinuji::iinuji_cmd::make_menu_overlay_columns(state);
  const std::string wanted{command};
  for (std::size_t i = 0; i < columns.commands.size(); ++i) {
    std::string text =
        cuwacunu::iinuji::iinuji_cmd::trim(columns.commands[i].text);
    if (text == wanted) {
      return static_cast<int>(i);
    }
    if (!text.empty() && text.back() == '~') {
      text.pop_back();
      if (!text.empty() && wanted.rfind(text, 0) == 0)
        return static_cast<int>(i);
    }
  }
  for (std::size_t i = 0; i < columns.commands.size(); ++i) {
    const std::string text =
        cuwacunu::iinuji::iinuji_cmd::trim(columns.commands[i].text);
    if (text.find(command) != std::string::npos) {
      return static_cast<int>(i);
    }
  }
  return std::nullopt;
}

std::optional<int> menu_current_screen_row_index(
    const cuwacunu::iinuji::iinuji_cmd::CmdState &state,
    std::string_view command) {
  const auto first =
      cuwacunu::iinuji::iinuji_cmd::menu_current_screen_first_row(state);
  if (!first.has_value())
    return std::nullopt;

  const auto columns =
      cuwacunu::iinuji::iinuji_cmd::make_menu_overlay_columns(state);
  const std::string wanted{command};
  for (std::size_t i = static_cast<std::size_t>(*first);
       i < columns.commands.size(); ++i) {
    const auto scoped_text =
        cuwacunu::iinuji::iinuji_cmd::menu_current_screen_command_text_at(
            state, static_cast<int>(i));
    if (!scoped_text.has_value())
      break;
    std::string text = *scoped_text;
    if (text == wanted)
      return static_cast<int>(i);
    if (!text.empty() && text.back() == '~') {
      text.pop_back();
      if (!text.empty() && wanted.rfind(text, 0) == 0)
        return static_cast<int>(i);
    }
  }
  return std::nullopt;
}

} // namespace

int main() {
  using namespace cuwacunu::iinuji;
  using namespace cuwacunu::iinuji::iinuji_cmd;

  bool ok = true;
  CmdState state{};
  CmdUi ui{};
  ActionRegistry actions = create_default_actions();

  const std::filesystem::path gui_config_path{
      "/tmp/cuwacunu_cmd_gui_assets.config"};
  {
    std::ofstream config{gui_config_path};
    config << "[GUI]\n";
    config << "iinuji_logs_buffer_capacity = 123\n";
    config << "iinuji_logs_show_date = true\n";
    config << "iinuji_logs_show_thread = yes\n";
    config << "iinuji_logs_show_metadata = false\n";
    config << "iinuji_logs_metadata_filter = path\n";
    config << "iinuji_logs_show_color = off\n";
    config << "iinuji_logs_auto_follow = no\n";
    config << "iinuji_logs_mouse_capture = 0\n";
    config << "iinuji_loading_logo_path = relative/loading.png\n";
    config << "iinuji_closing_logo_path = /tmp/closing.png\n";
    config << "iinuji_home_animation_path = \"relative/home.apng\"\n";
  }
  const CmdUiAssetPaths asset_paths =
      resolve_cmd_ui_asset_paths(gui_config_path);
  ok = require(asset_paths.loading_logo ==
                   gui_config_path.parent_path() / "relative/loading.png",
               "GUI loading logo path should resolve relative to config") &&
       ok;
  ok = require(asset_paths.closing_logo ==
                   std::filesystem::path{"/tmp/closing.png"},
               "GUI closing logo path should support absolute paths") &&
       ok;
  ok = require(asset_paths.home_animation ==
                   gui_config_path.parent_path() / "relative/home.apng",
               "GUI home animation path should strip quotes and resolve") &&
       ok;
  {
    const CmdUiAssetPaths default_asset_paths =
        resolve_cmd_ui_asset_paths(state.global_config_path);
    ok = require(default_asset_paths.loading_logo.filename() ==
                     "waajacamaya_hello.png",
                 "default GUI loading logo should restore the legacy bundled "
                 "hello image") &&
         ok;
    ok = require(default_asset_paths.closing_logo.filename() ==
                     "waajacamaya.png",
                 "default GUI closing logo should restore the legacy bundled "
                 "farewell image") &&
         ok;
  }
  {
    const std::string loading_logo_path{
        "/cuwacunu/src/resources/waajacamaya_hello.png"};
    const std::string missing_animation_path{
        "/tmp/cuwacunu_cmd_missing_home.apng"};
    const HomeShowcaseAssets static_assets =
        load_home_showcase_assets(loading_logo_path, missing_animation_path);
    const rgba_image_t static_showcase =
        make_static_home_showcase_image(static_assets);
    ok = require(static_assets.loading_logo_ok &&
                     !static_assets.home_animation_ok,
                 "static Home fallback fixture should load the logo while "
                 "missing animation") &&
         ok;
    ok = require(static_showcase.valid() &&
                     static_showcase.height >
                         static_assets.loading_logo_image.height,
                 "static Home fallback should preserve the legacy Cuwacunu "
                 "wordmark area below the loading logo") &&
         ok;

    const std::filesystem::path static_config_path{
        "/tmp/cuwacunu_cmd_static_home_visual.config"};
    {
      std::ofstream config{static_config_path};
      config << "[GUI]\n";
      config << "iinuji_loading_logo_path = " << loading_logo_path << "\n";
      config << "iinuji_closing_logo_path = "
             << "/cuwacunu/src/resources/waajacamaya.png\n";
      config << "iinuji_home_animation_path = " << missing_animation_path
             << "\n";
    }
    const std::string static_visual_snapshot =
        make_home_visual_snapshot_text(static_config_path);
    ok = require(static_visual_snapshot.find(
                     "status: static loading logo + wordmark") !=
                     std::string::npos,
                 "Home visual snapshot should report the composited static "
                 "logo fallback") &&
         ok;
    const std::string static_splash_snapshot = make_splash_snapshot_text(
        SplashSnapshotMode::Bootstrap, static_config_path);
    ok = require(static_splash_snapshot.find(
                     "source: loading logo + wordmark") != std::string::npos,
                 "bootstrap splash fallback should use the legacy composited "
                 "loading logo and wordmark") &&
         ok;
  }
  {
    const std::string loading_logo_path{
        "/cuwacunu/src/resources/waajacamaya_hello.png"};
    const std::string animation_path{
        "/cuwacunu/src/resources/waajacamaya.apng"};
    const std::string missing_closing_path{
        "/tmp/cuwacunu_cmd_missing_closing_logo.png"};
    const std::filesystem::path missing_closing_config_path{
        "/tmp/cuwacunu_cmd_missing_closing_logo.config"};
    {
      std::ofstream config{missing_closing_config_path};
      config << "[GUI]\n";
      config << "iinuji_loading_logo_path = " << loading_logo_path << "\n";
      config << "iinuji_closing_logo_path = " << missing_closing_path << "\n";
      config << "iinuji_home_animation_path = " << animation_path << "\n";
    }
    const HomeShowcaseAssets fallback_assets =
        load_home_showcase_assets(loading_logo_path, animation_path);
    const CmdUi missing_closing_ui = create_ui(missing_closing_config_path);
    ok = require(fallback_assets.loading_logo_ok &&
                     missing_closing_ui.home_animation.valid(),
                 "missing closing-logo fixture should load the legacy loading "
                 "logo and Home animation") &&
         ok;
    ok = require(missing_closing_ui.closing_static_image.valid() &&
                     missing_closing_ui.closing_static_image.width ==
                         fallback_assets.loading_logo_image.width &&
                     missing_closing_ui.closing_static_image.height ==
                         fallback_assets.loading_logo_image.height &&
                     missing_closing_ui.closing_static_image.pixels ==
                         fallback_assets.loading_logo_image.pixels,
                 "interactive farewell splash should fall back to the legacy "
                 "loading logo when the closing logo is unavailable") &&
         ok;
  }
  const CmdUiLogDefaults log_defaults =
      resolve_cmd_ui_log_defaults(gui_config_path);
  ok = require(log_defaults.buffer_capacity == 123u,
               "GUI logs buffer capacity should parse positive integers") &&
       ok;
  ok = require(log_defaults.show_timestamp && log_defaults.show_thread,
               "GUI logs bool defaults should parse true/yes forms") &&
       ok;
  ok = require(!log_defaults.show_metadata && !log_defaults.show_color &&
                   !log_defaults.auto_follow && !log_defaults.mouse_capture,
               "GUI logs bool defaults should parse false/off/no/0 forms") &&
       ok;
  ok = require(log_defaults.metadata_filter == LogsMetadataFilter::WithPath,
               "GUI logs metadata filter should parse path token") &&
       ok;
  apply_cmd_ui_log_defaults(state, gui_config_path);
  ok = require(state.logs.buffer_capacity == 123u &&
                   state.logs.metadata_filter == LogsMetadataFilter::WithPath,
               "GUI logs defaults should apply to command state") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find("capacity    123") !=
                   std::string::npos,
               "Shell Logs control deck should report configured capacity") &&
       ok;
  const std::string section_rule = detail_section_rule();
  ok = require(make_logs_control_deck_text(state).find(
                   section_rule + "\nLens\n" + section_rule +
                   "\n  source      any") != std::string::npos,
               "Shell Logs control deck should restore the legacy Lens "
               "summary block with section framing") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   "behavior    follow off | mouse select/copy") !=
                   std::string::npos,
               "Shell Logs Lens summary should include behavior state") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   section_rule + "\nSettings\n" + section_rule +
                   "\n  Up/Down select | Left/Right change") !=
                   std::string::npos,
               "Shell Logs control deck should restore the legacy Settings "
               "block with section framing") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find("> source      [any]") !=
                   std::string::npos,
               "Shell Logs Settings block should show the selected row as a "
               "bracketed value") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   "f / Esc     full screen / split") != std::string::npos,
               "Shell Logs control deck should preserve the legacy Esc split "
               "hint") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find("Selected\n") !=
                   std::string::npos,
               "Shell Logs control deck should restore the selected-setting "
               "guidance block") &&
       ok;
  {
    const auto styled_logs_deck =
        style_content_text(make_logs_control_deck_text(state));
    bool settings_heading_is_accent = false;
    bool selected_heading_is_accent = false;
    for (const auto &line : styled_logs_deck) {
      if (line.text == "Settings" &&
          line.emphasis == text_line_emphasis_t::Accent)
        settings_heading_is_accent = true;
      if (line.text == "Selected" &&
          line.emphasis == text_line_emphasis_t::Accent)
        selected_heading_is_accent = true;
    }
    ok =
        require(settings_heading_is_accent,
                "Shell Logs Settings heading should receive section styling") &&
        ok;
    ok =
        require(selected_heading_is_accent,
                "Shell Logs Selected heading should receive section styling") &&
        ok;
  }
  state.logs.selected_setting = 2u;
  ok = require(make_logs_control_deck_text(state).find(
                   "> meta        [PATH+]") != std::string::npos,
               "Shell Logs Settings block should move the cursor with the "
               "active lens row") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   "meta filters entries by metadata payload "
                   "availability.") != std::string::npos,
               "Shell Logs selected-setting guidance should follow the active "
               "lens row") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   "wheel       scroll panels") != std::string::npos,
               "Shell Logs control deck should restore the legacy wheel "
               "utility hint") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find(
                   "tuned for live shell diagnostics") != std::string::npos &&
                   make_logs_control_deck_text(state).find("operator triage") ==
                       std::string::npos,
               "Shell Logs control deck should describe diagnostics without "
               "operator-era copy") &&
       ok;
  state.logs.selected_setting = 0u;
  state.logs.buffer_capacity = 2u;
  append_log(state, "command", "one");
  append_log(state, "command", "two");
  append_log(state, "command", "three");
  ok = require(state.log.size() == 2u && state.log.front().message == "two",
               "Shell Logs should retain only the configured capacity") &&
       ok;
  state.log.clear();
  state.logs = LogsState{};

  {
    CmdUi layout_ui{};
    layout_ui.workspace = create_grid_container(
        "layout.workspace", {len_spec::frac(1.0)}, {len_spec::frac(1.0)}, 0, 0);
    layout_ui.split_panel = create_grid_container(
        "layout.split", {len_spec::frac(1.0)}, {len_spec::frac(1.0)}, 0, 0);
    layout_ui.main_panel = create_panel("layout.main");
    layout_ui.detail_panel = create_panel("layout.detail");
    CmdState layout_state{};

    layout_state.screen = ScreenMode::Home;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require(layout_ui.workspace->grid->cols.size() == 2u,
                 "workspace layout should keep two columns") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[0], 0.42,
                          "Home workspace should restore the legacy 42/58 "
                          "visual split") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[1], 0.58,
                          "Home detail column should restore the legacy "
                          "42/58 visual split") &&
         ok;
    CmdUi home_layout_ui = create_ui(gui_config_path);
    ok = require(home_layout_ui.home_image != nullptr,
                 "Home should create the restored image primitive") &&
         ok;
    ok = require(home_layout_ui.title &&
                     home_layout_ui.title->style.title == " cuwacunu_cmd ",
                 "shell frame title should prefer the PATH command spelling") &&
         ok;
    {
      const rgba_image_t splash_probe = make_fallback_home_wordmark_image();
      SplashUi splash_layout_ui = create_splash_ui(splash_probe, " probe ");
      ok = require(splash_layout_ui.title &&
                       splash_layout_ui.title->style.title == " cuwacunu_cmd ",
                   "splash frame title should prefer the PATH command "
                   "spelling") &&
           ok;
    }
    ok = require(home_layout_ui.root && home_layout_ui.root->grid &&
                     home_layout_ui.root->grid->rows.size() >= 4u,
                 "shell root should expose the restored global row plan") &&
         ok;
    if (home_layout_ui.root && home_layout_ui.root->grid &&
        home_layout_ui.root->grid->rows.size() >= 4u) {
      ok = require_px_len(home_layout_ui.root->grid->rows[1], 0,
                          "legacy tab rail should live in the title frame, "
                          "not in a separate visible row") &&
           ok;
      ok = require_px_len(home_layout_ui.root->grid->rows[3], 3,
                          "legacy bottom status row should reserve one framed "
                          "hint line") &&
           ok;
    }
    ok = require_frac_len(home_layout_ui.home_image->layout.dock_size,
                          kHomeImageDockFraction,
                          "Home image stage should restore the legacy 56% "
                          "waajacamaya area") &&
         ok;
    update_ui(home_layout_ui, layout_state, actions);
    const auto title_data = text_data(home_layout_ui.title);
    ok = require(home_layout_ui.title &&
                     home_layout_ui.title->style.title == " cuwacunu_cmd ",
                 "live title frame should stay aligned with the PATH command "
                 "spelling") &&
         ok;
    ok = require(title_data &&
                     title_data->content == make_tab_text(layout_state),
                 "title frame should render the legacy F-key tab rail") &&
         ok;
    ok = require(title_data &&
                     title_data->content.find("pulse[") == std::string::npos &&
                     title_data->content.find("cuwacunu.cmd |") ==
                         std::string::npos,
                 "title frame should not duplicate the newer operator status "
                 "header") &&
         ok;
    layout_state.command_name = "cuwacunu.cmd";
    update_ui(home_layout_ui, layout_state, actions);
    ok = require(home_layout_ui.title &&
                     home_layout_ui.title->style.title == " cuwacunu.cmd ",
                 "live title frame should follow the invoked command alias") &&
         ok;
    layout_state.command_name = "cuwacunu_cmd";
    update_ui(home_layout_ui, layout_state, actions);
    const auto bottom_data = text_data(home_layout_ui.status);
    ok = require(bottom_data && bottom_data->content ==
                                    make_operator_hint_line(layout_state),
                 "live bottom row should render the legacy single-line hint") &&
         ok;
    ok = require(bottom_data &&
                     bottom_data->content.find('\n') == std::string::npos,
                 "live bottom row should not reserve a second status line") &&
         ok;
    layout_state.help_view = true;
    update_ui(home_layout_ui, layout_state, actions);
    layout_tree(home_layout_ui.root, Rect{0, 0, 120, 40});
    ok = require(home_layout_ui.menu_overlay &&
                     home_layout_ui.menu_overlay->visible,
                 "menu overlay should become visible while help is open") &&
         ok;
    ok = require(home_layout_ui.menu_overlay &&
                     home_layout_ui.menu_overlay->layout.mode ==
                         layout_mode_t::GridCell &&
                     home_layout_ui.menu_overlay->layout.grid_row == 2,
                 "legacy menu overlay should occupy the workspace grid row") &&
         ok;
    ok = require(home_layout_ui.menu_overlay && home_layout_ui.workspace &&
                     home_layout_ui.menu_overlay->screen.y ==
                         home_layout_ui.workspace->screen.y &&
                     home_layout_ui.menu_overlay->screen.h ==
                         home_layout_ui.workspace->screen.h,
                 "menu overlay should cover the workspace without hiding the "
                 "title, bottom hint, or command strip") &&
         ok;
    ok = require(home_layout_ui.menu_overlay && home_layout_ui.workspace &&
                     home_layout_ui.menu_overlay->z_index >
                         home_layout_ui.workspace->z_index,
                 "workspace menu overlay should remain above the screen "
                 "panels") &&
         ok;
    layout_state.help_view = false;
    layout_state.screen = ScreenMode::Runtime;
    layout_state.runtime.focus = RuntimeFocus::Devices;
    ok = require(open_runtime_selected_menu(layout_state),
                 "Runtime selected item menu should open from layout test") &&
         ok;
    update_ui(home_layout_ui, layout_state, actions);
    layout_tree(home_layout_ui.root, Rect{0, 0, 120, 40});
    ok = require(home_layout_ui.choice_overlay &&
                     home_layout_ui.choice_overlay->visible &&
                     home_layout_ui.choice_shadow &&
                     home_layout_ui.choice_shadow->visible &&
                     home_layout_ui.menu_overlay &&
                     !home_layout_ui.menu_overlay->visible,
                 "Runtime selected item menu should use the compact choice "
                 "overlay and shadow instead of the workspace menu overlay") &&
         ok;
    ok =
        require(home_layout_ui.choice_shadow && home_layout_ui.choice_overlay &&
                    home_layout_ui.choice_shadow->screen.x >
                        home_layout_ui.choice_overlay->screen.x &&
                    home_layout_ui.choice_shadow->screen.y >
                        home_layout_ui.choice_overlay->screen.y,
                "Runtime selected item menu shadow should sit on the popup "
                "edge") &&
        ok;
    ok = require(home_layout_ui.choice_overlay &&
                     home_layout_ui.choice_overlay->layout.mode ==
                         layout_mode_t::Normalized,
                 "Runtime selected item menu should be a centered normalized "
                 "popup") &&
         ok;
    ok = require(home_layout_ui.choice_overlay && home_layout_ui.workspace &&
                     home_layout_ui.choice_overlay->screen.w <
                         home_layout_ui.workspace->screen.w &&
                     home_layout_ui.choice_overlay->screen.h <
                         home_layout_ui.workspace->screen.h,
                 "Runtime selected item menu should stay smaller than the "
                 "workspace") &&
         ok;
    ok =
        require(home_layout_ui.choice_overlay && home_layout_ui.workspace &&
                    std::abs((home_layout_ui.choice_overlay->screen.x +
                              home_layout_ui.choice_overlay->screen.w / 2) -
                             (home_layout_ui.workspace->screen.x +
                              home_layout_ui.workspace->screen.w / 2)) <= 2,
                "Runtime selected item menu should be centered horizontally") &&
        ok;
    close_choice_menu(layout_state);
    open_content_popup(layout_state, CmdContentPopupKind::RuntimePolicy);
    update_ui(home_layout_ui, layout_state, actions);
    layout_tree(home_layout_ui.root, Rect{0, 0, 120, 40});
    ok = require(home_layout_ui.popup_overlay &&
                     home_layout_ui.popup_overlay->visible &&
                     home_layout_ui.popup_shadow &&
                     home_layout_ui.popup_shadow->visible,
                 "Runtime content popup should use a centered overlay with a "
                 "shadow") &&
         ok;
    close_content_popup(layout_state);
    layout_state.runtime.focus = RuntimeFocus::Devices;
    update_ui(home_layout_ui, layout_state, actions);
    ok = require(
             home_layout_ui.detail_panel &&
                 home_layout_ui.detail_panel->style.title.find("telemetry") !=
                     std::string::npos,
             "Runtime detail panel should be titled telemetry for devices") &&
         ok;
    layout_state.runtime.focus = RuntimeFocus::Jobs;
    update_ui(home_layout_ui, layout_state, actions);
    ok = require(home_layout_ui.detail_panel &&
                     home_layout_ui.detail_panel->style.title.find("details") !=
                         std::string::npos,
                 "Runtime detail panel should be titled details for jobs") &&
         ok;

    layout_state.screen = ScreenMode::Logs;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_frac_len(layout_ui.workspace->grid->cols[0], 0.68,
                          "Shell Logs should restore the wide stream column") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[1], 0.32,
                          "Shell Logs controls should use the legacy narrow "
                          "right column") &&
         ok;

    layout_state.screen = ScreenMode::Config;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_frac_len(layout_ui.workspace->grid->cols[0], 0.60,
                          "Config should restore the legacy catalog/detail "
                          "split") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[1], 0.40,
                          "Config detail should restore the legacy "
                          "catalog/detail split") &&
         ok;
    ok = require_px_len(layout_ui.split_panel->grid->rows[0], 7,
                        "Config split navigator should restore the compact "
                        "legacy height") &&
         ok;
    layout_state.config.file_editor_open = true;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_px_len(
             layout_ui.workspace->grid->cols[0], 0,
             "Config file editor should use the full workspace utility area") &&
         ok;
    ok = require_frac_len(
             layout_ui.workspace->grid->cols[1], 1.0,
             "Config file editor should expand across the workspace") &&
         ok;
    ok = require(layout_ui.split_panel && !layout_ui.split_panel->visible &&
                     layout_ui.detail_panel && layout_ui.detail_panel->visible,
                 "Config file editor should hide the catalog panes while "
                 "keeping the editor panel visible") &&
         ok;
    layout_state.config.file_editor_open = false;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require(workspace_toggle_current_screen_zoom(layout_state),
                 "Config should support the restored full/split workspace "
                 "utility") &&
         ok;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_px_len(layout_ui.workspace->grid->cols[0], 0,
                        "Config full view should hide the catalog column") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[1], 1.0,
                          "Config full view should expand the detail panel") &&
         ok;
    ok = require(workspace_restore_current_screen_split(layout_state),
                 "Config split restore should clear the full-view flag") &&
         ok;

    layout_state.screen = ScreenMode::Lattice;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_px_len(layout_ui.split_panel->grid->rows[0], 7,
                        "Lattice split navigator should restore the compact "
                        "legacy height") &&
         ok;
    ok = require(workspace_toggle_current_screen_zoom(layout_state),
                 "Lattice should support the restored full/split workspace "
                 "utility") &&
         ok;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_px_len(layout_ui.workspace->grid->cols[0], 0,
                        "Lattice full view should hide the target list "
                        "column") &&
         ok;
    ok = require_frac_len(layout_ui.workspace->grid->cols[1], 1.0,
                          "Lattice full view should expand the detail panel") &&
         ok;
    ok = require(workspace_restore_current_screen_split(layout_state),
                 "Lattice split restore should clear the full-view flag") &&
         ok;

    layout_state.screen = ScreenMode::Runtime;
    update_workspace_zoom(layout_ui, layout_state);
    ok = require_frac_len(layout_ui.workspace->grid->cols[0], 0.44,
                          "Runtime should keep the restored default split") &&
         ok;
    ok = require_px_len(layout_ui.split_panel->grid->rows[0], 15,
                        "Runtime navigator should reserve room for the moved "
                        "menu section") &&
         ok;
  }

  ok = require(function_key_from_csi_number(11) == KEY_F(1),
               "CSI 11~ should decode to F1") &&
       ok;
  ok = require(function_key_from_csi_number(12) == KEY_F(2),
               "CSI 12~ should decode to F2") &&
       ok;
  ok = require(function_key_from_csi_number(13) == KEY_F(3),
               "CSI 13~ should decode to F3") &&
       ok;
  ok = require(function_key_from_csi_number(14) == KEY_F(4),
               "CSI 14~ should decode to F4") &&
       ok;
  ok = require(function_key_from_csi_number(19) == KEY_F(8),
               "CSI 19~ should decode to F8") &&
       ok;
  ok = require(function_key_from_csi_number(20) == KEY_F(9),
               "CSI 20~ should decode to F9") &&
       ok;
  ok = require(function_key_from_linux_console_suffix('A') == KEY_F(1),
               "Linux console [[A should decode to F1") &&
       ok;
  ok = require(function_key_from_linux_console_suffix('D') == KEY_F(4),
               "Linux console [[D should decode to F4") &&
       ok;
  ok = require(function_key_from_linux_console_suffix('x') == key_escape,
               "unknown Linux console F-key suffix should be ignored") &&
       ok;

  state.screen = ScreenMode::Runtime;
  state.status = "Runtime inventory partially loaded.";
  mirror_operator_status_to_logs(state);
  ok = require(state.log.size() == 1u && state.log.back().source == "status",
               "operator status should mirror into Shell Logs status source") &&
       ok;
  mirror_operator_status_to_logs(state);
  ok = require(state.log.size() == 1u,
               "operator status mirror should dedupe unchanged statuses") &&
       ok;
  ok = require(logs_source_filter_label(LogsSourceFilter::Status) == "status",
               "Shell Logs should expose a status source lens") &&
       ok;
  ok = require(logs_source_filter_label(LogsSourceFilter::Show) == "show",
               "Shell Logs should expose a show source lens") &&
       ok;
  state.logs.source_filter = LogsSourceFilter::Status;
  state.logs.severity_filter = LogsSeverityFilter::DebugOrHigher;
  ok = require(logs_accept_entry(state, state.log.back()),
               "status source lens should accept mirrored status entries") &&
       ok;
  const LogEntry action_entry{0, "action", "runtime row selected"};
  ok = require(log_entry_severity(action_entry) ==
                   LogsSeverityFilter::DebugOrHigher,
               "action events should be classified as DEBUG+ noise by "
               "default") &&
       ok;
  state.logs.source_filter = LogsSourceFilter::All;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  ok = require(!logs_accept_entry(state, action_entry),
               "INFO+ Shell Logs should hide ordinary action events") &&
       ok;
  ok = require(log_entry_severity(LogEntry{0, "action", "error opening"}) ==
                   LogsSeverityFilter::ErrorOrHigher,
               "action errors should still rise above DEBUG+") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find("status      1") !=
                   std::string::npos,
               "Shell Logs control deck should count status entries") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.settings.source.status()", state),
               "direct status source command should dispatch") &&
       ok;
  ok = require(state.logs.source_filter == LogsSourceFilter::Status,
               "direct status source command should select status lens") &&
       ok;
  ok = require(state.status == "logs.source=status",
               "direct status source command should use legacy dotted status "
               "cadence") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.settings.source.show()", state),
               "direct show source command should dispatch") &&
       ok;
  ok = require(state.logs.source_filter == LogsSourceFilter::Show,
               "direct show source command should select show lens") &&
       ok;
  ok = require(state.status == "logs.source=show",
               "direct show source command should use legacy dotted status "
               "cadence") &&
       ok;
  state.logs.show_timestamp = false;
  ok = require(actions.dispatch("iinuji.logs.settings.date.toggle()", state),
               "direct date toggle command should dispatch") &&
       ok;
  ok = require(state.status == "logs.date=on",
               "date toggle should restore the legacy status spelling") &&
       ok;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  ok = require(actions.dispatch("iinuji.logs.settings.level.error()", state),
               "direct logs error level command should dispatch") &&
       ok;
  ok = require(state.status == "logs.level=ERROR+",
               "level command should restore the legacy dotted status "
               "spelling") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.settings.metadata.filter.path()",
                                state),
               "direct logs metadata path command should dispatch") &&
       ok;
  ok = require(state.status == "logs.metadata_filter=PATH+",
               "metadata filter command should restore the legacy dotted "
               "status spelling") &&
       ok;
  ok = require(actions.dispatch(
                   "iinuji.logs.settings.metadata.filter.any_meta()", state),
               "direct logs metadata any_meta command should dispatch") &&
       ok;
  ok = require(state.logs.metadata_filter ==
                       LogsMetadataFilter::WithAnyMetadata &&
                   state.status == "logs.metadata_filter=META+",
               "direct any_meta command should select the metadata lens") &&
       ok;
  ok = require(actions.dispatch(
                   "iinuji.logs.settings.metadata.filter.callsite()", state),
               "direct logs metadata callsite command should dispatch") &&
       ok;
  ok = require(state.logs.metadata_filter == LogsMetadataFilter::WithCallsite &&
                   state.status == "logs.metadata_filter=CALLSITE+",
               "direct callsite command should select the metadata lens") &&
       ok;
  state.runtime.focus = RuntimeFocus::Devices;
  ok = require(actions.dispatch("iinuji.runtime.row.prev()", state),
               "runtime row previous command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.focus == RuntimeFocus::Jobs &&
                   state.status == "runtime lane=Jobs",
               "runtime row previous should wrap from Devices to Jobs") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.row.next()", state),
               "runtime row next command should dispatch") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                   state.status == "runtime lane=Devices",
               "runtime row next should wrap from Jobs to Devices") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.row.next()", state),
               "runtime row next command should dispatch again") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                   state.status == "runtime lane=Jobs",
               "runtime row next should select Jobs with legacy lane status") &&
       ok;
  state.runtime.focus = RuntimeFocus::Devices;
  ok = require(actions.dispatch("runtime right", state),
               "natural runtime right command should dispatch") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                   state.status == "runtime lane=Jobs",
               "natural runtime right command should select Jobs lane") &&
       ok;
  ok = require(actions.dispatch("runtime left", state),
               "natural runtime left command should dispatch") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                   state.status == "runtime lane=Devices",
               "natural runtime left command should select Devices lane") &&
       ok;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  ok = require(standard_key_action(key_shift_tab) == key_action_t::Tab,
               "Shift-Tab should normalize to the legacy Runtime detail Tab "
               "action") &&
       ok;
  ok = require(translate_ncurses_key(KEY_BTAB) == key_shift_tab,
               "ncurses KEY_BTAB should reach the shared Shift-Tab key "
               "primitive") &&
       ok;
  ok = require(key_from_csi_final('Z') == key_shift_tab,
               "raw CSI Z should preserve the terminal Shift-Tab escape "
               "sequence") &&
       ok;
  ok = require(
           kEscapeSequenceTimeoutMs > 0 && kEscapeSequenceTimeoutMs <= 25 &&
               kEscapeSequenceTimeoutMs <
                   desired_input_timeout_for_screen(ScreenMode::Runtime),
           "plain Esc handling should use a short escape-sequence timeout") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.detail.next()", state),
               "runtime detail next command should dispatch") &&
       ok;
  ok = require(state.runtime.detail_mode == RuntimeDetailMode::Log &&
                   state.status == "runtime detail=log",
               "runtime detail next should restore compact detail status") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.detail.trace()", state),
               "runtime trace detail command should dispatch") &&
       ok;
  ok = require(state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                   state.status == "runtime detail=trace",
               "runtime trace detail should use compact detail status") &&
       ok;
  state.runtime.focus = RuntimeFocus::Devices;
  ok = require(actions.dispatch("runtime left", state),
               "runtime left command should dispatch") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                   state.status == "runtime lane=Jobs",
               "runtime left should wrap from Devices to Jobs") &&
       ok;
  ok = require(actions.dispatch("runtime right", state),
               "runtime right command should dispatch") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                   state.status == "runtime lane=Devices",
               "runtime right should wrap from Jobs to Devices") &&
       ok;
  state.runtime.focus = RuntimeFocus::Devices;
  state.runtime.device.gpus = {
      RuntimeGpuSnapshot{.index = "0", .name = "gpu.alpha"},
  };
  state.runtime.selected_device_row = 0;
  ok = require(actions.dispatch("iinuji.runtime.item.next()", state),
               "runtime device item next command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_device_row == 1u &&
                   state.status == "runtime device=next",
               "runtime device item next should use legacy item status") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.item.prev()", state),
               "runtime device item previous command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_device_row == 0u &&
                   state.status == "runtime device=prev",
               "runtime device item previous should use legacy item status") &&
       ok;
  ok = require(actions.dispatch("runtime down", state),
               "natural runtime down command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_device_row == 1u &&
                   state.status == "runtime device=next",
               "natural runtime down command should select the next device "
               "row") &&
       ok;
  ok = require(actions.dispatch("runtime up", state),
               "natural runtime up command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_device_row == 0u &&
                   state.status == "runtime device=prev",
               "natural runtime up command should select the previous device "
               "row") &&
       ok;
  ok = require(actions.dispatch("runtime up", state),
               "runtime device previous wrap command should dispatch") &&
       ok;
  ok = require(
           state.runtime.selected_device_row == 1u &&
               state.status == "runtime device=prev",
           "runtime device previous should wrap from host to the last GPU") &&
       ok;
  ok = require(actions.dispatch("runtime down", state),
               "runtime device next wrap command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_device_row == 0u &&
                   state.status == "runtime device=next",
               "runtime device next should wrap from the last GPU to host") &&
       ok;
  state.runtime.focus = RuntimeFocus::Jobs;
  state.runtime.jobs = {
      RuntimeJobSummary{.job_id = "job.alpha", .status = "ready"},
      RuntimeJobSummary{.job_id = "job.beta", .status = "running"},
  };
  state.runtime.selected_job = 0;
  ok = require(actions.dispatch("iinuji.runtime.item.next()", state),
               "runtime job item next command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_job == 1u &&
                   state.status == "runtime job=next",
               "runtime job item next should use legacy item status") &&
       ok;
  ok = require(actions.dispatch("iinuji.runtime.item.next()", state),
               "runtime job item next wrap command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_job == 0u &&
                   state.status == "runtime job=next",
               "runtime job item next should wrap from the last job to the "
               "first job") &&
       ok;
  ok = require(actions.dispatch("job up", state),
               "natural job up command should dispatch") &&
       ok;
  ok = require(state.runtime.selected_job == 1u &&
                   state.status == "runtime job=prev",
               "runtime job previous should wrap from the first job to the "
               "last job") &&
       ok;
  ok = require(actions.dispatch("files", state),
               "legacy files alias should dispatch to Config catalog") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.status == "config files",
               "files alias should switch to Config catalog") &&
       ok;
  ok = require(actions.dispatch("iinuji.config.file.show()", state),
               "Config file show path should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.status == "show config",
               "Config file show path should show selected Config summary") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("config show", state),
               "natural Config show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.status == "show config" && !state.log.empty() &&
                   state.log.front().message == "screen=config",
               "natural Config show alias should run the selected Config "
               "summary utility") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  state.home_visual.loading_logo_path = "/tmp/loading.png";
  state.home_visual.closing_logo_path = "/tmp/closing.png";
  state.home_visual.home_animation_path = "/tmp/home.apng";
  state.home_visual.animation_frames = 127u;
  state.home_visual.animation_dynamic = true;
  ok = require(actions.dispatch("cuwacunu.cmd --visual", state),
               "primary shell Home visual command should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home && state.status == "home visual" &&
               state.home_visual.presentation == HomePresentationMode::Showcase,
           "primary shell Home visual command should open the visual "
           "showcase") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --visual", state),
               "compatibility shell Home visual command should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home && state.status == "home visual" &&
               state.home_visual.presentation == HomePresentationMode::Showcase,
           "CLI-style Home visual command should open the visual "
           "showcase") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --home-visual", state),
               "CLI-style Home visual long option should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home && state.status == "home visual" &&
               state.home_visual.presentation == HomePresentationMode::Showcase,
           "CLI-style Home visual long option should open the visual "
           "showcase") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("home-visual", state),
               "positional Home visual utility alias should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home && state.status == "home visual" &&
               state.home_visual.presentation == HomePresentationMode::Showcase,
           "positional Home visual utility alias should open the visual "
           "showcase") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --animation", state),
               "CLI-style Home animation command should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home && state.status == "home visual" &&
               state.home_visual.presentation == HomePresentationMode::Showcase,
           "CLI-style Home animation command should open the visual "
           "showcase") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("splash", state),
               "Home splash utility alias should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home &&
               state.status == "bootstrap splash" && state.log.size() >= 5u &&
               state.home_visual.presentation ==
                   HomePresentationMode::BootstrapSplash &&
               state.log.front().message == "splash=bootstrap" &&
               state.log[4].message == "animation_frames=127 dynamic=yes",
           "Home splash utility should expose bootstrap visual diagnostics") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --splash", state),
               "primary shell Home splash command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.log.size() >= 5u &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   state.log.front().message == "splash=bootstrap",
               "primary shell Home splash command should expose bootstrap "
               "diagnostics") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --splash", state),
               "compatibility shell Home splash command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.log.size() >= 5u &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   state.log.front().message == "splash=bootstrap",
               "CLI-style Home splash command should expose bootstrap "
               "diagnostics") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --bootstrap", state),
               "CLI-style bootstrap command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "CLI-style bootstrap command should use the bootstrap splash "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --boot", state),
               "CLI-style boot command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "CLI-style boot command should use the bootstrap splash "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --splash=boot", state),
               "primary shell splash=boot command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "primary shell splash=boot command should use the bootstrap "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd --splash loading", state),
               "legacy iinuji shell splash loading command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "legacy iinuji shell splash loading command should use the "
               "bootstrap presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("boot", state),
               "positional boot splash alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "positional boot splash alias should use the bootstrap splash "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("iinuji.home.farewell()", state),
               "Home farewell canonical utility should dispatch") &&
       ok;
  ok = require(
           state.screen == ScreenMode::Home &&
               state.status == "farewell splash" && state.log.size() >= 3u &&
               state.home_visual.presentation ==
                   HomePresentationMode::FarewellSplash &&
               state.log.front().message == "splash=farewell" &&
               state.log[2].message == "closing_logo=/tmp/closing.png",
           "Home farewell utility should expose closing splash diagnostics") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("good luck", state),
               "direct Good luck farewell utility alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "direct Good luck farewell utility alias should use the "
               "farewell splash presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --splash=farewell", state),
               "primary shell Home farewell splash command should dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home &&
                  state.status == "farewell splash" && state.log.size() >= 3u &&
                  state.home_visual.presentation ==
                      HomePresentationMode::FarewellSplash &&
                  state.log.front().message == "splash=farewell",
              "primary shell Home farewell splash command should expose "
              "closing diagnostics") &&
      ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --splash=farewell", state),
               "compatibility shell Home farewell splash command should "
               "dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home &&
                  state.status == "farewell splash" && state.log.size() >= 3u &&
                  state.home_visual.presentation ==
                      HomePresentationMode::FarewellSplash &&
                  state.log.front().message == "splash=farewell",
              "CLI-style Home farewell splash command should expose closing "
              "diagnostics") &&
      ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --closing", state),
               "CLI-style closing command should dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home &&
                  state.status == "farewell splash" && state.log.size() >= 3u &&
                  state.home_visual.presentation ==
                      HomePresentationMode::FarewellSplash &&
                  state.log.front().message == "splash=farewell",
              "CLI-style closing command should expose closing diagnostics") &&
      ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(
           actions.dispatch("cuwacunu_cmd --splash farewell", state),
           "spaced CLI-style Home farewell splash command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "spaced CLI-style Home farewell splash command should expose "
               "closing diagnostics") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --splash close", state),
               "primary shell splash close command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "primary shell splash close command should use the farewell "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok =
      require(actions.dispatch("iinuji_cmd --splash=good luck", state),
              "legacy iinuji shell splash good luck command should dispatch") &&
      ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "legacy iinuji shell splash good luck command should use the "
               "farewell presentation") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --splash=good-luck", state),
               "shell splash good-luck command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "shell splash good-luck command should use the farewell "
               "presentation") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("runtime show", state),
               "reverse Runtime show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "show runtime" && !state.log.empty() &&
                   state.log.front().message == "screen=runtime",
               "reverse Runtime show alias should run the Runtime summary") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("workbench show", state),
               "reverse Workbench show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "show workbench" && !state.log.empty() &&
                   state.log.front().message == "screen=workbench",
               "reverse Workbench show alias should run the blank Workbench "
               "summary") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("logs show", state),
               "reverse Shell Logs show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.status == "show shell logs" && !state.log.empty() &&
                   state.log.front().message == "screen=shell_logs",
               "reverse Shell Logs show alias should run the Shell Logs "
               "summary") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  state.lattice.target_ids = {"target.alpha", "target.beta"};
  state.lattice.selected_target = 1u;
  ok = require(actions.dispatch("show target", state),
               "natural Lattice target show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "show lattice" && !state.log.empty() &&
                   state.log.front().message == "screen=lattice",
               "natural Lattice target show alias should run the selected "
               "target proof utility") &&
       ok;
  ok = require(state.log.size() >= 4u &&
                   state.log[3].message == "selected_target=target.beta",
               "natural Lattice target show alias should include the selected "
               "target proof") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("show proof", state),
               "natural Lattice proof show alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "show lattice" && !state.log.empty() &&
                   state.log.front().message == "screen=lattice",
               "natural Lattice proof show alias should run the Lattice "
               "summary utility") &&
       ok;
  state.log.clear();
  state.screen = ScreenMode::Logs;
  state.status = "logs cleared";
  mirror_operator_status_to_logs(state);
  ok = require(
           state.log.empty(),
           "Shell Logs screen should not re-add a status entry after clear") &&
       ok;
  state.logs = LogsState{};

  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("show", state),
               "show command should dispatch") &&
       ok;
  ok = require(state.status == "show home",
               "show command should update status with the shown screen") &&
       ok;
  ok = require(state.log.size() >= 4u,
               "show command should append structured show lines and command "
               "audit") &&
       ok;
  ok = require(state.log[0].source == "show" &&
                   state.log[0].message == "screen=home",
               "show command should restore legacy screen log output") &&
       ok;
  ok = require(state.log[1].source == "show" &&
                   state.log[1].message.find("F2 workbench") !=
                       std::string::npos,
               "show command should restore legacy screen hint output") &&
       ok;
  ok = require(state.log[2].source == "show" &&
                   state.log[2].message == "site=waajacu.com",
               "show command should restore the legacy site log output") &&
       ok;
  state.logs.source_filter = LogsSourceFilter::Show;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  ok = require(logs_accept_entry(state, state.log[0]),
               "show source lens should accept show entries at default info "
               "severity") &&
       ok;
  ok = require(make_logs_control_deck_text(state).find("show        3") !=
                   std::string::npos,
               "Shell Logs control deck should count show entries") &&
       ok;
  state.log.clear();
  state.logs = LogsState{};

  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("cuwacunu_cmd --run \"show runtime\"", state),
               "shell-style run command should dispatch from the command "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "show runtime" && !state.log.empty() &&
                   state.log.front().message == "screen=runtime",
               "shell-style run command should route through the nested "
               "iinuji command") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --run \"show runtime\"", state),
               "primary shell-style run command should dispatch from the "
               "command prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "show runtime" && !state.log.empty() &&
                   state.log.front().message == "screen=runtime",
               "primary shell-style run command should route through the "
               "nested iinuji command") &&
       ok;
  state.screen = ScreenMode::Home;
  state.help_view = false;
  state.config.files = {
      ConfigFileSummary{.path = "/cuwacunu/src/config/hero.config.dsl",
                        .relative_path = "hero.config.dsl"},
      ConfigFileSummary{.path = "/cuwacunu/src/config/hero.runtime.dsl",
                        .relative_path = "hero.runtime.dsl"},
  };
  state.log.clear();
  ok = require(
           actions.dispatch("cuwacunu.cmd src/config/hero.runtime.dsl "
                            "--snapshot",
                            state),
           "shell-style managed config path snapshot should dispatch from the "
           "command prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "shell-style managed config path snapshot should open Config "
               "on the matching managed file") &&
       ok;
  state.config.files.clear();
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(!actions.dispatch("cuwacunu.cmd --run \"not a command\"", state),
               "shell-style run command should propagate nested command "
               "failures") &&
       ok;
  ok =
      require(state.status ==
                      "unknown command: cuwacunu.cmd --run \"not a command\"" &&
                  !state.log.empty() &&
                  state.log.back().message ==
                      "cuwacunu.cmd --run \"not a command\"",
              "shell-style run command should preserve the full failed "
              "command in status and logs") &&
      ok;
  state.log.clear();
  ok = require(!actions.dispatch("cuwacunu_cmd --unknown-option", state),
               "shell-style command parser should reject unknown options") &&
       ok;
  ok = require(state.status == "unknown command: cuwacunu_cmd --unknown-option",
               "shell-style command parser should report the full unknown "
               "utility option") &&
       ok;
  state.screen = ScreenMode::Home;
  state.help_view = false;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --help", state),
               "shell-style help option should route through the command "
               "prompt") &&
       ok;
  ok = require(state.help_view &&
                   state.status.find("help overlay=open") != std::string::npos,
               "shell-style help option should open the restored menu "
               "overlay") &&
       ok;
  state.help_view = false;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd --help", state),
               "legacy iinuji shell help option should route through the "
               "command prompt") &&
       ok;
  ok = require(state.help_view &&
                   state.status.find("help overlay=open") != std::string::npos,
               "legacy iinuji shell help option should open the restored "
               "menu overlay") &&
       ok;
  state.help_view = false;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --version", state),
               "shell-style version option should route through the command "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs && state.status == "version" &&
                   state.command_name == "cuwacunu.cmd" && !state.log.empty() &&
                   state.log.front().message == "cuwacunu.cmd iinuji interface",
               "shell-style version option should publish identity lines to "
               "Shell Logs") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  state.log.clear();
  ok = require(workspace_zoom_hint_text(state, "detail panel")
                       .find("Zoom: split -> [full] detail panel") !=
                   std::string::npos,
               "workspace zoom hint should show current split state before "
               "the full-panel action") &&
       ok;
  ok = require(actions.dispatch("cuwacunu.cmd --full", state),
               "shell-style full option should route through the command "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace full",
               "shell-style full option should toggle the current workspace "
               "to full panel mode") &&
       ok;
  ok = require(workspace_zoom_hint_text(state, "detail panel")
                       .find("Zoom: full -> [split] detail panel") !=
                   std::string::npos,
               "workspace zoom hint should show current full state before the "
               "split action") &&
       ok;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd --zoom", state),
               "legacy iinuji shell zoom option should route through the "
               "command prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   !workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace split",
               "legacy iinuji shell zoom option should toggle the current "
               "workspace back to split mode") &&
       ok;
  state.screen = ScreenMode::Logs;
  state.workspace.zoom_flags = 0u;
  ok = require(logs_scroll_rail_text(state, 0u).find("zoom=split->[full]") !=
                   std::string::npos,
               "Shell Logs scroll rail should show current split state before "
               "the full-stream action") &&
       ok;
  state.workspace.zoom_flags = 0u;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("version", state),
               "natural version command should dispatch through the action "
               "registry") &&
       ok;
  bool workbench_identity_seen = false;
  for (const auto &entry : state.log) {
    if (entry.message == "workbench: F2 core workspace")
      workbench_identity_seen = true;
  }
  ok = require(state.screen == ScreenMode::Logs && workbench_identity_seen,
               "natural version command should expose the Workbench "
               "identity without reserved wording") &&
       ok;
  state.screen = ScreenMode::Home;
  state.action_menu_open = false;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd --catalog --run \"show runtime\"",
                                state),
               "catalog/run command should dispatch from the command prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime && state.action_menu_open &&
                   state.status == "action menu" && !state.log.empty() &&
                   state.log.front().message == "screen=runtime",
               "catalog/run command should route the nested command before "
               "opening the palette") &&
       ok;
  state.screen = ScreenMode::Home;
  state.action_menu_open = false;
  state.log.clear();
  ok = require(
           actions.dispatch(
               "cuwacunu.cmd --palette --command \"show shell logs\"", state),
           "palette/command spelling should dispatch from the command "
           "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs && state.action_menu_open &&
                   state.status == "action menu" && !state.log.empty() &&
                   state.log.front().message == "screen=shell_logs",
               "palette/command spelling should route the nested command "
               "before opening the palette") &&
       ok;
  state.screen = ScreenMode::Home;
  state.action_menu_open = false;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd runtime --snapshot", state),
               "shell-style positional screen snapshot should dispatch inside "
               "the prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "shell-style positional screen snapshot should switch to the "
               "requested screen") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu.cmd runtime --snapshot", state),
               "legacy shell command alias should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "legacy shell command alias should switch to the requested "
               "screen") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd runtime --snapshot", state),
               "legacy iinuji shell command alias should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "legacy iinuji shell command alias should switch to the "
               "requested screen") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd --visual", state),
               "legacy iinuji shell visual utility should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.home_visual.presentation ==
                       HomePresentationMode::Showcase &&
                   state.status == "home visual",
               "legacy iinuji shell visual utility should restore the Home "
               "showcase") &&
       ok;
  {
    const auto shell_payload =
        payload_after_shell_command_prefix("iinuji_cmd --splash=farewell");
    ok = require(shell_payload.has_value() &&
                     shell_payload.value() == "--splash=farewell",
                 "shared shell prefix parser should recognize the legacy "
                 "iinuji command prefix") &&
         ok;
  }
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("iinuji_cmd --splash=farewell", state),
               "legacy iinuji shell farewell utility should dispatch inside "
               "the prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   state.status == "farewell splash" && !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "legacy iinuji shell farewell utility should restore the "
               "closing visual diagnostics") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd --screen lattice", state),
               "shell-style screen option should dispatch inside the prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "screen=lattice",
               "shell-style screen option should switch to the requested "
               "screen") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("cuwacunu_cmd \"show shell logs\" --snapshot",
                                state),
               "shell-style quoted command snapshot should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.status == "show shell logs" && !state.log.empty() &&
                   state.log.front().message == "screen=shell_logs",
               "shell-style quoted command snapshot should route through the "
               "nested show command") &&
       ok;
  state.screen = ScreenMode::Home;
  state.help_view = false;
  state.action_menu_open = false;
  ok = require(actions.dispatch("cuwacunu_cmd --menu", state),
               "shell-style menu command should dispatch inside the prompt") &&
       ok;
  ok = require(state.help_view && !state.action_menu_open,
               "shell-style menu command should open the restored menu "
               "overlay") &&
       ok;
  state.help_view = false;
  ok = require(actions.dispatch("iinuji_cmd --menu", state),
               "legacy iinuji shell menu command should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.help_view && !state.action_menu_open,
               "legacy iinuji shell menu command should open the restored "
               "menu overlay") &&
       ok;
  state.help_view = false;
  ok = require(actions.dispatch("cuwacunu_cmd --actions", state),
               "shell-style actions command should dispatch inside the "
               "prompt") &&
       ok;
  ok = require(state.action_menu_open && !state.help_view,
               "shell-style actions command should open the action palette") &&
       ok;
  state.action_menu_open = false;
  ok = require(actions.dispatch("iinuji_cmd --actions", state),
               "legacy iinuji shell actions command should dispatch inside "
               "the prompt") &&
       ok;
  ok = require(state.action_menu_open && !state.help_view,
               "legacy iinuji shell actions command should open the action "
               "palette") &&
       ok;
  state.help_view = false;
  state.action_menu_open = false;
  ok =
      require(actions.dispatch("cuwacunu.cmd --snapshot --menu", state),
              "snapshot-prefixed shell menu command should dispatch inside the "
              "prompt") &&
      ok;
  ok = require(state.help_view && !state.action_menu_open,
               "snapshot-prefixed shell menu command should open the restored "
               "menu overlay") &&
       ok;
  state.help_view = false;
  ok = require(actions.dispatch("cuwacunu_cmd --actions --snapshot", state),
               "snapshot-suffixed shell actions command should dispatch inside "
               "the prompt") &&
       ok;
  ok = require(state.action_menu_open && !state.help_view,
               "snapshot-suffixed shell actions command should open the action "
               "palette") &&
       ok;
  state.action_menu_open = false;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  ok =
      require(actions.dispatch("cuwacunu.cmd --snapshot --full", state),
              "snapshot-prefixed shell full command should dispatch inside the "
              "prompt") &&
      ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace full",
               "snapshot-prefixed shell full command should toggle the current "
               "workspace to full panel mode") &&
       ok;
  state.workspace.zoom_flags = 0u;
  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("iinuji_cmd --snapshot --visual", state),
               "snapshot-prefixed shell visual command should dispatch inside "
               "the prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Home &&
                   state.home_visual.presentation ==
                       HomePresentationMode::Showcase &&
                   state.status == "home visual",
               "snapshot-prefixed shell visual command should restore the Home "
               "showcase") &&
       ok;
  ok = require(
           actions.dispatch("cuwacunu.cmd --splash loading --snapshot", state),
           "snapshot-suffixed shell splash command should dispatch inside "
           "the prompt") &&
       ok;
  ok = require(
           actions.dispatch("cuwacunu_cmd --splash loading --snapshot", state),
           "PATH snapshot-suffixed shell splash command should dispatch inside "
           "the prompt") &&
       ok;
  ok = require(state.home_visual.presentation ==
                       HomePresentationMode::BootstrapSplash &&
                   state.status == "bootstrap splash",
               "snapshot-suffixed shell splash command should restore the "
               "loading visual diagnostics") &&
       ok;
  ok =
      require(actions.dispatch("cuwacunu.cmd --snapshot --splash close", state),
              "snapshot-prefixed shell closing splash command should dispatch "
              "inside the prompt") &&
      ok;
  ok =
      require(
          actions.dispatch("cuwacunu_cmd --snapshot --splash close", state),
          "PATH snapshot-prefixed shell closing splash command should dispatch "
          "inside the prompt") &&
      ok;
  ok = require(state.home_visual.presentation ==
                       HomePresentationMode::FarewellSplash &&
                   state.status == "farewell splash",
               "snapshot-prefixed shell closing splash command should restore "
               "the farewell visual diagnostics") &&
       ok;
  state.screen = ScreenMode::Home;
  ok = require(
           actions.dispatch("cuwacunu.cmd --snapshot --screen lattice", state),
           "snapshot-prefixed shell screen command should dispatch inside "
           "the prompt") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "screen=lattice",
               "snapshot-prefixed shell screen command should switch to the "
               "requested screen") &&
       ok;

  state.help_view = false;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.help_scroll_x = 0;
  ok = require(actions.dispatch("iinuji.help()", state),
               "canonical help open command should dispatch") &&
       ok;
  ok = require(state.help_view && state.help_scroll_y == 0 &&
                   state.help_scroll_x == 0 &&
                   state.status ==
                       "help overlay=open (Esc or click [x] to close)",
               "help open should restore the legacy overlay status") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.close()", state),
               "canonical help close command should dispatch") &&
       ok;
  ok = require(!state.help_view && state.status == "help overlay=closed",
               "help close should restore the legacy overlay status") &&
       ok;
  ok = require(actions.dispatch("h", state),
               "legacy h help alias should dispatch") &&
       ok;
  ok = require(state.help_view && state.help_scroll_y == 0 &&
                   state.help_scroll_x == 0,
               "legacy h help alias should open the help overlay") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.scroll.down()", state),
               "canonical help scroll down command should dispatch") &&
       ok;
  ok = require(state.help_view && state.help_scroll_y == 3 &&
                   state.status == "help scroll=down",
               "help scroll down should restore the legacy three-row step and "
               "status") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.scroll.up()", state),
               "canonical help scroll up command should dispatch") &&
       ok;
  ok = require(state.help_scroll_y == 0 && state.status == "help scroll=up",
               "help scroll up should clamp after the legacy three-row step "
               "and status") &&
       ok;
  ok = require(actions.dispatch("help down", state),
               "natural help down command should dispatch") &&
       ok;
  ok = require(state.help_scroll_y == 3 && state.status == "help scroll=down",
               "natural help down should use the legacy three-row step") &&
       ok;
  ok = require(actions.dispatch("menu up", state),
               "natural menu up command should dispatch") &&
       ok;
  ok =
      require(state.help_scroll_y == 0 && state.status == "help scroll=up",
              "natural menu up should clamp after the legacy three-row step") &&
      ok;
  ok = require(actions.dispatch("iinuji.help.scroll.page.down()", state),
               "canonical help page down command should dispatch") &&
       ok;
  ok = require(state.help_scroll_y == 20 &&
                   state.status == "help scroll=page-down",
               "help page down should restore the legacy twenty-row step and "
               "status") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.scroll.page.up()", state),
               "canonical help page up command should dispatch") &&
       ok;
  ok =
      require(state.help_scroll_y == 0 && state.status == "help scroll=page-up",
              "help page up should clamp after the legacy twenty-row step "
              "and status") &&
      ok;
  ok = require(actions.dispatch("help page down", state),
               "natural help page down command should dispatch") &&
       ok;
  ok = require(
           state.help_scroll_y == 20 && state.status == "help scroll=page-down",
           "natural help page down should use the legacy twenty-row step") &&
       ok;
  ok = require(actions.dispatch("menu page up", state),
               "natural menu page up command should dispatch") &&
       ok;
  ok =
      require(state.help_scroll_y == 0 && state.status == "help scroll=page-up",
              "natural menu page up should clamp after the legacy twenty-row "
              "step") &&
      ok;
  ok = require(actions.dispatch("iinuji.help.scroll.right()", state),
               "canonical help scroll right command should dispatch") &&
       ok;
  ok = require(state.help_scroll_x == 16 && state.status == "help scroll=right",
               "help scroll right should restore the legacy sixteen-column "
               "step and status") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.scroll.left()", state),
               "canonical help scroll left command should dispatch") &&
       ok;
  ok = require(state.help_scroll_x == 0 && state.status == "help scroll=left",
               "help scroll left should clamp after the legacy sixteen-column "
               "step and status") &&
       ok;
  ok = require(actions.dispatch("help right", state),
               "natural help right command should dispatch") &&
       ok;
  ok =
      require(state.help_scroll_x == 16 && state.status == "help scroll=right",
              "natural help right should use the legacy sixteen-column step") &&
      ok;
  ok = require(actions.dispatch("menu left", state),
               "natural menu left command should dispatch") &&
       ok;
  ok = require(state.help_scroll_x == 0 && state.status == "help scroll=left",
               "natural menu left should clamp after the legacy column step") &&
       ok;
  state.help_scroll_y = 7;
  state.help_scroll_x = 11;
  ok = require(actions.dispatch("iinuji.help.scroll.home()", state),
               "canonical help scroll home command should dispatch") &&
       ok;
  ok = require(state.help_scroll_y == 0 && state.help_scroll_x == 0 &&
                   state.status == "help scroll=home",
               "help scroll home should restore legacy status") &&
       ok;
  ok = require(actions.dispatch("iinuji.help.scroll.end()", state),
               "canonical help scroll end command should dispatch") &&
       ok;
  ok = require(state.help_scroll_y == std::numeric_limits<int>::max() &&
                   state.status == "help scroll=end",
               "help scroll end should restore legacy status") &&
       ok;
  state.help_view = true;
  state.help_scroll_y = 20;
  ok = require(scroll_help_overlay(state, key_page_up),
               "direct help overlay page-up key should scroll") &&
       ok;
  ok =
      require(state.help_scroll_y == 0 && state.status == "help scroll=page-up",
              "direct help overlay key scroll should restore legacy status") &&
      ok;
  ok = require(scroll_help_overlay_by(state, 3, 0),
               "direct help overlay relative scroll should scroll") &&
       ok;
  ok = require(state.help_scroll_y == 3 && state.status == "help scroll=down",
               "direct help overlay relative scroll should restore legacy "
               "status") &&
       ok;
  state.help_scroll_y = 0;
  state.help_scroll_x = 16;
  ok = require(scroll_help_overlay(state, 'j') && state.help_scroll_y == 3 &&
                   state.status == "help scroll=down",
               "help overlay j key should restore legacy down movement") &&
       ok;
  ok = require(scroll_help_overlay(state, 'k') && state.help_scroll_y == 0 &&
                   state.status == "help scroll=up",
               "help overlay k key should restore legacy up movement") &&
       ok;
  ok = require(scroll_help_overlay(state, 'h') && state.help_scroll_x == 0 &&
                   state.status == "help scroll=left",
               "help overlay h key should restore legacy left movement") &&
       ok;
  ok = require(scroll_help_overlay(state, 'l') && state.help_scroll_x == 16 &&
                   state.status == "help scroll=right",
               "help overlay l key should restore legacy right movement") &&
       ok;

  state.logs = LogsState{};
  state.logs.scroll_y = 0;
  state.logs.selected_setting = 0;
  ok = require(actions.dispatch("iinuji.logs.settings.select.prev()", state),
               "canonical logs setting previous command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.logs.selected_setting == logs_settings_count() - 1u &&
                   state.status == "logs.settings.cursor=prev",
               "logs setting previous should restore legacy cursor status") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.settings.select.next()", state),
               "canonical logs setting next command should dispatch") &&
       ok;
  ok = require(state.logs.selected_setting == 0u &&
                   state.status == "logs.settings.cursor=next",
               "logs setting next should restore legacy cursor status") &&
       ok;
  state.logs.selected_setting = 1u;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  ok = require(actions.dispatch("iinuji.logs.settings.change.next()", state),
               "canonical logs selected-setting change next command should "
               "dispatch") &&
       ok;
  ok = require(state.logs.severity_filter ==
                       LogsSeverityFilter::WarningOrHigher &&
                   state.status == "logs.level=WARNING+",
               "logs selected-setting change next should advance the active "
               "lens") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.settings.change.prev()", state),
               "canonical logs selected-setting change previous command "
               "should dispatch") &&
       ok;
  ok = require(state.logs.severity_filter == LogsSeverityFilter::InfoOrHigher &&
                   state.status == "logs.level=INFO+",
               "logs selected-setting change previous should reverse the "
               "active lens") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.scroll.down()", state),
               "canonical logs scroll down command should dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Logs && state.logs.scroll_y == 6 &&
                  !state.logs.auto_follow && state.status == "logs scroll=down",
              "logs scroll down should restore the legacy six-row step and "
              "status") &&
      ok;
  ok = require(actions.dispatch("iinuji.logs.scroll.up()", state),
               "canonical logs scroll up command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 0 && !state.logs.auto_follow &&
                   state.status == "logs scroll=up",
               "logs scroll up should clamp after the legacy six-row step and "
               "status") &&
       ok;
  ok = require(actions.dispatch("logs down", state),
               "natural logs down command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 6 && !state.logs.auto_follow &&
                   state.status == "logs scroll=down",
               "natural logs down should use the legacy six-row step and stop "
               "follow") &&
       ok;
  ok = require(actions.dispatch("log up", state),
               "natural log up command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 0 && !state.logs.auto_follow &&
                   state.status == "logs scroll=up",
               "natural log up should clamp after the legacy six-row step") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.scroll.page.down()", state),
               "canonical logs page down command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 20 && !state.logs.auto_follow &&
                   state.status == "logs scroll=page-down",
               "logs page down should restore the legacy twenty-row step and "
               "status") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.scroll.page.up()", state),
               "canonical logs page up command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 0 && !state.logs.auto_follow &&
                   state.status == "logs scroll=page-up",
               "logs page up should clamp after the legacy twenty-row step "
               "and status") &&
       ok;
  ok = require(actions.dispatch("logs page down", state),
               "natural logs page down command should dispatch") &&
       ok;
  ok =
      require(state.logs.scroll_y == 20 && !state.logs.auto_follow &&
                  state.status == "logs scroll=page-down",
              "natural logs page down should use the legacy twenty-row step") &&
      ok;
  ok = require(actions.dispatch("log page up", state),
               "natural log page up command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 0 && !state.logs.auto_follow &&
                   state.status == "logs scroll=page-up",
               "natural log page up should clamp after the legacy twenty-row "
               "step") &&
       ok;
  state.logs.scroll_y = 7;
  ok = require(actions.dispatch("iinuji.logs.scroll.home()", state),
               "canonical logs home command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == 0 && !state.logs.auto_follow &&
                   state.status == "logs scroll=home",
               "logs home should restore legacy status") &&
       ok;
  ok = require(actions.dispatch("iinuji.logs.scroll.end()", state),
               "canonical logs end command should dispatch") &&
       ok;
  ok = require(state.logs.scroll_y == std::numeric_limits<int>::max() &&
                   state.logs.auto_follow && state.status == "logs scroll=end",
               "logs end should restore legacy status and follow") &&
       ok;

  ui.nav = std::make_shared<iinuji_object_t>();
  ui.nav->id = "cmd.tabs";
  ui.nav->visible = true;
  ui.nav->screen = Rect{4, 2, 96, 1};

  state.screen = ScreenMode::Home;
  ok = require(dispatch_function_key(KEY_F(2), actions, state),
               "F2 should dispatch to Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "F2 should switch to Workbench with legacy status") &&
       ok;
  ok = require(dispatch_function_key(KEY_F(3), actions, state),
               "F3 should dispatch to Runtime") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "F3 should switch to Runtime with legacy status") &&
       ok;
  ok = require(dispatch_function_key(KEY_F(4), actions, state),
               "F4 should dispatch to Lattice") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "screen=lattice",
               "F4 should switch to Lattice with legacy status") &&
       ok;
  ok = require(dispatch_function_key(KEY_F(8), actions, state),
               "F8 should dispatch to Shell Logs") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.status == "screen=shell_logs",
               "F8 should switch to Shell Logs with legacy status") &&
       ok;
  ok = require(dispatch_function_key(KEY_F(9), actions, state),
               "F9 should dispatch to Config") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.status == "screen=config",
               "F9 should switch to Config with legacy status") &&
       ok;
  ok = require(dispatch_function_key(KEY_F(1), actions, state),
               "F1 should dispatch to Home") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home && state.status == "screen=home",
              "F1 should switch to Home with legacy status") &&
      ok;

  {
    CmdUi command_ui = create_ui(gui_config_path);
    state.screen = ScreenMode::Home;
    state.command_mode = false;
    state.command_focused = false;
    state.status = "ready";
    set_text(command_ui.input, "");
    handle_key('\t', actions, state, command_ui);
    ok = require(state.command_focused && command_ui.input &&
                     command_ui.input->focused && command_ui.command_hint &&
                     !command_ui.command_hint->visible,
                 "Tab should focus the command input and show its caret") &&
         ok;
    handle_key(key_escape, actions, state, command_ui);
    ok = require(!state.command_focused && !state.command_mode &&
                     command_ui.input && !command_ui.input->focused,
                 "Esc should blur an empty focused command input") &&
         ok;
    for (const char ch : std::string{"config"})
      handle_key(ch, actions, state, command_ui);
    ok = require(state.command_mode && state.command_focused &&
                     get_text(command_ui.input) == "config" &&
                     state.status == "editing",
                 "plain typing should enter command mode without requiring a "
                 "colon prefix") &&
         ok;
    handle_key(key_enter, actions, state, command_ui);
    ok = require(state.screen == ScreenMode::Config && !state.command_mode &&
                     !state.command_focused &&
                     get_text(command_ui.input).empty() &&
                     !state.command_history.empty() &&
                     state.command_history.back() == "config",
                 "directly typed commands should dispatch through the command "
                 "registry") &&
         ok;
    state.screen = ScreenMode::Home;
    state.help_view = true;
    state.command_mode = false;
    state.command_focused = false;
    state.status = "help overlay=open";
    set_text(command_ui.input, "");
    handle_key(':', actions, state, command_ui);
    handle_key('r', actions, state, command_ui);
    handle_key(key_enter, actions, state, command_ui);
    ok = require(!state.help_view && !state.command_mode &&
                     get_text(command_ui.input).empty() &&
                     state.status == "hero shell refresh queued",
                 "typed commands from the help overlay should close the menu "
                 "before dispatching non-help utilities") &&
         ok;
  }

  state.screen = ScreenMode::Home;
  state.help_view = false;
  state.command_name = "cuwacunu_cmd";
  state.status = "ready";
  const std::string home_rail = make_tab_text(state);
  ok = require(home_rail.find("[F1 HOME]  F2 WORKBENCH") != std::string::npos,
               "title rail should restore the legacy padded gap before F2") &&
       ok;
  ok = require(home_rail.find("F2 WORKBENCH   F3 RUNTIME") != std::string::npos,
               "title rail should preserve the legacy padded gap between "
               "inactive tabs") &&
       ok;
  ok = require(make_title_text(state) == home_rail,
               "title helper should return the restored F-key tab rail") &&
       ok;
  ok = require(make_title_text(state).find("screen=") == std::string::npos,
               "title should use the restored screen badge form") &&
       ok;
  ok = require(make_title_text(state).find("F1 HOME") != std::string::npos,
               "title should include the active F-key badge") &&
       ok;
  const std::string shell_snapshot = make_snapshot_text(state);
  ok =
      require(shell_snapshot.rfind(home_rail + "\n\n", 0) == 0,
              "snapshot chrome should start with the title-frame F-key rail") &&
      ok;
  ok = require(shell_snapshot.find("pulse[") == std::string::npos &&
                   shell_snapshot.find("cuwacunu.cmd |") == std::string::npos,
               "snapshot chrome should not duplicate the newer title header") &&
       ok;
  ok =
      require(make_nav_text(state).find("F2  WORKBENCH") != std::string::npos &&
                  make_nav_text(state).find("operator workbench") ==
                      std::string::npos,
              "navigation panel should carry the Workbench label without "
              "operator-era wording") &&
      ok;
  ok = require(make_nav_text(state).rfind("cuwacunu_cmd\nhome", 0) == 0,
               "navigation panel should use a screen-aware cuwacunu_cmd "
               "subtitle") &&
       ok;
  {
    CmdState launched_state = state;
    launched_state.command_name = "cuwacunu_cmd";
    ok = require(make_nav_text(launched_state).rfind("cuwacunu_cmd\nhome", 0) ==
                     0,
                 "navigation panel should reflect the PATH command name when "
                 "launched through cuwacunu_cmd") &&
         ok;
  }
  ok = require(make_nav_text(state).find("h/H / ? / help") != std::string::npos,
               "Home navigation should advertise both h and H help "
               "shortcuts") &&
       ok;
  ok = require(make_nav_text(state).find("a / actions") != std::string::npos,
               "navigation menu should advertise the action palette") &&
       ok;
  ok = require(make_nav_text(state).find("a / actions / palette / commands") !=
                   std::string::npos,
               "home current tools should advertise the restored action "
               "shortcut spelling") &&
       ok;
  ok = require(menu_command_row_index(state, "a | actions | palette | commands")
                   .has_value(),
               "menu overlay should expose the typeable legacy a / actions "
               "shortcut") &&
       ok;
  ok = require(make_nav_text(state).find(": command") != std::string::npos,
               "navigation menu should advertise command mode") &&
       ok;
  ok =
      require(
          make_nav_text(state).find("quit | exit | q | x") != std::string::npos,
          "navigation menu should advertise restored quit and exit aliases") &&
      ok;
  {
    CmdState menu_hint_state = state;
    menu_hint_state.help_view = true;
    ok =
        require(make_operator_hint_line(menu_hint_state)
                        .find("arrows/hjkl scroll") != std::string::npos,
                "menu bottom hint should advertise legacy h/j/k/l scrolling") &&
        ok;
    ok =
        require(
            make_operator_hint_line(menu_hint_state).find("Pg/Home/End jump") !=
                std::string::npos,
            "menu bottom hint should advertise page and edge scrolling") &&
        ok;
    ok =
        require(make_operator_hint_line(menu_hint_state).find("a / actions") !=
                    std::string::npos,
                "menu bottom hint should preserve the typed action shortcut") &&
        ok;
    ok = require(
             make_operator_hint_line(menu_hint_state).find("Esc/[x] close") !=
                 std::string::npos,
             "menu bottom hint should expose both restored close "
             "affordances") &&
         ok;

    const std::string menu_snapshot =
        make_menu_overlay_snapshot_text(menu_hint_state);
    ok = require(menu_snapshot.find('~') == std::string::npos,
                 "menu overlay snapshot should keep restored guide rows "
                 "readable without hard truncation") &&
         ok;
    ok = require(menu_snapshot.find("Arrows or h/j/k/l") != std::string::npos,
                 "menu overlay should advertise legacy h/j/k/l scrolling") &&
         ok;
    ok = require(menu_snapshot.find("PgUp / PgDn") != std::string::npos,
                 "menu overlay should keep page scrolling visible after "
                 "restoring h/j/k/l") &&
         ok;
    ok = require(menu_snapshot.find(detail_section_rule() + "\nMENU OVERLAY\n" +
                                    detail_section_rule()) != std::string::npos,
                 "menu overlay snapshot should frame its title band") &&
         ok;
    {
      const std::size_t screens_pos = menu_snapshot.find("SCREENS");
      const std::size_t context_pos = menu_snapshot.find("CONTEXT");
      const std::size_t aliases_pos = menu_snapshot.find("COMMAND ALIASES");
      const std::size_t command_line_pos = menu_snapshot.find("COMMAND LINE");
      const std::size_t current_pos =
          command_line_pos == std::string::npos
              ? std::string::npos
              : menu_snapshot.find("CURRENT SCREEN", command_line_pos);
      ok = require(screens_pos != std::string::npos &&
                       context_pos != std::string::npos &&
                       current_pos != std::string::npos &&
                       aliases_pos != std::string::npos &&
                       command_line_pos != std::string::npos &&
                       screens_pos < context_pos && context_pos < aliases_pos &&
                       command_line_pos < current_pos,
                   "menu overlay should surface compact screen context near "
                   "the top while keeping full current-screen utilities "
                   "scoped below command-line rows") &&
           ok;
      ok = require(menu_snapshot.find("screen tools") != std::string::npos,
                   "menu overlay compact context should point operators to "
                   "the full current-screen utility section") &&
           ok;
      ok = require(menu_current_screen_row_index(menu_hint_state,
                                                 "F2 / F3 / F4 / F8 / F9")
                       .has_value(),
                   "menu current-screen section should keep Home F-key escape "
                   "hatches scoped to that section") &&
           ok;
    }
    ok = require(menu_snapshot.find("show runtime | iinuji.show.runtime()") !=
                     std::string::npos,
                 "menu overlay should keep full show command paths readable") &&
         ok;
    ok = require(menu_snapshot.find(
                     "toggle full/split panel when the screen supports it") !=
                     std::string::npos,
                 "menu overlay should keep workspace utility comments "
                 "readable") &&
         ok;
    const std::string framed_home_disclosures =
        "\n---\ndisclosures\n\n" + detail_section_rule() + "\nHome showcase\n" +
        detail_section_rule();
    ok = require(menu_snapshot.find(framed_home_disclosures) ==
                     std::string::npos,
                 "menu overlay snapshot should not leak the covered Home "
                 "detail panel") &&
         ok;
  }
  ok = require(make_screen_menu_text(ScreenMode::Home).find("F4 lattice") !=
                   std::string::npos,
               "Home compact menu should advertise the restored F4 Lattice "
               "slot") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Home).find("F9 config") !=
                   std::string::npos,
               "Home compact menu should advertise the restored F9 Config "
               "slot") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Home).find("a / actions") !=
                   std::string::npos,
               "Home compact menu should use the restored typed action "
               "shortcut spelling") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Home).find("h/H/? help") !=
                   std::string::npos,
               "Home compact menu should advertise both h and H help") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Home).find("r / refresh") !=
                   std::string::npos,
               "Home compact menu should advertise the restored refresh "
               "shortcut") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Home).find("rail [") ==
                   std::string::npos,
               "Home compact menu should avoid visible command rail blocks") &&
       ok;
  {
    CmdState home_nav_state = state;
    home_nav_state.screen = ScreenMode::Home;
    const std::string home_nav = make_nav_text(home_nav_state);
    ok = require(
             home_nav.find("cuwacunu_cmd --visual") != std::string::npos &&
                 home_nav.find("--home-visual") != std::string::npos &&
                 home_nav.find("cuwacunu_cmd --image") != std::string::npos &&
                 home_nav.find("cuwacunu_cmd --animation") !=
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --image / --animation") ==
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --waajacamaya") !=
                     std::string::npos &&
                 home_nav.find("splash | bootstrap") != std::string::npos &&
                 home_nav.find("cuwacunu_cmd --splash") != std::string::npos &&
                 home_nav.find("cuwacunu_cmd --splash loading") !=
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --bootstrap / --boot") !=
                     std::string::npos &&
                 home_nav.find("farewell | closing") != std::string::npos &&
                 home_nav.find("cuwacunu_cmd --farewell / --closing") !=
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --splash close") !=
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --splash=farewell") !=
                     std::string::npos &&
                 home_nav.find("cuwacunu_cmd --splash=good-luck") !=
                     std::string::npos,
             "Home navigation current block should expose restored visual "
             "and splash shell utilities") &&
         ok;
  }
  ok = require(make_screen_menu_text(ScreenMode::Workbench).find("F8 logs") !=
                   std::string::npos,
               "blank Workbench compact menu should still offer screen "
               "escape hatches") &&
       ok;
  ok = require(make_screen_menu_text(ScreenMode::Workbench).find("F1 home") !=
                   std::string::npos,
               "blank Workbench compact menu should include the F1 Home "
               "escape hatch") &&
       ok;
  {
    CmdState workbench_nav_state = state;
    workbench_nav_state.screen = ScreenMode::Workbench;
    ok =
        require(make_nav_text(workbench_nav_state)
                        .rfind("cuwacunu_cmd\nworkbench", 0) == 0,
                "Workbench navigation subtitle should identify F2 Workbench") &&
        ok;
    ok =
        require(
            make_nav_text(workbench_nav_state).find("F1 home") !=
                    std::string::npos &&
                make_nav_text(workbench_nav_state).find("F4 lattice") !=
                    std::string::npos &&
                make_nav_text(workbench_nav_state).find("F8 logs  F9 config") !=
                    std::string::npos,
            "blank Workbench current block should expose restored "
            "navigation escape hatches") &&
        ok;
    ok = require(make_main_text(workbench_nav_state).empty(),
                 "Workbench workspace should stay empty until objectives are "
                 "modeled") &&
         ok;
    ok = require(make_detail_text(workbench_nav_state).empty(),
                 "Workbench detail panel should stay empty until objectives "
                 "are modeled") &&
         ok;
    const std::string workbench_snapshot =
        make_snapshot_text(workbench_nav_state);
    ok = require(workbench_snapshot.find("reserved") == std::string::npos &&
                     workbench_snapshot.find("Workbench detail") ==
                         std::string::npos,
                 "Workbench snapshot should not invent placeholder content") &&
         ok;
    ok = require(workbench_snapshot.find("\n---\n\n---\n") == std::string::npos,
                 "blank Workbench snapshot should skip empty workspace "
                 "sections") &&
         ok;
  }
  ok = require(
           make_screen_menu_text(ScreenMode::Workbench).find("a / actions") !=
               std::string::npos,
           "blank Workbench compact menu should preserve the typed action "
           "shortcut spelling") &&
       ok;
  ok = require(
           make_screen_menu_text(ScreenMode::Workbench).find("h/H/? help") !=
               std::string::npos,
           "blank Workbench compact menu should advertise both h and H help") &&
       ok;
  ok = require(
           make_screen_menu_text(ScreenMode::Workbench).find("r / refresh") !=
               std::string::npos,
           "blank Workbench compact menu should advertise the restored "
           "refresh shortcut without adding content") &&
       ok;
  const ScreenMode compact_menu_screens[] = {
      ScreenMode::Runtime, ScreenMode::Lattice, ScreenMode::Logs,
      ScreenMode::Config};
  for (ScreenMode screen : compact_menu_screens) {
    const std::string compact_menu = make_screen_menu_text(screen);
    ok = require(compact_menu.find("a / actions") != std::string::npos,
                 screen_label(screen) +
                     " compact menu should advertise the action palette") &&
         ok;
    ok = require(compact_menu.find("type/: command") != std::string::npos,
                 screen_label(screen) +
                     " compact menu should advertise direct typed commands") &&
         ok;
    ok = require(compact_menu.find("PgUp/PgDn") != std::string::npos,
                 screen_label(screen) +
                     " compact menu should advertise page scrolling") &&
         ok;
    const std::string refresh_text =
        screen == ScreenMode::Config ? "r reload" : "r refresh";
    ok = require(compact_menu.find(refresh_text) != std::string::npos,
                 screen_label(screen) +
                     " compact menu should advertise the screen refresh "
                     "shortcut") &&
         ok;
    ok =
        require(compact_menu.find("rail [") == std::string::npos,
                screen_label(screen) +
                    " compact menu should avoid visible command rail blocks") &&
        ok;
  }
  {
    CmdState nav_state = state;
    nav_state.screen = ScreenMode::Runtime;
    nav_state.runtime.focus = RuntimeFocus::Jobs;
    nav_state.runtime.detail_mode = RuntimeDetailMode::Trace;
    ok = require(make_nav_text(nav_state).find("H / ? / help") !=
                     std::string::npos,
                 "Runtime navigation should advertise uppercase H because "
                 "lowercase h browses rows") &&
         ok;
    ok = require(make_nav_text(nav_state).find("Current\n  focus Jobs") !=
                     std::string::npos,
                 "navigation panel should expose current Runtime utilities") &&
         ok;
    ok = require(make_nav_text(nav_state).find("Tab/Shift-Tab detail") !=
                     std::string::npos,
                 "navigation panel should advertise Runtime detail cycling") &&
         ok;
    ok = require(make_nav_text(nav_state).find("d devices  J jobs") !=
                     std::string::npos,
                 "Runtime current block should advertise direct lane focus "
                 "keys") &&
         ok;
    ok =
        require(make_nav_text(nav_state).find("r refresh") != std::string::npos,
                "Runtime current block should advertise screen-local refresh "
                "key") &&
        ok;
    ok = require(make_nav_text(nav_state).find("Enter menu  a actions") !=
                     std::string::npos,
                 "Runtime current block should advertise Enter menu and "
                 "explicit action access") &&
         ok;

    nav_state.screen = ScreenMode::Logs;
    ok = require(make_nav_text(nav_state).find("h/H / ? / help") !=
                     std::string::npos,
                 "Shell Logs navigation should advertise both h and H help") &&
         ok;
    ok = require(make_nav_text(nav_state).find("s source  v level  m meta") !=
                     std::string::npos,
                 "navigation panel should expose current Shell Logs lenses") &&
         ok;
    ok =
        require(make_nav_text(nav_state).find("t time    u thread e fields") !=
                    std::string::npos,
                "Shell Logs current block should expose timestamp, thread, and "
                "metadata-field quick keys") &&
        ok;
    ok = require(make_nav_text(nav_state).find("o color   l follow p mouse") !=
                     std::string::npos,
                 "Shell Logs current block should expose color, follow, and "
                 "mouse quick keys") &&
         ok;
    ok = require(make_nav_text(nav_state).find("Home/End logs") !=
                     std::string::npos,
                 "Shell Logs current block should expose oldest/newest jump "
                 "keys") &&
         ok;
    ok =
        require(make_nav_text(nav_state).find("r refresh") != std::string::npos,
                "Shell Logs current block should advertise refresh key") &&
        ok;
    ok = require(make_nav_text(nav_state).find("Left/Right settings") !=
                     std::string::npos,
                 "Shell Logs current block should expose selected-setting "
                 "change keys") &&
         ok;
    ok = require(
             make_screen_menu_text(ScreenMode::Logs)
                         .find("Left/Right settings") != std::string::npos &&
                 make_screen_menu_text(ScreenMode::Logs)
                         .find("u thread  e fields o color l follow p mouse") !=
                     std::string::npos &&
                 make_screen_menu_text(ScreenMode::Logs).find("h/H/? help") !=
                     std::string::npos,
             "Shell Logs compact menu should expose display toggles, "
             "selected-setting changes, and both help keys") &&
         ok;
    ok = require(make_detail_text(nav_state).find("h/H/?       menu/help") !=
                     std::string::npos,
                 "Shell Logs detail quick keys should use the restored "
                 "h/H/? menu/help spelling") &&
         ok;

    nav_state.screen = ScreenMode::Config;
    nav_state.config.files = {ConfigFileSummary{.relative_path = "alpha.dsl"}};
    ok = require(make_nav_text(nav_state).find("H / ? / help") !=
                     std::string::npos,
                 "Config navigation should advertise uppercase H because "
                 "lowercase h browses files") &&
         ok;
    ok = require(make_nav_text(nav_state).find("files | show file") !=
                     std::string::npos,
                 "navigation panel should expose current Config utilities") &&
         ok;
    ok =
        require(
            make_nav_text(nav_state).find("Enter/e preview") !=
                std::string::npos,
            "Config current block should advertise restored Enter/e preview") &&
        ok;
    ok =
        require(make_nav_text(nav_state).find("Home/End edges") !=
                    std::string::npos,
                "Config current block should advertise first/last file keys") &&
        ok;
    ok = require(make_nav_text(nav_state).find("r reload") != std::string::npos,
                 "Config current block should advertise screen-local reload "
                 "key") &&
         ok;
  }
  ok = require(make_help_text(state).find("F8 SHELL LOGS") != std::string::npos,
               "help screen list should use restored F-key badges") &&
       ok;
  ok = require(make_help_text(state).find("h/H / ? / help / menu") !=
                   std::string::npos,
               "help text should describe contextual h/H help shortcuts") &&
       ok;
  ok = require(make_help_text(state).find(
                   "h/j/k/l          scroll overlays; move rows on browser "
                   "screens") != std::string::npos,
               "help text should advertise legacy h/j/k/l overlay and browser "
               "movement") &&
       ok;
  ok = require(make_help_text(state).find(
                   "h/j/k/l and arrows             scroll this help/menu "
                   "overlay") != std::string::npos,
               "help text should advertise legacy h/j/k/l menu scrolling") &&
       ok;
  ok = require(make_help_text(state).find("splash / farewell") !=
                       std::string::npos &&
                   make_help_text(state).find("iinuji.home.animation()") !=
                       std::string::npos &&
                   make_help_text(state).find("iinuji.splash()") !=
                       std::string::npos &&
                   make_help_text(state).find("iinuji.home.farewell()") !=
                       std::string::npos &&
                   make_help_text(state).find("iinuji.farewell()") !=
                       std::string::npos,
               "help text should advertise restored Home animation and "
               "splash/farewell utilities") &&
       ok;
  ok =
      require(
          make_help_text(state).find("cuwacunu.cmd --actions / --palette") !=
                  std::string::npos &&
              make_help_text(state).find(
                  "cuwacunu.cmd --commands / --catalog") != std::string::npos &&
              make_help_text(state).find(
                  "cuwacunu.cmd --command \"show\" --snapshot") !=
                  std::string::npos &&
              make_help_text(state).find("iinuji_cmd --commands / --catalog") !=
                  std::string::npos,
          "help text should advertise restored palette and command catalog "
          "utilities") &&
      ok;
  ok = require(make_help_text(state).find("Screen-local utilities") !=
                       std::string::npos &&
                   make_help_text(state).find("Runtime Enter menu") !=
                       std::string::npos &&
                   make_help_text(state).find("Shell Logs Home / End") !=
                       std::string::npos,
               "help text should advertise restored screen-local utilities") &&
       ok;
  ok = require(make_splash_detail_text("bootstrap", 0).find("F2 WORKBENCH") !=
                   std::string::npos,
               "splash detail should use restored F-key badges") &&
       ok;
  ok = require(bootstrap_splash_status_text() ==
                   "waajacu™ | bootstrap in progress",
               "bootstrap splash status strip should keep the legacy branded "
               "bottom line") &&
       ok;
  ok = require(farewell_splash_status_text() == "waajacu™ | closing terminal",
               "farewell splash status strip should keep the legacy branded "
               "bottom line") &&
       ok;
  ok = require(
           make_splash_detail_text("bootstrap", 0, 0.50).find("progress [") !=
               std::string::npos,
           "splash detail should expose a restored progress bar") &&
       ok;
  ok = require(
           animated_rule(0) == "●" && animated_rule(1) == "•" &&
               is_animated_rule_line(animated_rule(0)) &&
               make_splash_detail_text("bootstrap", 0, 0.50).find("●") !=
                   std::string::npos &&
               make_splash_detail_text("bootstrap", 0, 0.50).find(".:-=+*#") ==
                   std::string::npos,
           "bootstrap splash should use a single-cell breathing dot instead "
           "of the old ASCII loading ramp") &&
       ok;
  ok = require(make_splash_detail_text("bootstrap", 0, 0.50).find("█") !=
                       std::string::npos &&
                   make_splash_detail_text("bootstrap", 0, 0.50).find("░") !=
                       std::string::npos,
               "splash progress bars should use solid and shaded blocks") &&
       ok;
  ok = require(
           make_splash_detail_text("bootstrap", 0, 0.50).find("GUI assets") !=
               std::string::npos,
           "bootstrap splash should expose GUI asset stage") &&
       ok;
  ok = require(make_splash_detail_text("bootstrap", 0, 0.50)
                           .find("[  ] Workbench") != std::string::npos &&
                   make_splash_detail_text("bootstrap", 0, 0.50)
                           .find("F-key rail around F2 Workbench") !=
                       std::string::npos &&
                   make_splash_detail_text("bootstrap", 0, 0.50)
                           .find("restore the F-key rail") ==
                       std::string::npos &&
                   make_splash_detail_text("bootstrap", 0, 0.50)
                           .find("Operator workbench") == std::string::npos,
               "bootstrap splash should expose the Workbench stage without "
               "operator-era wording") &&
       ok;
  ok = require(make_splash_detail_text("bootstrap", 0, 0.50)
                       .find("Runtime inventory") != std::string::npos,
               "bootstrap splash should expose Runtime inventory stage") &&
       ok;
  ok = require(
           make_splash_detail_text("farewell", 0, 1.0).find("[ok] Good luck") !=
               std::string::npos,
           "farewell splash should restore the warm closing line") &&
       ok;
  ok = require(make_splash_detail_text("farewell", 0, 1.0).find("[..]") ==
                   std::string::npos,
               "farewell splash should use Unicode stage dots instead of "
               "ASCII loading brackets") &&
       ok;
  ok = require(kAnimatedHomeFrameStepMs == 33,
               "Home animation cadence should keep the legacy 30fps frame "
               "step") &&
       ok;
  ok = require(kFarewellHoldMs == 1100,
               "farewell splash should keep the legacy hold duration") &&
       ok;
  {
    CmdState idle_state{};
    idle_state.screen = ScreenMode::Home;
    ok = require(refresh_visible_screen_on_idle(idle_state, true),
                 "animated Home should still request idle frames") &&
         ok;
    ok = require(!refresh_visible_screen_on_idle(idle_state, false),
                 "static Home should not request idle redraws") &&
         ok;
    idle_state.screen = ScreenMode::Runtime;
    ok = require(!refresh_visible_screen_on_idle(idle_state, false),
                 "Runtime idle should not rescan snapshots during cursor "
                 "navigation") &&
         ok;
    idle_state.screen = ScreenMode::Lattice;
    ok = require(!refresh_visible_screen_on_idle(idle_state, false),
                 "Lattice idle should not rescan snapshots during worklist "
                 "navigation") &&
         ok;
    idle_state.screen = ScreenMode::Config;
    ok = require(!refresh_visible_screen_on_idle(idle_state, false),
                 "Config idle should not rescan files during list "
                 "navigation") &&
         ok;
    idle_state.screen = ScreenMode::Logs;
    ok = require(!refresh_visible_screen_on_idle(idle_state, false),
                 "Shell Logs idle should not repaint without new state") &&
         ok;

    CmdState poll_state{};
    poll_state.screen = ScreenMode::Runtime;
    poll_state.runtime.focus = RuntimeFocus::Jobs;
    ok = require(!should_poll_runtime_device_snapshot(poll_state, 3000, 0),
                 "Runtime Jobs worklist should not run device polling while "
                 "navigating jobs") &&
         ok;
    poll_state.runtime.focus = RuntimeFocus::Devices;
    ok = require(should_poll_runtime_device_snapshot(poll_state, 1000, 1000),
                 "Runtime device polling should not wait for a keyboard idle "
                 "window") &&
         ok;
    poll_state.command_focused = true;
    ok = require(!should_poll_runtime_device_snapshot(poll_state, 2000, 0),
                 "Runtime device polling should pause while the command panel "
                 "is focused") &&
         ok;

    RuntimeState cpu_delta_state{};
    RuntimeDeviceSnapshot cpu_first{};
    cpu_first.cpu_user_ticks = 100;
    cpu_first.cpu_idle_ticks = 100;
    push_runtime_device_history(cpu_delta_state, cpu_first);
    RuntimeDeviceSnapshot cpu_second{};
    cpu_second.cpu_user_ticks = 150;
    cpu_second.cpu_idle_ticks = 150;
    push_runtime_device_history(cpu_delta_state, cpu_second);
    ok = require(cpu_delta_state.device.cpu_usage_pct == 50,
                 "Runtime CPU telemetry should be computed from consecutive "
                 "proc/stat snapshots") &&
         ok;
  }
  {
    const auto &stages = snapshot_refresh_stages();
    ok = require(stages.size() == 6u,
                 "snapshot refresh should expose staged bootstrap progress") &&
         ok;
    ok = require(stages[1].label == std::string_view{"Runtime policy"},
                 "bootstrap progress should expose Runtime policy loading") &&
         ok;
    ok =
        require(stages[2].label == std::string_view{"Runtime inventory"},
                "bootstrap progress should expose Runtime inventory loading") &&
        ok;
    ok = require(stages[5].label == std::string_view{"Workbench"} &&
                     stages[5].detail.find("F2 Workbench") !=
                         std::string_view::npos,
                 "bootstrap progress should end at the Workbench stage") &&
         ok;
  }
  {
    std::ostringstream help_out{};
    print_help(help_out);
    const std::string cli_help = help_out.str();
    const std::string help_rule = detail_section_rule();
    ok = require(cli_help.rfind("usage: cuwacunu_cmd", 0) == 0,
                 "CLI help should present cuwacunu_cmd as the PATH shell "
                 "entrypoint") &&
         ok;
    ok = require(cli_help.find(help_rule + "\nScreens\n" + help_rule) !=
                         std::string::npos &&
                     cli_help.find(help_rule + "\nOptions\n" + help_rule) !=
                         std::string::npos &&
                     cli_help.find(help_rule + "\nExamples\n" + help_rule) !=
                         std::string::npos,
                 "CLI help should frame the main shell utility sections") &&
         ok;
    ok = require(cli_help.find(help_rule + "\nShortcut Commands\n" +
                               help_rule) != std::string::npos &&
                     cli_help.find(help_rule + "\nCommand Paths\n" +
                                   help_rule) != std::string::npos &&
                     cli_help.find(help_rule + "\nGUI Config Keys\n" +
                                   help_rule) != std::string::npos,
                 "CLI help should frame shortcut, canonical path, and GUI "
                 "config sections") &&
         ok;
    ok = require(cli_help.find("--version, -V") != std::string::npos,
                 "CLI help should advertise the non-interactive version "
                 "utility") &&
         ok;
    ok =
        require(cli_help.find("--run|--command COMMAND") != std::string::npos &&
                    cli_help.find("--actions|--palette|--commands|--catalog") !=
                        std::string::npos &&
                    cli_help.find("--visual|--home-visual|--image|--animation|"
                                  "--waajacamaya") != std::string::npos,
                "CLI usage should include restored shell utility aliases") &&
        ok;
    ok = require(cli_help.find("F1 home") != std::string::npos &&
                     cli_help.find("F8 shell logs") != std::string::npos,
                 "CLI help should advertise the restored F-key screen order") &&
         ok;
    ok = require(cli_help.find("exit command shell") != std::string::npos &&
                     cli_help.find("exit cuwacunu.cmd") == std::string::npos,
                 "CLI help should keep exit copy tied to the command shell") &&
         ok;
    ok =
        require(cli_help.find("--visual, --home-visual") != std::string::npos,
                "CLI help should advertise the Home visual snapshot utility") &&
        ok;
    ok = require(cli_help.find("menu/actions/visual/splash/full/screen") !=
                     std::string::npos,
                 "CLI help should describe splash as part of the prompt "
                 "snapshot family") &&
         ok;
    ok =
        require(
            cli_help.find("--image") != std::string::npos &&
                cli_help.find("--animation") != std::string::npos &&
                cli_help.find("still image preview") != std::string::npos &&
                cli_help.find("APNG motion preview") != std::string::npos &&
                cli_help.find("cuwacunu.cmd --home-visual") !=
                    std::string::npos &&
                cli_help.find("cuwacunu.cmd --image") != std::string::npos &&
                cli_help.find("cuwacunu.cmd --animation") !=
                    std::string::npos &&
                cli_help.find("cuwacunu_cmd --image") != std::string::npos &&
                cli_help.find("cuwacunu_cmd --waajacamaya") !=
                    std::string::npos &&
                cli_help.find("cuwacunu.cmd --waajacamaya") !=
                    std::string::npos &&
                cli_help.find("cuwacunu_cmd --animation") !=
                    std::string::npos &&
                cli_help.find("iinuji.home.animation()") != std::string::npos &&
                cli_help.find("iinuji_cmd --visual") != std::string::npos,
            "CLI help should advertise restored image and animation "
            "snapshot utilities and legacy iinuji command aliases") &&
        ok;
    ok = require(cli_help.find("--splash[=bootstrap|boot|loading|farewell|"
                               "closing|close|good-luck]") != std::string::npos,
                 "CLI help should advertise splash diagnostics and accepted "
                 "mode aliases") &&
         ok;
    ok = require(cli_help.find("--bootstrap, --boot") != std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash=boot") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash loading") !=
                         std::string::npos &&
                     cli_help.find("iinuji_cmd --splash loading") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash farewell") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu_cmd --boot") != std::string::npos,
                 "CLI help should advertise bootstrap and spaced splash "
                 "diagnostics") &&
         ok;
    ok = require(cli_help.find("--farewell, --closing") != std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash=farewell") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash=good-luck") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu_cmd --splash good luck") !=
                         std::string::npos &&
                     cli_help.find("cuwacunu.cmd --splash good luck") !=
                         std::string::npos &&
                     cli_help.find("iinuji_cmd --closing") != std::string::npos,
                 "CLI help should advertise the restored closing splash "
                 "utility") &&
         ok;
    ok = require(
             cli_help.find("cuwacunu_cmd --menu --run runtime") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --menu --run runtime") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --menu") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --menu") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --actions") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --actions") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --catalog") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --catalog") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --visual") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --visual") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --image") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --animation") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --image") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --animation") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --splash loading") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --splash loading") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --splash close") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --splash close") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --splash good-luck") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --splash good-luck") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --screen runtime") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --screen runtime") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd --snapshot --full") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu.cmd --snapshot --full") !=
                     std::string::npos &&
                 cli_help.find("cuwacunu_cmd src/config/hero.runtime.dsl "
                               "--snapshot") != std::string::npos &&
                 cli_help.find("cuwacunu.cmd src/config/hero.runtime.dsl "
                               "--snapshot") != std::string::npos &&
                 cli_help.find("cuwacunu_cmd --command \"show config\" "
                               "--snapshot") != std::string::npos &&
                 cli_help.find("cuwacunu.cmd --command \"show config\" "
                               "--snapshot") != std::string::npos &&
                 cli_help.find("cuwacunu_cmd --palette --command \"show shell "
                               "logs\"") != std::string::npos &&
                 cli_help.find("cuwacunu.cmd --palette --command \"show shell "
                               "logs\"") != std::string::npos &&
                 cli_help.find("cuwacunu.cmd --catalog") != std::string::npos &&
                 cli_help.find("iinuji_cmd --commands") != std::string::npos,
             "CLI help should advertise composable menu/run snapshots "
             "and restored command/palette/catalog utilities") &&
         ok;
    ok = require(cli_help.find("cuwacunu.cmd runtime --snapshot") !=
                     std::string::npos,
                 "CLI help should advertise positional screen snapshots") &&
         ok;
    ok = require(cli_help.find("cuwacunu_cmd runtime --snapshot") !=
                         std::string::npos &&
                     cli_help.find("iinuji_cmd runtime --snapshot") !=
                         std::string::npos &&
                     cli_help.find("also open this command shell") !=
                         std::string::npos,
                 "CLI help should advertise alternate shell command "
                 "aliases") &&
         ok;
    ok = require(
             cli_help.find("compatibility") == std::string::npos,
             "CLI help should not expose migration-era compatibility copy") &&
         ok;
    ok = require(cli_help.find("legacy marshal/inbox/human names open "
                               "Workbench") == std::string::npos,
                 "CLI help should not advertise removed F2 hero names") &&
         ok;
    ok = require(cli_help.find("legacy") == std::string::npos,
                 "CLI help should present restored utilities as current "
                 "commands, not migration artifacts") &&
         ok;
    ok = require(cli_help.find("aliases: work/w, run/rt, lat/proof, "
                               "log/shell/events, cfg") != std::string::npos,
                 "CLI help should document current --screen short aliases") &&
         ok;
    ok =
        require(cli_help.find("reserved operator cockpit") == std::string::npos,
                "CLI help should describe Workbench as blank for now, not "
                "reserved") &&
        ok;
    ok = require(
             cli_help.find("iinuji.screen.marshal()") == std::string::npos &&
                 cli_help.find("iinuji.screen.inbox()") == std::string::npos &&
                 cli_help.find("iinuji.screen.human()") == std::string::npos,
             "CLI help should keep removed canonical F2 hero paths hidden") &&
         ok;
    ok = require(cli_help.find("PATH|SCREEN|COMMAND") != std::string::npos,
                 "CLI help should describe positional shortcuts") &&
         ok;
    ok = require(cli_help.find("Shortcut Commands") != std::string::npos &&
                     cli_help.find("a / actions / palette / commands") !=
                         std::string::npos,
                 "CLI help should advertise restored one-key action palette "
                 "shortcuts") &&
         ok;
    ok =
        require(cli_help.find("home / f1 / 1") != std::string::npos &&
                    cli_help.find("workbench / work / w / f2 / 2") !=
                        std::string::npos &&
                    cli_help.find("runtime / run / rt / f3 / 3") !=
                        std::string::npos &&
                    cli_help.find("lattice / lat / proof / f4 / 4") !=
                        std::string::npos &&
                    cli_help.find("logs / log / shell / events / f8 / 8") !=
                        std::string::npos &&
                    cli_help.find("config / cfg / f9 / 9") != std::string::npos,
                "CLI help should advertise restored F-key screen aliases as "
                "typeable commands") &&
        ok;
    ok = require(
             cli_help.find("h/H / ? / help / menu") != std::string::npos &&
                 cli_help.find("lowercase h browses rows on browser screens") !=
                     std::string::npos,
             "CLI help should advertise the restored h/H help affordance") &&
         ok;
    ok =
        require(cli_help.find("Screen Keys") != std::string::npos &&
                    cli_help.find("Runtime Enter menu") != std::string::npos &&
                    cli_help.find("Config Enter / e") != std::string::npos &&
                    cli_help.find("Shell Logs Left / Right") !=
                        std::string::npos &&
                    cli_help.find("Shell Logs Home / End") != std::string::npos,
                "CLI help should advertise restored screen-local utility "
                "keys") &&
        ok;
    ok =
        require(cli_help.find("quit / q / exit / x") != std::string::npos,
                "CLI help should advertise restored quit and exit shortcuts") &&
        ok;
    ok = require(cli_help.find("--screen NAME|=NAME") != std::string::npos &&
                     cli_help.find("--global-config PATH|=PATH") !=
                         std::string::npos,
                 "CLI help should advertise equals-form long options") &&
         ok;
    ok = require(cli_help.find("iinuji.workspace.*()") != std::string::npos &&
                     cli_help.find("iinuji.workspace.zoom.toggle()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.workspace.split()") !=
                         std::string::npos,
                 "CLI help should advertise restored workspace utilities") &&
         ok;
    ok = require(cli_help.find("iinuji.version()") != std::string::npos,
                 "CLI help should advertise command identity path") &&
         ok;
    ok = require(cli_help.find("iinuji.config.*()") != std::string::npos,
                 "CLI help should advertise Config catalog utilities") &&
         ok;
    ok = require(cli_help.find("iinuji.runtime.row.prev()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.runtime.row.next()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.runtime.item.prev()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.runtime.item.next()") !=
                         std::string::npos,
                 "CLI help should advertise two-way Runtime row and item "
                 "utilities") &&
         ok;
    ok = require(cli_help.find("iinuji.logs.settings.source.next()") !=
                     std::string::npos,
                 "CLI help should advertise canonical Shell Logs source "
                 "cycle path") &&
         ok;
    ok = require(cli_help.find("iinuji.logs.settings.select.prev()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.logs.settings.select.next()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.logs.settings.change.prev()") !=
                         std::string::npos &&
                     cli_help.find("iinuji.logs.settings.change.next()") !=
                         std::string::npos,
                 "CLI help should advertise Shell Logs setting cursor "
                 "and change utilities") &&
         ok;
    ok = require(cli_help.find("logs setting prev/next") != std::string::npos &&
                     cli_help.find("logs setting left/right") !=
                         std::string::npos,
                 "CLI help should advertise natural Shell Logs setting "
                 "commands") &&
         ok;
    ok = require(cli_help.find("logs up/down") != std::string::npos &&
                     cli_help.find("logs page up/down") != std::string::npos &&
                     cli_help.find("logs home/end") != std::string::npos,
                 "CLI help should advertise natural Shell Logs scroll "
                 "commands") &&
         ok;
    ok = require(
             cli_help.find("iinuji.logs.scroll.up()") != std::string::npos &&
                 cli_help.find("iinuji.logs.scroll.down()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.scroll.page.up()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.scroll.page.down()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.scroll.home()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.scroll.end()") != std::string::npos,
             "CLI help should advertise direct Shell Logs scroll "
             "utilities") &&
         ok;
    ok = require(cli_help.find("help/menu up/down/left/right") !=
                         std::string::npos &&
                     cli_help.find("help/menu page/home/end") !=
                         std::string::npos,
                 "CLI help should advertise the restored menu scroll "
                 "utilities") &&
         ok;
    ok = require(
             cli_help.find("iinuji.help()") != std::string::npos &&
                 cli_help.find("iinuji.help.close()") != std::string::npos &&
                 cli_help.find("iinuji.help.scroll.*()") != std::string::npos,
             "CLI help should advertise canonical help/menu lifecycle paths") &&
         ok;
    ok =
        require(
            cli_help.find("actions up/down") != std::string::npos &&
                cli_help.find("actions page/home/end") != std::string::npos &&
                cli_help.find("iinuji.actions()") != std::string::npos &&
                cli_help.find("iinuji.commands()") != std::string::npos &&
                cli_help.find("commands namespace for action palette") !=
                    std::string::npos &&
                cli_help.find("alternate action palette namespace") ==
                    std::string::npos &&
                cli_help.find("iinuji.actions.run()") != std::string::npos &&
                cli_help.find("iinuji.commands.run.selected()") !=
                    std::string::npos &&
                cli_help.find("iinuji.actions.select.*()") !=
                    std::string::npos &&
                cli_help.find("iinuji.commands.select.*()") !=
                    std::string::npos &&
                cli_help.find("commands namespace movement utilities") !=
                    std::string::npos &&
                cli_help.find("alternate movement namespace") ==
                    std::string::npos &&
                cli_help.find("iinuji.actions.close()") != std::string::npos &&
                cli_help.find("iinuji.commands.close()") != std::string::npos &&
                cli_help.find("alternate close command") == std::string::npos,
            "CLI help should advertise direct action palette lifecycle and "
            "movement utilities") &&
        ok;
    ok = require(
             cli_help.find("iinuji.logs.settings.source.show()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.any()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.all()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.refresh()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.action()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.command()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.source.status()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.date.toggle()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.follow.toggle()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.level.debug()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.level.info()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.level.warning()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.level.error()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.level.fatal()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.metadata.filter.any()") !=
                     std::string::npos &&
                 cli_help.find(
                     "iinuji.logs.settings.metadata.filter.any_meta()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.metadata.filter.path()") !=
                     std::string::npos &&
                 cli_help.find(
                     "iinuji.logs.settings.metadata.filter.callsite()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.metadata.toggle()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.thread.toggle()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.color.toggle()") !=
                     std::string::npos &&
                 cli_help.find("iinuji.logs.settings.mouse.capture.toggle()") !=
                     std::string::npos,
             "CLI help should advertise direct Shell Logs source, severity, "
             "metadata, and display toggle utilities") &&
         ok;
    ok = require(cli_help.find("iinuji.logs.clear()") != std::string::npos,
                 "CLI help should advertise the canonical Shell Logs clear "
                 "path") &&
         ok;
    ok = require(cli_help.find("iinuji.quit() / iinuji.exit()") !=
                     std::string::npos,
                 "CLI help should advertise canonical quit and exit paths") &&
         ok;
    ok = require(cli_help.find("GUI.iinuji_home_animation_path") !=
                     std::string::npos,
                 "CLI help should advertise GUI animation config key") &&
         ok;
    ok = require(cli_help.find("empty operator workspace") ==
                         std::string::npos &&
                     cli_help.find("operator workbench") == std::string::npos,
                 "CLI help should not expose operator-era Workbench wording") &&
         ok;
  }
  {
    std::ostringstream version_out{};
    print_version(version_out);
    const std::string version_text = version_out.str();
    ok =
        require(
            version_text.rfind("cuwacunu_cmd iinuji interface", 0) == 0 &&
                version_text.find("aliases: cuwacunu_cmd, cuwacunu.cmd, "
                                  "iinuji_cmd") != std::string::npos &&
                version_text.find("F2 Workbench") != std::string::npos &&
                version_text.find("F2 core workspace") != std::string::npos &&
                version_text.find("F2 operator workspace") == std::string::npos,
            "version utility should identify aliases and Workbench") &&
        ok;
    ok = require(version_text.find(detail_section_rule() + "\nIdentity\n" +
                                   detail_section_rule()) != std::string::npos,
                 "version utility should frame its command identity details") &&
         ok;
    std::ostringstream alias_version_out{};
    print_version(alias_version_out, "cuwacunu.cmd");
    ok =
        require(alias_version_out.str().rfind("cuwacunu.cmd iinuji interface",
                                              0) == 0,
                "version utility should reflect an explicitly supplied command "
                "alias") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--version";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.invoked_command_name == "cuwacunu_cmd" &&
                     parsed.version && !parsed.snapshot,
                 "version option should request a non-interactive identity "
                 "response and retain the invoked command name") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "-v";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.version && !parsed.snapshot,
                 "lowercase version short option should request a "
                 "non-interactive identity response") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash=farewell";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell,
                 "splash snapshot option should parse farewell mode") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash=close";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell,
                 "splash=close option should parse farewell mode") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash=good-luck";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell,
                 "splash=good-luck option should parse the legacy Good luck "
                 "closing mode") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--farewell";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell,
                 "farewell shortcut option should parse closing splash mode") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen";
    char arg2[] = "shell logs";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.initial_screen == ScreenMode::Logs,
                 "screen parser should accept the visible Shell Logs name") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen=runtime";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.initial_screen == ScreenMode::Runtime,
                 "screen parser should accept --screen=NAME utility form") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen";
    char arg2[] = "iinuji.screen.runtime()";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok =
        require(parsed.initial_screen == ScreenMode::Runtime,
                "screen parser should accept canonical Runtime screen paths") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen=iinuji.screen.logs()";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.initial_screen == ScreenMode::Logs,
                 "screen parser should accept canonical Shell Logs screen "
                 "paths") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen";
    char arg2[] = "iinuji.screen.config()";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.initial_screen == ScreenMode::Config,
                 "screen parser should accept canonical Config screen paths") &&
         ok;
  }
  {
    ScreenMode parsed = ScreenMode::Home;
    ok = require(parse_screen_name("iinuji.screen.home()", parsed) &&
                     parsed == ScreenMode::Home,
                 "screen parser should accept canonical Home screen paths") &&
         ok;
    ok =
        require(parse_screen_name("iinuji.screen.lattice()", parsed) &&
                    parsed == ScreenMode::Lattice,
                "screen parser should accept canonical Lattice screen paths") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen";
    char arg2[] = "marshal";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.initial_screen == ScreenMode::Workbench,
                 "screen parser should map legacy Marshal name to Workbench") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--screen";
    char arg2[] = "iinuji.screen.human()";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.initial_screen == ScreenMode::Workbench,
                 "screen parser should map legacy human canonical path to "
                 "Workbench") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "runtime";
    char arg2[] = "--snapshot";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok =
        require(parsed.snapshot && parsed.initial_screen == ScreenMode::Runtime,
                "positional screen name should select the snapshot screen") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "show shell logs";
    char arg2[] = "--snapshot";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot && parsed.commands.size() == 1u &&
                     parsed.commands[0] == "show shell logs",
                 "positional command should be queued for dispatch") &&
         ok;
    CmdState positional_command_state{};
    positional_command_state.screen = ScreenMode::Home;
    ok =
        require(
            apply_option_commands(positional_command_state, parsed, actions),
            "positional command should dispatch through the action registry") &&
        ok;
    ok = require(positional_command_state.screen == ScreenMode::Logs,
                 "positional command should affect the same state as --run") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "/tmp/cuwacunu_cmd_gui_assets.config";
    char arg2[] = "--snapshot";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot &&
                     parsed.global_config_path ==
                         std::filesystem::path{
                             "/tmp/cuwacunu_cmd_gui_assets.config"},
                 "path-like positional argument should remain a global config "
                 "path") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "src/config/hero.runtime.dsl";
    char arg2[] = "--snapshot";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok =
        require(parsed.snapshot &&
                    parsed.global_config_path ==
                        std::filesystem::path{"/cuwacunu/src/config/.config"} &&
                    parsed.commands.size() == 1u &&
                    parsed.commands[0] ==
                        "config file src/config/hero.runtime.dsl",
                "managed config positional path should dispatch through the "
                "Config file selector instead of replacing global config") &&
        ok;

    CmdState positional_config_state{};
    positional_config_state.screen = ScreenMode::Home;
    positional_config_state.config.files = {
        ConfigFileSummary{.path = "/cuwacunu/src/config/hero.config.dsl",
                          .relative_path = "hero.config.dsl"},
        ConfigFileSummary{.path = "/cuwacunu/src/config/hero.runtime.dsl",
                          .relative_path = "hero.runtime.dsl"},
    };
    ok =
        require(apply_option_commands(positional_config_state, parsed, actions),
                "managed config positional path should dispatch") &&
        ok;
    ok = require(positional_config_state.screen == ScreenMode::Config &&
                     positional_config_state.config.selected_file == 1u,
                 "managed config positional path should open Config on the "
                 "matching file") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--global-config=/tmp/cuwacunu_cmd_gui_assets.config";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(
             parsed.global_config_path ==
                 std::filesystem::path{"/tmp/cuwacunu_cmd_gui_assets.config"},
             "global config parser should accept --global-config=PATH "
             "utility form") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--commands";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot && parsed.command_catalog,
                 "commands option should request a command catalog snapshot") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--catalog";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot && parsed.command_catalog,
                 "catalog option should request a command catalog snapshot") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--menu";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot && parsed.menu,
                 "menu option should request a non-interactive menu "
                 "snapshot utility") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--actions";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot && parsed.actions,
                 "actions option should request a non-interactive action "
                 "palette snapshot utility") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--visual";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok =
        require(parsed.snapshot && parsed.visual_snapshot &&
                    parsed.visual_snapshot_mode == HomeVisualSnapshotMode::Full,
                "visual option should request the Home visual snapshot") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--image";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot && parsed.visual_snapshot &&
                     parsed.visual_snapshot_mode ==
                         HomeVisualSnapshotMode::Image,
                 "image option should request the still Home image snapshot") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--animation";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(
             parsed.snapshot && parsed.visual_snapshot &&
                 parsed.visual_snapshot_mode ==
                     HomeVisualSnapshotMode::Animation,
             "animation option should request the Home animation snapshot") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--waajacamaya";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok =
        require(parsed.snapshot && parsed.visual_snapshot &&
                    parsed.visual_snapshot_mode == HomeVisualSnapshotMode::Full,
                "waajacamaya option should request the Home visual snapshot") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--bootstrap";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Bootstrap &&
                     parsed.commands.empty(),
                 "bootstrap option should request the bootstrap splash "
                 "snapshot directly") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash=boot";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Bootstrap &&
                     parsed.commands.empty(),
                 "splash=boot option should request bootstrap splash "
                 "diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash";
    char arg2[] = "loading";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Bootstrap &&
                     parsed.commands.empty(),
                 "spaced splash loading option should request bootstrap "
                 "splash diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash";
    char arg2[] = "farewell";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell &&
                     parsed.commands.empty(),
                 "spaced splash farewell option should render the same closing "
                 "diagnostics as the typed command alias") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash";
    char arg2[] = "close";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell &&
                     parsed.commands.empty(),
                 "spaced splash close option should request farewell splash "
                 "diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash";
    char arg2[] = "good-luck";
    char *argv[] = {arg0, arg1, arg2};
    const CmdOptions parsed = parse_options(3, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell &&
                     parsed.commands.empty(),
                 "spaced splash good-luck option should request farewell "
                 "splash diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--splash";
    char arg2[] = "good";
    char arg3[] = "luck";
    char *argv[] = {arg0, arg1, arg2, arg3};
    const CmdOptions parsed = parse_options(4, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell &&
                     parsed.commands.empty(),
                 "spaced splash good luck option should consume both words "
                 "and request farewell splash diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--closing";
    char *argv[] = {arg0, arg1};
    const CmdOptions parsed = parse_options(2, argv);
    ok = require(parsed.snapshot &&
                     parsed.splash_snapshot == SplashSnapshotMode::Farewell,
                 "closing option should request farewell splash diagnostics") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--run";
    char arg2[] = "show";
    char arg3[] = "--command=logs source show";
    char *argv[] = {arg0, arg1, arg2, arg3};
    const CmdOptions parsed = parse_options(4, argv);
    ok = require(parsed.commands.size() == 2u && parsed.commands[0] == "show" &&
                     parsed.commands[1] == "logs source show",
                 "run/command options should preserve command order") &&
         ok;

    CmdState command_state{};
    command_state.screen = ScreenMode::Home;
    command_state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
    ok = require(apply_option_commands(command_state, parsed, actions),
                 "run/command options should dispatch through the action "
                 "registry") &&
         ok;
    ok = require(command_state.screen == ScreenMode::Logs &&
                     command_state.logs.source_filter == LogsSourceFilter::Show,
                 "run/command options should affect the same state as the "
                 "interactive command line") &&
         ok;
    ok = require(command_state.command_history.size() == 2u &&
                     command_state.command_history.back() == "logs source show",
                 "run/command options should feed the command prompt "
                 "history") &&
         ok;
    ok =
        require(!command_state.log.empty() &&
                    command_state.log.front().source == "show",
                "run/command options should expose show output in snapshots") &&
        ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--snapshot";
    char arg2[] = "--menu";
    char arg3[] = "--run";
    char arg4[] = "runtime";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};
    const CmdOptions parsed = parse_options(5, argv);
    ok = require(parsed.snapshot && parsed.menu && parsed.commands.size() == 1u,
                 "menu snapshot options should compose with a run command") &&
         ok;

    CmdState menu_state{};
    menu_state.screen = ScreenMode::Home;
    ok = require(apply_option_commands(menu_state, parsed, actions),
                 "menu snapshot run command should dispatch before "
                 "presentation") &&
         ok;
    apply_option_presentation(menu_state, parsed);
    ok = require(menu_state.screen == ScreenMode::Runtime &&
                     menu_state.help_view && !menu_state.action_menu_open,
                 "menu presentation should be applied after run commands "
                 "settle the target screen") &&
         ok;
    const std::string menu_after_run =
        make_menu_overlay_snapshot_text(menu_state);
    ok = require(menu_after_run.find("F3  RUNTIME") != std::string::npos &&
                     menu_after_run.find("CURRENT SCREEN") != std::string::npos,
                 "menu snapshot after run should show Runtime as the current "
                 "screen") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--snapshot";
    char arg2[] = "--actions";
    char arg3[] = "--run";
    char arg4[] = "show shell logs";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};
    const CmdOptions parsed = parse_options(5, argv);

    CmdState action_state{};
    action_state.screen = ScreenMode::Home;
    ok = require(apply_option_commands(action_state, parsed, actions),
                 "action snapshot run command should dispatch before "
                 "presentation") &&
         ok;
    apply_option_presentation(action_state, parsed, &actions);
    ok = require(action_state.screen == ScreenMode::Logs &&
                     action_state.action_menu_open && !action_state.help_view,
                 "action presentation should be applied after run commands "
                 "settle the target screen") &&
         ok;
    const std::string actions_after_run =
        make_action_menu_snapshot_text(action_state, actions);
    ok = require(
             actions_after_run.find("Action palette") != std::string::npos &&
                 actions_after_run.find("F8 SHELL LOGS") != std::string::npos,
             "action snapshot after run should keep the palette over Shell "
             "Logs") &&
         ok;
  }
  {
    char arg0[] = "cuwacunu_cmd";
    char arg1[] = "--snapshot";
    char arg2[] = "--actions";
    char arg3[] = "--run";
    char arg4[] = "actions end";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};
    const CmdOptions parsed = parse_options(5, argv);

    CmdState action_state{};
    action_state.screen = ScreenMode::Home;
    ok = require(apply_option_commands(action_state, parsed, actions),
                 "action snapshot selection command should dispatch before "
                 "presentation") &&
         ok;
    apply_option_presentation(action_state, parsed, &actions);
    ok = require(!actions.actions().empty() && action_state.action_menu_open &&
                     action_state.action_menu_selected + 1u ==
                         actions.actions().size(),
                 "action presentation should preserve an explicit end "
                 "selection") &&
         ok;
    const std::string actions_after_select =
        make_action_menu_snapshot_text(action_state, actions);
    ok = require(actions_after_select.find("> exit") != std::string::npos &&
                     actions_after_select.find("iinuji.exit()") !=
                         std::string::npos,
                 "action snapshot should preserve prior palette selection "
                 "commands instead of reopening on the contextual Home row") &&
         ok;
  }
  {
    const std::string catalog = make_command_catalog_snapshot_text(actions);
    const std::string catalog_rule = detail_section_rule();
    ok = require(catalog.find("Command catalog") != std::string::npos,
                 "command catalog should expose a catalog header") &&
         ok;
    ok = require(catalog.find("Generated from the active action registry") !=
                     std::string::npos,
                 "command catalog should identify the registry source") &&
         ok;
    ok = require(catalog.find(catalog_rule + "\n[screen]\n" + catalog_rule) !=
                     std::string::npos,
                 "command catalog should frame screen navigation commands") &&
         ok;
    ok = require(catalog.find(catalog_rule + "\n[show]\n" + catalog_rule) !=
                     std::string::npos,
                 "command catalog should frame show commands") &&
         ok;
    ok = require(catalog.find(catalog_rule + "\n[dynamic]\n" + catalog_rule) !=
                     std::string::npos,
                 "command catalog should frame dynamic selector commands") &&
         ok;
    {
      CmdState current_catalog_state{};
      current_catalog_state.screen = ScreenMode::Runtime;
      current_catalog_state.runtime.focus = RuntimeFocus::Jobs;
      current_catalog_state.runtime.detail_mode = RuntimeDetailMode::Trace;
      const std::string current_catalog =
          make_command_catalog_snapshot_text(actions, &current_catalog_state);
      ok = require(current_catalog.find(catalog_rule + "\n[current]\n" +
                                        catalog_rule) != std::string::npos &&
                       current_catalog.find("screen: F3 RUNTIME") !=
                           std::string::npos &&
                       current_catalog.find("shortcuts:") != std::string::npos,
                   "command catalog should expose a current-screen utility "
                   "header when rendered from the shell") &&
           ok;
      ok = require(current_catalog.find("focus Jobs") != std::string::npos &&
                       current_catalog.find("d devices  J jobs") !=
                           std::string::npos &&
                       current_catalog.find("Tab/Shift-Tab detail") !=
                           std::string::npos &&
                       current_catalog.find("rail [") == std::string::npos,
                   "command catalog current header should mirror the "
                   "screen-local shortcuts without command rail blocks") &&
           ok;

      current_catalog_state.screen = ScreenMode::Home;
      const std::string home_catalog =
          make_command_catalog_snapshot_text(actions, &current_catalog_state);
      ok = require(home_catalog.find("rail [") == std::string::npos,
                   "Home command catalog current header should avoid command "
                   "rail blocks") &&
           ok;

      current_catalog_state.screen = ScreenMode::Logs;
      const std::string logs_catalog =
          make_command_catalog_snapshot_text(actions, &current_catalog_state);
      ok = require(logs_catalog.find("rail [") == std::string::npos,
                   "Shell Logs command catalog current header should avoid "
                   "command rail blocks") &&
           ok;

      current_catalog_state.screen = ScreenMode::Config;
      const std::string config_catalog =
          make_command_catalog_snapshot_text(actions, &current_catalog_state);
      ok = require(config_catalog.find("rail [") == std::string::npos,
                   "Config command catalog current header should avoid command "
                   "rail blocks") &&
           ok;

      current_catalog_state.screen = ScreenMode::Workbench;
      const std::string workbench_catalog =
          make_command_catalog_snapshot_text(actions, &current_catalog_state);
      ok = require(workbench_catalog.find("[current]") != std::string::npos &&
                       workbench_catalog.find("F3 runtime") !=
                           std::string::npos &&
                       workbench_catalog.find("F8 logs  F9 config") !=
                           std::string::npos &&
                       workbench_catalog.find("blank for now") ==
                           std::string::npos &&
                       workbench_catalog.find("Marshal") == std::string::npos &&
                       workbench_catalog.find("marshal") == std::string::npos &&
                       workbench_catalog.find("Human") == std::string::npos &&
                       workbench_catalog.find("human") == std::string::npos,
                   "Workbench command catalog current header should stay "
                   "blank and avoid removed F2 hero names") &&
           ok;
    }
    ok = require(catalog.find("iinuji.screen.home()") != std::string::npos,
                 "command catalog should include canonical screen paths") &&
         ok;
    {
      const std::size_t screen_header = catalog.find("[screen]");
      const std::size_t show_header = catalog.find("[show]");
      ok = require(screen_header != std::string::npos &&
                       show_header != std::string::npos &&
                       screen_header < show_header,
                   "command catalog should separate screen navigation from "
                   "show summaries") &&
           ok;
      const std::string screen_section =
          catalog.substr(screen_header, show_header - screen_header);
      ok = require(screen_section.find("iinuji.show.runtime()") ==
                       std::string::npos,
                   "screen catalog entries should not advertise show paths as "
                   "navigation aliases") &&
           ok;
      ok = require(catalog.find("iinuji.show.runtime()", show_header) !=
                       std::string::npos,
                   "show catalog should include Runtime summary paths") &&
           ok;
      const std::size_t menu_header = catalog.find("[menu]");
      const std::string show_section =
          menu_header == std::string::npos
              ? std::string{}
              : catalog.substr(show_header, menu_header - show_header);
      ok = require(show_section.find("aliases: show lattice | show target | "
                                     "show proof") != std::string::npos,
                   "show catalog should expose natural Lattice target/proof "
                   "aliases") &&
           ok;
      ok = require(
               show_section.find("runtime show") != std::string::npos &&
                   show_section.find("workbench show") != std::string::npos &&
                   show_section.find("proof show") != std::string::npos &&
                   show_section.find("logs show") != std::string::npos,
               "show catalog should expose restored reverse show aliases") &&
           ok;
      ok = require(show_section.find("iinuji.version()") != std::string::npos &&
                       show_section.find("cuwacunu.cmd --version") !=
                           std::string::npos,
                   "show catalog should expose command identity utilities") &&
           ok;
      const auto require_show_alias_order = [&](std::string_view first,
                                                std::string_view second,
                                                const char *message) {
        const std::size_t first_pos = show_section.find(first);
        const std::size_t second_pos = show_section.find(second);
        return require(first_pos != std::string::npos &&
                           second_pos != std::string::npos &&
                           first_pos < second_pos,
                       message);
      };
      ok = require_show_alias_order(
               "cuwacunu_cmd --visual", "cuwacunu.cmd --visual",
               "Home visual catalog aliases should prefer the PATH command") &&
           ok;
      ok = require_show_alias_order(
               "cuwacunu_cmd --splash", "cuwacunu.cmd --splash",
               "Home splash catalog aliases should prefer the PATH command") &&
           ok;
      ok =
          require_show_alias_order(
              "cuwacunu_cmd --farewell", "cuwacunu.cmd --farewell",
              "Home farewell catalog aliases should prefer the PATH command") &&
          ok;
    }
    ok = require(
             catalog.find("cuwacunu.cmd --menu") != std::string::npos &&
                 catalog.find("cuwacunu.cmd --help") != std::string::npos &&
                 catalog.find("cuwacunu.cmd --version") != std::string::npos &&
                 catalog.find("cuwacunu.cmd --actions") != std::string::npos &&
                 catalog.find("cuwacunu.cmd --commands") != std::string::npos &&
                 catalog.find("cuwacunu.cmd --catalog") != std::string::npos &&
                 catalog.find("cuwacunu_cmd --menu") != std::string::npos &&
                 catalog.find("cuwacunu_cmd --help") != std::string::npos &&
                 catalog.find("cuwacunu_cmd --actions") != std::string::npos &&
                 catalog.find("cuwacunu_cmd --commands") != std::string::npos &&
                 catalog.find("cuwacunu_cmd --catalog") != std::string::npos &&
                 catalog.find("iinuji_cmd --menu") != std::string::npos &&
                 catalog.find("iinuji_cmd --help") != std::string::npos &&
                 catalog.find("iinuji_cmd --actions") != std::string::npos &&
                 catalog.find("iinuji_cmd --commands") != std::string::npos &&
                 catalog.find("iinuji_cmd --catalog") != std::string::npos,
             "command catalog should expose shell-style menu, identity, and "
             "action "
             "utilities as typeable prompt aliases") &&
         ok;
    {
      const std::size_t menu_header = catalog.find("[menu]");
      const std::size_t refresh_header = catalog.find("[refresh]");
      const std::string menu_section =
          menu_header == std::string::npos
              ? std::string{}
              : catalog.substr(menu_header, refresh_header - menu_header);
      const auto require_menu_alias_order = [&](std::string_view first,
                                                std::string_view second,
                                                const char *message) {
        const std::size_t first_pos = menu_section.find(first);
        const std::size_t second_pos = menu_section.find(second);
        return require(first_pos != std::string::npos &&
                           second_pos != std::string::npos &&
                           first_pos < second_pos,
                       message);
      };
      ok = require_menu_alias_order(
               "cuwacunu_cmd --help", "cuwacunu.cmd --help",
               "Help catalog aliases should prefer the PATH command") &&
           ok;
      ok = require_menu_alias_order(
               "cuwacunu_cmd --menu", "cuwacunu.cmd --menu",
               "Menu catalog aliases should prefer the PATH command") &&
           ok;
      ok = require_menu_alias_order(
               "cuwacunu_cmd --actions", "cuwacunu.cmd --actions",
               "Action palette catalog aliases should prefer the PATH "
               "command") &&
           ok;
      ok = require_menu_alias_order(
               "cuwacunu_cmd --commands", "cuwacunu.cmd --commands",
               "Command catalog aliases should prefer the PATH command") &&
           ok;
    }
    ok =
        require(catalog.find("waajacamaya") != std::string::npos,
                "command catalog should expose restored Home visual aliases") &&
        ok;
    ok = require(catalog.find("iinuji.home.visual()") != std::string::npos,
                 "command catalog should expose canonical Home visual paths") &&
         ok;
    ok =
        require(
            catalog.find("cuwacunu.cmd --image") != std::string::npos &&
                catalog.find("cuwacunu.cmd --animation") != std::string::npos &&
                catalog.find("cuwacunu.cmd --home-visual") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --waajacamaya") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --image") != std::string::npos &&
                catalog.find("cuwacunu_cmd --animation") != std::string::npos &&
                catalog.find("cuwacunu_cmd --home-visual") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --waajacamaya") !=
                    std::string::npos &&
                catalog.find("iinuji_cmd --image") != std::string::npos &&
                catalog.find("iinuji_cmd --animation") != std::string::npos &&
                catalog.find("iinuji_cmd --home-visual") != std::string::npos &&
                catalog.find("home-visual") != std::string::npos &&
                catalog.find("iinuji_cmd --waajacamaya") != std::string::npos,
            "command catalog should expose restored Home image and "
            "animation shell utilities") &&
        ok;
    ok =
        require(
            catalog.find("iinuji.home.splash()") != std::string::npos &&
                catalog.find("splash | bootstrap | boot | loading | load | "
                             "show splash") != std::string::npos &&
                catalog.find("iinuji.home.farewell()") != std::string::npos &&
                catalog.find("aliases: farewell | closing | show farewell") !=
                    std::string::npos &&
                catalog.find("good luck splash") != std::string::npos &&
                catalog.find("cuwacunu.cmd --visual") != std::string::npos &&
                catalog.find("cuwacunu.cmd --splash") != std::string::npos &&
                catalog.find("cuwacunu.cmd --bootstrap") != std::string::npos &&
                catalog.find("cuwacunu.cmd --boot") != std::string::npos &&
                catalog.find("cuwacunu.cmd --splash=boot") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --splash loading") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --splash=farewell") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --splash farewell") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --splash close") !=
                    std::string::npos &&
                catalog.find("cuwacunu.cmd --splash=good-luck") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --visual") != std::string::npos &&
                catalog.find("cuwacunu_cmd --splash") != std::string::npos &&
                catalog.find("cuwacunu_cmd --bootstrap") != std::string::npos &&
                catalog.find("cuwacunu_cmd --boot") != std::string::npos &&
                catalog.find("cuwacunu_cmd --splash=loading") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --splash=farewell") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --splash farewell") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --splash=good luck") !=
                    std::string::npos &&
                catalog.find("cuwacunu_cmd --splash=good-luck") !=
                    std::string::npos &&
                catalog.find("iinuji_cmd --visual") != std::string::npos &&
                catalog.find("iinuji_cmd --splash") != std::string::npos &&
                catalog.find("iinuji_cmd --bootstrap") != std::string::npos &&
                catalog.find("iinuji_cmd --boot") != std::string::npos &&
                catalog.find("iinuji_cmd --splash loading") !=
                    std::string::npos &&
                catalog.find("iinuji_cmd --splash=farewell") !=
                    std::string::npos &&
                catalog.find("iinuji_cmd --splash farewell") !=
                    std::string::npos,
            "command catalog should expose restored Home splash/farewell "
            "utilities") &&
        ok;
    ok = require(catalog.find("label: show Home visual showcase") !=
                     std::string::npos,
                 "command catalog should expose Home visual as a first-class "
                 "showcase utility") &&
         ok;
    ok =
        require(catalog.find("shell logs") != std::string::npos,
                "command catalog should expose the visible Shell Logs alias") &&
        ok;
    ok = require(catalog.find("[logs]") != std::string::npos,
                 "command catalog should group log utilities") &&
         ok;
    {
      const std::size_t logs_header = catalog.find("[logs]");
      ok = require(logs_header != std::string::npos &&
                       catalog.find("[logs]", logs_header + 1u) ==
                           std::string::npos,
                   "command catalog should group each category once") &&
           ok;
    }
    ok = require(catalog.find("[refresh]") < catalog.find("[logs]"),
                 "command catalog should use a stable grouped utility order") &&
         ok;
    {
      const std::size_t refresh_header = catalog.find("[refresh]");
      const std::size_t logs_header = catalog.find("[logs]");
      ok = require(refresh_header != std::string::npos &&
                       logs_header != std::string::npos,
                   "command catalog should group refresh utilities") &&
           ok;
      const std::string refresh_section =
          catalog.substr(refresh_header, logs_header - refresh_header);
      ok = require(refresh_section.find("iinuji.workbench.refresh()") !=
                       std::string::npos,
                   "command catalog should expose Workbench refresh as its own "
                   "canonical utility") &&
           ok;
      ok = require(refresh_section.find("iinuji.runtime.refresh()") !=
                       std::string::npos,
                   "command catalog should expose Runtime refresh as its own "
                   "canonical utility") &&
           ok;
      ok = require(refresh_section.find("iinuji.lattice.refresh()") !=
                       std::string::npos,
                   "command catalog should expose Lattice refresh as its own "
                   "canonical utility") &&
           ok;
      ok = require(refresh_section.find("iinuji.config.reload()") !=
                       std::string::npos,
                   "command catalog should expose Config reload as its own "
                   "canonical utility") &&
           ok;
    }
    ok = require(catalog.find("iinuji.logs.settings.source.status()") !=
                     std::string::npos,
                 "command catalog should include status source utility") &&
         ok;
    ok = require(catalog.find("iinuji.logs.settings.source.next()") !=
                     std::string::npos,
                 "command catalog should expose the logs source cycle utility "
                 "as a canonical path") &&
         ok;
    ok = require(catalog.find("iinuji.logs.clear()") != std::string::npos,
                 "command catalog should expose the canonical Shell Logs clear "
                 "path") &&
         ok;
    ok = require(catalog.find("\n  logs filter\n") == std::string::npos,
                 "command catalog should not use natural log aliases as "
                 "top-level command paths") &&
         ok;
    ok = require(catalog.find("iinuji.logs.settings.source.show()") !=
                     std::string::npos,
                 "command catalog should include show source utility") &&
         ok;
    {
      const std::size_t config_header = catalog.find("[config]");
      ok = require(config_header != std::string::npos,
                   "command catalog should group Config utilities") &&
           ok;
      ok = require(config_header != std::string::npos &&
                       catalog.find("iinuji.config.files()", config_header) !=
                           std::string::npos,
                   "command catalog should expose Config files utility under "
                   "Config") &&
           ok;
      ok = require(config_header != std::string::npos &&
                       catalog.find("iinuji.config.show()", config_header) !=
                           std::string::npos,
                   "command catalog should expose Config show utility under "
                   "Config") &&
           ok;
      ok = require(catalog.find("aliases: show config | config show | show "
                                "selected config",
                                config_header) != std::string::npos,
                   "command catalog should expose natural Config show "
                   "aliases") &&
           ok;
      ok = require(config_header != std::string::npos &&
                       catalog.find("iinuji.config.file.show()",
                                    config_header) != std::string::npos,
                   "command catalog should expose Config file show utility "
                   "under Config") &&
           ok;
    }
    ok = require(catalog.find("iinuji.config.file.index.n1()") !=
                     std::string::npos,
                 "command catalog should include legacy dynamic config index "
                 "pattern") &&
         ok;
    ok = require(
             catalog.find("iinuji.config.file.index.N()") != std::string::npos,
             "command catalog should retain generic config index pattern") &&
         ok;
    ok = require(catalog.find("config file 1 | file n1") != std::string::npos,
                 "command catalog should include natural config selectors") &&
         ok;
    ok = require(catalog.find("iinuji.config.file.index(n=1)") !=
                     std::string::npos,
                 "command catalog should include config index argument form") &&
         ok;
    ok = require(catalog.find("iinuji.lattice.target.index.n1()") !=
                     std::string::npos,
                 "command catalog should include legacy dynamic lattice index "
                 "pattern") &&
         ok;
    ok = require(catalog.find("iinuji.lattice.target.id.TOKEN()") !=
                     std::string::npos,
                 "command catalog should include dynamic lattice id pattern") &&
         ok;
    ok = require(catalog.find("lattice target 1 | target n1") !=
                     std::string::npos,
                 "command catalog should include natural lattice selectors") &&
         ok;
    ok =
        require(catalog.find("iinuji.lattice.target.index(n=1)") !=
                    std::string::npos,
                "command catalog should include lattice index argument form") &&
        ok;
    ok = require(
             catalog.find("marshal | inbox | human") == std::string::npos &&
                 catalog.find("iinuji.screen.marshal()") == std::string::npos &&
                 catalog.find("iinuji.screen.inbox()") == std::string::npos &&
                 catalog.find("iinuji.screen.human()") == std::string::npos,
             "command catalog should not advertise removed F2 screen "
             "hero names") &&
         ok;
    ok = require(catalog.find("iinuji.show.marshal()") == std::string::npos &&
                     catalog.find("iinuji.show.inbox()") == std::string::npos &&
                     catalog.find("iinuji.show.human()") == std::string::npos,
                 "command catalog should not advertise removed F2 show "
                 "hero names") &&
         ok;
    ok = require(catalog.find("iinuji.commands.run.selected()") !=
                     std::string::npos,
                 "command catalog should include selected action run path") &&
         ok;
    ok = require(catalog.find("label: run selected palette action") !=
                         std::string::npos &&
                     catalog.find("run selected action palette row") ==
                         std::string::npos,
                 "command catalog should use the same compact palette-run copy "
                 "as help and the action overlay") &&
         ok;
    {
      const std::size_t menu_header = catalog.find("[menu]");
      const std::size_t refresh_header = catalog.find("[refresh]");
      ok = require(menu_header != std::string::npos &&
                       refresh_header != std::string::npos,
                   "command catalog should group palette utilities under "
                   "menu") &&
           ok;
      ok = require(catalog.find("iinuji.actions.run()", menu_header) !=
                           std::string::npos &&
                       catalog.find("iinuji.actions.run()", menu_header) <
                           refresh_header,
                   "command catalog should expose the palette run path inside "
                   "the menu group") &&
           ok;
      ok = require(catalog.find("cuwacunu.cmd --snapshot --menu",
                                menu_header) != std::string::npos &&
                       catalog.find("cuwacunu_cmd --snapshot --full",
                                    menu_header) != std::string::npos &&
                       catalog.find("cuwacunu.cmd --snapshot --full",
                                    menu_header) != std::string::npos &&
                       catalog.find("cuwacunu_cmd --snapshot --screen runtime",
                                    menu_header) != std::string::npos &&
                       catalog.find("cuwacunu.cmd --snapshot --screen runtime",
                                    menu_header) != std::string::npos &&
                       catalog.find("cuwacunu_cmd hero.runtime.dsl --snapshot",
                                    menu_header) != std::string::npos &&
                       catalog.find("cuwacunu.cmd hero.runtime.dsl --snapshot",
                                    menu_header) != std::string::npos,
                   "command catalog should expose snapshot-wrapped shell "
                   "utilities inside the menu group") &&
           ok;
      ok = require(
               catalog.find("cuwacunu_cmd --snapshot --actions", menu_header) !=
                       std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --actions",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --visual",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --visual",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --image",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --image",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --animation",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --animation",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --catalog",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --catalog",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --splash loading",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --splash loading",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --splash close",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --splash close",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu_cmd --snapshot --splash "
                                "good-luck",
                                menu_header) != std::string::npos &&
                   catalog.find("cuwacunu.cmd --snapshot --splash "
                                "good-luck",
                                menu_header) != std::string::npos,
               "command catalog should advertise shell snapshot utilities for "
               "menu, palette, catalog, and Home visual/splash "
               "utilities") &&
           ok;
      ok = require(
               catalog.find("cuwacunu_cmd --snapshot --screen runtime",
                            menu_header) <
                       catalog.find("cuwacunu.cmd --snapshot --screen runtime",
                                    menu_header) &&
                   catalog.find("cuwacunu_cmd --snapshot --full", menu_header) <
                       catalog.find("cuwacunu.cmd --snapshot --full",
                                    menu_header) &&
                   catalog.find("cuwacunu_cmd hero.runtime.dsl --snapshot",
                                menu_header) <
                       catalog.find("cuwacunu.cmd hero.runtime.dsl "
                                    "--snapshot",
                                    menu_header),
               "command catalog should prefer PATH command forms before "
               "cuwacunu.cmd shell snapshot forms") &&
           ok;
      const std::size_t action_wrapper =
          catalog.find("cuwacunu.cmd --snapshot --actions", menu_header);
      const std::size_t catalog_wrapper =
          catalog.find("cuwacunu.cmd --snapshot --catalog", menu_header);
      const std::size_t visual_wrapper =
          catalog.find("cuwacunu.cmd --snapshot --visual", menu_header);
      ok = require(action_wrapper != std::string::npos &&
                       catalog_wrapper != std::string::npos &&
                       visual_wrapper != std::string::npos &&
                       action_wrapper < catalog_wrapper &&
                       catalog_wrapper < visual_wrapper,
                   "command catalog should list the catalog snapshot as its "
                   "own shell utility, not as an action-palette alias") &&
           ok;
      const std::size_t path_action_wrapper =
          catalog.find("cuwacunu_cmd --snapshot --actions", menu_header);
      const std::size_t path_catalog_wrapper =
          catalog.find("cuwacunu_cmd --snapshot --catalog", menu_header);
      const std::size_t path_visual_wrapper =
          catalog.find("cuwacunu_cmd --snapshot --visual", menu_header);
      ok = require(path_action_wrapper != std::string::npos &&
                       path_catalog_wrapper != std::string::npos &&
                       path_visual_wrapper != std::string::npos &&
                       path_action_wrapper < path_catalog_wrapper &&
                       path_catalog_wrapper < path_visual_wrapper,
                   "command catalog should list PATH command snapshots in the "
                   "same utility order") &&
           ok;
      ok = require(catalog.find("label: render command catalog snapshot") !=
                       std::string::npos,
                   "command catalog should describe --snapshot --catalog as a "
                   "catalog renderer") &&
           ok;
      ok = require(
               catalog.find("cuwacunu.cmd --actions --snapshot | cuwacunu.cmd "
                            "--snapshot --palette | cuwacunu.cmd --snapshot "
                            "--catalog") == std::string::npos,
               "command catalog should not describe --snapshot --catalog "
               "as an action-palette alias") &&
           ok;
      ok = require(catalog.find("[action-run]") == std::string::npos,
                   "command catalog should not split palette run paths into a "
                   "separate group") &&
           ok;
    }
    ok =
        require(catalog.find("help page down") != std::string::npos,
                "command catalog should include natural help scroll aliases") &&
        ok;
    ok = require(catalog.find("actions down") != std::string::npos &&
                     catalog.find("commands down") != std::string::npos,
                 "command catalog should include natural palette movement "
                 "aliases") &&
         ok;
    ok = require(catalog.find("aliases: a | actions") != std::string::npos,
                 "command catalog should expose the legacy one-key action "
                 "palette alias") &&
         ok;
    ok = require(catalog.find("logs page down") != std::string::npos,
                 "command catalog should include natural log scroll aliases") &&
         ok;
    ok = require(catalog.find("runtime down") != std::string::npos,
                 "command catalog should include natural Runtime browser "
                 "aliases") &&
         ok;
    ok = require(catalog.find("config down") != std::string::npos,
                 "command catalog should include natural Config browser "
                 "aliases") &&
         ok;
    ok = require(catalog.find("lattice down") != std::string::npos,
                 "command catalog should include natural Lattice browser "
                 "aliases") &&
         ok;
    {
      const std::size_t workspace_header = catalog.find("[workspace]");
      const std::size_t app_header = catalog.find("[app]");
      ok = require(workspace_header != std::string::npos,
                   "command catalog should group workspace utilities") &&
           ok;
      const std::string workspace_section =
          workspace_header == std::string::npos
              ? std::string{}
              : catalog.substr(workspace_header, app_header - workspace_header);
      ok = require(workspace_section.find("cuwacunu.cmd --full") !=
                           std::string::npos &&
                       workspace_section.find("cuwacunu_cmd --zoom") !=
                           std::string::npos &&
                       workspace_section.find("iinuji_cmd --full") !=
                           std::string::npos,
                   "command catalog should expose shell-style full/zoom "
                   "workspace utilities") &&
           ok;
    }
    {
      const std::size_t app_header = catalog.find("[app]");
      ok = require(app_header != std::string::npos,
                   "command catalog should group app lifecycle utilities") &&
           ok;
      ok = require(app_header != std::string::npos &&
                       catalog.find("iinuji.quit()", app_header) !=
                           std::string::npos,
                   "command catalog should expose canonical quit utility") &&
           ok;
      ok = require(app_header != std::string::npos &&
                       catalog.find("iinuji.exit()", app_header) !=
                           std::string::npos,
                   "command catalog should expose canonical exit utility as "
                   "its own path") &&
           ok;
    }
  }
  {
    const std::string splash_snapshot = make_splash_snapshot_text(
        SplashSnapshotMode::Bootstrap, gui_config_path);
    const std::string splash_rule = detail_section_rule();
    ok =
        require(splash_snapshot.rfind("loading cuwacunu_cmd", 0) == 0,
                "bootstrap splash snapshot should use the primary PATH command "
                "identity") &&
        ok;
    const std::string path_splash_snapshot = make_splash_snapshot_text(
        SplashSnapshotMode::Bootstrap, gui_config_path, "cuwacunu_cmd");
    ok = require(path_splash_snapshot.rfind("loading cuwacunu_cmd", 0) == 0,
                 "bootstrap splash snapshot should use the invoked PATH "
                 "command name when provided") &&
         ok;
    ok = require(splash_snapshot.find("Splash snapshot") != std::string::npos,
                 "splash snapshot should expose the visual surface") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nwaajacamaya\n" +
                                      splash_rule + "\n  image panel:") !=
                     std::string::npos,
                 "splash snapshot should frame the waajacamaya panel") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nAsset report\n" +
                                      splash_rule) != std::string::npos,
                 "splash snapshot should frame image asset diagnostics") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nPreview image\n" +
                                      splash_rule) != std::string::npos,
                 "splash snapshot should frame the visual image preview") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nbootstrap\n" +
                                      splash_rule) != std::string::npos,
                 "splash snapshot should frame bootstrap progress") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nScreens\n" +
                                      splash_rule) != std::string::npos,
                 "splash snapshot should frame screen shortcuts") &&
         ok;
    ok = require(splash_snapshot.find(splash_rule + "\nStatus\n" + splash_rule +
                                      "\n waajacu™ | bootstrap in "
                                      "progress") != std::string::npos,
                 "bootstrap splash snapshot should preserve the legacy branded "
                 "status strip") &&
         ok;
    ok = require(splash_snapshot.find("image panel: waajacamaya") !=
                     std::string::npos,
                 "splash snapshot should use the restored waajacamaya panel "
                 "label") &&
         ok;
    ok = require(splash_snapshot.find("Asset report") != std::string::npos,
                 "splash snapshot should expose image asset diagnostics") &&
         ok;
    ok = require(splash_snapshot.find("image_box") != std::string::npos,
                 "splash snapshot should name the image primitive") &&
         ok;
    ok = require(splash_snapshot.find("animation primitive") !=
                     std::string::npos,
                 "splash snapshot should name the animation primitive") &&
         ok;
    ok =
        require(splash_snapshot.find("relative/home.apng") != std::string::npos,
                "splash snapshot should expose configured animation path") &&
        ok;
    ok = require(splash_snapshot.find("status: unavailable") !=
                     std::string::npos,
                 "splash snapshot should report unavailable configured test "
                 "assets") &&
         ok;
    ok = require(splash_snapshot.find("Preview image") != std::string::npos,
                 "splash snapshot should render the visual image preview") &&
         ok;
    ok = require(
             splash_snapshot.find("source: fallback wordmark") !=
                 std::string::npos,
             "splash snapshot should describe the preview fallback source") &&
         ok;
    ok = require(
             contains_non_ascii(splash_snapshot),
             "splash snapshot should include rendered terminal image glyphs") &&
         ok;
  }
  {
    const std::string farewell_snapshot = make_splash_snapshot_text(
        SplashSnapshotMode::Farewell, state.global_config_path);
    ok = require(farewell_snapshot.find("waajacamaya.png") != std::string::npos,
                 "farewell splash should expose the legacy bundled closing "
                 "image path") &&
         ok;
    ok = require(farewell_snapshot.find("source: closing logo") !=
                     std::string::npos,
                 "farewell splash should render from the closing logo instead "
                 "of sharing the loading logo") &&
         ok;
    ok = require(farewell_snapshot.find(detail_section_rule() + "\nStatus\n" +
                                        detail_section_rule() +
                                        "\n waajacu™ | closing terminal") !=
                     std::string::npos,
                 "farewell splash snapshot should preserve the legacy branded "
                 "status strip") &&
         ok;
    ok = require(
             farewell_snapshot.find(detail_section_rule() + "\nwaajacamaya\n" +
                                    detail_section_rule() +
                                    "\n  image panel:") != std::string::npos,
             "farewell splash snapshot should use the restored "
             "waajacamaya panel label") &&
         ok;
  }
  {
    const std::string visual_snapshot =
        make_home_visual_snapshot_text(gui_config_path);
    const std::string visual_rule = detail_section_rule();
    ok = require(visual_snapshot.find("Home visual snapshot") !=
                     std::string::npos,
                 "Home visual snapshot should expose the visual surface") &&
         ok;
    ok = require(visual_snapshot.find(visual_rule + "\nHome visual\n" +
                                      visual_rule) != std::string::npos,
                 "Home visual snapshot should frame the visual summary") &&
         ok;
    ok = require(visual_snapshot.find(visual_rule + "\nAssets\n" +
                                      visual_rule) != std::string::npos,
                 "Home visual snapshot should frame asset paths") &&
         ok;
    ok = require(visual_snapshot.find(visual_rule + "\nPreview image\n" +
                                      visual_rule) != std::string::npos,
                 "Home visual snapshot should frame the rendered preview") &&
         ok;
    ok = require(visual_snapshot.find("image_box + APNG/GIF/PNG") !=
                     std::string::npos,
                 "Home visual snapshot should name the image and animation "
                 "primitive") &&
         ok;
    ok = require(visual_snapshot.find("preview frame 1/1") != std::string::npos,
                 "Home visual snapshot should render at least one frame") &&
         ok;
    ok = require(contains_non_ascii(visual_snapshot),
                 "Home visual snapshot should include rendered terminal image "
                 "glyphs") &&
         ok;
  }
  ok = require(make_operator_state_line(state).find("F1 HOME") !=
                   std::string::npos,
               "operator status should use restored F-key screen badges") &&
       ok;
  {
    CmdState status_state = state;
    status_state.screen = ScreenMode::Runtime;
    status_state.status = "runtime screen refresh queued";
    const std::string status_line = make_operator_state_line(status_state);
    ok = require(status_line.find("runtime screen refresh queued") !=
                     std::string::npos,
                 "operator status should not truncate restored Runtime refresh "
                 "statuses") &&
         ok;
    ok = require(status_line.find("runtime screen re~") == std::string::npos,
                 "operator status should avoid rough tilde truncation for "
                 "common queued statuses") &&
         ok;
  }
  {
    CmdState status_state = state;
    status_state.help_view = true;
    status_state.status = "help overlay=open (Esc or click [x] to close)";
    const std::string status_line = make_operator_state_line(status_state);
    ok =
        require(status_line.find("help overlay") != std::string::npos,
                "operator status should keep long help-open status readable") &&
        ok;
    ok = require(status_line.find('~') == std::string::npos,
                 "operator status should avoid rough tilde truncation for the "
                 "restored help overlay affordance") &&
         ok;
  }
  ok = require(make_operator_state_line(state).find("fixed") ==
                   std::string::npos,
               "Home operator status should describe the visual surface, not "
               "a fixed workspace") &&
       ok;
  {
    CmdState visual_state = state;
    visual_state.home_visual.loading_logo_path =
        "/cuwacunu/src/resources/waajacamaya_hello.png";
    visual_state.home_visual.closing_logo_path =
        "/cuwacunu/src/resources/waajacamaya.png";
    visual_state.home_visual.home_animation_path =
        "/cuwacunu/src/resources/waajacamaya.apng";
    visual_state.home_visual.loading_logo_ok = true;
    visual_state.home_visual.animation_ok = true;
    visual_state.home_visual.animation_dynamic = true;
    visual_state.home_visual.animation_frames = 127u;
    visual_state.home_visual.animation_width = 274;
    visual_state.home_visual.animation_height = 290;
    ok = require(make_operator_state_line(visual_state)
                         .find("visual=animated frames=127") !=
                     std::string::npos,
                 "Home operator status should report loaded animation "
                 "frames") &&
         ok;
    ok = require(make_detail_text(visual_state)
                         .find("Home visual: visual=animated frames=127") !=
                     std::string::npos,
                 "Home detail should report loaded animation frames") &&
         ok;
    ok = require(make_detail_text(visual_state).find("Home assets") !=
                     std::string::npos,
                 "Home detail should expose GUI visual asset diagnostics") &&
         ok;
    ok = require(make_detail_text(visual_state)
                         .find("waajacamaya.apng [loaded frames=127 "
                               "dynamic=yes size=274x290") != std::string::npos,
                 "Home detail should expose APNG path, status, frame count, "
                 "and size") &&
         ok;
    ok = require(make_detail_text(visual_state).find("waajacamaya.png") !=
                     std::string::npos,
                 "Home detail should expose the legacy farewell image path") &&
         ok;
  }
  {
    CmdState unavailable_visual_state{};
    refresh_home_visual_state(unavailable_visual_state, gui_config_path);
    const std::string unavailable_detail =
        make_detail_text(unavailable_visual_state);
    ok = require(unavailable_detail.find("relative/home.apng") !=
                     std::string::npos,
                 "Home detail should expose configured animation paths even "
                 "when assets are unavailable") &&
         ok;
    ok = require(unavailable_detail.find("unavailable") != std::string::npos,
                 "Home detail should expose unavailable visual asset status") &&
         ok;
  }
  ok = require(make_operator_hint_line(state).find("waajacu™") !=
                   std::string::npos,
               "home operator hint should preserve the waajacu brand note") &&
       ok;
  ok = require(make_operator_hint_line(state).find("home | ready") !=
                   std::string::npos,
               "home operator hint should restore the legacy bottom-line "
               "cadence") &&
       ok;
  ok = require(make_operator_hint_line(state).find("F4 lattice") !=
                   std::string::npos,
               "home operator hint should advertise the F4 Lattice shortcut") &&
       ok;
  ok = require(make_operator_hint_line(state).find("F9 config") !=
                   std::string::npos,
               "home operator hint should advertise the F9 Config shortcut") &&
       ok;
  ok = require(make_operator_hint_line(state).find("? menu") ==
                   std::string::npos,
               "home operator hint should keep the clean legacy footer instead "
               "of duplicating utility hints") &&
       ok;
  {
    CmdState workbench_hint_state = state;
    workbench_hint_state.screen = ScreenMode::Workbench;
    ok = require(
             make_operator_hint_line(workbench_hint_state).find("F4 lattice") !=
                 std::string::npos,
             "Workbench operator hint should expose the F4 Lattice exit "
             "from the blank cockpit") &&
         ok;
    ok = require(
             make_operator_hint_line(workbench_hint_state).find("F9 config") !=
                 std::string::npos,
             "Workbench operator hint should expose the F9 Config exit "
             "from the blank cockpit") &&
         ok;
    ok =
        require(
            make_operator_hint_line(workbench_hint_state).find("a / actions") !=
                std::string::npos,
            "Workbench operator hint should preserve the restored typed "
            "action shortcut spelling") &&
        ok;
    ok = require(
             make_operator_hint_line(workbench_hint_state).find("h/H/? menu") !=
                 std::string::npos,
             "Workbench operator hint should advertise both restored help "
             "keys") &&
         ok;
    ok =
        require(make_operator_hint_line(workbench_hint_state)
                        .find("type/: command") != std::string::npos,
                "Workbench operator hint should advertise direct typed command "
                "entry") &&
        ok;
  }
  {
    CmdState hint_state = state;
    hint_state.screen = ScreenMode::Runtime;
    hint_state.runtime.focus = RuntimeFocus::Devices;
    hint_state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    ok = require(make_operator_hint_line(hint_state).find("lane=Devices") !=
                     std::string::npos,
                 "Runtime device hint should restore the legacy lane prefix") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("lower panel shows "
                               "live history") != std::string::npos,
                 "Runtime device hint should preserve the legacy live-history "
                 "cue") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Tab/Shift-Tab cycles manifest/log/trace") !=
                     std::string::npos,
                 "Runtime device hint should restore the legacy Tab detail "
                 "cycle wording") &&
         ok;
    ok = require(
             make_operator_hint_line(hint_state).find("Esc restores split") !=
                 std::string::npos,
             "Runtime device hint should advertise the legacy Esc split "
             "shortcut") &&
         ok;
    hint_state.runtime.focus = RuntimeFocus::Jobs;
    ok = require(make_operator_hint_line(hint_state).find("lane=Jobs") !=
                     std::string::npos,
                 "Runtime job hint should restore the legacy Jobs lane "
                 "prefix") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Enter opens file menu; a opens actions") !=
                     std::string::npos,
                 "Runtime job hint should distinguish Enter menu from "
                 "action utility access") &&
         ok;

    hint_state.screen = ScreenMode::Logs;
    hint_state.logs.auto_follow = true;
    hint_state.logs.source_filter = LogsSourceFilter::All;
    hint_state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
    ok = require(
             make_operator_hint_line(hint_state).find("logs | filter=INFO+") !=
                 std::string::npos,
             "Shell Logs hint should restore the legacy logs bottom line") &&
         ok;
    ok = require(make_operator_hint_line(hint_state).find("filter=any") ==
                     std::string::npos,
                 "Shell Logs hint should keep filter tied to severity, not the "
                 "source lens") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Up/Down settings. Left/Right changes") !=
                     std::string::npos,
                 "Shell Logs hint should expose selected-setting navigation "
                 "and change keys") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("f/z toggles full screen. Esc restores split") !=
                     std::string::npos,
                 "Shell Logs hint should advertise restored fullscreen and "
                 "Esc split shortcuts") &&
         ok;

    hint_state.screen = ScreenMode::Config;
    hint_state.config.files = {
        ConfigFileSummary{.relative_path = "alpha.dsl"},
        ConfigFileSummary{.relative_path = "beta.dsl"},
    };
    hint_state.config.selected_file = 1u;
    ok = require(
             make_operator_hint_line(hint_state).find("config | files focus") !=
                 std::string::npos,
             "Config hint should restore the legacy files-focus bottom "
             "line") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Enter/e opens read-only preview") !=
                     std::string::npos,
                 "Config hint should describe the current read-only preview "
                 "utility") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("f/z toggles full screen. Esc restores split") !=
                     std::string::npos,
                 "Config hint should advertise the restored full/split "
                 "workspace utility") &&
         ok;
    ok = require(make_screen_menu_text(ScreenMode::Config)
                             .find("read-only managed Config file preview") !=
                         std::string::npos &&
                     make_screen_menu_text(ScreenMode::Config)
                             .find("writes return") == std::string::npos,
                 "Config compact menu should describe read-only preview "
                 "without internal write-back copy") &&
         ok;
    ok = require(make_operator_state_line(hint_state).find("split") !=
                     std::string::npos,
                 "Config status should expose the current workspace split "
                 "mode") &&
         ok;
    ok = require(make_screen_menu_text(ScreenMode::Config)
                         .find("f/z full  Esc split") != std::string::npos,
                 "Config compact menu should advertise the restored "
                 "full/split shortcut") &&
         ok;
    ok =
        require(
            make_screen_menu_text(ScreenMode::Config).find("Enter/e preview") !=
                std::string::npos,
            "Config compact menu should advertise Enter/e preview") &&
        ok;
    ok = require(make_nav_text(hint_state).find("f/z zoom  Esc split") !=
                     std::string::npos,
                 "Config nav current block should advertise the restored "
                 "zoom shortcut") &&
         ok;

    hint_state.screen = ScreenMode::Lattice;
    hint_state.lattice.target_ids = {"target.alpha", "target.beta"};
    hint_state.lattice.selected_target = 1u;
    ok = require(make_operator_hint_line(hint_state).find("section=targets") !=
                     std::string::npos,
                 "Lattice hint should restore the legacy section prefix") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Home/End jump "
                               "edges") != std::string::npos,
                 "Lattice hint should advertise edge navigation utility") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("Enter shows target proof") != std::string::npos,
                 "Lattice hint should advertise the restored Enter target "
                 "preview utility") &&
         ok;
    ok = require(make_operator_hint_line(hint_state)
                         .find("f/z toggles full screen. Esc restores split") !=
                     std::string::npos,
                 "Lattice hint should advertise the restored full/split "
                 "workspace utility") &&
         ok;
    ok = require(make_operator_state_line(hint_state).find("split") !=
                     std::string::npos,
                 "Lattice status should expose the current workspace split "
                 "mode") &&
         ok;
    ok = require(make_screen_menu_text(ScreenMode::Lattice)
                         .find("f/z full  Esc split") != std::string::npos,
                 "Lattice compact menu should advertise the restored "
                 "full/split shortcut") &&
         ok;
    ok =
        require(make_screen_menu_text(ScreenMode::Lattice)
                        .find("Enter show target") != std::string::npos,
                "Lattice compact menu should advertise Enter target preview") &&
        ok;
    ok = require(make_nav_text(hint_state).find("f/z zoom  Esc split") !=
                     std::string::npos,
                 "Lattice nav current block should advertise the restored "
                 "zoom shortcut") &&
         ok;
    ok = require(make_nav_text(hint_state).find("Enter show target") !=
                     std::string::npos,
                 "Lattice nav current block should advertise restored Enter "
                 "target preview") &&
         ok;
    ok = require(make_nav_text(hint_state).find("r refresh") !=
                     std::string::npos,
                 "Lattice nav current block should advertise screen-local "
                 "refresh key") &&
         ok;
  }
  ok = require(make_home_text(state).find("READY") == std::string::npos,
               "home main text should not reserve a READY overlay") &&
       ok;
  ok = require(make_home_text(state).find(
                   animated_rule(state.animation_tick)) == std::string::npos,
               "home main text should not show the animated ASCII rule") &&
       ok;
  ok = require(make_home_text(state).find("Visual\n") == std::string::npos &&
                   make_home_text(state).find("APNG animation") ==
                       std::string::npos &&
                   make_home_text(state).find("visual=wordmark") ==
                       std::string::npos,
               "home main text should keep visual diagnostics in disclosures, "
               "not the primary overlay") &&
       ok;
  ok =
      require(make_home_text(state).find("F3   Runtime") != std::string::npos &&
                  make_home_text(state).find("F8   Shell Logs") !=
                      std::string::npos,
              "home main text should expose restored legacy F-key shortcut "
              "rows") &&
      ok;
  ok =
      require(make_home_text(state).find("h/H / ? / help") != std::string::npos,
              "home utility block should expose the restored h and H help "
              "aliases") &&
      ok;
  ok = require(make_home_text(state).find("waajacu.com") != std::string::npos,
               "home main text should keep the visual footer") &&
       ok;
  ok = require(make_home_text(state).find("exec:") == std::string::npos,
               "home main text should stay visual-first instead of report "
               "first") &&
       ok;
  ok = require(make_home_text(state).find("config keys:") == std::string::npos,
               "home main text should leave GUI config diagnostics in "
               "disclosures") &&
       ok;
  ok = require(make_home_text(state).find("reserved") == std::string::npos,
               "home main text should keep the primary overlay sparse like the "
               "legacy Home") &&
       ok;
  ok = require(make_home_text(state).find("[visual]") == std::string::npos &&
                   make_home_text(state).find("iinuji.home.visual()/") ==
                       std::string::npos,
               "home main text should avoid visible command rail blocks") &&
       ok;
  ok = require(make_home_text(state).find("Nav:") == std::string::npos,
               "home main text should not label the utility rail as Nav") &&
       ok;
  ok = require(make_detail_text(state).find("iinuji_home_animation_path") !=
                   std::string::npos,
               "home detail should name the GUI animation config key") &&
       ok;
  ok = require(
           make_detail_text(state).find("disclosures") == 0u,
           "home detail content should use the restored disclosures label") &&
       ok;
  const std::string home_detail_rule = detail_section_rule();
  ok = require(make_detail_text(state).find(
                   home_detail_rule + "\nHome showcase\n" + home_detail_rule) !=
                   std::string::npos,
               "home detail should restore section-rule framing around the "
               "showcase disclosure") &&
       ok;
  ok = require(make_detail_text(state).find(
                   home_detail_rule + "\nHome assets\n" + home_detail_rule) !=
                   std::string::npos,
               "home detail should restore section-rule framing around GUI "
               "asset diagnostics") &&
       ok;
  ok = require(make_detail_text(state).find("Runtime\n") != std::string::npos,
               "home detail should carry Runtime summary") &&
       ok;
  ok = require(make_detail_text(state).find("Active wave") != std::string::npos,
               "home detail should carry active wave summary") &&
       ok;
  state.screen = ScreenMode::Home;
  state.home_visual.presentation = HomePresentationMode::Showcase;
  {
    const std::string home_snapshot =
        make_snapshot_text_with_visuals(state, gui_config_path);
    ok = require(home_snapshot.find("Status\n") != std::string::npos,
                 "snapshot should include the operator status block") &&
         ok;
    ok = require(home_snapshot.find(detail_section_rule() + "\nStatus\n" +
                                    detail_section_rule()) != std::string::npos,
                 "snapshot should frame the operator status block") &&
         ok;
    ok = require(
             home_snapshot.find("waajacu.com") != std::string::npos,
             "home snapshot should expose the restored waajacu.com footer") &&
         ok;
    ok = require(home_snapshot.find("waajacamaya + Cuwacunu wordmark") !=
                     std::string::npos,
                 "home snapshot should expose image/wordmark showcase text") &&
         ok;
    const std::size_t visual_preview_pos = home_snapshot.find("Visual preview");
    const std::size_t shortcut_pos = home_snapshot.find("F1   Home");
    ok = require(home_snapshot.find("waajacamaya\n" + home_detail_rule +
                                    "\nVisual preview\n" + home_detail_rule) !=
                     std::string::npos,
                 "home snapshot should frame the restored visual preview "
                 "inside the waajacamaya panel") &&
         ok;
    ok = require(home_snapshot.find("\nREADY\n") == std::string::npos &&
                     visual_preview_pos != std::string::npos &&
                     shortcut_pos != std::string::npos &&
                     visual_preview_pos < shortcut_pos,
                 "home snapshot should mirror the interactive Home layout by "
                 "rendering the image stage before shortcut text") &&
         ok;
    ok = require(home_snapshot.find("F1   Home") != std::string::npos &&
                     home_snapshot.find("F9   Config") != std::string::npos,
                 "home snapshot should expose legacy-style visual F-key "
                 "shortcut rows") &&
         ok;
    ok = require(home_snapshot.find(home_detail_rule + "\nRuntime\n" +
                                    home_detail_rule + "\n  exec:") !=
                     std::string::npos,
                 "home snapshot should move Runtime summary to detail") &&
         ok;
    ok = require(home_snapshot.find("disclosures\n\n" + home_detail_rule +
                                    "\nHome showcase\n" + home_detail_rule) !=
                     std::string::npos,
                 "home snapshot should expose framed disclosures content") &&
         ok;
    ok = require(home_snapshot.find("Visual preview") != std::string::npos,
                 "home snapshot should include a rendered visual preview") &&
         ok;
    ok = require(home_snapshot.find(home_detail_rule + "\nPreview image\n" +
                                    home_detail_rule) != std::string::npos,
                 "home snapshot visual preview should frame the rendered "
                 "image stage") &&
         ok;
    ok = require(home_snapshot.find("primitive: image_box + APNG/GIF/PNG") !=
                     std::string::npos,
                 "home snapshot visual preview should name the restored image "
                 "primitive") &&
         ok;
    ok = require(home_snapshot.find("preview frame 1/") != std::string::npos,
                 "home snapshot visual preview should expose animation frame "
                 "context") &&
         ok;
    ok = require(contains_non_ascii(home_snapshot),
                 "home snapshot visual preview should include rendered image "
                 "glyphs") &&
         ok;
    {
      CmdState splash_state = state;
      splash_state.home_visual.presentation =
          HomePresentationMode::BootstrapSplash;
      const std::string splash_home_snapshot = make_snapshot_text_with_visuals(
          splash_state, state.global_config_path);
      ok = require(
               splash_home_snapshot.find(
                   "waajacamaya\n" + home_detail_rule + "\nSplash preview\n" +
                   home_detail_rule) != std::string::npos &&
                   splash_home_snapshot.find("mode: bootstrap") !=
                       std::string::npos &&
                   splash_home_snapshot.find("source: home animation frame") !=
                       std::string::npos,
               "Home splash command snapshots should render the bootstrap "
               "splash visual instead of the normal visual preview") &&
           ok;
    }
    {
      CmdState farewell_state = state;
      farewell_state.home_visual.presentation =
          HomePresentationMode::FarewellSplash;
      const std::string farewell_home_snapshot =
          make_snapshot_text_with_visuals(farewell_state,
                                          state.global_config_path);
      ok = require(farewell_home_snapshot.find(
                       "waajacamaya\n" + home_detail_rule +
                       "\nSplash preview\n" + home_detail_rule) !=
                           std::string::npos &&
                       farewell_home_snapshot.find("mode: farewell") !=
                           std::string::npos &&
                       farewell_home_snapshot.find("source: closing logo") !=
                           std::string::npos,
                   "Home farewell command snapshots should render the closing "
                   "splash visual instead of the normal visual preview") &&
           ok;
    }
    const CmdUiAssetPaths default_asset_paths =
        resolve_cmd_ui_asset_paths(state.global_config_path);
    if (std::filesystem::exists(default_asset_paths.home_animation)) {
      const HomeShowcaseAssets default_assets = load_home_showcase_assets(
          default_asset_paths.loading_logo.string(),
          default_asset_paths.home_animation.string());
      const std::string default_home_visual =
          make_home_snapshot_visual_preview_text(state.global_config_path);
      ok = require(default_home_visual.find("motion frame ") !=
                       std::string::npos,
                   "home snapshot visual preview should expose a second "
                   "animation frame when APNG assets are available") &&
           ok;
      ok = require(default_home_visual.find(
                       detail_section_rule() + "\nMotion frame\n" +
                       detail_section_rule()) != std::string::npos,
                   "home snapshot visual preview should frame the second "
                   "animation frame") &&
           ok;
      const std::string default_visual_snapshot =
          make_home_visual_snapshot_text(state.global_config_path);
      const std::string default_image_snapshot = make_home_visual_snapshot_text(
          state.global_config_path, HomeVisualSnapshotMode::Image);
      const std::string default_animation_snapshot =
          make_home_visual_snapshot_text(state.global_config_path,
                                         HomeVisualSnapshotMode::Animation);
      const std::string last_frame_label =
          "preview frame " +
          std::to_string(default_assets.home_animation.frames.size()) + "/" +
          std::to_string(default_assets.home_animation.frames.size());
      ok = require(default_visual_snapshot.find("motion strip") !=
                           std::string::npos &&
                       default_visual_snapshot.find(last_frame_label) !=
                           std::string::npos,
                   "Home visual utility should render a multi-frame motion "
                   "strip through the final APNG frame") &&
           ok;
      ok = require(default_visual_snapshot.find(
                       detail_section_rule() + "\nMotion strip\n" +
                       detail_section_rule()) != std::string::npos,
                   "Home visual utility should frame the motion strip") &&
           ok;
      ok =
          require(default_image_snapshot.rfind("Home image snapshot", 0) == 0 &&
                      default_image_snapshot.find("still frame") !=
                          std::string::npos &&
                      default_image_snapshot.find("Motion strip") ==
                          std::string::npos,
                  "Home image utility should render a distinct still-image "
                  "snapshot without the animation strip") &&
          ok;
      ok = require(default_animation_snapshot.rfind("Home animation snapshot",
                                                    0) == 0 &&
                       default_animation_snapshot.find("Animation preview") !=
                           std::string::npos &&
                       default_animation_snapshot.find("Motion strip") !=
                           std::string::npos &&
                       default_animation_snapshot.find(last_frame_label) !=
                           std::string::npos,
                   "Home animation utility should render a distinct APNG "
                   "motion snapshot") &&
           ok;
      CmdState default_visual_state{};
      refresh_home_visual_state(default_visual_state, state.global_config_path);
      ok = require(default_visual_state.home_visual.animation_width >=
                           default_assets.home_animation.width &&
                       default_visual_state.home_visual.animation_height >=
                           default_assets.home_animation.height,
                   "Home visual state should report the rendered composited "
                   "image size rather than only the raw APNG size") &&
           ok;
    }
    ok = require(home_snapshot.find(detail_section_rule() + "\nCommand\n" +
                                    detail_section_rule() + "\n cmd> ready") !=
                     std::string::npos,
                 "snapshot should restore the visible command prompt strip") &&
         ok;
    ok = require(home_snapshot.find("type command or : mode | Enter run") !=
                     std::string::npos,
                 "snapshot command strip should advertise command utilities") &&
         ok;
  }
  {
    CmdState command_state = state;
    command_state.command_mode = true;
    command_state.command_history = {"runtime", "logs source status"};
    const std::string command_line =
        make_command_line_snapshot_text(command_state);
    ok = require(command_line.find(detail_section_rule() + "\nCommand\n" +
                                   detail_section_rule()) != std::string::npos,
                 "command strip should frame its prompt surface") &&
         ok;
    ok = require(command_line.find("cmd> editing") != std::string::npos,
                 "command strip should keep the restored legacy prompt while "
                 "editing") &&
         ok;
    ok = require(command_line.find("history=2") != std::string::npos,
                 "command strip should report command history depth") &&
         ok;
    ok = require(command_line.find("last: logs source status") !=
                     std::string::npos,
                 "command strip should show the most recent command") &&
         ok;
    ok = require(command_line.find("Esc cancel command") != std::string::npos,
                 "command strip should describe Esc as command cancellation "
                 "while editing") &&
         ok;
  }
  {
    CmdState overlay_state = state;
    overlay_state.help_view = true;
    const std::string command_line =
        make_command_line_snapshot_text(overlay_state);
    ok =
        require(command_line.find("Esc/[x] close overlay") != std::string::npos,
                "command strip should advertise both restored overlay close "
                "affordances") &&
        ok;
  }
  {
    CmdState zoom_state = state;
    zoom_state.screen = ScreenMode::Logs;
    ok = require(workspace_toggle_current_screen_zoom(zoom_state),
                 "test setup should zoom Shell Logs") &&
         ok;
    const std::string command_line =
        make_command_line_snapshot_text(zoom_state);
    ok = require(command_line.find("Esc restore split") != std::string::npos,
                 "command strip should advertise Esc split while zoomed") &&
         ok;
  }
  {
    CmdState zoom_state = state;
    zoom_state.screen = ScreenMode::Config;
    zoom_state.config.files = {
        ConfigFileSummary{.path = "/tmp/alpha.dsl",
                          .relative_path = "alpha.dsl"},
    };
    ok = require(workspace_toggle_current_screen_zoom(zoom_state),
                 "test setup should zoom Config") &&
         ok;
    const std::string snapshot = make_snapshot_text(zoom_state);
    ok = require(snapshot.find("workspace=full") != std::string::npos &&
                     snapshot.find("F9 CONFIG") != std::string::npos,
                 "Config zoomed snapshot should expose full workspace mode") &&
         ok;
    ok = require(snapshot.find("Config detail") != std::string::npos,
                 "Config zoomed snapshot should keep the detail panel") &&
         ok;
    ok = require(snapshot.find("Worklist") == std::string::npos,
                 "Config zoomed snapshot should hide the catalog worklist") &&
         ok;
  }
  {
    CmdState zoom_state = state;
    zoom_state.screen = ScreenMode::Lattice;
    zoom_state.lattice.target_ids = {"target.alpha"};
    ok = require(workspace_toggle_current_screen_zoom(zoom_state),
                 "test setup should zoom Lattice") &&
         ok;
    const std::string snapshot = make_snapshot_text(zoom_state);
    ok = require(snapshot.find("workspace=full") != std::string::npos &&
                     snapshot.find("F4 LATTICE") != std::string::npos,
                 "Lattice zoomed snapshot should expose full workspace mode") &&
         ok;
    ok = require(snapshot.find("Lattice detail") != std::string::npos,
                 "Lattice zoomed snapshot should keep the detail panel") &&
         ok;
    ok = require(snapshot.find("Worklist") == std::string::npos,
                 "Lattice zoomed snapshot should hide the target worklist") &&
         ok;
  }

  const std::size_t runtime_offset = home_rail.find("F3 RUNTIME");
  ok = require(runtime_offset != std::string::npos,
               "runtime tab should be present in the rendered rail") &&
       ok;
  ok = require(dispatch_nav_rail_click(actions, state, ui,
                                       ui.nav->screen.x +
                                           static_cast<int>(runtime_offset),
                                       ui.nav->screen.y),
               "clicking the F3 segment should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "clicking F3 should switch to Runtime with legacy status") &&
       ok;

  const std::string runtime_rail = make_tab_text(state);
  const std::size_t logs_offset = runtime_rail.find("F8 SHELL LOGS");
  ok = require(logs_offset != std::string::npos,
               "logs tab should be present in the rendered rail") &&
       ok;
  ok = require(dispatch_nav_rail_click(actions, state, ui,
                                       ui.nav->screen.x +
                                           static_cast<int>(logs_offset),
                                       ui.nav->screen.y),
               "clicking the F8 segment should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.status == "screen=shell_logs",
               "clicking F8 should switch to Shell Logs with legacy status") &&
       ok;

  state.screen = ScreenMode::Home;
  const std::size_t gap_offset = make_tab_text(state).find(" F2 WORKBENCH");
  ok = require(gap_offset != std::string::npos,
               "rail should preserve the gap before F2") &&
       ok;
  ok = require(!dispatch_nav_rail_click(actions, state, ui,
                                        ui.nav->screen.x +
                                            static_cast<int>(gap_offset),
                                        ui.nav->screen.y),
               "clicking the space between tabs should not dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home,
               "gap click should leave the active screen unchanged") &&
       ok;

  ok = require(!dispatch_nav_rail_click(actions, state, ui, ui.nav->screen.x,
                                        ui.nav->screen.y + 1),
               "clicking outside the nav row should not dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Home,
               "outside click should leave the active screen unchanged") &&
       ok;

  state.help_view = true;
  ok = require(!dispatch_nav_rail_click(actions, state, ui, ui.nav->screen.x,
                                        ui.nav->screen.y),
               "nav rail should stay inactive while an overlay is open") &&
       ok;
  state.help_view = false;

  ui.title = std::make_shared<iinuji_object_t>();
  ui.title->id = "cmd.title";
  ui.title->visible = true;
  ui.title->style.border = true;
  ui.title->screen = Rect{2, 0, 100, 3};
  state.screen = ScreenMode::Home;
  const std::size_t title_runtime_offset =
      make_tab_text(state).find("F3 RUNTIME");
  ok = require(title_runtime_offset != std::string::npos,
               "runtime tab should be present in the title rail") &&
       ok;
  ok = require(
           dispatch_nav_rail_click(actions, state, ui,
                                   ui.title->screen.x + 1 +
                                       static_cast<int>(title_runtime_offset),
                                   ui.title->screen.y + 1),
           "clicking the title-frame F3 segment should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "title-frame F3 click should switch to Runtime") &&
       ok;

  ui.menu_commands = std::make_shared<iinuji_object_t>();
  ui.menu_commands->id = "cmd.menu.commands";
  ui.menu_commands->visible = true;
  ui.menu_commands->screen = Rect{8, 4, 30, 140};
  ui.menu_comments = std::make_shared<iinuji_object_t>();
  ui.menu_comments->id = "cmd.menu.comments";
  ui.menu_comments->visible = true;
  ui.menu_comments->screen = Rect{40, 4, 50, 140};

  state.action_menu_open = true;
  state.action_menu_selected = 0;
  state.action_menu_scroll_y = 0;
  const auto action_index = [&](ActionId id) -> std::optional<std::size_t> {
    const auto &rows = actions.actions();
    for (std::size_t i = 0; i < rows.size(); ++i) {
      if (rows[i].id == id)
        return i;
    }
    return std::nullopt;
  };
  const auto runtime_action_index = action_index(ActionId::ShowRuntime);
  ok = require(runtime_action_index.has_value(),
               "action palette should include the Runtime action") &&
       ok;
  const int runtime_row = ui.menu_commands->screen.y + kActionMenuHeaderRows +
                          static_cast<int>(runtime_action_index.value_or(0u));
  ok = require(dispatch_action_menu_click(actions, state, ui,
                                          ui.menu_comments->screen.x + 1,
                                          runtime_row, false),
               "clicking an action palette row should select it") &&
       ok;
  ok = require(state.action_menu_open,
               "single-clicking an action row should leave the menu open") &&
       ok;
  ok =
      require(
          state.action_menu_selected == runtime_action_index.value_or(0u),
          "clicking the Runtime action row should select the Runtime action") &&
      ok;

  ok = require(
           !dispatch_action_menu_click(actions, state, ui,
                                       ui.menu_commands->screen.x,
                                       ui.menu_commands->screen.y + 1, false),
           "clicking the action palette header should not select an action") &&
       ok;

  state.screen = ScreenMode::Home;
  const int runtime_visible_row =
      ui.menu_commands->screen.y + kActionMenuHeaderRows +
      static_cast<int>(runtime_action_index.value_or(0u)) -
      state.action_menu_scroll_y;
  ok = require(dispatch_action_menu_click(actions, state, ui,
                                          ui.menu_commands->screen.x + 1,
                                          runtime_visible_row, true),
               "double-clicking an action row should run it") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime,
               "running the Runtime action row should switch to Runtime") &&
       ok;
  ok = require(!state.action_menu_open,
               "running an action row should close the action menu") &&
       ok;

  state.screen = ScreenMode::Home;
  state.action_menu_selected = runtime_action_index.value_or(0u);
  state.action_menu_open = true;
  ok = require(actions.dispatch("iinuji.commands.run.selected()", state),
               "commands.run.selected path should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime && !state.action_menu_open,
               "commands.run.selected path should run the selected palette "
               "action") &&
       ok;

  state.screen = ScreenMode::Home;
  state.action_menu_selected = runtime_action_index.value_or(0u);
  state.action_menu_open = false;
  ok = require(actions.dispatch("actions run", state),
               "actions run should dispatch when the palette is closed") &&
       ok;
  ok = require(state.screen == ScreenMode::Home && state.action_menu_open,
               "actions run should open the palette instead of running an "
               "unseen stale selection") &&
       ok;

  state.screen = ScreenMode::Workbench;
  state.action_menu_selected = 0u;
  state.action_menu_open = false;
  ok = require(actions.dispatch("a", state),
               "blank Workbench action menu should dispatch") &&
       ok;
  ok = require(actions.actions()[state.action_menu_selected].id ==
                   ActionId::ShowRuntime,
               "blank Workbench action menu should anchor on the Runtime "
               "escape hatch instead of the already-active Workbench row") &&
       ok;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    ok = require(action_snapshot.find("> switch F3 RUNTIME") !=
                     std::string::npos,
                 "blank Workbench action palette should open on Runtime") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.actions.run()", state),
               "blank Workbench selected escape action should run") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime && !state.action_menu_open,
               "running the contextual blank Workbench action should switch to "
               "Runtime") &&
       ok;

  state.screen = ScreenMode::Logs;
  state.logs.selected_setting = 0u;
  state.logs.source_filter = LogsSourceFilter::All;
  state.action_menu_selected = 0u;
  state.action_menu_open = false;
  ok = require(actions.dispatch("a", state),
               "Shell Logs action menu should dispatch") &&
       ok;
  ok = require(actions.actions()[state.action_menu_selected].id ==
                   ActionId::CycleLogsSourceFilter,
               "Shell Logs action menu should anchor on the selected source "
               "control instead of the scroll-end row") &&
       ok;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    ok = require(action_snapshot.find("> cycle logs source filter") !=
                     std::string::npos,
                 "Shell Logs action palette should open on the selected "
                 "control deck row") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.actions.run()", state),
               "Shell Logs selected control action should run") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs && !state.action_menu_open &&
                   state.logs.source_filter == LogsSourceFilter::Refresh,
               "running the contextual Shell Logs source action should cycle "
               "the source lens and close the palette") &&
       ok;

  state.screen = ScreenMode::Config;
  state.action_menu_selected = 0u;
  state.action_menu_open = false;
  ok = require(actions.dispatch("a", state),
               "legacy one-key a command should open the action menu") &&
       ok;
  ok = require(state.action_menu_open,
               "legacy one-key a command should behave like actions") &&
       ok;
  state.action_menu_open = false;
  ok = require(actions.dispatch("actions", state),
               "actions alias should open the action menu") &&
       ok;
  ok = require(state.action_menu_open && state.action_menu_selected > 0u,
               "action menu should anchor on current screen utilities") &&
       ok;
  ok = require(actions.actions()[state.action_menu_selected].id ==
                   ActionId::ConfigFileShow,
               "Config action menu should select the restored file preview "
               "utility") &&
       ok;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    ok =
        require(action_snapshot.find("> config file show selected") !=
                    std::string::npos,
                "Config action palette should open on selected file preview") &&
        ok;
    ok = require(action_snapshot.find("selected file preview") !=
                     std::string::npos,
                 "Config action palette should describe the selected file "
                 "preview utility") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.actions.run()", state),
               "Config selected preview action should run") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.status == "show config" && !state.action_menu_open,
               "running the contextual Config action should show the selected "
               "file preview") &&
       ok;
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Jobs;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  state.action_menu_selected = 0u;
  state.action_menu_open = false;
  ok = require(actions.dispatch("a", state),
               "Runtime Jobs action menu should dispatch") &&
       ok;
  ok = require(actions.actions()[state.action_menu_selected].id ==
                   ActionId::RuntimeDetailLog,
               "Runtime Jobs action menu should anchor on the next useful job "
               "detail lens instead of the already-active Jobs focus") &&
       ok;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    ok = require(action_snapshot.find("> runtime detail log") !=
                     std::string::npos,
                 "Runtime Jobs action palette should open on the log detail "
                 "utility") &&
         ok;
    ok = require(action_snapshot.find("iinuji.runtime.detail.log()") !=
                     std::string::npos,
                 "Runtime Jobs action palette should keep the canonical log "
                 "detail path visible") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.actions.run()", state),
               "Runtime Jobs selected action should run") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.detail_mode == RuntimeDetailMode::Log &&
                   state.status == "runtime detail=log",
               "running the contextual Runtime Jobs action should open the "
               "selected job log lens") &&
       ok;
  state.screen = ScreenMode::Lattice;
  state.lattice.target_ids = {"target.alpha", "target.beta"};
  state.lattice.selected_target = 1u;
  state.action_menu_selected = 0u;
  state.action_menu_open = false;
  ok = require(actions.dispatch("a", state),
               "Lattice action menu should dispatch") &&
       ok;
  ok = require(actions.actions()[state.action_menu_selected].id ==
                   ActionId::ShowLatticeSummary,
               "Lattice action menu should select the restored target proof "
               "utility") &&
       ok;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    ok =
        require(action_snapshot.find("> show F4 LATTICE summary") !=
                    std::string::npos,
                "Lattice action palette should open on target proof summary") &&
        ok;
    ok = require(action_snapshot.find("selected target proof") !=
                     std::string::npos,
                 "Lattice action palette should describe the selected target "
                 "proof utility") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.actions.run()", state),
               "Lattice selected target proof action should run") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.status == "show lattice" && !state.action_menu_open,
               "running the contextual Lattice action should show the selected "
               "target proof") &&
       ok;
  state.screen = ScreenMode::Config;
  state.action_menu_open = true;
  state.action_menu_selected = 1u;
  ok = require(handle_action_menu_key('k', actions, state) &&
                   state.action_menu_selected == 0u,
               "action menu k key should restore legacy previous movement") &&
       ok;
  ok = require(handle_action_menu_key('j', actions, state) &&
                   state.action_menu_selected == 1u,
               "action menu j key should restore legacy next movement") &&
       ok;
  state.action_menu_selected = 0u;
  ok = require(actions.dispatch("actions down", state),
               "natural actions down command should dispatch") &&
       ok;
  ok = require(state.action_menu_open && !state.help_view &&
                   state.action_menu_selected == 1u &&
                   state.status == "action menu",
               "natural actions down should move the palette selection") &&
       ok;
  ok = require(actions.dispatch("palette up", state),
               "natural palette up command should dispatch") &&
       ok;
  ok =
      require(state.action_menu_selected == 0u && state.status == "action menu",
              "natural palette up should move the palette selection") &&
      ok;
  ok = require(actions.dispatch("commands page down", state),
               "natural commands page down command should dispatch") &&
       ok;
  ok =
      require(state.action_menu_selected == 8u && state.status == "action menu",
              "natural commands page down should use the palette page step") &&
      ok;
  ok = require(actions.dispatch("commands home", state),
               "natural commands home command should dispatch") &&
       ok;
  ok =
      require(state.action_menu_selected == 0u && state.status == "action menu",
              "natural commands home should jump to the first palette row") &&
      ok;

  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("marshal", state),
               "legacy marshal alias should dispatch to Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "legacy marshal alias should navigate to the blank Workbench "
               "screen") &&
       ok;
  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("inbox", state),
               "legacy inbox alias should dispatch to Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "legacy inbox alias should navigate to Workbench") &&
       ok;
  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("human", state),
               "legacy human alias should dispatch to Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "legacy human alias should navigate to Workbench") &&
       ok;
  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("iinuji.screen.human()", state),
               "legacy human canonical screen path should dispatch to "
               "Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "legacy human canonical screen path should navigate to "
               "Workbench") &&
       ok;
  state.screen = ScreenMode::Home;
  ok = require(actions.dispatch("iinuji.screen.marshal()", state),
               "legacy marshal canonical screen path should dispatch to "
               "Workbench") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "screen=workbench",
               "legacy marshal canonical screen path should navigate to "
               "Workbench") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("show workbench", state),
               "natural show workbench alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "show workbench",
               "show workbench alias should show the Workbench summary") &&
       ok;
  ok = require(!state.log.empty() && state.log[0].source == "show" &&
                   state.log[0].message == "screen=workbench",
               "show workbench alias should append Workbench show lines") &&
       ok;
  state.screen = ScreenMode::Home;
  state.log.clear();
  ok = require(actions.dispatch("iinuji.show.human()", state),
               "legacy human canonical show path should dispatch to "
               "Workbench summary") &&
       ok;
  ok = require(state.screen == ScreenMode::Workbench &&
                   state.status == "show workbench",
               "legacy human canonical show path should show Workbench") &&
       ok;
  ok = require(!state.log.empty() && state.log[0].source == "show" &&
                   state.log[0].message == "screen=workbench",
               "legacy human canonical show path should append Workbench show "
               "lines") &&
       ok;
  state.log.clear();
  ok = require(actions.dispatch("iinuji.screen.runtime()", state),
               "canonical screen runtime command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime",
               "screen runtime command should navigate to Runtime with legacy "
               "status") &&
       ok;
  state.log.clear();
  ok = require(actions.dispatch("iinuji.show.runtime()", state),
               "canonical show runtime command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "show runtime",
               "show runtime command should show the Runtime summary") &&
       ok;
  ok = require(!state.log.empty() && state.log[0].source == "show" &&
                   state.log[0].message == "screen=runtime",
               "show runtime command should append Runtime show lines") &&
       ok;
  state.log.clear();
  ok = require(actions.dispatch("show runtime", state),
               "natural show runtime alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "show runtime",
               "natural show runtime alias should show Runtime") &&
       ok;
  state.log.clear();
  ok = require(actions.dispatch("show shell logs", state),
               "natural show shell logs alias should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs &&
                   state.status == "show shell logs",
               "show shell logs alias should show Shell Logs") &&
       ok;
  ok = require(!state.log.empty() && state.log[0].source == "show" &&
                   state.log[0].message == "screen=shell_logs",
               "show shell logs alias should append Shell Logs show lines") &&
       ok;
  ok = require(actions.dispatch("shell logs", state),
               "visible Shell Logs screen name should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Logs,
               "visible Shell Logs screen name should switch to Shell Logs") &&
       ok;
  ok = require(actions.dispatch("waajacamaya", state),
               "visual Home alias should dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home && state.status == "home visual",
              "visual Home alias should switch to the Home visual utility") &&
      ok;
  ok = require(actions.dispatch("iinuji.home.visual()", state),
               "canonical Home visual alias should dispatch") &&
       ok;
  ok =
      require(state.screen == ScreenMode::Home && state.status == "home visual",
              "canonical Home visual alias should keep Home active as a "
              "first-class utility") &&
      ok;
  {
    CmdState refresh_state{};
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("iinuji.refresh()", refresh_state),
                 "canonical shell refresh command should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Home &&
                     refresh_state.status == "hero shell refresh queued",
                 "shell refresh should restore the legacy queued status") &&
         ok;
    ok = require(operator_status_emphasis(refresh_state) ==
                     text_line_emphasis_t::Success,
                 "queued refresh statuses should remain visually eventful") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("iinuji.workbench.refresh()", refresh_state),
                 "canonical Workbench refresh command should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Workbench &&
                     refresh_state.status == "workbench refresh queued",
                 "Workbench refresh should restore the legacy queued status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("workbench refresh", refresh_state),
                 "natural Workbench refresh alias should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Workbench &&
                     refresh_state.status == "workbench refresh queued",
                 "natural Workbench refresh alias should keep the queued "
                 "status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("iinuji.runtime.refresh()", refresh_state),
                 "canonical Runtime refresh command should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Runtime &&
                     refresh_state.status == "runtime screen refresh queued",
                 "Runtime refresh should restore the legacy queued status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("runtime refresh", refresh_state),
                 "natural Runtime refresh alias should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Runtime &&
                     refresh_state.status == "runtime screen refresh queued",
                 "natural Runtime refresh alias should keep the queued "
                 "status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("iinuji.lattice.refresh()", refresh_state),
                 "canonical Lattice refresh command should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Lattice &&
                     refresh_state.status == "lattice refresh queued",
                 "Lattice refresh should restore the legacy queued status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("lattice refresh", refresh_state),
                 "natural Lattice refresh alias should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Lattice &&
                     refresh_state.status == "lattice refresh queued",
                 "natural Lattice refresh alias should keep the queued "
                 "status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("iinuji.config.reload()", refresh_state),
                 "canonical Config reload command should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Config &&
                     refresh_state.status == "config reload queued",
                 "Config reload should restore the legacy queued status") &&
         ok;
    refresh_state.screen = ScreenMode::Home;
    ok = require(actions.dispatch("config reload", refresh_state),
                 "natural Config reload alias should dispatch") &&
         ok;
    ok = require(refresh_state.screen == ScreenMode::Config &&
                     refresh_state.status == "config reload queued",
                 "natural Config reload alias should keep the queued status") &&
         ok;
  }
  {
    CmdState quit_state{};
    ok = require(actions.dispatch("iinuji.quit()", quit_state),
                 "canonical quit command should dispatch") &&
         ok;
    ok = require(quit_state.quit && quit_state.status == "quit",
                 "canonical quit command should set quit status") &&
         ok;
    CmdState exit_state{};
    ok = require(actions.dispatch("iinuji.exit()", exit_state),
                 "canonical exit command should dispatch") &&
         ok;
    ok = require(exit_state.quit && exit_state.status == "exit",
                 "canonical exit command should preserve the exit spelling") &&
         ok;
    CmdState x_exit_state{};
    ok = require(actions.dispatch("x", x_exit_state),
                 "legacy x exit alias should dispatch") &&
         ok;
    ok = require(x_exit_state.quit && x_exit_state.status == "exit",
                 "legacy x exit alias should preserve the exit spelling") &&
         ok;
  }

  state.screen = ScreenMode::Home;
  {
    CmdState action_hint_state = state;
    action_hint_state.action_menu_open = true;
    ok = require(make_operator_hint_line(action_hint_state)
                         .find("Up/Down/j/k move") != std::string::npos,
                 "action palette bottom hint should advertise legacy j/k "
                 "movement") &&
         ok;
    ok = require(make_operator_hint_line(action_hint_state)
                         .find("Pg/Home/End edge") != std::string::npos,
                 "action palette bottom hint should advertise legacy edge "
                 "movement") &&
         ok;
    ok =
        require(
            make_operator_hint_line(action_hint_state).find("Esc/[x] close") !=
                    std::string::npos &&
                make_operator_hint_line(action_hint_state).find("close path") ==
                    std::string::npos,
            "action palette bottom hint should expose both restored close "
            "affordances") &&
        ok;

    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    const std::size_t palette_start = action_snapshot.find("ACTION MENU");
    const std::size_t palette_end = action_snapshot.find(
        std::string{"\n---\n"} + detail_section_rule() + "\nCommand",
        palette_start);
    const std::string palette_section =
        palette_start == std::string::npos
            ? std::string{}
            : action_snapshot.substr(palette_start,
                                     palette_end - palette_start);
    ok = require(action_snapshot.find("Action palette") != std::string::npos,
                 "action snapshot should render the palette surface") &&
         ok;
    ok = require(action_snapshot.find("Esc / [x]") != std::string::npos,
                 "action palette snapshot should expose the restored close "
                 "corner affordance") &&
         ok;
    ok = require(action_snapshot.find("CONTROLS") != std::string::npos &&
                     action_snapshot.find("GROUPS") != std::string::npos &&
                     action_snapshot.find("ACTIONS") != std::string::npos,
                 "action palette should restore sectioned overlay bands") &&
         ok;
    {
      CmdState runtime_palette_state = state;
      runtime_palette_state.screen = ScreenMode::Runtime;
      runtime_palette_state.runtime.focus = RuntimeFocus::Devices;
      actions.open_action_menu(runtime_palette_state);
      const std::string runtime_action_snapshot =
          make_action_menu_snapshot_text(runtime_palette_state, actions);
      ok = require(runtime_action_snapshot.find("group") != std::string::npos &&
                       runtime_action_snapshot.find("runtime 1/10") !=
                           std::string::npos,
                   "action palette should expose the selected action group "
                   "position for deep contextual menus") &&
           ok;
    }
    ok = require(action_snapshot.find("run selected action path") ==
                         std::string::npos &&
                     action_snapshot.find("close action menu path") ==
                         std::string::npos &&
                     action_snapshot.find("move selection paths") ==
                         std::string::npos &&
                     action_snapshot.find("iinuji.actions.run()") !=
                         std::string::npos &&
                     action_snapshot.find("run selected action") !=
                         std::string::npos &&
                     action_snapshot.find("move action selection") !=
                         std::string::npos,
                 "action palette control copy should stay operator-facing") &&
         ok;
    ok = require(
             action_snapshot.find(detail_section_rule() + "\nCONTROLS\n" +
                                  detail_section_rule()) != std::string::npos &&
                 action_snapshot.find(detail_section_rule() + "\nACTIONS\n" +
                                      detail_section_rule()) !=
                     std::string::npos,
             "action palette snapshot should frame its restored overlay "
             "bands") &&
         ok;
    ok = require(palette_section.find('~') == std::string::npos,
                 "action palette visible window should avoid hard-truncated "
                 "alias rows") &&
         ok;
    ok = require(palette_section.find("iinuji.screen.runtime()") !=
                     std::string::npos,
                 "action palette compact rows should keep canonical screen "
                 "paths visible") &&
         ok;
    ok = require(action_snapshot.find("marshal/inbox/human") ==
                         std::string::npos &&
                     action_snapshot.find("marshal/inbox/human | reserved") ==
                         std::string::npos,
                 "action palette should describe F2 as Workbench, not removed "
                 "hero names") &&
         ok;
    const std::string framed_home_disclosures =
        "\n---\ndisclosures\n\n" + detail_section_rule() + "\nHome showcase\n" +
        detail_section_rule();
    ok = require(action_snapshot.find(framed_home_disclosures) ==
                     std::string::npos,
                 "action palette snapshot should not leak the covered Home "
                 "detail panel") &&
         ok;
    ok = require(action_snapshot.find("switch F1 HOME") != std::string::npos,
                 "action palette should distinguish screen switching from "
                 "show summaries") &&
         ok;
    ok = require(action_snapshot.find("> show Home visual showcase") !=
                     std::string::npos,
                 "Home action palette should open on the restored Home visual "
                 "utility instead of the already-active F1 switch row") &&
         ok;
    ok =
        require(
            palette_section.find("show Home visual showcase (waajacamaya)") !=
                    std::string::npos &&
                palette_section.find("show Home bootstrap splash (loading)") !=
                    std::string::npos &&
                palette_section.find("show Home farewell splash (Good luck)") !=
                    std::string::npos,
            "Home action palette should surface the restored waajacamaya, "
            "loading, and Good luck visual names in the action rows") &&
        ok;
    ok = require(action_snapshot.find("iinuji.home.visual()") !=
                     std::string::npos,
                 "Home action palette should show the canonical Home visual "
                 "path") &&
         ok;
    ok = require(
             palette_section.find("--visual | --image | --animation") !=
                     std::string::npos &&
                 palette_section.find("iinuji.home.visual()") !=
                     std::string::npos &&
                 palette_section.find("--splash/--boot") != std::string::npos &&
                 palette_section.find("iinuji.splash()") != std::string::npos &&
                 palette_section.find("--farewell/--closing") !=
                     std::string::npos &&
                 palette_section.find("iinuji.farewell()") != std::string::npos,
             "Home action palette should expose restored image, "
             "animation, splash, and farewell aliases beside canonical "
             "Home paths") &&
         ok;
    ok = require(action_snapshot.find("visual") != std::string::npos,
                 "action palette should expose Home visual aliases") &&
         ok;
    ok = require(action_snapshot.find("waajacamaya") != std::string::npos,
                 "action palette should expose the restored waajacamaya "
                 "alias before truncation") &&
         ok;
    ok = require(action_snapshot.find("shell logs") != std::string::npos,
                 "action palette should expose the visible Shell Logs alias "
                 "before truncation") &&
         ok;
    ok = require(action_snapshot.find("visible") != std::string::npos &&
                     action_snapshot.find("shown |") != std::string::npos,
                 "action palette snapshot should expose the visible option "
                 "window") &&
         ok;
    ok = require(action_snapshot.find("groups") != std::string::npos,
                 "action palette should expose utility groups") &&
         ok;
    ok = require(action_snapshot.find("Up/Down or j/k") != std::string::npos,
                 "action palette should advertise legacy j/k movement") &&
         ok;
    ok = require(action_snapshot.find("Home/End edge") != std::string::npos,
                 "action palette should advertise legacy Home/End movement") &&
         ok;
    ok = require(action_snapshot.find("screen/show/menu") != std::string::npos,
                 "action palette groups should advertise the restored show "
                 "summary group") &&
         ok;
    ok = require(action_snapshot.find("[screen]") != std::string::npos,
                 "action palette should tag screen actions") &&
         ok;
    ok = require(action_snapshot.find(detail_section_rule() + "\nCommand\n" +
                                      detail_section_rule() +
                                      "\n cmd> ready") != std::string::npos,
                 "action palette snapshot should keep the command strip "
                 "visible") &&
         ok;
  }
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Devices;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    const std::size_t palette_start = action_snapshot.find("ACTION MENU");
    const std::size_t palette_end = action_snapshot.find(
        std::string{"\n---\n"} + detail_section_rule() + "\nCommand",
        palette_start);
    const std::string palette_section =
        palette_start == std::string::npos
            ? std::string{}
            : action_snapshot.substr(palette_start,
                                     palette_end - palette_start);
    ok = require(action_snapshot.find("> runtime focus devices") !=
                     std::string::npos,
                 "Runtime action palette snapshot should open on Runtime "
                 "utilities") &&
         ok;
    ok = require(palette_section.find('~') == std::string::npos,
                 "Runtime action palette window should avoid hard-truncated "
                 "alias rows") &&
         ok;
    ok = require(palette_section.find("iinuji.runtime.devices()") !=
                     std::string::npos,
                 "Runtime action palette window should keep canonical utility "
                 "paths visible") &&
         ok;
    ok = require(palette_section.find("runtime focus jobs") !=
                         std::string::npos &&
                     palette_section.find("runtime detail next") !=
                         std::string::npos,
                 "Runtime action palette window should show neighboring "
                 "Runtime utilities instead of ending on the selected row") &&
         ok;
    ok = require(palette_section.find("logs metadata") == std::string::npos,
                 "Runtime action palette window should not open inside the "
                 "previous Logs utility group") &&
         ok;
    ok =
        require(action_snapshot.find("[runtime]") != std::string::npos,
                "Runtime action palette window should tag Runtime utilities") &&
        ok;
  }
  state.screen = ScreenMode::Config;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    const std::size_t palette_start = action_snapshot.find("ACTION MENU");
    const std::size_t palette_end = action_snapshot.find(
        std::string{"\n---\n"} + detail_section_rule() + "\nCommand",
        palette_start);
    const std::string palette_section =
        palette_start == std::string::npos
            ? std::string{}
            : action_snapshot.substr(palette_start,
                                     palette_end - palette_start);
    ok = require(action_snapshot.find("> config file show selected") !=
                     std::string::npos,
                 "Config action palette snapshot should open on the restored "
                 "selected file preview utility") &&
         ok;
    ok = require(palette_section.find('~') == std::string::npos,
                 "Config action palette window should avoid hard-truncated "
                 "alias rows") &&
         ok;
    ok = require(palette_section.find("iinuji.config.file.show()") !=
                     std::string::npos,
                 "Config action palette window should keep canonical utility "
                 "paths visible") &&
         ok;
    ok = require(action_snapshot.find("[config]") != std::string::npos,
                 "Config action palette window should tag Config utilities") &&
         ok;
    ok = require(action_snapshot.find("switch F1 HOME") == std::string::npos,
                 "Config action palette snapshot should render the selected "
                 "window instead of the full registry") &&
         ok;
  }
  state.screen = ScreenMode::Lattice;
  {
    const std::string action_snapshot =
        make_action_menu_snapshot_text(state, actions);
    const std::size_t palette_start = action_snapshot.find("ACTION MENU");
    const std::size_t palette_end = action_snapshot.find(
        std::string{"\n---\n"} + detail_section_rule() + "\nCommand",
        palette_start);
    const std::string palette_section =
        palette_start == std::string::npos
            ? std::string{}
            : action_snapshot.substr(palette_start,
                                     palette_end - palette_start);
    ok = require(action_snapshot.find("> show F4 LATTICE summary") !=
                     std::string::npos,
                 "Lattice action palette snapshot should open on the restored "
                 "selected target proof utility") &&
         ok;
    ok = require(palette_section.find('~') == std::string::npos,
                 "Lattice action palette window should avoid hard-truncated "
                 "alias rows") &&
         ok;
    ok = require(palette_section.find("iinuji.show.lattice()") !=
                     std::string::npos,
                 "Lattice action palette window should keep canonical proof "
                 "path visible") &&
         ok;
    ok = require(action_snapshot.find("selected target proof") !=
                     std::string::npos,
                 "Lattice action palette window should describe the selected "
                 "target proof") &&
         ok;
  }

  const auto click_scrolled_menu_row_at = [&](int row, int x_offset) {
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    return dispatch_menu_overlay_click(
        actions, state, ui, ui.menu_commands->screen.x + x_offset,
        ui.menu_commands->screen.y + row - state.help_scroll_y);
  };
  const auto click_scrolled_menu_row = [&](int row) {
    return click_scrolled_menu_row_at(row, 1);
  };

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Home;
  ok = require(dispatch_menu_overlay_click(
                   actions, state, ui, ui.menu_comments->screen.x + 1,
                   ui.menu_comments->screen.y + kMenuScreenFirstRow + 5),
               "clicking a menu overlay screen row should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Config,
               "clicking the F9 menu row should switch to Config") &&
       ok;
  ok = require(!state.help_view,
               "screen row click should close the menu overlay") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  {
    const auto home_fkey_alias_row =
        menu_command_row_index(state, "home | f1 | 1");
    ok = require(home_fkey_alias_row.has_value(),
                 "Home menu should expose clickable F1 command alias row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + home_fkey_alias_row.value_or(0)),
             "clicking the F1 command alias row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home && !state.help_view &&
                   state.status == "screen=home",
               "F1 command alias row should switch to Home and close the "
               "menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Home;
  {
    const std::string home_menu_snapshot =
        make_menu_overlay_snapshot_text(state);
    ok = require(home_menu_snapshot.find("F2 / F3 / F4 / F8 / F9") !=
                     std::string::npos,
                 "Home current-screen menu should advertise all restored "
                 "F-key shortcuts") &&
         ok;
    ok = require(home_menu_snapshot.find("cuwacunu_cmd --visual") !=
                     std::string::npos,
                 "Home menu should advertise the visual snapshot utility") &&
         ok;
    ok =
        require(
            home_menu_snapshot.find("cuwacunu_cmd --image") !=
                    std::string::npos &&
                home_menu_snapshot.find("--home-visual") != std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --animation") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu.cmd --snapshot --image") !=
                    std::string::npos &&
                home_menu_snapshot.find(
                    "cuwacunu.cmd --snapshot --animation") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --snapshot --image") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --waajacamaya") !=
                    std::string::npos,
            "Home menu should advertise restored image and animation "
            "snapshot utilities") &&
        ok;
    ok = require(home_menu_snapshot.find("command rail") == std::string::npos &&
                     home_menu_snapshot.find("rail [") == std::string::npos,
                 "Home current-screen menu should avoid command rail blocks") &&
         ok;
    {
      CmdState rail_state = state;
      rail_state.screen = ScreenMode::Runtime;
      const std::string runtime_menu_snapshot =
          make_menu_overlay_snapshot_text(rail_state);
      ok =
          require(runtime_menu_snapshot.find("command rail") ==
                          std::string::npos &&
                      runtime_menu_snapshot.find("rail [") == std::string::npos,
                  "Runtime current-screen menu should avoid command rail "
                  "blocks") &&
          ok;

      rail_state.screen = ScreenMode::Logs;
      const std::string logs_menu_snapshot =
          make_menu_overlay_snapshot_text(rail_state);
      ok = require(logs_menu_snapshot.find("command rail") ==
                           std::string::npos &&
                       logs_menu_snapshot.find("rail [") == std::string::npos,
                   "Shell Logs current-screen menu should avoid command rail "
                   "blocks") &&
           ok;

      rail_state.screen = ScreenMode::Config;
      const std::string config_menu_snapshot =
          make_menu_overlay_snapshot_text(rail_state);
      ok = require(config_menu_snapshot.find("command rail") ==
                           std::string::npos &&
                       config_menu_snapshot.find("rail [") == std::string::npos,
                   "Config current-screen menu should avoid command rail "
                   "blocks") &&
           ok;

      rail_state.screen = ScreenMode::Lattice;
      const std::string lattice_menu_snapshot =
          make_menu_overlay_snapshot_text(rail_state);
      ok =
          require(lattice_menu_snapshot.find("command rail") ==
                          std::string::npos &&
                      lattice_menu_snapshot.find("rail [") == std::string::npos,
                  "Lattice current-screen menu should avoid command rail "
                  "blocks") &&
          ok;
    }
    {
      const auto current_first = menu_current_screen_first_row(state);
      const auto path_visual_row = menu_command_row_index(
          state, "cuwacunu_cmd --visual / --home-visual");
      const auto shell_visual_row = menu_command_row_index(
          state, "cuwacunu.cmd --visual / --home-visual");
      const auto path_boot_row =
          menu_command_row_index(state, "cuwacunu_cmd --bootstrap / --boot");
      const auto shell_boot_row =
          menu_command_row_index(state, "cuwacunu.cmd --bootstrap / --boot");
      const auto path_closing_row =
          menu_command_row_index(state, "cuwacunu_cmd --farewell / --closing");
      const auto shell_closing_row =
          menu_command_row_index(state, "cuwacunu.cmd --farewell / --closing");
      ok = require(
               current_first.has_value() && path_visual_row.has_value() &&
                   shell_visual_row.has_value() && path_boot_row.has_value() &&
                   shell_boot_row.has_value() && path_closing_row.has_value() &&
                   shell_closing_row.has_value(),
               "global menu should expose restored Home shell utility "
               "aliases") &&
           ok;
      ok = require(
               current_first.has_value() && path_visual_row.has_value() &&
                   shell_visual_row.has_value() && path_boot_row.has_value() &&
                   shell_boot_row.has_value() && path_closing_row.has_value() &&
                   shell_closing_row.has_value() &&
                   *path_visual_row < *shell_visual_row &&
                   *shell_visual_row < *current_first &&
                   *path_boot_row < *shell_boot_row &&
                   *shell_boot_row < *current_first &&
                   *path_closing_row < *shell_closing_row &&
                   *shell_closing_row < *current_first,
               "restored Home shell utilities should put the PATH command "
               "before the legacy shell alias in the global command-alias "
               "section") &&
           ok;
    }
    ok =
        require(home_menu_snapshot.find("splash | bootstrap") !=
                        std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --bootstrap") !=
                        std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --splash loading") !=
                        std::string::npos &&
                    home_menu_snapshot.find("--boot") != std::string::npos &&
                    home_menu_snapshot.find("farewell | closing") !=
                        std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --farewell") !=
                        std::string::npos &&
                    home_menu_snapshot.find("--closing") != std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --splash farewell") !=
                        std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --splash close") !=
                        std::string::npos &&
                    home_menu_snapshot.find("cuwacunu_cmd --splash=farewell") !=
                        std::string::npos &&
                    home_menu_snapshot.find(
                        "cuwacunu_cmd --splash=good-luck") != std::string::npos,
                "Home menu should advertise restored splash/farewell visual "
                "utilities") &&
        ok;
    ok = require(home_menu_snapshot.find("home | visual | waajacamaya") !=
                     std::string::npos,
                 "Home menu should advertise the restored waajacamaya alias") &&
         ok;
    ok = require(
             home_menu_snapshot.find(
                 "actions down / palette down / commands down") !=
                 std::string::npos,
             "Home menu should advertise the restored commands movement alias "
             "for the action palette") &&
         ok;
    ok =
        require(home_menu_snapshot.find("home | f1 | 1") != std::string::npos &&
                    home_menu_snapshot.find("workbench | work | w | f2 | 2") !=
                        std::string::npos &&
                    home_menu_snapshot.find("runtime | run | rt | f3 | 3") !=
                        std::string::npos &&
                    home_menu_snapshot.find("lattice | lat | proof | f4 | 4") !=
                        std::string::npos &&
                    home_menu_snapshot.find(
                        "logs | log | shell logs | events | f8 | 8") !=
                        std::string::npos &&
                    home_menu_snapshot.find("config | cfg | f9 | 9") !=
                        std::string::npos,
                "Home menu should advertise restored F-key command aliases "
                "beside the natural screen names") &&
        ok;
    ok = require(home_menu_snapshot.find("version | about") !=
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.version()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("cuwacunu_cmd --version") !=
                         std::string::npos,
                 "Home menu should advertise command identity utilities") &&
         ok;
    ok = require(home_menu_snapshot.find(
                     "logs | log | shell logs | events | f8 | 8") !=
                     std::string::npos,
                 "Home menu should advertise the visible Shell Logs alias") &&
         ok;
    ok = require(home_menu_snapshot.find("MENU SCROLL UTILITIES") !=
                         std::string::npos &&
                     home_menu_snapshot.find("help/menu up | down") !=
                         std::string::npos &&
                     home_menu_snapshot.find("help/menu page up | page down") !=
                         std::string::npos &&
                     home_menu_snapshot.find("help/menu left | right") !=
                         std::string::npos &&
                     home_menu_snapshot.find("help/menu home | end") !=
                         std::string::npos,
                 "Home menu should expose the restored menu scroll utility "
                 "rows") &&
         ok;
    ok = require(home_menu_snapshot.find("ACTION PALETTE UTILITIES") !=
                         std::string::npos &&
                     home_menu_snapshot.find("actions up | down") !=
                         std::string::npos &&
                     home_menu_snapshot.find("actions page up | page down") !=
                         std::string::npos &&
                     home_menu_snapshot.find("actions home | end") !=
                         std::string::npos &&
                     home_menu_snapshot.find("actions close | run") !=
                         std::string::npos,
                 "Home menu should expose direct action palette utility "
                 "rows") &&
         ok;
    ok = require(home_menu_snapshot.find("RUNTIME UTILITIES") !=
                         std::string::npos &&
                     home_menu_snapshot.find("runtime devices | jobs") !=
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "runtime manifest | log | trace") != std::string::npos,
                 "Home menu should expose direct Runtime focus and detail "
                 "utility rows") &&
         ok;
    ok =
        require(home_menu_snapshot.find("logs source all | refresh") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs setting prev | next") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs setting left | right") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs up | down") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs page up | page down") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs home | end") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs source action | command") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs source show | status") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs debug | info | warning") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs error | fatal") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs meta any | any_meta") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs meta function | path") !=
                        std::string::npos &&
                    home_menu_snapshot.find("logs meta callsite") !=
                        std::string::npos,
                "Home menu should advertise direct Shell Logs scroll, setting, "
                "source, severity, and metadata utilities") &&
        ok;
    ok = require(home_menu_snapshot.find("show home | iinuji.show.home()") !=
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "show workbench | iinuji.show.workbench()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("show workbench | show inbox") ==
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.show.workbench()") !=
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "show lattice | iinuji.show.lattice()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("show target | show proof") !=
                         std::string::npos &&
                     home_menu_snapshot.find("runtime show | show runtime") !=
                         std::string::npos &&
                     home_menu_snapshot.find("lattice show | proof show") !=
                         std::string::npos &&
                     home_menu_snapshot.find("config show | show config") !=
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "show config | iinuji.show.config()") !=
                         std::string::npos,
                 "Home menu should expose the restored show aliases for every "
                 "visible screen") &&
         ok;
    ok = require(home_menu_snapshot.find("cuwacunu_cmd --run/--command "
                                         "\"show\"") != std::string::npos &&
                     home_menu_snapshot.find("cuwacunu.cmd --run/--command "
                                             "\"show\"") != std::string::npos &&
                     home_menu_snapshot.find("cuwacunu.cmd --catalog --run "
                                             "\"show\"") == std::string::npos &&
                     home_menu_snapshot.find("cuwacunu_cmd --run/--command "
                                             "\"show\"") <
                         home_menu_snapshot.find("cuwacunu.cmd --run/--command "
                                                 "\"show\""),
                 "menu surface should advertise primary and alternate "
                 "non-interactive command execution") &&
         ok;
    ok = require(
             home_menu_snapshot.find("exit command shell") !=
                     std::string::npos &&
                 home_menu_snapshot.find("exit cuwacunu.cmd") ==
                     std::string::npos,
             "menu overlay should keep exit copy tied to the command shell") &&
         ok;
    ok = require(
             home_menu_snapshot.find("cuwacunu_cmd --splash=good-luck") !=
                     std::string::npos &&
                 home_menu_snapshot.find("render Good luck closing splash "
                                         "diagnostics") != std::string::npos &&
                 home_menu_snapshot.find(
                     "render Good luck closing diagnostics") ==
                     std::string::npos,
             "Home menu should keep Good luck splash copy consistent across "
             "command aliases") &&
         ok;
    ok =
        require(
            home_menu_snapshot.find("cuwacunu.cmd --menu") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --menu") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --help") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --version") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu.cmd --help") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu.cmd --version") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu_cmd --actions / --palette") !=
                    std::string::npos &&
                home_menu_snapshot.find("cuwacunu.cmd --actions / --palette") !=
                    std::string::npos &&
                home_menu_snapshot.find(
                    "cuwacunu_cmd --commands / --catalog") !=
                    std::string::npos &&
                home_menu_snapshot.find(
                    "cuwacunu.cmd --commands / --catalog") != std::string::npos,
            "menu command-line section should advertise shell-style menu "
            "and action utilities") &&
        ok;
    ok = require(home_menu_snapshot.find(
                     "open the action palette from the in-app prompt") !=
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "print the active command catalog outside the TUI") !=
                         std::string::npos &&
                     home_menu_snapshot.find("cuwacunu_cmd --commands / "
                                             "--catalog") <
                         home_menu_snapshot.find(
                             "print the active command catalog outside the "
                             "TUI") &&
                     home_menu_snapshot.find("open the action palette from a "
                                             "prompt command") ==
                         std::string::npos &&
                     home_menu_snapshot.find(
                         "open the command palette from a shell-style "
                         "command") == std::string::npos,
                 "menu command-line section should distinguish action palette "
                 "aliases from terminal catalog utilities") &&
         ok;
    ok = require(
             home_menu_snapshot.find("cuwacunu_cmd runtime --snapshot") !=
                     std::string::npos &&
                 home_menu_snapshot.find("cuwacunu.cmd runtime --snapshot") !=
                     std::string::npos &&
                 home_menu_snapshot.find("iinuji_cmd runtime --snapshot") !=
                     std::string::npos &&
                 home_menu_snapshot.find("cuwacunu_cmd --screen runtime") !=
                     std::string::npos &&
                 home_menu_snapshot.find("cuwacunu.cmd --screen runtime") !=
                     std::string::npos &&
                 home_menu_snapshot.find("cuwacunu_cmd runtime --snapshot") <
                     home_menu_snapshot.find(
                         "cuwacunu.cmd runtime --snapshot") &&
                 home_menu_snapshot.find("cuwacunu_cmd --screen runtime") <
                     home_menu_snapshot.find("cuwacunu.cmd --screen runtime"),
             "menu command-line section should advertise shell-style screen "
             "examples and alternate command names as prompt-compatible "
             "commands") &&
         ok;
    ok = require(home_menu_snapshot.find("compatibility") == std::string::npos,
                 "menu overlay should not expose migration-era compatibility "
                 "copy") &&
         ok;
    ok = require(home_menu_snapshot.find("legacy") == std::string::npos,
                 "menu overlay should present restored utilities as current "
                 "commands, not migration artifacts") &&
         ok;
    ok = require(home_menu_snapshot.find("PATH command") == std::string::npos &&
                     home_menu_snapshot.find("alternate command spelling") ==
                         std::string::npos &&
                     home_menu_snapshot.find("alternate menu") ==
                         std::string::npos,
                 "menu overlay should avoid migration-era command-name "
                 "explanations") &&
         ok;
    ok = require(home_menu_snapshot.find("iinuji.home.splash()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.home.animation()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.splash()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.home.farewell()") !=
                         std::string::npos &&
                     home_menu_snapshot.find("iinuji.farewell()") !=
                         std::string::npos,
                 "Home menu canonical path section should expose animation, "
                 "splash, and farewell visual utilities") &&
         ok;
    ok = require(home_menu_snapshot.find(detail_section_rule() + "\nCommand\n" +
                                         detail_section_rule() +
                                         "\n cmd> ready") != std::string::npos,
                 "menu overlay snapshot should keep the command strip "
                 "visible") &&
         ok;
    ok = require(home_menu_snapshot.find("iinuji.home.visual()") !=
                     std::string::npos,
                 "Home menu should advertise the canonical visual path") &&
         ok;
    ok = require(home_menu_snapshot.find("iinuji.config.file.index(n=1)") !=
                     std::string::npos,
                 "menu overlay should advertise config argument selectors") &&
         ok;
    ok = require(home_menu_snapshot.find("iinuji.lattice.target.index(n=1)") !=
                     std::string::npos,
                 "menu overlay should advertise lattice argument selectors") &&
         ok;
    const auto shell_run_row =
        menu_command_row_index(state, "cuwacunu_cmd --run/--command \"show\"");
    ok = require(shell_run_row.has_value(),
                 "Home menu should expose clickable shell-style run row") &&
         ok;
    state.log.clear();
    ok = require(click_scrolled_menu_row(shell_run_row.value_or(0)),
                 "clicking the shell-style run row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "show home" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "screen=home",
                 "shell-style run row should route through the restored show "
                 "utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto shell_catalog_row =
        menu_command_row_index(state, "cuwacunu.cmd --commands / --catalog");
    ok = require(shell_catalog_row.has_value(),
                 "Home menu should expose clickable shell-style catalog row") &&
         ok;
    ok = require(click_scrolled_menu_row(shell_catalog_row.value_or(0)),
                 "clicking the shell-style catalog row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home && state.action_menu_open &&
                     state.status == "action menu" && !state.help_view,
                 "shell-style catalog row should open the action palette") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto shell_command_row =
        menu_command_row_index(state, "cuwacunu.cmd --run/--command \"show\"");
    ok = require(shell_command_row.has_value(),
                 "Home menu should expose clickable --command row") &&
         ok;
    ok = require(click_scrolled_menu_row(shell_command_row.value_or(0)),
                 "clicking the shell-style --command row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "show home" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "screen=home",
                 "shell-style --command row should route through the restored "
                 "show utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto path_shell_screen_flag_row =
        menu_command_row_index(state, "cuwacunu_cmd --screen runtime");
    ok = require(
             path_shell_screen_flag_row.has_value(),
             "Home menu should expose clickable PATH shell screen flag row") &&
         ok;
    ok =
        require(click_scrolled_menu_row(path_shell_screen_flag_row.value_or(0)),
                "clicking the PATH shell screen flag row should dispatch") &&
        ok;
    ok = require(state.screen == ScreenMode::Runtime &&
                     state.status == "screen=runtime" && !state.help_view,
                 "PATH shell screen flag row should switch to Runtime") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto shell_screen_row =
        menu_command_row_index(state, "cuwacunu_cmd runtime --snapshot");
    ok = require(shell_screen_row.has_value(),
                 "Home menu should expose clickable shell-style screen row") &&
         ok;
    ok = require(click_scrolled_menu_row(shell_screen_row.value_or(0)),
                 "clicking the shell-style screen row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Runtime &&
                     state.status == "screen=runtime" && !state.help_view,
                 "shell-style screen row should switch to Runtime") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto alternate_shell_screen_row =
        menu_command_row_index(state, "cuwacunu.cmd runtime --snapshot");
    ok = require(alternate_shell_screen_row.has_value(),
                 "Home menu should expose clickable alternate shell command "
                 "alias row") &&
         ok;
    ok =
        require(
            click_scrolled_menu_row(alternate_shell_screen_row.value_or(0)),
            "clicking the alternate shell command alias row should dispatch") &&
        ok;
    ok =
        require(state.screen == ScreenMode::Runtime &&
                    state.status == "screen=runtime" && !state.help_view,
                "alternate shell command alias row should switch to Runtime") &&
        ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto alternate_iinuji_screen_row =
        menu_command_row_index(state, "iinuji_cmd runtime --snapshot");
    ok = require(alternate_iinuji_screen_row.has_value(),
                 "Home menu should expose clickable alternate iinuji command "
                 "alias row") &&
         ok;
    ok = require(
             click_scrolled_menu_row(alternate_iinuji_screen_row.value_or(0)),
             "clicking the alternate iinuji command alias row should "
             "dispatch") &&
         ok;
    ok = require(
             state.screen == ScreenMode::Runtime &&
                 state.status == "screen=runtime" && !state.help_view,
             "alternate iinuji command alias row should switch to Runtime") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto show_lattice_row =
        menu_command_row_index(state, "show lattice | iinuji.show.lattice()");
    ok = require(show_lattice_row.has_value(),
                 "Home menu should expose clickable show lattice alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + show_lattice_row.value_or(0)),
                 "clicking the show lattice alias row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "show lattice" && !state.help_view,
                 "show lattice menu row should route through the restored "
                 "show utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto show_target_row =
        menu_command_row_index(state, "show target | show proof");
    ok = require(show_target_row.has_value(),
                 "Home menu should expose clickable show target/proof alias "
                 "row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + show_target_row.value_or(0)),
                 "clicking the show target/proof alias row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "show lattice" && !state.help_view,
                 "show target/proof menu row should route through the "
                 "selected target proof utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto runtime_show_row =
        menu_command_row_index(state, "runtime show | show runtime");
    ok =
        require(runtime_show_row.has_value(),
                "Home menu should expose clickable reverse Runtime show row") &&
        ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + runtime_show_row.value_or(0)),
                 "clicking the reverse Runtime show row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Runtime &&
                     state.status == "show runtime" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "screen=runtime",
                 "reverse Runtime show menu row should route through the "
                 "Runtime summary utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto lattice_show_row =
        menu_command_row_index(state, "lattice show | proof show");
    ok =
        require(lattice_show_row.has_value(),
                "Home menu should expose clickable reverse Lattice show row") &&
        ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + lattice_show_row.value_or(0)),
                 "clicking the reverse Lattice show row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "show lattice" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "screen=lattice",
                 "reverse Lattice show menu row should route through the "
                 "selected proof summary") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto splash_row = menu_command_row_index(state, "splash | bootstrap");
    ok = require(splash_row.has_value(),
                 "Home menu should expose clickable splash utility row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + splash_row.value_or(0)),
                 "clicking the splash utility row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "bootstrap splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=bootstrap",
                 "splash menu row should route through the restored visual "
                 "diagnostic utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto splash_loading_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --splash loading");
    ok = require(splash_loading_command_row.has_value(),
                 "Home menu should expose clickable spaced CLI-style loading "
                 "splash row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         splash_loading_command_row.value_or(0)),
                 "clicking the spaced CLI-style loading splash row should "
                 "dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "bootstrap splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=bootstrap",
                 "spaced CLI-style loading splash row should route through "
                 "the bootstrap visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto bootstrap_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --bootstrap / --boot");
    ok = require(bootstrap_command_row.has_value(),
                 "Home menu should expose clickable CLI-style bootstrap row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         bootstrap_command_row.value_or(0)),
                 "clicking the CLI-style bootstrap row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "bootstrap splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=bootstrap",
                 "CLI-style bootstrap menu row should route through the "
                 "bootstrap visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto farewell_spaced_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --splash farewell");
    ok = require(farewell_spaced_command_row.has_value(),
                 "Home menu should expose clickable spaced CLI-style farewell "
                 "row") &&
         ok;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui, ui.menu_commands->screen.x + 1,
                    ui.menu_commands->screen.y +
                        farewell_spaced_command_row.value_or(0)),
                "clicking the spaced CLI-style farewell row should dispatch") &&
        ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "spaced CLI-style farewell menu row should route through the "
                 "closing visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto farewell_close_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --splash close");
    ok = require(farewell_close_command_row.has_value(),
                 "Home menu should expose clickable spaced CLI-style close "
                 "splash row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         farewell_close_command_row.value_or(0)),
                 "clicking the spaced CLI-style close splash row should "
                 "dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "spaced CLI-style close splash row should route through the "
                 "closing visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto farewell_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --splash=farewell");
    ok = require(farewell_command_row.has_value(),
                 "Home menu should expose clickable CLI-style farewell row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + farewell_command_row.value_or(0)),
             "clicking the CLI-style farewell row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "CLI-style farewell menu row should route through the closing "
                 "visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto good_luck_command_row =
        menu_command_row_index(state, "cuwacunu_cmd --splash=good-luck");
    ok = require(good_luck_command_row.has_value(),
                 "Home menu should expose clickable Good luck splash row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         good_luck_command_row.value_or(0)),
                 "clicking the Good luck splash row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "Good luck splash menu row should route through the closing "
                 "visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto animation_current_command_row =
        menu_current_screen_row_index(state, "cuwacunu_cmd --animation");
    ok = require(animation_current_command_row.has_value(),
                 "Home current-screen menu should expose clickable animation "
                 "snapshot row") &&
         ok;
    ok = require(
             click_scrolled_menu_row(animation_current_command_row.value_or(0)),
             "clicking the current-screen animation row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "home visual" && !state.help_view,
                 "current-screen animation row should route through the Home "
                 "visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto farewell_current_row =
        menu_current_screen_row_index(state, "farewell / closing");
    ok = require(
             farewell_current_row.has_value(),
             "Home current-screen menu should expose farewell utility row") &&
         ok;
    ok = require(click_scrolled_menu_row(farewell_current_row.value_or(0)),
                 "clicking the current-screen farewell row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "current-screen farewell row should route through the "
                 "restored closing visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto farewell_current_command_row =
        menu_current_screen_row_index(state, "cuwacunu_cmd --splash=farewell");
    ok = require(farewell_current_command_row.has_value(),
                 "Home current-screen menu should expose clickable "
                 "CLI-style farewell row") &&
         ok;
    ok = require(
             click_scrolled_menu_row(farewell_current_command_row.value_or(0)),
             "clicking the current-screen CLI-style farewell row should "
             "dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "current-screen CLI-style farewell row should route through "
                 "the restored closing visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    state.log.clear();
    const auto good_luck_current_command_row =
        menu_current_screen_row_index(state, "cuwacunu_cmd --splash=good-luck");
    ok = require(good_luck_current_command_row.has_value(),
                 "Home current-screen menu should expose clickable Good luck "
                 "splash row") &&
         ok;
    ok = require(
             click_scrolled_menu_row(good_luck_current_command_row.value_or(0)),
             "clicking the current-screen Good luck splash row should "
             "dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "farewell splash" && !state.help_view &&
                     !state.log.empty() &&
                     state.log.front().message == "splash=farewell",
                 "current-screen Good luck row should route through the "
                 "restored closing visual utility") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto inbox_row =
        menu_command_row_index(state, "iinuji.screen.inbox()");
    ok = require(!inbox_row.has_value(),
                 "Home menu should keep removed canonical inbox path hidden") &&
         ok;
    const auto home_workbench_row =
        menu_command_row_index(state, "workbench | work | w | f2 | 2");
    ok = require(home_workbench_row.has_value(),
                 "Home menu should expose the current Workbench F2 row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + home_workbench_row.value_or(0)),
             "clicking the Workbench row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Workbench && !state.help_view,
                 "Workbench menu row should open the blank Workbench and close "
                 "menu") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto first = menu_current_screen_first_row(state);
    ok = require(first.has_value(),
                 "Home menu should expose current-screen shortcut rows") &&
         ok;
    const std::string command = "F2 / F3 / F4 / F8 / F9";
    const std::size_t f4_offset = command.find("F4");
    ok = require(f4_offset != std::string::npos,
                 "Home current-screen shortcut row should contain F4") &&
         ok;
    ok =
        require(click_scrolled_menu_row_at(*first, static_cast<int>(f4_offset)),
                "clicking Home current F4 shortcut should dispatch") &&
        ok;
    ok = require(state.screen == ScreenMode::Lattice && !state.help_view,
                 "Home current F4 shortcut should switch to Lattice and close "
                 "menu") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Home;
    const auto home_visual_row =
        menu_current_screen_row_index(state, "visual / waajacamaya");
    ok = require(home_visual_row.has_value(),
                 "Home menu should expose clickable visual utility row") &&
         ok;
    ok = require(click_scrolled_menu_row(home_visual_row.value_or(0)),
                 "clicking Home visual utility row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Home &&
                     state.status == "home visual" && !state.help_view,
                 "Home visual utility row should run the restored visual path "
                 "and close menu") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = ScreenMode::Workbench;
    const auto workbench_row =
        menu_current_screen_row_index(state, "F3 / F4 / F8 / F9");
    ok = require(workbench_row.has_value(),
                 "blank Workbench menu should expose screen escape row") &&
         ok;
    const std::string workbench_command = "F3 / F4 / F8 / F9";
    const std::size_t f8_offset = workbench_command.find("F8");
    ok = require(f8_offset != std::string::npos,
                 "Workbench escape row should contain F8") &&
         ok;
    ok = require(click_scrolled_menu_row_at(workbench_row.value_or(0),
                                            static_cast<int>(f8_offset)),
                 "clicking Workbench current F8 shortcut should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Logs && !state.help_view,
                 "Workbench current F8 shortcut should switch to Shell Logs "
                 "and close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Home;
  {
    const auto snapshot_actions_row =
        menu_command_row_index(state, "cuwacunu_cmd --snapshot --actions");
    ok = require(snapshot_actions_row.has_value(),
                 "menu should expose clickable shell snapshot actions row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_actions_row.value_or(0)),
                 "clicking the shell snapshot actions row should dispatch") &&
         ok;
  }
  ok = require(state.action_menu_open && !state.help_view &&
                   state.status == "action menu",
               "shell snapshot actions row should open the action palette") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  {
    const auto snapshot_full_row =
        menu_command_row_index(state, "cuwacunu_cmd --snapshot --full");
    ok = require(snapshot_full_row.has_value(),
                 "menu should expose clickable shell snapshot full row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_full_row.value_or(0)),
                 "clicking the shell snapshot full row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime && !state.help_view &&
                   workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace full",
               "shell snapshot full row should use the workspace full route") &&
       ok;
  state.workspace.zoom_flags = 0u;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  {
    const auto snapshot_visual_row =
        menu_command_row_index(state, "cuwacunu_cmd --snapshot --visual");
    ok = require(snapshot_visual_row.has_value(),
                 "menu should expose clickable shell snapshot visual row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_visual_row.value_or(0)),
                 "clicking the shell snapshot visual row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "home visual" && !state.help_view,
               "shell snapshot visual row should route through the Home visual "
               "utility") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  {
    const auto snapshot_splash_row = menu_command_row_index(
        state, "cuwacunu_cmd --snapshot --splash loading");
    ok = require(snapshot_splash_row.has_value(),
                 "menu should expose clickable shell snapshot splash row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_splash_row.value_or(0)),
                 "clicking the shell snapshot splash row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "bootstrap splash" && !state.help_view &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=bootstrap",
               "shell snapshot splash row should route through the bootstrap "
               "visual utility") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  {
    const auto snapshot_closing_row =
        menu_command_row_index(state, "cuwacunu_cmd --snapshot --splash close");
    ok = require(snapshot_closing_row.has_value(),
                 "menu should expose clickable shell snapshot closing row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_closing_row.value_or(0)),
                 "clicking the shell snapshot closing row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" && !state.help_view &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "shell snapshot closing row should route through the farewell "
               "visual utility") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.log.clear();
  {
    const auto snapshot_good_luck_row = menu_command_row_index(
        state, "cuwacunu_cmd --snapshot --splash good-luck");
    ok = require(snapshot_good_luck_row.has_value(),
                 "menu should expose clickable shell snapshot Good luck row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_good_luck_row.value_or(0)),
                 "clicking the shell snapshot Good luck row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home &&
                   state.status == "farewell splash" && !state.help_view &&
                   !state.log.empty() &&
                   state.log.front().message == "splash=farewell",
               "shell snapshot Good luck row should route through the "
               "farewell visual utility") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Home;
  {
    const auto snapshot_runtime_row = menu_command_row_index(
        state, "cuwacunu.cmd --snapshot --screen runtime");
    ok = require(snapshot_runtime_row.has_value(),
                 "menu should expose clickable shell snapshot screen row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_runtime_row.value_or(0)),
                 "clicking the shell snapshot screen row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.status == "screen=runtime" && !state.help_view,
               "shell snapshot screen row should switch to Runtime") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Home;
  state.config.files = {
      ConfigFileSummary{.path = "/cuwacunu/src/config/hero.config.dsl",
                        .relative_path = "hero.config.dsl"},
      ConfigFileSummary{.path = "/cuwacunu/src/config/hero.runtime.dsl",
                        .relative_path = "hero.runtime.dsl"},
  };
  {
    const auto snapshot_config_path_row = menu_command_row_index(
        state, "cuwacunu.cmd hero.runtime.dsl --snapshot");
    ok = require(snapshot_config_path_row.has_value(),
                 "menu should expose clickable shell snapshot managed Config "
                 "path row") &&
         ok;
    ok = require(click_scrolled_menu_row(snapshot_config_path_row.value_or(0)),
                 "clicking the shell snapshot managed Config path row should "
                 "dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Config &&
                   state.config.selected_file == 1u &&
                   state.status == "selected file=2" && !state.help_view,
               "shell snapshot managed Config path row should open Config on "
               "the matching managed file") &&
       ok;
  state.config.files.clear();

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  ok =
      require(dispatch_menu_overlay_click(
                  actions, state, ui, ui.menu_commands->screen.x + 1,
                  ui.menu_commands->screen.y + kMenuActionsRow),
              "clicking the actions menu row should open the action palette") &&
      ok;
  ok = require(
           state.action_menu_open && !state.help_view,
           "actions row click should swap menu overlay for action palette") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const auto actions_run_row = menu_command_row_index(state, "actions run");
    ok = require(actions_run_row.has_value(),
                 "menu should expose the actions run alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + actions_run_row.value_or(0)),
                 "clicking actions run from the menu should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Home && state.action_menu_open &&
                   !state.help_view,
               "actions run menu row should open the palette when no selected "
               "palette row is visible") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  {
    const std::string command = "help down | help page down";
    const auto help_page_down_row = menu_command_row_index(state, command);
    ok = require(help_page_down_row.has_value(),
                 "menu should expose the help down/page down alias row") &&
         ok;
    const std::size_t page_down_offset = command.find("page down");
    ok = require(page_down_offset != std::string::npos,
                 "help down/page down alias row should contain page down "
                 "label") &&
         ok;
    ok =
        require(
            dispatch_menu_overlay_click(
                actions, state, ui,
                ui.menu_commands->screen.x + static_cast<int>(page_down_offset),
                ui.menu_commands->screen.y + help_page_down_row.value_or(0)),
            "clicking help page down label should dispatch") &&
        ok;
  }
  ok = require(state.help_view && state.help_scroll_y == 20 &&
                   state.status == "help scroll=page-down",
               "help page down label click should use the legacy twenty-row "
               "scroll step") &&
       ok;

  state.help_scroll_y = 0;
  {
    const std::string command = "menu down | menu page down";
    const auto menu_page_down_row = menu_command_row_index(state, command);
    ok = require(menu_page_down_row.has_value(),
                 "menu should expose the menu down/page down alias row") &&
         ok;
    const std::size_t page_down_offset = command.find("page down");
    ok = require(page_down_offset != std::string::npos,
                 "menu down/page down alias row should contain page down "
                 "label") &&
         ok;
    ok =
        require(
            dispatch_menu_overlay_click(
                actions, state, ui,
                ui.menu_commands->screen.x + static_cast<int>(page_down_offset),
                ui.menu_commands->screen.y + menu_page_down_row.value_or(0)),
            "clicking menu page down label should dispatch") &&
        ok;
  }
  ok = require(state.help_view && state.help_scroll_y == 20 &&
                   state.status == "help scroll=page-down",
               "menu page down label click should use the same twenty-row "
               "scroll step") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.help_scroll_x = 0;
  {
    const std::string command = "help/menu left | right";
    const auto help_right_row = menu_command_row_index(state, command);
    ok = require(help_right_row.has_value(),
                 "menu should expose the help/menu left/right utility row") &&
         ok;
    const std::size_t right_offset = command.find("right");
    ok = require(right_offset != std::string::npos,
                 "help/menu left/right row should contain right label") &&
         ok;
    const int row = help_right_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui,
                    ui.menu_commands->screen.x + static_cast<int>(right_offset),
                    ui.menu_commands->screen.y + row - state.help_scroll_y),
                "clicking help right label should dispatch") &&
        ok;
  }
  ok = require(state.help_view && state.help_scroll_x == 16 &&
                   state.status == "help scroll=right",
               "help right label click should use the legacy sixteen-column "
               "scroll step") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.help_scroll_y = 0;
  state.action_menu_selected = 0u;
  {
    const std::string command = "actions page up | page down";
    const auto action_page_down_row = menu_command_row_index(state, command);
    ok = require(action_page_down_row.has_value(),
                 "menu should expose direct action palette page row") &&
         ok;
    const std::size_t page_down_offset = command.find("page down");
    ok = require(page_down_offset != std::string::npos,
                 "action palette page row should contain page down label") &&
         ok;
    const int row = action_page_down_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x +
                         static_cast<int>(page_down_offset),
                     ui.menu_commands->screen.y + row - state.help_scroll_y),
                 "clicking action palette page down label should dispatch") &&
         ok;
  }
  ok = require(state.action_menu_open && !state.help_view &&
                   state.action_menu_selected == 8u &&
                   state.status == "action menu",
               "action palette page down label should open the palette and "
               "move selection by the legacy page step") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.help_scroll_y = 0;
  state.action_menu_selected = 4u;
  {
    const std::string command = "actions home | end";
    const auto action_home_row = menu_command_row_index(state, command);
    ok = require(action_home_row.has_value(),
                 "menu should expose direct action palette home/end row") &&
         ok;
    const std::size_t home_offset = command.find("home");
    ok = require(home_offset != std::string::npos,
                 "action palette home/end row should contain home label") &&
         ok;
    const int row = action_home_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(home_offset),
                     ui.menu_commands->screen.y + row - state.help_scroll_y),
                 "clicking action palette home label should dispatch") &&
         ok;
  }
  ok = require(state.action_menu_open && !state.help_view &&
                   state.action_menu_selected == 0u &&
                   state.status == "action menu",
               "action palette home label should open the palette on the "
               "first row") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.help_scroll_y = 0;
  state.runtime.focus = RuntimeFocus::Devices;
  {
    const std::string command = "runtime devices | jobs";
    const auto runtime_jobs_row = menu_command_row_index(state, command);
    ok = require(runtime_jobs_row.has_value(),
                 "menu should expose direct Runtime devices/jobs row") &&
         ok;
    const std::size_t jobs_offset = command.find("jobs");
    ok = require(jobs_offset != std::string::npos,
                 "Runtime devices/jobs row should contain jobs label") &&
         ok;
    const int row = runtime_jobs_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(jobs_offset),
                     ui.menu_commands->screen.y + row - state.help_scroll_y),
                 "clicking Runtime jobs label should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.focus == RuntimeFocus::Jobs &&
                   state.status == "runtime lane=Jobs" && !state.help_view,
               "Runtime jobs utility label should focus Jobs and close menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.help_scroll_y = 0;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  {
    const std::string command = "runtime manifest | log | trace";
    const auto runtime_trace_row = menu_command_row_index(state, command);
    ok = require(runtime_trace_row.has_value(),
                 "menu should expose direct Runtime detail utility row") &&
         ok;
    const std::size_t trace_offset = command.find("trace");
    ok = require(trace_offset != std::string::npos,
                 "Runtime detail utility row should contain trace label") &&
         ok;
    const int row = runtime_trace_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui,
                    ui.menu_commands->screen.x + static_cast<int>(trace_offset),
                    ui.menu_commands->screen.y + row - state.help_scroll_y),
                "clicking Runtime trace label should dispatch") &&
        ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                   state.status == "runtime detail=trace" && !state.help_view,
               "Runtime trace utility label should choose trace and close "
               "menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  ok = require(dispatch_menu_overlay_click(
                   actions, state, ui, ui.menu_commands->screen.x + 1,
                   ui.menu_commands->screen.y + kMenuCloseRow),
               "clicking the close menu row should dispatch") &&
       ok;
  ok = require(!state.help_view, "close row click should hide the menu") && ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.log = {LogEntry{0, "command", "seed"}};
  {
    const auto clear_logs_row =
        menu_command_row_index(state, "clear logs | logs clear | clear");
    ok = require(clear_logs_row.has_value(),
                 "menu should expose the clear logs alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + clear_logs_row.value_or(0)),
                 "clicking the clear logs alias row should dispatch") &&
         ok;
  }
  ok =
      require(state.screen == ScreenMode::Logs && state.log.size() == 1u &&
                  state.log.back().source == "action" && !state.help_view,
              "clear logs alias row should clear feed, audit action, and close "
              "menu") &&
      ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.logs.show_timestamp = false;
  {
    const auto timestamp_row =
        menu_command_row_index(state, "logs time | timestamp");
    ok = require(timestamp_row.has_value(),
                 "menu should expose the timestamp alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + timestamp_row.value_or(0)),
                 "clicking the timestamp alias row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Logs && state.logs.show_timestamp &&
                   !state.help_view,
               "timestamp alias row should toggle timestamps and close menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.logs.source_filter = LogsSourceFilter::All;
  {
    const std::string command = "logs source show | status";
    const auto source_status_row = menu_command_row_index(state, command);
    ok = require(source_status_row.has_value(),
                 "menu should expose direct Shell Logs source show/status "
                 "row") &&
         ok;
    const std::size_t status_offset = command.find("status");
    ok = require(status_offset != std::string::npos,
                 "Shell Logs source show/status row should contain status "
                 "label") &&
         ok;
    const int row = source_status_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui,
                 ui.menu_commands->screen.x + static_cast<int>(status_offset),
                 ui.menu_commands->screen.y + row - state.help_scroll_y),
             "clicking the Shell Logs source status label should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Logs &&
                   state.logs.source_filter == LogsSourceFilter::Status &&
                   state.status == "logs.source=status" && !state.help_view,
               "Shell Logs source status label should set the direct source "
               "lens") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.logs.selected_setting = 0;
  {
    const std::string command = "logs setting prev | next";
    const auto setting_next_row = menu_command_row_index(state, command);
    ok = require(setting_next_row.has_value(),
                 "menu should expose direct Shell Logs setting cursor row") &&
         ok;
    const std::size_t next_offset = command.find("next");
    ok = require(next_offset != std::string::npos,
                 "Shell Logs setting cursor row should contain next label") &&
         ok;
    const int row = setting_next_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui,
                    ui.menu_commands->screen.x + static_cast<int>(next_offset),
                    ui.menu_commands->screen.y + row - state.help_scroll_y),
                "clicking the Shell Logs setting next label should dispatch") &&
        ok;
  }
  ok = require(
           state.screen == ScreenMode::Logs &&
               state.logs.selected_setting == 1u &&
               state.status == "logs.settings.cursor=next" && !state.help_view,
           "Shell Logs setting next label should move the settings cursor") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.logs.selected_setting = 1;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  {
    const std::string command = "logs setting left | right";
    const auto setting_right_row = menu_command_row_index(state, command);
    ok = require(setting_right_row.has_value(),
                 "menu should expose direct Shell Logs setting change row") &&
         ok;
    const std::size_t right_offset = command.find("right");
    ok = require(right_offset != std::string::npos,
                 "Shell Logs setting change row should contain right label") &&
         ok;
    const int row = setting_right_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui,
                    ui.menu_commands->screen.x + static_cast<int>(right_offset),
                    ui.menu_commands->screen.y + row - state.help_scroll_y),
                "clicking the Shell Logs setting right label should "
                "dispatch") &&
        ok;
  }
  ok = require(
           state.screen == ScreenMode::Logs &&
               state.logs.severity_filter ==
                   LogsSeverityFilter::WarningOrHigher &&
               state.status == "logs.level=WARNING+" && !state.help_view,
           "Shell Logs setting right label should change the selected lens") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.logs.scroll_y = 0;
  state.logs.auto_follow = true;
  {
    const std::string command = "logs home | end";
    const auto logs_end_row = menu_command_row_index(state, command);
    ok = require(logs_end_row.has_value(),
                 "menu should expose direct Shell Logs home/end row") &&
         ok;
    const std::size_t end_offset = command.find("end");
    ok = require(end_offset != std::string::npos,
                 "Shell Logs home/end row should contain end label") &&
         ok;
    const int row = logs_end_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(end_offset),
                     ui.menu_commands->screen.y + row - state.help_scroll_y),
                 "clicking the Shell Logs end label should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Logs &&
                   state.logs.scroll_y == std::numeric_limits<int>::max() &&
                   state.logs.auto_follow &&
                   state.status == "logs scroll=end" && !state.help_view,
               "Shell Logs end label should jump to newest and keep follow") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  {
    const std::string command = "logs debug | info | warning";
    const auto severity_warning_row = menu_command_row_index(state, command);
    ok = require(severity_warning_row.has_value(),
                 "menu should expose direct Shell Logs severity row") &&
         ok;
    const std::size_t warning_offset = command.find("warning");
    ok = require(warning_offset != std::string::npos,
                 "Shell Logs severity row should contain warning label") &&
         ok;
    const int row = severity_warning_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui,
                 ui.menu_commands->screen.x + static_cast<int>(warning_offset),
                 ui.menu_commands->screen.y + row - state.help_scroll_y),
             "clicking the Shell Logs warning severity label should "
             "dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Logs &&
                   state.logs.severity_filter ==
                       LogsSeverityFilter::WarningOrHigher &&
                   state.status == "logs.level=WARNING+" && !state.help_view,
               "Shell Logs warning label should set the direct severity "
               "lens") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.logs.metadata_filter = LogsMetadataFilter::Any;
  {
    const std::string command = "logs meta function | path";
    const auto metadata_path_row = menu_command_row_index(state, command);
    ok = require(metadata_path_row.has_value(),
                 "menu should expose direct Shell Logs metadata row") &&
         ok;
    const std::size_t path_offset = command.find("path");
    ok = require(path_offset != std::string::npos,
                 "Shell Logs metadata row should contain path label") &&
         ok;
    const int row = metadata_path_row.value_or(0);
    state.help_scroll_y = row > 0 ? row - 1 : 0;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui,
                 ui.menu_commands->screen.x + static_cast<int>(path_offset),
                 ui.menu_commands->screen.y + row - state.help_scroll_y),
             "clicking the Shell Logs metadata path label should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Logs &&
                   state.logs.metadata_filter == LogsMetadataFilter::WithPath &&
                   state.status == "logs.metadata_filter=PATH+" &&
                   !state.help_view,
               "Shell Logs metadata path label should set the direct metadata "
               "lens") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.logs.mouse_capture = false;
  {
    const std::string command = "logs color | mouse";
    const auto mouse_row = menu_command_row_index(state, command);
    ok = require(mouse_row.has_value(),
                 "menu should expose the logs color/mouse alias row") &&
         ok;
    const std::size_t mouse_offset = command.find("mouse");
    ok = require(mouse_offset != std::string::npos,
                 "logs color/mouse alias row should contain mouse label") &&
         ok;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui,
                    ui.menu_commands->screen.x + static_cast<int>(mouse_offset),
                    ui.menu_commands->screen.y + mouse_row.value_or(0)),
                "clicking the mouse label in the logs alias row should "
                "dispatch") &&
        ok;
  }
  ok = require(state.screen == ScreenMode::Logs && state.logs.mouse_capture &&
                   !state.help_view,
               "mouse label click should toggle logs mouse capture") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.runtime.focus = RuntimeFocus::Devices;
  state.runtime.device.gpus = {
      RuntimeGpuSnapshot{.index = "0", .name = "gpu.alpha"},
      RuntimeGpuSnapshot{.index = "1", .name = "gpu.beta"},
  };
  state.runtime.selected_device_row = 1u;
  {
    const std::string command = "runtime down / up";
    const auto runtime_up_row = menu_command_row_index(state, command);
    ok = require(runtime_up_row.has_value(),
                 "menu should expose the runtime down/up alias row") &&
         ok;
    const std::size_t up_offset = command.find("up");
    ok = require(up_offset != std::string::npos,
                 "runtime down/up alias row should contain up label") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(up_offset),
                     ui.menu_commands->screen.y + runtime_up_row.value_or(0)),
                 "clicking the up label in the runtime alias row should "
                 "dispatch") &&
         ok;
  }
  ok = require(
           state.screen == ScreenMode::Runtime &&
               state.runtime.selected_device_row == 0u && !state.help_view &&
               state.status == "runtime device=prev",
           "runtime up label click should select the previous visible row") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.runtime.focus = RuntimeFocus::Jobs;
  {
    const std::string command = "runtime right / left";
    const auto runtime_left_row = menu_command_row_index(state, command);
    ok = require(runtime_left_row.has_value(),
                 "menu should expose the runtime right/left alias row") &&
         ok;
    const std::size_t left_offset = command.find("left");
    ok = require(left_offset != std::string::npos,
                 "runtime right/left alias row should contain left label") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(left_offset),
                     ui.menu_commands->screen.y + runtime_left_row.value_or(0)),
                 "clicking the left label in the runtime alias row should "
                 "dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.focus == RuntimeFocus::Devices &&
                   !state.help_view && state.status == "runtime lane=Devices",
               "runtime left label click should focus Devices") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.lattice.target_ids = {"target.alpha", "target.beta", "target.gamma"};
  state.lattice.selected_target = 1u;
  {
    const std::string command = "lattice down / up";
    const auto lattice_up_row = menu_command_row_index(state, command);
    ok = require(lattice_up_row.has_value(),
                 "menu should expose the lattice down/up alias row") &&
         ok;
    const std::size_t up_offset = command.find("up");
    ok = require(up_offset != std::string::npos,
                 "lattice down/up alias row should contain up label") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(up_offset),
                     ui.menu_commands->screen.y + lattice_up_row.value_or(0)),
                 "clicking the up label in the lattice alias row should "
                 "dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.lattice.selected_target == 0u && !state.help_view &&
                   state.status == "lattice target",
               "lattice up label click should select the previous target") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.lattice.selected_target = 2u;
  {
    const std::string command = "lattice target n1 / id";
    const auto lattice_n1_row = menu_command_row_index(state, command);
    ok = require(lattice_n1_row.has_value(),
                 "menu should expose the lattice target example alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y + lattice_n1_row.value_or(0)),
                 "clicking the lattice target example alias row should "
                 "dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.lattice.selected_target == 0u && !state.help_view &&
                   state.status == "lattice target 1/3",
               "lattice target example row should execute its n1 selector") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.lattice.selected_target = 2u;
  {
    const std::string command = "lattice index(n=1)";
    const auto lattice_index_row = menu_command_row_index(state, command);
    ok = require(lattice_index_row.has_value(),
                 "menu should expose the lattice index argument alias row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + lattice_index_row.value_or(0)),
             "clicking the lattice index argument alias row should dispatch") &&
         ok;
  }
  ok =
      require(
          state.screen == ScreenMode::Lattice &&
              state.lattice.selected_target == 0u && !state.help_view &&
              state.status == "lattice target 1/3",
          "lattice index argument row should execute its canonical selector") &&
      ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.config.files = {
      ConfigFileSummary{.path = "/tmp/alpha.dsl", .relative_path = "alpha.dsl"},
      ConfigFileSummary{.path = "/tmp/beta.dsl", .relative_path = "beta.dsl"},
      ConfigFileSummary{.path = "/tmp/gamma.dsl", .relative_path = "gamma.dsl"},
  };
  if (state.config.files.size() > 1u) {
    state.config.selected_file = 1u;
    const std::string command = "config down / up";
    const auto config_prev_row = menu_command_row_index(state, command);
    ok = require(config_prev_row.has_value(),
                 "menu should expose the config down/up alias row") &&
         ok;
    const std::size_t prev_offset = command.find("up");
    ok = require(prev_offset != std::string::npos,
                 "config down/up alias row should contain up label") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x + static_cast<int>(prev_offset),
                     ui.menu_commands->screen.y + config_prev_row.value_or(0)),
                 "clicking the up label in the config alias row should "
                 "dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Config &&
                     state.config.selected_file == 0u && !state.help_view &&
                     state.status == "selected file=1",
                 "config up label click should select the previous file with "
                 "legacy status") &&
         ok;
    state.help_view = true;
    state.action_menu_open = false;
    state.screen = ScreenMode::Home;
  }

  state.config.selected_file = 2u;
  {
    const std::string command = "config file n1 / id";
    const auto config_n1_row = menu_command_row_index(state, command);
    ok = require(config_n1_row.has_value(),
                 "menu should expose the config file example alias row") &&
         ok;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui, ui.menu_commands->screen.x + 1,
                    ui.menu_commands->screen.y + config_n1_row.value_or(0)),
                "clicking the config file example alias row should dispatch") &&
        ok;
  }
  ok = require(state.screen == ScreenMode::Config &&
                   state.config.selected_file == 0u && !state.help_view &&
                   state.status == "selected file=1",
               "config file example row should execute its n1 selector") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.config.selected_file = 2u;
  {
    const std::string command = "config index(n=1)";
    const auto config_index_row = menu_command_row_index(state, command);
    ok = require(config_index_row.has_value(),
                 "menu should expose the config index argument alias row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + config_index_row.value_or(0)),
             "clicking the config index argument alias row should dispatch") &&
         ok;
  }
  ok = require(
           state.screen == ScreenMode::Config &&
               state.config.selected_file == 0u && !state.help_view &&
               state.status == "selected file=1",
           "config index argument row should execute its canonical selector") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const std::string command = "files | show file";
    const auto show_file_row = menu_command_row_index(state, command);
    ok = require(show_file_row.has_value(),
                 "menu should expose the Config show file alias row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui,
                     ui.menu_commands->screen.x +
                         static_cast<int>(command.find("show file")),
                     ui.menu_commands->screen.y + show_file_row.value_or(0)),
                 "clicking the Config show file alias label should dispatch") &&
         ok;
  }
  ok = require(
           state.screen == ScreenMode::Config &&
               state.status == "show config" && !state.help_view,
           "Config show file alias label should show selected Config preview "
           "and close menu") &&
       ok;
  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Config;
  {
    const std::string config_menu_snapshot =
        make_menu_overlay_snapshot_text(state);
    ok = require(config_menu_snapshot.find("inspect managed files without "
                                           "edits") != std::string::npos &&
                     config_menu_snapshot.find("editing returns later") ==
                         std::string::npos,
                 "Config menu current rows should describe read-only "
                 "inspection without internal write-back copy") &&
         ok;
  }
  state.screen = ScreenMode::Lattice;
  {
    const std::string lattice_menu_snapshot =
        make_menu_overlay_snapshot_text(state);
    ok = require(lattice_menu_snapshot.find("inspect target evidence without "
                                            "edits") != std::string::npos &&
                     lattice_menu_snapshot.find("fresh Lattice Hero routing") ==
                         std::string::npos,
                 "Lattice menu current rows should describe read-only "
                 "inspection without routing-status copy") &&
         ok;
  }

  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Jobs;
  {
    const std::string menu_snapshot = make_menu_overlay_snapshot_text(state);
    ok = require(menu_snapshot.find("Menu overlay") != std::string::npos,
                 "menu snapshot should render the overlay surface") &&
         ok;
    ok = require(menu_snapshot.find("Esc / [x]") != std::string::npos,
                 "menu snapshot should expose the restored close corner "
                 "affordance") &&
         ok;
    ok = require(menu_snapshot.find("Status\n") != std::string::npos,
                 "menu snapshot should include the operator status block") &&
         ok;
    ok = require(menu_snapshot.find(detail_section_rule() + "\nStatus\n" +
                                    detail_section_rule()) != std::string::npos,
                 "menu snapshot should frame the operator status block") &&
         ok;
    ok = require(menu_snapshot.find("CURRENT SCREEN") != std::string::npos,
                 "menu snapshot should include current-screen rows") &&
         ok;
    ok =
        require(menu_snapshot.find("F3  RUNTIME") != std::string::npos,
                "menu snapshot screen list should use restored F-key badges") &&
        ok;
    ok = require(menu_snapshot.find("Left / Right") != std::string::npos &&
                     menu_snapshot.find("k / j / h / l") != std::string::npos,
                 "menu snapshot should expose legacy Runtime movement utility "
                 "rows") &&
         ok;
    ok = require(
             menu_snapshot.find("Enter menu | a actions") != std::string::npos,
             "menu snapshot should expose distinct Runtime Enter and action "
             "utilities") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.commands.*()") != std::string::npos,
                 "menu snapshot should advertise the commands namespace") &&
         ok;
    ok = require(menu_snapshot.find("cuwacunu_cmd --snapshot --menu") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --menu") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --full") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --full") !=
                         std::string::npos,
                 "menu snapshot should advertise snapshot-wrapped shell "
                 "utilities") &&
         ok;
    ok = require(menu_snapshot.find("cuwacunu_cmd --snapshot --menu") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --menu") !=
                         std::string::npos &&
                     menu_snapshot.find("render menu overlay snapshot") !=
                         std::string::npos &&
                     menu_snapshot.find("render menu/actions/visual/splash/"
                                        "full/screen snapshots") ==
                         std::string::npos,
                 "menu snapshot should describe menu snapshot rows as menu "
                 "overlay utilities") &&
         ok;
    ok = require(menu_snapshot.find("cuwacunu_cmd --snapshot --actions") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --actions") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --catalog") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --catalog") !=
                         std::string::npos &&
                     menu_snapshot.find("render command catalog snapshot") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --visual") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --visual") !=
                         std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --splash "
                                        "loading") != std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --splash "
                                        "loading") != std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --splash "
                                        "close") != std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --splash "
                                        "close") != std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --splash "
                                        "good-luck") != std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --splash "
                                        "good-luck") != std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd --snapshot --screen "
                                        "runtime") != std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd --snapshot --screen "
                                        "runtime") != std::string::npos &&
                     menu_snapshot.find("cuwacunu_cmd hero.runtime.dsl "
                                        "--snapshot") != std::string::npos &&
                     menu_snapshot.find("cuwacunu.cmd hero.runtime.dsl "
                                        "--snapshot") != std::string::npos,
                 "menu snapshot should advertise the complete shell snapshot "
                 "utility family") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.show.*()") != std::string::npos &&
                     menu_snapshot.find("show Home, Workbench, Runtime, "
                                        "Lattice, Logs, Config") !=
                         std::string::npos,
                 "menu snapshot should advertise legacy show paths") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.screen.marshal()") ==
                         std::string::npos &&
                     menu_snapshot.find("iinuji.screen.inbox()") ==
                         std::string::npos &&
                     menu_snapshot.find("iinuji.screen.human()") ==
                         std::string::npos,
                 "menu snapshot should keep removed canonical F2 hero paths "
                 "hidden") &&
         ok;
    ok = require(menu_snapshot.find(detail_section_rule() + "\nCommand\n" +
                                    detail_section_rule() + "\n cmd> ready") !=
                     std::string::npos,
                 "runtime menu snapshot should keep the command strip "
                 "visible") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.workbench.refresh()") !=
                     std::string::npos,
                 "menu snapshot should advertise Workbench refresh path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.runtime.refresh()") !=
                     std::string::npos,
                 "menu snapshot should advertise Runtime refresh path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.lattice.refresh()") !=
                     std::string::npos,
                 "menu snapshot should advertise Lattice refresh path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.config.reload()") !=
                     std::string::npos,
                 "menu snapshot should advertise Config reload path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.config.files()") !=
                     std::string::npos,
                 "menu snapshot should advertise Config files path") &&
         ok;
    ok =
        require(menu_snapshot.find("iinuji.config.show()") != std::string::npos,
                "menu snapshot should advertise Config show path") &&
        ok;
    ok = require(menu_snapshot.find("iinuji.workspace.*()") !=
                         std::string::npos &&
                     menu_snapshot.find("iinuji.workspace.zoom.toggle()") !=
                         std::string::npos &&
                     menu_snapshot.find("iinuji.workspace.split()") !=
                         std::string::npos,
                 "menu snapshot should advertise workspace utilities") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.runtime.devices()") !=
                     std::string::npos,
                 "menu snapshot should advertise direct Runtime focus paths") &&
         ok;
    ok = require(menu_snapshot.find("quit | exit | q | x") != std::string::npos,
                 "menu snapshot should advertise the legacy x exit alias") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.logs.settings.source.next()") !=
                     std::string::npos,
                 "menu snapshot should advertise canonical Shell Logs source "
                 "cycle path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.logs.settings.level.*()") !=
                     std::string::npos,
                 "menu snapshot should advertise canonical Shell Logs "
                 "severity namespace") &&
         ok;
    ok = require(
             menu_snapshot.find("iinuji.logs.settings.date.toggle()") !=
                     std::string::npos &&
                 menu_snapshot.find("iinuji.logs.settings.follow.toggle()") !=
                     std::string::npos &&
                 menu_snapshot.find("iinuji.logs.settings.metadata.toggle()") !=
                     std::string::npos &&
                 menu_snapshot.find("iinuji.logs.settings.thread.toggle()") !=
                     std::string::npos &&
                 menu_snapshot.find("iinuji.logs.settings.color.toggle()") !=
                     std::string::npos &&
                 menu_snapshot.find(
                     "iinuji.logs.settings.mouse.capture.toggle()") !=
                     std::string::npos,
             "menu snapshot should advertise direct canonical Shell Logs "
             "toggle paths") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.logs.clear()") != std::string::npos,
                 "menu snapshot should advertise the canonical Shell Logs "
                 "clear path") &&
         ok;
    ok = require(menu_snapshot.find("iinuji.quit() / iinuji.exit()") !=
                     std::string::npos,
                 "menu snapshot should advertise canonical quit paths") &&
         ok;
    ok = require(menu_snapshot.find("Help\n\nScreens") == std::string::npos,
                 "menu snapshot should not fall back to the plain help text") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.runtime.focus = RuntimeFocus::Jobs;
  {
    const auto runtime_devices_row =
        menu_command_row_index(state, "iinuji.runtime.devices()");
    ok = require(runtime_devices_row.has_value(),
                 "menu should expose the Runtime devices canonical row") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + runtime_devices_row.value_or(0)),
             "clicking the Runtime devices canonical row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.focus == RuntimeFocus::Devices &&
                   !state.help_view && state.status == "runtime lane=Devices",
               "canonical Runtime devices menu row should focus Devices and "
               "close menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const auto config_reload_row =
        menu_command_row_index(state, "iinuji.config.reload()");
    ok = require(config_reload_row.has_value(),
                 "menu should expose the Config reload canonical row") &&
         ok;
    ok =
        require(dispatch_menu_overlay_click(
                    actions, state, ui, ui.menu_commands->screen.x + 1,
                    ui.menu_commands->screen.y + config_reload_row.value_or(0)),
                "clicking the Config reload canonical row should dispatch") &&
        ok;
  }
  ok = require(state.screen == ScreenMode::Config && !state.help_view &&
                   state.status == "config reload queued",
               "canonical Config reload menu row should execute reload and "
               "close menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const auto actions_namespace_row =
        menu_command_row_index(state, "iinuji.actions.*()");
    ok = require(actions_namespace_row.has_value(),
                 "menu should expose the Actions namespace row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         actions_namespace_row.value_or(0)),
                 "clicking the Actions namespace row should dispatch") &&
         ok;
  }
  ok = require(state.action_menu_open && !state.help_view &&
                   state.status == "action menu",
               "Actions namespace row should open the action palette") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  {
    const auto workspace_namespace_row =
        menu_command_row_index(state, "iinuji.workspace.*()");
    ok = require(workspace_namespace_row.has_value(),
                 "menu should expose the Workspace namespace row") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         workspace_namespace_row.value_or(0)),
                 "clicking the Workspace namespace row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime && !state.help_view &&
                   workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace full",
               "Workspace namespace row should toggle full/split utility") &&
       ok;
  state.workspace.zoom_flags = 0u;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  {
    const auto workspace_zoom_row =
        menu_command_row_index(state, "iinuji.workspace.zoom.toggle()");
    ok = require(workspace_zoom_row.has_value(),
                 "menu should expose the explicit workspace zoom path") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + workspace_zoom_row.value_or(0)),
             "clicking the explicit workspace zoom row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime && !state.help_view &&
                   workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace full",
               "explicit workspace zoom row should toggle full/split "
               "utility") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.workspace.zoom_flags = 0u;
  (void)workspace_toggle_current_screen_zoom(state);
  {
    const auto workspace_split_row =
        menu_command_row_index(state, "iinuji.workspace.split()");
    ok = require(workspace_split_row.has_value(),
                 "menu should expose the explicit workspace split path") &&
         ok;
    ok = require(
             dispatch_menu_overlay_click(
                 actions, state, ui, ui.menu_commands->screen.x + 1,
                 ui.menu_commands->screen.y + workspace_split_row.value_or(0)),
             "clicking the explicit workspace split row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime && !state.help_view &&
                   !workspace_is_current_screen_zoomed(state) &&
                   state.status == "workspace split",
               "explicit workspace split row should restore split layout") &&
       ok;
  state.workspace.zoom_flags = 0u;

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Devices;
  {
    const auto runtime_row_namespace =
        menu_command_row_index(state, "iinuji.runtime.row.*()");
    ok = require(runtime_row_namespace.has_value(),
                 "menu should expose the Runtime row namespace") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         runtime_row_namespace.value_or(0)),
                 "clicking the Runtime row namespace should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.focus == RuntimeFocus::Jobs &&
                   !state.help_view && state.status == "runtime lane=Jobs",
               "Runtime row namespace should select the next lane") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Devices;
  state.runtime.device.gpus = {
      RuntimeGpuSnapshot{.index = "0", .name = "gpu.alpha"},
  };
  state.runtime.selected_device_row = 0u;
  {
    const auto runtime_item_namespace =
        menu_command_row_index(state, "iinuji.runtime.item.*()");
    ok = require(runtime_item_namespace.has_value(),
                 "menu should expose the Runtime item namespace") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         runtime_item_namespace.value_or(0)),
                 "clicking the Runtime item namespace should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Runtime &&
                   state.runtime.selected_device_row == 1u &&
                   !state.help_view && state.status == "runtime device=next",
               "Runtime item namespace should select the next visible row") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.logs.selected_setting = 0u;
  {
    const auto logs_settings_namespace =
        menu_command_row_index(state, "iinuji.logs.settings.*()");
    ok = require(logs_settings_namespace.has_value(),
                 "menu should expose the Shell Logs settings namespace") &&
         ok;
    ok = require(dispatch_menu_overlay_click(
                     actions, state, ui, ui.menu_commands->screen.x + 1,
                     ui.menu_commands->screen.y +
                         logs_settings_namespace.value_or(0)),
                 "clicking the Shell Logs settings namespace should "
                 "dispatch") &&
         ok;
  }
  ok = require(
           state.screen == ScreenMode::Logs &&
               state.logs.selected_setting == 1u && !state.help_view &&
               state.status == "logs.settings.cursor=next",
           "Shell Logs settings namespace should move the settings cursor") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.logs.severity_filter = LogsSeverityFilter::DebugOrHigher;
  {
    const auto logs_level_namespace =
        menu_command_row_index(state, "iinuji.logs.settings.level.*()");
    ok = require(logs_level_namespace.has_value(),
                 "menu should expose the Shell Logs severity namespace") &&
         ok;
    ok = require(click_scrolled_menu_row(logs_level_namespace.value_or(0)),
                 "clicking the Shell Logs severity namespace should "
                 "dispatch") &&
         ok;
  }
  ok = require(
           state.screen == ScreenMode::Logs &&
               state.logs.severity_filter == LogsSeverityFilter::InfoOrHigher &&
               !state.help_view && state.status == "logs.level=INFO+",
           "Shell Logs severity namespace should cycle the severity lens") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.logs.auto_follow = false;
  {
    const auto logs_follow_toggle =
        menu_command_row_index(state, "iinuji.logs.settings.follow.toggle()");
    ok = require(logs_follow_toggle.has_value(),
                 "menu should expose the Shell Logs follow toggle path") &&
         ok;
    ok =
        require(click_scrolled_menu_row(logs_follow_toggle.value_or(0)),
                "clicking the Shell Logs follow toggle path should dispatch") &&
        ok;
  }
  ok = require(state.screen == ScreenMode::Logs && state.logs.auto_follow &&
                   !state.help_view && state.status == "logs.follow=on",
               "Shell Logs follow toggle menu row should toggle follow mode") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Lattice;
  state.lattice.target_ids = {"target.alpha", "target.beta"};
  state.lattice.selected_target = 0u;
  {
    const auto lattice_namespace =
        menu_command_row_index(state, "iinuji.lattice.target.*()");
    ok = require(lattice_namespace.has_value(),
                 "menu should expose the Lattice target namespace row") &&
         ok;
    ok = require(click_scrolled_menu_row(lattice_namespace.value_or(0)),
                 "clicking the Lattice target namespace row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.lattice.selected_target == 1u && !state.help_view,
               "Lattice target namespace row should browse to the next "
               "target") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Config;
  state.config.files = {
      ConfigFileSummary{.path = "/tmp/alpha.dsl", .relative_path = "alpha.dsl"},
      ConfigFileSummary{.path = "/tmp/beta.dsl", .relative_path = "beta.dsl"},
  };
  state.config.selected_file = 0u;
  {
    const auto config_file_namespace =
        menu_command_row_index(state, "iinuji.config.file.*()");
    ok = require(config_file_namespace.has_value(),
                 "menu should expose the Config file namespace row") &&
         ok;
    ok = require(click_scrolled_menu_row(config_file_namespace.value_or(0)),
                 "clicking the Config file namespace row should dispatch") &&
         ok;
  }
  ok = require(state.screen == ScreenMode::Config &&
                   state.config.selected_file == 1u && !state.help_view &&
                   state.status == "selected file=2",
               "Config file namespace row should browse to the next file") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const auto help_close_row =
        menu_command_row_index(state, "iinuji.help.close()");
    ok = require(help_close_row.has_value(),
                 "menu should expose the explicit help close canonical row") &&
         ok;
    ok = require(click_scrolled_menu_row(help_close_row.value_or(0)),
                 "clicking the help close canonical row should dispatch") &&
         ok;
  }
  ok = require(!state.help_view && state.status == "help overlay=closed",
               "canonical help close menu row should close the menu") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  state.quit = false;
  {
    const std::string command = "iinuji.quit() / iinuji.exit()";
    const auto exit_row = menu_command_row_index(state, command);
    ok = require(exit_row.has_value(),
                 "menu should expose the canonical quit/exit row") &&
         ok;
    const std::size_t exit_offset = command.find("iinuji.exit()");
    ok = require(exit_offset != std::string::npos,
                 "canonical quit/exit row should contain exit label") &&
         ok;
    ok = require(click_scrolled_menu_row_at(exit_row.value_or(0),
                                            static_cast<int>(exit_offset)),
                 "clicking the canonical exit label should dispatch") &&
         ok;
  }
  ok = require(state.quit && state.status == "exit" && !state.help_view,
               "canonical exit label should preserve exit status and close "
               "menu") &&
       ok;
  state.quit = false;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Home;
  {
    const std::string command = "quit | exit | q | x";
    const auto exit_alias_row = menu_command_row_index(state, command);
    ok = require(exit_alias_row.has_value(),
                 "menu should expose the quit/exit/x alias row") &&
         ok;
    const std::size_t x_offset = command.rfind("x");
    ok = require(x_offset != std::string::npos,
                 "quit/exit alias row should contain x label") &&
         ok;
    ok = require(click_scrolled_menu_row_at(exit_alias_row.value_or(0),
                                            static_cast<int>(x_offset)),
                 "clicking the x alias label should dispatch") &&
         ok;
  }
  ok = require(state.quit && state.status == "exit" && !state.help_view,
               "x alias menu label should dispatch the legacy exit alias") &&
       ok;
  state.quit = false;
  {
    const std::string help_text = make_help_text(state);
    ok = require(help_text.find("iinuji.workspace.zoom.toggle()") !=
                     std::string::npos,
                 "help text should advertise workspace zoom path") &&
         ok;
    ok = require(help_text.find("iinuji.show.runtime()") != std::string::npos,
                 "help text should advertise restored show paths") &&
         ok;
    ok = require(
             help_text.find("iinuji.screen.marshal()") == std::string::npos &&
                 help_text.find("iinuji.screen.inbox()") == std::string::npos &&
                 help_text.find("iinuji.screen.human()") == std::string::npos,
             "help text should keep removed canonical Workbench screen paths "
             "hidden") &&
         ok;
    ok = require(help_text.find("iinuji.show.marshal()") == std::string::npos &&
                     help_text.find("iinuji.show.inbox()") ==
                         std::string::npos &&
                     help_text.find("iinuji.show.human()") == std::string::npos,
                 "help text should keep removed canonical Workbench show paths "
                 "hidden") &&
         ok;
    ok = require(help_text.find("iinuji.workbench.refresh()") !=
                     std::string::npos,
                 "help text should advertise Workbench refresh path") &&
         ok;
    ok =
        require(help_text.find("iinuji.runtime.refresh()") != std::string::npos,
                "help text should advertise Runtime refresh path") &&
        ok;
    ok = require(help_text.find("iinuji.runtime.row.prev()") !=
                         std::string::npos &&
                     help_text.find("iinuji.runtime.row.next()") !=
                         std::string::npos &&
                     help_text.find("iinuji.runtime.item.prev()") !=
                         std::string::npos &&
                     help_text.find("iinuji.runtime.item.next()") !=
                         std::string::npos,
                 "help text should advertise two-way Runtime row and item "
                 "utilities") &&
         ok;
    ok = require(help_text.find("iinuji.logs.settings.source.show()") !=
                     std::string::npos,
                 "help text should advertise show source lens") &&
         ok;
    ok = require(help_text.find("iinuji.logs.settings.source.next()") !=
                     std::string::npos,
                 "help text should advertise source lens cycling path") &&
         ok;
    ok = require(help_text.find("iinuji.logs.settings.select.prev()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.select.next()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.change.prev()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.change.next()") !=
                         std::string::npos,
                 "help text should advertise Shell Logs setting cursor "
                 "and change utilities") &&
         ok;
    ok =
        require(
            help_text.find("actions page up/down") != std::string::npos &&
                help_text.find("actions home/end") != std::string::npos &&
                help_text.find("iinuji.help()") != std::string::npos &&
                help_text.find("iinuji.help.close()") != std::string::npos &&
                help_text.find("iinuji.help.scroll.*()") != std::string::npos &&
                help_text.find("iinuji.actions()") != std::string::npos &&
                help_text.find("iinuji.commands()") != std::string::npos &&
                help_text.find("commands namespace for action palette") !=
                    std::string::npos &&
                help_text.find("alternate action palette namespace") ==
                    std::string::npos &&
                help_text.find("iinuji.actions.run()") != std::string::npos &&
                help_text.find("iinuji.commands.run.selected()") !=
                    std::string::npos &&
                help_text.find("iinuji.actions.close()") != std::string::npos &&
                help_text.find("iinuji.commands.close()") !=
                    std::string::npos &&
                help_text.find("alternate close command") ==
                    std::string::npos &&
                help_text.find("iinuji.actions.select.page.down()") !=
                    std::string::npos &&
                help_text.find("iinuji.actions.select.end()") !=
                    std::string::npos &&
                help_text.find("iinuji.commands.select.*()") !=
                    std::string::npos &&
                help_text.find("commands namespace movement utilities") !=
                    std::string::npos &&
                help_text.find("alternate palette movement namespace") ==
                    std::string::npos,
            "help text should advertise direct help and action palette "
            "lifecycle utilities") &&
        ok;
    ok =
        require(
            help_text.find("iinuji.logs.scroll.up()") != std::string::npos &&
                help_text.find("iinuji.logs.scroll.down()") !=
                    std::string::npos &&
                help_text.find("iinuji.logs.scroll.home()") !=
                    std::string::npos &&
                help_text.find("iinuji.logs.scroll.end()") != std::string::npos,
            "help text should advertise direct Shell Logs scroll "
            "utilities") &&
        ok;
    ok = require(help_text.find("iinuji.logs.settings.source.any()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.all()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.refresh()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.action()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.command()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.show()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.source.status()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.level.debug()") !=
                         std::string::npos &&
                     help_text.find("iinuji.logs.settings.level.fatal()") !=
                         std::string::npos,
                 "help text should advertise direct Shell Logs source and "
                 "severity utilities") &&
         ok;
    ok =
        require(
            help_text.find("iinuji.logs.settings.metadata.filter.any()") !=
                    std::string::npos &&
                help_text.find("iinuji.logs.settings.metadata.filter.next()") !=
                    std::string::npos &&
                help_text.find(
                    "iinuji.logs.settings.metadata.filter.any_meta()") !=
                    std::string::npos &&
                help_text.find(
                    "iinuji.logs.settings.metadata.filter.function()") !=
                    std::string::npos &&
                help_text.find("iinuji.logs.settings.metadata.filter.path()") !=
                    std::string::npos &&
                help_text.find(
                    "iinuji.logs.settings.metadata.filter.callsite()") !=
                    std::string::npos,
            "help text should advertise direct Shell Logs metadata "
            "filters") &&
        ok;
    ok = require(help_text.find("iinuji.logs.clear()") != std::string::npos,
                 "help text should advertise the canonical Shell Logs clear "
                 "path") &&
         ok;
    ok = require(help_text.find("home / f1 / 1") != std::string::npos &&
                     help_text.find("workbench / work / w / f2 / 2") !=
                         std::string::npos &&
                     help_text.find("runtime / run / rt / f3 / 3") !=
                         std::string::npos &&
                     help_text.find("lattice / lat / proof / f4 / 4") !=
                         std::string::npos &&
                     help_text.find("logs / log / shell / events / f8 / 8") !=
                         std::string::npos &&
                     help_text.find("config / cfg / f9 / 9") !=
                         std::string::npos,
                 "help text should advertise restored F-key screen aliases as "
                 "typeable commands") &&
         ok;
    ok =
        require(
            help_text.find("Shell utilities") != std::string::npos &&
                help_text.find("cuwacunu.cmd --menu") != std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --menu   render menu "
                               "overlay snapshot") != std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --menu") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --menu   render menu "
                               "overlay snapshot") != std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --actions") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --catalog") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --visual") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --splash loading") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --splash loading") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --splash close") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --splash close") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --splash good-luck") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --splash good-luck") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --screen runtime") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --screen runtime") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --snapshot --full") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --snapshot --full") !=
                    std::string::npos &&
                help_text.find("iinuji_cmd --menu") != std::string::npos &&
                help_text.find("cuwacunu.cmd --actions") != std::string::npos &&
                help_text.find("cuwacunu.cmd --visual") != std::string::npos &&
                help_text.find("--home-visual") != std::string::npos &&
                help_text.find("iinuji_cmd --visual") != std::string::npos &&
                help_text.find("cuwacunu.cmd --bootstrap / --boot") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --splash loading") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --farewell / --closing") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --splash farewell") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --splash close") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --splash=good-luck") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --splash good luck") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd --splash good luck") !=
                    std::string::npos &&
                help_text.find("iinuji_cmd --splash=farewell") !=
                    std::string::npos &&
                help_text.find("cuwacunu.cmd runtime --snapshot") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd src/config/hero.runtime.dsl "
                               "--snapshot") != std::string::npos &&
                help_text.find("cuwacunu.cmd src/config/hero.runtime.dsl "
                               "--snapshot") != std::string::npos &&
                help_text.find("cuwacunu.cmd --run \"show\" --snapshot") !=
                    std::string::npos &&
                help_text.find("cuwacunu_cmd --run \"show\" --snapshot") !=
                    std::string::npos,
            "help text should advertise primary shell utilities and alternate "
            "command names") &&
        ok;
    ok = require(help_text.find("compatibility") == std::string::npos,
                 "help text should not expose migration-era compatibility "
                 "copy") &&
         ok;
    ok =
        require(help_text.find("iinuji.lattice.refresh()") != std::string::npos,
                "help text should advertise Lattice refresh path") &&
        ok;
    ok = require(help_text.find("iinuji.config.reload()") != std::string::npos,
                 "help text should advertise Config reload path") &&
         ok;
    ok = require(help_text.find("iinuji.config.files()") != std::string::npos,
                 "help text should advertise Config files path") &&
         ok;
    ok = require(help_text.find("iinuji.config.show()") != std::string::npos,
                 "help text should advertise Config show path") &&
         ok;
    ok = require(help_text.find("iinuji.config.file.show()") !=
                     std::string::npos,
                 "help text should advertise Config file show path") &&
         ok;
    ok = require(help_text.find("iinuji.config.file.first()") !=
                     std::string::npos,
                 "help text should advertise config first path") &&
         ok;
    ok = require(help_text.find("iinuji.config.file.index(n=1)") !=
                     std::string::npos,
                 "help text should advertise config argument selector") &&
         ok;
    ok = require(help_text.find("config file 1 / file default") !=
                     std::string::npos,
                 "help text should advertise natural config selectors") &&
         ok;
    ok = require(help_text.find("iinuji.lattice.target.last()") !=
                     std::string::npos,
                 "help text should advertise lattice last path") &&
         ok;
    ok = require(help_text.find("iinuji.lattice.target.index(n=1)") !=
                     std::string::npos,
                 "help text should advertise lattice argument selector") &&
         ok;
    ok = require(help_text.find("lattice target 1 / target default") !=
                     std::string::npos,
                 "help text should advertise natural lattice selectors") &&
         ok;
    ok = require(help_text.find("iinuji.quit()") != std::string::npos,
                 "help text should advertise canonical quit path") &&
         ok;
    ok = require(help_text.find("iinuji.exit()") != std::string::npos,
                 "help text should advertise canonical exit path") &&
         ok;
    ok = require(help_text.find("quit / q / exit / x") != std::string::npos,
                 "help text should advertise the legacy x exit alias") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Devices;
  {
    const auto row = menu_current_screen_row_index(state, "Left / Right");
    ok = require(row.has_value(), "menu should expose Runtime lane row") && ok;
    ok = require(click_scrolled_menu_row_at(row.value_or(0), 7),
                 "clicking Runtime current lane row should dispatch") &&
         ok;
    ok = require(state.runtime.focus == RuntimeFocus::Jobs && !state.help_view,
                 "Runtime current lane row should focus jobs and close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Runtime;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  {
    const auto row = menu_current_screen_row_index(state, "Tab / Shift-Tab");
    ok = require(row.has_value(),
                 "Runtime menu should expose Tab and Shift-Tab row") &&
         ok;
    ok =
        require(click_scrolled_menu_row(row.value_or(0)),
                "clicking Runtime current Tab/Shift-Tab row should dispatch") &&
        ok;
    ok = require(state.runtime.detail_mode == RuntimeDetailMode::Log &&
                     !state.help_view,
                 "Runtime current Tab/Shift-Tab row should cycle detail and "
                 "close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Runtime;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  {
    const auto row =
        menu_current_screen_row_index(state, "manifest / log / trace");
    ok = require(row.has_value(),
                 "Runtime menu should expose direct detail lens row") &&
         ok;
    const std::string command = "manifest / log / trace";
    const std::size_t trace_offset = command.find("trace");
    ok = require(trace_offset != std::string::npos,
                 "Runtime direct detail row should contain trace") &&
         ok;
    ok = require(click_scrolled_menu_row_at(row.value_or(0),
                                            static_cast<int>(trace_offset)),
                 "clicking Runtime current trace detail row should dispatch") &&
         ok;
    ok = require(state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                     state.status == "runtime detail=trace" && !state.help_view,
                 "Runtime current trace detail row should choose trace and "
                 "close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  state.runtime.focus = RuntimeFocus::Jobs;
  {
    const auto row =
        menu_current_screen_row_index(state, "Enter menu | a actions");
    ok = require(row.has_value(),
                 "Runtime menu should expose Enter menu/action row") &&
         ok;
    ok = require(click_scrolled_menu_row(row.value_or(0)),
                 "clicking Runtime Enter menu row should dispatch") &&
         ok;
    ok = require(cmd_choice_menu_open(state) && !state.action_menu_open &&
                     !state.help_view &&
                     state.choice_menu.kind ==
                         CmdChoiceMenuKind::RuntimeSelection &&
                     !workspace_is_current_screen_zoomed(state),
                 "Runtime Enter row should open selected item popup menu "
                 "without opening the action palette or full-screen detail") &&
         ok;
    state.workspace.zoom_flags = 0u;
    close_choice_menu(state);
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.help_scroll_y = 0;
  state.screen = ScreenMode::Runtime;
  {
    const auto row = menu_current_screen_row_index(state, "r");
    ok = require(row.has_value(),
                 "Runtime menu should expose screen-specific refresh row") &&
         ok;
    ok = require(click_scrolled_menu_row(row.value_or(0)),
                 "clicking Runtime current refresh row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Runtime &&
                     state.status == "runtime screen refresh queued" &&
                     !state.help_view,
                 "Runtime current refresh row should run the Runtime refresh "
                 "utility and close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  {
    const auto first = menu_current_screen_first_row(state);
    ok = require(first.has_value(),
                 "Logs menu should expose current-screen rows") &&
         ok;
    ok = require(click_scrolled_menu_row_at(*first + 2, 4),
                 "clicking Logs current v row should dispatch") &&
         ok;
    ok = require(state.logs.severity_filter ==
                         LogsSeverityFilter::WarningOrHigher &&
                     !state.help_view,
                 "Logs current v row should cycle severity and close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.log = {LogEntry{0, "command", "seed"}};
  {
    const auto first = menu_current_screen_first_row(state);
    ok = require(first.has_value(),
                 "Logs current menu rows should remain discoverable") &&
         ok;
    ok = require(click_scrolled_menu_row(*first + 5),
                 "clicking Logs current clear row should dispatch") &&
         ok;
    ok = require(state.log.size() == 1u &&
                     state.log.back().source == "action" && !state.help_view,
                 "Logs current clear row should clear feed and close menu") &&
         ok;
  }

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Logs;
  state.logs.selected_setting = 1u;
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  {
    const auto row =
        menu_current_screen_row_index(state, "Left / Right settings");
    ok = require(row.has_value(),
                 "Logs current menu should expose selected-setting change "
                 "row") &&
         ok;
    const std::string command = "Left / Right settings";
    const std::size_t right_offset = command.find("Right");
    ok = require(right_offset != std::string::npos,
                 "Logs current setting row should contain Right label") &&
         ok;
    ok = require(click_scrolled_menu_row_at(row.value_or(0),
                                            static_cast<int>(right_offset)),
                 "clicking Logs current Right settings row should dispatch") &&
         ok;
    ok = require(state.logs.severity_filter ==
                         LogsSeverityFilter::WarningOrHigher &&
                     state.status == "logs.level=WARNING+" && !state.help_view,
                 "Logs current Right settings row should change the selected "
                 "setting and close menu") &&
         ok;
  }

  ui.main = create_text_box("cmd.main", "", true);
  ui.main->visible = true;
  ui.main->screen = Rect{6, 8, 80, 40};
  ui.detail = create_text_box("cmd.detail", "", true);
  ui.detail->visible = true;
  ui.detail->screen = Rect{90, 8, 60, 40};
  ui.split_panel = std::make_shared<iinuji_object_t>();
  ui.split_panel->id = "cmd.split";
  ui.split_panel->visible = false;
  ui.split_panel->screen = Rect{6, 8, 80, 40};
  ui.split_nav_panel = std::make_shared<iinuji_object_t>();
  ui.split_nav_panel->id = "cmd.split.nav.panel";
  ui.split_nav_panel->visible = true;
  ui.split_nav_panel->screen = Rect{6, 8, 80, 8};
  ui.split_nav = create_text_box("cmd.split.nav", "", true);
  ui.split_nav->visible = true;
  ui.split_nav->screen = Rect{6, 8, 80, 8};
  ui.split_worklist_panel = std::make_shared<iinuji_object_t>();
  ui.split_worklist_panel->id = "cmd.split.worklist.panel";
  ui.split_worklist_panel->visible = true;
  ui.split_worklist_panel->screen = Rect{6, 16, 80, 32};
  ui.split_worklist = create_text_box("cmd.split.worklist", "", true);
  ui.split_worklist->visible = true;
  ui.split_worklist->screen = Rect{6, 16, 80, 32};

  state.screen = ScreenMode::Runtime;
  ui.split_panel->visible = true;
  scroll_text_to(ui.split_nav, 0, 0);
  scroll_text_to(ui.split_worklist, 0, 0);
  ui.last_mouse_x = ui.split_nav_panel->screen.x + 1;
  ui.last_mouse_y = ui.split_nav_panel->screen.y + 1;
  ok = require(scroll_hovered_workspace_panel(state, ui, 3, 0),
               "wheel over split navigator should dispatch to navigator "
               "primitive") &&
       ok;
  ok = require(text_data(ui.split_nav)->scroll_y == 3 &&
                   text_data(ui.split_worklist)->scroll_y == 0 &&
                   state.status == "navigator scroll",
               "split navigator wheel should not move the worklist") &&
       ok;
  ui.last_mouse_x = ui.split_worklist_panel->screen.x + 1;
  ui.last_mouse_y = ui.split_worklist_panel->screen.y + 1;
  ok = require(scroll_hovered_workspace_panel(state, ui, 2, 0),
               "wheel over split worklist should dispatch to worklist "
               "primitive") &&
       ok;
  ok = require(text_data(ui.split_nav)->scroll_y == 3 &&
                   text_data(ui.split_worklist)->scroll_y == 2 &&
                   state.status == "view scroll",
               "split worklist wheel should remain scoped to the worklist") &&
       ok;
  set_styled_lines(ui.split_worklist,
                   style_content_text(
                       "worklist row with a deliberately long runtime path "
                       "that needs horizontal scrolling past the panel width"),
                   false);
  ui.last_mouse_x = ui.split_worklist_panel->screen.x + 1;
  ui.last_mouse_y = ui.split_worklist_panel->screen.y + 1;
  ok = require(scroll_hovered_workspace_panel(state, ui, 2, 0),
               "plain wheel over a non-vertical split worklist should dispatch "
               "as horizontal scroll") &&
       ok;
  ok = require(text_data(ui.split_worklist)->scroll_y == 0 &&
                   text_data(ui.split_worklist)->scroll_x == 8 &&
                   state.status == "view scroll",
               "split worklist should use horizontal scroll when vertical "
               "scrolling is unavailable") &&
       ok;
  ok = require(scroll_hovered_workspace_panel(state, ui, 0, 8),
               "modified horizontal wheel over split worklist should still "
               "dispatch to worklist primitive") &&
       ok;
  ok =
      require(text_data(ui.split_worklist)->scroll_x == 16 &&
                  state.status == "view scroll",
              "split worklist should also accept explicit horizontal deltas") &&
      ok;
  scroll_text_to(ui.split_nav, 0, 0);
  scroll_text_to(ui.split_worklist, 0, 0);
  ui.split_panel->visible = false;

  const auto require_current_detail_page_down = [&](ScreenMode screen,
                                                    std::string_view label) {
    state.help_view = true;
    state.action_menu_open = false;
    state.help_scroll_y = 0;
    state.screen = screen;
    scroll_text_to(ui.detail, 0, 0);

    const auto row = menu_current_screen_row_index(state, "PgUp / PgDn");
    ok = require(row.has_value(), std::string(label) +
                                      " menu should expose detail page "
                                      "scroll row") &&
         ok;
    const std::string command = "PgUp / PgDn";
    const std::size_t pgdn_offset = command.find("PgDn");
    ok = require(pgdn_offset != std::string::npos,
                 "detail page scroll row should contain PgDn") &&
         ok;
    ok = require(click_scrolled_menu_row_at(row.value_or(0),
                                            static_cast<int>(pgdn_offset)),
                 std::string("clicking ") + std::string(label) +
                     " current PgDn row should dispatch") &&
         ok;
    ok = require(text_data(ui.detail) && text_data(ui.detail)->scroll_y == 8 &&
                     state.status == "context scroll" && !state.help_view,
                 std::string(label) +
                     " current PgDn row should page the detail panel and "
                     "close menu") &&
         ok;
  };

  require_current_detail_page_down(ScreenMode::Runtime, "Runtime");
  require_current_detail_page_down(ScreenMode::Lattice, "Lattice");
  require_current_detail_page_down(ScreenMode::Config, "Config");
  scroll_text_to(ui.detail, 0, 0);

  state.screen = ScreenMode::Logs;
  state.log = {LogEntry{0, "refresh", "seed"}};
  {
    const std::string logs_text = make_logs_text(state);
    ok = require(logs_text.find("scroll y=") != std::string::npos &&
                     logs_text.find("[up] [down]") == std::string::npos &&
                     logs_text.find("iinuji.logs.scroll.up()/") ==
                         std::string::npos,
                 "Shell Logs should show compact scroll state without command "
                 "rail blocks") &&
         ok;
  }
  state.logs.scroll_y = 7;
  scroll_text_to(ui.main, 0, 0);
  ok = require(!dispatch_content_rail_click(
                   actions, state, ui, ui.main->screen.x, ui.main->screen.y),
               "hidden Shell Logs command rail should not dispatch") &&
       ok;
  scroll_text_to(ui.main, 0, 0);
  state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
  ok =
      require(dispatch_logs_lens_click(
                  actions, state, ui, ui.detail->screen.x + 2,
                  ui.detail->screen.y + logs_lens_first_setting_row(state) + 1),
              "clicking Shell Logs level lens should dispatch") &&
      ok;
  ok = require(state.logs.selected_setting == 1u &&
                   state.logs.severity_filter ==
                       LogsSeverityFilter::WarningOrHigher,
               "logs lens click should select and cycle severity") &&
       ok;
  {
    CmdUi key_ui{};
    state.screen = ScreenMode::Logs;
    state.logs.selected_setting = 0;
    state.logs.source_filter = LogsSourceFilter::All;
    handle_key(key_right, actions, state, key_ui);
    ok = require(state.logs.source_filter == LogsSourceFilter::Refresh &&
                     state.status == "logs.source=refresh",
                 "Shell Logs Right should advance the selected source lens") &&
         ok;
    handle_key(key_left, actions, state, key_ui);
    ok = require(state.logs.source_filter == LogsSourceFilter::All &&
                     state.status == "logs.source=any",
                 "Shell Logs Left should reverse the selected source lens") &&
         ok;

    state.logs.selected_setting = 1;
    state.logs.severity_filter = LogsSeverityFilter::InfoOrHigher;
    handle_key(key_right, actions, state, key_ui);
    ok =
        require(state.logs.severity_filter ==
                        LogsSeverityFilter::WarningOrHigher &&
                    state.status == "logs.level=WARNING+",
                "Shell Logs Right should advance the selected severity lens") &&
        ok;
    handle_key(key_left, actions, state, key_ui);
    ok = require(state.logs.severity_filter ==
                         LogsSeverityFilter::InfoOrHigher &&
                     state.status == "logs.level=INFO+",
                 "Shell Logs Left should reverse the selected severity lens") &&
         ok;

    state.logs.selected_setting = 2;
    state.logs.metadata_filter = LogsMetadataFilter::Any;
    handle_key(key_right, actions, state, key_ui);
    ok =
        require(state.logs.metadata_filter ==
                        LogsMetadataFilter::WithAnyMetadata &&
                    state.status == "logs.metadata_filter=META+",
                "Shell Logs Right should advance the selected metadata lens") &&
        ok;
    handle_key(key_left, actions, state, key_ui);
    ok = require(state.logs.metadata_filter == LogsMetadataFilter::Any &&
                     state.status == "logs.metadata_filter=ANY",
                 "Shell Logs Left should reverse the selected metadata lens") &&
         ok;
  }

  state.screen = ScreenMode::Runtime;
  state.runtime.device.mem_total_kib = 2048;
  state.runtime.device.mem_available_kib = 1024;
  state.runtime.device.gpus = {
      RuntimeGpuSnapshot{.index = "0", .name = "gpu.alpha"},
      RuntimeGpuSnapshot{.index = "1", .name = "gpu.beta"},
  };
  state.runtime.device.gpu_processes.clear();
  state.runtime.jobs = {
      RuntimeJobSummary{.job_id = "job.alpha",
                        .status = "ready",
                        .job_kind = "channel_inference_mdn",
                        .wave_action = "train"},
      RuntimeJobSummary{.job_id = "job.beta",
                        .status = "running",
                        .job_kind = "channel_representation_vicreg",
                        .wave_action = "eval"},
  };
  state.runtime.focus = RuntimeFocus::Devices;
  state.runtime.selected_device_row = 0;
  state.runtime.selected_job = 0;
  {
    CmdUi key_ui{};
    workspace_toggle_current_screen_zoom(state);
    handle_key(key_escape, actions, state, key_ui);
    ok = require(!workspace_is_current_screen_zoomed(state) &&
                     state.status == "workspace split",
                 "Esc should restore split when Runtime is full screen") &&
         ok;
    state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    handle_key(key_shift_tab, actions, state, key_ui);
    ok = require(state.runtime.detail_mode == RuntimeDetailMode::Log &&
                     state.status == "runtime detail=log",
                 "Runtime Shift-Tab should preserve the legacy detail-cycle "
                 "shortcut") &&
         ok;
    state.runtime.focus = RuntimeFocus::Devices;
    handle_key(key_left, actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                     state.status == "runtime lane=Jobs",
                 "Runtime Left should wrap from Devices to Jobs") &&
         ok;
    handle_key(key_right, actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                     state.status == "runtime lane=Devices",
                 "Runtime Right should wrap from Jobs to Devices") &&
         ok;
    handle_key('j', actions, state, key_ui);
    ok = require(state.runtime.selected_device_row == 1u &&
                     state.status == "runtime device=next",
                 "Runtime j key should restore legacy next-row movement") &&
         ok;
    handle_key('k', actions, state, key_ui);
    ok = require(state.runtime.selected_device_row == 0u &&
                     state.status == "runtime device=prev",
                 "Runtime k key should restore legacy previous-row movement") &&
         ok;
    handle_key('l', actions, state, key_ui);
    ok = require(state.runtime.selected_device_row == 1u &&
                     state.status == "runtime device=next",
                 "Runtime l key should browse to the next row") &&
         ok;
    handle_key('h', actions, state, key_ui);
    ok = require(state.runtime.selected_device_row == 0u &&
                     state.status == "runtime device=prev",
                 "Runtime h key should browse to the previous row") &&
         ok;
    state.runtime.detail_mode = RuntimeDetailMode::Trace;
    handle_key(key_enter, actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                     state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                     cmd_choice_menu_open(state) &&
                     !workspace_is_current_screen_zoomed(state) &&
                     state.status == "runtime host menu",
                 "Runtime Devices Enter should open a selected device popup "
                 "menu without forcing full screen") &&
         ok;
    handle_key(key_escape, actions, state, key_ui);
    ok = require(!cmd_choice_menu_open(state) &&
                     !workspace_is_current_screen_zoomed(state) &&
                     state.status == "Runtime",
                 "Runtime Esc should close the selected item popup menu") &&
         ok;
    handle_key(key_enter, actions, state, key_ui);
    handle_key(key_enter, actions, state, key_ui);
    ok = require(cmd_choice_menu_open(state) && cmd_content_popup_open(state) &&
                     state.content_popup.kind ==
                         CmdContentPopupKind::RuntimePolicy &&
                     state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                     !workspace_is_current_screen_zoomed(state) &&
                     state.status == "runtime popup",
                 "Runtime Devices popup default action should open runtime "
                 "policy without rebasing the telemetry panel or losing the "
                 "menu") &&
         ok;
    handle_key(key_escape, actions, state, key_ui);
    ok =
        require(!cmd_content_popup_open(state) && cmd_choice_menu_open(state) &&
                    state.status == "runtime menu",
                "Runtime Esc should close the content popup and return to the "
                "menu") &&
        ok;
    handle_key(key_escape, actions, state, key_ui);
    ok = require(!cmd_choice_menu_open(state) && state.status == "Runtime",
                 "Runtime second Esc should close the selected item menu") &&
         ok;
    handle_key('H', actions, state, key_ui);
    ok = require(state.help_view && state.status.find("help overlay=open") !=
                                        std::string::npos,
                 "Runtime uppercase H should open help because lowercase h "
                 "browses rows") &&
         ok;
    handle_key('x', actions, state, key_ui);
    ok = require(!state.help_view && state.status == "Runtime",
                 "x key should close the advertised help overlay affordance "
                 "without exiting") &&
         ok;
    handle_key('J', actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                     state.status == "runtime lane=Jobs",
                 "Runtime uppercase J should keep a direct Jobs focus key") &&
         ok;
    state.runtime.detail_mode = RuntimeDetailMode::Trace;
    handle_key(key_enter, actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                     !state.action_menu_open && cmd_choice_menu_open(state) &&
                     state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                     !workspace_is_current_screen_zoomed(state) &&
                     state.status == "runtime job menu",
                 "Runtime Jobs Enter should open selected job file/action menu "
                 "without opening the action palette") &&
         ok;
    handle_key(key_down, actions, state, key_ui);
    handle_key(key_enter, actions, state, key_ui);
    ok = require(cmd_choice_menu_open(state) && cmd_content_popup_open(state) &&
                     state.content_popup.kind ==
                         CmdContentPopupKind::RuntimeJobState &&
                     state.runtime.detail_mode == RuntimeDetailMode::Trace &&
                     !workspace_is_current_screen_zoomed(state) &&
                     state.status == "runtime popup",
                 "Runtime Jobs popup should render job file choices in a "
                 "content popup while preserving the menu") &&
         ok;
    handle_key(key_escape, actions, state, key_ui);
    ok =
        require(!cmd_content_popup_open(state) && cmd_choice_menu_open(state) &&
                    state.status == "runtime menu",
                "Runtime Esc should close job content popup and return to the "
                "menu") &&
        ok;
    handle_key(key_escape, actions, state, key_ui);
    ok = require(!cmd_choice_menu_open(state) && state.status == "Runtime",
                 "Runtime second Esc should close the job menu") &&
         ok;
    handle_key('a', actions, state, key_ui);
    ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                     state.action_menu_open && state.status == "action menu",
                 "Runtime Jobs a should keep explicit action palette access") &&
         ok;
    handle_key('x', actions, state, key_ui);
    ok = require(!state.action_menu_open && state.status == "Runtime",
                 "x key should close the advertised action palette affordance "
                 "without exiting") &&
         ok;
    state.quit = false;
    handle_key('x', actions, state, key_ui);
    ok = require(state.quit && state.status == "exit",
                 "x key should dispatch the advertised legacy exit alias when "
                 "no overlay is open") &&
         ok;
    state.quit = false;
    state.status = "Runtime";
    state.runtime.focus = RuntimeFocus::Devices;
  }
  {
    const std::string runtime_text = make_runtime_text(state);
    ok = require(runtime_text.find("Runtime\n\nFocus:") != std::string::npos &&
                     runtime_text.find("Navigator\n") == std::string::npos,
                 "Runtime should expose compact focus controls without a "
                 "navigator heading") &&
         ok;
    ok = require(runtime_text.find("\nWorklist\n") != std::string::npos,
                 "Runtime should expose a worklist section") &&
         ok;
    ok = require(make_runtime_worklist_text(state).find("\nMenu\n") ==
                         std::string::npos &&
                     make_runtime_nav_text(state).find("\nMenu\n") !=
                         std::string::npos,
                 "Runtime compact menu should live in the navigator panel "
                 "above the worklist") &&
         ok;
    ok = require(runtime_text.find("[ready") != std::string::npos &&
                     runtime_text.find("[train") != std::string::npos &&
                     runtime_text.find("[channel_inference_mdn") !=
                         std::string::npos &&
                     runtime_text.find("[running") != std::string::npos &&
                     runtime_text.find("[eval") != std::string::npos &&
                     runtime_text.find("[channel_representation_vicreg]") !=
                         std::string::npos,
                 "Runtime job rows should restore fixed scan badges") &&
         ok;
    ok =
        require(runtime_text.find("\n      job.alpha\n") == std::string::npos &&
                    runtime_text.find("[channel_inference_mdn") <
                        runtime_text.find("job.alpha"),
                "Runtime job worklist rows should include the job id on the "
                "same line") &&
        ok;
    ok = require(runtime_text.find("\n> Devices  host + 2 gpu(s)\n") !=
                         std::string::npos &&
                     runtime_text.find("\n  >> host ") != std::string::npos,
                 "Runtime worklist should use a single section marker and a "
                 "double item marker") &&
         ok;

    CmdState job_marker_state = state;
    job_marker_state.runtime.focus = RuntimeFocus::Jobs;
    job_marker_state.runtime.selected_job = 1u;
    const std::string job_worklist =
        make_runtime_worklist_text(job_marker_state);
    ok = require(job_worklist.find("\n> Jobs\n") != std::string::npos &&
                     job_worklist.find("\n  >> [running") != std::string::npos,
                 "Runtime job lane should apply the same section/item marker "
                 "hierarchy") &&
         ok;
  }
  {
    const auto styled_worklist =
        style_content_text(make_runtime_worklist_text(state));
    bool host_line_segmented = false;
    bool host_marker_warning = false;
    bool host_body_warning = false;
    bool host_background_empty = false;
    for (const auto &line : styled_worklist) {
      if (line.text.find(">> host ") == std::string::npos)
        continue;
      host_line_segmented = line.segments.size() >= 2u;
      host_background_empty = line.background_color.empty();
      for (const auto &segment : line.segments) {
        if (segment.text == ">>" &&
            segment.emphasis == text_line_emphasis_t::Warning)
          host_marker_warning = true;
        if (segment.text.find(" host ") != std::string::npos &&
            segment.emphasis == text_line_emphasis_t::Warning)
          host_body_warning = true;
      }
    }
    ok = require(host_line_segmented && host_marker_warning &&
                     host_background_empty && !host_body_warning,
                 "Runtime selected-row styling should keep selection color on "
                 "the marker only") &&
         ok;
  }
  {
    CmdUi runtime_worklist_ui = create_ui(gui_config_path);
    CmdState runtime_worklist_state = state;
    runtime_worklist_state.screen = ScreenMode::Runtime;
    update_ui(runtime_worklist_ui, runtime_worklist_state, actions);
    ok = require(text_data(runtime_worklist_ui.split_worklist) &&
                     !text_data(runtime_worklist_ui.split_worklist)->wrap,
                 "Runtime split worklist should not wrap so horizontal "
                 "scrolling can reveal long rows") &&
         ok;

    runtime_worklist_state.screen = ScreenMode::Config;
    update_ui(runtime_worklist_ui, runtime_worklist_state, actions);
    ok = require(text_data(runtime_worklist_ui.split_worklist) &&
                     text_data(runtime_worklist_ui.split_worklist)->wrap,
                 "Config file list should keep wrapping until it gets its own "
                 "horizontal-scroll policy") &&
         ok;
  }
  {
    const std::string rule = detail_section_rule();
    CmdState detail_state = state;
    detail_state.screen = ScreenMode::Runtime;
    detail_state.runtime.focus = RuntimeFocus::Devices;
    detail_state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    detail_state.runtime.device.cpu_usage_pct = 50;
    detail_state.runtime.device.mem_total_kib = 4096;
    detail_state.runtime.device.mem_available_kib = 2048;
    detail_state.runtime.device.gpus[0].uuid = "gpu-alpha";
    detail_state.runtime.device.gpus[0].utilization_gpu_pct = 75;
    detail_state.runtime.device.gpus[0].utilization_mem_pct = 12;
    detail_state.runtime.device.gpus[0].memory_used_mib = 3000;
    detail_state.runtime.device.gpus[0].memory_total_mib = 6000;
    detail_state.runtime.device_history.clear();
    for (int pct : {15, 35, 55, 75, 95}) {
      RuntimeDeviceSnapshot sample = detail_state.runtime.device;
      const int mem_pct = pct / 2;
      sample.cpu_usage_pct = pct;
      sample.mem_total_kib = 1000;
      sample.mem_available_kib =
          static_cast<std::uint64_t>(1000 - mem_pct * 10);
      sample.gpus[0].utilization_gpu_pct = pct;
      sample.gpus[0].memory_used_mib = static_cast<std::uint64_t>(mem_pct);
      sample.gpus[0].memory_total_mib = 100;
      detail_state.runtime.device_history.push_back(sample);
    }
    const std::string device_detail = make_runtime_detail_text(detail_state);
    ok = require(device_detail.find(rule + "\nhost device\n" + rule) !=
                     std::string::npos,
                 "Runtime device detail should restore legacy host "
                 "section-rule framing") &&
         ok;
    ok = require(device_detail.find("focus: devices") == std::string::npos &&
                     device_detail.find(rule + "\ncompute processes\n" +
                                        rule) != std::string::npos,
                 "Runtime host telemetry should omit redundant focus copy and "
                 "keep a compute-process section") &&
         ok;
    ok = require(bar_pct(50, 4).find('#') == std::string::npos &&
                     bar_pct(50, 4).find('.') == std::string::npos &&
                     bar_pct(50, 4).find("█") != std::string::npos &&
                     bar_pct(50, 4).find("░") != std::string::npos,
                 "Runtime percentage bars should use solid and shaded block "
                 "fill") &&
         ok;
    ok = require(device_detail.find(rule + "\nhistory\n" + rule) ==
                         std::string::npos &&
                     device_detail.find("history_plot:") == std::string::npos,
                 "Runtime device detail should leave history labeling to the "
                 "plot axes") &&
         ok;
    CmdUi plot_ui{};
    plot_ui.detail = create_text_box("test.runtime.detail", "", true);
    plot_ui.runtime_device_plot = create_plot_box("test.runtime.plot");
    update_runtime_device_plot_ui(plot_ui, detail_state);
    const auto host_plot = plot_data(plot_ui.runtime_device_plot);
    ok = require(plot_ui.runtime_device_plot->visible && host_plot &&
                     host_plot->series.size() == 2u &&
                     host_plot->series[0].size() == 5u &&
                     host_plot->series[1].size() == 5u &&
                     host_plot->series_cfg.size() == 2u &&
                     host_plot->series_cfg[0].mode == plot_mode_t::Line &&
                     host_plot->series_cfg[0].color_fg == "#67c1ff" &&
                     host_plot->series_cfg[0].use_y2_axis == false &&
                     host_plot->series_cfg[1].color_fg == "#f0d060" &&
                     host_plot->series_cfg[1].use_y2_axis == true &&
                     !host_plot->series_cfg[0].scatter &&
                     plot_ui.runtime_device_plot->layout.dock ==
                         dock_t::Bottom &&
                     plot_ui.runtime_device_plot->layout.pad_right ==
                         kRuntimeDevicePlotPadRight &&
                     host_plot->opts.draw_axes == true &&
                     host_plot->opts.draw_grid == true &&
                     host_plot->opts.baseline0 == false &&
                     host_plot->opts.draw_y_tick_labels == true &&
                     host_plot->opts.draw_y2_tick_labels == true &&
                     host_plot->opts.draw_x_tick_labels == false &&
                     host_plot->opts.margin_left == 5 &&
                     host_plot->opts.margin_right == 6 &&
                     host_plot->opts.y_label == "ram%" &&
                     host_plot->opts.y2_label == "cpu%" &&
                     approx(host_plot->opts.y_max, 47.0) &&
                     approx(host_plot->opts.y2_max, 95.0),
                 "Runtime device activity should use the plot primitive as "
                 "one overlaid memory and CPU chart with colored dual y axes "
                 "scaled to displayed data max") &&
         ok;
    ok = require_frac_len(plot_ui.runtime_device_plot->layout.dock_size,
                          kRuntimeDevicePlotDockFraction,
                          "Runtime device plot should use compact "
                          "proportional bottom history space") &&
         ok;

    detail_state.runtime.selected_device_row = 1u;
    detail_state.runtime.device.gpu_processes = {RuntimeGpuProcessSnapshot{
        .pid = 4242, .gpu_uuid = "gpu-alpha", .used_memory_mib = 256}};
    const std::string device_worklist = make_device_text(detail_state.runtime);
    ok = require(
             device_worklist.find("procs=1") != std::string::npos &&
                 device_worklist.find("gpu processes:") == std::string::npos &&
                 device_worklist.find("\n      temp=") == std::string::npos &&
                 device_worklist.find("\n      load=") == std::string::npos,
             "Runtime device worklist should keep device records on one "
             "line and avoid the duplicate gpu processes row") &&
         ok;
    const std::string gpu_detail = make_runtime_detail_text(detail_state);
    ok = require(gpu_detail.find(rule + "\nhistory\n" + rule) ==
                         std::string::npos &&
                     gpu_detail.find("history_plot:") == std::string::npos,
                 "Runtime GPU detail should leave history labeling to the "
                 "plot axes") &&
         ok;
    ok = require(gpu_detail.find("vram_used: 50%") != std::string::npos &&
                     gpu_detail.find("memory_controller: 12%") !=
                         std::string::npos,
                 "Runtime GPU detail should distinguish VRAM occupancy from "
                 "memory-controller utilization") &&
         ok;
    ok = require(gpu_detail.find(rule + "\ngpu device\n" + rule) !=
                         std::string::npos &&
                     gpu_detail.find("┌ gpu device") == std::string::npos,
                 "Runtime GPU detail should match host section framing "
                 "without a box border") &&
         ok;
    ok = require(gpu_detail.find(rule + "\ncompute processes\n" + rule) !=
                         std::string::npos &&
                     gpu_detail.find("pid=4242  vram=256 MiB") !=
                         std::string::npos,
                 "Runtime GPU detail should render compute processes as a "
                 "matching section") &&
         ok;
    update_runtime_device_plot_ui(plot_ui, detail_state);
    const auto gpu_plot = plot_data(plot_ui.runtime_device_plot);
    ok = require(gpu_plot && gpu_plot->series.size() == 2u &&
                     gpu_plot->series[0].size() == 5u &&
                     gpu_plot->series_cfg[0].mode == plot_mode_t::Line &&
                     gpu_plot->series_cfg[0].color_fg == "#67c1ff" &&
                     gpu_plot->series_cfg[0].use_y2_axis == false &&
                     gpu_plot->series_cfg[1].color_fg == "#7fe08a" &&
                     gpu_plot->series_cfg[1].use_y2_axis == true &&
                     !gpu_plot->series_cfg[0].scatter &&
                     gpu_plot->opts.draw_axes == true &&
                     gpu_plot->opts.draw_y2_tick_labels == true &&
                     gpu_plot->opts.y_label == "vram%" &&
                     gpu_plot->opts.y2_label == "util%" &&
                     approx(gpu_plot->opts.y_max, 47.0) &&
                     approx(gpu_plot->opts.y2_max, 95.0),
                 "Runtime GPU activity should use the plot primitive as "
                 "one overlaid VRAM and utilization line chart with colored "
                 "dual y axes") &&
         ok;
    detail_state.runtime.selected_device_row = 0u;

    detail_state.runtime.focus = RuntimeFocus::Jobs;
    detail_state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    const std::string job_detail = make_runtime_detail_text(detail_state);
    ok = require(job_detail.find(rule + "\njob summary\n" + rule) !=
                         std::string::npos &&
                     job_detail.find(rule + "\nartifacts\n" + rule) !=
                         std::string::npos &&
                     job_detail.find(rule + "\njob state preview\n" + rule) !=
                         std::string::npos,
                 "Runtime job detail should restore legacy section-rule "
                 "framing") &&
         ok;

    detail_state.runtime.focus = RuntimeFocus::Devices;
    detail_state.runtime.detail_mode = RuntimeDetailMode::Trace;
    const std::string trace_detail = make_runtime_detail_text(detail_state);
    ok = require(trace_detail.find(rule + "\nruntime policy\n" + rule) !=
                         std::string::npos &&
                     trace_detail.find(rule + "\nactive wave\n" + rule) !=
                         std::string::npos,
                 "Runtime trace detail should restore section-rule framing "
                 "around policy and wave blocks") &&
         ok;
  }
  {
    const std::string focus_line = "Focus: [Devices] Jobs";
    const std::size_t jobs_offset = focus_line.find("Jobs");
    ok = require(jobs_offset != std::string::npos,
                 "runtime focus selector should include Jobs") &&
         ok;
    ok = require(dispatch_runtime_selector_click(
                     actions, state, ui,
                     ui.main->screen.x + static_cast<int>(jobs_offset),
                     ui.main->screen.y + kRuntimeFocusSelectorRow),
                 "clicking Runtime Jobs selector should dispatch") &&
         ok;
    ok = require(state.runtime.focus == RuntimeFocus::Jobs,
                 "Runtime Jobs selector click should focus jobs") &&
         ok;
  }
  state.runtime.focus = RuntimeFocus::Devices;
  ui.split_panel->visible = true;
  {
    const std::string focus_line = "Focus: [Devices] Jobs";
    const std::size_t jobs_offset = focus_line.find("Jobs");
    ok = require(dispatch_runtime_selector_click(
                     actions, state, ui,
                     ui.split_nav->screen.x + static_cast<int>(jobs_offset),
                     ui.split_nav->screen.y + kRuntimeFocusSelectorRow),
                 "split Runtime navigator Jobs selector should dispatch") &&
         ok;
    ok = require(state.runtime.focus == RuntimeFocus::Jobs,
                 "split Runtime navigator should focus jobs") &&
         ok;
  }
  ui.split_panel->visible = false;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  {
    const std::string detail_line =
        runtime_detail_mode_selector(state.runtime.detail_mode);
    const std::size_t trace_offset = detail_line.find("trace");
    ok = require(trace_offset != std::string::npos,
                 "runtime detail selector should include trace") &&
         ok;
    ok = require(dispatch_runtime_selector_click(
                     actions, state, ui,
                     ui.main->screen.x + static_cast<int>(trace_offset),
                     ui.main->screen.y + kRuntimeDetailSelectorRow),
                 "clicking Runtime trace selector should dispatch") &&
         ok;
    ok = require(state.runtime.detail_mode == RuntimeDetailMode::Trace,
                 "Runtime detail selector click should choose trace") &&
         ok;
  }
  {
    state.runtime.focus = RuntimeFocus::Devices;
    state.runtime.detail_mode = RuntimeDetailMode::Manifest;
    state.workspace.zoom_flags = 0u;
    const std::string runtime_nav = make_runtime_nav_text(state);
    ok = require(
             runtime_nav.find("Focus: [Devices] Jobs") != std::string::npos &&
                 runtime_nav.find("[devices] [jobs]") == std::string::npos &&
                 runtime_nav.find("iinuji.runtime.devices()/") ==
                     std::string::npos,
             "Runtime navigator should expose compact selectors without "
             "command rail blocks") &&
         ok;
    ok = require(!dispatch_content_rail_click(
                     actions, state, ui, ui.main->screen.x, ui.main->screen.y),
                 "hidden Runtime command rail should not dispatch") &&
         ok;
    state.workspace.zoom_flags = 0u;
  }
  state.runtime.focus = RuntimeFocus::Devices;
  state.runtime.detail_mode = RuntimeDetailMode::Manifest;
  const int runtime_device_row =
      runtime_device_header_row(ui.main, kRuntimeDeviceHeaderRow);
  ok = require(dispatch_browser_row_click(state, ui, ui.main->screen.x + 2,
                                          ui.main->screen.y +
                                              runtime_device_row + 3),
               "clicking a runtime GPU row should select it") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Devices &&
                   state.runtime.selected_device_row == 2u,
               "runtime GPU row click should focus the selected GPU") &&
       ok;

  ok = require(dispatch_browser_row_click(state, ui, ui.main->screen.x + 2,
                                          ui.main->screen.y +
                                              runtime_device_row + 5),
               "clicking the runtime jobs header should focus jobs") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Jobs,
               "runtime jobs header click should move focus to jobs") &&
       ok;

  ok = require(dispatch_browser_row_click(state, ui, ui.main->screen.x + 2,
                                          ui.main->screen.y +
                                              runtime_device_row + 7),
               "clicking a runtime job row should select it") &&
       ok;
  ok = require(state.runtime.focus == RuntimeFocus::Jobs &&
                   state.runtime.selected_job == 1u,
               "runtime job row click should focus the selected job") &&
       ok;

  state.screen = ScreenMode::Config;
  state.config.files = {
      ConfigFileSummary{
          .path = "/tmp/alpha.dsl", .relative_path = "alpha.dsl", .size = 10},
      ConfigFileSummary{
          .path = "/tmp/beta.dsl", .relative_path = "beta.dsl", .size = 20},
      ConfigFileSummary{
          .path = "/tmp/gamma.dsl", .relative_path = "gamma.dsl", .size = 30},
  };
  state.config.selected_file = 0;
  {
    CmdUi key_ui{};
    handle_key('j', actions, state, key_ui);
    ok = require(state.config.selected_file == 1u &&
                     state.status == "selected file=2",
                 "Config j key should browse to the next file") &&
         ok;
    handle_key('k', actions, state, key_ui);
    ok = require(state.config.selected_file == 0u &&
                     state.status == "selected file=1",
                 "Config k key should browse to the previous file") &&
         ok;
    handle_key('l', actions, state, key_ui);
    ok = require(state.config.selected_file == 1u &&
                     state.status == "selected file=2",
                 "Config l key should browse to the next file") &&
         ok;
    handle_key('h', actions, state, key_ui);
    ok = require(state.config.selected_file == 0u &&
                     state.status == "selected file=1",
                 "Config h key should browse to the previous file") &&
         ok;
    handle_key('h', actions, state, key_ui);
    ok = require(state.config.selected_file == 2u &&
                     state.status == "selected file=3",
                 "Config h key should wrap from the first file to the last") &&
         ok;
    handle_key('l', actions, state, key_ui);
    ok = require(state.config.selected_file == 0u &&
                     state.status == "selected file=1",
                 "Config l key should wrap from the last file to the first") &&
         ok;
    handle_key('H', actions, state, key_ui);
    ok = require(state.help_view && state.status.find("help overlay=open") !=
                                        std::string::npos,
                 "Config uppercase H should open help because lowercase h "
                 "browses files") &&
         ok;
    state.help_view = false;
    handle_key(key_enter, actions, state, key_ui);
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "show config" && !state.help_view &&
                     state.config.file_editor_open,
                 "Config Enter should restore the advertised read-only preview "
                 "utility") &&
         ok;
    state.status = "ready";
    handle_key('e', actions, state, key_ui);
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "show config" && !state.command_mode &&
                     get_text(key_ui.input).empty() &&
                     state.config.file_editor_open,
                 "Config e key should restore the legacy selected file preview "
                 "shortcut without entering command mode") &&
         ok;
    state.status = "ready";
    handle_key('E', actions, state, key_ui);
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "show config" && !state.command_mode &&
                     get_text(key_ui.input).empty() &&
                     state.config.file_editor_open,
                 "Config uppercase E should mirror the legacy selected file "
                 "preview shortcut") &&
         ok;
  }
  {
    ok =
        require(primitives::editor_syntax_kind_for_path("src/config/.config") ==
                        primitives::editor_syntax_kind_t::Assignment &&
                    primitives::editor_syntax_kind_for_path(
                        "src/config/hero.runtime.dsl") ==
                        primitives::editor_syntax_kind_t::Assignment &&
                    primitives::editor_syntax_kind_for_path(
                        "src/config/grammar/hero.runtime.dsl.bnf") ==
                        primitives::editor_syntax_kind_t::Bnf &&
                    primitives::editor_syntax_kind_for_path(
                        "src/config/README.md") ==
                        primitives::editor_syntax_kind_t::Markdown,
                "Editor syntax highlighter should select config-oriented "
                "modes by file path") &&
        ok;
    const std::filesystem::path editor_path =
        "/tmp/iinuji_cmd_config_editor_test.dsl";
    {
      std::ofstream out(editor_path);
      out << "alpha = 1;\n";
      out << "beta = 2;\n";
    }
    state.config.files[0].path = editor_path;
    state.config.files[0].relative_path = "iinuji_cmd_config_editor_test.dsl";
    state.config.selected_file = 0u;
    state.config.file_editor_open = true;

    CmdUi editor_ui{};
    editor_ui.detail_panel = create_panel("test.detail.panel");
    editor_ui.detail_panel->visible = true;
    editor_ui.detail = create_text_box("test.detail", "", true);
    editor_ui.config_editor =
        create_editor_box("test.config.editor", "", "", true);
    sync_config_editor_ui(editor_ui, state);
    const auto editor = editor_data(editor_ui.config_editor);
    ok = require(editor_ui.config_editor->visible && !editor_ui.detail->visible,
                 "Config preview should show the editor primitive and hide the "
                 "text detail") &&
         ok;
    ok = require(editor && editor->read_only &&
                     primitives::editor_text(*editor).find("alpha = 1;") !=
                         std::string::npos &&
                     static_cast<bool>(editor->line_colorizer),
                 "Config preview editor should load the selected file as "
                 "read-only highlighted text") &&
         ok;
    handle_key(key_down, actions, state, editor_ui);
    ok = require(editor && editor->cursor_line == 1 &&
                     editor->top_line <= editor->cursor_line &&
                     state.status == "cursor",
                 "Config editor should handle navigation at the primitive "
                 "level while open") &&
         ok;
    handle_key(key_escape, actions, state, editor_ui);
    ok = require(!state.config.file_editor_open &&
                     state.status == screen_label(ScreenMode::Config),
                 "Config editor Esc should return to the catalog") &&
         ok;
  }
  {
    CmdUi refresh_ui{};
    CmdState refresh_state = state;
    refresh_state.command_mode = false;
    refresh_state.help_view = false;
    refresh_state.action_menu_open = false;

    refresh_state.screen = ScreenMode::Runtime;
    handle_key('r', actions, refresh_state, refresh_ui);
    ok = require(refresh_state.screen == ScreenMode::Runtime &&
                     refresh_state.status == "runtime screen refresh queued",
                 "Runtime r key should route through the screen-local refresh "
                 "action") &&
         ok;

    refresh_state.screen = ScreenMode::Lattice;
    handle_key('r', actions, refresh_state, refresh_ui);
    ok = require(refresh_state.screen == ScreenMode::Lattice &&
                     refresh_state.status == "lattice refresh queued",
                 "Lattice r key should route through the screen-local refresh "
                 "action") &&
         ok;

    refresh_state.screen = ScreenMode::Config;
    handle_key('r', actions, refresh_state, refresh_ui);
    ok = require(refresh_state.screen == ScreenMode::Config &&
                     refresh_state.status == "config reload queued",
                 "Config r key should route through the screen-local reload "
                 "action") &&
         ok;
  }
  {
    const std::string config_text = make_config_text(state);
    ok = require(config_text.find("Config\n\nRoot:") != std::string::npos &&
                     config_text.find("Navigator\n") == std::string::npos,
                 "Config should expose root metadata without a navigator "
                 "heading") &&
         ok;
    ok = require(config_text.find("\nWorklist\n") != std::string::npos,
                 "Config should expose a worklist section") &&
         ok;
  }
  {
    CmdState family_state = state;
    family_state.config.files = {
        ConfigFileSummary{.path = "/tmp/.config", .relative_path = ".config"},
        ConfigFileSummary{.path = "/tmp/hero.runtime.dsl",
                          .relative_path = "hero.runtime.dsl"},
        ConfigFileSummary{.path = "/tmp/grammar/runtime.bnf",
                          .relative_path = "grammar/runtime.bnf"},
        ConfigFileSummary{.path = "/tmp/man/hero.runtime.man",
                          .relative_path = "man/hero.runtime.man"},
    };
    family_state.config.selected_file = 1u;
    const std::string family_text = make_config_nav_text(family_state);
    ok = require(family_text.find("\nFamilies\n") != std::string::npos,
                 "Config navigator should restore the legacy family summary "
                 "orientation") &&
         ok;
    ok = require(family_text.find("> [hero     ] 1 file") != std::string::npos,
                 "Config family summary should mark the selected catalog "
                 "family") &&
         ok;
    ok = require(
             family_text.find("  [grammar  ] 1 file") != std::string::npos &&
                 family_text.find("  [manuals  ] 1 file") != std::string::npos,
             "Config family summary should count current catalog groups") &&
         ok;
    ok = require(family_text.find("[prev] [next]") == std::string::npos &&
                     family_text.find("iinuji.config.file.prev()/") ==
                         std::string::npos,
                 "Config navigator should avoid browser command rail blocks") &&
         ok;
    const std::string family_worklist = make_config_worklist_text(family_state);
    ok =
        require(family_worklist.find("> [ 2] [hero     ]") != std::string::npos,
                "Config worklist rows should restore the legacy per-row family "
                "badge") &&
        ok;
    ok = require(make_operator_state_line(family_state).find("family=hero") !=
                     std::string::npos,
                 "Config status should expose the selected catalog family") &&
         ok;
  }
  state.config.selected_file = 0u;
  ok = require(!dispatch_content_rail_click(
                   actions, state, ui, ui.main->screen.x, ui.main->screen.y),
               "hidden Config command rail should not dispatch") &&
       ok;
  ok = require(dispatch_browser_row_click(
                   state, ui, ui.main->screen.x + 2,
                   ui.main->screen.y +
                       text_row_after_heading(ui.main, "Worklist",
                                              kConfigBrowserFirstRow) +
                       1),
               "clicking a config browser row should select it") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "config row click should update selected file with legacy "
               "status") &&
       ok;

  state.config.selected_file = 0;
  ui.split_panel->visible = true;
  ok = require(dispatch_browser_row_click(
                   state, ui, ui.split_worklist->screen.x + 2,
                   ui.split_worklist->screen.y + kSplitWorklistFirstRow + 1),
               "split Config worklist row should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "split Config row click should select the second file with "
               "legacy status") &&
       ok;
  scroll_text_to(ui.split_worklist, 9, 0);
  ok = require(!dispatch_content_rail_click(actions, state, ui,
                                            ui.split_nav->screen.x,
                                            ui.split_nav->screen.y),
               "hidden split Config command rail should not dispatch") &&
       ok;
  ui.split_panel->visible = false;

  const int config_first_row =
      text_row_after_heading(ui.main, "Worklist", kConfigBrowserFirstRow);
  ok = require(!dispatch_browser_row_click(state, ui, ui.main->screen.x + 2,
                                           ui.main->screen.y +
                                               config_first_row - 1),
               "clicking above config browser rows should not select") &&
       ok;

  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Config;
  state.config.selected_file = 0;
  {
    const auto edge_row = menu_current_screen_row_index(state, "Home / End");
    ok = require(edge_row.has_value(),
                 "Config menu should expose edge navigation row") &&
         ok;
    ok = require(click_scrolled_menu_row_at(edge_row.value_or(0), 7),
                 "clicking Config current End row should dispatch") &&
         ok;
    ok = require(state.config.selected_file == 2u && !state.help_view,
                 "Config current End row should select last file and close "
                 "menu") &&
         ok;
    state.help_view = true;
    state.config.selected_file = 0u;
    const auto enter_row = menu_current_screen_row_index(state, "Enter / e");
    ok = require(enter_row.has_value(),
                 "Config menu should expose Enter/e preview row") &&
         ok;
    ok = require(click_scrolled_menu_row(enter_row.value_or(0)),
                 "clicking Config current Enter/e row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "show config" && !state.help_view,
                 "Config current Enter/e row should show selected file preview "
                 "and close menu") &&
         ok;
    state.help_view = true;
    state.config.selected_file = 0u;
    const auto files_row =
        menu_current_screen_row_index(state, "files / show config");
    ok = require(files_row.has_value(),
                 "Config menu should expose files/show row") &&
         ok;
    ok = require(click_scrolled_menu_row(files_row.value_or(0)),
                 "clicking Config current files row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "config files" && !state.help_view,
                 "Config current files row should list config files and close "
                 "menu") &&
         ok;
    state.help_view = true;
    const auto show_file_row =
        menu_current_screen_row_index(state, "show file");
    ok = require(show_file_row.has_value(),
                 "Config menu should expose show file row") &&
         ok;
    ok = require(click_scrolled_menu_row(show_file_row.value_or(0)),
                 "clicking Config current show file row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "show config" && !state.help_view,
                 "Config current show file row should show selected file "
                 "summary and close menu") &&
         ok;
    state.help_view = true;
    const auto refresh_row = menu_current_screen_row_index(state, "r");
    ok = require(refresh_row.has_value(),
                 "Config menu should expose screen-specific refresh row") &&
         ok;
    ok = require(click_scrolled_menu_row(refresh_row.value_or(0)),
                 "clicking Config current refresh row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Config &&
                     state.status == "config reload queued" && !state.help_view,
                 "Config current refresh row should run Config reload and "
                 "close menu") &&
         ok;
  }
  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Lattice;
  state.lattice.target_ids = {"target.alpha", "target.beta"};
  state.lattice.selected_target = 1u;
  {
    const auto enter_row = menu_current_screen_row_index(state, "Enter");
    ok = require(enter_row.has_value(),
                 "Lattice menu should expose Enter target proof row") &&
         ok;
    ok = require(click_scrolled_menu_row(enter_row.value_or(0)),
                 "clicking Lattice current Enter row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "show lattice" && !state.help_view,
                 "Lattice current Enter row should show selected target proof "
                 "and close menu") &&
         ok;
    state.help_view = true;
    const auto refresh_row = menu_current_screen_row_index(state, "r");
    ok = require(refresh_row.has_value(),
                 "Lattice menu should expose screen-specific refresh row") &&
         ok;
    ok = require(click_scrolled_menu_row(refresh_row.value_or(0)),
                 "clicking Lattice current refresh row should dispatch") &&
         ok;
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "lattice refresh queued" &&
                     !state.help_view,
                 "Lattice current refresh row should run Lattice refresh and "
                 "close menu") &&
         ok;
  }
  state.config.files = {
      ConfigFileSummary{
          .path = "/tmp/alpha.dsl", .relative_path = "alpha.dsl", .size = 10},
      ConfigFileSummary{
          .path = "/tmp/beta.dsl", .relative_path = "beta.dsl", .size = 20},
      ConfigFileSummary{
          .path = "/tmp/gamma.dsl", .relative_path = "gamma.dsl", .size = 30},
  };
  state.config.selected_file = 0u;
  ok = require(actions.dispatch("config file 2", state),
               "natural config file index command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Config &&
                   state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "natural config file index command should select by index with "
               "legacy status") &&
       ok;
  ok = require(actions.dispatch("file n3", state),
               "short natural config file index command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "short natural config file index command should accept "
               "n-prefix with legacy status") &&
       ok;
  ok = require(actions.dispatch("iinuji.config.file.index.idx2()", state),
               "legacy idx config index atom should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "legacy idx config index atom should select by index with "
               "legacy status") &&
       ok;
  ok = require(actions.dispatch("iinuji.config.file.index.n1()", state),
               "legacy n-prefix config index atom should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 0u &&
                   state.status == "selected file=1",
               "legacy n-prefix config index atom should select by index with "
               "legacy status") &&
       ok;
  ok = require(actions.dispatch("iinuji.config.file.index(n=3)", state),
               "config index argument command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "config index argument command should select by index with "
               "legacy status") &&
       ok;
  ok = require(actions.dispatch("config beta.dsl", state),
               "natural config file id command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "natural config file id command should select by id token with "
               "legacy status") &&
       ok;
  ok = require(
           actions.dispatch("iinuji.config.file.id(value='gamma.dsl')", state),
           "config id argument command should dispatch quoted value") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "config id argument command should select by quoted id token "
               "with legacy status") &&
       ok;
  ok = require(actions.dispatch("file \"alpha.dsl\"", state),
               "natural config file id command should dispatch quoted token") &&
       ok;
  ok = require(state.config.selected_file == 0u &&
                   state.status == "selected file=1",
               "natural config file id command should select by quoted token "
               "with legacy status") &&
       ok;
  ok = require(actions.dispatch("file next", state),
               "short config file next command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "short config file next command should select next file with "
               "legacy status") &&
       ok;
  ok = require(actions.dispatch("config down", state),
               "natural config down command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "natural config down command should select the next file") &&
       ok;
  ok = require(actions.dispatch("config up", state),
               "natural config up command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 1u &&
                   state.status == "selected file=2",
               "natural config up command should select the previous file") &&
       ok;
  ok = require(actions.dispatch("config home", state),
               "natural config home command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 0u &&
                   state.status == "selected file=1",
               "natural config home command should select the first file") &&
       ok;
  ok = require(actions.dispatch("config end", state),
               "natural config end command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "natural config end command should select the last file") &&
       ok;
  ok = require(actions.dispatch("file next", state),
               "config next wrap command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 0u &&
                   state.status == "selected file=1",
               "config next should wrap from the last file to the first") &&
       ok;
  ok = require(actions.dispatch("file prev", state),
               "config previous wrap command should dispatch") &&
       ok;
  ok = require(state.config.selected_file == 2u &&
                   state.status == "selected file=3",
               "config previous should wrap from the first file to the last") &&
       ok;

  state.screen = ScreenMode::Lattice;
  state.lattice.target_ids = {"target.alpha", "target.beta", "target.gamma"};
  state.lattice.selected_target = 0;
  {
    CmdUi key_ui{};
    handle_key('j', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 1u &&
                     state.status == "lattice target",
                 "Lattice j key should browse to the next target") &&
         ok;
    handle_key('k', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 0u &&
                     state.status == "lattice target",
                 "Lattice k key should browse to the previous target") &&
         ok;
    handle_key('l', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 1u &&
                     state.status == "lattice target",
                 "Lattice l key should browse to the next target") &&
         ok;
    handle_key('h', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 0u &&
                     state.status == "lattice target",
                 "Lattice h key should browse to the previous target") &&
         ok;
    handle_key('h', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 2u &&
                     state.status == "lattice target",
                 "Lattice h key should wrap from the first target to the "
                 "last") &&
         ok;
    handle_key('l', actions, state, key_ui);
    ok = require(state.lattice.selected_target == 0u &&
                     state.status == "lattice target",
                 "Lattice l key should wrap from the last target to the "
                 "first") &&
         ok;
    handle_key('H', actions, state, key_ui);
    ok = require(state.help_view && state.status.find("help overlay=open") !=
                                        std::string::npos,
                 "Lattice uppercase H should open help because lowercase h "
                 "browses targets") &&
         ok;
    state.help_view = false;
    handle_key(key_enter, actions, state, key_ui);
    ok = require(state.screen == ScreenMode::Lattice &&
                     state.status == "show lattice" && !state.help_view,
                 "Lattice Enter should restore selected target proof summary "
                 "access") &&
         ok;
  }
  {
    const std::string lattice_text = make_lattice_text(state);
    ok = require(lattice_text.find("Lattice\n\nConfigured targets:") !=
                         std::string::npos &&
                     lattice_text.find("Navigator\n") == std::string::npos,
                 "Lattice should expose target metadata without a navigator "
                 "heading") &&
         ok;
    ok = require(lattice_text.find("\nWorklist\n") != std::string::npos,
                 "Lattice should expose a worklist section") &&
         ok;
    const std::filesystem::path evidence_root =
        "/tmp/iinuji_cmd_lattice_evidence_test";
    std::filesystem::remove_all(evidence_root);
    std::filesystem::create_directories(evidence_root / "job.alpha");
    {
      std::ofstream out(evidence_root / "job.alpha" / "lattice.exposure.fact");
      out << "exposure\n";
    }
    {
      std::ofstream out(evidence_root / "job.alpha" /
                        "lattice.checkpoint.fact");
      out << "checkpoint\n";
    }
    CmdState evidence_state{};
    evidence_state.lattice.runtime_root = evidence_root;
    ok = require(refresh_lattice_runtime_evidence_counts(evidence_state) &&
                     evidence_state.lattice.exposure_fact_count == 1u &&
                     evidence_state.lattice.checkpoint_fact_count == 1u,
                 "Lattice runtime evidence refresh should count fact files") &&
         ok;
    std::filesystem::remove_all(evidence_root);
    ok = require(refresh_lattice_runtime_evidence_counts(evidence_state) &&
                     evidence_state.lattice.exposure_fact_count == 0u &&
                     evidence_state.lattice.checkpoint_fact_count == 0u,
                 "Lattice runtime evidence refresh should notice dev_nuke "
                 "emptied the runtime root") &&
         ok;
    ok = require(make_lattice_nav_text(evidence_state)
                         .find("Runtime evidence: empty after dev_nuke") !=
                     std::string::npos,
                 "Lattice navigator should explain empty runtime evidence") &&
         ok;
  }
  {
    CmdState family_state = state;
    family_state.lattice.target_ids = {
        "channel_mdn_ready",
        "vicreg_train_core_ready",
        "cwu_01v_mdn_train_core_ready",
        "mtf_jepa_mae_vicreg_train_core_ready",
    };
    family_state.lattice.selected_target = 2u;
    const std::string family_text = make_lattice_text(family_state);
    ok = require(family_text.find("\nFamilies\n") != std::string::npos,
                 "Lattice navigator should restore the legacy family summary "
                 "orientation") &&
         ok;
    ok = require(family_text.find("> [cwu_01v    ] 1 target") !=
                     std::string::npos,
                 "Lattice family summary should mark the selected target "
                 "family") &&
         ok;
    ok = require(family_text.find("  [channel_mdn] 1 target") !=
                         std::string::npos &&
                     family_text.find("  [vicreg     ] 1 target") !=
                         std::string::npos,
                 "Lattice family summary should count current target groups") &&
         ok;
    ok = require(family_text.find("[prev] [next]") == std::string::npos &&
                     family_text.find("iinuji.lattice.target.prev()/") ==
                         std::string::npos,
                 "Lattice navigator should avoid browser command rail "
                 "blocks") &&
         ok;
    ok = require(family_text.find("> [ 3] [cwu_01v    ]") != std::string::npos,
                 "Lattice worklist rows should restore the legacy per-row "
                 "family badge") &&
         ok;
    ok =
        require(make_operator_state_line(family_state).find("family=cwu_01v") !=
                    std::string::npos,
                "Lattice status should expose the selected target family") &&
        ok;
  }
  {
    const std::filesystem::path lattice_target_path{
        "/tmp/cuwacunu_cmd_lattice_target_summary.dsl"};
    {
      std::ofstream target_file{lattice_target_path};
      target_file << "LATTICE_TARGET {\n";
      target_file << "  TARGET_ID = channel_mdn_ready;\n";
      target_file << "  TARGET_CLASS = readiness;\n";
      target_file << "  TARGET_KIND = channel_mdn_ready;\n";
      target_file << "  SUBJECT_COMPONENT = "
                     "wikimyei.inference.expected_value.mdn;\n";
      target_file << "  CHECKPOINT_SOURCE = latest_satisfying:vicreg_ready;\n";
      target_file << "  SOURCE_RANGE = anchor_index;\n";
      target_file << "  PROTECT_SPLIT = validation_holdout;\n";
      target_file << "  WAVE_MODE = run|debug;\n";
      target_file << "};\n";
    }
    CmdState detail_state = state;
    detail_state.lattice.targets_path = lattice_target_path;
    detail_state.lattice.target_ids = {"channel_mdn_ready"};
    detail_state.lattice.selected_target = 0u;
    const std::string detail_text = make_lattice_detail_text(detail_state);
    const std::string rule = detail_section_rule();
    ok = require(detail_text.find("\ntarget summary\n") != std::string::npos,
                 "Lattice detail should restore compact selected-target "
                 "metadata before the raw block") &&
         ok;
    ok = require(detail_text.find(rule + "\ntarget summary\n" + rule) !=
                         std::string::npos &&
                     detail_text.find(rule + "\ntarget block\n" + rule) !=
                         std::string::npos,
                 "Lattice detail should restore legacy section-rule framing") &&
         ok;
    ok =
        require(detail_text.find("family: channel_mdn") != std::string::npos &&
                    detail_text.find("class: readiness") != std::string::npos &&
                    detail_text.find("kind: channel_mdn_ready") !=
                        std::string::npos &&
                    detail_text.find("component: "
                                     "wikimyei.inference.expected_value.mdn") !=
                        std::string::npos,
                "Lattice target summary should expose family, class, kind, "
                "and component") &&
        ok;
    ok =
        require(detail_text.find("split: validation_holdout") !=
                        std::string::npos &&
                    detail_text.find("wave: run|debug") != std::string::npos,
                "Lattice target summary should expose split and wave policy") &&
        ok;
  }
  {
    const std::filesystem::path config_detail_path{
        "/tmp/cuwacunu_cmd_config_section_detail.dsl"};
    {
      std::ofstream config_file{config_detail_path};
      config_file << "alpha = beta\n";
    }
    CmdState detail_state = state;
    detail_state.config.files = {ConfigFileSummary{
        .path = config_detail_path, .relative_path = "alpha.dsl", .size = 13}};
    detail_state.config.selected_file = 0u;
    const std::string detail_text = make_config_detail_text(detail_state);
    const std::string rule = detail_section_rule();
    ok =
        require(detail_text.find(rule + "\npreview\n" + rule) !=
                        std::string::npos &&
                    detail_text.find("  1 | alpha = beta") != std::string::npos,
                "Config detail should restore legacy section-rule framing "
                "around the read-only preview") &&
        ok;
  }
  state.lattice.selected_target = 0u;
  ok = require(!dispatch_content_rail_click(
                   actions, state, ui, ui.main->screen.x, ui.main->screen.y),
               "hidden Lattice command rail should not dispatch") &&
       ok;
  ok = require(dispatch_browser_row_click(
                   state, ui, ui.main->screen.x + 2,
                   ui.main->screen.y +
                       text_row_after_heading(ui.main, "Worklist",
                                              kLatticeBrowserFirstRow) +
                       2),
               "clicking a lattice browser row should select it") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "lattice row click should update selected target") &&
       ok;
  state.lattice.selected_target = 0;
  ui.split_panel->visible = true;
  scroll_text_to(ui.split_worklist, 0, 0);
  ok = require(dispatch_browser_row_click(
                   state, ui, ui.split_worklist->screen.x + 2,
                   ui.split_worklist->screen.y + kSplitWorklistFirstRow + 1),
               "split Lattice worklist row should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 1u,
               "split Lattice row click should select the second target") &&
       ok;
  ui.split_panel->visible = false;
  state.help_view = true;
  state.action_menu_open = false;
  state.screen = ScreenMode::Lattice;
  state.lattice.selected_target = 1u;
  {
    const auto first = menu_current_screen_first_row(state);
    ok = require(first.has_value(),
                 "Lattice menu should expose current-screen rows") &&
         ok;
    ok = require(click_scrolled_menu_row_at(*first, 0),
                 "clicking Lattice current Up row should dispatch") &&
         ok;
    ok = require(state.lattice.selected_target == 0u && !state.help_view,
                 "Lattice current Up row should select previous target and "
                 "close menu") &&
         ok;
  }
  ok = require(actions.dispatch("iinuji.lattice.target.index.2()", state),
               "lattice target index command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.lattice.selected_target == 1u,
               "lattice target index command should select by index") &&
       ok;
  ok = require(
           actions.dispatch("iinuji.lattice.target.id.target.gamma()", state),
           "lattice target id command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "lattice target id command should select by normalized id") &&
       ok;
  ok = require(actions.dispatch("target n2", state),
               "short natural lattice target index command should dispatch") &&
       ok;
  ok = require(state.screen == ScreenMode::Lattice &&
                   state.lattice.selected_target == 1u,
               "short natural lattice target index command should select by "
               "index") &&
       ok;
  ok = require(actions.dispatch("iinuji.lattice.target.index(v3)", state),
               "legacy lattice index atom should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "legacy lattice index atom should select by index") &&
       ok;
  ok = require(actions.dispatch("iinuji.lattice.target.index.n1()", state),
               "legacy n-prefix lattice index atom should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 0u,
               "legacy n-prefix lattice index atom should select by index") &&
       ok;
  ok = require(actions.dispatch("iinuji.lattice.target.index(index=2)", state),
               "lattice index argument command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 1u,
               "lattice index argument command should select by index") &&
       ok;
  ok = require(actions.dispatch("lattice target target.gamma", state),
               "natural lattice target id command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "natural lattice target id command should select by id token") &&
       ok;
  ok = require(actions.dispatch(
                   "iinuji.lattice.target.id(value=\"target.beta\")", state),
               "lattice id argument command should dispatch quoted value") &&
       ok;
  ok = require(state.lattice.selected_target == 1u,
               "lattice id argument command should select quoted id token") &&
       ok;
  ok = require(actions.dispatch("target 'target.gamma'", state),
               "natural lattice target id command should dispatch quoted "
               "token") &&
       ok;
  ok =
      require(state.lattice.selected_target == 2u,
              "natural lattice target id command should select quoted token") &&
      ok;
  ok = require(actions.dispatch("target prev", state),
               "short lattice target previous command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 1u,
               "short lattice target previous command should select previous "
               "target") &&
       ok;
  ok = require(actions.dispatch("lattice down", state),
               "natural lattice down command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "natural lattice down command should select the next target") &&
       ok;
  ok = require(actions.dispatch("lattice up", state),
               "natural lattice up command should dispatch") &&
       ok;
  ok =
      require(state.lattice.selected_target == 1u,
              "natural lattice up command should select the previous target") &&
      ok;
  ok = require(actions.dispatch("lattice home", state),
               "natural lattice home command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 0u,
               "natural lattice home command should select the first target") &&
       ok;
  ok = require(actions.dispatch("lattice end", state),
               "natural lattice end command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "natural lattice end command should select the last target") &&
       ok;
  ok = require(actions.dispatch("target next", state),
               "lattice next wrap command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 0u,
               "lattice next should wrap from the last target to the first") &&
       ok;
  ok = require(actions.dispatch("target prev", state),
               "lattice previous wrap command should dispatch") &&
       ok;
  ok = require(state.lattice.selected_target == 2u,
               "lattice previous should wrap from the first target to the "
               "last") &&
       ok;

  if (ok)
    std::cout << "iinuji cmd nav rail smoke ok\n";
  return ok ? 0 : 1;
}
