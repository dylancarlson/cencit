/* -------------------------------------------------------------------- */
/*  CRON.C                       Citadel                                */
/* -------------------------------------------------------------------- */
/*  This file contains all the code to deal with the cron events        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <alloc.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*                                                                      */
/*  readcron()      reads cron.cit values into events structure         */
/*  do_cron()       called when the system is ready to do an event      */
/*  cando_event()   Can we do this event?                               */
/*	do_event()		Actually do this event								*/
/*  list_event()    List all events                                     */
/*  cron_commands() Sysop Fn: Cron commands                             */
/*  zap_event()     Zap an event out of the cron list                   */
/*	unzap_event()	Zap an event out of the cron list					*/
/*  reset_event()   Reset an even so that it has not been done          */
/*  done_event()    Set event so it seems to have been done             */
/*  did_net()       Set all events for a node to done                   */
/*                                                                      */
/* -------------------------------------------------------------------- */


/* --------------------------------------------------------------------
 *
 *  HISTORY:
 *
 *  05/07/89    (PAT)   Made history, cleaned up comments, reformated
 *                      icky code. Also added F6CAll Done
 *  03/19/90    FJM     Linted & partial cleanup
 *  04/17/90    FJM     Clean up on readcron()
 *  06/06/90    FJM     Changed strftime to cit_strftime
 *  06/09/90    FJM     Added readCrontab & writeCrontab
 *  07/30/90    FJM     Corrected bug in readcron() case for any day,
 *                      was expecting MAXGROUPS days, not 7!
 *  08/20/90    FJM     EXIT events added.
 *  12/01/90    FJM     Added support for local netting.
 *  12/02/90    FJM     Enhanced local network events.
 *  12/09/90    FJM     Prettier cron table.
 *  01/13/91    FJM     Name overflow fixes.
 *  01/28/91    FJM     Improved cron list display. Added cronNextEvent().
 *  02/05/91    FJM     Changes for screen saver.
 *	06/12/91	BLJ 	Added Un-zap ability
 *	06/18/91	BLJ 	Added 1/4 hour accuracy for cron exents
 *	07/03/91	BLJ 	Added Sound Code
 *	09/14/91	BLJ 	Dynamically Linked Lists for events
 *
 * -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static int on_event = 0;
static int numevents = 0;

static void do_event(int evnt);
static void done_event(void);
static int cronNextEvent(int evnt);
static void list_event(void);
static int cando_event(int evnt);
static void zap_event(void);
static void unzap_event(void);
static void reset_event(void);
/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */
extern int local_mode;

/* -------------------------------------------------------------------- */
/*      readcron()     reads cron.cit values into events structure      */
/* -------------------------------------------------------------------- */
void readcron(void)
{
    FILE *fBuf;
    char line[91];
	char temp[10];
	char *words[50], *ptr;
    int i, j, k, l, count;
    int cronslot = ERROR;
	uchar hours, minutes, range, end_minutes, end_hour;

	struct event *save_event;

	/* move to home-path */
	changedir(cfg->homepath);

    if ((fBuf = fopen("cron.cit", "r")) == NULL) {  /* ASCII mode */
        cPrintf("Can't find Cron.cit!");
        doccr();
        exit(1);
    }
    on_event = 0;
    line[90] = '\0';

	/*	We are re-reading the table so......
	 *	Let's release the memory and start over
	 */
	if(start_event) {
		save_event = start_event->next;
        while(save_event) {
			events = save_event;
            save_event = save_event->next;
			farfree(events);
		}
		start_event->next = NULL;
	}
	else {
		crashout("Lost the starting cron event pointer");
	}
	events = start_event;

	events->e_type = ERROR;

    while (fgets(line, 90, fBuf)) {
        if (line[0] != '#')
            continue;

        count = parse_it(words, line);

        for (i = 0; cronkeywords[i]; i++) {
            if (strcmpi(words[0], cronkeywords[i]) == SAMESTRING) {
                break;
            }
        }

        switch (i) {
            case CR_DAYS:
                if (cronslot == ERROR)
                    break;

				/* init days */
                for (j = 0; j < 7; j++)
					events->e_days[j] = 0;

                for (j = 1; j < count; j++) {
                    for (k = 0; daykeywords[k]; k++) {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING) {
                            break;
                        }
                    }
                    if (k < 7)
						events->e_days[k] = TRUE;
                    else if (k == 7) {  /* any */
						for (l = 0; l < 7; ++l)
							events->e_days[l] = TRUE;
                    } else {
                        doccr();
                        cPrintf("Cron.cit - Warning: Unknown day %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case CR_DO:
                cronslot = (cronslot == ERROR) ? 0 : (cronslot + 1);

                if (cronslot > MAXCRON) {
                    doccr();
                    illegal("Cron.Cit - too many entries");
                }
				if(cronslot > 0) {
                    if(events->next == NULL) {
						events->next = farcalloc(1UL,(ulong) sizeof(struct event));
						if(events->next == NULL) {
							crashout("Can not allocate memory for cron events");
						}
						events = events->next;
						events->next = NULL;
					}
					else events = events->next;
				}
				events->e_type = ERROR;

				for (k = 0; crontypes[k] != NULL; k++) {
                    if (strcmpi(words[1], crontypes[k]) == SAMESTRING)
						events->e_type = (uchar) k;
                }

				strncpy(events->e_str, words[2], NAMESIZE);
				events->e_str[NAMESIZE-1] = '\0';
				events->l_sucess = (long) 0;
				events->l_try = (long) 0;
                break;

            case CR_HOURS:
                if (cronslot == ERROR)
                    break;

				/* init hours */
				for (j = 0; j < 96; j++)
					events->e_hours[j] = 0;

                for (j = 1; j < count; j++) {
                    if (strcmpi(words[j], "Any") == SAMESTRING) {
						for (l = 0; l < 96; ++l)
							events->e_hours[l] = TRUE;
                    } else {
						range = FALSE;
						if(strchr(words[j],'-')) {
							range = TRUE;
							strcpy(temp,strchr(words[j],'-')+1);
							if((ptr = strchr(temp,':')) != NULL) {
								end_minutes = atoi(ptr+1) / 15;
								*ptr = 0;
							}
							else {
								end_minutes = 0;
							}
							end_hour = atoi(temp);
							ptr = strchr(words[j],'-');
							*ptr = 0;
						}
						if((ptr = strchr(words[j],':')) != NULL) {
							minutes = atoi(ptr+1) / 15;
							*ptr = 0;
						}
						else {
							minutes = 0;
						}
						hours = atoi(words[j]);

						if (hours > 23) {
                            doccr();
                            cPrintf("Cron.Cit - Warning: Invalid hour %d ",
							hours);
                            doccr();
                        } else
							if(range) {
								l=(hours*4) + minutes;
								while(TRUE) {
									events->e_hours[l] = TRUE;
									l++;
									if(l == 96) l = 0;
									if(l ==(end_hour*4)+end_minutes) break;
                                }
                            }
							else {
								hours = (hours*4) + minutes;
								events->e_hours[hours] = TRUE;
							}
					}
                }
                break;

            case CR_REDO:
                if (cronslot == ERROR)
                    break;

				events->e_redo = atoi(words[1]);
                break;

            case CR_RETRY:
                if (cronslot == ERROR)
                    break;

				events->e_retry = atoi(words[1]);
                break;

            default:
                cPrintf("Cron.cit - Warning: Unknown variable %s", words[0]);
                doccr();
                break;
        }
    }
    fclose(fBuf);

    numevents = cronslot;

	events = start_event;
	while(events->next) {
		events->l_sucess = (long) 0;
		events->l_try = (long) 0;
		events->e_zap = 0;
		events = events->next;
	}
}

/* --------------------------------------------------------------------
 *  cronNextEvent()		Set next cron event to do.
 * -------------------------------------------------------------------- */
int cronNextEvent(int evnt)
{
	if(evnt < 0 || evnt > numevents)
		return ERROR;
	else {
		on_event = evnt;
		return SUCCESS;
	}
}

/* -------------------------------------------------------------------- */
/*  do_cron()       called when the system is ready to do an event      */
/* -------------------------------------------------------------------- */
int do_cron(int why_called)
{
	int was_event, done, i;

    why_called = why_called; /* to prevent a -W3 warning */

    was_event = on_event;
    done = FALSE;

    do {
        if (cando_event(on_event)) {
			do_idle(0); 		/* clear screen save state	*/
			restore_sound();	/* Saves 32K - Important!!	*/
			do_event(on_event);
			init_sound();		/* We MUST have the sound!! */
			do_idle(2); 		/* reset screen saver state */
            done = TRUE;
        }
		on_event = on_event >= numevents ? 0 : on_event + 1;
    } while (!done && on_event != was_event);

	events = start_event;
	if(was_event) {
		for(i=0; i<was_event; i++) events = events->next;
	}
	if (!done || (events->e_type == CR_EXIT)) {
		/* done or exit event dumps loops */
        if (debug)
            mPrintf("No Job> ");

		/* Host Only */
		Initport();

		return FALSE;
    }
    doCR();
	return TRUE;
}

/* -------------------------------------------------------------------- */
/*  cando_event()   Can we do this event?                               */
/* -------------------------------------------------------------------- */
int cando_event(int evnt)
{
	int i;
	long l, time();

	/* not a valid (possible zapped) event */
	events = start_event;
	if(evnt) {
		for(i=0; i<evnt; i++) events = events->next;
	}
	if (events->e_zap)
        return FALSE;

    /* not right time || day */
	if (!events->e_hours[((hour()*4)+quarter())] ||
		!events->e_days[dayofweek()])
        return FALSE;

    /* already done, wait a little longer */
	if ((time(&l) - events->l_sucess) / (long) 60 < (long) events->e_redo)
        return FALSE;

    /* didnt work, give it time to get un-busy */
	if ((time(&l) - events->l_try) / (long) 60 < (long) events->e_retry)
        return FALSE;

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*	do_event()		Actually do this event								*/
/* -------------------------------------------------------------------- */
void do_event(int evnt)
{
	int i;
	long time(), l;

	struct event *save_event;

    events = start_event;
	if(evnt) {
		for(i=0; i<evnt; i++) events = events->next;
    }

	if(!events->e_zap) {
		switch (events->e_type) {
			case CR_SHELL_1:
				mPrintf("SHELL: \"%s\"", events->e_str);
				if (changedir(cfg->aplpath) == ERROR) {
					mPrintf("  -- Can't find application directory.\n\n");
					changedir(cfg->homepath);
					return;
				}
				apsystem(events->e_str);
				changedir(cfg->homepath);
				events->l_sucess = time(&l);
				events->l_try = time(&l);

				/* Host Only */
				Initport();
				break;
			case CR_LOCAL:
				if (!drv_diff) {
					if (!loggedIn) {
						local_mode = TRUE;
						mPrintf("LOCAL: with \"%s\"", events->e_str);
						save_event = events;
						net_callout(events->e_str);
						events = save_event;
						events->l_try = time(&l);
					} else {
						mPrintf("Can not network with user logged in.\n ");
					}
				}
				break;
			case CR_NET:
				if (!drv_diff) {
					/* Host Only */
					if (!loggedIn) {
						mPrintf("NETWORK: with \"%s\"", events->e_str);
						save_event = events;
						net_callout(events->e_str);
						events = save_event;
						events->l_try = time(&l);
					} else {
						mPrintf("Can not network with user logged in.\n ");
					}
				}
				break;
			case CR_EXIT:
				ExitToMsdos = atoi(events->e_str) + 1;
				events->l_sucess = time(&l);
				events->l_try = time(&l);
				break;
			default:
				mPrintf(" Unknown event type %d, slot %d\n ", events->e_type, evnt);
				/* We have bug here somewhere so we make this event zapped */
				events->e_zap = 1;
				break;
		}
	}
}

/* -------------------------------------------------------------------- */
/*  list_event()    List all events                                     */
/* -------------------------------------------------------------------- */
void list_event(void)
{
    int i;
    char dtstr[20];
	uchar next_chr = 175;
	uchar endnext_chr = 174;
	uchar do_chr = 254;

	outFlag = OUTOK;
	mtPrintf(TERM_BOLD,
		"   No.     Type                Text    Redo Retry Last");
    doCR();

	events = start_event;
	i = 0;
	while(i <= numevents) {

		if (on_event == i) termCap(TERM_REVERSE);
		mPrintf(" %c%c%02d%10s, %20s, %4d, %4d ",
			on_event == i ? next_chr : ' ',
			cando_event(i) ? do_chr : ' ',
			i,
			events->e_zap ? "Zapped" : crontypes[events->e_type],
			events->e_str,
			events->e_redo,
			events->e_retry);
		if (events->l_try) {
			cit_strftime(dtstr, 19, "%X %y%b%D", events->l_try);
                mPrintf("%s", dtstr);
		} else
			mPrintf("                ");
		mPrintf("%c%c", cando_event(i) ? do_chr : ' ',
			on_event == i ? endnext_chr : ' ');
		if (on_event == i)
			termCap(TERM_NORMAL);

		doCR();
		events = events->next;
		i++;
    }
}

/* -------------------------------------------------------------------- */
/*  cron_commands() Sysop Fn: Cron commands                             */
/* -------------------------------------------------------------------- */
void cron_commands(void)
{
	 const char no_event[] = "\n No cron events\n";

	 switch (toupper(iChar())) {
		  case 'A':
            mPrintf("\bAll Done\n ");
            doCR();
            mPrintf("Seting all events to done...");
			events = start_event;
			while(events->next != NULL) {
				events->l_sucess = time(NULL);
				events->l_try = time(NULL);
				events = events->next;
			}
            doCR();
            break;
        case 'D':
            mPrintf("\bDone event\n ");
            if (numevents == ERROR) {
					 mPrintf("%s",no_event);
            } else {
				done_event();
			}
            break;
        case 'E':
            mPrintf("\bEnter Cron file\n ");
            readcron();
            break;
        case 'L':
            mPrintf("\bList events");
            doCR();
            doCR();
            list_event();
            break;
		case 'N':
            mPrintf("\bNext event");
			cronNextEvent((int)getNumber("next event number",0L,
				(long)numevents,(long)on_event));
			break;
        case 'R':
            mPrintf("\bReset event\n ");
            if (numevents == ERROR) {
					 mPrintf("%s",no_event);
            } else {
				reset_event();
			}
            break;
		case 'U':
			mPrintf("\bUnZap event\n ");
            if (numevents == ERROR) {
					 mPrintf("%s",no_event);
            } else {
				unzap_event();
			}
            break;
		case 'Z':
            mPrintf("\bZap event\n ");
            if (numevents == ERROR) {
					 mPrintf("%s",no_event);
            } else {
				zap_event();
			}
            break;
        case '?':
            doCR();
            doCR();
            mpPrintf(" <A>ll done\n");
			mpPrintf(" <D>one event\n");
			mpPrintf(" <E>nter Cron file\n");
            mpPrintf(" <L>ist event\n");
			mpPrintf(" <N>ext event\n");
			mpPrintf(" <R>eset event\n");
			mpPrintf(" <U>nzap event\n");
			mpPrintf(" <Z>ap event\n");
            mpPrintf(" <?> -- this menu\n ");
            break;
        default:
            if (!expert)
                mPrintf("\n '?' for menu.\n ");
            else
                mPrintf(" ?\n ");
            break;
    }
}

/* -------------------------------------------------------------------- */
/*  zap_event()     Zap an event out of the cron list                   */
/* -------------------------------------------------------------------- */
void zap_event(void)
{
	int i, x;

    i = (int) getNumber("event", 0L, (long) numevents, (long) on_event);
    if (i == ERROR)
        return;
	events = start_event;
	if(i) {
		for(x=0; x<i; x++) events = events->next;
	}
	events->e_zap = 1;
}

/* -------------------------------------------------------------------- */
/*	unzap_event()	  Un-Zap an event out of the cron list				*/
/* -------------------------------------------------------------------- */
void unzap_event(void)
{
	int i, x;

    i = (int) getNumber("event", 0L, (long) numevents, (long) on_event);
    if (i == ERROR)
        return;
	events = start_event;
	if(i) {
		for(x=0; x<i; x++) events = events->next;
    }
	events->e_zap = 0;
}

/* -------------------------------------------------------------------- */
/*  reset_event()   Reset an even so that it has not been done          */
/* -------------------------------------------------------------------- */
void reset_event(void)
{
	int i, x;

    i = (int) getNumber("event", 0L, (long) numevents, (long) on_event);
    if (i == ERROR)
        return;
	events = start_event;
	if(i) {
		for(x=0; x<i; x++) events = events->next;
    }
	events->l_sucess = 0L;
	events->l_try = 0L;
}

/* -------------------------------------------------------------------- */
/*  done_event()    Set event so it seems to have been done             */
/* -------------------------------------------------------------------- */
void done_event(void)
{
	int i, x;
    long l, time();

    i = (int) getNumber("event", 0L, (long) numevents, (long) on_event);
    if (i == ERROR)
        return;
	events = start_event;
	if(i) {
		for(x=0; x<i; x++) events = events->next;
    }
	events->l_sucess = time(&l);
	events->l_try = time(&l);
}

/* -------------------------------------------------------------------- */
/*  did_net()       Set all events for a node to done                   */
/* -------------------------------------------------------------------- */
void did_net(char *callnode)
{
    int i;
    long l, time();

	events = start_event;
    for (i = 0; i <= numevents; i++) {
		if (strcmpi(events->e_str, callnode) == SAMESTRING &&
			((events->e_type == CR_NET) ||
			(events->e_type == CR_LOCAL))) {
			events->l_sucess = time(&l);
			events->l_try = time(&l);
        }
		events = events->next;
	}
}

/* --------------------------------------------------------------------

        writeCrontab() -  write cron events to a file

   -------------------------------------------------------------------- */

void writeCrontab(void)
{
    FILE *fp;
    const char *name = "cron.tab";

	unlink(name);
	if ((fp = fopen(name, "wb")) == NULL) {
        crashout("Can't make Cron.tab");
    }
    /* write out Cron.tab */
	events = start_event;
	while(numevents > -1) {
		fwrite(events, sizeof(struct event), 1, fp);
		events = events->next;
		numevents--;
	}
    fclose(fp);
}

/* --------------------------------------------------------------------

        readCrontab() -  read cron events from file

   -------------------------------------------------------------------- */

void readCrontab(void)
{
    FILE *fp;
	const char *name = "cron.tab";

    fp = fopen(name, "rb");
    if (fp) {
		events = start_event;
		numevents = -1;
		while(fread(events, sizeof(struct event), 1, fp)) {
			numevents++;
			events->next = NULL;  /* Since it's no good on disk anyway!  */
			events->next = farcalloc(1UL,(ulong) sizeof(struct event));
			if(events->next == NULL) {
				crashout("Can not allocate memory for cron events");
			}
			events = events->next;
			events->next = NULL;
        }
        fclose(fp);
		if(!cfg->LAN)
			unlink(name);
    }
}

/* EOF */
