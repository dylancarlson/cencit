/* -------------------------------------------------------------------- */
/*  All the global variables in all of citadel. Put here to             */
/*  make moving functions around a hell of a lot easier.                */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  History:
 *
 *  03/07/90    {zm}    Add 'softname' from VERSION.C
 *  03/12/90    {zm}    Add #DEFAULT_GROUP to external.cit
 *  03/12/90    FJM     Added IBM Graphics character translation
 *  03/19/90    FJM     Linted & partial cleanup
 *                      Changed my comptime/compdate thingy.  Now always
 *                      recompile.
 *  06/16/90    FJM     Made IBM Graphics characters a seperate option.
 *  07/22/90    FJM     Added 0 marker at end of baud rate list
 *  11/18/90    FJM     Deleted locally used variable infosize.
 *  11/23/90    FJM     Changed my full name to just initials.
 *  07/03/91    BLJ     Added Global data for Music and Sound
 *  09/14/91    BLJ     Added structure pointers for Dynamically Linked Lists
 *  09/16/91    BLJ     Added char release for Alpha, Beta, or Production
 *	05/25/92	BLJ 	Changed structure arrays to pointers to save memory
 *	07/01/92	BLJ 	Added multiple group access for rooms and halls
 *
 * -------------------------------------------------------------------- */

#define FILT_LEN  256

#ifdef MISC
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* FILES */
FILE *msgfl;			/* msg file descriptor			   */
FILE *printfile;		/* printer.out					   */
FILE *trapfl;			/* trap file descriptor 		   */
FILE *upfd;
FILE *aideFl = NULL;	/* a temp file for aide messages   */
FILE *hallfl;			/* Hall file descriptor 		   */
FILE *grpfl;			/* Group file descriptor		   */
FILE *roomfl;			/* Room file descriptor 		   */
FILE *logfl;			/* log file descriptor			   */
FILE *hallgrpfl;		/* multiple groups/hall descriptor */
FILE *roomgrpfl;		/* multiple groups/room descriptor */

/* STRUCTURES */
label editcmd;                     /* External editor commands         */
struct aRoom roomBuf;              /* Room buffer                      */
struct accounting far *accountBuf; /* Accounting buffer 			   */
struct accounting *start_acct;     /* Accounting buffer ptr to first   */
struct accounting thisAccount;     /* Groupdata used for current user  */
struct config far *cfg; 		   /* A buncha variables			   */
struct groupBuffer far *grpBuf;    /* Group data goes here			   */
struct logBuffer lBuf;             /* Temp buffer...........           */
struct logBuffer logBuf;           /* Log buffer of a person           */
struct mfilter mf;                 /* Message filter                   */
struct msginfo mesginfo;           /* User/room for message from aplic */
struct parms parm;                 /* command line parms               */
struct talleyBuffer talleyBuf;     /* message talley for rooms         */

struct ext_editor far *edit;	   /* External editor commands		   */
struct ext_editor *start_edit;     /* External editor commands         */
struct fDir *filedir;              /* pointer for filedir struct       */
struct fInfo *fileinfo;            /* pointer for file-info struct     */
struct tzrules tzr[4];			   /* Timezone rules structure		   */
struct hallBuffer far *hallBuf;    /* Hall stuff right here 		   */
struct hallGroup far *hallgrp;	   /* Multiple groups per hall		   */
struct roomGroup far *roomgrp;	   /* Multiple groups per room		   */
struct lTable far *logTab;         /* RAM index of pippuls             */
struct messagetable huge *msgTab;  /* THE message table                */
struct msgB far *msgBuf2;		   /* The Old-message buffer		   */
struct msgB far *msgBuf;		   /* The -sole- message buffer 	   */
struct nodest far *node;		   /* node buffer					   */
struct ndgroups far *ndgrp, *start_ndgrp;
struct ndrooms far *ndroom, *start_ndroom;

#endif

#ifdef MISC2
char extrncmd[MAXEXTERN];            /* External command letters     */
struct ext_prot *extrn,*start_extrn; /* External protocols buffer    */
struct ext_command *extCmd;          /* External commands buffer     */
struct ext_command *start_extCmd;    /* External commands buffer     */
struct ChatSound ChatSnd;            /* Chat Sound Parmeters         */
struct ChatBell far *chat_bell; 	 /* Chat & Bell on/off times	 */
struct rTable far *roomTab; 		 /* RAM index of rooms			 */
label default_group;                 /* Default for net messages     */
struct event far *events, *start_event; /* CRON Events					*/

/* Function pointers */
void (*charattr) (char, uchar);
void (*stringattr) (uint, char *, uchar);

/* OTHER */
char specialTime;       /* Is it a "special" hour           */
float startbalance;     /* Number of credits at log-in      */
long lasttime;          /* timestamp of last update         */
char readMessage = TRUE;
char ExitToMsdos = FALSE;   /* bring system down         */
char sysReq = FALSE;        /* SysOp wants system        */
char *confirm = "Confirm";
char mailFlag;          /* Sending private mail?        */
char oldFlag;           /* Entering old message?        */
char limitFlag;         /* Entering Limited-Access      */
char chatReq = FALSE;   /* Did they want to chat?       */
char dowhat = DUNO;     /* What is calling iChar        */
int xpd, duplic;
char duplicate, expired;	/* count duplicates and expired   */
char netError = FALSE;		/* save net error message?		  */
unsigned char thisHall; 	/* hall we're in                  */
int thisLog;				/* entry currently in logBuf	  */
int thisRoom = LOBBY;		/* Current room 				  */
char etcpath[64];			/* Path etc.dat and ctdl.exe in   */
int thisSlot;				/* logTab slot logBuf found via   */
char loggedIn = FALSE;		/* Global have-caller flag		  */
char prevChar;				/* for EOLN/EOParagraph stuff	  */
char expert;				/* true to suppress hints etc.	  */
char termUpper; 			/* uppercase only flag			  */
char termLF;				/* LF-after-CR flag 			  */
char aide;					/* aide privileges flag 		  */
char sysop; 				/* sysop privileges flag		  */
char twit;					/* Problem user?				  */
char unlisted;				/* List in userlog? 			  */
char termTab;				/* Tabs 						  */
char oldToo;				/* Send last old on new request?  */
char roomtell;				/* Print room descriptions? 	  */
char Sound; 				/* Sound Flag					  */
char drv_diff;				/* Drive difference (LAN Phase I) */
char ctdlConfig = TRUE; 	/* To bypass the room passwords   */
unsigned char linesScreen;  /* Lines per Screen or No pause   */
unsigned char termWidth;	/* width to format output to	  */
unsigned char termNulls;	/* # nulls to send at EOLN		  */
int people; 				/* this is the old slot 		  */
char sysopNew = FALSE;
long uptimestamp;
long logtimestamp;
long conntimestamp;
ulong transmitted;            /* how many characters transmitted      */
ulong received;               /* how many characters received         */
char backout = FALSE;         /* Words out backward?                  */
char justLostCarrier = FALSE; /* Just lost it                         */
char newCarrier = FALSE;      /* Just got carrier                     */
char onConsole;               /* Who's in control?!?                  */
unsigned char modStat;        /* Had carrier LAST time you            */
                              /* you checked?                         */
char whichIO = CONSOLE;       /* CONSOLE or MODEM                     */
char haveCarrier;             /* set if DCD == TRUE                   */
char echo;                    /* Either NEITHER, CALLER, or BOTH      */
char echoChar;                /* What to echo with if echo == NEITHER */
char anyEcho = TRUE;
char warned;
char sysopkey = FALSE;
char chatkey = FALSE;
char eventkey = FALSE;
char disabled;          /* is modem disabled?   */
int getkey;
char speed;
char detectflag = FALSE;    /* got carrier while on console */

#ifdef OLD_ASCII_TBL
char filter[FILT_LEN];      /* full ASCII filtering */

#else
#include "translat.h"
#endif
char modem = TRUE;
char printing = FALSE;
char console = TRUE;        /* output flag for console */
char ConLock = FALSE;
char debug = FALSE;
char callout = FALSE;
unsigned char crtColumn;    /* current position on screen  */
char outFlag = OUTOK;       /* will be one of the above    */
char dotoMessage = NO_SPECIAL;
ulong markedMId = 0L;       /* id of message to be marked  */

char heldMessage;
char replyFlag;         /* Sending a reply?            */
char linkMess;          /* Sending a file-linked message? */
int mread, entered;     /* #messages read and entered  */

ulong originalId = 0L;  /* this is the ID of origin    */
unsigned char originalattr; /* Attribute byte of origin      */
char copyflag = FALSE;      /* true when viewing a duplicate */
char MessageRoom[MAXROOMS]; /* number of message/room this call */
char seen;
unsigned int bauds[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 0};
char showdir, showhidden, showgroup;
char skiproom = TRUE;
int oldroom, ug_lvisit, ug_new;
uchar scrollpos = 23;       /* adjusts for help menu         */
uchar cursorch, cursorcl;
uchar ansiattr;
char ansiOn = FALSE;
char IBMOn = FALSE;
char SoundOn = FALSE;
char Music = FALSE;
char Sound_Buffer[300];
int sound_pos = 0;
uchar Sound_Entries = 0;
int sizeLTentry;

char grpZap = FALSE, hallZap = FALSE, msgZap = FALSE, logZap = FALSE;
char roomZap = FALSE, hallgrpZap = FALSE, roomgrpZap = FALSE;

char roomBuild = FALSE,     /* Rebuild room-file?           */
hallBuild = FALSE;      /* Rebuild hall-file?           */

char gprompt[80];


#endif
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifndef MISC
/* FILES */
extern FILE *msgfl; 	/* msg file descriptor			   */
extern FILE *printfile; /* printer.out					   */
extern FILE *trapfl;	/* trap file descriptor 		   */
extern FILE *upfd;
extern FILE *aideFl;	/* a temp file for aide messages   */
extern FILE *hallfl;	/* Hall file descriptor 		   */
extern FILE *grpfl; 	/* Group file descriptor		   */
extern FILE *roomfl;	/* Room file descriptor 		   */
extern FILE *logfl; 	/* log file descriptor			   */
extern FILE *hallgrpfl; /* multiple groups/hall descriptor */
extern FILE *roomgrpfl; /* multiple groups/room descriptor */

/* STRUCTURES */
extern label editcmd;                   /* External editor commands     */
extern struct aRoom roomBuf;            /* Room buffer                  */
extern struct accounting far *accountBuf; /* Accounting buffer			  */
extern struct accounting *start_acct;   /* Accounting buffer ptr to first   */
extern struct accounting thisAccount;   /* Groupdata used for current user  */
extern struct config far *cfg;			/* A buncha variables			*/
extern struct groupBuffer far *grpBuf;	/* Group data goes here 		*/
extern struct logBuffer lBuf;           /* Temp buffer...........       */
extern struct logBuffer logBuf;         /* Log buffer of a person       */
extern struct mfilter mf;               /* Message filter               */
extern struct msginfo mesginfo;         /* User/room for message from aplic */
extern struct parms parm;
extern struct talleyBuffer talleyBuf;   /* message talley for rooms     */

extern struct ext_editor far *edit; 	/* External editor commands 	*/
extern struct ext_editor *start_edit;   /* External editor commands     */
extern struct fDir far *filedir;		/* pointer for filedir struct	*/
extern struct fInfo far *fileinfo;		/* pointer for file-info struct */
extern struct tzrules tzr[4];			/* Timezone rules structure 	*/
extern struct hallBuffer far *hallBuf;	/* Hall stuff right here		*/
extern struct hallGroup far *hallgrp;	/* multiple group per hall		*/
extern struct roomGroup far *roomgrp;	/* multiple group per room		*/
extern struct lTable far *logTab;       /* RAM index of pippuls         */
extern struct messagetable huge *msgTab;/* THE message table            */
extern struct msgB far *msgBuf2;		/* The Old-message buffer		*/
extern struct msgB far *msgBuf; 		/* The -sole- message buffer	*/
extern struct nodest far *node; 		/* node buffer					*/
extern struct ndgroups far *ndgrp, *start_ndgrp;
extern struct ndrooms far *ndroom, *start_ndroom;

#endif

#ifndef MISC2
extern char extrncmd[MAXEXTERN];           /* External command letters     */
extern struct ext_prot *extrn;             /* External protocols buffer    */
extern struct ext_prot *start_extrn;       /* External protocols buffer    */
extern struct ext_command *extCmd;         /* External commands buffer     */
extern struct ext_command *start_extCmd;   /* External commands buffer     */
extern struct ChatSound ChatSnd;           /* Chat Sound Parmeters         */
extern struct ChatBell far *chat_bell;	   /* Chat & Bell on/off times	   */
extern struct rTable *roomTab;             /* RAM index of rooms           */
extern label default_group;                /* Default for net messages     */
extern struct event far *events, *start_event; /* CRON Events				   */

/* Function pointers */
extern void (*charattr) (char, uchar);
extern void (*stringattr) (uint, char *, uchar);

/* OTHER */
extern char specialTime;            /* Is it a "special" hour         */
extern float startbalance;          /* Number of credits at log-in    */
extern long lasttime;               /* timestamp of last update       */
extern char readMessage;
extern char ExitToMsdos;            /* bring system down              */
extern char sysReq;                 /* SysOp wants system             */
extern char *confirm;
extern char mailFlag;               /* Sending private mail?          */
extern char oldFlag;                /* Entering old message?          */
extern char limitFlag;              /* Entering Limited-Access        */
extern char chatReq;                /* Did they want to chat?         */
extern char dowhat;                 /* What is calling iChar          */
extern int xpd, duplic;
extern char duplicate, expired;     /* count duplicates and expired   */
extern char netError;               /* save net error message?        */
extern unsigned char thisHall;      /* hall we're in                  */
extern int thisLog;                 /* entry currently in logBuf      */
extern int thisRoom;                /* Current room                   */
extern char etcpath[64];            /* Path etc.dat and ctdl.exe in   */
extern int thisSlot;                /* logTab slot logBuf found via   */
extern char loggedIn;               /* Global have-caller flag        */
extern char prevChar;               /* for EOLN/EOParagraph stuff     */
extern char expert;                 /* true to suppress hints etc.    */
extern char termUpper;              /* uppercase only flag            */
extern char termLF;                 /* LF-after-CR flag               */
extern char aide;                   /* aide privileges flag           */
extern char sysop;                  /* sysop privileges flag          */
extern char twit;                   /* Problem user?                  */
extern char unlisted;               /* List in userlog?               */
extern char termTab;                /* Tabs                           */
extern char oldToo;                 /* Send last old on new request?  */
extern char roomtell;               /* Print room descriptions?       */
extern char Sound;                  /* Sound Flag                     */
extern char drv_diff;				/* Drive difference (LAN Phase I) */
extern char ctdlConfig; 			/* To bypass the room passwords   */
extern unsigned char linesScreen;   /* Lines per screen or 0          */
extern unsigned char termWidth;     /* width to format output to      */
extern unsigned char termNulls;     /* # nulls to send at EOLN        */
extern int people;                  /* this is the old slot           */
extern char sysopNew;
extern long uptimestamp;
extern long logtimestamp;
extern long conntimestamp;
extern ulong transmitted;           /* how many characters transmitted */
extern ulong received;              /* how many characters transmitted */
extern char backout;                /* Words out backward?             */
extern char justLostCarrier;        /* Just lost it                    */
extern char newCarrier;             /* Just got carrier                */
extern char onConsole;              /* Who's in control?!?             */
extern unsigned char modStat;       /* Had carrier LAST time you       */
                                    /* you checked?                    */
extern char whichIO;                /* CONSOLE or MODEM                */
extern char haveCarrier;            /* set if DCD == TRUE              */
extern char echo;                   /* Either NEITHER, CALLER, or BOTH */
extern char echoChar;               /* What to echo with if echo == NEITHER */
extern char anyEcho;
extern char sysopkey;
extern char chatkey;
extern char eventkey;
extern char disabled;               /* is modem disabled?              */
extern int getkey;
extern char speed;
extern char detectflag;             /* got carrier while on console    */
extern unsigned char filt_in[];     /* translate input                 */
extern unsigned char filt_out[];    /* translate output                */
extern char modem;
extern char printing;
extern char console;                /* output flag for console         */
extern char ConLock;
extern char debug;
extern char callout;
extern unsigned char crtColumn;     /* current position on screen       */
extern char outFlag;                /* will be one of the above         */
extern char dotoMessage;
extern ulong markedMId;             /* id of message to be marked       */

extern ulong originalId;            /* this is the ID of origin         */
extern char copyflag;               /* true when viewing a duplicate    */
extern char MessageRoom[MAXROOMS];  /* number of message/room this call */
extern char seen;
extern unsigned int bauds[];
extern char showdir, showhidden, showgroup;
extern char skiproom;
extern int oldroom, ug_lvisit, ug_new;
extern uchar scrollpos;     /* adjusts for help menu         */
extern uchar ansiattr;
extern char ansiOn;
extern char IBMOn;
extern char SoundOn;
extern char Music;
extern char Sound_Buffer[300];
extern int sound_pos;
extern uchar Sound_Entries;

extern char heldMessage;
extern char replyFlag;      /* Sending a reply?               */
extern char linkMess;       /* Sending a file-linked message? */
extern int mread, entered;  /* #messages read and entered     */

extern unsigned char originalattr;  /* Attribute byte of origin      */

extern char grpZap, hallZap, msgZap, logZap, roomZap, hallgrpZap, roomgrpZap;

extern char roomBuild,      /* Rebuild room-file?           */
hallBuild;                  /* Rebuild hall-file?           */

extern char gprompt[80];


#endif

extern char *softname;
extern char *version;
extern char release;
extern const char compdate[];
extern const char comptime[];
extern char *welcome[];
extern char *copyright[];
extern unsigned int copycrcs[];
extern unsigned int welcomecrcs[];

/* EOF */
