/* -------------------------------------------------------------------- */
/*  MODEM.C                   Citadel                                   */
/* -------------------------------------------------------------------- */
/*  High level modem code, should not need to be changed for porting(?) */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#include <alloc.h>
#include <dos.h>
#endif

#include "proto.h"
#include "global.h"

/* --------------------------------------------------------------------
 *                              Contents
 * --------------------------------------------------------------------
 *  carrier()       checks carrier
 *  carrdetect()    sets global flags for carrier detect
 *  carrloss()      sets global flags for carrier loss
 *  checkCR()       Checks for CRs from the data port for half a second.
 *  doccr()         Do CR on console, used to not scroll the window
 *  domcr()         print cr on modem, nulls and lf's if needed
 *  findbaud()      Finds the baud from sysop and user supplied data.
 *  fkey()          Deals with function keys from console
 *  KBReady()       returns TRUE if a console char is ready
 *  offhook()       sysop fn: to take modem off hook
 *  outCon()        put a character out to the console
 *  outstring()     push a string directly to the modem
 *  ringdetectbaud()    sets baud rate according to ring detect
 *  verbosebaud()   sets baud rate according to verbodse codes
 *  getModStr()     get a string from the modem, waiting for upto 3 secs
 * -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *	05/11/82	(CrT)	Created
 *  05/26/89    (PAT)   Code moved to Input.c, output.c, and timedate.c
 *  02/07/89    (PAT)   Hardeware dependant code moved to port.c,
 *                      History recreated. PLEASE KEEP UP-TO-DATE
 *
 *	03/12/90	FJM 	Added IBM Grahics character translation
 *  03/19/90    FJM     Linted & partial cleanup
 *  04/02/90    FJM     Added ALT-M memory/stach check
 *	11/24/90	FJM 	Changes for shell/door mode.
 *	11/24/90	FJM 	Modified to eat characters after a function key.
 *
 * -------------------------------------------------------------------- */

/* extern unsigned _stklen; */

/* --------------------------------------------------------------------
 *  carrier()       checks carrier
 * -------------------------------------------------------------------- */

int carrier(void)
{
    unsigned char c;

	if ((c = (uchar) gotCarrier()) != modStat
        && (!detectflag)
    ) {
    /* carrier changed   */
        if (c) {        /* carrier present   */
			switch (cfg->dumbmodem) {
                case 0:     /* numeric */
        /* do not use this routine for carrier detect */
                    return (1);

                case 1:     /* returns */
                    if (!findbaud()) {
                        Initport();
                        return TRUE;
                    }
                    break;

                case 2:     /* HS on RI */
                    ringdetectbaud();
                    break;

                case 3:     /* verbose */
                    verbosebaud();
                    break;

                default:
                case 4:     /* forced */
					baud(cfg->initbaud);
                    break;
            }

            if (!onConsole) {
                carrdetect();
                detectflag = FALSE;
                return (0);
            } else {
                detectflag = TRUE;
                /* update25();	*/
				do_idle(0);
                return (1);
            }
        } else {
            delay(2000);    /* confirm it's not a glitch */
            if (!gotCarrier()) {/* check again */
                carrloss();

                return (0);
            }
        }
    }
    return (1);
}

/* -------------------------------------------------------------------- */
/*  carrdetect()    sets global flags for carrier detect                */
/* -------------------------------------------------------------------- */
void carrdetect(void)
{
    char temp[30];

    haveCarrier = TRUE;
    modStat = TRUE;
    newCarrier = TRUE;
    justLostCarrier = FALSE;

    time(&conntimestamp);

    connectcls();
    /* update25();	*/
	do_idle(0);

    sprintf(temp, "Carrier-Detect (%d)", bauds[speed]);
    trap(temp, T_CARRIER);

	logBuf.credits = cfg->unlogbal;
}

/* -------------------------------------------------------------------- */
/*  carrloss()      sets global flags for carrier loss                  */
/* -------------------------------------------------------------------- */
void carrloss(void)
{
    outFlag = OUTSKIP;
    haveCarrier = FALSE;
    modStat = FALSE;
    justLostCarrier = TRUE;
    if (parm.door)
        ExitToMsdos = TRUE;
    Initport();

    trap("Carrier-Loss", T_CARRIER);
}

/* -------------------------------------------------------------------- */
/*  checkCR()       Checks for CRs from the data port for half a second.*/
/* -------------------------------------------------------------------- */
BOOL checkCR(void)
{
    int i;

    for (i = 0; i < 50; i++) {
        delay(10);
        if (MIReady())
            if (getMod() == '\r')
                return FALSE;
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  doccr()         Do CR on console, used to not scroll the window     */
/* -------------------------------------------------------------------- */
void doccr(void)
{
    unsigned char row, col;


    if (!console)
        return;
    if (!anyEcho)
        return;

    readpos(&row, &col);

    if (row == (scrollpos + 1)) {
        position(0, 0);     /* clear screen if we hit our window */
    }
    if (row >= scrollpos) {
		scroll(scrollpos, 1, cfg->attr);
        position(scrollpos, 0);
    } else {
        putch('\n');
        putch('\r');
    }
}

/* -------------------------------------------------------------------- */
/*  domcr()         print cr on modem, nulls and lf's if needed         */
/* -------------------------------------------------------------------- */
void domcr(void)
{
    int i;

    outMod('\r');
    for (i = termNulls; i; i--)
        outMod(0);
    if (termLF)
        outMod('\n');
}

/* -------------------------------------------------------------------- */
/*  findbaud()      Finds the baud from sysop and user supplied data.   */
/* -------------------------------------------------------------------- */
int findbaud(void)
{
    char noGood = TRUE;
    int Time = 0;
    int baudRunner;     /* Only try for 60 seconds      */

    while (MIReady())
        getMod();       /* Clear garbage        */
    baudRunner = 0;
    while (gotCarrier() && noGood && Time < 120) {
        Time++;
        baud(baudRunner);
        noGood = checkCR();
        if (noGood)
            baudRunner = (baudRunner + 1) % (3 /* 2400 too */ );
    }
    return !noGood;
}

/* -------------------------------------------------------------------- */
/*  fkey()          Deals with function keys from console               */
/* -------------------------------------------------------------------- */
int fkey(void)
{
    char key;
    int oldIO, i;
    label string;
    unsigned space;

#define F1		59
#define F2		60
#define F3		61
#define F4		62
#define F5		63
#define F6		64
#define S_F6	89
#define A_F6	109
#define F7		65
#define F8		66
#define F9		67
#define F10 	68
#define ALT_P	25
#define ALT_D	32
#define ALT_B	48
#define ALT_L	38
#define ALT_M	50
#define ALT_T	20
#define ALT_X	45
#define ALT_C	46
#define ALT_E	18
#define PGUP	73
#define PGDN	81
#define HOME	71
#define END 	79
#define UPARROW 72
#define DNARROW 80
#define LTARROW 75
#define RTARROW 77


    key = (char) getch();
    while (kbhit())     /* flush keyboard buffer */
        getch();
	if (strcmpi(cfg->f6pass, "f6disabled") != SAMESTRING)
        if (ConLock == TRUE && key == ALT_L &&
		strcmpi(cfg->f6pass, "disabled") != SAMESTRING) {
            ConLock = FALSE;

            oldIO = whichIO;
            whichIO = CONSOLE;
            onConsole = TRUE;
            /* update25();	*/
			do_idle(0);
            string[0] = 0;
            getNormStr("System Password", string, NAMESIZE, NO_ECHO);
			if (strcmpi(string, cfg->f6pass) != SAMESTRING)
                ConLock = TRUE;
            whichIO = (BOOL) oldIO;
            onConsole = (BOOL) (whichIO == CONSOLE);
            /* update25();	*/
			do_idle(0);
            givePrompt();
			return(0);
        }
	if (ConLock && !sysop && strcmpi(cfg->f6pass, "f6disabled") != SAMESTRING)
		return(0);

    switch (key) {
        case F1:
            drop_dtr();
            detectflag = FALSE;
            break;

        case F2:
            if (parm.door)
                ExitToMsdos = 1;
            else
                Initport();
            detectflag = FALSE;
            break;

        case F3:
            sysReq = (BOOL) (!sysReq);
            break;

        case F4:
            anyEcho = (BOOL) (!anyEcho);
            break;

        case F5:
            if (whichIO == CONSOLE)
                whichIO = MODEM;
            else
                whichIO = CONSOLE;

            onConsole = (BOOL) (whichIO == CONSOLE);

			break;

        case S_F6:
			if (!ConLock) {
                aide = (BOOL) (!aide);
			}
            break;

        case A_F6:
			if (!ConLock) {
                sysop = (BOOL) (!sysop);
			}
            break;

        case F6:
            if (sysop || !ConLock)
                sysopkey = TRUE;
            break;

        case F7:
			if(cfg->noBells) {
				cfg->noBells = !cfg->noBells;
				cfg->Sound = FALSE;
			}
			else {
				if(cfg->Sound) {
					cfg->noBells = TRUE;
				}
				cfg->Sound = !cfg->Sound;
            }
            break;

        case ALT_C:
        case F8:
            chatkey = !chatkey;    /* will go into chat from main() */
            break;

        case F9:
			cfg->noChat = !cfg->noChat;
            chatReq = FALSE;
            break;

        case F10:
            help();
            break;

        case ALT_B:
            backout = !backout;
            break;

        case ALT_D:
            debug = !debug;
            break;

        case ALT_E:
            eventkey = TRUE;
            break;

        case ALT_L:
			if (cfg->f6pass[0] && strcmpi(cfg->f6pass, "f6disabled") != SAMESTRING)
                ConLock = (BOOL) (!ConLock);
            break;

        case ALT_M:
#ifndef ATARI_ST
			if(coreleft() == farcoreleft()) {
				cPrintf("\nFree heap        %lXh (%ld.)\n", coreleft(), coreleft());
            }
			else {
				cPrintf("\nFree near heap   %lXh (%ld.)\n", coreleft(), coreleft());
				cPrintf("Free far heap    %lXh (%ld.)\n", farcoreleft(), farcoreleft());
			}
            space = _stklen - ((unsigned) 0xffff - _SP);
            cPrintf("Free stack       %Xh (%d.)\n", space, space);
			if(farheapcheck() < 0)
				cPrintf("Heap is corrupted!!");
#endif
            break;

        case ALT_P:
            if (printing) {
                printing = FALSE;
                fclose(printfile);
            } else {
				printfile = fopen(cfg->printer, "a");
                if (printfile) {
                    printing = TRUE;
                } else {
                    printing = FALSE;
                    fclose(printfile);
                }
            }
            break;

        case ALT_T:
            twit = (BOOL) (!twit);
            break;

        case ALT_X:
            if (dowhat == MAINMENU || dowhat == SYSOPMENU) {
                if (loggedIn) {
                    i = getYesNo("Exit to MS-DOS", 0);
                } else {
                    doCR();
                    doCR();
                    mPrintf("Exit to MS-DOS");
                    doCR();
                    i = TRUE;
                }

                if (!i) {
                    if (dowhat == MAINMENU) {
                        givePrompt();
                    } else {
                        doCR();
                        mPrintf("Privileged function: ");
                    }
                    break;
                }
                ExitToMsdos = TRUE;
            }
            break;

		case PGUP:
			if(cfg->accounting) {
				if(!logBuf.lbflags.NOACCOUNT &&
					(logBuf.credits+10 < cfg->maxbalance))
						logBuf.credits += 10;
            }
            break;
        case PGDN:
			if(cfg->accounting) {
				if(!logBuf.lbflags.NOACCOUNT &&
					(logBuf.credits-10 > 0))
					logBuf.credits -= 10;
            }
            break;
        case HOME:
			if(cfg->accounting) {
				logBuf.lbflags.NOACCOUNT = 0;
			}
			break;
        case END:
			if(cfg->accounting) {
				logBuf.lbflags.NOACCOUNT = 1;
            }
            break;
			case RTARROW:
				return('+');

			case LTARROW:
				return('-');

			case UPARROW:
				return('>');

			case DNARROW:
				return('<');

		default:
            break;
    }

    /* update25();	*/
	do_idle(0);
	return(0);
}

/* -------------------------------------------------------------------- */
/*  KBReady()       returns TRUE if a console char is ready             */
/* -------------------------------------------------------------------- */
BOOL KBReady(void)
{
    int c;

    if (getkey)
        return (TRUE);

    if (kbhit()) {
        c = getch();

        if (!c) {
			c = fkey();
			if(c) {
				ungetch(c);
                getkey = 1;
				return(TRUE);
			}
			else
				return (FALSE);
        } else
            ungetch(c);

        getkey = 1;

        return (TRUE);
    } else
        return (FALSE);
}

/* -------------------------------------------------------------------- */
/*  offhook()       sysop fn: to take modem off hook                    */
/* -------------------------------------------------------------------- */
void offhook(void)
{
    Initport();
    outstring("ATM0H1\r");
}

/* -------------------------------------------------------------------- */
/*  outCon()        put a character out to the console                  */
/* -------------------------------------------------------------------- */
void outCon(char c)
{
    unsigned char row, col;
    static char escape = FALSE;

    if (!console)
        return;

	if (c == '\a' /* BELL */ && cfg->noBells)
        return;
    if (c == 27 || escape) {    /* ESC || ANSI sequence */
        escape = ansi(c);
        return;
    }
    if (c == 0x1a)      /* CT-Z */
        return;

    if (!anyEcho)
        return;

    /* if we dont have carrier then count what goes to console */
    if (!gotCarrier())
        transmitted++;

    if (c == '\n')
        doccr();
    else if (c == '\r') {
        putch(c);
    } else {
        readpos(&row, &col);
        if (c == '\b' || c == 7) {
            if (c == '\b' && col == 0 && prevChar != '\n')
				position(row - 1, 79);

			if(c == 7) {
				submit_sound(955,240);
				submit_sound(0,250);
			}
			else {
				putch(c);
			}
        } else {
            (*charattr) (c, ansiattr);
            if (col == 79) {
                position(row, col);
                doccr();
            }
        }
    }
}

/* -------------------------------------------------------------------- */
/*  outstring()     push a string directly to the modem                 */
/* -------------------------------------------------------------------- */
void outstring(char *string)
{
    int mtmp;

    mtmp = modem;
    modem = TRUE;

    while (*string) {
        outMod(*string++);  /* output string */
    }

    modem = (uchar) mtmp;
}

/* -------------------------------------------------------------------- */
/*  ringdetectbaud()    sets baud rate according to ring detect         */
/* -------------------------------------------------------------------- */
void ringdetectbaud(void)
{
    baud(ringdetect());
}

/* -------------------------------------------------------------------- */
/*  verbosebaud()   sets baud rate according to verbodse codes          */
/* -------------------------------------------------------------------- */
void verbosebaud(void)
{
    char c, f = 0;
    long t;

    if (debug)
        outCon('[');

    time(&t);

    while (gotCarrier() && time(NULL) < (t + 4) && !KBReady()) {
        if (MIReady()) {
            c = (char) getMod();
        } else {
            c = 0;
        }

        if (debug && c) {
            outCon(c);
        }
        if (f) {
            switch (c) {
                case '\n':
                case '\r':      /* CONNECT */
                    baud(0);
                    if (debug)
                        outCon(']');
                    return;
                case '1':       /* CONNECT 1200 */
                    baud(1);
                    if (debug)
                        outCon(']');
                    return;
                case '2':       /* CONNECT 2400 */
                    baud(2);
                    if (debug)
                        outCon(']');
                    return;
                case '4':       /* CONNECT 4800 */
                    baud(3);
                    if (debug)
                        outCon(']');
                    return;
                case '9':       /* CONNECT 9600 */
                    baud(4);
                    if (debug)
                        outCon(']');
                    return;
                default:
                    break;
            }
        }
        if (c == 'C') {
            if (debug) {
                outCon('@');
            }
            f = 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  getModStr()     get a string from the modem, waiting for upto 3 secs*/
/*                  for it. Returns TRUE if it gets one.                */
/* -------------------------------------------------------------------- */
int getModStr(char *str)
{
    long tm;
    int l = 0, c;

    time(&tm);

    if (debug)
        cPrintf("[");

    while (
        (time(NULL) - tm) < 4
        && !KBReady()
        && l < 40
    ) {
        if (MIReady()) {
            c = getMod();

            if (c == 13 || c == 10) {   /* CR || LF */
                str[l] = EOS;
                if (debug)
                    cPrintf("]\n");
                return TRUE;
            } else {
                if (debug)
                    cPrintf("%c", c);
                str[l] = (char) c;
                l++;
            }
        }
    }

    if (debug)
        cPrintf(":F]\n");

    str[0] = EOS;

    return FALSE;
}
