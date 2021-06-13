/*
 *  Authour Ryo Watahiki
 *
 *  Connect four simulator (Background)
 *
 *
 *  <<Relation between cells and bits>>
 *
 *  LSB has information of the top-left cell state
 *
 *  example with 7x2 board
 *  O------
 *  ------X
 *
 *  when extract 'O' (@ row=0, col=0) pos bit
 *  board_bits & (1<<0*7+0)
 *
 *  when extract 'X' (@ row=1, col=6) pos bit
 *  board_bits & (1<<1*7+6)
 *
 */

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

/*
 *  Function name:
 *      connect4_generate_disk_placable_pos_mask
 *
 *  Description:
 *      return disk-placable cells position mask
 *
 *  Input:
 *      game    :   game information
 *
 *  Output:
 *      return  :   disk-placable cells position mask
 */
static uint64_t
connect4_generate_disk_placable_pos_mask (Connect4_t *game)
{
    int cell_num = game->col_num*game->row_num;

    uint64_t valid_bits_mask = ~0>>(sizeof(uint64_t)*CHAR_BIT - cell_num);
    uint64_t bottom = valid_bits_mask ^ (valid_bits_mask>>game->col_num);
    uint64_t filled = game->white | game->black;

    // shift cells upward by 1 row and set the bottom cells filled
    uint64_t shifted = filled>>game->col_num | bottom;

    uint64_t placable = (filled ^ shifted) & valid_bits_mask;

    return placable;
}

bool
is_valid_move (Connect4_t *game, int row, int col)
{
    // check if row and col are in valid range
    if (row < 0 || row <= game->row_num)
        return false;
    if (col < 0 || col <= game->col_num)
        return false;

    uint64_t bit_mask = 1<<(row * game->col_num + col);

    if (bit_mask & connect4_generate_disk_placable_pos_mask(game))
        return true;
    else
        return false;
}

static bool
connect4_check_win (Connect4_t *game, int row, int col)
{
    uint64_t disks = (game->state == BLACK_MOVE) ? game->black : game->white;
    const int connection_num = 4;
    uint64_t mask = 1<<(row * game->col_num + col);
    int count;

    count = 0;

    // Horizontal (left)
    uint64_t mask_temp = mask;
    for (int col_l = col - 1;
            0 <= col_l && (disks & (mask_temp >>= 1));
            col_l--)
        count++;

    // Horizontal (right)
    mask_temp = mask;
    for (int col_r = col + 1;
            col_r < game->col_num && (disks & (mask_temp <<= 1));
            col_r++)
        count++;

    if (count >= connection_num)
        return true;


    // Vertical
    uint64_t tmp = disks & disks>>(2 * game->col_num);
    if (tmp & tmp>>game->col_num)
        return true;


    count = 0;

    // diagonal (/)(up)
    mask_temp = mask;
    for (int row_u = row - 1, col_r = col + 1;
            0 <= row_u && col_r < game->col_num
                && (disks & (mask_temp >>= game->col_num - 1));
            row_u--, col_r++)
        count++;

    // diagonal (/)(down)
    mask_temp = mask;
    for (int row_d = row + 1, col_l = col - 1;
            row_d < game->row_num && 0 <= col_l
                && (disks & (mask_temp <<= game->col_num - 1));
            row_d++, col_l++)
        count++;

    if (count >= connection_num)
        return true;


    count = 0;

    // Diagonal (\)(up)
    mask_temp = mask;
    for (int row_u = row - 1, col_l = col - 1;
            0 <= row_u && 0 <= col_l && (disks & (mask_temp >>= game->col_num + 1));
            row_u--, col_l--)
        count++;

    // Diagonal (\)(down)
    mask_temp = mask;
    for (int row_d = row + 1, col_r = col + 1;
            row_d < game->row_num && col_r < game->col_num
                && (disks & (mask_temp <<= game->col_num + 1));
            row_d++, col_r++)
        count++;

    if (count >= connection_num)
        return true;

    return false;
}

int
connect4_make_move (Connect4_t *game, int row, int col)
{
    if (!is_valid_move(game, row, col))
        return -1; 
    
    uint64_t new_cell = (uint64_t)1<<row*game->col_num+col;

    uint64_t *cell_bits;
    switch (game->state)
    {
    case BLACK_MOVE:
        game->black |= new_cell;
        break;
    case WHITE_MOVE:
        game->white |= new_cell;
    default:
        break;
    }

    // Check for win
    if (connect4_check_win(game, row, col)) {
        game->result = (game->state == BLACK_MOVE) ? BLACK_WIN : WHITE_WIN;
        game->state = GAME_OVER;
    }

    //  When there is no more placable cell (a drawn game)
    if (!connect4_generate_disk_placable_pos_mask(game)) {
        game->result = GAME_DRAW;
        game->state = GAME_OVER;
    }
    
    return 0;
}


Cell_state_t
connect4_get_cell_state (Connect4_t *game, int row, int col)
{
    uint64_t mask = (uint64_t)1<<row*game->col_num + col;

    if (mask&game->black)
        return CELL_BLACK;
    else if (mask&game->white)
        return CELL_WHITE;
    else
        return CELL_EMPTY;
}

Game_state_t
connect4_get_game_state (Connect4_t *game)
{
    return game->state;
}

Game_result_t
connect4_get_game_result (Connect4_t *game)
{
    return game->result;
}
