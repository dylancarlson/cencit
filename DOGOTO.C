/************************************************************************
 *              Command-interpreter code for Citadel
 ************************************************************************/

#include <string.h>
#include <time.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <dos.h>
#include <conio.h>
#endif

#include "keywords.h"
#include "proto.h"
#include "global.h"

/************************************************************************
 *                              Contents
 *
 *      doGoto()                handles G(oto)          command
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 * History:
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doGoto(char expand, int skip);

/************************************************************************/
/*      doGoto() handles G(oto) command                                 */
/************************************************************************/
void doGoto(expand, skip)
char expand;            /* TRUE to accept following parameters  */
{
    label roomName;

    if (!skip) {
        mPrintf("\bGoto");
        skiproom = FALSE;
    } else {
        mPrintf("\bBypass to");
        skiproom = TRUE;
    }

    if (!expand) {
		mPrintf(" ");
        gotoRoom("");
    } else {
		getString("", roomName, NAMESIZE, 1, ECHO, "");
		normalizeString(roomName);
	
		if (roomName[0] == '?') {
			listRooms(OLDNEW, FALSE, FALSE);
		} else {
			mPrintf(" ");
			gotoRoom(roomName);
		}
	}
}
