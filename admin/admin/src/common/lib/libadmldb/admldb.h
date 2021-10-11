/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#ifndef _ADM_LDB_H
#define _ADM_LDB_H

#pragma	ident	"@(#)admldb.h	1.2	95/02/19 SMI"

#include <sys/types.h>
#include <string.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef ADM_FAILCLEAN
#define	ADM_FAILCLEAN	(u_int) 1
#define	ADM_FAILDIRTY	(u_int) 2
#endif

#define	DB_NS_UFS		1
#define	DB_NS_NIS		2
#define	DB_NS_NISPLUS		4
#define	DB_NS_ALL		(DB_NS_UFS|DB_NS_NIS|DB_NS_NISPLUS)

#define	DB_DISABLE_LOCKING	1	/* Don't use fcntl advisory locks */
#define	DB_ADD			2	/* Set will add new */
#define	DB_MODIFY		4	/* Set will modify existing */
#define	DB_ADD_MODIFY	(DB_ADD|DB_MODIFY)
#define	DB_SORT_LIST		8	/* Ensure list sorted on primary key */
#define	DB_LIST_SINGLE		16	/* List will return first match */
#define	DB_LIST_SHADOW		32	/* List of passwd will include shadow */

#define	DB_AUTO_HOME_TBL	0
#define	DB_BOOTPARAMS_TBL	1
#define	DB_CRED_TBL		2
#define	DB_ETHERS_TBL		3
#define	DB_GROUP_TBL		4
#define	DB_HOSTS_TBL		5
#define	DB_LOCALE_TBL		6
#define	DB_MAIL_ALIASES_TBL	7
#define	DB_NETGROUP_TBL		8
#define	DB_NETMASKS_TBL		9
#define	DB_NETWORKS_TBL		10
#define	DB_PASSWD_TBL		11
#define	DB_POLICY_TBL		12
#define	DB_PROTOCOLS_TBL	13
#define	DB_RPC_TBL		14
#define	DB_SERVICES_TBL		15
#define	DB_SHADOW_TBL		16
#define	DB_TIMEZONE_TBL		17
#define	DB_NUM_TBLS		18

typedef struct db_error {
	ulong_t errno;		/* Error code */
	ulong_t dirty;		/* Clean/dirty failure flag */
	char *msg;		/* Error message */
} Db_error;

struct ufs_column {
	ushort_t num;		/* Number of this column */
	ushort_t case_flag;	/* Handle column as case-insensitive? */
	ushort_t match_flag;	/* Was match successful on this column? */
	char *match_val;	/* Value to match against. */
};

typedef struct column {
	char *name;		/* Name of this column */
	char *val;		/* Column value */
	struct ufs_column *up;	/* Utility pointer; internal use only */
	struct column *next;	/* Next list member */
	struct column *prev;	/* Previous list member */
} Column;

typedef struct table_row_info {
        char*   domain;
        char*   owner;
	char*   group_owner;
	ulong_t permissions;
	ulong_t ttl;
} Table_row_info;

typedef struct row {
	Table_row_info *tri;
	Column *start;
	Column *end;
	struct row *next;
} Row;

typedef struct table_data {
	Row *start;
	Row *end;
	Row *current;
	ulong_t rows;
} Table_data;

typedef struct table_names {
	char *ufs;
	char *nis;
	char *nisplus;
	char *column_sep;
	char *comment_sep;
	ushort_t yp_compat;	/* Does UFS table have '+' syntax for NIS? */
} Table_names;

typedef struct table {
	ulong_t type;
	Table_names tn;
	Table_data *tdh;
	Table_row_info *tri;
} Table;

extern int lcl_list_table(ulong_t, char *, char *, ulong_t, Db_error **, 
    Table *, ...);
extern int get_next_entry(Db_error **, Table *, ...);
extern int lcl_set_table_entry(ulong_t, char *, char *, ulong_t, Db_error **, 
    Table *, ...);
extern int lcl_remove_table_entry(ulong_t, char *, char *, ulong_t, Db_error **,
    Table *, ...);
extern Table *table_of_type(int);
extern void free_table(Table *);
extern void db_err_free(Db_error **);

#ifdef  __cplusplus
}
#endif

#endif	/* !_ADM_LDB_H */
