/* -------------------------------------------------------------------- */
/*  MSG2.C                    Citadel                                   */
/* -------------------------------------------------------------------- */
/*  This file contains the low level code for accessing messages        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <alloc.h>
#endif

#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  indexslot()     give it a message # and it returns a slot#          */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/*  getMsgChar()    reads a character from msg file, curent position    */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/*  notelogmessage() notes private message into recip.'s log entry      */
/*  putMsgChar()    writes character to message file                    */
/*  sizetable()     returns # messages in table                         */
/*  copyindex()     copies msg index source to message index dest w/o   */
/*  dPrintf()       sends formatted output to message file              */
/*  overwrite()     checks for any overwriting of old messages          */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  05/25/89    (PAT)   Created from MSG.C and ROOMA.C to move all the
 *                      low-level message handling code here.
 *  03/17/90    FJM     added checks for farfree()
 *  06/06/90    FJM     fix for mayseeindexmsg()
 *  06/09/90    FJM     fix for fix for mayseeindexmsg()
 *  01/18/91    FJM     Reset colors at end of user name
 *  05/06/91    FJM     Fix for released messages to net (about line 171).
 *  05/15/91    BLJ     Added string search test
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  indexslot()     give it a message # and it returns a slot#          */
/* -------------------------------------------------------------------- */
int indexslot(ulong msgno)
{
	if	(msgno < cfg->mtoldest) {
        if  (debug) {
            doCR();
            mPrintf("Can't find attribute");
            doCR();
        }
        return (ERROR);
    }
	return ((int) (msgno - cfg->mtoldest));
}

/* -------------------------------------------------------------------- */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/* -------------------------------------------------------------------- */
BOOL mayseemsg(void)
{
    int i;
    uchar attr;

    if (!copyflag)
        attr = msgBuf->mbattr;
    else
        attr = originalattr;

    /* mfUser */
    if (mf.mfUser[0]) {
        if (!u_match(msgBuf->mbto, mf.mfUser)
            && !u_match(msgBuf->mbauth, mf.mfUser))
            return (FALSE);
    }
    /* check for PUBLIC non problem user messages first */
    if (!msgBuf->mbto[0] && !msgBuf->mbx[0])
        return (TRUE);

	/* let not logged in users read copies (including nodes) */
    if (!loggedIn) {
		if (msgBuf->mbx[0] && copyflag)
			return TRUE;
		else
			return (FALSE);
	}
	
    /* problem users can't see copys of their own messages */
    if (strcmpi(msgBuf->mbauth, logBuf.lbname) == SAMESTRING && msgBuf->mbx[0]
        && copyflag)
        return (FALSE);

    /* but everyone else can't see the orignal if it has been released */
    if (strcmpi(msgBuf->mbauth, logBuf.lbname) != SAMESTRING && msgBuf->mbx[0]
        && !copyflag && ((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
        )
        return (FALSE);

    /* author can see his own private messages */
    if (strcmpi(msgBuf->mbauth, logBuf.lbname) == SAMESTRING)
        return (TRUE);

    if (msgBuf->mbx[0]) {
        if (!aide && msgBuf->mbx[0] == 'Y' &&
            !(attr & ATTR_MADEVIS))
            return (FALSE);
        if (msgBuf->mbx[0] == 'M' && !(attr & ATTR_MADEVIS))
			if (!(sysop || (aide && !cfg->moderate)))
                return (FALSE);
    }
    if (msgBuf->mbto[0]) {
	/* recipient can see private messages IF the origin system is not here */
		if (strcmpi(msgBuf->mbto, logBuf.lbname) == SAMESTRING) {
			if(msgBuf->mboname[0]) {
				if(strcmpi(msgBuf->mboname,cfg->nodeTitle) != SAMESTRING)
					return(TRUE);
			}
			else return (TRUE);
		}
		/* forwardee can see private messages	   */
        if (strcmpi(msgBuf->mbfwd, logBuf.lbname) == SAMESTRING)
            return (TRUE);

    /* sysops see messages to 'Sysop'           */
        if (sysop && (strcmpi(msgBuf->mbto, "Sysop") == SAMESTRING))
            return (TRUE);

    /* aides see messages to 'Aide'           */
        if (aide && (strcmpi(msgBuf->mbto, "Aide") == SAMESTRING))
            return (TRUE);

    /* none of those so cannot see message     */
        return (FALSE);
    }
    if (msgBuf->mbgroup[0]) {
        if (mf.mfGroup[0]) {
            if (strcmpi(mf.mfGroup, msgBuf->mbgroup) != SAMESTRING)
                return (FALSE);
        }
        for (i = 0; i < MAXGROUPS; ++i) {
        /* check to see which group message is to */
			if (strcmpi(grpBuf->group[i].groupname, msgBuf->mbto) == SAMESTRING) {
        /* if in that group */
				if (logBuf.groups[i] == grpBuf->group[i].groupgen)
                    return (TRUE);
            }
        }           /* group can't see message, return false */
        return (FALSE);
    }
    return (TRUE);
}


/* -------------------------------------------------------------------- */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/* -------------------------------------------------------------------- */
BOOL mayseeindexmsg(int slot)
{
    int i;

    if (msgTab[slot].mtoffset > slot)
        return (FALSE);

    /* check for PUBLIC non problem user messages first */
    if (!msgTab[slot].mttohash && !msgTab[slot].mtmsgflags.PROBLEM &&
        !msgTab[slot].mtmsgflags.LIMITED)
        return TRUE;

    if (!loggedIn)
        return (FALSE);

    if (msgTab[slot].mtmsgflags.PROBLEM) {
        if (msgTab[slot].mtmsgflags.COPY) {
        /* problem users can not see copys of their messages */
            if (msgTab[slot].mtauthhash == hash(logBuf.lbname))
                return FALSE;
        } else {
        /* if you are a aide/sop and it is not MADEVIS */
            if (((!aide && !sysop) || msgTab[slot].mtmsgflags.MADEVIS)
                && msgTab[slot].mtauthhash != hash(logBuf.lbname))
                return FALSE;
        }
    }
    if (msgTab[slot].mtmsgflags.MAIL) {
    /* author can see his own private messages */
        if (msgTab[slot].mtauthhash == hash(logBuf.lbname)
            && msgTab[slot].mtorigin == 0)
            return (TRUE);

    /* recipient can see private messages      */
        if (msgTab[slot].mttohash == hash(logBuf.lbname)
            && !msgTab[slot].mtmsgflags.NET)
            return (TRUE);

    /* forwardee can see private messages      */
        if (msgTab[slot].mtfwdhash == hash(logBuf.lbname))
            return (TRUE);

    /* sysops see messages to 'Sysop'           */
        if (sysop && (msgTab[slot].mttohash == hash("Sysop")))
            return (TRUE);

    /* aides see messages to 'Aide'           */
        if (aide && (msgTab[slot].mttohash == hash("Aide")))
            return (TRUE);

    /* none of those so cannot see message     */
        return (FALSE);
    }
    if (msgTab[slot].mtmsgflags.LIMITED) {
        if (*(mf.mfGroup)) {
            if (hash(mf.mfGroup) != msgTab[slot].mttohash)
                return (FALSE);
        }
        for (i = 0; i < MAXGROUPS; ++i) {
        /* check to see which group message is to */
			if (hash(grpBuf->group[i].groupname) == msgTab[slot].mttohash) {
        /* if in that group */
				if (logBuf.groups[i] == grpBuf->group[i].groupgen)
                    return (TRUE);
            }
        }           /* group can't see message, return false */
        return (FALSE);
    }
    return (TRUE);
}

/* -------------------------------------------------------------------- */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/* -------------------------------------------------------------------- */
void changeheader(ulong id, uchar roomno, uchar attr)
{
    long loc;
    int slot;
    int c;
    long pos;
    int room;

    pos = ftell(msgfl);
    slot = indexslot(id);

    /*
     * Change the room # for the message
     */
    if (roomno != 255) {
    /* determine room # of message to be changed */
        room = msgTab[slot].mtroomno;

    /* fix the message tallys from */
        talleyBuf.room[room].total--;
        if (mayseeindexmsg(slot)) {
            talleyBuf.room[room].messages--;
			if ((ulong) (cfg->mtoldest + slot) >
                logBuf.lbvisit[logBuf.lbroom[room].lvisit])
                talleyBuf.room[room].new--;
        }
    /* fix room tallys to */
        talleyBuf.room[roomno].total++;
        if (mayseeindexmsg(slot)) {
            talleyBuf.room[roomno].messages++;
			if ((ulong) (cfg->mtoldest + slot) >
                logBuf.lbvisit[logBuf.lbroom[roomno].lvisit])
                talleyBuf.room[room].new++;
        }
    }
    loc = msgTab[slot].mtmsgLoc;
    if (loc == ERROR)
        return;

    fseek(msgfl, loc, SEEK_SET);

    /* find start of message */
    do
        c = getMsgChar();
        while (c != 0xFF);

    if (roomno != 255) {
        overwrite(1);
    /* write room #    */
        putMsgChar(roomno);

        msgTab[slot].mtroomno = roomno;
    } else {
        getMsgChar();
    }

    if (attr != 255) {
        overwrite(1);
    /* write attribute */
        putMsgChar(attr);

        msgTab[slot].mtmsgflags.RECEIVED
        = ((attr & ATTR_RECEIVED) == ATTR_RECEIVED);

        msgTab[slot].mtmsgflags.REPLY
        = ((attr & ATTR_REPLY) == ATTR_REPLY);

        msgTab[slot].mtmsgflags.MADEVIS
        = ((attr & ATTR_MADEVIS) == ATTR_MADEVIS);
    }
    fseek(msgfl, pos, SEEK_SET);
}

/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;

    for (i = 0; i < howmany; ++i) {
        room = msgTab[i].mtroomno;

        talleyBuf.room[room].total--;

        if (mayseeindexmsg(i)) {
            talleyBuf.room[room].messages--;

			if ((ulong) (cfg->mtoldest + i) >
                logBuf.lbvisit[logBuf.lbroom[room].lvisit])
                talleyBuf.room[room].new--;
        }
    }

    hmemcpy(&(msgTab[0]), &(msgTab[howmany]),
	((long) (cfg->nmessages - howmany) * (long) sizeof(*msgTab)));

	cfg->mtoldest = cfg->mtoldest + howmany;
}

/* -------------------------------------------------------------------- */
/*  dGetWord()      Gets one word from current message or FALSE         */
/* -------------------------------------------------------------------- */
BOOL dGetWord(char *dest, int lim)
{
    int c;

    --lim;          /* play it safe */

    /* pick up any leading blanks: */
    for (c = getMsgChar();
        c == ' ' && c && lim;
    c = getMsgChar()) {
        if (lim) {
            *dest++ = (char) c;
            lim--;
        }
    }

    /* step through word: */
    for (; c != ' ' && c && lim; c = getMsgChar()) {
        if (lim) {
            *dest++ = (char) c;
            lim--;
        }
    }

    /* trailing blanks: */
    for (; c == ' ' && c && lim; c = getMsgChar()) {
        if (lim) {
            *dest++ = (char) c;
            lim--;
        }
    }

    /* took one too many */
    if (c)
        ungetc(c, msgfl);

    *dest = '\0';       /* tie off string       */

    return (BOOL) c;
}

/* -------------------------------------------------------------------- */
/*  getMsgChar()    reads a character from msg file, curent position    */
/* -------------------------------------------------------------------- */
getMsgChar()
{
    int c;

    c = fgetc(msgfl);

    if (c == ERROR) {
    /* check for EOF */
        if (feof(msgfl)) {
            clearerr(msgfl);
			fseek(msgfl, 0L, SEEK_SET);
            c = fgetc(msgfl);
        }
    }
    return c;
}

/* -------------------------------------------------------------------- */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/* -------------------------------------------------------------------- */
void getMsgStr(char *dest, int lim)
{
    char c;

    while ((c = (char) getMsgChar()) != 0) {    /* read the complete string     */
        if (lim) {      /* if we have room then         */
            lim--;
            *dest++ = c;    /* copy char to buffer          */
        }
    }
    *dest = '\0';       /* tie string off with null     */
}

/* -------------------------------------------------------------------- */
/*  notelogmessage() notes private message into recip.'s log entry      */
/* -------------------------------------------------------------------- */
void notelogmessage(char *name)
{
    int logNo;
    struct logBuffer *lBuf2;

	if (strcmpi(msgBuf->mbto, "Sysop") == SAMESTRING
        || strcmpi(msgBuf->mbto, "Aide") == SAMESTRING
		|| (msgBuf->mbzip[0]
		&& strcmpi(msgBuf->mbzip, cfg->nodeTitle) != SAMESTRING))
		return;

	if ((lBuf2 = farcalloc(1UL, sizeof(struct logBuffer))) == NULL) {
        crashout("Can not allocate memory in notelogmessage()");
    }
    if ((logNo = findPerson(name, lBuf2)) == ERROR) {
        if (lBuf2)
            farfree(lBuf2);
        lBuf2 = NULL;
        return;
    }
	if (lBuf2->lbflags.L_INUSE) {
		/* if room unknown and public room, make it known */
		if ((lBuf2->lbroom[thisRoom].lbgen != roomBuf.rbgen)
			&& roomBuf.rbflags.PUBLIC)
		{
			lBuf2->lbroom[thisRoom].lbgen = roomBuf.rbgen;
		}

        lBuf2->lbroom[thisRoom].mail = TRUE;    /* Note there's mail */
        putLog(lBuf2, logNo);
    }
    if (lBuf2)
        farfree(lBuf2);
    lBuf2 = NULL;
}

/* -------------------------------------------------------------------- */
/*  putMsgChar()    writes character to message file                    */
/* -------------------------------------------------------------------- */
void putMsgChar(uchar c)
{
	if	(ftell(msgfl) >= (long) ((long) cfg->messagek * 1024L)) {
    /* scroll to the beginning */
		fseek(msgfl, 0L, 0);
    }
    /* write character out */
    fputc(c, msgfl);
}

/* -------------------------------------------------------------------- */
/*  sizetable()     returns # messages in table                         */
/* -------------------------------------------------------------------- */
uint sizetable(void)
{
	return (int) ((cfg->newest - cfg->mtoldest) + 1);
}

/* -------------------------------------------------------------------- */
/*  copyindex()     copies msg index source to message index dest w/o   */
/*                  certain fields (attr, room#)                        */
/* -------------------------------------------------------------------- */
void copyindex(int dest, int source)
{
    msgTab[dest].mttohash = msgTab[source].mttohash;
    msgTab[dest].mtomesg = msgTab[source].mtomesg;
    msgTab[dest].mtorigin = msgTab[source].mtorigin;
    msgTab[dest].mtauthhash = msgTab[source].mtauthhash;
    msgTab[dest].mtfwdhash = msgTab[source].mtfwdhash;
    msgTab[dest].mtmsgflags.MAIL = msgTab[source].mtmsgflags.MAIL;
    msgTab[dest].mtmsgflags.LIMITED = msgTab[source].mtmsgflags.LIMITED;
    msgTab[dest].mtmsgflags.PROBLEM = msgTab[source].mtmsgflags.PROBLEM;

    msgTab[dest].mtmsgflags.COPY = TRUE;
}

/* -------------------------------------------------------------------- */
/*  dPrintf()       sends formatted output to message file              */
/* -------------------------------------------------------------------- */
void dPrintf(char *fmt,...)
{
    char buff[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    putMsgStr(buff);
}

/* -------------------------------------------------------------------- */
/*  overwrite()     checks for any overwriting of old messages          */
/* -------------------------------------------------------------------- */
void overwrite(int bytes)
{
    long pos;
    int i;

    pos = ftell(msgfl);

	fseek(msgfl, 0L, 1);

    for (i = 0; i < bytes; ++i) {
        if (getMsgChar() == -1) /* obliterating a message */
			logBuf.lbvisit[(MAXVISIT - 1)] = ++cfg->oldest;
    }
    fseek(msgfl, pos, 0);
}

/* -------------------------------------------------------------------- */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */
void putMsgStr(char *string)
{
    char *s;

    /* check for obliterated messages */
    overwrite(strlen(string) + 1);  /* the '+1' is for the null */

    for (s = string; *s; s++)
        putMsgChar(*s);

    /* null to tie off string */
    putMsgChar(0);
}

/* EOF */
