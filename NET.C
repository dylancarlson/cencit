/* -------------------------------------------------------------------- */
/*  NET.C                     Citadel                                   */
/* -------------------------------------------------------------------- */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <alloc.h>
#include <string.h>
#include <time.h>
#include "response.h"			/* Hayes command responses */
#include "ctdl.h"

#ifndef ATARI_ST
#include <conio.h>
#include <dos.h>
#endif

#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*	GetMessage()	Gets a message from a file							*/
/*  fPutMessage()   Puts a message to a file                            */
/*  NewRoom()       Puts all new messages in a room to a file           */
/*  saveMessage()   saves a message to file if it is netable            */
/*  ReadMsgFl()     Reads a message file into thisRoom                  */
/*	NfindRoom() 	find the room for mail								*/
/*  readnode()      read the node.cit to get the nodes info for logbuf  */
/*  getnode()       read the node.cit to get the nodes info             */
/*  net_slave()     network entry point from LOGIN                      */
/*  net_master()    entry point to call a node                          */
/*  slave()         Actual networking slave                             */
/*  master()        During network master code                          */
/*  n_dial()        call the bbs in the node buffer                     */
/*  n_login()       Login to the bbs with the macro in the node file    */
/*  wait_for()      wait for a string to come from the modem            */
/*  net_callout()   Entry point from Cron.C                             */
/*  cleanup()       Done with other system, save mail and messages      */
/*  netcanseeroom() Can the node see this room?                         */
/*  alias()         return the name of the BBS from the #ALIAS          */
/*  route()         return the routing of a BBS from the #ROUTE         */
/*  alias_route()   returns the route or alias specified                */
/*  get_first_room()    get the first room in the room list             */
/*  get_next_room() gets the next room in the list                      */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 *
 *  HISTORY:
 *
 *  06/05/89  (PAT)  Made history, cleaned up comments, reformatted
 *                   icky code.
 *  02/06/90  {zm}   Fix #GROUP bug so they're in right order.
 *  03/09/90  {zm}   Release all incoming [viewable twit] msgs.
 *                   Add netmail count to "messages entered" in trap.
 *  03/10/90  {zm}   If not sysop, send [viewable] msgs as normal.
 *  03/11/90  {zm}   Change the above to sysop >or< aide.
 *                   Add a default group for group-only msgs.
 *  03/11/90  FJM    Usage of memset in netting
 *  03/15/90  {zm}   Add [title] name [surname] everywhere.
 *  03/16/90  {zm}   Add route path and other new DragCit msg fields.
 *  04/08/90  FJM    Force creation of empty mail file if none exists
 *  04/18/90  FJM    Cleaned up networking code some in NewRoom().
 *  04/19/90  FJM    Cleaned up string copy routines
 *  08/10/90  FJM    Renamed mail file to mesg.tmp before send
 *                   in slave()
 *  08/16/90  FJM    Modified group mapping for no group found.
 *  09/02/90  FJM    Fixed length of "str" in ReadMsgFl().
 *  10/13/90  FJM    Changed length test in GetStr().
 *  10/18/90  FJM    Added #DEFAULT_ROUTE keyword.
 *  12/01/90  FJM    Added local netting support.
 *  12/02/90  FJM    Enhanced local netting.
 *  12/19/90  FJM    Modifed ReadMessage to drop messages without
 *                   a digit starting the source ID field.
 *  02/05/91  FJM    Added modem response code detection for dial out.
 *  02/10/91  FJM    Made response code check explicit.
 *  02/28/91  FJM    Flush modem before response detect in verbose_response().
 *  04/11/91  FJM    Added delay before buffer flush after dial out.
 *	07/04/91  BLJ	 Added the strip_sound() function.
 *	07/05/91  BLJ	 Fixed a local netting bug causing phantom aide messages
 *	08/08/91  BLJ	 Cleanup of dialing display
 *	09/12/91  BLJ	 Added new net fields
 *	09/14/91  BLJ	 Changed to Dynamically Linked Lists
 *	09/25/91  BLJ	 Added net path internal trucation- !..!Memory Alpha
 *	09/25/91  BLJ	 Added strip_ansi() where needed
 *	11/16/92  BLJ	 Increase netpath and add Software orig.node code
 *	03/18/92  BLJ	 Increase "Waiting..." to 3 characters
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Global Data                                                         */
/* -------------------------------------------------------------------- */
int local_mode = FALSE;

/*
 * Shared with MSG3.C
 */

label default_route = "";

/* local functions */
static void NewRoom(int room, char *filename);
static unsigned char net_master(void);
static unsigned char netcanseeroom(int roomslot);
static void release_mem(void);
static int NfindRoom(char *str);
static void saveMessage(unsigned long id, FILE *fl);
static void fPutMessage(FILE *fl);
static int ReadMsgFl(int room, char *filename, char *here, char *there);
static unsigned char wait_for(char *str);
static unsigned char GetMessage(FILE *fl);
static unsigned char get_first_room(char *here, char *there);
static unsigned char get_next_room(char *here, char *there);
static void save_special(char *savename);
static unsigned char alias_route(char *str, char *srch);
static unsigned char n_dial(void);
static unsigned char n_login(void);
static int verbose_response(char *buf);
static void strip_sound(void);

/* -------------------------------------------------------------------- */
/*  GetMessage()    Gets a message from a file, returns success     (i) */
/* -------------------------------------------------------------------- */
BOOL GetMessage(FILE * fl)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do {
		c = fgetc(fl);
    } while (c != -1 && !feof(fl));

    if (feof(fl))
        return FALSE;

    /* get message's attribute byte */
    msgBuf->mbattr = (uchar) fgetc(fl);

    GetStr(fl, msgBuf->mbId, NAMESIZE);

    do {
		c = fgetc(fl);
        switch (c) {
            case 'A':
                GetStr(fl, msgBuf->mbauth, NAMESIZE);
                break;
            case 'N':
                GetStr(fl, msgBuf->mbtitle, NAMESIZE);
                break;
            case 'n':
                GetStr(fl, msgBuf->mbsur, NAMESIZE);
                break;
            case 'D':
                GetStr(fl, msgBuf->mbtime, NAMESIZE);
                break;
            case 'G':
                GetStr(fl, msgBuf->mbgroup, NAMESIZE);
                break;
            case 'I':
                GetStr(fl, msgBuf->mbreply, NAMESIZE);
                break;
            case 'M':       /* will be read off disk later */
                break;
            case 'O':
                GetStr(fl, msgBuf->mboname, NAMESIZE);
                break;
            case 'o':
                GetStr(fl, msgBuf->mboreg, NAMESIZE);
                break;
			case 'Q':
				GetStr(fl, msgBuf->mbostate, NAMESIZE);
                break;
			case 'U':
                GetStr(fl, msgBuf->mbocont, NAMESIZE);
                break;
			case 'H':
				GetStr(fl, msgBuf->mbophone, NAMESIZE);
                break;
			case 's':
				GetStr(fl, msgBuf->mbsoftware, NAMESIZE);
                break;
            case 'P':
				GetStr(fl, msgBuf->mbfpath, 255);
                break;
            case 'p':
				GetStr(fl, msgBuf->mbtpath, 255);
                break;
            case 'R':
                GetStr(fl, msgBuf->mbroom, NAMESIZE);
                break;
            case 'S':
                GetStr(fl, msgBuf->mbsrcId, NAMESIZE);
                break;
            case 'T':
                GetStr(fl, msgBuf->mbto, NAMESIZE);
                break;
            case 'X':
                GetStr(fl, msgBuf->mbx, NAMESIZE);
                break;
            case 'Z':
                GetStr(fl, msgBuf->mbzip, NAMESIZE);
                break;
            case 'z':
                GetStr(fl, msgBuf->mbrzip, NAMESIZE);
                break;
            case 'q':
				GetStr(fl, msgBuf->mbszip, NAMESIZE);
                break;
			case 'u':
                GetStr(fl, msgBuf->mbczip, NAMESIZE);
                break;
			case 'h':
				GetStr(fl, msgBuf->mbzphone, NAMESIZE);
                break;

			case 'J':
				GetStr(fl, msgBuf->mbtreg, NAMESIZE);
                break;

			case 'j':
				GetStr(fl, msgBuf->mbtcont, NAMESIZE);
                break;

            default:
				GetStr(fl, msgBuf->mbtext, cfg->maxtext);	 /* discard unknown field */
                msgBuf->mbtext[0] = '\0';
                break;
        }
    } while (c != 'M' && !feof(fl));

    if (!*msgBuf->mboname) {    /* 'O' */
		strncpy(msgBuf->mboname, node->ndname, sizeof(label) - 1);
    }
    if (!*msgBuf->mboreg) { /* 'o' */
		strncpy(msgBuf->mboreg, node->ndregion, sizeof(label) - 1);
    }
	if(stricmp(msgBuf->mboname, cfg->nodeTitle) == SAMESTRING) {
		/* It's us, just old data in the message base   */
        if (!*msgBuf->mbsoftware) { /* 's' */
			sprintf(msgBuf->mbsoftware, "%s Ver %s", softname, version);
		}
		if (!*msgBuf->mbostate) { /* 'Q' */
			strncpy(msgBuf->mbostate, cfg->nodeState, sizeof(label) - 1);
		}
		if (!*msgBuf->mbocont) { /* 'U' */
			strncpy(msgBuf->mboreg, cfg->nodeCntry, sizeof(label) - 1);
		}
		if (!*msgBuf->mbophone) { /* 'H' */
			strncpy(msgBuf->mboreg, cfg->nodePhone, sizeof(label) - 1);
		}
		if (!*msgBuf->mbtreg && cfg->SysOpRegion) { /* 'J' */
			strncpy(msgBuf->mbtreg, cfg->SysOpRegion, sizeof(label) - 1);
        }
		if (!*msgBuf->mbtcont && cfg->SysOpCntry) { /* 'j' */
			strncpy(msgBuf->mbtcont, cfg->SysOpCntry, sizeof(label) - 1);
        }
    }
	if (!*msgBuf->mbsrcId) {	/* 'S' */
        strncpy(msgBuf->mbsrcId, msgBuf->mbId, sizeof(label) - 1);
    }
    if (!*msgBuf->mbfpath) {    /* 'P' - no route path, so start one. */
		strncpy(msgBuf->mbfpath, node->ndname, sizeof(label) - 1);
    }
    if (feof(fl))
        return FALSE;

	GetStr(fl, msgBuf->mbtext, cfg->maxtext);	 /* get the message field  */

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  fPutMessage()    Puts a message to a file                        (o) */
/* -------------------------------------------------------------------- */
void fPutMessage(FILE *fl)
{
	uchar i, chpos, chpos2;

	/* write start of message */
    fputc(0xFF, fl);

    /* put message's attribute byte */
    fputc(msgBuf->mbattr, fl);

    /* put local ID # out */
    PutStr(fl, msgBuf->mbId);

    if (msgBuf->mbauth[0]) {
        fputc('A', fl);
        PutStr(fl, msgBuf->mbauth);
    }
    if (!msgBuf->mbtime[0]) {
        sprintf(msgBuf->mbtime, "%ld", time(NULL));
    }
    fputc('D', fl);
    PutStr(fl, msgBuf->mbtime);

    if (msgBuf->mbgroup[0]) {
        fputc('G', fl);
		PutStr(fl, strip_ansi(msgBuf->mbgroup));
    }
    if (msgBuf->mbreply[0]) {
        fputc('I', fl);
        PutStr(fl, msgBuf->mbreply);
    }
    if (!msgBuf->mboname[0]) {
		strncpy(msgBuf->mboname, strip_ansi(cfg->nodeTitle),
			sizeof(label) - 1);
		strncpy(msgBuf->mboreg, strip_ansi(cfg->nodeRegion),
			sizeof(label) - 1);
    }
    if (msgBuf->mboname[0]) {
        fputc('O', fl);
        PutStr(fl, msgBuf->mboname);
    }
    if (msgBuf->mboreg[0]) {
        fputc('o', fl);
        PutStr(fl, msgBuf->mboreg);
    }
	if(stricmp(msgBuf->mboname, cfg->nodeTitle) == SAMESTRING) {
        /* It's us, just old data in the message base   */

		if (!msgBuf->mbsoftware[0]) {
			sprintf(msgBuf->mbsoftware, "%s Ver %s", softname, version);
        }
		if (!msgBuf->mbostate[0]) {
			strncpy(msgBuf->mbostate, cfg->nodeState, sizeof(label) - 1);
		}
		if (!msgBuf->mbocont[0]) {
			strncpy(msgBuf->mbocont, cfg->nodeCntry, sizeof(label) - 1);
		}
		if (!msgBuf->mbophone[0]) {
			strncpy(msgBuf->mbophone, cfg->nodePhone, sizeof(label) - 1);
		}
		if (!msgBuf->mbtreg[0] && cfg->SysOpRegion) {
			strncpy(msgBuf->mbtreg, cfg->SysOpRegion, sizeof(label) - 1);
        }
		if (!msgBuf->mbtcont[0] && cfg->SysOpCntry) {
			strncpy(msgBuf->mbtcont, cfg->SysOpCntry, sizeof(label) - 1);
        }
    }
	if (msgBuf->mbostate[0]) {
		fputc('Q', fl);
		PutStr(fl, msgBuf->mbostate);
	}
	if (msgBuf->mbocont[0]) {
		fputc('U', fl);
		PutStr(fl, msgBuf->mbocont);
	}
	if (msgBuf->mbophone[0]) {
		fputc('H', fl);
		PutStr(fl, msgBuf->mbophone);
	}
	if (msgBuf->mbsoftware[0]) {
		fputc('s', fl);
		PutStr(fl, msgBuf->mbsoftware);
    }
    if (msgBuf->mbtreg[0]) {
		fputc('J', fl);
		PutStr(fl, msgBuf->mbtreg);
    }
	if (msgBuf->mbtcont[0]) {
		fputc('j', fl);
		PutStr(fl, msgBuf->mbtcont);
    }
    fputc('P', fl);     /* always have a route path */
    if (msgBuf->mbfpath[0]) {

/* New logic
 We want to change from:
  Amber!Photosynthesis!Shades of Blue!Programmers Golden Realm!Memory Alpha
	To:
  Amber!..!Programmers Golden Realm!Memory Alpha!Centauri's Place
 */

		if(strlen(msgBuf->mbfpath) + strlen(cfg->nodeTitle) + 2 >= 255) {
			chpos = 0;
			while(msgBuf->mbfpath[chpos++] != '!');
			while(msgBuf->mbfpath[chpos] == '!') chpos++;
			msgBuf->mbfpath[chpos++] = '.';
			msgBuf->mbfpath[chpos++] = '.';
			msgBuf->mbfpath[chpos++] = '!';
			chpos2 = chpos;
			while(msgBuf->mbfpath[chpos2++] != '!');
			while(msgBuf->mbfpath[chpos2] == '!') chpos2++;

			for(i=chpos; msgBuf->mbfpath[i+(chpos2-chpos)]; i++)
				msgBuf->mbfpath[i] = msgBuf->mbfpath[i+(chpos2-chpos)];

			msgBuf->mbfpath[i] = '\0';

			while (strlen(msgBuf->mbfpath) + strlen(cfg->nodeTitle) + 2 >= 255) {
				chpos2 = chpos;
				while(msgBuf->mbfpath[chpos2++] != '!');
				while(msgBuf->mbfpath[chpos2] == '!') chpos2++;
				for(i=chpos; msgBuf->mbfpath[i+(chpos2-chpos)]; i++)
					msgBuf->mbfpath[i] = msgBuf->mbfpath[i+(chpos2-chpos)];

				msgBuf->mbfpath[i] = '\0';
			}
		}
		strcat(msgBuf->mbfpath, "!");
		strcat(msgBuf->mbfpath, cfg->nodeTitle);
    } else {
		strcpy(msgBuf->mbfpath, cfg->nodeTitle); /* our node name only */
    }
    PutStr(fl, msgBuf->mbfpath);

    if (msgBuf->mbtpath[0]) {
        fputc('p', fl);
        PutStr(fl, msgBuf->mbtpath);
    }
    if (msgBuf->mbroom[0]) {
        fputc('R', fl);
		PutStr(fl, strip_ansi(msgBuf->mbroom));
    }
    if (!msgBuf->mbsrcId[0]) {
        strncpy(msgBuf->mbsrcId, msgBuf->mbId, sizeof(label) - 1);
    }
    if (msgBuf->mbsrcId[0]) {
        fputc('S', fl);
        PutStr(fl, msgBuf->mbsrcId);
    }
    if (msgBuf->mbtitle[0]) {
        fputc('N', fl);
        PutStr(fl, msgBuf->mbtitle);
    }
    if (msgBuf->mbsur[0]) {
        fputc('n', fl);
        PutStr(fl, msgBuf->mbsur);
    }
    if (msgBuf->mbto[0]) {
        fputc('T', fl);
        PutStr(fl, msgBuf->mbto);
    }
    if (msgBuf->mbx[0]) {   /* problem user message */
        fputc('X', fl);
        PutStr(fl, msgBuf->mbx);
    }
    if (msgBuf->mbzip[0]) {
        fputc('Z', fl);
        PutStr(fl, msgBuf->mbzip);
    }
    if (msgBuf->mbrzip[0]) {
        fputc('z', fl);
        PutStr(fl, msgBuf->mbrzip);
    }
	if (msgBuf->mbszip[0]) {
        fputc('q', fl);
		PutStr(fl, msgBuf->mbszip);
    }
	if (msgBuf->mbczip[0]) {
		fputc('u', fl);
        PutStr(fl, msgBuf->mbczip);
    }
	if (msgBuf->mbzphone[0]) {
		fputc('h', fl);
		PutStr(fl, msgBuf->mbzphone);
    }
    /* put the message field  */
    fputc('M', fl);
    PutStr(fl, msgBuf->mbtext);
}

/* -------------------------------------------------------------------- */
/*  NewRoom()       Puts all new messages in a room to a file           */
/* -------------------------------------------------------------------- */
void NewRoom(int room, char *filename)
{
    int i, h;
	char str[100];
    ulong lowLim, highLim, msgNo;
    FILE *file;
    unsigned tsize;

    lowLim = logBuf.lbvisit[logBuf.lbroom[room].lvisit] + 1;
	highLim = cfg->newest;

    logBuf.lbroom[room].lvisit = 0;

    /* stuff may have scrolled off system unseen, so: */
	if (cfg->oldest > lowLim)
		lowLim = cfg->oldest;

	flipFileLan(cfg->temppath,1);

	sprintf(str, "%s\\%s", cfg->temppath, filename);
	flipFileLan(cfg->temppath,0);

    file = fopen(str, "ab");
    if (!file) {
        return;
    }
	h = hash(cfg->nodeTitle);
    /* need to clean up here !!!! Start at first new message! */
    /* don't sizetable() every pass */
    tsize = sizetable();
    for (i = 0; i != tsize; i++) {
    /* skip messages not in this room */
        if (msgTab[i].mtroomno != (uchar) room)
            continue;

    /* no open messages from the system */
        if (msgTab[i].mtauthhash == h)
            continue;

    /* skip mail */
        if (msgTab[i].mtmsgflags.MAIL)
            continue;

    /* skip >unreleased< twit/moderated messages only.         */
    /* remember that 'aide_can_see_moderated' is in CONFIG.CIT */
    /* but we are ignoring that fact since it's too much fuss. */
/*      (omit for testing)
        if ((msgTab[i].mtmsgflags.PROBLEM
          || msgTab[i].mtmsgflags.MODERATED)
         && (!msgTab[i].mtmsgflags.MADEVIS)
         && (!msgTab[i].mtmsgflags.RELEASED)
         && !(sysop || aide))  continue;
*/
		msgNo = (ulong) (cfg->mtoldest + i);

        if (msgNo >= lowLim && highLim >= msgNo) {
            saveMessage(msgNo, file);
            mread++;
        }
    }
    fclose(file);
}

/* -------------------------------------------------------------------- */
/*  saveMessage()   saves a message to file if it is nettable       (o) */
/* -------------------------------------------------------------------- */
#define msgstuff  (msgTab[slot].mtmsgflags)
void saveMessage(ulong id, FILE * fl)
{
    ulong here;
    ulong loc;
	int slot;

    slot = indexslot(id);

    if (slot == ERROR)
        return;

    if (msgTab[slot].mtmsgflags.COPY) {
        copyflag = TRUE;
        originalId = id;
        originalattr = 0;

        originalattr = (uchar)
        (originalattr | (msgstuff.RECEIVED) ? ATTR_RECEIVED : 0);
        originalattr = (uchar)
        (originalattr | (msgstuff.REPLY) ? ATTR_REPLY : 0);
        if (sysop || aide) {    /* non-sysop (or aide) gets "normal" messages. */
            originalattr = (uchar)
            (originalattr | (msgstuff.MADEVIS) ? ATTR_MADEVIS : 0);
        }
        if (msgTab[slot].mtoffset <= slot)
            saveMessage((ulong) (id - (ulong) msgTab[slot].mtoffset), fl);

        return;
    }
    /* in case it returns without clearing buffer */
    msgBuf->mbfwd[0] = '\0';
    msgBuf->mbto[0] = '\0';

    loc = msgTab[slot].mtmsgLoc;
    if (loc == ERROR)
        return;

    if (copyflag) {
        slot = indexslot(originalId);
    } else {            /* the following doesn't notice copyflag. */
        if (!mayseeindexmsg(slot) && !msgTab[slot].mtmsgflags.NET)
            return;
    }

    fseek(msgfl, loc, 0);

    getMessage();
	getMsgStr(msgBuf->mbtext, cfg->maxtext);

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if ((int) here == 1)
        return;

    if (!mayseemsg() && !msgTab[slot].mtmsgflags.NET)
        return;

    if (here != id) {
		if(debug) {
			cPrintf(" Can't find message. Looking for %lu at byte %ld! (net)\n ",
			id, loc);
			return;
		}
    }
    if (!(sysop || aide)) {
        msgBuf->mbattr &= ~(ATTR_MADEVIS);  /* strip the [viewable] bit */
        msgBuf->mbx[0] = '\0';  /* since it's a normal msg. */
    }
    if (msgBuf->mblink[0])
        return;

	if(!logBuf.SOUND)
		strip_sound(); /* strip out the ^B music ^B 0x0E */

	fPutMessage(fl);
}

/*--- CentWin ------------------------------------------------*/
/* Strip out all music and sound effects before netting 	  */
/*------------------------------------------------------------*/
static void strip_sound(void)
{
    int i, b1pos, b2pos;

    /* Now we are going to "crunch up" the message by moving all the
     * data after the second ^B to where the first ^B starts
     */

    do {
        b1pos = b2pos = 0;
		for(i=0; i<cfg->maxtext; i++) {
            if(msgBuf->mbtext[i] == 0x02) {
                if(b2pos) {
                    msgBuf->mbtext[b1pos++] = msgBuf->mbtext[i];
                }
                else {
                    if(!b1pos)
                        b1pos = i;
                    else
                        b2pos = ++i;
                }
            }
            else {
                if(b2pos) {
                    msgBuf->mbtext[b1pos++] = msgBuf->mbtext[i];
                }
                if(msgBuf->mbtext[i] == '\0') break;
            }
        }
    }while(b2pos);
}


/* -------------------------------------------------------------------- */
/*  ReadMsgFile()   Reads a message file into thisRoom              (i) */
/* -------------------------------------------------------------------- */
int ReadMsgFl(int room, char *filename, char *here, char *there)
{
    FILE *file;
    ulong oid, id, loc;
    long l;
	int i, bad, oname, temproom, lp;
    int goodmsg = 0;
    int rejects = 0;
    char str[80];

	changedir(cfg->temppath);
    expired = duplicate = 0;

    file = fopen(filename, "rb");

    if (!file)
        return -1;

    while (GetMessage(file) == TRUE) {
        msgBuf->mbroomno = (uchar) room;

        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        oname = hash(msgBuf->mboname);

        memcpy(msgBuf2, msgBuf, sizeof(struct msgB));

        bad = FALSE;

		if (strcmpi(cfg->nodeTitle, msgBuf->mboname) == SAMESTRING) {
            bad = TRUE;
            duplicate++;
        }
        if (*msgBuf->mbzip) {   /* is mail */
			/* not for this system */
			if (strcmpi(msgBuf->mbzip, cfg->nodeTitle) != SAMESTRING) {
                if (!save_mail()) {
                    clearmsgbuf();
					strncpy(msgBuf->mbauth, cfg->nodeTitle, sizeof(label) - 1);
                    strncpy(msgBuf->mbto, msgBuf2->mbauth, sizeof(label) - 1);
                    strncpy(msgBuf->mbzip, msgBuf2->mboname, sizeof(label) - 1);
                    strncpy(msgBuf->mbrzip, msgBuf2->mboreg, sizeof(label) - 1);
					strncpy(msgBuf->mbszip, msgBuf2->mbostate, sizeof(label) - 1);
					strncpy(msgBuf->mbczip, msgBuf2->mbocont, sizeof(label) - 1);
					strncpy(msgBuf->mbzphone, msgBuf2->mbophone, sizeof(label) - 1);
                    strncpy(msgBuf->mbroom, msgBuf2->mbroom, sizeof(label) - 1);
                    sprintf(msgBuf->mbtext,
                    " \n Can not route mail to '%s' via path '%s'.\n",
                    msgBuf2->mbzip, msgBuf2->mbfpath);
                    amPrintf(
                    " Can not route mail from '%s' to '%s' via path '%s'.\n",
                    msgBuf2->mboname, msgBuf2->mbzip, msgBuf->mbfpath);

                    save_mail();
                }
                bad = TRUE;
            } else {
				/* for this system */
				if (*msgBuf->mbto &&
					strcmpi(msgBuf->mbto, cfg->nodeTitle) == SAMESTRING) {
					save_special("OUTPUT.NET");
					bad = TRUE;
				} else {
					if (*msgBuf->mbto && personexists(msgBuf->mbto) == ERROR
					&& strcmpi(msgBuf->mbto, "Sysop") != SAMESTRING) {
						clearmsgbuf();
						strncpy(msgBuf->mbauth, cfg->nodeTitle, sizeof(label) - 1);
						strncpy(msgBuf->mbto, msgBuf2->mbauth, sizeof(label) - 1);
						strncpy(msgBuf->mbzip, msgBuf2->mboname, sizeof(label) - 1);
						strncpy(msgBuf->mbrzip, msgBuf2->mboreg, sizeof(label) - 1);
						strncpy(msgBuf->mbszip, msgBuf2->mbostate, sizeof(label) - 1);
						strncpy(msgBuf->mbczip, msgBuf2->mbocont, sizeof(label) - 1);
						strncpy(msgBuf->mbzphone, msgBuf2->mbophone, sizeof(label) - 1);
                        strncpy(msgBuf->mbroom, msgBuf2->mbroom, sizeof(label) - 1);
						if(strcmpi(msgBuf2->mbto, "CenCit_Audit") == SAMESTRING) {
							sprintf(msgBuf->mbtext,
							" \n I am a CenCit Node running Version %s on %s.",
							version, cfg->nodeTitle);
						} else {
							sprintf(msgBuf->mbtext,
							" \n No '%s' user found on %s.", msgBuf2->mbto,
							cfg->nodeTitle);
						}
						save_mail();
						bad = TRUE;
					}
				}
            }
        } else {
			/* is public */
			if (!isdigit(*msgBuf->mbId)) {
				save_special("OUTPUT.REJ");
				bad = TRUE;
				++rejects;
			}
            if (!bad) {
                for (i = sizetable(); i != -1 && !bad; i--) {
                    if (msgTab[i].mtorigin == oname
                    && oid == msgTab[i].mtomesg) {
                        loc = msgTab[i].mtmsgLoc;
                        fseek(msgfl, loc, 0);
                        getMessage();
                        if (strcmpi(msgBuf->mbauth, msgBuf2->mbauth)
                            == SAMESTRING
                            && strcmpi(msgBuf->mboname, msgBuf2->mboname)
                            == SAMESTRING
                            && strcmpi(msgBuf->mbtime, msgBuf2->mbtime)
                            == SAMESTRING
                        ) {
                            bad = TRUE;
                            duplicate++;
                        }
                    }
                }
            }
            memcpy(msgBuf, msgBuf2, sizeof(struct msgB));

			/* fix group only messages, or discard them! */
            if (*msgBuf->mbgroup && !bad) {
                bad = TRUE;

				ndgrp = start_ndgrp;
				while(ndgrp->here[0]) {
					if (strcmpi(ndgrp->there, msgBuf->mbgroup)
						== SAMESTRING) {	/* fixed */
						strncpy(msgBuf->mbgroup, ndgrp->here,sizeof(label) - 1);
                        bad = FALSE;
                    }
					ndgrp = ndgrp->next;
					if(ndgrp == NULL) break;
				}
                if (bad) {  /* still didn't find the group! */
                    if (default_group[0]) {
                        sprintf(str, "Message #%s mapped from group '%s'",
                        msgBuf->mbId, msgBuf->mbgroup);
                        trap(str, T_NETWORK);
                        amPrintf("%s\n", str);
                        netError = TRUE;
                        strncpy(msgBuf->mbgroup, default_group, sizeof(label) - 1);
                        bad = FALSE;
                    }       /* #DEFAULT_GROUP is in external.cit */
                }
            }
			/* Expired? */
            if (atol(msgBuf2->mbtime)
			< (time(&l) - ((long) node->ndexpire * 60 * 60 * 24))
			&& !*msgBuf->mbto) {
                bad = TRUE;
                expired++;
            }
        }

        if (!bad) {     /* it's good, save it */
            temproom = room;

			strcpy(str,strip_ansi(msgBuf->mbroom));
			strcpy(msgBuf->mbroom,str);

			if (*msgBuf->mbto) {
				if(*msgBuf->mbroom) {
					ndroom = start_ndroom;
					while(ndroom->here[0]) {
						if (
						strcmpi(msgBuf->mbroom,
							strip_ansi(ndroom->there)) == SAMESTRING)
						{
							strncpy(msgBuf->mbroom, strip_ansi(ndroom->here),
								sizeof(label) - 1);
							break;
						}
						ndroom = ndroom->next;
						if(ndroom == NULL) break;
					}
				}
				temproom = NfindRoom(msgBuf->mbroom);
			}
			else {
				if (strcmpi(msgBuf->mbroom,
					strip_ansi(there)) == SAMESTRING)
					strncpy(msgBuf->mbroom, strip_ansi(here),
						sizeof(label) - 1);
			}
            msgBuf->mbroomno = (uchar) temproom;

            putMessage();
            noteMessage();
            goodmsg++;

            if (*msgBuf->mbto) {
                lp = thisRoom;
				getRoom(temproom);
				notelogmessage(msgBuf->mbto);
				getRoom(lp);
            }
			/* a viewable twit/moderated message.
			 * Simulate a local aide releasing it.
			 */

            if (msgBuf->mbx[0] && (msgBuf->mbattr & ATTR_MADEVIS)) {
                sscanf(msgBuf->mbId, "%lu", &id);
                copymessage(id, (uchar) temproom);
            }
        }
    }
    fclose(file);
    if (rejects)
        amPrintf(" Rejected %d messages with non-numeric source ID numbers.\n",
			rejects);


    return goodmsg;
}

/* -------------------------------------------------------------------- */
/*	NfindRoom() 	find the room for mail								*/
/* -------------------------------------------------------------------- */
int NfindRoom(char *str)
{
	int i;

	i = roomExists(str);

	if (i == ERROR) i = MAILROOM;

    return (i);
}

/* -------------------------------------------------------------------- */
/*  readnode()      read the nodes.cit to get the nodes info for logbuf */
/* -------------------------------------------------------------------- */
BOOL readnode(void)
{
    return getnode(logBuf.lbname);
}

/* -------------------------------------------------------------------- */
/*  getnode()       read the nodes.cit to get the node info             */
/* -------------------------------------------------------------------- */
BOOL getnode(char *nodename)
{
    FILE *fBuf;
	char line[240], ltmp[240];
    char *words[256];
    int i, j, k, found = FALSE;
    long pos, ftell();
	char path[80];

	flipFileLan(cfg->homepath,1);

	sprintf(path, "%s\\nodes.cit", cfg->homepath);

	flipFileLan(cfg->homepath,0);

    if ((fBuf = fopen(path, "r")) == NULL) {    /* ASCII mode */
        cPrintf("Can't find nodes.cit!");
        doccr();
        return FALSE;
    }
    pos = ftell(fBuf);
	while (fgets(line, 239, fBuf) != NULL) {
        if (line[0] != '#') {
            pos = ftell(fBuf);
            continue;
        }
        if (!found && strnicmp(line, "#NODE", 5) != SAMESTRING) {
            pos = ftell(fBuf);
            continue;
        }
        strcpy(ltmp, line);
        parse_it(words, line);

        for (i = 0; nodekeywords[i] != NULL; i++) {
            if (strcmpi(words[0], nodekeywords[i]) == SAMESTRING) {
                break;
            }
        }

        if (i == NOK_NODE) {
            if (found) {
                fclose(fBuf);
                return TRUE;
            }
            if (strcmpi(nodename, words[1]) == SAMESTRING) {
                found = TRUE;
	/* initialize a few optional fields */
				node->ndpacing = 0;  /*  Set it anyways!  */
				node->ndScriptSlave[0] = '\0';
				node->ndScriptMaster[0] = '\0';
					}
        }
        if (found)
        switch (i) {
            case NOK_BAUD:
                j = atoi(words[1]);
            switch (j) {    /* ycky hack */
                case 300:
					node->ndbaud = 0;
                    break;
                case 1200:
					node->ndbaud = 1;
                    break;
                case 2400:
					node->ndbaud = 2;
                    break;
                case 4800:
					node->ndbaud = 3;
                    break;
                case 9600:
					node->ndbaud = 4;
                    break;
                default:
					node->ndbaud = 1;
                    break;
            }
                break;

            case NOK_PHONE:
                if (strlen(words[1]) < NAMESIZE)
					strncpy(node->ndphone, words[1], sizeof(label) - 1);
                break;

            case NOK_PROTOCOL:
                if (strlen(words[1]) < NAMESIZE)
					strncpy(node->ndprotocol, words[1], sizeof(label) - 1);
                break;
			case NOK_SCRIPT_SLAVE:
				if (strlen(words[1]) < 13)
					strncpy(node->ndScriptSlave, words[1], 12);
                break;

			case NOK_SCRIPT_MASTER:
				if (strlen(words[1]) < 13)
					strncpy(node->ndScriptMaster, words[1], 12);
                break;


            case NOK_MAIL_TMP:
                if (strlen(words[1]) < NAMESIZE)
					strncpy(node->ndmailtmp, words[1], sizeof(label) - 1);
                break;

            case NOK_LOGIN:
				strncpy(node->ndlogin, ltmp, 238);
				break;

            case NOK_NODE:
                if (strlen(words[1]) < NAMESIZE)
					strncpy(node->ndname, words[1], sizeof(label) - 1);
                if (strlen(words[2]) < NAMESIZE)
					strncpy(node->ndregion, words[2], sizeof(label) - 1);

                ndgrp = start_ndgrp;
				ndgrp->here[0] = '\0';

                node->roomoff = 0L;
                break;

            case NOK_REDIAL:
				node->ndredial = atoi(words[1]);
                break;

            case NOK_DIAL_TIMEOUT:
				node->nddialto = atoi(words[1]);
                break;

            case NOK_WAIT_TIMEOUT:
				node->ndwaitto = atoi(words[1]);
                break;

            case NOK_EXPIRE:
				node->ndexpire = atoi(words[1]);
                break;

            case NOK_ROOM:
				if (!node->roomoff)
					node->roomoff = pos;
                break;

			case NOK_PACING:
				node->ndpacing = atoi(words[1]);
				break;

            case NOK_GROUP:
				ndgrp = start_ndgrp;
				j = 0;
				k = ERROR;
				while(TRUE) {
					if (ndgrp->here[0] == '\0') {
                        k = j;
						break;
                    }
					j++;
					if(j >= MAXGROUPS) break;
					if(ndgrp->next == NULL) {
						ndgrp->next = farcalloc(1UL, (ulong) sizeof(struct ndgroups));
						if(ndgrp->next == NULL) {
							crashout("Can not allocate memory for node group");
						}
						ndgrp = ndgrp->next;
						ndgrp->next = NULL;
						ndgrp->here[0] = '\0';
					}
					else ndgrp = ndgrp->next;
                }

                if (k == ERROR) {
                    cPrintf("Too many groups!!\n ");
                    break;
                }
                if (strlen(words[1]) < NAMESIZE)
					strncpy(ndgrp->here, strip_ansi(words[1]),
						sizeof(label) - 1);
                if (strlen(words[2]) < NAMESIZE)
					strncpy(ndgrp->there, strip_ansi(words[2]),
						sizeof(label) - 1);
                if (!strlen(words[2]))
					strncpy(ndgrp->there, strip_ansi(words[1]),
						sizeof(label) - 1);
                break;

            default:
                cPrintf("Nodes.cit - Warning: Unknown variable %s", words[0]);
                doccr();
                break;
        }
        pos = ftell(fBuf);
    }
    fclose(fBuf);
    return (BOOL) (found);
}

/* -------------------------------------------------------------------- */
/*  net_slave()     network entry point from LOGIN                      */
/* -------------------------------------------------------------------- */
BOOL net_slave(void)
{
    if  (readnode() == FALSE)
        return FALSE;

    if (debug) {
		cPrintf("Node:  \"%s\" \"%s\"", node->ndname, node->ndregion);
        doccr();
		cPrintf("Phone: \"%s\" %d", node->ndphone, node->nddialto);
        doccr();
		cPrintf("Login: \"%s\" %d", node->ndlogin, node->ndwaitto);
        doccr();
		cPrintf("Baud:  %d    Protocol: \"%s\"\n ", node->ndbaud, node->ndprotocol);
		cPrintf("Expire:%d    Waitout:  %d", node->ndexpire, node->ndwaitto);
        doccr();
    }
    netError = FALSE;

    /* cleanup */
	changedir(cfg->temppath);
    ambigUnlink("room.[0-9]*", FALSE);
    ambigUnlink("roomin.*", FALSE);

    if (slave() && master()) {
        cleanup();
		did_net(node->ndname);
        return TRUE;
    } else {
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
		release_mem();
		return FALSE;
    }
}

/* -------------------------------------------------------------------- */
/*  net_master()    entry point to call a node (cron event)             */
/* -------------------------------------------------------------------- */
BOOL net_master(void)
{
    if  (readnode() == FALSE) {
        cPrintf("\n No nodes.cit entry!\n ");
        return FALSE;
    }
	netError = FALSE;
    if (!local_mode) {
        if (n_dial() == FALSE)
            return FALSE;
        if (n_login() == FALSE)
            return FALSE;
    }
    /* cleanup */
	changedir(cfg->temppath);
    ambigUnlink("room.*", FALSE);
    ambigUnlink("roomin.*", FALSE);

    if (master() && slave()) {
        cleanup();
		did_net(node->ndname);
        return TRUE;

    } else {
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
		release_mem();
        return FALSE;
    }
}

/* -------------------------------------------------------------------- */
/*  slave()         Actual networking slave                             */
/* -------------------------------------------------------------------- */
BOOL slave(void)
{
    label troo, fn;
    FILE *file, *fopen();
    int i = 0, rm;

    cPrintf(" Sending mail file.");
    doccr();

	changedir(cfg->homepath);
    /* create empty mail file if it doesn't exist */
	file = fopen(node->ndmailtmp, "rb");
    if (!file) {        /* create empty mail file       */
		file = fopen(node->ndmailtmp, "wb");
    }
    if (file) {
        fclose(file);
    }
    unlink("mesg.tmp");
	rename(node->ndmailtmp, "mesg.tmp");
	wxsnd(cfg->homepath, "mesg.tmp",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));

    /* duplicate code here, make subfunction */
    if (!local_mode && !gotCarrier()) {
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
        return FALSE;
    }
    cPrintf(" Receiving room request file.");
    doccr();

	wxrcv(cfg->temppath, "roomreq.tmp",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));
    if (!local_mode && !gotCarrier()) {
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
        return FALSE;
    }
    file = fopen("roomreq.tmp", "rb");
    if (!file) {
        perror("Error opening roomreq.tmp");
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
        return FALSE;
    }
    GetStr(file, troo, NAMESIZE);

    doccr();
    cPrintf(" Fetching:");
    doccr();

    while (strlen(troo)) {
        rm = roomExists(troo);
        if (rm != ERROR && netcanseeroom(rm)) {
            if (debug) {
				/* trap room requests if debug on (FJM) */
                sprintf(msgBuf->mbtext, "Room requested by remote: %s", troo);
                trap(msgBuf->mbtext, T_NETWORK);
            }
            sprintf(fn, "room.%d", i);
            cPrintf(" %-*s  ", NAMESIZE, troo);
            if (!((i + 1) % 2))
                doccr();
            NewRoom(rm, fn);
        } else {
            doccr();
            cPrintf(" Can't see or find %s.", troo);
            doccr();
            amPrintf(" '%s' room not found for remote.\n", troo);
            netError = TRUE;
        }

        i++;
        GetStr(file, troo, NAMESIZE);
    }
    doccr();
    fclose(file);
    unlink("roomreq.tmp");

    cPrintf(" Sending message files.");
    doccr();

    if (!local_mode && !gotCarrier()) {
		changedir(cfg->homepath);
		rename("mesg.tmp", node->ndmailtmp);
        return FALSE;
    }
	wxsnd(cfg->temppath, "room.*",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));

    ambigUnlink("room.*", FALSE);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  master()        During network master code                          */
/* -------------------------------------------------------------------- */
BOOL master(void)
{
	char line[75], line2[FILESIZE];
    label here, there;
    FILE *file, *fopen();
    int i, rms;
	long lts, ts, tss;

    if (!local_mode && !gotCarrier())
        return FALSE;

	flipFileLan(cfg->temppath,1);

	sprintf(line, "%s\\mesg.tmp", cfg->temppath);
	flipFileLan(cfg->temppath,0);

	unlink(line);

    cPrintf(" Receiving mail file.");
    doccr();

	wxrcv(cfg->temppath, "mesg.tmp",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));

    if (!local_mode && !gotCarrier())
        return FALSE;

	flipFileLan(cfg->temppath,1);

	sprintf(line, "%s\\roomreq.tmp", cfg->temppath);
    unlink(line);
	flipFileLan(cfg->temppath,0);

    if ((file = fopen(line, "ab")) == NULL) {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }
    for (i = get_first_room(here, there), rms = 0;
        i;
    i = get_next_room(here, there), rms++) {
        PutStr(file, there);
    }

    PutStr(file, "");
    fclose(file);

    cPrintf(" Sending room request file.");
    doccr();

	wxsnd(cfg->temppath, "roomreq.tmp",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));
    unlink("roomreq.tmp");

    if (!local_mode && !gotCarrier())
        return FALSE;

    /* show time delay while waiting here */
    /* clear the buffer */
    while (gotCarrier() && MIReady()) {
        getMod();
    }

	cPrintf(" Waiting for transfer...  ");

	time(&tss);

    lts = -1;

    /* wait for them to get their shit together */
	while (gotCarrier() && !MIReady()) {
		ts = (time(NULL) - tss);
		if(ts != lts) {
			cPrintf("\b\b\b%03ld",ts);
			lts = ts;
		}
		if(ts > 999) return FALSE;
	}
    if (!local_mode && !gotCarrier())
        return FALSE;

	cPrintf("\n Receiving message files.");
    doccr();

	wxrcv(cfg->temppath, "",
	(char) strpos((char) tolower(node->ndprotocol[0]), extrncmd));

    for (i = 0; i < rms; i++) {
        sprintf(line, "room.%d", i);
        sprintf(line2, "roomin.%d", i);
        rename(line, line2);
    }

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  n_dial()        call the bbs in the node buffer                     */
/* -------------------------------------------------------------------- */
BOOL n_dial(void)
{
	long lts, ts, tss;
    char str[40];
    char ch;
	char resp_buf[40];
	int resp = HAYES_UNKNOWN;
	int resp_index = 0;

	/* clear the buffer */
	while (gotCarrier() && MIReady()) {
		getMod();
	}

	cPrintf("\n \n Dialing...   ");

    if (debug)
		cPrintf("(%s%s)", cfg->dialpref, node->ndphone);

	baud(node->ndbaud);
    /* update25();	*/
	do_idle(0);

	outstring(cfg->dialsetup);
    outstring("\r");

	delay(1000);
    while (MIReady()) {
        ch = (char) getMod();
        if (debug)
            outCon(ch);
    }
	
	strcpy(str, cfg->dialpref);
	strcat(str, node->ndphone);
    outstring(str);
    outstring("\r");

	time(&tss);

	lts = -1;

	while (TRUE) {
		ts = (time(NULL) - tss);
		if(ts != lts) {
			cPrintf("\b\b%02ld",ts);
			lts = ts;
		}
		if ((int) (ts) > node->nddialto) {	/* Timeout */
			resp = -2;
            break;
		}

        if (KBReady()) {    /* Aborted by user */
            getch();
            getkey = 0;
			resp = -1;
            break;
        }
        if (local_mode || gotCarrier()) {   /* got carrier!  */
			cPrintf("\b\bsuccess");
            return TRUE;
        } else if (MIReady()) {
			ch = (char) getMod();

			if (debug) outCon(ch);

			if (ch != '\r') {               /* dump CR's */
				if (ch == '\n') {
					resp_buf[resp_index] = '\0';	/* terminate string */
					resp_index = 0;
					
					resp = verbose_response(resp_buf);

					delay(300);

					if(gotCarrier()) {
						cPrintf("\b\bsuccess");
						return TRUE;
					}

					if (debug) {
						cPrintf("\n(response was %d:%s)\n",resp,hayes_string[resp]);
						cPrintf("\ngotCarrier() %c\n",gotCarrier() ? 'Y' : 'N');
					}
					/* otherwise unknown */
					if ((resp == HAYES_NOCARRIER) ||
						(resp == HAYES_ERROR) ||
						(resp == HAYES_NODIAL) ||
						(resp == HAYES_BUSY) ||
						(resp == HAYES_NOANSWER))
						break;
					
				} else {		/* add character to buffer */
					if (resp_index < NAMESIZE) {
						resp_buf[resp_index] = ch;
						++resp_index;
					}
				}
			}
		}
    }

	if (resp == -1)
		cPrintf("\b\baborted ");
	else if (resp == -2)
		cPrintf("\b\btimeout ");
	else
		cPrintf("\b\bfailed: %s ",hayes_string[resp]);

    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  n_login()       Login to the bbs with the macro in the node file    */
/* -------------------------------------------------------------------- */
BOOL n_login(void)
{
    time_t ptime, now;
	char ch;
	uchar mtmp;
    int i, count, x;
    char *words[256];

    cPrintf("\n Logging in...");

	if(debug)
		cPrintf("%s",node->ndlogin);

	count = parse_it(words, node->ndlogin);

	i = 1;

    while (i <= count) {
		switch (*words[i++]) {
			case 'P':
			case 'p':
				cPrintf(".");
				if (debug)
					cPrintf(" Pause For (%s) ", words[i]);
                time(&ptime);
                now = ptime;
                ptime += atoi(words[i++]);
                while (now < ptime) {
                    time(&now);
                    if (MIReady()) {
                        ch = (char) getMod();
                        if (debug)
                            outCon(ch);
                    }
                }
                break;
			case 'S':
			case 's':
				cPrintf(".");
				if(node->ndpacing) {
					mtmp = modem;
					modem = TRUE;
					for(x=0;words[i][x];x++) {
						ch = words[i][x];
						if(debug) cPrintf("%c",ch);
						outMod(ch);
						delay(node->ndpacing);
					}
					modem = mtmp;
					i++;
				}
				else {
					outstring(words[i++]);
				}
                break;
			case 'T':
			case 't':
				node->ndpacing = atoi(words[i++]);
				break;
			case 'W':
			case 'w':
				cPrintf(".");
                if (debug)
					cPrintf("Wait For (%s) ", words[i]);
				if (!wait_for(words[i])) {
					cPrintf("failed waiting for %s",words[i]);
					i++;
					doccr();
                    return FALSE;
                }
                break;
            case '!':
				cPrintf(".");
                apsystem(words[i++]);
                break;
            default:
                break;
        }
    }
    cPrintf("success");
    doccr();
    doccr();
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  wait_for()      wait for a string to come from the modem            */
/* -------------------------------------------------------------------- */
BOOL wait_for(char *str)
{
    char line[80];
    long st;
    int stl;

    stl = strlen(str);

    memset(line, 0, stl);

    time(&st);

    while (TRUE) {
        if (MIReady()) {
            memcpy(line, line + 1, stl);
            line[stl - 1] = (char) getMod();
            line[stl] = '\0';
            if (debug)
                outCon(line[stl - 1]);
            if (strcmpi(line, str) == SAMESTRING)
                return TRUE;
        } else {
			if ((time(NULL) - st) > (long) node->ndwaitto)
                return FALSE;
            if (KBReady()) {    /* Aborted by user */
                getch();
                getkey = 0;
                return FALSE;
            }
        }
    }
}

/* -------------------------------------------------------------------- */
/*  net_callout()   Entry point from Cron.C                             */
/* -------------------------------------------------------------------- */
BOOL net_callout(char *node)
{
    int slot;
    int tmp;

    /* login user */

    mread = entered = 0;

    slot = personexists(node);

    if (slot == ERROR) {
        cPrintf("\n No such node in userlog!");
        local_mode = FALSE;
        return FALSE;
    }
    getLog(&logBuf, logTab[slot].ltlogSlot);

    thisSlot = slot;
    thisLog = logTab[slot].ltlogSlot;

    loggedIn = TRUE;
    setsysconfig();
    setgroupgen();
    setroomgen();
    setlbvisit();

    /* update25();	*/
	do_idle(0);

    sprintf(msgBuf->mbtext, "NetCallout %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_NETWORK);

    /* node logged in */

    tmp = net_master();

    /* terminate user */

    if (tmp == TRUE) {
		logBuf.callno = cfg->callno;
        time(&logtimestamp);
        logBuf.calltime = logtimestamp;
		logBuf.lbvisit[0] = cfg->newest;
		logTab[0].ltcallno = cfg->callno;

        slideLTab(thisSlot);
		cfg->callno++;

        storeLog();
        loggedIn = FALSE;

    /* trap it */
        sprintf(msgBuf->mbtext, "NetLogout %s (succeeded)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);

        outFlag = IMPERVIOUS;
        cPrintf("Networked with \"%s\"\n ", logBuf.lbname);

		if (cfg->accounting)
            unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        logBuf.lbname[0] = 0;

        setalloldrooms();
    } else {
        loggedIn = FALSE;

        sprintf(msgBuf->mbtext, "NetLogout %s (failed)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);
    }

    /* user terminated */
    onConsole = FALSE;
    callout = FALSE;

    delay(1000);

	Initport();

    local_mode = FALSE;
    return (BOOL) (tmp);
}

/* -------------------------------------------------------------------- */
/*  cleanup()       Done with other system, save mail and messages      */
/* -------------------------------------------------------------------- */
void cleanup(void)
{
    int t, i, rm, err;
	unsigned int new = 0, exp = 0, dup = 0, rms = 0;
    label fn, here, there;

	if(!parm.door)
		drop_dtr();
	else
		ExitToMsdos = 1;

	changedir(cfg->temppath);

    outFlag = IMPERVIOUS;

    doccr();
    cPrintf(" Incorporating:");
    doccr();
    cPrintf("                           Room:  New: Expired: Duplicate:");
    doccr();            /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX */
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();

	ndroom = start_ndroom;
	for (t = get_first_room(here, there), i = 0;
        t;
		t = get_next_room(here, there), i++) {
        sprintf(fn, "roomin.%d", i);

		strncpy(ndroom->here, here, sizeof(label) - 1);
		strncpy(ndroom->there, there, sizeof(label) - 1);
		if(ndroom->next == NULL) {
			ndroom->next = farcalloc(1UL, (ulong) sizeof(struct ndrooms));
			if(ndroom->next == NULL) {
				crashout("Can not allocate memory for node room");
			}
			ndroom = ndroom->next;
			ndroom->next = NULL;
			ndroom->here[0] = '\0';
		}
		else {
			/* Shouldn't Happen!! */
			ndroom = ndroom->next;
			cPrintf("\nNo allocation needed for room\n");
		}

		rm = roomExists(here);
        if (rm != ERROR) {
            cPrintf(" %*.*s  ", NAMESIZE, NAMESIZE, here);
            err = ReadMsgFl(rm, fn, here, there);
            if (err != ERROR) {
				cPrintf("  %3u    %3u    %3u", err, expired, duplicate);
                new += err;
                exp += expired;
                dup += duplicate;
                rms++;
            } else {
                amPrintf(" Can not see room '%s' on remote\n", there);
                netError = TRUE;
                cPrintf(" Room not found on other system.");
            }
            doccr();
        } else {
            cPrintf(" %*.*s   Room not found on local system.",
            NAMESIZE, NAMESIZE, here);
			netError = TRUE;
            amPrintf(" No '%s' room accessable local.\n", here);
            doccr();
        }

        unlink(fn);
    }
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();
	cPrintf("Totals:                      %2u    %3u    %3u    %3u", rms, new, exp, dup);
    /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX */
    doccr();
    doccr();

    cPrintf("Incorporating MAIL");
    i = ReadMsgFl(MAILROOM, "mesg.tmp", "", "");
    cPrintf(" %d new message(s)", i == ERROR ? 0 : i);
    doccr();

    ambigUnlink("room.[0-9]*", FALSE);
    ambigUnlink("roomin.*", FALSE);
    unlink("mesg.tmp");
	if (!strcmp(cfg->temppath, cfg->homepath))
        ambigUnlink("room.*", FALSE);

	changedir(cfg->homepath);
    unlink("mesg.tmp");

    xpd = exp;
    duplic = dup;
    entered = new + (i == ERROR ? 0 : i);   /* add mail too. */

    if (netError) {
        amPrintf(" \n While netting with '%s'\n", logBuf.lbname);
        SaveAideMess();
    }
	release_mem();
}

/* -------------------------------------------------------------------- */
/*	release_mem()	Release the Dynamically linked lists				*/
/* -------------------------------------------------------------------- */
void release_mem()
{
	struct ndgroups *save_ndgrp;
	struct ndrooms	*save_ndroom;

	save_ndgrp = start_ndgrp->next;
	while(save_ndgrp) {
		ndgrp = save_ndgrp;
        save_ndgrp = save_ndgrp->next;
		farfree(ndgrp);
	}
	start_ndgrp->next = NULL;

	save_ndroom = start_ndroom->next;
	while(save_ndroom) {
		ndroom = save_ndroom;
		save_ndroom = save_ndroom->next;
		farfree(ndroom);
    }
	start_ndroom->next = NULL;
}

/* -------------------------------------------------------------------- */
/*  netcanseeroom() Can the node see this room?                     (o) */
/* -------------------------------------------------------------------- */
BOOL netcanseeroom(int roomslot)
{
    /* is room in use              */
    if  (roomTab[roomslot].rtflags.INUSE

    /* and it is shared            */
        &&  roomTab[roomslot].rtflags.SHARED

    /* and group can see this room */
		&&	groupseesroom(roomslot)

	/* only aides go to aide room  */
    &&  (roomslot != AIDEROOM || aide)) {
        return TRUE;
    }
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  alias()         return the name of the BBS from the #ALIAS          */
/* -------------------------------------------------------------------- */
BOOL alias(char *str)
{
    return alias_route(str, "#ALIAS");
}

/* -------------------------------------------------------------------- */
/*  route()         return the routing of a BBS from the #ROUTE         */
/* -------------------------------------------------------------------- */
BOOL route(char *str)
{
    if  (alias_route(str, "#ROUTE"))
        return (TRUE);
    else
        return (alias_route(default_route, "#ROUTE"));
}

/* -------------------------------------------------------------------- */
/*  alias_route()   returns the route or alias specified                */
/* -------------------------------------------------------------------- */
BOOL alias_route(char *str, char *srch)
{
    FILE *fBuf;
    char line[90];
    char *words[256];
	char path[80];

	flipFileLan(cfg->homepath,1);
	sprintf(path, "%s\\route.cit", cfg->homepath);
	flipFileLan(cfg->homepath,0);

    if ((fBuf = fopen(path, "r")) == NULL) {
        crashout("Can't find route.cit!");
    }
    while (fgets(line, 90, fBuf) != NULL) {
        if (line[0] != '#')
            continue;
		/* silly way to do this, time to rewrite another module - FJM */
        else if (!strnicmp(line, "#DEFAULT_ROUTE", 14)) {
            parse_it(words, line);
            strcpy(default_route, words[1]);
            continue;
        } else if (strnicmp(line, srch, 5) != SAMESTRING)
            continue;

        parse_it(words, line);

        if (strcmpi(srch, words[0]) == SAMESTRING) {
            if (strcmpi(str, words[1]) == SAMESTRING) {
                fclose(fBuf);
                strcpy(str, words[2]);
                return TRUE;
            }
        }
    }
    fclose(fBuf);
    return FALSE;
}

/* ------------------------------------------------------------------------ */
/*  the following two routines are used for scanning through the rooms      */
/*  requested from a node                                                   */
/* ------------------------------------------------------------------------ */
FILE *nodefile;

/* -------------------------------------------------------------------- */
/*  get_first_room()    get the first room in the room list             */
/* -------------------------------------------------------------------- */
BOOL get_first_room(char *here, char *there)
{
	if	(!node->roomoff)
        return FALSE;

    /* move to home-path */
	changedir(cfg->homepath);

    if ((nodefile = fopen("nodes.cit", "r")) == NULL) {
        crashout("Can't find nodes.cit!");
    }
	changedir(cfg->temppath);

	fseek(nodefile, node->roomoff, SEEK_SET);

    return get_next_room(here, there);
}

/* -------------------------------------------------------------------- */
/*  get_next_room() gets the next room in the list                      */
/* -------------------------------------------------------------------- */
BOOL get_next_room(char *here, char *there)
{
	int count;
	char line[90];
    char *words[256];

    while (fgets(line, 90, nodefile) != NULL) {
        if (line[0] != '#')
            continue;

		count = parse_it(words, line);

        if (strcmpi(words[0], "#NODE") == SAMESTRING) {
            fclose(nodefile);
            return FALSE;
        }
        if (strcmpi(words[0], "#ROOM") == SAMESTRING) {
            strcpy(here, words[1]);
			if(count == 3) {
				strcpy(there, words[2]);
			}
			else {
				strcpy(there, words[1]);
			}
			return TRUE;
        }
    }
    fclose(nodefile);
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */
BOOL save_mail()
{
    label tosystem;
    char filename[100];
    char our_path[NAMESIZE + 1];
    FILE *fl;

    /* where are we sending it? */
    strncpy(tosystem, msgBuf->mbzip, sizeof(label) - 1);

    /* send it via... */
    route(tosystem);

	/* get the node entry */
    if (!getnode(tosystem)) {
        if (!*default_route || !getnode(default_route)) {
            amPrintf("\n Default mail route not found\n");
            netError = TRUE;
            return FALSE;
        } else {
            amPrintf("\n Can't find mail route to %s\n", tosystem);
            amPrintf("\n Defaulting mail route to %s\n", default_route);
            netError = TRUE;
            strcpy(tosystem, default_route);
        }
    }
    /* check to see if this mail has been routed through here before */
	strcpy(our_path, cfg->nodeTitle);
    strcat(our_path, "!");

	/* test for our node name (e.g. "Centauri's Place!") */
    if (strstr(msgBuf->mbfpath, our_path)) {
        amPrintf("\n Circular route found in mail path.\n");
        netError = TRUE;
        return FALSE;
    }
	sprintf(filename, "%s\\%s", cfg->homepath, node->ndmailtmp);

    fl = fopen(filename, "ab");
    if (!fl)
        return FALSE;

    fPutMessage(fl);

	fclose(fl);

    return TRUE;
}

void save_special(char *savename)
{
	char filename[100];
    FILE *fl;

	flipFileLan(cfg->mailpath,1);
	sprintf(filename, "%s\\%s", cfg->mailpath, savename);
	flipFileLan(cfg->mailpath,0);

	fl = fopen(filename, "a+b");
    if (!fl)
		return;

    fPutMessage(fl);

    fclose(fl);
}

static int verbose_response(char *buf)
{
	int i;
	
	i = 0;
	if (debug)
		cPrintf("\n(decoding response '%s')\n",buf);
	while (stricmp(buf,hayes_string[i])) {
		++i;
		if (i >= HAYES_NUMCMDS) {
			i = HAYES_UNKNOWN;
			break;
		}
	}
	return i;
}
/* EOF */
