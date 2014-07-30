/************************************************************************/
/*                              account2.c                              */
/*        time accounting code for Citadel bulletin board system        */
/************************************************************************/

#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************
 *                              Contents                                *
 *                                                                      *
 *      clearthisAccount()      initializes all current user group data *
 *      logincrement()          increments balance according to data    *
 *      logincheck()            logs-out a user if can't log in now     *
 *      negotiate()             determines lowest cost, highest time    *
 *      newaccount()            sets up accounting balance for new user *
 *      unlogthisAccount()      NULL group accounting to thisAccount    *
 *      calc_trans()            adjust time after a transfer            *
 ************************************************************************
 *
 *      03/19/90    FJM     Linted & partial cleanup
 *                      Changed float (unsresolved data type) to double
 *      05/19/90    FJM     Cleaned negotiate(), and nade it negotiate upload
 *                                              and download multipliers.
 *      11/24/90    FJM     Changes for shell/door mode
 *      04/04/91    FJM     Made accountBuf Dynamically allocated
 *		09/14/91	BLJ 	Changes for Dynamically Linked Lists
 *
 ************************************************************************/


/************************************************************************
 *      clearthisAccount()  initializes all current user group data     *
 ************************************************************************/
void clearthisAccount(void)
{
    int i;

    /* init days */
    for (i = 0; i < 7; i++)
        thisAccount.days[i] = 0;

    /* init hours & special hours */
    for (i = 0; i < 24; i++) {
        thisAccount.hours[i] = 0;
        thisAccount.special[i] = 0;
    }

    thisAccount.have_acc = FALSE;
    thisAccount.dayinc = 0.;
    thisAccount.maxbal = 0.;
    thisAccount.dlmult = -1.;   /* charge full time */
    thisAccount.ulmult = 1.;    /* credit full time (xtra)  */
}

/************************************************************************/
/*      logincrement()          increments balance according to data    */
/*                              give em' more minutes if new day!       */
/************************************************************************/
void logincrement(void)
{
    long diff, timestamp;
    long day = 86400L;
    double numdays, numcredits;

    time(&timestamp);

    diff = timestamp - logBuf.calltime;
    /* how many days since last call(1st of day) */
    numdays = (double) ((double) diff / (double) day);

    if (numdays < 0.0) {    /* date was wrong..             */
        mPrintf("[Date correction made.]");
        numdays = 1;        /* give em something for sysops mistake..   */
        time(&lasttime);
        logBuf.calltime = lasttime;
    }
    numcredits = numdays * thisAccount.dayinc;

    /* If they have negative minutes this will bring them up MAYBE  */
	logBuf.credits = logBuf.credits + (float) numcredits;

    if (logBuf.credits > thisAccount.maxbal)
        logBuf.credits = thisAccount.maxbal;

    /* Credits/Minutes to start with.   */
    startbalance = logBuf.credits;

    time(&lasttime);        /* Now, this is the last time we were on.   */
}


/************************************************************************/
/*      logincheck()            logs-out a user if can't log in now     */
/************************************************************************/
int logincheck(void)
{
    static int ncount = 0;

    if (thisAccount.special[hour()]) {  /* Is is free time?     */
        specialTime = TRUE;
    } else {
        specialTime = FALSE;
    }

    /* Local and bad calls get no accounting.   */
    if (!gotCarrier() || onConsole)
        return (TRUE);

    if (!thisAccount.days[dayofweek()]
        || !thisAccount.hours[hour()]
    || logBuf.credits <= 0.0) {
        nextblurb("nologin", &ncount, 1);

        loggedIn = FALSE;

        terminate(TRUE, FALSE);

        return (FALSE);
    }
	if (thisAccount.special[hour()]) {	/* Is it free time? 	*/
        specialTime = TRUE;
    }
    return (TRUE);
}

/************************************************************************/
/*      negotiate()  determines lowest cost, highest time for user      */
/************************************************************************/
void negotiate(void)
{
    int groupslot, i;
    int firstime = TRUE;
    double priority = 0.0;
	struct accounting *topBuf;

    clearthisAccount();

	accountBuf = start_acct;
	topBuf = start_acct;

	while(accountBuf) {
		groupslot = accountBuf->grpslot;
		if (ingroup(groupslot) && accountBuf->have_acc) {
			/* is in a group with accounting */
            thisAccount.have_acc = TRUE;

			if (accountBuf->priority >= priority) {
				topBuf = accountBuf;
				if (accountBuf->priority > priority) {
					priority = accountBuf->priority;
                    firstime = TRUE;
				}
			}
			if (topBuf->dlmult > thisAccount.dlmult || firstime)
                thisAccount.dlmult =    /* these are */
				topBuf->dlmult; 		/* special	 */

			if (topBuf->ulmult > thisAccount.dlmult || firstime)
				thisAccount.ulmult = topBuf->ulmult;

			if (accountBuf->dayinc > thisAccount.dayinc || firstime)
				thisAccount.dayinc = accountBuf->dayinc;

			if (accountBuf->maxbal > thisAccount.maxbal || firstime) {
                firstime = FALSE;
				thisAccount.maxbal = accountBuf->maxbal;
            }
		}							/* if ingroup() &&have_acc	 */
		accountBuf = accountBuf->next;
	}								/* while (accountBuf)		 */

    for (i = 0; i < 7; ++i)
		if (topBuf->days[i])
            thisAccount.days[i] = 1;

    for (i = 0; i < 24; ++i) {
		if (topBuf->hours[i])
            thisAccount.hours[i] = 1;

		if (topBuf->special[i])
            thisAccount.special[i] = 1;
    }

}

/************************************************************************/
/*      newaccount()  sets up accounting balance for new user           */
/*      extra set-up stuff for new user                                 */
/************************************************************************/
void newaccount(void)
{

	logBuf.credits = cfg->newbal;
    time(&lasttime);
    logBuf.calltime = lasttime;

}

/************************************************************************/
/*      unlogthisAccount()  sets up NULL group accounting to thisAccount*/
/************************************************************************/
void unlogthisAccount(void)
{
    int i;

    /* set up unlogged balance */
	logBuf.credits = cfg->unlogbal;

    /* reset transmitted & received */
    transmitted = 0L;
    received = 0L;

    time(&lasttime);
    logBuf.calltime = lasttime;

	accountBuf = start_acct;
	/* init days */
    for (i = 0; i < 7; i++)
		thisAccount.days[i] = accountBuf->days[i];

    /* init hours & special hours */
    for (i = 0; i < 24; i++) {
		thisAccount.hours[i] = accountBuf->hours[i];
		thisAccount.special[i] = accountBuf->special[i];
    }

	thisAccount.dayinc = accountBuf->dayinc;
	thisAccount.maxbal = accountBuf->maxbal;
	thisAccount.dlmult = accountBuf->dlmult;
	thisAccount.ulmult = accountBuf->ulmult;

}

/************************************************************************/
/*      calc_trans()        Calculate and adjust time after a transfer  */
/*                          (trans == 1) ? Upload : Download            */
/************************************************************************/
void calc_trans(long time1, long time2, char trans)
{
    long diff;          /* # secs trans took            */
    double credcost;    /* their final credit/cost      */
    double c;           /* # minutes transfer took      */
    double change;      /* how much we change their bal */
    double mult;        /* the multiplyer for trans     */
    char neg;           /* is credcost negative?        */

    if (trans)
        mult = thisAccount.ulmult;
    else
        mult = thisAccount.dlmult;

    diff = time2 - time1;   /* how many secs did ul take                 */

    c = (double) diff / 60.0;   /* how many minutes did it take  */

    change = c * mult;      /* take care of multiplyer...    */

    logBuf.credits = logBuf.credits + change;

    credcost = change;

    if (credcost < 0.0) {
        neg = TRUE;
    } else {
        credcost = credcost + c;/* Make less confusion, add time used */
    }               /* when telling them about credit     */

    mPrintf(" Your transfer took %.2f %s. You have been %s %.2f %s%s.",
    c,
    (c == 1) ? "minute" : "minutes",
    (neg == 1) ? "charged" : "credited",
    (neg == 1) ? (-credcost) : credcost,
	cfg->credit_name,
    (credcost == 1) ? "" : "s");

    time(&lasttime);        /* don't charge twice...    */
}
