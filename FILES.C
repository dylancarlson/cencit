/************************************************************************/
/* Files.c                                                              */
/* File handling routines for ctdl                                      */
/************************************************************************/

#include <string.h>
#include <time.h>
#include "ctdl.h"

#ifndef ATARI_ST
#include <dos.h>
#include <alloc.h>
#include <io.h>
#endif

#include "proto.h"
#include "global.h"

/************************************************************************
 *
 *                              CONTENTS
 *
 *      ambig()                 returns true if filename is ambiguous
 *      ambigUnlink()           unlinks ambiguous filenames
 *      attributes()            aide fn to set file attributes
 *      blocks()                displays how many blocks file is
 *      checkfilename()         returns ERROR on illegal filenames
 *      checkup()               returns TRUE if filename can be uploaded
 *      dir()                   very high level, displays directory
 *      download()              menu level download routine
 *      dltime()                computes dl time from size & global rate
 *      dump()                  does Unformatted dump of specified file
 *      dumpf()                 does Formatted dump of specified file
 *      entertextfile()         menu level .et
 *      enterwc()               menu level .ew file
 *      entrycopy()             readable struct -> directory array
 *      entrymake()             dos transfer struct -> readable struct
 *      filexists()             returns TRUE if a specified file exists
 *      filldirectory()         fills our directory structure
 *      getfirst()              low level, read first item of directory
 *      getnext()               low level, read next item of directory
 *      hello()                 prints random hello blurbs
 *      hide()                  hides a file. for limited-access u-load
 *      nextblurb()             show next .BLB or .BL@ file
 *      readdirectory()         menu level .rd .rvd routine
 *      readinfofile()          menu level .ri .rvi routine
 *      readtextfile()          menu level .rt routine
 *      readwc()                menu level .rw file
 *      renamefile()            aide fn to rename a file
 *      strlwr()                makes any string lower case
 *      textdown()              does wildcarded unformatted file dumps
 *      textup()                handles actual text upload
 *      tutorial()              handles wildcarded helpfile dumps
 *      upload()                menu level file upload routine
 *      unlinkfile()            handles the .au command
 *      wcdown()                calls xmodem downloading routines
 *      wcup()                  calls xmodem uploading routines
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *
 * History:
 *
 * 03/15/90    {zm}    Use FILESIZE (20) instead of NAMESIZE.
 * 03/17/90    FJM     added checks for farfree()
 * 04/14/90    FJM     Changed checkfilename(), renamefile() to use FILESIZE
 * 04/16/90    FJM     Changed checkfilename(), renamefile() to use
 *                     FILESIZE+1 for storage.
 * 04/20/90    FJM     Created nextblurb().
 * 06/28/90    FJM     Added CLOCK$, COM3, COM4 as devices.
 * 07/08/90    FJM     Made nextblurb() a bit smarter.
 * 08/07/90    FJM     Made textup,wcup blurbs rotate (why not?).
 * 09/20/90    FJM     Corrected bug that always set screen height to 0
 *                     in goodbye()!
 * 10/15/90    FJM     Added console only protocol flag (field #1)
 * 11/04/90    FJM     Added code to terminate after up/download.
 * 11/18/90    FJM     Cleaned up filldirectory(), fillinfo().
 *                     Fixed error of one in fillinfo() and buffer
 *                     mis-allocation in readinfofile().
 * 11/23/90    FJM     Added countdown() for autologoff.
 * 11/01/90    FJM     Fixed bugs related to non-existant room
 *                     directories defaulting to the home path.
 * 01/28/91    FJM     Added batchinfo(FALSE) to batch uploads.
 *                     Made reset file info not reset files that exist.
 * 06/21/92    BLJ	   Added szmodem.log to badfiles list.
 *					   Do not allow szmodem.log to display in directory
 *
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 * local functions
 * -------------------------------------------------------------------- */
static double dltime(long size);
static void hide(char *filename);
static void entrycopy(int element, char verbose);
static void entrymake(struct ffblk *file_buf);
static void upDownMnu(char cmd);
static void strlower(char *string);
static int dump(char *filename);
static void textdown(char *filename);
static void textup(char *filename);
static int checkup(char *filename);
static void blocks(char *filename, int bsize);
static int countdown(char *prompt, long timeout);

/* --------------------------------------------------------------------
 * local defs
 * -------------------------------------------------------------------- */
#define MAXWORD 256

/*
 * --------------------------------------------------------------------
 * local structs
 * -------------------------------------------------------------------- */

/* our readable transfer structure */
static struct {
    char            filename[13];
    unsigned char   attribute;
    char            date[9];
    char            time[9];
    long            size;
}directory;

/* --------------------------------------------------------------------
 * local data
 * --------------------------------------------------------------------*/
static char    *devices[] = {
    "CON", "AUX", "COM1", "COM2", "COM3", "COM4", "LPT1", "PRN", "LPT2",
    "LPT3", "NUL", NULL
};

static char    *badfiles[] ={
    "LOG.DAT", "MSG.DAT", "GRP.DAT", "CONFIG.CIT", "HALL.DAT", "ROOM.DAT",
	"FILEINFO.CIT", "TZ.CIT", "GRPDATA.CIT", "CLOCK$", "SZMODEM.LOG", NULL
};


/************************************************************************/
/* ambig() returns TRUE if string is an ambiguous filename         */
/************************************************************************/
int ambig(char *filename)
{
    int i;

    for (i = 0; i < strlen(filename); ++i) {
        if ((filename[i] == '*') || (filename[i] == '?'))
            return (TRUE);
    }
    return (FALSE);
}

/************************************************************************/
/* ambigUnlink() unlinks ambiguous filenames                       */
/************************************************************************/
int ambigUnlink(char *filename, char change)
{
    char            file[FILESIZE];
    int             i = 0;

    if (change)
        if (changedir(roomBuf.rbdirname) == -1) {
            return 0;
        }
    filldirectory(filename, TRUE);

    while (filedir[i].entry[0]) {
        filedir[i].entry[13] = ' ';
        sscanf(filedir[i].entry, "%s ", file);
        if (file[0])
            unlink(file);
        i++;
    }

    /* free file directory structure */
    if (filedir != NULL) {
        if (filedir)
            free((void *) filedir);
        filedir = NULL;
    }
    return (i);
}

/************************************************************************/
/* attributes() aide fn to set file attributes                     */
/************************************************************************/
void attributes(void)
{
    label           filename;
    char            hidden = 0, readonly = 0;
    unsigned char   attr, getattr();

    doCR();
    getNormStr("filename", filename, FILESIZE, ECHO);

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    if ((checkfilename(filename, 0) == ERROR)
    || ambig(filename)) {
        mPrintf("\n Invalid filename.");
		changedir(cfg->homepath);
        return;
    }
    if (!filexists(filename)) {
        mPrintf(" File not found\n");
		changedir(cfg->homepath);
        return;
    }
    attr = getattr(filename);

    readonly = attr & 1;
    hidden = ((attr & 2) == 2);

    /* set readonly and hidden bits to zero */
    attr = (attr ^ readonly);
    attr = (attr ^ (hidden * 2));

    readonly = getYesNo("Read only", readonly);
    hidden = getYesNo("Hidden", hidden);

    /* set readonly and hidden bits */
    attr = (attr | readonly);
    attr = (attr | (hidden * 2));

    setattr(filename, attr);

    sprintf(msgBuf->mbtext,
    "Attributes of file %s changed in %s] by %s",
    filename,
    roomBuf.rbname,
    logBuf.lbname);

    trap(msgBuf->mbtext, T_AIDE);

    aideMessage();

	changedir(cfg->homepath);
}

/************************************************************************/
/* blocks()  displays how many blocks file is upon download             */
/************************************************************************/
void blocks(char *filename, int bsize)
{
    FILE *stream;
    long length;
    int blks;
    double dltime();

    outFlag = OUTOK;

    stream = fopen(filename, "r");

    length = filelength(fileno(stream));

    fclose(stream);

	if (length == -1L)
        return;

    if (bsize)
        blks = ((int) (length / (long) bsize) + 1);
    else
        blks = 0;

    doCR();

    if (!bsize)
        mPrintf("File Size: %ld %s",
		length, (length == 1L) ? "byte" : "bytes");
    else
        mPrintf("File Size: %d %s, %ld %s",
        blks, (blks == 1) ? "block" : "blocks",
		length, (length == 1L) ? "byte" : "bytes");

    doCR();
    mPrintf("Transfer Time: %.0f minutes", dltime(length));
    doCR();
}

/************************************************************************/
/* checkfilename() checks a filename for illegal characters             */
/************************************************************************/
int checkfilename(char *filename, char xtype)
{
    char *s, *s2;
    char i;
    char device[6];
    char invalid[25];

    strcpy(invalid, " '\"/\\[]:|<>+=;,");
	extrn = start_extrn;
	xtype--;
	for(i=0; i<xtype; i++) extrn = extrn->next;


	if (extrn->ex_batch)
        strcpy(invalid, "'\"/\\[]:|<>+=;,");

    for (s = filename; *s; ++s)
        for (s2 = invalid; *s2; ++s2)
            if ((*s == *s2) || *s < 0x20)
                return (ERROR);

    for (i = 0; i <= 4 && filename[i] != '.'; ++i) {
        device[i] = filename[i];
    }
    device[i] = '\0';

    for (i = 0; devices[i] != NULL; ++i)
        if (strcmpi(device, devices[i]) == SAMESTRING)
            return (ERROR);

    for (i = 0; badfiles[i] != NULL; ++i)
        if (strcmpi(filename, badfiles[i]) == SAMESTRING)
            return (ERROR);

    return (TRUE);
}

/***********************************************************************/
/* checkup()  returns TRUE if filename can be uploaded                 */
/***********************************************************************/
int checkup(char *filename)
{
    if (ambig(filename))
        return (ERROR);

    /* no bad files */
    if (checkfilename(filename, 0) == ERROR) {
        mPrintf("\n Invalid filename.");
        return (ERROR);
    }
    if (changedir(roomBuf.rbdirname) == -1) {
        return FALSE;
    }
    if (filexists(filename)) {
        mPrintf("\n File exists.");
		changedir(cfg->homepath);
        return (ERROR);
    }
    return (TRUE);
}

/************************************************************************/
/* filldirectory()  this routine will fill the directory structure      */
/* according to wildcard                                                */
/************************************************************************/
void filldirectory(char *filename, char verbose)
{
    int             i, rc;
    struct ffblk    file_buf;
    int             strip;

    /*
     * allocate the first record of the file dir structure
     *
	 * Since we null terminate, really only cfg->maxfiles-1 files allowed.
     *
     */

	filedir = farcalloc((long) (sizeof(struct fDir)), cfg->maxfiles);
    /* return on error allocating */
    if (!filedir) {
        cPrintf("Failed to allocate memory for FILEDIR\n");
        return;
    }
    /* keep going till it errors, which is end of directory */
    for (rc = findfirst("*.*", &file_buf, (aide ? FA_HIDDEN : 0)), i = 0;
    !rc; rc = findnext(&file_buf), ++i) {
	/* Only cfg->maxfiles # of files files (was error of one here) */
		if (i >= cfg->maxfiles - 1)
            break;

    /* translate dos's structure to something we can read */
        entrymake(&file_buf);

        if (!strpos('.', directory.filename)) {
            strcat(directory.filename, ".");
            strip = TRUE;
        } else {
            strip = FALSE;
        }

    /*
     * copy "directory" to "filedir" NO zero length filenames
     *
     * Probably should check against 'badfiles' here.
     *
     */

        if ((!(directory.attribute &
            (FA_HIDDEN | FA_SYSTEM | FA_DIREC | FA_LABEL)))
    /* either aide or not a hidden file */
    /* && (aide || !(directory.attribute & FA_HIDDEN) ) */

    /* filename match wildcard? */
            && (u_match(directory.filename, filename))
	/* never display szmodem.log  */
		&& (strcmpi(directory.filename, "szmodem.log") != SAMESTRING)
	/* never display fileinfo.cit */
        && (strcmpi(directory.filename, "fileinfo.cit") != SAMESTRING)) {

        /* if passed, put into structure, else loop again */
            if (strip)
                directory.filename[strlen(directory.filename) - 1] = '\0';
            entrycopy(i, verbose);
        } else {
            i--;
        }

    }
    filedir[i].entry[0] = '\0'; /* tie it off with a null */

    /* alphabetical order */
    qsort(filedir, i, sizeof(struct fDir), strcmp);
}

/***********************************************************************/
/* dir() highest level file directory display function                 */
/***********************************************************************/
void dir(char *filename, char verbose)
{
    int             i;
    long            bytesfree();

    outFlag = OUTOK;

    /* no bad files */
    /*
     * if (checkfilename(filename, 0) == ERROR) { mPrintf("\n No file %s",
     * filename); return; }
     */

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    /* load our directory structure according to filename */
    filldirectory(filename, verbose);

    if (filedir[0].entry[0]) {
        if (verbose)
            mtPrintf(TERM_BOLD, "\n Filename     Date      Size   D/L Time");
        else
            doCR();
    }
    /* check for matches */
    if (!filedir[0].entry[0])
        mPrintf("\n No file %s", filename);

    for (i = 0;
        (filedir[i].entry[0] && (outFlag != OUTSKIP) && !mAbort());
    ++i) {
        if (verbose) {
            filedir[i].entry[0] = filedir[i].entry[13];
            filedir[i].entry[13] = ' ';
            filedir[i].entry[40] = '\0';    /* cut timestamp off */
            doCR();
        }
    /* display filename */
        mPrintf(filedir[i].entry);
    }

    if (verbose && outFlag != OUTSKIP) {
        doCR();
        mtPrintf(TERM_BOLD, "        %d %s    %ld bytes free", i,
        (i == 1) ? "File" : "Files", bytesfree());
    }
    /* free file directory structure */
    if (filedir)
        farfree((void *) filedir);
    filedir = NULL;


    /* go to our home-path */
	changedir(cfg->homepath);
}

/***********************************************************************/
/* dltime()  give this routine the size of your file and it will       */
/* return the amount of time it will take to download according to     */
/* speed                                                               */
/***********************************************************************/
double dltime(long size)
{
    double          time;
    static long     fudge_factor[] = {1800L, 7200L, 14400L, 28800L, 57600L};
    /* could be more accurate */

    time = (double) size / (double) (fudge_factor[speed]);

    return (time);
}

/************************************************************************/
/* dump()  does unformatted dump of specified file                      */
/* returns ERROR if carrier is lost, or file is aborted                 */
/************************************************************************/
int dump(char *filename)
{
    FILE           *fbuf;
    int             c, returnval = TRUE;
	int lastchar, charb4, char2b4;

    /* last itteration might have been N>exted */
    outFlag = OUTOK;
    doCR();

    if ((fbuf = fopen(filename, "r")) == NULL) {
        mPrintf("\n No file %s", filename);
        return (ERROR);
    }
    /* looks like a kludge, but we need speed!! */

	lastchar = charb4 = char2b4 = 0;
	while ((c = getc(fbuf)) != ERROR && (c != 0x1a /* CPMEOF */ )
    && (outFlag != OUTNEXT) && (outFlag != OUTSKIP) && !mAbort()) {
		if (c == '\n') {
            doCR();
		}
		else {
			if(char2b4 == 27 && charb4 == '[' && lastchar == 'M') {
				if(cfg->Sound) {
					SoundOn = TRUE;
					sound_pos = 0;
					if(c == 'F')
						Music = TRUE;
					else
						Music = FALSE;
				}
			}
			oChar(c);
            if(c == 14) {
				SoundOn = FALSE;
				if(sound_pos && cfg->Sound) {
					Sound_Buffer[sound_pos] = '\0';
					sound_pos = 0;
					if(Music)
						play();
					else
						sound_effect();
				}
			}
			char2b4 = charb4;
			charb4 = lastchar;
			lastchar = c;
		}
    }

	/* The following is in case of stopping music output with S */
	sound_pos = 0;
	SoundOn = FALSE;

	if (outFlag == OUTSKIP)
        returnval = ERROR;

    fclose(fbuf);

    return returnval;
}

/************************************************************************/
/* dumpf()  does formatted dump of specified file                       */
/* returns ERROR if carrier is lost, or file is aborted                 */
/************************************************************************/
int dumpf(char *filename)
{
    FILE           *fbuf;
    char            line[MAXWORD];
    int             returnval = TRUE;

    /* last itteration might have been N>exted */
    outFlag = OUTOK;
    doCR();

    if ((fbuf = fopen(filename, "r")) == NULL) {
        mPrintf("\n No helpfile %s", filename);
        return (ERROR);
    }
    /* looks like a kludge, but we need speed!! */

    while (fgets(line, MAXWORD, fbuf) && (outFlag != OUTNEXT)
    && (outFlag != OUTSKIP) && !mAbort()) {
        mFormat(line);
    }
    if (outFlag == OUTSKIP)
        returnval = ERROR;

    fclose(fbuf);

    return returnval;
}

/************************************************************************/
/* entertextfile()  menu level .et                                      */
/************************************************************************/
void entertextfile(void)
{
    label           filename;
    char            comments[64];

    doCR();
    getNormStr("filename", filename, FILESIZE, ECHO);

    if (checkup(filename) == ERROR)
        return;

    getString("comments", comments, 64, FALSE, TRUE, "");

    if (strlen(filename))
        textup(filename);

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    if (filexists(filename)) {
        entercomment(filename, logBuf.lbname, comments);
        if (!comments[0])
            sprintf(msgBuf->mbtext, " %s uploaded by %s", filename, logBuf.lbname);
        else
            sprintf(msgBuf->mbtext, " %s uploaded by %s\n Comments: %s",
            filename, logBuf.lbname, comments);
        specialMessage();
    }
	changedir(cfg->homepath);
}

/************************************************************************/
/* entrycopy()                                                          */
/* This routine copies the single readable "directory" array to the     */
/* to the specified element of the "dir" array according to verbose.    */
/************************************************************************/
void entrycopy(int element, char verbose)
{
    double          dltime();

    if (verbose) {
    /* need error checks here! */
        sprintf(filedir[element].entry, " %-12s %s %7ld %9.2f %s ",
        directory.filename,
        directory.date,
        directory.size,
        dltime(directory.size),
        directory.time
        );

        if ((directory.attribute & FA_HIDDEN))
            filedir[element].entry[13] = '*';

    } else
        sprintf(filedir[element].entry, "%-12s ", directory.filename);
}

/************************************************************************/
/* entrymake()                                                          */
/* This routine converts one filename from the entry structure to the   */
/* "directory" structure.                                               */
/************************************************************************/
void entrymake(struct ffblk * file_buf)
{
    char            string[20];

    /* copy filename   */
    strcpy(directory.filename, file_buf->ff_name);
    strlower(directory.filename);   /* make it lower case */

    /* copy attribute  */
    directory.attribute = file_buf->ff_attrib;

    /* copy date       */
    getdstamp(string, file_buf->ff_fdate);
    strcpy(directory.date, string);

    /* copy time       */
    gettstamp(string, file_buf->ff_ftime);
    strcpy(directory.time, string);

    /* copy filesize   */
    directory.size = file_buf->ff_fsize;
}

/************************************************************************
 * nextblurb(char *name, int *count, int showhelp) print next blurb
 ************************************************************************/
void nextblurb(char *name, int *count, int help)
{
    char            filename[FILESIZE];
    char            filename2[FILESIZE];

	if (changedir(cfg->helppath) == -1)
        return;
    if (*count) {
        sprintf(filename, "%.6s%d.blb", name, *count);
        sprintf(filename2, "%.6s%d.bl@", name, *count);

		if (!filexists(filename)) {
			if(ansiOn) {
				if(!filexists(filename2)) {
					if (changedir(cfg->helppath_2) == -1)
						return;
				}
			}
			else {
				if (changedir(cfg->helppath_2) == -1)
                    return;
            }

			if (!filexists(filename)) {
				if(ansiOn) {
					if(!filexists(filename2))
						*count = 0;
				}
				else *count = 0;
			}
		}
        if (debug) {
            cPrintf("checking blurbs %s and %s\n", filename, filename2);
        }
    }
    if (!*count) {
        sprintf(filename, "%.8s.blb", name);
        sprintf(filename2, "%.8s.bl@", name);
	}
	changedir(cfg->homepath);
    if (debug) {
        cPrintf("Next blurb %s\n", filename);
    }
    tutorial(filename, help);
    *count += 1;
}

/************************************************************************/
/* hello()  prints random hello blurbs                                  */
/************************************************************************/
void hello(void)
{
    static int      whichHello = 0;
    expert = TRUE;
    nextblurb("hello", &whichHello, 0);
    expert = FALSE;
}

/************************************************************************/
/* goodbye()  prints random goodbye blurbs                              */
/************************************************************************/
void goodbye(void)
{
    uchar           screen_height;
    static int      whichBye = 0;
    screen_height = logBuf.linesScreen;
    logBuf.linesScreen = 0;
    nextblurb("logout", &whichBye, 0);
    logBuf.linesScreen = screen_height;
}

/************************************************************************/
/* hide()  hides a file. for limited-access u-load                      */
/************************************************************************/
void hide(char *filename)
{
    unsigned char   attr, getattr();

    attr = getattr(filename);

    /* set hidden bit on */
    attr = (attr | 2);

    setattr(filename, attr);
}

/************************************************************************/
/* readdirectory()  menu level .rd .rvd routine  HIGH level routine     */
/************************************************************************/
void readdirectory(char verbose)
{
    label           filename;

    getNormStr("", filename, FILESIZE, ECHO);

    if (strlen(filename))
        dir(filename, verbose);
    else
        dir("*.*", verbose);
}

/************************************************************************/
/* readtextfile()  menu level .rt  HIGH level routine                   */
/************************************************************************/
void readtextfile(void)
{
    label           filename;

    doCR();
    getNormStr("filename", filename, FILESIZE, ECHO);
    if (strlen(filename))
        textdown(filename);
}

/************************************************************************/
/* renamefile()  aide fn to rename a file                               */
/************************************************************************/
void renamefile(void)
{
    char            source[FILESIZE + 1], destination[FILESIZE + 1];

    doCR();
    getNormStr("source filename", source, FILESIZE, ECHO);
    if (!strlen(source))
        return;

    getNormStr("destination filename", destination, FILESIZE, ECHO);
    if (!strlen(destination))
        return;

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    if ((checkfilename(source, 0) == ERROR)
        || (checkfilename(destination, 0) == ERROR)
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
    if (filexists(destination)) {
        mPrintf("\n File exists.");
		changedir(cfg->homepath);
        return;
    }
    /* if successful */
    if (rename(source, destination) == 0) {
        sprintf(msgBuf->mbtext,
        "File %s renamed to file %s in %s] by %s",
        source, destination,
        roomBuf.rbname,
        logBuf.lbname);

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();
    } else
        mPrintf("\n Cannot rename %s\n", source);

	changedir(cfg->homepath);
}

/************************************************************************/
/* strlwr()   makes a string lower case                                 */
/************************************************************************/
void strlower(char *string)
{
	char *s;

	for (s = string; *s; ++s)
        *s = tolower(*s);
}

/************************************************************************/
/* textdown() dumps a host file with no formatting                      */
/* this routine handles wildcarding of text downloads                   */
/************************************************************************/
void textdown(char *filename)
{
    int             i;

    outFlag = OUTOK;

    /* no bad files */
    if (checkfilename(filename, 0) == ERROR) {
        mPrintf("\n No file %s", filename);
        return;
    }
    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    if (ambig(filename)) {
    /* fill our directory array according to filename */
        filldirectory(filename, 0);

    /* print out all the files */
        for (i = 0; filedir[i].entry[0] && (dump(filedir[i].entry) != ERROR);
            i++);

        if (!i)
            mPrintf("\n No file %s", filename);

    /* free file directory structure */
        if (filedir != NULL)
            farfree((void *) filedir);
        filedir = NULL;
    } else
        dump(filename);

    sprintf(msgBuf->mbtext, "Text download of file %s in room %s]",
    filename, roomBuf.rbname);

    trap(msgBuf->mbtext, T_DOWNLOAD);

    doCR();

    /* go to our home-path */
	changedir(cfg->homepath);
}

/************************************************************************/
/* textup()  handles textfile uploads                                   */
/************************************************************************/
void textup(char *filename)
{
    int             i;
    static int      tcount = 0;

    if (!expert)
        nextblurb("textup", &tcount, 1);

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    doCR();

    if ((upfd = fopen(filename, "wt")) == NULL) {
        mPrintf("\n Can't create %s!\n", filename);
    } else {
        while (((i = iChar()) != 0x1a /* CNTRLZ */ )
            && outFlag != OUTSKIP
        && (onConsole || gotCarrier())) {
            fputc(i, upfd);
        }
        fclose(upfd);

        sprintf(msgBuf->mbtext, "Text upload of file %s in room %s]",
        filename, roomBuf.rbname);

        if (limitFlag && filexists(filename))
            hide(filename);

        trap(msgBuf->mbtext, T_UPLOAD);
    }

	changedir(cfg->homepath);
}

/************************************************************************
 *      tutorial() dumps fomatted help files
 *      this routine handles wildcarding of formatted text downloads
 *              will not print "can't find %s.blb" if showhelp is 0
 ************************************************************************/
void tutorial(char *filename, int showhelp)
{
    int             i;
    char            temp[FILESIZE];

    outFlag = OUTOK;

    if (!expert)
        mPrintf("\n <J>ump <N>ext <P>ause <S>top\n");
    doCR();

	if (changedir(cfg->helppath) == -1) {
        return;
    }
    /* no bad files */
    if (checkfilename(filename, 0) == ERROR) {
        if (showhelp) {
            mPrintf("\n No helpfile %s", filename);
        }
		changedir(cfg->homepath);
        return;
    }
    if (ambig(filename)) {
    /* fill our directory array according to filename */
        filldirectory(filename, 0);

    /* print out all the files */
        for (i = 0; filedir[i].entry[0] && (dumpf(filedir[i].entry) != ERROR);
            i++);

        if (!i && showhelp)
            mPrintf("\n No helpfile %s", filename);

    /* free file directory structure */
        if (filedir != NULL)
            farfree((void *) filedir);
        filedir = NULL;
    } else {
        /* First, we'll look in the primary path - then, if the file is not
         * there we'll look in the secondary path
         */

        strcpy(temp, filename);
        temp[strlen(temp) - 1] = '@';

        if (filexists(temp) && ansiOn) {
			if(debug) cPrintf("\nFile %s in dir %s\n",temp,cfg->helppath);
			dump(temp);
        }
        else {
            if (filexists(filename)) {
				if(debug) cPrintf("\nFile %s in dir %s\n",filename,cfg->helppath);
                dumpf(filename);
            }
            else {
				if (changedir(cfg->helppath_2) == -1) {
					changedir(cfg->homepath);
                    return;
                }
				if (filexists(temp) && ansiOn) {
					if(debug) cPrintf("\nFile %s in dir %s\n",temp,cfg->helppath_2);
                    dump(temp);
				}
				else {
					if(debug) cPrintf("\nFile %s in dir %s\n",filename,cfg->helppath_2);
                    dumpf(filename);
				}
            }
        }
    }

    /* go to our home-path */
	changedir(cfg->homepath);
}

/************************************************************************/
/* unlinkfile()  handles .au  aide unlink                               */
/************************************************************************/
void unlinkfile(void)
{
    label           filename;

    getNormStr("filename", filename, FILESIZE, ECHO);

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    if (checkfilename(filename, 0) == ERROR) {
        mPrintf(" No file %s", filename);
		changedir(cfg->homepath);
        return;
    }
    if (!filexists(filename)) {
        mPrintf(" No file %s", filename);
		changedir(cfg->homepath);
        return;
    }
    /* if successful */
    if (unlink(filename) == 0) {
        sprintf(msgBuf->mbtext,
        "File %s unlinked in %s] by %s",
        filename,
        roomBuf.rbname,
        logBuf.lbname);

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

        killinfo(filename);

    } else
        mPrintf("\n Cannot unlink %s\n", filename);

	changedir(cfg->homepath);
}

/************************************************************************
 *  Extended Download
 ************************************************************************/
void wxsnd(char *path, char *file, char trans)
{
	char			stuff[100], i;
    label           tmp1, tmp2;

    if (changedir(path) == -1) {
        return;
    }
	sprintf(tmp1, "%d", cfg->mdata);
    sprintf(tmp2, "%d", bauds[speed]);
	extrn = start_extrn;
	trans--;
	for(i=0; i<trans; i++) extrn = extrn->next;

	sformat(stuff, extrn->ex_snd,
		"fpsan", file, tmp1, tmp2, cfg->aplpath, logBuf.lbname);
    apsystem(stuff);

    if (debug)
        cPrintf("(%s)", stuff);
}

/************************************************************************
 *  Extended Upload
 ************************************************************************/
void wxrcv(char *path, char *file, char trans)
{
	char			stuff[100],i;
    label           tmp1, tmp2;

    if (changedir(path) == -1) {
        return;
    }
	sprintf(tmp1, "%d", cfg->mdata);
    sprintf(tmp2, "%d", bauds[speed]);
	extrn = start_extrn;
	trans--;
	for(i=0; i<trans; i++) extrn = extrn->next;

	sformat(stuff, extrn->ex_rcv,
	"fpsan", file, tmp1, tmp2, cfg->aplpath, logBuf.lbname);
    apsystem(stuff);

    if (debug)
        cPrintf("(%s)", stuff);
}

/************************************************************************
 * countdown - return nonzero on abort
 ************************************************************************/
int countdown(char *prompt, long timeout)
{
    long            timer, cur, last;

    time(&timer);
    time(&cur);
    last = timeout;
    timer += timeout;
	mPrintf("\n\nCountdown to %s, ESC to abort.\n\n%03ld\a", prompt, timeout);
    while (timer > cur) {
        time(&cur);
        if (last > timer - cur) {
            last = timer - cur;
			mPrintf("\b\b\b%03ld", last);
        }
        if (BBSCharReady()) {
            if (iChar() == 0x1b) {
                mPrintf(" Aborted!\n");
                return 1;
            }
        }
    }
    mPrintf("\n\n%s!\a\n\n", prompt);
    return 0;
}

/************************************************************************/
/* download()  menu level download routine                              */
/************************************************************************/
void download(char c)
{
    long            transTime1, transTime2; /* to give no cost uploads       */
    char            filename[80];
	char			ch, xtype, i;
    static int      wcount;
    int             term_user;

    if (!c)
        ch = tolower(iChar());
    else
        ch = c;

    xtype = strpos(ch, extrncmd);

    if (!xtype) {
        if (ch == '?')
            upDownMnu('D');
        else {
            mPrintf(" ?");
            if (!expert)
                upDownMnu('D');
        }
        return;
    } else {
		extrn = start_extrn;
		xtype--;
		for(i=0; i<xtype; i++) extrn = extrn->next;
		xtype++;
		if (!onConsole && extrn->ex_console) {
            doCR();
            mPrintf("Protocol is console or network only");
            return;
        } else
			mPrintf("\b%s", extrn->ex_name);
    }

    doCR();

    if (changedir(roomBuf.rbdirname) == -1) {
        return;
    }
    getNormStr("filename", filename,
	(extrn->ex_batch) ? 80 : FILESIZE, ECHO);

	if (extrn->ex_batch) {
        char           *words[256];
        int             count, i;
        char            temp[80];

        strcpy(temp, filename);

        count = parse_it(words, temp);

        if (count == 0)
            return;

        for (i = 0; i < count; i++) {
            if (checkfilename(words[i], 0) == ERROR) {
                mPrintf("\n No file %s", words[i]);
				changedir(cfg->homepath);
                return;
            }
            if (!filexists(words[i]) && !ambig(words[i])) {
                mPrintf("\n No file %s", words[i]);
                return;
            }
            if (!ambig(words[i])) {
                doCR();
                mPrintf("%s: ", words[i]);
				blocks(words[i], extrn->ex_block);
            }
        }
    } else {
        if (checkfilename(filename, 0) == ERROR) {
            mPrintf("\n No file %s", filename);
			changedir(cfg->homepath);
            return;
        }
        if (ambig(filename)) {
            mPrintf("\n Not a batch protocol");
			changedir(cfg->homepath);
            return;
        }
        if (!filexists(filename)) {
            mPrintf("\n No file %s", filename);
            return;
        }
		blocks(filename, extrn->ex_block);
    }

    if (!strlen(filename))
        return;

    if (!expert)
        nextblurb("wcdown", &wcount, 1);

    term_user = getYesNo("Hang up after file transfer", 0);

    if (getYesNo("Ready for file transfer", 0)) {
        time(&transTime1);
        wxsnd(roomBuf.rbdirname, filename, xtype);
        time(&transTime2);

		if (cfg->accounting && !logBuf.lbflags.NOACCOUNT && !specialTime) {
            calc_trans(transTime1, transTime2, 0);
        }
        sprintf(msgBuf->mbtext, "%s download of file %s in room %s]",
		extrn->ex_name, filename, roomBuf.rbname);

        trap(msgBuf->mbtext, T_DOWNLOAD);
		if (term_user && !countdown("logoff", 10L)) { /* 10 second countdown */
            terminate( /* hangUp == */ TRUE, TRUE);
        }
    }
    /* go back to home */
	changedir(cfg->homepath);
}

/************************************************************************/
/* upload()  menu level routine                                         */
/************************************************************************/
void upload(char c)
{
    long            transTime1, transTime2;
    label           filename;
    char            comments[64];
	char			ch, xtype, i;
    static int      wcount = 0;
    int             term_user = 0;

    if (!c)
        ch = tolower(iChar());
    else
        ch = c;

    xtype = strpos(ch, extrncmd);

    if (!xtype) {
        if (ch == '?')
            upDownMnu('U');
        else {
            mPrintf(" ?");
            if (!expert)
                upDownMnu('U');
        }
        return;
    } else {
		extrn = start_extrn;
		xtype--;
		for(i=0; i<xtype; i++) extrn = extrn->next;
		xtype++;

		if (!onConsole && extrn->ex_console) {
            doCR();
            mPrintf("Protocol is console or network only");
            return;
        } else {
			mPrintf("\b%s", extrn->ex_name);
        }
    }

    doCR();

	if (!extrn->ex_batch) {
        getNormStr("filename", filename, FILESIZE, ECHO);

        if (checkup(filename) == ERROR)
            return;

        if (strlen(filename))
            getString("comments", comments, 64, FALSE, TRUE, "");
        else
            return;
		term_user = getYesNo("Hang up after file transfer", 0);
    }

    if (!expert)
        nextblurb("wcup", &wcount, 1);

    doCR();

    if (getYesNo("Ready for file transfer", 0)) {
        batchinfo(FALSE);		/* reset file comments */
        time(&transTime1);  /* when did they start the Uload    */
		wxrcv(roomBuf.rbdirname, extrn->ex_batch ? "" : filename,
        xtype);
        time(&transTime2);  /* when did they get done           */

		if (cfg->accounting && !logBuf.lbflags.NOACCOUNT && !specialTime) {
            calc_trans(transTime1, transTime2, 1);
        }
		if (!extrn->ex_batch) {
            if (limitFlag && filexists(filename))
                hide(filename);

            if (filexists(filename)) {
                entercomment(filename, logBuf.lbname, comments);

                sprintf(msgBuf->mbtext, "%s upload of file %s in room %s]",
				extrn->ex_name, filename, roomBuf.rbname);

                trap(msgBuf->mbtext, T_UPLOAD);

                if (comments[0])
                    sprintf(msgBuf->mbtext, "%s uploaded by %s\n Comments: %s",
                    filename, logBuf.lbname, comments);
                else
                    sprintf(msgBuf->mbtext, "%s uploaded by %s", filename,
                    logBuf.lbname);

                specialMessage();
            }
        } else {
            sprintf(msgBuf->mbtext, "%s file upload in room %s]",
			extrn->ex_name, roomBuf.rbname);

            trap(msgBuf->mbtext, T_UPLOAD);

            if (batchinfo(TRUE))
                specialMessage();
        }
		if (term_user && !countdown("logoff", 10L))
            terminate( /* hangUp == */ TRUE, TRUE);
    }
	changedir(cfg->homepath);
}

/*
 * Up/Down menu
 */
void upDownMnu(char cmd)
{
    int             i;

    doCR();
    doCR();
	extrn = start_extrn;
	for (i = 0; i < strlen(extrncmd); i++) {
		if (!onConsole && extrn->ex_console) {
            continue;
		}
		else {
			mPrintf(" .%c%c>%s\n", cmd, extrn->ex_name[0],
			&(extrn->ex_name[1]));
		}
		extrn = extrn->next;
    }
    mPrintf(" .%c?> -- this list\n", cmd);
    doCR();
}

/* EOF */
