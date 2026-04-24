#ifndef GAME_H
#define GAME_H

#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <uchar.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

/*
This is the header file for the game module.
This file contains the declarations for structs, constants, and functions related to the game logic.
You can add more game-related declarations here that can be used in both C and NASM assembly files.
However, function definitions should go in the corresponding game.c file.
*/

#define WELCOME_MSG "Hello World!"
#define GAME_TITLE "Danger Dash"
#define GAME_VERSION "0.0.1-alpha"
#define MAX_PLAYERS 4
#define MAX_NAME_LENGTH 20

#ifndef NASM_FUNCTIONS
#define NASM_FUNCTIONS
int check_for_collision( int player_x, int player_y );
#endif

typedef enum { Benjamin, Ethan, Muhammad, } Characters;

const char OBSTACLES[][3] = {"#@&", "#@&", "#@&"}; // 0 = mixed, 1 = air, 2 = land
const double OBSTACLE_ODDS = 0.1; // 10% chance of new obstacle each frame

typedef struct {
    char name[MAX_NAME_LENGTH];
    int x, y;
    int score;
    Characters character;
    pthread_t keystroke;
} Player;

typedef struct {
    WINDOW *wstatus;
    WINDOW *wgame;
    WINDOW *winfo;
    char **map;
    time_t seed;
    unsigned long frame_rate;
} Environment;

typedef struct {
    int player_count;
    Player *players[MAX_PLAYERS];
    Environment *env;
} Game;

// Available functions to be called from NASM assembly
Game* init();
void helloWorld(), update(Game *), run(Game *), displace(Player *), end(Game *), deinit(Game *);

// Internal helper functions
void __clear_all_windows__(Game *), __refresh_all_windows__(Game *);
void __start_curses_colors__();
void __show_initial_screen__(Environment *, int, int), __adjust_map__(Game *, int, int);
char32_t __resolveCharacter__(Characters*);

#endif