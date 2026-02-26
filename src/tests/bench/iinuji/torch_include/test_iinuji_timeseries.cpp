// test_iinuji_timeseries.cpp
// Interactive ncurses demo that slices MemoryMappedConcatDataset by time & channel
// and plots *both* raw data and the *real* VICReg embedding from the trained encoder.
//
// Run:
//   /cuwacunu/src/tests/build/test_iinuji_timeseries BTCUSDT
//
// Controls:
//   ←/→ : pan           (by ~window/8)
//   ↑/↓ : zoom in/out   (cycle presets)
//   g/G : big pan ±10×
//   c/C : next/prev channel (concat source)
//   d/D : next/prev feature dimension (raw feature)
//   n   : z‑score normalize the *raw feature* series
//   m   : embedding value = L2 norm (on/off → component)
//   e/E : next/prev embedding component (when in component mode)
//   t   : for [B,T',De] embeddings, toggle mean over time vs last step
//   1..5: quick window sizes
//   q   : quit

// ────────────────────────────────────────────────────────────────────────────
// 1) Include LibTorch FIRST (avoid ncurses macro 'timeout' corrupting torch)
// ────────────────────────────────────────────────────────────────────────────
#include <torch/torch.h>

// ────────────────────────────────────────────────────────────────────────────
// 2) Then the rest (some of these include ncurses transitively)
// ────────────────────────────────────────────────────────────────────────────
#include <locale.h>
#include <ncursesw/ncurses.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "piaabo/dconfig.h"
#include "piaabo/dutils.h"

// Data layer
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"

// Observation pipeline decode helper
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"

// GUI (iinuji)
#include "iinuji/render/renderer.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

// VICReg embedding model (required)
#include "wikimyei/representation/VICReg/vicreg_4d.h"

using torch::indexing::Slice;

// Choose your base record type
using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;
// Alt: using Datatype_t = cuwacunu::camahjucunu::exchange::basic_t;

using Dataset_t  = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
using Datasample_t   = cuwacunu::camahjucunu::data::observation_sample_t;
using kvalue_t = typename Datatype_t::key_type_t;  // NOTE: not 'key_t' (conflicts with sys/types.h)

/* --------------------- small utilities --------------------- */
template <typename T>
static inline T clampv(T v, T lo, T hi) { return std::min(std::max(v, lo), hi); }

static std::string fmt_double(double v) {
  char buf[64]; std::snprintf(buf, sizeof(buf), "%.6g", v); return buf;
}

// Get anchor key (time t) for channel 'ch' from a sample's past_keys [C,T] (last)
static inline double anchor_key_from_sample(const Datasample_t& s, int64_t ch, int64_t max_N_past) {
  auto tk = s.past_keys;
  if (!tk.defined()) return std::numeric_limits<double>::quiet_NaN();
  if (tk.dim() == 2) return tk.index({ ch, max_N_past - 1 }).to(torch::kFloat64).item<double>();
  if (tk.dim() == 1) return tk.index({ max_N_past - 1 }).to(torch::kFloat64).item<double>();
  return std::numeric_limits<double>::quiet_NaN();
}

// Extract y = features[ch, t=max_N_past-1, d]
static inline double feature_value_t_at_dim(const Datasample_t& s, int64_t ch, int64_t max_N_past, int64_t d) {
  auto fx = s.features; // [C, T, D]
  if (!fx.defined() || fx.dim() != 3) return std::numeric_limits<double>::quiet_NaN();
  return static_cast<double>(fx.index({ ch, max_N_past - 1, d }).item<float>());
}

// Simple z-score normalization for a series (in-place)
static inline void zscore_inplace(std::vector<std::pair<double,double>>& pts) {
  if (pts.size() < 2) return;
  double mu = 0.0, ss = 0.0;
  size_t n = 0;
  for (auto& p : pts) if (std::isfinite(p.second)) { mu += p.second; ++n; }
  if (n < 2) return;
  mu /= static_cast<double>(n);
  for (auto& p : pts) if (std::isfinite(p.second)) ss += (p.second - mu)*(p.second - mu);
  double sd = std::sqrt(ss / (n - 1));
  if (sd <= 0) return;
  for (auto& p : pts) p.second = (p.second - mu) / sd;
}

/* --------------------- UI state --------------------- */
struct AppState {
  // Data / geometry
  Dataset_t dataset;
  int64_t K{0};                   // number of channels (concat sources)
  int64_t D{0};                   // feature dimension
  int64_t max_N_past{1};          // length of past window
  int64_t max_N_future{1};        // future window (not used for y here)

  kvalue_t leftmost{};
  kvalue_t rightmost{};
  kvalue_t step{};
  std::size_t num_records{0};     // number of valid anchor positions on global grid

  // View
  std::vector<int> window_sizes{256, 512, 1024, 2048, 4096};
  int window_idx{2};              // index into window_sizes
  std::size_t center_idx{0};      // anchor index in [0 .. num_records-1]
  int64_t ch_idx{0};              // channel selection
  int64_t dim_idx{0};             // feature dimension selection
  bool normalize_main{false};

  // Embedding view config
  bool   enc_use_norm{true};      // if true: L2 norm across De (after optional time reduce)
  bool   enc_reduce_time_mean{true}; // for [B,T',De]: mean across time vs. last step
  int64_t enc_comp_idx{0};        // component when enc_use_norm == false
  int64_t De{0};                  // embedding dimensionality (discovered at runtime)

  // VICReg model + device + SWA toggle
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D* rep{nullptr};
  torch::Device rep_device{torch::kCPU};
  bool use_swa{true};

  // GUI containers
  std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> root;
  std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> info_panel;
  std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> plot_main;
  std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> plot_emb;

  // colors
  std::string bg_color{"#101214"};
  std::string fg_text{"#C8C8C8"};
  std::string accent1{"#FFC857"}; // main plot
  std::string accent2{"#58A6FF"}; // embedding plot
};

/* --------------------- build GUI tree --------------------- */
static std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
build_ui(AppState& S)
{
  using namespace cuwacunu::iinuji;

  // Root grid: 2 rows (top help bar, main area)
  auto root = create_grid_container("root",
    { len_spec::px(1), len_spec::frac(1) },   // rows
    { len_spec::frac(1) },                    // cols
    0, 0,
    iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, false });

  // Help bar
  auto help = create_text_box("help",
    "  ←/→ pan   ↑/↓ zoom   g/G big pan   c/C chan   d/D dim   n norm   m norm↔comp   e/E comp±   t time red   1..5 presets   q quit",
    false, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{ S.bg_color, S.bg_color, false });
  cuwacunu::iinuji::place_in_grid(help, 0, 0, 1, 1);

  // Main area: sidebar + plots
  auto main = create_grid_container("main",
    { len_spec::frac(1) },
    { len_spec::px(30), len_spec::frac(1) },  // sidebar + plots
    0, 1,
    iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, false });
  cuwacunu::iinuji::place_in_grid(main, 1, 0, 1, 1);

  // Sidebar: info panel
  S.info_panel = create_text_box("info", "", true, text_align_t::Left,
    iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, true, "#2D3748" });
  cuwacunu::iinuji::place_in_grid(S.info_panel, 0, 0, 1, 1);

  // Right: plots stacked (2 rows)
  auto plots = create_grid_container("plots",
    { len_spec::frac(1), len_spec::frac(1) },
    { len_spec::frac(1) },
    1, 0,
    iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, false });
  cuwacunu::iinuji::place_in_grid(plots, 0, 1, 1, 1);

  // Main plot (raw feature at t)
  {
    cuwacunu::iinuji::plotbox_opts_t opts;
    opts.draw_axes = true; opts.draw_grid = true;
    opts.y_label = "feature value"; opts.x_label = "key (t)";
    opts.margin_left = 10; opts.margin_bot = 2;
    std::vector<std::vector<std::pair<double,double>>> series(1);
    std::vector<cuwacunu::iinuji::plot_series_cfg_t> cfg(1);
    cfg[0].color_fg = S.accent1;
    cfg[0].mode = cuwacunu::iinuji::plot_mode_t::Line;
    S.plot_main = create_plot_box("plot_main", series, cfg, opts,
      iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, true, "#2D3748", false, false, "Feature @ t" });
    cuwacunu::iinuji::place_in_grid(S.plot_main, 0, 0, 1, 1);
  }

  // Embedding plot
  {
    cuwacunu::iinuji::plotbox_opts_t opts;
    opts.draw_axes = true; opts.draw_grid = true;
    opts.y_label = "embedding"; opts.x_label = "key (t)";
    opts.margin_left = 10; opts.margin_bot = 2;
    std::vector<std::vector<std::pair<double,double>>> series(1);
    std::vector<cuwacunu::iinuji::plot_series_cfg_t> cfg(1);
    cfg[0].color_fg = S.accent2;
    cfg[0].mode = cuwacunu::iinuji::plot_mode_t::Line;
    S.plot_emb = create_plot_box("plot_emb", series, cfg, opts,
      iinuji_layout_t{}, iinuji_style_t{ S.fg_text, S.bg_color, true, "#2D3748", false, false, "Embedding @ t" });
    cuwacunu::iinuji::place_in_grid(S.plot_emb, 1, 0, 1, 1);
  }

  root->add_children({ help, main });
  main->add_children({ S.info_panel, plots });
  plots->add_children({ S.plot_main, S.plot_emb });

  return S.root = root;
}

/* --------------------- compute and update series --------------------- */
static void update_plots(AppState& S)
{
  using namespace cuwacunu::iinuji;

  if (S.num_records == 0) return;

  // Visible [left_idx, right_idx]
  std::size_t win = (std::size_t)S.window_sizes[clampv(S.window_idx, 0, (int)S.window_sizes.size()-1)];
  win = std::max<std::size_t>(1, std::min(win, S.num_records));

  std::size_t half = win / 2;
  std::size_t left_idx  = (S.center_idx > half) ? (S.center_idx - half) : 0;
  if (left_idx + win > S.num_records) left_idx = S.num_records - win;
  std::size_t right_idx = left_idx + win - 1;

  // Keys on the global grid
  kvalue_t left_key  = S.leftmost + static_cast<kvalue_t>(left_idx)  * S.step;
  kvalue_t right_key = S.leftmost + static_cast<kvalue_t>(right_idx) * S.step;

  // Pull samples (aligned & clamped by dataset)
  auto samples = S.dataset.range_samples_by_keys(left_key, right_key);
  if (samples.empty()) return;

  // Build X vector first (anchor key per sample for selected channel)
  const int64_t ch = clampv<int64_t>(S.ch_idx, 0, std::max<int64_t>(0, S.K-1));
  const int64_t dd = clampv<int64_t>(S.dim_idx, 0, std::max<int64_t>(0, S.D-1));
  std::vector<double> X; X.reserve(samples.size());
  for (const auto& s : samples) X.push_back(anchor_key_from_sample(s, ch, S.max_N_past));

  // Raw feature series at t
  std::vector<std::pair<double,double>> series_main; series_main.reserve(samples.size());
  for (size_t i=0;i<samples.size();++i) {
    double y = feature_value_t_at_dim(samples[i], ch, S.max_N_past, dd);
    series_main.emplace_back(X[i], y);
  }
  if (S.normalize_main) zscore_inplace(series_main);

  // --------- Embedding via VICReg encode(past_features, mask) ----------
  // Batch-collate only the PAST (like your RepresentationDataloaderView)
  Datasample_t batch_past = Datasample_t::collate_fn_past(samples); // [B,C,T,D], [B,C,T]
  auto feats = batch_past.features; // CPU
  auto mask  = batch_past.mask;     // CPU
  TORCH_CHECK(feats.defined() && mask.defined(), "Past feats/mask undefined for encode()");
  TORCH_CHECK(feats.dim()==4 && mask.dim()==3, "Unexpected dims: feats ", feats.dim(), " mask ", mask.dim());

  torch::NoGradGuard ng;
  auto feats_dev = feats.to(S.rep_device, /*non_blocking=*/false);
  auto mask_dev  = mask.to (S.rep_device, /*non_blocking=*/false);

  // Encode: NOTE we do not detach to CPU inside; we move after reduction choice
  torch::Tensor enc = S.rep->encode(feats_dev, mask_dev, /*use_swa=*/S.use_swa, /*detach_to_cpu=*/false);

  // enc is either [B,De] or [B,T',De]
  torch::Tensor E = enc;
  if (E.dim() == 3) {
    if (S.enc_reduce_time_mean) {
      E = E.mean(/*dim=*/1);          // [B,De]
    } else {
      E = E.index({ Slice(), E.size(1)-1, Slice() }); // last time → [B,De]
    }
  } else if (E.dim() != 2) {
    // Fallback: collapse all dims to scalar
    E = E.flatten(1); // [B,?]
  }
  E = E.to(torch::kCPU).contiguous(); // make CPU for scalar extraction
  S.De = (E.dim() >= 2 ? E.size(1) : 1);

  std::vector<std::pair<double,double>> series_emb; series_emb.reserve(samples.size());
  if (S.enc_use_norm) {
    // L2 norm across De
    auto norms = E.pow(2).sum(/*dim=*/1).sqrt(); // [B]
    for (int64_t i=0; i<norms.size(0); ++i) {
      double val = norms.index({ i }).item<double>();
      series_emb.emplace_back(X[(size_t)i], val);
    }
  } else {
    // Single component
    int64_t De = std::max<int64_t>(1, S.De);
    int64_t comp = (De > 0) ? ((S.enc_comp_idx % De + De) % De) : 0;
    auto compVec = E.index({ Slice(), comp }); // [B]
    for (int64_t i=0; i<compVec.size(0); ++i) {
      double val = compVec.index({ i }).item<double>();
      series_emb.emplace_back(X[(size_t)i], val);
    }
  }

  // Push into plot boxes
  auto pm = std::dynamic_pointer_cast<plotBox_data_t>(S.plot_main->data);
  auto pe = std::dynamic_pointer_cast<plotBox_data_t>(S.plot_emb->data);
  if (pm) {
    pm->series.resize(1);
    pm->series[0].swap(series_main);
    pm->opts.x_min = (double)left_key;  pm->opts.x_max = (double)right_key;
    pm->opts.y_label = std::string("feat[ch=") + std::to_string(ch) + "][t][" + std::to_string(dd) + "]"
                     + (S.normalize_main ? " (z)" : "");
  }
  if (pe) {
    pe->series.resize(1);
    pe->series[0].swap(series_emb);
    pe->opts.x_min = (double)left_key;  pe->opts.x_max = (double)right_key;
    if (S.enc_use_norm) {
      pe->opts.y_label = "||enc|| (";
      pe->opts.y_label += (E.dim()==2 ? "B,De" : "B,?,De");
      pe->opts.y_label += ")";
    } else {
      pe->opts.y_label = std::string("enc[") + std::to_string((long long)((S.enc_comp_idx % std::max<int64_t>(1,S.De) + S.De)%S.De)) + "]";
    }
  }

  // Sidebar info
  {
    std::ostringstream os;
    os << "Channels (K): "    << S.K << "   Selected: " << ch << "\n";
    os << "Feature dims (D): "<< S.D << "   Dim: "     << dd << "\n";
    os << "Past window (T): "<< S.max_N_past << "   Future (Tf): " << S.max_N_future << "\n";
    os << "Grid step: "      << fmt_double((double)S.step) << "\n";
    os << "Anchors: "        << S.num_records << "\n";
    os << "Window size: "    << win << "   Center idx: " << S.center_idx << "\n";
    os << "Keys: ["          << fmt_double((double)left_key) << ", "
                             << fmt_double((double)right_key) << "]\n";
    os << "Normalize main: " << (S.normalize_main ? "yes" : "no") << "\n";
    os << "Embedding: VICReg encode(past,mask) → ";
    if (enc.dim()==2) os << "[B,De]=" << E.size(0) << "," << E.size(1) << "\n";
    else if (enc.dim()==3) os << "[B,T',De]=" << enc.size(0) << "," << enc.size(1) << "," << enc.size(2)
                              << (S.enc_reduce_time_mean ? " (mean over T')" : " (last step)") << "\n";
    else os << "dim=" << enc.dim() << " (flattened)\n";
    os << "Enc view: " << (S.enc_use_norm ? "L2 norm" : ("component #" + std::to_string((long long)((S.enc_comp_idx % std::max<int64_t>(1,S.De) + S.De)%S.De)))) << "\n";
    auto tb = std::dynamic_pointer_cast<textBox_data_t>(S.info_panel->data);
    if (tb) tb->content = os.str();
  }
}

/* --------------------- ncurses bootstrap --------------------- */
static void init_curses(const std::string& bg_color) {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  // Safe now because libtorch was included first:
  timeout(40); // 40 ms getch() poll

  if (has_colors()) {
    start_color();
    cuwacunu::iinuji::set_global_background(bg_color);
  }
}

/* --------------------- main --------------------- */
int main(int argc, char** argv)
{
  // Load config
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  // Instrument
  std::string INSTRUMENT = (argc > 1 ? std::string(argv[1]) : std::string("BTCUSDT"));

  // Observation instruction from config
  auto obs_inst = cuwacunu::camahjucunu::decode_observation_instruction_from_config();

  const bool force_bin =
    cuwacunu::piaabo::dconfig::config_space_t::get<bool>("DATA_LOADER","dataloader_force_binarization");

  // Dataset_t (for time slicing / channels)
  auto inst_copy = INSTRUMENT; // API expects non-const lvalue
  Dataset_t dataset = cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<Datatype_t>(
      inst_copy, obs_inst, force_bin);

  // VICReg model (device from config)
  auto model_path = cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>("VICReg", "model_path");
  auto model_dev  = cuwacunu::piaabo::dconfig::config_device("VICReg");
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D representation_model(model_path, model_dev);

  // Probe shapes/metadata
  AppState S{ .dataset = std::move(dataset) };
  S.leftmost      = S.dataset.leftmost_key_value_;
  S.rightmost     = S.dataset.rightmost_key_value_;
  S.step          = S.dataset.key_value_step_;
  S.num_records   = S.dataset.num_records_;
  S.max_N_past    = (int64_t)S.dataset.max_N_past_;
  S.max_N_future  = (int64_t)S.dataset.max_N_future_;
  S.center_idx    = S.num_records / 2;

  // Datasample_t once to recover K and D
  try {
    kvalue_t mid_key = S.leftmost + static_cast<kvalue_t>(S.center_idx) * S.step;
    Datasample_t s = S.dataset.get_by_key_value(mid_key);
    if (!s.features.defined() || s.features.dim() != 3) {
      std::fprintf(stderr, "Unexpected sample feature shape; need [C,T,D]\n");
      return 2;
    }
    S.K = s.features.size(0);
    S.D = s.features.size(2);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Failed to probe dataset: %s\n", e.what());
    return 2;
  }

  // Wire model into state
  S.rep = &representation_model;
  S.rep_device = model_dev;
  S.use_swa = true;

  // Init ncurses + renderer
  init_curses(S.bg_color);
  cuwacunu::iinuji::NcursesRend rend;
  cuwacunu::iinuji::set_renderer(&rend);

  // Build UI tree
  build_ui(S);

  // Main loop
  bool running = true;
  while (running) {
    int H=0, W=0;
    rend.size(H, W);

    // Layout and update data
    cuwacunu::iinuji::layout_tree(S.root, cuwacunu::iinuji::Rect{0,0,W,H});
    update_plots(S);

    // Render
    rend.clear();
    cuwacunu::iinuji::render_tree(S.root);
    rend.flush();

    // Input
    int ch = getch();
    switch (ch) {
      case 'q': running = false; break;

      case KEY_LEFT: {
        std::size_t stride = std::max<std::size_t>(1, S.window_sizes[S.window_idx] / 8);
        S.center_idx = (S.center_idx > stride) ? (S.center_idx - stride) : 0;
      } break;
      case KEY_RIGHT: {
        std::size_t stride = std::max<std::size_t>(1, S.window_sizes[S.window_idx] / 8);
        S.center_idx = std::min<std::size_t>(S.center_idx + stride, (S.num_records? S.num_records-1 : 0));
      } break;
      case 'g': {
        std::size_t stride = std::max<std::size_t>(1, (S.window_sizes[S.window_idx] / 8) * 10);
        S.center_idx = (S.center_idx > stride) ? (S.center_idx - stride) : 0;
      } break;
      case 'G': {
        std::size_t stride = std::max<std::size_t>(1, (S.window_sizes[S.window_idx] / 8) * 10);
        S.center_idx = std::min<std::size_t>(S.center_idx + stride, (S.num_records? S.num_records-1 : 0));
      } break;

      case KEY_UP:   if (S.window_idx > 0) S.window_idx--; break;       // zoom in
      case KEY_DOWN: if (S.window_idx+1 < (int)S.window_sizes.size()) S.window_idx++; break; // zoom out

      case '1': S.window_idx = 0; break;
      case '2': if (S.window_sizes.size()>1) S.window_idx = 1; break;
      case '3': if (S.window_sizes.size()>2) S.window_idx = 2; break;
      case '4': if (S.window_sizes.size()>3) S.window_idx = 3; break;
      case '5': if (S.window_sizes.size()>4) S.window_idx = 4; break;

      case 'c': S.ch_idx = (S.ch_idx + 1) % std::max<int64_t>(1, S.K); break;
      case 'C': S.ch_idx = (S.ch_idx + S.K - 1) % std::max<int64_t>(1, S.K); break;

      case 'd': S.dim_idx = (S.dim_idx + 1) % std::max<int64_t>(1, S.D); break;
      case 'D': S.dim_idx = (S.dim_idx + S.D - 1) % std::max<int64_t>(1, S.D); break;

      case 'n': S.normalize_main = !S.normalize_main; break;

      case 'm': S.enc_use_norm = !S.enc_use_norm; break;
      case 'e': S.enc_comp_idx += 1; break;
      case 'E': S.enc_comp_idx -= 1; break;
      case 't': S.enc_reduce_time_mean = !S.enc_reduce_time_mean; break;

      case KEY_RESIZE:
      default: break;
    }
  }

  // Teardown
  cuwacunu::iinuji::set_renderer(nullptr);
  endwin();
  return 0;
}
