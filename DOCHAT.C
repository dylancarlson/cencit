/************************************************************************
 * $Header:   H:/VCS/FCIT/DOCHAT.C_V   1.26   19 Apr 1991 23:38:30   FJM  $
 *              Command-interpreter code for Citadel
 ************************************************************************/

#include <string.h>
#include <time.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#include <dos.h>
#endif

#include "keywords.h"
#include "proto.h"
#include "global.h"

/************************************************************************
 *                              Contents
 *
 *      doChat()                handles C(hat)          command
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *	History:
 * 06/12/91   BLJ	Added code for No Chat for specific User
 * 06/13/91   BLJ	Changed the chat bell sound
 * 06/19/91   BLJ	Made chat bell sysop configurable
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions
 ************************************************************************/

void doChat(char moreYet, char first);

/************************************************************************/
/*      doChat()                                                        */
/************************************************************************/
void doChat(moreYet, first)
char moreYet;           /* TRUE to accept following parameters  */
char first;         /* first parameter if TRUE              */
{
    static int chatnum = 0;

    moreYet = moreYet;      /* -W3 */
    first = first;      /* -W3 */

    chatReq = TRUE;

    mPrintf("\bChat ");

    trap("Chat request", T_CHAT);

	if (cfg->noChat || logBuf.NOCHAT) {
        nextblurb("nochat", &chatnum, 1);
        return;
    }
    if (whichIO == MODEM)
        ringSysop();
    else {
        chat();
    }
}

/* -------------------------------------------------------------------- */
/*  chat()          This is the chat mode                               */
/* -------------------------------------------------------------------- */
void chat(void)
{
    int c, from, lastfrom, wsize = 0, i;
    char word[50];
    static chatcnt = 0;

    chatkey = FALSE;
    chatReq = FALSE;

    if (!gotCarrier()) {
        dial_out();
        return;
    }
    lastfrom = 2;

    nextblurb("chat", &chatcnt, 1);
    outFlag = IMPERVIOUS;
    /* outFlag = OUTOK;         - changed per Peter's suggestion */

    do {
        c = 0;

        if (KBReady()) {
            c = getch();
            if ((c == -1) || (c == 0xff)) {
                c = 0x1a;
            }
            getkey = 0;
            from = 0;
        }
        if (MIReady()) {
            if (!onConsole) {
                c = getMod();
                if ((c == -1) || (c == 0xff)) {
                    c = 0x1a;
                }
                from = 1;
            } else {
                getMod();
            }
        }
        if (c < 128)
            c = filt_in[c];

        if (c && c != 0x1a /* CNTRLZ */ ) {
            if (from != lastfrom) {
                if (from) {
                    termCap(TERM_NORMAL);
                } else {
                    termCap(TERM_BOLD);
                }
                lastfrom = from;
            }
            if (c == '\r' || c == '\n' || c == ' ' || c == '\t' || wsize == 50) {
                wsize = 0;
            } else if (crtColumn >= (termWidth - 1)) {
                if (wsize) {
                    for (i = 0; i < wsize; i++)
                        doBS();
                    doCR();
                    for (i = 0; i < wsize; i++)
                        echocharacter(word[i]);
                } else {
                    doCR();
                }

                wsize = 0;
            } else {
                word[wsize] = c;
                wsize++;
            }

            echocharacter(c);
        }
    } while ((c != 0x1a /* CNTRLZ */ ) && gotCarrier());

    time(&lasttime);
    termCap(TERM_NORMAL);

    doCR();
    outFlag = OUTOK;
}

/* -------------------------------------------------------------------- */
/*  ringSysop()     ring the sysop                                      */
/* -------------------------------------------------------------------- */
#define FANCYBELL 1

void ringSysop(void)
{
	int x;
	char i, rep, y;
    char answered = FALSE;
    int oldBells;
    char ringlimit = 30;
    static nosop = 0;

    /* turn the ringer on */
	oldBells = cfg->noBells;
	cfg->noBells = FALSE;

	mPrintf("\n Ringing %s.", cfg->sysop);

	if(ChatSnd.type == 0) {
		ChatSnd.type = 5;
		ChatSnd.hi = 1012;
		ChatSnd.lo = 774;
		ChatSnd.incr = 50;
		ChatSnd.dly1 = 69;
		ChatSnd.dly2 = 5;
	}

	rep = 1;
	while(ChatSnd.type > 4) {
		ChatSnd.type -= 4;
		rep++;
	}

	answered = FALSE;
    for (i = 0; (i < ringlimit) && !answered && gotCarrier(); i++) {
#ifdef FANCYBELL
        outMod(7);
		switch(ChatSnd.type) {
			case 1:    /*	bell	  */
				for(y=0; y<rep; y++) {
					if (!cfg->noBells)
					   /* retain ablility to turn off with F7  */
						sound(ChatSnd.hi);
					delay(ChatSnd.dly1);
					nosound();
					delay(ChatSnd.dly2);

					if (!cfg->noBells)
					   /* retain ablility to turn off with F7  */
						sound(ChatSnd.lo);
					delay(ChatSnd.dly1);
					nosound();
					delay(ChatSnd.dly2);
				}
				break;
			case 2:    /* slide up	  */
				for(y=0; y<rep; y++) {
					for(x=ChatSnd.lo; x < ChatSnd.hi; x += ChatSnd.incr) {
						if (!cfg->noBells)
						  /* retain ablility to turn off with F7  */
						   sound(x);
					   delay(ChatSnd.dly1);
					   nosound();
					   delay(ChatSnd.dly2);
					}
				}
				break;
			case 3:    /* slide down  */
				for(y=0; y<rep; y++) {
					for(x=ChatSnd.hi; x > ChatSnd.lo; x -= ChatSnd.incr) {
						if (!cfg->noBells)
						   /* retain ablility to turn off with F7  */
						  sound(x);
						delay(ChatSnd.dly1);
						nosound();
						delay(ChatSnd.dly2);
					}
				}
				break;
			case 4:    /* siren 	  */
				for(y=0; y<rep; y++) {
					for(x=ChatSnd.lo; x < ChatSnd.hi; x += ChatSnd.incr) {
						if (!cfg->noBells)
						   /* retain ablility to turn off with F7  */
							sound(x);
						delay(ChatSnd.dly1);
						nosound();
						delay(ChatSnd.dly2);
					}
					for(x=ChatSnd.hi; x > ChatSnd.lo; x -= ChatSnd.incr) {
						if (!cfg->noBells)
						   /* retain ablility to turn off with F7  */
							sound(x);
						delay(ChatSnd.dly1);
						nosound();
						delay(ChatSnd.dly2);
					}
				}
				break;
		}

		delay(1000);
#else
        oChar(7 /* BELL */ );
        delay(800);
#endif
        if (BBSCharReady() || KBReady())
            answered = TRUE;
    }

	/* put the type back to what it was */
	while(rep > 1) {
		ChatSnd.type += 4;
		rep--;
    }

	cfg->noBells = oldBells;

    if (KBReady()) {
        chat();
    } else if (i >= ringlimit) {
        nextblurb("nosop", &nosop, 1);
    } else
        iChar();
}


