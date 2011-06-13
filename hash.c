/* transposition hash table for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

uint64_t zobrist[8][64];

/* initialise the table of zobrist numbers */
void init_zobrist(void) {
    int piece, square;

    /* generate the zobrist number for each piece on each square */
    for(piece = 0; piece < 8; piece++) {
        for(square = 0; square < 64; square++) {
            /* fill in each byte individually; this can't fill in each short
             * individually because RAND_MAX is only guaranteed to be at least
             * 32767, meaning we're only guaranteed bits.
             */
            for(i = 0; i < 8; i++)
                zobrist[piece][square] =
                    (zobrist[piece][square] << 8) | (random() & 0xff);
        }
    }
}
