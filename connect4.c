#include "connect4.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

void new_game (Connect4_t *game, int col_num, int row_num)
{
    assert(0<col_num && 0<row_num);
    assert(col_num*row_num <= sizeof(uint64_t)*CHAR_BIT);

    game->black = 0;
    game->white = 0;
    game->state = BLACK_MOVE;

    game->col_num = col_num;
    game->row_num = row_num;
}

static bool
is_valid_move (Connect4_t *cnct4, int row, int col)
{
    assert(0<row && row<cnct4->row_num);
    assert(0<col && col<cnct4->col_num);

    if (row == cnct4->row_num-1)
        return true;
    
    uint64_t filled = cnct4->black|cnct4->white;
    uint64_t empty  = ~filled;
    uint64_t mask   = (uint64_t)1<<row*cnct4->col_num + col;
    uint64_t below  = mask<<cnct4->col_num;

    if (empty&mask && filled&below)
        return true;
    else
        return false;
}

int
connect4_make_move (Connect4_t *cnct4, int row, int col)
{
    if (!is_valid_move(cnct4, row, col))
        return -1; 
    
    uint64_t new_cell = (uint64_t)1<<row*cnct4->col_num+col;

    uint64_t *cell_bits;
    switch (cnct4->state)
    {
    case BLACK_MOVE:
        cnct4->black |= new_cell;
        break;
    case WHITE_MOVE:
        cnct4->white |= new_cell;
    default:
        break;
    }
    
    return 0;
}


Cell_state_t
connect4_cell_state (Connect4_t *cnct4, int row, int col)
{
    uint64_t mask = (uint64_t)1<<row*cnct4->col_num + col;

    if (mask&cnct4->black)
        return CELL_BLACK;
    else if (mask&cnct4->white)
        return CELL_WHITE;
    else
        return CELL_EMPTY;
}
