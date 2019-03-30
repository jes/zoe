/* game tree searching for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"
#include <time.h>

#define SEARCHDEPTH 6

static int nodes;

/* sort the list of moves to put the ones most likely to be good first */
static void sort_moves(Move *moves, int nmoves, Game *game) {
    int firstcap = 0;
    int firstnot = 0;
    Move tmp;

    while(1) {
        /* look for the next non-capture move */
        while((game->board.occupied & (1ull << moves[firstnot].end))
                    && firstnot < nmoves)
                firstnot++;

        if(firstnot >= nmoves)
            break;

        /* look for the next capture move */
        while(!(game->board.occupied & (1ull << moves[firstcap].end))
                && firstcap < nmoves)
            firstcap++;

        if(firstcap >= nmoves)
            break;

        /* swap the capture with the non-capture */
        tmp = moves[firstcap];
        moves[firstcap] = moves[firstnot];
        moves[firstnot] = tmp;

        firstnot++;
    }
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

    nodes++;

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
        best.score = game.eval;
        best.pv[0].begin = 64;
        hash_store(orig_game.board.zobrist, depth, EXACTLY, best,
                orig_game.turn);
        return best;
    }

    /* get a list of valid moves */
    generate_movelist(&game, moves, &nmoves);

    /* sort the moves */
    sort_moves(moves, nmoves, &game);

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
    clock_t start = clock();
    nodes = 0;

    MoveScore best = iterative_deepening(game);

    printf("# %.2f n/s\n", (float)(nodes * CLOCKS_PER_SEC) / (clock() - start));

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
    return best.move;
}
