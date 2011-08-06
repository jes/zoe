/* game tree searching for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

#define SEARCHDEPTH 7

static int piece_score[6] = { /* pawn */ 100, /* knight */ 300,
    /* bishop */ 325, /* rook */ 500, /* queen */ 900, /* king */ 0 };

Game *movecmp_game;

/* compare two moves for sorting */
static int movecmp(const void *m1, const void *m2) {
    Move *a = (Move *)m1, *b = (Move *)m2;
    Game *g = movecmp_game;
    int n1 = 0, n2 = 0;

    /* if either move ends in an occupied square, that move is a capture and
     * should come sooner in the sort order.
     */
    if(g->board.occupied && (1ull << a->end))
        n1 = piece_score[g->board.mailbox[a->end]];
    if(g->board.occupied && (1ull << b->end))
        n2 = piece_score[g->board.mailbox[b->end]];

    return n2 - n1;
}

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
    Move moves[121];/* 121 moves is enough for anybody */
    int nmoves;
    int move;
    Move m;
    MoveScore best, new;
    Game orig_game;
    int legal_move = 0;
    int hashtype = ATMOST;
    int i;

    /* store a copy of the game */
    orig_game = game;

    /* try to retrieve the score from the transposition table */
    new = hash_retrieve(orig_game.board.zobrist, depth, alpha, beta,
            orig_game.turn);
    if(new.move.begin != 64) {
        /* TODO: ensure that the move is valid (i.e. that this zobrist key is
         * not just a coincidence).
         */
        return new;
    }

    /* store lower bound on best score */
    if(depth < SEARCHDEPTH - 1)
        best.score = alpha;
    else
        best.score = -INFINITY;

    /* if at a leaf node, return position evaluation */
    if(depth == 0) {
        best.move.begin = 64;
        best.score = evaluate(&game);
        best.pv[0].begin = 64;
        hash_store(orig_game.board.zobrist, depth, EXACTLY, best,
                orig_game.turn);
        return best;
    }

    /* get a list of valid moves */
    generate_movelist(&game, moves, &nmoves);

    /* sort the moves */
    movecmp_game = &game;
    qsort(moves, nmoves, sizeof(Move), movecmp);

    /* for each of the moves */
    for(move = 0; move < nmoves; move++) {
        m = moves[move];

        /* reset the game state */
        game = orig_game;

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
            hash_store(orig_game.board.zobrist, depth, ATLEAST, best,
                    orig_game.turn);
            return best;
        }

        /* new best move? */
        if(new.score > best.score) {
            /* best movescore has the move we should play and the score we
             * got from the child alphabeta.
             */
            best.move = m;
            best.score = new.score;

            /* we know the score for this node is exactly best.score */
            hashtype = EXACTLY;

            /* copy the pv from the best move */
            for(i = 0; i < 15; i++) {
                best.pv[i+1] = new.pv[i];
            }
            best.pv[0] = m;
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
    else {
        /* we found a legal move and more searching was done, so we have a
         * lower bound on the score.
         */
        hash_store(orig_game.board.zobrist, depth, hashtype, best,
                orig_game.turn);
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

/* return the best move from the current position along with it's score */
MoveScore iterative_deepening(Game game) {
    int d;
    MoveScore best;

    /* iteratively deepen until the maximum depth is reached */
    for(d = 1; d <= SEARCHDEPTH; d++) {
        best = alphabeta(game, -INFINITY, INFINITY, d);

        /* if we have no legal moves, return now */
        if(best.move.begin == 64)
            return best;

        /* if this is a mate, return now */
        if(best.score == INFINITY) {
            printf("# Mate in %d.\n", d/2);
            return best;
        }

        /* TODO: stop searching if the entire tree is searched */

        /* TODO: stop searching if time limit reached */
    }

    return best;
}

/* return the best move for the current player */
Move best_move(Game game) {
    MoveScore best = iterative_deepening(game);

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
