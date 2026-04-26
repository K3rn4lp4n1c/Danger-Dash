#ifndef GAME_H
#define GAME_H

#define _GNU_SOURCE
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
#define INITIAL_FRAME_RATE 100000 // microseconds (10 FPS)

#ifndef NASM_FUNCTIONS
#define NASM_FUNCTIONS
int check_for_collision( int player_x, int player_y );
#endif

typedef enum { Benjamin, Ethan, Muhammad, Youssef, } Characters;
typedef enum { INACTIVE, ACTIVE, IDLE, } States;

const char OBSTACLES[][3] = {"#@&", "#@&", "#@&", "#@&"}; // 0 = mixed, 1 = air, 2 = land
const double OBSTACLE_ODDS = 0.05; // 10% chance of new obstacle each frame
const int KEY_MAPPINGS[][4] = {
    // {DOWN, UP, LEFT, RIGHT}
    { KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT }, // 0402 - 0405
    { 's', 'w', 'a', 'd' },
    { 'k', 'i', 'j', 'l' }, // k - l
    { '5', '8', '4', '6' } // '5' - '6'
};

typedef struct {
    char name[MAX_NAME_LENGTH];
    int x, y;
    int score;
    Characters character;
    States state;
    pthread_mutex_t lock;
    pthread_t thread;
    int key;
} Player;

typedef struct {
    WINDOW *wstatus;
    WINDOW *wgame;
    WINDOW *winfo;
    char **map;
    unsigned long frame_rate;
} Environment;

typedef struct {
    int player_count;
    Player *players[MAX_PLAYERS];
    Environment *env;
    States state;
    pthread_t input;
} Game;

// Available functions to be called from NASM assembly
Game* init();
void helloWorld(), update(Game *), run(Game *), end(Game *), deinit(Game *);
int* displace(int, int, int, int, int);

// Internal helper functions
void __refresh_all_windows__(Game *), __initialize_curses__();
void __show_initial_screen__(Game *, int, int), __adjust_map__(Game *, int, int);
void* __keypress__(void *), *__player_effect__(void *);
char32_t __resolve_character__(Characters*);

#endif