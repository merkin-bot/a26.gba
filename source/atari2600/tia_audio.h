#ifndef DSSMM_TIA_AUDIO_H
#define DSSMM_TIA_AUDIO_H
#include "types.h"

void tia_audio_init(void);
void tia_audio_write(unsigned address, BYTE value);
unsigned tia_audio_mix(void);
void tia_audio_refill(void);
#endif
