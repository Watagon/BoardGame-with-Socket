#include "connect4.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

void new_game (Connect4_t *game, int col_num, int row_num)
{
    assert(0<col_num && 0<row_num);
    assert(col_num*row_num <= sizeof(int64_t)*CHAR_BIT);

    game->black = 0;
    game->white = 0;
    game->state = BLACK_MOVE;

    game->col = col_num;
    game->row = row_num;
}
