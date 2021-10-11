/*
 *
 * exMess.c
 *
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sendMsg.c	1.1	94/02/04 SMI"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <desktop/tt_c.h>


void
main(int argc,char **argv) {
	char			*procid;
	int			ttfd;
	int		mark;
	Tt_message	msg;
	char *file;


	procid = tt_open();
	
	if (tt_pointer_error(procid) != TT_OK) {
		fprintf(stderr,"%s: Can't initialize ToolTalk\n", argv[0]);
		exit(1);
	   }

	ttfd = tt_fd();

	/* register a dynamic pattern to catch whenever the remote */
	/* editor creates a sub-file object so that we can update our */
	/* object query panel. */

	file = argv[2];


	mark = tt_mark();
	tt_default_file_set(file);
	msg = tt_pnotice_create(TT_FILE, argv[1]);
	tt_message_file_set(msg,file); /* behavior indicates that 
												 TT_FILE with tt_default_file_set
												 already have set this, but documentation
												 is vague; -- so just in case */
	tt_file_join(file);  /* behavior indicates that this is redundant for
									same reason as previous, but just in case ... */
	tt_message_arg_add(msg, TT_IN, "string", NULL);
	tt_message_arg_val_set(msg, 0, argv[3]);
	tt_message_arg_add(msg, TT_IN, "string", NULL);
	tt_message_arg_val_set(msg, 1, argv[4]);
	tt_message_arg_add(msg, TT_IN, "string", NULL);
	tt_message_arg_val_set(msg, 2, argv[5]);
	tt_message_send(msg);
	tt_release(mark);

	tt_close();
	exit(0);
}


