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

typedef enum { Benjamin, Ethan, Muhammad, } Characters;

typedef struct {
    char name[10];
    int x, y;
    Characters character;
    pthread_t keystroke;
} Player;

typedef struct {
    WINDOW *wstatus;
    WINDOW *wgame;
    WINDOW *winfo;
} Environment;

typedef struct {
    int score;
    Player *player;
    Environment *environment;
} Game;

Game* init();
void helloWorld(), update(Game *), run(Game *), displace(Player *), end(Game *), deinit(Game *);
char32_t __resolveCharacter__(Characters character);

#ifndef check_for_collision
int check_for_collision(int player_x, int player_y);
#endif

#endif