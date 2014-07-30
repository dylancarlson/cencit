/*------------------------------------------------------------
*
*
*  Å
*     Å   Centauri Enterprises           Copyright (c) 1991
*   Å
*
*------------------------------------------------------------
* History:
*
*------------------------------------------------------------
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloc.h>

typedef struct new {
	char name[20];
	char addr[20];
};

void chg(char *sp);

void main()
{
	struct new *nw;

	if((nw = calloc(1UL,sizeof(struct new))) == NULL) {
		printf("cannot allocate memory for struct nw\n");
		exit(1);
	}

	strcpy(nw->name,"Brad L. Johnson");
	strcpy(nw->addr,"7942 Fall Creek Rd");

	chg(nw->name);

	printf("%s",nw->name);
}

void chg(char *sp)
{
	strcpy(sp,"Rhonda Barton");
}

