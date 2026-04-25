#include "game.h"

// Initializes the game state, resources, etc.
Game* init() {
    __initialize_curses__();
    WINDOW *wstatus = newwin(LINES / 3, COLS, 0, 0);
    box(wstatus, 0, 0);
    
    WINDOW *wgame = newwin(LINES / 3, COLS, LINES / 3, 0);

    WINDOW *winfo = newwin(LINES / 3, COLS, 2 * LINES / 3, 0);
    box(winfo, 0, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus = wstatus;
    env->wgame = wgame;
    env->winfo = winfo;
    env->frame_rate = INITIAL_FRAME_RATE;
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
    game->env = env;
    game->player_count = 1; // for now, we'll have just one player
    for (int i = 0; i < MAX_PLAYERS; i++) game->players[i] = NULL;
    

    for (int i = 0; i < game->player_count; i++) {
        Player *player = malloc(sizeof(Player));
        char player_name[MAX_NAME_LENGTH];
        snprintf(player_name, sizeof(player_name), "Player%d", i + 1);
        strcpy(player->name, player_name);
        player->x = 1;
        player->y = getmaxy(wgame) - 1; // start on the ground
        player->score = 0;
        player->character = Benjamin;
        game->players[i] = player;
        pthread_mutex_init(&player->lock, NULL);
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
    int wgame_height = getmaxy(game->env->wgame);
    int wgame_width = getmaxx(game->env->wgame);
    if (game->env->seed != 0) __adjust_map__(game, wgame_height, wgame_width);
    else __show_initial_screen__(game->env, wgame_height, wgame_width);
    for (int i = 0; i < game->player_count; i++) {
        game->players[i]->score++;
        mvwprintw(game->env->wstatus, 1, 1, "Score: %d", game->players[i]->score);
        mvwaddch(game->env->wgame, game->players[i]->y, game->players[i]->x, __resolveCharacter__(&(game->players[i]->character)));
    }

    __refresh_all_windows__(game);
}

void run(Game *game) {
    pthread_create(&game->input, NULL, __keypress__, (void*)game);
    game->env->seed = time(NULL);
    srand(game->env->seed);

    for (int i = 0; i < game->player_count; i++) {
        mvwprintw(game->env->winfo, 1, 1, "Player: %s", game->players[i]->name); 
    }
    box(game->env->wstatus, 0, 0);
    box(game->env->winfo, 0, 0);

    while (1) {
        update(game);
        usleep(game->env->frame_rate);
    }
}

void* __keypress__(void *arg) {
    Game *game = (Game *)arg;
    while (1) {
        int ch = getch();
        Player *player = NULL;
        if (ch > KEY_MIN && ch < KEY_MAX) {
            if (game->player_count > 0 && ch == KEY_BACKSPACE) {
                game->players[0]->y = game->players[0]->x = 0;
                continue;
            }
            player = game->players[0];
        } else if (game->player_count > 3 && ch >= '0' && ch <= '9') {
            if (ch == '0') {
                game->players[3]->y = game->players[3]->x = 0;
                continue;
            }
            player = game->players[3]; 
        } else if (game->player_count > 2 && (ch >= 'i' && ch <= 'p' || ch >= 'I' && ch <= 'P')) {
            if (ch == 'p' || ch == 'P') {
                game->players[2]->y = game->players[2]->x = 0;
                continue;
            }
            player = game->players[2]; 
        } else if (game->player_count > 1) {
            if (ch == 'q' || ch == 'Q') {
                game->players[1]->y = game->players[1]->x = 0;
                continue;
            }
            player = game->players[1];
        }
        if (player == NULL) continue;
        pthread_mutex_lock(&(player->lock));
        player->key = ch;
        pthread_create(&player->thread, NULL, __player_effect__, (void*)player);
    }
}

void* __player_effect__(void *arg) {
    pthread_detach(pthread_self());
    Player *player = (Player *)arg;
    int lines = LINES / 3, cols = COLS;
    int *new_yx = displace(player->key, player->x, player->y, lines, cols);
    if (player->y > new_yx[0]) {
        // from ground to air, need to animate the jump
        for (int i = player->y; i >= new_yx[0]; i--) {
            player->y = i;
            usleep(100000);
        }
        // then fall back down
        for (int i = player->y; i <= lines - 1; i++) {
            player->y = i;
            usleep(100000);
        }
    } else if (player->y < new_yx[0]) {
        // from air to ground, need to animate the fall
        for (int i = player->y; i <= new_yx[0]; i++) {
            player->y = i;
            usleep(50000);
        }
    } else {
        player->y = new_yx[0];
    }
    player->x = new_yx[1];
    free(new_yx);
    pthread_mutex_unlock(&(player->lock));
    pthread_exit(NULL);
}

int* displace(int key, int x, int y, int max_y, int max_x) {

    switch (key) {
        case KEY_UP:
        case 'w':
        case 'W':
        case 'i':
        case 'I':
        case '2':
            // ensure player is on ground
            y = (y == max_y - 1) ? max_y / 2 : y;
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
        case 'k':
        case 'K':
        case '8':
            // ensure player is in the air
            y = (y < max_y - 1) ? max_y - 1 : y;
            break;
        case KEY_LEFT:
        case 'a':
        case 'A':
        case 'j':
        case 'J':
        case '4':
            // prevent moving into the left wall
            x = (x > 1) ? x - 1 : x;
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
        case 'l':
        case 'L':
        case '6':
            // prevent moving into the right wall
            x = (x < max_x - 1) ? x + 1 : x;
            break;
    }
    int *new_yx = malloc(2 * sizeof(int));
    new_yx[0] = y;
    new_yx[1] = x;
    return new_yx;
}

void end(Game *game) {
    // Placeholder for any end-of-game logic, such as displaying a game over screen
    pthread_join(game->input, NULL);
    werase(game->env->wstatus);
    werase(game->env->wgame);
    werase(game->env->winfo);
    mvwprintw(game->env->wstatus, 1, 1, "Game Over! Final Score");
    for(int i = 0; i < game->player_count; i++) {
        mvwprintw(game->env->wstatus, 2 + i, 1, "%s: %d", game->players[i]->name, game->players[i]->score);
    }
    __refresh_all_windows__(game);
    mvprintw(LINES - 1, 0, "Exiting game... Press any key to continue.");
    getch();
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
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN, COLOR_BLACK);
    }
}

void __show_initial_screen__(Environment *env, int wgame_height, int wgame_width) {
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width; j++) {
            env->map[i][j] = ' ';
        }
    }

    box(env->wgame, 0, 0);

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

    mvwprintw(env->wgame, quit_y, (wgame_width - (int)strlen(quit_msg)) / 2, "%s", quit_msg);
}

void __adjust_map__(Game *game, int wgame_height, int wgame_width) {
    // shift all existing obstacles to the left
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width - 1; j++) {
            game->env->map[i][j] = game->env->map[i][j + 1];
        }
        game->env->map[i][wgame_width - 1] = ' ';
    }
    double r = (double)rand() / RAND_MAX;

    int obstacle_type = rand() % 3; // 0 = mixed, 1 = air, 2 = land
    if (r < (double)OBSTACLE_ODDS && obstacle_type == 0) {
        // mixed obstacle
        int obstacle_placement = rand() % 2; // 0 = middle, 1 = bottom
        if (obstacle_placement == 0) {
            // Place obstacle in the middle row of the last column
            int middle_y = wgame_height / 2;
            for (int i = 0; i < game->player_count; i++) {
                game->env->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][0];
            }
        } else {
            // Place obstacle at the bottom row of the last column
            int bottom_y = wgame_height - 1; // -2 to account for box borders
            for (int i = 0; i < game->player_count; i++) {
                game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
            }
        }
    } else if (r < (double)OBSTACLE_ODDS && obstacle_type == 1) {
        // air obstacle
        int middle_y = wgame_height / 2;
        for (int i = 0; i < game->player_count; i++) {
            game->env->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
        }
    } else if (r < (double)OBSTACLE_ODDS && obstacle_type == 2) {
        // land obstacle
        int bottom_y = wgame_height - 1; // -2 to account for box borders
        for (int i = 0; i < game->player_count; i++) {
            game->env->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][2];
        }
    }
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width; j++) {
            mvwaddch(game->env->wgame, i, j, game->env->map[i][j]);
        }
    }
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