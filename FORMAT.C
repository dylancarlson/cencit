/* -------------------------------------------------------------------- */
/*  FORMAT.C                  Citadel                                   */
/* -------------------------------------------------------------------- */
/*  Contains string handling stuff                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "proto.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  sformat()       Configurable format                                 */
/*  normalizeString() deletes leading & trailing blanks etc.            */
/*  parse_it()      routines to parse strings separated by white space  */
/*  qtext()         Consumes quoted strings and expands escape chars    */
/*  strpos()        find a character in a string                        */
/*  u_match()       Unix wildcarding                                    */
/*  cclass()        Used with u_match()                                 */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 *  Version control log:
 * $Log:   H:/VCS/FCIT/FORMAT.C_V  $
 * 
 *    Rev 1.41   19 Apr 1991 23:38:46   FJM
 * No change.
 * 
 *    Rev 1.40   19 Apr 1991 23:26:42   FJM
 * No change.
 * 
 *    Rev 1.39   10 Apr 1991  9:05:04   FJM
 * No change.
 * 
 *    Rev 1.38   09 Apr 1991  7:14:20   FJM
 * Fix to sformat() suggested by Joo Sama
 *
 *    Rev 1.37   04 Apr 1991 14:12:54   FJM
 * No change.
 *
 *    Rev 1.36   13 Mar 1991 15:13:02   FJM
 * No change.
 *
 *    Rev 1.35   13 Mar 1991 15:00:38   FJM
 * No change.
 *
 *    Rev 1.34   10 Feb 1991 18:01:56   FJM
 * No change.
 *
 *    Rev 1.33   10 Feb 1991 17:33:18   FJM
 * No change.
 *
 *    Rev 1.32   05 Feb 1991 14:50:44   FJM
 * No change.
 *
 *    Rev 1.31   05 Feb 1991 14:31:18   FJM
 * No change.
 *
 *    Rev 1.30   19 Jan 1991 14:32:26   FJM
 * No change.
 *
 *    Rev 1.29   19 Jan 1991 14:15:14   FJM
 * No change.
 *
 *    Rev 1.28   18 Jan 1991 17:33:24   FJM
 * No change.
 *
 *    Rev 1.27   18 Jan 1991 17:20:08   FJM
 * No change.
 *
 *    Rev 1.26   18 Jan 1991 16:49:38   FJM
 * Attempts at locating the JNPS ANSI bug.
 *
 *    Rev 1.18   06 Jan 1991 12:45:20   FJM
 * Porting, formating, fixes and misc changes.
 *
 *    Rev 1.17   27 Dec 1990 20:16:16   FJM
 * Changes for porting.
 *
 *    Rev 1.14   22 Dec 1990 13:38:36   FJM
 *
 *    Rev 1.8   15 Dec 1990 22:42:10   FJM
 * Many speedups.  Moved substr() code to rooma.c since it's used
 * only in that overlay.
 *
 *    Rev 1.6   07 Dec 1990 23:10:16   FJM
 *
 *    Rev 1.4   23 Nov 1990 13:24:54   FJM
 * Header change
 *
 *    Rev 1.2   17 Nov 1990 16:11:44   FJM
 * Added version control log header
 *
 * --------------------------------------------------------------------
 *  EARLY HISTORY:
 *
 *  05/26/89    (PAT)   Cleanup, history created
 *  03/19/90    FJM     Linted & partial cleanup
 *  03/30/90    FJM     MUCH faster strpos()
 *  08/02/90    FJM     Made ch arg to strpos() an int
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  sformat()       Configurable format
 * --------------------------------------------------------------------
 *
 *  sformat, a 10 minute project
 *  by Peter Torkelson
 *
 *    passes a number of arguments this lets you make configurable things..
 *    call it with:
 *      str    a string buffer
 *      fmt    the format string
 *      val    valid % escapes
 *    here is an example:
 *      sformat(str, "Hello %n, I am %w. (%%, %c)", "nw", "you", "me");
 *    gives you:
 *      "Hello you, I am me. (%, (NULL))"
 *
 * -------------------------------------------------------------------- */

void sformat(char *str, char *fmt, char *val,...)
{
    int i;

    char s[2];
    va_list ap;

    s[1] = 0;
    *str = 0;

    while (*fmt) {
        if (*fmt != '%') {
            *s = *fmt;
            strcat(str, s);
        } else {
            fmt++;
            if (*fmt == '\0')   /* "somthing %", not nice */
                return;
            if (*fmt == '%') {  /* %% = %                 */
                *s = *fmt;
                strcat(str, s);
            } else {        /* it must be a % something */
                i = strpos(*fmt, val) - 1;
                if (i != -1) {
                    va_start(ap, val);
                    while (i--)
                        va_arg(ap, char *);

                    strcat(str, va_arg(ap, char *));
                    va_end(ap);
                } else {
                    strcat(str, "(NULL)");
                }

            }           /* fmt == '%' */

        }           /* first fmt == '%' */

        fmt++;

    }               /* end while */
}

/* -------------------------------------------------------------------- */
/*  normalizeString() deletes leading & trailing blanks etc.            */
/* -------------------------------------------------------------------- */
void normalizeString(char *s)
{
    char *pc;

    pc = s;

    /* find end of string   */
    while (*pc) {
        if (*pc < ' ' && (*pc != 1))    /* less then space & not ^A */
            *pc = ' ';      /* zap tabs etc... */
        pc++;
    }

    /* no trailing spaces: */
    while (pc > s && isspace(*(pc - 1)))
        pc--;
    *pc = '\0';

    /* no leading spaces: */
    while (isspace(*s))
        strcpy(s, s + 1);

    /* no double blanks */
    for (; *s;)
        if (isspace(*s) && isspace(*(s + 1)))
            strcpy(s, s + 1);
        else
            s++;
}

/* -------------------------------------------------------------------- */
/*  parse_it()      routines to parse strings separated by white space  */
/* -------------------------------------------------------------------- */
/*                                                                      */
/* strategy:  called as                                                 */
/*            count = parse_it(workspace,input);                        */
/*                                                                      */
/* where workspace is a two-d char array of the form:                   */
/*                                                                      */
/* char *workspace[MAXWORD];                                            */
/*                                                                      */
/* and input is the input string to be parsed.  it returns              */
/* the actual number of things parsed.                                  */
/*                                                                      */
/* -------------------------------------------------------------------- */
int parse_it(char *words[], char input[])
{
    /* states of machine... */
#define INWORD          0
#define OUTWORD         1
#define INQUOTES        2

    /* characters */
#define QUOTE           '\"'
#define QUOTE2          '\''
#define MXWORD         128

    int i, state, thisword;

    input[strlen(input) + 1] = 0;   /* double-null */

    for (state = OUTWORD, thisword = i = 0; input[i]; i++) {
        switch (state) {
            case INWORD:
        /* FJM - speedup */
                while (input[i] && !isspace(input[i]))
                    ++i;
                if (!input[i])
                    continue;
                input[i] = '\0';
                state = OUTWORD;
                break;
            case OUTWORD:
                if (input[i] == QUOTE || input[i] == QUOTE2) {
                    state = INQUOTES;
                } else if (!isspace(input[i])) {
                    state = INWORD;
                }
        /* if we are now in a string, setup, otherwise, break */

                if (state != OUTWORD) {
                    if (thisword >= MXWORD) {
                        return thisword;
                    }
                    if (state == INWORD) {
                        words[thisword++] = (input + i);
                    } else {
                        words[thisword++] = (input + i + 1);
                    }
                }
                break;
            case INQUOTES:
                i += qtext(input + i, input + i, input[i - 1]);
                state = OUTWORD;
                break;
        }
    }
    return thisword;
}

/* -------------------------------------------------------------------- */
/*  qtext()         Consumes quoted strings and expands escape chars    */
/* -------------------------------------------------------------------- */
int qtext(char *buf, char *line, char end)
{
    int index = 0;
    int slash = 0;
    char chr;

    while (line[index] && (line[index] != end || slash)) {
        if (slash == 0) {
            if (line[index] == '\\') {
                slash = 1;
            } else if (line[index] == '^') {
                slash = 2;
            } else {
                *(buf++) = line[index];
            }
        } else if (slash == 1) {
            switch (line[index]) {
                default:
                    *(buf++) = line[index];
                    break;
                case 'n':       /* newline */
                    *(buf++) = '\n';
                    break;
                case 't':       /* tab */
                    *(buf++) = '\t';
                    break;
                case 'r':       /* carriage return */
                    *(buf++) = '\r';
                    break;
                case 'f':       /* formfeed */
                    *(buf++) = '\f';
                    break;
                case 'b':       /* backspace */
                    *(buf++) = '\b';
                    break;
            }
            slash = 0;
        } else {        /* if (slash == 2 ) */
            if (line[index] == '?') {
                chr = 127;
            } else if (line[index] >= 64 && line[index] < 96) {
                chr = line[index] - 64;
            } else {
                chr = line[index];
            }

            *(buf++) = chr;
            slash = 0;
        }

        index++;
    }

    *buf = 0;
    return line[index] == end ? index + 1 : index;
}

/* -------------------------------------------------------------------- */
/*  strpos()        find a character in a string                        */
/* -------------------------------------------------------------------- */

int strpos(int ch, char *str)
{
    char *p;

    p = strchr(str, ch);
    return p ? p - str + 1 : 0;
}

/* -------------------------------------------------------------------- */
/*  u_match()       Unix wildcarding                                    */
/* -------------------------------------------------------------------- */
/*
 * int u_match(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 */

#define        CMASK   0377
#undef  QUOTE
#define QUOTE   0200
#define        QMASK   (CMASK&~QUOTE)
#define        NOT     '!'  /* less then sp */

static  char    *cclass(register char *p, register int sub);

int u_match(register char *s, register char *p)
{
    int sc, pc;

    if (s == NULL || p == NULL) return(0); while ((pc = *p++ & CMASK) != '\0') { sc = *s++ & QMASK;
        switch (pc) {
            case '[':
                if ((p = cclass(p, sc)) == NULL)
                    return(0);
                break;

            case '?':
                if (sc == 0)
                    return(0);
                break;

            case '*':
                s--;
                do {
                    if (*p == '\0' || u_match(s, p))
                        return(1);
                } while (*s++ != '\0');
                return(0);

            default:
                if (tolower(sc) != (tolower(pc&~QUOTE)))
                    return(0);
        }
    }
    return(*s == 0);
}

/* -------------------------------------------------------------------- */
/*  cclass()        Used with u_match()                                 */
/* -------------------------------------------------------------------- */
static char *cclass(register char *p, register int sub)
{
    int c, d, not, found;

    sub = tolower(sub);

    if ((not = *p == NOT) != 0)
        p++;
    found = not;
    do {
        if (*p == '\0')
            return (NULL);
        c = tolower(*p & CMASK);
        if (p[1] == '-' && p[2] != ']') {
            d = tolower(p[2] & CMASK);
            p++;
        } else
            d = c;
        if (c == sub || c <= sub && sub <= d)
            found = !not;
    } while (*++p != ']');
    return (found ? p + 1 : NULL);
}

/* EOF */
