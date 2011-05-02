/* zoe - the Zoe Opponent Engine
 *
 * An xboard protocol chess engine.
 *
 * James Stanley 2011
 */

#include "zoe.h"

static int protover = 1;

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
        else if(is_xboard_move(line)) {
            printf("# received move %s\n", line);

            /* apply this move to our board */
            Move m = get_xboard_move(line);
            apply_move(&game, m);

            /* toggle player */
            game.turn = !game.turn;
        }

        /* play a move if it is now our turn */
        if(game.turn == game.engine) {
            /* find the best move */
            Move m = best_move(&game);
            apply_move(&game, m);

            /* tell xboard about our move */
            printf("move %s\n", xboard_move(m));
            printf("# ! move %s\n", xboard_move(m));

            /* toggle player */
            game.turn = !game.turn;
        }
    }
}
