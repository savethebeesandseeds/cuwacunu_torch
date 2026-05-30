#include <ncurses.h>
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define LOG_FILE stderr

bool in_ncurses_mode = true; // Tracks current mode (true: ncurses, false: terminal)

// Function to set terminal to raw mode for key capture in terminal mode
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to reset terminal to original mode
void disable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ICANON | ECHO); // Re-enable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to switch to terminal output mode
void switch_to_terminal_mode() {
    if (in_ncurses_mode) {
        endwin(); // Exit ncurses mode
        fprintf(LOG_FILE, "Switched to terminal output mode.\n");
        fflush(LOG_FILE);
        enable_raw_mode(); // Enable raw input mode for terminal
        in_ncurses_mode = false;
    }
}

// Function to switch back to ncurses mode
void switch_to_ncurses_mode() {
    if (!in_ncurses_mode) {
        disable_raw_mode(); // Restore terminal mode before switching
        initscr(); // Reinitialize ncurses
        cbreak();
        noecho();
        clear();
        refresh();
        mvprintw(0, 0, "Switched back to ncurses mode.");
        refresh();
        in_ncurses_mode = true;
    }
}

// Function to get key input directly in terminal mode
int get_key_in_terminal() {
    int ch;
    ch = getchar(); // Read a single character from stdin
    return ch;
}

// Main loop to handle input
void main_loop() {
    int ch;
    while (1) {
        if (in_ncurses_mode) {
            ch = getch(); // Use ncurses input
        } else {
            ch = get_key_in_terminal(); // Use raw input
        }

        if (ch == 'q') { // Exit on 'q'
            break;
        } else if (ch == '\t') { // Handle Tab key (ASCII 9)
            if (in_ncurses_mode) {
                switch_to_terminal_mode();
            } else {
                switch_to_ncurses_mode();
            }
        } else if (in_ncurses_mode) {
            mvprintw(1, 0, "Key pressed: %d", ch);
            refresh();
        } else {
            fprintf(LOG_FILE, "Key pressed in terminal mode: %d\n", ch);
            fflush(LOG_FILE);
        }
    }
}

int main() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();

    // Display initial ncurses screen
    mvprintw(0, 0, "Welcome to the ncurses application!");
    mvprintw(1, 0, "Press Tab to toggle view, 'q' to quit.");
    refresh();

    // Run main loop
    main_loop();

    // Clean up ncurses and terminal modes
    if (!in_ncurses_mode) {
        disable_raw_mode();
    }
    endwin();
    return 0;
}
