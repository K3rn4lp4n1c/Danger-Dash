#include "game.h"

// Initializes the game state, resources, etc.
Game* init() {
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    srand(time(NULL));

    WINDOW *wstatus = newwin(LINES/3, COLS, 0, 0);
    box(wstatus, 0, 0);
    
    WINDOW *wgame = newwin(LINES/3, COLS, LINES / 3, 0);

    WINDOW *winfo = newwin(LINES/3, COLS, 2 * LINES / 3, 0);
    box(winfo, 0, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus = wstatus;
    env->wgame = wgame;
    env->winfo = winfo;

    Player *player = malloc(sizeof(Player));
    strcpy(player->name, "Player1");
    player->x = COLS / 2;
    player->y = LINES / 2;
    player->character = Benjamin;

    Game *game = malloc(sizeof(Game));
    game->score = 0;
    game->player = player;
    game->environment = env;

    return game;
}

char32_t __resolveCharacter__(Characters *character) {
    switch (*character) {
        case Benjamin: return U'B';
        case Ethan: return U'E';
        case Muhammad: return U'M';
        default: return U'?';
    }
}

void update(Game *game) {
    /* Add more obstacles before printing game state based on player position */
    /* Also reduce the vertical distance of the player to simulate gravity */
    mvwprintw(game->environment->wstatus, 1, 1, "Score: %d", game->score);
    mvwprintw(game->environment->wgame, game->player->y, game->player->x, "%lc", __resolveCharacter__(game->player->character));
    mvwprintw(game->environment->winfo, 1, 1, "Player: %s", game->player->name);

    wrefresh(game->environment->wstatus);
    wrefresh(game->environment->wgame);
    wrefresh(game->environment->winfo);

    refresh();
}

void run(Game *game) {
    pthread_create(&(game->player->keystroke), NULL, (void *)displace, (void *)&(game->player));

    while (pthread_kill(game->player->keystroke, 0) != ESRCH) {
        update(game);
        if (check_for_collision(game->player->x, game->player->y)) {
            // Handle collision
            break;
        }
    }
    pthread_join(game->player->keystroke, NULL);
}

void displace(Player *player) {
    while (1) {
        int keystroke = getch();
            switch (keystroke) {
            // directional keys (WASD or arrow keys)
            case 'w':
            case KEY_UP:
                player->y = (player->y > 1) ? player->y - 1 : player->y;
                continue;
            case 's':
            case KEY_DOWN:
                player->y = (player->y < LINES - 2) ? player->y + 1 : player->y;
                continue;
            case 'a':
            case KEY_LEFT:
                player->x = (player->x > 1) ? player->x - 1 : player->x;
                continue;
            case 'd':
            case KEY_RIGHT:
                player->x = (player->x < COLS - 2) ? player->x + 1 : player->x;
                continue;
            case 'q':
                // Exit the game loop
                pthread_exit(NULL);
            default: break;
        }
    }
}

void end(Game *game) {
    // Placeholder for any end-of-game logic, such as displaying a game over screen
    mvwprintw(game->environment->wstatus, 1, 1, "Game Over! Final Score: %d", game->score);
    wrefresh(game->environment->wstatus);
    getch();
}

void deinit(Game *game) {
    delwin(game->environment->wstatus);
    delwin(game->environment->wgame);
    delwin(game->environment->winfo);

    free(game);
    endwin();
}

// A simple function to demonstrate calling a C function from NASM assembly
void helloWorld() {
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
    snprintf(game_info, sizeof(game_info), "%s %s %d", GAME_TITLE, GAME_VERSION, 2024);
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