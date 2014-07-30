/* -------------------------------------------------------------------- */
/*                              window.c                                */
/*            Machine dependent windowing routines for Citadel          */
/* -------------------------------------------------------------------- */

#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*      cls()                   clears the screen                       */
/*      connectcls()            clears the screen upon carrier detect   */
/*      cursoff()               turns cursor off                        */
/*      curson()                turns cursor on                         */
/*      gmode()                 checks for monochrome card              */
/*      help()                  toggles help menu                       */
/*      position()              positions the cursor                    */
/*      readpos()               returns current row, col position       */
/*      scroll()                scrolls window up                       */
/*      update25()              updates the 25th line                   */
/*      updatehelp()            updates the help window                 */
/*      directchar()            Direct screen write char with attr      */
/*      directstring()          Direct screen write string w/attr at row*/
/*      bioschar()              BIOS print char with attr               */
/*      biosstring()            BIOS print string w/attr at row         */
/*      setscreen()             Set SCREEN to correct address for VIDEO */
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- *
 *  HISTORY:
 *
 *  03/14/90    FJM     Added 43/50 line mode support.
 *  03/20/90    FJM     Added wscroll() for window scrolling.
 *  03/29/90    FJM     Fixed bug in save_screen() (cursor pos).
 *  04/02/90    FJM     Another fixed bug in save_screen() (cursor pos).
 *	11/24/90	FJM 	Changes for shell/door mode.
 *	12/09/90	FJM 	ANSI/ISO color support for local screen.
 *	01/06/91	FJM 	Porting, formating, fixes and misc changes.
 *	01/19/91	FJM 	Fixed update25().  Added time display.
 *	07/04/91	BLJ 	Fixed Phantom cursor
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */
static long f10timeout;     /* when was the f10 window opened?        */
static char FAR *screen;    /* memory address for direct screen I/O   */
static char FAR *saveBuffer;    /* memory buffer for screen saves         */
static unsigned char srow, scolumn; /* static vars for saved position    */
static struct text_info t_info; /* Turbo C Text info                 */

#define SCR_BUFSIZ  (t_info.screenheight*t_info.screenwidth*2)

/*
 *  screenheight()  Number of rows on console
 *
 */

unsigned char screenheight(void)
{
    return t_info.screenheight;
}

/*
 *  screenwidth()   Number of columns on console
 *
 */

unsigned char screenwidth(void)
{
    return t_info.screenwidth;
}

/* -------------------------------------------------------------------- */
/*      cls()  clears the screen                                        */
/* -------------------------------------------------------------------- */
void cls(void)
{
    /* scroll everything but kitchen sink */
	scroll(t_info.screenheight - 1, 0, cfg->attr);
    position(0, 0);
}


/* -------------------------------------------------------------------- */
/*      connectcls()  clears the screen upon carrier detect             */
/* -------------------------------------------------------------------- */
void connectcls(void)
{
    cls();
    /* update25();	*/
	do_idle(0);
}


/* -------------------------------------------------------------------- */
/*      cursoff()  make cursor disapear                                 */
/* -------------------------------------------------------------------- */
void cursoff(void)
{
    union REGS reg;

    reg.h.ah = 01;
    reg.h.bh = 00;
    reg.h.ch = 0x26;
    reg.h.cl = 7;
    int86(0x10, &reg, &reg);
}


/* -------------------------------------------------------------------- */
/*      curson()  Put cursor back on screen checking for adapter.       */
/* -------------------------------------------------------------------- */
void curson(void)
{
    union REGS regs;
    uchar st, en;

    if (gmode() == 7) {     /* Monocrone adapter */
        st = 12;
        en = 13;
    } else {            /* Color graphics adapter. */
        st = 6;
        en = 7;
    }

    regs.h.ah = 0x01;
    regs.h.bh = 0x00;
    regs.h.ch = st;
    regs.h.cl = en;
    int86(0x10, &regs, &regs);
}


/* -------------------------------------------------------------------- */
/*      gmode()  Check for monochrome or graphics.                      */
/* -------------------------------------------------------------------- */
int gmode(void)
{
    /* the turboc way */
    return ((int) t_info.currmode);
}


/* -------------------------------------------------------------------- */
/*      help()  this toggles our help menu                              */
/* -------------------------------------------------------------------- */
void help(void)
{
    /* the turboc way */
    uchar row, col;
    int i;

    readpos(&row, &col);

    if (scrollpos == (t_info.screenheight - 2)) {   /* small window */
        if (row > (t_info.screenheight - 6)) {
            scroll(t_info.screenheight - 2, row - (t_info.screenheight - 6),
			cfg->wattr);
            row = t_info.screenheight - 6;
        }
        scrollpos = t_info.screenheight - 6;    /* big window */

        time(&f10timeout);

		textattr((int) cfg->attr);
        for (i = 0; i < 4; i++) {
            position(t_info.screenheight - 5 + i, 0);
            clreol();
        }

    } else {            /* big window */
        scrollpos = t_info.screenheight - 2;    /* small window */

        time(&f10timeout);

		textattr((int) cfg->attr);
        for (i = 0; i < 4; i++) {
            position(t_info.screenheight - 5 + i, 0);
            clreol();
        }
    }

    position(row, col);
}


/* -------------------------------------------------------------------- */
/*      position()  positions the cursor                                */
/* -------------------------------------------------------------------- */
void position(uchar row, uchar column)
{
    /* the turboc way */
    gotoxy((int) column + 1, (int) row + 1);
}


/* -------------------------------------------------------------------- */
/*      readpos()  returns current cursor position                      */
/* -------------------------------------------------------------------- */
void readpos(uchar *row, uchar *column)
{
    struct text_info t_buf, *t = &t_buf;

    /* the turboc way */
    gettextinfo(t);
    *row = (unsigned char) (t->cury - 1);
    *column = (unsigned char) (t->curx - 1);
}


/* -------------------------------------------------------------------- */
/*      scroll()  scrolls window up from specified line                 */
/* -------------------------------------------------------------------- */
void scroll(uchar row, uchar howmany, uchar attr)
{
    union REGS regs;
    uchar rw, col;

    readpos(&rw, &col);

    regs.h.al = howmany;    /* scroll how many lines           */

    regs.h.cl = 0;      /* row # of upper left hand corner */
    regs.h.ch = 0x00;       /* col # of upper left hand corner */
    regs.h.dh = row;        /* row # of lower left hand corner */
    regs.h.dl = 79;     /* col # of upper left hand corner */

    regs.h.ah = 0x06;       /* scroll window up interrupt      */
    regs.h.bh = attr;       /* char attribute for blank lines  */

    int86(0x10, &regs, &regs);

    /* put cursor back! */
    position(rw, col);
}


/* -------------------------------------------------------------------- */
/*      wscroll()  scrolls a window between specified lines             */
/* -------------------------------------------------------------------- */
void wscroll(int left, int top, int right, int bottom,
int lines, unsigned char attr)
{
    union REGS regs;
    uchar rw, col;

    readpos(&rw, &col);
    regs.h.al = lines;      /* scroll how many lines           */

    regs.h.ch = top;        /* row number, upper left            */
    regs.h.cl = left;       /* column number, upper left         */
    regs.h.dl = right;      /* column number, lower right        */
    regs.h.dh = bottom;     /* row number, lower right           */

    regs.h.ah = 0x06;       /* scroll window up interrupt      */
    regs.h.bh = attr;       /* char attribute for blank lines  */

    int86(0x10, &regs, &regs);

    /* put cursor back! */
    position(rw, col);
}

/* -------------------------------------------------------------------- */
/*      update25()  updates the 25th line according to global variables */
/* -------------------------------------------------------------------- */
#define NYMFIELD	29
/* one more place where IBM graphics characters are in strings.  Ugh */
#ifdef __MSDOS__
#define FILLCHAR1	205
#define FILLCHAR2	247
#else
#define FILLCHAR1	'~'
#define FILLCHAR2	' '
#endif
void update25(void)
{
    char string[83];
    char str2[80];
    label name;
	label temp;
	char flags[10];
    char carr[5];
	char chatstr[5];
	char prtstr[4];
	char reqstr[4];
	uchar row, col, notechar;
	char *p;
	int len;

    readpos(&row, &col);

    if (scrollpos == t_info.screenheight - 6)
        updatehelp();

	if(cfg->bios)
		cursoff();

    readpos(&row, &col);

    if (loggedIn) {
		/* center nym in field of squiggly characters */
		memset (name,FILLCHAR2,sizeof(name)-1);
		len = strlen(logBuf.lbname);
		p = ((len) >= NYMFIELD) ?
			(name) : (name + ((NYMFIELD-len)/2+1));
		memcpy(p,logBuf.lbname,min(NYMFIELD,len));
	} else {
		/* center date in field of อ characters */
		memset (name,FILLCHAR1,sizeof(name)-1);
		cit_strftime(temp, NYMFIELD, " %a  %x  %I:%M %p ", 0L);
		len = strlen(temp);
		p = ((len) >= NYMFIELD) ?
			(name) : (name + ((NYMFIELD-len)/2+1));
		memcpy(p,temp,min(NYMFIELD,len));

    }
	name[NAMESIZE] = '\0';

	/* carrier field */
    if (justLostCarrier || ExitToMsdos)
        strcpy(carr, "JL");		/* Just loss carrier	*/
    else if (haveCarrier)
        strcpy(carr, "CD");		/* carrier detected		*/
    else
        strcpy(carr, "NC");		/* no carrier			*/

	/* chat field */
	memset(chatstr,' ',sizeof(chatstr)-1);
	if (cfg->noChat) {
		if (chatReq)
			strcpy(chatstr,"rcht");
	} else {
		if (chatReq)
			strcpy(chatstr,"RCht");
		else
			strcpy(chatstr,"Chat");
	}
	chatstr[sizeof(chatstr)-1] = '\0';
	
	/* prt field */
	memset(prtstr,' ',sizeof(prtstr)-1);
	if (printing) {
		strcpy(prtstr,"Prt");
	}
	prtstr[sizeof(prtstr)-1] = '\0';
	
	/* req field */
	memset(reqstr,' ',sizeof(reqstr)-1);
	if (sysReq) {
		strcpy(reqstr,"Req");
	}
	reqstr[sizeof(reqstr)-1] = '\0';
	
	/* flags field */
	memset(flags,' ',sizeof(flags)-1);
	flags[sizeof(flags)-1]='\0';

    if (aide)
        flags[0] = 'A';
		
    if (twit)
        flags[1] = 'T';
		
    if (sysop)
        flags[5] = 'S';

    if (loggedIn) {
        if (logBuf.lbflags.PERMANENT)
            flags[2] = 'P';
        if (logBuf.lbflags.UNLISTED)
            flags[3] = 'U';
        if (logBuf.lbflags.NETUSER)
            flags[4] = 'N';
        if (logBuf.lbflags.NOMAIL)
            flags[6] = 'M';
    }

	notechar = FILLCHAR2;
	if(!cfg->noBells) {
		if(cfg->Sound)
			notechar = '\x0E';
		else
			notechar = '\x0D';
	}

	sprintf(string, "%-*.*sณ%sณ%sณ%4d baudณ%sณ%c%c%c%cณ%sณ%sณ%sณ%s",
		NYMFIELD,NYMFIELD,name,
		(whichIO == CONSOLE) ? "Console" : " Modem ",
		carr,bauds[speed],
		(disabled)    ? "DS"      : "EN",
		notechar,
		(backout)	  ? '\x9d'    : ' ',
		(debug) 	  ? '\xe8'    : ' ',
		(ConLock)	  ? '\x0f'    : ' ',
		chatstr,prtstr,reqstr,flags
	);

    sprintf(str2, "%-79s ", string);

	(*stringattr) (t_info.screenheight - 1, str2, cfg->wattr);
    position(row, col);

	if (cfg->bios)
        curson();

}


/* -------------------------------------------------------------------- */
/*      updatehelp()  updates the help menu according to global vars    */
/* -------------------------------------------------------------------- */
void updatehelp(void)
{
    long time(), l;
    char bigline[81];
    uchar row, col;

    if (f10timeout < (time(&l) - (long) (60 * 2))) {
        help();
        return;
    }
	cursoff();

    strcpy(bigline,
    "ษอออออออออออออออัอออออออออออออออัออออออออออออออัอออออออออออออออัอออออออออออออออป");

    readpos(&row, &col);

    position(t_info.screenheight - 5, 0);

	(*stringattr) (t_info.screenheight - 5, bigline, cfg->wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
    " F1 þ Shutdown ", " F2 þ Startup  ", " F3 þ Request ",
    (anyEcho) ? " F4 þ Echo-Off " : " F4 þ Echo-On  ",
    (whichIO == CONSOLE) ? " F5  þ Modem   " : " F5  þ Console ");

	(*stringattr) (t_info.screenheight - 4, bigline, cfg->wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
	" F6 þ Sysop Fn ", (cfg->noBells) ? " F7 þ Bells-On " : " F7 þ Bells-Off",
	" F8 þ ChatMode", (cfg->noChat) ? " F9 þ Chat-On  " : " F9 þ Chat-Off ",
    " F10 þ Help    ");

	(*stringattr) (t_info.screenheight - 3, bigline, cfg->wattr);

    strcpy(bigline,
    "ศอออออออออออออออฯอออออออออออออออฯออออออออออออออฯอออออออออออออออฯอออออออออออออออผ");

	(*stringattr) (t_info.screenheight - 2, bigline, cfg->wattr);

    position(row >= (t_info.screenheight - 6) ? scrollpos : row, col);

	curson();
}


/* -------------------------------------------------------------------- */
/*      directstring() print a string with attribute at row             */
/* -------------------------------------------------------------------- */
void directstring(unsigned int row, char *str, uchar attr)
{
    register int i, j, l;

    l = strlen(str);

    for (i = (row * 160), j = 0; j < l; i += 2, j++) {
        screen[i] = str[j];
        screen[i + 1] = attr;
    }
}

/* -------------------------------------------------------------------- */
/*      directchar() print a char directly with attribute at row        */
/* -------------------------------------------------------------------- */
void directchar(char ch, uchar attr)
{
    int i;
    uchar row, col;

    readpos(&row, &col);

    i = (row * 160) + (col * 2);

    screen[i] = ch;
    screen[i + 1] = attr;

    position(row, col + 1);
}

/* -------------------------------------------------------------------- */
/*      biosstring() print a string with attribute                      */
/* -------------------------------------------------------------------- */
void biosstring(unsigned int row, char *str, uchar attr)
{
    union REGS regs;
    union REGS temp_regs;
    register int i = 0;

    regs.h.ah = 9;      /* service 9, write character # attribute */
    regs.h.bl = attr;       /* character attribute                    */
    regs.x.cx = 1;      /* number of character to write           */
    regs.h.bh = 0;      /* display page                           */

    while (str[i]) {
        position((uchar) row, (uchar) i);   /* Move cursor to the correct position */
        regs.h.al = str[i]; /* set character to write     0x0900   */
        int86(0x10, &regs, &temp_regs);
        i++;
    }
}


/* -------------------------------------------------------------------- */
/*      bioschar() print a char with attribute                          */
/* -------------------------------------------------------------------- */
void bioschar(char ch, uchar attr)
{
    uchar row, col;
    union REGS regs;

    regs.h.ah = 9;      /* service 9, write character # attribute */
    regs.h.bl = attr;       /* character attribute                    */
    regs.h.al = ch;     /* write 0x0900                           */
    regs.x.cx = 1;      /* write 1 character                      */
    regs.h.bh = 0;      /* display page                           */
    int86(0x10, &regs, &regs);

    readpos(&row, &col);
    position(row, col + 1);
}

/* -------------------------------------------------------------------- */
/*      setscreen() set video mode flag 0 mono 1 cga                    */
/* -------------------------------------------------------------------- */
void setscreen(void)
{
    gettextinfo(&t_info);
    if (scrollpos != (t_info.screenheight - 6))
        scrollpos = t_info.screenheight - 2;

    if (gmode() == 7) {
        screen = (char FAR *) 0xB0000000L;  /* mono */
    } else {
        screen = (char FAR *) 0xB8000000L;  /* cga */
		outp(0x03d9, cfg->battr);/* set border color */
    }

	if (cfg->bios) {
        charattr = bioschar;
        stringattr = biosstring;
    } else {
        charattr = directchar;
        stringattr = directstring;
    }

	ansiattr = cfg->attr;
}

/* -------------------------------------------------------------------- */
/* Handle ansi escape sequences                                         */
/* -------------------------------------------------------------------- */
char ansi(char c)
{
    static char args[20], first = FALSE;
    static uchar c_x = 0, c_y = 0;
    uchar argc, a[5];
    int i;						/* array index				*/
	char rc;					/* return code				*/
    uchar x, y;					/* screen column and row	*/
    char *p;

    if (c == 27 /* ESC */ ) {
        strcpy(args, "");
        first = TRUE;
        rc = TRUE;
    } else if (first && c != '[') {
        first = FALSE;
        rc = FALSE;
    } else if (first && c == '[') {
        first = FALSE;
        rc = TRUE;
    } else if (isalpha(c)) {
        i = 0;
        p = args;
        argc = 0;
        while (*p) {
            if (isdigit(*p)) {
                char done = 0;

                a[argc] = (uchar) atoi(p);
                while (!done) {
                    p++;
                    if (!(*p) || !isdigit(*p))
                        done = TRUE;
                }
                argc++;
            } else
                p++;
        }
        switch (c) {
            case 'J':       /* cls */
                cls();
                /* update25();	*/
				do_idle(0);
                break;
            case 'K':       /* del to end of line */
                clreol();
                break;
            case 'm':
                for (i = 0; i < argc; i++) {

                    switch (a[i]) {
                        case 5:
                            ansiattr = ansiattr | 128;  /* blink */
                            break;
                        case 4:
                            ansiattr = ansiattr | 1;    /* underline */
                            break;
                        case 7:
							ansiattr = cfg->wattr;	 /* Reverse Vido */
                            break;
                        case 0:
							ansiattr = cfg->attr;	 /* default */
                            break;
                        case 1:
			/* ansiattr = cfg->cattr;		Bold */
                            ansiattr |= 8;  /* set bold */
                            break;

                        case 30:    /* black fg      */
                            ansiattr = (ansiattr & 0xf8) | BLACK;
                            break;
                        case 31:    /* red fg        */
                            ansiattr = (ansiattr & 0xf8) | RED;
                            break;
                        case 32:    /* green fg      */
                            ansiattr = (ansiattr & 0xf8) | GREEN;
                            break;
                        case 33:    /* yellow fg     */
                            ansiattr = (ansiattr & 0xf8) | BROWN;
                            break;
                        case 34:    /* blue fg       */
                            ansiattr = (ansiattr & 0xf8) | BLUE;
                            break;
                        case 35:    /* magenta fg    */
                            ansiattr = (ansiattr & 0xf8) | MAGENTA;
                            break;
                        case 36:    /* cyan fg       */
                            ansiattr = (ansiattr & 0xf8) | CYAN;
                            break;
                        case 37:    /* white fg      */
                            ansiattr = (ansiattr & 0xf8) | LIGHTGRAY;
                            break;

                        case 40:    /* black bg     */
                            ansiattr = (ansiattr & 0x8f) | (BLACK << 4);
                            break;
                        case 41:    /* red bg        */
                            ansiattr = (ansiattr & 0x8f) | (RED << 4);
                            break;
                        case 42:    /* green bg      */
                            ansiattr = (ansiattr & 0x8f) | (GREEN << 4);
                            break;
                        case 43:    /* yellow bg     */
                            ansiattr = (ansiattr & 0x8f) | (YELLOW << 4);
                            break;
                        case 44:    /* blue bg       */
                            ansiattr = (ansiattr & 0x8f) | (BLUE << 4);
                            break;
                        case 45:    /* magenta bg    */
                            ansiattr = (ansiattr & 0x8f) | (MAGENTA << 4);
                            break;
                        case 46:    /* cyan bg       */
                            ansiattr = (ansiattr & 0x8f) | (CYAN << 4);
                            break;
                        case 47:    /* white bg      */
                            ansiattr = (ansiattr & 0x8f) | (WHITE << 4);
                            break;

                        default:
                            break;
                    }
                }
                break;
            case 's':       /* save cursor */
                readpos(&c_x, &c_y);
                break;
            case 'u':       /* restore cursor */
                position(c_x, c_y);
                break;
            case 'A':
                readpos(&x, &y);
                if (argc)
                    x -= a[0];
                else
                    x--;
                x = x % (t_info.screenheight - 1);
                position(x, y);
                break;
            case 'B':
                readpos(&x, &y);
                if (argc)
                    x += a[0];
                else
                    x++;
                x = x % (t_info.screenheight - 1);
                position(x, y);
                break;
            case 'D':
                readpos(&x, &y);
                if (argc)
                    y -= a[0];
                else
                    y--;
                y = y % 80;
                position(x, y);
                break;
            case 'C':
                readpos(&x, &y);
                if (argc)
                    y += a[0];
                else
                    y++;
                y = y % 80;		/* shouldn't 80 be the terminal width? */
                position(x, y);
                break;
            case 'f':
            case 'H':
                if (!argc) {
                    position(0, 0);
                    break;
                }
                if (argc == 1) {
                    if (args[0] == ';') {
                        a[1] = a[0];
                        a[0] = 1;
                    } else {
                        a[1] = 1;
                    }
                    argc = 2;
                }
								/* shouldn't 80 be the terminal width? */
                if (argc == 2 && a[0] < t_info.screenheight && a[1] < 80) {
                    position(a[0] - 1, a[1] - 1);
                    break;
                }
            default:
            {
                char str[80];

                sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
				(*stringattr) (0, str, cfg->wattr);
            }
                break;
        }
        if (debug) {
            char str[80];

            sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
			(*stringattr) (0, str, cfg->wattr);
        }
        rc = FALSE;
    } else {
		i = strlen(args);
		args[i] = c;
		args[i + 1] = NULL;
        rc = TRUE;
    }
	return rc;
}

/* -------------------------------------------------------------------- */
/*  save_screen() allocates a buffer and saves the screen               */
/* -------------------------------------------------------------------- */
void save_screen(void)
{
	saveBuffer = farcalloc((ulong) SCR_BUFSIZ, sizeof(char));
    if (saveBuffer)
        memcpy(saveBuffer, screen, SCR_BUFSIZ);
    readpos(&srow, &scolumn);
}


/* -------------------------------------------------------------------- */
/*   restore_screen() restores screen and free's buffer                 */
/* -------------------------------------------------------------------- */
void restore_screen(void)
{
    int size;

    size = SCR_BUFSIZ;
    setscreen();
    size = (SCR_BUFSIZ <= size) ? SCR_BUFSIZ : size;

    if (saveBuffer) {
        memcpy(screen, saveBuffer, SCR_BUFSIZ);
        farfree((void *) saveBuffer);
    }
    if (gmode() != 7)       /* if color display */
		outp(0x03d9, cfg->battr);/* set border color */

    position(srow, scolumn);
}
