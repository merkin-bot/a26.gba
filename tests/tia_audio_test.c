#include "tia_audio.h"
#include "address.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    tia_audio_init();
    assert(tia_audio_mix() == 0);

    tia_audio_write(AUDV0, 8);
    assert(tia_audio_mix() == 8);

    tia_audio_write(AUDV0, 0);
    assert(tia_audio_mix() == 0);

    tia_audio_init();
    tia_audio_write(AUDF0, 1);
    tia_audio_write(AUDV0, 15);
    unsigned first = tia_audio_mix();
    unsigned second = tia_audio_mix();
    assert(first <= 15);
    assert(second <= 15);

    tia_audio_write(AUDF1, 3);
    tia_audio_write(AUDV1, 7);
    assert(tia_audio_mix() <= 22);

    tia_audio_init();
    tia_audio_write(AUDC0, 0);
    tia_audio_write(AUDV0, 12);
    assert(tia_audio_mix() == 12);

    tia_audio_init();
    tia_audio_write(AUDC0, 12);
    tia_audio_write(AUDF0, 1);
    tia_audio_write(AUDV0, 10);
    for (unsigned i = 0; i < 20; ++i)
        (void)tia_audio_mix();

    puts("TIA audio state/divider smoke tests passed");
    return 0;
}
