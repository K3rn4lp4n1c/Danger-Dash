#include "game.h"

Game* init(int count, char **names, Characters *characters) {
    if (count <= 0 || count > MAX_PLAYERS) {
        fprintf(stderr, "Player count must be between 1 and %d: %d\n", MAX_PLAYERS, count);
        exit(EXIT_FAILURE);
    }

    __initialize_curses__();

    WINDOW *wstatus = newwin(LINES / 3, COLS, 0, 0);
    WINDOW *wgame = newwin(LINES / 3, COLS, LINES / 3, 0);
    WINDOW *winfo = newwin(LINES / 3, COLS, 2 * LINES / 3, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus = wstatus;
    env->wgame = wgame;
    env->winfo = winfo;
    env->map = NULL;
    env->frame_rate = INITIAL_FRAME_RATE;

    int wgame_height = getmaxy(wgame);
    int wgame_width = getmaxx(wgame);
    env->map = malloc(wgame_height * sizeof(char *));
    for (int i = 0; i < wgame_height; i++) {
        env->map[i] = malloc(wgame_width * sizeof(char));
        memset(env->map[i], ' ', wgame_width);
    }

    Player *players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) players[i] = NULL;

    for (int i = 0; i < count; i++) {
        Player *player = malloc(sizeof(Player));
        strncpy(player->name, names[i], MAX_NAME_LENGTH - 1);
        player->name[MAX_NAME_LENGTH - 1] = '\0';
        player->character = (Characters)characters[i];
        player->state = INACTIVE;
        player->x = 1;
        player->y = wgame_height - 1;
        players[i] = player;
        pthread_mutex_init(&player->lock, NULL);
    }

    Game *game = malloc(sizeof(Game));
    game->env = env;
    game->state = INACTIVE;
    game->score = 0;
    pthread_mutex_init(&game->lock, NULL);
    game->player_count = count;
    memcpy(game->players, players, MAX_PLAYERS * sizeof(Player *));

    __initial_screen__(game);
    __refresh_all_windows__(game);
    return game;
}

void __initialize_curses__() {
    setlocale(LC_ALL, "");
    initscr();
    refresh();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(UI_TITLE, COLOR_CYAN, -1);
        init_pair(UI_STATUS, COLOR_WHITE, -1);
        init_pair(UI_ACCENT, COLOR_YELLOW, -1);
        init_pair(UI_PLAYER_1, COLOR_GREEN, -1);
        init_pair(UI_PLAYER_2, COLOR_CYAN, -1);
        init_pair(UI_PLAYER_3, COLOR_MAGENTA, -1);
        init_pair(UI_PLAYER_4, COLOR_BLUE, -1);
        init_pair(UI_OBSTACLE, COLOR_RED, -1);
    }
}

void run(Game *game) {
    __erase_all_windows__(game->env);

    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width = getmaxx(game->env->wgame);
    for (int i = 0; i < wgame_height; i++) memset(game->env->map[i], ' ', wgame_width);

    game->env->frame_rate = INITIAL_FRAME_RATE;
    srand(time(NULL));

    for (int i = 0; i < game->player_count; i++) {
        game->players[i]->x = 1;
        game->players[i]->y = wgame_height - 1;
        game->players[i]->state = ACTIVE;
    }

    pthread_mutex_lock(&game->lock);
    game->state = ACTIVE;
    pthread_mutex_unlock(&game->lock);

    __draw_status__(game, game->player_count);
    __draw_info__(game);
    __render_world__(game, wgame_height, wgame_width);
    __refresh_all_windows__(game);

    pthread_create(&game->input, NULL, __keypress__, (void*)game);
    game->score = 0;

    States s;
    do {
        update(game);
        usleep(game->env->frame_rate);
        pthread_mutex_lock(&game->lock);
        s = game->state;
        pthread_mutex_unlock(&game->lock);
    } while (s != INACTIVE);
}

void update(Game *game) {
    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width  = getmaxx(game->env->wgame);

    int positions[MAX_PLAYERS][2];
    States pstates[MAX_PLAYERS];
    for (int i = 0; i < game->player_count; i++) {
        pthread_mutex_lock(&game->players[i]->lock);
        positions[i][0] = game->players[i]->y;
        positions[i][1] = game->players[i]->x;
        pstates[i] = game->players[i]->state;
        pthread_mutex_unlock(&game->players[i]->lock);
    }

    pthread_mutex_lock(&game->lock);
    States gState = game->state;
    pthread_mutex_unlock(&game->lock);

    int active_players = game->player_count;
    for (int i = 0; i < game->player_count; i++) {
        if (gState == ACTIVE && check_for_collision(positions[i][0], positions[i][1])) {
            pthread_mutex_lock(&game->players[i]->lock);
            game->players[i]->state = INACTIVE;
            pthread_mutex_unlock(&game->players[i]->lock);
            active_players--;
        } else if (pstates[i] == IDLE || pstates[i] == INACTIVE) {
            active_players--;
        }
    }

    pthread_mutex_lock(&game->lock);
    if (active_players <= 0) game->state = INACTIVE;
    else if (gState == ACTIVE && active_players < game->player_count) game->state = IDLE;
    else if (gState == IDLE && active_players == game->player_count) game->state = ACTIVE;
    gState = game->state;
    pthread_mutex_unlock(&game->lock);

    if (gState == ACTIVE) __adjust_map__(game);

    __draw_status__(game, active_players);
    __draw_info__(game);
    __render_world__(game, wgame_height, wgame_width);
    __refresh_all_windows__(game);
}

const wchar_t *__resolve_character__(Characters *character) {
    switch (*character) {
        case Benjamin: return L"🥷";
        case Ethan: return L"👨";
        case Muhammad: return L"👳🏻";
        case Youssef: return L"🕵";
        default: return L"❓";
    }
}

int __resolve_character_color__(Characters character) {
    switch (character) {
        case Benjamin: return UI_PLAYER_1;
        case Ethan: return UI_PLAYER_2;
        case Muhammad: return UI_PLAYER_3;
        case Youssef: return UI_PLAYER_4;
        default: return UI_STATUS;
    }
}

const char* __resolve_state__(States state) {
    switch (state) {
        case INACTIVE: return "INACTIVE";
        case ACTIVE: return "ACTIVE";
        case IDLE: return "PAUSED";
        case BUSY: return "MOVING";
        default: return "UNKNOWN";
    }
}

void __initial_screen__(Game *game) {
    Environment *env = game->env;
    int wgame_height = getmaxy(env->wgame);
    int wgame_width = getmaxx(env->wgame);

    int winfo_height = getmaxy(env->winfo);
    int winfo_width = getmaxx(env->winfo);

    for (int i = 0; i < wgame_height; i++) memset(env->map[i], ' ', wgame_width);

    werase(env->wstatus);
    werase(env->wgame);
    werase(env->winfo);

    box(env->wstatus, 0, 0);
    box(env->winfo, 0, 0);

    char title_and_version[30];
    snprintf(title_and_version, sizeof(title_and_version), "%s v%s", GAME_TITLE, GAME_VERSION);
    const char *start_msg = "Press SPACE to start";
    const char *quit_msg = "Press ESC to quit";
    const char *pause_msg = "Pause or resume with BACKSPACE (P1), Q (P2), P (P3), or 0(P4)";
    const char *controls_msg = "P1: arrows | P2: WASD | P3: IJKL | P4: keypad";
    const char *copyright = "© 2026 K3RN4LP4N1C. Dakota State University. All rights reserved.";

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(UI_TITLE) | A_BOLD);
    mvwprintw(env->wgame, wgame_height / 2 - 4, (wgame_width - (int)strlen(title_and_version)) / 2, "%s", title_and_version);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(UI_TITLE) | A_BOLD);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(UI_ACCENT) | A_BOLD);
    wattron(env->wgame, A_BLINK);
    mvwprintw(env->wgame, wgame_height / 2 + 1, (wgame_width - (int)strlen(start_msg)) / 2, "%s", start_msg);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(UI_ACCENT) | A_BOLD);
    wattroff(env->wgame, A_BLINK);

    mvwprintw(env->winfo, 3, 2, "%s", controls_msg);
    mvwprintw(env->winfo, 5, 2, "%s", pause_msg);
    mvwprintw(env->winfo, 7, 2, "%s", quit_msg);

    mvwprintw(env->wstatus, 1, 2, "Ready for launch");
    mvwprintw(env->winfo, 1, 2, "Tip: Don't resize the terminal during gameplay!");
    mvwprintw(env->winfo, winfo_height - 2, (winfo_width - (int)strlen(copyright)) / 2, "%s", copyright);
}

void __draw_status__(Game *game, int active_players) {
    WINDOW *wstatus = game->env->wstatus;

    pthread_mutex_lock(&game->lock);
    States gState = game->state;
    int score = game->score;
    unsigned long frame_rate = game->env->frame_rate;
    pthread_mutex_unlock(&game->lock);

    werase(wstatus);
    box(wstatus, 0, 0);

    if (has_colors()) wattron(wstatus, COLOR_PAIR(UI_TITLE) | A_BOLD);
    mvwprintw(wstatus, 1, 2, "%s", GAME_TITLE);
    if (has_colors()) wattroff(wstatus, COLOR_PAIR(UI_TITLE) | A_BOLD);

    if (has_colors()) wattron(wstatus, COLOR_PAIR(UI_STATUS));
    mvwprintw(wstatus, 1, 20, "Score: %-6d", score);
    mvwprintw(wstatus, 1, 38, "State: %-8s", __resolve_state__(gState));
    mvwprintw(wstatus, 1, 60, "Players: %d/%d", active_players, game->player_count);
    mvwprintw(wstatus, 2, 20, "Frame delay: %lu us", frame_rate);
    mvwprintw(wstatus, 2, 48, "Version: %s", GAME_VERSION);
    if (has_colors()) wattroff(wstatus, COLOR_PAIR(UI_STATUS));
}

void __draw_info__(Game *game) {
    WINDOW *winfo = game->env->winfo;

    werase(winfo);
    box(winfo, 0, 0);

    if (has_colors()) wattron(winfo, COLOR_PAIR(UI_ACCENT) | A_BOLD);
    mvwprintw(winfo, 1, 2, "Controls");
    if (has_colors()) wattroff(winfo, COLOR_PAIR(UI_ACCENT) | A_BOLD);
    mvwprintw(winfo, 1, 14, "P1 arrows   P2 WASD   P3 IJKL   P4 keypad");
    mvwprintw(winfo, 2, 2, "Pause: BACKSPACE / Q / P / 0     Quit: ESC");

    if (has_colors()) wattron(winfo, COLOR_PAIR(UI_ACCENT) | A_BOLD);
    mvwprintw(winfo, 4, 2, "Players");
    if (has_colors()) wattroff(winfo, COLOR_PAIR(UI_ACCENT) | A_BOLD);

    for (int i = 0; i < game->player_count; i++) {
        Player *player = game->players[i];
        pthread_mutex_lock(&player->lock);
        int color = __resolve_character_color__(player->character);
        const wchar_t *glyph = __resolve_character__(&player->character);
        const char *state = __resolve_state__(player->state);
        int row = 5 + (i / 2);
        int col = 2 + (i % 2) * 34;

        if (has_colors()) wattron(winfo, COLOR_PAIR(color) | A_BOLD);
        mvwaddwstr(winfo, row, col, glyph);
        if (has_colors()) wattroff(winfo, COLOR_PAIR(color) | A_BOLD);

        mvwprintw(winfo, row, col + 3, "%s [%s]", player->name, state);
        pthread_mutex_unlock(&player->lock);
    }
}

void __adjust_map__(Game *game) {
    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width = getmaxx(game->env->wgame);
    for (int i = 0; i < wgame_height; i++) {
        memmove(game->env->map[i], game->env->map[i] + 1, (size_t)(wgame_width - 1));
        game->env->map[i][wgame_width - 1] = ' ';
    }

    double r = (double)rand() / RAND_MAX;
    int obstacle_type = rand() % 3; /* 0 = mixed, 1 = air, 2 = land */

    if (r < OBSTACLE_ODDS && obstacle_type == 0) {
        int obstacle_placement = rand() % 2;
        if (obstacle_placement == 0) {
            int middle_y = wgame_height / 2;
            game->env->map[middle_y][wgame_width - 1] = OBSTACLES[0][0];
        } else {
            int bottom_y = wgame_height - 1;
            game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[0][1];
        }
    } else if (r < OBSTACLE_ODDS && obstacle_type == 1) {
        int middle_y = wgame_height / 2;
        game->env->map[middle_y][wgame_width - 1] = OBSTACLES[0][1];
    } else if (r < OBSTACLE_ODDS && obstacle_type == 2) {
        int bottom_y = wgame_height - 1;
        game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[0][2];
    }

    game->score++;
}

void __render_world__(Game *game, int wgame_height, int wgame_width) {
    WINDOW *wgame = game->env->wgame;
    int air_marker_y = (wgame_height / 2) - 1;
    int ground_marker_y = wgame_height - 2;

    werase(wgame);

    if (air_marker_y > 0) {
        if (has_colors()) wattron(wgame, COLOR_PAIR(UI_ACCENT));
        mvwhline(wgame, air_marker_y, 0, ACS_HLINE, wgame_width);
        if (wgame_width > 12) mvwprintw(wgame, air_marker_y, 2, " AIR ");
        if (has_colors()) wattroff(wgame, COLOR_PAIR(UI_ACCENT));
    }

    if (ground_marker_y > 0) {
        if (has_colors()) wattron(wgame, COLOR_PAIR(UI_ACCENT));
        mvwhline(wgame, ground_marker_y, 0, ACS_HLINE, wgame_width);
        if (wgame_width > 15) mvwprintw(wgame, ground_marker_y, 2, " GROUND ");
        if (has_colors()) wattroff(wgame, COLOR_PAIR(UI_ACCENT));
    }

    if (wgame_height > 3 && wgame_width > 10) {
        int star_offset = game->score % 7;
        for (int x = 4; x < wgame_width - 2; x += 9) {
            int star_x = x - star_offset;
            if (star_x > 1) mvwaddch(wgame, 1 + ((x / 9) % 2), star_x, '.');
        }
    }

    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width; j++) {
            char tile = game->env->map[i][j];
            if (tile == ' ') continue;
            if (has_colors()) wattron(wgame, COLOR_PAIR(UI_OBSTACLE) | A_BOLD);
            mvwaddch(wgame, i, j, tile);
            if (has_colors()) wattroff(wgame, COLOR_PAIR(UI_OBSTACLE) | A_BOLD);
        }
    }

    for (int i = 0; i < game->player_count; i++) {
        Player *player = game->players[i];
        pthread_mutex_lock(&player->lock);
        int color = __resolve_character_color__(player->character);
        if (player->state != INACTIVE) {
            if (has_colors()) wattron(wgame, COLOR_PAIR(color) | A_BOLD);
            mvwaddwstr(wgame, player->y, player->x, __resolve_character__(&player->character));
            if (has_colors()) wattroff(wgame, COLOR_PAIR(color) | A_BOLD);
        } else {
            mvwaddch(wgame, player->y, player->x, 'x');
        }
        pthread_mutex_unlock(&player->lock);
    }
}

void* __keypress__(void *arg) {
    Game *game = (Game *)arg;
    timeout(50);

    pthread_mutex_lock(&game->lock);
    States gState = game->state;
    pthread_mutex_unlock(&game->lock);

    while (gState != INACTIVE) {
        int ch = getch();
        Player *player = NULL;

        if (ch == ERR) {
            pthread_mutex_lock(&game->lock);
            gState = game->state;
            pthread_mutex_unlock(&game->lock);
            continue;
        }
        if (ch == 27) {
            pthread_mutex_lock(&game->lock);
            game->state = INACTIVE;
            pthread_mutex_unlock(&game->lock);
            break;
        }

        if (game->player_count > 0 && ch > KEY_MIN && ch < KEY_MAX) {
            player = game->players[0];
        } else if (game->player_count > 3 && ch >= '0' && ch <= '9') {
            player = game->players[3];
        } else if (game->player_count > 2 &&
                   ((ch >= 'i' && ch <= 'p') || (ch >= 'I' && ch <= 'P'))) {
            player = game->players[2];
        } else if (game->player_count > 1) {
            player = game->players[1];
        }

        if (player == NULL) continue;

        pthread_mutex_lock(&player->lock);

        if (ch == 'q' || ch == 'Q' || ch == 'p' || ch == 'P' ||
            ch == KEY_BACKSPACE || ch == '0') {
            player->state = (player->state == IDLE) ? ACTIVE : IDLE;
            pthread_mutex_unlock(&player->lock);
            continue;
        }

        if (player->state == IDLE || player->state == BUSY) {
            pthread_mutex_unlock(&player->lock);
            continue;
        }

        player->state = BUSY;

        Input *input = malloc(sizeof(Input));
        if (input == NULL) {
            player->state = ACTIVE;
            pthread_mutex_unlock(&player->lock);
            continue;
        }

        input->player = player;
        input->key = ch;
        input->frame_rate = game->env->frame_rate;

        if (pthread_create(&player->thread, NULL, __player_effect__, input) != 0) {
            free(input);
            player->state = ACTIVE;
            pthread_mutex_unlock(&player->lock);
            continue;
        }

        pthread_mutex_unlock(&player->lock);
        pthread_mutex_lock(&game->lock);
        gState = game->state;
        pthread_mutex_unlock(&game->lock);
    }

    return NULL;
}

void* __player_effect__(void *arg) {
    pthread_detach(pthread_self());

    Input *input = (Input *)arg;
    Player *player = input->player;
    int key = input->key;
    unsigned long frame_rate = input->frame_rate;
    free(input);

    int start_x, start_y;
    int lines = LINES / 3;
    int cols = COLS;

    pthread_mutex_lock(&player->lock);

    if (player->state != BUSY) {
        pthread_mutex_unlock(&player->lock);
        return NULL;
    }

    start_x = player->x;
    start_y = player->y;

    pthread_mutex_unlock(&player->lock);

    int *new_yx = malloc(2 * sizeof(int));
    if (new_yx == NULL) {
        pthread_mutex_lock(&player->lock);
        if (player->state == BUSY) player->state = ACTIVE;
        pthread_mutex_unlock(&player->lock);
        return NULL;
    }

    switch (key) {
        case KEY_UP:
            key = 'w';
            break;
        case KEY_DOWN:
            key = 's';
            break;
        case KEY_LEFT:
            key = 'a';
            break;
        case KEY_RIGHT:
            key = 'd';
            break;
    }

    move_player(new_yx, key, start_y, start_x, lines, cols);

    int target_y = new_yx[0];
    int target_x = new_yx[1];
    free(new_yx);

    if (start_y > target_y) {
        for (int i = start_y; i >= target_y; --i) {
            pthread_mutex_lock(&player->lock);
            if (player->state == IDLE || player->state == INACTIVE) {
                pthread_mutex_unlock(&player->lock);
                return NULL;
            }
            player->y = i;
            pthread_mutex_unlock(&player->lock);
            usleep(frame_rate);
        }

        for (int i = target_y; i <= lines - 1; ++i) {
            pthread_mutex_lock(&player->lock);
            if (player->state == IDLE || player->state == INACTIVE) {
                pthread_mutex_unlock(&player->lock);
                return NULL;
            }
            player->y = i;
            pthread_mutex_unlock(&player->lock);
            usleep(frame_rate);
        }
    } else if (start_y < target_y) {
        for (int i = start_y; i <= target_y; ++i) {
            pthread_mutex_lock(&player->lock);
            if (player->state == IDLE || player->state == INACTIVE) {
                pthread_mutex_unlock(&player->lock);
                return NULL;
            }
            player->y = i;
            pthread_mutex_unlock(&player->lock);
            usleep(frame_rate / 10);
        }
    }

    pthread_mutex_lock(&player->lock);
    if (player->state == BUSY) {
        player->x = target_x;
        player->state = ACTIVE;
    }
    pthread_mutex_unlock(&player->lock);

    return NULL;
}

void __erase_all_windows__(Environment *env) {
    werase(env->wstatus);
    werase(env->wgame);
    werase(env->winfo);
}

void __refresh_all_windows__(Game *game) {
    wnoutrefresh(stdscr);
    wnoutrefresh(game->env->wstatus);
    wnoutrefresh(game->env->wgame);
    wnoutrefresh(game->env->winfo);
    doupdate();
}

void end(Game *game) {
    game->state = INACTIVE;
    for (int i = 0; i < game->player_count; i++) game->players[i]->state = INACTIVE;
    pthread_join(game->input, NULL);

    __erase_all_windows__(game->env);
    box(game->env->wstatus, 0, 0);
    box(game->env->winfo, 0, 0);

    if (has_colors()) wattron(game->env->wstatus, COLOR_PAIR(UI_OBSTACLE) | A_BOLD);
    mvwprintw(game->env->wstatus, 1, 2, "Run finished");
    if (has_colors()) wattroff(game->env->wstatus, COLOR_PAIR(UI_OBSTACLE) | A_BOLD);

    mvwprintw(game->env->wstatus, 2, 2, "Final score: %d", game->score);
    mvwprintw(game->env->winfo, 1, 2, "Press any key to restart, or q to quit.");
    mvprintw(LINES - 1, 0, "Exiting game... Press any key to restart, press q to quit.");

    __refresh_all_windows__(game);
    timeout(-1);
}

void deinit(Game *game) {
    for (int i = 0; i < getmaxy(game->env->wgame); i++) free(game->env->map[i]);
    delwin(game->env->wstatus);
    delwin(game->env->wgame);
    delwin(game->env->winfo);
    free(game->env->map);
    free(game->env);
    pthread_mutex_destroy(&game->lock);

    for (int i = 0; i < game->player_count; i++) {
        pthread_mutex_destroy(&game->players[i]->lock);
        free(game->players[i]);
    }

    free(game);
    endwin();
    exit(0);
}

void helloWorld() {
    initscr();
    curs_set(0);
    noecho();

    WINDOW *wlcm_win = newwin(10, 2 * COLS / 3, 0, 0);
    mvwin(wlcm_win, (LINES - getmaxy(wlcm_win)) / 2, (COLS - getmaxx(wlcm_win)) / 2);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_RED, -1);
        init_pair(2, COLOR_GREEN, -1);
    }

    wattron(wlcm_win, COLOR_PAIR(1));
    mvwprintw(wlcm_win, getmaxy(wlcm_win) / 2 - 1,
              (getmaxx(wlcm_win) - (int)strlen(WELCOME_MSG)) / 2,
              "%s", WELCOME_MSG);
    wattroff(wlcm_win, COLOR_PAIR(1));

    char game_info[100];
    snprintf(game_info, sizeof(game_info), "%s %s %d", GAME_TITLE, GAME_VERSION, 2024);
    wattron(wlcm_win, COLOR_PAIR(2));
    mvwprintw(wlcm_win, getmaxy(wlcm_win) / 2 + 1,
              (getmaxx(wlcm_win) - (int)strlen(game_info)) / 2,
              "%s", game_info);
    wattroff(wlcm_win, COLOR_PAIR(2));

    refresh();
    wrefresh(wlcm_win);
    getch();

    delwin(wlcm_win);
    endwin();
}
