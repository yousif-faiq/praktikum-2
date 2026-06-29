
#include <stdio.h>
#include <stdint.h>

static uint32_t beta(uint32_t x, int d);

static uint32_t alpha(uint32_t x, int d) {
    if (d <= 0 || (x & 0xF) == 0xA) return x ^ 0x5C;
    uint32_t t = (x * 0x1B) ^ ((x >> 3) + d);
    return beta(t, d - 1);
}

static uint32_t beta(uint32_t x, int d) {
    if (d <= 0 || (x & 0xF) == 0x5) return x ^ 0xC3;
    uint32_t t = ((x << 2) | (x >> 30)) + 0x77 * d;
    return alpha(t ^ (d * 0x13), d - 1);
}

static uint32_t run(uint32_t seed) {
    uint32_t a = alpha(seed, 8);
    uint32_t b = beta(seed ^ a, 8);
    return (a ^ b) & 0xFF;
}

int main(void) {
    uint32_t seeds[] = { 0x33, 0xCC, 0x1FF, 0xBEEF };
    for (int i = 0; i < 4; i++)
        printf("run(%04X) = 0x%02X\n", seeds[i], run(seeds[i]));
    return 0;
}
