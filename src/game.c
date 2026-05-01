#include "game.h"

static int __player_color_pair__(Characters c) {
    switch (c) {
        case Benjamin: return 2;
        case Ethan:    return 4;
        case Muhammad: return 5;
        case Youssef:  return 6;
        default:       return 7;
    }
}

static int __state_color_pair__(States s) {
    switch (s) {
        case ACTIVE:   return 2;
        case IDLE:     return 3;
        case BUSY:     return 6;
        case INACTIVE: return 1;
        default:       return 7;
    }
}

static const char *__state_text__(States s) {
    switch (s) {
        case ACTIVE:   return "ACTIVE";
        case IDLE:     return "PAUSED";
        case BUSY:     return "MOVING";
        case INACTIVE: return "OUT";
        default:       return "UNKNOWN";
    }
}

static void __draw_panel__(WINDOW *win, const char *title, int pair) {
    if (has_colors()) wattron(win, COLOR_PAIR(pair) | A_BOLD);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", title);
    if (has_colors()) wattroff(win, COLOR_PAIR(pair) | A_BOLD);
}

static void __draw_hud__(Game *game, int positions[MAX_PLAYERS][2], States pstates[MAX_PLAYERS], States gState) {
    Environment *env = game->env;
    int info_h = getmaxy(env->winfo);

    werase(env->wstatus);
    werase(env->winfo);

    __draw_panel__(env->wstatus, " STATUS ", 6);
    __draw_panel__(env->winfo, " PLAYERS / CONTROLS ", 5);

    if (has_colors()) wattron(env->wstatus, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(env->wstatus, 1, 2, "Score: %06d", game->score);
    if (has_colors()) wattroff(env->wstatus, COLOR_PAIR(3) | A_BOLD);

    if (has_colors()) wattron(env->wstatus, COLOR_PAIR(__state_color_pair__(gState)) | A_BOLD);
    mvwprintw(env->wstatus, 1, 22, "State: %s", __state_text__(gState));
    if (has_colors()) wattroff(env->wstatus, COLOR_PAIR(__state_color_pair__(gState)) | A_BOLD);

    mvwprintw(env->wstatus, 1, 42, "Frame: %luus", game->env->frame_rate);

    for (int i = 0; i < game->player_count && i < info_h - 3; i++) {
        int row = i + 1;

        if (has_colors()) wattron(env->winfo, COLOR_PAIR(__player_color_pair__(game->players[i]->character)) | A_BOLD);
        mvwprintw(env->winfo, row, 2, "P%d %-12s", i + 1, game->players[i]->name);
        if (has_colors()) wattroff(env->winfo, COLOR_PAIR(__player_color_pair__(game->players[i]->character)) | A_BOLD);

        mvwprintw(env->winfo, row, 20, "x:%-3d y:%-3d", positions[i][1], positions[i][0]);

        if (has_colors()) wattron(env->winfo, COLOR_PAIR(__state_color_pair__(pstates[i])) | A_BOLD);
        mvwprintw(env->winfo, row, 34, "%-7s", __state_text__(pstates[i]));
        if (has_colors()) wattroff(env->winfo, COLOR_PAIR(__state_color_pair__(pstates[i])) | A_BOLD);
    }

    mvwprintw(env->winfo, info_h - 2, 2, "Move: Arrow Keys   Pause: BACKSPACE   Quit: ESC");
}

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
    int wgame_height = getmaxy(wgame);
    int wgame_width = getmaxx(wgame);
    env->map = malloc(wgame_height * sizeof(char *));
    for (int i = 0; i < wgame_height; i++) env->map[i] = malloc(wgame_width * sizeof(char));
    
    Player *players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) players[i] = NULL;
    for (int i = 0; i < count; i++) {
        Player *player = malloc(sizeof(Player));
        strncpy(player->name, names[i], MAX_NAME_LENGTH - 1);
        player->name[MAX_NAME_LENGTH - 1] = '\0';
        player->character = (Characters)characters[i];
        player->state = INACTIVE;
        players[i] = player;
        pthread_mutex_init(&player->lock, NULL);
    }

    Game *game = malloc(sizeof(Game));
    game->env = env;
    game->state = INACTIVE;
    pthread_mutex_init(&game->lock, NULL);
    game->player_count = count;
    memcpy(game->players, players, MAX_PLAYERS * sizeof(Player *));

    __initial_screen__(game, wgame_height, wgame_width);
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

        init_pair(1, COLOR_RED,     -1);
        init_pair(2, COLOR_GREEN,   -1);
        init_pair(3, COLOR_YELLOW,  -1);
        init_pair(4, COLOR_BLUE,    -1);
        init_pair(5, COLOR_MAGENTA, -1);
        init_pair(6, COLOR_CYAN,    -1);
        init_pair(7, COLOR_WHITE,   -1);
        init_pair(8, COLOR_RED,     -1);
        init_pair(9, COLOR_YELLOW,  -1);
        init_pair(10, COLOR_CYAN,   -1);
    }
}

void run(Game *game) {
    __erase_all_windows__(game->env);
    box(game->env->wstatus, 0, 0);
    box(game->env->winfo, 0, 0);
    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width = getmaxx(game->env->wgame);
    for (int i = 0; i < wgame_height; i++) memset(game->env->map[i], ' ', wgame_width);
    game->env->frame_rate = INITIAL_FRAME_RATE;
    
    srand(time(NULL));

    for (int i = 0; i < game->player_count; i++) {
        game->players[i]->x = 1;
        game->players[i]->y = getmaxy(game->env->wgame) - 1;
        game->players[i]->state = ACTIVE;

        if (has_colors()) wattron(game->env->winfo, COLOR_PAIR(6) | A_DIM);
        mvwprintw(game->env->winfo, 1, 1, "PLAYER");
        if (has_colors()) wattroff(game->env->winfo, COLOR_PAIR(6) | A_DIM);

        if (has_colors()) wattron(game->env->winfo, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(game->env->winfo, 2, 1, "%s", game->players[i]->name);
        if (has_colors()) wattroff(game->env->winfo, COLOR_PAIR(2) | A_BOLD);

        const char *char_name;
        int cpair;
        switch (game->players[i]->character) {
            case Benjamin: char_name = "Benjamin"; cpair = 1; break;
            case Ethan:    char_name = "Ethan";    cpair = 2; break;
            case Muhammad: char_name = "Muhammad"; cpair = 5; break;
            case Youssef:  char_name = "Youssef";  cpair = 4; break;
            default:       char_name = "Unknown";  cpair = 6; break;
        }
        if (has_colors()) wattron(game->env->winfo, COLOR_PAIR(cpair));
        mvwprintw(game->env->winfo, 3, 1, "[ %s ]", char_name);
        if (has_colors()) wattroff(game->env->winfo, COLOR_PAIR(cpair));
    }
    __refresh_all_windows__(game);

    pthread_mutex_lock(&game->lock);
    game->state = ACTIVE;
    pthread_mutex_unlock(&game->lock);
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
        pstates[i]      = game->players[i]->state;
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
        } else if (pstates[i] == IDLE) {
            active_players--;
        } else if (pstates[i] == INACTIVE) {
            active_players--;
        }
    }

    pthread_mutex_lock(&game->lock);
    if (active_players <= 0) game->state = INACTIVE;
    else if (gState == ACTIVE && active_players < game->player_count) game->state = IDLE;
    else if (gState == IDLE && active_players == game->player_count) game->state = ACTIVE;
    pthread_mutex_unlock(&game->lock);

    if (gState == ACTIVE) __adjust_map__(game, wgame_height, wgame_width);

    __draw_hud__(game, positions, pstates, gState);

    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width; j++) {
            char cell = game->env->map[i][j];

            if (cell != ' ') {
                if (has_colors()) wattron(game->env->wgame, COLOR_PAIR(8) | A_BOLD);
                mvwaddch(game->env->wgame, i, j, cell);
                if (has_colors()) wattroff(game->env->wgame, COLOR_PAIR(8) | A_BOLD);
            } else if (i < wgame_height / 3 && ((i * 11 + j + game->score) % 29 == 0)) {
                if (has_colors()) wattron(game->env->wgame, COLOR_PAIR(10) | A_DIM);
                mvwaddch(game->env->wgame, i, j, '.');
                if (has_colors()) wattroff(game->env->wgame, COLOR_PAIR(10) | A_DIM);
            } else if (i == wgame_height / 2 + 1 && (j % 7 == 0)) {
                if (has_colors()) wattron(game->env->wgame, COLOR_PAIR(5) | A_DIM);
                mvwaddch(game->env->wgame, i, j, '-');
                if (has_colors()) wattroff(game->env->wgame, COLOR_PAIR(5) | A_DIM);
            } else if (i == wgame_height - 2) {
                if (has_colors()) wattron(game->env->wgame, COLOR_PAIR(4) | A_DIM);
                mvwaddch(game->env->wgame, i, j, ((j + game->score / 2) % 6 < 3) ? '_' : ' ');
                if (has_colors()) wattroff(game->env->wgame, COLOR_PAIR(4) | A_DIM);
            } else {
                mvwaddch(game->env->wgame, i, j, ' ');
            }
        }
    }

    for (int i = 0; i < game->player_count; i++) {
        int pair = __player_color_pair__(game->players[i]->character);
        if (has_colors()) wattron(game->env->wgame, COLOR_PAIR(pair) | A_BOLD);
        mvwaddwstr(game->env->wgame, positions[i][0], positions[i][1],
                   __resolve_character__(&game->players[i]->character));
        if (has_colors()) wattroff(game->env->wgame, COLOR_PAIR(pair) | A_BOLD);
    }

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

void __initial_screen__(Game *game, int wgame_height, int wgame_width) {
    Environment *env = game->env;
    for (int i = 0; i < wgame_height; i++) memset(env->map[i], ' ', wgame_width);

    werase(env->wgame);
    __draw_panel__(env->wgame, " DANGER DASH ", 6);

    const char *title = GAME_TITLE;
    const char *version = GAME_VERSION;
    const char *start_msg = "PRESS SPACE TO START";
    const char *controls_msg = "ARROW KEYS TO MOVE";
    const char *quit_msg = "BACKSPACE: PAUSE / RESUME    ESC: QUIT";
    const char *tagline = "Dodge fast. Stay alive. Push the score.";

    int title_y = wgame_height / 2 - 4;
    int version_y = title_y + 1;
    int line_y = title_y + 2;
    int tagline_y = title_y + 3;
    int controls_y = tagline_y + 2;
    int start_y = controls_y + 2;
    int quit_y = start_y + 1;

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(env->wgame, title_y, (wgame_width - (int)strlen(title)) / 2, "%s", title);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(1) | A_BOLD);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(2));
    mvwprintw(env->wgame, version_y, (wgame_width - (int)strlen(version)) / 2, "%s", version);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(2));

    if (wgame_width > 10) {
        int line_len = wgame_width > 40 ? 36 : wgame_width - 8;
        mvwhline(env->wgame, line_y, (wgame_width - line_len) / 2, ACS_HLINE, line_len);
    }

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(7) | A_DIM);
    mvwprintw(env->wgame, tagline_y, (wgame_width - (int)strlen(tagline)) / 2, "%s", tagline);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(7) | A_DIM);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(env->wgame, controls_y, (wgame_width - (int)strlen(controls_msg)) / 2, "%s", controls_msg);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(4) | A_BOLD);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(env->wgame, start_y, (wgame_width - (int)strlen(start_msg)) / 2, "%s", start_msg);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(3) | A_BOLD);

    mvwprintw(env->wgame, quit_y, (wgame_width - (int)strlen(quit_msg)) / 2, "%s", quit_msg);
}

void __adjust_map__(Game *game, int wgame_height, int wgame_width) {
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width - 1; j++) {
            game->env->map[i][j] = game->env->map[i][j + 1];
        }
        game->env->map[i][wgame_width - 1] = ' ';
    }
    double r = (double)rand() / RAND_MAX;

    int obstacle_type = rand() % 3;
    if (r < (double)OBSTACLE_ODDS && obstacle_type == 0) {
        int obstacle_placement = rand() % 2;
        if (obstacle_placement == 0) {
            int middle_y = wgame_height / 2;
            for (int i = 0; i < game->player_count; i++)
                game->env->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][0];
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

    game->score++;
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

    switch (key) {
        case KEY_UP:
        case ' ':       // ← JUMP FIX: space maps to 'w' same as up arrow
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

    if (new_yx == NULL) return NULL;
    move_player(new_yx, key, start_y, start_x, lines, cols);
    if (new_yx == NULL) {
        pthread_mutex_lock(&player->lock);
        if (player->state == BUSY) player->state = ACTIVE;
        pthread_mutex_unlock(&player->lock);
        return NULL;
    }

    int target_y = new_yx[0];
    int target_x = new_yx[1];
    free(new_yx);

    if (start_y > target_y) {
        for (int i = start_y; i >= target_y; --i) {
            pthread_mutex_lock(&player->lock);
            if (player == NULL) return NULL;
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

    __draw_panel__(game->env->wstatus, " RUN OVER ", 1);
    __draw_panel__(game->env->winfo, " NEXT STEP ", 3);

    {
        int sw = getmaxx(game->env->wstatus);
        const char *over = "GAME OVER";
        char score_line[64];
        snprintf(score_line, sizeof(score_line), "Final Score: %d", game->score);

        if (has_colors()) wattron(game->env->wstatus, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(game->env->wstatus, 1, (sw - (int)strlen(over)) / 2, "%s", over);
        if (has_colors()) wattroff(game->env->wstatus, COLOR_PAIR(1) | A_BOLD);

        if (has_colors()) wattron(game->env->wstatus, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(game->env->wstatus, 2, (sw - (int)strlen(score_line)) / 2, "%s", score_line);
        if (has_colors()) wattroff(game->env->wstatus, COLOR_PAIR(3) | A_BOLD);
    }

    mvwprintw(game->env->winfo, 1, 2, "Press any key to restart");
    mvwprintw(game->env->winfo, 2, 2, "Press q to quit");

    move(LINES - 1, 0);
    clrtoeol();
    mvprintw(LINES - 1, 0, "Exiting game... Press any key to restart, press q to quit.");

    __refresh_all_windows__(game);
    timeout(-1);
}

void deinit(Game *game) {
    for(int i = 0; i < getmaxy(game->env->wgame); i++) free(game->env->map[i]);
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