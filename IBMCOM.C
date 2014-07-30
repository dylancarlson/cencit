/*---------------------------------------------------------------------------*
 *                                 ibmcom.c                                  *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: This file contains a set of routines for doing low-level     *
 *              serial communications on the IBM PC.  It was translated      *
 *              directly from Wayne Conrad's IBMCOM.PAS version 3.1, with    *
 *              the goal of near-perfect functional correspondence between   *
 *              the Pascal and C versions.                                   *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Original translation from IBMCOM.PAS, with *
 *                                liberal plagiarism of comments from the    *
 *                                Pascal.                                    *
 *                                                                           *
 * 2/4/90 - RGJ - Added a ring detect function com_ring().                   *
 *                Modified com_speed() to take a long for the baud rate      *
 *                Changed com_deinstall() to not change the MODEM CONTROL    *
 *                    REGISTERS when exiting, needed to shell to DOS         *
 *                Reformatted code.                                          *
 *
 * 03/18/90 FJM     Made com_rs() unsigned.
 * 03/19/90 FJM     Linted & partial cleanup                                 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <dos.h>
#include "ibmcom.h"

/*---------------------------------------------------------------------------*
 *                             8250 Definitions                              *
 *---------------------------------------------------------------------------*/

/*      Offsets to various 8250 registers.  Taken from IBM Technical         */
/*      Reference Manual, p. 1-225                                           */

#define TXBUFF  0
#define RXBUFF  0
#define DLLSB   0
#define DLMSB   1
#define IER     1
#define IIR     2
#define LCR     3
#define MCR     4
#define LSR     5
#define MSR     6

/*      Modem control register bits                                          */

#define DTR     0x01
#define RTS     0x02
#define OUT1    0x04
#define OUT2    0x08
#define LPBK    0x10

/*      Modem status register bits                                           */

#define DCTS    0x01
#define DDSR    0x02
#define TERI    0x04
#define DRLSD   0x08
#define CTS     0x10
#define DSR     0x20
#define RI      0x40
#define RLSD    0x80

/*      Line control register bits                                           */

#define DATA5   0x00
#define DATA6   0x01
#define DATA7   0x02
#define DATA8   0x03

#define STOP1   0x00
#define STOP2   0x04

#define NOPAR   0x00
#define ODDPAR  0x08
#define EVNPAR  0x18
#define STKPAR  0x28
#define ZROPAR  0x38

/*      Line status register bits                                            */

#define RDR     0x01
#define ERRS    0x1E
#define TXR     0x20

/*      Interrupt enable register bits                                       */

#define DR      0x01
#define THRE    0x02
#define RLS     0x04

/*---------------------------------------------------------------------------*
 *                             Names for Numbers                             *
 *---------------------------------------------------------------------------*/

#define MAX_PORT        4

#define TRUE            1
#define FALSE           0

/*---------------------------------------------------------------------------*
 *                                Global Data                                *
 *---------------------------------------------------------------------------*/

/*  UART i/o addresses.  Values depend upon which COMM port is selected  */

int uart_data;          /* Data register */
int uart_ier;           /* Interrupt enable register */
int uart_iir;           /* Interrupt identification register */
int uart_lcr;           /* Line control register */
int uart_mcr;           /* Modem control register */
int uart_lsr;           /* Line status register */
int uart_msr;           /* Modem status register */

char com_installed;     /* Flag: Communications routines installed */
int intnum;         /* Interrupt vector number for chosen port */
char i8259bit;          /* 8259 bit mask */
char old_i8259_mask;        /* Copy as it was when we were called */
char old_ier;           /* Modem register contents saved for */
char old_mcr;           /* restoring when we're done */
void interrupt(*old_vector) (); /* Place to save old COM# vector */

/*  Transmit queue.  Characters to be transmitted are held here until the  */
/*  UART is ready to transmit them.  */

#define TX_QUEUE_SIZE   48

char tx_queue[TX_QUEUE_SIZE];
int tx_in;          /* Index of where to store next character */
int tx_out;         /* Index of where to retrieve next character */
int tx_chars;           /* Count of characters in queue */

/*  Receive queue.  Received characters are held here until retrieved by  */
/*  com_rx()  */

#define RX_QUEUE_SIZE   256

char rx_queue[RX_QUEUE_SIZE];
int rx_in;          /* Index of where to store next character */
int rx_out;         /* Index of where to retrieve next character */
int rx_chars;           /* Count of characters in queue */

/*---------------------------------------------------------------------------*
 *                               com_install()                               *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Installs the communications drivers.                         *
 *                                                                           *
 * SYNOPSIS:    status = com_install(int portnum);                           *
 *              int     portnum;        Desired port number                  *
 *              int     status;         0 = Successful installation          *
 *                                      1 = Invalid port number              *
 *                                      2 = No UART for specified port       *
 *                                      3 = Drivers already installed        *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from IBMCOM.PAS                 *
 *---------------------------------------------------------------------------*/

const int uart_base[] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};
const char intnums[] = {0x0C, 0x0B, 0x0C, 0x0B};
const char i8259levels[] = {4, 3, 4, 3};

int com_install(int portnum)
{

    if  (com_installed)     /* Drivers already installed */
        return 3;
    if ((portnum < 1) || (portnum > MAX_PORT))  /* Port number out of bounds */
        return 1;

    uart_data = uart_base[portnum - 1]; /* Set UART I/O addresses */
    uart_ier = uart_data + IER; /* for the selected comm */
    uart_iir = uart_data + IIR; /* port */
    uart_lcr = uart_data + LCR;
    uart_mcr = uart_data + MCR;
    uart_lsr = uart_data + LSR;
    uart_msr = uart_data + MSR;
    intnum = intnums[portnum - 1];  /* Ditto for interrupt */
    i8259bit = 1 << i8259levels[portnum - 1];   /* vector and 8259 bit mask */

    old_ier = inportb(uart_ier);/* Return an error if we */
    outportb(uart_ier, 0);  /* can't access the UART */
    if (inportb(uart_ier) != 0)
        return 2;

    disable();          /* Save the original 8259 */
    old_i8259_mask = inportb(0x21); /* mask, then disable the */
    outportb(0x21, old_i8259_mask | i8259bit);  /* 8259 for this interrupt */
    enable();

    com_flush_tx();     /* Clear the transmit and */
    com_flush_rx();     /* receive queues */

    old_vector = getvect(intnum);   /* Save old COMM vector, */
    setvect(intnum, com_interrupt_driver);  /* then install a new one, */
    com_installed = TRUE;   /* and note that we did */

    outportb(uart_lcr, DATA8 + NOPAR + STOP1);  /* 8 data, no parity, 1 stop */

    disable();          /* Save MCR, then enable */
    old_mcr = inportb(uart_mcr);/* interrupts onto the bus, */
    outportb(uart_mcr,      /* activate RTS and leave */
    (old_mcr & DTR) | (OUT2 + RTS));   /* DTR the way it was */
    enable();

    outportb(uart_ier, DR); /* Enable receive interrupts */

    disable();          /* Now enable the 8259 for */
    outportb(0x21, inportb(0x21) & ~i8259bit);  /* this interrupt */
    enable();
    return 0;           /* Successful installation */
}               /* End com_install() */

/*---------------------------------------------------------------------------*
 *                              com_deinstall()                              *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: De-installs the communications drivers completely, without   *
 *              changing the baud rate or DTR.  It tries to leave the        *
 *              interrupt vectors and enables and everything else as they    *
 *              were when the driver was installed.                          *
 *                                                                           *
 * NOTE:        This function MUST be called before returning to DOS, so the *
 *              interrupt vector won't point to our driver anymore, since it *
 *              will surely get overwritten by some other transient program  *
 *              eventually.                                                  *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from IBMCOM.PAS                 *
 *---------------------------------------------------------------------------*/

void com_deinstall(void)
{

    if  (com_installed) {   /* Don't de-install twice! */
    /* don't mess with these!!! */
    /* outportb(uart_mcr, old_mcr); *//* Restore the UART */
        outportb(uart_ier, old_ier);    /* registers ... */
        disable();      /* ... the 8259 interrupt */
        outportb(0x21,      /* mask ... */
        (inportb(0x21) & ~i8259bit) | (old_i8259_mask & i8259bit));
        enable();
        setvect(intnum, old_vector);    /* ... and the comm */
        com_installed = FALSE;  /* interrupt vector */
    }               /* End com_installed */
}               /* End com_deinstall() */

/*---------------------------------------------------------------------------*
 *                              com_set_speed()                              *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Sets the baud rate.                                          *
 *                                                                           *
 * SYNOPSIS:    void com_set_speed(unsigned speed);                          *
 *              unsigned speed;                 Desired baud rate            *
 *                                                                           *
 * NOTES:       The input parameter can be anything between 2 and 65535.     *
 *              However, I (Wayne) am not sure that extremely high speeds    *
 *              (those above 19200) will always work, since the baud rate    *
 *              divisor will be six or less, where a difference of one can   *
 *              represent a difference in baud rate of 3840 bits per second  *
 *              or more.)                                                    *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from IBMCOM.PAS                 *
 *---------------------------------------------------------------------------*/

void com_set_speed(long speed)
{
    unsigned divisor;       /* A local temp */

    if (com_installed) {    /* Force proper input */
        if (speed < 2)      /* Recond baud rate divisor */
            speed = 2;      /* Interrupts off */
        divisor = (unsigned) (115200L / speed); /* Set up to load baud rate */
        disable();      /* divisor into UART */
        outportb(uart_lcr, inportb(uart_lcr) | 0x80);
        outport(uart_data, divisor);    /* Do so */
        outportb(uart_lcr, inportb(uart_lcr) & ~0x80);
        enable();       /* Back to normal UART ops */
    }               /* Interrupts back on */
}               /* End "comm installed" */

 /* End com_set_speed() */

/*---------------------------------------------------------------------------*
 *                             com_set_parity()                              *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Sets the parity and stop bits.                               *
 *                                                                           *
 * SYNOPSIS:    void com_set_parity(enum par_code parity, int stop_bits);    *
 *              int     code;           COM_NONE = 8 data bits, no parity    *
 *                                      COM_EVEN = 7 data, even parity       *
 *                                      COM_ODD  = 7 data, odd parity        *
 *                                      COM_ZERO = 7 data, parity bit = zero *
 *                                      COM_ONE  = 7 data, parity bit = one  *
 *              int     stop_bits;      Must be 1 or 2                       *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal                 *
 *---------------------------------------------------------------------------*/

static const char lcr_vals[] = {DATA8 + NOPAR, DATA7 + EVNPAR, DATA7 + ODDPAR,
DATA7 + STKPAR, DATA7 + ZROPAR};

void com_set_parity(enum par_code parity, int stop_bits)
{
    disable();
    outportb(uart_lcr, lcr_vals[parity] | ((stop_bits == 2) ? STOP2 : STOP1));
    enable();
}               /* End com_set_parity() */

/*---------------------------------------------------------------------------*
 *                              com_raise_dtr()                              *
 *                              com_lower_dtr()                              *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: These routines raise and lower the DTR line.  Lowering DTR   *
 *              causes most modems to hang up.                               *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Transltated from the Pascal.               *
 *---------------------------------------------------------------------------*/

void com_lower_dtr(void)
{
    if  (com_installed) {
        disable();
        outportb(uart_mcr, inportb(uart_mcr) & ~DTR);
        enable();
    }               /* End 'comm installed' */
}               /* End com_raise_dtr() */

void com_raise_dtr(void)
{
    if  (com_installed) {
        disable();
        outportb(uart_mcr, inportb(uart_mcr) | DTR);
        enable();
    }               /* End 'comm installed' */
}               /* End com_lower_dtr() */

/*---------------------------------------------------------------------------*
 *                                 com_tx()                                  *
 *                              com_tx_string()                              *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Transmit routines.  com_tx() sends a single character by     *
 *              waiting until the transmit buffer isn't full, then putting   *
 *              the character into it.  The interrupt driver will then send  *
 *              the character once it is at the head of the transmit queue   *
 *              and a transmit interrupt occurs.  com_tx_string() sends a    *
 *              string by repeatedly calling com_tx().                       *
 *                                                                           *
 * SYNOPSES:    void    com_tx(char c);         Send the character c         *
 *              void    com_tx_string(char *s); Send the string s            *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal                 *
 *---------------------------------------------------------------------------*/

void com_tx(char c)
{
    if  (com_installed) {   /* Wait for non-full buffer */
        while (!com_tx_ready());/* Interrupts off */
        disable();      /* Stuff character in queue */
        tx_queue[tx_in++] = c;  /* Wrap index if needed */
        if (tx_in == TX_QUEUE_SIZE)
            tx_in = 0;      /* Number of char's in queue */
        tx_chars++;     /* Enable UART tx interrupt */
        outportb(uart_ier, inportb(uart_ier) | THRE);
        enable();       /* Interrupts back on */
    }               /* End 'comm installed' */
}               /* End com_tx() */

void com_tx_string(char *s)
{
    while (*s)
        com_tx(*s++);
}               /* Send the string! */

 /* End com_tx_string() */
/*---------------------------------------------------------------------------*
 *                                 com_rx()                                  *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Returns the next character from the receive buffer, or a     *
 *              NULL character ('\0') if the buffer is empty.                *
 *                                                                           *
 * SYNOPSIS:    c = com_rx();                                                *
 *              char    c;                      The returned character       *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal.                *
 *---------------------------------------------------------------------------*/

unsigned char com_rx(void)
{

    unsigned char rv;       /* Local temp */

    if (!rx_chars || !com_installed)    /* Return NULL if receive */
        return '\0';        /* buffer is empty */
    disable();          /* Interrupts off */
    rv = rx_queue[rx_out++];    /* Grab char from queue */
    if (rx_out == RX_QUEUE_SIZE)/* Wrap index if needed */
        rx_out = 0;
    rx_chars--;         /* One less char in queue */
    enable();           /* Interrupts back on */
    return rv;          /* The answer! */
}               /* End com_rx() */

/*---------------------------------------------------------------------------*
 *                           Queue Status Routines                           *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Small routines to return status of the transmit and receive  *
 *              queues.                                                      *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal.                *
 *---------------------------------------------------------------------------*/

int com_tx_ready(void)
{               /* Return TRUE if the */
    return ((tx_chars < TX_QUEUE_SIZE) ||   /* transmit queue can */
    (!com_installed));  /* accept a character */
}               /* End com_tx_ready() */

int com_tx_empty(void)
{               /* Return TRUE if the */
    return (!tx_chars || (!com_installed)); /* transmit queue is empty */
}               /* End com_tx_empty() */

int com_rx_empty(void)
{               /* Return TRUE if the */
    return (!rx_chars || (!com_installed)); /* receive queue is empty */
}               /* End com_rx_empty() */

/*---------------------------------------------------------------------------*
 *                              com_flush_tx()                               *
 *                              com_flush_rx()                               *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Buffer flushers!  These guys just initialize the transmit    *
 *              and receive queues (respectively) to their empty state.      *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal                 *
 *---------------------------------------------------------------------------*/

void com_flush_tx()
{
    disable();
    tx_chars = tx_in = tx_out = 0;
    enable();
}

void com_flush_rx()
{
    disable();
    rx_chars = rx_in = rx_out = 0;
    enable();
}

/*---------------------------------------------------------------------------*
 *                               com_carrier()                               *
 *                                com_ring()                                 *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: com_carrier returns TRUE if a carrier is present.            *
 *              com_ring returns TRUE if ring detect is high.                *
 *                                                                           *
 * REVISIONS:   18 OCT 89 - RAC - Translated from the Pascal.                *
 *---------------------------------------------------------------------------*/

int com_carrier(void)
{
    return com_installed && (inportb(uart_msr) & RLSD);
}               /* End com_carrier() */

int com_ring(void)
{
    return com_installed && (inportb(uart_msr) & RI);
}               /* End com_ring() */

/*---------------------------------------------------------------------------*
 *                          com_interrupt_driver()                           *
 *---------------------------------------------------------------------------*
 * DESCRIPTION: Handles communications interrupts.  The UART will interrupt  *
 *              whenever a character has been received or when it is ready   *
 *              to transmit another character.  This routine responds by     *
 *              sticking received characters into the receive queue and      *
 *              yanking characters to be transmitted from the transmit queue *
 *                                                                           *
 * REVISIOSN:   18 OCT 89 - RAC - Translated from the Pascal.                *
 *---------------------------------------------------------------------------*/

void interrupt com_interrupt_driver()
{

    char iir;           /* Local copy if IIR */
    char c;         /* Local character variable */

/*  While bit 0 of the IIR is 0, there remains an interrupt to process  */

    while (!((iir = inportb(uart_iir)) & 1)) {  /* While there is an int ... */
        switch (iir) {      /* Branch on interrupt type */
            case 0:     /* Modem status interrupt */
                inportb(uart_msr);  /* Just clear the interrupt */
                break;

            case 2:     /* Transmit register empty */

/*---------------------------------------------------------------------------*
 *  NOTE:  The test of the line status register is to see if the transmit    *
 *         holding register is truly empty.  Some UARTS seem to cause        *
 *         transmit interrupts when the holding register isn't empty,        *
 *         causing transmitted characters to be lost.                        *
 *---------------------------------------------------------------------------*/
        /* If tx buffer empty, turn */
                if (tx_chars <= 0)  /* off transmit interrupts */
                    outportb(uart_ier, inportb(uart_ier) & ~2);
                else {      /* Tx buffer not empty */
                    if (inportb(uart_lsr) & TXR) {
                        outportb(uart_data, tx_queue[tx_out++]);
                        if (tx_out == TX_QUEUE_SIZE)
                            tx_out = 0;
                        tx_chars--;
                    }
                }           /* End 'tx buffer not empty */
                break;

            case 4:     /* Received data interrupt */
                c = inportb(uart_data); /* Grab received character */
                if (rx_chars < RX_QUEUE_SIZE) { /* If queue not full, save */
                    rx_queue[rx_in++] = c;  /* the new character */
                    if (rx_in == RX_QUEUE_SIZE) /* Wrap index if needed */
                        rx_in = 0;
                    rx_chars++; /* Count the new character */
                }           /* End queue not full */
                break;

            case 6:     /* Line status interrupt */
                inportb(uart_lsr);  /* Just clear the interrupt */
                break;

        }           /* End switch */
    }               /* End 'is an interrupt' */
    outportb(0x20, 0x20);   /* Send EOI to 8259 */
}               /* End com_interrupt_driver() */
