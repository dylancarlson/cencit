/************************************************************************
 *                              doaide.c
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
 *      doAide()                handles Aide-only       commands
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 * History:
 *	01/28/91  FJM		Made room edit prompt inverse.
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doAide(char moreYet, char first);

/************************************************************************
 *      doAide() handles the aide-only menu
 *          return FALSE to fall invisibly into default error msg
 ************************************************************************/

void doAide(moreYet, first)
char moreYet;
char first;         /* first parameter if TRUE */
{
    int roomExists();
    char oldchat;
    static int acount = 0;
	static int jcount = 0;

    if (moreYet)
        first = '\0';

    mtPrintf(TERM_REVERSE, "\bAide special fn:");
    mPrintf(" ");

    if (first)
        oChar(first);

    switch (toupper(first ? (char) first : (char) iChar())) {
        case 'A':
            mPrintf("\bAttributes ");
            if (roomBuf.rbflags.MSDOSDIR != 1) {
                if (expert)
                    mPrintf("? ");
                else
                    mPrintf("\n Not a directory room.");
            } else
                attributes();
            break;
        case 'C':
            chatReq = TRUE;
			oldchat = (char) cfg->noChat;
			cfg->noChat = FALSE;
            mPrintf("\bChat\n ");
            if (whichIO == MODEM)
                ringSysop();
            else
                chat();
			cfg->noChat = oldchat;
            break;
        case 'E':
            mPrintf("\bEdit room\n  \n");
            renameRoom();
            break;
        case 'F':
            mPrintf("\bFile set\n  \n");
            batchinfo(TRUE);
            break;
        case 'G':
            mPrintf("\bGroup membership\n  \n");
            groupfunc();
            break;
        case 'H':
            mPrintf("\bHallway changes\n  \n");
			if (!cfg->aidehall && !sysop)
                mPrintf(" Must be a Sysop!\n");
            else
                hallfunc();
            break;
        case 'I':
            mPrintf("\bInsert message\n ");
            insert();
            break;
        case 'K':
			if(logBuf.DUNGEONED) {
				nextblurb("dungeon",&jcount,1);
			}
			else {
				mPrintf("\bKill room\n ");
				killroom();
			}
            break;
        case 'L':
            mPrintf("\bList group");
            listgroup();
            break;
        case 'M':
            mPrintf("\bMove file ");
            moveFile();
            break;
        case 'R':
            mPrintf("\bRename file ");
            if (roomBuf.rbflags.MSDOSDIR != 1) {
                if (expert)
                    mPrintf("? ");
                else
                    mPrintf("\n Not a directory room.");
            } else
                renamefile();
            break;

        case 'S':
            mPrintf("\bSet file info\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1) {
                if (expert)
                    mPrintf("? ");
                else
                    mPrintf("\n Not a directory room.");
            } else
                setfileinfo();
            break;

        case 'U':
            mPrintf("\bUnlink file\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1) {
                if (expert)
                    mPrintf("? ");
                else
                    mPrintf("\n Not a directory room.");
            } else
                unlinkfile();
            break;
        case 'V':
            mPrintf("\bView Help Text File\n ");
            nextblurb("aide", &acount, 1);
            break;
        case 'W':
            mPrintf("\bWindow into hallway\n ");
			if (!cfg->aidehall && !sysop)
                mPrintf(" Must be a Sysop!\n");
            else
                windowfunc();
            break;
        case '?':
            tutorial("aide.mnu", 1);
            break;
        default:
            if (!expert)
                mPrintf("\n '?' for menu.\n ");
            else
                mPrintf(" ?\n ");
            break;
    }
}
