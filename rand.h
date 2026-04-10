#include <stdint.h>
#include "mouse.h"
#include "timer.h"
#include "memory.h"
static uint32_t rng_state = 123456789;

void srand(uint32_t seed) {
    rng_state ^= seed;
}

uint32_t rand() {
    rng_state ^= mouse.x;
    rng_state ^= mouse.y;
    rng_state ^= ms_since_startup();
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}