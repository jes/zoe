/* board handling for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* reset the given board to the initial state */
void reset_board(Board *board) {
    int i;

    /* fill in the empty squares of the mailbox board */
    for(i = 16; i < 56; i++)
        board->mailbox[i] = EMPTY;

    /* fill in the pieces of the mailbox board */
    for(i = 0; i < 2; i++) {
        uint8_t *rank = board->mailbox + i * 56;
        rank[0] = ROOK;
        rank[1] = KNIGHT;
        rank[2] = BISHOP;
        rank[3] = KING;
        rank[4] = QUEEN;
        rank[5] = BISHOP;
        rank[6] = KNIGHT;
        rank[7] = ROOK;

        rank = board->mailbox + 8 + i * 40;
        for(j = 0; j < 8; j++) {
            rank[j] = PAWN;
        }
    }

    board->b[WHITE][PAWN]     = 0x000000000000ff00ull;
    board->b[WHITE][KNIGHT]   = 0x0000000000000042ull;
    board->b[WHITE][BISHOP]   = 0x0000000000000028ull;
    board->b[WHITE][ROOK]     = 0x0000000000000081ull;
    board->b[WHITE][QUEEN]    = 0x0000000000000008ull;
    board->b[WHITE][KING]     = 0x0000000000000010ull;
    board->b[WHITE][OCCUPIED] = 0x000000000000ffffull;

    board->b[BLACK][PAWN]     = 0x00ff000000000000ull;
    board->b[BLACK][KNIGHT]   = 0x4200000000000000ull;
    board->b[BLACK][BISHOP]   = 0x2800000000000000ull;
    board->b[BLACK][ROOK]     = 0x8100000000000000ull;
    board->b[BLACK][QUEEN]    = 0x0800000000000000ull;
    board->b[BLACK][KING]     = 0x1000000000000000ull;
    board->b[BLACK][OCCUPIED] = 0xffff000000000000ull;

    board->occupied = 0xffff00000000ffffull;
}
