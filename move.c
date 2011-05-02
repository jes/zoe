/* move handling for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* return a pointer to a static char array containing the xboard representation
 * of the given move.
 */
char *xboard_move(Move m) {
    static char move[6];
    int i = 0;

    /* start and finish co-ordinates */
    move[i++] = 'a' + m.begin % 8;
    move[i++] = '1' + m.begin / 8;
    move[i++] = 'a' + m.end % 8;
    move[i++] = '1' + m.end / 8;
    
    /* pawn promotions */
    if(m.promotion > 0 && m.promotion < 5)
        move[i++] = ".kbrq"[m.promotion];

    move[i++] = '\0';

    return move;
}

/* apply the given move to the given game */
void apply_move(Game *game, Move m) {
    Board *board = &(game->board);
    int beginpiece, endpiece;
    int begincolour, endcolour;

    /* find the piece from the mailbox */
    beginpiece = board->mailbox[m.begin];
    endpiece = board->mailbox[m.end];

    /* find the colour from white's occupied bitboard */
    begincolour = !(board->b[WHITE][OCCUPIED] & m.begin);
    endcolour = !(board->b[WHITE][OCCUPIED] & m.end);

    /* remove the piece from the begin square */
    board->mailbox[m.begin] = EMPTY;
    board->b[begincolour][beginpiece] ^= m.begin;

    /* remove the piece from the end square */
    if(endpiece != EMPTY)
        board->b[endcolour][endpiece] ^= m.end;

    /* insert the piece at the end square */
    board->mailbox[m.end] = beginpiece;
    board->b[begincolour][beginpiece] |= m.end;

    /* TODO: en passant, castling */
}
