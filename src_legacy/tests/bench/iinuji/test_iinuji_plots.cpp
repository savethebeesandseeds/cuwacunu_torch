// test_iinuji_plots.cpp
#include <locale.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <functional>
#include <string>
#include <limits>
#include <cstring>
#include "iinuji/primitives/plot.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

using cuwacunu::iinuji::PlotOptions;
using cuwacunu::iinuji::Series;
using cuwacunu::iinuji::SeriesStyle;
using cuwacunu::iinuji::PlotMode;

/* ----------------------------- Data builders ------------------------------ */
std::vector<std::pair<double,double>> make_points_market(int points_hint) {
  std::vector<std::pair<double,double>> pts;
  const int N = std::max(800, points_hint);
  const double dt = 1.0 / 252.0;
  const double mu = 0.08;
  const double sigma = 0.22;
  const double S0 = 100.0;

  const double jump_prob  = 0.02;
  const double jump_sigma = 0.08;

  std::mt19937 rng(1234567);
  std::normal_distribution<double> z(0.0, 1.0);
  std::bernoulli_distribution J(jump_prob);
  std::normal_distribution<double> Jsize(0.0, jump_sigma);

  double S = S0;
  pts.reserve(N);
  for (int i = 0; i < N; ++i) {
    double dlogS = (mu - 0.5*sigma*sigma)*dt + sigma*std::sqrt(dt)*z(rng);
    if (J(rng)) dlogS += Jsize(rng);
    S *= std::exp(dlogS);
    pts.emplace_back((double)i, S);
  }
  return pts;
}

std::vector<std::pair<double,double>> make_points_sine(double a=1.0, double f=1.0, double phase=0.0) {
  std::vector<std::pair<double,double>> pts;
  for (double x = 0; x <= 2 * M_PI; x += 0.02) {
    pts.emplace_back(x, a * std::sin(f*x + phase));
  }
  return pts;
}

std::vector<std::pair<double,double>> make_points_cos(double a=1.0, double f=1.0, double phase=0.0) {
  std::vector<std::pair<double,double>> pts;
  for (double x = 0; x <= 2 * M_PI; x += 0.02) {
    pts.emplace_back(x, a * std::cos(f*x + phase));
  }
  return pts;
}

std::vector<std::pair<double,double>> make_noisy_sine(double noise=0.2) {
  std::vector<std::pair<double,double>> pts;
  std::mt19937 rng(42);
  std::normal_distribution<double> n(0.0, noise);
  for (double x = 0; x <= 4 * M_PI; x += 0.01) {
    pts.emplace_back(x, std::sin(x) + n(rng));
  }
  return pts;
}

std::vector<std::pair<double,double>> make_piecewise_with_gaps() {
  // Two segments with a NaN gap in between
  std::vector<std::pair<double,double>> pts;
  for (double x=0; x<=2.0; x+=0.01) pts.emplace_back(x, std::sin(3*x));
  // gap
  pts.emplace_back(std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN());
  for (double x=2.5; x<=4.0; x+=0.01) pts.emplace_back(x, 0.5*std::cos(4*x));
  return pts;
}

// Tall, narrow spikes embedded in a smooth signal.
// Multiple points fall into the same X sub-pixel column; envelope overlay preserves the spike.
std::vector<std::pair<double,double>> make_signal_with_narrow_spikes() {
  std::vector<std::pair<double,double>> pts;
  std::mt19937 rng(7);
  std::uniform_int_distribution<int> kpos(50, 950);
  std::uniform_real_distribution<double> amp(4.0, 8.0);

  // Base line (many samples)
  const int N = 1000;
  pts.reserve(N + 100);
  for (int i = 0; i < N; ++i) {
    double x = (double)i;
    double y = 0.7 * std::sin(2*M_PI * x / 200.0);
    pts.emplace_back(x, y);
  }

  // Insert 6 very narrow spikes (cluster several x values around same integer)
  for (int s = 0; s < 6; ++s) {
    int center = kpos(rng);
    double A = amp(rng);
    for (int dx = -1; dx <= 1; ++dx) {
      // cluster points into same X column after rounding when width is small
      pts.emplace_back(center + 0.02*dx, A - 0.3*std::abs(dx));
    }
    // quick return to baseline
    pts.emplace_back(center + 0.08, 0.0);
  }
  std::sort(pts.begin(), pts.end(), [](auto& a, auto& b){ return a.first < b.first; });
  return pts;
}

// Simple histogram helper: return (bin_center, count)
std::vector<std::pair<double,double>> make_histogram(const std::vector<double>& samples, int bins) {
  if (samples.empty()) return {};
  double mn = *std::min_element(samples.begin(), samples.end());
  double mx = *std::max_element(samples.begin(), samples.end());
  if (mx <= mn) mx = mn + 1.0;
  std::vector<int> H(bins, 0);
  for (double v : samples) {
    int b = (int)std::floor((v - mn) / (mx - mn) * bins);
    if (b < 0) b = 0;
    if (b >= bins) b = bins-1;
    H[b]++;
  }
  std::vector<std::pair<double,double>> pts;
  pts.reserve(bins);
  for (int i = 0; i < bins; ++i) {
    double cx = mn + (i + 0.5) * (mx - mn) / bins;
    pts.emplace_back(cx, (double)H[i]);
  }
  return pts;
}

/* ----------------------------- UI helpers --------------------------------- */
void draw_caption(int screen_h, int screen_w, const std::string& title,
          const std::string& subtitle, int page, int total) {
  std::string head = " " + title + " ";
  mvaddnstr(0, 1, head.c_str(), (int)head.size());
  char buf[128]; std::snprintf(buf, sizeof(buf), "[%d/%d]  (press any key; q to quit)", page, total);
  int x = std::max(1, screen_w - (int)std::strlen(buf) - 2);
  mvaddnstr(0, x, buf, (int)std::strlen(buf));
  mvaddnstr(screen_h-1, 1, subtitle.c_str(), (int)subtitle.size());
}

void draw_legend(int y, int x, const std::vector<std::pair<std::string,short>>& items, int max_w=80) {
  int cx = x;
  for (auto& it : items) {
    const std::string& name = it.first;
    short cp = it.second;
    if (cp>0) attron(COLOR_PAIR(cp));
    mvaddch(y, cx++, ACS_DIAMOND);
    if (cp>0) attroff(COLOR_PAIR(cp));
    mvaddch(y, cx++, ' ');
    mvaddnstr(y, cx, name.c_str(), std::min<int>((int)name.size(), max_w - (cx-x)));
    cx += (int)name.size() + 2;
    if (cx >= x + max_w) break;
  }
}

/* ----------------------------- Demos (existing) --------------------------- */
void demo_basic_sine(int W, int H) {
  auto s1 = make_points_sine();
  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "sin(x)";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 8;
  opt.margin_bot = 2;

  SeriesStyle st;
  st.color_pair = 1;
  st.mode = PlotMode::Line;

  Series a{&s1, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Basic line", "Grid + axes + baseline", 1, 11);
  draw_legend(1, 2, {{"sin(x)", 1}});
}

void demo_multi_series(int W, int H) {
  auto s1 = make_points_sine(1.0, 1.0, 0.0);
  auto s2 = make_points_cos(0.6, 2.0, 0.0);

  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "f(x)";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 8;
  opt.margin_bot = 2;
  opt.y_ticks = 6;
  opt.x_ticks = 7;

  SeriesStyle st1; st1.color_pair = 1; st1.mode = PlotMode::Line;
  SeriesStyle st2; st2.color_pair = 2; st2.mode = PlotMode::Scatter; st2.scatter = true; st2.scatter_every = 2;

  Series a{&s1, st1};
  Series b{&s2, st2};

  cuwacunu::iinuji::plot_braille_multi({a,b}, 0, 0, W, H, opt);

  draw_caption(H, W, "Multi series", "Line + scatter; Bresenham lines", 2, 11);
  draw_legend(1, 2, {{"sin(x)", 1}, {"0.6*cos(2x)", 2}});
}

void demo_market_autoscale(int W, int H) {
  auto mkt = make_points_market(W*2);

  PlotOptions opt;
  opt.x_label = "t (days)";
  opt.y_label = "Price";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 10;
  opt.margin_bot = 2;

  SeriesStyle st; st.color_pair = 3; st.mode = PlotMode::Line;

  Series a{&mkt, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Market path (autoscale)", "GBM with jumps", 3, 11);
  draw_legend(1, 2, {{"price", 3}});
}

void demo_fixed_ranges(int W, int H) {
  auto s = make_points_sine(1.0, 1.0);

  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "f(x)";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 8;
  opt.margin_bot = 2;
  opt.x_min = 0.0;        // fixed window
  opt.x_max = 2*M_PI;
  opt.y_min = -2.0;
  opt.y_max = 2.0;

  SeriesStyle st; st.color_pair = 4; st.mode = PlotMode::Line;

  Series a{&s, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Fixed ranges", "x:[0,2π], y:[-2,2]", 4, 11);
  draw_legend(1, 2, {{"sin(x)", 4}});
}

void demo_nan_gaps(int W, int H) {
  auto g = make_piecewise_with_gaps();

  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "piecewise";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 10;
  opt.margin_bot = 2;

  SeriesStyle st; st.color_pair = 5; st.mode = PlotMode::Line;

  Series a{&g, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "NaN gaps", "Non-finite breaks the line", 5, 11);
  draw_legend(1, 2, {{"piecewise", 5}});
}

void demo_noisy_high_density(int W, int H) {
  auto n = make_noisy_sine(0.25);

  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "sin(x)+noise";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 12;
  opt.margin_bot = 2;

  SeriesStyle st; st.color_pair = 6; st.mode = PlotMode::Line;

  Series a{&n, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Noisy line (Bresenham)", "Crisp steep segments; no jaggies", 6, 11);
  draw_legend(1, 2, {{"sin(x)+noise", 6}});
}

void demo_soft_clip(int W, int H) {
  // A signal that goes out of bounds of the fixed y-range
  std::vector<std::pair<double,double>> s;
  for (double x = -1.0; x <= 7.5; x += 0.01) {
    s.emplace_back(x, 1.5*std::sin(x) + 1.2);
  }

  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "soft-clip demo";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 12;
  opt.margin_bot = 2;
  opt.x_min = 0.0; opt.x_max = 2*M_PI;
  opt.y_min = -0.5; opt.y_max = 1.5;
  opt.hard_clip = false; // allow sampling beyond for nicer edge coverage

  SeriesStyle st; st.color_pair = 7; st.mode = PlotMode::Line;

  Series a{&s, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Hard-clip OFF", "Line drawn even if data exceeds window", 7, 11);
  draw_legend(1, 2, {{"1.5*sin(x)+1.2", 7}});
}

void demo_steps(int W, int H) {
  auto s1 = make_points_sine(1.0, 0.7);
  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "stairs(sin)";
  opt.draw_grid = true; opt.baseline0 = true;
  opt.margin_left = 10; opt.margin_bot = 2;

  SeriesStyle st; st.color_pair = 1; st.mode = PlotMode::Stairs;

  Series a{&s1, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Step plot", "Horizontal then vertical transitions", 8, 11);
  draw_legend(1, 2, {{"stairs", 1}});
}

void demo_stems_as_bars(int W, int H) {
  // Histogram-like bars using stems
  std::mt19937 rng(123);
  std::normal_distribution<double> g1(-1.2, 0.7), g2(1.4, 0.4);
  std::vector<double> samp; samp.reserve(5000);
  for (int i=0;i<2500;++i) samp.push_back(g1(rng));
  for (int i=0;i<2500;++i) samp.push_back(g2(rng));
  auto Hbins = make_histogram(samp, 60);

  PlotOptions opt;
  opt.x_label = "value";
  opt.y_label = "count";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 10; opt.margin_bot = 2;

  SeriesStyle st; st.color_pair = 3; st.mode = PlotMode::Stem; st.stem_y = 0.0;

  Series a{&Hbins, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);
  draw_caption(H, W, "Stems / Bars", "Use PlotMode::Stem for vertical bars", 9, 11);
  draw_legend(1, 2, {{"histogram", 3}});
}

void demo_line_preserve_spikes(int W, int H) {
  auto s = make_signal_with_narrow_spikes();

  PlotOptions opt;
  opt.x_label = "t";
  opt.y_label = "signal";
  opt.draw_grid = true;
  opt.baseline0 = true;
  opt.margin_left = 10; opt.margin_bot = 2;

  SeriesStyle stA;
  stA.color_pair = 4;
  stA.mode = PlotMode::Line;
  stA.fill_vertical_if_same_x = false;  // emphasize difference vs overlay

  SeriesStyle stB;
  stB.color_pair = 2;
  stB.mode = PlotMode::Line;
  // Envelope overlay ON
  stB.envelope_enabled    = true;
  stB.envelope_draw_base  = true;
  stB.envelope_source     = SeriesStyle::EnvelopeSource::OriginalSamples;
  stB.envelope_min_count  = 2;
  stB.envelope_min_height = 3;

  // Top: plain line (no same-X vertical fill)
  cuwacunu::iinuji::plot_braille_multi({Series{&s, stA}}, 0, 0, W, H/2, opt);
  // Bottom: line + per-column min..max envelope
  cuwacunu::iinuji::plot_braille_multi({Series{&s, stB}}, 0, H/2, W, H - H/2, opt);

  draw_caption(H, W, "Outlier preservation",
               "Top: Line (no same-X vertical fill). Bottom: Line+Envelope (per-column min..max).",
               10, 11);
  draw_legend(1, 2, {{"line", 4}, {"minmax overlay", 2}});
}

void demo_log_scale(int W, int H) {
  // Positive-only data for log y
  std::vector<std::pair<double,double>> s;
  for (int i=1;i<=500;++i) {
    double x = i;
    double y = 0.5 + 0.001*i + 0.1*std::fabs(std::sin(0.02*i));
    s.emplace_back(x, y);
  }
  PlotOptions opt;
  opt.x_label = "x";
  opt.y_label = "log10(y)";
  opt.draw_grid = true; opt.baseline0 = false;
  opt.margin_left = 10; opt.margin_bot = 2;
  opt.y_log = true; opt.y_log_eps = 1e-6;

  SeriesStyle st; st.color_pair = 5; st.mode = PlotMode::Line;

  Series a{&s, st};
  cuwacunu::iinuji::plot_braille_multi({a}, 0, 0, W, H, opt);

  draw_caption(H, W, "Log scale (Y)", "y mapped to log10(y+eps)", 11, 11);
  draw_legend(1, 2, {{"series", 5}});
}

/* ----------------------------- Main --------------------------------------- */
int main() {
  /* initialize the renderer */
  cuwacunu::iinuji::NcursesRend rend;
  cuwacunu::iinuji::set_renderer(&rend);

  setlocale(LC_ALL, "");
  initscr();
  curs_set(0);
  keypad(stdscr, TRUE);
  noecho();
  clear();

  bool colors_ok = has_colors();
  if (colors_ok) {
    start_color();
    use_default_colors();
  }
  if (colors_ok) {
    init_pair(1, COLOR_YELLOW,  -1);
    init_pair(2, COLOR_CYAN,    -1);
    init_pair(3, COLOR_GREEN,   -1);
    init_pair(4, COLOR_BLUE,    -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN,    -1);
    init_pair(7, COLOR_WHITE,   -1);
  }

  int H, W;
  getmaxyx(stdscr, H, W);

  using DemoFn = std::function<void(int,int)>;
  std::vector<DemoFn> pages = {
    demo_basic_sine,
    demo_multi_series,
    demo_market_autoscale,
    demo_fixed_ranges,
    demo_nan_gaps,
    demo_noisy_high_density,
    demo_soft_clip,
    demo_steps,
    demo_stems_as_bars,
    demo_line_preserve_spikes,
    demo_log_scale
  };

  int page = 0;
  while (page < (int)pages.size()) {
    clear();
    mvhline(0,   0, ACS_HLINE, W);
    mvhline(H-1, 0, ACS_HLINE, W);
    getmaxyx(stdscr, H, W);
    pages[page](W, H);
    refresh();
    int ch = getch();
    if (ch == 'q' || ch == 'Q') break;
    ++page;
  }

  endwin();
  return 0;
}
