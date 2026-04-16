#include <stdio.h>
#include "game.h"

void helloWorld() {
    Game game;
    game.year = 2026;
    printf("%s. We are in the year %d\n", HELLO_WORLD_MSG, game.year);
}