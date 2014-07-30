/* --------------------------------------------------------------------
 *  INPUT.C                   Citadel
 * --------------------------------------------------------------------
 *  This file contains the input functions
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#endif

#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  getNormStr()    gets a string and normalizes it. No default.        */
/*  getNumber()     Get a number in range (top, bottom)                 */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/*  setio()         set io flags according to whichio,echo and outflag  */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 * HISTORY:
 *
 *  05/26/89    (PAT)   Created from MISC.C to break that module into
 *                      more managable and logical pieces. Also draws
 *                      off MODEM.C
 *  03/12/90    FJM     Added IBM Graphics character translation
 *  03/15/90    {zm}    Use FILESIZE (20) instead of NAMESIZE.
 *  03/16/90    FJM     Fixed bug in iChar that prevented graphics entry
 *  03/18/90    FJM     Entry bug fix (again?)
 *  03/19/90    FJM     Linted & partial cleanup
 *  07/23/90    FJM     Prevent entry of 0xff in iChar
 *  08/06/90    FJM     Made "Sleeping? Call again :-)" hang up logged
 *                      in users in door mode.
 *  08/07/90    FJM     Corrected sleeping fix to exit cit, instead of
 *                      hanging up the phone line.
 *  09/19/90    FJM     Made prompts more polite :)
 *	11/24/90	FJM 	Changes for shell/door mode.
 *	01/18/91	FJM 	Allow ANSI in getString
 *	01/19/91	FJM 	Simplified normalizePW()
 *	01/28/91	FJM 	Added getASCString().
 *	02/05/91	FJM 	Added screen saver routines.
 *	02/11/91	FJM 	Made screen saver ignore modem responses with no carrier.
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  getNormStr()    gets a string and normalizes it. No default.        */
/* -------------------------------------------------------------------- */
void getNormStr(char *prompt, char *buffer, int size, char doEcho)
{
    getString(prompt, buffer, size, FALSE, doEcho, "");
    normalizeString(buffer);
}

/* -------------------------------------------------------------------- */
/*  getNumber()     Get a number in range (top, bottom)                 */
/* -------------------------------------------------------------------- */
long getNumber(char *prompt, long bottom, long top, long dfaultnum)
{
    long try;
    char numstring[FILESIZE];
    char buffer[20];
    char *dfault;

    dfault = ltoa(dfaultnum, buffer, 10);

    if (dfaultnum == -1L)
        dfault[0] = '\0';

    do {
        getString(prompt, numstring, FILESIZE, FALSE, ECHO, dfault);
        try = atol(numstring);
        if (try < bottom)
            mPrintf("Sorry, must be at least %ld\n", bottom);
        if (try > top)
            mPrintf("Sorry, must be no more than %ld\n", top);
    } while ((try < bottom || try > top) && !ExitToMsdos &&
		(haveCarrier || onConsole));
    return (long) try;
}

/* -------------------------------------------------------------------- */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/* -------------------------------------------------------------------- */
void getString(char *prompt, char *buf, int lim, char QuestIsSpecial,
char doEcho, char *dfault)
{
	getASCString(prompt,buf,lim,QuestIsSpecial,doEcho,dfault,0);
}

/* --------------------------------------------------------------------
 *  getASCString()  gets a string from user w/ prompt & default, ext.
 *					w/option to inhibit non-ASCII entry
 * -------------------------------------------------------------------- */
void getASCString(char *prompt, char *buf, int lim, char QuestIsSpecial,
char doEcho, char *dfault, char ASCIIOnly)
/* char *prompt;           Enter PROMPT */
/* char *buf;              Where to put it */
/* char doEcho;            To echo, or not to echo, that is the question */
/* int  lim;               max # chars to read */
/* char QuestIsSpecial;    Return immediately on '?' input? */
/* char *dfault;           Default for the lazy. */
/* char ASCIIOnly;         Restrict to ' ' thru '~'? */
{
    char c, oldEcho, tmpEcho, errors = 0;
    int i;

    outFlag = IMPERVIOUS;

    if (strlen(prompt)) {
        doCR();

        if (strlen(dfault)) {
            sprintf(gprompt, "Please enter %s [%s]:", prompt, dfault);
        } else {
            sprintf(gprompt, "Please enter %s:", prompt);
        }

        mtPrintf(TERM_REVERSE, gprompt);
        mPrintf(" ");

        dowhat = PROMPT;
    } else {
        mPrintf(": ");
	}
    oldEcho = echo;

    if (!doEcho) {
		if (!cfg->nopwecho)
            echo = CALLER;

		else if (cfg->nopwecho == 1) {
            echo = NEITHER;
            echoChar = '\0';
        } else {
            echo = NEITHER;
			echoChar = cfg->nopwecho;
        }
    }
    i = 0;
    while (c = (char) iChar(), c != 10  /* NEWLINE */
    && i < lim && (!ExitToMsdos && (haveCarrier || onConsole))) {
        outFlag = OUTOK;

        if ((echoChar >= '0') && (echoChar <= '9')) {
            echoChar++;
            if (echoChar > '9')
                echoChar = '0';
        }
    /* handle delete chars: */
        if (c == '\b') {
            if ((echoChar >= '0') && (echoChar <= '9')) {
                echoChar--;
                if (echoChar < '0')
                    echoChar = '9';
            }
			if ((echo != NEITHER) || (cfg->nopwecho != 1)) {
                if (buf[i - 1] == '\t') {
                    while ((crtColumn % 8) != 1)
                        doBS();
                }
            }
            if (i > 0) {
                i--;

                if ((echoChar >= '0') && (echoChar <= '9')) {
                    echoChar--;
                    if (echoChar < '0')
                        echoChar = '9';
                }
            } else {
				if ((echo != NEITHER) || (cfg->nopwecho != 1))
                    oChar(' ');
                oChar('\a');
            }
        } else if (c == 1) {	/* Cit ANSI */
			if (ansiOn && !ASCIIOnly) {
				if (doEcho == ECHO) {
					buf[i++] = c;
					tmpEcho = echo;
					echo = NEITHER;
					c = iChar();
					echo = tmpEcho;
					termCap(c);
					buf[i++] = c;
				} else {
					/* don't let them enter ANSI they can't see */
					oChar ('\a');
				}
			} else
                oChar('\a');
		} else if (!ASCIIOnly || ((c >= ' ') && (c <= '~'))) {
            buf[i++] = c;
        }

        if (i >= lim) {
            oChar(7 /* bell */ );
			if ((echo != NEITHER) || (cfg->nopwecho != 1)) {
                doBS();
            }
            i--;

            if ((echoChar >= '0') && (echoChar <= '9')) {
                echoChar--;
                if (echoChar < '0')
                    echoChar = '9';
            }
            errors++;
            if (errors > 15 && !onConsole) {
                drop_dtr();
            }
        }
    /* kludge to return immediately on single '?': */
        if (QuestIsSpecial && *buf == '?') {
            doCR();
            break;
        }
    }

    echo = oldEcho;
    buf[i] = '\0';
    echoChar = '\0';

    if (strlen(dfault) && !strlen(buf))
        strcpy(buf, dfault);

    dowhat = DUNO;
}


/* -------------------------------------------------------------------- */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/* -------------------------------------------------------------------- */
int getYesNo(char *prompt, char dfault)
{
    int toReturn;
    char c;
    char oldEcho;

/*    while (MIReady()) getMod(); */

    doCR();
    toReturn = ERROR;

    outFlag = IMPERVIOUS;
    sprintf(gprompt, "%s? ", prompt);

    switch (dfault) {
        case 0:
            strcat(gprompt, "(Y/N)[N]:");
            break;
        case 1:
            strcat(gprompt, "(Y/N)[Y]:");
            break;
        case 2:
            strcat(gprompt, "(Y/N/A)[A]:");
            break;
        case 3:
            strcat(gprompt, "(Y/N/A)[N]:");
            break;
        case 4:
            strcat(gprompt, "(Y/N/A)[Y]:");
            break;
        default:
            strcat(gprompt, "(Y/N)[N]:");
            dfault = 0;
            break;
    }

    mtPrintf(TERM_REVERSE, gprompt);
    mPrintf(" ");

    dowhat = PROMPT;

    do {
        oldEcho = echo;
        echo = NEITHER;
        c = (char) iChar();
        echo = oldEcho;

        if ((c == '\n') || (c == '\r')) {
            if (dfault == 1 || dfault == 4)
                c = 'Y';
            if (dfault == 0 || dfault == 3)
                c = 'N';
            if (dfault == 2)
                c = 'A';
        }
        switch (toupper(c)) {
            case 'Y':
                mPrintf("Yes");
                doCR();
                toReturn = 1;
                break;
            case 'N':
                mPrintf("No");
                doCR();
                toReturn = 0;
                break;
            case 'A':
                if (dfault > 1) {
                    mPrintf("Abort");
                    doCR();
                    toReturn = 2;
                }
                break;
        }
    } while (toReturn == ERROR && (!ExitToMsdos && (haveCarrier || onConsole)));

    outFlag = OUTOK;
    dowhat = DUNO;
    return toReturn;
}

/* -------------------------------------------------------------------- */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/* -------------------------------------------------------------------- */
int BBSCharReady(void)
{
    return ((haveCarrier && (whichIO == MODEM) && MIReady()) ||
    KBReady());
}

/* -------------------------------------------------------------------- */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/* -------------------------------------------------------------------- */
int iChar(void)
{
    unsigned char c = 0;
 	int char_waiting = 0;
    long timer, timer2, curent;
    char str[40];       /* for baud detect */
    static int scount = 0;
    static int sscount = 0;

    if (justLostCarrier || ExitToMsdos)
        return 0;       /* ugly patch   */

    sysopkey = FALSE;       /* go into sysop mode from main()? */
    eventkey = FALSE;       /* fo an event? */

    time(&timer);
    timer2 = timer;

    while (TRUE) {
		int car;
		car = gotCarrier();
    /* just in case person hangs up in console mode */
        if (detectflag /* && !onConsole */ && !car) {
            Initport();
            detectflag = FALSE;
        }
        if (!carrier() || ExitToMsdos) {
			do_idle(0);
            return 0;
		}

    /* got carrier in console mode, switched to modem */
        if (detectflag && !onConsole && car) {
            carrdetect();
            detectflag = FALSE;
			do_idle(0);
            return (0);
        }
        if (KBReady()) {
			do_idle(0);
            c = getch();
            getkey = 0;
            ++received;     /* increment received char count */
            break;
        }
        if (MIReady() && !detectflag) {
			if (car)		/* screen saver will ignore modem responses */
				do_idle(0);
			if (!modStat && (cfg->dumbmodem == 0)) {
				/* seek baud rate */
                if (getModStr(str)) {
                    c = TRUE;

                    switch (atoi(str)) {
                        case 13:    /* 9600   13 or 17   */
                        case 17:
                            baud(4);
                            break;

                        case 10:    /* 2400   10 or 16   */
                        case 16:
                            baud(2);
                            break;

                        case 5: /* 1200   15 or  5   */
                        case 15:
                            baud(1);
                            break;

                        case 1: /* 300    1  */
                            baud(0);
                            break;

                        case 2: /* ring, hold cron event */
                            time(&timer);
                            c = FALSE;
                            break;

                        default:    /* something else */
                            c = FALSE;
                            break;
                    }

                    if (c) {    /* detected */
                        if (!onConsole) {
                            detectflag = FALSE;
                            carrdetect();
                            return (0);
                        } else {
                            detectflag = TRUE;
                            c = 0;
                            /* update25();	*/
							do_idle(0);
                        }
                    }
                } else {
            /* failed to adjust */
                    Initport();
                }
            } else {
                c = getMod();
            }

            if (haveCarrier) {
                if (whichIO == MODEM)
                    break;
            } else {
                c = 0;
            }
        }
		char_waiting = KBReady() || MIReady();
		if (!char_waiting) {
			/* CRON events */
			time(&curent);
			if (((int) (curent - timer) / (int) 60) >= cfg->idle
			&& !loggedIn && !gotCarrier() && dowhat == MAINMENU) {
				time(&timer);
				if (do_cron(CRON_TIMEOUT)) {
					return (0);
				}
			} else if (
			(cfg->ad_time && ((int) (curent - timer2) >= cfg->ad_time))
			&& loggedIn && dowhat == MAINMENU) {
				time(&timer2);
				if((!onConsole) || (onConsole && !expert)) {
					doAd(1);
					givePrompt();
				}
			}
			if (chatkey || sysopkey || eventkey) {
				do_idle(0);
				return (0);
			}
		}

        if (chatkey && dowhat == PROMPT) {
            char oldEcho;

            oldEcho = echo;
            echo = BOTH;

            doCR();
            chat();
            doCR();
			mtPrintf(TERM_REVERSE, gprompt);
			mPrintf(" ");

            echo = oldEcho;

            time(&timer);

            chatkey = FALSE;
        }
        if (systimeout(timer)) {
            nextblurb("sleep", &scount, 1);

            if (parm.door)  /* supress initial hangup in door mode */
                ExitToMsdos = 1;
            else
                Initport();
				
        } else if (systimeout(timer + 30)) {
            nextblurb("sleepy", &sscount, 1);
        }
		/* do the idle display */
		do_idle(1);
    }
	

    if (c < 128)
        c = filt_in[c];
    else if (c == 255)
        c = 0;
    /* don't print out the P at the main menu. & Don't print ^A's  */
    if (c != 1 && ((c != 'p' && c != 'P') || dowhat != MAINMENU))
        echocharacter(c);

    return (c);
}

/* -------------------------------------------------------------------- */
/*  setio()         set io flags according to whicio, echo and outflag  */
/* -------------------------------------------------------------------- */
void setio(char whichio, char echo_who, char outflag)
{
    if  ((outflag != OUTOK) && (outFlag != IMPERVIOUS)) {
        modem = FALSE;
        console = FALSE;
	} else if (echo_who == BOTH) {
		if(!onConsole) {
			modem = TRUE;
			console = TRUE;
		} else {
			modem = FALSE;
			console = TRUE;
		}
    } else if (echo_who == CALLER) {
        if (whichio == MODEM) {
            modem = TRUE;
            console = FALSE;
        } else if (whichio == CONSOLE) {
            modem = FALSE;
            console = TRUE;
        }
    } else if (echo_who == NEITHER) {
        modem = TRUE;       /* FALSE; */
        console = TRUE;     /* FALSE; */
    }
    if (!haveCarrier)
        modem = FALSE;
}

/* EOF */
