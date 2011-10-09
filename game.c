/* game state handling for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* reset the given game to the initial state */
void reset_game(Game *game) {
    int i, j;

    reset_board(&(game->board));

    /* set castling rights for each player for each side */
    for(i = 0; i < 2; i++)
        for(j = 0; j < 2; j++)
            game->can_castle[i][j] = 1;

    game->quiet_moves = 0;
    game->turn = WHITE;
    game->engine = BLACK;
    game->ep = 9;
    game->eval = 0;
}
