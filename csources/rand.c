#include "common.h"
#include "config.h"

typedef unsigned long long uint64;

rand_t gen[PARALLELISM];

uint64 rand_next(rand_t *x) {
    uint64 a = *x;
    uint64 b = 4294967296UL - a;
    uint64 c = a * b;
    uint64 d = c << 2;
    *x = d >> 32;
    uint64 z = d >> 62 | d;
    uint64 l = z & 0x00000000ffffffffUL;
    return *x ^ l;
}

double rand_next_double(rand_t *gen, double min, double max) {
    double r = (double)rand_next(gen);
    return r * (max - min) / 0xffffffff + min;
}

void rand_init(rand_t *gen) {
    *gen = RAND_MAX - rand() + 1;
    rand_next(&(*gen));
}

void rand_seed(unsigned int seed){
    srand(seed);

    for(int i=0; i<PARALLELISM; i++){
        rand_init(&gen[i]);
    }
}
