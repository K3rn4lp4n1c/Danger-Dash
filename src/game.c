#include <stdio.h>
#include "game.h"

void helloWorld() {
    Game game;
    game.year = 2026;
    printf("%s Year: %d\n", HELLO_WORLD_MSG, game.year);
}