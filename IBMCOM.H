/*---------------------------------------------------------------------------*
 *                                 ibmcom.h                                  *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: ANSI C function prototypes and other definitions for the     *
 *              routines in ibmcom.c                                         *
 *  03/19/90    FJM     Linted & partial cleanup                             *
 *---------------------------------------------------------------------------*/

/* --------------------------------------------------------------------
 *  Version control log:
 * $Log:   H:/VCS/FCIT/IBMCOM.H_V  $
 * 
 *    Rev 1.37   19 Apr 1991 23:40:00   FJM
 * No change.
 * 
 *    Rev 1.36   19 Apr 1991 23:28:10   FJM
 * No change.
 * 
 *    Rev 1.35   10 Apr 1991  9:06:54   FJM
 * No change.
 * 
 *    Rev 1.34   04 Apr 1991 14:14:46   FJM
 * No change.
 * 
 *    Rev 1.33   13 Mar 1991 15:14:12   FJM
 * No change.
 * 
 *    Rev 1.32   13 Mar 1991 15:01:50   FJM
 * No change.
 *
 *   Rev 1.31   10 Feb 1991 18:03:34   FJM
 *No change.
 *
 *   Rev 1.30   10 Feb 1991 17:34:30   FJM
 *No change.
 *
 *   Rev 1.29   05 Feb 1991 14:51:50   FJM
 *No change.
 *
 *   Rev 1.28   05 Feb 1991 14:33:12   FJM
 *No change.
 *
 *   Rev 1.27   19 Jan 1991 14:33:32   FJM
 *No change.
 *
 *   Rev 1.26   19 Jan 1991 14:17:56   FJM
 *No change.
 *
 *   Rev 1.25   18 Jan 1991 17:34:30   FJM
 *No change.
 *
 *   Rev 1.24   18 Jan 1991 17:21:10   FJM
 *No change.
 *
 *   Rev 1.23   18 Jan 1991 16:53:02   FJM
 *No change.
 *
 *   Rev 1.22   13 Jan 1991  0:59:04   FJM
 *No change.
 *
 *   Rev 1.21   13 Jan 1991  0:33:30   FJM
 *No change.
 *
 *   Rev 1.20   11 Jan 1991 13:10:52   FJM
 *No change.
 *
 *   Rev 1.19   11 Jan 1991 12:44:08   FJM
 *No change.
 *
 *   Rev 1.18   06 Jan 1991 16:52:10   FJM
 *No change.
 *
 *   Rev 1.17   06 Jan 1991 14:10:48   FJM
 *No change.
 *
 *   Rev 1.16   06 Jan 1991 13:19:10   FJM
 *No change.
 *
 *   Rev 1.15   06 Jan 1991 12:46:38   FJM
 *Porting, formating, fixes and misc changes.
 *
 *   Rev 1.14   27 Dec 1990 20:17:18   FJM
 *Changes for porting.
 *
 *     Rev 1.3   23 Nov 1990 13:25:40   FJM
 *  Header change
 *
 *     Rev 1.1   17 Nov 1990 16:12:34   FJM
 *  Added version control log header
 * -------------------------------------------------------------------- */


enum par_code   { COM_NONE, COM_EVEN, COM_ODD, COM_ZERO, COM_ONE };

int             com_carrier(void);
void            com_deinstall(void);
void            com_flush_rx(void);
void            com_flush_tx(void);
int             com_install(int portnum);
void interrupt  com_interrupt_driver();
void            com_lower_dtr(void);
void            com_raise_dtr(void);
int             com_ring(void);
unsigned char            com_rx(void);
int             com_rx_empty(void);
void            com_set_parity(enum par_code parity, int stop_bits);
void            com_set_speed(long speed);
void            com_tx(char c);
int             com_tx_empty(void);
int             com_tx_ready(void);
void            com_tx_string(char *s);

