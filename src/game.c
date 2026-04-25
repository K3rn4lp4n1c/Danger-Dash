#include "game.h"

Game* init() {
    __initialize_curses__();
    WINDOW *wstatus = newwin(LINES / 3, COLS, 0, 0);
    box(wstatus, 0, 0);
    WINDOW *wgame = newwin(LINES / 3, COLS, LINES / 3, 0);
    WINDOW *winfo = newwin(LINES / 3, COLS, 2 * LINES / 3, 0);
    box(winfo, 0, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus    = wstatus;
    env->wgame      = wgame;
    env->winfo      = winfo;
    env->frame_rate = INITIAL_FRAME_RATE;
    env->map        = NULL;
    env->seed       = 0;

    int wgame_height = getmaxy(wgame);
    int wgame_width  = getmaxx(wgame);
    env->map = malloc(wgame_height * sizeof(char *));
    for (int i = 0; i < wgame_height; i++) {
        env->map[i] = malloc(wgame_width * sizeof(char));
        memset(env->map[i], ' ', wgame_width);
    }

    Game *game = malloc(sizeof(Game));
    game->env          = env;
    game->player_count = 1;
    for (int i = 0; i < MAX_PLAYERS; i++) game->players[i] = NULL;

    for (int i = 0; i < game->player_count; i++) {
        Player *player = malloc(sizeof(Player));
        char player_name[MAX_NAME_LENGTH];
        snprintf(player_name, sizeof(player_name), "Player%d", i + 1);
        strcpy(player->name, player_name);
        player->x         = i * 5 + 1;
        player->y         = 1;
        player->score     = 0;
        player->character = (Characters)i;
        game->players[i]  = player;
    }

    /* Initialize shared InputState — one mutex, one thread, flags per player */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->input.players[i].up    = 0;
        game->input.players[i].down  = 0;
        game->input.players[i].left  = 0;
        game->input.players[i].right = 0;
    }
    game->input.quit = 0;
    pthread_mutex_init(&game->input.lock, NULL);

    return game;
}

char32_t __resolveCharacter__(Characters *character) {
    switch (*character) {
        case Benjamin: return U'B';
        case Ethan:    return U'E';
        case Muhammad: return U'M';
        case Youssef:  return U'Y';
        default:       return U'?';
    }
}

void update(Game *game) {
    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width  = getmaxx(game->env->wgame);
    if (game->env->seed != 0) __adjust_map__(game, wgame_height, wgame_width);
    else __show_initial_screen__(game->env, wgame_height, wgame_width);
    for (int i = 0; i < game->player_count; i++) {
        mvwprintw(game->env->wstatus, 1, 1, "Score: %d", game->players[i]->score);
        mvwaddch(game->env->wgame, game->players[i]->y, game->players[i]->x,
                 __resolveCharacter__(&game->players[i]->character));
    }
    __refresh_all_windows__(game);
}

/*
 * input_thread — ONE thread for ALL players.
 * Reads one key, looks it up in KEY_MAPPINGS to find which player it belongs to,
 * locks mutex, sets that player's flag, unlocks.
 * ESC quits the game.
 *
 * Key ownership (physically separated clusters):
 * Player 1: Arrow keys    Player 2: WASD
 * Player 3: IJKL          Player 4: Numpad 8456
 */
void *input_thread(void *arg) {
    Game *game = (Game *)arg;
    timeout(100);  /* getch() returns ERR after 100ms if no key — lets thread check quit flag */
    while (1) {
        int key = getch();

        if (key == ERR) {
            /* no key pressed, just check if we should quit */
            pthread_mutex_lock(&game->input.lock);
            int q = game->input.quit;
            pthread_mutex_unlock(&game->input.lock);
            if (q) break;
            continue;
        }

        pthread_mutex_lock(&game->input.lock);

        if (key == 27) {  /* ESC quits */
            game->input.quit = 1;
            pthread_mutex_unlock(&game->input.lock);
            break;
        }

        /* Match key to a player using KEY_MAPPINGS — order: down, up, left, right */
        for (int p = 0; p < game->player_count; p++) {
            if      (key == KEY_MAPPINGS[p][0]) { game->input.players[p].down  = 1; break; }
            else if (key == KEY_MAPPINGS[p][1]) { game->input.players[p].up    = 1; break; }
            else if (key == KEY_MAPPINGS[p][2]) { game->input.players[p].left  = 1; break; }
            else if (key == KEY_MAPPINGS[p][3]) { game->input.players[p].right = 1; break; }
        }

        pthread_mutex_unlock(&game->input.lock);
    }
    return NULL;
}

/*
 * apply_input — called once per frame by the game loop (main thread).
 * Reads flags set by input_thread, moves players, clears flags.
 * x/y only written here — no race condition on position.
 */
void apply_input(Game *game) {
    int h = getmaxy(game->env->wgame);
    int w = getmaxx(game->env->wgame);

    pthread_mutex_lock(&game->input.lock);

    for (int i = 0; i < game->player_count; i++) {
        Player     *p  = game->players[i];
        PlayerInput *in = &game->input.players[i];

        if (in->up)    { p->y = (p->y > 1)   ? p->y-1 : p->y; in->up    = 0; }
        if (in->down)  { p->y = (p->y < h-1) ? p->y+1 : p->y; in->down  = 0; }
        if (in->left)  { p->x = (p->x > 1)   ? p->x-1 : p->x; in->left  = 0; }
        if (in->right) { p->x = (p->x < w-1) ? p->x+1 : p->x; in->right = 0; }
    }

    pthread_mutex_unlock(&game->input.lock);
}

/*
 * run — spawns ONE input thread for all players, runs game loop on main thread.
 */
void run(Game *game) {
    pthread_create(&game->input.thread, NULL, input_thread, (void *)game);

    game->env->seed = time(NULL);
    srand(game->env->seed);

    for (int i = 0; i < game->player_count; i++)
        mvwprintw(game->env->winfo, 1, 1, "Player: %s", game->players[i]->name);
    box(game->env->wstatus, 0, 0);
    box(game->env->winfo, 0, 0);

    int running = 1;
    while (running) {
        apply_input(game);
        update(game);
        usleep(game->env->frame_rate);

        pthread_mutex_lock(&game->input.lock);
        if (game->input.quit) running = 0;
        pthread_mutex_unlock(&game->input.lock);
    }

    /* Signal input thread to stop and wait for it */
    pthread_mutex_lock(&game->input.lock);
    game->input.quit = 1;
    pthread_mutex_unlock(&game->input.lock);
    pthread_join(game->input.thread, NULL);
    pthread_mutex_destroy(&game->input.lock);
}

/* Stub — kept for asm_io.asm linker compatibility. Do not delete. */
void displace(Player *player) { (void)player; }

void end(Game *game) {
    werase(game->env->wstatus);
    werase(game->env->wgame);
    werase(game->env->winfo);
    mvwprintw(game->env->wstatus, 1, 1, "Game Over! Final Score");
    for(int i = 0; i < game->player_count; i++) {
        mvwprintw(game->env->wstatus, 2 + i, 1, "%s: %d", game->players[i]->name, game->players[i]->score);
    }
    __refresh_all_windows__(game);
    mvprintw(LINES - 1, 0, "Thanks for playing! Exiting...");
    refresh();
    usleep(2000000);
}

void deinit(Game *game) {
    delwin(game->env->wstatus);
    delwin(game->env->wgame);
    delwin(game->env->winfo);
    free(game);
    endwin();
    exit(0);
}

void __refresh_all_windows__(Game *game) {
    wnoutrefresh(game->env->wstatus);
    wnoutrefresh(game->env->wgame);
    wnoutrefresh(game->env->winfo);
    doupdate();
}

void __initialize_curses__() {
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN,    COLOR_BLACK);
    }
}

void __show_initial_screen__(Environment *env, int wgame_height, int wgame_width) {
    for (int i = 0; i < wgame_height; i++)
        for (int j = 0; j < wgame_width; j++)
            env->map[i][j] = ' ';

    box(env->wgame, 0, 0);

    const char *title        = GAME_TITLE;
    const char *version      = GAME_VERSION;
    const char *controls_msg = "P1:Arrows  P2:WASD  P3:IJKL  P4:Numpad";
    const char *start_msg    = "Press SPACE to start";
    const char *quit_msg     = "Press ESC to quit";

    int title_y    = wgame_height / 2 - 3;
    int version_y  = title_y + 1;
    int controls_y = title_y + 3;
    int start_y    = controls_y + 2;
    int quit_y     = start_y + 1;

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(env->wgame, title_y,    (wgame_width - strlen(title))        / 2, "%s", title);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(1) | A_BOLD);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(2));
    mvwprintw(env->wgame, version_y,  (wgame_width - strlen(version))      / 2, "%s", version);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(2));

    mvwprintw(env->wgame, controls_y, (wgame_width - strlen(controls_msg)) / 2, "%s", controls_msg);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(env->wgame, start_y,    (wgame_width - strlen(start_msg))    / 2, "%s", start_msg);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(3) | A_BOLD);

    mvwprintw(env->wgame, quit_y,     (wgame_width - strlen(quit_msg))     / 2, "%s", quit_msg);
}

void __adjust_map__(Game *game, int wgame_height, int wgame_width) {
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width - 1; j++)
            game->env->map[i][j] = game->env->map[i][j + 1];
        game->env->map[i][wgame_width - 1] = ' ';
    }

    double r = (double)rand() / RAND_MAX;
    int obstacle_type = rand() % 3;

    if (r < (double)OBSTACLE_ODDS && obstacle_type == 0) {
        int obstacle_placement = rand() % 2;
        if (obstacle_placement == 0) {
            int middle_y = wgame_height / 2;
            for (int i = 0; i < game->player_count; i++) {
                game->players[i]->score++;
                game->env->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][0];
            }
        } else {
            int bottom_y = wgame_height - 1;
            for (int i = 0; i < game->player_count; i++)
                game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
        }
    } else if (r < (double)OBSTACLE_ODDS && obstacle_type == 1) {
        int middle_y = wgame_height / 2;
        for (int i = 0; i < game->player_count; i++)
            game->env->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
    } else if (r < (double)OBSTACLE_ODDS && obstacle_type == 2) {
        int bottom_y = wgame_height - 1;
        for (int i = 0; i < game->player_count; i++)
            game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][2];
    }

    for (int i = 0; i < wgame_height; i++)
        for (int j = 0; j < wgame_width; j++)
            mvwaddch(game->env->wgame, i, j, game->env->map[i][j]);
}

void helloWorld() {
    initscr(); curs_set(0); noecho();
    WINDOW *wlcm_win = newwin(10, 2 * COLS / 3, 0, 0);
    mvwin(wlcm_win, (LINES - getmaxy(wlcm_win)) / 2, (COLS - getmaxx(wlcm_win)) / 2);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED,   COLOR_BLACK);
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