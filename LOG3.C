/* -------------------------------------------------------------------- */
/*  LOG3.C                   Citadel                                    */
/* -------------------------------------------------------------------- */
/*                     Overlayed newuser log code                       */
/*                  and configuration / userlog edit                    */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  initroomgen()   initializes room gen# with log gen                  */
/*  newlog()        sets up a new log entry for new users returns ERROR */
/*                  if cannot find a usable slot                        */
/*  newslot()       attempts to find a slot for a new user to reside in */
/*                  puts slot in global var  thisSlot                   */
/*  newUser()       prompts for name and password                       */
/*  newUserFile()   Writes new user info out to a file                  */
/*  configure()     sets user configuration via menu                    */
/*  userEdit()      Edit a user via menu                                */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  06/14/89  (PAT)  Created from LOG.C to move some of the system
 *                   out of memory. Also cleaned up moved code to
 *                   -W3, ext.
 *  03/07/90  {zm}   "Enter your 'nym" instead of "Enter full name"
 *                   and clean up some other nuisances in #PRIVATE 4
 *  03/15/90  {zm}   Add [title] name [surname] everywhere.
 *  06/06/90  FJM    Changed strftime to cit_strftime
 *  06/16/90  FJM    Fixes to allow entry of 30 char nym & initials.
 *  06/16/90  FJM    Made IBM Graphics characters a seperate option.
 *  08/07/90  FJM    Made password blurb rotate.
 *  10/05/90  FJM    Made inits/password show in userlog edit (console only).
 *	11/24/90  FJM	 Changes for shell/door mode.
 *	01/18/91  FJM	 Improved .AL
 *	06/12/91  BLJ	 Added code for new user bit flags (nochat, trapfile)
 *	06/29/91  BLJ	 Added code for sound bit flag
 *	08/03/91  BLJ	 Added prompt for backing up last visit pointer
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  initroomgen()   initializes room gen# with log gen                  */
/* -------------------------------------------------------------------- */
void initroomgen(void)
{
	uchar i;

    for (i = 0; i < MAXROOMS; i++) {
    /* Clear mail and xclude flags in logbuff for every room */

        logBuf.lbroom[i].mail = FALSE;
        logBuf.lbroom[i].xclude = FALSE;

        if (roomTab[i].rtflags.PUBLIC == 1) {
        /* make public rooms known: */
            logBuf.lbroom[i].lbgen = roomTab[i].rtgen;
            logBuf.lbroom[i].lvisit = MAXVISIT - 1;

        } else {
        /* make private rooms unknown: */
            logBuf.lbroom[i].lbgen =
            (roomTab[i].rtgen + (MAXGEN - 1)) % MAXGEN;

            logBuf.lbroom[i].lvisit = MAXVISIT - 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  newlog()        sets up a new log entry for new users,              */
/*                  returns ERROR if cannot find a usable slot          */
/* -------------------------------------------------------------------- */
int newlog(char *fullnm, char *in, char *pw)
{
    int ourSlot, i;

    /* get a new slot for this user */
    thisSlot = newslot();

    if (thisSlot == ERROR) {
        thisSlot = 0;
        return (ERROR);
    }
    ourSlot = logTab[thisSlot].ltlogSlot;

    getLog(&logBuf, ourSlot);

    /* copy info into record: */
    strcpy(logBuf.lbname, fullnm);
    strcpy(logBuf.lbin, in);
    strcpy(logBuf.lbpw, pw);
    logBuf.title[0] = '\0'; /* no starting title */
    logBuf.surname[0] = '\0';   /* no starting surname */
    logBuf.forward[0] = '\0';   /* no starting forwarding */
	if (ansiOn)
		strcpy(logBuf.tty, "ANSI_BBS");
	else
		strcpy(logBuf.tty, "TTY");

    logBuf.lbflags.L_INUSE = TRUE;
	logBuf.lbflags.PROBLEM = cfg->user[D_PROBLEM];
	logBuf.lbflags.PERMANENT = cfg->user[D_PERMANENT];
	logBuf.lbflags.NOACCOUNT = cfg->user[D_NOACCOUNT];
	logBuf.lbflags.NETUSER = cfg->user[D_NETWORK];
	logBuf.lbflags.NOMAIL = cfg->user[D_NOMAIL];
    logBuf.lbflags.AIDE = FALSE;
    logBuf.lbflags.NODE = FALSE;
    logBuf.lbflags.SYSOP = FALSE;

    logBuf.DUNGEONED = FALSE;
    logBuf.MSGAIDE = FALSE;
    logBuf.FORtOnODE = FALSE;
    logBuf.NEXTHALL = FALSE;
    logBuf.VERIFIED = FALSE;
	logBuf.NOCHAT = FALSE;
	logBuf.TRAPIT = FALSE;
	logBuf.READER = FALSE;

    for (i = 1; i < MAXVISIT; i++)
		logBuf.lbvisit[i] = cfg->oldest;

	logBuf.lbvisit[0] = cfg->newest;
	logBuf.lbvisit[(MAXVISIT - 1)] = cfg->oldest;

    initroomgen();

    cleargroupgen();

    /* put user into group NULL */
	logBuf.groups[0] = grpBuf->group[0].groupgen;

	/*	AND any default groups	*/
	for(i=2; i<MAXGROUPS; i++) {
		if(grpBuf->group[i].g_inuse && grpBuf->group[i].dfault)
			logBuf.groups[i] = grpBuf->group[i].groupgen;
	}

	/* accurate read-userlog for first time call */
	logBuf.callno = cfg->callno + 1;
    logBuf.credits = (float) 0;
    time(&logBuf.calltime);

    /* trap it */
    sprintf(msgBuf->mbtext, "New user %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_LOGIN);

    loggedIn = TRUE;
    slideLTab(thisSlot);
    storeLog();

    return (TRUE);
}

/* -------------------------------------------------------------------- */
/*  newslot()       attempts to find a slot for a new user to reside in */
/*                  puts slot in global var  thisSlot                   */
/* -------------------------------------------------------------------- */
int newslot(void)
{
    int i;
    int foundit = ERROR;

	for (i = cfg->MAXLOGTAB - 1; ((i > -1) && (foundit == ERROR)); --i) {
        if (!logTab[i].permanent)
            foundit = i;
    }
    if (foundit == ERROR) {
        mPrintf("\n All log slots taken.\n");
    }
    return foundit;
}

/* -------------------------------------------------------------------- */
/*  newUser()       prompts for name and password                       */
/* -------------------------------------------------------------------- */
void newUser(char *initials, char *password)
{
    label fullnm;
    char InitPw[NAMESIZE*2+2];
    char Initials[NAMESIZE*2+2];
    char passWord[NAMESIZE*2+2];
    char *semicolon;
    static int pcount = 0;

	int i, l, abort, good = 0;
    char firstime = 1;

    if (justLostCarrier || ExitToMsdos)
        return;

	unlisted = FALSE;		  /* default [N] for list in userlog		   */
	roomtell = TRUE;		  /* default [Y] for display of room desc	   */
	logBuf.linesScreen = 23;  /* 23 lines before pause					   */
	logBuf.SOUND = FALSE;	  /* default No Sound						   */

	configure(TRUE);	  /* make sure new users configure reasonably	   */

    nextblurb("password", &pcount, 1);

    do {
        do {
            getNormStr("your 'nym", fullnm, NAMESIZE, ECHO);    /* 03/07/90 */

            if (!strlen(fullnm)) {
                mPrintf("Blank names are not allowed\n");
                good = FALSE;
            } else if ((personexists(fullnm) != ERROR)
				|| (strcmpi(fullnm, "Sysop") == SAMESTRING)) {
                mPrintf("We already have a %s, try again\n", fullnm);
                good = FALSE;
			} else {
				good = TRUE;
                l = strlen(fullnm);
				for(i=0; i<l; i++) {
					if(fullnm[i] < 32 || fullnm[i] > 126) {
						good = FALSE;
						mPrintf("Normal characters only, try again\n");
						break;
					}
				}
			}
        } while (!good && (!ExitToMsdos && (haveCarrier || whichIO == CONSOLE)));

        if (firstime)
            strcpy(Initials, initials);
        else {
            getNormStr("your initials", InitPw, NAMESIZE*2+2, NO_ECHO);
            dospCR();

            semicolon = strchr(InitPw, ';');

            if (semicolon) {
                normalizepw(InitPw, Initials, passWord, semicolon);
            } else
                strncpy(Initials, InitPw, NAMESIZE);
			Initials[NAMESIZE] = '\0';
        }

        do {
            if (firstime)
                strcpy(passWord, password);
            else if (!semicolon) {
                getNormStr("password", passWord, NAMESIZE, NO_ECHO);
                dospCR();
            }
            firstime = FALSE;   /* keeps from going in infinite loop */
            semicolon = FALSE;

            if (pwexists(passWord) != ERROR || strlen(passWord) < 2) {
                good = FALSE;
                mPrintf("\n Poor password\n ");
            } else
                good = TRUE;
        } while (!good && (!ExitToMsdos && (haveCarrier || whichIO == CONSOLE)));

        displaypw(fullnm, Initials, passWord);

        abort = getYesNo("OK", 2);

        if (abort == 2)
            return;     /* check for Abort at (Y/N/A)[A]: */
    } while ((!abort) && (!ExitToMsdos && (haveCarrier || whichIO == CONSOLE)));

    if (!ExitToMsdos && (haveCarrier || whichIO == CONSOLE))
        if (newlog(fullnm, Initials, passWord) == ERROR)
            return;
}

/* -------------------------------------------------------------------- */
/*  newUserFile()   Writes new user info out to a file                  */
/*  as used by #PRIVATE 3 or 4, the most common selections.             */
/* -------------------------------------------------------------------- */
void newUserFile(void)
{
    FILE *fl;
    char name[40];
    char phone[30];
    char title[31];
    char surname[31];
    char temp[60];
    char dtstr[80];
    int tempmaxtext;
    int clm = 0;
    int l = 0;

    *name = '\0';
    *phone = '\0';
    *surname = '\0';

	if (cfg->surnames) {
        getNormStr("the title you desire", title, 30, ECHO);
        getNormStr("the surname you desire", surname, 30, ECHO);
    }
    getNormStr("your REAL name", name, 40, ECHO);

    if (name[0])
        getNormStr("your phone number [(xxx)xxx-xxxx]", phone, 30, ECHO);

	strcpy(msgBuf->mbto, cfg->sysop);
    strcpy(msgBuf->mbauth, logBuf.lbname);
    msgBuf->mbtext[0] = 0;
    msgBuf->mbtitle[0] = EOS;
    msgBuf->mbsur[0] = EOS;
	tempmaxtext = cfg->maxtext;
	cfg->maxtext = 2048;  /* often had problems with 1024 being too small! */

    getText();

	cfg->maxtext = tempmaxtext;

	if (changedir(cfg->homepath) == ERROR)
        return;

    fl = fopen("newuser.log", "at");
	cit_strftime(dtstr, 79, cfg->vdatestamp, 0L);

    sprintf(temp, "\n %s\n", dtstr);
    fwrite(temp, strlen(temp), 1, fl);

    if (surname[0]) {       /* argh... */
        sprintf(temp, " Nym:       [%s] %s\n", surname, logBuf.lbname);
    } else {
        sprintf(temp, " Nym:       %s\n", logBuf.lbname);
    }
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Real name: %s\n", name);
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Phone:     %s\n", phone);
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, " Baud:      %d\n", bauds[speed]);
    fwrite(temp, strlen(temp), 1, fl);

    sprintf(temp, "\n");

    if (msgBuf->mbtext[0]) {    /* xPutStr(fl, msgBuf->mbtext); */
        do {
            if ((msgBuf->mbtext[l] == 32 || msgBuf->mbtext[l] == 9) && clm > 73) {
                fwrite(temp, strlen(temp), 1, fl);
                clm = 0;
                l++;
            } else {
                fputc(msgBuf->mbtext[l], fl);
                clm++;
                if (msgBuf->mbtext[l] == 10)
                    clm = 0;
                if (msgBuf->mbtext[l] == 9)
                    clm = clm + 7;
                l++;
            }
        } while (msgBuf->mbtext[l]);
    }
    fclose(fl);
    doCR();
}

/* -------------------------------------------------------------------- */
/*  configure()     sets user configuration via menu                    */
/* -------------------------------------------------------------------- */
void configure(BOOL new)
{
    BOOL prtMess = TRUE;
    BOOL quit = FALSE;
    int c;
    char temp[30];
    char oldEcho;

    doCR();

	do {
        if (prtMess) {
            doCR();
            outFlag = OUTOK;
            mpPrintf("Screen <W>idth....... %d", termWidth);
            doCR();
			mpPrintf("Height of <S>creen... %s", linesScreen
			? itoa(linesScreen, temp, 10) :
            "Screen Pause Off");
            doCR();
            mpPrintf("<U>ppercase only..... %s", termUpper ? "On" : "Off");
            doCR();
            mpPrintf("<L>inefeeds.......... %s", termLF ? "On" : "Off");
            doCR();
            mpPrintf("<T>abs............... %s", termTab ? "On" : "Off");
            doCR();
            mpPrintf("<N>ulls.............. %s", termNulls
            ? itoa(termNulls, temp, 10) :
            "Off");
            doCR();
            mpPrintf("Terminal <E>mulation. %s", ansiOn ? "ANSI-BBS" : "Off");
            doCR();
            mpPrintf("IBM <G>raphics....... %s", IBMOn ? "On" : "Off");
            doCR();
            mpPrintf("<H>elpful Hints...... %s", !expert ? "On" : "Off");
            doCR();
            mpPrintf("l<I>st in userlog.... %s", !unlisted ? "Yes" : "No");
            doCR();
            mpPrintf("Last <O>ld on New.... %s", oldToo ? "On" : "Off");
            doCR();
            mpPrintf("<R>oom descriptions.. %s", roomtell ? "On" : "Off");
            doCR();
            mpPrintf("<A>uto-next hall..... %s", logBuf.NEXTHALL ? "On" : "Off");
            doCR();
			mpPrintf("have soun<D>......... %s", Sound ? "On" : "Off");
            doCR();

            doCR();
            mpPrintf("<Q> to save and quit.");
            doCR();
            prtMess = (BOOL) (!expert);
        }
        if (new) {
            if (getYesNo("Is this OK", 1)) {
                quit = TRUE;
                continue;
            }
            new = FALSE;
        }
        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("Change: ");

        oldEcho = echo;
        echo = NEITHER;
        c = iChar();
        echo = oldEcho;

        if (!(onConsole || gotCarrier()))
            return;

        switch (toupper(c)) {
            case 'W':
                mPrintf("Screen Width");
                doCR();
                termWidth =
				(uchar) getNumber("Screen width", 10L, 255L, (long) termWidth);
        /* kludge for carr-loss */
                if (termWidth < 10)
					termWidth = cfg->width;
                break;

            case 'S':
				if (!linesScreen) {
                    mPrintf("Pause on full screen");
                    doCR();
					linesScreen =
                    (uchar) getNumber("Lines per screen", 10L, 80L, 21L);
                } else {
                    mPrintf("Pause on full screen off");
                    doCR();
					linesScreen = 0;
                }
                break;

            case 'U':
                termUpper = (BOOL) (!termUpper);
                mPrintf("Uppercase only %s", termUpper ? "On" : "Off");
                doCR();
                break;

            case 'L':
                termLF = (BOOL) (!termLF);
                mPrintf("Linefeeds %s", termLF ? "On" : "Off");
                doCR();
                break;

            case 'T':
                termTab = (BOOL) (!termTab);
                mPrintf("Tabs %s", termTab ? "On" : "Off");
                doCR();
                break;

            case 'N':
                if (!termNulls) {
                    mPrintf("Nulls");
                    doCR();
                    termNulls = (uchar) getNumber("number of Nulls", 0L, 255L, 5L);
                } else {
                    mPrintf("Nulls off");
                    doCR();
                    termNulls = 0;
                }
                break;

            case 'E':
                ansiOn = (BOOL) (!ansiOn);
                mPrintf("Terminal Emulation %s", ansiOn ? "On" : "Off");
                doCR();
                break;

            case 'G':
                IBMOn = !IBMOn;
                mPrintf("IBM Character Graphics %s", IBMOn ? "On" : "Off");
                doCR();
                break;

            case 'H':
                expert = (BOOL) (!expert);
                mPrintf("Helpful Hints %s", !expert ? "On" : "Off");
                doCR();
                break;

            case 'I':
                unlisted = (BOOL) (!unlisted);
                mPrintf("List in userlog %s", !unlisted ? "Yes" : "No");
                doCR();
                break;

            case 'O':
                oldToo = (BOOL) (!oldToo);
                mPrintf("Last Old on New %s", oldToo ? "On" : "Off");
                doCR();
                break;

            case 'R':
                roomtell = (BOOL) (!roomtell);
                mPrintf("Room descriptions %s", roomtell ? "On" : "Off");
                doCR();
                break;

            case 'A':
                logBuf.NEXTHALL = (BOOL) (!logBuf.NEXTHALL);
                mPrintf("Auto-next hall %s", logBuf.NEXTHALL ? "On" : "Off");
                doCR();
                break;

			case 'D':
				Sound = (BOOL) (!Sound);
				mPrintf("Have Sound %s", Sound ? "On" : "Off");
                doCR();
                break;

            case 'Q':
                mPrintf("Save changes");
                doCR();
                quit = TRUE;
                break;

            case '\r':
            case '\n':
            case '?':
                mPrintf("Menu");
                doCR();
                prtMess = TRUE;
                break;

            default:
                mPrintf("%c ? for help", c);
                doCR();
                break;
        }

    } while (!quit);
}

/* -------------------------------------------------------------------- */
/*  userEdit()      Edit a user via menu                                */
/* -------------------------------------------------------------------- */
void userEdit(void)
{
    label who;

    getNormStr("who", who, NAMESIZE, ECHO);
	userEdit2(who);
}

void userEdit2(char *who)
{
    BOOL prtMess = TRUE;
    BOOL quit = FALSE;
	int c, num_visit;
    char string[200], temp[80];
    char oldEcho;
    int logNo, ltabSlot, tsys;
    BOOL editSelf = FALSE;
	
    logNo = findPerson(who, &lBuf);
    ltabSlot = personexists(who);

    if (!strlen(who) || logNo == ERROR) {
        mPrintf("No \'%s\' known. \n ", who);
		return;
    }
    /* make sure we use current info */
    if (strcmpi(who, logBuf.lbname) == SAMESTRING) {
        tsys = logBuf.lbflags.SYSOP;
        setlogconfig();     /* update current user */
        logBuf.lbflags.SYSOP = tsys;
        lBuf = logBuf;      /* use their online logbuffer */
        editSelf = TRUE;
    }
    doCR();

    do {
        if (prtMess) {
            doCR();
            outFlag = OUTOK;
            mpPrintf("<N>ame............... %s", lBuf.lbname);
            doCR();
            cPrintf("Initials............. %s", lBuf.lbin);
            doCR();
            cPrintf("Password............. %s", lBuf.lbpw);
            doCR();
            mpPrintf("<Z> (Title).......... %s", lBuf.title);
            doCR();
			mpPrintf("    <X> (Locked)..... %s", lBuf.TITLELOK ? "Yes" : "No");
            doCR();
            mpPrintf("s<U>rname............ %s", lBuf.surname);
            doCR();
            mpPrintf("    <L>ocked......... %s", lBuf.SURNAMLOK ? "Yes" : "No");
            doCR();
            mpPrintf("<S>ysop.............. %s", lBuf.lbflags.SYSOP
            ? "Yes" : "No");

			while(crtColumn < 40) oChar(' ');

			mpPrintf("<P>roblem user....... %s", lBuf.lbflags.PROBLEM
            ? "Yes" : "No");
            doCR();
            mpPrintf("<A>ide............... %s", lBuf.lbflags.AIDE
            ? "Yes" : "No");

			while(crtColumn < 40) oChar(' ');

			mpPrintf("no <C>hat............ %s", lBuf.NOCHAT
            ? "Yes" : "No");
            doCR();
            mpPrintf("n<O>de............... %s", lBuf.lbflags.NODE
            ? "Yes" : "No");

			while(crtColumn < 40) oChar(' ');

			mpPrintf("trap to <F>ile....... %s", lBuf.TRAPIT
            ? "Yes" : "No");
            doCR();
            mpPrintf("p<E>rmanent.......... %s", lBuf.lbflags.PERMANENT
            ? "Yes" : "No");

			while(crtColumn < 40) oChar(' ');

			mpPrintf("no <M>ail............ %s", lBuf.lbflags.NOMAIL
            ? "Yes" : "No");
            doCR();
            mpPrintf("ne<T>user............ %s", lBuf.lbflags.NETUSER
            ? "Yes" : "No");

            while(crtColumn < 40) oChar(' ');

            mpPrintf("<V>erified........... %s", !lBuf.VERIFIED
            ? "Yes" : "No");
            doCR();
            mpPrintf("<H>ave sound......... %s", lBuf.SOUND
            ? "Yes" : "No");

            while(crtColumn < 40) oChar(' ');

			mpPrintf("<D>ungeoned.......... %s", lBuf.DUNGEONED
			? "Yes" : "No");
            doCR();
			mpPrintf("off-line <R>eader.... %s", lBuf.READER
            ? "Yes" : "No");

            while(crtColumn < 40) oChar(' ');

			mpPrintf("<K>eep in room/hall.. %s", lBuf.STICKYRM
			? "Yes" : "No");
            doCR();
			if (cfg->accounting) {
                mpPrintf("m<I>nutes............ ");

                if (lBuf.lbflags.NOACCOUNT)
                    mPrintf("N/A");
                else
                    mPrintf("%.0f", lBuf.credits);

            }


            while(crtColumn < 40) oChar(' ');

			mpPrintf("<B>ack up pointers... ");
            doCR();
			doCR();
            doCR();
            mpPrintf("<Q> to save and quit.");
            doCR();
            prtMess = (BOOL) (!expert);
        }
        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("Change: ");

        oldEcho = echo;
        echo = NEITHER;
        c = iChar();
        echo = oldEcho;

        if (!(onConsole || gotCarrier()))
            return;

        switch (toupper(c)) {
            case 'N':
                mPrintf("Name");
                doCR();
                strcpy(temp, lBuf.lbname);
                getString("New name", lBuf.lbname, 30, FALSE, ECHO, temp);
                normalizeString(lBuf.lbname);
                if (!strlen(lBuf.lbname))
                    strcpy(lBuf.lbname, temp);
                break;

            case 'Z':
                mPrintf("Title");
                doCR();
				strcpy(temp, lBuf.title);
				getString("New title", lBuf.title, 30, FALSE, ECHO, temp);
				normalizeString(lBuf.title);
                break;

			case 'X':
				lBuf.TITLELOK = (BOOL) (!lBuf.TITLELOK);
				mPrintf("Title Locked %s", lBuf.TITLELOK ? "Yes" : "No");
                doCR();
                break;

            case 'U':
                mPrintf("sUrname");
                doCR();
                if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf) {
                    doCR();
                    mPrintf("User has locked their surname!");
                    doCR();
                } else {
                    strcpy(temp, lBuf.surname);
					getString("New surname", lBuf.surname, 30, FALSE, ECHO, temp);
                    normalizeString(lBuf.surname);
                }
                break;

            case 'L':
                if (!(lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)) {
                    lBuf.SURNAMLOK = (BOOL) (!lBuf.SURNAMLOK);
                }
                mPrintf("Surname Locked %s", lBuf.SURNAMLOK ? "Yes" : "No");
                doCR();
                if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf) {
                    doCR();
                    mPrintf("You can not change that!");
                    doCR();
                }
                break;

            case 'S':
                lBuf.lbflags.SYSOP = (BOOL) (!lBuf.lbflags.SYSOP);
                mPrintf("Sysop %s", lBuf.lbflags.SYSOP ? "Yes" : "No");
                doCR();
                break;

            case 'A':
                lBuf.lbflags.AIDE = (BOOL) (!lBuf.lbflags.AIDE);
                mPrintf("Aide %s", lBuf.lbflags.AIDE ? "Yes" : "No");
                doCR();
                break;

            case 'O':
                lBuf.lbflags.NODE = (BOOL) (!lBuf.lbflags.NODE);
                mPrintf("nOde %s", lBuf.lbflags.NODE ? "Yes" : "No");
                doCR();
                break;

            case 'I':
                mPrintf("mInutes");
                doCR();
				if (cfg->accounting) {
                    lBuf.lbflags.NOACCOUNT =
                    getYesNo("Disable user's accounting",
                    (BOOL) lBuf.lbflags.NOACCOUNT);

                    if (!lBuf.lbflags.NOACCOUNT) {
						sprintf(temp, "%ss left", cfg->credit_name);
                        lBuf.credits = (float) getNumber(temp, 0L,
						(long) cfg->maxbalance, (long) lBuf.credits);
                    }
                } else {
                    doCR();
                    mPrintf("Accounting turned off for system.");
                }
                break;

            case 'E':
                lBuf.lbflags.PERMANENT = (BOOL) (!lBuf.lbflags.PERMANENT);
                mPrintf("pErmanent %s", lBuf.lbflags.PERMANENT ? "Yes" : "No");
                doCR();
                break;

			case 'D':
				lBuf.DUNGEONED = (BOOL) (!lBuf.DUNGEONED);
				mPrintf("Dungeoned %s", lBuf.DUNGEONED ? "Yes" : "No");
                doCR();
                break;

			case 'K':
				lBuf.STICKYRM = (BOOL) (!lBuf.STICKYRM);
				mPrintf("Keep in room %s", lBuf.STICKYRM ? "Yes" : "No");
                doCR();
                break;

            case 'B':
				getString("Back up Visits (1..15)", temp, 4, FALSE, ECHO, "3");
				normalizeString(temp);
				if (!strlen(temp)) break;
				num_visit = atoi(temp);
				if(num_visit < 1 || num_visit > 15) {
					mPrintf("1 to 15 only!!");
					doCR();
					break;
				}
				setbackrooms(num_visit);
				break;

			case 'T':
                lBuf.lbflags.NETUSER = (BOOL) (!lBuf.lbflags.NETUSER);
                mPrintf("neTuser %s", lBuf.lbflags.NETUSER ? "Yes" : "No");
                doCR();
                break;

			case 'H':
				lBuf.SOUND = (BOOL) (!lBuf.SOUND);
				mPrintf("Sound %s", lBuf.SOUND ? "Yes" : "No");
                doCR();
                break;

			case 'R':
				lBuf.READER = (BOOL) (!lBuf.READER);
				mPrintf("Reader %s", lBuf.READER ? "Yes" : "No");
                doCR();
                break;

            case 'C':
				lBuf.NOCHAT = (BOOL) (!lBuf.NOCHAT);
				mPrintf("no Chat %s", lBuf.NOCHAT ? "Yes" : "No");
                doCR();
                break;

			case 'F':
				lBuf.TRAPIT = (BOOL) (!lBuf.TRAPIT);
				mPrintf("trap to File %s", lBuf.TRAPIT ? "Yes" : "No");
                doCR();
                break;

            case 'P':
                lBuf.lbflags.PROBLEM = (BOOL) (!lBuf.lbflags.PROBLEM);
                mPrintf("Problem user %s", lBuf.lbflags.PROBLEM ? "Yes" : "No");
                doCR();
                break;

            case 'M':
                lBuf.lbflags.NOMAIL = (BOOL) (!lBuf.lbflags.NOMAIL);
                mPrintf("no Mail %s", lBuf.lbflags.NOMAIL ? "Yes" : "No");
                doCR();
                break;

            case 'V':
                lBuf.VERIFIED = (BOOL) (!lBuf.VERIFIED);
                mPrintf("Verified %s", !lBuf.VERIFIED ? "Yes" : "No");
                doCR();
                break;

            case 'Q':
                mPrintf("Quit");
                doCR();
                quit = TRUE;
                break;

            case '\r':
            case '\n':
            case '?':
                mPrintf("Menu");
                doCR();
                prtMess = TRUE;
                break;

            default:
                mPrintf("%c ? for help", c);
                doCR();
                break;
        }

    } while (!quit);

    if (!getYesNo("Save changes", 0))
        return;

    /* trap it */
    sprintf(string, "%s has:", who);
    if (lBuf.lbflags.SYSOP)
        strcat(string, " Sysop Priv:");
    if (lBuf.lbflags.AIDE)
        strcat(string, " Aide Priv:");
    if (lBuf.lbflags.NODE)
        strcat(string, " Node status:");
	if (cfg->accounting)
        if (lBuf.lbflags.NOACCOUNT)
            strcat(string, " No Accounting:");
        else {
            sprintf(temp, " %.0f %s:", lBuf.credits);
            strcat(string, temp);
        }

    if (lBuf.lbflags.PERMANENT)
        strcat(string, " Permanent Log Entry:");
    if (lBuf.lbflags.NETUSER)
        strcat(string, " Network User:");
    if (lBuf.lbflags.PROBLEM)
        strcat(string, " Problem User:");
    if (lBuf.lbflags.NOMAIL)
        strcat(string, " No Mail:");
    if (lBuf.VERIFIED)
        strcat(string, " Un-Verified:");

/*  trap(temp, T_SYSOP); -- for a long time, this incredible bug was here. */
    trap(string, T_SYSOP);

    /* see if it is us: */
    if (loggedIn && editSelf) {
    /* move it back */
        logBuf = lBuf;

    /* make our environment match */
        setsysconfig();
    }
    putLog(&lBuf, logNo);
    logTab[ltabSlot].permanent = (BOOL) lBuf.lbflags.PERMANENT;
    logTab[ltabSlot].ltnmhash = hash(lBuf.lbname);
}

/* EOF */
