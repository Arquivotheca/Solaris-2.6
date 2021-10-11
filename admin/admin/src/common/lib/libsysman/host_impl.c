/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)host_impl.c	1.9	95/01/31 SMI"


#include <stdlib.h>
#include <stddef.h>
#include "sysman_impl.h"
#include "admldb.h"


int
_root_add_host(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanHostArg	*ha_p = (SysmanHostArg *)arg_p;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_HOSTS_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_ADD, &err, tbl,
	    ha_p->ipaddr_key, ha_p->hostname_key,
	    &ha_p->ipaddr, &ha_p->hostname, &ha_p->aliases, &ha_p->comment);

	free_table(tbl);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		return (SYSMAN_HOST_ADD_FAILED);
	}

	return (status);
}


int
_root_delete_host(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanHostArg	*ha_p = (SysmanHostArg *)arg_p;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_HOSTS_TBL);

	status = lcl_remove_table_entry(DB_NS_UFS, NULL, NULL, DB_ADD,
	    &err, tbl, ha_p->ipaddr_key, ha_p->hostname_key);

	free_table(tbl);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		return (SYSMAN_HOST_DEL_FAILED);
	}

	return (status);
}


int
_root_modify_host(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanHostArg	*ha_p = (SysmanHostArg *)arg_p;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_HOSTS_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_MODIFY,
	    &err, tbl,
	    ha_p->ipaddr_key, ha_p->hostname_key,
	    &ha_p->ipaddr, &ha_p->hostname, &ha_p->aliases, &ha_p->comment);

	free_table(tbl);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		return (SYSMAN_HOST_MOD_FAILED);
	}

	return (status);
}


int
_get_host(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanHostArg	*ha_p = (SysmanHostArg *)arg_p;
	char		*ipaddr;
	char		*hostname;
	char		*aliases;
	char		*comment;


	if (ha_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_HOSTS_TBL);

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_LIST_SINGLE,
	    &err, tbl,
	    ha_p->ipaddr_key, ha_p->hostname_key,
	    &ipaddr, &hostname, &aliases, &comment);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		return (SYSMAN_HOST_GET_FAILED);
	}

	ha_p->ipaddr = ipaddr ? strdup(ipaddr) : strdup(ha_p->ipaddr_key);
	ha_p->hostname =
	    hostname ? strdup(hostname) : strdup(ha_p->hostname_key);
	ha_p->aliases = aliases ? strdup(aliases) : NULL;
	ha_p->comment = comment ? strdup(comment) : NULL;

	free_table(tbl);

	return (status);
}


int
_list_host(SysmanHostArg **ha_pp, char *buf, int len)
{

	int		i;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*ipaddr;
	char		*hostname;
	char		*aliases;
	char		*comment;


	if (ha_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_HOSTS_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_SORT_LIST,
	    &err, tbl, NULL, NULL);

	if (cnt < 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_HOST_GET_FAILED);
	}

	*ha_pp =
	    (SysmanHostArg *)malloc((unsigned)(cnt * sizeof (SysmanHostArg)));

	if (*ha_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &ipaddr, &hostname, &aliases, &comment);

		(*ha_pp)[i].ipaddr_key = NULL;
		(*ha_pp)[i].hostname_key = NULL;
		(*ha_pp)[i].ipaddr = ipaddr ? strdup(ipaddr) : NULL;
		(*ha_pp)[i].hostname = hostname ? strdup(hostname) : NULL;
		(*ha_pp)[i].aliases = aliases ? strdup(aliases) : NULL;
		(*ha_pp)[i].comment = comment ? strdup(comment) : NULL;
	}

	return (cnt);
}


void
_free_host(SysmanHostArg *ha_p)
{
	if (ha_p->ipaddr != NULL) {
		free((void *)ha_p->ipaddr);
	}
	if (ha_p->hostname != NULL) {
		free((void *)ha_p->hostname);
	}
	if (ha_p->aliases != NULL) {
		free((void *)ha_p->aliases);
	}
	if (ha_p->comment != NULL) {
		free((void *)ha_p->comment);
	}
}


void
_free_host_list(SysmanHostArg *ha_p, int cnt)
{

	int	i;


	if (ha_p == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		_free_host(ha_p + i);
	}

	free((void *)ha_p);
}
