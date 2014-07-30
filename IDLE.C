/* --------------------------------------------------------------------
 *  IDLE.C                   Citadel
 * --------------------------------------------------------------------
 *  This file contains the IDLE functions
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include <alloc.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#include <dos.h>
#endif

#include "proto.h"
#include "global.h"

/* --------------------------------------------------------------------
 *                              Contents
 * --------------------------------------------------------------------
 *
 *
 *
 * -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 * History:
 * 02/28/91 	FJM 	Initial revision.
 * 05/16/91 	FJM 	Speedups.
 * 09/24/91 	BLJ 	When Idle blank border too!
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/*
 * idle_flag true if idle
 *
 */

void do_idle(int idle_flag)
{
	static enum {I_INITIAL,I_NORMAL,I_IN_IDLE} idle_state=I_INITIAL;
	long timer;
	static long last_time;
	static int loopcnt = 0;
	char save_attr;

	time (&timer);

	if (cfg->screen_save) {
		switch (idle_flag) {
			case 0:				/* clear idle state */
				if (idle_state == I_IN_IDLE) {
					idle_state = I_NORMAL;
					restore_screen();
					curson();
				}
				++loopcnt;
				if (!(loopcnt % 100)) {		/* do these one time in 100 */
					if (farheapcheck() < 0)
						crashout("memory corrupted 2");
				}
				update25();
				last_time = timer;
			break;
			case 1:				/* maybe idle state	*/
				if (idle_state == I_NORMAL) {
					if (!loggedIn && timer - last_time >= (long)cfg->screen_save) {
						idle_state = I_IN_IDLE;
						save_screen();
						cursoff();
						save_attr = cfg->attr;
						cfg->attr = 7;
						setscreen();
						if(gmode() != 7)
							outp(0x3d9, 0);
						cls();
						cfg->attr = save_attr;
                    } else {
						if(!cfg->screen_save) {
							/* display the time */
							update25();
						}
					}
				} else if (idle_state == I_INITIAL) {
					idle_state = I_NORMAL;
					last_time = timer;
				}
			break;
			case 2:				/* ignore elapsed time	*/
				if (idle_state == I_IN_IDLE)
					do_idle(0);		/* recurse to do restore */
				idle_state = I_INITIAL;
			break;
		}
	} else
		update25();
}

