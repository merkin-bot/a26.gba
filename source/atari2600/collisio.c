/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: collision.c,v 1.2 1996/03/21 16:36:01 alex Exp $
******************************************************************************/

/* The 2600 collision detection code */

#include "types.h"
#include "address.h"
#include "vmachine.h"

/*
  There are 6 different objects on the screen. Each takes one bit of the 
  collision vector.
  Bit 0: Player 0
  Bit 1: Player 1
  Bit 2: Missile 0
  Bit 3: Missile 1
  Bit 4: Ball
  Bit 5: Playfield
*/

#define PL0_MASK 0x01
#define PL1_MASK 0x02
#define ML0_MASK 0x04
#define ML1_MASK 0x08
#define BL_MASK 0x10
#define PF_MASK 0x20

BYTE colvect[256];
static BYTE collision_table[64];
static unsigned collision_ready;


/* Resets the collision registers of the tia */
void reset_collisions(void)
{
	int i;
	for(i=CXM0P; i<INPT0; i++)
	tiaRead[i]=0;
	if(!collision_ready) {
		int object;
		for(object=0;object<64;++object) {
			BYTE result=0;
			if((object&ML0_MASK)&&(object&PL1_MASK))result|=0x01;
			if((object&ML0_MASK)&&(object&PL0_MASK))result|=0x02;
			if((object&ML1_MASK)&&(object&PL0_MASK))result|=0x04;
			if((object&ML1_MASK)&&(object&PL1_MASK))result|=0x08;
			if((object&PL0_MASK)&&(object&PF_MASK))result|=0x10;
			if((object&PL0_MASK)&&(object&BL_MASK))result|=0x20;
			if((object&PL1_MASK)&&(object&PF_MASK))result|=0x40;
			if((object&PL1_MASK)&&(object&BL_MASK))result|=0x80;
			collision_table[object]=result;
		}
		collision_ready=1;
	}
}


/* Does collision testing on the pixel b */
void set_collisions(BYTE b)
{
	BYTE p=collision_table[b&63];
	tiaRead[CXM0P]|=(p&0x01)?0x80:0;
	tiaRead[CXM0P]|=(p&0x02)?0x40:0;
	tiaRead[CXM1P]|=(p&0x04)?0x80:0;
	tiaRead[CXM1P]|=(p&0x08)?0x40:0;
	tiaRead[CXP0FB]|=(p&0x10)?0x80:0;
	tiaRead[CXP0FB]|=(p&0x20)?0x40:0;
	tiaRead[CXP1FB]|=(p&0x40)?0x80:0;
	tiaRead[CXP1FB]|=(p&0x80)?0x40:0;
	if((b&ML0_MASK)&&(b&PF_MASK))tiaRead[CXM0FB]|=0x80;
	if((b&ML0_MASK)&&(b&BL_MASK))tiaRead[CXM0FB]|=0x40;
	if((b&ML1_MASK)&&(b&PF_MASK))tiaRead[CXM1FB]|=0x80;
	if((b&ML1_MASK)&&(b&BL_MASK))tiaRead[CXM1FB]|=0x40;
	if((b&BL_MASK)&&(b&PF_MASK))tiaRead[CXBLPF]|=0x80;
	if((b&PL0_MASK)&&(b&PL1_MASK))tiaRead[CXPPMM]|=0x80;
	if((b&ML0_MASK)&&(b&ML1_MASK))tiaRead[CXPPMM]|=0x40;
}



