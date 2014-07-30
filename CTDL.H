/* -------------------------------------------------------------------- */
/*                              Ctdl.h                                  */
/*               #include file for all Citadel C files.                 */
/*              Now includes only #defines and structs.                 */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  HISTORY:
 *
 *  03/01/90  {zm}   Add anonymous rooms.
 *  03/15/90  {zm}   Add [title] name [surname] everywhere @ 30 chars.
 *  03/16/90  {zm}   Add route path and other new DragCit msg fields.
 *  03/19/90  FJM    Linted & partial cleanup
 *  04/01/90  FJM    Removed unused variables
 *  05/10/90  FJM    Fixed FileInfo.  Requires convert.
 *  09/21/90  FJM    Added #SYSOP, #CREDIT_NAME
 *  10/15/90  FJM    Added console/net only protocol flag to ext_prot
 *  12/09/90  FJM    Added color codes for screen & remote.
 *  12/20/90  JDW    Added #define ATARI_ST for porting...
 *  01/06/91  FJM    Added login to parm struct.
 *  01/13/91  FJM    Enlarged e_str to prevent overflow.
 *  01/28/91  FJM    Added SUCCESS define.
 *  02/05/91  FJM    New cfg.screen_save.
 *  05/15/91  BLJ    Added mfWord to mfilter struct
 *                   Removed cost1200, cost2400
 *  06/12/91  BLJ    Added bit flags for chat for user, trap to print file
 *                   cfg LAN access flag
 *  06/29/91  BLJ    Added room width to config (TCit compatability)
 *                   Added sound bit in userlog
 *  07/03/91  BLJ    Added Config data for Music and Sound
 *  07/08/91  BLJ    Added Max Sound Per user Flag
 *  08/21/91  BLJ    Added the chat_bell structure and config flag
 *  09/03/91  BLJ    Linted and clean-up
 *  09/12/91  BLJ    Added the new Network fields
 *  09/14/91  BLJ    Made some structures dynamically-linked lists
 *  11/16/91  BLJ    Added Software Net field and lengthened netpath
 *	04/11/92  BLJ	 Added tzrules structure
 *	04/20/92  BLJ	 Added bit flags to room structure
 *	06/21/92  BLJ	 Added MenuOk flag to config structure
 *	06/29/92  BLJ	 Added MenuLines to config structure
 * -------------------------------------------------------------------- */

#include "machine.h"

/*
** For reference the ANSI header files are:
**  assert.h
**  ctype.h
**  errno.h
**  float.h
**  limits.h
**  locale.h
**  math.h
**  setjmp.h
**  signal.h
**  stdarg.h
**  stddef.h
**  stdio.h
**  stdlib.h
**  string.h
**  time.h
**
** On the other hand, things like dos.h, bios.h, alloc.h etc.
** are not ANSI headers and reduce the portability of each module
** they appear in a great deal...
**
**      (jdw/gan)
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef ATARI_ST
#include <dir.h>
#endif

#define EOS     ((char)0)           /* End-Of-String define */
#undef toupper
#undef tolower

/* macro to translate setmem calls to the equivalent memset call */
#define setmem(p, n, c) memset(p, c, n)

typedef unsigned long  ulong ;   /* Makes it easier to format            */
typedef unsigned short ushort;   /* Makes it easier to format            */
typedef unsigned int   uint  ;   /* Makes it easier to format            */
typedef unsigned char  uchar ;   /* Makes it easier to format            */
typedef unsigned char  BOOL  ;   /* Makes it easier to format            */

#define FILESIZE       20        /* length of file names                 */
#define NAMESIZE       30        /* length of room names                 */

typedef char label[31];          /* must be NAMESIZE + 1                 */

/* -------------------------------------------------------------------- */
/*                      Stuff to size system with:                      */
/* -------------------------------------------------------------------- */

#define MAXROOMS       250      /* number of rooms allowed in system    */
#define MAXHALLS       64       /* number of halls allowed in system    */
#define MAXCRON        40       /* number of events in cron.cit         */
#define MAXEXTERN      40       /* number of externs in protocol.cit    */

#define MAXGROUPS      64       /* number of groups allowed in system   */
#define MAXTEXT        8192     /* cheating  (message buffer size)      */

#define MAXDIRS        6        /* number of directions                 */

/* -------------------------------------------------------------------- */
/* Citadel uses readTables() and writeTables() to write images          */
/* of the external variables in RAM to disk, and later restore          */
/* them.  The images are stored in Etc.dat and all the .TAB files.      */
/* Ctdl.Exe will automatically reconstruct these files the hard way     */
/* when necessary and write them out when finished.                     */
/* Etc.dat and the .TAB files are always destroyed after reading, to    */
/* minimize the possibility of reading out-of-date versions.  In        */
/* general, this technique works well and saves time and head-banging   */
/* upon bootup.  You should, however, note carefully the following      */
/* caution:                                                             */
/*  o  Whenever you change the declarations in Ctdl.h you should:       */
/*   -->  Destroy the current Etc.dat file.                             */
/*   -->  Recompile and reload Ctdl.Exe.                                */
/*   -->  Ctdl.Exe will automatically build new Etc.dat and tables.     */
/*                                                                      */
/* If you ignore this warning, little pixies will prick you in your     */
/* sleep for the rest of your life.                                     */
/* -------------------------------------------------------------------- */

                /* Let's begin by defining the configuration struct.    */
                /* This is part of the contents of Etc.tab              */
typedef struct config
{
    char compdate[20];
    char comptime[20];
    unsigned messagek;          /* how many K message table is..        */
    int      maxtext;           /* # of characters per message          */

    int maxfiles;               /* Max number of files per directory    */

    ulong callno;               /* how many calls the system has gotten */

    ulong oldest;               /* 32-bit ID# of first message in system*/
    ulong newest;               /* 32-bit ID# of last  message in system*/

    ulong mtoldest;             /* oldest message ID # in message table */

    label nodeTitle;            /* name of the system                   */

    label nodeRegion;           /* name of the system's City            */

    label nodeState;            /* name of the systems's State          */

    label nodeCntry;            /* name of the systems's Country        */

    label nodePhone;            /* The systems's Phone Number           */

    label SysOpRegion;          /* Configurable region                  */

    label SysOpCntry;           /* Configurable country                 */

    label f6pass;

    char  datestamp[64];        /* time date format string              */
    char  vdatestamp[64];       /* time date format string (verbose)    */

    char  modsetup[64];         /* Normal init string                   */
    label offhooksetup[20];
    char  dialsetup[64];        /* Dial out string                      */
    label dialpref;

    int   idle;                 /* how long we wait before we net       */
    int   screen_save;          /* delay before the screen saver starts */
    int   mdata;                /* 3f8 com 1,2f8 com 2, 3e8 com 3 */

    char  dumbmodem;
    char  initbaud;

    char homepath[64];          /* which path system files will be in   */
    char msgpath[64];           /* which path message file will be in   */
    char mailpath[64];          /* which path mail files are in         */
    char helppath[64];          /* path which help files live in        */
    char helppath_2[64];        /* Alternate path help files live in    */
    char temppath[64];          /* temporary files (networking) go here */
    char roompath[64];          /* where to look for room descriptions  */
    char trapfile[64];          /* where trap file goes                 */
    char aplpath[64];           /* which path applications will be in   */
    char printer[64];           /* where to put output at alt-p         */
    char sysop[64];             /* sysop's name                         */
    char credit_name[64];       /* what to call credit's (e.g. gold pieces) */
    char swapfile[64];          /* name of swap file                    */

    char newuserapp[13];        /* application to run for new users     */

    uchar width;                /* Default unlogged screen width        */
    uchar nulls;                /* Default unlogged # nulls             */

    char unlogtimeout;          /* how many idle mins unlogged user gets*/
    char timeout;               /* how many idle mins logged user gets  */

    int  oldcount;              /* how many message new to new users    */
	uchar MenuOk;				/* SysOp allows Menus?					*/
	uchar MenuLines;			/* Min # of lines for Menu display		*/
	uchar Stickyrm; 			/* Sysop config Stickyrm for each user	*/
	ulong MEMFREE;				/* SysOp config Memory free (SuperShell */

    uchar attr;                 /* color of text displayed on screen    */
    uchar wattr;                /* color of text displayed on window    */
    uchar cattr;                /* color of text displayed on window    */
    uchar battr;                /* color of text screen border lines    */
    uchar uttr;                 /* color of text displayed in underline */

    long catLoc;                /* where to begin writing next message  */

    int MAXLOGTAB;              /* number of log entries supported      */
    int nmessages;              /* # of messages to be stored in table  */

    char maxwarn;               /* # times to warn user before hangup   */

    float unlogbal  ;           /* # credits unlogged users get         */
    float newbal    ;           /* # credits new users get              */
    float maxbalance;           /* maximum amount of credits user gets  */

    char trapit[16];            /* which events are logged              */
    char user[5];               /* #user default status                 */

    uchar MessageRoom;          /* max messages per room per call       */

    uchar nopwecho;             /* what to echo initials & pw at login  */

    char  extcmd[20];           /* command character list               */

    unsigned ad_chance;         /* chance 0-100% of an ad appearing     */

    unsigned ad_time;           /* time before random ad appears        */

    unsigned subhubs     : 2;   /* provision for special g)oto looping  */
    unsigned kill        : 1;   /* TRUE if user can kill own messages   */
    unsigned offhook     : 1;   /* TRUE to go off-hook when 'L' is hit  */

    unsigned bios        : 1;   /* TRUE to use BIOS calls for screen I/O*/
    unsigned noChat      : 1;   /* TRUE to suppress chat attempts       */
    unsigned noBells     : 1;   /* TRUE to supress all bells            */
    unsigned roomtell    : 1;   /* display room descriptions?           */

    unsigned loginstats    : 1;   /* display log-in statistics?           */
    unsigned moderate      : 1;   /* can aides see moderated messages?    */
    unsigned unlogEnterOk  : 1;   /* TRUE if OK to enter messages anon    */
    unsigned unlogReadOk   : 1;   /* TRUE if unlogged folks can read mess */

    unsigned nonAideRoomOk : 1;   /* TRUE general folks can make rooms    */
    unsigned uppercase     : 1;   /* Default is upper case?               */
    unsigned linefeeds     : 1;   /* Default linefeeds given?             */
    unsigned tabs          : 1;   /* Default do we have tabs?             */

    unsigned forcelogin    : 1;   /* Automatically log someone in?        */
    unsigned accounting    : 1;   /* is accounting enabled on the system? */
    unsigned surnames      : 1;   /* are the surnames on?                 */
    unsigned aidehall      : 1;   /* Aides mess with halls.               */

    unsigned readluser     : 1;   /* Read Limited Access Userlog?         */
    unsigned readuser      : 1;   /* Read Userlog?                        */
    unsigned aidereaduser  : 1;   /* Aides Read Userlog?                  */
    unsigned entersur      : 1;   /* Let users enter their own surname?   */

    unsigned netsurname    : 1;   /* Display networked surnames?          */
    unsigned private       : 5;   /* Private type                         */
    unsigned dummy         : 2;   /* unused fields                        */

    unsigned ibmroom       : 1;   /* Do we show rooms with IBM Extended?  */
    unsigned wideroom      : 1;   /* Show rooms in two columns ?          */
    unsigned LAN           : 1;   /* Use with LAN?                        */
    unsigned Sound         : 1;   /* Can we hear the sound on the console */
    unsigned ChatTab       : 1;   /* Use the Chat Table ?                 */
    unsigned dummy2        : 3;   /* unused fields                        */

    unsigned MaxSound;            /* max sound entries per visit          */
    unsigned pace;                /* pacing rate                          */
    char lastExtern;
};


/* -------------------------------------------------------------------- */
/*                           Misc. #defines                             */
/* -------------------------------------------------------------------- */
/* values for functions to return: */

#define TRUE            1
#define FALSE           0
#define ERROR          -1
#define SUCCESS         0
#define SAMESTRING      0       /* value for strcmp() & friend          */


/* -------------------------------------------------------------------- */
/*                              Room data                               */
/* -------------------------------------------------------------------- */
#define LOBBY           0       /* Lobby> is >always< room 0.           */
#define MAILROOM        1       /* Mail>  is >always< room 1.           */
#define AIDEROOM        2       /* Aide)  is >always< room 2.           */
#define DUMP            3       /* Dump>  is >always< room 3.           */
#define DUNGEON         4       /* ....>  is >always< room 4.           */

#define MAXGEN        128       /* six bits of generation => 64 of them */

#define OLDNEW          0       /* List both unread and read rooms      */
#define NEWRMS          1       /* Only list unread rooms.              */
#define WINDWS          2       /* List rooms which are windows         */
#define DIRRMS          4       /* Directory rooms..                    */
#define MAILRM          5       /* Mail rooms......                     */
#define APLRMS          6       /* Aplication rooms                     */
#define STATRM          7       /* Status of Room                       */
#define LIMRMS          8       /* Limited Access Rooms                 */
#define OLDRMS          9       /* Old Rooms                            */
#define XCLRMS          10      /* Xcluded Rooms                        */
#define SHRDRM          11      /* Shared rooms                         */
#define ANONRM          12      /* Anonymous rooms                      */


typedef struct parms   /* all defaults are 0   */
{
    char *login;                /* user login string                    */
    char *user;                 /* user login name                      */
    unsigned long baud;         /* initial baud rate                    */
    unsigned pace;              /* pacing delay on modem transmit       */
    unsigned dv          : 1;   /* TRUE to use BIOS calls for screen I/O*/
    unsigned door        : 1;   /* run as door from another BBS         */
    unsigned ems         : 1;   /* use EMS for overlays                 */
    unsigned ext         : 1;   /* use extended memory for overlays     */
    unsigned events      : 1;   /* just run cron events                 */
	unsigned config 	 : 1;	/* just reconfig						*/
	unsigned unused 	 : 2;	/* extras								*/
};

typedef struct rflags					/* Room flags							*/
{
    unsigned INUSE    : 1;      /* Room in use?                         */
    unsigned PUBLIC   : 1;      /* Room public?                         */
    unsigned MSDOSDIR : 1;      /* Room directory?                      */
    unsigned PERMROOM : 1;      /* Room permanent?                      */
    unsigned GROUPONLY: 1;      /* Room Owned by a group?               */
    unsigned READONLY : 1;      /* Room Read only?                      */
    unsigned DOWNONLY : 1;      /* Room download only?                  */
    unsigned SHARED   : 1;      /* Room Shared?                         */
    unsigned MODERATED: 1;      /* Room Moderated?                      */
    unsigned APLIC    : 1;      /* Room have application?               */
    unsigned BIO      : 1;      /* By invintation only..                */
    unsigned UPONLY   : 1;      /* Upload only                          */
    unsigned WINDOW   : 1;      /* Window into other nav system         */
    unsigned ANONYMOUS: 1;      /* Room Anonymous?                      */
    unsigned UNUSED   : 2;      /* extra space, formerly 3 bits..       */
} ;

typedef struct rTable           /* The summation of a room              */
{
    uchar         rtgen;        /* generation # of room                 */
    uchar         grpno;        /* group # which owns it                */
    uchar         grpgen;       /* generation # of group which owns it  */
	uchar		  mult_grp; 	/* Multiple group flag					*/
	struct rflags rtflags;		/* public/private flag etc				*/
    label         rtname;       /* name of room                         */
} ;                             /* And see ROOMA.C for declaration      */

typedef struct aRoom                /* The appearance of a room:            */
{
    uchar           rbgen;          /* generation # of room                 */
    struct rflags   rbflags;        /* same bits as flags above             */
    label           rbname;         /* name of room                         */
    char            rbdirname[64];  /* user directory for this room's files */
    char            rbroomtell[13]; /* filename for room description        */
    char            rbaplic[13];    /* filename for room aplication         */
    uchar           rbgrpgen;       /* group gen# for Limited Access rooms  */
    uchar           rbgrpno;        /* group # which owns it                */
    int             rbmodhash;      /* moderator's hash, not used yet       */
	uchar			password[4];	/* Room password, used with BIO flag	*/
	unsigned		autoapp  : 1;	/* Automatically run Application ?		*/
	unsigned		lockpass : 1;	/* Lock the password?					*/
	unsigned		lockapp  : 1;	/* password required for application?	*/
	unsigned		lockroom : 1;	/* password required for room entry?	*/
	unsigned		mult_grp : 1;	/* More than one group for this room?	*/
	unsigned		unused	 : 11;	/* Unused bits							*/
    char            descript[80];   /* for .kv[...]                         */
    char            netID[20];      /* Network ID for room                  */
};

/* -------------------------------------------------------------------- */
/*                      group & accounting stuff                        */
/* -------------------------------------------------------------------- */
#define MAXGROUPGEN 128

typedef struct accounting
{
    char days[7];        /* Which Days this group can log-in        */
    char hours[24];      /* Which hours of the day group can log-in */
    char special[24];    /* Which hours are special                 */
    char grpslot;        /* The Actual Group Number                 */
    float dayinc;        /* Maximum time add per day                */
    float priority;      /* What is the priority for this group     */
    float maxbal;        /* This groups maxtime on the system       */
    float dlmult;        /* Multiplyer for Downloads                */
    float ulmult;        /* Multiplyer for Uploads                  */
    struct accounting *next;   /* Pointer to next structure         */
    unsigned have_acc: 1;/* Does group/user have accounting?        */
};

/* -------------------------------------------------------------------- */
/*                          group stuff                                 */
/* -------------------------------------------------------------------- */
typedef struct groupBuffer       /* 7296 bytes */
{
    struct group_buffer          /* 114 bytes */
    {
        label    groupname  ;      /* Name of the group                 */
        uchar    groupgen   ;      /* Group generation #.               */
        char     desc[80]   ;      /* Description of group              */
        unsigned g_inuse : 1;      /* Is group inuse?                   */
        unsigned lockout : 1;      /* Sysop access needed for changes   */
        unsigned hidden  : 1;      /* hidden to all, unless specified
                                      by name or in group               */
        unsigned dfault  : 1;      /* Is it a default group?            */
        unsigned UNUSED  :12;
    } group[MAXGROUPS];
} ;

/* -------------------------------------------------------------------- */
/*                             hall stuff                               */
/* -------------------------------------------------------------------- */
typedef struct hallBuffer            /* 35072 bytes */
{
    struct hall_buffer
    {
        label    hallname;           /* Name of our hall                */
        uchar    grpno;              /* owning group's #                */
        unsigned h_inuse    : 1;     /* Is hall inuse?                  */
        unsigned owned      : 1;     /* Is this hall owned?             */
        unsigned described  : 1;     /* described?                      */
        unsigned dmaint     : 1;     /* default maintenance             */
		unsigned mult_grp	: 1;	 /* Multiple group access flag		*/
		unsigned unused 	: 11;	 /* left over bits ...				*/
        struct
        {
            unsigned inhall : 1;     /* is room in this hall?           */
            unsigned window : 1;     /* is room a window into hallway?  */
            unsigned UNUSED :14;
        } hroomflags[MAXROOMS];
        uchar    mdefault;           /* hall default maintenance hall   */
        char     htell[13];          /* Hall description                */
    } hall[MAXHALLS];
} ;

/* -------------------------------------------------------------------- */
/*							   hall multiple group stuff				*/
/* -------------------------------------------------------------------- */
typedef struct hallGroup			/* 4096 Bytes */
{
	struct hall_groups
    {
		struct groupsh				/* 64 Bytes */
		{
			uchar grpgen;			/* owning group's gen #             */
		} grp[MAXGROUPS];
	} hall[MAXHALLS];
} ;

/* -------------------------------------------------------------------- */
/*							   room multiple group stuff				*/
/* -------------------------------------------------------------------- */
typedef struct roomGroup			/* 16000 Bytes */
{
	struct room_groups
    {
		struct groupsr				/* 64 Bytes */
		{
			uchar grpgen;			/* owning group's gen #            */
		} grp[MAXGROUPS];
	} room[MAXROOMS];
} ;

/* -------------------------------------------------------------------- */
/*                            userlog stuff                             */
/* -------------------------------------------------------------------- */
typedef struct lflags			/* Flags for person in log				*/
{
    unsigned L_INUSE   : 1;     /* Is this slot in use?                 */
    unsigned UCMASK    : 1;     /* Uppercase?                           */
    unsigned LFMASK    : 1;     /* Linefeeds?                           */
    unsigned EXPERT    : 1;     /* Expert?                              */
    unsigned AIDE      : 1;     /* Aide?                                */
    unsigned TABS      : 1;     /* Tabs?                                */
    unsigned OLDTOO    : 1;     /* Print out last oldmessage on  N>ew?  */
    unsigned PROBLEM   : 1;     /* Twit bit                             */
    unsigned UNLISTED  : 1;     /* List in Userlog?                     */
    unsigned PERMANENT : 1;     /* Permanent log entry?                 */
    unsigned SYSOP     : 1;     /* Sysop special access.                */
    unsigned NODE      : 1;     /* Are they a node?                     */
    unsigned NETUSER   : 1;     /* Can they send network E-mail         */
    unsigned NOACCOUNT : 1;     /* Accounting disabled for this person? */
    unsigned NOMAIL    : 1;     /* Can user send exclusive mail?        */
    unsigned ROOMTELL  : 1;     /* Room-descriptions?                   */
} ;

#define MAXVISIT       16       /* #visits we remember old newestLo for */

typedef struct lbroomdata
{
    uchar    lbgen;             /* which rooms are visible?             */
    unsigned xclude : 1;        /* is room excluded?                    */
    unsigned mail   : 1;        /* private mail in this room?           */
	unsigned lvisit : 5;		/* lastvisit pointer 1 - 16 			*/
    unsigned UNUSED : 8;
};

typedef struct logBuffer        /* The appearance of a user:            */
{
    label         lbname;       /* caller's name                        */
    label         lbin;         /* caller's initials                    */
    label         lbpw;         /* caller's password                    */
    label         forward;      /* forwarding address                   */
    label         surname;      /* user's surname                       */
    label         title;        /* user's title                         */
    int           hallhash;     /* default hallway hash                 */
    uchar         lbnulls;      /* #Nulls, lCase, lFeeds                */
    uchar         lbwidth;      /* terminal width                       */
    long          calltime;     /* Time/date stamp of last call         */
    ulong         callno;       /* Which caller # they were             */
    float         credits;      /* Credits for accounting               */
    uchar     groups[MAXGROUPS];/* Group generation # 16 of them        */
    struct lbroomdata lbroom[MAXROOMS]; /* Data for each room in LogBuf */
    ulong     lbvisit[MAXVISIT];/* newestLo for this and 15 prev.visits */

	uchar	 lastRoom;			/* room they were in last call			*/
    uchar    lastHall;          /* hall they were in last call          */

    uchar    linesScreen;       /* the number of lines a caller has per */
                                /* screen */
    char     tty[10];           /* type of TTY                          */

    struct lflags lbflags;      /* UCMASK, LFMASK, EXPERT, AIDE, INUSE  */
    unsigned DUNGEONED : 1;     /* dungeoned user?                      */
    unsigned MSGAIDE   : 1;     /* message only aide?                   */
    unsigned FORtOnODE : 1;     /* forward to node?                     */
    unsigned NEXTHALL  : 1;     /* Auto Next Hallway?                   */
    unsigned OTHERNAV  : 2;     /* other nav type                       */
    unsigned VERIFIED  : 1;     /* has the user been verified?          */
    unsigned SURNAMLOK : 1;     /* surname locked?                      */
        /* new for version 13 FredCit   */
    unsigned IBMGRAPH  : 1;     /* IBM Graphics characters ?            */
        /* New for version 1.4 CenCit   */
	unsigned NOCHAT    : 1; 	/* User can chat or not 				*/
	unsigned TRAPIT    : 1; 	/* Trap user output to the print file	*/
	unsigned READER    : 1; 	/* user allowed to use off-line reader	*/
        /* New for version 1.5 CenCit   */
    unsigned SOUND     : 1;     /* user has sound capability            */
		/* New for version 1.27 CenCit	*/
	unsigned TITLELOK  : 1; 	/* title Locked?						*/
	unsigned STICKYRM  : 1; 	/* return to same room/hallway? 		*/
	unsigned UNUSED    : 9; 	/* unused remaining bits				*/
        /* New for version 1.11 CenCit  */
	unsigned char MAXSOUND; 	/* Max Sound per user per day			*/
};

typedef struct lTable           /* Summation of a person:               */
{
    int   ltpwhash;             /* hash of password                     */
    int   ltinhash;             /* hash of initials                     */
    int   ltnmhash;             /* hash of name                         */
    int   ltlogSlot;            /* location in userlog.buf              */
    ulong ltcallno;             /* Which caller # they were             */
    char  permanent;            /* permanent log entry.                 */
};

/* -------------------------------------------------------------------- */
/*                      message stuff                                   */
/* -------------------------------------------------------------------- */
#define HELD           3

#define ATTR_RECEIVED  1
#define ATTR_REPLY     2
#define ATTR_MADEVIS   4

typedef struct msgflags 		/* Message attribute flags				*/
{                               /* included in msg buffer * msg table   */
    unsigned MAIL     : 1;
    unsigned RECEIVED : 1;      /* Was it received?                     */
    unsigned REPLY    : 1;      /* Reply sent?                          */
    unsigned PROBLEM  : 1;      /* Problem User message?                */
    unsigned MADEVIS  : 1;      /* Made Visible?                        */
    unsigned LIMITED  : 1;      /* Limited-access message?              */
    unsigned MODERATED: 1;      /* Moderated message?                   */
    unsigned RELEASED : 1;      /* Released by moderator?               */
    unsigned COPY     : 1;      /* Is message a copy?                   */
    unsigned NET      : 1;      /* Networked mail en-route?             */
    unsigned UNUSED   : 6;
};

typedef struct msgB             /* This is what a msg looks like         */
{
    uchar mbtext[MAXTEXT];      /* buffer text is edited in              */

    long  mbheadLoc;            /* Start of message                      */
    uchar mbroomno;             /* room # of message                     */
    uchar mbattr;               /* 3 attribute bits                      */

    label mbauth ;              /* name of author                    (A) */
    label mbcopy ;              /* message ID of copy this msg is of     */
    label mbfwd  ;              /* forwarding address                    */
    label mbgroup;              /* group name for limited access msg (G) */
    label mbtime ;              /* creation time                     (D) */
    label mbId   ;              /* local number of message               */
    label mboname;              /* name of origin system             (O) */
    label mboreg ;              /*  "        "    city               (o) */
    label mbostate;             /*  "        "    state              (Q) */
    label mbocont;              /*  "        "    country            (U) */
    label mbophone;             /*  "        "    phone number       (H) */
    label mbsoftware;           /*  Software version of orig. node   (s) */
    label mbreply;              /* message ID of message replied to  (I) */
    label mbnetID;              /* Netwide ID of room                (W) */
    label mbroom ;              /* creation room                     (R) */
    label mbsrcId;              /* message ID on system of origin    (S) */
    label mbto   ;              /* private message to                (T) */
    label mbx    ;              /* twit/moderated message header     (X) */
    label mbtitle;              /* title                             (N) */
    label mbsur  ;              /* surname                           (n) */
    char  mblink[64];           /* file linked message                   */
    label mbzip  ;              /* name of destination system        (Z) */
    label mbrzip ;              /*   "          "      city          (z) */
    label mbszip ;              /*   "          "      state         (q) */
    label mbczip ;              /*   "          "      country       (u) */
    label mbzphone;             /*   "          "      phone number  (h) */
    label mbtreg;               /* SysOp configurable region         (J) */
    label mbtcont;              /* SysOp configurable country        (j) */
    char  mbfpath[256];         /* where did the message come from?  (P) */
    char  mbtpath[256];         /* forced routing via path           (p) */
} ;                             /* declaration in MSG.C                  */

/* The following is the message filter table.. */

typedef struct mfilter
{
    unsigned mfMai   : 1;
    unsigned mfLim   : 1;
    unsigned mfPub   : 1;
    label    mfUser;
    label    mfWord;
    label    mfGroup;
} ;

/* values for showMess routine */
#define NEWoNLY         0
#define OLDaNDnEW       1
#define OLDoNLY         2

/* -------------------------------------------------------------------- */
/*                      message table stuff                             */
/* -------------------------------------------------------------------- */
typedef struct messagetable
{
    struct msgflags mtmsgflags; /* every message gets flags             */
    long     mtmsgLoc;          /* offset where message starts          */
    ushort   mtoffset;          /* msg# - mtoffset == actual msg no     */
                                /* to determine if copy has scrolled    */
    uchar    mtroomno;          /* room # of message                    */
    int      mttohash;          /* hash of recipient or group           */
    int      mtauthhash;        /* hash of author of message            */
    int      mtfwdhash;         /* hash of forwardee                    */
    long     mtomesg;           /* this and the next are to check for   */
                                /* duplicate messages                   */
    int      mtorigin;          /* hash of the origin system's name     */
};

/* -------------------------------------------------------------------- */
/*          TalleyBuffer Stuff (message counts for each room)           */
/* -------------------------------------------------------------------- */
typedef struct talleyBuffer {
    struct {
        ushort total;           /* total # message in room (sysops & aides) */
        ushort messages;        /* # message user can read */
        ushort new;             /* # new message user can read */
    } room[MAXROOMS];
};

/* -------------------------------------------------------------------- */
/*                       node stuff                                     */
/* -------------------------------------------------------------------- */
typedef struct nodest
{
    int   ndbaud;
    int   nddialto;
    int   ndredial;
    int   ndwaitto;
    int   ndexpire;
	uint  ndpacing;

    label ndname;
    label ndregion;
    label ndprotocol;
    label ndphone;
    label ndmailtmp;
	char  ndScriptSlave[13];
	char  ndScriptMaster[13];
    ulong roomoff;

    char  ndlogin[240];
};

typedef struct ndgroups
{
    label here;
    label there;
    struct ndgroups *next;
};

typedef struct ndrooms
{
    label here;
    label there;
    struct ndrooms *next;
};


/* -------------------------------------------------------------------- */
/*                      modem stuff                                     */
/* -------------------------------------------------------------------- */
/* fiddle factor to timeout on no input: */
#define HITIMEOUT       10000   /* processor dependent, see MODEM.C     */

#define NEWCARRIER   0x01       /* returned to main prog on login       */

#define MODEM           0       /* current user of system is            */
#define CONSOLE         1       /* one of these                         */

/*  output XON/XOFF etc flag... */
#define OUTOK           0       /* normal output                        */
#define OUTPAUSE        1       /* a pause has been requested           */
#define OUTNEXT         2       /* quit this message, get the next      */
#define OUTSKIP         3       /* stop current process                 */
#define OUTPARAGRAPH    4       /* skip to next paragraph               */
#define IMPERVIOUS      5       /* make current output unstoppable      */

#define NEITHER         0       /* don't echo input at all              */
#define CALLER          1       /* echo to caller only --passwords etc  */
#define BOTH            2       /* echo to caller and console both      */

#define NO_ECHO         0       /* Echo input as .'s                    */
#define ECHO            1       /* Echo input   (getString())           */


/* -------------------------------------------------------------------- */
/*                           Trap File Stuff                            */
/* -------------------------------------------------------------------- */
#define T_ALL        0             /* All events are logged             */
#define T_CARRIER    1             /* carrier detect & loss are logged  */
#define T_LOGIN      2             /* login, logout, new users          */
#define T_NEWROOM    3             /* new room creations are logged     */
#define T_PROBLEM    4             /* problem user messages are logged  */
#define T_CHAT       5             /* chat requests are logged          */
#define T_PASSWORD   6             /* password changes are logged       */
#define T_AIDE       7             /* aide functions are logged         */
#define T_SYSOP      8             /* sysop functions are logged        */
#define T_UPLOAD     9             /* uploads are logged                */
#define T_DOWNLOAD   10            /* all downloads are logged          */
#define T_ERROR      11            /* internal errors are logged        */
#define T_NETWORK    12            /* network events are logged         */
#define T_ACCOUNT    13            /* accounting information is logged  */
#define T_APPLIC     14            /* application executions are logged */

/* -------------------------------------------------------------------- */
/*                        Default User Stuff                            */
/* -------------------------------------------------------------------- */
#define D_PROBLEM     0            /* Everyone defaults to problem user */
#define D_PERMANENT   1            /* Everyone gets perm log entry      */
#define D_NOACCOUNT   2            /* Everyone has accounting disabled  */
#define D_NETWORK     3            /* Everyone's a network user         */
#define D_NOMAIL      4            /* Everyone sends no mail            */

/* -------------------------------------------------------------------- */
/*  External stuff                                                      */
/* -------------------------------------------------------------------- */
/*  External protocol structure  */
typedef struct ext_prot
{
    label ex_name;          /* protocol name, first char is command key */
    char  ex_rcv[40];       /* recieve command                          */
    char  ex_snd[40];       /* send command                             */
    char  ex_batch;         /* batch protocol flag                      */
    int   ex_block;         /* block size, 0 for unknown/variable       */
    int   ex_console;       /* console/net only protocol flag           */
    struct ext_prot *next;  /* pointer to next protocol                 */
};

/*  External editor structure  */
typedef struct ext_editor
{
    label ed_name;
    char  ed_cmd[30];
    char  ed_local;
    struct ext_editor *next;
};

/* external commands structure - FJM */
typedef struct ext_command
{
    label name;
    char command[40];
    char local;
    struct ext_command *next;
};

/*  Chat Sound structure - BLJ */
typedef struct ChatSound
{
    char type;
    int hi;
    int lo;
    int incr;
    int dly1;
    int dly2;
};

/*  Chat and Bells on or off structure - BLJ */
typedef struct ChatBell
{
    struct {
        char hours[96];   /* valid 1/4 hours           */
    }days[7];             /* for each day of the week  */
};


/*  External other command structure  */
typedef struct ext_other
{
    label    eo_name1;
    label    eo_name2;
    char     eo_cmd1;
    char     eo_cmd2;
    char     eo_cmd[40];
    unsigned DIR    :1;
    unsigned SYSOP  :1;
    unsigned AIDE   :1;
    unsigned CON    :1;
    unsigned UNUSED :12;
};

/* -------------------------------------------------------------------- */
/*                       cron stuff                                     */
/* -------------------------------------------------------------------- */
typedef struct event
{
    char   e_type;        /* Event type (network, shell, ext)       */
    label  e_str;         /* nodename, shell command, ext           */
    char   e_hours[96];   /* valid 1/4 hours for event              */
    char   e_days[7];     /* valid days for event                   */
    int    e_redo;        /* minutes before it will redo this event */
    int    e_retry;       /* minutes before it will retry           */
    char   e_zap;         /* Zap cron event flag                    */

    long l_sucess;     /* last time it has sucsess                  */
    long l_try;        /* last time it tryed                        */
    struct event *next;
};

#define CRON_TIMEOUT   0   /* why do_cron() is called                   */
#define CRON_LOGOUT    1
#define CRON_PROMPT    2

/* -------------------------------------------------------------------- */
/* Infofile nonsense                                                    */
/* -------------------------------------------------------------------- */
typedef struct fInfo						  /*  fileinfo data structure */
{
    char fn[13];
    label uploader;
    char comment[65];
};

/* -------------------------------------------------------------------- */
/*  File directory structure.                                           */
/* -------------------------------------------------------------------- */
typedef struct fDir
{
    char entry[90];
};

/* -------------------------------------------------------------------- */
/*	TimeZone structure. 												*/
/* -------------------------------------------------------------------- */
typedef struct tzrules
{
	int from;
	int to;
	char typ[11];
	char month;
	char on[8];
	char at_hr;
	char at_min;
	char save_hr;
	char save_min;
	char ltr;
	char done;
};

/* -------------------------------------------------------------------- */
/*  What we are doing..                                                 */
/* -------------------------------------------------------------------- */
#define DUNO        0
#define MAINMENU    1
#define SYSOPMENU   2
#define PROMPT      3
#define MESSAGETEXT 4
#define DIALOUT     5
#define NETWORKING  6

/* -------------------------------------------------------------------- */
/*  TERMCAP definitions                                                 */
/* -------------------------------------------------------------------- */
#define TERM_NORMAL     '0'
#define TERM_BLINK      '1'
#define TERM_REVERSE    '2'
#define TERM_BOLD       '3'
#define TERM_UNDERLINE  '4'

/* foreground colors */
#define TERM_BLACK_FG    'a'
#define TERM_RED_FG      'b'
#define TERM_GREEN_FG    'c'
#define TERM_YELLOW_FG   'd'
#define TERM_BLUE_FG     'e'
#define TERM_MAGENTA_FG  'f'
#define TERM_CYAN_FG     'g'
#define TERM_WHITE_FG    'h'

/* light foreground colors - not used, reserved */
#define TERM_LBLACK_FG   'i'
#define TERM_LRED_FG     'j'
#define TERM_LGREEN_FG   'k'
#define TERM_LYELLOW_FG  'l'
#define TERM_LBLUE_FG    'm'
#define TERM_LMAGENTA_FG 'n'
#define TERM_LCYAN_FG    'o'
#define TERM_LWHITE_FG   'p'

/* background colors */
#define TERM_BLACK_BG    'A'
#define TERM_RED_BG      'B'
#define TERM_GREEN_BG    'C'
#define TERM_YELLOW_BG   'D'
#define TERM_BLUE_BG     'E'
#define TERM_MAGENTA_BG  'F'
#define TERM_CYAN_BG     'G'
#define TERM_WHITE_BG    'H'
/* -------------------------------------------------------------------- */
/*  These flags are for special action during message retrvial          */
/* -------------------------------------------------------------------- */
#define              NO_SPECIAL     0   /* dont do anything with message */
#define              PULL_IT        1   /* kill the message */
#define              MARK_IT        2   /* mark the message to be moved */
#define              REVERSE_READ   3   /* change read direction */
#define              COPY_IT        4   /* copy message to file */

/* -------------------------------------------------------------------- */
/*  Other includes                                                      */
/* -------------------------------------------------------------------- */
#include "applic.h"

/* EOF */
