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
 *    Rev 1.1   06 Jan 1991 12:45:58   FJM
 * Porting, formating, fixes and misc changes.
 *
 *    Rev 1.0   27 Dec 1990 20:17:10   FJM
 * Changes for porting.
 *
 *	 Rev 1.0   22 Dec 1990	0:27:42   FJM
 *Initial revision.
 *
 * -------------------------------------------------------------------- */
  
#include <dos.h>
#include <io.h>
#include "ctdl.h"
#include "proto.h"
  
/************************************************************************/
/*      getattr() returns file attribute                                */
/************************************************************************/
unsigned char getattr(filename)
char FAR *filename;
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
