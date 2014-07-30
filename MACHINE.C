/************************************************************************
 *
 *                              CONTENTS
 *
 *      bytesfree()             returns #bytes free on current drive
 *      getattr()               returns a file attribute
 *      setattr()               sets file attributes
 *
 ************************************************************************/

/* --------------------------------------------------------------------
 *
 * History:
 *
 *   12/22/90   FJM     Initial revision.
 *   04/19/91   FJM     New harderror handler.
 *
 * -------------------------------------------------------------------- */

#include <dos.h>
#include <io.h>
#include "ctdl.h"
#include "proto.h"

/************************************************************************/
/*      getattr() returns file attribute                                */
/************************************************************************/
unsigned char getattr(char FAR *filename)
{
    union REGS inregs;
    union REGS outregs;

    inregs.x.dx = FP_OFF(filename);
    inregs.h.ah = 0x43;     /* CHMOD */
    inregs.h.al = 0;        /* GETATTR */

    intdos(&inregs, &outregs);

    return ((int) outregs.x.cx);
}


/************************************************************************/
/*      setattr() sets file attributes                                  */
/************************************************************************/
void setattr(char FAR * filename, unsigned char attr)
{
    union REGS inregs;
    union REGS outregs;

    inregs.x.dx = FP_OFF(filename);
    inregs.h.ah = 0x43;     /* CHMOD */
    inregs.h.al = 1;        /* SET ATTR */

    inregs.x.cx = attr;     /* attribute */

    intdos(&inregs, &outregs);
}

/************************************************************************/
/*      bytesfree() returns # bytes free on drive                       */
/************************************************************************/
long bytesfree()
{
    char path[64];
    long bytes;
    union REGS REG;

    getcwd(path, 64);

    REG.h.ah = 0x36;        /* select drive */

    REG.h.dl = (path[0] - '@');

    intdos(&REG, &REG);

    bytes = (long) ((long) REG.x.cx * (long) REG.x.ax * (long) REG.x.bx);

	return (bytes);
}
#define IGNORE  0
#define RETRY   1
#define ABORT   2

/* define the error messages for trapping disk problems */
static char *err_msg[] = {
	"write protect",
	"unknown unit",
	"drive not ready",
	"unknown command",
	"data error (CRC)",
	"bad request",
	"seek error",
	"unknown media type",
	"sector not found",
	"printer out of paper",
	"write fault",
	"read fault",
	"general failure",
	"reserved",
	"reserved",
	"invalid disk change"
};

int cit_herror_handler(int errval,int ax,int bp,int si)
{
   static char msg[80];
   unsigned di;
   int drive;
   int errorno;

   di= _DI;
/* if this is not a disk error then it was another device having trouble */

   if (ax < 0)
   {
	  /* report the error */
	  mPrintf("\nDevice error\n");
	  /* and return to the program directly
	  requesting abort */
	  hardretn(ABORT);
   }
/* otherwise it was a disk error */
   drive = ax & 0x00FF;
   errorno = di & 0x00FF;
/* report which error it was */
   sprintf(msg,"\nError: %s on drive %c\n",err_msg[errorno],'A'+drive);

   mPrintf(msg);
/* return to the program via dos interrupt 0x23 with abort, retry or */
/* ignore as input by the user.  */
   hardretn(ABORT);
   return ABORT;
}
