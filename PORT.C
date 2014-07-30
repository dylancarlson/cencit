/* -------------------------------------------------------------------- */
/*  PORT.C                    Citadel                                   */
/* -------------------------------------------------------------------- */
/*  This module should contain all the code specific to the modem       */
/*  hardware. This is done in an attempt to make the code more portable */
/*                                                                      */
/*  Note: under the MS-DOS implementation there is also an ASM file     */
/*  contains some of the very low-level io rutines.                     */
/* -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *  History:
 *  05/15/91    BLJ     Moved history
 *	03/30/92	BLJ 	Added LAN code to not init com port
 *
 * -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <time.h>
#include <dos.h>
#include "machine.h"
#include "ctdl.h"
#include "proto.h"
#include "global.h"
#include "ibmcom.h"

void raise_dtr(void);

#ifdef OLD_COMM
extern void INT_INIT(int, int);
extern void COM_EXIT(void);
extern void COM_INIT(int, int, int, int);
extern unsigned int COM_STAT(void);
extern int COM_READ(void);
extern void COM_WRITE(unsigned char);
extern void DROP_DTR(void);
extern void RAISE_DTR(void);

#endif

/* --------------------------------------------------------------------
                                Contents
   --------------------------------------------------------------------
        baud()                  sets serial baud rate
        carrier()               checks carrier
        drop_dtr()              turns DTR off
        getMod()                bottom-level modem-input
        outMod()                bottom-level modem output
        Hangup()                breaks modem connection
        Initport()              sets up modem port and modem
        portExit()              Exit cserl.obj pakage
        portInit()              Init cserl.obj pakage
        ringdetect()            returns 1 if ring detect port is true
   --------------------------------------------------------------------

   --------------------------------------------------------------------
    HISTORY:

    07/23/89    (RGJ)   Fixed outMod to not overrun 300 baud on a 4.77
    04/06/89    (PAT)   Speed up outMod to keep up with 2400 on a 4.77
    04/01/89    (RGJ)   Changed outp() calls to not screw with the
                        CSERL.OBJ package.
    04/01/89    (PAT)   Moved outMod() into port.c
    03/22/89    (RGJ)   Change gotCarrier() and ringdetect() to
                        use COM_STAT() instead of polled I/O.
                        Also removed all outp's from baud().
                        And changed drop_dtr() and Hangup() so interupt
                        package is off when fiddling with DTR.
    02/22/89    (RGJ)   Change to Hangup, disable, initport
    02/07/89    (PAT)   Module created from MODEM.C
    03/02/90    FJM     Use of IBMCOM-C serial package
 *  03/02/90    FJM     Added IBMCOM-C drivers to replace ASM ones
 *  06/15/90    FJM     Added option for modem output pacing.
 *                      Mods for running from another BBS as a door.
 *  11/17/90    FJM     Prevented output of modem init string in shell mode.
 *  11/23/90    FJM     Header change
 *  12/27/90    FJM     Changes for porting.

   -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static char modcheck = 1;

 /* COM     #1     #2  */
static char modemcheck[] = {1, 5, 10, 20, 40, 0};

/* -------------------------------------------------------------------- */
/*      ringdetect() returns true if the High Speed line is up          */
/*                   if there is no High Speed line avalible to your    */
/*                   hardware it should return the ring indicator line  */
/*                   In this way you can make a custom cable and use it */
/*                   that way                                           */
/* -------------------------------------------------------------------- */
int ringdetect(void)
{
	if(drv_diff) return(0);

	return com_ring();	/* RI */
}

/* -------------------------------------------------------------------- */
/*      MOReady() is modem port ready for output                        */
/* -------------------------------------------------------------------- */
int MOReady(void)
{
	if(drv_diff) return(0);

	return com_tx_ready();
}

/* -------------------------------------------------------------------- */
/*      MIReady() Ostensibly checks to see if input from modem ready    */
/* -------------------------------------------------------------------- */
int MIReady(void)
{
	if(drv_diff) return(0);

	return !com_rx_empty();
}

/* -------------------------------------------------------------------- */
/*      Initport()  sets up the modem port and modem. No return value   */
/* -------------------------------------------------------------------- */
void Initport(void)
{
	if(drv_diff) return;

	haveCarrier = modStat = (char) gotCarrier();

    Hangup();

    disabled = FALSE;

	baud(cfg->initbaud);

    if (!parm.door) {
		outstring(cfg->modsetup);
        outstring("\r");
        delay(1000);
    }
	do_idle(1);  /* We use 1 because we may be in idle state */
}

/* -------------------------------------------------------------------- */
/*      Hangup() breaks the modem connection                            */
/* -------------------------------------------------------------------- */
void Hangup(void)
{
    long timer, timer2;

	if(drv_diff) return;

	time(&timer);
    while (!com_tx_empty()) {
        time(&timer2);
        if ((timer2 - timer) >= 8L)
            break;
    }
    drop_dtr();
    raise_dtr();
    delay(500);
}

/* -------------------------------------------------------------------- */
/*      gotCarrier() returns nonzero on valid carrier, else zero        */
/* -------------------------------------------------------------------- */
int gotCarrier(void)
{
	if(drv_diff) return(0);

    return com_carrier();
}

/* -------------------------------------------------------------------- */
/*      getMod() is bottom-level modem-input routine                    */
/* -------------------------------------------------------------------- */
int getMod(void)
{
	if(drv_diff) return(0);

	received++;
    return com_rx();
}

/* -------------------------------------------------------------------- */
/*		Mflush() is bottom-level modem-flush routine					*/
/* -------------------------------------------------------------------- */
void Mflush(void)
{
	if(drv_diff) return;

	com_flush_rx();
}

/* -------------------------------------------------------------------- */
/*      drop_dtr() turns dtr off                                        */
/* -------------------------------------------------------------------- */
void drop_dtr(void)
{
	if(drv_diff) return;

	disabled = TRUE;

    if (parm.door)      /* supress initial hangup in door mode */
        return;

    com_lower_dtr();
    delay(500);
}

/* -------------------------------------------------------------------- */
/*      raise_dtr() turns dtr on                                        */
/* -------------------------------------------------------------------- */

void raise_dtr(void)
{
	if(drv_diff) return;

	com_raise_dtr();
}

/* -------------------------------------------------------------------- */
/*      baud() sets up serial baud rate  0=300; 1=1200; 2=2400; 3=4800  */
/*										 4=9600 5=19200 6=38400 		*/
/*      and initializes port for general bbs usage   N81                */
/* -------------------------------------------------------------------- */


void baud(int baudrate)
{
	if(drv_diff) return;

	com_set_speed((long) bauds[baudrate]);
    com_set_parity(COM_NONE, 1);
	com_install(cfg->mdata);
    speed = baudrate;
    modcheck = modemcheck[speed];
}

/* -------------------------------------------------------------------- */
/*      outMod() stuffs a char out the modem port                       */
/* -------------------------------------------------------------------- */
void outMod(unsigned char ch)
{
    long t, l;

	if(drv_diff) return;

    if (!modem && !callout)
        return;

    /* dont go faster than the modem, check every modcheck */
    if (!(transmitted % modcheck) && !MOReady()) {
        time(&t);

        while (!MOReady()) {
            if (time(&l) > t + 3) {
                cPrintf("Modem write failed!\n");
                return;
            }
        }
    }
	com_tx(ch);

    if (parm.pace) {        /* output pacing delay in 1/1000 seconds     */
        delay(parm.pace);
    }
    ++transmitted;      /* keep track of how many chars sent */
}

/* -------------------------------------------------------------------- */
/*      portInit() sets up the interupt driven I/O package              */
/* -------------------------------------------------------------------- */
void portInit(void)
{
	if(drv_diff) return;

	com_install(cfg->mdata);
}

/* -------------------------------------------------------------------- */
/*      portExit() removes the interupt driven I/O package              */
/* -------------------------------------------------------------------- */
void portExit(void)
{
    time_t timer, current;

	if(drv_diff) return;

	time(&timer);
    time(&current);
    while (!com_tx_empty() && (current - timer < (time_t) 6)) {
        time(&current);
    }
    com_deinstall();
}
