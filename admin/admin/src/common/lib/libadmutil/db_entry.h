/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#ifndef _DB_ENTRY_H
#define _DB_ENTRY_H

#pragma	ident	"@(#)db_entry.h	1.14	94/09/01 SMI"

#include <sys/types.h>
#include <limits.h>

/* Definitions for column number fields of column structure */
#define FIRST_COL	0
#define	LAST_COL	USHRT_MAX

/* Definitions for 'flag' parameter to replace_db_entry */
#define	DBE_ADD			1
#define	DBE_OVERWRITE		2
#define	DBE_ADD_OVERWRITE	(DBE_ADD | DBE_OVERWRITE)
#define	DBE_YP_COMPAT		4
#define	DBE_IGNORE_MIX_MATCH	8

/* Definitions for case_flag element of column structure */
#define	DBE_CASE_INSENSITIVE	1
#define	DBE_CASE_SENSITIVE	0

/* Control variable for locking */
extern int locking_disabled;

typedef struct column {
	ushort_t num;		/* Number of this column */
	ushort_t first_match;	/* First column to match against */
	ushort_t last_match;	/* Last column to match against */
	char *match_val;	/* Value to match */
	char *replace_val;	/* New value when replacing */
	ushort_t case_flag;	/* Handle column as case-insensitive? */
	ushort_t match_flag;	/* Was match successful on this column? */
	struct column *next;	/* Next list member */
	struct column *prev;	/* Previous list member */
} Column;

typedef struct col_list {
	Column *start;		/* First column in list */
	Column *end;		/* Last column in list */
	char *column_sep;	/* Separator string for columns */
	char *comment_sep;	/* Separator string for comment */
	char *comment;		/* Comment for this entry */
} Col_list;

struct list_db_callback {
	int (*foreach)();
	void *data;
};

#ifdef __STDC__
extern int replace_db(char *, char *);
extern int read_db(char *, char *, int);
extern int set_env_var(char *, char *, char *);
extern char *gettok(char *, char *);
extern int parse_db_buffer(char *, char *, char *, Col_list **);
extern int new_col_list(Col_list **, char *, char *, char *);
extern void free_col_list(Col_list **);
extern int new_column(Col_list *, ushort_t, ushort_t, ushort_t, char *, char *, ushort_t);
extern Column *find_column(Col_list *, ushort_t);
extern int get_db_entry(char *, Col_list *, char *, int);
extern int replace_db_entry(char *, Col_list *, int);
extern int remove_db_entry(char *, Col_list *);
extern int list_db(char *, char *, char *, struct list_db_callback *);
extern void remove_component(char *);
extern int trav_link(char **);
extern char *tempfile(const char *);
#else
extern int replace_db();
extern int read_db();
extern int set_env_var();
extern char *gettok();
extern int parse_db_buffer();
extern int new_col_list();
extern void free_col_list();
extern int new_column();
extern Column *find_column();
extern int get_db_entry();
extern int replace_db_entry();
extern int remove_db_entry();
extern int list_db();
#endif /* __STDC__ */

#endif	/* !_DB_ENTRY_H */
