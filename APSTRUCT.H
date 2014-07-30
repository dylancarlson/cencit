/* -------------------------------------------------------------------- */
/*	APSTRUCT.H				CenCit										*/
/* -------------------------------------------------------------------- */
/*  This file contains the table to generate OUTPUT.APL and INPUT.APL   */
/* -------------------------------------------------------------------- */
  
/*  ------------------------------------------------------------------  */
/*  #defines                                                            */
/*  ------------------------------------------------------------------  */
#define TYP_CHAR    0
#define TYP_INT     1
#define TYP_STR     2
#define TYP_BOOL    3
#define TYP_FLOAT   4
#define TYP_LONG    5
#define TYP_OTHER   6
#define TYP_GRP     7
#define TYP_END     8

/*  ------------------------------------------------------------------  */
/*  Local variables                                                     */
/*  ------------------------------------------------------------------  */
struct ApplicTable {
    int item;
    void *variable;
    int type;
    int length;
    int keep;
};
  
struct ApplicTable AplTab[] =
{
    {APL_NAME, logBuf.lbname, TYP_STR, NAMESIZE, 0},
    {APL_SURNAME, logBuf.surname, TYP_STR, NAMESIZE, 1},
    {APL_TITLE, logBuf.title, TYP_STR, NAMESIZE, 1},
    {APL_SYSOP, &sysop, TYP_BOOL, 0, 1},
    {APL_AIDE, &aide, TYP_BOOL, 0, 1},
    {APL_TWIT, &twit, TYP_BOOL, 0, 1},
    {APL_EXPERT, &expert, TYP_BOOL, 0, 1},
    {APL_CREDITS, &logBuf.credits, TYP_FLOAT, 0, 1},
    {APL_PERMANENT, NULL, TYP_OTHER, 0, 1},
    {APL_NETUSER, NULL, TYP_OTHER, 0, 1},
    {APL_NOMAIL, NULL, TYP_OTHER, 0, 1},
    {APL_VERIFIED, NULL, TYP_OTHER, 0, 1},
    {APL_NULLS, &termNulls, TYP_CHAR, 0, 1},
    {APL_LINEFEED, &termLF, TYP_CHAR, 0, 1},
    {APL_UPPERCASE, &termUpper, TYP_BOOL, 0, 1},
    {APL_ANSION, &ansiOn, TYP_BOOL, 0, 1},
    {APL_IBM_GRAPHIC, &IBMOn, TYP_BOOL, 0, 1},
    {APL_COLUMNS, &termWidth, TYP_CHAR, 0, 1},
    {APL_LINES, &logBuf.linesScreen, TYP_CHAR, 0, 1},
    {APL_ULISTED, &unlisted, TYP_BOOL, 0, 1},
	{APL_MDATA, NULL, TYP_OTHER, 0, 0},
    {APL_BAUD, &speed, TYP_CHAR, 0, 0},
	{APL_NODECOUNTRY, NULL, TYP_OTHER, NAMESIZE, 0},
	{APL_NODE, NULL, TYP_OTHER, NAMESIZE, 0},
	{APL_REGION, NULL, TYP_OTHER, NAMESIZE, 0},
    {APL_VERSION, NULL, TYP_OTHER, NAMESIZE, 0},
    {APL_HALL, NULL, TYP_OTHER, 0, 1},
    {APL_ROOM, NULL, TYP_OTHER, 0, 1},
	{APL_TEMPPATH, NULL, TYP_OTHER, 64, 0},
	{APL_APPLPATH, NULL, TYP_OTHER, 64, 0},
	{APL_HELPPATH, NULL, TYP_OTHER, 64, 0},
	{APL_HOMEPATH, NULL, TYP_OTHER, 64, 0},
	{APL_ROOMPATH, NULL, TYP_OTHER, 64, 0},
	{APL_MSGPATH, NULL, TYP_OTHER, 64, 0},
	{APL_PRINTPATH, NULL, TYP_OTHER, 64, 0},
	{APL_TRAPFILE, NULL, TYP_OTHER, 64, 0},
    {APL_ACCOUNTING, NULL, TYP_OTHER, 0, 0},
	{APL_ATTR, NULL, TYP_OTHER, 0, 0},
	{APL_WATTR, NULL, TYP_OTHER, 0, 0},
	{APL_CATTR, NULL, TYP_OTHER, 0, 0},
	{APL_UTTR, NULL, TYP_OTHER, 0, 0},
	{APL_BATTR, NULL, TYP_OTHER, 0, 0},
    {APL_TRANSMIT, &transmitted, TYP_LONG, 0, 1},
    {APL_RECEIVE, &received, TYP_LONG, 0, 1},
    {APL_SYSREQ, &sysReq, TYP_BOOL, 0, 1},
	{APL_CONLOCK, NULL, TYP_OTHER, NAMESIZE, 0},
    {APL_ECHO, &anyEcho, TYP_BOOL, 0, 1},
    {APL_CHAT, NULL, TYP_OTHER, 0, 1},
    {APL_BELLS, NULL, TYP_OTHER, 0, 1},
    {APL_PRINTING, &printing, TYP_BOOL, 0, 1},
    {GRP_ADD, NULL, TYP_GRP, NAMESIZE, 1},
    {GRP_DEL, NULL, TYP_GRP, NAMESIZE, 1},
	{APL_APPOFF, NULL, TYP_OTHER, 0, 1},
	{APL_NEWHALL, NULL, TYP_OTHER, 0, 1},
	{APL_END, NULL, TYP_END, 0, 1}
};
  
/* EOF */
