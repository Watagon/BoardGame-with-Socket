#include "othello.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

void new_game (Connect4_t *game, int row, int col)
{
    assert(0<row && 0<col);
    assert(row*col <= sizeof(int64_t)*CHAR_BIT);

    game->black = 0;
    game->white = 0;
    game->state = BLACK_MOVE;

    game->size_x = row;
    game->size_y = col;
}
