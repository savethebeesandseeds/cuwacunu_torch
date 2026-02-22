#include <iostream>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"

#include "iinuji/iinuji_cmd/commands/iinuji.path.handlers.h"

namespace {

bool require(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  try {
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    using namespace cuwacunu::iinuji::iinuji_cmd;

    CmdState st{};
    st.config = load_config_view_from_config();
    clamp_selected_tab(st);

    st.board = load_board_from_config();
    clamp_selected_circuit(st);

    st.data = load_data_view_from_config(&st.board);
    clamp_selected_data_channel(st);
    clamp_data_plot_mode(st);
    clamp_data_plot_x_axis(st);
    clamp_data_nav_focus(st);
    clamp_selected_tsi_tab(st);

    IinujiPathHandlers handlers{st};

    std::vector<std::string> infos;
    std::vector<std::string> warns;
    std::vector<std::string> errs;
    std::vector<std::string> appends;

    auto push_info = [&](const std::string& m) { infos.push_back(m); };
    auto push_warn = [&](const std::string& m) { warns.push_back(m); };
    auto push_err = [&](const std::string& m) { errs.push_back(m); };
    auto append_log = [&](const std::string& text, const std::string&, const std::string&) {
      appends.push_back(text);
    };

    bool ok = true;

    ok = ok && require(handlers.dispatch_text("iinuji.screen.data()", push_info, push_warn, push_err),
                       "screen.data canonical path should be handled");
    ok = ok && require(st.screen == ScreenMode::Data,
                       "screen.data canonical path should switch to data screen");

    ok = ok && require(handlers.dispatch_text("iinuji.screen.home()", push_info, push_warn, push_err),
                       "screen.home canonical path should be handled");
    ok = ok && require(st.screen == ScreenMode::Home,
                       "screen.home canonical path should switch to home screen");

    ok = ok && require(handlers.dispatch_text("iinuji.help()", push_info, push_warn, push_err),
                       "help canonical path should be handled");
    ok = ok && require(st.help_view, "help canonical path should open help overlay");
    st.help_view = false;
    ok = ok && require(handlers.dispatch_text("iinuji.help", push_info, push_warn, push_err),
                       "help canonical shorthand should be handled");
    ok = ok && require(st.help_view, "help canonical shorthand should open help overlay");
    st.help_view = false;

    const std::size_t err_count_before_strict_help = errs.size();
    ok = ok && require(handlers.dispatch_canonical_text("iinuji.help()", push_info, push_warn, push_err),
                       "strict canonical dispatch should accept exact help path");
    ok = ok && require(st.help_view, "strict canonical help path should open help overlay");
    st.help_view = false;
    ok = ok && require(!handlers.dispatch_canonical_text("iinuji.help", push_info, push_warn, push_err),
                       "strict canonical dispatch should reject help shorthand without ()");
    ok = ok && require(errs.size() > err_count_before_strict_help,
                       "strict canonical dispatch rejection should emit an error");

    st.help_scroll_y = 0;
    st.help_scroll_x = 0;
    ok = ok && require(handlers.dispatch_text("iinuji.help.scroll.down()", push_info, push_warn, push_err),
                       "help.scroll.down canonical path should be handled");
    ok = ok && require(st.help_view, "help.scroll.down should open help overlay");
    ok = ok && require(st.help_scroll_y > 0, "help.scroll.down should increase help y scroll");

    ok = ok && require(handlers.dispatch_text("iinuji.help.scroll.right()", push_info, push_warn, push_err),
                       "help.scroll.right canonical path should be handled");
    ok = ok && require(st.help_scroll_x > 0, "help.scroll.right should increase help x scroll");

    ok = ok && require(handlers.dispatch_text("iinuji.help.scroll.home()", push_info, push_warn, push_err),
                       "help.scroll.home canonical path should be handled");
    ok = ok && require(st.help_scroll_y == 0 && st.help_scroll_x == 0,
                       "help.scroll.home should reset help scroll");

    ok = ok && require(handlers.dispatch_text("iinuji.help.scroll.end()", push_info, push_warn, push_err),
                       "help.scroll.end canonical path should be handled");
    ok = ok && require(st.help_scroll_y == std::numeric_limits<int>::max(),
                       "help.scroll.end should jump to tail");

    ok = ok && require(handlers.dispatch_text("iinuji.help.close()", push_info, push_warn, push_err),
                       "help.close canonical path should be handled");
    ok = ok && require(!st.help_view, "help.close should hide help overlay");

    ok = ok && require(handlers.dispatch_text("iinuji.quit()", push_info, push_warn, push_err),
                       "quit canonical path should be handled");
    ok = ok && require(!st.running, "quit canonical path should set running=false");
    st.running = true;

    ok = ok && require(handlers.dispatch_text("iinuji.exit()", push_info, push_warn, push_err),
                       "exit canonical path should be handled");
    ok = ok && require(!st.running, "exit canonical path should set running=false");
    st.running = true;

    ok = ok && require(handlers.dispatch_text("iinuji.logs.clear()", push_info, push_warn, push_err),
                       "logs.clear canonical path should be handled");
    ok = ok && require(st.screen == ScreenMode::Logs,
                       "logs.clear canonical path should switch to logs screen");

    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_board_select_index(1), push_info, push_warn, push_err),
        "board.select.index canonical path should be handled");
    if (!st.board.board.circuits.empty()) {
      ok = ok && require(st.screen == ScreenMode::Board,
                         "board.select.index canonical path should switch to board screen");
      ok = ok && require(st.board.selected_circuit == 0,
                         "board.select.index.n1() should select first circuit");
    }
    ok = ok && require(
        handlers.dispatch_text("iinuji.board.select.index.n1", push_info, push_warn, push_err),
        "board.select.index shorthand should be handled");
    if (!st.board.board.circuits.empty()) {
      ok = ok && require(st.board.selected_circuit == 0,
                         "board.select.index.n1 shorthand should select first circuit");
    }

    const std::size_t err_count_before_strict_board_index = errs.size();
    ok = ok && require(
        handlers.dispatch_canonical_text(canonical_paths::build_board_select_index(1), push_info, push_warn, push_err),
        "strict canonical dispatch should accept board.select.index.n1()");
    ok = ok && require(
        !handlers.dispatch_canonical_text("iinuji.board.select.index.n1", push_info, push_warn, push_err),
        "strict canonical dispatch should reject board.select.index shorthand without ()");
    ok = ok && require(errs.size() > err_count_before_strict_board_index,
                       "strict canonical board index rejection should emit an error");

    ok = ok && require(
        handlers.dispatch_text("iinuji.board.select.next()", push_info, push_warn, push_err),
        "board.select.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.board.select.prev()", push_info, push_warn, push_err),
        "board.select.prev canonical path should be handled");

    const std::size_t appends_before_board_list = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.board.list()", push_info, push_warn, push_err, append_log),
        "board.list canonical path should be handled");
    if (!st.board.board.circuits.empty()) {
      ok = ok && require(appends.size() > appends_before_board_list,
                         "board.list canonical path should append list lines");
    }

    ok = ok && require(handlers.dispatch_text("iinuji.view.data.plot(mode=seq)", push_info, push_warn, push_err),
                       "data.plot(mode=seq) canonical path should be handled");
    ok = ok && require(st.screen == ScreenMode::Data,
                       "data.plot canonical path should switch to data screen");
    ok = ok && require(st.data.plot_mode == DataPlotMode::SeqLength,
                       "data.plot(mode=seq) should set SeqLength mode");

    ok = ok && require(handlers.dispatch_text("iinuji.data.plot.off()", push_info, push_warn, push_err),
                       "data.plot.off canonical path should be handled");
    ok = ok && require(!st.data.plot_view,
                       "data.plot.off should disable plot view");
    ok = ok && require(handlers.dispatch_text("iinuji.data.plot.on()", push_info, push_warn, push_err),
                       "data.plot.on canonical path should be handled");
    ok = ok && require(st.data.plot_view,
                       "data.plot.on should enable plot view");
    const bool plot_view_before_toggle = st.data.plot_view;
    ok = ok && require(handlers.dispatch_text("iinuji.data.plot.toggle()", push_info, push_warn, push_err),
                       "data.plot.toggle canonical path should be handled");
    ok = ok && require(st.data.plot_view != plot_view_before_toggle,
                       "data.plot.toggle should flip plot view");

    ok = ok && require(handlers.dispatch_text("iinuji.data.plot.mode.future()", push_info, push_warn, push_err),
                       "data.plot.mode.future canonical path should be handled");
    ok = ok && require(st.data.plot_mode == DataPlotMode::FutureSeqLength,
                       "data.plot.mode.future should set FutureSeqLength mode");
    ok = ok && require(handlers.dispatch_text("iinuji.data.plot.mode.seq()", push_info, push_warn, push_err),
                       "data.plot.mode.seq canonical path should be handled");
    ok = ok && require(st.data.plot_mode == DataPlotMode::SeqLength,
                       "data.plot.mode.seq should set SeqLength mode");

    ok = ok && require(handlers.dispatch_text("iinuji.state.reload.data()", push_info, push_warn, push_err),
                       "state.reload.data canonical path should be handled");
    ok = ok && require(st.data.ok || !st.data.error.empty(),
                       "reload.data should produce either ok data or a concrete error");

    ok = ok && require(handlers.dispatch_text("iinuji.data.reload()", push_info, push_warn, push_err),
                       "data.reload canonical path should be handled");
    ok = ok && require(st.data.ok || !st.data.error.empty(),
                       "data.reload should produce either ok data or a concrete error");

    const std::size_t appends_before_data_channels = appends.size();
    const std::size_t warns_before_data_channels = warns.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.channels()", push_info, push_warn, push_err, append_log),
        "data.channels canonical path should be handled");
    if (data_has_channels(st)) {
      ok = ok && require(appends.size() > appends_before_data_channels,
                         "data.channels canonical path should append channel lines");
      ok = ok && require(st.screen == ScreenMode::Data,
                         "data.channels canonical path should switch to data screen");
    } else {
      ok = ok && require(warns.size() > warns_before_data_channels,
                         "data.channels canonical path should warn when no channels exist");
    }

    ok = ok && require(
        handlers.dispatch_text("iinuji.data.ch.next()", push_info, push_warn, push_err),
        "data.ch.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.ch.prev()", push_info, push_warn, push_err),
        "data.ch.prev canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_data_ch_index(1), push_info, push_warn, push_err),
        "data.ch.index canonical path should be handled");
    if (data_has_channels(st)) {
      ok = ok && require(st.data.selected_channel == 0,
                         "data.ch.index.n1() should select first data channel");
    }

    const auto x_before = st.data.plot_x_axis;
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.x(axis=toggle)", push_info, push_warn, push_err),
        "data.x canonical path should be handled");
    ok = ok && require(st.data.plot_x_axis != x_before || data_plot_x_axis_count() <= 1,
                       "data.x(axis=toggle) should toggle x-axis when multiple options exist");

    ok = ok && require(
        handlers.dispatch_text("iinuji.data.axis.idx()", push_info, push_warn, push_err),
        "data.axis.idx canonical path should be handled");
    ok = ok && require(st.data.plot_x_axis == DataPlotXAxis::Index,
                       "data.axis.idx should set index axis");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.axis.key()", push_info, push_warn, push_err),
        "data.axis.key canonical path should be handled");
    ok = ok && require(st.data.plot_x_axis == DataPlotXAxis::KeyValue,
                       "data.axis.key should set key axis");
    const auto axis_before_toggle = st.data.plot_x_axis;
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.axis.toggle()", push_info, push_warn, push_err),
        "data.axis.toggle canonical path should be handled");
    ok = ok && require(st.data.plot_x_axis != axis_before_toggle || data_plot_x_axis_count() <= 1,
                       "data.axis.toggle should toggle x-axis when multiple options exist");

    const bool mask_before = st.data.plot_mask_overlay;
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.mask(view=toggle)", push_info, push_warn, push_err),
        "data.mask canonical path should be handled");
    ok = ok && require(st.data.plot_mask_overlay != mask_before,
                       "data.mask(view=toggle) should toggle mask flag");

    ok = ok && require(
        handlers.dispatch_text("iinuji.data.mask.off()", push_info, push_warn, push_err),
        "data.mask.off canonical path should be handled");
    ok = ok && require(!st.data.plot_mask_overlay,
                       "data.mask.off should disable mask");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.mask.on()", push_info, push_warn, push_err),
        "data.mask.on canonical path should be handled");
    ok = ok && require(st.data.plot_mask_overlay,
                       "data.mask.on should enable mask");
    const bool mask_before_toggle = st.data.plot_mask_overlay;
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.mask.toggle()", push_info, push_warn, push_err),
        "data.mask.toggle canonical path should be handled");
    ok = ok && require(st.data.plot_mask_overlay != mask_before_toggle,
                       "data.mask.toggle should flip mask flag");

    ok = ok && require(
        handlers.dispatch_text("iinuji.data.sample.next()", push_info, push_warn, push_err),
        "data.sample.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.sample.prev()", push_info, push_warn, push_err),
        "data.sample.prev canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.sample.random()", push_info, push_warn, push_err),
        "data.sample.random canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.sample.rand()", push_info, push_warn, push_err),
        "data.sample.rand canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_data_sample_index(1), push_info, push_warn, push_err),
        "data.sample.index canonical path should be handled");
    if (st.data.plot_sample_count > 0) {
      ok = ok && require(st.data.plot_sample_index == 0,
                         "data.sample.index.n1() should select first sample");
    }

    ok = ok && require(
        handlers.dispatch_text("iinuji.data.dim.next()", push_info, push_warn, push_err),
        "data.dim.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.dim.prev()", push_info, push_warn, push_err),
        "data.dim.prev canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_data_dim_index(1), push_info, push_warn, push_err),
        "data.dim.index canonical path should be handled");
    if (st.data.plot_D > 0) {
      ok = ok && require(st.data.plot_feature_dim == 0,
                         "data.dim.index.n1() should select first dim");
      const auto& c = st.data.channels[std::min(st.data.selected_channel, st.data.channels.size() - 1)];
      const auto names = data_feature_names_for_record_type(c.record_type);
      if (!names.empty()) {
        ok = ok && require(
            handlers.dispatch_text(canonical_paths::build_data_dim_id(names.front()),
                                   push_info,
                                   push_warn,
                                   push_err),
            "data.dim.id canonical path should be handled");
      }
    }

    st.data.nav_focus = DataNavFocus::Channel;
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.next()", push_info, push_warn, push_err),
        "data.focus.next canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Sample,
                       "data.focus.next should advance focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.prev()", push_info, push_warn, push_err),
        "data.focus.prev canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Channel,
                       "data.focus.prev should move focus backward");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.channel()", push_info, push_warn, push_err),
        "data.focus.channel canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Channel,
                       "data.focus.channel should set channel focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.sample()", push_info, push_warn, push_err),
        "data.focus.sample canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Sample,
                       "data.focus.sample should set sample focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.dim()", push_info, push_warn, push_err),
        "data.focus.dim canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Dim,
                       "data.focus.dim should set dim focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.plot()", push_info, push_warn, push_err),
        "data.focus.plot canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::PlotMode,
                       "data.focus.plot should set plot mode focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.xaxis()", push_info, push_warn, push_err),
        "data.focus.xaxis canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::XAxis,
                       "data.focus.xaxis should set x-axis focus");
    ok = ok && require(
        handlers.dispatch_text("iinuji.data.focus.mask()", push_info, push_warn, push_err),
        "data.focus.mask canonical path should be handled");
    ok = ok && require(st.data.nav_focus == DataNavFocus::Mask,
                       "data.focus.mask should set mask focus");

    const std::size_t appends_before_tsi_tabs = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.tsi.tabs()", push_info, push_warn, push_err, append_log),
        "tsi.tabs canonical path should be handled");
    if (tsi_tab_count() > 0) {
      ok = ok && require(st.screen == ScreenMode::Tsiemene,
                         "tsi.tabs canonical path should switch to tsi screen");
      ok = ok && require(appends.size() > appends_before_tsi_tabs,
                         "tsi.tabs canonical path should append tab lines");
    }

    ok = ok && require(
        handlers.dispatch_text("iinuji.tsi.tab.next()", push_info, push_warn, push_err),
        "tsi.tab.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.tsi.tab.prev()", push_info, push_warn, push_err),
        "tsi.tab.prev canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_tsi_tab_index(1), push_info, push_warn, push_err),
        "tsi.tab.index canonical path should be handled");

    const auto& docs = tsi_node_docs();
    if (!docs.empty()) {
      ok = ok && require(
          handlers.dispatch_text(canonical_paths::build_tsi_tab_id(docs.front().id),
                                 push_info,
                                 push_warn,
                                 push_err),
          "tsi.tab.id canonical path should be handled");
      ok = ok && require(st.tsiemene.selected_tab == 0,
                         "tsi.tab.id.<token>() should select first tab");
    }

    const std::size_t appends_before_config_tabs = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.config.tabs()", push_info, push_warn, push_err, append_log),
        "config.tabs canonical path should be handled");
    if (config_has_tabs(st)) {
      ok = ok && require(st.screen == ScreenMode::Config,
                         "config.tabs canonical path should switch to config screen");
      ok = ok && require(appends.size() > appends_before_config_tabs,
                         "config.tabs canonical path should append tab lines");
    }

    ok = ok && require(
        handlers.dispatch_text("iinuji.config.tab.next()", push_info, push_warn, push_err),
        "config.tab.next canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text("iinuji.config.tab.prev()", push_info, push_warn, push_err),
        "config.tab.prev canonical path should be handled");
    ok = ok && require(
        handlers.dispatch_text(canonical_paths::build_config_tab_index(1), push_info, push_warn, push_err),
        "config.tab.index canonical path should be handled");

    if (config_has_tabs(st)) {
      ok = ok && require(st.config.selected_tab == 0,
                         "config.tab.index.n1() should select first config tab");
      ok = ok && require(
          handlers.dispatch_text(canonical_paths::build_config_tab_id(st.config.tabs.front().id),
                                 push_info,
                                 push_warn,
                                 push_err),
          "config.tab.id canonical path should be handled");
      ok = ok && require(st.config.selected_tab == 0,
                         "config.tab.id.<token>() should select first config tab");
    }

    ok = ok && require(
        handlers.dispatch_text("iinuji.config.reload()", push_info, push_warn, push_err),
        "config.reload canonical path should be handled");
    ok = ok && require(st.config.ok || !st.config.error.empty(),
                       "config.reload should produce either ok config or a concrete error");

    const std::size_t appends_before_config_show = appends.size();
    const std::size_t warns_before_config_show = warns.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.config.show()", push_info, push_warn, push_err, append_log),
        "config.show canonical path should be handled");
    if (config_has_tabs(st)) {
      ok = ok && require(appends.size() > appends_before_config_show,
                         "config.show canonical path should append show lines");
    } else {
      ok = ok && require(warns.size() > warns_before_config_show,
                         "config.show canonical path should warn when no tabs exist");
    }

    const std::size_t appends_before_config_tab_show = appends.size();
    const std::size_t warns_before_config_tab_show = warns.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.config.tab.show()", push_info, push_warn, push_err, append_log),
        "config.tab.show canonical path should be handled");
    if (config_has_tabs(st)) {
      ok = ok && require(appends.size() > appends_before_config_tab_show,
                         "config.tab.show canonical path should append show lines");
    } else {
      ok = ok && require(warns.size() > warns_before_config_tab_show,
                         "config.tab.show canonical path should warn when no tabs exist");
    }

    const std::size_t appends_before_show_data = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.show.data()", push_info, push_warn, push_err, append_log),
        "show.data canonical path should be handled");
    ok = ok && require(appends.size() > appends_before_show_data,
                       "show.data canonical path should append show lines");

    const std::size_t appends_before_show_home = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.show.home()", push_info, push_warn, push_err, append_log),
        "show.home canonical path should be handled");
    ok = ok && require(appends.size() > appends_before_show_home,
                       "show.home canonical path should append lines");

    const std::size_t appends_before_show_logs = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.show.logs()", push_info, push_warn, push_err, append_log),
        "show.logs canonical path should be handled");
    ok = ok && require(appends.size() > appends_before_show_logs,
                       "show.logs canonical path should append lines");

    st.screen = ScreenMode::Logs;
    const std::size_t appends_before_show_current = appends.size();
    ok = ok && require(
        handlers.dispatch_text("iinuji.show()", push_info, push_warn, push_err, append_log),
        "show() canonical path should be handled");
    ok = ok && require(appends.size() > appends_before_show_current,
                       "show() canonical path should append lines for current screen");

    st.logs.level_filter = LogsLevelFilter::DebugOrHigher;
    st.logs.show_date = true;
    st.logs.show_thread = true;
    st.logs.show_color = true;
    st.logs.auto_follow = true;
    st.logs.mouse_capture = true;
    st.logs.selected_setting = 0;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.select.next()", push_info, push_warn, push_err),
        "logs.settings.select.next canonical path should be handled");
    ok = ok && require(st.logs.selected_setting == 1,
                       "logs.settings.select.next should move selected setting forward");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.select.prev()", push_info, push_warn, push_err),
        "logs.settings.select.prev canonical path should be handled");
    ok = ok && require(st.logs.selected_setting == 0,
                       "logs.settings.select.prev should move selected setting backward");

    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.level.info()", push_info, push_warn, push_err),
        "logs.settings.level.info canonical path should be handled");
    ok = ok && require(st.logs.level_filter == LogsLevelFilter::InfoOrHigher,
                       "logs.settings.level.info should update level filter");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.level.warning()", push_info, push_warn, push_err),
        "logs.settings.level.warning canonical path should be handled");
    ok = ok && require(st.logs.level_filter == LogsLevelFilter::WarningOrHigher,
                       "logs.settings.level.warning should update level filter");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.level.error()", push_info, push_warn, push_err),
        "logs.settings.level.error canonical path should be handled");
    ok = ok && require(st.logs.level_filter == LogsLevelFilter::ErrorOrHigher,
                       "logs.settings.level.error should update level filter");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.level.fatal()", push_info, push_warn, push_err),
        "logs.settings.level.fatal canonical path should be handled");
    ok = ok && require(st.logs.level_filter == LogsLevelFilter::FatalOnly,
                       "logs.settings.level.fatal should update level filter");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.level.debug()", push_info, push_warn, push_err),
        "logs.settings.level.debug canonical path should be handled");
    ok = ok && require(st.logs.level_filter == LogsLevelFilter::DebugOrHigher,
                       "logs.settings.level.debug should update level filter");

    const bool logs_date_before = st.logs.show_date;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.date.toggle()", push_info, push_warn, push_err),
        "logs.settings.date.toggle canonical path should be handled");
    ok = ok && require(st.logs.show_date != logs_date_before,
                       "logs.settings.date.toggle should flip show_date");

    const bool logs_thread_before = st.logs.show_thread;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.thread.toggle()", push_info, push_warn, push_err),
        "logs.settings.thread.toggle canonical path should be handled");
    ok = ok && require(st.logs.show_thread != logs_thread_before,
                       "logs.settings.thread.toggle should flip show_thread");

    const bool logs_color_before = st.logs.show_color;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.color.toggle()", push_info, push_warn, push_err),
        "logs.settings.color.toggle canonical path should be handled");
    ok = ok && require(st.logs.show_color != logs_color_before,
                       "logs.settings.color.toggle should flip show_color");

    const bool logs_follow_before = st.logs.auto_follow;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.follow.toggle()", push_info, push_warn, push_err),
        "logs.settings.follow.toggle canonical path should be handled");
    ok = ok && require(st.logs.auto_follow != logs_follow_before,
                       "logs.settings.follow.toggle should flip auto_follow");

    const bool logs_mouse_before = st.logs.mouse_capture;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.settings.mouse.capture.toggle()", push_info, push_warn, push_err),
        "logs.settings.mouse.capture.toggle canonical path should be handled");
    ok = ok && require(st.logs.mouse_capture != logs_mouse_before,
                       "logs.settings.mouse.capture.toggle should flip mouse_capture");
    ok = ok && require(st.screen == ScreenMode::Logs,
                       "logs settings commands should switch to logs screen");

    st.logs.pending_scroll_y = 0;
    st.logs.pending_scroll_x = 0;
    st.logs.pending_jump_home = false;
    st.logs.pending_jump_end = false;
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.up()", push_info, push_warn, push_err),
        "logs.scroll.up canonical path should be handled");
    ok = ok && require(st.logs.pending_scroll_y < 0,
                       "logs.scroll.up should queue negative vertical scroll");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.down()", push_info, push_warn, push_err),
        "logs.scroll.down canonical path should be handled");
    ok = ok && require(st.logs.pending_scroll_y == 0,
                       "logs.scroll.down should cancel prior up step");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.page.up()", push_info, push_warn, push_err),
        "logs.scroll.page.up canonical path should be handled");
    ok = ok && require(st.logs.pending_scroll_y < 0,
                       "logs.scroll.page.up should queue page-up scroll");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.page.down()", push_info, push_warn, push_err),
        "logs.scroll.page.down canonical path should be handled");
    ok = ok && require(st.logs.pending_scroll_y == 0,
                       "logs.scroll.page.down should cancel prior page-up scroll");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.home()", push_info, push_warn, push_err),
        "logs.scroll.home canonical path should be handled");
    ok = ok && require(st.logs.pending_jump_home && !st.logs.pending_jump_end,
                       "logs.scroll.home should queue home jump only");
    ok = ok && require(!st.logs.auto_follow,
                       "logs.scroll.home should disable auto-follow");
    ok = ok && require(
        handlers.dispatch_text("iinuji.logs.scroll.end()", push_info, push_warn, push_err),
        "logs.scroll.end canonical path should be handled");
    ok = ok && require(st.logs.pending_jump_end && !st.logs.pending_jump_home,
                       "logs.scroll.end should queue end jump only");
    ok = ok && require(st.logs.auto_follow,
                       "logs.scroll.end should enable auto-follow");

    const std::size_t errs_before_unknown = errs.size();
    ok = ok && require(handlers.dispatch_text("iinuji.unknown()", push_info, push_warn, push_err),
                       "unknown canonical path should be consumed by canonical dispatcher");
    ok = ok && require(errs.size() > errs_before_unknown,
                       "unknown canonical path should emit an error");

    ok = ok && require(!handlers.dispatch_text("help", push_info, push_warn, push_err),
                       "non-tsi command should not be consumed by canonical dispatcher");

    std::cout << "infos=" << infos.size() << " warns=" << warns.size() << " errs=" << errs.size() << "\n";
    std::cout << "[round-canon] NOTE(hashimyei): revisit hash function design space (word-combo/fun encodings).\n";

    if (!ok) return 1;
    std::cout << "[ok] iinuji canonical path handlers smoke passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[test_iinuji_cmd_canonical_paths] exception: " << e.what() << "\n";
    return 1;
  }
}
