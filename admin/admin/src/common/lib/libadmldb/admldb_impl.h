/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#ifndef _ADM_LDB_IMPL_H
#define _ADM_LDB_IMPL_H

#pragma	ident	"@(#)admldb_impl.h	1.3	95/06/22 SMI"

#include <admldb.h>
#include <cl_database_parms.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define DB_TABLE_ERROR  -1
#define DB_TABLE_NOTFND -2

#define	DB_CODE_AND_MSG(i)	i,ADMLDB_MSGS(i)

#define	DB_UFS_INDEX		0
#define	DB_NIS_INDEX		1
#define	DB_NISPLUS_INDEX	2
#define	DB_NUM_NAMESERVICES	3

#define	DEFAULT_COLUMN_SEP	"\t "
#define	DEFAULT_COMMENT_SEP	"#"

#define	DB_LIST		0
#define	DB_GET		1
#define	DB_SET		2
#define	DB_REMOVE	3
#define	DB_NUM_ACTIONS	4

/* Definitions for case_flag element of column structure */
#define	DB_CASE_INSENSITIVE	1
#define	DB_CASE_SENSITIVE	0

#define	MAX_COLS	16
#define	MAX_ARGS	20

#define	COL_NOT_MATCHED	2
#define	COL_MATCHED	1
#define	EXACT_MATCH	COL_MATCHED
#define	MIX_MATCH	(COL_NOT_MATCHED | COL_MATCHED)
#define	NO_MATCH	0

struct match_arg_list {
	ulong_t cnt;
	struct {
		char *name;		/* Framework argument name */
		char *compat_name;	/* Framework argument name for V1 */
		int fixed;		/* Modifiable under V1? */
		int colnum[DB_NUM_NAMESERVICES];	/* Column in each ns */
	} at[MAX_ARGS];
};

struct io_arg_list {
	ulong_t cnt;
	struct {
		char *name;		/* Framework argument name */
		int (*valid)(const char *);	/* Validation function */
		ulong_t valid_err_index; /* Error code for validation error */
	} at[MAX_ARGS];
};

struct action_impl {
	int (*func)();			/* Function to invoke for an action */
	char *method;			/* Method to invoke for an action */
};

struct column_trans {
	uint_t key;			/* Is this a key? */
	ushort_t case_flag;		/* Case-sensitive? */
	char *param;			/* Parameter name */
	uint_t unique;			/* Must value be unique? */
	int first_match;		/* First matching column */
	int last_match;			/* Last matching column */
};

struct tbl_fmt {
	/*
	 * special has different interpretations for each ns.
	 * UFS - no meaning
	 * NIS - concatenate key & value columns before parsing entry
	 * NIS+ - entries consume multiple NIS+ objects (e.g. hosts)
	 */
	ushort_t special;		
        int alias_col;	/* Which column has aliases */
        int comment_col;	/* Which column is the comment column */
	int (*sort_function)();	/* Function for qsort to use */
	uint_t cnt;
	struct column_trans data_cols[MAX_COLS];
};

struct tbl_trans_data {
	ulong_t type;
	Table_names tn;
	struct tbl_fmt fmts[DB_NUM_NAMESERVICES];
	struct match_arg_list match_args;
	struct io_arg_list io_args;
	struct action_impl actions[DB_NUM_ACTIONS];
};

extern struct tbl_trans_data *adm_tbl_trans[];

extern void free_column(Row *, Column *);
extern Row *new_row(void);
extern void free_row(Row *);
extern Table_data *new_tdh(void);
extern void free_tdh(Table *);
extern void free_tri(Table_row_info *);
extern Table_row_info *new_tri(void);
extern Table_row_info *copy_tri(Table_row_info *);
extern Table *new_table(void);
extern void free_table(Table *);
extern int append_row(Table *, Row *);
extern Column *column_num_in_row(Row *, int);
/*
extern char *ldb_get_db_line(char *, int, FILE *, struct tbl_trans_data *);
extern void ldb_build_nis_name(char *, char *, char *, char *);
extern int ldb_do_nis_lookup(char *, nis_result **, Db_error **);
*/

#ifdef PERF_DEBUG
#include <sys/time.h>
static struct timeval ts_start, ts_now, ts_elapsed, ts_previous = { 0 };

enum { TS_NOELAPSED, TS_ELAPSED };

#define timestamp(message,variable,totalflag) {				\
		gettimeofday(&ts_now, "");				\
		if (ts_previous.tv_sec == 0) {				\
			/* first time */				\
			ts_start = ts_now;				\
			ts_elapsed = ts_now;				\
		} else {						\
			ts_elapsed.tv_sec =				\
			    (ts_now.tv_sec - ts_previous.tv_sec);	\
			ts_elapsed.tv_usec =				\
			    (ts_now.tv_usec - ts_previous.tv_usec);	\
		}							\
		if (ts_elapsed.tv_usec < 0) {				\
			ts_elapsed.tv_sec -= 1;				\
			ts_elapsed.tv_usec += 1000000L;			\
		} else if (ts_elapsed.tv_usec > 999999L) {		\
			ts_elapsed.tv_sec += 1;				\
			ts_elapsed.tv_usec -= 1000000L;			\
		}							\
		printf("%9.1d.%03d %.80s %.80s\n", ts_elapsed.tv_sec,	\
		    (ts_elapsed.tv_usec/1000L),				\
		    message ? message : "(null)", variable);		\
		if ((totalflag) == TS_ELAPSED) {			\
			ts_previous = ts_start;				\
			ts_elapsed.tv_sec =				\
			    (ts_now.tv_sec - ts_previous.tv_sec);	\
			ts_elapsed.tv_usec =				\
			    (ts_now.tv_usec - ts_previous.tv_usec);	\
			if (ts_elapsed.tv_usec < 0) {			\
				ts_elapsed.tv_sec -= 1;			\
				ts_elapsed.tv_usec += 1000000L;		\
			} else if (ts_elapsed.tv_usec > 999999L) {	\
				ts_elapsed.tv_sec += 1;			\
				ts_elapsed.tv_usec -= 1000000L;		\
			}						\
			printf("%9.1d.%03d Total Elapsed Time\n",	\
			    ts_elapsed.tv_sec,				\
			    (ts_elapsed.tv_usec/1000L));		\
		}							\
		ts_previous = ts_now;					\
		fflush(stdout);						\
	}
#endif

#ifdef  __cplusplus
}
#endif

#endif	/* !_ADM_LDB_IMPL_H */
