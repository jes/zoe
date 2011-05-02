/* move handling for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

#define NW 0
#define NE 1
#define SW 2
#define SE 3
#define NORTH 4
#define EAST  5
#define SOUTH 6
#define WEST  7

static uint64_t ray[8][65];
static uint64_t king_moves[64];
static uint64_t knight_moves[64];

/* generate king movement table */
void generate_king_moves(void) {
    int tile;
    int x, y;
    uint64_t moves;

    for(tile = 0; tile < 64; tile++) {
        x = tile % 8;
        y = tile / 8;
        moves = 0;

        if(y != 7) /* north */
            moves |= 1ull << (tile + 8);
        if(y != 0) /* south */
            moves |= 1ull << (tile - 8);
        if(x != 7) /* east */
            moves |= 1ull << (tile + 1);
        if(x != 0) /* west */
            moves |= 1ull << (tile - 1);
        if(x != 0 && y != 7) /* north west */
            moves |= 1ull << (tile + 7);
        if(x != 7 && y != 7) /* north east */
            moves |= 1ull << (tile + 9);
        if(x != 0 && y != 0) /* south west */
            moves |= 1ull << (tile - 9);
        if(x != 7 && y != 0) /* south east */
            moves |= 1ull << (tile - 7);

        king_moves[tile] = moves;
    }
}

/* generate knight movement table */
void generate_knight_moves(void) {
    int tile;
    int x, y;
    uint64_t moves;

    for(tile = 0; tile < 64; tile++) {
        x = tile % 8;
        y = tile / 8;
        moves = 0;

        /* north west */
        if(y < 6 && x > 0)
            moves |= 1ull << (tile + 15);
        if(y < 7 && x > 1)
            moves |= 1ull << (tile + 6);
        /* north east */
        if(y < 6 && x < 7)
            moves |= 1ull << (tile + 17);
        if(y < 7 && x < 6)
            moves |= 1ull << (tile + 10);
        /* south west */
        if(y > 1 && x > 0)
            moves |= 1ull << (tile - 17);
        if(y > 0 && x > 1)
            moves |= 1ull << (tile - 10);
        /* south east */
        if(y > 1 && x < 7)
            moves |= 1ull << (tile - 15);
        if(y > 0 && x < 6)
            moves |= 1ull << (tile - 6);

        knight_moves[tile] = moves;
    }
}

/* generate the ray tables */
void generate_rays(void) {
    int offset[8] = { 9, 7, -7, -9, 8, 1, -8, -1 };
    int dir, tile;
    int idx;

    for(dir = 0; dir < 8; dir++) {
        ray[dir][64] = 0;

        for(tile = 0; tile < 64; tile++) {
            ray[dir][tile] = 0;
            idx = tile;

            while(1) {
                /* step along the ray */
                idx += offset[dir];

                /* stop if we wrap around */
                if((offset[dir] < 0) && ((idx%8) - (tile%8) > 0))
                    break;
                else if((offset[dir] > 0) && ((idx%8) - (tile%8) < 0))
                    break;
                else if(idx < 0 || idx > 63)
                    break;

                /* add this tile to the ray */
                ray[dir][tile] |= 1ull < idx;
            }
        }
    }
}

/* generate all movement tables */
void generate_tables(void) {
    generate_rays();
    generate_king_moves();
    generate_knight_moves();
}


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
    m.begin = (move[0] - 'a') + ((move[1] - '1') * 8);
    m.end = (move[2] - 'a') + ((move[3] - '1') * 8);

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

/* return the set of tiles that can be reached by a positive ray in the given
 * direction from the given tile.
 */
uint64_t negative_rays(Game *game, int tile, int dir) {
    uint64_t tiles = ray[dir][tile];
    uint64_t blocker = tiles & game->board.occupied;

    /* remove all tiles beyond the first blocking tile */
    tiles ^= ray[dir][bsr(blocker)];

    return tiles;
}

/* return the set of tiles that can be reached by a positive ray in the given
 * direction from the given tile.
 */
uint64_t positive_rays(Game *game, int tile, int dir) {
    uint64_t tiles = ray[dir][tile];
    uint64_t blocker = tiles & game->board.occupied;

    /* remove all tiles beyond the first blocking tile */
    tiles ^= ray[dir][bsf(blocker)];

    return tiles;
}

/* return the set of tiles a rook can move to from the given tile */
uint64_t rook_moves(Game *game, int tile) {
    return positive_rays(game, tile, NORTH) | positive_rays(game, tile, EAST) |
        negative_rays(game, tile, SOUTH) | negative_rays(game, tile, WEST);
}

/* return the set of tiles a bishop can move to from the given tile */
uint64_t bishop_moves(Game *game, int tile) {
    return positive_rays(game, tile, NW) | positive_rays(game, tile, NE) |
        negative_rays(game, tile, SW) | negative_rays(game, tile, SE);
}

/* return the set of all squares the given piece is able to move to, without
 * considering a king left in check */
uint64_t generate_moves(Game *game, int tile) {
    Board *board = &(game->board);
    int type = board->mailbox[tile];
    int colour = !(board->b[WHITE][OCCUPIED] & tile);
    uint64_t moves = 0;
    int target;

    switch(type) {
    case PAWN:
        /* TODO: en passant, double square first moves */
        if(colour == WHITE) {
            target = tile + 8;
            moves = 1ull << target;
        }
        else {
            target = tile - 8;
            moves = 1ull << target;
        }

        /* remove this move if the target square is occupied */
        if(board->occupied & moves)
            moves = 0;

        /* add on the appropriate attack squares */
        if(target % 8 != 0)
            moves |= 1ull << (target - 1);
        if(target % 8 != 7)
            moves |= 1ull << (target + 1);
        break;

    case KNIGHT:
        moves = knight_moves[tile];
        break;

    case BISHOP:
        moves = bishop_moves(game, tile);
        break;

    case ROOK:
        moves = rook_moves(game, tile);
        break;

    case QUEEN:
        moves = bishop_moves(game, tile) | rook_moves(game, tile);
        break;

    case KING:
        /* TODO: castling */
        moves = king_moves[tile];
        break;
    }

    /* return the moves, with any moves that take our own tiles removed */
    return moves & ~board->b[colour][OCCUPIED];
}
