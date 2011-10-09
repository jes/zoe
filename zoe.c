/* zoe - the Zoe Opponent Engine
 *
 * An xboard protocol chess engine.
 *
 * James Stanley 2011
 */

#include "zoe.h"

int post = 0;

/* handle the board edit mode */
void edit_mode(Game *g) {
    static char *piece_letter = "PNBRQK";
    char *line = NULL;
    size_t len = 0;
    int tile;
    uint64_t tilebit;
    int i, j;
    int piece;
    int gameturn = g->turn;

    /* TODO: "[upon leaving edit mode] for purposes of the draw by repetition
     *        rule, no prior positions are deemed to have occurred."
     */
    g->ep = 9;
    g->quiet_moves = 0;

    if(g->turn == BLACK) {
        /* make it white's turn so that the evaluation makes sense */
        g->turn = WHITE;
        g->eval = -g->eval;
    }

    printf("turn is %c\n", "WB"[g->turn]);

    /* remove castling rights for each player for each side */
    for(i = 0; i < 2; i++)
        for(j = 0; j < 2; j++)
            g->can_castle[i][j] = 0;

    while(getline(&line, &len, stdin) != -1) {
        printf("# %s\n", line);

        /* strip the endline character */
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';

        if(strcmp(line, "c") == 0) {
            /* switch colour */
            g->turn = !g->turn;
            g->eval = -g->eval;
        }
        else if(strcmp(line, "#") == 0) {
            /* clear the board */
            g->eval = 0;
            clear_board(&(g->board));
        }
        else if(strcmp(line, ".") == 0) {
            /* leave edit mode */
            free(line);
            break;
        }
        else {
            /* change a tile */
            tile = line[1] - 'a' + ((line[2] - '1') * 8);
            tilebit = 1ull << tile;

            /* remove this piece */
            g->eval -= piece_square_score(g->board.mailbox[tile], tile,
                    !!(g->board.b[WHITE][OCCUPIED] & tilebit));

            g->board.zobrist ^= zobrist[g->board.mailbox[tile]][tile];
            g->board.zobrist ^= zobrist[EMPTY][tile];

            g->board.mailbox[tile] = EMPTY;
            g->board.occupied &= ~tilebit;

            for(i = 0; i < 7; i++) {
                g->board.b[BLACK][i] &= ~tilebit;
                g->board.b[WHITE][i] &= ~tilebit;
            }

            /* add a replacement piece if appropriate */
            if(line[0] != 'x') {
                if(strchr(piece_letter, line[0])) {
                    piece = strchr(piece_letter, line[0]) - piece_letter;

                    g->eval += piece_square_score(piece, tile, g->turn);

                    g->board.zobrist ^= zobrist[EMPTY][tile];
                    g->board.zobrist ^= zobrist[piece][tile];

                    g->board.mailbox[tile] = piece;
                    g->board.occupied |= tilebit;
                    g->board.b[g->turn][piece] |= tilebit;
                    g->board.b[g->turn][OCCUPIED] |= tilebit;
                }
                else {
                    /* do what? */
                }
            }
        }
    }

    if(g->turn != gameturn) {
        /* toggle the turn back if necessary */
        g->turn = !g->turn;
        g->eval = -g->eval;
    }

    /* allow castling where appropriate */
    if(g->board.b[BLACK][KING] & (1ull << 4)) {
        if(g->board.b[BLACK][ROOK] & (1ull << 7))
            g->can_castle[BLACK][KINGSIDE] = 1;
        if(g->board.b[BLACK][ROOK] & (1ull))
            g->can_castle[BLACK][QUEENSIDE] = 1;
    }
    if(g->board.b[WHITE][KING] & (1ull << 60)) {
        if(g->board.b[WHITE][ROOK] & (1ull << 63))
            g->can_castle[WHITE][KINGSIDE] = 1;
        if(g->board.b[WHITE][ROOK] & (1ull << 56))
            g->can_castle[WHITE][QUEENSIDE] = 1;
    }
}

int main(int argc, char **argv) {
    Game game;
    char *line = NULL;
    size_t len = 0;

    /* don't quit when xboard sends SIGINT */
    if(!isatty(STDIN_FILENO))
        signal(SIGINT, SIG_IGN);

    /* force line buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* build zobrist tables */
    init_zobrist();

    /* build tables for move generation */
    generate_movetables();

    /* setup the initial game state */
    reset_game(&game);

    /* let xboard know that we are done initialising */
    puts("feature done=1");

    /* repeatedly handle commands from xboard */
    while(getline(&line, &len, stdin) != -1) {
        printf("# %s", line);

        /* strip the endline character */
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';

        if(strcmp(line, "new") == 0) {
            /* start a new game */
            reset_game(&game);
        }
        else if(strcmp(line, "force") == 0) {
            /* enter force mode where we just ensure that moves are valid */
            game.engine = FORCE;
        }
        else if(strcmp(line, "go") == 0) {
            /* the engine becomes the player currently on move */
            game.engine = game.turn;
        }
        else if(strcmp(line, "post") == 0) {
            /* turn on thinking output */
            post = 1;
        }
        else if(strcmp(line, "nopost") == 0) {
            /* turn off thinking output */
            post = 0;
        }
        else if(strcmp(line, "edit") == 0) {
            /* enter edit mode */
            edit_mode(&game);
        }
        else if(strcmp(line, "quit") == 0) {
            printf("# Be seeing you...\n");
            exit(0);
        }
        else if(is_xboard_move(line)) {
            Move m = get_xboard_move(line);

            /* validate and apply the move */
            if(is_valid_move(game, m, 1)) {
                apply_move(&game, m);

                /* give game information */
                draw_board(&(game.board));
                printf("# current eval = %d\n", game.eval);
            }
        }

        /* play a move if it is now our turn */
        if(game.turn == game.engine) {
            /* find the best move */
            Move m = best_move(game);
            /* only do anything if we have a legal move */
            if(m.begin != 64) {
                apply_move(&game, m);

                /* give game information */
                draw_board(&(game.board));
                printf("# current eval = %d\n", -game.eval);

                /* tell xboard about our move */
                printf("move %s\n", xboard_move(m));
                printf("# ! move %s\n", xboard_move(m));

                /* claim victory or draw if our opponent has no response */
                MoveScore response = alphabeta(game, -INFINITY, INFINITY, 1);
                printf("best response score = %d (move = %s)\n", response.score, xboard_move(response.move));
                if(response.move.begin == 64) {
                    if(response.score == 0)
                        printf("1/2-1/2 {Stalemate}\n");
                    else /* response.score == -INFINITY */ {
                        if(game.turn == WHITE)
                            printf("0-1 {Black mates}\n");
                        else
                            printf("1-0 {White mates}\n");
                    }
                }
            }
        }
    }

    return 0;
}
