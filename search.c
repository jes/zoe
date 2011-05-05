/* game tree searching for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

static int piece_score[6] = { /* pawn */ 100, /* knight */ 300,
    /* bishop */ 325, /* rook */ 500, /* queen */ 900, /* king */ 0 };

/* return the value of the game for the player currently on move */
int evaluate(Game *game) {
    int score = 0;
    int piece;

    for(piece = 0; piece < 5; piece++) {
        score += piece_score[piece]
            * count_ones(game->board.b[game->turn][piece]);
        score -= piece_score[piece]
            * count_ones(game->board.b[!game->turn][piece]);
    }

    return score;
}

/* return the best move from the current position along with it's score */
MoveScore alphabeta(Game *game, int alpha, int beta, int depth) {
    Move m;
    MoveScore best, new;
    Board origboard;
    uint64_t pieces = game->board.b[game->turn][OCCUPIED];

    /* store a copy of the board */
    origboard = game->board;

    /* store lower bound on best score */
    best.score = alpha;

    /* if at a leaf node, return position evaluation */
    if(depth == 0) {
        best.score = evaluate(game);
        return best;
    }

    /* for each of our pieces... */
    while(pieces) {
        /* pick the next piece */
        int piece = bsf(pieces);

        /* remove this piece from the set */
        pieces ^= 1ull << piece;

        /* set the start square of the moves */
        m.begin = piece;

        /* generate the moves for this piece */
        uint64_t moves = generate_moves(game, piece);

        /* for each of this piece's moves */
        while(moves) {
            /* pick the next move */
            int move = bsf(moves);

            /* remove this move from the set */
            moves ^= 1ull << move;

            /* set the end square of this move */
            m.end = move;

            /* TODO: try each possiblity of pawn promotion */
            m.promote = 0;

            /* ensure that we are not left in check or taking one of our own
             * pieces.
             */
            /*if(!quick_valid_move(game, piece, move))
                continue;*/

            /* make the move */
            apply_move(game, m);

            /* continue the search */
            new = alphabeta(game, -beta, -best.score, depth - 1);
            new.score = -new.score;

            /* reset the board */
            game->board = origboard;

            /* toggle the turn back */
            game->turn = !game->turn;

            /* beta cut-off */
            if(new.score >= beta) {
                best.score = beta;
                return best;
            }

            /* new best move? */
            if(new.score > best.score) {
                /* best movescore has the move we just played and the score we
                 * got from the child alphabeta.
                 */
                best.move = m;
                best.score = new.score;
            }
        }
    }

    return best;
}

/* return the best move for the current player */
Move best_move(Game *game) {
    MoveScore best = alphabeta(game, -INFINITY, INFINITY, 6);
    return best.move;
}
