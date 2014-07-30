
/*
 *  Citadel Application Header file
 *  for use with the Citadel BBS system
 *  version 3.03.9d and up.
 *
 *	written by Peter Torkelson	6/2/88, for the public domain
 *	Modified by Brad L. Johnson for CenCit
 *
 *	This does not use bit fields in the hopes that applications
 *  can be writen with other compilers, or even basic if it comes
 *  to that. (have fun reading this into basic however)
 */


/* --------------------------------------------------------------------
 *
 * -------------------------------------------------------------------- */

/*
 *	This structure is written to a file 'output.apl' when cit calls an
 *	application, and if there is a file 'input.apl' upon return it is
 *  read in.
 *
 *	Things tagged with a '@' can be changed, the rest are ignored apon
 *  return to the BBS. Sorry, no sysop back doors in the applications! :-)
 */

/*
 *  Citadel Application Header file
 *	for use with the CenCit BBS system
 *	version 1.27 and up.
 *
 *  Written by Peter Torkelson & Louis Erikson
 *  11/25/89, for the public domain.
 */

/*
 *	  CenCit will write an OUTPUT.APL file, and read in an INPUT.APL file.
 *  These files contain information that applications might find useful,
 *  the format of the file is as follows:
 *
 *    Multiple lines, the first character defines what that line will
 *  contain. The rest of the line is the value in ascii format. The
 *  defines in this file will tell you what that first char means.
 *  Valid values are 32-249.
 *    Values 250-255 are used for control flags. See below:
 *
 *    APL_EXTEND (254) marks an extended field, this is for
 *  later expansion, and is yet to be fully defined.
 *    APL_END (255) marks the end of the file. No further data
 *  should be read past this point.
 *
 *    Future versions of the program may write out more fields, so if you
 *	see something you don't understand, just ignore it. CenCit will do
 *  the same.
 *    Fields that have a comment marked with a '@' or a '!' will be read in
 *	by CenCit, others will simply be ignored.
 *	  '!' fields are not written out by CenCit...
 *
 *    This method of data transfer is being adopted so there can be
 *  additions without screwing up all older applications, as when ANSION
 *  was added to the old structures.  Also, this makes it so structures in
 *  programs written with the applic.lib can be bitwise-packed.
 *
 *
 *	  If there is something you want to know from CenCit that it knows,
 *  suggest it for future versions.
 */

/*
 * control stuff
 */
#define APL_EXTEND      254         /*@ Extended field, for expansion       */
#define APL_END         255         /*@ Last field in file                  */

/*
 * who they are
 */
#define APL_NAME        32          /*  name of current user                */
#define APL_SURNAME     33          /*@ surname of currnet user             */
#define APL_TITLE       34          /*@ title of current user               */
#define APL_SYSOP       35          /*@ is user a sysop?  1 or 0            */
#define APL_AIDE        36          /*@ is user an aide?  1 or 0            */
#define APL_TWIT        37          /*@ is user twitted?  1 or 0            */
#define APL_EXPERT      38          /*@ is user in expert mode?  1 or 0     */
#define APL_CREDITS     39          /*@ number of minutes left to the user  */
#define APL_PERMANENT   40          /*@ user have a permanent acct? 1 or 0  */
#define APL_NETUSER     41          /*@ user have net access?  1 or 0       */
#define APL_NOMAIL      42          /*@ can user send mail?  1 or 0         */
#define APL_VERIFIED    43          /*@ Verified user? 1 || 0               */

/*
 * terminal stuff!
 */
#define APL_NULLS       50          /*@ how many nulls do they need?        */
#define APL_LINEFEED    51          /*@ linefeeds?  1 or 0                  */
#define APL_UPPERCASE   52          /*@ uppercase only terminal?  1 or 0    */
#define APL_ANSION      53          /*@ ansi?  1 or 0                       */
#define APL_COLUMNS     54          /*@ number of columns on screen         */
#define APL_LINES       55          /*@ number lines on screen              */
#define APL_ULISTED     56          /*@ unlusted userlog entry? 1 or 0      */
#define APL_MDATA       57          /*  on what com port?                   */
#define APL_BAUD        58          /*  what baud are they at? (300-9600)   */
#define APL_IBM_GRAPHIC 59          /*@ IBM Graphics?  1 or 0               */
#define APL_TRM_FORE    60          /*@ foreground color code, 0-7.         */
#define APL_TRM_BACK    61          /*@ background color code, 0-7.         */
#define APL_TRM_NORM    62          /* the rest of them..                   */
#define APL_TRM_BOLD    63
#define APL_TRM_INVR    64
#define APL_TRM_BLINK   65
#define APL_TRM_UNDER   66
/* reserve APL_EFF_BAUD 67 effective baud (see 58) for Maven.  Never read in.*/
/*
 * where they are
 */
#define APL_NODECOUNTRY 69          /*  Country of the BBS                  */
#define APL_NODE        70          /*  What node are we on?                */
#define APL_REGION      71          /*  Region of the BBS                   */
#define APL_VERSION 	72			/*	text version string from CenCit    */
#define APL_HALL        73          /*@ name of hall we are in              */
#define APL_ROOM        74          /*@ name of room we are in              */
#define APL_TEMPPATH    75          /*  full 'temp' path for scratch files  */
#define APL_APPLPATH    76          /*  full applications directory path    */
#define APL_HELPPATH    77          /*  full helpfile path                  */
#define APL_HOMEPATH    78          /*  full Citadel homepath               */
#define APL_ROOMPATH    79          /*  full path of room descriptions      */
#define APL_MSGPATH     80          /*  full path for messgae base          */
#define APL_PRINTPATH   81          /*  name of printer capture file        */
#define APL_TRAPFILE    82          /*  full filename of the TRAP file      */
#define APL_ACCOUNTING  83          /*  is system accunting on?             */
#define APL_TRANSPATH   84          /*  full path of mail & transient files */

/*
 * console stuff
 */
#define APL_ATTR        90          /*  main text attribute                 */
#define APL_WATTR       91          /*  Statline attribute                  */
#define APL_CATTR       92          /*  hi-lited attribute                  */
#define APL_UTTR        93          /*  underlining attribute               */
#define APL_BATTR       94          /*  Borderline attribute                */
#define APL_TRANSMIT	95			/*@ number characters sent by CenCit	*/
#define APL_RECEIVE     96          /*@ number characters recived           */
#define APL_SYSREQ      97          /*@ Has SysReq been pushed? 1 or 0      */
#define APL_CONLOCK     98          /*  are Fkeys locked out? 1 or 0        */
#define APL_ECHO        99          /*@ display paused w/F4  1 or 0         */
#define APL_CHAT       100          /*@ is chat mode on or off? 1 or 0      */
#define APL_BELLS      101          /*@ are bells on? 1 or 0                */
#define APL_PRINTING   102          /*@ is alt-p printer catch on? 1 or 0   */

/*
 * Group stuff
 */
#define GRP_IN         110          /*  The user is in this group..         */
#define GRP_ADD        111          /*! Add the user to this group..        */
#define GRP_DEL        112          /*! Remove the user from this group..   */

/*
 * Room Moving Stuff
 */

#define APL_NEWHALL    120			/*@ The new hallway to put the room on	*/
#define APL_APPOFF	   121			/*@ Turn application off				*/

/*
 * Message Info
 *
 * For use with the MESSAGE.APL file, tells CenCit what to do with it...
 *
 */
#define MSG_NAME       200          /*! Name of the person the message is   */
                                    /*  from, default: Nodename             */
#define MSG_TO         201          /*! Name of the person the message is   */
                                    /*  to, default: Public Message         */
#define MSG_GROUP      202          /*! Group the message is for,           */
                                    /*  default: Public Message             */
#define MSG_ROOM       203          /*! Room the message is in,             */
                                    /*  default: Current Room               */
struct msginfo {
    label roomName;     /* Where do we put the msg */
    label author;       /* Who wrote the dang thing?       */
    char mi_res1[10];       /* Reserved...             */
};

