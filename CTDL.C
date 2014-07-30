/************************************************************************
 *                              ctdl.c
 *              Command-interpreter code for Citadel
 ************************************************************************/

#define MAIN

#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "response.h"

#ifndef ATARI_ST
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <alloc.h>
#endif

#include "keywords.h"
#include "proto.h"
#include "global.h"

unsigned int _stklen = 1024 * 18;

/* unsigned int _ovrbuffer = 0x800; */

void interrupt ( *oldhandler)(void);

extern void interrupt criterr(unsigned bp,unsigned si,unsigned ds,unsigned es,unsigned dx,
	unsigned cx,unsigned bx,unsigned ax);
	
/************************************************************************
 *                              Contents
 *
 *      main()                  has the central menu code
 *
 ************************************************************************/


/* --------------------------------------------------------------------
 *  History:
 *
 *  03/08/90    {zm}    Add 'softname' as the name of the software.
 *  03/15/90    {zm}    Add [title] name [surname], 30 characters long.
 *  03/19/90    FJM     Linted & partial cleanup
 *  05/15/90    FJM     Changed Next/previous room/hall prompts
 *  05/17/90    FJM     Added external commands
 *  05/18/90    FJM     Made = an alias for + at room prompt
 *  05/20/90    FJM     Improved external commands. Message if can't enter
 *                      surname.  Made NOCHAT.BLB,NOCHAT1.BLB... cycle
 *  06/06/90    FJM     Changed strftime to cit_strftime
 *  06/15/90    FJM     Added option for modem output pacing, -p.
 *                      Added help display if incorrect option.
 *                      Mods for running from another BBS as a door.
 *  06/16/90    FJM     Made IBM Graphics characters a seperate option.
 *  06/20/90    FJM     Moved most functions to COMMAND.C
 *  07/22/90    FJM     Added -b (baud) option.
 *  07/23/90    FJM     Added -v (events) option.
 *  07/24/90    FJM     Increased overlay buffer size from 22k to 32k.
 *  11/23/90    FJM     Changes to login for shell mode.
 *  11/24/90    FJM     Changes for shell/door mode.
 *  01/06/91    FJM     Added -l parameter.  Made -d work.
 *  01/13/91    FJM     Name overflow fixes.
 *  01/19/91    FJM     Added time display, and properly init parm struct.
 *  02/10/91    FJM     Changes to shell mode carrier detect.
 *  05/06/91    FJM     Added new prompt code.
 *	08/12/91	BLJ 	Allow netting to sub-board
 *
 * -------------------------------------------------------------------- */

/************************************************************************
 *              External function definitions for CTDL.C
 ************************************************************************/

void greeting(void);
char doRegular(char x, char c);
char doSysop(void);
char getCommand(char *c);
void doLogin(char moreYet);
static void command_loop(void);

/************************************************************************
 *      command_loop() contains the central menu code
 ************************************************************************/

static void command_loop(void)
{
	char c, more = FALSE, help = FALSE;
    int i;

    while (!ExitToMsdos) {
        if (sysReq && !loggedIn && !haveCarrier && !ExitToMsdos) {
            sysReq = FALSE;
			if (cfg->offhook) {
                offhook();
            } else {
                drop_dtr();
            }
            ringSystemREQ();
        }
		/* using #forcelogin in door mode */
		if (!loggedIn && parm.door && cfg->forcelogin) {
            for (i = 0; !loggedIn && i < 4; ++i)
                doLogin(2);
            if (!loggedIn)
                ExitToMsdos = 1;
        }
        if (!ExitToMsdos) {
			if(logBuf.lbflags.NODE && parm.door && haveCarrier) {
				newCarrier = FALSE;
				c = '\0';
			}
			else {
				more = getCommand(&c);

				outFlag = IMPERVIOUS;

				if (chatkey)
					chat();
				if (eventkey && !haveCarrier) {
					do_cron(CRON_TIMEOUT);
					eventkey = FALSE;
				}
			}
			if (sysopkey)
                help = doSysop();
            else
                help = doRegular(more, c);

            if (help) {
                if (!expert)
                    mPrintf("\n '?' for menu, 'H' for help.\n \n");
                else
                    mPrintf(" ?\n \n");
			}
        }
    }
}

/************************************************************************
 *      main() Initialize & start citadel
 ************************************************************************/
void main(int argc, char *argv[])
{
    int i, cfg_ctdl = FALSE;
    long b;
    char init_baud;
    static char prompt[92];
    char *envprompt;

	if ((cfg = farcalloc(1UL, (ulong) sizeof(struct config))) == NULL) {
		crashout("Can not allocate memory for config");
    }

	cfg->bios = 1;

	memset(&parm,0,sizeof(parm));
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '/'
        || argv[i][0] == '-') {
            switch (tolower(argv[i][1])) {
#ifndef ATARI_ST
                case 'd':       /* DesqView/TopView     */
                    parm.dv = 1;
                    printf("DesqView/TopView mode\n");
                    break;

                case 'b':       /* baud rate (for shell) */
                    if (argv[i + 1]) {
						b = atol(argv[++i]);
                        for (init_baud = 0; bauds[init_baud]; ++init_baud)
                            if (bauds[init_baud] == b) {
                                parm.baud = init_baud;
                                printf("Initial baud rate fixed at %ld\n", b);
                                break;
							}
                    }
                    break;
#endif
                case 'c':       /* Configure system     */
                    cfg_ctdl = TRUE;
                    break;

                case 'p':       /* pace output          */
                    if (argv[i + 1]) {
                        parm.pace = atoi(argv[++i]);
                        printf("Output pacing %d\n", parm.pace);
                    }
                    break;

                case 's':       /* run in shell from another BBS (door).        */
                    printf("Shell mode\n");
                    parm.door = TRUE;
                    break;
/*#ifndef ATARI_ST*/
#ifndef FLOPPY
                case 'e':       /* use EMS                      */
                    if (_OvrInitEms(0, 0, 0)) {
                        printf("EMS memory initialization failed!\n");
                        parm.ems = 1;
                    }
                    break;
#endif
/*#endif*/
				case 'r':
                    cfg_ctdl = TRUE;
					parm.config = 1;
					break;

				case 'v':       /* just do events       */
                    parm.events = 1;
                    break;
/*#ifndef ATARI_ST*/
#ifndef FLOPPY
                case 'x':       /* use extended memory */
					if (_OvrInitExt(0, 0))
                        printf("Extended memory initialization failed!\n");
                    parm.ext = 1;
                    break;
#endif
/*#endif*/
				case 'l':		/* log in user */
                    if (argv[i + 1]) {
						parm.login = argv[++i];
						printf("Auto-login\n");
					}
					break;
					
				case 'u':		/* log in user */
                    if (argv[i + 1]) {
						parm.user = argv[++i];
						printf("Auto-login %s\n", parm.user);
					}
					break;
					
                default:
                    printf("\nUnknown commandline switch '%s'.\n", argv[i]);
                    printf("Valid DOS command line switches:\n");
                    printf("    -b baud   Starting baud rate (300-19200)\n");
                    printf("    -c        Read configuration files\n");
                    printf("    -d        DesqView/TopView\n");
#ifndef FLOPPY
                    printf("    -e        Use EMS memory for overlays\n");
#endif
					printf("    -l str    Log in using initials;password in str\n");
                    printf("    -p num    Set output pacing to num\n");
					printf("    -r        Just re-config and exit\n");
					printf("    -s        Run as a shell from another BBS\n");
					printf("    -u 'name' Log in using specifed user name\n");
                    printf("    -v        Just run cron events\n");
#ifndef FLOPPY
					printf("    -x        Use extended memory for overlays (386/486 only!)\n");
#endif
                    exit(1);
            }
        }
    }

    if (cfg_ctdl)
		unlink("etc.tab");

	cfg->attr = 15; 	  /* logo gets bright white letters */

    setscreen();

    logo();

	if (time(NULL) < 727168424L) {
        cPrintf("\n\nPlease set your time and date!\n");
        exit(1);
    }
	/* set prompt for shells */
    envprompt = getenv("PROMPT");
	if (!envprompt)
		envprompt = "$p$g";
	sprintf(prompt, "PROMPT=\r\nType EXIT to return to CenCit\r\n%s", envprompt);
    if (putenv(prompt)) {
        cPrintf("\n\nCan not set DOS prompt!\n");
		delay (5000);
	}
	/* initialize citadel */
    initCitadel();

    if (parm.baud) {
		cfg->initbaud = parm.baud;
		baud(cfg->initbaud);
    }
	if (parm.door) {
		carrier();
		if (haveCarrier) {
			carrdetect();
			newCarrier = 1;		/* make hello blurb show up */
		}
    }
	if ((!gotCarrier() && (parm.login || parm.user)) || drv_diff) {
		whichIO = CONSOLE;
		onConsole = (char) (whichIO == CONSOLE);
		if (cfg->offhook)
			offhook();
	}
    /* We need to test for Bells off here */
	changedir(cfg->aplpath);
	if(filexists("input.apl"))
		getBellStatus();

	greeting();

	sysReq = FALSE;

	if (cfg->f6pass[0])
		ConLock = TRUE;

	if (parm.dv) {
		cfg->bios = 1;
		directvideo = 0;
	}

	if (parm.login) {
		normalizeString(parm.login);	/* normalize string in environment */
		login(parm.login,NULL);
	}

	if (parm.user && !loggedIn) {
		normalizeString(parm.user);	/* normalize string in environment */
		if (findPerson(parm.user, &lBuf) != ERROR)
			login(lBuf.lbin,lBuf.lbpw);
	}

		/* read in door interface files */
	if (parm.door) {
		readDoorFiles(0);
	}
	/* update25();	*/
	do_idle(0);

	/* install critical error handler */
	harderr(cit_herror_handler);
	
	/* execute main command loop */
	if (!parm.events && !parm.config)
        command_loop();
    else {
		if (!parm.config)
			while (do_cron(CRON_TIMEOUT));
    }

    exitcitadel();
}

/* EOF */

