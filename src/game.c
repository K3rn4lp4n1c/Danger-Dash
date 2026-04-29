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
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN, COLOR_BLACK);
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
    else if(gState == ACTIVE && active_players < game->player_count) game->state = IDLE;
    else if (gState == IDLE && active_players == game->player_count) game->state = ACTIVE;
    pthread_mutex_unlock(&game->lock);

    if (gState == ACTIVE) __adjust_map__(game, wgame_height, wgame_width);

    for (int i = 0; i < game->player_count; i++) {
        if (has_colors()) wattron(game->env->wstatus, COLOR_PAIR(6) | A_DIM);
        mvwprintw(game->env->wstatus, 1, 1, "SCORE");
        if (has_colors()) wattroff(game->env->wstatus, COLOR_PAIR(6) | A_DIM);
        if (has_colors()) wattron(game->env->wstatus, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(game->env->wstatus, 2, 1, "%d", game->score);
        if (has_colors()) wattroff(game->env->wstatus, COLOR_PAIR(3) | A_BOLD);
        mvwaddwstr(game->env->wgame, positions[i][0], positions[i][1], __resolve_character__(&game->players[i]->character));
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

    box(env->wgame, 0, 0);

    const char *title = GAME_TITLE;
    const char *version = GAME_VERSION;
    const char *start_msg = "Press SPACE to start";
    const char *quit_msg = "Press BACKSPACE during game to pause/resume, ESC to quit";
    const char *controls_msg = "Move: Arrow Keys";

    int title_y = wgame_height / 2 - 3;
    int version_y = title_y + 1;
    int controls_y = title_y + 3;
    int start_y = controls_y + 2;
    int quit_y = start_y + 1;

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(4) | A_DIM);
    for (int x = 1; x < wgame_width - 1; x++)
        mvwaddch(env->wgame, title_y - 2, x, '=');
    for (int x = 1; x < wgame_width - 1; x++)
        mvwaddch(env->wgame, quit_y + 2, x, '=');
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(4) | A_DIM);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(env->wgame, title_y, (wgame_width - (int)strlen(title)) / 2, "%s", title);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(1) | A_BOLD);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(2));
    mvwprintw(env->wgame, version_y, (wgame_width - (int)strlen(version)) / 2, "%s", version);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(2));

    mvwprintw(env->wgame, controls_y, (wgame_width - (int)strlen(controls_msg)) / 2, "%s", controls_msg);

    if (has_colors()) wattron(env->wgame, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(env->wgame, start_y, (wgame_width - (int)strlen(start_msg)) / 2, "%s", start_msg);
    if (has_colors()) wattroff(env->wgame, COLOR_PAIR(3) | A_BOLD);

    if (has_colors()) wattron(env->wgame, A_DIM);
    mvwprintw(env->wgame, quit_y, (wgame_width - (int)strlen(quit_msg)) / 2, "%s", quit_msg);
    if (has_colors()) wattroff(env->wgame, A_DIM);
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
    mvwprintw(game->env->wstatus, 1, 1, "Game Over! Final Score");
    mvwprintw(game->env->wstatus, 2, 1, "Score: %d", game->score);
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