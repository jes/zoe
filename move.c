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

/* return 1 if the given string is an xboard move and 0 otherwise */
int is_xboard_move(const char *move) {
    /* fail if the move is too long or short */
    if(strlen(move) < 4 || strlen(move) > 5)
        return 0;

    /* fail if the co-ordinates are invalid */
    if(move[0] < 'a' || move[0] > 'h' || move[1] < '1' || move[1] > '8' ||
            move[2] < 'a' || move[2] > 'h' || move[3] < '1' || move[3] > '8')
        return 0;

    /* don't validate the promotion piece... */

    return 1;
}

/* return the move for the given xboard move */
Move get_xboard_move(const char *move) {
    Move m;

    /* set co-ordinates */
    m.begin = ((move[0] - 'a') * 8) + (move[1] - 1);
    m.end = ((move[2] - 'a') * 8) + (move[3] - 1);

    /* set promotion piece */
    switch(move[5]) {
        case 'k': m.promotion = KNIGHT; break;
        case 'b': m.promotion = BISHOP; break;
        case 'r': m.promotion = ROOK;   break;
        case 'q': m.promotion = QUEEN;  break;
        default:  m.promotion = 0;      break;
    }

    return m;
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
