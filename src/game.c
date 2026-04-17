#include "game.h"

// A simple function to demonstrate calling a C function from NASM assembly
void helloWorld() {
    Game game = {"Danger Dash", "0.0.1-alpha", 2024};
    initscr(); curs_set(0); noecho();

    WINDOW *wlcm_win = newwin(10, 2 * COLS / 3, 0, 0);
    mvwin(wlcm_win, (LINES - getmaxy(wlcm_win)) / 2, (COLS - getmaxx(wlcm_win)) / 2);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
    }

    wattron(wlcm_win, COLOR_PAIR(1));
    mvwprintw(wlcm_win, getmaxy(wlcm_win)/2 - 1, (getmaxx(wlcm_win) - strlen(WELCOME_MSG)) / 2, "%s", WELCOME_MSG);
    wattroff(wlcm_win, COLOR_PAIR(1));

    char game_info[100];
    snprintf(game_info, sizeof(game_info), "%s %s %d", game.name, game.version, game.year_of_release);
    wattron(wlcm_win, COLOR_PAIR(2));
    mvwprintw(wlcm_win, getmaxy(wlcm_win)/2 + 1, (getmaxx(wlcm_win) - strlen(game_info)) / 2, "%s", game_info);
    wattroff(wlcm_win, COLOR_PAIR(2));

    refresh();
    wrefresh(wlcm_win);
    
    getch();

    delwin(wlcm_win);
    endwin();
}

// You can add more game-related functions here that can be called from assembly or other C files