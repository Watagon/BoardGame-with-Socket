#pragma once

#include <stdint.h>

typedef enum {
    BLACK_MOVE,
    WHITE_MOVE,
    GAME_OVER
} Game_state_t;

typedef enum {
    CELL_BLACK,
    CELL_WHITE,
    CELL_EMPTY
} Cell_state_t;

typedef struct othello {
    uint64_t black;
    uint64_t white;
    Game_state_t state;
    int size_x, size_y;
} Othello_t;

void new_game (Othello_t *game, int row, int col);
