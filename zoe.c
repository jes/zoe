/* zoe - the Zoe Opponent Engine
 *
 * An xboard protocol chess engine.
 *
 * James Stanley 2011
 */

#include "zoe.h"

int main(int argc, char **argv) {
    Game game;
    char *line = NULL;
    size_t len = 0;

    /* force line buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* build tables for move generation */
    generate_tables();

    /* setup the initial game state */
    reset_game(&game);

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
        else if(strcmp(line, "quit") == 0) {
            printf("# Be seeing you...\n");
            exit(0);
        }
        else if(is_xboard_move(line)) {
            Move m = get_xboard_move(line);

            /* validate and apply the move */
            if(is_valid_move(&game, m))
                apply_move(&game, m);
            else {
                printf("Illegal move: %s\n", line);
                printf("# ! Illegal move: %s\n", line);
            }
        }

        /* play a move if it is now our turn */
        if(game.turn == game.engine) {
            /* show the current board evaluation */
            printf("# current eval = %d\n", evaluate(&game));

            /* find the best move */
            Move m = best_move(&game);

            /* only do anything if we have a legal move */
            if(m.begin != 64) {
                apply_move(&game, m);

                /* tell xboard about our move */
                printf("move %s\n", xboard_move(m));
                printf("# ! move %s\n", xboard_move(m));
            }
        }

        printf("# engine = %s\n", game.engine ? "black" : "white");
        printf("# turn = %s\n", game.turn ? "black" : "white");

        draw_board(&(game.board));
    }

    return 0;
}
