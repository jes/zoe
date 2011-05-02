/* game tree searching for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* return the best move for the current player */
Move best_move(Game *game) {
    MoveScore best = alphabeta(game, -INFINITY, INFINITY, 3);
    return best.move;
}

/* return the best move from the current position along with it's score */
static MoveScore alphabeta(Game *game, int alpha, int beta, int depth) {
    uint64_t pieces = game->board.b[game->turn][OCCUPIED];

    /* if at a leaf node, return position evaluation */
    if(depth == 0)
        return evaluate(game);

    /* for each of our pieces... */
    while(pieces) {
        /* pick the next piece */
        int piece = bsf(pieces);

        /* remove this piece from the set */
        pieces ^= 1 << piece;

        /* generate the moves for this piece */
        uint64_t moves = generate_moves(game, piece);

        /* for each of this piece's moves */
        while(moves) {
            /* pick the next move */
            int move = bsf(moves);

            /* remove this move from the set */
            moves ^= 1 << move;

            /* ensure that we are not left in check or taking one of our own
             * pieces.
             */
            if(!quick_valid_move(game, piece, move))
                continue;
        }
    }
}
