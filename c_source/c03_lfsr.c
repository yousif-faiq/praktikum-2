
#include <stdio.h>
#include <stdint.h>

static uint32_t lfsr_step(uint32_t state) {
    uint32_t bit = ((state >> 31) ^ (state >> 21) ^ (state >> 1) ^ state) & 1;
    return (state >> 1) | (bit << 31);
}

static uint32_t lfsr_run(uint32_t seed, int steps) {
    uint32_t s = seed;
    for (int i = 0; i < steps; i++)
        s = lfsr_step(s);
    return s & 0xFF;
}

static uint32_t chain(uint32_t seed) {
    uint32_t a = lfsr_run(seed,        17);
    uint32_t b = lfsr_run(seed ^ a,    31);
    uint32_t c = lfsr_run(a ^ b,       13);
    return (a ^ b ^ c) & 0xFF;
}

int main(void) {
    uint32_t seeds[] = { 0xACE1, 0x1234, 0xDEAD, 0x0001 };
    for (int i = 0; i < 4; i++)
        printf("chain(%04X) = 0x%02X\n", seeds[i], chain(seeds[i]));
    return 0;
}
