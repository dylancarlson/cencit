/************************************************************************
 * $Header:   H:/VCS/FCIT/DOENTER.C_V   1.26   19 Apr 1991 23:38:32   FJM  $
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
 *      doEnter()               handles E(nter)         command
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doEnter(char moreYet, char first);
void exclude(void);

/***********************************************************************/
/*     doEnter() handles E(nter) command                               */
/***********************************************************************/
void doEnter(char moreYet, char first)
/* moreYet;           TRUE to accept following parameters */
/* first;             first parameter if true             */
{
    char done;
    char letter;
	static int jcount = 0;

    if (moreYet)
        first = '\0';

    done = TRUE;
    mailFlag = FALSE;
    oldFlag = FALSE;
    limitFlag = FALSE;
    linkMess = FALSE;

    mPrintf("\bEnter ");

    if (first)
		oChar((uchar) first);

    do {
        outFlag = IMPERVIOUS;
        done = TRUE;

        letter = (char) (toupper(first ? (char) first : (char) iChar()));

    /* allow exclusive mail entry only */
		if (!loggedIn && !cfg->unlogEnterOk && (letter != 'E')) {
            mPrintf("\b\n  --Must log in to enter.\n ");
            break;
        }
    /* handle readonly flag! */
		if (roomTab[thisRoom].rtflags.READONLY && !(sysop || aide)
            && ((letter == '\r') || (letter == '\n') || (letter == 'M')
        || (letter == 'E') || (letter == 'O') || (letter == 'G'))) {
            mPrintf("\b\n\n  --Room is readonly.\n ");
            break;
        }
	/* handle steve */
		if (MessageRoom[thisRoom] == cfg->MessageRoom && !(sysop || aide)
            && ((letter == '\r') || (letter == '\n') || (letter == 'M')
        || (letter == 'E') || (letter == 'O') || (letter == 'G'))) {
			mPrintf("\b\n\n  --Only %d messages per room.\n ", cfg->MessageRoom);
            break;
        }
    /* handle nomail flag! */
        if (logBuf.lbflags.NOMAIL && (letter == 'E')) {
            mPrintf("\b\n\n  --You can't enter mail.\n ");
            break;
        }
    /* handle downonly flag! */
        if (roomTab[thisRoom].rtflags.DOWNONLY && !groupseesroom(thisRoom)
        && ((letter == 'T') || (letter == 'W'))) {
            mPrintf("\b\n\n  --Room is download only.\n ");
            break;
        }
        switch (letter) {
            case '\r':
            case '\n':
                moreYet = FALSE;
                makeMessage();
                break;
            case 'C':
                mPrintf("\bConfiguration ");
                configure(FALSE);
                break;
            case 'D':
                mPrintf("\bDefault-hallway ");
                doCR();
                defaulthall();
                break;
            case 'E':
                mPrintf("\bExclusive message ");
                doCR();
                if (whichIO != CONSOLE)
                    echo = CALLER;
                limitFlag = FALSE;
                mailFlag = TRUE;
                makeMessage();
                echo = BOTH;
                break;
            case 'F':
                mPrintf("\bForwarding-address ");
                doCR();
                forwardaddr();
                break;
            case 'H':
                mPrintf("\bHallway ");
                doCR();
                enterhall();
                break;
            case 'L':
                mPrintf("\bLimited-access ");
                done = FALSE;
                limitFlag = TRUE;
                break;
            case '*':
                if (!sysop)
                    mPrintf("\b\n\n  --You can't enter a file-linked message.\n ");
                else {
                    mPrintf("\bFile-linked ");
                    done = FALSE;
                    linkMess = TRUE;
                }
                break;
            case 'M':
                mPrintf("\bMessage ");
                doCR();
                makeMessage();
                break;
            case 'G':
                mPrintf("\bGroup-only Message");
                doCR();
                limitFlag = TRUE;
                makeMessage();
                break;
            case 'O':
                mPrintf("\bOld-message ");
                done = FALSE;
                oldFlag = TRUE;
                break;
            case 'P':
                mPrintf("\bPassword ");
                doCR();
                newPW();
                break;
            case 'R':
                mPrintf("\bRoom ");
				if (!cfg->nonAideRoomOk && !aide) {
                    mPrintf("\n --Must be aide to create room.\n ");
                    break;
                }
                if (!loggedIn) {
                    mPrintf("\n --Must log in to create new room.\n ");
                    break;
                }
				if(logBuf.DUNGEONED) {
					nextblurb("dungeon",&jcount,1);
				}
				else {
					doCR();
					makeRoom();
				}
                break;
            case 'T':
                mPrintf("\bTextfile ");
                if (roomBuf.rbflags.MSDOSDIR != 1) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                    return;
                }
                if (!loggedIn) {
                    mPrintf("\n --Must be logged in.\n ");
                    break;
                }
                entertextfile();
                break;

        /* WC isn't needed on DOS machines which use DSZ or equiv. */
#ifndef __MSDOS__
            case 'W':
                mPrintf("\bWC-protocol upload ");
                if (!roomBuf.rbflags.MSDOSDIR) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else if (!loggedIn) {
                    mPrintf("\n --Must be logged in.\n ");
                } else
                    enterwc();
                doCR();
                break;
#endif
            case 'A':
                mPrintf("\bApplication");
                if (!loggedIn) {
                    mPrintf("\n --Must be logged in.\n ");
                } else
                    ExeAplic();
                break;
            case 'X':
                mPrintf("\bExclude Room ");
                exclude();
                break;
            case 'S':

				if (!cfg->surnames) {
                    mPrintf(
                    "\b\n\nSurnames & Titles are not allowed on this system.\n");
                    doCR();
				} else if (!sysop && !cfg->entersur) {
                    mPrintf("\b\n\n  --Users can't enter their surname.\n ");
                    break;
                } else {
                    label tempsur;

                    mPrintf("\bSurname / Title\n ");

					if (!sysop && logBuf.TITLELOK) {
						mPrintf("\n\n  --Your title has been locked!\n ");
					} else {
						getString("title", tempsur, NAMESIZE, 0, ECHO,
						logBuf.title);
						if (*tempsur)
							strcpy(logBuf.title, tempsur);
						normalizeString(logBuf.title);
					}

					if (!sysop && logBuf.SURNAMLOK) {
						mPrintf("\n\n  --Your surname has been locked!\n ");
					} else {
						getString("surname", tempsur, NAMESIZE, 0, ECHO,
						logBuf.surname);
						if (*tempsur)
							strcpy(logBuf.surname, tempsur);
						normalizeString(logBuf.surname);
					}
                }
                break;
            case '?':
                tutorial("entopt.mnu", 1);
                break;
            default:
                mPrintf("? ");
                if (!expert)
                    tutorial("entopt.mnu", 1);
                break;
        }
    }
    while (!done && moreYet);

    oldFlag = FALSE;
    mailFlag = FALSE;
    limitFlag = FALSE;

}
