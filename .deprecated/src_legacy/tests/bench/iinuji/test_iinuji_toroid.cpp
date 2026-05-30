// tests/bench/iinuji/test_iinuji_toroid.cpp
#include <clocale>   // setlocale
#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <string>
#include <algorithm>
#include <numeric>

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

using namespace cuwacunu::iinuji;

/* ============================ Math & sampling ============================ */
struct ToroidSettings {
  double R_min = 0.6, R_max = 1.6;
  double r_min = 0.06, r_max = 0.45;
  double eps_max = 0.90;
  double pitch_max = 3.14159265/2; // 90 deg

  double a2_max = 0.22; // of r0
  double b2_max = 0.18; // of R0

  double max_tube_ratio = 0.95; // safety: r_eff / R_eff_min
};

struct TorusParams {
  double R0{1.2}, r0{0.2}, eps{0.2}, pitch{0.7};
  double a2{0.05}; // cross-section scallop (2-lobed)
  double b2{0.04}; // ring ripple (2-lobed)
  // reserved (kept zero for readability):
  double a3{0.0}, b3{0.0};
};

struct Vec3 { double x,y,z; };
static inline Vec3 lerp(const Vec3& a, const Vec3& b, double t){
  return { a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t };
}
static inline void pitch_rotate(Vec3& p, double pitch){
  double cp = std::cos(pitch), sp = std::sin(pitch);
  double y =  cp * p.y - sp * p.z;
  double z =  sp * p.y + cp * p.z;
  p.y = y; p.z = z;
}

// Point on phase-free torus (no yaw/roll; only pitch)
static inline Vec3 torus_point(const TorusParams& P, double u, double v){
  const double cu = std::cos(u),  su = std::sin(u);
  const double cv = std::cos(v),  sv = std::sin(v);

  // World-frame basis
  const Vec3 er{cu, su, 0.0};
  const Vec3 ez{0.0, 0.0, 1.0};

  // Ring radius with zero-phase harmonics (only b2)
  double Ru = P.R0 * (1.0 + P.b2*std::cos(2*u) /* + P.b3*std::cos(3*u) */);

  // Elliptical tube
  const double r1 = P.r0*(1.0 + P.eps);
  const double r2 = P.r0*(1.0 - P.eps);

  // Base ellipse in local plane
  double cx = r1 * cv; // along e_r
  double cz = r2 * sv; // along e_z

  // Outward direction (normalized in that plane)
  double denom = std::sqrt((r1*cv)*(r1*cv) + (r2*sv)*(r2*sv)) + 1e-12;
  double nx = (r1*cv) / denom;
  double nz = (r2*sv) / denom;

  // Cross-section scallop (only a2)
  double dr = P.r0 * (P.a2*std::cos(2*v) /* + P.a3*std::cos(3*v) */);

  // Canonical (no yaw/roll)
  Vec3 p {
    Ru*er.x + (cx + dr*nx)*er.x + (cz + dr*nz)*ez.x,
    Ru*er.y + (cx + dr*nx)*er.y + (cz + dr*nz)*ez.y,
    Ru*er.z + (cx + dr*nx)*er.z + (cz + dr*nz)*ez.z
  };
  pitch_rotate(p, P.pitch);
  return p;
}

// Safety scaling to avoid self-intersection
static inline void enforce_safety(TorusParams& P, const ToroidSettings& S){
  double sum_b = std::abs(P.b2) + std::abs(P.b3);
  double Reff_min = P.R0 * std::max(0.05, 1.0 - sum_b);

  double r1 = P.r0*(1.0 + P.eps);
  double r2 = P.r0*(1.0 - P.eps);
  double sum_a = std::abs(P.a2) + std::abs(P.a3);
  double r_eff_max = std::max(r1, r2) + P.r0*sum_a;

  if (r_eff_max >= S.max_tube_ratio * Reff_min) {
    double scale = (S.max_tube_ratio * Reff_min) / (r_eff_max + 1e-9);
    P.r0 *= scale;
    P.a2 *= scale;
    P.a3 *= scale;
  }
}

/* ----------------------- Wireframe sampling (iso-u/iso-v) ------------------ */
using Poly2 = std::vector<std::pair<double,double>>;

static void split_polyline_by_z_sign_and_project(
  const std::vector<Vec3>& closed3,
  std::vector<Poly2>& out_back,  // z<0
  std::vector<Poly2>& out_front  // z>=0
){
  if (closed3.size() < 2) return;
  auto proj = [](const Vec3& p){ return std::pair<double,double>(p.x, p.y); };

  auto push_seg = [&](Poly2& cur, bool front){
    if (cur.size() < 2) return;
    (front ? out_front : out_back).push_back(cur);
    cur.clear();
  };

  std::vector<Vec3> pts = closed3;
  if (!(std::abs(pts.front().x - pts.back().x) < 1e-12 &&
        std::abs(pts.front().y - pts.back().y) < 1e-12 &&
        std::abs(pts.front().z - pts.back().z) < 1e-12)) {
    pts.push_back(pts.front());
  }

  bool front = (pts[0].z >= 0.0);
  Poly2 cur; cur.reserve(pts.size());
  cur.push_back(proj(pts[0]));

  for (size_t i=1; i<pts.size(); ++i) {
    const Vec3& a = pts[i-1];
    const Vec3& b = pts[i];
    bool f = (b.z >= 0.0);

    if (f == front) {
      cur.push_back(proj(b));
    } else {
      double t = 0.5;
      double denom = (b.z - a.z);
      if (std::abs(denom) > 1e-12) t = (-a.z) / denom;
      t = std::clamp(t, 0.0, 1.0);
      Vec3 x = lerp(a, b, t);
      cur.push_back(proj(x));
      push_seg(cur, front);
      cur.push_back(proj(x));
      cur.push_back(proj(b));
      front = f;
    }
  }
  push_seg(cur, front);
}

struct Wireframe {
  std::vector<Poly2> back;
  std::vector<Poly2> front;
};

static Wireframe sample_wireframe(const TorusParams& P,
                                  int n_iso_u, int n_iso_v,
                                  int samples_per_line){
  Wireframe wf;
  const double TWO_PI = 6.283185307179586;
  samples_per_line = std::max(16, samples_per_line);

  // iso-u: vary v
  for (int i=0; i<n_iso_u; ++i) {
    double u = TWO_PI * (double(i) / n_iso_u);
    std::vector<Vec3> closed3; closed3.reserve(samples_per_line+1);
    for (int j=0; j<=samples_per_line; ++j) {
      double v = TWO_PI * (double(j) / samples_per_line);
      closed3.push_back(torus_point(P, u, v));
    }
    split_polyline_by_z_sign_and_project(closed3, wf.back, wf.front);
  }
  // iso-v: vary u
  for (int j=0; j<n_iso_v; ++j) {
    double v = TWO_PI * (double(j) / n_iso_v);
    std::vector<Vec3> closed3; closed3.reserve(samples_per_line+1);
    for (int i=0; i<=samples_per_line; ++i) {
      double u = TWO_PI * (double(i) / samples_per_line);
      closed3.push_back(torus_point(P, u, v));
    }
    split_polyline_by_z_sign_and_project(closed3, wf.back, wf.front);
  }
  return wf;
}

/* ----------------------- Closed “Lissajous on torus” curve ------------------ */
// We use integers (p, q), coprime, and trace (u, v) = (2π p t, 2π q t), t∈[0,1].
struct CurvePQ { int p{2}, q{3}; int n_points{900}; };

static Poly2 sample_torus_lissajous_2d(const TorusParams& P, const CurvePQ& C){
  const double TWO_PI = 6.283185307179586;
  int N = std::max(3, C.n_points);
  Poly2 out; out.reserve(N+1);
  for (int i=0; i<=N; ++i) {               // <= ensures closure
    double t = double(i) / double(N);
    double u = TWO_PI * double(C.p) * t;
    double v = TWO_PI * double(C.q) * t;
    // modulo to [0, 2π)
    u = std::fmod(u, TWO_PI); if (u<0) u += TWO_PI;
    v = std::fmod(v, TWO_PI); if (v<0) v += TWO_PI;
    Vec3 p = torus_point(P, u, v);
    out.emplace_back(p.x, p.y);
  }
  return out;
}

/* ============================ Randomization ============================ */
static std::mt19937& rng(){ static std::mt19937 g{ std::random_device{}() }; return g; }
static double urand(double a=0.0, double b=1.0){ std::uniform_real_distribution<double> d(a,b); return d(rng()); }
static double biased01(double p){ return std::pow(urand(), p); }
static int igcd(int a,int b){ while (b){ int t=a%b; a=b; b=t; } return std::abs(a); }
static int irand_int(int lo, int hi){ std::uniform_int_distribution<int> d(lo,hi); return d(rng()); }

static void randomize_params(TorusParams& P, CurvePQ& C, const ToroidSettings& T){
  // Global shape (slight bias to readable shapes)
  P.R0    = T.R_min + (T.R_max - T.R_min) * urand();
  P.r0    = T.r_min + (T.r_max - T.r_min) * urand();
  P.eps   = T.eps_max * biased01(2.0);
  P.pitch = (urand()<0.5 ? -1.0 : 1.0) * T.pitch_max * biased01(1.2);

  // Gentle harmonics; bias small amplitudes (a3=b3 kept zero)
  P.a2 = T.a2_max * biased01(2.4);
  P.b2 = T.b2_max * biased01(2.0);
  P.a3 = 0.0;
  P.b3 = 0.0;

  enforce_safety(P, T);

  // Pick coprime (p,q) with small magnitude (readable knot)
  // p in 1..5, q in 1..8, avoid (1,1), enforce gcd(p,q)=1
  do {
    C.p = irand_int(1,5);
    C.q = irand_int(1,8);
  } while ((C.p==1 && C.q==1) || igcd(C.p,C.q) != 1);
  C.n_points = 900;
}

/* ============================ UI glue ============================ */
struct Colors {
  std::string bg   = "black";
  std::string back = "#5A5A5A";   // back wireframe
  std::string front= "#D0D0D0";   // front wireframe
  std::string curve = "#FFC857";  // torus-Lissajous
  std::string title = "white";
  std::string border= "gray";
};

struct App {
  std::shared_ptr<iinuji_state_t> st;
  std::shared_ptr<iinuji_object_t> root;
  std::shared_ptr<iinuji_object_t> plot;
  Colors palette;
  ToroidSettings T;
  TorusParams P;
  CurvePQ C;

  bool show_wireframe = true;
  bool show_curve     = true;
};

static void build_ui(App& app) {
  iinuji_layout_t Lroot; Lroot.mode = layout_mode_t::Normalized; Lroot.width=1; Lroot.height=1; Lroot.normalized = true;
  iinuji_style_t  Sroot; Sroot.background_color = app.palette.bg;

  app.root = create_object("root", true, Lroot, Sroot);

  plotbox_opts_t opts;
  opts.draw_axes = false;
  opts.draw_grid = false;
  opts.baseline0 = false;
  opts.margin_left = 1; opts.margin_right = 1; opts.margin_top = 0; opts.margin_bot = 0;

  std::vector<std::vector<std::pair<double,double>>> empty_series;
  std::vector<plot_series_cfg_t> empty_cfg;
  iinuji_style_t Splot;
  Splot.border = true; Splot.border_color = app.palette.border;
  Splot.background_color = app.palette.bg;
  Splot.title = "Phase-free toroid";

  app.plot = create_plot_box("plot", empty_series, empty_cfg, opts, Lroot, Splot);
  app.root->add_child(app.plot);

  app.st = initialize_iinuji_state(app.root, true);
  app.st->register_id("plot", app.plot);
}

static void set_title(App& app) {
  auto* pbj = app.plot.get();
  if (!pbj) return;
  std::string t = "Phase-free toroid  "
                  "[Enter=randomize | t=torus "
                  + std::string(app.show_wireframe?"ON":"OFF")
                  + " | l=curve "
                  + std::string(app.show_curve?"ON":"OFF")
                  + " | q=quit]  "
                  "curve(p:q)=" + std::to_string(app.C.p) + ":" + std::to_string(app.C.q);
  pbj->style.title = t;
}

// Convert sampled wireframe + curve to plot series
static void plot_from_samples(App& app,
                              const Wireframe* wfOpt,
                              const Poly2* curveOpt) {
  auto pb = std::dynamic_pointer_cast<plotBox_data_t>(app.plot->data);
  if (!pb) return;

  pb->series.clear();
  pb->series_cfg.clear();

  auto push_series = [&](const Poly2& poly, const std::string& color){
    pb->series.push_back(poly);
    plot_series_cfg_t cfg;
    cfg.color_fg = color;
    cfg.mode = plot_mode_t::Line;
    cfg.scatter = false;
    cfg.scatter_every = 1;
    cfg.fill_vertical_if_same_x = true;
    cfg.envelope_enabled = false;
    pb->series_cfg.push_back(cfg);
  };

  if (wfOpt && app.show_wireframe) {
    for (const auto& p : wfOpt->back)  push_series(p, app.palette.back);
    for (const auto& p : wfOpt->front) push_series(p, app.palette.front);
  }
  if (curveOpt && app.show_curve && !curveOpt->empty()) {
    push_series(*curveOpt, app.palette.curve);
  }
}

static void render(App& app) {
  int H=0, W=0; if (auto* R = get_renderer()) R->size(H,W);
  layout_tree(app.root, Rect{0,0,W,H});
  if (auto* R = get_renderer()) {
    R->clear();
    render_tree(app.root);
    R->flush();
  }
}

static void randomize_and_redraw(App& app) {
  randomize_params(app.P, app.C, app.T);

  // sampling density (tuned for terminal)
  int n_iso_u = 22, n_iso_v = 8, samples  = 140;

  Wireframe wf;
  Poly2 curve;

  if (app.show_wireframe) {
    wf = sample_wireframe(app.P, n_iso_u, n_iso_v, samples);
  }
  if (app.show_curve) {
    curve = sample_torus_lissajous_2d(app.P, app.C);
  }

  set_title(app);
  plot_from_samples(app, app.show_wireframe ? &wf : nullptr,
                          app.show_curve     ? &curve : nullptr);
  render(app);
}

static void refresh_only(App& app) {
  // rebuild series from current params respecting toggles
  int n_iso_u = 22, n_iso_v = 8, samples  = 140;
  Wireframe wf;
  Poly2 curve;

  if (app.show_wireframe) wf = sample_wireframe(app.P, n_iso_u, n_iso_v, samples);
  if (app.show_curve)     curve = sample_torus_lissajous_2d(app.P, app.C);

  set_title(app);
  plot_from_samples(app, app.show_wireframe ? &wf : nullptr,
                          app.show_curve     ? &curve : nullptr);
  render(app);
}

/* ============================ Main loop ============================ */
int main() {
  // IMPORTANT: enable wide chars (braille/box) BEFORE initscr()
  std::setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nonl();
  curs_set(0);
  start_color();
  use_default_colors();

  NcursesRend rend;
  set_renderer(&rend);
  set_global_background("black");

  App app;
  app.palette = Colors{};
  build_ui(app);

  randomize_and_redraw(app);

  while (true) {
    int ch = getch();
    if (ch == ERR) continue;
    if (ch == 'q' || ch == 'Q' || ch == 27) break;

    if (ch == KEY_ENTER || ch == '\n' || ch == '\r') {
      randomize_and_redraw(app);
      continue;
    }
    if (ch == 't' || ch == 'T') {
      app.show_wireframe = !app.show_wireframe;
      refresh_only(app);
      continue;
    }
    if (ch == 'l' || ch == 'L') {
      app.show_curve = !app.show_curve;
      refresh_only(app);
      continue;
    }
    if (ch == KEY_RESIZE) {
      render(app);
      continue;
    }
  }
  endwin();
  return 0;
}
