#ifndef GAME_H
#define GAME_H

#define _XOPEN_SOURCE_EXTENDED 1

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <ncursesw/curses.h>

#define WELCOME_MSG "Hello World!"
#define GAME_TITLE "Danger Dash"
#define GAME_VERSION "0.0.1-alpha"
#define MAX_PLAYERS 4
#define MAX_NAME_LENGTH 20
#define INITIAL_FRAME_RATE 50000 /* ~20 FPS */

#ifndef NASM_FUNCTIONS
#define NASM_FUNCTIONS
int check_for_collision(int, int);
void move_player(int *, int, int, int, int, int);
#endif

typedef enum { Benjamin, Ethan, Muhammad, Youssef, } Characters;
typedef enum { INACTIVE, ACTIVE, IDLE, BUSY, } States;
typedef enum {
    UI_TITLE = 1,
    UI_STATUS,
    UI_ACCENT,
    UI_PLAYER_1,
    UI_PLAYER_2,
    UI_PLAYER_3,
    UI_PLAYER_4,
    UI_OBSTACLE
} ThemePairs;

const char OBSTACLES[][3] = {"#@&", "#@&", "#@&", "#@&"}; /* 0 = mixed, 1 = air, 2 = land */
const double OBSTACLE_ODDS = 0.05; /* 5% chance of new obstacle each frame */

typedef struct {
    char name[MAX_NAME_LENGTH];
    int x, y;
    Characters character;
    States state;
    pthread_mutex_t lock;
    pthread_t thread;
} Player;

typedef struct {
    Player *player;
    int key;
    unsigned long frame_rate;
} Input;

typedef struct {
    WINDOW *wstatus;
    WINDOW *wgame;
    WINDOW *winfo;
    char **map;
    unsigned long frame_rate;
} Environment;

typedef struct {
    int player_count;
    int score;
    Environment *env;
    States state;
    pthread_t input;
    pthread_mutex_t lock;
    Player *players[MAX_PLAYERS];
} Game;

Game* init(int, char **, Characters *);
void helloWorld(), update(Game *), run(Game *), end(Game *), deinit(Game *);

void __refresh_all_windows__(Game *), __initialize_curses__();
void __initial_screen__(Game *), __adjust_map__(Game *);
void __erase_all_windows__(Environment *);
void* __keypress__(void *), *__player_effect__(void *);
const wchar_t* __resolve_character__(Characters*);
int __resolve_character_color__(Characters);
const char* __resolve_state__(States);
void __draw_status__(Game *, int);
void __draw_info__(Game *);
void __render_world__(Game *, int, int);

#endif
