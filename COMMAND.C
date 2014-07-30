/************************************************************************
 *                              command.c
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
 *      mAbort()        		returns TRUE if the user has aborted typeout
 *      exclude()
 *      doHelp()                handles H(elp)          command
 *      doIntro()               handles I(ntro)         command
 *      doKnown()               handles K(nown rooms)   command
 *      doLogin()               handles L(ogin)         command
 *      doLogout()              handles T(erminate)     command
 *      doXpert()
 *      doRegular()             fanout for above commands
 *      doNext()                handles '+' next room
 *      doPrevious()            handles '-' previous room
 *      doNextHall()            handles '>' next room
 *      doPreviousHall()        handles '<' previous room
 *      doAd(void)              show and ad n% of the time.
 *      getCommand()            prints prompt and gets command char
 *      greeting()              System-entry blurb etc
 *      updatebalance()         updates user's accounting balance
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *
 *  History:
 *
 *  06/30/90    FJM     New module.
 *  07/26/90    FJM     Moved mAbort here to save on overlay thrashing
 *  08/07/90    FJM     Improved ad_chance method.
 *  10/15/90    FJM     Added console only protocol flag (field #1)
 *  11/04/90    FJM     Added '\b' alias for '-'
 *  11/23/90    FJM     Cleanup.
 *  11/24/90    FJM     Changes for shell/door mode.
 *  12/08/90    FJM     Made mAbort set normal ANSI attribute when
 *                      message read aborted.
 *  12/08/90    FJM     Changed #EXTERNAL commands so expanded commands
 *                      don't run shell program.
 *  12/09/90    FJM     Pretty prompts.
 *  01/13/91    FJM     Name overflow fixes
 *  01/18/91    FJM     Message for screen pause.
 *  01/19/91    FJM     Added time display.
 *  04/10/91    FJM     Inhibit .RB in anon rooms.
 *  05/15/91    BLJ     Remove '\b' alias for '-' (it's annoying!)
 *	06/13/91	BLJ 	Added .z command (Off-line reader)
 *	08/21/91	BLJ 	Added chat and bell check to getCommand loop
 *	06/21/92	BLJ 	Disable Ads on the console
 *						Add the YAH (You Are Here) code to getCommand loop
 *	08/03/92	BLJ 	Added AutoANSI detect
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doAide(char moreYet, char first);
void doDownload(char ex);
void doUpload(char ex);
void doChat(char moreYet, char first);
void doEnter(char moreYet, char first);
void exclude(void);
void doGoto(char expand, int skip);
void doLogin(char moreYet);
void doRead(char moreYet, char first);
char doRegular(char x, char c);
char doSysop(void);
void do_SysopGroup(void);
void do_SysopHall(void);
char getCommand(char *c);
void greeting(void);
void autoansi(void);
static void doHelp(char expand);
static void doIntro(void);
static void doKnown(char moreYet, char first);
static void doLogout(char expand, char first);
static void doXpert(void);
static void do_chatck(void);
static void youAreHere(void);
static int doPause(void);
static void doNext(void);
static void doPrevious(void);
static void doNextHall(void);
static void doPreviousHall(void);


/* --------------------------------------------------------------------
 * doPause - wait for user to cancel pause.
 * -------------------------------------------------------------------- */

static int doPause(void)
{
    char c, rc;
	char *msg;
	int i;
	int showmsg;

	showmsg = (outFlag == OUTPAUSE);
    outFlag = OUTOK;
	/* show the pause message */
	if (!expert && showmsg) {
		if (aide)
			msg = " <[Enter] J/K/M/N/R/C/~/!/S> ";
		else if ((cfg->kill &&
				(strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
				&& loggedIn))
			msg = " <[Enter], J, K, N, R, or S> ";
		else
			msg = " <[Enter], J, N, R, or S> ";
		mPrintf(msg);
	}
	c = (char) toupper(iChar());    /* wait to resume */
	switch (c) {
		case 'J':       /* jump paragraph: */
			outFlag = OUTPARAGRAPH;
			rc = FALSE;
			break;
		case 'K':       /* kill:          */
			if (aide ||
				(cfg->kill &&
				(strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
				&& loggedIn))
				dotoMessage = PULL_IT;
			rc = FALSE;
			break;
		case 'M':       /* mark:          */
			if (aide)
				dotoMessage = MARK_IT;
			rc = FALSE;
			break;
		case 'N':       /* next:          */
			outFlag = OUTNEXT;
			rc = TRUE;
			break;
		case 'S':       /* skip:          */
			outFlag = OUTSKIP;
			rc = TRUE;
			break;
		case 'R':
			dotoMessage = REVERSE_READ;
			rc = FALSE;
			break;
		case 'C':
			dotoMessage = COPY_IT;
			rc = FALSE;
			break;
		case '~':
			termCap(TERM_NORMAL);
			ansiOn = !ansiOn;
			break;
		case '!':
			IBMOn = !IBMOn;
			break;
		default:
			rc = FALSE;
			break;
	}
	if (!expert && showmsg)
		for (i=strlen(msg); i; --i)
			doBS();
	return rc;
}
/* -------------------------------------------------------------------- */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/* -------------------------------------------------------------------- */
BOOL mAbort(void)
{
    char c, rc, oldEcho, tmpOutFlag;

    /* Check for abort/pause from user */
    if (outFlag == IMPERVIOUS)
        return FALSE;

    if (!BBSCharReady() && outFlag != OUTPAUSE) {
    /* Let modIn() report the problem */
        if (haveCarrier && !gotCarrier())
            iChar();
        rc = FALSE;
    } else {
        oldEcho = echo;
        echo = NEITHER;
        echoChar = 0;

        if (outFlag == OUTPAUSE) {
            c = 'P';
        } else {
            c = (char) toupper(iChar());
        }

        switch (c) {
            case 19:        /* XOFF */
            case 'P':       /* pause:         */
				rc = doPause();
                break;
            case 'C':
                dotoMessage = COPY_IT;
                rc = FALSE;
                break;
            case 'J':       /* jump paragraph: */
                outFlag = OUTPARAGRAPH;
                rc = FALSE;
                break;
            case 'K':       /* kill:          */
                if (aide ||
					(cfg->kill && (strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
                    && loggedIn))
                    dotoMessage = PULL_IT;
                rc = FALSE;
                break;
            case 'M':       /* mark:          */
                if (aide)
                    dotoMessage = MARK_IT;
                rc = FALSE;
                break;
            case 'N':       /* next:          */
                outFlag = OUTNEXT;
                rc = TRUE;
                break;
            case 'S':       /* skip:          */
                outFlag = OUTSKIP;
                rc = TRUE;
                break;
            case 'R':
                dotoMessage = REVERSE_READ;
                rc = FALSE;
                break;
            case '~':
                termCap(TERM_NORMAL);
                ansiOn = !ansiOn;
                break;
            case '!':
                IBMOn = !IBMOn;
                break;
            default:
                rc = FALSE;
                break;
        }
        echo = oldEcho;
    }
    if (rc) {
		tmpOutFlag = outFlag;
		outFlag = OUTOK;
        termCap(TERM_NORMAL);
		outFlag = tmpOutFlag;
	}
    return rc;
}


/************************************************************************/
/*      exclude() handles X>clude room,  toggles the bit                */
/************************************************************************/
void exclude(void)
{
    if  (!logBuf.lbroom[thisRoom].xclude) {
        mPrintf("\n \n Room now excluded from G)oto loop.\n ");
        logBuf.lbroom[thisRoom].xclude = TRUE;
    } else {
        mPrintf("\n \n Room now in G)oto loop.\n ");
        logBuf.lbroom[thisRoom].xclude = FALSE;
    }
}

/************************************************************************/
/*      doHelp() handles H(elp) command                                 */
/************************************************************************/
void doHelp(expand)
char expand;            /* TRUE to accept following parameters  */
{
    label fileName;

    mPrintf("\bHelp File(s)");
    if (!expand) {
        mPrintf("\n\n");
        tutorial("dohelp.hlp", 1);
        return;
    }
    getString("", fileName, 9, 1, ECHO, "");
    normalizeString(fileName);

    if (strlen(fileName) == 0)
        strcpy(fileName, "dohelp");

    if (fileName[0] == '?') {
        tutorial("helpopt.hlp", 1);
    } else {
    /* adding the extention makes things look simpler for           */
    /* the user... and restricts the files which can be read        */
        strcat(fileName, ".hlp");

        tutorial(fileName, 1);
    }
}

/************************************************************************/
/*      doIntro() handles Intro to ....  command.                       */
/************************************************************************/
void doIntro()
{
	mPrintf("\bIntro to %s\n ", cfg->nodeTitle);
    tutorial("intro.hlp", 1);
}

/***********************************************************************/
/*      doKnown() handles K(nown rooms) command.                       */
/***********************************************************************/
void doKnown(moreYet, first)
char moreYet;           /* TRUE to accept following parameters */
char first;         /* first parameter if true             */
{
    char letter;
    char verbose = FALSE;
    char numMess = FALSE;
    char done;

    if (moreYet)
        first = '\0';

    mPrintf("\bKnown ");

    if (first)
        oChar(first);

    do {
        outFlag = IMPERVIOUS;
        done = TRUE;

        letter = (char) (toupper(first ? (char) first : (char) iChar()));
        switch (letter) {
            case 'A':
                mPrintf("\bApplication Rooms ");
                mPrintf("\n ");
                listRooms(APLRMS, verbose, numMess);
                break;
            case 'D':
                mPrintf("\bDirectory Rooms ");
                mPrintf("\n ");
                listRooms(DIRRMS, verbose, numMess);
                break;
            case 'H':
                mPrintf("\bHallways ");
                knownhalls();
                break;
            case 'L':
                mPrintf("\bLimited Access Rooms ");
                mPrintf("\n ");
                listRooms(LIMRMS, verbose, numMess);
                break;
            case 'N':
                mPrintf("\bNew Rooms ");
                mPrintf("\n ");
                listRooms(NEWRMS, verbose, numMess);
                break;
            case 'O':
                mPrintf("\bOld Rooms ");
                mPrintf("\n ");
                listRooms(OLDRMS, verbose, numMess);
                break;
            case 'M':
                mPrintf("\bMail Rooms ");
                mPrintf("\n ");
                listRooms(MAILRM, verbose, numMess);
                break;
            case 'S':
                mPrintf("\bShared Rooms ");
                mPrintf("\n ");
                listRooms(SHRDRM, verbose, numMess);
                break;
            case 'I':
                mPrintf("\bRoom Info");
                mPrintf("\n ");
                RoomStatus();
                break;
            case '\r':
            case '\n':
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'R':
                mPrintf("\bRooms ");
                mPrintf("\n ");
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'V':
                mPrintf("\bVerbose ");
                done = FALSE;
                verbose = TRUE;
                break;
            case 'W':
                mPrintf("\bWindows ");
                mPrintf("\n ");
                listRooms(WINDWS, verbose, numMess);
                break;
            case 'X':
                mPrintf("\bXcluded Rooms ");
                mPrintf("\n ");
                listRooms(XCLRMS, verbose, numMess);
                break;
			case 'Y':
				mPrintf("\banonYmous Rooms ");
                mPrintf("\n ");
				listRooms(ANONRM, verbose, numMess);
                break;
            case '#':
                mPrintf(" of Messages ");
                done = FALSE;
                numMess = TRUE;
                break;
            default:
                mPrintf("? ");
                if (expert)
                    break;
            case '?':
                tutorial("known.mnu", 1);
                break;
        }
    }
    while (!done && moreYet);
}

/************************************************************************/
/*      doLogin() handles L(ogin) command                               */
/************************************************************************/
void doLogin(moreYet)
char moreYet;           /* TRUE to accept following parameters  */
{
    char InitPw[NAMESIZE*2+2];
    char passWord[NAMESIZE*2+2];
    char Initials[NAMESIZE*2+2];
    char *semicolon;

    if (justLostCarrier || ExitToMsdos)
        return;

    if (moreYet == 2)
        moreYet = FALSE;
    else
        mPrintf("\bLogin");

    /* we want to be in console mode when we log in from local */
    if (!gotCarrier() && !loggedIn) {
        whichIO = CONSOLE;
        onConsole = (char) (whichIO == CONSOLE);
        /* update25();	*/
		do_idle(0);
		if (cfg->offhook)
            offhook();
    }
    if (loggedIn) {
        mPrintf("\n Already logged in!\n ");
        return;
    }
    getNormStr((moreYet) ? "" : "your initials", InitPw, NAMESIZE*2+2, NO_ECHO);
    dospCR();

    semicolon = strchr(InitPw, ';');

    if (!semicolon) {
        strncpy(Initials, InitPw,NAMESIZE);
		Initials[NAMESIZE] = '\0';
        getNormStr("password", passWord, NAMESIZE, NO_ECHO);
        dospCR();
    } else
        normalizepw(InitPw, Initials, passWord, semicolon);

    /* don't allow anything over NAMESIZE characters */
    Initials[NAMESIZE] = '\0';

    login(Initials, passWord);
}

/************************************************************************/
/*      doLogout() handles T(erminate) command                          */
/************************************************************************/
void doLogout(expand, first)
char expand;            /* TRUE to accept following parameters  */
char first;         /* first parameter if TRUE              */
{
    char done = FALSE, verbose = FALSE;

    if (expand)
        first = '\0';

    mPrintf("\bTerminate ");

    if (first)
        oChar(first);

    if (first == 'q')
        verbose = 1;

    while (!done) {
        done = TRUE;

        switch (toupper(first ? (int) first : (int) iChar())) {
            case '?':
                mPrintf("\n Logout options:\n \n ");

                mPrintf("Q>uit-also\n ");
                mPrintf("S>tay on line\n ");
                mPrintf("V>erbose\n ");
                mPrintf("? -- this\n ");
                break;
            case 'Y':
            case 'Q':
                mPrintf("\bQuit-also\n ");
                if (!expand) {
                    if (!getYesNo(confirm, 0))
                        break;
                }
                if (!(haveCarrier || onConsole))
                    break;
                terminate( /* hangUp == */ TRUE, verbose);
                break;
            case 'S':
                mPrintf("\bStay\n ");
                if (!(haveCarrier || onConsole))
                    break;
                terminate( /* hangUp == */ FALSE, verbose);
                break;
            case 'V':
                mPrintf("\bVerbose ");
                verbose = 2;
                done = FALSE;
                break;
            default:
                if (expert)
                    mPrintf("? ");
                else
                    mPrintf("? for help");
                break;
        }
        first = '\0';
    }
}

/************************************************************************/
/*      doXpert                                                         */
/************************************************************************/
void doXpert()
{
    mPrintf("\bXpert %s", (expert) ? "Off" : "On");
    doCR();
    expert = (char) (!expert);
}

/************************************************************************/
/*      doRegular()                                                     */
/************************************************************************/
char doRegular(expand, c)
char expand, c;
{
    char toReturn;
    int i;
    int done = 0;
    label doorinfo;

    toReturn = FALSE;

	extCmd = start_extCmd;
	while(!expand && extCmd->name[0]) {
		if (c == toupper(extCmd->name[0]) && (onConsole || !extCmd->local)) {
            done = 1;
			mPrintf("\b%s", extCmd->name);
            doCR();
			if (changedir(cfg->aplpath) == ERROR) {
                mPrintf("  -- Can't find application directory.\n\n");
				changedir(cfg->homepath);
            }
			/* apsystem(extCmd.command); */
			sprintf(doorinfo, "DORINFO%d.DEF", onConsole ? 0 : cfg->mdata);
			extFmtRun(extCmd->command, doorinfo);
        }
		extCmd = extCmd->next;
		if(extCmd == NULL) break;
	}
    if (!done) {
        switch (c) {

            case 'S':
                if (sysop && expand) {
                    mPrintf("\b\bSysop Menu");
                    doCR();
                    doSysop();
                } else {
                    toReturn = TRUE;
                }
                break;

            case 'A':
                if (aide) {
                    doAide(expand, 'E');
                } else {
                    toReturn = TRUE;
                }
                break;

            case 'C':
                doChat(expand, '\0');
                break;
            case 'D':
                doDownload(expand);
                break;
            case 'E':
                doEnter(expand, 'm');
                break;
            case 'F':
                doRead(expand, 'f');
                break;
            case 'G':
                doGoto(expand, FALSE);
                break;
            case 'H':
                doHelp(expand);
                break;
            case 'I':
                doIntro();
                break;
            case 'J':
                mPrintf("\bJump back to ");
                unGotoRoom();
                break;
            case 'K':
                doKnown(expand, 'r');
                break;
            case 'L':
                if (!loggedIn) {
                    doLogin(expand);
                }
                break;
			case 'N':
			case 'O':
            case 'R':
                doRead(expand, tolower(c));
                break;

            case 'B':
                doGoto(expand, TRUE);
                break;
            case 'T':
                doLogout(expand, 'q');
                break;
            case 'U':
                doUpload(expand);
                break;
            case 'X':
                if (!expand) {
                    doEnter(expand, 'x');
                } else {
                    doXpert();
                }
                break;

			case 'Z':
				if (logBuf.READER && expand) {
					mPrintf("\b\bOff-Line Reader");
                    doCR();
					doReader();
                } else {
                    toReturn = TRUE;
                }
                break;

            case '=':
            case '+':
				doNext();
                break;

			case '\b':
				mPrintf("  ");
                break;

			case '-':
				doPrevious();
                break;

			case ']':
            case '>':
                doNextHall();
                break;

            case '[':
            case '<':
				doPreviousHall();
                break;
            case '~':
                mPrintf("\bAnsi %s\n ", ansiOn ? "Off" : "On");
                ansiOn = (char) (!ansiOn);
                break;

            case '!':
                mPrintf("\bIBM Graphics %s\n ", IBMOn ? "Off" : "On");
                IBMOn = (char) (!IBMOn);
                break;

            case '?':
                tutorial("mainopt.mnu", 1);
					/* show external commands      */
				outFlag = OUTOK;
				extCmd = start_extCmd;
				if (extCmd->name[0]) {
					mPrintf("\nExternal Commands:");
                    doCR();
                    doCR();
					while(extCmd) {
						if (onConsole || !extCmd->local) {
							mPrintf("%c> %s", extCmd->name[0], extCmd->name);
							if (extCmd->local)
                                mPrintf(" (local)");
							doCR();
                        }
						extCmd = extCmd->next;
					}
                }
                break;

			case 0:
                if (newCarrier) {
					greeting();

					if (cfg->forcelogin) {
                        doCR();
                        doCR();
                        i = 0;
                        while (!loggedIn && gotCarrier()) {
                            doLogin(2);
                            if (++i > 3) {
                                Initport();
                                toReturn = TRUE;
                                break;
                            }
                        }
                    }
                    newCarrier = FALSE;
                }
                if (logBuf.lbflags.NODE && loggedIn) {
					if(!haveCarrier) break;

					net_slave();

                    haveCarrier = FALSE;
                    modStat = FALSE;
                    newCarrier = FALSE;
                    justLostCarrier = FALSE;
                    onConsole = FALSE;
                    disabled = FALSE;
                    callout = FALSE;

                    delay(2000);

                    Initport();

					cfg->callno++;
                    terminate(FALSE, FALSE);
                }
                if (justLostCarrier || ExitToMsdos) {
					justLostCarrier = FALSE;
                    if (loggedIn)
                        terminate(FALSE, FALSE);
                }
                break;      /* irrelevant value */

            default:
                toReturn = TRUE;
                break;
        }
    }
    /* if they get unverified online */
	if (logBuf.VERIFIED) {
        terminate(FALSE, FALSE);
	}

    /* update25();	*/
	do_idle(0);
    return toReturn;
}

/************************************************************************/
/*	   doReader() handles the '.z' for off-line reading                 */
/************************************************************************/
void doReader(void)
{
	if(onConsole) {
		mPrintf("You MUST be remote for this!");
		doCR();
		return;
	}
	if(getYesNo("Are you ready to receive your rooms",0) == 1)
		net_slave();
}

/************************************************************************/
/*     doNext() handles the '+' for next room                           */
/************************************************************************/
void doNext()
{
    mPrintf("\bEntering Next Room: ");
    stepRoom(1);
}

/************************************************************************/
/*     doPrevious() handles the '-' for previous room                   */
/************************************************************************/
void doPrevious()
{
    mPrintf("\bEntering Previous Room: ");
    stepRoom(0);
}

/************************************************************************/
/*     doNextHall() handles the '>' for next hall                       */
/************************************************************************/
void doNextHall()
{
    mPrintf("\bEntering Next Hall: ");
    stephall(1);
}

/************************************************************************/
/*     doPreviousHall() handles the '<' for previous hall               */
/************************************************************************/
void doPreviousHall()
{
    mPrintf("\bEntering Previous Hall: ");
    stephall(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   this is the (soon to be) world famous YouAreHere!!!
                     version 2.00
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void youAreHere( void )
{
   int i, rLen, tLen, tPos, listCnt, perCol;
   char *tmpRoom;
   typedef struct theListStruct
   {
      char *desc;
      struct theListStruct *next;
   } THELIST;
   THELIST *theHead, *theHeadX, *theHeadY, *theCurr, *theLast;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   initialize some stuff
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   listCnt = 0;
   tmpRoom = (char *)calloc(1,128);
   theHead = theCurr = theLast = (THELIST  *)calloc(1,sizeof( THELIST));
   theCurr->next = NULL;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   setup the Halls heading
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   theCurr->desc = (char *)calloc(1,26);
   strcpy(theCurr->desc,"<----------Halls-------->");
   listCnt++;
   theCurr->next = NULL;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   build list of halls off of the current room
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   for( i=0 ; i<MAXHALLS ; i++ )
      if( ( i == thisHall ) ||
          ( (hallBuf->hall[i].hroomflags[thisRoom].inhall == TRUE)
            && (hallBuf->hall[i].hallname[0] != 0 )
			&& (hallBuf->hall[i].hroomflags[thisRoom].window)
			&& groupseeshall(i) )
        )
      {
		 theLast = theCurr;
		 theCurr = theCurr->next;
		 theCurr = (THELIST *)calloc(1,sizeof(THELIST));
         theCurr->next = NULL;
		 theLast->next = theCurr;
         theCurr->desc = (char *)calloc(1,strlen(hallBuf->hall[i].hallname)+3);
         strcpy(theCurr->desc, (i<thisHall) ? "< " : (i==thisHall) ? "* " : "> " );
         strncat(theCurr->desc, strip_ansi(hallBuf->hall[i].hallname), 23 );
         listCnt++;
      }
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   setup the Rooms heading
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   theLast = theCurr;
   theCurr = theCurr->next;
   theCurr = (THELIST *)calloc(1,sizeof(THELIST));
   theCurr->next = NULL;
   theLast->next = theCurr;
   theCurr->desc = (char *)calloc(1,26);
   strcpy(theCurr->desc,"<----------Rooms-------->");
   listCnt++;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
   build list of rooms that the user can see off of the current Hall
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   for( i=0 ; i<MAXROOMS ; i++ )
      if( hallBuf->hall[thisHall].hroomflags[i].inhall == TRUE )
		 if( canseeroom(i) && roomTab[i].rtgen == logBuf.lbroom[i].lbgen )
         {
            fmtrm( i, tmpRoom );
            strcpy( tmpRoom, strip_ansi(tmpRoom) );
            tLen=strlen(tmpRoom); 
            if( tLen > 23 )
            {
			   rLen=strlen(tmpRoom);
               tPos = tLen - ((rLen>23) ? 23 : rLen);
               memcpy( tmpRoom+(rLen-tPos), tmpRoom+(rLen), (tPos+1) );
            }
			theLast = theCurr;
			theCurr = theCurr->next;
            theCurr = (THELIST *)calloc(1,sizeof(THELIST));
            theCurr->next = NULL;
			theLast->next = theCurr;
            theCurr->desc = (char *)calloc(1,strlen(tmpRoom)+3);
            strcpy(theCurr->desc, (i<thisRoom) ? "- " : (i==thisRoom) ? "* " : "+ " );
            strncat(theCurr->desc, tmpRoom, 23 );
            listCnt++;
         }
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - * 
   a Sysop configurable param. they are the minimum
   number of entries per row that will be displayed
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   perCol = (listCnt / 3) + 1;
   if( perCol < cfg->MenuLines ) perCol = cfg->MenuLines;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - * 
   theHeadX is used to point to the first line to be displayed on row two
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   theHeadX = theHead;
   for( i=0 ; i<perCol ; i++ )
      if( theHeadX != NULL )
         theHeadX = theHeadX->next;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - * 
   theHeadY is used to point to the first line to be displayed on row three
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   theHeadY = theHeadX;
   for( i=0 ; i<perCol ; i++ )
      if( theHeadY != NULL )
         theHeadY = theHeadY->next;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - * 
   here's where we display the YAH menu. we also Free used space after each
   mPrintf. Hey!, i actually used doCR(). gee, this works much better.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   doCR(); doCR();

   theCurr = theHead;

   for( i=0 ; i<perCol ; i++ )
   {
	  if( theCurr != NULL )
      {
		 mPrintf( "%-25.25s ", theCurr->desc );
		 theCurr = theCurr->next;
      }
      if( theHeadX != NULL ) 
      {
         mPrintf( "%-25.25s ", theHeadX->desc );
		 theHeadX = theHeadX->next;
      }
      if( theHeadY != NULL ) 
      {
         mPrintf( "%-25.25s ", theHeadY->desc );
		 theHeadY = theHeadY->next;
      }

      doCR();
   }


   while(theHead) {
	  theCurr = theHead; theHead = theHead->next;
	  free( theCurr->desc ) ; free( theCurr );
   }
   free (tmpRoom);	/* 128 Bytes */
}

/************************************************************************
 *
 * doAd() shows an ad/blurb cfg->ad_chance % of the time
 *
 ************************************************************************/

void doAd(int force)
{
    static int count = 0;

	if ((cfg->ad_chance && (random(100) + 1 <= cfg->ad_chance)) || force) {
		if(!loggedIn && cfg->forcelogin) return;
		nextblurb("ad", &count, 0);
	}
}

/************************************************************************
 *
 * do_chatck() checks for chat on or off according to time
 *
 ************************************************************************/

void do_chatck()
{
	if(!chat_bell->days[dayofweek()].hours[((hour()*4)+quarter())]) {
		cfg->noChat = TRUE;
		cfg->noBells = TRUE;
		return;
	}
	else {
		if(loggedIn && logBuf.NOCHAT) return;
		cfg->noChat = FALSE;
		cfg->noBells = FALSE;
		return;
	}
}

/************************************************************************/
/*      getCommand() prints menu prompt and gets command char           */
/*      Returns: char via parameter and expand flag as value  --        */
/*               i.e., TRUE if parameters follow else FALSE.            */
/************************************************************************/
char getCommand(char *c)
{
    char expand;
	char *s, *coms, *tzone, newtz[10], setverb[20];

    outFlag = IMPERVIOUS;

    /* update user's balance */
	if (cfg->accounting && !logBuf.lbflags.NOACCOUNT)
        updatebalance();

	if((!onConsole) || (onConsole && !expert)) doAd(0);

	if(cfg->ChatTab)
		do_chatck();

	if(cfg->MenuOk && !expert) {
		outFlag = OUTOK;
		youAreHere();
		outFlag = IMPERVIOUS;
	}

	givePrompt();

    do {
        dowhat = MAINMENU;
        *c = (char) toupper(iChar());
        dowhat = DUNO;
		if(chkdate()) {
			tzone = getenv("TZ");
			coms = getenv("COMSPEC");
			strcpy(newtz,tzone);
			if(strlen(newtz) == 4) {
				newtz[4] = newtz[0];
				newtz[5] = 'D';
				newtz[6] = newtz[2];
				newtz[7] = '\0';
			}
			else newtz[4] = '\0';

			s = strstr(coms,"4DOS");

			if(s) {
				sprintf(setverb,"SET /M TZ=%s",newtz);
				system(setverb);
			}
			else {
				sprintf(setverb,"TZ=%s",newtz);
				putenv(setverb);
			}
			/* And now the Aide message */
			if(strlen(newtz) == 4)
				strcpy(setverb,"Standard");
			else
				strcpy(setverb,"Daylight Savings");

			sprintf(msgBuf->mbtext,
			"%s time is now in effect. Please edit AUTOEXEC.BAT => "
			"\"TZ=%s\" and reboot your computer",
			setverb,newtz);

			aideMessage();

		}
	} while (*c == 'P');

    expand = (char)
    ((*c == ' ') || (*c == '.') || (*c == ',') || (*c == '/'));

    if (expand) {
        *c = (char) toupper(iChar());
    }
    if (justLostCarrier || ExitToMsdos) {
        justLostCarrier = FALSE;
        if (loggedIn)
            terminate(FALSE, FALSE);
    }
    return expand;
}

/************************************************************************/
/*      greeting() gives system-entry blurb etc                         */
/************************************************************************/
void greeting()
{
    int messages;
    char dtstr[80];

	if (loggedIn) {
        terminate(FALSE, FALSE);
	}
    echo = BOTH;

    setdefaultconfig();
    initroomgen();
    cleargroupgen();
	if (cfg->accounting)
        unlogthisAccount();

    delay(100);

	if (newCarrier || debug) {
		if(newCarrier) autoansi();
        hello();
	}

	newCarrier = FALSE;

	if(cfg->SysOpRegion[0]) {
		strcpy(dtstr,cfg->SysOpRegion);
		if(cfg->SysOpCntry[0]) {
			strcat(dtstr,", ");
			strcat(dtstr,cfg->SysOpCntry);
		}
	}
	else {
		strcpy(dtstr, cfg->nodeRegion);
	}
	mPrintf("\n Welcome to %s, %s", cfg->nodeTitle, dtstr);
    mPrintf("\n Running %s v%s", softname, version);

	if(release == 'A')
		mPrintf("\n Alpha Test Site");

	if(release == 'B')
		mPrintf("\n Beta Test Site");

	doCR();
    doCR();

	cit_strftime(dtstr, 79, cfg->vdatestamp, 0L);
    mPrintf(" %s", dtstr);

	if (!cfg->forcelogin) {
        mPrintf("\n H for Help");
        mPrintf("\n ? for Menu");
        mPrintf("\n L to Login");
    }
    getRoom(LOBBY);

	messages = talleyBuf.room[thisRoom].messages;

    doCR();

    mPrintf("  %d %s ", messages,
    (messages == 1) ? "message" : "messages");

    doCR();

    while (MIReady())
        getMod();

}

/* -------------------------------------------------------------------- */
/*  autoansi()  this sets term at according to cfg.autoansi or does     */
/*              actual auto ansi detect.                                */
/* -------------------------------------------------------------------- */
void autoansi(void)
{
     char x;
     char inch;
     char detected = 0;
     long t;

	 Mflush();

	 delay(200);

	 for (x = 0; x < 3 && !MIReady(); x++)
	 {
		 outstring("[6n");
		 delay(100);
	 }

	 if (MIReady())
	 {
		 inch = (char)getMod();

		 if (inch == 27)
		 {
			 detected = TRUE;
			 mPrintf(" Ansi Detected . . .");
			 ansiOn = TRUE;
			 IBMOn = TRUE;
			 doCR();
			 doCR();
		 }
	 }

	 if (!detected)
	 {
		 mPrintf(" Ansi NOT Detected . . .");
		 ansiOn = FALSE;
		 IBMOn = FALSE;
		 doCR();
		 doCR();
	 }
     Mflush();
}

/************************************************************************
 *
 *      updatebalance()  updates user's accounting balance
 *      This routine will warn the user of excessive use, and terminate
 *      user when warnings have run out
 *
 ************************************************************************/

void updatebalance(void)
{
    double drain;
    long timestamp, diff;
    static int gcount = 0;

    if (thisAccount.special[hour()] && !specialTime) {
        specialTime = TRUE;
        doCR();
        if (loggedIn) {
            mPrintf("Free time is now beginning, you have no time limit.");
            doCR();
        }
    }
    /* if it's no longer special time....                     */
    if (specialTime && !thisAccount.special[hour()]) {

        doCR();

        if (loggedIn) {
            mPrintf("Free time is over. You have %.0f %s%s left today.",
			logBuf.credits, cfg->credit_name, (logBuf.credits == 1) ? "" : "s");
            doCR();
        }
        specialTime = FALSE;

        time(&lasttime);
    }
    if (specialTime)        /* don't charge them for FREE time!             */
        return;


    /* get current time stamp */
    time(&timestamp);

    diff = timestamp - lasttime;

    /* If the time was set wrong..... */
    if (diff < 0)
        diff = 0;

    drain = (double) diff / 60.0;

    logBuf.credits = logBuf.credits - (float) drain;

    time(&lasttime);

    if (!gotCarrier() || onConsole)
        return;

    if (logBuf.credits < 5.0) {
        doCR();
		mPrintf(" Only %.0f %s%s left today!", logBuf.credits, cfg->credit_name,
        ((int) logBuf.credits == 1) ? "" : "s");
        doCR();
    }
    if (!thisAccount.days[dayofweek()]  /* if times up of it's no  */
        ||!thisAccount.hours[hour()]    /* login time for them..   */
    ||logBuf.credits <= 0.0) {
        nextblurb("goodbye", &gcount, 1);
        terminate(TRUE, 1);
    }
}

/* EOF */
