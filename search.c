/* game tree searching for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

#define SEARCHDEPTH 6

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
MoveScore alphabeta(Game game, int alpha, int beta, int depth) {
    Move m;
    MoveScore best, new;
    Game orig_game;
    uint64_t pieces = game.board.b[game.turn][OCCUPIED];
    int legal_move = 0;
    int i;

    /* store a copy of the game */
    orig_game = game;

    /* store lower bound on best score */
    if(depth < SEARCHDEPTH - 1)
        best.score = alpha;
    else
        best.score = -INFINITY;

    /* if at a leaf node, return position evaluation */
    if(depth == 0) {
        best.score = evaluate(&game);
        best.pv[0].begin = 64;
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

        /* reset game state */
        game = orig_game;

        /* generate the moves for this piece */
        uint64_t moves = generate_moves(&game, piece);

        /* for each of this piece's moves */
        while(moves) {
            /* pick the next move */
            int move = bsf(moves);

            /* remove this move from the set */
            moves ^= 1ull << move;

            /* set the end square of this move */
            m.end = move;

            /* reset game state */
            game = orig_game;

            /* TODO: try each possiblity of pawn promotion */
            if(game.board.mailbox[piece] == PAWN
                    && (m.end / 8 == 0 || m.end / 8 == 7))
                m.promote = QUEEN;
            else
                m.promote = 0;

            /* make the move */
            apply_move(&game, m);

            /* don't search this move if the king is left in check */
            if(king_in_check(&(game.board), !game.turn)) {
                if(depth == SEARCHDEPTH)
                    printf("%s leaves the king in check\n", xboard_move(m));

                continue;
            }

            /* if this is the first legal move, store it as the best so that
             * we at least have a move to play, and remember that we have found
             * a legal move.
             */
            if(!legal_move) {
                best.move = m;
                legal_move = 1;
            }

            /* search the next level; we need to do a full search from the top
             * level in order to get the pv for each move.
             */
            if(depth == SEARCHDEPTH)
                new = alphabeta(game, -INFINITY, INFINITY, depth - 1);
            else
                new = alphabeta(game, -beta, -best.score, depth - 1);
            new.score = -new.score;

            /* show the expected line of play from this move at top level */
            if(depth == SEARCHDEPTH) {
                printf("%s: ", xboard_move(m));
                for(i = 0; i < 16 && new.pv[i].begin < 64; i++) {
                    printf("%s ", xboard_move(new.pv[i]));
                }
                printf("%d\n", new.score);
            }

            /* beta cut-off */
            if(new.score >= beta) {
                best.move = m;
                best.score = beta;
                return best;
            }

            /* new best move? */
            if(new.score > best.score) {
                /* best movescore has the move we should play and the score we
                 * got from the child alphabeta.
                 */
                best.move = m;
                best.score = new.score;

                /* copy the pv from the best move */
                for(i = 0; i < 15; i++) {
                    best.pv[i+1] = new.pv[i];
                }
                best.pv[0] = m;
            }
        }
    }

    /* reset to original game */
    game = orig_game;

    /* no legal moves? checkmate or stalemate */
    if(!legal_move) {
        /* adding (depth - SEARCHDEPTH) ensures that we drag out a forced
         * loss for as long as possible, and also that we force a win as
         * quickly as possible.
         */
        if(king_in_check(&(game.board), game.turn))
            best.score = -INFINITY + (depth - SEARCHDEPTH);
        else
            best.score = 0;

        best.move.begin = 64;
    }

    /* TODO: deal with post mode */

    /* show the pv */
    if(depth == SEARCHDEPTH) {
        printf("# pv: ");
        for(i = 0; i < 16 && best.pv[i].begin < 64; i++) {
            printf("%s ", xboard_move(best.pv[i]));
        }
        printf("%d\n", best.score);
    }

    return best;
}

/* return the best move for the current player */
Move best_move(Game game) {
    MoveScore best = alphabeta(game, -INFINITY, INFINITY, SEARCHDEPTH);

    if(best.move.begin == 64) { /* we had no legal moves */
        if(best.score == 0)
            printf("1/2-1/2 {Stalemate}\n");
        else /* best.score == -INFINITY */ {
            if(game.turn == WHITE)
                printf("0-1 {Black mates}\n");
            else
                printf("1-0 {White mates}\n");
        }
    }

    /* TODO: consider our enemy's options and see if we've won, etc. */

    return best.move;
}
