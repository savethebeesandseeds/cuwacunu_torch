/*
  test_iinuji_data_viz.cpp
  TUI to visualize (1) input [1,C,T,D], (2) embedding [1,De], (3) MDN E[y] ± 1σ.

  - Pulls a real observation from MemoryMappedConcatDataset<Datatype_t> at a RANDOM index.
  - Highlights masked timestamps as RED scatter markers (on top of the line plot).
  - Embedding panel renders a colored tile matrix (diverging palette).
  - Value panel shows per-channel E[y] ± 1σ over horizons (naïve MDN from actual future).
  - No changes to iinuji sources.
*/

#include <locale.h>
#include <random>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include <torch/torch.h>

/* ---- Your data & pipeline ---- */
#include "piaabo/dconfig.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"

/* ---- iinuji UI ---- */
#include "iinuji/render/renderer.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/primitives/plot.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

using namespace cuwacunu::iinuji;
using cuwacunu::camahjucunu::data::observation_sample_t;

/* ──────────────────────────────────────────────────────────────────────────
   Small helpers
   ────────────────────────────────────────────────────────────────────────── */

static inline int wrap_idx(int v, int lo, int hi) {
  const int n = hi - lo + 1;
  return lo + (((v - lo) % n) + n) % n;
}

static inline Rect content_rect_like(const iinuji_object_t& o) {
  Rect r = o.screen;
  if (o.style.border) {
    r.x += 1; r.y += 1; r.w = std::max(0, r.w - 2); r.h = std::max(0, r.h - 2);
  }
  r.x += o.layout.pad_left; r.y += o.layout.pad_top;
  r.w = std::max(0, r.w - (o.layout.pad_left + o.layout.pad_right));
  r.h = std::max(0, r.h - (o.layout.pad_top  + o.layout.pad_bottom));
  return r;
}

static inline std::string hex_from_rgb(int R, int G, int B) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", R, G, B); // FIXED
  return std::string(buf);
}
static inline const std::vector<std::string>& diverging_palette_21() {
  static std::vector<std::string> pal;
  if (!pal.empty()) return pal;
  const int N = 21; pal.reserve(N);
  for (int i=0; i<N; ++i) {
    float t = float(i) / float(N-1);
    float r,g,b;
    if (t < 0.5f) { float u = t/0.5f; r=u; g=u; b=1.f; }
    else          { float u=(t-0.5f)/0.5f; r=1.f; g=1.f-u; b=1.f-u; }
    pal.push_back(hex_from_rgb(int(std::round(r*255)), int(std::round(g*255)),
                               int(std::round(b*255))));
  }
  return pal;
}

static inline std::pair<int,int> grid_rc_for_count(int N) {
  if (N <= 0) return {1,1};
  int cols = (int)std::ceil(std::sqrt((double)N));
  int rows = (int)std::ceil((double)N / (double)cols);
  return {rows, cols};
}

static inline torch::Tensor to_cpu_contig_float(const torch::Tensor& t) {
  return t.to(torch::kCPU).contiguous().to(torch::kFloat32);
}
static inline torch::Tensor as_BCTD(const torch::Tensor& features) {
  if (!features.defined()) return torch::Tensor();
  if (features.dim() == 4) {
    TORCH_CHECK(features.size(0) == 1, "Expected B=1 for [B,C,T,D]");
    return features;
  } else if (features.dim() == 3) {
    return features.unsqueeze(0); // [C,T,D] -> [1,C,T,D]
  }
  TORCH_CHECK(false, "features must be [B,C,T,D] or [C,T,D], got dim=", features.dim());
  return torch::Tensor();
}
static inline torch::Tensor as_BDe(const torch::Tensor& enc) {
  if (!enc.defined()) return torch::Tensor();
  TORCH_CHECK(enc.dim() == 2 || enc.dim() == 3, "encoding must be [B,De] or [B,T',De]");
  TORCH_CHECK(enc.size(0) == 1, "encoding B must be 1; got B=", enc.size(0));
  if (enc.dim() == 2) return enc;          // [1,De]
  return enc.mean(/*dim=*/1, false);       // [1,De] from [1,T',De]
}

/* ──────────────────────────────────────────────────────────────────────────
   Data Provider — REAL dataset, display-friendly embedding & MDN stub
   ────────────────────────────────────────────────────────────────────────── */

struct IDataProvider {
  virtual ~IDataProvider() = default;
  virtual bool snapshot_random(observation_sample_t& obs,
                               torch::Tensor& log_pi_BCHfK,
                               torch::Tensor& mu_BCHfKDy,
                               torch::Tensor& sigma_BCHfKDy,
                               int d_sel_for_y) = 0;
  virtual int C()  const = 0;
  virtual int T()  const = 0;
  virtual int D()  const = 0;
  virtual int De() const = 0;
  virtual int Hf() const = 0;
  virtual int K()  const = 0;
  virtual int Dy() const = 0;
  virtual int Tf() const = 0;
  virtual std::vector<std::string> channel_names() const { return {}; }
  virtual std::vector<std::string> input_dim_names() const { return {}; }
  virtual std::vector<std::string> output_dim_names() const { return {}; }
};

struct DatasetProvider final : public IDataProvider {
  using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t; // same as your test
  cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t> concat;
  std::mt19937 rng{1234567};

  int c_{0}, t_{0}, d_{0}, de_{64}, hf_{0}, k_{1}, dy_{0}, tf_{0};
  std::vector<std::string> ch_names_, in_names_, out_names_;

  // deterministic projection for embedding display
  torch::Tensor proj_; // [C*D, De]

  DatasetProvider(const std::string& instrument = "BTCUSDT",
                  const char* config_folder = "/cuwacunu/src/config/",
                  int De_vis = 64)
  : de_{De_vis} {
    // load config (paths, obs pipeline)
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    auto obs_inst = cuwacunu::camahjucunu::decode_observation_instruction_from_config();

    bool force_bin =
      cuwacunu::piaabo::dconfig::config_space_t::get<bool>("DATA_LOADER","dataloader_force_binarization");

    std::string inst = instrument;
    concat = cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(inst, obs_inst, force_bin);

    // infer shapes from first item
    auto s0 = concat.get(0);
    c_  = (int)s0.features.size(0);
    t_  = (int)s0.features.size(1);
    d_  = (int)s0.features.size(2);
    tf_ = (int)s0.future_features.size(1);
    hf_ = tf_;
    k_  = 1;
    dy_ = d_; // show all D as potential outputs (select with dy_sel)
    if (de_ <= 0) de_ = 64;

    ch_names_.reserve(c_);
    for (int i=0;i<c_;++i) ch_names_.push_back("ch"+std::to_string(i));
    in_names_.reserve(d_);
    for (int i=0;i<d_;++i) in_names_.push_back("x"+std::to_string(i));
    out_names_.reserve(d_);
    for (int i=0;i<d_;++i) out_names_.push_back("y"+std::to_string(i));

    // fixed random projection (masked pooled [C,D] -> [De])
    {
      std::mt19937 rr(42);
      std::normal_distribution<float> N(0.f, 1.f/std::sqrt(float(std::max(1, c_*d_))));
      proj_ = torch::empty({c_*d_, de_}, torch::kFloat32);
      auto* p = proj_.data_ptr<float>();
      for (int i=0;i<c_*d_*de_; ++i) p[i] = N(rr);
    }
  }

  int C()  const override { return c_;  }
  int T()  const override { return t_;  }
  int D()  const override { return d_;  }
  int De() const override { return de_; }
  int Hf() const override { return hf_; }
  int K()  const override { return k_;  }
  int Dy() const override { return dy_; }
  int Tf() const override { return tf_; }
  std::vector<std::string> channel_names() const override { return ch_names_; }
  std::vector<std::string> input_dim_names() const override { return in_names_; }
  std::vector<std::string> output_dim_names() const override { return out_names_; }

  /* masked pooled embedding + projection to [1,De] */
  torch::Tensor display_embedding(const observation_sample_t& obs) {
    // X: [C,T,D], M: [C,T]
    auto X = to_cpu_contig_float(obs.features);
    auto M = obs.mask.defined()? to_cpu_contig_float(obs.mask) : torch::ones({c_, t_}, torch::kFloat32);
    TORCH_CHECK(X.dim()==3 && (int)X.size(0)==c_ && (int)X.size(1)==t_ && (int)X.size(2)==d_, "unexpected X shape");

    auto M3 = M.unsqueeze(-1);                     // [C,T,1]
    auto sum = (X * M3).sum(/*dim=*/1);            // [C,D]
    auto cnt = M.sum(/*dim=*/1).clamp_min(1.0f).unsqueeze(-1); // [C,1]
    auto mean_CD = sum / cnt;                      // [C,D]
    auto flat = mean_CD.reshape({c_*d_});          // [C*D]
    auto z = torch::tanh(torch::matmul(flat, proj_)); // [De]
    return z.unsqueeze(0);                         // [1,De]
  }

  bool snapshot_random(observation_sample_t& obs,
                       torch::Tensor& log_pi_BCHfK,
                       torch::Tensor& mu_BCHfKDy,
                       torch::Tensor& sigma_BCHfKDy,
                       int d_sel_for_y) override {
    std::uniform_int_distribution<long> U(0, (long)concat.size().value()-1);
    long idx = U(rng);

    observation_sample_t s = concat.get((std::size_t)idx); // features: [C,T,D]
    // Representation (stub): [1,De]
    s.encoding = display_embedding(s);

    // Naïve MDN from actual future: K=1, Dy=D
    const int C = (int)s.features.size(0);
    const int Hf= (int)s.future_features.size(1);
    const int D = (int)s.features.size(2);

    auto Xf = to_cpu_contig_float(s.future_features); // [C,Hf,D]
    auto Mf = s.future_mask.defined()? s.future_mask.to(torch::kFloat32).contiguous()
                                    : torch::ones({C,Hf}, torch::kFloat32);

    log_pi_BCHfK = torch::zeros({1, C, Hf, 1}, torch::kFloat32);        // log 1.0
    mu_BCHfKDy   = torch::full ({1, C, Hf, 1, D}, std::numeric_limits<float>::quiet_NaN(), torch::kFloat32);
    sigma_BCHfKDy= torch::zeros({1, C, Hf, 1, D}, torch::kFloat32);

    // per-horizon sigma across channels (only over valid masks)
    for (int h=0; h<Hf; ++h) {
      for (int dd=0; dd<D; ++dd) {
        // gather valid channels
        std::vector<float> vals; vals.reserve(C);
        for (int c=0;c<C;++c) {
          float m = Mf[c][h].item<float>();
          if (m > 0.5f) vals.push_back(Xf[c][h][dd].item<float>());
        }
        float mean = 0.f, sd = 0.f;
        if (!vals.empty()) {
          if (!vals.empty()) {
            for (float v : vals) { mean += v; }
            mean /= static_cast<float>(vals.size());

            float v2 = 0.f;
            for (float v : vals) { float d = v - mean; v2 += d * d; }
            sd = std::sqrt(v2 / std::max(1, static_cast<int>(vals.size()) - 1));
          }
          float v2=0.f; for (float v:vals){ float d=v-mean; v2+=d*d; }
          sd = std::sqrt(v2 / std::max(1,(int)vals.size()-1));
        }
        for (int c=0;c<C;++c) {
          float m = Mf[c][h].item<float>();
          if (m > 0.5f) {
            mu_BCHfKDy[0][c][h][0][dd]    = Xf[c][h][dd];
            sigma_BCHfKDy[0][c][h][0][dd] = sd;        // same band width for all channels at this horizon
          }
        }
      }
    }

    obs = std::move(s);
    return true;
  }
};

/* ──────────────────────────────────────────────────────────────────────────
   GUI selections
   ────────────────────────────────────────────────────────────────────────── */

struct GuiSel {
  int c_sel{0};         // channel for MDN plot
  int d_sel{0};         // input feature index
  int dy_sel{0};        // output dim for MDN
  bool emb_symmetric{true};
  bool emb_grid{false};
  long t_index{0};      // kept for completeness (not used by DatasetProvider)
};

/* ──────────────────────────────────────────────────────────────────────────
   Data → UI adapters
   ────────────────────────────────────────────────────────────────────────── */

/* Input plot: per-channel lines; masked samples as red scatter markers */
static void fill_input_plot_with_mask(const observation_sample_t& obs,
                                      int d_sel,
                                      std::vector<std::vector<std::pair<double,double>>>& series,
                                      std::vector<plot_series_cfg_t>& cfg,
                                      plotbox_opts_t& opts) {
  auto X = as_BCTD(obs.features);     // [1,C,T,D] or [C,T,D]
  auto M = obs.mask.defined() ? (obs.mask.dim()==3 ? obs.mask : obs.mask.unsqueeze(0)) : torch::Tensor(); // [1,C,T]
  X = to_cpu_contig_float(X);
  if (M.defined()) M = to_cpu_contig_float(M);

  const int C = (int)X.size(1);
  const int T = (int)X.size(2);

  series.clear(); cfg.clear();
  series.reserve(2*C); cfg.reserve(2*C);

  static const char* pal[] = {
    "#F94144","#277DA1","#efef09","#43AA8B",
    "#577590","#90BE6D","#4D908E","#F9C74F","#b0b0b0"
  };
  const int palN = (int)(sizeof(pal)/sizeof(pal[0]));

  opts.x_min = 0; opts.x_max = std::max(1, T-1);
  opts.y_min = std::numeric_limits<double>::quiet_NaN(); // auto
  opts.y_max = std::numeric_limits<double>::quiet_NaN();
  opts.x_label = "t (samples)";
  opts.y_label = "x[., d]";

  for (int c=0;c<C;++c) {
    std::vector<std::pair<double,double>> pts_line;    pts_line.reserve(T);
    std::vector<std::pair<double,double>> pts_missing; pts_missing.reserve(T/8);

    float last_valid = 0.f; bool have_last=false;

    for (int i=0;i<T;++i) {
      float v = X[0][c][i][d_sel].item<float>();
      float m = M.defined() ? M[0][c][i].item<float>() : 1.f;

      if (m > 0.5f) {
        pts_line.emplace_back((double)i, (double)v);
        have_last = true; last_valid = v;
      } else {
        // Break line with NaN and place a red marker near last valid (or zero if none yet)
        pts_line.emplace_back((double)i, std::numeric_limits<double>::quiet_NaN());
        float mark_y = have_last ? last_valid : 0.f;
        pts_missing.emplace_back((double)i, (double)mark_y);
      }
    }

    // main line series
    plot_series_cfg_t sc_line;
    sc_line.color_fg = pal[c % palN];
    sc_line.mode = plot_mode_t::Line;
    sc_line.scatter = false;
    sc_line.envelope_enabled = false;
    series.push_back(std::move(pts_line));
    cfg.push_back(sc_line);

    // missing markers as red scatter
    if (!pts_missing.empty()) {
      plot_series_cfg_t sc_miss;
      sc_miss.color_fg = "#FF4D4D";
      sc_miss.mode = plot_mode_t::Scatter;
      sc_miss.scatter = true;
      sc_miss.scatter_every = 1;
      series.push_back(std::move(pts_missing));
      cfg.push_back(sc_miss);
    } else {
      // still push an empty to keep indices predictable
      series.push_back({});
      plot_series_cfg_t dummy; dummy.color_fg = "#FF4D4D"; dummy.mode = plot_mode_t::Scatter; dummy.scatter = true;
      cfg.push_back(dummy);
    }
  }
}

/* MDN panel: compute E[y] and ±1σ; skip non-finite points gracefully */
static void fill_mdn_plot_from_params(const torch::Tensor& log_pi_BCHfK,
                                      const torch::Tensor& mu_BCHfKDy,
                                      const torch::Tensor& sigma_BCHfKDy,
                                      int c_sel, int dy_sel,
                                      std::vector<std::vector<std::pair<double,double>>>& series,
                                      std::vector<plot_series_cfg_t>& cfg,
                                      plotbox_opts_t& opts) {
  auto log_pi = to_cpu_contig_float(log_pi_BCHfK[0][c_sel]);       // [Hf,K]
  auto mu     = to_cpu_contig_float(mu_BCHfKDy[0][c_sel]);         // [Hf,K,Dy]
  auto sigma  = to_cpu_contig_float(sigma_BCHfKDy[0][c_sel]);      // [Hf,K,Dy]

  const int Hf = (int)log_pi.size(0);
  const int K  = (int)log_pi.size(1);

  // log_pi is already log_softmax from your forward
  torch::Tensor pi = torch::exp(log_pi); // [Hf,K]

  std::vector<std::pair<double,double>> mean_pts; mean_pts.reserve(Hf);
  std::vector<std::pair<double,double>> band_pts; band_pts.reserve(Hf*2);

  bool any=false; double ymin=+1e30, ymax=-1e30;

  for (int h=0; h<Hf; ++h) {
    auto pi_h = pi[h];                          // [K]
    auto mu_h = mu[h].select(1, dy_sel);        // [K]
    auto sg_h = sigma[h].select(1, dy_sel);     // [K]

    // Check if any component is finite; otherwise skip this horizon
    bool finite_any=false;
    for (int kk=0; kk<K; ++kk) {
      float mk = mu_h[kk].item<float>();
      if (std::isfinite(mk)) { finite_any=true; break; }
    }
    if (!finite_any) continue;

    double Ey = 0.0;
    for (int kk=0; kk<K; ++kk) {
      double pk = (double)pi_h[kk].item<float>();
      double mk = (double)mu_h[kk].item<float>();
      if (std::isfinite(mk)) Ey += pk * mk;
    }

    double second = 0.0;
    for (int kk=0; kk<K; ++kk) {
      double pk = (double)pi_h[kk].item<float>();
      double mk = (double)mu_h[kk].item<float>();
      double sk = (double)sg_h[kk].item<float>();
      if (std::isfinite(mk)) second += pk * (sk*sk + mk*mk);
    }
    double var = std::max(0.0, second - Ey*Ey);
    double sd  = std::sqrt(var);

    mean_pts.emplace_back((double)h, Ey);
    band_pts.emplace_back((double)h, Ey - sd);
    band_pts.emplace_back((double)h, Ey + sd);

    any  = true;
    ymin = std::min(ymin, Ey - sd);
    ymax = std::max(ymax, Ey + sd);
  }

  series.clear(); cfg.clear();

  // Band
  {
    plot_series_cfg_t sc;
    sc.color_fg = "#90BE6D";
    sc.mode = plot_mode_t::Line;
    sc.envelope_enabled = true;
    sc.envelope_source = envelope_source_t::OriginalSamples;
    sc.envelope_min_count = 2;
    sc.envelope_min_height = 1;
    sc.envelope_draw_base = false;
    series.push_back(std::move(band_pts));
    cfg.push_back(sc);
  }
  // Mean line
  {
    plot_series_cfg_t sc;
    sc.color_fg = "#277DA1";
    sc.mode = plot_mode_t::Line;
    series.push_back(std::move(mean_pts));
    cfg.push_back(sc);
  }

  if (!any) { ymin = 0.0; ymax = 1.0; }
  opts.x_min = 0; opts.x_max = std::max(1, (int)log_pi.size(0)-1);
  opts.y_min = ymin; opts.y_max = ymax;
  opts.x_label = "horizon (steps)";
  opts.y_label = "E[y] ± 1σ";
}

/* Extract embedding vector [De] as CPU floats for painting */
static std::vector<float> extract_embedding_1D(const observation_sample_t& obs) {
  if (!obs.encoding.defined()) return {};
  auto E = as_BDe(obs.encoding);        // [1,De] or [1,T',De] → [1,De]
  auto z = to_cpu_contig_float(E[0]);   // [De]
  std::vector<float> v(z.size(0));
  for (int i=0;i<z.size(0);++i) v[i] = z[i].item<float>();
  return v;
}

/* ──────────────────────────────────────────────────────────────────────────
   Embedding painter (no change to iinuji required)
   ────────────────────────────────────────────────────────────────────────── */

static void paint_embedding_into_object(const std::shared_ptr<iinuji_object_t>& obj,
                                        const std::vector<float>& values,
                                        bool symmetric_scale,
                                        bool draw_grid_lines) {
  if (!obj) return;
  auto* R = get_renderer(); if (!R) return;

  Rect r = content_rect_like(*obj);
  if (r.w <= 0 || r.h <= 0) return;

  // clear panel
  short panel_pair = (short)get_color_pair(obj->style.label_color, obj->style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, panel_pair);

  if (values.empty()) return;

  const int N = (int)values.size();
  auto [rows, cols] = grid_rc_for_count(N);
  int cell_w = std::max(1, r.w / cols);
  int cell_h = std::max(1, r.h / rows);

  float vmin = +std::numeric_limits<float>::infinity();
  float vmax = -std::numeric_limits<float>::infinity();
  for (float v : values) { vmin = std::min(vmin,v); vmax = std::max(vmax,v); }
  if (!std::isfinite(vmin) || !std::isfinite(vmax) || vmin==vmax) { vmin = -1.f; vmax = 1.f; }
  if (symmetric_scale) { float m = std::max(std::fabs(vmin), std::fabs(vmax)); vmin = -m; vmax = m; }
  float span = (vmax - vmin); if (!(span > 0.f)) span = 1.f;

  const auto& pal = diverging_palette_21();
  const int PN = (int)pal.size();

  for (int rr=0; rr<rows; ++rr) {
    for (int cc=0; cc<cols; ++cc) {
      int idx = rr*cols + cc;
      float v = (idx < N ? values[idx] : 0.f);
      float t = (v - vmin) / span; t = std::min(1.f, std::max(0.f, t));
      int k = std::min(PN-1, std::max(0, (int)std::floor(t*(PN-1))));

      // IMPORTANT: set palette color as BACKGROUND, since fillRect draws spaces
      short cp = (short)get_color_pair(obj->style.label_color, pal[k]);

      int x = r.x + cc*cell_w;
      int y = r.y + rr*cell_h;
      R->fillRect(y, x, cell_h, cell_w, cp);

      if (draw_grid_lines && cell_w > 2 && cell_h > 1) {
        short gp = (short)get_color_pair(obj->style.border_color, obj->style.background_color);
        for (int hh=0; hh<cell_h; ++hh) R->putGlyph(y+hh, x+cell_w-1, L'│', gp);
        for (int ww=0; ww<cell_w; ++ww) R->putGlyph(y+cell_h-1, x+ww,   L'─', gp);
      }
    }
  }
}

/* ──────────────────────────────────────────────────────────────────────────
   UI wiring
   ────────────────────────────────────────────────────────────────────────── */

struct Ui {
  std::shared_ptr<iinuji_state_t> st;
  std::shared_ptr<iinuji_object_t> root, header, plot_input, embed_panel, plot_mdn;

  Ui() {
    root = create_grid_container("root",
      { len_spec::px(3), len_spec::frac(1), len_spec::frac(1) },
      { len_spec::frac(1), len_spec::frac(1) }, 1, 2,
      {}, iinuji_style_t{ .label_color="white", .background_color="black", .border=false });

    header = create_text_box("header", " ", false, text_align_t::Left,
      {}, iinuji_style_t{ .label_color="black", .background_color="#E9ECEF", .border=true, .border_color="#ADB5BD", .title="Expected Value – Snapshot" });
    place_in_grid(header, 0, 0, 1, 2); root->add_child(header);

    plot_input = create_plot_box("input", {}, {},
      plotbox_opts_t{ .draw_axes=true, .draw_grid=true, .baseline0=true, .y_ticks=5, .x_ticks=6, .x_label="t", .y_label="x" },
      {}, iinuji_style_t{ .label_color="black", .background_color="black", .border=true, .border_color="#555", .title="Input (all channels; red = masked)" });
    place_in_grid(plot_input, 1, 0, 1, 1); root->add_child(plot_input);

    embed_panel = create_object("embed", true, {},
      iinuji_style_t{ .label_color="white", .background_color="black", .border=true, .border_color="#555", .title="Embedding" });
    place_in_grid(embed_panel, 1, 1, 1, 1); root->add_child(embed_panel);

    plot_mdn = create_plot_box("mdn", {}, {},
      plotbox_opts_t{ .draw_axes=true, .draw_grid=true, .baseline0=true, .y_ticks=5, .x_ticks=6, .x_label="h", .y_label="E[y]±σ", .margin_left=10 },
      {}, iinuji_style_t{ .label_color="black", .background_color="black", .border=true, .border_color="#555", .title="Value estimation (naïve from future)" });
    place_in_grid(plot_mdn, 2, 0, 1, 2); root->add_child(plot_mdn);

    st = initialize_iinuji_state(root, true);
  }
};

static std::string header_line(const IDataProvider& P, const GuiSel& G,
                               const std::vector<std::string>& chn,
                               const std::vector<std::string>& in_names,
                               const std::vector<std::string>& out_names) {
  auto sn = [&](const std::vector<std::string>& v, int i)->std::string {
    if (v.empty()) return std::to_string(i);
    if (i >= 0 && i < (int)v.size()) return v[i];
    return std::to_string(i);
  };
  char buf[512];
  std::snprintf(buf, sizeof(buf),
    "C=%d T=%d D=%d De=%d Hf=%d K=%d Dy=%d | [ch:%d:%s  d:%d:%s  y:%d:%s] | keys: q r a/d [/] ,/. ;/' h g",
    P.C(), P.T(), P.D(), P.De(), P.Hf(), P.K(), P.Dy(),
    G.c_sel, sn(chn,G.c_sel).c_str(),
    G.d_sel, sn(in_names,G.d_sel).c_str(),
    G.dy_sel, sn(out_names,G.dy_sel).c_str());
  return std::string(buf);
}

static void refresh_ui(const observation_sample_t& OBS,
                       const torch::Tensor& LOGPI,
                       const torch::Tensor& MU,
                       const torch::Tensor& SIGMA,
                       const IDataProvider& P, const GuiSel& G, Ui& ui) {
  auto chn = P.channel_names(); auto in_names = P.input_dim_names(); auto out_names = P.output_dim_names();

  if (auto tb = std::dynamic_pointer_cast<textBox_data_t>(ui.header->data)) {
    tb->content = header_line(P, G, chn, in_names, out_names);
  }

  // Input + mask overlay
  {
    auto pb = std::dynamic_pointer_cast<plotBox_data_t>(ui.plot_input->data);
    fill_input_plot_with_mask(OBS, G.d_sel, pb->series, pb->series_cfg, pb->opts);
  }

  // Value estimation
  {
    auto pb = std::dynamic_pointer_cast<plotBox_data_t>(ui.plot_mdn->data);
    fill_mdn_plot_from_params(LOGPI, MU, SIGMA, G.c_sel, G.dy_sel, pb->series, pb->series_cfg, pb->opts);
  }

  // Embedding painted after render_tree()
}

/* ──────────────────────────────────────────────────────────────────────────
   Main
   ────────────────────────────────────────────────────────────────────────── */

int main() {
  setlocale(LC_ALL, "");

  // ncurses init
  initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); nodelay(stdscr, FALSE);
  if (has_colors()) { start_color(); use_default_colors(); }
  cuwacunu::iinuji::set_renderer(new cuwacunu::iinuji::NcursesRend());
  set_global_background("black");

  // Provider + UI
  DatasetProvider prov; // instrument defaults to "BTCUSDT"
  GuiSel G;
  Ui ui;

  observation_sample_t OBS;
  torch::Tensor LOGPI, MU, SIGMA;

  // First random snapshot
  prov.snapshot_random(OBS, LOGPI, MU, SIGMA, G.d_sel);
  auto emb = extract_embedding_1D(OBS);
  refresh_ui(OBS, LOGPI, MU, SIGMA, prov, G, ui);

  bool running = true;
  while (running) {
    int H,W; if (auto* R = get_renderer()) R->size(H,W); else { H=40; W=160; }
    layout_tree(ui.root, Rect{0,0,W,H});
    render_tree(ui.root);

    // Paint embedding
    {
      auto zz = extract_embedding_1D(OBS);
      paint_embedding_into_object(ui.embed_panel, zz, G.emb_symmetric, G.emb_grid);
    }

    if (auto* R = get_renderer()) R->flush();

    // Keys
    int ch = getch();
    bool need_new_random=false, need_recompute=false;
    switch (ch) {
      case 'q': case 'Q': running=false; break;
      case 'r': case 'R': need_new_random=true; break; // new random anchor

      case KEY_LEFT: case 'a': G.t_index = std::max<long>(0, G.t_index-1); need_new_random=true; break; // still random
      case KEY_RIGHT: case 'd': G.t_index = G.t_index+1; need_new_random=true; break;

      case '[': G.c_sel = wrap_idx(G.c_sel-1, 0, std::max(0, prov.C()-1)); need_recompute=true; break;
      case ']': G.c_sel = wrap_idx(G.c_sel+1, 0, std::max(0, prov.C()-1)); need_recompute=true; break;

      case ',': G.d_sel = wrap_idx(G.d_sel-1, 0, std::max(0, prov.D()-1)); need_new_random=true; break; // y depends on d
      case '.': G.d_sel = wrap_idx(G.d_sel+1, 0, std::max(0, prov.D()-1)); need_new_random=true; break;

      case ';':  G.dy_sel = wrap_idx(G.dy_sel-1, 0, std::max(0, prov.Dy()-1)); need_recompute=true; break;
      case '\'': G.dy_sel = wrap_idx(G.dy_sel+1, 0, std::max(0, prov.Dy()-1)); need_recompute=true; break;

      case 'h': case 'H': G.emb_symmetric = !G.emb_symmetric; break;
      case 'g': case 'G': G.emb_grid = !G.emb_grid; break;

      default: break;
    }

    if (need_new_random) {
      prov.snapshot_random(OBS, LOGPI, MU, SIGMA, G.d_sel);
      refresh_ui(OBS, LOGPI, MU, SIGMA, prov, G, ui);
    } else if (need_recompute) {
      refresh_ui(OBS, LOGPI, MU, SIGMA, prov, G, ui);
    }
  }

  endwin();
  return 0;
}
