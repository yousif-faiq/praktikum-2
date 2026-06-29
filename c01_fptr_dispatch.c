
#include <stdio.h>
#include <stdint.h>

static uint32_t f0(uint32_t x) { return (x * 0x6B43) ^ 0xA5A5; }
static uint32_t f1(uint32_t x) { return ((x << 3) | (x >> 29)) ^ 0x1234; }
static uint32_t f2(uint32_t x) { return ~x ^ (x >> 7); }
static uint32_t f3(uint32_t x) { return (x + 0xDEAD) * 0x45; }
static uint32_t f4(uint32_t x) { return ((x ^ 0xBEEF) << 1) | (x >> 31); }
static uint32_t f5(uint32_t x) { return (x * x) ^ (x >> 4); }
static uint32_t f6(uint32_t x) { return ~(x + 0xFF) ^ 0x5A5A; }
static uint32_t f7(uint32_t x) { return ((x >> 2) ^ (x << 6)) & 0xFFFF; }

typedef uint32_t (*fn_t)(uint32_t);
static fn_t table[8] = { f0, f1, f2, f3, f4, f5, f6, f7 };

static uint32_t dispatch(uint32_t state, int steps) {
    for (int i = 0; i < steps; i++) {
        int idx = (state ^ (state >> 4)) & 0x7;
        state = table[idx](state);
    }
    return state & 0xFF;
}

int main(void) {
    uint32_t inputs[] = { 0x11, 0xAB, 0x200, 0xFFFF };
    for (int i = 0; i < 4; i++) {
        printf("dispatch(%04X) = 0x%02X\n", inputs[i], dispatch(inputs[i], 12));
    }
    return 0;
}
