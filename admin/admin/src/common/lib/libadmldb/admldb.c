/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)admldb.c	1.1	94/09/07 SMI"

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <cl_database_parms.h>
#include <admldb.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>

#define	DB_FUNCTION_CALL	0
#define	DB_METHOD_CALL		1

#define	LOCALHOST_ADDR	"127.0.0.1"

int	
lcl_list_table(
	ulong_t ns_mask,	/* Nameservice(s) to use */
	char *host,		/* Host to invoke operation on */
	char *domain,		/* Nameservice domain */
	ulong_t flags,		/* Flag values */
	Db_error **db_err,	/* Error return structure */
	Table *tbl,		/* Table to list from */
	...)
{
	va_list ap;
	char **iargs, ***oargs;
	ulong_t argno;
	struct tbl_trans_data *ttp;
	ulong_t action;
	int null_match = 1;
	int status;

	ttp = adm_tbl_trans[tbl->type];
	if (ttp->type != tbl->type) {
		db_err_set(db_err, DB_ERR_STRUCT_MISMATCH, ADM_FAILCLEAN,
			"list_table");
		return (-1);
	}

	iargs = (char **) calloc(ttp->match_args.cnt, sizeof(char *));
	if (iargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
			"list_table");
		return (-1);
	}

	oargs = (char ***) calloc(ttp->io_args.cnt, sizeof(char **));
	if (oargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
		    "list_table");
		free(iargs);
		return (-1);
	}
	if ((flags & DB_LIST_SINGLE)) {
		action = DB_GET;
	} else {
		action = DB_LIST;
	}

	if (action == DB_GET) {
		va_start(ap, tbl);
		for (argno = 0; argno < ttp->match_args.cnt; ++argno) {
			iargs[argno] = va_arg(ap, char *);
			if ((iargs[argno] != NULL) && strlen(iargs[argno]))
				null_match = 0;
		}
		if (null_match) {
			db_err_set(db_err, DB_ERR_MATCH_CRITERIA, ADM_FAILCLEAN,
			    "list_table");
			free(iargs);
			free(oargs);
			return (-1);
		}
		for (argno = 0; argno < ttp->io_args.cnt; 
		    ++argno) {
			oargs[argno] = va_arg(ap, char **);
			if (oargs[argno] != NULL)
				*oargs[argno] = NULL;
		}
		va_end(ap);
	}

	status = (*ttp->actions[action].func)(ns_mask, host, domain, 
	    flags, db_err, tbl, iargs, oargs, ttp, action, NULL);
	free(iargs);
	free(oargs);
	return (status);
}

/*
 * Function to return the next entry from a previous list operation.
 */
int
get_next_entry(
	Db_error **db_err,
	Table *tbl,
	...)
{
	va_list ap;
	int i, argno;
	Column *cp;
	struct tbl_trans_data *ttp;
	char ***oargs;
	
	ttp = adm_tbl_trans[tbl->type];
	if (ttp->type != tbl->type) {
		db_err_set(db_err, DB_ERR_STRUCT_MISMATCH, ADM_FAILCLEAN,
			"list_table");
		return (-1);
	}
	va_start(ap, tbl);
	oargs = (char ***) calloc(ttp->io_args.cnt, sizeof(char **));
	if (oargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
			"list_table");
		return (-1);
	}
	for (argno = 0; argno < ttp->io_args.cnt; ++argno) {
		oargs[argno] = va_arg(ap, char **);
		if (oargs[argno] != NULL)
			*oargs[argno] = NULL;
	}
	va_end(ap);
	if ((tbl->tdh == NULL) || (tbl->tdh->current == NULL)) {
		free(oargs);
	        return (0);
	}
	for (cp = tbl->tdh->current->start; cp != NULL; cp = cp->next)
		for (i = 0; i < argno; ++i)
			if (!strcmp(cp->name, ttp->io_args.at[i].name))
				if (oargs[i] != NULL)
					*oargs[i] = cp->val;
	tbl->tdh->current = tbl->tdh->current->next;
	free(oargs);
	return (1);
}

int
lcl_remove_table_entry(
	ulong_t ns_mask,	/* Nameservice(s) to use */
	char *host,		/* Host to invoke operation on */
	char *domain,		/* Nameservice domain */
	ulong_t flags,		/* Flag values */
	Db_error **db_err,	/* Error return structure */
	Table *tbl,		/* Table to list from */
	...)
{

	va_list ap;
	char **iargs, ***oargs = NULL;
	ulong_t argno;
	struct tbl_trans_data *ttp;
	ulong_t action;
	int status;
	
	ttp = adm_tbl_trans[tbl->type];
	if (ttp->type != tbl->type) {
		db_err_set(db_err, DB_ERR_STRUCT_MISMATCH, ADM_FAILCLEAN, 
		    "remove_table_entry");
		return (-1);
	}

	iargs = (char **) calloc(ttp->match_args.cnt, sizeof(char *));
	if (iargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, 
		    "remove_table_entry");
		return (-1);
	}

	va_start(ap, tbl);
	for (argno = 0; argno < ttp->match_args.cnt; ++argno)
		iargs[argno] = va_arg(ap, char *);
	for (argno = 0; argno < ttp->match_args.cnt; ++argno)
		if ((iargs[argno] != NULL) && strlen(iargs[argno]))
			break;
	if (argno == ttp->match_args.cnt) {
		db_err_set(db_err, DB_ERR_MATCH_CRITERIA, ADM_FAILCLEAN, 
		    "remove_table_entry");
		free(iargs);
		return (-1);
	}
	va_end(ap);

	status = (*ttp->actions[DB_REMOVE].func)(ns_mask, host, domain, 
	    flags, db_err, tbl, iargs, oargs, ttp, DB_REMOVE, NULL);
	free(iargs);
	return (status);
}

int
lcl_set_table_entry(
	ulong_t ns_mask,	/* Nameservice(s) to use */
	char *host,		/* Host to invoke operation on */
	char *domain,		/* Nameservice domain */
	ulong_t flags,		/* Flag values */
	Db_error **db_err,	/* Error return structure */
	Table *tbl,		/* Table to list from */
	...)
{
	va_list ap;
	char **iargs, ***oargs;
	ulong_t argno;
	struct tbl_trans_data *ttp;
	ulong_t action;
	int (*NULL_FUN)() = NULL;
	int status;
	
	ttp = adm_tbl_trans[tbl->type];
	if (ttp->type != tbl->type) {
		db_err_set(db_err, DB_ERR_STRUCT_MISMATCH, ADM_FAILCLEAN, 
		    "set_table_entry");
		return (-1);
	}

	iargs = (char **) calloc(ttp->match_args.cnt, sizeof(char *));
	if (iargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, 
		    "set_table_entry");
		return (-1);
	}

	oargs = (char ***) calloc(ttp->io_args.cnt, sizeof(char **));
	if (oargs == NULL) {
		db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, 
		    "set_table_entry");
		free(iargs);
		return (-1);
	}

	va_start(ap, tbl);
	for (argno = 0; argno < ttp->match_args.cnt; ++argno)
		iargs[argno] = va_arg(ap, char *);
	for (argno = 0; argno < ttp->match_args.cnt; ++argno)
		if ((iargs[argno] != NULL) && strlen(iargs[argno]))
			break;
	if (argno == ttp->match_args.cnt) {
		db_err_set(db_err, DB_ERR_MATCH_CRITERIA, ADM_FAILCLEAN, 
		    "set_table_entry");
		free(iargs);
		free(oargs);
		return (-1);
	}
	for (argno = 0; argno < ttp->io_args.cnt; ++argno) {
		oargs[argno] = va_arg(ap, char **);
		if ((oargs[argno] != NULL) && (*oargs[argno] != NULL) &&
		    (ttp->io_args.at[argno].valid != NULL_FUN) &&
		    !((*ttp->io_args.at[argno].valid)(*oargs[argno]))) {
			db_err_set(db_err,
			    ttp->io_args.at[argno].valid_err_index,
			    ADM_FAILCLEAN, "set_table_entry", *oargs[argno]);
			free(iargs);
			free(oargs);
			return (-1);
		}
	}
	va_end(ap);

	status = (*ttp->actions[DB_SET].func)(ns_mask, host, domain, 
	    flags, db_err, tbl, iargs, oargs, ttp, DB_SET, NULL);
	free(iargs);
	free(oargs);
	return (status);
}

void
db_err_free(Db_error **db_err)
{
	if (*db_err != NULL)
		free((*db_err)->msg);
	free(*db_err);
	*db_err = NULL;
}
