#include "game.h"

// Initializes the game state, resources, etc.
Game* init() {
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    WINDOW *wstatus = newwin(LINES/3, COLS, 0, 0);
    box(wstatus, 0, 0);
    
    WINDOW *wgame = newwin(LINES/3, COLS, LINES / 3, 0);
    WINDOW *winfo = newwin(LINES/3, COLS, 2 * LINES / 3, 0);
    box(winfo, 0, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus = wstatus;
    env->wgame = wgame;
    env->winfo = winfo;
    env->map = NULL;
    int wgame_height = getmaxy(wgame);
    int wgame_width = getmaxx(wgame);
    env->seed = 0;
    env->map = malloc(wgame_height * sizeof(char *));
    for (int i = 0; i < wgame_height; i++) {
        env->map[i] = malloc(wgame_width * sizeof(char));
        memset(env->map[i], ' ', COLS); 
    }

    Game *game = malloc(sizeof(Game));
    game->environment = env;
    game->player_count = 1;
    for (int i = 0; i < 4; i++) game->players[i] = NULL;

    for (int i = 0; i < game->player_count; i++) {
        Player *player = malloc(sizeof(Player));
        char player_name[MAX_NAME_LENGTH];
        snprintf(player_name, sizeof(player_name), "Player%d", i + 1);
        strcpy(player->name, player_name);
        player->x = 1;
        player->y = 1;
        player->score = 0;
        player->character = Benjamin;

        // Initialize input flags to zero and set up the mutex
        player->inputState.up    = 0;
        player->inputState.down  = 0;
        player->inputState.left  = 0;
        player->inputState.right = 0;
        player->inputState.quit  = 0;
        pthread_mutex_init(&player->inputState.lock, NULL);

        game->players[i] = player;
    }
    
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
    __clear_all_windows__(game);
    if (game->environment->seed != 0) {
        int wgame_height = getmaxy(game->environment->wgame);
        int wgame_width = getmaxx(game->environment->wgame);
        for (int i = 0; i < wgame_height; i++) {
            for (int j = 0; j < wgame_width; j++) {
                if (rand() % 100 < 5) {
                    game->environment->map[i][j] = '#';
                } else {
                    game->environment->map[i][j] = ' ';
                }
            }
        }
        usleep(100000);
    } else {
        int wgame_height = getmaxy(game->environment->wgame);
        int wgame_width = getmaxx(game->environment->wgame);
        for (int i = 0; i < wgame_height; i++) {
            for (int j = 0; j < wgame_width; j++) {
                game->environment->map[i][j] = ' ';
            }
        }

        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_CYAN, COLOR_BLACK);
            init_pair(2, COLOR_YELLOW, COLOR_BLACK);
            init_pair(3, COLOR_GREEN, COLOR_BLACK);
        }

        box(game->environment->wgame, 0, 0);

        const char *title = GAME_TITLE;
        const char *version = GAME_VERSION;
        const char *start_msg = "Press SPACE to start";
        const char *quit_msg = "Press Q during game to quit";
        const char *controls_msg = "Move: WASD or Arrow Keys";

        int title_y = wgame_height / 2 - 3;
        int version_y = title_y + 1;
        int controls_y = title_y + 3;
        int start_y = controls_y + 2;
        int quit_y = start_y + 1;

        if (has_colors()) wattron(game->environment->wgame, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(game->environment->wgame, title_y, (wgame_width - (int)strlen(title)) / 2, "%s", title);
        if (has_colors()) wattroff(game->environment->wgame, COLOR_PAIR(1) | A_BOLD);

        if (has_colors()) wattron(game->environment->wgame, COLOR_PAIR(2));
        mvwprintw(game->environment->wgame, version_y, (wgame_width - (int)strlen(version)) / 2, "%s", version);
        if (has_colors()) wattroff(game->environment->wgame, COLOR_PAIR(2));

        mvwprintw(game->environment->wgame, controls_y, (wgame_width - (int)strlen(controls_msg)) / 2, "%s", controls_msg);

        if (has_colors()) wattron(game->environment->wgame, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(game->environment->wgame, start_y, (wgame_width - (int)strlen(start_msg)) / 2, "%s", start_msg);
        if (has_colors()) wattroff(game->environment->wgame, COLOR_PAIR(3) | A_BOLD);

        mvwprintw(game->environment->wgame, quit_y, (wgame_width - (int)strlen(quit_msg)) / 2, "%s", quit_msg);
    }

    for (int i = 0; i < game->player_count; i++) {
        mvwprintw(game->environment->wstatus, 1, 1, "Score: %d", game->players[i]->score);
        mvwprintw(game->environment->wgame, game->players[i]->y, game->players[i]->x, "%lc", __resolveCharacter__(&(game->players[i]->character)));
        mvwprintw(game->environment->winfo, 1, 1, "Player: %s", game->players[i]->name);
    }

    __refresh_all_windows__(game);
    refresh();
}

/*
 * run - called by assembly when SPACE is pressed.
 * Spawns one input_thread per player, then runs the game loop on the main thread.
 * Main thread calls apply_input() + update() each frame.
 * When quit flag is set, joins all input threads cleanly before returning.
 */
void run(Game *game) {
    // Spawn one input thread per player
    for (int i = 0; i < game->player_count; i++) {
        pthread_create(&(game->players[i]->keystroke), NULL, input_thread, (void *)game->players[i]);
    }

    game->environment->seed = time(NULL);
    srand(game->environment->seed);

    // Game loop on main thread
    int running = 1;
    while (running) {
        apply_input(game);  // read flags from input thread, move players
        update(game);       // render

        // Check if any player pressed Q
        for (int i = 0; i < game->player_count; i++) {
            pthread_mutex_lock(&game->players[i]->inputState.lock);
            if (game->players[i]->inputState.quit) running = 0;
            pthread_mutex_unlock(&game->players[i]->inputState.lock);
        }
    }

    // Signal input threads to stop and wait for them
    for (int i = 0; i < game->player_count; i++) {
        pthread_mutex_lock(&game->players[i]->inputState.lock);
        game->players[i]->inputState.quit = 1;
        pthread_mutex_unlock(&game->players[i]->inputState.lock);
        pthread_join(game->players[i]->keystroke, NULL);
        pthread_mutex_destroy(&game->players[i]->inputState.lock);
    }
}

/*
 * input_thread - runs on a dedicated thread, one per player.
 * Only job: read a key, lock mutex, set the right flag, unlock.
 * Never touches x/y - that belongs to apply_input on the main thread.
 */
void *input_thread(void *arg) {
    Player *player = (Player *)arg;

    while (1) {
        int key = getch();  // blocks waiting for a keypress

        pthread_mutex_lock(&player->inputState.lock);
        switch (key) {
            case 'w': case KEY_UP:    player->inputState.up    = 1; break;
            case 's': case KEY_DOWN:  player->inputState.down  = 1; break;
            case 'a': case KEY_LEFT:  player->inputState.left  = 1; break;
            case 'd': case KEY_RIGHT: player->inputState.right = 1; break;
            case 'q':                 player->inputState.quit  = 1; break;
            default: break;
        }
        int should_quit = player->inputState.quit;
        pthread_mutex_unlock(&player->inputState.lock);

        if (should_quit) break;
    }
    return NULL;
}

/*
 * apply_input - called once per frame by the game loop (main thread).
 * Reads flags written by input_thread, moves the player, then clears the flags.
 * x/y is only ever written here - no race condition on position.
 */
void apply_input(Game *game) {
    int h = getmaxy(game->environment->wgame);
    int w = getmaxx(game->environment->wgame);

    for (int i = 0; i < game->player_count; i++) {
        Player *p = game->players[i];

        pthread_mutex_lock(&p->inputState.lock);
        if (p->inputState.up)    { p->y = (p->y > 1)   ? p->y-1 : p->y; p->inputState.up    = 0; }
        if (p->inputState.down)  { p->y = (p->y < h-2) ? p->y+1 : p->y; p->inputState.down  = 0; }
        if (p->inputState.left)  { p->x = (p->x > 1)   ? p->x-1 : p->x; p->inputState.left  = 0; }
        if (p->inputState.right) { p->x = (p->x < w-2) ? p->x+1 : p->x; p->inputState.right = 0; }
        pthread_mutex_unlock(&p->inputState.lock);
    }
}

/*
 * displace - STUB only. asm_io.asm declares this as extern so it must exist.
 * The actual input logic is in input_thread() and apply_input() above.
 * Do not delete this.
 */
void displace(Player *player) {
    (void)player;
}

void end(Game *game) {
    __clear_all_windows__(game);
    mvwprintw(game->environment->wstatus, 1, 1, "Game Over! Final Score");
    for(int i = 0; i < game->player_count; i++) {
        mvwprintw(game->environment->wstatus, 2 + i, 1, "%s: %d", game->players[i]->name, game->players[i]->score);
    }
    __refresh_all_windows__(game);
    mvprintw(LINES - 1, 0, "Exiting game... Press any key to continue.");
    getch();
}

void deinit(Game *game) {
    delwin(game->environment->wstatus);
    delwin(game->environment->wgame);
    delwin(game->environment->winfo);
    free(game);
    endwin();
    exit(0);
}

void __clear_all_windows__(Game *game) {
    werase(game->environment->wstatus);
    werase(game->environment->wgame);
    werase(game->environment->winfo);
}

void __refresh_all_windows__(Game *game) {
    wrefresh(game->environment->wstatus);
    wrefresh(game->environment->wgame);
    wrefresh(game->environment->winfo);
}

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