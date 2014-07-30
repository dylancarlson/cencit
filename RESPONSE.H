/************************************************************************/
/*                            response.h                                */
/*        Hayse Modem Response Keywords                                 */
/************************************************************************
 *
 *  History:
 *
 *
 ************************************************************************/

/************************************************************************/
/*                         config.cit keywords                          */
/************************************************************************/
#ifdef MAIN
char *hayes_string[] =
{
    "OK",
    "CONNECT",
    "RING",
    "NO CARRIER",
    "ERROR",
    "CONNECT 1200",
    "NO DIALTONE",
    "BUSY",
    "NO ANSWER",
    "CONNECT 2400",
    NULL
};

#else
extern char *hayes_string[];
#endif

#define HAYES_UNKNOWN     -1
#define HAYES_OK          0
#define HAYES_CONNECT     1
#define HAYES_RING        2
#define HAYES_NOCARRIER   3
#define HAYES_ERROR       4
#define HAYES_CONNECT1200 5
#define HAYES_NODIAL      6
#define HAYES_BUSY        7
#define HAYES_NOANSWER    8
#define HAYES_CONNECT2400 10
#define HAYES_NUMCMDS     10      /* set to last keyword value */
 /* (number of last word) */


