/* bitscan code for zoe
 *
 * James Stanley 2011
 */

#include "zoe.h"

/* return the index of the LS1B of n; undefined if n == 0 */
int bsf(uint64_t n) {
    if(n == 0)
        return 64;

#ifdef ASM_BITSCAN
    uint64_t r;
    asm volatile(" bsfq %1, %0": "=r"(r): "rm"(n));
    return r;
#else
    uint64_t bit = 1;
    int idx = 0;

    while(!(n & bit)) {
        bit <<= 1;
        idx++;
    }

    return idx;
#endif
}

/* return the index of the MS1B of n; undefined if n == 0 */
int bsr(uint64_t n) {
    if(n == 0)
        return 64;

#ifdef ASM_BITSCAN
    uint64_t r;
    asm volatile(" bsrq %1, %0": "=r"(r): "rm"(n));
    return r;
#else
    uint64_t bit = 1ull << 63;
    int idx = 63;

    while(!(n & bit)) {
        bit >>= 1;
        idx--;
    }

    return idx;
#endif
}

/* return the number of 1 bits in n */
int count_ones(uint64_t n) {
    /*int count = 0;

    while(n) {
        count += n & 1;
        n >>= 1;
    }

    return count;*/
    /* http://graphics.stanford.edu/~seander/bithacks.html */
    n = n - ((n >> 1) & (uint64_t)~(uint64_t)0/3);                           // temp
    n = (n & (uint64_t)~(uint64_t)0/15*3) + ((n >> 2) & (uint64_t)~(uint64_t)0/15*3);      // temp
    n = (n + (n >> 4)) & (uint64_t)~(uint64_t)0/255*15;                      // temp
    return (uint64_t)(n * ((uint64_t)~(uint64_t)0/255)) >> (sizeof(uint64_t) - 1) * 8; // count
}
