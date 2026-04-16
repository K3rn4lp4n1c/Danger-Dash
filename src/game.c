#include <stdio.h>
#include "game.h"

// A simple function to demonstrate calling a C function from NASM assembly
void helloWorld() {
    Game game;
    game.year = 2026;
    printf("%s. We are in the year %d\n", HELLO_WORLD_MSG, game.year);
}

// You can add more game-related functions here that can be called from assembly or other C files