/* -------------------------------------------------------------------- */
/*  MSG.C                     Citadel                                   */
/* -------------------------------------------------------------------- */
/*               This is the high level message code.                   */
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
/*  aideMessage()   save auto message in Aide>                          */
/*  specialMessage()    saves a special message in curent room          */
/*  clearmsgbuf()   this clears the message buffer out                  */
/*  getMessage()    reads a message off disk into RAM.                  */
/*  putMessage()    stores a message to disk                            */
/*  noteMessage()   puts message in mesgBuf into message index          */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  06/02/89    (PAT)   Made history, cleaned up comments, reformated
 *                      icky code.
 *  03/15/90    {zm}    Add [title] name [surname], 30 characters long.
 *  06/16/90    FJM     Made IBM Graphics characters a seperate option.
 *	09/12/91	BLJ 	Added the new net fields
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/* -------------------------------------------------------------------- */
void aideMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = AIDEROOM;

	strcpy(msgBuf->mbauth, cfg->nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf.room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*	specialMessage()	saves a special message in current room 		*/
/* -------------------------------------------------------------------- */
void specialMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = (uchar) thisRoom;
	strcpy(msgBuf->mbauth, cfg->nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[thisRoom].lvisit)
        talleyBuf.room[thisRoom].new--;
}

/* -------------------------------------------------------------------- */
/*  clearmsgbuf()   this clears the message buffer out                  */
/* -------------------------------------------------------------------- */
void clearmsgbuf(void)
{
    /* clear msgBuf out */
    msgBuf->mbroomno = 0;
    msgBuf->mbattr = 0;
    msgBuf->mbauth[0] = '\0';
    msgBuf->mbcopy[0] = '\0';
    msgBuf->mbfwd[0] = '\0';
    msgBuf->mbgroup[0] = '\0';
    msgBuf->mbtime[0] = '\0';
    msgBuf->mbId[0] = '\0';
    msgBuf->mbsrcId[0] = '\0';
    msgBuf->mboname[0] = '\0';
    msgBuf->mboreg[0] = '\0';
	msgBuf->mbostate[0] = '\0';
	msgBuf->mbocont[0] = '\0';
	msgBuf->mbophone[0] = '\0';
	msgBuf->mbsoftware[0] = '\0';
	msgBuf->mbreply[0] = '\0';
    msgBuf->mbroom[0] = '\0';
    msgBuf->mbto[0] = '\0';
    msgBuf->mbtitle[0] = '\0';
    msgBuf->mbsur[0] = '\0';
    msgBuf->mblink[0] = '\0';
    msgBuf->mbx[0] = '\0';
    msgBuf->mbzip[0] = '\0';
    msgBuf->mbrzip[0] = '\0';
	msgBuf->mbszip[0] = '\0';
	msgBuf->mbczip[0] = '\0';
	msgBuf->mbzphone[0] = '\0';
	msgBuf->mbtreg[0] = '\0';
	msgBuf->mbtcont[0] = '\0';
	msgBuf->mbfpath[0] = '\0';
    msgBuf->mbtpath[0] = '\0';
}

/* -------------------------------------------------------------------- */
/*  getMessage()    reads a message off disk into RAM.                  */
/* -------------------------------------------------------------------- */
void getMessage(void)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do {
        c = (uchar) getMsgChar();
    } while (c != -1);

    /* record exact position of start of message */
    msgBuf->mbheadLoc = (long) (ftell(msgfl) - (long) 1);

    /* get message's room #         */
    msgBuf->mbroomno = (uchar) getMsgChar();

    /* get message's attribute byte */
    msgBuf->mbattr = (uchar) getMsgChar();

    getMsgStr(msgBuf->mbId, NAMESIZE);

    do {
        c = (char) getMsgChar();
        switch (c) {
            case 'A':
                getMsgStr(msgBuf->mbauth, NAMESIZE);
                break;
            case 'N':
                getMsgStr(msgBuf->mbtitle, NAMESIZE);
                break;
            case 'n':
                getMsgStr(msgBuf->mbsur, NAMESIZE);
                break;
            case 'C':
                getMsgStr(msgBuf->mbcopy, NAMESIZE);
                break;
            case 'D':
                getMsgStr(msgBuf->mbtime, NAMESIZE);
                break;
            case 'F':
                getMsgStr(msgBuf->mbfwd, NAMESIZE);
                break;
            case 'G':
                getMsgStr(msgBuf->mbgroup, NAMESIZE);
                break;
            case 'I':
                getMsgStr(msgBuf->mbreply, NAMESIZE);
                break;
            case 'L':
                getMsgStr(msgBuf->mblink, 64);
                break;
            case 'M':       /* will be read off disk later */
                break;
            case 'O':
                getMsgStr(msgBuf->mboname, NAMESIZE);
                break;
            case 'o':
                getMsgStr(msgBuf->mboreg, NAMESIZE);
                break;
            case 'Q':
				getMsgStr(msgBuf->mbostate, NAMESIZE);
                break;
			case 'U':
                getMsgStr(msgBuf->mbocont, NAMESIZE);
                break;
			case 'H':
				getMsgStr(msgBuf->mbophone, NAMESIZE);
                break;
			case 's':
				getMsgStr(msgBuf->mbsoftware, NAMESIZE);
				break;
			case 'P':
				getMsgStr(msgBuf->mbfpath, 255);
                break;
            case 'p':
				getMsgStr(msgBuf->mbtpath, 255);
                break;
            case 'R':
                getMsgStr(msgBuf->mbroom, NAMESIZE);
                break;
            case 'S':
                getMsgStr(msgBuf->mbsrcId, NAMESIZE);
                break;
            case 'T':
                getMsgStr(msgBuf->mbto, NAMESIZE);
                break;
            case 'X':
                getMsgStr(msgBuf->mbx, NAMESIZE);
                break;
            case 'Z':
                getMsgStr(msgBuf->mbzip, NAMESIZE);
                break;
            case 'z':
                getMsgStr(msgBuf->mbrzip, NAMESIZE);
                break;
            case 'q':
				getMsgStr(msgBuf->mbszip, NAMESIZE);
                break;
			case 'u':
                getMsgStr(msgBuf->mbczip, NAMESIZE);
                break;
			case 'h':
				getMsgStr(msgBuf->mbzphone, NAMESIZE);
                break;
			case 'J':
				getMsgStr(msgBuf->mbtreg, NAMESIZE);
                break;
			case 'j':
				getMsgStr(msgBuf->mbtcont, NAMESIZE);
                break;

            default:
				getMsgStr(msgBuf->mbtext, cfg->maxtext); /* discard unknown field */
                msgBuf->mbtext[0] = '\0';
                break;
        }
    } while (c != 'M' && c != 'L' && isalpha(c));
}

/* -------------------------------------------------------------------- */
/*  putMessage()    stores a message to disk                            */
/*  note, the fields are in no crucial order except the first & last.   */
/* -------------------------------------------------------------------- */
BOOL putMessage(void)
{
       /********************************************************
        *                                                      *
        *  Incredible waste of time on redundent disk access   *
        *                                                      *
        *******************************************************/

/*      alternate idea: build block to be written
        check its size
        read the block to be overwritten
        write the original block

        Note: Text can't be over 8k, and we are in large model
                ------------------------------------------------------
                another idea (Ray's):

                message table contains pointers to messages
                write message, and then consult message table to determine
				count of overwritten messages
				update as in overwrite:
				logBuf.lbvisit[(MAXVISIT-1)]	= ++cfg->oldest;
*/

    long timestamp;
    char stamp[20];

    time(&timestamp);

	sprintf(msgBuf->mbId, "%lu", (unsigned long) (cfg->newest + 1));

    sprintf(stamp, "%ld", timestamp);

    /* record start of message to be noted */
	msgBuf->mbheadLoc = (long) cfg->catLoc;

    /* tell putMsgChar where to write   */
	fseek(msgfl, cfg->catLoc, 0);

    overwrite(1);
    putMsgChar(0xFF);       /* start-of-message */

    overwrite(1);
    putMsgChar(msgBuf->mbroomno);   /* write room # */

    overwrite(1);
    putMsgChar(msgBuf->mbattr); /* write attribute byte  */

    dPrintf("%s", msgBuf->mbId);/* write message ID */

	if (loggedIn || strcmpi(msgBuf->mbauth, cfg->nodeTitle) == SAMESTRING) {
    /* write author's name out:     */
        if (!msgBuf->mbcopy[0])
            dPrintf("A%s", msgBuf->mbauth);
    }
    if (msgBuf->mbcopy[0]) {
        dPrintf("C%s", msgBuf->mbcopy);
    }
    if (!msgBuf->mbcopy[0]) /* write time/datestamp: */
        if (!*msgBuf->mbtime)
            dPrintf("D%s", stamp);
        else
            dPrintf("D%s", msgBuf->mbtime);

    if (msgBuf->mbtitle[0]) {   /* if there is a title, write it out */
        dPrintf("N%s", msgBuf->mbtitle);
    }
    if (msgBuf->mbsur[0]) { /* if there is a surname, write it out */
        dPrintf("n%s", msgBuf->mbsur);
    }
    if (msgBuf->mbfwd[0]) { /* write forwarding address */
        dPrintf("F%s", msgBuf->mbfwd);
    }
    if (msgBuf->mbgroup[0]) {   /* group only message -- write group name */
        dPrintf("G%s", msgBuf->mbgroup);
    }
    if (*msgBuf->mboname) { /* write room name out: */
        dPrintf("R%s", msgBuf->mbroom);
    } else {
        if (!msgBuf->mbcopy[0])
            dPrintf("R%s", roomTab[msgBuf->mbroomno].rtname);
    }

    if (msgBuf->mbsrcId[0]) {   /* msg ID on originating node. */
        dPrintf("S%s", msgBuf->mbsrcId);
    }
    if (msgBuf->mboname[0]) {   /* originating node's name. */
        dPrintf("O%s", msgBuf->mboname);
    }
    if (msgBuf->mboreg[0]) {    /* originating node's region. */
        dPrintf("o%s", msgBuf->mboreg);
    }
	if (msgBuf->mbostate[0]) {	 /* originating node's state.   */
		dPrintf("Q%s", msgBuf->mbostate);
    }
	if (msgBuf->mbocont[0]) {	/* originating node's country. */
		dPrintf("U%s", msgBuf->mbocont);
    }
	if (msgBuf->mbophone[0]) {	 /* originating node's phone.   */
		dPrintf("H%s", msgBuf->mbophone);
    }
	if (msgBuf->mbsoftware[0]) {   /* originating node's software.   */
		dPrintf("s%s", msgBuf->mbsoftware);
    }
    if (msgBuf->mbfpath[0]) {   /* the path it took to get here. */
        dPrintf("P%s", msgBuf->mbfpath);
    }
    if (msgBuf->mblink[0]) {    /* File-linked message */
        dPrintf("L%s", msgBuf->mblink);
    }
    if (msgBuf->mbto[0]) {  /* private message -- write addressee */
        dPrintf("T%s", msgBuf->mbto);
    }
    if (msgBuf->mbreply[0]) {   /* write message # being replied to */
        dPrintf("I%s", msgBuf->mbreply);
    }
    if (msgBuf->mbzip[0]) { /* destination node's name. */
        dPrintf("Z%s", msgBuf->mbzip);
    }
    if (msgBuf->mbrzip[0]) {    /* destination node's region. */
        dPrintf("z%s", msgBuf->mbrzip);
    }
	if (msgBuf->mbszip[0]) {	/* destination node's state. */
		dPrintf("q%s", msgBuf->mbszip);
    }
	if (msgBuf->mbczip[0]) {	/* destination node's country. */
		dPrintf("u%s", msgBuf->mbczip);
    }
	if (msgBuf->mbzphone[0]) {	  /* destination node's phone. */
		dPrintf("h%s", msgBuf->mbzphone);
    }
	if (msgBuf->mbtreg[0]) {	/* SysOp configurable region. (i.e. Sector 1) */
		dPrintf("J%s", msgBuf->mbtreg);
    }
	if (msgBuf->mbtcont[0]) {	 /* SysOp configurable country (i.e. Space) */
		dPrintf("j%s", msgBuf->mbtcont);
    }
    if (msgBuf->mbtpath[0]) {   /* the path to send it along. */
        dPrintf("p%s", msgBuf->mbtpath);
    }
    if (msgBuf->mbx[0]) {   /* problem user or moderated message */
        dPrintf("X%s", msgBuf->mbx);
    }
    overwrite(1);       /* M-for-message. */
    putMsgChar('M');

    putMsgStr(msgBuf->mbtext);  /* write message text */

    fflush(msgfl);      /* now finish writing */

    /* record where to begin writing next message */
	cfg->catLoc = ftell(msgfl);

    talleyBuf.room[msgBuf->mbroomno].total++;

    if (mayseemsg()) {
        talleyBuf.room[msgBuf->mbroomno].messages++;
        talleyBuf.room[msgBuf->mbroomno].new++;
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */
void noteMessage(void)
{
    ulong id, copy;
    int crunch = 0;
    int slot, copyslot;

	logBuf.lbvisit[0] = ++cfg->newest;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* mush up any obliterated messages */
	if (cfg->mtoldest < cfg->oldest)
		crunch = ((ushort) (cfg->oldest - cfg->mtoldest));

    /* scroll index at #nmessages mark */
	if ((ushort) (id - cfg->mtoldest) >= cfg->nmessages)
        crunch++;

    if (crunch)
        crunchmsgTab(crunch);

    /* now record message info in index */
    indexmessage(id);

    /* special for duplicated messages */
    /* This is special. */
    if (*msgBuf->mbcopy) {
    /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        copyslot = indexslot(copy);
        slot = indexslot(id);

        if (copyslot != ERROR) {
            copyindex(slot, copyslot);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */
void indexmessage(ulong here)
{
    ushort slot;
    ulong copy;
    ulong oid;
    struct messagetable HUGE *msgSlt;

    slot = indexslot(here);

    msgSlt = &msgTab[slot];

    msgSlt->mtmsgLoc = (long) 0;

    msgSlt->mtmsgflags.MAIL = 0;
    msgSlt->mtmsgflags.RECEIVED = 0;
    msgSlt->mtmsgflags.REPLY = 0;
    msgSlt->mtmsgflags.PROBLEM = 0;
    msgSlt->mtmsgflags.MADEVIS = 0;
    msgSlt->mtmsgflags.LIMITED = 0;
    msgSlt->mtmsgflags.MODERATED = 0;
    msgSlt->mtmsgflags.RELEASED = 0;
    msgSlt->mtmsgflags.COPY = 0;
    msgSlt->mtmsgflags.NET = 0;

    msgSlt->mtauthhash = 0;
    msgSlt->mttohash = 0;
    msgSlt->mtfwdhash = 0;
    msgSlt->mtoffset = 0;
    msgSlt->mtorigin = 0;
    msgSlt->mtomesg = (long) 0;

    msgSlt->mtroomno = DUMP;

    /* --- */

    msgSlt->mtmsgLoc = msgBuf->mbheadLoc;

    if (*msgBuf->mbsrcId) {
        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        msgSlt->mtomesg = oid;
    }
    if (*msgBuf->mbauth)
        msgSlt->mtauthhash = hash(msgBuf->mbauth);

    if (*msgBuf->mbto) {
        msgSlt->mttohash = hash(msgBuf->mbto);

        msgSlt->mtmsgflags.MAIL = 1;

        if (*msgBuf->mbfwd)
            msgSlt->mtfwdhash = hash(msgBuf->mbfwd);
    } else if (*msgBuf->mbgroup) {
        msgSlt->mttohash = hash(msgBuf->mbgroup);
        msgSlt->mtmsgflags.LIMITED = 1;
    }
    if (*msgBuf->mboname)
        msgSlt->mtorigin = hash(msgBuf->mboname);

	if (strcmpi(msgBuf->mbzip, cfg->nodeTitle) != SAMESTRING && *msgBuf->mbzip) {
        msgSlt->mtmsgflags.NET = 1;
        msgSlt->mttohash = hash(msgBuf->mbzip);
    }
    if (*msgBuf->mbx)
        msgSlt->mtmsgflags.PROBLEM = 1;

    msgSlt->mtmsgflags.RECEIVED =
    ((msgBuf->mbattr & ATTR_RECEIVED) == ATTR_RECEIVED);

    msgSlt->mtmsgflags.REPLY =
    ((msgBuf->mbattr & ATTR_REPLY) == ATTR_REPLY);

    msgSlt->mtmsgflags.MADEVIS =
    ((msgBuf->mbattr & ATTR_MADEVIS) == ATTR_MADEVIS);

    msgSlt->mtroomno = msgBuf->mbroomno;

    /* This is special. */
    if (*msgBuf->mbcopy) {
        msgSlt->mtmsgflags.COPY = 1;

    /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        msgSlt->mtoffset = (ushort) (here - copy);
    }
    if (roomBuild)
        buildroom();
}

/* EOF */
