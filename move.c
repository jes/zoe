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
    if(m.promote > 0 && m.promote < 5)
        move[i++] = ".kbrq"[m.promote];

    move[i++] = '\0';

    return move;
}

/* return 1 if the given string is an xboard move and 0 otherwise */
int is_xboard_move(const char *move) {
    /* fail if the move is too long or short */
    if(strlen(move) < 4 || strlen(move) > 5)
        return 0;

    /* fail if the co-ordinates are invalid */
    if(move[0] < 'a' || move[0] > 'h' || move[1] < '1'|| move[1] > '8'
            || move[2] < 'a' || move[2] > 'h' || move[3] < '1'|| move[3] > '8')
        return 0;

    /* don't validate the promotion piece... */

    return 1;
}

/* return the move for the given xboard move */
Move get_xboard_move(const char *move) {
    Move m;

    /* set co-ordinates */
    m.begin = (move[0] - 'a') + ((move[1] - '1') * 8);
    m.end = (move[2] - 'a') + ((move[3] - '1') * 8);

    /* set promotion piece */
    switch(move[5]) {
        case 'k': m.promote = KNIGHT; break;
        case 'b': m.promote = BISHOP; break;
        case 'r': m.promote = ROOK;   break;
        case 'q': m.promote = QUEEN;  break;
        default:  m.promote = 0;      break;
    }

    return m;
}

/* apply the given move to the given game */
void apply_move(Game *game, Move m) {
    Board *board = &(game->board);
    int beginpiece, endpiece;
    int begincolour, endcolour;
    uint64_t beginbit, endbit;

    /* find the piece from the mailbox */
    beginpiece = board->mailbox[m.begin];
    endpiece = board->mailbox[m.end];

    /* find the bits to use */
    beginbit = 1ull << m.begin;
    endbit = 1ull << m.end;

    /* find the colour from white's occupied bitboard */
    begincolour = !(board->b[WHITE][OCCUPIED] & beginbit);
    endcolour = !(board->b[WHITE][OCCUPIED] & endbit);

    /* remove the piece from the begin square */
    board->mailbox[m.begin] = EMPTY;
    board->b[begincolour][beginpiece] ^= beginbit;

    /* remove the piece from the end square */
    if(endpiece != EMPTY) {
        board->b[endcolour][endpiece] ^= endbit;
        board->b[endcolour][OCCUPIED] ^= endbit;
    }

    /* insert the piece at the end square */
    board->mailbox[m.end] = beginpiece;
    board->b[begincolour][beginpiece] |= endbit;

    /* make start square unoccupied */
    board->b[begincolour][OCCUPIED] ^= beginbit;
    board->occupied ^= beginbit;

    /* make end square occupied */
    board->b[begincolour][OCCUPIED] |= endbit;
    board->occupied |= endbit;

    /* TODO: en passant, castling, pawn promotion */

    /* toggle current player */
    game->turn = !game->turn;
}
/* return the set of all squares the given piece is able to move to, without
 * considering a king left in check */
uint64_t generate_moves(Game *game, int tile) {
    Board *board = &(game->board);
    int type = board->mailbox[tile];
    int colour = !(board->b[WHITE][OCCUPIED] & (1ull << tile));
    uint64_t moves = 0;
    int x = tile % 8, y = tile / 8;
    int target;
    uint64_t move;

    switch(type) {
    case PAWN:
        /* TODO: en passant, bitboards */

        /* attack left if it's an enemy */
        if(x > 0) {
            if(colour == WHITE)
                target = tile + 7;
            else
                target = tile - 9;

            move = 1ull << target;
            if(board->b[!colour][OCCUPIED] & move)
                moves |= move;
        }

        /* attack right if it's an enemy */
        if(x < 7) {
            if(colour == WHITE)
                target = tile + 9;
            else
                target = tile - 7;

            move = 1ull << target;
            if(board->b[!colour][OCCUPIED] & move)
                moves |= move;
        }

        /* move forward one square */
        if(colour == WHITE)
            target = tile + 8;
        else
            target = tile - 8;

        move = 1ull << target;
        if(!(board->occupied & move))
            moves |= move;

        /* move forward two squares if we could move one */
        if(((y == 1 && colour == WHITE) || (y == 6 && colour == BLACK))
                && (moves & move)) {
            if(colour == WHITE)
                target = tile + 16;
            else
                target = tile - 16;

            move = 1ull << target;
            if(!(board->occupied & move))
                moves |= move;
        }
        break;

    case KNIGHT:
        moves = knight_moves[tile];
        break;

    case BISHOP:
        moves = bishop_moves(board, tile);
        break;

    case ROOK:
        moves = rook_moves(board, tile);
        break;

    case QUEEN:
        moves = bishop_moves(board, tile) | rook_moves(board, tile);
        break;

    case KING:
        /* TODO: castling */
        moves = king_moves[tile];
        break;
    }

    /* return the moves, with any moves that take our own tiles removed */
    return moves & ~board->b[colour][OCCUPIED];
}

/* returns 1 if the move is valid and 0 otherwise */
int is_valid_move(Game *game, Move m) {
    Board *board = &(game->board);
    /* Board tmp; */
    uint64_t beginbit, endbit;

    beginbit = 1ull << m.begin;
    endbit = 1ull << m.end;

    /* ensure that the piece belongs to the current player */
    if(!(board->b[game->turn][OCCUPIED] & beginbit))
        return 0;

    /* ensure that the end tile can be moved to from the begin tile */
    if(!(generate_moves(game, m.begin) & endbit))
        return 0;

    /* TODO: ensure that the king is not left in check */
    /* tmp = *board;
    apply_move(game, m);
    if(king_in_check(Game *game, game->turn))
        return 0;
    game->board = tmp; */

    /* ensure that pawns reaching the eighth rank promote */
    if(!m.promote && ((m.end / 8) == 0 || (m.end / 8) == 7)
            && board->mailbox[m.begin] == PAWN)
        return 0;

    /* ensure that no other pieces promote */
    if(m.promote && (board->mailbox[m.begin] != PAWN
                || ((m.end / 8) != 0 && (m.end / 8) != 7)))
        return 0;

    return 1;
}
