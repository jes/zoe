/* transposition hash table for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

uint64_t zobrist[8][64];
HashEntry hashtable[HT_SIZE];

/* initialise the table of zobrist numbers */
void init_zobrist(void) {
    int piece, square;
    int i;

    /* generate the zobrist number for each piece on each square */
    for(piece = 0; piece < 8; piece++) {
        for(square = 0; square < 64; square++) {
            /* fill in each byte individually; this can't fill in each short
             * individually because RAND_MAX is only guaranteed to be at least
             * 32767, meaning we're only guaranteed 15 bits.
             */
            for(i = 0; i < 8; i++)
                zobrist[piece][square] =
                    (zobrist[piece][square] << 8) | (random() & 0xff);
        }
    }

    /* since all of the fields in hashtable[] are initialised to 0, the first
     * entry will appear to be the real entry for any positions with a key of 0
     * (most notably, the initial board configuration), so we change the key
     * of the first entry such that it doesn't match.
     */
    hashtable[0].key = 1;
}

/* store the given information in the transposition table */
void hash_store(uint64_t key, uint8_t depth, uint8_t type, MoveScore move,
        int colour) {
    int index = (key % HT_SIZE);

    /* store scores for white player, by inverting things if we are black */
    if(colour == BLACK) {
        move.score = -move.score;

        if(type == ATLEAST)
            type = ATMOST;
        else if(type == ATMOST)
            type = ATLEAST;
    }

    /* always replace the existing hashtable entry */
    hashtable[index].key = key;
    hashtable[index].depth = depth;
    hashtable[index].type = type;
    hashtable[index].move = move;
    hashtable[index].colour = colour;
}

/* retrieve a MoveScore from the hashtable with the given bounds on score;
 * if not suitable transposition table entry can be found, a move starting at
 * tile 64 is returned.
 */
MoveScore hash_retrieve(uint64_t key, uint8_t depth, int alpha, int beta,
        int colour) {
    int index = (key % HT_SIZE);
    MoveScore fail;
    HashEntry e = hashtable[index];

    /* scores are stored for white player */
    if(colour == BLACK) {
        e.move.score = -e.move.score;

        if(e.type == ATLEAST)
            e.type = ATMOST;
        else if(e.type == ATMOST)
            e.type = ATLEAST;
    }

    /* set the start and tile of the "failure" move to be invalid */
    fail.move.begin = 64;
    fail.move.end = 64;

    /* if the colour is wrong, we don't want the result */
    if(e.colour != colour)
        return fail;

    /* if the key is wrong, the board is wrong */
    if(e.key != key)
        return fail;

    /* if the cached search wasn't deep enough, we don't want it */
    if(e.depth < depth)
        return fail;

    /* if we know the exact score, return it */
    if(e.type == EXACTLY)
        return e.move;

    /* if we have a lower bound that is not lower than beta, return it. */
    if(e.type == ATLEAST && e.move.score >= beta)
        return e.move;

    /* if we have an upper bound that is not higher than alpha, return it */
    if(e.type == ATMOST && e.move.score <= alpha)
        return e.move;

    /* if all else fails, fail */
    return fail;
}
