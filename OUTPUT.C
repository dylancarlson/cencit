/* -------------------------------------------------------------------- */
/*  OUTPUT.C                  Citadel                                   */
/* -------------------------------------------------------------------- */
/*  This file contains the output functions                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#endif

#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  getWord()       Gets the next word from the buffer and returns it   */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/*  putWord()       Writes one word to modem and console, w/ wordwrap   */
/*  doBS()          does a backspace to modem & console                 */
/*  doCR()          does a return to both modem & console               */
/*  dospCR()        does CR for entry of initials & pw                  */
/*  doTAB()         prints a tab to modem & console according to flag   */
/*  oChar()         is the top-level user-output function (one byte)    */
/*  updcrtpos()     updates crtColumn according to character            */
/*  mPrintf()       sends formatted output to modem & console           */
/*  cPrintf()       send formatted output to console                    */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  05/26/89    (PAT)   Created from MISC.C to break that moduel into
 *                      more managable and logical peices. Also draws
 *                      off MODEM.C and FORMAT.C
 *  03/13/90     FJM    Added IBM Graphics
 *  03/15/90    {zm}    It's called PsYcHo ChIcKeN, so you'll know.
 *  06/16/90     FJM    Made IBM Graphics characters a seperate option.
 *	12/09/90	 FJM	ANSI/ISO color support added!
 *	12/15/90	 FJM	Modifed the way wrapping is done.
 *						Added function mtPrintf()
 *						Reduced input filtering in normalizeString
 *	07/03/91	 BLJ	Added code for sound ^BM (music) and ^BS (sound)
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data/functions                                               */
/* -------------------------------------------------------------------- */
static void ansiCode(char *str);

/* -------------------------------------------------------------------- */
/*  getWord()       Gets the next word from the buffer and returns it   */
/* -------------------------------------------------------------------- */
int getWord(char *dest, char *source, register int offset, int lim)
{
    register int i;     /* , j; */

    /* skip leading blanks if any  */
    for (i = 0; source[offset + i] == ' ' && (i < lim); i++);

    /* step over word              */
    for (; source[offset + i] != ' ' && (i < lim) && source[offset + i]; i++);

    /* pick up any trailing blanks */
    for (; source[offset + i] == ' ' && (i < lim); i++);

    /* copy word over */
    /* for (j  = 0; j < i; j++) */
    strncpy(dest, source + offset, i);
    /* dest[j] = source[offset+j]; */
    dest[i] = 0;        /* null to tie off string */

    return (offset + i);
}

/* -------------------------------------------------------------------- */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/* -------------------------------------------------------------------- */
#define MAXWORD 256
void mFormat(char *string)
{
    char wordBuf[MAXWORD + 8];
    int i;

    for (i = 0; string[i] &&
    (outFlag == OUTOK || outFlag == IMPERVIOUS || outFlag == OUTPARAGRAPH);) {
        i = getWord(wordBuf, string, i, MAXWORD);
        putWord(wordBuf);
        if (mAbort())
            return;
    }
}


/* -------------------------------------------------------------------- */
/*  putWord()       Writes one word to modem and console, w/ wordwrap   */
/* -------------------------------------------------------------------- */
void putWord(char *st)
{
    char *s;
    int newColumn;

    setio(whichIO, echo, outFlag);
    /* calculate word end */
    for (newColumn = crtColumn, s = st; *s; s++) {
        if (*s == '\t')
            while ((++newColumn % 8) != 1);
		else if (*s == 1 || *s == 2)
			--newColumn;	/* don't count ANSI or MUSIC codes  */
		else if (*s > ' ')  /* don't count what we don't print  */
            ++newColumn;
    }
    if (newColumn >= termWidth) /* Wrap words that don't fit */
        doCR();

    for (; *st; ++st) {
		/* check for Ctrl-A codes */
        if (*st == 1) {
            if (*++st) {    /* must have something after ^A */
                termCap(*st);
                continue;
            } else {
                break;
            }
        }
		/* check for Ctrl-B (music) codes */
		if (*st == 2) {
			if (*++st) {	/* must have something after ^B */
				if(logBuf.SOUND && ansiOn) {
					if(*st == 14) {
						outMod(14);
						SoundOn = FALSE;
						if(sound_pos) {
							Sound_Buffer[sound_pos] = '\0';
							sound_pos = 0;
							outFlag = OUTOK;
							if(Music)
								play();
							else
								sound_effect();
						}
					}
					else {
						outFlag = IMPERVIOUS;
						termCap(*st);
					}
				}
				else {
					while(*st && *st != 2) st++;
					st++;	 /* bypass the ^B		 */
					st++;	 /* bypass the 0x0E -   */
				}
				continue;
            } else {
                break;
            }
        }
        /* worry about words longer than a line:   */
        if (crtColumn > termWidth)
            doCR();

        if ((prevChar != '\n') || (*st > ' ')) {
            oChar(*st);
        } else {
        /* end of paragraph: */
            if (outFlag == OUTPARAGRAPH) {
                outFlag = OUTOK;
            }
            doCR();
			oChar(*st);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  termCap()       Does a terminal command                             */
/* -------------------------------------------------------------------- */
void termCap(char c)
{
    if  (!ansiOn)
        return;

    switch (c) {
    /* foreground color settings */
        case TERM_BLACK_FG:
            ansiCode("30m");
            ansiattr = (ansiattr & 0xf8) | BLACK;
            break;
        case TERM_RED_FG:
            ansiCode("31m");
            ansiattr = (ansiattr & 0xf8) | RED;
            break;
        case TERM_GREEN_FG:
            ansiCode("32m");
            ansiattr = (ansiattr & 0xf8) | GREEN;
            break;
        case TERM_YELLOW_FG:
            ansiCode("33m");
            ansiattr = (ansiattr & 0xf8) | BROWN;
            break;
        case TERM_BLUE_FG:
            ansiCode("34m");
            ansiattr = (ansiattr & 0xf8) | BLUE;
            break;
        case TERM_MAGENTA_FG:
            ansiCode("35m");
            ansiattr = (ansiattr & 0xf8) | MAGENTA;
            break;
        case TERM_CYAN_FG:
            ansiCode("36m");
            ansiattr = (ansiattr & 0xf8) | CYAN;
            break;
        case TERM_WHITE_FG:
            ansiCode("37m");
            ansiattr = (ansiattr & 0xf8) | LIGHTGRAY;
            break;

    /* background color settings */
        case TERM_BLACK_BG:
            ansiCode("40m");
            ansiattr = (ansiattr & 0x8f) | (BLACK << 4);
            break;
        case TERM_RED_BG:
            ansiCode("41m");
            ansiattr = (ansiattr & 0x8f) | (RED << 4);
            break;
        case TERM_GREEN_BG:
            ansiCode("42m");
            ansiattr = (ansiattr & 0x8f) | (GREEN << 4);
            break;
        case TERM_YELLOW_BG:
            ansiCode("43m");
            ansiattr = (ansiattr & 0x8f) | (BROWN << 4);
            break;
        case TERM_BLUE_BG:
            ansiCode("44m");
            ansiattr = (ansiattr & 0x8f) | (BLUE << 4);
            break;
        case TERM_MAGENTA_BG:
            ansiCode("45m");
            ansiattr = (ansiattr & 0x8f) | (MAGENTA << 4);
            break;
        case TERM_CYAN_BG:
            ansiCode("46m");
            ansiattr = (ansiattr & 0x8f) | (CYAN << 4);
            break;
        case TERM_WHITE_BG:
            ansiCode("47m");
            ansiattr = (ansiattr & 0x8f) | (LIGHTGRAY << 4);
            break;

    /* attribute settings */
        case TERM_BLINK:
            ansiCode("5m");
            ansiattr = ansiattr | 128;
            break;
        case TERM_REVERSE:
            ansiCode("7m");
			ansiattr = cfg->wattr;	 /* these cfg references have to change now */
            break;
        case TERM_BOLD:
            ansiCode("1m");
			ansiattr = cfg->cattr;
            break;
        case TERM_UNDERLINE:
            ansiCode("4m");
			ansiattr = cfg->uttr;
            break;
		case 'M':  /*  Music        */
			ansiCode("MF");
			SoundOn = TRUE;
			sound_pos = 0;
			Music = TRUE;
			break;
		case 'S':  /*  Sound Effect */
			ansiCode("MZ");
			SoundOn = TRUE;
			sound_pos = 0;
			Music = FALSE;
            break;
		case TERM_NORMAL:
        default:
            ansiCode("0m");
			ansiattr = cfg->attr;
            break;
    }
	if(debug) {
		oChar('^');
		oChar('A');
		oChar(c);
	}
}

/* -------------------------------------------------------------------- */
/*   ansiCode() adds the escape if needed                               */
/* -------------------------------------------------------------------- */
static void ansiCode(char *str)
{
    char tmp[30], *p;

    if (!ansiOn)
        return;

    sprintf(tmp, "%c[%s", 27, str);

    p = tmp;

    while (*p) {
        outMod(*p++);
    }
}

/* -------------------------------------------------------------------- */
/*  doBS()          does a backspace to modem & console                 */
/* -------------------------------------------------------------------- */
void doBS(void)
{
    oChar('\b');
    oChar(' ');
    oChar('\b');
}

/* -------------------------------------------------------------------- */
/*  doCR()          does a return to both modem & console               */
/* -------------------------------------------------------------------- */
void doCR(void)
{
    static numLines = 0;

    crtColumn = 1;

    setio(whichIO, echo, outFlag);

    domcr();
    doccr();

    if (printing)
        fprintf(printfile, "\n");

    prevChar = ' ';

    /* pause on full screen */
    if (logBuf.linesScreen) {
        if (outFlag == OUTOK) {
            numLines++;
            if (numLines == logBuf.linesScreen) {
                outFlag = OUTPAUSE;
                mAbort();
                numLines = 0;
            }
        } else {
            numLines = 0;
        }
    } else {
        numLines = 0;
    }
}

/* -------------------------------------------------------------------- */
/*  dospCR()        does CR for entry of initials & pw                  */
/* -------------------------------------------------------------------- */
void dospCR(void)
{
    char oldecho;

    oldecho = echo;

    echo = BOTH;
    setio(whichIO, echo, outFlag);

	if (cfg->nopwecho == 1)
        doCR();
    else {
        if (onConsole) {
            if (gotCarrier())
                domcr();
        } else
            doccr();
    }
    echo = oldecho;
}

/* -------------------------------------------------------------------- */
/*  doTAB()         prints a tab to modem & console according to flag   */
/* -------------------------------------------------------------------- */
void doTAB(void)
{
    int column, column2;

    column = crtColumn;
    column2 = crtColumn;

    do {
        outCon(' ');
    } while ((++column % 8) != 1);

    if (haveCarrier) {
        if (termTab)
            outMod('\t');
        else
            do {
                outMod(' ');
            }
        while ((++column2 % 8) != 1);
    }
    updcrtpos('\t');
}

/* -------------------------------------------------------------------- */
/*  echocharacter() echos bbs input according to global flags           */
/* -------------------------------------------------------------------- */
void echocharacter(char c)
{
    setio(whichIO, echo, outFlag);

    if (echo == NEITHER) {
        if (echoChar != '\0') {
            echo = BOTH;
            if (c == '\b')
                doBS();
            else if (c == '\t')
                doTAB();
            else if (c == '\n') {
                echo = CALLER;
                doCR();
            } else
                oChar(echoChar);
            echo = NEITHER;
        }
    } else if (c == '\b')
        doBS();
    else if (c == '\n')
        doCR();
    else
        oChar(c);
}

/* -------------------------------------------------------------------- */
/*  oChar()         is the top-level user-output function (one byte)    */
/*        sends to modem port and console both                          */
/*        does conversion to upper-case etc as necessary                */
/*        in "debug" mode, converts control chars to uppercase letters  */
/*      Globals modified:       prevChar                                */
/* -------------------------------------------------------------------- */
void oChar(unsigned char c)
{
	static int UpDoWn = TRUE;

    if (!IBMOn)
        c = filt_out[c];

    prevChar = c;       /* for end-of-paragraph code    */

    if (c == 1)
        c = 0;
	else if (c == '\t') {
        doTAB();
    } else {
		if (c == '\n')
			c = ' ';        /* doCR() handles real newlines */
		else if (termUpper)
			c = (char) toupper(c);
		else if (backout) {
			if (UpDoWn)
				c = (char) toupper(c);
			else
				c = (char) tolower(c);
			UpDoWn = !UpDoWn;
		}
		/* show on console */
		if (console) {
			if(SoundOn) {
				Sound_Buffer[sound_pos++] = c;
			} else {
				outCon(c);
			}
		}
	
		/* show on printer */
		if (printing)
			fputc(c, printfile);
	
		/* send out the modem  */
		if (haveCarrier && modem)
			outMod(c);
	
		updcrtpos(c);       /* update CRT column count */
	}
}

/* -------------------------------------------------------------------- */
/*  updcrtpos()     updates crtColumn according to character            */
/* -------------------------------------------------------------------- */
void updcrtpos(char c)
{
    if  (c == '\b')
        crtColumn--;
    else if (c == '\t')
        while ((++crtColumn % 8) != 1);
    else if ((c == '\n') || (c == '\r'))
        crtColumn = 1;
    else
        crtColumn++;
}

/* -------------------------------------------------------------------- */
/*  mPrintf()       sends formatted output to modem & console           */
/* -------------------------------------------------------------------- */
void mPrintf(char *fmt,...)
{
    char buff[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    mFormat(buff);
}

/* --------------------------------------------------------------------
 *  mpPrintf()       sends menu prompt output to modem & console
 * -------------------------------------------------------------------- */
void mpPrintf(char *fmt,...)
{
    char buff[128], buff2[132];
    va_list ap;
	char *p,*p2;
	int markers = 0;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

	if (ansiOn) {
		for (p=buff,p2=buff2;*p;++p,++p2) {
			if (*p == '<' && markers == 0) {	/* parse first marker */
				*p2++ = 1;		/* ^A */
				*p2 = TERM_BOLD;
				++markers;
			} else if (*p == '>' && markers == 1) {
				*p2++ = 1;		/* ^A */
				*p2 = TERM_NORMAL;
				++markers;
			} else {
				*p2 = *p;
			}
		}
		*p2 = '\0';
		mFormat(buff2);
	} else
		mFormat(buff);
}

/* --------------------------------------------------------------------
 *  mtPrintf()      Sends formatted output to modem & console
 *                  with termCap attributes.  Resets attributes when
 *                  done.
 * -------------------------------------------------------------------- */
void mtPrintf(char attr, char *fmt,...)
{
    char buff[256];
    va_list ap;

    termCap(attr);
    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    mFormat(buff);
    termCap(TERM_NORMAL);
}

/* -------------------------------------------------------------------- */
/*  cPrintf()       send formatted output to console                    */
/* -------------------------------------------------------------------- */
void cPrintf(char *fmt,...)
{
    char buff[256];
    char *buf = buff;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    while (*buf) {
        outCon(*buf++);
    }
}

/* -------------------------------------------------------------------- */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */
void cCPrintf(char *fmt,...)
{
    char buff[256];
    va_list ap;
    int i;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    i = (80 - strlen(buff)) / 2;

    strrev(buff);

    while (i--)
        strcat(buff, " ");

    strrev(buff);

	(*stringattr) (wherey() - 1, buff, cfg->attr);
}

/* -------------------------------------------------------------------- */
/*  old  cCPrintf()      send formatted output to console, centered     */
/* -------------------------------------------------------------------- */
/*
void cCPrintf(char *fmt, ... )
{
    char buff[256];
    char register *buf = buff;
    va_list ap;
    int i;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    i = (80 - strlen(buff)) / 2;

    while(i--)
    {
        outCon(' ');
    }

    while(*buf)
    {
        outCon(*buf++);
    }
}
*/

/* EOF */
