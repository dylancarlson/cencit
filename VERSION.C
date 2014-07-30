/* ------------------------------------------------------------------------- */
/*  Makes us able to change the version at a glance       VERSION.C          */
/* ------------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  History:
 *
 *  03/03/90    GREN    Display the 'family tree' of some Citadels.
 *  03/07/90    {zm}    Add 'softname' as the name of the software!
 *  05/20/90    FJM     Put compdate[] and comptime here (force reconfig)
 *                                              Some cleanup.
 *  05/15/91    BLJ     New version Message
 *	09/16/91	BLJ 	Added char release so only this module must be
 *						Re-Compiled
 * -------------------------------------------------------------------- */
#include "ctdl.h"

char *softname = "CenCit";

#ifndef ATARI_ST
char *version = REL;

#else
char *version = "1.0";

#endif

#ifdef ALPHA_TEST
char release = 'A';
#else
#ifdef BETA_TEST
char release = 'B';
#else
char release = 'P';
#endif
#endif

const char compdate[] = __DATE__;
const char comptime[] = __TIME__;

char *welcome[] = {     /* 10 LINES MAX LENGTH!!!!!! */
    "                                                ",
    "   Å                                            ",
    "       Å         From the Stars we were born,   ",
    "                 To the Stars we will return.   ",
    "    Å                                           ",
    "                                                ",
    0,
    0,
    0,
    0,
    0
};

char *copyright[] = {       /* 2 LINES ONLY!!!! */
    "Centauri's Citadel",
    "Public Domain Software",
    0
};

unsigned int welcomecrcs[] = {
   0x2ed4, 0x4cb7, 0x2583, 0xa1b8, 0x2d86, 
   0x2ed4, 0x0000, 0x0000, 0x0000, 0x0000
};

unsigned int copycrcs[] = { 0x9809, 0x6b01 };

