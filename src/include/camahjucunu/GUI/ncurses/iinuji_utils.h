/* iinuji_utils.h */
#include <ncursesw/ncurses.h>
#include <locale.h>

#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <algorithm>
#include <map>

namespace cuwacunu {
namespace camahjucunu {
namespace GUI {

/* --- --- --- --- --- --- --- --- --- --- */
/*      Util Variables                     */
/* --- --- --- --- --- --- --- --- --- --- */

extern std::map<std::string, int> color_map;

/* --- --- --- --- --- --- --- --- --- --- */
/*      Color utility functions            */
/* --- --- --- --- --- --- --- --- --- --- */

int get_color(const std::string& color_name, int r, int g, int b, float dim_factor = 1.0f) {
    if (color_map.find(color_name) == color_map.end()) {
        static int next_color_id = 16; // Start with extended colors
        int dim_r = static_cast<int>(r * dim_factor);
        int dim_g = static_cast<int>(g * dim_factor);
        int dim_b = static_cast<int>(b * dim_factor);
        init_color(next_color_id, dim_r, dim_g, dim_b);
        color_map[color_name] = next_color_id++;
    }
    return color_map[color_name];
}

int get_color_pair(const std::string& label_color, const std::string& background_color) {
    static std::map<std::pair<int, int>, int> color_pairs;
    static int next_pair_id = 1;
    int label_color_id = get_color(label_color, 1000, 1000, 1000);     // Default white
    int background_color_id = get_color(background_color, 0, 0, 1000); // Default blue
    auto key = std::make_pair(label_color_id, background_color_id);
    if (color_pairs.find(key) == color_pairs.end()) {
        init_pair(next_pair_id, label_color_id, background_color_id);
        color_pairs[key] = next_pair_id++;
    }
    return color_pairs[key];
}

void set_global_background(const std::string& background_color) {
    int bg_pair = get_color_pair("white", background_color);
    bkgd(' ' | COLOR_PAIR(bg_pair));
}


} /* namespace GUI */
} /* namespace camahjucunu */
} /* namespace cuwacunu */