
#include <stdio.h>
#include <stdint.h>

#define WORDS 4

static void add_multiword(uint32_t *r, const uint32_t *a, const uint32_t *b) {
    uint64_t carry = 0;
    for (int i = 0; i < WORDS; i++) {
        uint64_t sum = (uint64_t)a[i] + b[i] + carry;
        r[i]  = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
    }
}

static void xor_rotate(uint32_t *w) {
    for (int i = 0; i < WORDS; i++) {
        uint32_t v = w[i] ^ w[(i + 1) % WORDS];
        w[i] = (v << 7) | (v >> 25);
    }
}

static uint32_t crunch(uint32_t seed) {
    uint32_t a[WORDS] = { seed, seed ^ 0xDEAD, seed + 0x1337, ~seed };
    uint32_t b[WORDS] = { 0xCAFE, seed * 3, seed ^ 0xBEEF, seed >> 1 };
    uint32_t r[WORDS] = { 0 };

    add_multiword(r, a, b);
    xor_rotate(r);
    add_multiword(r, r, a);
    xor_rotate(r);

    return (r[0] ^ r[1] ^ r[2] ^ r[3]) & 0xFF;
}

int main(void) {
    uint32_t seeds[] = { 0x42, 0x1337, 0xABCD, 0xFF00 };
    for (int i = 0; i < 4; i++)
        printf("crunch(%04X) = 0x%02X\n", seeds[i], crunch(seeds[i]));
    return 0;
}
