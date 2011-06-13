/* zoe - the Zoe Opponent Engine
 *
 * An xboard protocol chess engine.
 *
 * James Stanley 2011
 */

#include "zoe.h"

int post = 0;

int main(int argc, char **argv) {
    Game game;
    char *line = NULL;
    size_t len = 0;

    /* don't quit when xboard sends SIGINT */
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
    while(1) {
        /* attempt to read a line from stdin */
        if(getline(&line, &len, stdin) == -1)
            break;

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
                printf("# current eval = %d\n", evaluate(&game));
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
                printf("# current eval = %d\n", -evaluate(&game));

                /* tell xboard about our move */
                printf("move %s\n", xboard_move(m));
                printf("# ! move %s\n", xboard_move(m));
            }
        }
    }

    return 0;
}
