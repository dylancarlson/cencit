/* -------------------------------------------------------------------- */
/*  LOG.C                     Citadel                                   */
/* -------------------------------------------------------------------- */
/*                           Local log code                             */
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
/*  findPerson()    loads log record for named person.                  */
/*                  RETURNS: ERROR if not found, else log record #      */
/*  personexists()  returns slot# of named person else ERROR            */
/*  setdefaultconfig()  this sets the global configuration variables    */
/*  setlogconfig()  this sets the configuration in current logBuf equal */
/*                  to the global configuration variables               */
/*  setsysconfig()  this sets the global configuration variables equal  */
/*                  to the the ones in logBuf                           */
/*  showconfig()    displays user configuration                         */
/*  slideLTab()     crunches up slot, then frees slot at beginning,     */
/*                  it then copies information to first slot            */
/*  storeLog()      stores the current log record.                      */
/*  displaypw()     displays callers name, initials & pw                */
/*  normalizepw()   This breaks down inits;pw into separate strings     */
/*  pwexists()      returns TRUE if password exists in logtable         */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  06/14/89  (PAT)  Created from LOG.C to move some of the system
 *                   out of memory. Also cleaned up moved code to
 *                   -W3, ext.
 *  03/15/90  {zm}   Add [title] name [surname] everywhere.
 *  06/16/90  FJM    Fixes to allow entry of 30 char nym & initials.
 *  06/16/90  FJM    Made IBM Graphics characters a seperate option.
 *	01/19/91  FJM	 Cleaned up login.
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  findPerson()    loads log record for named person.                  */
/*                  RETURNS: ERROR if not found, else log record #      */
/* -------------------------------------------------------------------- */
int findPerson(char *name, struct logBuffer * l_buf)
{
    int slot, logno;

    slot = personexists(name);

    if (slot == ERROR)
        return (ERROR);

    getLog(l_buf, logno = logTab[slot].ltlogSlot);

    return (logno);
}

/* -------------------------------------------------------------------- */
/*  personexists()  returns slot# of named person else ERROR            */
/* -------------------------------------------------------------------- */
int personexists(char *name)
{
    int i, namehash;
    struct logBuffer logRead;

    namehash = hash(name);

    /* check to see if name is in log table */

	for (i = 0; i < cfg->MAXLOGTAB; i++) {
        if (namehash == logTab[i].ltnmhash) {
            getLog(&logRead, logTab[i].ltlogSlot);

            if (strcmpi(name, logRead.lbname) == SAMESTRING)
                return (i);
        }
    }

    return (ERROR);
}

/* -------------------------------------------------------------------- */
/*  setdefaultconfig()  this sets the global configuration variables    */
/* -------------------------------------------------------------------- */
/*very strange, sets odd locations.  i.e. some, but not all logBuf vars	*/
/* -------------------------------------------------------------------- */
void setdefaultconfig(void)
{
    prevChar = ' ';
	
	linesScreen = 23;
	Sound = FALSE;
	termWidth = cfg->width;
	termNulls = cfg->nulls;
	termUpper = (BOOL) cfg->uppercase;
	termLF = (BOOL) cfg->linefeeds;
    expert = FALSE;
    aide = FALSE;
	termTab = (BOOL) cfg->tabs;
	oldToo = TRUE;	   /* later a cfg->lastold */
	twit = cfg->user[D_PROBLEM];
    unlisted = FALSE;
    sysop = FALSE;
    roomtell = TRUE;
    logBuf.DUNGEONED = FALSE;
    logBuf.MSGAIDE = FALSE;
    logBuf.FORtOnODE = FALSE;
    logBuf.NEXTHALL = FALSE;
	if (ansiOn)
		strcpy(logBuf.tty, "ANSI-BBS");
	else
		strcpy(logBuf.tty, "TTY");
    logBuf.IBMGRAPH = IBMOn;
	logBuf.STICKYRM = cfg->Stickyrm;
		
}

/* -------------------------------------------------------------------- */
/*  setlogconfig()  this sets the configuration in current logBuf equal */
/*                  to the global configuration variables               */
/* -------------------------------------------------------------------- */
void setlogconfig(void)
{
    logBuf.lbwidth = termWidth;
	logBuf.linesScreen = linesScreen;
	logBuf.lbnulls = termNulls;
	if (ansiOn)
		strcpy(logBuf.tty, "ANSI-BBS");
	else
		strcpy(logBuf.tty, "TTY");
    logBuf.lbflags.UCMASK = termUpper;
    logBuf.lbflags.LFMASK = termLF;
    logBuf.lbflags.EXPERT = expert;
	logBuf.lbflags.TABS = termTab;
    logBuf.lbflags.OLDTOO = oldToo;
    logBuf.lbflags.PROBLEM = twit;
    logBuf.lbflags.UNLISTED = unlisted;
    logBuf.lbflags.ROOMTELL = roomtell;
    logBuf.IBMGRAPH = IBMOn;
	logBuf.SOUND = Sound;
}

/* -------------------------------------------------------------------- */
/*  setsysconfig()  this sets the global configuration variables equal  */
/*                  to the the ones in logBuf                           */
/* -------------------------------------------------------------------- */
/* NB: Does not include all values in logBuf; error?                    */
/* -------------------------------------------------------------------- */
void setsysconfig(void)
{
    termWidth = logBuf.lbwidth;
	linesScreen = logBuf.linesScreen;
	termNulls = logBuf.lbnulls;
    termLF = (BOOL) logBuf.lbflags.LFMASK;
    termUpper = (BOOL) logBuf.lbflags.UCMASK;
    expert = (BOOL) logBuf.lbflags.EXPERT;
    aide = (BOOL) logBuf.lbflags.AIDE;
    sysop = (BOOL) logBuf.lbflags.SYSOP;
    termTab = (BOOL) logBuf.lbflags.TABS;
    oldToo = (BOOL) logBuf.lbflags.OLDTOO;
    twit = (BOOL) logBuf.lbflags.PROBLEM;
    unlisted = (BOOL) logBuf.lbflags.UNLISTED;
    roomtell = (BOOL) logBuf.lbflags.ROOMTELL;
    IBMOn = (BOOL) logBuf.IBMGRAPH;
	ansiOn = (BOOL) (strcmpi(logBuf.tty, "ANSI-BBS") == SAMESTRING);
	Sound = logBuf.SOUND;
}

/* -------------------------------------------------------------------- */
/*  showconfig()    displays user configuration                         */
/* -------------------------------------------------------------------- */
void showconfig(struct logBuffer * l_buf)
{
    int i;

    if (loggedIn) {
        setlogconfig();

		if (cfg->surnames && l_buf->title[0]) {
            mPrintf("\n User [%s] %s", l_buf->title, l_buf->lbname);
        } else {
            mPrintf("\n User ");
			mtPrintf(TERM_BOLD,"%s", l_buf->lbname);
        }
		if (cfg->surnames && l_buf->surname[0]) {
            mPrintf(" [%s]", l_buf->surname);
        }
        if (
			l_buf->lbflags.UNLISTED ||
            l_buf->lbflags.SYSOP ||
            l_buf->lbflags.NODE ||
            l_buf->lbflags.NETUSER ||
            l_buf->DUNGEONED ||
			l_buf->MSGAIDE ||
            l_buf->lbflags.AIDE
			) {
            doCR();
			/* show Sysop, Aide first */
            if (l_buf->lbflags.SYSOP)
                mPrintf(" Sysop");
            if (l_buf->lbflags.AIDE)
                mPrintf(" Aide");
			
            if (l_buf->lbflags.UNLISTED)
                mPrintf(" Unlisted");
			/* l_buf->lbflags.PERMANENT */
            if (l_buf->lbflags.NODE)
                mPrintf(" Node");
            if (l_buf->lbflags.NETUSER)
                mPrintf(" Netuser");
            if (l_buf->MSGAIDE)
                mPrintf(" Moderator");
			/* l_buf->FORtOnODE */
			/* l_buf->NEXTHALL */
            if (l_buf->DUNGEONED)
                mPrintf(" Dungeoned");
			if (l_buf->STICKYRM)
				mPrintf(" Keep");
			if (l_buf->SOUND)
				mPrintf(" Sound");
        }
		/* NB: still need to check the remainder of this */
        if (l_buf->forward[0]) {
            doCR();
            mPrintf(" Private mail forwarded to ");
			/* l_buf->FORtOnODE */
            if (personexists(l_buf->forward) != ERROR)
                mPrintf("%s", l_buf->forward);
        }
        if (l_buf->hallhash) {
            doCR();
            mPrintf(" Default hallway: ");

            for (i = 1; i < MAXHALLS; ++i) {
                if (hash(hallBuf->hall[i].hallname) == l_buf->hallhash) {
                    if (groupseeshall(i))
                        mPrintf("%s", hallBuf->hall[i].hallname);
                }
            }
        }
        doCR();
        mPrintf(" Groups: ");

        for (i = 0; i < MAXGROUPS; ++i) {
			if (grpBuf->group[i].g_inuse
				&& (l_buf->groups[i] == grpBuf->group[i].groupgen))
				mPrintf("%s ", grpBuf->group[i].groupname);
        }
		if (l_buf->DUNGEONED || l_buf->STICKYRM) {
			doCR();
			mPrintf(" Current Room: %s ",roomTab[l_buf->lastRoom].rtname);

			doCR();
			mPrintf(" Current Hall: %s ",hallBuf->hall[l_buf->lastHall].hallname);

        }
    }
	if (cfg->accounting && !l_buf->lbflags.NOACCOUNT && loggedIn) {
        doCR();
        mPrintf(" Number of %s%s remaining %.0f",
		cfg->credit_name, ((int) l_buf->credits == 1) ? "" : "s",
        l_buf->credits);
    }
    mPrintf("\n Width %d, ", l_buf->lbwidth);

    if (l_buf->lbflags.UCMASK)
        mPrintf("UPPERCASE ONLY, ");

    if (!l_buf->lbflags.LFMASK)
        mPrintf("No ");

    mPrintf("Linefeeds, ");

    mPrintf("%d nulls, ", l_buf->lbnulls);

    if (!l_buf->lbflags.TABS)
        mPrintf("No ");

    mPrintf("Tabs");

    if (!l_buf->lbflags.OLDTOO)
        mPrintf("\n Do not print");
    else
        mPrintf("\n Print");
    mPrintf(" last Old message on N>ew Message request.");

    if (loggedIn)
        mPrintf("\n Terminal type: %s", l_buf->tty);

    if (l_buf->NEXTHALL)
        mPrintf("\n Auto-next hall on.");

	if (cfg->roomtell && loggedIn) {
        if (!l_buf->lbflags.ROOMTELL)
            mPrintf("\n Do not display");
        else
            mPrintf("\n Display");
        mPrintf(" room descriptions.");
    }
    doCR();
}

/* -------------------------------------------------------------------- */
/*  slideLTab()     crunches up slot, then frees slot at beginning,     */
/*                  it then copies information to first slot            */
/* -------------------------------------------------------------------- */
void slideLTab(int slot)
{               /* slot is current tabslot being moved */
    int ourSlot, i;

    people = slot;      /* number of people since last call */

    if (!slot)
        return;

    ourSlot = logTab[slot].ltlogSlot;

    /* Gee, this works.. */
    for (i = slot; i > 0; i--)
        logTab[i] = logTab[i - 1];

/*
    hmemcpy((char HUGE *)(&logTab[1]), (char HUGE *)(&logTab[0]),
       (long)(sizeof(*logTab) * slot));
 */

    thisSlot = 0;

    /* copy info to beginning of table */
    logTab[0].ltpwhash = hash(logBuf.lbpw);
    logTab[0].ltinhash = hash(logBuf.lbin);
    logTab[0].ltnmhash = hash(logBuf.lbname);
    logTab[0].ltlogSlot = ourSlot;
    logTab[0].ltcallno = logBuf.callno;
    logTab[0].permanent = (BOOL) logBuf.lbflags.PERMANENT;
}


/* -------------------------------------------------------------------- */
/*  storeLog()      stores the current log record.                      */
/* -------------------------------------------------------------------- */
void storeLog(void)
{
    /* make log configuration equal to our environment */
    setlogconfig();

    putLog(&logBuf, thisLog);
}

/* -------------------------------------------------------------------- */
/*  displaypw()     displays callers name, initials & pw                */
/* -------------------------------------------------------------------- */
void displaypw(char *name, char *in, char *pw)
{
    mPrintf("\n nm: %s", name);
    mPrintf("\n in: ");
    echo = CALLER;
    mPrintf("%s", in);
    echo = BOTH;
    mPrintf("\n pw: ");
    echo = CALLER;
    mPrintf("%s", pw);
    echo = BOTH;
    doCR();
}


/* -------------------------------------------------------------------- */
/*  normalizepw()   This breaks down inits;pw into separate strings     */
/* -------------------------------------------------------------------- */
void normalizepw(char *InitPw, char *Initials, char *passWord, char *semi)
{
    char *p;

	p = semi;
	*p = '\0';
	strncpy(Initials,InitPw,NAMESIZE);
	Initials[NAMESIZE]='\0';
	++p;
	strncpy(passWord,p,NAMESIZE);
	passWord[NAMESIZE]='\0';
	normalizeString(Initials);
	normalizeString(passWord);
}

/* -------------------------------------------------------------------- */
/*  pwexists()      returns TRUE if password exists in logtable         */
/* -------------------------------------------------------------------- */
int pwexists(char *pw)
{
    int i, pwhash;

    pwhash = hash(pw);

	for (i = 0; i < cfg->MAXLOGTAB; i++) {
        if (pwhash == logTab[i].ltpwhash)
            return (i);
    }
    return (ERROR);
}

/* EOF */
