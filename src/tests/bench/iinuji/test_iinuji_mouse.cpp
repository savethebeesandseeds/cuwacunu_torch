// test_iinuji_mouse.cpp
#include <locale.h>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <optional>
#include <cstdio>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/primitives/plot.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

using namespace cuwacunu::iinuji;

// Some ncurses builds may not define BUTTON_SHIFT; make it a no-op if missing.
#ifndef BUTTON_SHIFT
#define BUTTON_SHIFT 0
#endif

/* ---------- Sample data ---------- */
static std::vector<std::pair<double,double>> make_market(int N=1200) {
  std::vector<std::pair<double,double>> pts; pts.reserve(N);
  const double dt=1.0/252.0, mu=0.08, sigma=0.22, S0=100.0;
  std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::normal_distribution<double> z(0.0,1.0);
  double S=S0;
  for (int i=0;i<N;++i) {
    double dlogS=(mu-0.5*sigma*sigma)*dt + sigma*std::sqrt(dt)*z(rng);
    S *= std::exp(dlogS);
    pts.emplace_back(i, S);
  }
  return pts;
}
static std::vector<std::pair<double,double>> moving_avg(const std::vector<std::pair<double,double>>& s, int w) {
  std::vector<std::pair<double,double>> o; if (s.empty()) return o;
  o.reserve(s.size()); double acc=0; int k=0;
  for (size_t i=0;i<s.size();++i){ acc+=s[i].second; ++k; if ((int)k>w){ acc-=s[i-w].second; --k; } o.emplace_back(s[i].first, acc/std::max(1,k)); }
  return o;
}

/* ---------- Plot geometry helpers (mirror renderer) ---------- */
static Rect content_rect_for(const std::shared_ptr<iinuji_object_t>& obj) {
  Rect r = obj->screen;
  if (obj->style.border) r = Rect{ r.x+1, r.y+1, std::max(0,r.w-2), std::max(0,r.h-2) };
  r = Rect{
    r.x + obj->layout.pad_left,
    r.y + obj->layout.pad_top,
    std::max(0, r.w - (obj->layout.pad_left + obj->layout.pad_right)),
    std::max(0, r.h - (obj->layout.pad_top  + obj->layout.pad_bottom))
  };
  return r;
}
struct PlotGeom {
  Rect content; int plot_x0, plot_y0, plot_w, plot_h;
  double x_min, x_max, y_min, y_max;
  bool ok{false};
};
static bool is_finite_d(double v){ return std::isfinite(v); }

static PlotGeom compute_geom_for_plot(const std::shared_ptr<iinuji_object_t>& plot) {
  PlotGeom g{};
  auto pb = std::dynamic_pointer_cast<plotBox_data_t>(plot->data);
  if (!pb) return g;
  g.content = content_rect_for(plot);
  const auto& opt = pb->opts;
  g.plot_x0 = g.content.x + opt.margin_left;
  g.plot_y0 = g.content.y + opt.margin_top;
  g.plot_w  = std::max(0, g.content.w - (opt.margin_left + opt.margin_right));
  g.plot_h  = std::max(0, g.content.h - (opt.margin_top  + opt.margin_bot ));
  if (g.plot_w<=0 || g.plot_h<=0) return g;

  double x_min = pb->opts.x_min, x_max = pb->opts.x_max;
  double y_min = pb->opts.y_min, y_max = pb->opts.y_max;
  bool ax0=!is_finite_d(x_min), ax1=!is_finite_d(x_max), ay0=!is_finite_d(y_min), ay1=!is_finite_d(y_max);

  auto acc = [&](double x,double y){
    if (!is_finite_d(x)||!is_finite_d(y)) return;
    if (ax0) x_min = !is_finite_d(x_min)? x : std::min(x_min,x);
    if (ax1) x_max = !is_finite_d(x_max)? x : std::max(x_max,x);
    if (ay0) y_min = !is_finite_d(y_min)? y : std::min(y_min,y);
    if (ay1) y_max = !is_finite_d(y_max)? y : std::max(y_max,y);
  };
  for (auto& s : pb->series) for (auto& p : s) acc(p.first, p.second);

  if (!is_finite_d(x_min) || !is_finite_d(x_max) || x_max==x_min) { x_min=0; x_max=1; }
  if (!is_finite_d(y_min) || !is_finite_d(y_max) || y_max==y_min) { y_min=0; y_max=1; }

  g.x_min=x_min; g.x_max=x_max; g.y_min=y_min; g.y_max=y_max; g.ok=true;
  return g;
}
static inline double px_to_x(int px, const PlotGeom& g) {
  int XW = g.plot_w * 2 - 1; if (XW<=0) return g.x_min;
  double t = std::clamp((double)px / (double)XW, 0.0, 1.0);
  return g.x_min + t*(g.x_max - g.x_min);
}
static std::optional<int> screen_to_plot_px(int sx, int sy, const PlotGeom& g) {
  if (sx < g.plot_x0 || sx >= g.plot_x0 + g.plot_w) return std::nullopt;
  if (sy < g.plot_y0 || sy >= g.plot_y0 + g.plot_h) return std::nullopt;
  int local_c = sx - g.plot_x0;
  return local_c * 2; // left subcolumn of the braille cell
}

/* Full x-range of a plot's data */
static std::pair<double,double> full_x_range(const std::shared_ptr<iinuji_object_t>& plot) {
  auto pb = std::dynamic_pointer_cast<plotBox_data_t>(plot->data);
  if (!pb || pb->series.empty()) return {0.0, 1.0};
  double xmin = +1e300, xmax = -1e300;
  for (auto& S : pb->series)
    for (auto& p : S) { xmin = std::min(xmin, p.first); xmax = std::max(xmax, p.first); }
  if (!(xmax > xmin)) return {0.0, 1.0};
  return {xmin, xmax};
}

/* Build a zoomed series around an x window */
static void build_zoom_series(const plotBox_data_t& src,
                              double x_center, double window_frac,
                              std::vector<std::vector<std::pair<double,double>>>& out_series)
{
  out_series.clear();
  if (src.series.empty()) return;

  double xmin=+1e300,xmax=-1e300;
  for (auto& S:src.series) for (auto& p:S){ xmin=std::min(xmin,p.first); xmax=std::max(xmax,p.first); }
  if (!(xmax>xmin)) { xmin=0; xmax=1; }

  double span = xmax - xmin;
  double half = 0.5 * std::clamp(window_frac, 0.001, 1.0) * span;
  double x0 = x_center - half, x1 = x_center + half;

  for (auto& S : src.series) {
    std::vector<std::pair<double,double>> z;
    for (auto& p : S) if (p.first >= x0 && p.first <= x1) z.push_back(p);
    if (z.size() < 2 && !S.empty()) { z.push_back(S.front()); z.push_back(S.back()); }
    out_series.push_back(std::move(z));
  }
}

/* ---------- Main ---------- */
int main() {
  NcursesRend rend;
  set_renderer(&rend);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
  nodelay(stdscr, TRUE);
  mmask_t supported = mousemask(ALL_MOUSE_EVENTS, nullptr); // what the terminal actually gives us
  mouseinterval(0);

  if (has_colors()) { start_color(); use_default_colors(); }
  set_global_background("#101014");

  // Root layout: 2 rows header + 1 row content; content: 2 columns (main, zoom)
  auto root = create_grid_container("root",
    { len_spec::px(3), len_spec::px(2), len_spec::frac(1.0) },
    { len_spec::frac(0.65), len_spec::frac(0.35) }, 0, 0,
    iinuji_layout_t{ layout_mode_t::Normalized, 0,0,1,1,true },
    iinuji_style_t{"#C8C8C8", "#101014", false, "#6C6C75"}
  );
  auto st = initialize_iinuji_state(root, true);
  st->register_id("root", root);

  auto title = create_text_box("title",
    "Mouse zoom — Click recenter | Wheel zoom | Shift+Wheel or MiddleClick to PAN | [-]/[+] zoom | [q] quit",
    true, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{"#E6E6E6", "#202028", true, "#6C6C75", true, false, " iinuji mouse "}
  );
  place_in_grid(title, 0,0,1,2);
  root->add_child(title); st->register_id("title", title);

  char initbuf[128];
  std::snprintf(initbuf, sizeof(initbuf), "Status: mousemask=0x%lx", (unsigned long)supported);
  auto status = create_text_box("status", initbuf, true, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{"#B0B0B8", "#101014", false, "#101014"});
  place_in_grid(status, 1,0,1,2);
  root->add_child(status); st->register_id("status", status);

  // Data
  auto mk = make_market(1200);
  auto ma = moving_avg(mk, 30);

  // Main chart (left)
  plotbox_opts_t mopts;
  mopts.x_label="t"; mopts.y_label="Price";
  mopts.margin_left=10; mopts.margin_bot=2;
  mopts.draw_grid=true; mopts.baseline0=true;

  std::vector<std::vector<std::pair<double,double>>> mk_series = { mk, ma };
  std::vector<plot_series_cfg_t> mk_cfg(2);
  mk_cfg[0].color_fg = "green"; mk_cfg[0].color_pair=-1; mk_cfg[0].mode=plot_mode_t::Line;
  mk_cfg[1].color_fg = "red";   mk_cfg[1].color_pair=-1; mk_cfg[1].mode=plot_mode_t::Line;

  auto mainPlot = create_plot_box("main", mk_series, mk_cfg, mopts,
    iinuji_layout_t{}, iinuji_style_t{ "#C8C8C8", "#101014", true, "#6C6C75", false, false, " Market " });
  place_in_grid(mainPlot, 2,0);
  root->add_child(mainPlot); st->register_id("main", mainPlot);

  // Zoom chart (right)
  plotbox_opts_t zopts = mopts;
  zopts.x_label="t (zoom)"; zopts.y_label="Price";
  std::vector<std::vector<std::pair<double,double>>> zoom_series = mk_series; // replaced by clicks/wheel
  std::vector<plot_series_cfg_t> zcfg = mk_cfg;

  auto zoomPlot = create_plot_box("zoom", zoom_series, zcfg, zopts,
    iinuji_layout_t{}, iinuji_style_t{ "#C8C8C8", "#101014", true, "#6C6C75", false, false, " Zoom " });
  place_in_grid(zoomPlot, 2,1);
  root->add_child(zoomPlot); st->register_id("zoom", zoomPlot);

  // Zoom state
  double window_frac   = 0.12; // fraction of full x-span
  double zoom_center_x = 0.0;  // current zoom center
  bool   pan_mode      = false; // middle click toggles pan vs zoom on wheel

  // Utility: update zoom based on target x
  auto update_zoom = [&](double x_center){
    auto src = std::dynamic_pointer_cast<plotBox_data_t>(mainPlot->data);
    auto dst = std::dynamic_pointer_cast<plotBox_data_t>(zoomPlot->data);
    build_zoom_series(*src, x_center, window_frac, dst->series);
  };

  // Initial zoom centered
  {
    auto [xmin, xmax] = full_x_range(mainPlot);
    zoom_center_x = 0.5 * (xmin + xmax);
    update_zoom(zoom_center_x);
  }

  // Keys: zoom width
  root->on(event_type::Key, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t& ev){
    if (ev.key=='q') { endwin(); std::exit(0); }
    if (ev.key=='-') { window_frac = std::clamp(window_frac*0.9, 0.01, 1.0); update_zoom(zoom_center_x); }
    if (ev.key=='+') { window_frac = std::clamp(window_frac/0.9,  0.01, 1.0); update_zoom(zoom_center_x); }
  });

  // Click on main -> recenter zoom to that x
  mainPlot->on(event_type::MouseDown, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t& ev){
    if (ev.button != 1) return;
    auto geom = compute_geom_for_plot(mainPlot);
    if (!geom.ok) return;
    auto px = screen_to_plot_px(ev.x, ev.y, geom);
    if (!px) return;
    zoom_center_x = px_to_x(*px, geom);
    update_zoom(zoom_center_x);

    auto dst = std::dynamic_pointer_cast<textBox_data_t>(status->data);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
      "Status: click (x=%d,y=%d) | center≈%.3f | window_frac=%.3f | pan_mode=%s | mask=0x%lx",
      ev.x, ev.y, zoom_center_x, window_frac, pan_mode?"ON":"OFF", (unsigned long)supported
    );
    dst->content = buf;
  });

  // Middle click toggles pan mode
  mainPlot->on(event_type::MouseDown, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t& ev){
    if (ev.button != 2) return;
    pan_mode = !pan_mode;
    auto dst = std::dynamic_pointer_cast<textBox_data_t>(status->data);
    dst->content = std::string("Status: pan_mode = ") + (pan_mode ? "ON" : "OFF");
  });

  // Wheel over main: Shift OR pan_mode => pan; else => zoom width
  mainPlot->on(event_type::MouseDown, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t& ev){
    if (ev.button != 4 && ev.button != 5) return;

    bool shifted = (ev.name == "shift"); // set by dispatcher when modifier detected
    bool do_pan  = pan_mode || shifted;

    auto [xmin, xmax] = full_x_range(mainPlot);
    double span = std::max(1e-12, xmax - xmin);

    if (do_pan) {
      // Pan step proportional to current window width (20%)
      double step = window_frac * span * 0.20;
      if (ev.button == 4) zoom_center_x -= step;  // up => left
      if (ev.button == 5) zoom_center_x += step;  // down=> right
      zoom_center_x = std::clamp(zoom_center_x, xmin, xmax);
    } else {
      if (ev.button == 4) window_frac = std::clamp(window_frac*0.9, 0.01, 1.0);
      if (ev.button == 5) window_frac = std::clamp(window_frac/0.9,  0.01, 1.0);
    }

    update_zoom(zoom_center_x);

    auto dst = std::dynamic_pointer_cast<textBox_data_t>(status->data);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
      "Status: %s (wheel=%d) | center≈%.3f | window_frac=%.3f | pan_mode=%s",
      do_pan ? "pan" : "zoom", ev.button, zoom_center_x, window_frac, pan_mode?"ON":"OFF"
    );
    dst->content = buf;
  });

  // Raw mouse dispatcher: clicks + wheel only (no hover/drag)
  auto deliver_mouse = [&](MEVENT& me){
    // Detect Shift modifier (if terminal reports it)
    bool shift = (me.bstate & BUTTON_SHIFT);

    event_t ev{};
    if (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) { ev.type=event_type::MouseDown; ev.button=1; }
    else if (me.bstate & (BUTTON2_PRESSED | BUTTON2_CLICKED)) { ev.type=event_type::MouseDown; ev.button=2; }
    else if (me.bstate & BUTTON1_RELEASED)               { ev.type=event_type::MouseUp;   ev.button=1; }
    else if (me.bstate & BUTTON4_PRESSED)                { ev.type=event_type::MouseDown; ev.button=4; }
    else if (me.bstate & BUTTON5_PRESSED)                { ev.type=event_type::MouseDown; ev.button=5; }
    else                                                 { return; }

    ev.x = me.x; ev.y = me.y;
    if ((ev.button == 4 || ev.button == 5) && shift) ev.name = "shift"; // tag Shift+Wheel when visible

    auto target = pick_topmost(root, ev.x, ev.y);
    if (target) {
      auto it = target->listeners.find(ev.type);
      if (it != target->listeners.end())
        for (auto& fn : it->second) fn(*st, target, ev);
    }
  };

  // ---- Main loop ----
  timeout(30);
  while (true) {
    int H,W; getmaxyx(stdscr, H, W);
    layout_tree(root, Rect{0,0,W,H});

    clear();
    mvhline(0,   0, ACS_HLINE, W);
    mvhline(H-1, 0, ACS_HLINE, W);

    render_tree(root);
    refresh();

    int ch = getch();
    if (ch == ERR) continue;
    if (ch == KEY_RESIZE) continue;
    if (ch == KEY_MOUSE) {
      MEVENT me; if (getmouse(&me) == OK) deliver_mouse(me);
      continue;
    }

    // keys
    event_t kev{}; kev.type=event_type::Key; kev.key=ch;
    auto it = root->listeners.find(event_type::Key);
    if (it != root->listeners.end())
      for (auto& fn : it->second) fn(*st, root, kev);
  }

  endwin();
  return 0;
}
