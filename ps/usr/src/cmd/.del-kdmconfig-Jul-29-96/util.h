/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 *
 * util.h: Common utility routines
 *
 * Description:
 *      External interface for common utility routines, used during config
 *      and unconfig operation.
 *
 *
 * External Routines:
 */

#pragma ident "@(#)util.h 1.2 94/03/09 SMI"

#define PROCESS_READ 1
#define FILE_READ 2

extern int read_generic(char *, char **, int);
extern void write_generic(char *, char *, int);
extern NODE parse_node( char *, cat_t);
extern void check_nsswitch(void);
extern void set_server_mode(char *);
extern int get_server_mode();
extern char * get_client();
