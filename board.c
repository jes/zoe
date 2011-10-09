/* board handling for zoe
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
uint64_t king_moves[64];
uint64_t knight_moves[64];

/* reset the given board to the initial state */
void reset_board(Board *board) {
    int i, j;

    /* fill in the empty squares of the mailbox board */
    for(i = 16; i < 56; i++)
        board->mailbox[i] = EMPTY;

    /* fill in the pieces of the mailbox board */
    for(i = 0; i < 2; i++) {
        uint8_t *rank = board->mailbox + i * 56;
        rank[0] = ROOK;
        rank[1] = KNIGHT;
        rank[2] = BISHOP;
        rank[3] = QUEEN;
        rank[4] = KING;
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
    board->b[WHITE][BISHOP]   = 0x0000000000000024ull;
    board->b[WHITE][ROOK]     = 0x0000000000000081ull;
    board->b[WHITE][QUEEN]    = 0x0000000000000008ull;
    board->b[WHITE][KING]     = 0x0000000000000010ull;
    board->b[WHITE][OCCUPIED] = 0x000000000000ffffull;

    board->b[BLACK][PAWN]     = 0x00ff000000000000ull;
    board->b[BLACK][KNIGHT]   = 0x4200000000000000ull;
    board->b[BLACK][BISHOP]   = 0x2400000000000000ull;
    board->b[BLACK][ROOK]     = 0x8100000000000000ull;
    board->b[BLACK][QUEEN]    = 0x0800000000000000ull;
    board->b[BLACK][KING]     = 0x1000000000000000ull;
    board->b[BLACK][OCCUPIED] = 0xffff000000000000ull;

    board->occupied = 0xffff00000000ffffull;

    /* this can be any value as long as it is consistent */
    board->zobrist = 0;
}

/* remove all pieces from the given board */
void clear_board(Board *board) {
    int i, j;

    /* fill in empty for entire mailbox */
    for(i = 0; i < 64; i++)
        board->mailbox[i] = EMPTY;

    /* set all bitboards to empty */
    for(i = 0; i < 2; i++)
        for(j = 0; j < 7; j++)
            board->b[i][j] = 0;

    board->occupied = 0;

    /* TODO: Set zobrist properly (change reset_board to place pieces rather
     * than just filling them in so that zobrist 0 = empty board.
     */
}

/* return 1 if the given board is internally consistent and 0 otherwise */
int consistent_board(Board *board) {
    int i, c, p;
    int piece;
    uint64_t piecebit;
    int colour;
    static char *colourname[2] = { "black", "white" };
    static char *piecename[7] = { "pawn", "knight", "bishop", "rook",
        "queen", "occupied" };

    /* the colour occupied sets do not match the overall occupied set */
    if(board->occupied !=
            (board->b[WHITE][OCCUPIED] | board->b[BLACK][OCCUPIED])) {
        fprintf(stderr, "occupied != white | black\n");
        return 0;
    }

    /* for each square on the board */
    for(i = 0; i < 64; i++) {
        piece = board->mailbox[i];
        piecebit = 1ull << i;

        /* check that the piece is real */
        if(piece > 5 && piece != EMPTY) {
            fprintf(stderr, "invalid piece in mailbox\n");
            return 0;
        }

        colour = -1;

        if(piece == EMPTY) {
            /* if an empty square is considered occupied, fail */
            if(board->occupied & piecebit) {
                fprintf(stderr, "empty square in occupied\n");
                return 0;
            }

            if(board->b[WHITE][OCCUPIED] & piecebit) {
                fprintf(stderr, "empty square in white occupied\n");
                return 0;
            }

            if(board->b[BLACK][OCCUPIED] & piecebit) {
                fprintf(stderr, "empty square in black occupied\n");
                return 0;
            }
        }
        else {
            /* if white is occupied, it's white's piece */
            if(board->b[WHITE][OCCUPIED] & piecebit)
                colour = WHITE;

            /* if black is occupied, it's black's... */
            if(board->b[BLACK][OCCUPIED] & piecebit) {
                /* ...unless white also has it */
                if(colour == WHITE) {
                    fprintf(stderr, "both teams own the piece\n");
                    return 0;
                }

                colour = BLACK;
            }

            /* if no player has the piece, fail */
            if(colour == -1) {
                fprintf(stderr, "neither team owns the piece\n");
                return 0;
            }

            /* if this colour doesn't have the bit in it's piece board, fail */
            if(!(board->b[colour][piece] & piecebit)) {
                fprintf(stderr, "piece occupied but not in piece board\n");
                return 0;
            }
        }

        /* for each colour */
        for(c = 0; c < 2; c++) {
            /* for each piece */
            for(p = 0; p < 6; p++) {
                /* if this is not the desired piece and colour */
                if(p == piece && c == colour)
                    continue;

                /* check that the player doesn't have the bit set in an
                 * inappropriate place.
                 */
                if(board->b[c][p] & piecebit) {
                    fprintf(stderr, "inappropriate bit set (%s %s, square "
                            "%c%c)\n", colourname[c], piecename[p],
                            'a' + i % 8, '1' + i / 8);
                    return 0;
                }
            }
        }
    }

    return 1;
}

/* draw the board to stdout */
void draw_board(Board *board) {
    int x, y;
    uint8_t piece;

    printf("#\n# 8 ");

    for(y = 7; y >= 0; y--) {
        for(x = 0; x < 8; x++) {
            piece = board->mailbox[y * 8 + x];
            if(piece < 8)
                putchar("PNBRQK?."[piece]);
            else
                putchar('?');

            if(board->occupied & (1ull << (y * 8 + x)))
                putchar((board->b[WHITE][OCCUPIED] & (1ull << (y*8 + x)))
                        ? 'w' : 'b');
            else
                putchar('.');
        }
        if(y == 0)
            printf("\n# ");
        else
            printf("\n# %d ", y);
    }
    printf("  a b c d e f g h\n#\n");
}

/* draw the bitboard to stdout */
void draw_bitboard(uint64_t board) {
    int x, y;

    printf("#\n# ");

    for(y = 7; y >= 0; y--) {
        for(x = 0; x < 8; x++) {
            if(board & (1ull << (y * 8 + x)))
                printf("1 ");
            else
                printf("0 ");
        }
        printf("\n# ");
    }

    printf("\n");
}

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
    int xoffset[8] = { 1, -1, 1, -1, 0, 1, 0, -1 };
    int dir, tile;
    int idx;
    int x;

    for(dir = 0; dir < 8; dir++) {
        ray[dir][64] = 0;

        for(tile = 0; tile < 64; tile++) {
            ray[dir][tile] = 0;
            idx = tile;
            x = tile % 8;

            while(1) {
                /* step along the ray */
                idx += offset[dir];
                x += xoffset[dir];

                /* stop if we wrap around */
                if(x < 0 || x > 7)
                    break;
                else if(idx < 0 || idx > 63)
                    break;

                /* add this tile to the ray */
                ray[dir][tile] |= 1ull << idx;
            }
        }
    }
}

/* generate all movement tables */
void generate_movetables(void) {
    generate_rays();
    generate_king_moves();
    generate_knight_moves();
}

/* return the set of tiles that can be reached by a positive ray in the given
 * direction from the given tile.
 */
uint64_t negative_ray(Board *board, int tile, int dir) {
    uint64_t tiles = ray[dir][tile];
    uint64_t blockers = tiles & board->occupied;

    /* remove all tiles beyond the first blocking tile */
    tiles &= ~ray[dir][bsr(blockers)];

    return tiles;
}

/* return the set of tiles that can be reached by a positive ray in the given
 * direction from the given tile.
 */
uint64_t positive_ray(Board *board, int tile, int dir) {
    uint64_t tiles = ray[dir][tile];
    uint64_t blockers = tiles & board->occupied;

    /* remove all tiles beyond the first blocking tile */
    tiles &= ~ray[dir][bsf(blockers)];

    return tiles;
}

/* return the set of tiles a rook can move to from the given tile */
uint64_t rook_moves(Board *board, int tile) {
    return positive_ray(board, tile, NORTH) | positive_ray(board, tile, EAST) |
        negative_ray(board, tile, SOUTH) | negative_ray(board, tile, WEST);
}

/* return the set of tiles a bishop can move to from the given tile */
uint64_t bishop_moves(Board *board, int tile) {
    return positive_ray(board, tile, NW) | positive_ray(board, tile, NE) |
        negative_ray(board, tile, SW) | negative_ray(board, tile, SE);
}

/* return the set of tiles the pawn can move to from the given tile */
uint64_t pawn_moves(Board *board, int tile) {
    int colour = !(board->b[WHITE][OCCUPIED] & (1ull << tile));
    uint64_t move, moves = 0;
    int x = tile % 8, y = tile / 8;
    int target;

    /* TODO: bitboards */

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

    /* try to move forward two squares if we could move one */
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

    return moves;
}

/* return 1 if the given tile is threatened by an enemy and 0 otherwise */
int is_threatened(Board *board, int tile) {
    int colour = !(board->b[WHITE][OCCUPIED] & (1ull << tile));
    int x = tile % 8;
    int pawn_tile;

    /* if the tile is not occupied, it is not threatened */
    if(!(board->occupied & (1ull << tile)))
        return 0;

    /* pretend the piece is a rook, bishop, knight and king. if it can then
     * take an enemy rook, bishop, knight or king respectively then it is
     * threatened.
     */
    if((rook_moves(board, tile) & (board->b[!colour][ROOK]
                    | board->b[!colour][QUEEN]))
            || (bishop_moves(board, tile) & (board->b[!colour][BISHOP]
                    | board->b[!colour][QUEEN]))
            || (knight_moves[tile] & board->b[!colour][KNIGHT])
            || (king_moves[tile] & board->b[!colour][KING]))
        return 1;

    /* check the left square from which pawns may attack */
    if(x > 0) {
        if(colour == WHITE)
            pawn_tile = tile + 7;
        else
            pawn_tile = tile - 9;

        if(board->b[!colour][PAWN] & (1ull << pawn_tile))
            return 1;
    }

    /* check the right square from which pawns may attack */
    if(x < 7) {
        if(colour == WHITE)
            pawn_tile = tile + 9;
        else
            pawn_tile = tile - 7;

        if(board->b[!colour][PAWN] & (1ull << pawn_tile))
            return 1;
    }

    return 0;
}

/* return 1 if the given colour's king is in check and 0 otherwise */
int king_in_check(Board *board, int colour) {
    return is_threatened(board, bsf(board->b[colour][KING]));
}
