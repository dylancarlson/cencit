/************************************************************************/
/*                            library.c                                 */
/*                                                                      */
/*                  Routines used by Ctdl & Confg                       */
/************************************************************************/

#include <string.h>

/* Citadel */
#include "ctdl.h"

#ifndef ATARI_ST
#include <alloc.h>
#endif

#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getGroup()              loads groupBuffer                       */
/*      putGroup()              stores groupBuffer to disk              */
/*                                                                      */
/*      getHall()               loads hallBuffer                        */
/*      putHall()               stores hallBuffer to disk               */
/*                                                                      */
/*      getLog()                loads requested CTDLLOG record          */
/*      putLog()                stores a logBuffer into citadel.log     */
/*                                                                      */
/*      getRoom()               load given room into RAM                */
/*      putRoom()               store room to given disk slot           */
/*                                                                      */
/************************************************************************/


/* --------------------------------------------------------------------
 *	History:
 *
 *  04/01/90    FJM     added checks reconfigure on new version
 *  04/08/90    FJM     cleanup
 *  06/09/90    FJM     Added readCrontab & writeCrontab
 *  08/13/90    FJM     Moved more allocation to allocateTables()
 *	04/18/91	BLJ 	Added code for BIO and password
 *
 * -------------------------------------------------------------------- */

/************************************************************************/
/*		getGroup() loads group data into RAM buffer 					*/
/************************************************************************/
void getGroup(void)
{
    fseek(grpfl, 0L, 0);

	if (fread(grpBuf, sizeof(struct groupBuffer), 1, grpfl) != 1) {
        crashout("getGroup-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putGroup() stores group data into grp.cit                       */
/************************************************************************/
void putGroup(void)
{
    fseek(grpfl, 0L, 0);

	if (fwrite(grpBuf, sizeof(struct groupBuffer), 1, grpfl) != 1) {
        crashout("putGroup-write fail");
    }
    fflush(grpfl);
}

/************************************************************************/
/*      getHall() loads hall data into RAM buffer                       */
/************************************************************************/
void getHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fread(hallBuf, sizeof(struct hallBuffer), 1, hallfl) != 1) {
        crashout("getHall-read fail EOF detected!");
    }
}

/************************************************************************/
/*		putHall() stores group data into hall.dat						*/
/************************************************************************/
void putHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fwrite(hallBuf, sizeof(struct hallBuffer), 1, hallfl) != 1) {
        crashout("putHall-write fail");
    }
    fflush(hallfl);
}

/************************************************************************/
/*		getHallgrp() loads hallgrp data into RAM buffer 				*/
/************************************************************************/
void getHallgrp(void)
{
	fseek(hallgrpfl, 0L, 0);

	if (fread(hallgrp, sizeof(struct hallGroup), 1, hallgrpfl) != 1) {
		crashout("getHallgrp-read fail EOF detected!");
    }
}

/************************************************************************/
/*		putHallgrp() stores hallgroup data into hallgrp.dat 			*/
/************************************************************************/
void putHallgrp(void)
{
	fseek(hallgrpfl, 0L, 0);

	if (fwrite(hallgrp, sizeof(struct hallGroup), 1, hallgrpfl) != 1) {
		crashout("putHallgrp-write fail");
    }
	fflush(hallgrpfl);
}

/************************************************************************/
/*		getRoomgrp() loads roomgrp data into RAM buffer 				*/
/************************************************************************/
void getRoomgrp(void)
{
	fseek(roomgrpfl, 0L, 0);

	if (fread(roomgrp, sizeof(struct roomGroup), 1, roomgrpfl) != 1) {
		crashout("getRoomgrp-read fail EOF detected!");
    }
}

/************************************************************************/
/*		putRoomgrp() stores roomgroup data into roomgrp.dat 			*/
/************************************************************************/
void putRoomgrp(void)
{
	fseek(roomgrpfl, 0L, 0);

	if (fwrite(roomgrp, sizeof(struct roomGroup), 1, roomgrpfl) != 1) {
		crashout("putRoomgrp-write fail");
    }
	fflush(roomgrpfl);
}

/************************************************************************/
/*      getLog() loads requested log record into RAM buffer             */
/************************************************************************/
void getLog(struct logBuffer *lBuffer, int n)
{
    long int s;

	if (lBuffer == &logBuf)
        thisLog = n;

    s = (long) n *(long) sizeof logBuf;

    fseek(logfl, s, 0);

	if (fread(lBuffer, sizeof logBuf, 1, logfl) != 1) {
        crashout("getLog-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putLog() stores given log record into log.cit                   */
/************************************************************************/
void putLog(struct logBuffer *lBuffer, int n)
{
    long int s;

    s = (long) n *(long) (sizeof(struct logBuffer));

    fseek(logfl, s, 0);

	if (fwrite(lBuffer, sizeof logBuf, 1, logfl) != 1) {
        crashout("putLog-write fail");
    }
    fflush(logfl);
}

/************************************************************************/
/*      getRoom()                                                       */
/************************************************************************/
void getRoom(int rm)
{
    long int s;
	char pw[4];
	int i;

    /* load room #rm into memory starting at buf */

    s = (long) rm *(long) sizeof roomBuf;

    fseek(roomfl, s, 0);
	if (fread(&roomBuf, sizeof roomBuf, 1, roomfl) != 1) {
        crashout("getRoom(): read failed error or EOF!");
    }

	if(ctdlConfig == TRUE) {
		thisRoom = rm;
		return;
	}

	if(roomBuf.autoapp) {
		thisRoom = rm;
		/*
			mPrintf("Executing Automatic Application...");
			doCR();
		*/
		ExeAplic();
		return;
	}

	if (roomBuf.rbflags.BIO) {
		if(roomBuf.lockroom) {
			if(roomBuf.password[0]) {
				if(!logBuf.lbflags.SYSOP) {
					getString("password",pw, 4, FALSE, ECHO, "");
					if (strcmp(pw,roomBuf.password) != SAMESTRING) {
						mPrintf("Sorry, Invalid password!");
						doCR();
						mPrintf(" ");
						i = thisRoom;
						getRoom(i);
						return;
					}
					doCR();
					mPrintf(" ");

					if(!roomBuf.lockpass) {
						if(getYesNo("Remove password",0)) roomBuf.password[0] = '\0';
						doCR();
						mPrintf(" ");
					}
				}
				else {
					mPrintf("Password: %s",roomBuf.password);
					doCR();
					mPrintf(" ");
				}
			}
			else {
				i = getYesNo("Put a password on this room",0);
				if(i) {
					getString("password",pw, 4, FALSE, ECHO, "");
					strncpy(roomBuf.password,pw,4);
					putRoom(rm);
					doCR();
					mPrintf(" ");
				}
			}
		}
	}

	thisRoom = rm;
}

/************************************************************************/
/*      putRoom() stores room in buf into slot rm in room.buf           */
/************************************************************************/
void putRoom(int rm)
{
    long int s;

    s = (long) rm *(long) sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fwrite(&roomBuf, sizeof roomBuf, 1, roomfl) != 1) {
        crashout("putRoom() crash! 0 returned.");
    }
    fflush(roomfl);
}

