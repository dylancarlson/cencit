/************************************************************************
 *                              doupdown.c
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
 *              doDownload()
 *              doUpload()
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *	History:
 *
 * 
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doDownload(char ex);
void doUpload(char ex);

/************************************************************************/
/*      doDownload()                                                    */
/************************************************************************/
void doDownload(char ex)
{
    ex = ex;

    mPrintf("\bDownload ");

	if (!loggedIn && !cfg->unlogReadOk) {
        mPrintf("\n --Must log in to download.\n ");
        return;
    }
    /* handle uponly flag! */
	if (roomTab[thisRoom].rtflags.UPONLY) {
        mPrintf("\n --Room is upload only.\n ");
        return;
    }
    if (!roomBuf.rbflags.MSDOSDIR) {
        if (expert)
            mPrintf("? ");
        else
            mPrintf("\n Not a directory room.");
        return;
    }
    download('\0');
}

/************************************************************************/
/*      doUpload()                                                      */
/************************************************************************/
void doUpload(char ex)
{
    ex = ex;

    mPrintf("\bUpload ");

    /* handle downonly flag! */
	if (roomTab[thisRoom].rtflags.DOWNONLY) {
        mPrintf("\n\n  --Room is download only.\n ");
        return;
    }
	if (!loggedIn && !cfg->unlogEnterOk) {
        mPrintf("\n\n  --Must log in to upload.\n ");
        return;
    }
    if (!roomBuf.rbflags.MSDOSDIR) {
        if (expert)
            mPrintf("? ");
        else
            mPrintf("\n Not a directory room.");
        return;
    }
    upload('\0');
    return;
}
