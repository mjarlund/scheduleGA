/* Measurement harness only: interpose time() so that srand(time(NULL)) is
 * reproducible.  The GA source is NOT modified.  Build:
 *   cc -shared -fPIC -O2 -o tools/fixed_seed.so tools/fixed_seed.c
 * Use:
 *   SGA_SEED=1 LD_PRELOAD=./tools/fixed_seed.so ./sga
 */
#include <time.h>
#include <stdlib.h>

time_t time(time_t *t) {
    const char *s = getenv("SGA_SEED");
    time_t v = s ? (time_t) atol(s) : 0;
    if (t) *t = v;
    return v;
}
