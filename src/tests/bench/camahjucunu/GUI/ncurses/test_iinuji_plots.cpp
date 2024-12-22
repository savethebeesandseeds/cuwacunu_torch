#include <ncursesw/ncurses.h>
#include <locale.h>
#include <vector>
#include <cmath>
#include <algorithm>

#include "camahjucunu/GUI/ncurses/iinuji_plot.h"

int main() {
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);
    clear();

    int width, height;
    getmaxyx(stdscr, height, width);

    // Example data: sine wave
    std::vector<std::pair<double, double>> points;
    for (double x = 0; x <= 2 * M_PI; x += 0.05) {
        points.emplace_back(x, sin(x));
    }

    // Plot the braille visualization starting at position (0, 0)
    cuwacunu::camahjucunu::GUI::plot_braille(
        /* points */        points, 
        /* start_x */       0, 
        /* start_y */       0, 
        /* plot width*/     width, 
        /* plot height */   height, 
        /* density */       4
    );

    refresh();
    getch();
    endwin();
    return 0;
}
