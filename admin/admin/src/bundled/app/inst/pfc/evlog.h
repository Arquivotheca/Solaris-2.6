#ifndef lint
#pragma ident "@(#)evlog.h 1.9 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1991-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	evlog.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _EVLOG_H
#define _EVLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define EVLOG_COLUMN_HEADING \
	"\tTime\t\tKeystroke\tAction/Selection\tScreen\n \
\t====\t\t=========\t================\t===========================\n"

#define EVLOG_METAKEY_FMT  "\t%s"
#define EVLOG_KEYSTROKE_FMT  "\t%c\t\t%s"
#define EVLOG_ACTION_FMT     "\t\t\t%s"
#define EVLOG_SCREEN_FMT     "\t\t\t\t\t\t%s"

	extern FILE *get_logfile_stream();
	
	extern int evlog_init(char *);
	extern void evlog_done(void);
	extern void evlog_event(char *,...);
	
	extern char logbuf[];

        extern int      _ev_log;

#ifdef __cplusplus
}
#endif

#endif _EVLOG_H

