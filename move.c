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
    switch(move[4]) {
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
    int eptile;
    uint64_t epbit;
    Move m2;

    /* check board consistency */
    /*if(!consistent_board(&(game->board))) {
        printf("!!! Inconsistent board at start of apply_move!\n");
        draw_board(&(game->board));
        printf("occupied:\n");
        draw_bitboard(game->board.occupied);
        printf("black occupied:\n");
        draw_bitboard(game->board.b[BLACK][OCCUPIED]);
        printf("white occupied:\n");
        draw_bitboard(game->board.b[WHITE][OCCUPIED]);
        exit(1);
    }*/

    /* find the piece from the mailbox */
    beginpiece = board->mailbox[m.begin];
    endpiece = board->mailbox[m.end];

    /* find the bits to use */
    beginbit = 1ull << m.begin;
    endbit = 1ull << m.end;

    /* find out if this move is quiet */
    if(board->occupied & endbit)
        game->quiet_moves = 0;
    else
        game->quiet_moves++;

    /* find the colour from white's occupied bitboard */
    begincolour = !(board->b[WHITE][OCCUPIED] & beginbit);
    endcolour = !(board->b[WHITE][OCCUPIED] & endbit);

    /* delete a pawn if taken en passant */
    if(beginpiece == PAWN && (m.begin == ((5 - begincolour * 3) * 8 + game->ep))
                && ((m.end % 8) == game->ep)) {
        eptile = (4 - begincolour) * 8 + game->ep;
        epbit = 1ull << eptile;
        board->mailbox[eptile] = EMPTY;
        board->occupied ^= epbit;
        board->b[!begincolour][OCCUPIED] ^= epbit;
        board->b[!begincolour][PAWN] ^= epbit;
    }

    /* update en passant availability */
    if(beginpiece == PAWN && abs(m.begin - m.end) == 16)
        game->ep = m.begin % 8;
    else
        game->ep = 9;

    /* remove the piece from the begin square */
    board->mailbox[m.begin] = EMPTY;
    board->occupied ^= beginbit;
    board->b[begincolour][beginpiece] ^= beginbit;
    board->b[begincolour][OCCUPIED] ^= beginbit;
    board->zobrist ^= zobrist[beginpiece][m.begin];
    board->zobrist ^= zobrist[EMPTY][m.begin];

    /* remove the piece from the end square if necessary */
    if(endpiece != EMPTY) {
        board->b[endcolour][endpiece] ^= endbit;
        board->b[endcolour][OCCUPIED] ^= endbit;
    }
    board->zobrist ^= zobrist[endpiece][m.end];

    /* change the piece to it's promotion if appropriate */
    if(m.promote)
        beginpiece = m.promote;

    /* insert the piece at the end square */
    board->mailbox[m.end] = beginpiece;
    board->occupied |= endbit;
    board->b[begincolour][beginpiece] |= endbit;
    board->b[begincolour][OCCUPIED] |= endbit;
    board->zobrist ^= zobrist[beginpiece][m.end];

    /* can't castle on one side if a rook was moved from it's original place */
    if(beginpiece == ROOK) {
        /* queenside */
        if((m.begin == 0 && begincolour == WHITE)
                || (m.begin == 56 && begincolour == BLACK))
            game->can_castle[begincolour][QUEENSIDE] = 0;

        /* kingisde */
        if((m.begin == 7 && begincolour == WHITE)
                || (m.begin == 63 && begincolour == BLACK))
            game->can_castle[begincolour][KINGSIDE] = 0;
    }

    /* can't castle on one side if that rook is taken */
    if(endpiece == ROOK) {
        /* queenside */
        if((m.end == 0 && endcolour == WHITE)
                || (m.end == 56 && endcolour == BLACK))
            game->can_castle[endcolour][QUEENSIDE] = 0;

        /* kingisde */
        if((m.end == 7 && endcolour == WHITE)
                || (m.end == 63 && endcolour == BLACK))
            game->can_castle[endcolour][KINGSIDE] = 0;
    }

    if(beginpiece == KING) {
        /* can no longer castle on either side if the king is moved */
        game->can_castle[begincolour][QUEENSIDE] = 0;
        game->can_castle[begincolour][KINGSIDE] = 0;

        /* move the rook for castling */
        if(abs(m.begin - m.end) == 2) {
            if(m.begin > m.end) {/* queenside */
                m2.begin = m.begin - 4;
                m2.end = m.end + 1;
            }
            else {/* kingside */
                m2.begin = m.begin + 3;
                m2.end = m.end - 1;
            }

            /* skip the opposing player's turn because we need to move another
             * piece
             */
            game->turn = !game->turn;

            /* make sure we don't try to promote */
            m2.promote = 0;

            /* apply the rook move */
            apply_move(game, m2);
        }
    }

    /* toggle current player */
    game->turn = !game->turn;

    /* check board consistency */
    /*if(!consistent_board(&(game->board))) {
        printf("!!! Inconsistent board at end of apply_move!\n");
        draw_board(&(game->board));
        printf("occupied:\n");
        draw_bitboard(game->board.occupied);
        printf("black occupied:\n");
        draw_bitboard(game->board.b[BLACK][OCCUPIED]);
        printf("white occupied:\n");
        draw_bitboard(game->board.b[WHITE][OCCUPIED]);
        exit(1);
    }*/
}

/* return the set of all squares the given piece is able to move to, without
 * considering a king left in check */
uint64_t generate_moves(Game *game, int tile) {
    Board *board = &(game->board);
    int colour = !(board->b[WHITE][OCCUPIED] & (1ull << tile));
    int type = board->mailbox[tile];
    uint64_t moves = 0;
    uint64_t blockers;

    switch(type) {
    case PAWN:
        moves = pawn_moves(board, tile);

        int x = tile % 8, y = tile / 8;

        if((game->ep == x + 1 || game->ep == x - 1) && (y == 4 - colour))
            moves |= 1ull << ((5 - colour * 3) * 8 + game->ep);
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
        moves = king_moves[tile];

        /* queenside castling */
        if(game->can_castle[colour][QUEENSIDE]) {
            blockers = ((1ull << (tile - 1)) | (1ull << (tile - 2))
                | (1ull << (tile - 3))) & board->occupied;

            /* if there are no pieces in the way and no intermediate tiles are
             * threatened, add the move
             */
            if(!blockers && !is_threatened(board, tile)
                    && !is_threatened(board, tile - 1)
                    && !is_threatened(board, tile - 2))
                moves |= 1ull << (tile - 2);
        }

        /* kingisde castling */
        if(game->can_castle[colour][KINGSIDE]) {
            blockers = ((1ull << (tile + 1)) | (1ull << (tile + 2)))
                & board->occupied;

            /* if there are no pieces in the way and no intermediate tiles are
             * threatened, add the move
             */
            if(!blockers && !is_threatened(board, tile)
                    && !is_threatened(board, tile + 1)
                    && !is_threatened(board, tile + 2))
                moves |= 1ull << (tile + 2);
        }
        break;
    }

    /* return the moves, removing the final tile if it ended on our own
     * colour.
     */
    return moves & ~board->b[colour][OCCUPIED];
}

/* return 1 if the move is valid and 0 otherwise, printing an appropriate
 * message if print is non-zero.
 */
int is_valid_move(Game game, Move m, int print) {
    Board *board = &(game.board);
    Game game2 = game;
    uint64_t beginbit, endbit;
    char *strmove = xboard_move(m);

    beginbit = 1ull << m.begin;
    endbit = 1ull << m.end;

    /* ensure that the piece belongs to the current player */
    if(!(board->b[game.turn][OCCUPIED] & beginbit)) {
        if(print)
            printf("Illegal move (%s): that is not your piece.\n", strmove);
        return 0;
    }

    /* ensure that the end tile can be moved to from the begin tile */
    if(!(generate_moves(&game, m.begin) & endbit)) {
        if(print)
            printf("Illegal move (%s): that piece can't move like that.\n",
                    strmove);
        return 0;
    }

    /* ensure that pawns reaching the eighth rank promote */
    if(!m.promote && ((m.end / 8) == 0 || (m.end / 8) == 7)
            && board->mailbox[m.begin] == PAWN) {
        if(print)
            printf("Illegal move (%s): pawns reaching the eighth rank must "
                    "promote.\n", strmove);
        return 0;
    }

    /* ensure that no other pieces promote */
    if(m.promote && (board->mailbox[m.begin] != PAWN
                || ((m.end / 8) != 0 && (m.end / 8) != 7))) {
        if(print)
            printf("Illegal move (%s): that piece may not promote at this "
                    "time.\n", strmove);
        return 0;
    }

    /* ensure that the king is not left in check
     * NOTE: we apply_move() here, so the state of the game is changed; this
     * check must be done last */
    apply_move(&game2, m);
    if(king_in_check(&(game2.board), game.turn)) {
        if(print)
            printf("Illegal move (%s): king is left in check.\n", strmove);
        return 0;
    }

    /* hasn't failed any validation, must be a valid move */
    return 1;
}
