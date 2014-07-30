/************************************************************************/
/*                              rooma.c                                 */
/*              room code for Citadel bulletin board system             */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      canseeroom()            returns TRUE if user can see a room     */
/*      dumpRoom()              tells us # new messages etc             */
/*      gotoRoom()              handles "g(oto)" command for menu       */
/*      indexslot()             returns index slot# for a message#      */
/*      listRooms()             lists known rooms                       */
/*      partialExist()          returns slot# of partially named room   */
/*      printroom()             displays name of specified room         */
/*      roomdescription()       prints out room description             */
/*      roomExists()            returns slot# of named room else ERROR  */
/*      roomtalley()            talleys up total,messages & new         */
/*     substr()                        is string 1 in string 2?                                */
/*                                                                      */
/************************************************************************/

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *	04/14/90	FJM 	Fixed references to room names to use label type
 *	06/16/90	FJM 	Made IBM Graphics characters a seperate option.
 *	07/26/90	FJM 	Made gotoRoom() start at current room & wrap.
 *	08/28/90	FJM 	More changes to gotoRoom()
 *	09/06/90	FJM 	Added room descript to roomdescription().
 *	01/18/90	FJM 	Improved Goto
 *	09/25/91	BLJ 	Added use of strip_ansi()
 *	04/04/92	BLJ 	Added termCap(BOLD) to all room display
 *
 * -------------------------------------------------------------------- */

/* Static function definitions	   */

static int partialExist(char *roomname);
static void roomdescription(void);
static int partialExist2(char *roomname);
static void printrm(int room);
static void printroomVer(int room, int verbose, char numMess);
void fmtrm(int room, char *string);
int canaccessroom(int roomslot);
int canseeroom(int roomslot);
int substr(char *str1, char *str2);
void dumpRoom(void);

/************************************************************************/
/*      canseeroom() returns TRUE if user has access to room            */
/************************************************************************/
int canseeroom(int roomslot)
{
	if(roomslot == ERROR) return(FALSE);

	/* is room in use			   */
    if  (roomTab[roomslot].rtflags.INUSE

    /* and room's in this hall     */
        &&  roominhall(roomslot)
    /* and group can see this room */
		&&	groupseesroom(roomslot)
    /* only aides go to aide room  */
        &&  (roomslot != AIDEROOM || aide))
        return (TRUE);

    return (FALSE);

}

/************************************************************************/
/*		canaccessroom() returns TRUE if user has access to room 		*/
/************************************************************************/
int canaccessroom(int roomslot)
{
	if(roomslot == ERROR) return(FALSE);

	/* is room in use			   */
    if  (roomTab[roomslot].rtflags.INUSE

    /* and group can see this room */
		&&	groupseesroom(roomslot)
    /* only aides go to aide room  */
        &&  (roomslot != AIDEROOM || aide))
        return (TRUE);

    return (FALSE);

}

/************************************************************************/
/*      dumpRoom() tells us # new messages etc                          */
/************************************************************************/
void dumpRoom(void)
{
    int total, messages, new;

    total = talleyBuf.room[thisRoom].total;
    messages = talleyBuf.room[thisRoom].messages;
    new = talleyBuf.room[thisRoom].new;

	if (cfg->roomtell)
        roomdescription();

    if (loggedIn && aide)
        mPrintf("%d total ", total);

    if (!aide) {
        mPrintf("%d %s ", messages, (messages == 1) ? "message" : "messages");
    } else {
        doCR();
        mPrintf(" %d %s ", messages, (messages == 1) ? "message" : "messages");
    }

    if (loggedIn) {
        if (new) {
            doCR();
            mPrintf(" %d new ", new);
        }
        if (logBuf.lbroom[thisRoom].mail) {
            doCR();
            mPrintf(" You have mail here. ");
        }
    }
}

/************************************************************************/
/*      gotoRoom() is the menu fn to travel to a new room               */
/*      returns TRUE if room is Lobby>, else FALSE                      */
/************************************************************************/

#define nextRoom(r) ((((r)+1) < MAXROOMS)?((r)+1):0)

int gotoRoom(char *roomname)
{
    int i, foundit, roomNo = ERROR;
    int check, foundflag = 0;
	static int jcount = 0;

	if (logBuf.DUNGEONED) {
	   nextblurb("dungeon",&jcount,1);
	   return FALSE;
	}

	oldroom = thisRoom;

    logBuf.lbroom[thisRoom].lbgen = roomBuf.rbgen;
    if (!skiproom) {
        ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
        ug_new = talleyBuf.room[thisRoom].new;

        logBuf.lbroom[thisRoom].lvisit = 0;
        logBuf.lbroom[thisRoom].mail = 0;
    /* zero new count in talleybuffer */
        talleyBuf.room[thisRoom].new = 0;
    }
    skiproom = FALSE;

    if (!strlen(roomname)) {
        foundit = FALSE;    /* leaves us in Lobby> if nothing found */

		/* start goto loop at current room +1, wrap if needed */
		/* search for new rooms first */
        for (i = nextRoom(thisRoom); (i != thisRoom) && !foundit;
			i = nextRoom(i)) {
			/* can user access this room?         */
            if (canseeroom(i)
			/* does it have new messages,         */
                && talleyBuf.room[i].new
				/* is it NOT excluded                 */
                && (!logBuf.lbroom[i].xclude || logBuf.lbroom[i].mail)
				/* we dont come back to current room  */
                && i != thisRoom
				/* and is it K>nown                   */
				&& (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)) {
                foundit = i;
				break;
            }
        }
		/* search for windowed rooms */
		if (!foundit && cfg->subhubs && thisHall != 1) {
			for (i = nextRoom(thisRoom);
				(i != thisRoom) && !foundit;i = nextRoom(i)) {
				/* can user access this room?         */
				if (canseeroom(i) && iswindow(i)
					/* is it NOT excluded                 */
					&& (!logBuf.lbroom[i].xclude || logBuf.lbroom[i].mail)
					/* we dont come back to current room  */
					&& i != thisRoom
					/* and is it K>nown                   */
					&& (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)) {
					foundit = i;
					break;
				}
			}
		}
/*
 * special subhub section - if off: stonehenge compatable,
 * and lobby is only window room, on every hall
 */

		if (!foundit && cfg->subhubs) {
            for (i = nextRoom(thisRoom); i != thisRoom && !foundflag; i = nextRoom(i)) {
                if (iswindow(i)
					/* can user access this room?     */
                    && canseeroom(i)
					/* is it NOT excluded             */
                    && !logBuf.lbroom[i].xclude
					/* and is it K>nown               */
					&& (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)) {
                    foundit = i;
                    foundflag = TRUE;
                }
            }
            if (!foundflag && iswindow(thisRoom)) {
                foundit = thisRoom;
            } else if (!foundflag && !roominhall(LOBBY)) {
                for (i = nextRoom(thisRoom); i != thisRoom && !foundit;
					i = nextRoom(i)) {
            /* can user access this room? */
                    if (canseeroom(i)
            /* is it NOT excluded         */
                        && !logBuf.lbroom[i].xclude

            /* and is it K>nown           */
                    && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)) {
                        foundit = i;
                    }
                }
                if (!foundflag) {
                    foundit = thisRoom;
                }
            }
        }
/* ^ special subhub section ^ */

        getRoom(foundit);

        mPrintf("%s ", roomBuf.rbname);

        if (iswindow(foundit) && logBuf.NEXTHALL) {
            mPrintf("AutoSkip to ");
            stephall(1);
            doCR();
        }
        doCR();
        mPrintf(" ");
    } else {
        foundit = FALSE;

        check = roomExists(roomname);

        if (!canseeroom(check))
            check = ERROR;

        if (check == ERROR)
            check = partialExist(roomname);

        if (check != ERROR)
            roomNo = check;

        if (roomNo != ERROR && canseeroom(roomNo)) {

            foundit = roomNo;
            getRoom(roomNo);

        /* if may have been unknown... if so, note it:      */
            if ((logBuf.lbroom[thisRoom].lbgen) != roomBuf.rbgen) {
                logBuf.lbroom[thisRoom].lbgen = roomBuf.rbgen;
                logBuf.lbroom[thisRoom].lvisit = (MAXVISIT - 1);
            }
		}
		else {
			if(roomNo == ERROR) {
				check = partialExist2(roomname);
				if (check != ERROR) roomNo = check;

			}
			if(canaccessroom(roomNo))
				mPrintf(" No '%s' room on this hall\n ", roomname);
            else
				mPrintf(" No '%s' room\n ", roomname);
        }
    }

    dumpRoom();

    return foundit;
}

/**********************************************************************
 *     listRooms() lists known rooms
 **********************************************************************/

void listRooms(unsigned int what, char verbose, char numMess)
{
    int i, j;
    char firstime;
	static int jcount = 0;

	if (logBuf.DUNGEONED) {
	   nextblurb("dungeon",&jcount,1);
	   return;
	}

	outFlag = OUTOK;

    showdir = 0;
    showhidden = 0;
    showgroup = 0;

    /* criteria for NEW rooms */

    if (what == NEWRMS || what == OLDNEW) {
        mPrintf("\n Rooms with unread ");

        if (thisHall) {
            mPrintf("messages along %s:", hallBuf->hall[thisHall].hallname);
        } else
            mPrintf("messages:");
        doCR();

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
                && talleyBuf.room[i].new
            && !logBuf.lbroom[i].xclude) {
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    /* for dir rooms */

    if (what == DIRRMS || what == APLRMS || what == LIMRMS || what == SHRDRM) {
        firstime = TRUE;

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
                && ((roomTab[i].rtflags.MSDOSDIR && what == DIRRMS)
                || (roomTab[i].rtflags.APLIC && what == APLRMS)
                || (roomTab[i].rtflags.SHARED && what == SHRDRM)
            || (roomTab[i].rtflags.GROUPONLY && what == LIMRMS))) {
                if (firstime) {
                    mPrintf("\n %s room:",
                    what == DIRRMS ? "Directory" :
                    what == LIMRMS ? "Limited Access" :
                    what == SHRDRM ? "Shared" : "Application");
                    doCR();

                    firstime = FALSE;
                }
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    /* criteria for MAIL rooms */

    if (what == NEWRMS || what == OLDNEW || what == MAILRM) {
        firstime = TRUE;

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            && logBuf.lbroom[i].mail) {
                if (firstime) {
                    mPrintf("\n You have private mail in:");
                    doCR();

                    firstime = FALSE;
                }
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    /* criteria for OLD rooms */

    if (what == OLDNEW || what == OLDRMS) {
        mPrintf("\n No unseen msgs in:");
        doCR();

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            && !talleyBuf.room[i].new) {
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    /* criteria for EXCLUDED rooms */

    if (what == OLDNEW || what == XCLRMS) {
        firstime = TRUE;

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
                && logBuf.lbroom[i].xclude
            && (talleyBuf.room[i].new || what == XCLRMS)) {
                if (firstime) {
                    mPrintf("\n Excluded rooms:");
                    doCR();

                    firstime = FALSE;
                }
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    /* criteria for WINDOWS */

    if (what == WINDWS) {
        mPrintf("\n Rooms exiting to other halls:\n ");
        if (!verbose)
            mPrintf(" ");

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
            && iswindow(i)) {
                if (verbose)
                    mPrintf(" ");

                printroomVer(i, FALSE, numMess);

                if (verbose) {
                    for (j = 0; j < MAXHALLS; j++) {
                        if (hallBuf->hall[j].hroomflags[i].window
                            && hallBuf->hall[j].h_inuse
                        && groupseeshall(j)) {
                            mPrintf("%s: ", hallBuf->hall[j].hallname);
                        }
                    }
                    doCR();
                    mPrintf(" ");
                }
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

	/* criteria for ANONYMOUS rooms */

	if (what == ANONRM) {
	   mPrintf("\n anonYmous rooms:");
	   doCR();

        for (i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++) {
            if (canseeroom(i)
                && (roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
				&& roomTab[i].rtflags.ANONYMOUS)
			{
                printroomVer(i, verbose, numMess);
            }
        }
    }
    if (outFlag == OUTSKIP)
        return;

    if (!expert) {
        if (showhidden)
            mPrintf("\n %c => hidden room", ')');
        if (showgroup)
			mPrintf("\n %c => group only room", cfg->ibmroom ? '\xb3' : ':');
        if (showdir && what != DIRRMS)
			mPrintf("\n %c => directory room", cfg->ibmroom ? '\xb9' : ']');
    }
}

/* ------------------------------------------------------------------------ */
/*  RoomStatus() Shows the status of a room...                              */
/* ------------------------------------------------------------------------ */
void RoomStatus(void)
{
    char buff[256];
    int j;

    doCR();
    formatSummary(buff);
    mPrintf(buff);
    doCR();
    doCR();

    mPrintf("Windowed in Halls ");

    for (j = 0; j < MAXHALLS; j++) {
        if (hallBuf->hall[j].hroomflags[thisRoom].window
            && hallBuf->hall[j].h_inuse
        && groupseeshall(j)) {
            mPrintf("%s: ", hallBuf->hall[j].hallname);
        }
    }
    readhalls();
    doCR();
}

/* --------------------------------------------------------------------
 *  substr()        is string str2 in string str1?
 * -------------------------------------------------------------------- */
int substr(char *str1, char *str2)
{
    char *s1, *s2;
    int rc;

    /* get working copies */
    s1 = strdup(str1);
    s2 = strdup(str2);
    /* check for out-of-memory     */
    if (!s1 || !s2)
        crashout("low mem");
    /* make the strings the same case (lower) */
    strlwr(s1);
    strlwr(s2);
    /* check for match */
    rc = strstr(s1, s2) != NULL;
    /* free workspace */
    free(s2);
    free(s1);
    /* 1 for match, 0 for no match */
    return rc;
}

/************************************************************************/
/*      partialExist() the list looking for a partial match             */
/************************************************************************/
partialExist(roomname)
char *roomname;
{
    label compare;
    int i, j, length;

    length = strlen(roomname);

    j = thisRoom + 1;

    for (i = 0; i < MAXROOMS; ++i, ++j) {
        if (j == MAXROOMS)
            j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen)) {
        /* copy roomname into scratch buffer */
            strcpy(compare, roomTab[j].rtname);

        /* make both strings the same length */
            compare[length] = '\0';

            if ((strcmpi(compare, roomname) == SAMESTRING) && roominhall(j)
                && canseeroom(j))
                return (j);
        }
    }

    for (i = 0, j = thisRoom + 1; i < MAXROOMS; ++i, ++j) {
        if (j == MAXROOMS)
            j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen)) {
            if (substr(roomTab[j].rtname, roomname)
                && roominhall(j)
                && canseeroom(j))
                return (j);
        }
    }
    return (ERROR);
}

/************************************************************************/
/*		partialExist2() the list looking for a partial match			*/
/************************************************************************/
int partialExist2(char *roomname)
{
    label compare;
    int i, j, length;

    length = strlen(roomname);

    j = thisRoom + 1;

    for (i = 0; i < MAXROOMS; ++i, ++j) {
        if (j == MAXROOMS)
            j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen)) {
        /* copy roomname into scratch buffer */
            strcpy(compare, roomTab[j].rtname);

        /* make both strings the same length */
            compare[length] = '\0';

			if ((strcmpi(compare, roomname) == SAMESTRING)
				&& canaccessroom(j))
                return (j);
        }
    }

    for (i = 0, j = thisRoom + 1; i < MAXROOMS; ++i, ++j) {
        if (j == MAXROOMS)
            j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen)) {
            if (substr(roomTab[j].rtname, roomname)
				&& canaccessroom(j))
                return (j);
        }
    }
    return (ERROR);
}


/***********************************************************************
 *     fmtroom()  formats display name of specified room into string.
 *                                      sets global flags rtflags
 ***********************************************************************/
void fmtrm(int room, char *string)
{
    char *p;
    char symb[] = "]:)>>";
    char ibmsymb[] = {'\xb9', '\xb3', ')', ' ', '\xaf'};
    char *sym;

	if (IBMOn && cfg->ibmroom)
        sym = ibmsymb;
    else
        sym = symb;

    p = strcpy(string, roomTab[room].rtname);
    p += strlen(roomTab[room].rtname);

    if (roomTab[room].rtflags.MSDOSDIR)
        *p++ = sym[0];
    if (roomTab[room].rtflags.GROUPONLY)
        *p++ = sym[1];
    if (!roomTab[room].rtflags.PUBLIC)
        *p++ = sym[2];
    else if (!roomTab[room].rtflags.GROUPONLY &&
        !roomTab[room].rtflags.MSDOSDIR)
        *p++ = sym[3];
    if (iswindow(room))
        *p++ = sym[4];
    *p = '\0';

    if (roomTab[room].rtflags.GROUPONLY)
        showgroup = TRUE;
    if (roomTab[room].rtflags.MSDOSDIR)
        showdir = TRUE;
    if (!roomTab[room].rtflags.PUBLIC)
        showhidden = TRUE;
    return;
}

/***********************************************************************/
/*     printroom()  displays name of specified room.                   */
/*     Called by listRooms  Sets global flags.                         */
/***********************************************************************/
void printrm(int room)
{
    char string[NAMESIZE + 6];

    fmtrm(room, string);

	termCap(TERM_BOLD); 
	mPrintf("%s", string);
	termCap(TERM_NORMAL);
}

/***********************************************************************
 *     printroomver()  displays verbose name of specified room.
 *     Called by listRooms  Sets global flags.
 ***********************************************************************/
void printroomVer(int room, int verbose, char numMess)
{
    char string[NAMESIZE + 6];
	uchar oldRoom;

    strcpy(string, roomTab[room].rtname);
    fmtrm(room, string);
    if (crtColumn + strlen(string) >= termWidth)
        doCR();
    if (!verbose && !numMess) {
		if(cfg->wideroom) {
			if(crtColumn > 3 && crtColumn < 40) {
				while(crtColumn < 40) {
					oChar(' ');
				}
			}
			mtPrintf(TERM_BOLD, "%-38s", string);
        }
        else
			mtPrintf(TERM_BOLD, "%s", string);

		mPrintf(" ");
    } else {
        oldRoom = thisRoom;
        getRoom(room);
		if(cfg->wideroom) {
			if(crtColumn > 3 && crtColumn < 40) {
				while(crtColumn < 40) {
					oChar(' ');
				}
			}
			mtPrintf(TERM_BOLD, "%-38s", string);
		}
		else
			mtPrintf(TERM_BOLD, "%s", string);

		/*
		mPrintf("%-*s", NAMESIZE + 1 - strlen(string), " ");
		*/

		if (numMess) {
            if (aide) {
                mPrintf("%3d total, ", talleyBuf.room[room].total);
            }
            mPrintf("%3d messages, %3d new", talleyBuf.room[room].messages,
            talleyBuf.room[room].new);

            if (talleyBuf.room[room].new && logBuf.lbroom[room].mail) {
                mPrintf(", (Mail)");
            }
            doCR();

            if (*roomBuf.descript && verbose) {
                mPrintf("    ");
            }
        }
        if (verbose) {
            if (*roomBuf.descript)
                mPrintf("%s", roomBuf.descript);
            if (!numMess || *roomBuf.descript)
                doCR();
        }
        if (verbose && numMess)
            doCR();

        getRoom(oldRoom);
    }
}


/************************************************************************/
/*      roomdescription()  prints out room description                  */
/************************************************************************/
void roomdescription(void)
{
    outFlag = OUTOK;

    if (!roomtell)
        return;

    /* do room description string every visit */
    if (*roomBuf.descript) {
        mPrintf("%s", roomBuf.descript);
        doCR();
        mPrintf(" ");
    }
    /* only do room description file upon first visit this call to a room    */
    if (!logBuf.lbroom[thisRoom].lvisit)
        return;

    if (!roomBuf.rbroomtell[0])
        return;

	if (changedir(cfg->roompath) == -1)
        return;

    /* no bad files */
    if (checkfilename(roomBuf.rbroomtell, 0) == ERROR) {
		changedir(cfg->homepath);
        return;
    }
    if (!filexists(roomBuf.rbroomtell)) {
        mPrintf("No room description %s\n \n ", roomBuf.rbroomtell);
		changedir(cfg->homepath);
        return;
    }
    if (!expert)
        mPrintf("\n <J>ump <N>ext <P>ause <S>top\n");

    /* print it out */
	termCap(TERM_BOLD);
	dumpf(roomBuf.rbroomtell);
	termCap(TERM_NORMAL);

    /* go to our home-path */
	changedir(cfg->homepath);

    outFlag = OUTOK;

    mPrintf("\n ");
}

/************************************************************************/
/*      roomExists() returns slot# of named room else ERROR             */
/************************************************************************/
int roomExists(char *room)
{
    int i;
	char sroom[NAMESIZE+1];

	strcpy(sroom,strip_ansi(room));

	for (i = 0; i < MAXROOMS; i++) {
        if (roomTab[i].rtflags.INUSE == 1 &&
		strcmpi(sroom, strip_ansi(roomTab[i].rtname))
			== SAMESTRING) {
            return (i);
        }
    }
    return (ERROR);
}

/************************************************************************/
/*   roomtalley()  talleys up total,messages & new for every room       */
/************************************************************************/
void roomtalley(void)
{
    register int i, num;
    register int room;

    for (room = 0; room < MAXROOMS; room++) {
        talleyBuf.room[room].total = 0;
        talleyBuf.room[room].messages = 0;
        talleyBuf.room[room].new = 0;
    }

    num = sizetable();

    for (i = 0; i < num; ++i) {
        room = msgTab[i].mtroomno;

        if (msgTab[i].mtoffset <= i)
            talleyBuf.room[room].total++;

        if (mayseeindexmsg(i)) {
            talleyBuf.room[room].messages++;

			if ((ulong) (cfg->mtoldest + i) >
                logBuf.lbvisit[logBuf.lbroom[room].lvisit])
                talleyBuf.room[room].new++;
        }
    }
}

/************************************************************************/
/*      givePrompt() prints the usual "CURRENTROOM>" prompt.            */
/************************************************************************/
void givePrompt(void)
{
    while (MIReady())
        getMod();

    outFlag = IMPERVIOUS;
    echo = BOTH;
    onConsole = (char) (whichIO == CONSOLE);

	ansiattr = cfg->attr;

    doCR();

	/* termCap(TERM_REVERSE); */
	ansiattr = cfg->wattr;
    printrm(thisRoom);
	/* termCap(TERM_NORMAL); */
	ansiattr = cfg->attr;
    mPrintf(" ");

    if (strcmp(roomBuf.rbname, roomTab[thisRoom].rtname) != SAMESTRING) {
		if(debug) {
			cPrintf("%s - %s - %d \n",roomBuf.rbname,
				roomTab[thisRoom].rtname, thisRoom);
				delay(1000);
		}

		crashout("Dependent variables mismatch!");
    }
    outFlag = OUTOK;
}

/************************************************************************/
/*      indexRooms() -- build RAM index to ROOM.CIT, by CITADEL, to     */
/*      delete empty rooms.                                             */
/************************************************************************/
void indexRooms(void)
{
    int goodRoom, slot, i;

    for (slot = 0; slot < MAXROOMS; slot++) {
        if (roomTab[slot].rtflags.INUSE) {
            goodRoom = FALSE;

            if (roomTab[slot].rtflags.PERMROOM) {
                goodRoom = TRUE;
            } else {
                for (i = 0; i < sizetable(); ++i) {
                    if (msgTab[i].mtroomno == (uchar) slot)
                        goodRoom = TRUE;
                }
            }

            if (!goodRoom) {
                getRoom(slot);
                roomBuf.rbflags.INUSE = FALSE;
                roomBuf.rbflags.PUBLIC = FALSE;
                roomBuf.rbflags.MSDOSDIR = FALSE;
                roomBuf.rbflags.PERMROOM = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbflags.READONLY = FALSE;
                roomBuf.rbflags.SHARED = FALSE;
                roomBuf.rbflags.MODERATED = FALSE;
                roomBuf.rbflags.DOWNONLY = FALSE;
                roomBuf.rbflags.UPONLY = FALSE;
                putRoom(slot);
                strcat(msgBuf->mbtext, roomBuf.rbname);
                strcat(msgBuf->mbtext, "> ");
                noteRoom();
            }
        }
    }
}

/************************************************************************/
/*      noteRoom() -- enter room into RAM index array.                  */
/************************************************************************/
void noteRoom(void)
{
    strcpy(roomTab[thisRoom].rtname, roomBuf.rbname);
    roomTab[thisRoom].rtgen = roomBuf.rbgen;
    roomTab[thisRoom].grpno = roomBuf.rbgrpno;
    roomTab[thisRoom].grpgen = roomBuf.rbgrpgen;
    roomTab[thisRoom].rtflags = roomBuf.rbflags;
}

/************************************************************************/
/*     stepRoom()  1 for next 0, 0 for previous room                    */
/************************************************************************/
void stepRoom(int direction)
{
    int i;
    char done = 0;
	static int jcount = 0;

	if (logBuf.DUNGEONED) {
	   nextblurb("dungeon",&jcount,1);
	   return;
	}

	i = thisRoom;
    oldroom = thisRoom;

    if (loggedIn) {
        ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
        ug_new = talleyBuf.room[thisRoom].new;

        logBuf.lbroom[thisRoom].lbgen = roomBuf.rbgen;
        logBuf.lbroom[thisRoom].lvisit = 0;
        logBuf.lbroom[thisRoom].mail = 0;

    /* zero new count in talleybuffer */
        talleyBuf.room[thisRoom].new = 0;
    }
    do {
        if (direction == 1)
            ++i;
        else
            --i;

        if ((direction == 1) && (i == MAXROOMS))
            i = 0;
        else if ((direction == 0) && (i == -1))
            i = MAXROOMS - 1;

        if ((canseeroom(i) || i == thisRoom)
    /* and is it K>nown                             */
    /* sysops can plus their way into hidden rooms! */
            && ((roomTab[i].rtgen == logBuf.lbroom[i].lbgen)
        || logBuf.lbflags.SYSOP)) {
            mPrintf("%s\n ", roomTab[i].rtname);

            getRoom(i);


            dumpRoom();

            done = 1;
        }
    } while (done != 1);
}

/************************************************************************/
/*     unGotoRoom()                                                     */
/************************************************************************/
void unGotoRoom(void)
{
    int i;
	static int jcount = 0;

	if (logBuf.DUNGEONED) {
	   nextblurb("dungeon",&jcount,1);
	   return;
	}

    i = oldroom;

    if (canseeroom(i) && i != thisRoom) {
        mPrintf("%s\n ", roomTab[i].rtname);

        getRoom(i);

        logBuf.lbroom[thisRoom].lvisit = ug_lvisit;
        talleyBuf.room[thisRoom].new = ug_new;

        dumpRoom();
    }
}
