/************************************************************************/
/*                              aplic.c                                 */
/*                    Application code for Citadel                      */
/************************************************************************/

#include <string.h>

#include "ctdl.h"

#ifndef ATARI_ST
#include <dos.h>
#include <alloc.h>
#include <process.h>
#endif

#include "proto.h"
#include "global.h"
#include "swap.h"
#include "apstruct.h"

#pragma warn -ucp

/************************************************************************
 *                              Contents
 *
 *      aplreadmess()           read message in from application
 *
 *      apsystem()              turns off interupts and makes
 *                              a system call
 *
 *      ExeAplic()              gets name of aplication and executes
 *
 *      shellescape()           handles the sysop '!' shell command
 *
 *      tableIn()               allocates RAM and reads log and msg
 *                              and tab files into RAM
 *
 *      tableOut()              writes msg and log tab files to disk
 *                              and frees RAM
 *
 *      writeDoorFiles()        write door interface files
 ************************************************************************/


/* --------------------------------------------------------------------
 *  History:
 *
 *  03/08/90    {zm}    Add 'softname' as the name of the software.
 *  03/17/90    FJM     added checks for farfree()
 *  03/19/90    FJM     Linted & partial cleanup
 *  03/27/90    FJM     Removed debug check for int 0x22
 *  07/08/90    FJM     Removed putenv() from shellescape() to save shell
 *                      memory.
 *  09/14/90    FJM     Fixed bug where amPrintf() could double close the
 *                      file.
 *  11/04/90    FJM     Added extremly large supershells.
 *	01/06/91	FJM 	Seperated out operations into readDoorFiles()
 *	01/13/91	FJM 	Name overflow fixes.
 *	01/18/91	FJM 	Added #SWAPFILE
 *	02/05/91	FJM 	Reversed bells and chat in the output.apl
 *	02/10/91	FJM 	Reveresed sense of chat & bells in output.apl again.
 *	04/10/91	FJM 	Prevent posting 0 length MESSAGE.APL files.
 *	04/17/91	FJM 	Changes to farheapcheck() message.
 *	05/06/91	FJM 	Removed old prompt code.
 *	06/18/91	BLJ 	MESSAGE.APL must be more than 10 characters!
 *	07/03/91	BLJ 	Remove sound code for super shell
 *	07/17/91	BLJ 	New routine! getBellStatus()
 *
 * -------------------------------------------------------------------- */

/************************************************************************/
/*              External declarations in APLIC.C                        */
/************************************************************************/

#ifdef OLD_SWAP
static int tableOut(void);
static void tableIn(void);

#endif
static int write_rbbs(char *fname, char *sysop1, char *city, char *state);
static int write_rbbs_name(FILE * fp, char *name);
static int aplreadmess(void);
static void readuserin(void);
static void writeDoorFiles(void);
static void writeAplFile(void);
static void readAplFile(void);
void getBellStatus(void);

#define     NUM_SAVE    15

static int saveem[] = {
	0x00, 0x03, 0x22, 0x34,
    0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F
};

static void (interrupt FAR * int_save[NUM_SAVE]) (),
(interrupt FAR * int_temp[NUM_SAVE]) ();

/************************************************************************/
/*              External variable definitions for APLIC.C               */
/************************************************************************/
extern label default_route;

/************************************************************************/
/*     aplreadmess()  read message in from application                  */
/************************************************************************/
static int aplreadmess(void)
{
    FILE *fd;
    int roomno, in = FALSE;

    if (filexists("msginfo.apl")) {
        in = TRUE;
        if ((fd = fopen("msginfo.apl", "rb")) == NULL)
            return ERROR;
        if (fread(&mesginfo, sizeof(mesginfo), 1, fd) != 1) {
            fclose(fd);
            return ERROR;
        }
        fclose(fd);
        if ((roomno = roomExists(mesginfo.roomName)) == ERROR) {
            cPrintf(" AP: No room \"%s\"! \n", mesginfo.roomName);
            return ERROR;
        }
    }
    if ((fd = fopen("message.apl", "rb")) == NULL)
        return ERROR;
    clearmsgbuf();
	GetStr(fd, msgBuf->mbtext, cfg->maxtext);
    fclose(fd);

    if (in && mesginfo.author[0])
        strcpy(msgBuf->mbauth, mesginfo.author);
    else
		strcpy(msgBuf->mbauth, cfg->nodeTitle);

    msgBuf->mbroomno = (uchar) (in ? roomno : thisRoom);

	/* must include at least 10 characters! */
	if (strlen(msgBuf->mbtext) > 9) {	
		putMessage();
		noteMessage();
	}

    return TRUE;
}

/************************************************************************/
/*      ExeAplic() gets the name of an aplication and executes it.      */
/************************************************************************/
void ExeAplic(void)
{
	char pw[4];

	doCR();
    doCR();

    if (!roomBuf.rbflags.APLIC) {
        mPrintf("  -- Room has no application.\n\n");
		changedir(cfg->homepath);
	}
	else {
		if(roomBuf.lockapp) {
			if(!logBuf.lbflags.SYSOP) {
				getString("password",pw, 4, FALSE, ECHO, "");
				if (strcmp(pw,roomBuf.password) != SAMESTRING) {
					mPrintf("Sorry, Invalid password!");
					doCR();
					mPrintf(" ");
					return;
				}
				doCR();
				mPrintf(" ");

				if(!roomBuf.lockpass) {
					if(getYesNo("Remove password",0)) roomBuf.password[0] = '\0';
					doCR();
					mPrintf(" ");
				}
			}
			else {
				mPrintf("Password: %s",roomBuf.password);
				doCR();
				mPrintf(" ");
			}
		}
		roomFmtRun(roomBuf.rbaplic);
	}
	return;
}

/************************************************************************
 *      roomFmtRun - run a command in normal room command format
 *
 *      command arguments:
 *
 *              %0      command name text
 *              %1      port: LOCAL|COM1|COM2|COM3|COM4
 *              %2      baud: 300|1200|2400|4800|9600|19200
 *              %3      sysop name
 *              %4      user name
 *
 *      See apsystem() for allowed command prefixes.
 *
 ************************************************************************/

void roomFmtRun(char *cmd)
{
    char comm[5];
    char stuff[100];

	if (changedir(cfg->aplpath) == ERROR) {
        mPrintf("  -- Can't find application directory.\n\n");
    } else {
		sprintf(comm, "COM%d", onConsole ? 0 : cfg->mdata);
        sprintf(stuff, "%s %s %d %d %s",
        cmd,
        onConsole ? "LOCAL" : comm,
        onConsole ? 2400 : bauds[speed],
        sysop,
        logBuf.lbname);

        apsystem(stuff);
    }
	changedir(cfg->homepath);
    return;
}

/************************************************************************
 *
 *      extFmtRun - run a command in normal external command format
 *                                      used by editText()
 *
 *      command parameter substitutions:
 *
 *              %f  file name.
 *              %p  port: 0-4   (0 is local)
 *              %b  baud: 300|1200|2400|4800|9600|19200
 *              %a  application file path.
 *              %n  user login name (may be more then one word).
 *              %s  sysop flag (1 if sysop, 0 if not)
 *
 *      See apsystem() for allowed command prefixes.
 *
 ************************************************************************/

void extFmtRun(char *cmd, char *file)
{
    char comm[5];
    char stuff[100];
    label dspeed;

	if (changedir(cfg->aplpath) == ERROR) {
        mPrintf("  -- Can't find application directory.\n\n");
    } else {
		sprintf(comm, "%d", onConsole ? 0 : cfg->mdata);
        sprintf(dspeed, "%d", bauds[speed]);
        sformat(stuff, cmd, "fpbans", file, comm, dspeed,
		cfg->aplpath, logBuf.lbname, sysop ? "1" : "0");
        apsystem(stuff);
        if (debug)
            cPrintf("(%s)", stuff);
		changedir(cfg->homepath);
    }
    return;
}

/************************************************************************/
/*      shellescape()  handles the sysop '!' shell command              */
/************************************************************************/
void shellescape(char super)
{
    char command[80];

	changedir(roomBuf.rbflags.MSDOSDIR ? roomBuf.rbdirname : cfg->homepath);

    sprintf(command, "%s%s", super ? "!" : "", getenv("COMSPEC"));

    apsystem(command);

	do_idle(0);

	changedir(cfg->homepath);
}


/************************************************************************
 *
 * readDoorFiles() - read in application door interface files
 *
 ************************************************************************/

void readDoorFiles(int door)
{
    char pathbuf[80];
    char path[64];
	
    getcwd(path, 64);

	changedir(cfg->aplpath);
    if (filexists("readme.apl")) {
        dumpf("readme.apl");
		unlink("readme.apl");
        doCR();
    }
    if (filexists("input.apl"))
        readAplFile();

    if (filexists("message.apl") && readMessage)
        aplreadmess();

    /* delete unneeded files */
    if (door) {
        unlink("output.apl");
        unlink("input.apl");
    }
	if (readMessage) {
        unlink("message.apl");
        unlink("msginfo.apl");
	}
    if (door) {
		sprintf(pathbuf,"DORINFO%d.DEF",onConsole ? 0 : cfg->mdata);
		unlink(pathbuf);
    }
    readMessage = TRUE;

	flipFileLan(path,0); /* This may look like an error, but it's needed! */

	changedir(path);
}

/************************************************************************
 * apsystem() turns off interupts and makes a system call
 *
 * Creates files:
 *
 *    DORINFO?.DEF    QBBS/RBBS door info file (in aplic dir).
 *
 *
 * Allowed command prefixes:
 *
 *         @       Don't save & clear screen
 *         $       Use DOS COMMAND.COM to run command (allows path, batch files)
 *         !       Use supershell (all but about 2.5k free!)
 *         ?       Don't write dorinfo?.def, userdato.apl
 *
 ************************************************************************/
void apsystem(cmd)
char *cmd;
{
    int clearit = TRUE, superit = FALSE, batch = FALSE, door = TRUE;
    char pathbuf[80];
    char cmdline[128];
    static char *words[256];
    int count, i, i2, AideFlOpen;
    int rc = 0;
    unsigned char exe_ret = 0;

    while (*cmd == '!' || *cmd == '@' || *cmd == '$' || *cmd == '?') {
        if (*cmd == '!')
            superit = TRUE;
        else if (*cmd == '@')
            clearit = FALSE;
        else if (*cmd == '$')
            batch = TRUE;
        else if (*cmd == '?')
            door = FALSE;
        cmd++;
    }
	if (farcoreleft() < cfg->MEMFREE) superit = TRUE;
    if (disabled)
        drop_dtr();

    portExit();

    /* close all files */
    if (aideFl != NULL) {
		flipFileLan(cfg->temppath,1);
		sprintf(pathbuf, "%s\\%s", cfg->temppath, "aidemsg.tmp");
		flipFileLan(cfg->temppath,0);

		AideFlOpen = TRUE;
    } else
        AideFlOpen = FALSE;

    fcloseall();

    if (AideFlOpen && (aideFl = fopen(pathbuf, "a")) == NULL) {
        crashout("Can not open AIDEMSG.TMP!");
    }
    if (clearit) {
        save_screen();
        cls();
    }
    if (debug)
        cPrintf("(%s)\n", cmd);

	if (stricmp(cmd, getenv("COMSPEC")) == SAMESTRING) {
        cPrintf("Use the EXIT command to return to %s\n", softname);
		if(debug) door = TRUE;
	}

	if (door)
        writeDoorFiles();   /* write door interface files */

    if (!batch) {
        count = parse_it(words, cmd);
        words[count] = NULL;
    }
    /*
     * Save the Floating point emulator interupts & the overlay interupt.
     */
    for (i = 0; i < NUM_SAVE; i++) {
        int_save[i] = getvect(saveem[i]);
    }

	restore_sound();

    if (superit) {
		if (batch) {
            sprintf(cmdline, "/C %s", cmd);
			flipFileLan(cfg->swapfile,1);
			rc = swap(getenv("COMSPEC"), cmdline, &exe_ret, cfg->swapfile);
			flipFileLan(cfg->swapfile,0);

		} else {
			/* rebuild command line */
            if (words[1]) {
                strcpy(cmdline, words[1]);
                for (i = 2; words[i]; ++i) {
                    strcat(cmdline, " ");
                    strcat(cmdline, words[i]);
                }
            } else {
                *cmdline = '\0';
            }

        /* Execute program, DOS search order is: .COM, .EXE, .BAT */

			/* no changes, extension fully specified */
            if (strchr(words[0], '.')) {
                sprintf(pathbuf, "%s", words[0]);

				flipFileLan(cfg->swapfile,1);
				rc = swap(pathbuf, cmdline, &exe_ret, cfg->swapfile);
				flipFileLan(cfg->swapfile,0);

			} else {
				/* try as a .COM */
                sprintf(pathbuf, "%s.COM", words[0]);

				flipFileLan(cfg->swapfile,1);
				rc = swap(pathbuf, cmdline, &exe_ret, cfg->swapfile);
				flipFileLan(cfg->swapfile,0);


				/* try as a .EXE if failed */
                if (rc && (exe_ret == FILE_NOT_FOUND)) {
                    exe_ret = 0;
                    sprintf(pathbuf, "%s.EXE", words[0]);

					flipFileLan(cfg->swapfile,1);
					rc = swap(pathbuf, cmdline, &exe_ret, cfg->swapfile);
					flipFileLan(cfg->swapfile,0);

				}
            }
        }
    } else {
        if (batch) {
            exe_ret = system(cmd);
        } else {
            exe_ret = spawnv(P_WAIT, words[0], words);
        }
    }

    /*
     * Load interupts for checking.
     */
    for (i = 0; i < NUM_SAVE; i++) {
        int_temp[i] = getvect(saveem[i]);
    }

    /*
     * Restore interupts.
     */
    for (i = 0; i < NUM_SAVE; i++) {
        setvect(saveem[i], int_save[i]);
    }

	do_idle(2);					/* clear & reset screen saver */
    if (clearit) {
        restore_screen();
	}

    /* test for execution failures */
    switch (rc) {
        case SWAP_NO_SHRINK:
            cPrintf("Error - Can not shrink DOS memory block\n");
            break;
        case SWAP_NO_SAVE:
            cPrintf("Error - Can not save Citadel to memory or disk\n");
            break;
        case SWAP_NO_EXEC:
            cPrintf("Error - Can not execute application '%s'\n", cmd);
        switch (exe_ret) {
            case BAD_FUNC:
                cPrintf("Bad function.\n");
                break;
            case FILE_NOT_FOUND:
                cPrintf("Program file not found.\n");
                break;
            case ACCESS_DENIED:
                cPrintf("Access to program file denied.\n");
                break;
            case NO_MEMORY:
                cPrintf("Insufficient memory to run program.\n");
                break;
            case BAD_ENVIRON:
                cPrintf("Bad environment.\n");
                break;
            case BAD_FORMAT:
                cPrintf("Bad format.\n");
                break;
        }
            break;
    }

    /* test saved interrupts */
    for (i = 0, i2 = 0; i < NUM_SAVE; i++) {
        if ((int_save[i] != int_temp[i]) && debug && (i != 0x22)) {
            if (!i2) {
                cPrintf(" '%s' changed interupt(s): ", cmd);
                doccr();
            }
            cPrintf("    0x%2X to %p from %p",
            saveem[i], int_temp[i], int_save[i]);
            doccr();

            i2++;
        }
    }

    portInit();
    baud((int) speed);

	flipFileLan(cfg->trapfile,1);
	trapfl = fopen(cfg->trapfile, "a+");
	flipFileLan(cfg->trapfile,0);

	flipFileLan(cfg->msgpath,1);
	sprintf(pathbuf, "%s\\%s", cfg->msgpath, "msg.dat");
	flipFileLan(cfg->msgpath,0);

	openFile(pathbuf, &msgfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "grp.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &grpfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "hall.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &hallfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "hallgrp.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &hallgrpfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "roomgrp.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &roomgrpfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "log.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &logfl);

	flipFileLan(cfg->homepath,1);
	sprintf(pathbuf, "%s\\%s", cfg->homepath, "room.dat");
	flipFileLan(cfg->homepath,0);

	openFile(pathbuf, &roomfl);

    if (disabled)
        drop_dtr();
    /* update25();	*/
	do_idle(0);

	readDoorFiles(door);

	init_sound();	/* We MUST have the sound system back!!!  */
}

/************************************************************************
 *      writeDoorFiles() - write door interface files
 ************************************************************************/

void writeDoorFiles(void)
{
    FILE *fd;
    char fname[FILESIZE];
	char path[64], pathbuf[80];
	
    getcwd(path, 64);
	changedir(cfg->aplpath);

	sprintf(pathbuf,"DORINFO%d.DEF",onConsole ? 0 : cfg->mdata);
	unlink(pathbuf);
    if (readMessage)
        unlink("message.apl");

	unlink("readme.apl");

	sprintf(fname, "DORINFO%d.DEF", onConsole ? 0 : cfg->mdata);
	write_rbbs(fname, cfg->sysop, "Anytown", "USA");
	writeAplFile();

	flipFileLan(path,0); /* This may look like an error, but it's needed! */
	changedir(path);
    return;
}

/*
 * FUNCTION NAME: write_rbbs_name
 *
 * DESCRIPTION: <c> Write user name to RBBS application (door) information file
 *                                      (2 lines)
 *
 * LOCAL VARIABLES USED:
 *
 * GLOBAL VARIABLES USED:
 *
 * FUNCTIONS CALLED:
 *
 * MACROS USED:
 *
 * RETURNED VALUE: 0 on success
 *
 */

static int write_rbbs_name(FILE * fp, char *name)
{
    char first_name[80];
    char last_name[80];
    char *p;
    int len;

    if (name && *name) {
        p = strchr(name, ' ');
        if (p) {
        /* I don't really care that this isn't portable to a VAX :)  */
            len = p - name; /* not allowed in VROOM? */
            strncpy(first_name, name, len);
            first_name[len] = '\0';
        /* skip blanks */
            while (*p == ' ') {
                ++p;
            }
            strcpy(last_name, p);
        /* convert blanks to '_' */
            p = last_name;
            while (*p) {
                if (*p == ' ') {
                    *p = '_';
                }
                ++p;
            }
            fprintf(fp, "%s\r\n%s\r\n", first_name, last_name);
        } else {
            fprintf(fp, "%s\r\n;\r\n", name);
        }
    } else {
        fprintf(fp, ";\r\n;\r\n");
    }
    return 0;
}

/*
 * FUNCTION NAME: write_rbbs
 *
 * DESCRIPTION: <c> Write RBBS application (door) information file
 *
 * LOCAL VARIABLES USED:
 *
 * GLOBAL VARIABLES USED:
 *
 * FUNCTIONS CALLED:
 *
 * MACROS USED:
 *
 * RETURNED VALUE: 0 on success
 *
 */

static int write_rbbs(char *fname, char *sysop1, char *city, char *state)
{
    FILE *fp;
    int status = 0;
    const char *sfmt = "%s\r\n";
    const char *dfmt = "%d\r\n";

    fp = fopen(fname, "wb");
    if (!fp) {
        return 1;
    }
    /* system */
	fprintf(fp, sfmt, cfg->nodeTitle);
    /* sysop 1 */
    write_rbbs_name(fp, sysop1);
    /* port */
	fprintf(fp, "COM%d\r\n", cfg->mdata);
    /* com parameter, includes baud rate */
	fprintf(fp,"%d",bauds[speed]);
    fprintf(fp, sfmt, " BAUD,N,8,1");
    /* unknown - 0 */
    fprintf(fp, dfmt, 0);

	/* user first name & last name */
	write_rbbs_name(fp, logBuf.lbname);
    /* location */
    fprintf(fp, "%s, %s\r\n", city, state);
    /* ansi */
	fprintf(fp, dfmt, ansiOn);
    /* access */
	if (sysop) {
        fprintf(fp, dfmt, 30);
	} else if (aide) {
        fprintf(fp, dfmt, 20);
    } else {
        fprintf(fp, dfmt, 10);
    }

    /* max time on system */
	if (cfg->accounting && !logBuf.lbflags.NOACCOUNT) {
		fprintf(fp, dfmt, (int) logBuf.credits);
    } else {
        fprintf(fp, dfmt, 999);
    }
    return status ? status : fclose(fp);
}


/* -------------------------------------------------------------------- */
/*      writeAplFile()  writes output.apl to disk                       */
/* -------------------------------------------------------------------- */
static void writeAplFile(void)
{
    FILE *fd;
    char buff[80];
    int i;

    if ((fd = fopen("output.apl", "wb")) == NULL) {
        mPrintf("Can't make output.apl");
        return;
    }
    for (i = 0; AplTab[i].item != APL_END; i++) {
        switch (AplTab[i].type) {
            case TYP_STR:
                sprintf(buff, "%c%s\n", AplTab[i].item, AplTab[i].variable);
                break;

            case TYP_BOOL:
            case TYP_CHAR:
                sprintf(buff, "%c%d\n", AplTab[i].item,
                *((char *) AplTab[i].variable));
                break;

            case TYP_INT:
                sprintf(buff, "%c%d\n", AplTab[i].item,
                *((int *) AplTab[i].variable));
                break;

            case TYP_FLOAT:
                sprintf(buff, "%c%f\n", AplTab[i].item,
                *((float *) AplTab[i].variable));
                break;

            case TYP_LONG:
                sprintf(buff, "%c%ld\n", AplTab[i].item,
                *((long *) AplTab[i].variable));
                break;

            case TYP_OTHER:
            switch (AplTab[i].item) {

				case APL_VERSION:
					sprintf(buff,"%c%s Ver %s\n", AplTab[i].item,
						softname, version);
					break;

                case APL_MDATA:
                    if (onConsole) {
                        sprintf(buff, "%c0 (LOCAL)\n", AplTab[i].item);
                    } else {
						sprintf(buff, "%c%d\n", AplTab[i].item, cfg->mdata);
                    }
                    break;

				case APL_NODECOUNTRY:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->nodeCntry);
					break;

				case APL_NODE:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->nodeTitle);
					break;

				case APL_REGION:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->nodeRegion);
					break;

				case APL_TEMPPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->temppath);
					break;

				case APL_APPLPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->aplpath);
					break;

				case APL_HELPPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->helppath);
					break;

				case APL_HOMEPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->homepath);
					break;

				case APL_ROOMPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->roompath);
					break;

				case APL_MSGPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->msgpath);
					break;

				case APL_PRINTPATH:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->printer);
					break;

				case APL_TRAPFILE:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->trapfile);
					break;

				case APL_ATTR:
					sprintf(buff,"%c%d\n",AplTab[i].item, cfg->attr);
					break;

				case APL_WATTR:
					sprintf(buff,"%c%d\n",AplTab[i].item, cfg->wattr);
					break;

				case APL_CATTR:
					sprintf(buff,"%c%d\n",AplTab[i].item, cfg->cattr);
					break;

				case APL_UTTR:
					sprintf(buff,"%c%d\n",AplTab[i].item, cfg->uttr);
					break;

				case APL_BATTR:
					sprintf(buff,"%c%d\n",AplTab[i].item, cfg->battr);
					break;

				case APL_CONLOCK:
					sprintf(buff,"%c%s\n",AplTab[i].item, cfg->f6pass);
					break;

				case APL_HALL:
                    sprintf(buff, "%c%s\n", AplTab[i].item,
                    hallBuf->hall[thisHall].hallname);
                    break;

                case APL_ROOM:
                    sprintf(buff, "%c%s\n", AplTab[i].item, roomBuf.rbname);
                    break;

                case APL_ACCOUNTING:
					if (!logBuf.lbflags.NOACCOUNT && cfg->accounting) {
                        sprintf(buff, "%c1\n", AplTab[i].item);
                    } else {
                        sprintf(buff, "%c0\n", AplTab[i].item);
                    }
                    break;

                case APL_PERMANENT:
                    sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.PERMANENT);
                    break;

                case APL_VERIFIED:
                    sprintf(buff, "%c%d\n", AplTab[i].item,
                    lBuf.VERIFIED ? 0 : 1);
                    break;

                case APL_NETUSER:
                    sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NETUSER);
                    break;

                case APL_NOMAIL:
                    sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NOMAIL);
                    break;

                case APL_CHAT:
					sprintf(buff, "%c%d\n", AplTab[i].item, !cfg->noChat);
                    break;

                case APL_BELLS:
					sprintf(buff, "%c%d\n", AplTab[i].item, !cfg->noBells);
                    break;

                default:
                    buff[0] = 0;
                    break;
            }
                break;

            default:
                buff[0] = 0;
                break;
        }

        if (strlen(buff) > 1) {
            fputs(buff, fd);
        }
    }

    fprintf(fd, "%c\n", APL_END);

    fclose(fd);
    /* parse node country later */
}

/* -------------------------------------------------------------------- */
/*      readAplFile()  reads input.apl from disk                        */
/* -------------------------------------------------------------------- */
static void readAplFile(void)
{
    FILE *fd;
    int i;
    char buff[200];
    int item;
    int roomno;
    int found;
    int slot;
	char rnode[NAMESIZE + NAMESIZE + 1];
	char temp[NAMESIZE + NAMESIZE + 1];

    if (readMessage) {
        clearmsgbuf();
		strcpy(msgBuf->mbauth, cfg->nodeTitle);
        msgBuf->mbroomno = thisRoom;
    }
    if ((fd = fopen("input.apl", "rt")) != NULL) {
        do {
            item = fgetc(fd);
            if (feof(fd)) {
                break;
            }
            fgets(buff, 198, fd);
            buff[strlen(buff) - 1] = 0;

            found = FALSE;

            for (i = 0; AplTab[i].item != APL_END; i++) {
                if (AplTab[i].item == item && AplTab[i].keep) {
                    found = TRUE;

                    switch (AplTab[i].type) {
                        case TYP_STR:
                            strncpy((char *) AplTab[i].variable, buff, AplTab[i].length);
                            ((char *) AplTab[i].variable)[AplTab[i].length - 1] = 0;
                            break;

						case TYP_BOOL:
                        case TYP_CHAR:
                            *((char *) AplTab[i].variable) = (char) atoi(buff);
                            break;

                        case TYP_INT:
                            *((int *) AplTab[i].variable) = atoi(buff);
                            break;

                        case TYP_FLOAT:
                            *((float *) AplTab[i].variable) = atof(buff);
                            break;

                        case TYP_LONG:
                            *((long *) AplTab[i].variable) = atol(buff);
                            break;

						case TYP_GRP:
							switch (AplTab[i].item) {
								case GRP_ADD:
									slot = groupexists(buff);
									if (slot == ERROR)
										slot = partialgroup(buff);
									if(slot != ERROR) {
										logBuf.groups[slot] =
											grpBuf->group[slot].groupgen;
									}
									break;

								case GRP_DEL:
									slot = groupexists(buff);
									if (slot == ERROR)
										slot = partialgroup(buff);
									if(slot != ERROR) {
										logBuf.groups[slot] =
											grpBuf->group[slot].groupgen - 1;
									}
									break;

								default:
									mPrintf("Bad value %d \"%s\"", item, buff);
									doCR();
									break;
							}
							break;

						case TYP_OTHER:
                        switch (AplTab[i].item) {
							case APL_APPOFF:
								roomBuf.autoapp = FALSE;
								roomBuf.rbflags.APLIC = FALSE;
								roomTab[thisRoom].rtflags.APLIC = FALSE;
								putRoom(thisRoom);
								break;

							case APL_HALL:
                                if (stricmp(buff, hallBuf->hall[thisHall].hallname)
                                != SAMESTRING) {
                                    slot = hallexists(buff);
                                    if (slot != ERROR) {
										cPrintf("Hall change to: %s", buff);
                                        doCR();
                                        thisHall = (unsigned char) slot;
                                    } else {
                                        cPrintf("No such hall %s!\n", buff);
                                    }
                                }
                                break;

							case APL_NEWHALL:
                                if (stricmp(buff, hallBuf->hall[thisHall].hallname)
                                != SAMESTRING) {
                                    slot = hallexists(buff);
                                    if (slot != ERROR) {
										/* Fix windowing and hall on old hall first! */
										hallBuf->hall[thisHall].hroomflags[thisRoom].inhall = FALSE;
										/* Just in case! */
										hallBuf->hall[thisHall].hroomflags[thisRoom].window = FALSE;
										/* Fix new hall  */
                                        thisHall = (unsigned char) slot;
										hallBuf->hall[thisHall].hroomflags[thisRoom].inhall = TRUE;

                                    } else {
                                        cPrintf("No such hall %s!\n", buff);
                                    }
                                }
                                break;

                            case APL_ROOM:
                                if ((roomno = roomExists(buff)) != ERROR) {
                                    if (roomno != thisRoom) {
										cPrintf("Room change to: %s", buff);
                                        doCR();
                                        logBuf.lbroom[thisRoom].lbgen
                                        = roomBuf.rbgen;
                                        ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
                                        ug_new = talleyBuf.room[thisRoom].new;
                                        logBuf.lbroom[thisRoom].lvisit = 0;
                                        logBuf.lbroom[thisRoom].mail = 0;
                                /* zero new count in talleybuffer */
                                        talleyBuf.room[thisRoom].new = 0;

                                        getRoom(roomno);

                                        if ((logBuf.lbroom[thisRoom].lbgen)
                                        != roomBuf.rbgen) {
                                            logBuf.lbroom[thisRoom].lbgen
                                            = roomBuf.rbgen;
                                            logBuf.lbroom[thisRoom].lvisit
                                            = (MAXVISIT - 1);
                                        }
                                    }
                                } else {
                                    cPrintf("No such room: %s!\n", buff);
                                }
                                break;


                            case APL_PERMANENT:
                                lBuf.lbflags.PERMANENT = atoi(buff);
                                break;

                            case APL_VERIFIED:
                                lBuf.VERIFIED = !atoi(buff);
                                break;

                            case APL_NETUSER:
                                lBuf.lbflags.NETUSER = atoi(buff);
                                break;

                            case APL_NOMAIL:
                                lBuf.lbflags.NOMAIL = atoi(buff);
                                break;

                            case APL_CHAT:
								cfg->noChat = !atoi(buff);
                                break;

                            case APL_BELLS:
								cfg->noBells = !atoi(buff);
								if(cfg->noBells)
									cfg->Sound = FALSE;
								else
									cfg->Sound = TRUE;
                                break;

                            default:
                                mPrintf("Bad value %d \"%s\"", item, buff);
                                doCR();
                                break;
                        }
                            break;

                        default:
                            break;
                    }
                }
            }

            if (!found && readMessage) {
                found = TRUE;

                switch (item) {
                    case MSG_NAME:
                        strcpy(msgBuf->mbauth, buff);
                        break;

                    case MSG_TO:
						rnode[0] = 0;
						if (strchr(buff, '@'))
							strcpy(rnode, strchr(buff, '@'));

						if (*rnode) {
							rnode[0] = ' ';

							normalizeString(rnode);

							if (strlen(rnode) > NAMESIZE) {
								mPrintf("\n No node %s.\n", rnode);
								break;
							}
							for (i = 0; i < NAMESIZE + NAMESIZE; i++)
								if (buff[i] == '@')
									buff[i] = '\0';

							normalizeString(buff);

							if (strlen(buff) > NAMESIZE) {
								mPrintf("\n User's name is too long!\n");
								break;
							}
							if (*rnode) {
								alias(rnode);
								strcpy(temp, rnode);
								route(temp);
								if (!getnode(temp)) {
									if (!getnode(default_route)) {
										mPrintf("Don't know how to reach '%s'", rnode);
										break;
									}
								}
							} else {
								mPrintf("No '%s' known", buff);
								break;
							}
						}
						if (*rnode) {
							strcpy(msgBuf->mbzip, rnode);
						}
                        strcpy(msgBuf->mbto, buff);

						break;

                    case MSG_GROUP:
                        strcpy(msgBuf->mbgroup, buff);
                        break;

                    case MSG_ROOM:
                        if ((roomno = roomExists(buff)) == ERROR) {
                            cPrintf(" AP: No room \"%s\"!\n", buff);
                        } else {
                            msgBuf->mbroomno = roomno;
                        }
                        break;

                    default:
                        doCR();
                        found = FALSE;
                        break;
                }
            }
            if (!found && AplTab[i].item != APL_END) {
                mPrintf("Bad value %d \"%s\"", item, buff);
                doCR();
            }
        }
        while (item != APL_END && !feof(fd));

#ifdef BAD
        unlink("input.apl");
#endif

        fclose(fd);
    }
    /* update25();	*/
	do_idle(0);

	if (readMessage) {
        if ((fd = fopen("message.apl", "rb")) != NULL) {
			GetStr(fd, msgBuf->mbtext, cfg->maxtext);
            fclose(fd);
			/* must include at least 10 characters! */
			if (strlen(msgBuf->mbtext) > 9) {
				putMessage();
				noteMessage();
				if(*msgBuf->mbzip)
					save_mail();
			}
        }
        unlink("message.apl");
    }
}

/* -------------------------------------------------------------------- */
/*	getBellStatus()  reads input.apl from disk only looking for bells	*/
/* -------------------------------------------------------------------- */
void getBellStatus(void)
{
    FILE *fd;
    int i;
    char buff[200];
    int item;

    if ((fd = fopen("input.apl", "rt")) != NULL) {
        do {
            item = fgetc(fd);
            if (feof(fd)) {
				fclose(fd);
				return;
            }
			fgets(buff, 198, fd);
            buff[strlen(buff) - 1] = 0;
		} while(item != APL_BELLS);

		cfg->noBells = !atoi(buff);
		if(cfg->noBells)
			cfg->Sound = FALSE;
		else
			cfg->Sound = TRUE;

        fclose(fd);
    }
}


/* EOF */
