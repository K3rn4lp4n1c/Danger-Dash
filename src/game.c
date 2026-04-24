#include "game.h"

// Initializes the game state, resources, etc.
Game* init() {
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    WINDOW *wstatus = newwin(LINES / 3, COLS, 0, 0);
    box(wstatus, 0, 0);
    
    WINDOW *wgame = newwin(LINES / 3, COLS, LINES / 3, 0);

    WINDOW *winfo = newwin(LINES / 3, COLS, 2 * LINES / 3, 0);
    box(winfo, 0, 0);

    Environment *env = malloc(sizeof(Environment));
    env->wstatus = wstatus;
    env->wgame = wgame;
    env->winfo = winfo;
    env->frame_rate = 10000; // ~10ms per frame
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
    game->player_count = 1; // for now, we'll have just one player
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
        if (rand() % 100 < OBSTACLE_ODDS) {
            int wgame_height = getmaxy(game->environment->wgame);
            int wgame_width = getmaxx(game->environment->wgame);
            __place_obstacle__(game, wgame_height, wgame_width);
            for (int i = 0; i < wgame_height; i++) {
                for (int j = 0; j < wgame_width; j++) {
                    mvwprintw(game->environment->wgame, i, j, "%c", game->environment->map[i][j]);
                }
            }
        }
    } else __show_initial_screen__(game->environment);
    
    for (int i = 0; i < game->player_count; i++) {
        mvwprintw(game->environment->wstatus, 1, 1, "Score: %d", game->players[i]->score);
        mvwprintw(game->environment->wgame, game->players[i]->y, game->players[i]->x, "%lc", __resolveCharacter__(&(game->players[i]->character)));
    }

    __refresh_all_windows__(game);

    refresh();
}

void run(Game *game) {
    // for (int i = 0; i < game->player_count; i++) {
    //     pthread_create(&(game->players[i]->keystroke), NULL, (void *)displace, (void *)&(game->players[i]));
    // }
    printf("%s", game->players[0]->name); // dummy printf to prevent compiler optimization of player struct
    game->environment->seed = time(NULL);
    srand(game->environment->seed);

    for (int i = 0; i < game->player_count; i++) {
        mvwprintw(game->environment->winfo, 1, 1, "Player: %s", game->players[i]->name); 
    }

    while (1) {
        update(game);
        usleep(game->environment->frame_rate);
    }
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
    if (check_for_collision(player->x, player->y)) {
        // Handle collision
    }
}

void end(Game *game) {
    // Placeholder for any end-of-game logic, such as displaying a game over screen
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

void __start_curses_colors__() {
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

void __show_initial_screen__(Environment *env) {
    int wgame_height = getmaxy(env->wgame);
    int wgame_width = getmaxx(env->wgame);
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
    wrefresh(env->wgame);
}

void __place_obstacle__(Game *game, int wgame_height, int wgame_width) {
    // shift all existing obstacles to the left
    for (int i = 0; i < wgame_height; i++) {
        for (int j = 0; j < wgame_width - 1; j++) {
            game->environment->map[i][j] = game->environment->map[i][j + 1];
        }
        game->environment->map[i][wgame_width - 1] = ' ';
    }
    int obstacle_type = rand() % 3; // 0 = mixed, 1 = air, 2 = land
    if (obstacle_type == 0) {
        // mixed obstacle
        int obstacle_placement = rand() % 2; // 0 = middle, 1 = bottom
        if (obstacle_placement == 0) {
            // Place obstacle in the middle row of the last column
            int middle_y = wgame_height / 2;
            for (int i = 0; i < game->player_count; i++) {
                game->environment->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][0];
            }
        } else {
            // Place obstacle at the bottom row of the last column
            int bottom_y = wgame_height - 2; // -2 to account for box borders
            for (int i = 0; i < game->player_count; i++) {
                game->environment->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
            }
        }
    } else if (obstacle_type == 1) {
        // air obstacle
        int middle_y = wgame_height / 2;
        for (int i = 0; i < game->player_count; i++) {
            game->environment->map[middle_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][1];
        }
    } else {
        // land obstacle
        int bottom_y = wgame_height - 2; // -2 to account for box borders
        for (int i = 0; i < game->player_count; i++) {
            game->environment->map[bottom_y][wgame_width - 1] = OBSTACLES[game->players[i]->character][2];
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