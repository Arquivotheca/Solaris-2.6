/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */
#ifndef lint
#ident   "@(#)ncmsg.c 1.1 93/07/06 SMI"
#endif				/* lint */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include "nhash.h"
#include "xgetsh.h"

#define	HASHSIZE	151
#define	BSZ		4

static Cache *msgs_cache = (Cache *) NULL;

int
cmsg(const char *msgid)
{
	Item *itemp;
	int len;

	if (msgs_cache == (Cache *) NULL)
		if (init_cache(&msgs_cache, HASHSIZE, BSZ,
		    (int (*)())NULL, (int (*)())NULL) == -1) {
			(void) fprintf(stderr,
			    gettext("cmsg(): init_cache() failed.\n"));
			exit(1);
		}

	len = strlen(msgid) + 1;

	if ((itemp = lookup_cache(msgs_cache, (void *) msgid, len)) ==
	    Null_Item) {
		if ((itemp = (Item *) malloc(sizeof (*itemp))) == Null_Item) {
			(void) fprintf(stderr,
			    gettext("cmsg(): itemp=malloc(%d)\n"),
			    sizeof (*itemp));
			exit(1);
		}

		if ((itemp->key = (char *) malloc(len)) == NULL) {
			(void) fprintf(stderr,
			    gettext("cmsg(): itemp->key=malloc(%d)\n"),
			    len);
			exit(1);
		}
		(void) memmove(itemp->key, msgid, len);
		itemp->keyl = len;

		itemp->data = NULL;
		itemp->datal = 0;

		if (add_cache(msgs_cache, itemp) == -1)
			(void) fprintf(stderr,
			    gettext("cmsg(): add_cache() failed.\n"));

		return (0);
	} else {
		return (1);
	}
}
