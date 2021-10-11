/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)alias_impl.c	1.1	95/07/19 SMI"


#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <sys/types.h>
#include "sysman_impl.h"
#include "admldb.h"


int
_root_add_alias(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanAliasArg	*aa_p = (SysmanAliasArg *)arg_p;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_MAIL_ALIASES_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_ADD, &err, tbl,
	    aa_p->alias_key,
	    &aa_p->alias, &aa_p->expansion, &aa_p->options, &aa_p->comment);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_ALIAS_ADD_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_root_delete_alias(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanAliasArg	*aa_p = (SysmanAliasArg *)arg_p;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_MAIL_ALIASES_TBL);

	status = lcl_remove_table_entry(DB_NS_UFS, NULL, NULL, 0L, &err, tbl,
	    aa_p->alias_key);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_ALIAS_DEL_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_root_modify_alias(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanAliasArg	*aa_p = (SysmanAliasArg *)arg_p;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_MAIL_ALIASES_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_MODIFY,
	    &err, tbl,
	    aa_p->alias_key,
	    &aa_p->alias, &aa_p->expansion, &aa_p->options, &aa_p->comment);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_ALIAS_MOD_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_get_alias(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanAliasArg	*aa_p = (SysmanAliasArg *)arg_p;
	char		*alias;
	char		*expansion;
	char		*options;
	char		*comment;


	if (aa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_MAIL_ALIASES_TBL);

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_LIST_SINGLE,
	    &err, tbl,
	    aa_p->alias_key,
	    &alias, &expansion, &options, &comment);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_ALIAS_GET_FAILED);
	}

	aa_p->alias =
	    alias ? strdup(alias) : strdup(aa_p->alias_key);
	aa_p->expansion = expansion ? strdup(expansion) : NULL;
	aa_p->options = options ? strdup(options) : NULL;
	aa_p->comment = comment ? strdup(comment) : NULL;

	free_table(tbl);

	return (status);
}


int
_list_alias(SysmanAliasArg **aa_pp, char *buf, int len)
{

	int		i;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*alias;
	char		*expansion;
	char		*options;
	char		*comment;


	if (aa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_MAIL_ALIASES_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_SORT_LIST,
	    &err, tbl, NULL, NULL);

	if (cnt < 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_ALIAS_GET_FAILED);
	}

	*aa_pp =
	    (SysmanAliasArg *)malloc((unsigned)(cnt * sizeof (SysmanAliasArg)));

	if (*aa_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &alias, &expansion, &options, &comment);

		(*aa_pp)[i].alias_key = NULL;
		(*aa_pp)[i].alias = alias ? strdup(alias) : NULL;
		(*aa_pp)[i].expansion = expansion ? strdup(expansion) : NULL;
		(*aa_pp)[i].options = options ? strdup(options) : NULL;
		(*aa_pp)[i].comment = comment ? strdup(comment) : NULL;
	}

	return (cnt);
}


void
_free_alias(SysmanAliasArg *aa_p)
{

	if (aa_p->alias != NULL) {
		free((void *)aa_p->alias);
	}
	if (aa_p->expansion != NULL) {
		free((void *)aa_p->expansion);
	}
	if (aa_p->options != NULL) {
		free((void *)aa_p->options);
	}
	if (aa_p->comment != NULL) {
		free((void *)aa_p->comment);
	}
}


void
_free_alias_list(SysmanAliasArg *aa_p, int cnt)
{

	int	i;


	if (aa_p == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		_free_alias(aa_p + i);
	}

	free((void *)aa_p);
}
