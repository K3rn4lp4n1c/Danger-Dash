#ifndef GAME_H
#define GAME_H

#define _XOPEN_SOURCE_EXTENDED 1

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <ncursesw/curses.h>

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
#define INITIAL_FRAME_RATE 60000 // microseconds (10 FPS)

#ifndef NASM_FUNCTIONS
#define NASM_FUNCTIONS
int check_for_collision(int, int);
void move_player(int *, int, int, int, int, int);
#endif

typedef enum { Benjamin, Ethan, Muhammad, Youssef, } Characters;
typedef enum { INACTIVE, ACTIVE, IDLE, BUSY, } States;

const char OBSTACLES[][3] = {"#@&", "#@&", "#@&", "#@&"}; // 0 = mixed, 1 = air, 2 = land
const double OBSTACLE_ODDS = 0.05; // 10% chance of new obstacle each frame

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
    WINDOW *wstatus; // window for status info (4 bytes on 32-bit, 8 bytes on 64-bit)
    WINDOW *wgame; // window for game area (4 bytes on 32-bit, 8 bytes on 64-bit)
    WINDOW *winfo; // window for player info (4 bytes on 32-bit, 8 bytes on 64-bit)
    char **map; // 2D array representing the game map (pointer to pointer, 4 bytes on 32-bit, 8 bytes on 64-bit)
    unsigned long frame_rate; // frame rate in microseconds (4 bytes on 32-bit, 8 bytes on 64-bit)
} Environment;

typedef struct {
    int player_count; // number of active players (4 bytes)
    int score; // total score (4 bytes)
    Environment *env; // pointer to the environment (4 bytes on 32-bit, 8 bytes on 64-bit)
    States state; // current game state (4 bytes)
    pthread_t input; // thread for handling input (4 bytes on 32-bit, 8 bytes on 64-bit)
    Player *players[MAX_PLAYERS]; // array of pointers to players (4 bytes per pointer on 32-bit, 8 bytes per pointer on 64-bit)
} Game;

// Available functions to be called from NASM assembly
Game* init();
void helloWorld(), update(Game *), run(Game *), end(Game *), deinit(Game *);

// Internal helper functions
void __refresh_all_windows__(Game *), __initialize_curses__();
void __show_initial_screen__(Game *, int, int), __adjust_map__(Game *, int, int);
void* __keypress__(void *), *__player_effect__(void *);
const wchar_t* __resolve_character__(Characters*);

#endif