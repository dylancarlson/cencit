/************************************************************************
 * $Header:   H:/VCS/FCIT/DOSYSOP.C_V   1.28   19 Apr 1991 23:38:36   FJM  $
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
 *      doSysop()               handles sysop-only      commands
 *		do_SysopGroup()
 *		do_SysopHall()
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *	History:
 * 01/28/91 FJM 	Improved some of the submenus.
 * 06/07/91 BLJ 	Changed <E>ntering to inlude config.cit
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

char doSysop(void);
void do_SysopGroup(void);
void do_SysopHall(void);
void doAide(char moreYet, char first);

/************************************************************************/
/*      doSysop() handles the sysop-only menu                           */
/*          return FALSE to fall invisibly into default error msg       */
/************************************************************************/
char doSysop(void)
{
    char oldIO;
    int c;
    static int scount = 0;

    oldIO = whichIO;

    /* we want to be in console mode when we go into sysop menu */
    if (!gotCarrier() || !sysop) {
        whichIO = CONSOLE;
        onConsole = (char) (whichIO == CONSOLE);
    }
    sysop = TRUE;

    /* update25();	*/
	do_idle(0);

    while (!ExitToMsdos && (onConsole || gotCarrier())) {
        amZap();

        outFlag = IMPERVIOUS;
        doCR();
        mtPrintf(TERM_REVERSE, "Privileged function:");
        mPrintf(" ");

        dowhat = SYSOPMENU;
        c = iChar();
        dowhat = DUNO;

        switch (toupper(c)) {
			case 0:
                /* We get this when there was a carrier detect while we
                 * are on the console!!  Without this case we have an
                 * endless loop!
                 */

			case 'A':
                mPrintf("\bAbort\n ");
        /* restore old mode */
                whichIO = oldIO;
                sysop = (char) (loggedIn ? logBuf.lbflags.SYSOP : 0);
                onConsole = (char) (whichIO == CONSOLE);
                /* update25();	*/
				do_idle(0);
                return FALSE;
            case 'C':
                mPrintf("\bCron special: ");
                cron_commands();
                break;
            case 'D':
                mPrintf("\bDate change\n ");
                changeDate();
                break;
            case 'E':
				mPrintf("\bEnter ");
                dowhat = SYSOPMENU;
				c = iChar();
				dowhat = DUNO;

				switch (toupper(c)) {
					case 'C':
						mPrintf("\bConfig.cit\n ");
						readconfig(0);
						break;
					case 'E':
						mPrintf("\bExternal.cit\n ");
                        readprotocols();
						break;
                    case 'G':
						mPrintf("\bGrpdata.cit\n ");
                        readaccount();
						break;
					case '?':
						doCR();
						mpPrintf("  <C>onfig.cit\n ");
						mpPrintf(" <E>xternal.cit\n ");
						mpPrintf(" <G>rpdata.cit\n ");
						doCR();
						break;
					default:
						mPrintf("\b\n ");
						break;
				}
				break;
            case 'F':
                doAide(1, 'E');
                break;
            case 'G':
                mPrintf("\bGroup special: ");
                do_SysopGroup();
                break;
            case 'H':
                mPrintf("\bHallway special: ");
                do_SysopHall();
                break;
            case 'K':
                mPrintf("\bKill account\n ");
                killuser();
                break;
            case 'L':
                mPrintf("\bLogin enabled\n ");
                sysopNew = TRUE;
                break;
            case 'M':
                mPrintf("\bMass delete\n ");
                massdelete();
                break;
            case 'N':
                mPrintf("\bNewUser Verification\n ");
                globalverify();
                break;
            case 'O':
                mPrintf("\bOff hook\n ");
                if (!onConsole)
                    break;
                offhook();
                break;
            case 'R':
                mPrintf("\bReset file info\n ");
                if (roomBuf.rbflags.MSDOSDIR != 1) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    updateinfo();
                break;
            case 'S':
                mPrintf("\bShow user\n ");
                showuser();
                break;
            case 'U':
                mPrintf("\bUserlog edit\n ");
                userEdit();
                break;
            case 'V':
                mPrintf("\bView Help Text File\n ");
                nextblurb("sysop", &scount, 1);
                break;
            case 'X':
                mPrintf("\bExit to MS-DOS\n ");
                doCR();
                if (!onConsole)
                    break;
                if (!getYesNo(confirm, 0))
                    break;
                ExitToMsdos = TRUE;
                return FALSE;
            case 'Z':
                mPrintf("\bZap empty rooms\n ");
                killempties();
                break;
            case '!':
                mPrintf("\b ");
                doCR();
                if (!onConsole)
                    break;
                shellescape(0);
				if(cfg->ChatTab)
					readchatbell();
				break;
            case '@':
                mPrintf("\b ");
                doCR();
                if (!onConsole)
                    break;
                shellescape(1);
				if(cfg->ChatTab)
					readchatbell();
                break;
            case '#':
                mPrintf("\bRead by message number\n ");
                readbymsgno();
                break;
            case '*':
                mPrintf("\bUnlink file(s)\n ");
                if (roomBuf.rbflags.MSDOSDIR != 1) {
                    if (expert)
                        mPrintf("? ");
                    else
                        mPrintf("\n Not a directory room.");
                } else
                    sysopunlink();
                break;
            case '?':
                tutorial("sysop.mnu", 1);
                break;

			default:
                if (!expert)
                    mPrintf("\n '?' for menu.\n ");
                else
                    mPrintf(" ?\n ");
                break;
        }
    }
    return FALSE;
}

/************************************************************************/
/*     do_SysopGroup() handles doSysop() Group functions                */
/************************************************************************/
void do_SysopGroup()
{
    switch (toupper(iChar())) {
        case 'G':
            mPrintf("\bGlobal Group membership\n  \n");
            globalgroup();
            break;
        case 'K':
            mPrintf("\bKill group");
            killgroup();
            break;
        case 'N':
            mPrintf("\bNew group");
            newgroup();
            break;
        case 'U':
            mPrintf("\bGlobal user membership\n  \n");
            globaluser();
            break;
        case 'R':
            mPrintf("\bRename group");
            renamegroup();
            break;
        case '?':
            doCR();
            mpPrintf(" <K>ill group\n");
            mpPrintf(" <N>ew group\n");
            mpPrintf(" <G>lobal membership\n");
            mpPrintf(" <U>ser global membership\n");
            doCR();
            mpPrintf(" <R>ename group");
            break;
        default:
            if (!expert)
                mPrintf("\n '?' for menu.\n ");
            else
                mPrintf(" ?\n ");
            break;
    }
}

/************************************************************************/
/*     do_SysopHall() handles the doSysop hall functions                */
/************************************************************************/
void do_SysopHall()
{

    switch (toupper(iChar())) {
        case 'F':
            mPrintf("\bForce access");
            force();
            break;
		case 'I':
			mPrintf("\bInfo on hallway");
			hallinfo();
			break;
		case 'K':
            mPrintf("\bKill hallway");
            killhall();
            break;
        case 'L':
            mPrintf("\bList halls");
            listhalls();
            break;
        case 'N':
            mPrintf("\bNew hall");
            newhall();
            break;
        case 'R':
            mPrintf("\bRename hall");
            renamehall();
            break;
        case 'G':
            mPrintf("\bGlobal Hall func");
            doCR();
            globalhall();
            break;
        case '?':
            doCR();
            mpPrintf(" <F>orce\n");
            mpPrintf(" <G>lobal hall func\n");
            mpPrintf(" <K>ill\n");
            mpPrintf(" <L>ist\n");
            mpPrintf(" <N>ew\n");
            mpPrintf(" <R>ename ");
            break;
        default:
            if (!expert)
                mPrintf("\n '?' for menu.\n ");
            else
                mPrintf(" ?\n ");
            break;
    }
}
