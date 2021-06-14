#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BLACK_WIN,
    WHITE_WIN,
    GAME_DRAW
} Game_result_t;

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
    Game_result_t result;
    int col_num, row_num;
} Connect4_t;

void new_game (Connect4_t *game, int col_num, int row_num);
int connect4_make_move (Connect4_t *game, int row, int col);
bool is_valid_move (Connect4_t *game, int row, int col);
Cell_state_t connect4_get_cell_state (Connect4_t *game, int row, int col);
Game_state_t connect4_get_game_state (Connect4_t *game);
Game_result_t connect4_get_game_result (Connect4_t *game);
Game_result_t connect4_get_my_win_result_value (Game_state_t my_move);
