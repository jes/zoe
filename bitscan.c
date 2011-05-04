/* bitscan code for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* return the index of the LS1B of n; undefined if n == 0 */
int bsf(uint64_t n) {
    uint64_t bit = 1;
    int idx = 0;

    if(n == 0)
        return 64;

    while(!(n & bit)) {
        bit <<= 1;
        idx++;
    }

    return idx;
}

/* return the index of the MS1B of n; undefined if n == 0 */
int bsr(uint64_t n) {
    uint64_t bit = 1ull << 63;
    int idx = 63;

    if(n == 0)
        return 64;

    while(!(n & bit)) {
        bit >>= 1;
        idx--;
    }

    return idx;
}
