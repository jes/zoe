/* board handling for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* reset the given board to the initial state */
void reset_board(Board *board) {
    board->b[WHITE][PAWNS]    = 0x000000000000ff00ull;
    board->b[WHITE][KNIGHTS]  = 0x0000000000000042ull;
    board->b[WHITE][BISHOPS]  = 0x0000000000000028ull;
    board->b[WHITE][ROOKS]    = 0x0000000000000081ull;
    board->b[WHITE][QUEENS]   = 0x0000000000000008ull;
    board->b[WHITE][KING]     = 0x0000000000000010ull;
    board->b[WHITE][OCCUPIED] = 0x000000000000ffffull;

    board->b[BLACK][PAWNS]    = 0x00ff000000000000ull;
    board->b[BLACK][KNIGHTS]  = 0x4200000000000000ull;
    board->b[BLACK][BISHOPS]  = 0x2800000000000000ull;
    board->b[BLACK][ROOKS]    = 0x8100000000000000ull;
    board->b[BLACK][QUEENS]   = 0x0800000000000000ull;
    board->b[BLACK][KING]     = 0x1000000000000000ull;
    board->b[BLACK][OCCUPIED] = 0xffff000000000000ull;

    board->occupied = 0xffff00000000ffffull;
}
