/* -------------------------------------------------------------------- */
/*  EDIT.C                   Citadel                                    */
/* -------------------------------------------------------------------- */
/*                Message editor and related code.                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  editText()      handles the end-of-message-entry menu.              */
/*  putheader()     prints header for message entry                     */
/*  getText()       reads a message from the user                       */
/*  matchString()   searches for match to given string.                 */
/*  replaceString() corrects typos in message entry                     */
/*  wordcount()     talleys # lines, word & characters message contains */
/*  ismostlyupper() tests if string is mostly uppercase.                */
/*  fakeFullCase()  converts an uppercase-only message to mixed case.   */
/*  xPutStr()       Put a string to a file w/o trailing blank           */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 * HISTORY:
 *
 *  06/06/89    (PAT)   Made history, cleaned up comments, reformated
 *                      icky code.
 *  06/18/89    (LWE)   Added wordwrap to message entry
 *  03/07/90    {zm}    Added ismostlyupper()
 *  03/12/90    FJM     Added IBM Graphics character translation
 *  03/15/90    {zm}    Add [title] name [surname], 30 characters long.
 *  03/16/90    FJM     Made graphics entry work
 *  03/19/90    FJM     Linted & partial cleanup
 *	11/24/90	FJM 	Changes for shell/door mode.
 *	12/09/90	FJM 	Now allows ^Aa-^Ah, ^AA-^AH in messages.
 *	01/06/91	FJM 	Porting, formating, fixes and misc changes.
 *	01/13/91	FJM 	Name overflow fixes.
 *	01/28/91	FJM 	Made edit prompt inverse.  Made edit/continue
 *						impervious.
 *	02/10/91	FJM 	Fixes for editor prompt.
 *	07/03/91	BLJ 	Allow ^BM and ^BS (Music and Sound) in messages
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  editText()      handles the end-of-message-entry menu.              */
/*      return TRUE  to save message to disk,                           */
/*             FALSE to abort message, and                              */
/*             ERROR if user decides to continue                        */
/* -------------------------------------------------------------------- */
int editText(char *buf, int lim, uchar sound_in_message)
{
	char ch, x, i, stuff[40];
    FILE *fd;
    int eom;

    dowhat = PROMPT;

    do {
        outFlag = IMPERVIOUS;
        while (MIReady())   /* flush modem input buffer */
            getMod();
		doCR();
		strcpy(gprompt,"Entry command:");	/* in case of ^A? */
        mtPrintf(TERM_REVERSE,gprompt);
		mPrintf(" ");
        switch (ch = (char) toupper(iChar())) {
            case 'A':
                mPrintf("\bAbort\n ");
                if (!strlen(buf))
                    return FALSE;
                else if (getYesNo(confirm, 0)) {
                    heldMessage = TRUE;

                    memcpy(msgBuf2, msgBuf, sizeof(struct msgB));

                    dowhat = DUNO;
                    return FALSE;
                }
                break;
            case 'C':
                mPrintf("\bContinue");
				/* dump message to display */
				outFlag = OUTOK;
                doCR();
                putheader();
                doCR();
                mFormat(buf);
                doBS();
                eom = strlen(buf);
                if (eom > 0)
                    buf[eom - 1] = '\0';    /* zap last character ('\n') */
				outFlag = IMPERVIOUS;
                return ERROR;				/* to return to this routine */
            case 'F':
                mPrintf("\bFind & Replace text\n ");
                replaceString(buf, lim, TRUE);
                break;
            case 'P':
                mPrintf("\bPrint formatted\n ");
                doCR();
                outFlag = OUTOK;
                putheader();
                doCR();
                mFormat(buf);
                termCap(TERM_NORMAL);
                doCR();
                break;
            case 'R':
                mPrintf("\bReplace text\n ");
                replaceString(buf, lim, FALSE);
                break;
            case 'S':
                mPrintf("\bSave buffer\n ");
                entered++;      /* increment # messages entered */
				if(sound_in_message) Sound_Entries++;
				dowhat = DUNO;
                return TRUE;
            case 'W':
                mPrintf("\bWord count\n ");
                wordcount(buf);
                break;
			case '*':
				if (sysop)
				{
					mPrintf("Name Change"); doCR();

					getString("title", stuff,
							  NAMESIZE, FALSE, ECHO, msgBuf->mbtitle);

					if (*stuff)
					{
						strcpy(msgBuf->mbtitle, stuff);
						normalizeString(msgBuf->mbtitle);
					}


					strcpy(stuff, msgBuf->mbauth);
					getString("name", stuff,
							  NAMESIZE, FALSE, ECHO, msgBuf->mbauth);
					if (*stuff)
					{
						strcpy(msgBuf->mbauth, stuff);
						normalizeString(msgBuf->mbauth);
					}


					strcpy(stuff, msgBuf->mbsur);
					getString("surname", stuff,
							  NAMESIZE, FALSE, ECHO, msgBuf->mbsur);
					if (*stuff)
					{
						strcpy(msgBuf->mbsur, stuff);
						normalizeString(msgBuf->mbsur);
					}

					break;
				}

			case '?':
                tutorial("edit.mnu", 1);
                break;
            default:
                if ((x = (char) strpos((char) tolower(ch), editcmd)) != 0) {
                    x--;
					edit = start_edit;
					for(i=0; i<x; i++) edit = edit->next;

					mPrintf("\b%s", edit->ed_name);
                    doCR();
					if (edit->ed_local && !onConsole) {
                        mPrintf("\n Local editor only!\n ");
                    } else {
						changedir(cfg->aplpath);
                        if ((fd = fopen("message.apl", "wb")) != NULL) {
                            xPutStr(fd, msgBuf->mbtext);
                            fclose(fd);
                        }
                        readMessage = FALSE;
						extFmtRun(edit->ed_cmd, "message.apl");
						changedir(cfg->aplpath);
                        if ((fd = fopen("message.apl", "rb")) != NULL) {
							GetStr(fd, msgBuf->mbtext, cfg->maxtext);
                            fclose(fd);
                            unlink("message.apl");
                        }
                    }
                    break;
                }
                if (!expert)
                    tutorial("edit.mnu", 1);
                else
                    mPrintf("\n '?' for menu.\n \n");
                break;
        }
    } while (!ExitToMsdos && (haveCarrier || onConsole));
    dowhat = DUNO;
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  putheader()     prints header for message entry                     */
/* -------------------------------------------------------------------- */
void putheader(void)
{
    char dtstr[80];

	cit_strftime(dtstr, 79, cfg->datestamp, 0L);

    termCap(TERM_BOLD);
    mPrintf("    %s", dtstr);
    if (loggedIn) {
        if (msgBuf->mbtitle[0]) {
            mPrintf(" From [%s] %s", msgBuf->mbtitle, msgBuf->mbauth);
        } else {
            mPrintf(" From %s", msgBuf->mbauth);
        }
        if (msgBuf->mbsur[0]) {
            mPrintf(" [%s]", msgBuf->mbsur, msgBuf->mbauth);
        }
    }
    if (msgBuf->mbto[0])
        mPrintf(" To %s", msgBuf->mbto);
    if (msgBuf->mbzip[0])
        mPrintf(" @ %s", msgBuf->mbzip);
	if (msgBuf->mbtreg[0]) {
		mPrintf(", %s", msgBuf->mbtreg);
		if (msgBuf->mbtcont[0])
			mPrintf(", %s", msgBuf->mbtcont);
    }
	else {
		if (msgBuf->mbrzip[0])
			mPrintf(", %s", msgBuf->mbrzip);
		if (msgBuf->mbszip[0])
			mPrintf(", %s", msgBuf->mbszip);
		if (msgBuf->mbczip[0])
			mPrintf(", %s", msgBuf->mbczip);
	}
	if (msgBuf->mbfwd[0])
        mPrintf(" Forwarded to %s", msgBuf->mbfwd);
    if (msgBuf->mbgroup[0])
        mPrintf(" (%s Only)", msgBuf->mbgroup);
    termCap(TERM_NORMAL);
}

/* -------------------------------------------------------------------- */
/*  getText()       reads a message from the user                       */
/*                  Returns TRUE if user decides to save it, else FALSE */
/* -------------------------------------------------------------------- */
BOOL getText(void)
{
#define MAXEWORD 50
    unsigned int temp, done, d, c = 0;
    char *buf, beeped = FALSE;
    int i, toReturn, lim, wsize = 0, j;
    unsigned int lastCh;
	unsigned char Sound = FALSE;
	unsigned char sound_in_message = FALSE;
	unsigned char word[MAXEWORD];
    static int ecount = 0;

    if (!expert) {
        nextblurb("entry", &ecount, 1);
        outFlag = OUTOK;
        doCR();
		mPrintf(" You may enter up to %d characters.", cfg->maxtext);
        mPrintf("\n Please enter message.  Use an empty line to end.");
    }
    outFlag = IMPERVIOUS;

    doCR();
    putheader();
    doCR();

    buf = msgBuf->mbtext;

    if (oldFlag) {
        for (i = strlen(buf); i > 0 && buf[i - 1] == 10; i--)
            buf[i - 1] = 0;
        mFormat(msgBuf->mbtext);
    }
    outFlag = OUTOK;
	lim = cfg->maxtext - 1;

    done = FALSE;

	do {
        i = strlen(buf);
        if (i)
            c = buf[i];
        else
            c = '\n';

        while (!done && i < lim && (!ExitToMsdos && (haveCarrier || onConsole))) {
            if (i)
                lastCh = c;

            c = iChar();

			switch (c) {	/* Analyze what they typed		 */
                case 1:     /* CTRL-A>nsi   */
                    temp = echo;
                    echo = NEITHER;
                    d = (char) iChar();
                    echo = temp;

                    if (d >= '0' && toupper(d) <= 'H' && ansiOn) {
                        if (d == '?') { /* ansi help */
                            mPrintf("ansi.hlp");
                            tutorial("ansi.hlp", 1);
                        } else {
                            termCap(d);
                            buf[i++] = 0x01;
                            buf[i++] = d;
                        }
                    } else {
						oChar('\b');
						oChar(7);
                    }
                    break;
				case 2: 	/* CTRL-B (Music or sound)	 */
					temp = echo;
                    echo = NEITHER;
                    d = (char) iChar();
					d = toupper(d);
					echo = temp;

					if ((d == 'M' || d == 'S' || d == 'E'
						|| d == '\?')
						&& Sound_Entries < cfg->MaxSound
						&& ansiOn && logBuf.SOUND) {
						if (d == '?') { /* music help */
							mPrintf("music.hlp");
							tutorial("music.hlp", 1);
                        } else {
							buf[i++] = 0x02;
							if(d == 'E') {
								d = 14;
								Sound = FALSE;
							}
							else {
								Sound = TRUE;
								sound_in_message = TRUE;
							}
							buf[i++] = d;
                        }
                    } else {
						oChar('\b');
						oChar(7);
                    }
                    break;
                case '\n':      /* NEWLINE      */
                    if ((lastCh == '\n') || (i == 0))
                        done = TRUE;
                    if (!done)
                        buf[i++] = 10;  /* A CR might be nice   */
                    break;
                case 27:        /* An ESC? No, No, NO!  */
                    oChar('\a');
                    break;
                case 0x1a:      /* CTRL-Z       */
                    done = TRUE;
                    entered++;  /* increment # messages entered */
                    break;
                case '\b':      /* CTRL-H bkspc */
                    if ((i > 0) && (buf[i - 1] == '\t')) {  /* TAB  */
                        i--;
                        while ((crtColumn % 8) != 1)
                            doBS();
                    } else if ((i > 0) && (buf[i - 1] != '\n')) {
                        i--;
                        if (wsize > 0)
                            wsize--;
                    } else {
                        oChar(32);
                        oChar('\a');
                    }
                    break;
                default:        /* '\r' and '\n' never get here */
					if(Sound) c = toupper(c);

					if ((c == ' ') || (c == '\t') || (wsize == MAXEWORD)) {
                        wsize = 0;
                    } else if (crtColumn >= (termWidth - 1)) {
                        if (wsize) {
                            for (j = 0; j < (wsize + 1); j++)
                                doBS();
                            doCR();
                            for (j = 0; j < wsize; j++)
                                echocharacter(word[j]);
                            echocharacter(c);
                        } else {
                            doBS();
                            doCR();
                            echocharacter(c);
                        }
                        wsize = 0;
                    } else {
                        word[wsize] = c;
                        wsize++;
                    }

                    if (c != 0)
                        buf[i++] = c;

					if (i > cfg->maxtext - 80 && !beeped) {
                        beeped = TRUE;
                        oChar('\a');
                    }
                    break;
            }

            buf[i] = '\0';  /* null to terminate message */
            if (i)
                lastCh = buf[i - 1];

			if (i == lim-4)
                mPrintf(" Buffer overflow.\n ");
        }
        done = FALSE;       /* In case they Continue */

        termCap(TERM_NORMAL);

		if (c == 0x1a) {	/* if was CTRL-Z */
			if(i >= lim-4) i = lim-4;
			if(Sound) {
				buf[i++] = 0x02;
				buf[i++] = 0x0E;
			}
			buf[i++] = '\n';    /* end with NEWLINE */
            buf[i] = '\0';
            toReturn = TRUE;    /* yes, save it */
            doCR();
            mPrintf(" Saving message");
            doCR();
		} else {		  /* done or lost carrier */
			if(Sound) {
				buf[i++] = 0x02;
				buf[i++] = 0x0E;
				buf[i++] = '\n';
				buf[i] = '\0';
			}
			toReturn = editText(buf, lim, sound_in_message);
		}
    } while ((toReturn == ERROR) && (!ExitToMsdos && (haveCarrier || onConsole)));
    /* ERROR returned from editor means continue    */

    if (toReturn == TRUE) { /* Filter null messages */
        toReturn = FALSE;
        for (i = 0; buf[i] != 0 && !toReturn; i++)
            toReturn = (buf[i] > ' ' && buf[i] < 127);
    }
    return (BOOL) toReturn;
}

/* -------------------------------------------------------------------- */
/*  matchString()   searches for match to given string.                 */
/*                  Runs backward  through buffer so we get most recent */
/*                  error first. Returns loc of match, else ERROR       */
/* -------------------------------------------------------------------- */
char *matchString(char *buf, char *pattern, char *bufEnd, char ver)
{
    char *loc, *pc1, *pc2;
    char subbuf[11];
    char foundIt;

	subbuf[10] = '\0';
    for (loc = bufEnd, foundIt = FALSE; !foundIt && --loc >= buf;) {
        for (pc1 = pattern, pc2 = loc, foundIt = TRUE; *pc1 && foundIt;) {
            if (!(tolower(*pc1++) == tolower(*pc2++)))
                foundIt = FALSE;
        }
        if (ver && foundIt) {
            doCR();
            strncpy(subbuf,
            buf + 10 > loc ? buf : loc - 10,
            (unsigned) (loc - buf) > 10 ? 10 : (unsigned) (loc - buf));
            subbuf[(unsigned) (loc - buf) > 10 ? 10 : (unsigned) (loc - buf)] = 0;
            mPrintf("%s", subbuf);
            if (ansiOn)
                termCap(TERM_BOLD);
            else
                mPrintf(">");
            mPrintf("%s", pattern);
            if (ansiOn)
                termCap(TERM_NORMAL);
            else
                mPrintf("<");
            strncpy(subbuf, loc + strlen(pattern), 10);
            subbuf[10] = 0;
            mPrintf("%s", subbuf);
            if (!getYesNo("Replace", 0))
                foundIt = FALSE;
        }
    }
    return foundIt ? loc : NULL;
}

/* -------------------------------------------------------------------- */
/*  replaceString() corrects typos in message entry                     */
/* -------------------------------------------------------------------- */
void replaceString(char *buf, int lim, char ver)
{
    char oldString[256];
    char newString[256];
    char *loc, *textEnd;
    char *pc;
    int incr, length;

    /* find terminal null */
    for (textEnd = buf, length = 0; *textEnd; length++, textEnd++);

    getString("text", oldString, 256, FALSE, ECHO, "");
    if (!*oldString) {
        mPrintf(" Text not found.\n");
        return;
    }
    if ((loc = matchString(buf, oldString, textEnd, ver)) == NULL) {
        mPrintf(" Text not found.\n ");
        return;
    }
    getString("replacement text", newString, 256, FALSE, ECHO, "");
    if (strlen(newString) > strlen(oldString)
    && ((strlen(newString) - strlen(oldString)) >= lim - length)) {
        mPrintf(" Buffer overflow.\n ");    /* FJM: should be trapped ??? */
        return;         /* nope, it's a user error.   */
    }
    /* delete old string: */
    for (pc = loc, incr = strlen(oldString); (*pc = *(pc + incr)) != 0; pc++)
        ;
    textEnd -= incr;

    /* make room for new string: */
    for (pc = textEnd, incr = strlen(newString); pc >= loc; pc--) {
        *(pc + incr) = *pc;
    }

    /* insert new string: */
    for (pc = newString; *pc; *loc++ = *pc++)
        ;
}

/* -------------------------------------------------------------------- */
/*  wordcount()     talleys # lines, word & characters message contains */
/* -------------------------------------------------------------------- */
void wordcount(char *buf)
{
    char *counter;
    int lines = 0, words = 0, chars;

    counter = buf;

    chars = strlen(buf);

    while (*counter++) {
        if (*counter == ' ') {
            if ((*(counter - 1) != ' ') && (*(counter - 1) != '\n'))
                words++;
        }
        if (*counter == '\n') {
            if ((*(counter - 1) != ' ') && (*(counter - 1) != '\n'))
                words++;
            lines++;
        }
    }
    mPrintf(" %d lines, %d words, %d characters\n ", lines, words, chars);
}

/* -------------------------------------------------------------------- */
/*  ismostlyupper() -- tests to see if string has n % uppercase letters */
/*  returns true if it thinks so...                                     */
/* -------------------------------------------------------------------- */
int ismostlyupper(char *s, int n)
/* s = string to inspect,  n = percentage value */
{
    int nupper, nlower;
    long percent_upper;
	char *cp;

    nupper = nlower = 0;
	for (cp = s; *cp; cp++) {
		if(*cp == 0x02) return FALSE;
		if (isupper(*cp))
            nupper++;
        if (islower(*cp))
            nlower++;
    }

    if (nupper == 0)
        return FALSE;

    percent_upper = (((long) nupper) * 100L) / ((long) nupper + (long) nlower);
    return (percent_upper >= (long) n);
}

/* -------------------------------------------------------------------- */
/*  fakeFullCase()  converts a message in uppercase-only to a           */
/*      reasonable mix.  It can't possibly make matters worse...        */
/*      Algorithm: First alphabetic after a period is uppercase, all    */
/*      others are lowercase, excepting pronoun "I" is a special case.  */
/*      We assume an imaginary period preceding the text.               */
/* -------------------------------------------------------------------- */
void fakeFullCase(char *text)
{
    char *c;
    char lastWasPeriod;
    char state;

    for (lastWasPeriod = TRUE, c = text; *c; c++) {
        if ((*c != '.') && (*c != '?') && (*c != '!')) {
            if (isalpha(*c)) {
                if (lastWasPeriod)
                    *c = (char) toupper(*c);
                else
                    *c = (char) tolower(*c);
                lastWasPeriod = FALSE;
            }
        } else {
            lastWasPeriod = TRUE;
        }
    }

    /* little state machine to search for ' i ': */
#define NUTHIN          0
#define FIRSTBLANK      1
#define BLANKI          2
    for (state = NUTHIN, c = text; *c; c++) {
        switch (state) {
            case NUTHIN:
                if (isspace(*c))
                    state = FIRSTBLANK;
                else
                    state = NUTHIN;
                break;
            case FIRSTBLANK:
                if (*c == 'i')
                    state = BLANKI;
                else
                    state = NUTHIN;
                break;
            case BLANKI:
                if (isspace(*c))
                    state = FIRSTBLANK;
                else
                    state = NUTHIN;

                if (!isalpha(*c))
                    *(c - 1) = 'I';
                break;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  xPutStr()       Put a string to a file w/o trailing blank           */
/* -------------------------------------------------------------------- */
void xPutStr(FILE * fl, char *str)
{
    while (*str) {
        fputc(*str, fl);
        str++;
    }
}
/* EOF */
