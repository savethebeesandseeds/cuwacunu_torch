/* iinuji_render.h */
#include <ncursesw/ncurses.h>
#include <locale.h>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <algorithm>
#include <cwchar> // For wide characters

#include "camahjucunu/GUI/ncurses/iinuji_plot.h"

namespace cuwacunu {
namespace camahjucunu {
namespace GUI {

// Function to convert normalized values to screen units
iinuji_layout_t normalize_to_screen(const iinuji_layout_t& layout) {
    // Get the screen dimensions
    int screen_width, screen_height;
    getmaxyx(stdscr, screen_height, screen_width);

    // If the layout is normalized, scale coordinates and dimensions
    if(layout.normalized) {
        return {
            layout.x * screen_width,
            layout.y * screen_height,
            layout.width * screen_width,
            layout.height * screen_height,
            false // The resulting layout is no longer normalized
        };
    }

    // If already in screen units, return as-is
    return layout;
}


void render_border(const iinuji_object_t& obj) {
    if(obj.style.border == false) {
        return;
    }
    // Convert normalized coordinates and dimensions to screen units
    iinuji_layout_t layout = normalize_to_screen(obj.layout);

    int x       = static_cast<int>(layout.x);
    int y       = static_cast<int>(layout.y);
    int width   = static_cast<int>(layout.width);
    int height  = static_cast<int>(layout.height);

    // Define reusable cchar_t variables
    cchar_t top_border, bottom_border, left_border, right_border;
    cchar_t top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner;

    // Set the color pair for the object's style
    int color_pair = get_color_pair(obj.style.border_color, obj.style.background_color);
    attron(COLOR_PAIR(color_pair));

    // Initialize cchar_t for borders
    setcchar(&top_border, L"─", A_NORMAL, color_pair, nullptr);
    setcchar(&bottom_border, L"─", A_NORMAL, color_pair, nullptr);
    setcchar(&left_border, L"│", A_NORMAL, color_pair, nullptr);
    setcchar(&right_border, L"│", A_NORMAL, color_pair, nullptr);
    setcchar(&top_left_corner, L"┌", A_NORMAL, color_pair, nullptr);
    setcchar(&top_right_corner, L"┐", A_NORMAL, color_pair, nullptr);
    setcchar(&bottom_left_corner, L"└", A_NORMAL, color_pair, nullptr);
    setcchar(&bottom_right_corner, L"┘", A_NORMAL, color_pair, nullptr);

    // Render the borders
    for (int col = 1; col < width - 1; ++col) {
        mvadd_wch(y, x + col, &top_border); // Top border
        mvadd_wch(y + height - 1, x + col, &bottom_border); // Bottom border
    }
    for (int row = 1; row < height - 1; ++row) {
        mvadd_wch(y + row, x, &left_border); // Left border
        mvadd_wch(y + row, x + width - 1, &right_border); // Right border
    }
    mvadd_wch(y, x, &top_left_corner); // Top-left corner
    mvadd_wch(y, x + width - 1, &top_right_corner); // Top-right corner
    mvadd_wch(y + height - 1, x, &bottom_left_corner); // Bottom-left corner
    mvadd_wch(y + height - 1, x + width - 1, &bottom_right_corner); // Bottom-right corner

    attroff(COLOR_PAIR(color_pair));
}


void render_iinuji_textBox(const iinuji_object_t& obj) {
    // Retrive the text
    auto content = std::dynamic_pointer_cast<textBox_data_t>(obj.data)->content;
    
    // Convert normalized coordinates and dimensions to screen units
    iinuji_layout_t layout = normalize_to_screen(obj.layout);

    int start_x = static_cast<int>(obj.style.border ? layout.x + 1 : layout.x);
    int start_y = static_cast<int>(obj.style.border ? layout.y + 1 : layout.y);
    int width   = static_cast<int>(obj.style.border ? layout.width - 2 : layout.width);
    int height  = static_cast<int>(obj.style.border ? layout.height - 2 : layout.height);

    // Set the color pair for the object's style
    int color_pair = get_color_pair(obj.style.label_color, obj.style.background_color);
    attron(COLOR_PAIR(color_pair));

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if(row == 0 && col < static_cast<int>(content.size())) {
                mvaddch(start_y + row, start_x + col, content[col] | COLOR_PAIR(color_pair));
            } else {
                mvaddch(start_y + row, start_x + col, ' ' | COLOR_PAIR(color_pair));
            }
        }
    }

    attroff(COLOR_PAIR(color_pair));
}


void render_iinuji_panelBox(const iinuji_object_t& obj) {
    // Convert normalized coordinates and dimensions to screen units
    iinuji_layout_t layout = normalize_to_screen(obj.layout);

    int start_x = static_cast<int>(obj.style.border ? layout.x + 1 : layout.x);
    int start_y = static_cast<int>(obj.style.border ? layout.y + 1 : layout.y);
    int width   = static_cast<int>(obj.style.border ? layout.width - 2 : layout.width);
    int height  = static_cast<int>(obj.style.border ? layout.height - 2 : layout.height);

    // Set the color pair for the object's style
    int color_pair = get_color_pair(obj.style.label_color, obj.style.background_color);
    attron(COLOR_PAIR(color_pair));

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            mvaddch(start_y + row, start_x + col, ' ' | COLOR_PAIR(color_pair));
        }
    }

    attroff(COLOR_PAIR(color_pair));
}


void render_iinuji_plotBox(const iinuji_object_t& obj) {
    // Retrive the data
    auto points = std::dynamic_pointer_cast<plotBox_data_t>(obj.data)->points;
    auto density = std::dynamic_pointer_cast<plotBox_data_t>(obj.data)->density;
    
    // Convert normalized coordinates and dimensions to screen units
    iinuji_layout_t layout = normalize_to_screen(obj.layout);

    int start_x = static_cast<int>(obj.style.border ? layout.x + 1 : layout.x);
    int start_y = static_cast<int>(obj.style.border ? layout.y + 1 : layout.y);
    int width   = static_cast<int>(obj.style.border ? layout.width - 2 : layout.width);
    int height  = static_cast<int>(obj.style.border ? layout.height - 2 : layout.height);

    // Set the color pair for the object's style
    int color_pair = get_color_pair(obj.style.label_color, obj.style.background_color);
    attron(COLOR_PAIR(color_pair));

    // Render as a plot
    plot_braille(points, start_x, start_y, width, height, density);
    
    attroff(COLOR_PAIR(color_pair));
}



void render_iinuji_object(std::shared_ptr<iinuji_object_t>& obj) {
    if(!obj || !obj->visible) {
        return; // Skip rendering ifthe object is null or not visible
    }

    // Render the border ifenabled
    render_border(*obj);

    // Render the content
    if(std::dynamic_pointer_cast<plotBox_data_t>(obj->data)) {
        render_iinuji_plotBox(*obj);
    } else if(std::dynamic_pointer_cast<textBox_data_t>(obj->data)) {
        render_iinuji_textBox(*obj);
    } else {
        render_iinuji_panelBox(*obj); // Fallback rendering
    }

    // Recursively render children
    for (auto& child : obj->children) {
        render_iinuji_object(child);
    }
}


} /* namespace GUI */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

