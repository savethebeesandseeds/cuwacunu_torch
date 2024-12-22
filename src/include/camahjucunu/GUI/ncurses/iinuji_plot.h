/* iinuji_plot.h */
#include <ncursesw/ncurses.h>
#include <locale.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace cuwacunu {
namespace camahjucunu {
namespace GUI {

static int dot_bit_index(int sub_x, int sub_y) {
  /* 
    Mapping (sub_x, sub_y) -> dot bit 
      (0,0)=dot1=0x01, (1,0)=dot4=0x08,
      (0,1)=dot2=0x02, (1,1)=dot5=0x10,
      (0,2)=dot3=0x04, (1,2)=dot6=0x20,
      (0,3)=dot7=0x40, (1,3)=dot8=0x80
  */
  static const int dot_map[4][2] = {
    {0x01,0x08}, /* row0 col0=dot1, col1=dot4 */
    {0x02,0x10}, /* row1 col0=dot2, col1=dot5 */
    {0x04,0x20}, /* row2 col0=dot3, col1=dot6 */
    {0x40,0x80}  /* row3 col0=dot7, col1=dot8 */
  };
  return dot_map[sub_y][sub_x];
}

void plot_braille(const std::vector<std::pair<double, double>> &points, int start_x, int start_y, int width, int height, int density = 10) {
  if (width <= 0 || height <= 0) {
    fprintf(stderr, "(iinuji_plot)[plot_braille] Width and height must be greater than zero.\n");
    return;
  }
  /* Create a buffer for braille cells */
  std::vector<std::vector<unsigned char>> braille_cells(height, std::vector<unsigned char>(width, 0));

  /* Determine data range */
  double x_min = points.front().first;
  double x_max = points.back().first;
  double y_min = points.front().second;
  double y_max = points.front().second;
  for (auto &p: points) {
    x_min = std::min(x_min, p.first);
    x_max = std::max(x_max, p.first);
    y_min = std::min(y_min, p.second);
    y_max = std::max(y_max, p.second);
  }

  /* Function to safely set a dot in braille_cells */
  auto set_dot = [&](int px, int py) {
    if (px < 0 || py < 0) return;
    int cell_x = px / 2;
    int cell_y = py / 4;
    if (cell_x < 0 || cell_x >= width || cell_y < 0 || cell_y >= height) return;

    int sub_x = px % 2;
    int sub_y = py % 4;
    int bit = dot_bit_index(sub_x, sub_y);
    braille_cells[cell_y][cell_x] |= (unsigned char)bit;
    braille_cells[cell_y][cell_x] &= 0xFF; /* Mask to valid 8-bit Braille pattern. */
  };

  /* Rasterize the line segments */
  for (size_t i = 1; i < points.size(); i++) {
    double x1 = points[i-1].first;
    double y1 = points[i-1].second;
    double x2 = points[i].first;
    double y2 = points[i].second;

    /* Increase density for smoother lines */
    for (int s = 0; s <= density; s++) {
      double t = (double)s / density;
      double x = x1 + (x2 - x1)*t;
      double y = y1 + (y2 - y1)*t;

      int px = (int)std::round(( (x - x_min) / (x_max - x_min) ) * (width * 2 - 1));
      int py = (int)std::round(( (y - y_min) / (y_max - y_min) ) * (height * 4 - 1));

      set_dot(px, py);
    }
  }

  /* Draw the braille cells */
  for (int row = 0; row < height; row++) {
    move(start_y + row, start_x);
    for (int col = 0; col < width; col++) {
      wchar_t ch = (wchar_t)(0x2800 + braille_cells[row][col]);
      cchar_t cchar;
      setcchar(&cchar, &ch, A_NORMAL, 0, NULL);
      add_wch(&cchar);
    }
  }
}

} /* namespace GUI */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
