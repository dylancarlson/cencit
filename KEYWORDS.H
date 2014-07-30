/************************************************************************/
/*                            keywords.h                                */
/*        all keywords used by Citadel configuration program            */
/************************************************************************
 *
 *  03/19/90    FJM     Linted & partial cleanup
 *  09/21/90    FJM     Added #SYSOP, #CREDIT_NAME
 *  10/13/90    FJM     Added #READLOG, #AIDEREADLOG
 *  10/18/90    FJM     Added #DEFAULT_ROUTE keyword.
 *  01/18/91    FJM     Added #SWAPFILE
 *  02/05/91    FJM     Added #SCREENSAVE
 *  05/15/91    BLJ     Removed #COST1200, #COST2400
 *                      Added #HELPPATH_2, #IBMROOM, #MAILPATH
 *  06/12/91    BLJ     Added #LAN
 *  06/29/91    BLJ     Added #WIDEROOM
 *  07/03/91    BLJ     Added #MAXSOUND
 *  08/21/91    BLJ     Added #CHATTAB
 *  09/12/91    BLJ     Added #NODESTATE, #NODECNTRY, #NODEPHONE,
 *                            #SYSOPREGION, #SYSOPCNTRY
 *	04/28/92	BLJ 	Added #PACING
 *	05/03/92	BLJ 	Added #NOK_SCRIPT_SLAVE, #NOK_SCRIPT_MASTER
 *	06/21/92	BLJ 	Added #MENUOK
 *	07/05/92	BLJ 	Added #MENULINES
 *	07/08/92	BLJ 	Adeed #STICKYRM
 *	08/30/92	BLJ 	Added #MEMFREE
 *
 ************************************************************************/

/************************************************************************/
/*                         config.cit keywords                          */
/************************************************************************/
#ifdef MAIN
char *keywords[] =
{
    "#ACCOUNTING",
    "#ALLDONE",
    "#ATTR",
    "#DUMB_MODEM",
    "#ENTEROK",
    "#FORCELOGIN",
    "#HELPPATH",
    "#HELPPATH_2",
    "#HOMEPATH",
    "#IBMROOM",
    "#IDLE_WAIT",
    "#KILL",
    "#LINEFEEDS",
    "#LOGINSTATS",
    "#MAXBALANCE",
    "#MAXLOGTAB",
    "#MAXTEXT",
    "#MAX_WARN",
    "#MDATA",
    "#MAXFILES",
    "#MESSAGEK",
    "#MODSETUP",
    "#MSGPATH",
    "#NMESSAGES",
    "#NODENAME",
    "#NOPWECHO",
    "#NULLS",
    "#OFFHOOK",
    "#OLDCOUNT",
    "#PRINTER",
    "#READOK",
    "#ROOMOK",
    "#ROOMPATH",
    "#ROOMTELL",
    "#SUBHUBS",
    "#TABS",
    "#TRAP",
    "#TRAP_FILE",
    "#TEMPPATH",
    "#TIMEOUT",
    "#UNLOGGEDBALANCE",
    "#UNLOGTIMEOUT",
    "#UPPERCASE",
    "#DIAL_INIT",
    "#USER",
    "#WIDTH",
    "#NEWBAL",
    "#SURNAMES",
    "#PRIVATE",
    "#MESSAGE_ROOM",
    "#APPLICATION",
    "#MODERATE",
    "#AIDEHALL",
    "#F6PASSWORD",
    "#NODEREGION",
    "#WATTR",
    "#INIT_BAUD",
    "#READLLOG",
    "#ENTERSUR",
    "#VDATESTAMP",
    "#CATTR",
    "#DIAL_PREF",
    "#DATESTAMP",
    "#BIOS",
    "#NEWUSERAPP",
    "#BATTR",
    "#UTTR",
    "#SYSOP",
    "#CREDIT_NAME",
    "#READLOG",
    "#AIDEREADLOG",
    "#SWAPFILE",
    "#SCREENSAVE",
    "#MAILPATH",
    "#LAN",
    "#WIDEROOM",
    "#MAXSOUND",
    "#CHATTAB",
    "#NODESTATE",
    "#NODECNTRY",
    "#NODEPHONE",
    "#SYSOPREGION",
    "#SYSOPCNTRY",
	"#MENUOK",
	"#MENULINES",
	"#STICKYRM",
	"#MEMFREE",
	NULL
};

#else
extern char *keywords[];

#endif

#define K_ACCOUNTING    0
#define K_ALLDONE       1
#define K_ATTR          2
#define K_DUMB_MODEM    3
#define K_ENTEROK       4
#define K_FORCELOGIN    5
#define K_HELPPATH      6
#define K_HELPPATH_2    7
#define K_HOMEPATH      8
#define K_IBMROOM       9
#define K_IDLE_WAIT     10
#define K_KILL          11
#define K_LINEFEEDS     12
#define K_LOGINSTATS    13
#define K_MAXBALANCE    14
#define K_MAXLOGTAB     15
#define K_MAXTEXT       16
#define K_MAX_WARN      17
#define K_MDATA         18
#define K_MAXFILES      19
#define K_MESSAGEK      20
#define K_MODSETUP      21
#define K_MSGPATH       22
#define K_NMESSAGES     23
#define K_NODENAME      24
#define K_NOPWECHO      25
#define K_NULLS         26
#define K_OFFHOOK       27
#define K_OLDCOUNT      28
#define K_PRINTER       29
#define K_READOK        30
#define K_ROOMOK        31
#define K_ROOMPATH      32
#define K_ROOMTELL      33
#define K_SUBHUBS       34
#define K_TABS          35
#define K_TRAP          36
#define K_TRAP_FILE     37
#define K_TEMPPATH      38
#define K_TIMEOUT       39
#define K_UNLOGGEDBALANCE  40
#define K_UNLOGTIMEOUT  41
#define K_UPPERCASE     42
#define K_DIAL_INIT     43
#define K_USER          44
#define K_WIDTH         45
#define K_NEWBAL        46
#define K_SURNAMES      47
#define K_PRIVATE       48
#define K_MESSAGE_ROOM  49
#define K_APPLICATIONS  50
#define K_MODERATE      51
#define K_AIDEHALL      52
#define K_F6PASSWORD    53
#define K_NODEREGION    54
#define K_WATTR         55
#define K_INIT_BAUD     56
#define K_READLLOG      57
#define K_ENTERSUR      58
#define K_VDATESTAMP    59
#define K_CATTR         60
#define K_DIAL_PREF     61
#define K_DATESTAMP     62
#define K_BIOS          63
#define K_NEWUSERAPP    64
#define K_BATTR         65
#define K_UTTR          66
#define K_SYSOP         67
#define K_CREDIT_NAME   68
#define K_READLOG       69
#define K_AIDEREADLOG   70
#define K_SWAP_FILE     71
#define K_SCREEN_SAVE   72
#define K_MAILPATH      73
#define K_LAN           74
#define K_WIDEROOM      75
#define K_MAXSOUND      76
#define K_CHATTAB       77
#define K_NODESTATE     78
#define K_NODECNTRY     79
#define K_NODEPHONE     80
#define K_SYSOPREGION   81
#define K_SYSOPCNTRY    82
#define K_MENUOK		83
#define K_MENULINES 	84
#define K_STICKYRM		85
#define K_MEMFREE		86
#define K_NWORDS		86		/* set to last keyword value */
                                /* (number of last word)     */

#ifdef MAIN
char *trapkeywords[] =
{
    "ALL",
    "CARRIER",
    "LOGIN",
    "NEWROOM",
    "PROBLEM_USER",
    "CHAT",
    "PASSWORD",
    "AIDE",
    "SYSOP",
    "UPLOAD",
    "DOWNLOAD",
    "ERROR",
    "NETWORK",
    "ACCOUNT",
    "APPLIC",
    NULL
};

char *userkeywords[] =
{
    "PROBLEM_USER",
    "PERMANENT",
    "NO_ACCOUNTING",
    "NETWORK",
    "NO_MAIL",
    NULL
};

#else
extern char *trapkeywords[];
extern char *userkeywords[];

#endif

/************************************************************************/
/*                        grpdata.cit keywords                          */
/************************************************************************/

#ifdef MAIN
char *grpkeywords[] =
{
    "#DAYS",
    "#GROUP",
    "#HOURS",
    "#DAY_INC",
    "#SPECIAL",
    "#PRIORITY",
    "#MAX_BAL",
    "#DL_MULT",
    "#UL_MULT",
    NULL
};

#else
extern char *grpkeywords[];

#endif

#define GRK_DAYS              0
#define GRK_GROUP             1
#define GRK_HOURS             2
#define GRK_DAYINC            3
#define GRK_SPECIAL           4
#define GRK_PRIORITY          5
#define GRK_MAXBAL            6
#define GRK_DLMULT            7
#define GRK_ULMULT            8

#ifdef MAIN
char *daykeywords[] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Any", NULL
};

#else
extern char *daykeywords[];

#endif


/************************************************************************/
/*                         nodes.cit keywords                           */
/************************************************************************/

#ifdef MAIN
char *nodekeywords[] =
{
    "#BAUD",
    "#DISCARD",
    "#DISCARD_USER",
    "#EXPIRE",
    "#GROUP",
    "#DIAL_TIMEOUT",
    "#LOGIN",
    "#WAIT_TIMEOUT",
    "#PROTOCOL",
    "#NODE",
    "#PHONE",
    "#REDIAL",
    "#MAIL_TMP",
    "#ROOM",
    "#ROUTE",
    "#ALIAS",
    "#REGION",
    "#DEFAULT_ROUTE",
	"#PACING",
	NULL
};

#else
extern char *nodekeywords[];

#endif

#define NOK_BAUD          0
#define NOK_DISCARD       1
#define NOK_DISCARD_USER  2
#define NOK_EXPIRE        3
#define NOK_GROUP         4
#define NOK_DIAL_TIMEOUT  5
#define NOK_LOGIN         6
#define NOK_WAIT_TIMEOUT  7
#define NOK_PROTOCOL      8
#define NOK_NODE          9
#define NOK_PHONE         10
#define NOK_REDIAL        11
#define NOK_MAIL_TMP      12
#define NOK_ROOM          13
#define NOK_ROUTE         14
#define NOK_ALIAS         15
#define NOK_REGION        16
#define NOK_DEFAULT_ROUTE 17
#define NOK_PACING		  18
#define NOK_SCRIPT_SLAVE  19
#define NOK_SCRIPT_MASTER 20

/************************************************************************/
/*                         cron.cit keywords                            */
/************************************************************************/
#ifdef MAIN
char *cronkeywords[] =
{
    "#DO",
    "#HOURS",
    "#DAYS",
    "#REDO_TIME",
    "#RETRY_TIME",
    "#PRIORITY",
    NULL
};

#else
extern char *cronkeywords[];

#endif

#define CR_DO            0
#define CR_HOURS         1
#define CR_DAYS          2
#define CR_REDO          3
#define CR_RETRY         4
#define CR_PRIORITY      5

#ifdef MAIN
char *crontypes[] =
{
    "NETWORK",
    "SHELL_1",
    "SHELL_2",
    "EXIT",
    "LOCAL",
    NULL
};

#else
extern char *crontypes[];

#endif

#define CR_NET      0
#define CR_SHELL_1  1
#define CR_SHELL_2  2
#define CR_EXIT     3
#define CR_LOCAL    4

