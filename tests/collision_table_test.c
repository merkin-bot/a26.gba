#include <assert.h>
#include <stdint.h>
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include <stdio.h>

#define PL0_MASK 0x01
#define PL1_MASK 0x02
#define ML0_MASK 0x04
#define PF_MASK 0x20

void reset_collisions(void);
void set_collisions(BYTE b);

BYTE tiaRead[0x0e];
int main(void) {
    reset_collisions();
    set_collisions(PL0_MASK);
    set_collisions(ML0_MASK | PL1_MASK);
    assert(tiaRead[CXM0P] & 0x80);
    reset_collisions();
    set_collisions(PL0_MASK | PF_MASK);
    assert(tiaRead[CXP0FB] & 0x80);
    puts("Collision lookup smoke tests passed");
    return 0;
}
