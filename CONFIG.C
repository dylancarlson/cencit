/************************************************************************/
/*                              config.c                                */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#include <alloc.h>
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "keywords.h"
#include "global.h"
#include "config.atm"

/************************************************************************/
/*                              Contents                                */
/*      buildcopies()           copies appropriate msg index members    */
/*      buildhalls()            builds hall-table (all rooms in Maint.) */
/*      buildroom()             builds a new room according to msg-buf  */
/*      clearaccount()          sets all group accounting data to zero  */
/*      configcit()             the main configuration for citadel      */
/*      illegal()               abort config.exe program                */
/*      initfiles()             opens & initalizes any missing files    */
/*      logInit()               indexes log.dat                         */
/*      logSort()               Sorts 2 entries in logTab               */
/*      msgInit()               builds message table from msg.dat       */
/*      readaccount()           reads grpdata.cit values into grp struct*/
/*      readprotocols()         reads external.cit values into ext struc*/
/*      readconfig()            reads config.cit values                 */
/*      RoomTabBld()            builds room.tab, index's room.dat       */
/*      slidemsgTab()           frees slots at beginning of msg table   */
/*      zapGrpFile()            initializes grp.dat                     */
/*      zapHallFile()           initializes hall.dat                    */
/*      zapLogFile()            initializes log.dat                     */
/*      zapMsgFile()            initializes msg.dat                     */
/*      zapRoomFile()           initializes room.dat                    */
/************************************************************************/

/* --------------------------------------------------------------------
 *
 *  HISTORY:
 *
 *  03/08/90    {zm}    Fix some spelling errors.
 *  03/10/90    {zm}    Allow for possible COM3 or COM4.
 *  03/11/90    FJM     Allow COM ports 1-4
 *  03/12/90    {zm}    Add #DEFAULT_GROUP to external.cit
 *  03/19/90    FJM     Linted & partial cleanup
 *  06/06/90    FJM     Modified logSort for TC++
 *  07/29/90    FJM     Added ADCHANCE.  Percentage chance of an ad
 *                      appearing.  0 to disable.
 *                      Added ADTIME.  Idle time in seconds before an ad
 *                      appears.  0 to disable.
 *  09/21/90    FJM     Added #SYSOP, #CREDIT_NAME
 *  10/15/90    FJM     Added console only protocol flag (field #1)
 *  10/18/90    FJM     Changed initfiles() to use changedir(), not chdir
 *  11/23/90    FJM     Modified for new "ATOMS" method of NLS support.
 *  11/23/90    FJM     Update to ATOMS method
 *  01/13/90    FJM     Name overflow fixes.
 *  01/18/90    FJM     Added #SWAPFILE
 *  02/05/91    FJM     Added #SCREEN_SAVE
 *	06/05/91	BLJ 	Changed config display of log, msg, room
 *	06/07/91	BLJ 	Added code for re-reading config.cit on-line
 *	06/11/91	BLJ 	Added #IBMROOM
 *	06/12/91	BLJ 	Added #LAN
 *	06/15/91	BLJ 	Added #WIDEROOM
 *	06/29/91	BLJ 	Improve the Message graph (count by file size)
 *	07/03/91	BLJ 	Added #MAXSOUND
 *	08/21/91	BLJ 	Added readchatbell()
 *	09/14/91	BLJ 	Changes for Dynamically Linked Lists
 *	03/28/92	BLJ 	Added Drive differences for LAN Phase I
 *	04/04/92	BLJ 	Fix of an OLD bug in Zapmsgfile
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *                local functions
 ************************************************************************/

/* static void checkDir(char *path); */
void msgInit(void);
void RoomTabBld(void);
void buildcopies(void);
void buildhalls(void);
void initfiles(void);
void cfgchatbell(int i, int count, char *words[]);
int logSort(const void *s1, const void *s2);
void logInit(void);
int zapMsgFile(void);
int zapLogFile(void);
int zapRoomFile(void);
void zapGrpFile(void);
void zapHallFile(void);
void zapHallgrpFile(void);
void zapRoomgrpFile(void);
void slidemsgTab(int howmany);
void clearaccount(void);
/************************************************************************
 *                External variables
 ************************************************************************/

/************************************************************************/
/*      buildcopies()  copies appropriate msg index members             */
/************************************************************************/
void buildcopies(void)
{
    int i;

    for (i = 0; i < sizetable(); ++i) {
        if (msgTab[i].mtmsgflags.COPY) {
            if (msgTab[i].mtoffset <= i) {
                copyindex(i, (i - msgTab[i].mtoffset));
            }
        }
    }
}

/************************************************************************/
/*      buildhalls()  builds hall-table (all rooms in Maint.)           */
/************************************************************************/
void buildhalls(void)
{
	uchar i;

    doccr();
    cPrintf(ATOM1001);
    doccr();

    for (i = 4; i < MAXROOMS; ++i) {
        if (roomTab[i].rtflags.INUSE) {
            hallBuf->hall[1].hroomflags[i].inhall = 1;  /* In Maintenance */
            hallBuf->hall[1].hroomflags[i].window = 0;  /* Not a Window   */
        }
    }
    putHall();
}

/************************************************************************/
/*      buildroom()  builds a new room according to msg-buf             */
/************************************************************************/
void buildroom(void)
{
    int roomslot;

    if (*msgBuf->mbcopy)
        return;
    roomslot = msgBuf->mbroomno;

    if (msgBuf->mbroomno < MAXROOMS) {
        getRoom(roomslot);

        if ((strcmp(roomBuf.rbname, msgBuf->mbroom) != SAMESTRING)
        || (!roomBuf.rbflags.INUSE)) {
            if (msgBuf->mbroomno > 3) {
                roomBuf.rbflags.INUSE = TRUE;
                roomBuf.rbflags.PERMROOM = FALSE;
                roomBuf.rbflags.MSDOSDIR = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbroomtell[0] = '\0';
                roomBuf.rbflags.PUBLIC = TRUE;
            }
            strcpy(roomBuf.rbname, msgBuf->mbroom);

            putRoom(thisRoom);
        }
    }
}

/************************************************************************/
/*      clearaccount()  initializes all group data                      */
/************************************************************************/
void clearaccount(void)
{
	uchar i;

	/* init days */
	for (i = 0; i < 7; i++)
		accountBuf->days[i] = 1;

	/* init hours & special hours */
	for (i = 0; i < 24; i++) {
		accountBuf->hours[i] = 1;
		accountBuf->special[i] = 0;
	}

	accountBuf->have_acc = FALSE;
	accountBuf->dayinc = 0.0;
	accountBuf->maxbal = 0.0;
	accountBuf->priority = 0.;
	accountBuf->dlmult = -1.0;
	accountBuf->ulmult = 1.0;
}

/************************************************************************/
/*      configcit() the <main> for configuration                        */
/************************************************************************/
void configcit(void)
{
    fcloseall();

    /* read config.cit */
	readconfig(1);

	/* read cron events */
    readcron();
	
    /* move to home-path */
	changedir(cfg->homepath);

    /* initialize & open any files */
    initfiles();

    if (msgZap)
        zapMsgFile();
    if (roomZap)
        zapRoomFile();
    if (logZap)
        zapLogFile();
    if (grpZap)
        zapGrpFile();
    if (hallZap)
        zapHallFile();
	if (hallgrpZap)
		zapHallgrpFile();
	if (roomgrpZap)
		zapRoomgrpFile();

    if (roomZap && !msgZap)
        roomBuild = TRUE;
    if (hallZap && !msgZap)
        hallBuild = TRUE;

    logInit();
    msgInit();
    RoomTabBld();

    if (hallBuild)
        buildhalls();

    fclose(grpfl);
    fclose(hallfl);
    fclose(roomfl);
    fclose(msgfl);
    fclose(logfl);
	fclose(hallgrpfl);
	fclose(roomgrpfl);

    doccr();
    cPrintf(ATOM1002);
    doccr();
}

/***********************************************************************/
/*    illegal() Prints out configure error message and aborts          */
/***********************************************************************/
void illegal(char *errorstring)
{
    doccr();
    cPrintf("%s", errorstring);
    doccr();
    cPrintf(ATOM1003);
    doccr();
    exit(7);
}

/************************************************************************/
/*      initfiles() -- initializes files, opens them                    */
/************************************************************************/
void initfiles(void)
{
    char *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
	char *hallgrpFile, *roomgrpFile;
	char scratch[64];

	changedir(cfg->homepath);

	if (cfg->msgpath[(strlen(cfg->msgpath) - 1)] == '\\')
		cfg->msgpath[(strlen(cfg->msgpath) - 1)] = '\0';


	flipFileLan(cfg->msgpath,1);

	sprintf(scratch, "%s\\%s", cfg->msgpath, ATOM1004);

	flipFileLan(cfg->msgpath,0);

    grpFile = ATOM1005;
    hallFile = ATOM1006;
    logFile = ATOM1007;
    msgFile = scratch;
    roomFile = ATOM1008;
	hallgrpFile = ATOM1151;
	roomgrpFile = ATOM1152;

    /* open group file */
    if ((grpfl = fopen(grpFile, "r+b")) == NULL) {
        cPrintf(ATOM1009, grpFile);
        doccr();
        if ((grpfl = fopen(grpFile, "w+b")) == NULL)
            illegal(ATOM1010);
        else {
            cPrintf(ATOM1011);
            doccr();
            grpZap = TRUE;
        }
    }
    /* open hall file */
    if ((hallfl = fopen(hallFile, "r+b")) == NULL) {
        cPrintf(ATOM1012, hallFile);
        doccr();
        if ((hallfl = fopen(hallFile, "w+b")) == NULL)
            illegal(ATOM1013);
        else {
            cPrintf(ATOM1014);
            doccr();
            hallZap = TRUE;
        }
    }
    /* open log file */
    if ((logfl = fopen(logFile, "r+b")) == NULL) {
        cPrintf(ATOM1015, logFile);
        doccr();
        if ((logfl = fopen(logFile, "w+b")) == NULL)
            illegal(ATOM1016);
        else {
            cPrintf(ATOM1017);
            doccr();
            logZap = TRUE;
        }
    }
    /* open message file */
    if ((msgfl = fopen(msgFile, "r+b")) == NULL) {
        cPrintf(ATOM1018);
        doccr();
        if ((msgfl = fopen(msgFile, "w+b")) == NULL)
            illegal(ATOM1019);
        else {
            cPrintf(ATOM1020);
            doccr();
            msgZap = TRUE;
        }
    }
    /* open room file */
    if ((roomfl = fopen(roomFile, "r+b")) == NULL) {
        cPrintf(ATOM1021, roomFile);
        doccr();
        if ((roomfl = fopen(roomFile, "w+b")) == NULL)
            illegal(ATOM1022);
        else {
            cPrintf(ATOM1023);
            doccr();
            roomZap = TRUE;
        }
    }
	/* open hallgrp file */
	if ((hallgrpfl = fopen(hallgrpFile, "r+b")) == NULL) {
		cPrintf(ATOM1154, hallgrpFile);
        doccr();
		if ((hallgrpfl = fopen(hallgrpFile, "w+b")) == NULL)
			illegal(ATOM1155);
        else {
			cPrintf(ATOM1153);
            doccr();
			hallgrpZap = TRUE;
        }
    }
	/* open roomgrp file */
	if ((roomgrpfl = fopen(roomgrpFile, "r+b")) == NULL) {
		cPrintf(ATOM1157, roomgrpFile);
        doccr();
		if ((roomgrpfl = fopen(roomgrpFile, "w+b")) == NULL)
			illegal(ATOM1158);
        else {
			cPrintf(ATOM1156);
            doccr();
			roomgrpZap = TRUE;
        }
    }
}

/************************************************************************/
/*      logInit() indexes log.dat                                       */
/************************************************************************/
void logInit(void)
{

	uchar i, x, incr, last_incr;
    int logSort();
    int count = 0;

	doccr();
    doccr();
    cPrintf(ATOM1024);
    doccr();

	cfg->callno = 0L;

    rewind(logfl);
    /* clear logTab */
	for (i = 0; i < cfg->MAXLOGTAB; i++)
		logTab[i].ltcallno = 0L;

	for(i=0;i<50;i++) cPrintf("%c",177);
	cPrintf("\r");

	incr = last_incr = 0;
    /* load logTab: */
	for (thisLog = 0; thisLog < cfg->MAXLOGTAB; thisLog++) {

		incr = (50 * thisLog) / cfg->MAXLOGTAB;
        if(incr != last_incr) {
			for(x=0;x < incr-last_incr; x++) cPrintf("%c",219);
        }
        last_incr = incr;

        getLog(&logBuf, thisLog);

		if (logBuf.callno > cfg->callno)
			cfg->callno = logBuf.callno;

    /* count valid entries:             */

        if (logBuf.lbflags.L_INUSE == 1)
            count++;


    /* copy relevant info into index:   */
        logTab[thisLog].ltcallno = logBuf.callno;
        logTab[thisLog].ltlogSlot = thisLog;
        logTab[thisLog].permanent = logBuf.lbflags.PERMANENT;

        if (logBuf.lbflags.L_INUSE == 1) {
            logTab[thisLog].ltnmhash = hash(logBuf.lbname);
            logTab[thisLog].ltinhash = hash(logBuf.lbin);
            logTab[thisLog].ltpwhash = hash(logBuf.lbpw);
        } else {
            logTab[thisLog].ltnmhash = 0;
            logTab[thisLog].ltinhash = 0;
            logTab[thisLog].ltpwhash = 0;
        }
    }
    doccr();
	cPrintf(ATOM1026, cfg->callno);
    doccr();
    cPrintf(ATOM1027, count);
    doccr();

	qsort(logTab, (unsigned) cfg->MAXLOGTAB,
    (unsigned) sizeof(*logTab), logSort);

}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
int logSort(const void *p1, const void *p2)
/* struct lTable *s1, *s2;     */
{
    struct lTable *s1, *s2;

    s1 = (struct lTable *) p1;
    s2 = (struct lTable *) p2;
    if (s1->ltnmhash == 0 && (struct lTable *) (s2)->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltcallno < s2->ltcallno)
        return 1;
    if (s1->ltcallno > s2->ltcallno)
        return -1;
    return 0;
}

/************************************************************************/
/*		msgInit() sets up lowId, highId, cfg->catSector and cfg->catChar, */
/*      by scanning over message.buf                                    */
/************************************************************************/
void msgInit(void)
{
	ulong first, here, cntr, maxpos;
    int makeroom;
    int skipcounter = 0;    /* maybe need to skip a few . Dont put them in message index */
    int slot;
	uchar x, incr, last_incr;

    doccr();
    doccr();
    cPrintf(ATOM1028);
    doccr();

	/* Go to the end		  */
	fseek(msgfl, 0L, SEEK_END);

	maxpos = ftell(msgfl);

	/* start at the beginning */
	fseek(msgfl, 0L, 0);

    getMessage();

    /* get the ID# */
    sscanf(msgBuf->mbId, "%ld", &first);

    /* put the index in its place */
    /* mark first entry of index as a reference point */

	cfg->mtoldest = first;

    indexmessage(first);

	cfg->newest = cfg->oldest = first;

	cfg->catLoc = ftell(msgfl);

	for(x=0;x<50;x++) cPrintf("%c",177);
    cPrintf("\r");

	incr = last_incr = 0;
	cntr = 1;

    while (TRUE) {
        getMessage();

        sscanf(msgBuf->mbId, "%ld", &here);

        if (here == first)
            break;

		cntr++;
		incr = (50 * ftell(msgfl)) / maxpos;
        if(incr != last_incr) {
			for(x=0;x < incr-last_incr; x++) cPrintf("%c",219);
        }
        last_incr = incr;

    /* find highest and lowest message IDs: */
    /* >here< is the dip pholks             */
		if (here < cfg->oldest) {
			slot = (indexslot(cfg->newest) + 1);

			makeroom = (int) (cfg->mtoldest - here);

        /* check to see if our table can hold remaining msgs */
			if (cfg->nmessages < (makeroom + slot)) {
				skipcounter = (makeroom + slot) - cfg->nmessages;

                slidemsgTab(makeroom - skipcounter);

				cfg->mtoldest = (here + (ulong) skipcounter);

            } else {
        /* nmessages can handle it.. Just make room */
                slidemsgTab(makeroom);
				cfg->mtoldest = here;
            }
			cfg->oldest = here;
        }
		if (here > cfg->newest) {
			cfg->newest = here;

        /* read rest of message in and remember where it ends,      */
        /* in case it turns out to be the last message              */
        /* in which case, that's where to start writing next message */
            while (dGetWord(msgBuf->mbtext, MAXTEXT));

			cfg->catLoc = ftell(msgfl);
        }
    /* check to see if our table is big enough to handle it */
		if ((int) (here - cfg->mtoldest) >= cfg->nmessages) {
            crunchmsgTab(1);
        }
        if (skipcounter) {
            skipcounter--;
        } else {
            indexmessage(here);
        }
    }
	doccr();
	cPrintf(ATOM1101, cntr, cfg->newest);
    doccr();

	buildcopies();
}

/************************************************************************/
/*      readaccount()  reads grpdata.cit values into group structure    */
/************************************************************************/
void readaccount(void)
{
    FILE *fBuf;
    char line[90];
    char *words[256];
	uchar i, j, k, l, count;
    int groupslot = ERROR;
	int hours, first;
	struct accounting *save_acct;

	/*	We are re-reading the table so......
	 *	Let's release the memory and start over
	 */
	if(start_acct) {
		if(start_acct->next) {
			save_acct = start_acct->next;
			while(save_acct) {
				accountBuf = save_acct;
				save_acct = save_acct->next;
				farfree(accountBuf);
			}
			start_acct->next = NULL;
			if(farheapcheck() < 0)
				crashout("Memory corrupted after releasing account pointers");
		}
	}
	else {
		crashout("Lost starting account pointer");
	}
	accountBuf = start_acct;

	clearaccount(); 	/* initialize accounting data */

    /* move to home-path */
	changedir(cfg->homepath);

    if ((fBuf = fopen(ATOM1029, "r")) == NULL) {
    /* ASCII mode */
        cPrintf(ATOM1030);
        doccr();
        exit(1);
    }

	first = TRUE;
	while (fgets(line, 90, fBuf) != NULL) {
        if (line[0] != '#')
            continue;

        count = parse_it(words, line);

        for (i = 0; grpkeywords[i] != NULL; i++) {
            if (strcmpi(words[0], grpkeywords[i]) == SAMESTRING) {
                break;
            }
        }

		switch (i) {
            case GRK_DAYS:
                if (groupslot == ERROR)
                    break;

				/* init days */
                for (j = 0; j < 7; j++)
					accountBuf->days[j] = 0;

                for (j = 1; j < count; j++) {
                    for (k = 0; daykeywords[k] != NULL; k++) {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING) {
                            break;
                        }
                    }
                    if (k < 7)
						accountBuf->days[k] = TRUE;
                    else if (k == 7) {  /* any */
						for (l = 0; l < 7; ++l)
							accountBuf->days[l] = TRUE;
                    } else {
                        doccr();
                        cPrintf(
                        ATOM1031,
                        words[j]);
                        doccr();
                    }
                }
                break;

            case GRK_GROUP:
                groupslot = groupexists(words[1]);
                if (groupslot == ERROR) {
                    doccr();
                    cPrintf(ATOM1032, words[1]);
                    doccr();
                }
				else {
					if(!first) {
						if(accountBuf->next == NULL) {
							accountBuf->next = farcalloc(1UL,(ulong) sizeof(struct accounting));
							if(accountBuf->next == NULL) {
								crashout("Can not allocate memory for accounting");
							}
							accountBuf = accountBuf->next;
							accountBuf->next = NULL;
						}
						else accountBuf = accountBuf->next;
					}
					if(first && groupslot != 0) {
						crashout("Null group MUST be first in grpdata.cit");
					}
					first = FALSE;
					accountBuf->grpslot = groupslot;
					accountBuf->have_acc = TRUE;
				}
                break;

            case GRK_HOURS:
                if (groupslot == ERROR)
                    break;

				/* init hours */
                for (j = 0; j < 24; j++)
					accountBuf->hours[j] = 0;

                for (j = 1; j < count; j++) {
                    if (strcmpi(words[j], ATOM1033) == SAMESTRING) {
                        for (l = 0; l < 24; l++)
							accountBuf->hours[l] = TRUE;
                    } else {
						hours = atoi(words[j]);

						if (hours > 23) {
                            doccr();
                            cPrintf(ATOM1034,
							hours);
                            doccr();
                        } else
							accountBuf->hours[hours] = TRUE;
                    }
                }
                break;

            case GRK_DAYINC:
                if (groupslot == ERROR)
                    break;

                if (count > 1) {
					sscanf(words[1], "%f ",&accountBuf->dayinc);  /* float */
                }
                break;

            case GRK_DLMULT:
                if (groupslot == ERROR)
                    break;

                if (count > 1) {
					sscanf(words[1], "%f ",&accountBuf->dlmult);  /* float */
                }
                break;

            case GRK_ULMULT:
                if (groupslot == ERROR)
                    break;

                if (count > 1) {
					sscanf(words[1], "%f ",&accountBuf->ulmult);  /* float */
                }
                break;

            case GRK_PRIORITY:
                if (groupslot == ERROR)
                    break;

                if (count > 1) {
					sscanf(words[1], "%f ",&accountBuf->priority);/* float */
                }
                break;

            case GRK_MAXBAL:
                if (groupslot == ERROR)
                    break;

                if (count > 1) {
					sscanf(words[1], "%f ",&accountBuf->maxbal);  /* float */
                }
                break;



            case GRK_SPECIAL:
                if (groupslot == ERROR)
                    break;

				/* init hours */
                for (j = 0; j < 24; j++)
					accountBuf->special[j] = 0;

                for (j = 1; j < count; j++) {
                    if (strcmpi(words[j], ATOM1035) == SAMESTRING) {
                        for (l = 0; l < 24; l++)
							accountBuf->special[l] = TRUE;
                    } else {
						hours = atoi(words[j]);

						if (hours > 23) {
                            doccr();
							cPrintf(ATOM1036, hours);
                            doccr();
                        } else
							accountBuf->special[hours] = TRUE;
                    }

                }
                break;
        }

    }
    fclose(fBuf);
}

/************************************************************************/
/*      readprotocols() reads external.cit values into ext   structure  */
/************************************************************************/
void readprotocols(void)
{
    FILE *fBuf;
	char *line;
    char *words[256];
	int i, j, count;
    int cmd = 0;
	struct ext_prot    *save_extrn;
	struct ext_editor  *save_edit;
	struct ext_command *save_extCmd;


    extrncmd[0] = NULL;
    editcmd[0] = NULL;
    default_group[0] = NULL;
	ChatSnd.type = 0;

	if((line = calloc(120,sizeof(char))) == NULL) {
		return;
	}

	/* move to home-path */
	changedir(cfg->homepath);

    if ((fBuf = fopen(ATOM1037, "r")) == NULL) {    /* ASCII mode */
        cPrintf(ATOM1038);
        doccr();
        exit(1);
    }

	/*	We are re-reading the table so......
	 *	Let's release the memory and start over
	 */
	if(start_edit) {
		if(start_edit->next) {
			save_edit = start_edit->next;
			while(save_edit) {
				edit = save_edit;
				save_edit = save_edit->next;
				farfree(edit);
			}
			start_edit->next = NULL;
			if(farheapcheck() < 0)
				crashout("Memory corrupted after releasing edit pointers");
		}
	}
	else {
		crashout("Lost starting edit pointer");
	}

	if(start_extCmd) {
		if(start_extCmd->next) {
			save_extCmd = start_extCmd->next;
			while(save_extCmd) {
				extCmd = save_extCmd;
				save_extCmd = save_extCmd->next;
				farfree(extCmd);
			}
			start_extCmd->next = NULL;
			if(farheapcheck() < 0)
				crashout("Memory corrupted after releasing extCmd pointers");
		}
	}
	else {
		crashout("Lost starting extCmd pointer");
	}

	if(start_extrn) {
		if(start_extrn->next) {
			save_extrn = start_extrn->next;
			while(save_extrn) {
				extrn = save_extrn;
				save_extrn = save_extrn->next;
				farfree(extrn);
			}
			start_extrn->next = NULL;
			if(farheapcheck() < 0)
				crashout("Memory corrupted after releasing extrn pointers");
		}
	}
	else {
		crashout("Lost starting extrn pointer");
	}

    while (fgets(line, 120, fBuf) != NULL) {
        if (line[0] != '#')
            continue;

        count = parse_it(words, line);

		if (strcmpi(ATOM1039, words[0]) == SAMESTRING) {
            j = strlen(extrncmd);
			extrn = start_extrn;
			for(i=0; i<j; i++) {
				if(extrn->next == NULL) {
					 extrn->next = farcalloc(1UL,(ulong) sizeof(struct ext_prot));
					 if(extrn->next == NULL) {
						 crashout("Can not allocate memory for external protocol");
					 }
					 extrn = extrn->next;
					 extrn->next = NULL;
				}
				else extrn = extrn->next;
            }

			/* Field 1, name */
            if (strlen(words[1]) > 19)
                illegal(ATOM1040);

			/* Field 2, Net/console only flag */
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
                illegal(ATOM1041);

			/* Field 3, Batch protcol flag */
            if (atoi(words[3]) < 0 || atoi(words[3]) > 1)
                illegal(ATOM1042);

			/* Field 4, Block size, 0 means unknown or variable */
            if (atoi(words[4]) < 0 || atoi(words[4]) > 10 * 1024)
                illegal(ATOM1043);

			/* Field 5, Recieve command */
            if (strlen(words[5]) > 39)
                illegal(ATOM1044);

			/* Field 3, Send command */
            if (strlen(words[6]) > 39)
                illegal(ATOM1045);

            if (j >= MAXEXTERN)
                illegal(ATOM1046);

			strcpy(extrn->ex_name, words[1]);
			extrn->ex_console = atoi(words[2]);
			extrn->ex_batch = atoi(words[3]);
			extrn->ex_block = atoi(words[4]);
			strcpy(extrn->ex_rcv, words[5]);
			strcpy(extrn->ex_snd, words[6]);
            extrncmd[j] = tolower(*words[1]);
            extrncmd[j + 1] = '\0';
        } else if (strcmpi(ATOM1047, words[0]) == SAMESTRING) {
            j = strlen(editcmd);
			edit = start_edit;
			for(i=0; i<j; i++) {
				if(edit->next == NULL) {
					 edit->next = farcalloc(1UL,(ulong) sizeof(struct ext_editor));
					 if(edit->next == NULL) {
						 crashout("Can not allocate memory for external editor");
					 }
					 edit = edit->next;
					 edit->next = NULL;
				}
				else edit = edit->next;
			}

            if (strlen(words[1]) > 19)
                illegal(ATOM1048);
            if (strlen(words[3]) > 29)
                illegal(ATOM1049);
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
                illegal(ATOM1050);
            if (j > 19)
                illegal(ATOM1051);

			strcpy(edit->ed_name, words[1]);
			edit->ed_local = atoi(words[2]);
			strcpy(edit->ed_cmd, words[3]);
            editcmd[j] = tolower(*words[1]);
            editcmd[j + 1] = '\0';
        } else if (strcmpi(ATOM1052, words[0]) == SAMESTRING) {
            if (strlen(words[1]) > 19)
                illegal(ATOM1053);
            strcpy(default_group, words[1]);
        } else if (strcmpi(ATOM1054, words[0]) == SAMESTRING) {
            if (cmd >= MAXEXTERN) {
                illegal(ATOM1055);
            }
            if (count < 3)
                illegal(ATOM1056);
            if (*words[2] < '0' || *words[2] > '1')
                illegal(ATOM1057);
            if (strlen(words[1]) > 30) {
                illegal(ATOM1058);
            }
            if (strlen(words[3]) > 40) {
                illegal(ATOM1059);
            }
			extCmd = start_extCmd;
			for(i=0; i<cmd; i++) {
				if(extCmd->next == NULL) {
					 extCmd->next = farcalloc(1UL,(ulong) sizeof(struct ext_command));
					 if(extCmd->next == NULL) {
						 crashout("Can not allocate memory for external commands");
					 }
					 extCmd = extCmd->next;
					 extCmd->next = NULL;
				}
				else extCmd = extCmd->next;
			}

			strcpy(extCmd->name, words[1]);
			extCmd->local = *words[2] - '0';
			strcpy(extCmd->command, words[3]);
            ++cmd;
        } else if (strcmpi(ATOM1060, words[0]) == SAMESTRING) {
            if (count != 2)
                illegal(ATOM1061);
            if (!isdigit(*words[1]))
                illegal(ATOM1062);
			cfg->ad_chance = atoi(words[1]);
			++cmd;
        } else if (strcmpi(ATOM1063, words[0]) == SAMESTRING) {
            if (count != 2)
                illegal(ATOM1064);
            if (!isdigit(*words[1]))
                illegal(ATOM1065);
			cfg->ad_time = atoi(words[1]);
            ++cmd;
		} else if (strcmpi(ATOM1126, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1127);
			ChatSnd.type = atoi(words[1]);
			if(ChatSnd.type < 1 || ChatSnd.type > 12)
			{
				illegal(ATOM1128);
				ChatSnd.type = 0;
			}
		} else if (strcmpi(ATOM1129, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1130);
			ChatSnd.hi = atoi(words[1]);
			if(ChatSnd.hi < 1 || ChatSnd.hi > 32767)
			{
				illegal(ATOM1131);
				ChatSnd.type = 0;
            }
        } else if (strcmpi(ATOM1132, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1133);
			ChatSnd.lo = atoi(words[1]);
			if(ChatSnd.lo < 0 || ChatSnd.lo > 32767)
			{
				illegal(ATOM1134);
				ChatSnd.type = 0;
            }
        } else if (strcmpi(ATOM1135, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1136);
			ChatSnd.incr = atoi(words[1]);
			if(ChatSnd.incr < 1 || ChatSnd.incr > 32767)
			{
				illegal(ATOM1137);
				ChatSnd.type = 0;
            }
        } else if (strcmpi(ATOM1138, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1139);
			ChatSnd.dly1 = atoi(words[1]);
			if(ChatSnd.dly1 < 0 || ChatSnd.dly1 > 32767)
			{
				illegal(ATOM1140);
				ChatSnd.type = 0;
            }
        } else if (strcmpi(ATOM1141, words[0]) == SAMESTRING) {
            if (count != 2)
				illegal(ATOM1142);
			ChatSnd.dly2 = atoi(words[1]);
			if(ChatSnd.dly2 < 0 || ChatSnd.dly2 > 32767)
			{
				illegal(ATOM1143);
				ChatSnd.type = 0;
            }
        }
    }
    fclose(fBuf);
	free(line);
}

/* -------------------------------------------------------------------- */
/*		readchatbell() reads chat.cit values into ChatBell structure	*/
/* -------------------------------------------------------------------- */
void readchatbell()
{
    FILE *fBuf;
    char line[91];
	char *words[50], *ptr;
	int i, count;

    /* move to home-path */
	changedir(cfg->homepath);

	if ((fBuf = fopen("chat.cit", "r")) == NULL) {  /* ASCII mode */
		cPrintf("Can't find Chat.cit!");
        doccr();
        exit(1);
    }
    line[90] = '\0';

    while (fgets(line, 90, fBuf)) {
        if (line[0] != '#')
            continue;

        count = parse_it(words, line);

		ptr = words[0];
		ptr++;
		for (i = 0; daykeywords[i]; i++) {
			if (strcmpi(ptr, daykeywords[i]) == SAMESTRING) {
                break;
            }
        }
		if(i == 9) {
			cPrintf("Chat.cit - Warning: Unknown variable %s", words[0]);
            doccr();
            break;
		}
		else {
			if(i == 8) {
				for(i=0; i<7; i++)
					cfgchatbell(i, count, words);
				break;
			}
			else {
				cfgchatbell(i, count, words);
			}
		}
	}
	fclose(fBuf);
}

/* -------------------------------------------------------------------- */
/*		cfgchatbell() reads chat.cit values into ChatBell structure 	*/
/* -------------------------------------------------------------------- */
void cfgchatbell(int i, int count, char *words[])
{

	char temp[10];
	char *ptr;
	int j, l;
	uchar hours, minutes, range, end_minutes, end_hour;


	/* init hours */
	for (j = 0; j < 96; j++)
		chat_bell->days[i].hours[j] = 0;

	for (j = 1; j < count; j++) {
		if (strcmpi(words[j], "Any") == SAMESTRING) {
			for (l = 0; l < 96; ++l)
				chat_bell->days[i].hours[l] = TRUE;
		} else {
			range = FALSE;
			if(strchr(words[j],'-')) {
				range = TRUE;
				strcpy(temp,strchr(words[j],'-')+1);
				if((ptr = strchr(temp,':')) != NULL) {
				   end_minutes = atoi(ptr+1) / 15;
				   *ptr = 0;
				}
				else {
				   end_minutes = 0;
				}
				end_hour = atoi(temp);
				ptr = strchr(words[j],'-');
				*ptr = 0;
			}
			if((ptr = strchr(words[j],':')) != NULL) {
				minutes = atoi(ptr+1) / 15;
				*ptr = 0;
			}
			else {
				minutes = 0;
			}
			hours = atoi(words[j]);

			if (hours > 23) {
				doccr();
				cPrintf("Chat.Cit - Warning: Invalid hour %d ",hours);
				doccr();
			} else {
				if(range) {
					l=(hours*4) + minutes;
					while(TRUE) {
						chat_bell->days[i].hours[l] = TRUE;
						l++;
						if(l == 96) l = 0;
						if(l ==(end_hour*4)+end_minutes) break;
					}
				}
				else {
					hours = (hours*4) + minutes;
					chat_bell->days[i].hours[hours] = TRUE;
				}
			}
		}
	}
}

/* -------------------------------------------------------------------- */
/*		rules_read() reads tz.cit values into tzr structure 			*/
/* -------------------------------------------------------------------- */
void rules_read()
{
	FILE *fp;
	char lne[80], tmp[20], *ptr;
	int i, x, start, mst, cntr, min;

    /* move to home-path */
	changedir(cfg->homepath);

	if((fp = fopen("tz.cit","rt")) == NULL) {
	   cPrintf("No file tz.cit\n");
	   return;
	}

	mst = 0;

	while(!feof(fp))
	{
		if(fgets(lne,80,fp) == NULL) return;
		lne[strlen(lne)-1] = ' ';
		if(strnicmp(lne,"rule",4) == SAMESTRING) {
			if(mst == 4) {
				cPrintf("Too many line entries in tz.cit!\n");
				return;
			}
			start = 4;
			cntr = 0;
			while(1)
			{
				while(isspace(lne[start])) start++;

				i = 0;
				for(x=start;!isspace(lne[x]);x++, start++)
				{
					tmp[i++] = lne[x];
					if(i >= 20) {
						cPrintf("Data in line too long to handle in tz.cit!\n");
						return;
					}
				}
				tmp[i] = '\0';
				switch(cntr)
				{
					case 0:   // from year
						tzr[mst].from = atoi(tmp);
						break;
					case 1:   // to year
						tzr[mst].to = atoi(tmp);
						if(tzr[mst].to == 0) tzr[mst].to = 2100;
						break;
					case 2:   // type
						strcpy(tzr[mst].typ,tmp);
						break;
					case 3:   // in - convert month
						i = index("JanFebMarAprMayJunJulAugSepOctNovDec",tmp,1);
						if(i)
						{
							tzr[mst].month = (i/3)+1;
						} else tzr[mst].month = 0;
						break;
					case 4:   // on
						strcpy(tzr[mst].on,tmp);
						break;
					case 5:   // at - convert time
						if(strlen(tmp) >= 1)
						{
							ptr = &tmp[strlen(tmp)-2];
							min = atoi(ptr);
							tmp[strlen(tmp) - 3] = '\0';
						} else min = 0;
						tzr[mst].at_min = min;
						tzr[mst].at_hr = atoi(tmp);
						break;
					case 6:   // save - convert time
						if(strlen(tmp) >= 1)
						{
							ptr = &tmp[strlen(tmp)-2];
							min = atoi(ptr);
							tmp[strlen(tmp) - 3] = '\0';
						} else min = 0;
						tzr[mst].save_min = min;
						tzr[mst].save_hr = atoi(tmp);
						break;
					case 7:   // letter
						tzr[mst].ltr = tmp[0];
						tzr[mst].done = 0;
						break;
				}
				cntr++;
				if(cntr == 8)
				{
					mst++;
					break;
				}
			}
		}
	}
	return;
}

/*
 * count the lines that start with keyword...
 *
int keyword_count(key, filename)
char *key;
char *filename;
{
    FILE *fBuf;
	char *line;
    char *words[256];
    int  count = 0;

	if((line = calloc(120,sizeof(char))) == NULL) {
		return;
	}


	changedir(cfg->homepath);

    if ((fBuf = fopen(filename, "r")) == NULL) {
        cPrintf(ATOM1066, filename); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL) {
        if (line[0] != '#')  continue;

        parse_it( words, line);

        if (strcmpi(key, words[0]) == SAMESTRING)
          count++;
   }

   fclose(fBuf);

   free(line);

   return (count == 0 ? 1 : count);
} */

/************************************************************************
 *      checkDir() check if required directory exists, crashout if not.
 ************************************************************************/
/*
void checkDir(char *path)
{
	char buf[128];
	if (changedir(path) == -1) {
		sprintf(buf, "No path %s", path);
		crashout(buf);
	}
}
*/
/************************************************************************/
/*      readconfig() reads config.cit values                            */
/************************************************************************/
void readconfig(int cfg_ctdl)
{
    FILE *fBuf;
	char *line;
	char *words[25];
	uchar i, j, k, l, count, att;
    label notkeyword;
    char valid = FALSE;
    char found[K_NWORDS + 2];
    int lineNo = 0;
	float flt;

	if((line = calloc(90,sizeof(char))) == NULL) {
		return;
	}

	for (i = 0; i <= K_NWORDS; i++)
        found[i] = FALSE;

	if ((fBuf = fopen(ATOM1067, "r")) == NULL) {    /* ASCII mode */
        cPrintf(ATOM1068);
        doccr();
        exit(3);
    }
    while (fgets(line, 90, fBuf) != NULL) {
        lineNo++;

        if (line[0] != '#')
            continue;

        count = parse_it(words, line);

        for (i = 0; keywords[i] != NULL; i++) {
            if (strcmpi(words[0], keywords[i]) == SAMESTRING) {
                break;
            }
        }

        if (keywords[i] == NULL) {
            cPrintf(ATOM1069, lineNo,
            words[0]);
            doccr();
            continue;
        } else {
            if (found[i] == TRUE) {
                cPrintf(ATOM1070, lineNo, words[0]);
                doccr();
            } else {
                found[i] = TRUE;
            }
        }

		switch (i) {
            case K_ACCOUNTING:
				cfg->accounting = atoi(words[1]);
                break;

            case K_IDLE_WAIT:
				cfg->idle = atoi(words[1]);
                break;

            case K_ALLDONE:
                break;

            case K_ATTR:
                sscanf(words[1], "%x ", &att);  /* hex! */
				cfg->attr = (uchar) att;
                break;

            case K_WATTR:
                sscanf(words[1], "%x ", &att);  /* hex! */
				cfg->wattr = (uchar) att;
                break;

            case K_CATTR:
                sscanf(words[1], "%x ", &att);  /* hex! */
				cfg->cattr = (uchar) att;
                break;

            case K_BATTR:
                sscanf(words[1], "%x ", &att);  /* hex! */
				cfg->battr = att;
                break;

            case K_UTTR:
                sscanf(words[1], "%x ", &att);  /* hex! */
				cfg->uttr = att;
                break;

            case K_IBMROOM:
				cfg->ibmroom = atoi(words[1]);
                break;

            case K_INIT_BAUD:
				cfg->initbaud = atoi(words[1]);
                break;

            case K_BIOS:
				cfg->bios = atoi(words[1]);
                break;

            case K_DUMB_MODEM:
				cfg->dumbmodem = atoi(words[1]);
                break;

            case K_READLLOG:
				cfg->readluser = atoi(words[1]);
                break;

            case K_READLOG:
				cfg->readuser = atoi(words[1]);
                break;

            case K_AIDEREADLOG:
				cfg->aidereaduser = atoi(words[1]);
                break;

            case K_ENTERSUR:
				cfg->entersur = atoi(words[1]);
                break;

            case K_DATESTAMP:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1071);

				strcpy(cfg->datestamp, words[1]);
                break;

            case K_VDATESTAMP:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1072);

				strcpy(cfg->vdatestamp, words[1]);
                break;

            case K_ENTEROK:
				cfg->unlogEnterOk = atoi(words[1]);
                break;

            case K_FORCELOGIN:
				cfg->forcelogin = atoi(words[1]);
                break;

            case K_MODERATE:
				cfg->moderate = atoi(words[1]);
                break;

            case K_HELPPATH:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1073);

				strcpy(cfg->helppath, words[1]);
                break;

            case K_HELPPATH_2:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1073);

				strcpy(cfg->helppath_2, words[1]);
                break;

            case K_TEMPPATH:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1074);

				strcpy(cfg->temppath, words[1]);
                break;

            case K_HOMEPATH:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1075);

				strcpy(cfg->homepath, words[1]);
                break;

            case K_MAILPATH:
                if (strlen(words[1]) > 63)
                    illegal("Mail path too long!");

				strcpy(cfg->mailpath, words[1]);
                break;

            case K_KILL:
				cfg->kill = atoi(words[1]);
                break;

            case K_LINEFEEDS:
				cfg->linefeeds = atoi(words[1]);
                break;

            case K_LOGINSTATS:
				cfg->loginstats = atoi(words[1]);
                break;

            case K_MAXBALANCE:
				sscanf(words[1], "%f ", &flt);   /* float */
				cfg->maxbalance = flt;
				break;

            case K_MAXLOGTAB:
				if(cfg_ctdl) {
					cfg->MAXLOGTAB = atoi(words[1]);
					if (cfg->MAXLOGTAB > 32000 || cfg->MAXLOGTAB < 5)
						illegal("#MAXLOGTAB out of range! 5 - 32000!");
                }
                break;

            case K_MESSAGE_ROOM:
				if(cfg_ctdl)
					cfg->MessageRoom = atoi(words[1]);
                break;

            case K_NEWUSERAPP:
                if (strlen(words[1]) > 12)
                    illegal(ATOM1076);

				strcpy(cfg->newuserapp, words[1]);
                break;

            case K_PRIVATE:
				cfg->private = atoi(words[1]);
                break;

            case K_MAXTEXT:
				if(cfg_ctdl)
					cfg->maxtext = atoi(words[1]);
                break;

            case K_MAX_WARN:
				cfg->maxwarn = atoi(words[1]);
                break;

            case K_MDATA:
				cfg->mdata = atoi(words[1]);
				if ((cfg->mdata < 1) || (cfg->mdata > 4))
                    illegal(ATOM1077);
                break;

            case K_MAXFILES:
				cfg->maxfiles = atoi(words[1]);
                break;

            case K_MSGPATH:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1078);

				strcpy(cfg->msgpath, words[1]);
                break;

            case K_F6PASSWORD:
                if (strlen(words[1]) > 19)
                    illegal(ATOM1079);

				strcpy(cfg->f6pass, words[1]);
                break;

            case K_APPLICATIONS:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1080);

				strcpy(cfg->aplpath, words[1]);
                break;

            case K_MESSAGEK:
				if(cfg_ctdl)
					cfg->messagek = atoi(words[1]);
                break;

            case K_MODSETUP:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1081);

				strcpy(cfg->modsetup, words[1]);
                break;

            case K_DIAL_INIT:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1082);

				strcpy(cfg->dialsetup, words[1]);
                break;

            case K_DIAL_PREF:
                if (strlen(words[1]) > 20)
                    illegal(ATOM1083);

				strcpy(cfg->dialpref, words[1]);
                break;

            case K_NEWBAL:
				sscanf(words[1], "%f ", &flt);   /* float */
				cfg->newbal = flt;
                break;

            case K_SURNAMES:
				cfg->surnames = (atoi(words[1]) != 0);
				cfg->netsurname = (atoi(words[1]) == 2);
                break;

            case K_AIDEHALL:
				cfg->aidehall = atoi(words[1]);
                break;

            case K_NMESSAGES:
				if(cfg_ctdl)
					cfg->nmessages = atoi(words[1]);
                break;

            case K_NODENAME:
				if (strlen(words[1]) > 30)
                    illegal(ATOM1084);

				strcpy(cfg->nodeTitle, words[1]);
                break;

            case K_NODEREGION:
				if (strlen(words[1]) > 30)
                    illegal(ATOM1085);

				strcpy(cfg->nodeRegion, words[1]);
                break;

            case K_NOPWECHO:
				cfg->nopwecho = (unsigned char) atoi(words[1]);
                break;

            case K_NULLS:
				cfg->nulls = atoi(words[1]);
                break;

            case K_OFFHOOK:
				cfg->offhook = atoi(words[1]);
                break;

            case K_OLDCOUNT:
				cfg->oldcount = atoi(words[1]);
                break;

            case K_PRINTER:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1086);

				strcpy(cfg->printer, words[1]);
                break;

            case K_READOK:
				cfg->unlogReadOk = atoi(words[1]);
                break;

            case K_ROOMOK:
				cfg->nonAideRoomOk = atoi(words[1]);
                break;

            case K_ROOMTELL:
				cfg->roomtell = atoi(words[1]);
                break;

            case K_ROOMPATH:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1087);

				strcpy(cfg->roompath, words[1]);
                break;

            case K_SUBHUBS:
				cfg->subhubs = atoi(words[1]);
                break;

            case K_TABS:
				cfg->tabs = atoi(words[1]);
                break;

            case K_TIMEOUT:
				cfg->timeout = atoi(words[1]);
                break;

            case K_TRAP:
                for (j = 1; j < count; j++) {
                    valid = FALSE;

                    for (k = 0; trapkeywords[k] != NULL; k++) {
                        sprintf(notkeyword, "!%s", trapkeywords[k]);

                        if (strcmpi(words[j], trapkeywords[k]) == SAMESTRING) {
                            valid = TRUE;

                            if (k == 0) {   /* ALL */
                                for (l = 0; l < 16; ++l)
									cfg->trapit[l] = TRUE;
                            } else
								cfg->trapit[k] = TRUE;
                        } else if (strcmpi(words[j], notkeyword) == SAMESTRING) {
                            valid = TRUE;

                            if (k == 0) {   /* ALL */
                                for (l = 0; l < 16; ++l)
									cfg->trapit[l] = FALSE;
                            } else
								cfg->trapit[k] = FALSE;
                        }
                    }

                    if (!valid) {
                        doccr();
                        cPrintf(ATOM1088, words[j]);
                        doccr();
                    }
                }
                break;

            case K_TRAP_FILE:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1090);

				strcpy(cfg->trapfile, words[1]);

                break;

            case K_UNLOGGEDBALANCE:
				sscanf(words[1], "%f ", &flt); /* float */
				cfg->unlogbal = flt;
                break;

            case K_UNLOGTIMEOUT:
				cfg->unlogtimeout = atoi(words[1]);
                break;

            case K_UPPERCASE:
				cfg->uppercase = atoi(words[1]);
                break;

            case K_USER:
                for (j = 0; j < 5; ++j)
					cfg->user[j] = 0;

                for (j = 1; j < count; j++) {
                    valid = FALSE;

                    for (k = 0; userkeywords[k] != NULL; k++) {
                        if (strcmpi(words[j], userkeywords[k]) == SAMESTRING) {
                            valid = TRUE;
							cfg->user[k] = TRUE;
                        }
                    }

                    if (!valid) {
                        doccr();
                        cPrintf(ATOM1091,
                        words[j]);
                        doccr();
                    }
                }
                break;

            case K_WIDTH:
				cfg->width = atoi(words[1]);
                break;

            case K_SYSOP:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1092);

				strcpy(cfg->sysop, words[1]);
                break;

            case K_CREDIT_NAME:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1093);

				strcpy(cfg->credit_name, words[1]);
                break;

            case K_SWAP_FILE:
                if (strlen(words[1]) > 63)
                    illegal(ATOM1125);

				strcpy(cfg->swapfile, words[1]);
                break;

			case K_SCREEN_SAVE:
				cfg->screen_save = atol(words[1]);
				break;

			case K_LAN:
				cfg->LAN = atoi(words[1]);
                break;

			case K_WIDEROOM:
				cfg->wideroom = atoi(words[1]);
                break;

			case K_MAXSOUND:
				cfg->MaxSound = atoi(words[1]);
                break;

			case K_CHATTAB:
				cfg->ChatTab = atoi(words[1]);
                break;

			case K_MENUOK:
				cfg->MenuOk = atoi(words[1]);
                break;

			case K_MENULINES:
				cfg->MenuLines = atoi(words[1]);
                break;

			case K_STICKYRM:
				cfg->Stickyrm = atoi(words[1]);
                break;

			case K_MEMFREE:
				cfg->MEMFREE = atol(words[1]);
                break;

			case K_NODESTATE:
				if (strlen(words[1]) > 4)
					illegal(ATOM1144);

				strcpy(cfg->nodeState, words[1]);
                break;

			case K_NODECNTRY:
				if (strlen(words[1]) > 4)
					illegal(ATOM1145);

				strcpy(cfg->nodeCntry, words[1]);
                break;

			case K_NODEPHONE:
				if (strlen(words[1]) > 30)
					illegal(ATOM1146);

				strcpy(cfg->nodePhone, words[1]);
                break;

			case K_SYSOPREGION:
				if (strlen(words[1]) > 30)
					illegal(ATOM1147);

				strcpy(cfg->SysOpRegion, words[1]);
                break;

			case K_SYSOPCNTRY:
				if (strlen(words[1]) > 30)
					illegal(ATOM1148);

				strcpy(cfg->SysOpCntry, words[1]);
                break;

            default:
                cPrintf(ATOM1094, words[0]);
                doccr();
                break;
        }
    }

    fclose(fBuf);

	free(line);

	if (!*cfg->credit_name) {
		strcpy(cfg->credit_name, ATOM1095);
    }
	if (!*cfg->sysop) {
		strcpy(cfg->sysop, ATOM1096);
    }
	for (i = 0, valid = TRUE; i <= K_NWORDS; i++) {
        if (!found[i]) {
            cPrintf(ATOM1097,keywords[i]);
            valid = FALSE;
        }
    }

	drv_diff = getdisk() - ((char) toupper(cfg->homepath[0])-'A');

	if (!valid)
        illegal("");

	/* check directories */
	/* checkDir(cfg->homepath); */
	/* checkDir(cfg->msgpath); */
	/* checkDir(cfg->helppath); */
	/* checkDir(cfg->temppath); */
	/* checkDir(cfg->roompath); */
	/* checkDir(cfg->trapfile); */
	/* checkDir(cfg->aplpath); */

	/* allocate memory for tables */
	if(cfg_ctdl)
		allocateTables();
}

/************************************************************************/
/*      RoomTabBld() -- build RAM index to ROOM.DAT, displays stats.    */
/************************************************************************/
void RoomTabBld(void)
{
	uchar slot, roomCount = 0;
	uchar x, incr, last_incr;


    doccr();
    doccr();
    cPrintf(ATOM1098);
    doccr();

	for(x=0;x<50;x++) cPrintf("%c",177);
	cPrintf("\r");

	incr = last_incr = 0;
    for (slot = 0; slot < MAXROOMS; slot++) {
        getRoom(slot);

		incr = (50 * slot) / MAXROOMS;
        if(incr != last_incr) {
			for(x=0;x < incr-last_incr; x++) cPrintf("%c",219);
        }
        last_incr = incr;

        if (roomBuf.rbflags.INUSE)
            ++roomCount;

        noteRoom();
        putRoom(slot);
    }
    doccr();
    cPrintf(ATOM1100, roomCount, MAXROOMS);
    doccr();

}

/************************************************************************/
/*      slidemsgTab() frees >howmany< slots at the beginning of the     */
/*      message table.                                                  */
/************************************************************************/
void slidemsgTab(int howmany)
{
    hmemcpy(&msgTab[howmany], &msgTab[0], (long)
	((long) ((long) cfg->nmessages - (long) howmany) * (long) (sizeof(*msgTab)))
    );
}

/************************************************************************/
/*      zapGrpFile(), erase & reinitialize group file                   */
/************************************************************************/
void zapGrpFile(void)
{
    doccr();
    cPrintf(ATOM1107);
    doccr();

	setmem(grpBuf, sizeof (struct groupBuffer), 0);

	strcpy(grpBuf->group[0].groupname, ATOM1108);
	grpBuf->group[0].g_inuse = 1;
	grpBuf->group[0].groupgen = 1;	 /* Group Null's gen# is one      */
    /* everyone's a member at log-in */

	strcpy(grpBuf->group[1].groupname, ATOM1109);
	grpBuf->group[1].g_inuse = 1;
	grpBuf->group[1].groupgen = 1;

    putGroup();
}

/************************************************************************/
/*      zapHallFile(), erase & reinitialize hall file                   */
/************************************************************************/
void zapHallFile(void)
{
    doccr();
    cPrintf(ATOM1110);
    doccr();

    strcpy(hallBuf->hall[0].hallname, ATOM1111);
    hallBuf->hall[0].owned = 0; /* Hall is not owned     */

    hallBuf->hall[0].h_inuse = 1;
    hallBuf->hall[0].hroomflags[0].inhall = 1;  /* Lobby> in Root        */
    hallBuf->hall[0].hroomflags[1].inhall = 1;  /* Mail>  in Root        */
    hallBuf->hall[0].hroomflags[2].inhall = 1;  /* Aide)  in Root        */

    strcpy(hallBuf->hall[1].hallname, ATOM1112);
    hallBuf->hall[1].owned = 0; /* Hall is not owned     */

    hallBuf->hall[1].h_inuse = 1;
    hallBuf->hall[1].hroomflags[0].inhall = 1;  /* Lobby> in Maintenance */
    hallBuf->hall[1].hroomflags[1].inhall = 1;  /* Mail>  in Maintenance */
    hallBuf->hall[1].hroomflags[2].inhall = 1;  /* Aide)  in Maintenance */


    hallBuf->hall[0].hroomflags[2].window = 1;  /* Aide) is the window   */
    hallBuf->hall[1].hroomflags[2].window = 1;  /* Aide) is the window   */

    putHall();
}

/************************************************************************/
/*		zapHallgrpFile(), erase & reinitialize hallgrp file 			*/
/************************************************************************/
void zapHallgrpFile(void)
{
    doccr();
	cPrintf(ATOM1149);
    doccr();

	setmem(hallgrp, sizeof (struct hallGroup), 0);

	putHallgrp();
}

/************************************************************************/
/*		zapRoomgrpFile(), erase & reinitialize roomgrp file 			*/
/************************************************************************/
void zapRoomgrpFile(void)
{
    doccr();
	cPrintf(ATOM1150);
    doccr();

	setmem(roomgrp, sizeof (struct roomGroup), 0);

	putRoomgrp();
}

/************************************************************************/
/*      zapLogFile() erases & re-initializes userlog.buf                */
/************************************************************************/
int zapLogFile(void)
{
	int i;

    /* clear RAM buffer out:                    */
    logBuf.lbflags.L_INUSE = FALSE;
    logBuf.lbflags.NOACCOUNT = FALSE;
    logBuf.lbflags.AIDE = FALSE;
    logBuf.lbflags.NETUSER = FALSE;
    logBuf.lbflags.NODE = FALSE;
    logBuf.lbflags.PERMANENT = FALSE;
    logBuf.lbflags.PROBLEM = FALSE;
    logBuf.lbflags.SYSOP = FALSE;
    logBuf.lbflags.ROOMTELL = FALSE;
    logBuf.lbflags.NOMAIL = FALSE;
    logBuf.lbflags.UNLISTED = FALSE;

	logBuf.callno = 0L;

    for (i = 0; i < NAMESIZE; i++) {
        logBuf.lbname[i] = 0;
        logBuf.lbin[i] = 0;
        logBuf.lbpw[i] = 0;
    }
    doccr();
    doccr();
	cPrintf(ATOM1113, cfg->MAXLOGTAB);
    doccr();

    /* write empty buffer all over file;        */
	for (i = 0; i < cfg->MAXLOGTAB; i++) {
        cPrintf(ATOM1114, i);
        putLog(&logBuf, i);
        logTab[i].ltcallno = logBuf.callno;
        logTab[i].ltlogSlot = i;
        logTab[i].ltnmhash = hash(logBuf.lbname);
        logTab[i].ltinhash = hash(logBuf.lbin);
        logTab[i].ltpwhash = hash(logBuf.lbpw);
    }
    doccr();
    return TRUE;
}

/************************************************************************/
/*      zapMsgFl() initializes message.buf                              */
/************************************************************************/
int zapMsgFile(void)
{
	uchar i;
    unsigned sect;
    unsigned char sectBuf[128];

	/* Null the first one out for Locksley */
    for (i = 0; i < 128; i++)
        sectBuf[i] = 0;

    /* put null message in first sector... */
    sectBuf[0] = 0xFF;      /* */
    sectBuf[1] = DUMP;      /* \  To the dump                   */
    sectBuf[2] = '\0';      /* /  Attribute                     */
    sectBuf[3] = '1';       /* >                                */
    sectBuf[4] = '\0';      /* \  Message ID "1" MS-DOS style   */
    sectBuf[5] = 'M';       /* /         Null messsage          */
    sectBuf[6] = '\0';      /* */

	cfg->newest = cfg->oldest = 1l;

	cfg->catLoc = 7l;

    if (fwrite(sectBuf, 128, 1, msgfl) != 1) {
        cPrintf(ATOM1115);
        doccr();
    }
    for (i = 0; i < 128; i++)
        sectBuf[i] = 0;

    doccr();
    doccr();
	cPrintf(ATOM1116, cfg->messagek);
    doccr();
	for (sect = 1; sect < (cfg->messagek * 8); sect++) {
        cPrintf(ATOM1117, sect);
        if (fwrite(sectBuf, 128, 1, msgfl) != 1) {
            cPrintf(ATOM1118);
            doccr();
        }
    }
    return TRUE;
}

/************************************************************************/
/*      zapRoomFile() erases and re-initailizes ROOM.DAT                */
/************************************************************************/
int zapRoomFile(void)
{
	uchar i;

    roomBuf.rbflags.INUSE = FALSE;
    roomBuf.rbflags.PUBLIC = FALSE;
    roomBuf.rbflags.MSDOSDIR = FALSE;
    roomBuf.rbflags.PERMROOM = FALSE;
    roomBuf.rbflags.GROUPONLY = FALSE;
    roomBuf.rbflags.READONLY = FALSE;
    roomBuf.rbflags.SHARED = FALSE;
    roomBuf.rbflags.MODERATED = FALSE;
    roomBuf.rbflags.DOWNONLY = FALSE;

    roomBuf.rbgen = 0;
    roomBuf.rbdirname[0] = 0;
    roomBuf.rbname[0] = 0;
    roomBuf.rbroomtell[0] = 0;
    roomBuf.rbgrpgen = 0;
    roomBuf.rbgrpno = 0;

    doccr();
    doccr();
    cPrintf(ATOM1119, MAXROOMS);
    doccr();

    for (i = 0; i < MAXROOMS; i++) {
        cPrintf(ATOM1120, i);
        putRoom(i);
        noteRoom();
    }

    /* Lobby> always exists -- guarantees us a place to stand! */
    thisRoom = 0;
    strcpy(roomBuf.rbname, ATOM1121);
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC = TRUE;
    roomBuf.rbflags.INUSE = TRUE;

    putRoom(LOBBY);
    noteRoom();

    /* Mail> is also permanent...       */
    thisRoom = MAILROOM;
    strcpy(roomBuf.rbname, ATOM1122);
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC = TRUE;
    roomBuf.rbflags.INUSE = TRUE;

    putRoom(MAILROOM);
    noteRoom();

    /* Aide) also...                    */
    thisRoom = AIDEROOM;
    strcpy(roomBuf.rbname, ATOM1123);
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC = FALSE;
    roomBuf.rbflags.INUSE = TRUE;

    putRoom(AIDEROOM);
    noteRoom();

    /* Dump> also...                    */
    thisRoom = DUMP;
    strcpy(roomBuf.rbname, ATOM1124);
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC = TRUE;
    roomBuf.rbflags.INUSE = TRUE;

    putRoom(DUMP);
    noteRoom();

    return TRUE;
}
/* Find a string within another string	   */
/* and report it's position                */

int index(char *str, char *sub,int occur)
{
	int length, sublen, i1=0, i2=0, pos=0, ok=0, occur_hold=1;

	length = strlen(str);
	sublen = strlen(sub);

	for(i1=0;i1<length;i1++) {
		if(str[i1]==sub[i2]) {
			ok = 1;
         pos = i1-i2;
			i2++;
		}
		else {
			ok = 0;
			i2 = 0;
		}
		if(i2==sublen) {
			if(ok) {
				if(occur>occur_hold) {
					occur_hold++;
					i2=0;
					ok=0;
				}
				else {
					if(ok) break;
				}
			}
		}
   }
	if(i2==sublen) {
		pos++;   /* Needed for PICK syntax understanding  */
		return(pos);
	}
	else return(0);
}


/* EOF */
