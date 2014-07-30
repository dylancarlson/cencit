  
/* --------------------------------------------------------------------
 * 
 *    Rev 1.16   06 Jan 1991 12:46:14   FJM
 * Porting, formating, fixes and misc changes.
 *
 *    Rev 1.15   27 Dec 1990 20:16:26   FJM
 * Changes for porting.
 *
 *    Rev 1.12   22 Dec 1990 13:38:48   FJM
 *
 *    Rev 1.4   23 Nov 1990 13:25:34   FJM
 * Header change
 *
 *    Rev 1.2   17 Nov 1990 16:12:06   FJM
 * Added version control log header
 * -------------------------------------------------------------------- */
  
/* -------------------------------------------------------------------- */
/*                           Xmodem Stuff                               */
/* -------------------------------------------------------------------- */
  
#include <time.h>
#include "ctdl.h"
  
#ifndef ATARI_ST
#include <dos.h>
#include <conio.h>
#endif
  
#include "proto.h"
#include "global.h"
  
#define NAK             0x15
#define SOH             1
#define EOT             4
#define ACK             6
#define CRC             'C'
#define CAN             0x18
#define RETRY           5
#define RECSIZE         128
  
static unsigned crcaccum;   /* Global CRC bytes              */
static int crc = 0;     /* CRC on/off flag               */
static char checksum;       /* Checksum accumulator          */
  
/* DOS systems only use CRC routines */
#ifndef __MSDOS__
static char buffer[RECSIZE + 1];/* Record buffer                 */
static FILE *fd;        /* File pointer                  */
static unsigned rec;        /* Record number                 */
static char outahere;       /* CARRIERLOSS or ERROR return   */
  
#endif
  
static void updcrc(unsigned char x);
static void clrcrc(void);
  
 /* Internal XMODEM isn't needed on DOS machines which use DSZ or equiv. */
#ifndef __MSDOS__
static void clrline(void);
static int fillbuf(void);
static char receive(int seconds);
extern void rxfile(char *name);
extern void trans(char *in_name);
static void txrec(char *buf);
static char waitcan(int time);
static void xmodemCancel(char pointer, char *str);
  
/* -------------------------------------------------------------------- */
/*      clrline() - Clears garbage off the line                         */
/* -------------------------------------------------------------------- */
void clrline(void)
{
    while (MIReady())
        getMod();
}
  
/* -------------------------------------------------------------------- */
/*      fillbuf() - Load the I/O buffer with a record                   */
/* -------------------------------------------------------------------- */
int fillbuf(void)
{
    register int i;
  
    i = fread(buffer, 1, RECSIZE, fd);
  
    if (i == 0)
        return (FALSE);     /* no data read */
  
    /* fill end of record with EOF's, if we do not have a full block */
  
    for (; i < RECSIZE; i++)
        buffer[i] = 0x1a;
  
    return (TRUE);
}
  
/* -------------------------------------------------------------------- */
/*      receive() - gets a modem character, or times out ...            */
/*      Returns:        char on success else ERROR                      */
/* -------------------------------------------------------------------- */
char receive(int seconds)
{
    long t;
  
    if (MIReady())
        return ((char) getMod());
  
    t = time(NULL);
  
    while ((time(NULL) - t) < seconds) {
        if (MIReady())
            return ((char) getMod());
    }
    if (!gotCarrier()) {
        xmodemCancel(FALSE, "Carrier loss");
        outahere = TRUE;
    }
    return (ERROR);
}
  
/* -------------------------------------------------------------------- */
/*      rxfile() - recieves a file                                      */
/* -------------------------------------------------------------------- */
void rxfile(char *name)
{
    char ch, response, cx;
    unsigned char crclo, crchi;
    uchar r1, r2;
    int i;
  
    outahere = FALSE;
  
    clrline();
  
    if ((fd = fopen(name, "wb")) == NULL) {
		xmodemCancel(FALSE, "Can't create file");
        return;
    }
    doccr();
  
    rec = 1;
  
    crc = TRUE;
    response = CRC;
  
    while (TRUE) {
        if (crc)
            cPrintf("\rReceiving CRC                 (block #%d)", rec);
        else
            cPrintf("\rReceiving Checksum            (block #%d)", rec);
  
        for (i = 1; i <= RETRY * 5; i++) {
            outMod(response);
            if ((ch = receive(10)) == SOH)  /* SOH indicates rec    */
                break;
            else if (rec == 1 && i > 4) {
                crc = FALSE;
                response = NAK;
            }
            if (ch == EOT) {    /* EOT indicates done   */
                cPrintf("\rReception Complete.           ");
                doccr();
                fclose(fd);
                outMod(ACK);
                return;
            }
            if (ch == CAN) {
                xmodemCancel(TRUE, "Received cancel request");
                unlink(name);
                return;
            }
            if (KBReady()) {
                cx = (char) getch();
                getkey = 0;
                if (cx == CAN) {
                    xmodemCancel(FALSE, "Console cancel request");
                    outahere = TRUE;
                }
            }
            if (outahere) {
                fclose(fd);
                unlink(name);
                return;
            }
            if (i == RETRY * 5) {   /* timout exit */
                xmodemCancel(TRUE, "Can't sync to sender");
                unlink(name);
                return;
            }
        }
        r1 = receive(10);   /* record number */
        r2 = receive(10);   /* 1's compliment */
  
        for (i = 0; i < RECSIZE && !outahere; i++) {
            buffer[i] = receive(10);
        }
  
        if (outahere) {
            fclose(fd);
            unlink(name);
            return;
        }
        if (crc)
            crchi = receive(10);/* get hibyte CRC */
        crclo = receive(10);    /* lobyte or chksm */
        response = NAK;     /* init response */
  
        if ((~r1 & 0xFF) != (r2 & 0xFF)) {
            cPrintf("\rRecord mis-match             ");
            delay(1500);
            continue;
        }
        clrcrc();       /* calc checksum or CRC */
        for (i = 0; i < RECSIZE; i++)
            updcrc(buffer[i]);
        updcrc(0);
        updcrc(0);
        if (crc) {      /* CRC test */
            if (((unsigned) crclo + ((unsigned) crchi << 8)) != crcaccum) {
                cPrintf("\rBad CRC received              ");
                delay(1500);
                continue;
            }
        } else {        /* checksum test */
            if (crclo != checksum) {
                cPrintf("\rBad checksum received         ");
                delay(1500);
                continue;
            }
        }
        if ((unsigned) r1 == ((rec - 1) & 0xFF)) {
            cPrintf("\rReceived duplicate record    ");
            response = ACK;
            delay(1500);
            continue;
        }
        if ((unsigned) r1 != (rec & 0xFF)) {
            xmodemCancel(TRUE, "Record number error");
            unlink(name);
            return;
        }
        rec++;
  
        fwrite(buffer, 1, RECSIZE, fd);
  
        response = ACK;
    }
}
  
/* -------------------------------------------------------------------- */
/*      trans() - Does the actual transmission of a file                */
/* -------------------------------------------------------------------- */
void trans(char *in_name)
{
    char ch, cx;        /* scratch char    */
  
    outahere = FALSE;
  
    rec = 1;
  
    if ((fd = fopen(in_name, "rb")) == NULL) {
        doccr();
        cPrintf("Unable to open: %s", in_name);
        doccr();
        return;
    }
    clrline();          /* eat garbage */
  
    doccr();
  
    while (!outahere) {
        cPrintf("\rAwaiting initial nak          (block #%d)", 1);
  
        if ((ch = waitcan(10)) == NAK) {
            crc = FALSE;
            break;
        }
        if (ch == CRC) {
            crc = TRUE;
            break;
        }
        if (KBReady()) {
            cx = (char) getch();
            getkey = 0;
            if (cx == CAN) {
                xmodemCancel(FALSE, "Console cancel request");
                outahere = TRUE;
            }
        }
    }
    while (fillbuf() && !outahere)
        txrec(buffer);
    outMod(EOT);
    while ((ch = waitcan(10)) != ACK && !outahere) {
        outMod(EOT);
    }
  
    fclose(fd);
    if (!outahere) {
        cPrintf("\rTransmission Complete.       ");
        doccr();
    }
}
  
/* -------------------------------------------------------------------- */
/*      txrec() - Sends a single record or block                        */
/* -------------------------------------------------------------------- */
void txrec(char buf[])
{
    register int i;
    unsigned char cr;
    char cx;
  
    while (!outahere) {
        if (crc)
            cPrintf("\rTransmitting CRC              (block #%d)", rec);
        else
            cPrintf("\rTransmitting Checksum         (block #%d)", rec);
  
    /* clear garbage off line */
        while (MIReady())
            getMod();
  
        if (KBReady()) {
            cx = (char) getch();
            getkey = 0;
            if (cx == CAN) {
                xmodemCancel(FALSE, "Console cancel request");
                outahere = TRUE;
            }
        }
        outMod(SOH);        /* start of header */
        outMod((unsigned char) rec);    /* record number   */
        outMod((unsigned char) ~rec);   /* 1's compliment  */
        clrcrc();       /* clear CRC accum */
  
        for (i = 0; i < RECSIZE; i++) {
            outMod(buf[i]);
            updcrc(buf[i]);
        }
  
        updcrc(0);
        updcrc(0);
        cr = (unsigned char) crcaccum;
  
        if (crc) {
            outMod((unsigned char) (crcaccum >> 8));
            outMod(cr);
        } else {
            outMod(checksum);
        }
        if (waitcan(10) == ACK)
            break;
        cPrintf("\rError in transmission         ");
        delay(1500);
    }
    rec++;
}
  
/* -------------------------------------------------------------------- */
/*      waitcan() - Waits for a character.                              */
/* -------------------------------------------------------------------- */
char waitcan(int time)
{
    char ch;
  
    if ((ch = receive(time)) == CAN) {
        xmodemCancel(FALSE, "Received cancel request");
        outahere = TRUE;
        return ERROR;
    }
    return (ch);
}
  
/* -------------------------------------------------------------------- */
/*      xmodemCancel() - Does an abort and closes file if passed TRUE   */
/* -------------------------------------------------------------------- */
void xmodemCancel(char pointer, char *str)
{
    doccr();
    cPrintf("Error - %s, aborting file transfer", str);
    doccr();
    if (pointer)
        fclose(fd);     /* close the file if TRUE       */
    outMod(CAN);        /* send a CAN to cancel transfer */
    outMod(CAN);        /* send a CAN another to be sure */
}
  
#endif
  
/* -------------------------------------------------------------------- */
/*      clrcrc() - Modifies crcaccum and checksum                       */
/* -------------------------------------------------------------------- */
void clrcrc(void)
{
    crcaccum = 0;
    checksum = 0;
}
  
/* -------------------------------------------------------------------- */
/*      updcrc() - Updates the crc accumulator or checksum              */
/* -------------------------------------------------------------------- */
void updcrc(unsigned char x)
{
    unsigned shifter, flag;
  
    if (crc) {
        for (shifter = 0x80; shifter; shifter >>= 1) {
            flag = (crcaccum & 0x8000);
            crcaccum <<= 1;
            crcaccum |= ((shifter & x) ? 1 : 0);
            if (flag)
                crcaccum ^= 0x1021;
        }
    } else {            /* update checksum instead */
        checksum += x;
    }
}
  
/* -------------------------------------------------------------------- */
/*    stringcrc returns an unsigned int of the strings crc value        */
/* -------------------------------------------------------------------- */
unsigned int stringcrc(char *buff)
{
    int i;
  
    clrcrc();
    crc = TRUE;
    for (i = 0; buff[i]; i++)
        updcrc(buff[i]);
    return (crcaccum);
}
