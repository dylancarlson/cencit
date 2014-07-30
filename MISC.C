/* -------------------------------------------------------------------- */
/*  MISC.C                    Citadel                                   */
/* -------------------------------------------------------------------- */
/*  Citadel garbage dump, if it aint elsewhere, its here.               */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *
 *  Early History:
 *
 *  02/08/89    (PAT)   History Re-Started
 *                      InitAideMess and SaveAideMess added
 *  05/26/89    (PAT)   Many of the functions move to other modules
 *  03/07/90    {zm}    Add 'softname' as the name of the software.
 *  03/17/90    FJM     added checks for farfree()
 *  04/01/90    FJM     added checks reconfigure on new version
 *  05/17/90    FJM     Added external commands
 *  06/06/90    FJM     Changed strftime to cit_strftime
 *  06/16/90    FJM     Changed reconfigure message.
 *	09/25/91	BLJ 	Added the strip_ansi function
 *	03/28/92	BLJ 	Added Code for Network Drive Differences to changedir
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  filexists()     Does the file exist?                                */
/*  hash()          Make an int out of their name                       */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/*	strip_ansi()	Returns a char buffer without CTRL-A codes			*/
/*  openFile()      Special to open a .cit file                         */
/*  trap()          Record a line to the trap file                      */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/*  h2memcpy()      Terible hack to copy over 64K, beacuse MSC cant. PT2*/
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/*  amPrintf()      aide message printf                                 */
/*  amZap()         Zap aide message being made                         */
/*  changedir()     changes curent drive and directory                  */
/*  changedisk()    change to another drive                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */

#define MISC

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

void hallinfo();

/* -------------------------------------------------------------------- */
/*  filexists()     Does the file exist?                                */
/* -------------------------------------------------------------------- */
BOOL filexists(char *filename)
{
	return (BOOL) ((access(filename, 4) == 0) ? TRUE : FALSE);
}

/* -------------------------------------------------------------------- */
/*	hallinfo()	   multiple group stuff for a hallway					*/
/* -------------------------------------------------------------------- */
void hallinfo()
{
	uchar i;

    mPrintf(" Hallway %s", hallBuf->hall[thisHall].hallname);

    if (hallBuf->hall[thisHall].owned) {
        mPrintf(", owned by group %s",
		grpBuf->group[hallBuf->hall[thisHall].grpno].groupname);
    }

	if (hallBuf->hall[thisHall].mult_grp) {
		for(i=0; i<MAXGROUPS; i++) {
			if(debug) {
				if( grpBuf->group[i].g_inuse) {
					cPrintf("grpBuf=%d hallgrp=%d   ",
						grpBuf->group[i].groupgen,
						hallgrp->hall[thisHall].grp[i].grpgen);
				}
			}
			if ( grpBuf->group[i].g_inuse &&
				(hallgrp->hall[thisHall].grp[i].grpgen ==
				grpBuf->group[i].groupgen))
			{
				mPrintf(", shared by group %s",
				grpBuf->group[i].groupname);
			}
		}
	}
	doCR();
}

/* -------------------------------------------------------------------- */
/*  hash()          Make an int out of their name                       */
/* -------------------------------------------------------------------- */
uint hash(char *str)
{
    int h, shift;

    for (h = shift = 0; *str; shift = (shift + 1) & 7, str++) {
        h ^= (toupper(*str)) << shift;
    }
    return h;
}

/* -------------------------------------------------------------------- */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/* -------------------------------------------------------------------- */
void ctrl_c(void)
{
    uchar row, col;

    signal(SIGINT, ctrl_c);
    readpos(&row, &col);
    position(row - 1, 19);
    ungetch('\r');
    getkey = TRUE;
}

/* -------------------------------------------------------------------- */
/*	strip_ansi()	Used to strip out the CTRL-A codes					*/
/* -------------------------------------------------------------------- */
char *strip_ansi(char *msg)
{
	int i, l, pos;
	static char buf[40];

	pos = 0;
	l = strlen(msg);
	if(l > 39) l = 39;

	for(i=0; i<l; i++) {
		if(msg[i] == 1)
			i++;
		else
			buf[pos++] = msg[i];
	}
	buf[pos] = '\0';

	return(buf);
}

/* -------------------------------------------------------------------- */
/*  openFile()      Special to open a .cit file                         */
/* -------------------------------------------------------------------- */
void openFile(char *filename, FILE ** fd)
{
	/* open message file */
	if	((*fd = fopen(filename, "r+b")) == NULL) {
        crashout(".DAT file missing!");
    }
}

/* -------------------------------------------------------------------- */
/*  trap()          Record a line to the trap file                      */
/* -------------------------------------------------------------------- */
void trap(char *string, int what)
{
    char dtstr[20];

    /* check to see if we are supposed to log this event */
	if (!cfg->trapit[what])
        return;

	cit_strftime(dtstr, 19, "%y%b%D %X", 0L);

    fprintf(trapfl, "%s:  %s\n", dtstr, string);

    fflush(trapfl);
}

/* -------------------------------------------------------------------- */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/* -------------------------------------------------------------------- */
#define K32  (32840L)
void hmemcpy(void HUGE * xto, void HUGE * xfrom, long size)
{
    char HUGE *from;
    char HUGE *to;

    to = xto;
    from = xfrom;

    if (to > from) {
        h2memcpy(to, from, size);
        return;
    }
    while (size > K32) {
        memcpy((char FAR *) to, (char FAR *) from, (unsigned int) K32);
        size -= K32;
        to += K32;
        from += K32;
    }

    if (size)
        memcpy((char FAR *) to, (char FAR *) from, (uint) size);
}

/* -------------------------------------------------------------------- */
/*  h2memcpy()     Terible hack to copy over 64K, beacuse MSC cant. PT2 */
/* -------------------------------------------------------------------- */
void h2memcpy(char HUGE * to, char HUGE * from, long size)
{
    to += size;
    from += size;

    size++;

    while (size--)
        *to-- = *from--;
}

/* -------------------------------------------------------------------- */
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/* -------------------------------------------------------------------- */
void SaveAideMess(void)
{
	char *temp;
    FILE *fd;

	if((temp = calloc(90, sizeof(char))) == NULL) {
		return;
	}

	flipFileLan(cfg->temppath,1);

	sprintf(temp, "%s\\%s", cfg->temppath, "aidemsg.tmp");

    if (aideFl == NULL) {
		flipFileLan(cfg->temppath,0);
		return;
    }
    fclose(aideFl);

    if ((fd = fopen(temp, "rb")) == NULL) {
        crashout("AIDEMSG.TMP file not found during aide message save!");
    }
    clearmsgbuf();

	GetStr(fd, msgBuf->mbtext, cfg->maxtext);

    fclose(fd);
    unlink(temp);

    aideFl = NULL;

	free(temp);

	flipFileLan(cfg->temppath,0);

	if (strlen(msgBuf->mbtext) < 10)
        return;

	strcpy(msgBuf->mbauth, cfg->nodeTitle);

    msgBuf->mbroomno = AIDEROOM;

    putMessage();
    noteMessage();
}

/* -------------------------------------------------------------------- */
/*  amPrintf()      aide message printf                                 */
/* -------------------------------------------------------------------- */
void amPrintf(char *fmt,...)
{
    va_list ap;
	char *temp;

	if((temp = calloc(90, sizeof(char))) == NULL) {
		return;
	}

	flipFileLan(cfg->temppath,1);

	/* no message in progress? */
    if (aideFl == NULL) {
		sprintf(temp, "%s\\%s", cfg->temppath, "aidemsg.tmp");

        unlink(temp);

        if ((aideFl = fopen(temp, "w")) == NULL) {
            crashout("Can not open AIDEMSG.TMP!");
        }
    }

	free(temp);

	flipFileLan(cfg->temppath,0);

	va_start(ap, fmt);
    vfprintf(aideFl, fmt, ap);
    va_end(ap);

}

/* -------------------------------------------------------------------- */
/*  amZap()         Zap aide message being made                         */
/* -------------------------------------------------------------------- */
void amZap(void)
{
	char *temp;

	if((temp = calloc(90, sizeof(char))) == NULL) {
		return;
	}

    if (aideFl != NULL) {
        fclose(aideFl);
    }

	flipFileLan(cfg->temppath,1);

	sprintf(temp, "%s\\%s", cfg->temppath, "aidemsg.tmp");

    unlink(temp);

	free(temp);

	flipFileLan(cfg->temppath,0);

	aideFl = NULL;
}

/* -------------------------------------------------------------------- */
/*	flipFileLan()	changes curent drive for the LAN					*/
/* -------------------------------------------------------------------- */
void flipFileLan(char *path, int Up)
{
	char save_drv;

	/* uppercase   */
	save_drv = (char) toupper(path[0]);
	if(Up) {
		path[0] = save_drv+drv_diff;
	} else {
		path[0] = save_drv-drv_diff;
	}
}

/* -------------------------------------------------------------------- */
/*  changedir()     changes curent drive and directory                  */
/* -------------------------------------------------------------------- */
int changedir(char *path)
{

	flipFileLan(path,1);

    /* change disk */
    changedisk(path[0]);

    /* change path */
    if (chdir(path) == -1) {
        mPrintf("\n Directory path invalid.");
		cPrintf("\n Invalid directory '%s' on drive '%c:'.\n", path, path[0]);
		amPrintf("\n Invalid directory '%s' on drive '%c:'.\n", path, path[0]);
        SaveAideMess();
		flipFileLan(path,0);
		return -1;
    }
	flipFileLan(path,0);
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  changedisk()    change to another drive                             */
/* -------------------------------------------------------------------- */
void changedisk(char disk)
{
    setdisk(disk - 'A');
}

/* -------------------------------------------------------------------- */
/*  GetStr()        gets a null-terminated string from a file           */
/* -------------------------------------------------------------------- */
void GetStr(FILE * fl, char *str, int mlen)
{
    int l;
	uchar ch;

    l = 0;
    ch = 1;
    while (!feof(fl) && ch && l < mlen) {
        ch = (uchar) fgetc(fl);
		if (ch != '\r' && ch != 0xFF) {
            str[l] = ch;
            l++;
        }
    }
    str[l] = '\0';
}

/* -------------------------------------------------------------------- */
/*  PutStr()        puts a null-terminated string to a file             */
/* -------------------------------------------------------------------- */
void PutStr(FILE * fl, char *str)
{
    fwrite(str, sizeof(char), (strlen(str) + 1), fl);
}
/* EOF */
