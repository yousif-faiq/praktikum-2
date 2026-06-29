
#include <stdio.h>
#include <stdint.h>

static uint32_t next_s1(uint32_t s1, uint32_t s2) {
    return ((s1 << 3) | (s1 >> 29)) ^ (s2 * 0x1D);
}

static uint32_t next_s2(uint32_t s1, uint32_t s2) {
    return ((s2 >> 5) | (s2 << 27)) + (s1 ^ 0xA3B2);
}

static uint32_t lockstep(uint32_t seed, int steps) {
    uint32_t s1 = seed ^ 0xCAFE;
    uint32_t s2 = seed * 0x1337;
    for (int i = 0; i < steps; i++) {
        uint32_t n1 = next_s1(s1, s2);
        uint32_t n2 = next_s2(s1, s2);
        s1 = n1;
        s2 = n2;
    }
    return (s1 ^ s2) & 0xFF;
}

int main(void) {
    uint32_t seeds[] = { 0x01, 0x80, 0x1234, 0xFFFF };
    for (int i = 0; i < 4; i++)
        printf("lockstep(%04X) = 0x%02X\n", seeds[i], lockstep(seeds[i], 10));
    return 0;
}
