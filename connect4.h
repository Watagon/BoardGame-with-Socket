#pragma once

#include <stdint.h>
#include <stdbool.h>

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
    int col_num, row_num;
} Connect4_t;

void new_game (Connect4_t *game, int row, int col);
int connect4_make_move (Connect4_t *cnct4, int row, int col);
Cell_state_t connec4_cell_state (Connect4_t *cnct4, int row, int col);
