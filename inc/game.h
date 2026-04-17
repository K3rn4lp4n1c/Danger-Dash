#ifndef GAME_H
#define GAME_H

#include <curses.h>
#include <string.h>

/*
This is the header file for the game module.
This file contains the declarations for structs, constants, and functions related to the game logic.
You can add more game-related declarations here that can be used in both C and NASM assembly files.
However, function definitions should go in the corresponding game.c file.
*/

#define WELCOME_MSG "Hello World!"

WINDOW *wmain;

typedef struct {
    char name[50];
    char version[15];
    int  year_of_release;
} Game;

void helloWorld();

#endif