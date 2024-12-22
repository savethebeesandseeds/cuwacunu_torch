#include <ncurses.h>
#include "camahjucunu/GUI/ncurses/iinuji_types.h"
#include "camahjucunu/GUI/ncurses/iinuji_utils.h"
#include "camahjucunu/GUI/ncurses/iinuji_render.h"

// Initialize color map
std::map<std::string, int> cuwacunu::camahjucunu::GUI::color_map;

int main() {
    
    // Initialize ncurses
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    use_default_colors();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    // Define colors
    cuwacunu::camahjucunu::GUI::get_color("dim_red", 1000, 0, 0, 0.3f);
    cuwacunu::camahjucunu::GUI::get_color("dim_green", 0, 1000, 0, 0.1f);
    cuwacunu::camahjucunu::GUI::get_color("dim_blue", 0, 0, 1000, 0.3f);
    cuwacunu::camahjucunu::GUI::get_color("dim_yellow", 1000, 1000, 0, 0.3f);
    cuwacunu::camahjucunu::GUI::get_color("bright_white", 1000, 1000, 1000, 1.0f);
    cuwacunu::camahjucunu::GUI::get_color("dim_black", 0, 0, 0, 0.0f);

    // Set global background
    cuwacunu::camahjucunu::GUI::set_global_background("dim_green");

    // Create root object
    auto root = cuwacunu::camahjucunu::GUI::create_iinuji_panelBox(
        true, 
        { .x = 0.0, .y = 0.0, .width = 0.5, .height = 0.5, .normalized = true },
        { .label_color = "bright_white", .background_color = "dim_green", .border = false, .border_color = "bright_white" }
    );

    // Create a box as a child
    auto box = cuwacunu::camahjucunu::GUI::create_iinuji_textBox(
        "Box!", 
        { .x = 0.25, .y = 0.25, .width = 0.1, .height = 0.1, .normalized = true },
        { .label_color = "bright_white", .background_color = "dim_green", .border = true, .border_color = "bright_white" }
    );

    // Create a plot as a child
    int density = 1;
    std::vector<std::pair<double, double>> points;
    for (double x = 0; x <= 2 * M_PI; x += 0.05) {
        points.emplace_back(x, sin(x));
    }

    auto plot = cuwacunu::camahjucunu::GUI::create_iinuji_plotBox(
        points, 
        density, 
        { .x = 0.05, .y = 0.05, .width = 0.25, .height = 0.1, .normalized = true },
        { .label_color = "bright_white", .background_color = "dim_green", .border = true, .border_color = "bright_white" }
    );

    // Initialize state
    root->add_child(plot);
    root->add_child(box);
    auto state = cuwacunu::camahjucunu::GUI::initialize_iinuji_state(root);

    // Main render loop
    while (state->running) {
        clear();
        cuwacunu::camahjucunu::GUI::render_iinuji_object(state->root); // Render root and all children
        refresh();

        int ch = getch();
        if (ch == 'q') {
            state->running = false;
        }
    }

    // Cleanup
    endwin();
    return 0;
}
