#include "tia_audio.h"
#include "address.h"
#include <stdint.h>

/*
 * TIA sound timing/register model adapted from StellaDS TIASound.
 * StellaDS Copyright (c) 2020-2026 Dave Bernazzani and Stella Team.
 * Licensed GPL-2.0; see StellaDS LICENSE/Copyright.txt.
 */

typedef struct {
    BYTE control;
    BYTE frequency;
    BYTE volume;
    unsigned divider;
    unsigned divider_max;
    unsigned phase;
} TiaTone;

static TiaTone tone[2];
static volatile uint32_t tia_fifo_ring[128] __attribute__((section(".sbss")));
static uint8_t poly4[2];
static uint8_t poly5[2];
static uint16_t poly9[2];

static const uint8_t tia_poly4[16] = {
    0,0,0,1,0,1,0,1,1,1,1,0,1,1,0,0
};
static const uint8_t tia_poly5[32] = {
    0,0,1,0,0,1,1,1,0,1,1,1,1,0,1,0,
    1,0,1,1,0,0,1,1,0,0,0,1,0,0,0,0
};

static unsigned poly4_bit(unsigned channel) {
    return tia_poly4[poly4[channel] & 15u];
}

static unsigned poly5_bit(unsigned channel) {
    return tia_poly5[poly5[channel] & 31u];
}

static unsigned poly9_bit(unsigned channel) {
    poly9[channel] = (poly9[channel] * 73u + 19u) & 0x1ffu;
    return (poly9[channel] & 1u) != 0;
}

static unsigned char direct_sample(unsigned value) {
    return (unsigned char)(128u + value * 4u);
}

static unsigned channel_mix(TiaTone *t);

static void tia_audio_dma_setup(void) {
#ifdef A26_GBA_BUILD
    *(volatile uint16_t *)0x04000080 = 0x0b0c;

    *(volatile uint32_t *)0x040000bc = (uint32_t)(uintptr_t)tia_fifo_ring;
    *(volatile uint32_t *)0x040000c0 = 0x040000a0u;
    *(volatile uint16_t *)0x040000c4 = 128;
    *(volatile uint16_t *)0x040000c6 = 0xb600;

    *(volatile uint16_t *)0x04000100 = (uint16_t)(0xffffu - 512u);
    *(volatile uint16_t *)0x04000102 = 0x0080;
#endif
}

void tia_audio_refill(void) {
    unsigned i;
    for (i = 0; i < 128u; ++i) {
        unsigned sample = channel_mix(&tone[0]) + channel_mix(&tone[1]);
        unsigned char value = direct_sample(sample);
        tia_fifo_ring[i] = value | ((uint32_t)value << 8) |
                           ((uint32_t)value << 16) | ((uint32_t)value << 24);
    }
}

void tia_audio_init(void) {
    tone[0].control = tone[0].frequency = tone[0].volume = 0;
    tone[1].control = tone[1].frequency = tone[1].volume = 0;
    tone[0].divider = tone[1].divider = 0;
    tone[0].divider_max = tone[1].divider_max = 0;
    tone[0].phase = tone[1].phase = 0;
    poly4[0] = poly4[1] = 0;
    poly5[0] = poly5[1] = 0;
    poly9[0] = 137;
    poly9[1] = 271;
    tia_audio_dma_setup();
}

void tia_audio_write(unsigned address, BYTE value) {
    unsigned channel;
    if (address == AUDC0) channel = 0, tone[channel].control = value & 15;
    else if (address == AUDC1) channel = 1, tone[channel].control = value & 15;
    else if (address == AUDF0) channel = 0, tone[channel].frequency = value & 31;
    else if (address == AUDF1) channel = 1, tone[channel].frequency = value & 31;
    else if (address == AUDV0) channel = 0, tone[channel].volume = value & 15;
    else if (address == AUDV1) channel = 1, tone[channel].volume = value & 15;
    else return;

    if (tone[channel].control == 0)
        tone[channel].divider = tone[channel].divider_max = 0;
    else {
        unsigned divisor = tone[channel].frequency + 1u;
        if ((tone[channel].control & 12u) == 12u)
            divisor *= 3u;
        tone[channel].divider_max = divisor;
        if (tone[channel].divider == 0)
            tone[channel].divider = divisor;
    }
}

static unsigned channel_mix(TiaTone *t) {
    unsigned channel = t == &tone[0] ? 0 : 1;
    if (t->control == 0)
        return t->volume;
    if (!t->divider_max)
        return 0;
    if (t->divider > 1u) {
        t->divider--;
        return t->phase ? t->volume : 0;
    }
    t->divider = t->divider_max;

    poly5[channel] = (poly5[channel] + 1u) & 31u;
    unsigned clocked = 1;
    if ((t->control & 2u) == 0)
        clocked = 1;
    else if ((t->control & 1u) == 0)
        clocked = poly5[channel] & 1u;
    else
        clocked = poly5[channel] == 0 || poly5_bit(channel);

    if (!clocked)
        return t->volume ? t->phase : 0;

    if (t->control & 4u) {
        t->phase ^= 1;
        return t->phase ? t->volume : 0;
    }
    if (t->control & 8u) {
        if (t->control == 9u)
            return poly9_bit(channel) ? t->volume : 0;
        return poly5_bit(channel) ? t->volume : 0;
    }
    poly4[channel] = (poly4[channel] + 1u) & 15u;
    return poly4_bit(channel) ? t->volume : 0;
}

unsigned tia_audio_mix(void) {
    return channel_mix(&tone[0]) + channel_mix(&tone[1]);
}
