/* -------------------------------------------------------------------- */
/*  INFO.C                    Citadel                                   */
/* -------------------------------------------------------------------- */
/*  This module deals with the fileinfo.cits and the .ri commands       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  updateFile()    Update or add file to fileinfo.cit                  */
/*  newFile()       Add a new file to the fileinfo.cit                  */
/*  entercomment()  Update/add comment, high level (assumes cur room)   */
/*  setfileinfo()   menu level .as routine sets entry to aide's name    */
/*                  if none present or leaves original uploader         */
/*  getInfo()       get infofile slot for this file (current room)      */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 * HISTORY:
 *
 *	05/07/89	(PAT)	Module created (rewrite of some infofile.c
 *						functions for speed)
 *	03/15/90	{zm}	Use FILESIZE (20) instead of NAMESIZE (30).
 *	03/30/90	FJM 	Added fflush() calls where needed
 *	12/27/90	FJM 	Changes for porting.
 *	01/06/91	FJM 	Porting, formating, fixes and misc changes.
 *	07/11/91	BLJ 	Fix to .as when re-writing a comment
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static void updateFile(char *dir, char *file, char *user, char *comment);
static void newFile(char *dir, struct fInfo *ours);
static BOOL getInfo(char *file, struct fInfo *ours);

/* -------------------------------------------------------------------- */
/*  updateFile()    Update or add file to fileinfo.cit                  */
/* -------------------------------------------------------------------- */
static void updateFile(char *dir, char *file, char *user, char *comment)
{
    struct fInfo info, ours;
    FILE *fl;
	unsigned cntr = 0;
	int i, l;
	char *ptr;
	BOOL found = FALSE;

	/* let's make the structure clean */
	l = sizeof(struct fInfo);
	ptr = (char *) &ours;
	for(i=0; i<l; i++) ptr[i] = '\0';

	/* setup the buffer for write */
    strcpy(ours.fn, file);
    strcpy(ours.uploader, user);
    strcpy(ours.comment, comment);

	if(changedir(dir) == -1) {
		changedir(cfg->homepath);
		return;
	}
	if ((fl = fopen("fileinfo.cit", "r+b")) == NULL) {
		newFile("fileinfo.cit", &ours);
        return;
    }
    while (fread(&info, sizeof(struct fInfo), 1, fl) == 1 && !found) {
        if (strcmpi(file, info.fn) == SAMESTRING) {
			/* seek back and overwrite it */
			fseek(fl, (long) (sizeof(struct fInfo)*cntr), SEEK_SET);
			fwrite(&ours, sizeof(struct fInfo), 1, fl);
            found = TRUE;
        }
		cntr++;
	}

    fclose(fl);

    if (!found)
		newFile("fileinfo.cit", &ours);

	changedir(cfg->homepath);
}

/* -------------------------------------------------------------------- */
/*  newFile()       Add a new file to the fileinfo.cit                  */
/* -------------------------------------------------------------------- */
static void newFile(char *file, struct fInfo * ours)
{
    FILE *fl;

    if ((fl = fopen(file, "ab")) == NULL) {
		return;
    }
    fwrite(ours, sizeof(struct fInfo), 1, fl);

    fclose(fl);
}


/* -------------------------------------------------------------------- */
/*  entercomment()  Update/add comment, high level (assumes cur room)   */
/* -------------------------------------------------------------------- */
void entercomment(char *filename, char *uploader, char *comment)
{
    updateFile(roomBuf.rbdirname, filename, uploader, comment);
}

/* -------------------------------------------------------------------- */
/*  setfileinfo()   menu level .as routine sets entry to aide's name    */
/*                  if none present or leaves original uploader         */
/* -------------------------------------------------------------------- */
void setfileinfo(void)
{
    label filename;
    label uploader;
    char comments[64];
    char path[80];
    struct fInfo old;

    getNormStr("filename", filename, FILESIZE, ECHO);

    sprintf(path, "%s\\%s", roomBuf.rbdirname, filename);

    /* no bad file names */
    if (checkfilename(filename, 0) == ERROR) {
        mPrintf("\n No file %s.\n ", filename);
        return;
    }
    /* no file name? */
    if (!filexists(path)) {
        mPrintf("\n No file %s.\n ", filename);
        return;
    }
    if (!getInfo(filename, &old)) {
        strcpy(uploader, logBuf.lbname);
    } else {
        strcpy(uploader, old.uploader);
    }

    getString("comments", comments, 64, FALSE, TRUE, "");

    entercomment(filename, uploader, comments);

    sprintf(msgBuf->mbtext, "File info changed for file %s by %s",
    filename, logBuf.lbname);

    trap(msgBuf->mbtext, T_AIDE);
}

/* -------------------------------------------------------------------- */
/*  getInfo()       get infofile slot for this file (current room)      */
/* -------------------------------------------------------------------- */
static BOOL getInfo(char *file, struct fInfo * ours)
{
    struct fInfo info;
    FILE *fl;
    BOOL found = FALSE;

	if(changedir(roomBuf.rbdirname) == -1) {
		changedir(cfg->homepath);
		return FALSE;
	}

	if ((fl = fopen("fileinfo.cit", "rb")) == NULL) {
        return FALSE;
    }
    while (fread(&info, sizeof(struct fInfo), 1, fl) == 1 && !found) {
        if (strcmpi(file, info.fn) == SAMESTRING) {
            *ours = info;

            found = TRUE;
        }
    }

	changedir(cfg->homepath);
    return found;
}

/* EOF */
