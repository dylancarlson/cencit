/************************************************************************/
/*                              Infofile.c                              */
/*                 Infofile handling routines for ctdl                  */
/************************************************************************/

#include <string.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <io.h>
#include <alloc.h>
#endif

#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      addinfo()               adds an entry to fileinfo.cit           */
/*      entercomment()          high level upload/comment routine       */
/*      fillinfo()              allocates buffer and reads fileinfo.cit */
/*      infoslot()              returns slot of filename in info-buffer */
/*      killinfo()              removes comment from fileinfo.cit       */
/*      readinfofile()          menu level .ri .rvi routine             */
/*      setfileinfo()           menu level .as routine                  */
/*      showinfo()              show info-buffer according to verbose   */
/*      updateinfo()            removes all non-existant entries        */
/*      batchinfo()             prompts for comments on new files when  */
/*                              TRUE, FALSE adds null comments          */
/*      moveFile()              copy info-buffer & move file            */
/************************************************************************/


/* --------------------------------------------------------------------
 *	History:
 *
 *	03/08/90	{zm}	Add 'softname' as the name of the software.
 *	03/15/90	{zm}	Use FILESIZE (20) instead of NAMESIZE.
 *	03/17/90	FJM 	added checks for farfree()
 *	04/14/90	FJM 	changed to use FILESIZE
 *	11/18/90	FJM 	Changed readinfofile() to correct buffer
 *                      misallocation.
 *                      Simplified filldirectory() and fillinfo().
 *                      Corrected error-of-one in filldirectory().
 *	11/19/90	FJM 	Patches to fileinfo.cit reads & fills.
 *  12/27/90    FJM     Changes for porting.
 *  01/06/91    FJM     Porting, formating, fixes and misc changes.
 *  01/13/91    FJM     Name overflow fixes.
 *	07/11/91	BLJ 	Fix to updateinfo
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *            Module scope variables.
 ************************************************************************/
static long infolength;     /* size (bytes) of fileinfo.cit */

/************************************************************************
 *            Internal function definitions for FILEINFO.C
 ************************************************************************/
static void addinfo(char *filename, char *uploader, char *comment);
static void fillinfo(void);
static int infoslot(char *filename);

/************************************************************************/
/*      addinfo()  appends comment fileinfo.cit                         */
/************************************************************************/
void addinfo(filename, uploader, comment)
char *filename, *uploader, *comment;
{
	int i, l;
	char *ptr;
	struct fInfo info;
    FILE *fd;

    if (changedir(roomBuf.rbdirname) != -1) {

		/* Let's clean up the info struct */
		l = sizeof(struct fInfo);
		ptr = (char *) &info;
		for(i=0; i<l; i++) ptr[i] = '\0';

		strcpy(info.fn, filename);
        strcpy(info.uploader, uploader);
        strcpy(info.comment, comment);

        if ((fd = fopen("fileinfo.cit", "ab")) != NULL) {
            fwrite(&info, sizeof(struct fInfo), 1, fd);
            fclose(fd);
        }
		changedir(cfg->homepath);
    }
}

/************************************************************************/
/*      fillinfo()  allocates buffer and reads fileinfo.cit in ram      */
/************************************************************************/
void fillinfo()
{
    FILE *fd;
    int infocnt;

    if ((changedir(roomBuf.rbdirname) == -1) ||
    (fd = fopen("fileinfo.cit", "rb")) == NULL) {
        infolength = 0L;    /* 0 length if file not there */
		changedir(cfg->homepath);
    } else {
        infolength = filelength(fileno(fd));
    /* calculate # records */
        infocnt = (int) (infolength / ((long) sizeof(struct fInfo)));

        if (infolength) {

        /* allocate exact size of structure */
            fileinfo = farcalloc((unsigned long) infocnt,
            (unsigned long) sizeof(struct fInfo));

            if (!fileinfo) {
                doCR();
                mPrintf(" Error allocating fileinfo ");
                infolength = 0L;
                doCR();
            } else {
        /* read only whole records */
                infocnt = fread(fileinfo, sizeof(struct fInfo), infocnt, fd);
        /* set size to amount actually read */
                infolength = (long) (infocnt * ((long) sizeof(struct fInfo)));
            }
        }
        fclose(fd);
    }
}

/************************************************************************
 *      infoslot()  returns slot of specified filename in info-buffer
 ************************************************************************/
int infoslot(char *filename)
{
    int i;
    int numrecords;

    if (!infolength)
        return (ERROR);     /* don't try if 0 length */

    numrecords = (int) (infolength / ((long) sizeof(struct fInfo)));

    for (i = 0; i < numrecords; ++i) {
        if (strcmpi(filename, fileinfo[i].fn) == SAMESTRING)
            return (i);
    }
    return (ERROR);
}


/************************************************************************
 *      killinfo()  removes one comment from fileinfo.cit if it exists.
 ************************************************************************/
void killinfo(filename)
char *filename;
{
    int slot, numslots;
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1)
        return;

    fillinfo();         /* allocate & file fileinfo structure */
    /* don't make blank infofile.cit's */
    /* don't hfree if 0 lenght wasn't alloc'ed */
    if (infolength) {

        numslots = (int) (infolength / ((long) sizeof(struct fInfo)));

        slot = infoslot(filename);

        if (slot != ERROR) {
            if ((fd = fopen("fileinfo.cit", "wb")) == NULL) {
				changedir(cfg->homepath);
                return;
            }
            --numslots;
            hmemcpy(&fileinfo[slot], &fileinfo[slot + 1],
            (((long) sizeof(struct fInfo)) * ((long) (numslots - slot))));

            fwrite(fileinfo, sizeof(struct fInfo), numslots, fd);
            fclose(fd);
            if (!numslots)
                unlink("fileinfo.cit"); /* remove if 0 length file */
        }
    }
    if (fileinfo)
        farfree((void *) fileinfo);
    fileinfo = NULL;
    infolength = 0L;

	changedir(cfg->homepath);
}

/************************************************************************/
/*      readinfofile()  menu level .ri .rvi routine                     */
/************************************************************************/
void readinfofile(char verbose)
{
    char filename[FILESIZE + 1];

    getNormStr("", filename, FILESIZE, ECHO);

    if (!strlen(filename))
        strcpy(filename, "*.*");

    if (changedir(roomBuf.rbdirname) != -1) {
    /* if there is no info-file, just do a normal disk directory */
        if (!filexists("fileinfo.cit")) {
            if (strlen(filename))
                dir(filename, verbose);
			changedir(cfg->homepath);
        } else {

            filldirectory(filename, TRUE /* verbose */ );

        /* check for matches */
            if (!filedir[0].entry[0]) {
                mPrintf("\n No file %s", filename);
				changedir(cfg->homepath);
            } else {
        /* allocate & read in fileinfo buffer */
                fillinfo();

        /* display info-buffer according to verbose */
                showinfo(verbose);

        /* free info-buffer */
                if (fileinfo)
                    farfree((void *) fileinfo);
                fileinfo = NULL;
                infolength = 0L;
            }
        /* free file directory structure */
            if (filedir)
                farfree((void *) filedir);
            filedir = NULL;

        }
    }
    return;
}

/************************************************************************/
/*      showinfo()  display info-buffer according to verbose            */
/************************************************************************/
void showinfo(char verbose)
{
    int i, slot;

    char comment[64];
    label uploader;
    char filename[FILESIZE];
    char size[10];
    char downtime[10];
    char date[20];
    char time[20];

    outFlag = OUTOK;

    if (changedir(roomBuf.rbdirname) == -1)
        return;

    /* check for matches */
    if (!filedir[0].entry[0]) {
        mPrintf("\n No file %s", filename);
    /* go to our home-path */
		changedir(cfg->homepath);
        return;
    }
    if (!verbose)
        mtPrintf(TERM_BOLD, "Filename        Size  Comments");
						   /*--------.--- -------  ------------------...*/
    doCR();

    for (i = 0; (filedir[i].entry[0] && (outFlag != OUTSKIP) && !mAbort());
    ++i) {


    /* get rid of asterisks */
        filedir[i].entry[0] = ' ';
        filedir[i].entry[13] = ' ';

        sscanf(filedir[i].entry, "%s %s %s %s %s",
        filename,
        date,
        size,
        downtime,
        time);

        slot = infoslot(filename);

        if (slot != ERROR) {
            strcpy(comment, fileinfo[slot].comment);
            strcpy(uploader, fileinfo[slot].uploader);
        } else {
            comment[0] = '\0';
            uploader[0] = '\0';
        }

        if (verbose) {
            doCR();
            mPrintf(" %-13s %s", "Filename:", filename);

            doCR();
            mPrintf(" %-13s %s (%s minutes to download)", "Size:",
            size, downtime);

            doCR();
            mPrintf(" %-13s %s %s", "When:", date, time);

            doCR();
            mPrintf(" %-13s %s", "Uploaded By:", uploader);

            doCR();
            mPrintf(" %-13s %s", "Comments:", comment);
            doCR();
        } else {
            mPrintf("%-12s %7s  %s", filename, size, comment);
            doCR();
        }
    }

    /* go to our home-path */
	changedir(cfg->homepath);
}

/************************************************************************/
/*      updateinfo()  removes all non-existant entries from fileinfo cit*/
/************************************************************************/
void updateinfo()
{
	int i, numrecords, save_pos, tot_recs;
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1)
        return;

    fillinfo();

    if (infolength == 0) {
		/* don't make blank fileinfo.cit's */
		changedir(cfg->homepath);
        return;
    }
    numrecords = infolength / ((long) sizeof(struct fInfo));

    filldirectory("*.*", TRUE);

	save_pos = -1;
	tot_recs = 0;
	for (i = 0; i < numrecords; ++i) {
		if (!filexists(fileinfo[i].fn)) {
			if(debug)
				cPrintf("file %s not found\n",fileinfo[i].fn);
			if(save_pos == -1) save_pos = i;
        }
		else {
			tot_recs++;
			if(save_pos >= 0) {
				if(debug) {
                    cPrintf("moving %s and %s\n",
                    fileinfo[save_pos].fn,fileinfo[i].fn);
                }
				memcpy(&fileinfo[save_pos], &fileinfo[i],
				((long) sizeof(struct fInfo)));
                save_pos = i;
			}
		}
    }
	if(debug)
		cPrintf("Cleaning out %d entrie(s)\n",numrecords-tot_recs);

	numrecords = tot_recs;
	unlink("fileinfo.cit");
	if ((fd = fopen("fileinfo.cit", "wb")) == NULL) {
		changedir(cfg->homepath);

		/* free file directory structure */
        if (filedir)
            farfree((void *) filedir);
        filedir = NULL;

        if (fileinfo)
            farfree((void *) fileinfo);
        fileinfo = NULL;
        infolength = 0L;
        return;
    }
    fwrite(fileinfo, (numrecords * sizeof(*fileinfo)), 1, fd);

    fclose(fd);

    if ((numrecords * sizeof(*fileinfo)) == 0)
        unlink("fileinfo.cit");

    /* free file directory structure */
    if (filedir)
        farfree((void *) filedir);
    filedir = NULL;

    if (fileinfo)
        farfree((void *) fileinfo);
    fileinfo = NULL;
    infolength = 0L;

	changedir(cfg->homepath);
}

/************************************************************************/
/*  batchinfo() askes for comments on all files not in fileinfo.cit     */
/*              when TRUE, adds null fields when FALSE                  */
/************************************************************************/
int batchinfo(askuser)
int askuser;
{
    int i, slot, total = 0;
    char comments[64];
    label uploader;
    char filename[FILESIZE];
    char size[10];
    char date[20];
    static char tmp[150];		/* attempt to reduce stack usage */

    sprintf(msgBuf->mbtext, "Batch upload by %s\n", logBuf.lbname);

    if (changedir(roomBuf.rbdirname) == -1)
        return (0);

    filldirectory("*.*", TRUE);

    fillinfo();

    if (askuser)
        strcpy(uploader, logBuf.lbname);
    else
        uploader[0] = '\0';

    for (i = 0; filedir[i].entry[0]; i++) {
        filedir[i].entry[13] = ' ';
        sscanf(filedir[i].entry, "%s %s %s", filename, date, size);
        slot = infoslot(filename);
        if (slot == ERROR) {
            if (askuser) {
                doCR();
                mPrintf("%-12s %7s %s", filename, size, date);
                doCR();
                getString("comments", comments, 64, FALSE, TRUE, "");
                addinfo(filename, uploader, comments);
                total++;
                if (!comments[0])
                    sprintf(tmp, " %s\n", filename);
                else
                    sprintf(tmp, " %s: %s\n", filename, comments);
                strcat(msgBuf->mbtext, tmp);

                sprintf(tmp, "Batch upload of %s in room %s]",
                filename, roomBuf.rbname);
                trap(tmp, T_UPLOAD);
            } else {
                comments[0] = '\0';
                addinfo(filename, uploader, comments);
            }
        }
    }
    if (fileinfo)
        farfree((void *) fileinfo);
    fileinfo = NULL;
    infolength = 0L;

	changedir(cfg->homepath);

    /* free file directory structure */
    if (filedir != NULL)
        farfree((void *) filedir);
    filedir = NULL;

    return total;
}

/************************************************************************/
/*      moveFile()  copy info-buffer & move file                        */
/************************************************************************/
void moveFile(void)
{
    struct fInfo info;
    FILE *fd;

    char source[FILESIZE+1], destination[64];
    char temp[84];

    int i, slot;

    char size[10];
    char downtime[10];
    char date[20];
    char time[20];

    doCR();
    getNormStr("source filename", source, FILESIZE, ECHO);
    if (!strlen(source))
        return;

    getNormStr("destination path", destination, 64, ECHO);
    if (!strlen(destination))
        return;

    if (changedir(destination) == -1) {
        mPrintf("\n Invalid pathname.");
		changedir(cfg->homepath);
        return;
    }
    if (changedir(roomBuf.rbdirname) == -1)
        return;

    if ((checkfilename(source, 0) == ERROR)
    || ambig(source) || ambig(destination)) {
        mPrintf("\n Invalid filename.");
		changedir(cfg->homepath);
        return;
    }
    if (!filexists(source)) {
        mPrintf(" No file %s", source);
		changedir(cfg->homepath);
        return;
    }
    sprintf(temp, "%s\\%s", destination, source);
    if (filexists(temp)) {
        mPrintf("\n File exists.");
		changedir(cfg->homepath);
        return;
    }
    filldirectory(source, TRUE /* verbose */ );

    /* check for matches */
    if (!filedir[0].entry[0]) {
        mPrintf("\n No file %s", source);

    /* free file directory structure */
        if (filedir != NULL)
            farfree((void *) filedir);
        filedir = NULL;

    /* go to our home-path */
		changedir(cfg->homepath);
        return;
    }
    fillinfo();

    for (i = 0; (filedir[i].entry[0]); ++i) {

    /* get rid of asterisks */
        filedir[i].entry[0] = ' ';
        filedir[i].entry[13] = ' ';

        sscanf(filedir[i].entry, "%s %s %s %s %s",
        source,
        date,
        size,
        downtime,
        time);

        slot = infoslot(source);

        if (slot != ERROR) {
            strcpy(info.fn, source);
            strcpy(info.uploader, fileinfo[slot].uploader);
            strcpy(info.comment, fileinfo[slot].comment);
        } else {
            strcpy(info.fn, source);
            info.comment[0] = '\0';
            info.uploader[0] = '\0';
        }

    }

    /* free file directory structure */
    if (filedir != NULL)
        farfree((void *) filedir);
    filedir = NULL;

    /* free info-buffer */
    if (fileinfo)
        farfree((void *) fileinfo);
    fileinfo = NULL;
    infolength = 0L;

    changedir(destination);

    if ((fd = fopen("fileinfo.cit", "ab")) == NULL) {
        mPrintf("\n Not a %s directory!\n", softname);
        return;
    }
    fwrite(&info, sizeof(*fileinfo), 1, fd);

    fclose(fd);

    if (changedir(roomBuf.rbdirname) == -1)
        return;

    /* if successful */
    if (rename(source, temp) == 0) {
        sprintf(msgBuf->mbtext,
        "File %s moved to %s in %s] by %s",
        source, destination,
        roomBuf.rbname,
        logBuf.lbname);

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

        killinfo(source);   /* kill old entry */
    } else {
        mPrintf("\n Cannot move %s\n", source);
    }

    /* go to our home-path */
	changedir(cfg->homepath);
}

/* EOF */
