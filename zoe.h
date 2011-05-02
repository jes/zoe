/* Header for the zoe chess engine
 *
 * James Stanley 2011
 */

#ifndef ZOE_H_INC
#define ZOE_H_INC

#include <stdint.h>

#define WHITE 0
#define BLACK 1

#define KINGSIDE  0
#define QUEENSIDE 1

#define PAWNS    0
#define KNIGHTS  1
#define BISHOPS  2
#define ROOKS    3
#define QUEENS   4
#define KING     5
#define OCCUPIED 6

typedef struct Board {
    uint64_t b[2][6];
    uint64_t occupied;
} Board;

typedef struct Game {
    Board board;
    int can_castle[2][2];
    int can_en_passant[2][8];
    int turn;
    int quiet_moves;
} Game;

/* game.c */
void reset_game(Game *game);

/* board.c */
void reset_board(Board *board);

#endif
