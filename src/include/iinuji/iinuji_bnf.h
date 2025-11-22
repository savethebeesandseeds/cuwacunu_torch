/* iinuji_bnf.h : Interpret iinuji_renderings (BNF) into iinuji UI + render */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <limits>
#include <cmath>
#include <algorithm>

#include "iinuji/iinuji_rend.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"

/* Parsed spec produced by BNF implementation */
#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

namespace cuwacunu {
namespace iinuji {

/* ────────────────────────────────────────────────────────────────────────────
   Data hooks for ArgN → data.
   Implement these to feed real series / scatter / embeddings / bands.
   All methods return false if data is unavailable.
   ──────────────────────────────────────────────────────────────────────────── */
struct IRenderingsData {
  virtual ~IRenderingsData() = default;

  /* For "curve d N" : return (x,y) series for channel d. */
  virtual bool series(const std::string& arg_name,
                      int d_index,
                      std::vector<std::pair<double,double>>& out) {
    (void)arg_name; (void)d_index; (void)out; return false;
  }

  /* For "mask_scatter d N" : return scatter points for channel d. */
  virtual bool mask_scatter(const std::string& arg_name,
                            int d_index,
                            std::vector<std::pair<double,double>>& out) {
    (void)arg_name; (void)d_index; (void)out; return false;
  }

  /* For "embedding ..." : return 2D (x,y) points. */
  virtual bool embedding(const std::string& arg_name,
                         std::vector<std::pair<double,double>>& out) {
    (void)arg_name; (void)out; return false;
  }

  /* For "mdn_band y N" : return per-x low/high envelopes for y index. */
  virtual bool mdn_band(const std::string& arg_name,
                        int y_index,
                        std::vector<double>& xs,
                        std::vector<double>& y_low,
                        std::vector<double>& y_high) {
    (void)arg_name; (void)y_index; (void)xs; (void)y_low; (void)y_high; return false;
  }

  /* Optional text retrieval (presenters etc.). */
  virtual bool text_for(const std::string& arg_name,
                        const std::string& key,
                        std::string& out) {
    (void)arg_name; (void)key; (void)out; return false;
  }
};

/* Functional adapter (convenience if prefer lambdas over subclassing) */
struct RenderingsDataFn : IRenderingsData {
  std::function<bool(const std::string&,int,std::vector<std::pair<double,double>>&)> series_fn;
  std::function<bool(const std::string&,int,std::vector<std::pair<double,double>>&)> mask_fn;
  std::function<bool(const std::string&,std::vector<std::pair<double,double>>&)>     embed_fn;
  std::function<bool(const std::string&,int,std::vector<double>&,std::vector<double>&,std::vector<double>&)> mdn_fn;
  std::function<bool(const std::string&,const std::string&,std::string&)>            text_fn;

  bool series(const std::string& a,int d,std::vector<std::pair<double,double>>& o) override {
    return series_fn ? series_fn(a,d,o) : IRenderingsData::series(a,d,o);
  }
  bool mask_scatter(const std::string& a,int d,std::vector<std::pair<double,double>>& o) override {
    return mask_fn ? mask_fn(a,d,o) : IRenderingsData::mask_scatter(a,d,o);
  }
  bool embedding(const std::string& a,std::vector<std::pair<double,double>>& o) override {
    return embed_fn ? embed_fn(a,o) : IRenderingsData::embedding(a,o);
  }
  bool mdn_band(const std::string& a,int y,std::vector<double>& xs,std::vector<double>& lo,std::vector<double>& hi) override {
    return mdn_fn ? mdn_fn(a,y,xs,lo,hi) : IRenderingsData::mdn_band(a,y,xs,lo,hi);
  }
  bool text_for(const std::string& a,const std::string& k,std::string& o) override {
    return text_fn ? text_fn(a,k,o) : IRenderingsData::text_for(a,k,o);
  }
};

/* ────────────────────────────────────────────────────────────────────────────
   Internal helpers
   ──────────────────────────────────────────────────────────────────────────── */
inline const cuwacunu::camahjucunu::iinuji_renderings_instruction_t::arg_t*
find_arg(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t::screen_t& sc,
         const std::string& name) {
  for (const auto& a : sc.args) if (a.name == name) return &a;
  return nullptr;
}

inline void symmetric_bounds_from_points(const std::vector<std::pair<double,double>>& pts,
                                         double& xmin, double& xmax,
                                         double& ymin, double& ymax) {
  xmin = ymin =  std::numeric_limits<double>::infinity();
  xmax = ymax = -std::numeric_limits<double>::infinity();
  for (const auto& p : pts) {
    xmin = std::min(xmin, p.first);
    xmax = std::max(xmax, p.first);
    ymin = std::min(ymin, p.second);
    ymax = std::max(ymax, p.second);
  }
  if (!std::isfinite(xmin) || !std::isfinite(xmax) || !std::isfinite(ymin) || !std::isfinite(ymax)) {
    xmin = 0; xmax = 1; ymin = 0; ymax = 1;
    return;
  }
  const double m = std::max({ std::fabs(xmin), std::fabs(xmax), std::fabs(ymin), std::fabs(ymax), 1e-9 });
  xmin = -m; xmax =  m; ymin = -m; ymax =  m;
}

/* Build a single panel object (plot/text) from spec + data */
inline std::shared_ptr<iinuji_object_t>
build_panel(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t::panel_t& P,
            const cuwacunu::camahjucunu::iinuji_renderings_instruction_t::screen_t& sc,
            IRenderingsData& data)
{
  using Spec = cuwacunu::camahjucunu::iinuji_renderings_instruction_t;

  // Absolute layout in terminal cells
  iinuji_layout_t lay{};
  lay.mode = layout_mode_t::Absolute;
  lay.normalized = false;
  lay.x = P.x; lay.y = P.y;
  lay.width  = std::max(0, P.w);
  lay.height = std::max(0, P.h);

  iinuji_style_t sty{};
  sty.border = true;
  sty.title  = P.id;

  auto obj = create_object(P.id, /*visible*/true, lay, sty);
  obj->z_index = P.z;

  // TEXT panel
  if (P.type == "text") {
    std::string content;
    for (const auto& shp : P.shapes) {
      if (shp.kind == Spec::shape_t::kind_e::Text) {
        if (auto it = shp.kv.find("content"); it != shp.kv.end()) {
          if (!content.empty()) content.push_back(' ');
          content += it->second;
        }
      }
    }
    obj->data = std::make_shared<textBox_data_t>(content, /*wrap*/true, text_align_t::Left);
    return obj;
  }

  // Otherwise: plot-like panel (plot / embed / mdn / custom)
  std::vector<std::vector<std::pair<double,double>>> series;
  std::vector<plot_series_cfg_t> cfg;
  plotbox_opts_t opts;
  opts.draw_grid = true;
  opts.draw_axes = true;

  const std::string arg = P.bind_arg;
  (void)find_arg(sc, arg); // placeholder for future presenter-aware decisions

  auto push_series = [&](std::vector<std::pair<double,double>>&& pts,
                         plot_mode_t mode,
                         const std::string& color = "#C8C8C8",
                         bool scatter=false,
                         std::function<void(plot_series_cfg_t&)> tune = nullptr) {
    series.emplace_back(std::move(pts));
    plot_series_cfg_t c;
    c.color_fg = color;
    c.mode = mode;
    c.scatter = scatter;
    if (tune) tune(c);
    cfg.push_back(c);
  };

  for (const auto& shp : P.shapes) {
    using K = Spec::shape_t::kind_e;
    switch (shp.kind) {
      case K::Curve: {
        int d = 0; if (auto it=shp.kv.find("d"); it!=shp.kv.end()) { try { d = std::stoi(it->second); } catch(...){} }
        std::vector<std::pair<double,double>> pts;
        if (data.series(arg, d, pts)) {
          push_series(std::move(pts), plot_mode_t::Line, "#90CAF9" /* blue-ish */);
        }
      } break;

      case K::MaskScatter: {
        int d = 0; if (auto it=shp.kv.find("d"); it!=shp.kv.end()) { try { d = std::stoi(it->second); } catch(...){} }
        std::vector<std::pair<double,double>> pts;
        if (data.mask_scatter(arg, d, pts)) {
          push_series(std::move(pts), plot_mode_t::Scatter, "#FFCC80", true);
        }
      } break;

      case K::Embedding: {
        std::vector<std::pair<double,double>> pts;
        if (data.embedding(arg, pts)) {
          bool grid=false, symmetric=false;
          if (auto it=shp.kv.find("grid"); it!=shp.kv.end()) grid      = (it->second=="true"||it->second=="1");
          if (auto it=shp.kv.find("symmetric"); it!=shp.kv.end()) symmetric = (it->second=="true"||it->second=="1");
          push_series(std::move(pts), plot_mode_t::Scatter, "#E0E0E0", true);
          opts.draw_grid = grid;
          if (symmetric && !series.empty()) {
            double xmin,xmax,ymin,ymax; symmetric_bounds_from_points(series.back(), xmin,xmax,ymin,ymax);
            opts.x_min = xmin; opts.x_max = xmax; opts.y_min = ymin; opts.y_max = ymax;
          }
        }
      } break;

      case K::MdnBand: {
        int yidx = 0; if (auto it=shp.kv.find("y"); it!=shp.kv.end()) { try { yidx = std::stoi(it->second); } catch(...){} }
        std::vector<double> xs, lo, hi;
        if (data.mdn_band(arg, yidx, xs, lo, hi) && xs.size()==lo.size() && lo.size()==hi.size() && !xs.empty()) {
          // Build one series with both low and high per x; envelope overlay fills vertical span.
          std::vector<std::pair<double,double>> pts; pts.reserve(xs.size()*2);
          for (size_t i=0;i<xs.size();++i) { pts.emplace_back(xs[i], lo[i]); pts.emplace_back(xs[i], hi[i]); }
          push_series(std::move(pts), plot_mode_t::Line, "#80CBC4" /* teal-ish */, false, [&](plot_series_cfg_t& c){
            c.envelope_enabled = true;
            c.envelope_source  = envelope_source_t::OriginalSamples;
            c.envelope_min_count  = 2;
            c.envelope_min_height = 1;
            c.envelope_draw_base  = false; // paint band only
          });
        }
      } break;

      case K::Text:
        // In non-text panels, ignore for now or add tiny overlay labels as extra children.
        break;
    }
  }

  if (series.empty()) {
    // fallback: simple label to make the panel visible
    obj->data = std::make_shared<textBox_data_t>(std::string("empty: ") + P.id, true, text_align_t::Left);
    return obj;
  }

  auto pb = std::make_shared<plotBox_data_t>();
  pb->series = std::move(series);
  pb->series_cfg = std::move(cfg);
  pb->opts = opts;
  obj->data = pb;

  return obj;
}

/* Build a full UI tree for a specific screen index */
inline std::shared_ptr<iinuji_object_t>
build_ui_for_screen(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& spec,
                    std::size_t screen_index,
                    IRenderingsData& data,
                    int term_cols,
                    int term_rows)
{
  if (screen_index >= spec.screens.size()) return nullptr;
  const auto& sc = spec.screens[screen_index];

  auto root = create_object("root");
  root->layout.mode = layout_mode_t::Absolute;
  root->layout.normalized = false;
  root->layout.x = 0; root->layout.y = 0;
  root->layout.width  = std::max(0, term_cols);
  root->layout.height = std::max(0, term_rows);
  root->style.background_color = "black";
  root->style.label_color = "white";

  for (const auto& P : sc.panels) {
    if (auto child = build_panel(P, sc, data)) root->add_child(child);
  }
  return root;
}

/* Convenience: query renderer size, layout, render in one call */
inline void materialize_and_render_once(const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& spec,
                                        std::size_t screen_index,
                                        IRenderingsData& data)
{
  auto* R = get_renderer();
  if (!R) return;
  int rows=0, cols=0; R->size(rows, cols);
  auto root = build_ui_for_screen(spec, screen_index, data, cols, rows);
  if (!root) return;
  layout_tree(root, Rect{0,0,cols,rows});
  render_tree(root);
}

} // namespace iinuji
} // namespace cuwacunu
