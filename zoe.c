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

    /* setup the initial game state */
    reset_game(&game);

    /* repeatedly handle commands from xboard */
    while(1) {
        /* attempt to read a line from stdin */
        if(getline(&line, &len, stdin) == -1)
            break;
    }
}
