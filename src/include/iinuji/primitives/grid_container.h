/* primitives/grid_container.h */
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

struct grid_container_opts_t {
  std::vector<len_spec> rows{};
  std::vector<len_spec> cols{};
  int gap_row{0};
  int gap_col{0};
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::None};
};

inline std::shared_ptr<iinuji_object_t>
create_grid_container(const std::string &id, grid_container_opts_t opts = {}) {
  auto object = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style),
      primitive_role_t::Container, opts.focus_mode);
  object->grid = std::make_shared<grid_spec_t>();
  object->grid->rows = std::move(opts.rows);
  object->grid->cols = std::move(opts.cols);
  object->grid->gap_row = opts.gap_row;
  object->grid->gap_col = opts.gap_col;
  return object;
}

inline std::shared_ptr<iinuji_object_t>
create_grid_container(const std::string &id, const std::vector<len_spec> &rows,
                      const std::vector<len_spec> &cols, int gap_row = 0,
                      int gap_col = 0, const iinuji_layout_t &layout = {},
                      const iinuji_style_t &style = {},
                      focus_mode_t focus_mode = focus_mode_t::None) {
  grid_container_opts_t opts{};
  opts.rows = rows;
  opts.cols = cols;
  opts.gap_row = gap_row;
  opts.gap_col = gap_col;
  opts.layout = layout;
  opts.style = style;
  opts.focus_mode = focus_mode;
  return create_grid_container(id, std::move(opts));
}

inline void place_in_grid(std::shared_ptr<iinuji_object_t> child, int row,
                          int col, int row_span = 1, int col_span = 1) {
  if (!child)
    return;
  child->layout.mode = layout_mode_t::GridCell;
  child->layout.grid_row = row;
  child->layout.grid_col = col;
  child->layout.grid_row_span = row_span;
  child->layout.grid_col_span = col_span;
}

} // namespace iinuji
} // namespace cuwacunu
