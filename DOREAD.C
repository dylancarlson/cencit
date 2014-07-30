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
 *      doRead()                handles R(ead)          command
 *
 ************************************************************************/

/*  04/10/91    FJM     Fixed (Node) bug for unlisted nodes.
 *  05/15/91    BLJ     Added w (word) command
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doRead(char moreYet, char first);

/************************************************************************/
/*      doRead() handles R(ead) command                                 */
/************************************************************************/
void doRead(moreYet, first)
char moreYet;           /* TRUE to accept following parameters */
char first;         /* first parameter if TRUE             */
{
    char abort, done, letter;
    char whichMess, revOrder, verbose;

    if (moreYet)
        first = '\0';

    mPrintf("\bRead ");

    abort = FALSE;
    revOrder = FALSE;
    verbose = FALSE;
    whichMess = NEWoNLY;
    mf.mfPub = FALSE;
    mf.mfMai = FALSE;
    mf.mfLim = FALSE;
    mf.mfUser[0] = FALSE;
    mf.mfWord[0] = FALSE;
    mf.mfGroup[0] = FALSE;

	if (!loggedIn && !cfg->unlogReadOk) {
        mPrintf("\n --Must log in to read.\n ");
        return;
    }
    if (first)
        oChar(first);

    do {
        outFlag = IMPERVIOUS;
        done = TRUE;

        letter = (char) (toupper(first ? (int) first : (int) iChar()));

    /* handle uponly flag! */
		if (roomTab[thisRoom].rtflags.UPONLY
        && (letter == 'T')) {
            mPrintf("\b\n\n  --Room is upload only.\n ");
            break;
        }
        switch (letter) {
            case '\n':
            case '\r':
                moreYet = FALSE;
                break;
            case 'B':
                mPrintf("\bBy-User ");
				if (!roomBuf.rbflags.ANONYMOUS || sysop) {
					mf.mfUser[0] = TRUE;
					done = FALSE;
				} else {
					mPrintf("\n Not allowed in anonymous rooms.\n");
					abort = TRUE;
				}
                break;
            case 'C':
                mPrintf("\bConfiguration ");
                showconfig(&logBuf);
                abort = TRUE;
                break;
            case 'D':
                mPrintf("\bDirectory");
                if (!roomBuf.rbflags.MSDOSDIR) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    readdirectory(verbose);
                abort = TRUE;
                break;
            case 'E':
                mPrintf("\bExclusive ");
                mf.mfMai = TRUE;
                done = FALSE;
                break;
            case 'F':
                mPrintf("\bForward ");
                revOrder = FALSE;
                whichMess = OLDaNDnEW;
                done = FALSE;
                break;
            case 'H':
                mPrintf("\bHallways ");
                readhalls();
                abort = TRUE;
                break;
            case 'I':
                mPrintf("\bInfo file(s)");
                if (!roomBuf.rbflags.MSDOSDIR) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    readinfofile(verbose);
                abort = TRUE;
                break;
            case 'L':
                mPrintf("\bLimited-access ");
                mf.mfLim = TRUE;
                done = FALSE;
                break;
            case 'N':
                mPrintf("\bNew ");
                whichMess = NEWoNLY;
                done = FALSE;
                break;
            case 'O':
                mPrintf("\bOld ");
                revOrder = TRUE;
                whichMess = OLDoNLY;
                done = FALSE;
                break;
            case 'P':
                mPrintf("\bPublic ");
                mf.mfPub = TRUE;
                done = FALSE;
                break;
            case 'R':
                mPrintf("\bReverse ");
                revOrder = TRUE;
                whichMess = OLDaNDnEW;
                done = FALSE;
                break;
            case 'S':
                mPrintf("\bStatus\n ");
                systat();
                abort = TRUE;
                break;
            case 'T':
                mPrintf("\bTextfile: ");
                if (!roomBuf.rbflags.MSDOSDIR) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    readtextfile();
                abort = TRUE;
                break;
            case 'U':
                mPrintf("\bUser log ");
                Readlog(verbose);
                abort = TRUE;
                break;
            case 'V':
                mPrintf("\bVerbose ");
                verbose = TRUE;
                done = FALSE;
                break;

            /* This is new to CenCit and is NOT compatable with old Cit's */
            case 'W':
                mPrintf("\bWord search ");
                mf.mfWord[0] = TRUE;
                done = FALSE;
                break;

            case 'Z':
                mPrintf("\bZIP-file(s)");
                if (!roomBuf.rbflags.MSDOSDIR) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    readzip(verbose);
                abort = TRUE;
                break;
            case '?':
                tutorial("readopt.mnu", 1);
                abort = TRUE;
                break;
            default:
                mPrintf("? ");
                abort = TRUE;
                if (!expert)
                    tutorial("readopt.mnu", 1);
                break;
        }
        first = '\0';

    } while (!done && moreYet && !abort);

    if (abort)
        return;

    showMessages(whichMess, revOrder, verbose);
}
