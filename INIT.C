/* --------------------------------------------------------------------
 *  INIT.C                    Citadel
 * --------------------------------------------------------------------
 *  Routines to initialize or exit Citadel
 * --------------------------------------------------------------------
 *
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *                              Contents
 * --------------------------------------------------------------------
 *  crashout()      Fatal system error
 *  exitcitadel()   Done with cit, time to leave
 *  asciitable()    initializes the ascii translation table
 *  initCitadel()   Load up data files and open everything.
 *  readTables()    loads all tables into ram
 *  freeTables()	deallocate tables from memory
 *  writeTables()   writes all system tables to disk
 *  allocateTables()allocate table space
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  Includes
 * -------------------------------------------------------------------- */

#define INIT

#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* Cit */
#include "ctdl.h"

#ifndef ATARI_ST
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <alloc.h>
#endif

#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data definitions                                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  asciitable()    initializes the ascii translation table             */
/* -------------------------------------------------------------------- */
void asciitable(void)
{
#ifdef OLD_ASCII_TBL
    unsigned char c;

    /* initialize input character-translation table:  */

    /* control chars -> nulls     */
    for (c = 0; c < '\40'; c++)
        filter[c] = '\0';

    /* pass printing chars        */
    for (c = '\40'; c < FILT_LEN; c++)
        filter[c] = c;

	filter[1] = 1;			/* ctrl-a	 0x01 = ctrl-a	  0x01	 */
	filter[1] = 2;			/* ctrl-b	 0x02 = ctrl-b	  0x02	 */
	filter[27] = 27;		/* special	 0x1b = special   0x1b	 */
	filter[8] = 8;			/* backspace 0x08 = backspace 0x08	 */
    filter[0x7f] = 8;       /* del       0x7f = backspace 0x08   */
    filter[19] = 'P';       /* xoff      0x13 = 'P'       0x50   */
    filter['\r'] = '\n';    /* '\r'      0x0d = NEWLINE   0x0a   */
    filter['\t'] = '\t';    /* '\t'      0x09 = '\t'      0x09   */
    filter[10] = NULL;      /* newline   0x0a = null      0x00   */
    filter[15] = 'N';       /* ctrlo     0x0f = 'N'       0x4e   */
    filter[26] = 26;        /* ctrlz     0x1a = ctrlz     0x1a   */
#endif
}

/* -------------------------------------------------------------------- */
/*  initCitadel()   Load up data files and open everything.             */
/* -------------------------------------------------------------------- */
void initCitadel(void)
{
    /* FILE *fd, *fp; */

	char *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
	char *hallgrpFile, *roomgrpFile;
	char scratch[80];

    /* lets ignore ^C's  */
    signal(SIGINT, ctrl_c);
    randomize();        /* we will use random numbers */
    asciitable();

	/* moved all allocation to allocateTables (called from readTables()) */

	if (!readTables()) {
        if (msgTab) {
            freeTables();
        }
		if (!parm.door)
			delay(3000);
        cls();
        cCPrintf("Configuring Citadel - Please wait");
        doccr();
		configcit();
    }
	/* Only host system will do this  */
	if (!drv_diff) portInit();

    setscreen();

	/* update25();	*/
	do_idle(0);

	if (cfg->msgpath[(strlen(cfg->msgpath) - 1)] == '\\')
		cfg->msgpath[(strlen(cfg->msgpath) - 1)] = '\0';

	flipFileLan(cfg->msgpath,1);
	sprintf(scratch, "%s\\%s", cfg->msgpath, "msg.dat");

	flipFileLan(cfg->msgpath,0);

    /* open message files: */
    grpFile = "grp.dat";
    hallFile = "hall.dat";
    logFile = "log.dat";
    msgFile = scratch;
    roomFile = "room.dat";
	hallgrpFile = "hallgrp.dat";
	roomgrpFile = "roomgrp.dat";

    openFile(grpFile, &grpfl);
    openFile(hallFile, &hallfl);
    openFile(logFile, &logfl);
    openFile(msgFile, &msgfl);
    openFile(roomFile, &roomfl);
	openFile(hallgrpFile, &hallgrpfl);
	openFile(roomgrpFile, &roomgrpfl);

    /* open Trap file */
	flipFileLan(cfg->trapfile,1);

	trapfl = fopen(cfg->trapfile, "a+");

	flipFileLan(cfg->trapfile,0);


	/* Only host system will do this  */
    sprintf(msgBuf->mbtext, "%s Started", softname);
    trap(msgBuf->mbtext, T_SYSOP);

    getGroup();
    getHall();
	getHallgrp();
	getRoomgrp();

	if (cfg->accounting)
        readaccount();      /* read in accounting data */
    readprotocols();
	if(cfg->ChatTab)
		readchatbell();

	rules_read();

	getRoom(LOBBY); 	/* load Lobby>	*/
	/* Only host system will do this  */
	if(!drv_diff) {
		Initport();
		Initport();
	}
    whichIO = MODEM;

    /* record when we put system up */
    time(&uptimestamp);

    cls();
    setdefaultconfig();
    /* update25();	*/
	do_idle(0);
    setalloldrooms();
    roomtalley();
	
	init_sound();	/* Initialize the Sound System	*/

	ctdlConfig = FALSE;  /* NOW enable room passwords! */
}

#define MAXRW   3000

/* -------------------------------------------------------------------- */
/*  readMsgTable()     Avoid the 64K limit. (stupid segments)           */
/* -------------------------------------------------------------------- */
int readMsgTab(void)
{
    FILE *fd;
    int i;
    char HUGE *tmp;
	char temp[80];

    /* read message table */
	flipFileLan(cfg->homepath,1);

	sprintf(temp, "%s\\%s", cfg->homepath, "msg.tab");

	flipFileLan(cfg->homepath,0);

    tmp = (char HUGE *) msgTab;

    if ((fd = fopen(temp, "rb")) == NULL)
        return (FALSE);

	for (i = 0; i < cfg->nmessages / MAXRW; i++) {
        if (fread((void FAR *) (tmp), sizeof(struct messagetable), MAXRW, fd)
        != MAXRW) {
            fclose(fd);
            return FALSE;
        }
        tmp += (sizeof(struct messagetable) * MAXRW);
    }

	i = cfg->nmessages % MAXRW;
    if (i) {
        if (fread((void FAR *) (tmp), sizeof(struct messagetable), i, fd)
        != i) {
            fclose(fd);
            return FALSE;
        }
    }
    fclose(fd);
	if(!cfg->LAN)
		unlink(temp);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  writeMsgTable()     Avoid the 64K limit. (stupid segments)          */
/* -------------------------------------------------------------------- */
void writeMsgTab(void)
{
    FILE *fd;
    int i;
    char HUGE *tmp;
	char temp[80];

	flipFileLan(cfg->homepath,1);

	sprintf(temp, "%s\\%s", cfg->homepath, "msg.tab");

	flipFileLan(cfg->homepath,0);

	tmp = (char HUGE *) msgTab;

    if ((fd = fopen(temp, "wb")) == NULL)
        return;

	for (i = 0; i < cfg->nmessages / MAXRW; i++) {
        if (fwrite((void FAR *) (tmp), sizeof(struct messagetable), MAXRW, fd)
        != MAXRW) {
            fclose(fd);
        }
        tmp += (sizeof(struct messagetable) * MAXRW);
    }

	i = cfg->nmessages % MAXRW;
    if (i) {
        if (fwrite((void FAR *) (tmp), sizeof(struct messagetable), i, fd)
        != i) {
            fclose(fd);
        }
    }
    fclose(fd);
}

/************************************************************************/
/*      readTables()  loads all tables into ram                         */
/************************************************************************/
readTables()
{
    FILE *fd;

    getcwd(etcpath, 64);

	if ((fd = fopen("etc.tab", "rb")) == NULL)
        return (FALSE);

	if (!fread(cfg, sizeof(struct config), 1, fd)) {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);

	/* this forces a re-config by comparing the last time-date stamp... */
	if(strcmp(compdate, cfg->compdate) || strcmp(comptime, cfg->comptime)) {
		return FALSE;
	}

	if(!cfg->LAN)
		unlink("etc.tab");

	allocateTables();

	drv_diff = getdisk() - ((char) toupper(cfg->homepath[0])-'A');

	changedir(cfg->homepath);

    if (logTab == NULL)
        crashout("Error allocating logTab \n");
    if (msgTab == NULL)
        crashout("Error allocating msgTab \n");

    /*
     * LOG.TAB
     */
    if ((fd = fopen("log.tab", "rb")) == NULL)
        return (FALSE);
	if (!fread(logTab, sizeof(struct lTable), cfg->MAXLOGTAB, fd)) {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);

	if(!cfg->LAN)
		unlink("log.tab");

    /*
     * MSG.TAB
     */
    if (readMsgTab() == FALSE)
        return FALSE;

    /*
     * ROOM.TAB
     */
    if ((fd = fopen("room.tab", "rb")) == NULL)
        return (FALSE);
    if (!fread(roomTab, sizeof(struct rTable), MAXROOMS, fd)) {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);

	if(!cfg->LAN)
        unlink("room.tab");

	readCrontab();

	if(!cfg->LAN)
        unlink("cron.tab");

    return (TRUE);
}


/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/* -------------------------------------------------------------------- */
void crashout(char *message)
{
    FILE *fd;           /* Record some crash data */

    Hangup();

    fcloseall();

    fd = fopen("crash.cit", "w");
    fprintf(fd, message);
    fclose(fd);

	cfg->attr = 7;		 /* exit with white letters */

    position(0, 0);
	cls();
	cPrintf("F\na\nt\na\nl\n \nS\ny\ns\nt\ne\nm\n \nC\nr\na\ns\nh\n");
    cPrintf(" %s\n", message);

	/* Only host system will do this  */
	if(!drv_diff) drop_dtr();

	/* Only host system will do this  */
	if(!drv_diff) portExit();

	restore_sound();   /* Restore system Interrupt 8 and clock ticks */

	exit(9);
}

/* -------------------------------------------------------------------- */
/*  exitcitadel()   Done with cit, time to leave                        */
/* -------------------------------------------------------------------- */

extern void interrupt ( *oldhandler)(void);

void exitcitadel(void)
{
    if  (loggedIn)
        terminate( /* hangUp == */ TRUE, FALSE);

	/* Only host system will do this  */
    drop_dtr();         /* turn DTR off */

    putGroup();         /* save group table */
    putHall();          /* save hall table  */

    writeTables();


	/* Only host system will do this  */
	sprintf(msgBuf->mbtext, "%s Terminated", softname);
    trap(msgBuf->mbtext, T_SYSOP);

    /* close all files */
    fcloseall();

	cfg->attr = 7;		 /* exit with white letters */
    cls();

	/* Host Only */
	if(!drv_diff) {
		drop_dtr();
		portExit();
	}

    freeTables();

    if (gmode() != 7) {
        outp(0x3d9, 0);
    }
    setvect(24, oldhandler);
	
	restore_sound();   /* Restore system Interrupt 8 and clock ticks */

	exit(ExitToMsdos - 1);
}

/************************************************************************
 *              freeTables()            Free all allocated tables
 ************************************************************************/

void freeTables(void)
{
	struct event	   *save_event;
	struct accounting  *save_acct;
	struct ext_command *save_extCmd;
	struct ext_prot    *save_extrn;
	struct ext_editor  *save_edit;

	if	(msgTab)
        farfree((void FAR *) msgTab);
    msgTab = NULL;
    if (logTab)
        farfree(logTab);
    logTab = NULL;
    if (roomTab)
        farfree(roomTab);
    roomTab = NULL;
	if (start_ndgrp)
		farfree(start_ndgrp);
	ndgrp = NULL;
	start_ndgrp = NULL;

	if (start_ndroom)
		farfree(start_ndroom);
	start_ndroom = NULL;
	ndroom = NULL;

	if (start_extCmd) {
		save_extCmd = start_extCmd->next;
		while(save_extCmd) {
			extCmd = save_extCmd;
			save_extCmd = save_extCmd->next;
			farfree(extCmd);
		}
		farfree(start_extCmd);
	}
	start_extCmd = NULL;
	extCmd = NULL;

	if (start_extrn) {
		save_extrn = start_extrn->next;
		while(save_extrn) {
			extrn = save_extrn;
			save_extrn = save_extrn->next;
			farfree(extrn);
		}
		farfree(start_extrn);
	}
	start_extrn = NULL;
	extrn = NULL;

	if (start_edit) {
		save_edit = start_edit->next;
		while(save_edit) {
			edit = save_edit;
			save_edit = save_edit->next;
			farfree(edit);
		}
		farfree(start_edit);
	}
	start_edit = NULL;
	edit = NULL;

    if (start_event) {
		save_event = start_event->next;
		while(save_event) {
			events = save_event;
			save_event = save_event->next;
			farfree(events);
		}
		farfree(start_event);
	}
	start_event = NULL;
	events = NULL;

	if (start_acct) {
		save_acct = start_acct->next;
		while(save_acct) {
			accountBuf = save_acct;
			save_acct = save_acct->next;
			farfree(accountBuf);
		}
		farfree(start_acct);
	}
	start_acct = NULL;
    accountBuf = NULL;

	if (chat_bell)
		farfree(chat_bell);
	chat_bell = NULL;
    if (hallBuf)
        farfree(hallBuf);
	if (node)
		farfree(node);
	node = NULL;
    hallBuf = NULL;
    if (msgBuf)
        farfree(msgBuf);
    msgBuf = NULL;
    if (msgBuf2)
        farfree(msgBuf2);
    msgBuf2 = NULL;
	if (grpBuf)
		farfree(grpBuf);
	grpBuf = NULL;
	if (hallgrp)
		farfree(hallgrp);
	hallgrp = NULL;
	if (roomgrp)
		farfree(roomgrp);
	roomgrp = NULL;
	if (farheapcheck() < 0)
        crashout("memory corrupted 1");
}

/************************************************************************/
/*      writeTables()  stores all tables to disk                        */
/************************************************************************/
void writeTables(void)
{
    FILE *fd;

	changedir(cfg->homepath);

	if ((fd = fopen("etc.tab", "wb")) == NULL) {
		crashout("Can't make Etc.tab");
    }
	strcpy(cfg->compdate, compdate);
	strcpy(cfg->comptime, comptime);
	/* write out Etc.tab */
	fwrite(cfg, sizeof(struct config), 1, fd);
    fclose(fd);

    if ((fd = fopen("log.tab", "wb")) == NULL) {
        crashout("Can't make Log.tab");
    }
    /* write out Log.tab */
	fwrite(logTab, sizeof(struct lTable), cfg->MAXLOGTAB, fd);
    fclose(fd);

    writeMsgTab();

    if ((fd = fopen("room.tab", "wb")) == NULL) {
        crashout("Can't make Room.tab");
    }
    /* write out Room.tab */
    fwrite(roomTab, sizeof(struct rTable), MAXROOMS, fd);
    fclose(fd);

    writeCrontab();

}

/************************************************************************
 *    allocateTables()   allocate msgTab, logTab and allocated tables
 *						 see tableOut, tableIn in APPLIC.C
 ************************************************************************/

void allocateTables(void)
{
	if ((hallBuf = farcalloc(1UL, (ulong) sizeof(struct hallBuffer))) == NULL) {
        crashout("Can not allocate memory for halls");
    }
	if ((msgBuf = farcalloc(1UL, (ulong) sizeof(struct msgB))) == NULL) {
        crashout("Can not allocate memory for message buffer 1");
    }
	if ((msgBuf2 = farcalloc(1UL, (ulong) sizeof(struct msgB))) == NULL) {
        crashout("Can not allocate memory for message buffer 2");
    }
	if ((roomTab = farcalloc((ulong) MAXROOMS, (ulong) sizeof(struct rTable))) == NULL) {
		crashout("Can not allocate memory for Room Table");
    }
	logTab = farcalloc((ulong) (cfg->MAXLOGTAB + 1), (ulong) sizeof(struct lTable));
    if (!logTab) {
        crashout("Can not allocate memory for user log");
    }
	msgTab = farcalloc((ulong) (cfg->nmessages + 1), (ulong) sizeof(struct messagetable));
    if (!msgTab) {
        crashout("Can not allocate memory for message base");
    }

	if	((edit = farcalloc(1UL, (ulong) sizeof(struct ext_editor))) == NULL) {
        crashout("Can not allocate external editors");
    }
	edit->next = NULL;
	start_edit = edit;

	if ((extrn = farcalloc(1UL, (ulong) sizeof(struct ext_prot))) == NULL) {
        crashout("Can not allocate memory for external protocol");
    }
	extrn->next = NULL;
	start_extrn = extrn;

	if ((extCmd = farcalloc(1UL, (ulong) sizeof(struct ext_other))) == NULL) {
        crashout("Can not allocate memory for external commands");
    }
	extCmd->next = NULL;
	start_extCmd = extCmd;

	accountBuf = farcalloc(1UL, (ulong) sizeof(struct accounting));
    if (!accountBuf) {
        crashout("Can not allocate memory for user accounting");
    }
	accountBuf->next = NULL;
	start_acct = accountBuf;

	events = farcalloc(1UL,(ulong) sizeof(struct event));
	if (!events) {
		crashout("Can not allocate memory for cron events");
    }
	events->next = NULL;
	start_event = events;

	ndgrp = farcalloc(1UL,(ulong) sizeof(struct ndgroups));
	if (!ndgrp) {
		crashout("Can not allocate memory for node groups");
    }
	ndgrp->next = NULL;
	start_ndgrp = ndgrp;

	ndroom = farcalloc(1UL,(ulong) sizeof(struct ndrooms));
	if (!ndroom) {
		crashout("Can not allocate memory for node rooms");
    }
	ndroom->next = NULL;
	start_ndroom = ndroom;

	chat_bell = farcalloc(1UL,(ulong) sizeof(struct ChatBell));
	if (!chat_bell) {
		crashout("Can not allocate memory for Chat & Bell times");
    }
	node = farcalloc(1UL,(ulong) sizeof(struct nodest));
	if (!node) {
		crashout("Can not allocate memory for Node Structure");
    }

	grpBuf = farcalloc(1UL,(ulong) sizeof(struct groupBuffer));
	if (!grpBuf) {
		crashout("Can not allocate memory for groupBuffer Structure");
    }
	hallgrp = farcalloc(1UL,(ulong) sizeof(struct hallGroup));
	if (!hallgrp) {
		crashout("Can not allocate memory for hallGroup Structure");
    }
	roomgrp = farcalloc(1UL,(ulong) sizeof(struct roomGroup));
	if (!roomgrp) {
		crashout("Can not allocate memory for roomGroup Structure");
    }
}


