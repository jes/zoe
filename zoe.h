/* Header for the zoe chess engine
 *
 * James Stanley 2011
 */

#ifndef ZOE_H_INC
#define ZOE_H_INC

#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define WHITE 0
#define BLACK 1
#define FORCE 2

#define KINGSIDE  0
#define QUEENSIDE 1

#define PAWN     0
#define KNIGHT   1
#define BISHOP   2
#define ROOK     3
#define QUEEN    4
#define KING     5
#define OCCUPIED 6
#define EMPTY    7

#define INFINITY (1 << 30)

typedef struct Board {
    uint8_t mailbox[64];
    uint64_t b[2][7];
    uint64_t occupied;
} Board;

typedef struct Game {
    Board board;
    uint8_t can_castle[2][2];
    uint8_t quiet_moves;
    uint8_t turn;
    uint8_t engine;
    uint8_t ep;
} Game;

typedef struct Move {
    uint8_t begin;
    uint8_t end;
    uint8_t promote;
} Move;

typedef struct MoveScore {
    Move move;
    int score;
    Move pv[16];
} MoveScore;

/* bitscan.c */
int bsf(uint64_t n);
int bsr(uint64_t n);
int count_ones(uint64_t n);

/* board.c */
extern uint64_t king_moves[64];
extern uint64_t knight_moves[64];

void reset_board(Board *board);
int consistent_board(Board *board);
void draw_board(Board *board);
void draw_bitboard(uint64_t board);
void generate_movetables(void);
uint64_t rook_moves(Board *board, int tile);
uint64_t bishop_moves(Board *board, int tile);
uint64_t pawn_moves(Board *board, int tile);
int is_threatened(Board *board, int tile);
int king_in_check(Board *board, int colour);

/* game.c */
void reset_game(Game *game);

/* hash.c */
extern uint64_t zobrist[8][64];

void init_zobrist(void);

/* move.c */
char *xboard_move(Move m);
int is_xboard_move(const char *move);
Move get_xboard_move(const char *move);
void apply_move(Game *game, Move m);
uint64_t generate_moves(Game *game, int tile);
int is_valid_move(Game game, Move m, int print);

/* search.c */
int evaluate(Game *game);
Move best_move(Game game);

/* zoe.c */
extern int post;

#endif
