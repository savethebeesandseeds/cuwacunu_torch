// test_iinuji_suite.cpp
#include <locale.h>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <chrono>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/primitives/plot.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

/* ---------- Data builders ---------- */
static std::vector<std::pair<double,double>> make_market(int N=1500) {
  std::vector<std::pair<double,double>> pts; pts.reserve(N);
  const double dt=1.0/252.0, mu=0.08, sigma=0.22, S0=100.0;
  const double jump_prob=0.02, jump_sigma=0.08;
  std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::normal_distribution<double> z(0.0,1.0);
  std::bernoulli_distribution J(jump_prob);
  std::normal_distribution<double> Jsize(0.0, jump_sigma);
  double S=S0;
  for (int i=0;i<N;++i) {
    double dlogS=(mu-0.5*sigma*sigma)*dt + sigma*std::sqrt(dt)*z(rng);
    if (J(rng)) dlogS += Jsize(rng);
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
static std::vector<double> sample_mixture(int n) {
  std::mt19937 rng(42);
  std::normal_distribution<double> g1(-1.2,0.7), g2(1.4,0.4);
  std::vector<double> v; v.reserve(n);
  for (int i=0;i<n/2;++i) v.push_back(g1(rng));
  for (int i=0;i<n/2;++i) v.push_back(g2(rng));
  return v;
}
static std::vector<std::pair<double,double>> make_histogram(const std::vector<double>& x, int bins) {
  if (x.empty()) return {};
  double mn=*std::min_element(x.begin(),x.end());
  double mx=*std::max_element(x.begin(),x.end());
  if (mx<=mn) mx=mn+1.0;
  std::vector<int> H(bins,0);
  for (double v : x) {
    int b=(int)std::floor((v-mn)/(mx-mn)*bins);
    if (b < 0)        b = 0;
    if (b >= bins)    b = bins - 1;
    H[b]++;
  }
  std::vector<std::pair<double,double>> pts; pts.reserve(bins);
  for (int i=0;i<bins;++i) {
    double cx = mn + (i+0.5)*(mx-mn)/bins;
    pts.emplace_back(cx, (double)H[i]);
  }
  return pts;
}
static std::vector<std::pair<double,double>> narrow_spike_signal() {
  std::vector<std::pair<double,double>> pts; pts.reserve(1000);
  for (int i=0;i<1000;++i) pts.emplace_back(i, 0.7*std::sin(2*M_PI*i/200.0));
  pts.emplace_back(250, 7.5); pts.emplace_back(251, 0.0);
  pts.emplace_back(700, 6.4); pts.emplace_back(701, 0.0);
  return pts;
}

/* ---------- Main ---------- */
using namespace cuwacunu::iinuji;

int main() {
  /* initialize the renderer */
  cuwacunu::iinuji::NcursesRend rend;
  cuwacunu::iinuji::set_renderer(&rend);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
  nodelay(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

  if (has_colors()) {
    start_color();
    use_default_colors();
  }
  set_global_background("#101014");

  // Root grid: rows = [title 3px][status 2px][center frac][outlier frac]
  double outlier_frac = 0.35; // <- tweak this or use [ / ] in demo
  auto root = create_grid_container("root",
    { len_spec::px(3), len_spec::px(2), len_spec::frac(1.0 - outlier_frac), len_spec::frac(outlier_frac) },
    { len_spec::frac(1.0) }, 0, 0,
    iinuji_layout_t{ layout_mode_t::Normalized, 0,0,1,1,true }, // full screen
    iinuji_style_t{"#C8C8C8", "#101014", false, "#6C6C75"}
  );

  auto st = initialize_iinuji_state(root, true);
  st->register_id("root", root);

  // Title
  auto title = create_text_box("title",
    "iinuji demo — Keys: [q] quit | [g] grid | [b] baseline (hist+outlier) | [m] overlay (outlier) | [l] logY (hist) | [r] regen | [t] theme | [ / ] outlier size",
    true, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{"#E6E6E6", "#202028", true, "#6C6C75", true, false, " iinuji "});
  place_in_grid(title, 0,0);
  root->add_child(title); st->register_id("title", title);

  // Status
  auto status = create_text_box("status", "Status: ready.", true, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{"#B0B0B8", "#101014", false, "#101014"});
  place_in_grid(status, 1,0);
  root->add_child(status); st->register_id("status", status);

  // Center container: 1 row x 2 columns (market/histogram)
  auto center = create_grid_container("center",
    { len_spec::frac(1.0) }, { len_spec::frac(0.65), len_spec::frac(0.35) }, 0, 0,
    iinuji_layout_t{}, iinuji_style_t{"#C8C8C8", "#101014", false, "#6C6C75"});
  place_in_grid(center, 2,0);
  root->add_child(center); st->register_id("center", center);

  // Data
  auto mk = make_market(1500);
  auto ma = moving_avg(mk, 30);
  auto smp = sample_mixture(4000);
  auto hist = make_histogram(smp, 40);
  auto spikes = narrow_spike_signal();

  // ── Market (GBM + MA) ───────────────────────────────────────────────────────
  plotbox_opts_t mopts;
  mopts.x_label = "t"; mopts.y_label = "Price";
  mopts.margin_left = 10; mopts.margin_bot = 2;
  mopts.draw_grid = true; mopts.baseline0 = true;

  std::vector<std::vector<std::pair<double,double>>> mk_series = { mk, ma };
  std::vector<plot_series_cfg_t> mk_cfg(2);
  // price
  mk_cfg[0].color_fg = "#3b633b";
  mk_cfg[0].color_pair = -1;
  mk_cfg[0].mode = plot_mode_t::Line;
  mk_cfg[0].scatter = false;
  mk_cfg[0].scatter_every = 1;
  mk_cfg[0].fill_vertical_if_same_x = true;
  mk_cfg[0].stem_y = std::numeric_limits<double>::quiet_NaN();
  mk_cfg[0].envelope_enabled = false;
  // moving average
  mk_cfg[1].color_fg = "#7a4747";
  mk_cfg[1].color_pair = -1;
  mk_cfg[1].mode = plot_mode_t::Line;
  mk_cfg[1].scatter = false;
  mk_cfg[1].scatter_every = 1;
  mk_cfg[1].fill_vertical_if_same_x = true;
  mk_cfg[1].stem_y = std::numeric_limits<double>::quiet_NaN();
  mk_cfg[1].envelope_enabled = false;

  auto market = create_plot_box(
    "market", mk_series, mk_cfg, mopts,
    iinuji_layout_t{}, iinuji_style_t{ "#C8C8C8", "#101014", true, "#6C6C75", false, false, " Market (GBM+Jumps) " }
  );
  place_in_grid(market, 0, 0);
  center->add_child(market);
  st->register_id("market", market);

  // ── Histogram (stems) ───────────────────────────────────────────────────────
  plotbox_opts_t hopts;
  hopts.x_label = "value"; hopts.y_label = "count";
  hopts.margin_left = 10; hopts.margin_bot = 2;
  hopts.draw_grid = true; hopts.baseline0 = true;

  std::vector<std::vector<std::pair<double,double>>> h_series = { hist };
  std::vector<plot_series_cfg_t> h_cfg(1);
  h_cfg[0].color_fg = "#8AC926";
  h_cfg[0].color_pair = -1;
  h_cfg[0].mode = plot_mode_t::Stem;
  h_cfg[0].scatter = false;
  h_cfg[0].scatter_every = 1;
  h_cfg[0].fill_vertical_if_same_x = true;
  h_cfg[0].stem_y = 0.0;
  h_cfg[0].envelope_enabled = false;

  auto histp = create_plot_box(
    "hist", h_series, h_cfg, hopts,
    iinuji_layout_t{}, iinuji_style_t{ "#C8C8C8", "#101014", true, "#6C6C75", false, false, " Histogram (stems) " }
  );
  place_in_grid(histp, 0, 1);
  center->add_child(histp);
  st->register_id("hist", histp);

  // ── Outlier preservation (Line + Envelope Overlay) ─────────────────────────
  plotbox_opts_t sopts;
  sopts.x_label = "t"; sopts.y_label = "signal";
  sopts.margin_left = 10; sopts.margin_bot = 2;
  sopts.draw_grid = true; sopts.baseline0 = true;

  std::vector<std::vector<std::pair<double,double>>> s_series = { spikes };
  std::vector<plot_series_cfg_t> s_cfg(1);
  s_cfg[0].color_fg = "#00D1FF";
  s_cfg[0].color_pair = -1;
  s_cfg[0].mode = plot_mode_t::Line;
  s_cfg[0].scatter = false;
  s_cfg[0].scatter_every = 1;
  s_cfg[0].fill_vertical_if_same_x = false; // emphasize overlay effect
  s_cfg[0].stem_y = std::numeric_limits<double>::quiet_NaN();
  s_cfg[0].envelope_enabled    = true;  // overlay ON by default
  s_cfg[0].envelope_source     = envelope_source_t::OriginalSamples;
  s_cfg[0].envelope_min_count  = 2;
  s_cfg[0].envelope_min_height = 3;
  s_cfg[0].envelope_draw_base  = true;

  auto outlier = create_plot_box(
    "outlier", s_series, s_cfg, sopts,
    iinuji_layout_t{}, iinuji_style_t{ "#C8C8C8", "#101014", true, "#6C6C75", false, false, " Outlier preservation " }
  );
  place_in_grid(outlier, 3, 0);
  root->add_child(outlier);
  st->register_id("outlier", outlier);

  // Theme toggle
  bool dark = true;

  // ---- Event listeners (keys on root) ----
  root->on(event_type::Key, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t& ev){
    if (ev.key=='q') { endwin(); std::exit(0); }

    if (ev.key=='t') {
      dark = !dark;
      auto bg = dark ? "#101014" : "#F0F0F2";
      set_global_background(bg);
      for (auto id : {"title","status","market","hist","outlier","center","root"}) {
        auto o = st->by_id(id); if (!o) continue; o->style.background_color = bg;
      }
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        std::string("Status: theme = ") + (dark ? "dark" : "light");
    }

    if (ev.key=='g') {
      auto mp = std::dynamic_pointer_cast<plotBox_data_t>(market->data);
      auto hp = std::dynamic_pointer_cast<plotBox_data_t>(histp->data);
      auto sp = std::dynamic_pointer_cast<plotBox_data_t>(outlier->data);
      mp->opts.draw_grid ^= 1; hp->opts.draw_grid ^= 1; sp->opts.draw_grid ^= 1;
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        std::string("Status: grid = ") + (mp->opts.draw_grid ? "on" : "off");
    }

    if (ev.key=='b') {
      auto hp = std::dynamic_pointer_cast<plotBox_data_t>(histp->data);
      auto sp = std::dynamic_pointer_cast<plotBox_data_t>(outlier->data);
      hp->opts.baseline0 ^= 1; sp->opts.baseline0 ^= 1;
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        std::string("Status: baseline0 (hist+outlier) = ") + (hp->opts.baseline0 ? "on" : "off");
    }

    if (ev.key=='m') {
      auto sp = std::dynamic_pointer_cast<plotBox_data_t>(outlier->data);
      auto& cfg = sp->series_cfg[0];
      cfg.envelope_enabled = !cfg.envelope_enabled; // toggle overlay
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        std::string("Status: outlier overlay = ") + (cfg.envelope_enabled ? "on" : "off");
    }

    if (ev.key=='l') {
      auto hp = std::dynamic_pointer_cast<plotBox_data_t>(histp->data);
      hp->opts.y_log ^= 1;
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        std::string("Status: hist logY = ") + (hp->opts.y_log ? "on" : "off");
    }

    if (ev.key=='r') {
      static int k=0; k=(k+1)%3; int win = (k==0?15:(k==1?30:60));
      auto mp = std::dynamic_pointer_cast<plotBox_data_t>(market->data);
      auto mk2 = make_market(1500);
      mp->series[0] = mk2; mp->series[1] = moving_avg(mk2, win);

      auto hp = std::dynamic_pointer_cast<plotBox_data_t>(histp->data);
      auto smp2 = sample_mixture(4000);
      hp->series[0] = make_histogram(smp2, 40);

      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        "Status: regenerated data, MA window=" + std::to_string(win);
    }

    if (ev.key=='[' || ev.key==']') {
      // Adjust outlier row fraction in root grid
      auto g = root->grid;
      // rows: [0]=3px, [1]=2px, [2]=center frac, [3]=outlier frac
      auto& r2 = g->rows[2];
      auto& r3 = g->rows[3];
      double total = (r2.u==unit_t::Frac ? r2.v : 0) + (r3.u==unit_t::Frac ? r3.v : 0);
      if (total<=0) total = 1.0;
      double delta = (ev.key==']' ? +0.05 : -0.05);
      r3.v = std::clamp(r3.v + delta, 0.10, 0.90);
      r2.v = std::max(0.05, total - r3.v);
      outlier_frac = r3.v;
      std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
        "Status: outlier frac = " + std::to_string(outlier_frac);
    }
  });

  // Mouse: click on outlier toggles overlay
  outlier->on(event_type::MouseDown, [&](iinuji_state_t&, std::shared_ptr<iinuji_object_t>&, const event_t&){
    auto sp = std::dynamic_pointer_cast<plotBox_data_t>(outlier->data);
    auto& cfg = sp->series_cfg[0];
    cfg.envelope_enabled = !cfg.envelope_enabled;
    std::dynamic_pointer_cast<textBox_data_t>(status->data)->content =
      std::string("Status: outlier overlay = ") + (cfg.envelope_enabled ? "on" : "off") + " (mouse)";
  });

  // ---- Main loop ----
  auto last_tick = std::chrono::steady_clock::now();
  timeout(30);

  while (true) {
    // Layout (full screen)
    int H,W; getmaxyx(stdscr, H, W);
    layout_tree(root, Rect{0,0,W,H});

    clear();
    // subtle top/bot lines
    mvhline(0,   0, ACS_HLINE, W);
    mvhline(H-1, 0, ACS_HLINE, W);

    render_tree(root);
    refresh();

    // Timer event every ~0.5s
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count() > 500) {
      last_tick = now;
      // Example timer: you could update clocks/animations here.
    }

    int ch = getch();
    if (ch == ERR) continue;
    if (ch == KEY_RESIZE) {
      // implicit; we relayout each frame
      continue;
    }
    if (ch == KEY_MOUSE) {
      MEVENT me;
      if (getmouse(&me) == OK) {
        event_t ev{};
        if (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) { ev.type=event_type::MouseDown; ev.button=1; }
        else if (me.bstate & BUTTON1_RELEASED) { ev.type=event_type::MouseUp; ev.button=1; }
        else { ev.type=event_type::MouseMove; }
        ev.x = me.x; ev.y = me.y;
        auto target = pick_topmost(root, ev.x, ev.y);
        if (target) {
          auto it = target->listeners.find(ev.type);
          if (it != target->listeners.end())
            for (auto& fn : it->second) fn(*st, target, ev);
        }
      }
      continue;
    }
    // Key -> deliver to root (simplest)
    event_t kev{}; kev.type=event_type::Key; kev.key=ch;
    auto it = root->listeners.find(event_type::Key);
    if (it != root->listeners.end())
      for (auto& fn : it->second) fn(*st, root, kev);
  }

  endwin();
  return 0;
}
