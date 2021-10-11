
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the exported definitions for handling
 *	logs in the administrative framework.  It includes the
 *	definitions for:
 *
 *		o Category logging indicators.
 *		o Miscellaneous constants.
 *		o Log information block structure.
 *		o Exported interfaces for handling logs.
 *
 ****************************************************************************
 */

#ifndef _adm_log_h
#define _adm_log_h

#pragma	ident	"@(#)adm_log.h	1.5	95/09/08 SMI"

/*
 *---------------------------------------------------------------------
 * Category logging indicators.
 *---------------------------------------------------------------------
 */

#define ADM_CATLOG_OFF		0	/* Don't log category lists */
#define ADM_CATLOG_BASIC	1	/* Log only basic categories from list */
#define ADM_CATLOG_ALL		2	/* Log all categories in list */

/*
 *---------------------------------------------------------------------
 * Miscellaneous constants.
 *---------------------------------------------------------------------
 */

#define ADM_ALL_LOGS		NULL	/* Used to specify all known logs */
#define ADM_LOG_STDHDR		NULL	/* Indicate use of standard log header */

/*
 *---------------------------------------------------------------------
 * Log information block structure.
 *---------------------------------------------------------------------
 */

typedef struct Adm_logID Adm_logID;	/* Agent log information struct */
struct Adm_logID {
	int fd;					/* File Descriptor */
	char *pathname;				/* Path to log */
	int  cat_logging;			/* Log category info? */
	boolean_t and_flag;			/* Filter on all/any categories */
	char *categories;			/* Categories to filter on */
	char *adm_class;			/* Class name being exec'd */
	char *class_vers;			/* Class version being exec'd */
	char *method;				/* Method being exec'd */
	char *host;				/* Host where method exec'd */
	char *domain;				/* Domain where method exec'd */
	char *client_host;			/* Host of invoking client */
	Adm_requestID reqID;			/* Invocation request ID */
	char *reqID_str;			/* Request ID in string form */
	Adm_logID *prev;			/* Previous log info block */
	Adm_logID *next;			/* Next log info block */
};

/*
 *---------------------------------------------------------------------
 * Exported log handling interfaces.
 *---------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int adm_log_end(Adm_logID *);
extern int adm_log_entry(Adm_logID *, time_t, char *, char *, char *, int *);
extern int adm_log_info(Adm_logID *, char *, char *, char *, char *, char *,
			char *, Adm_requestID);
extern int adm_log_start(char *, int, int, boolean_t, char *, Adm_logID **);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_log_h */

