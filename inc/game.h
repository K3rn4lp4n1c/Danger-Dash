#ifndef GAME_H
#define GAME_H

/*
This is the header file for the game module.
This file contains the declarations for structs, constants, and functions related to the game logic.
You can add more game-related declarations here that can be used in both C and NASM assembly files.
However, function definitions should go in the corresponding game.c file.
*/

#define HELLO_WORLD_MSG "Hello World!"

typedef struct {
    int year;
} Game;

void helloWorld();

#endif