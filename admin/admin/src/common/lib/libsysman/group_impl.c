/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)group_impl.c	1.11	95/01/31 SMI"


#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <sys/types.h>
#include "sysman_impl.h"
#include "admldb.h"


int
_root_add_group(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanGroupArg	*ga_p = (SysmanGroupArg *)arg_p;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_GROUP_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_ADD, &err, tbl,
	    ga_p->groupname_key, ga_p->gid_key,
	    &ga_p->groupname, &ga_p->passwd, &ga_p->gid, &ga_p->members);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_GROUP_ADD_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_root_delete_group(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanGroupArg	*ga_p = (SysmanGroupArg *)arg_p;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_GROUP_TBL);

	status = lcl_remove_table_entry(DB_NS_UFS, NULL, NULL, 0L, &err, tbl,
	    ga_p->groupname_key, ga_p->gid_key);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_GROUP_DEL_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_root_modify_group(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanGroupArg	*ga_p = (SysmanGroupArg *)arg_p;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_GROUP_TBL);

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_MODIFY,
	    &err, tbl,
	    ga_p->groupname_key, ga_p->gid_key,
	    &ga_p->groupname, &ga_p->passwd, &ga_p->gid, &ga_p->members);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		status = SYSMAN_GROUP_MOD_FAILED;
	}

	free_table(tbl);

	return (status);
}


int
_get_group(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanGroupArg	*ga_p = (SysmanGroupArg *)arg_p;
	char		*groupname;
	char		*passwd;
	char		*gid;
	char		*members;


	if (ga_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_GROUP_TBL);

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_LIST_SINGLE,
	    &err, tbl,
	    ga_p->groupname_key, ga_p->gid_key,
	    &groupname, &passwd, &gid, &members);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_GROUP_GET_FAILED);
	}

	ga_p->groupname =
	    groupname ? strdup(groupname) : strdup(ga_p->groupname_key);
	ga_p->passwd = passwd ? strdup(passwd) : NULL;
	ga_p->gid = gid ? strdup(gid) : strdup(ga_p->gid_key);
	ga_p->members = members ? strdup(members) : NULL;

	free_table(tbl);

	return (status);
}


int
_list_group(SysmanGroupArg **ga_pp, char *buf, int len)
{

	int		i;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*groupname;
	char		*passwd;
	char		*gid;
	char		*members;


	if (ga_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_GROUP_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_SORT_LIST,
	    &err, tbl, NULL, NULL);

	if (cnt < 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_GROUP_GET_FAILED);
	}

	*ga_pp =
	    (SysmanGroupArg *)malloc((unsigned)(cnt * sizeof (SysmanGroupArg)));

	if (*ga_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &groupname, &passwd, &gid, &members);

		(*ga_pp)[i].groupname_key = NULL;
		(*ga_pp)[i].gid_key = NULL;
		(*ga_pp)[i].groupname = groupname ? strdup(groupname) : NULL;
		(*ga_pp)[i].passwd = passwd ? strdup(passwd) : NULL;
		(*ga_pp)[i].gid = gid ? strdup(gid) : NULL;
		(*ga_pp)[i].members = members ? strdup(members) : NULL;
	}

	return (cnt);
}


void
_free_group(SysmanGroupArg *ga_p)
{

	if (ga_p->groupname != NULL) {
		free((void *)ga_p->groupname);
	}
	if (ga_p->passwd != NULL) {
		free((void *)ga_p->passwd);
	}
	if (ga_p->gid != NULL) {
		free((void *)ga_p->gid);
	}
	if (ga_p->members != NULL) {
		free((void *)ga_p->members);
	}
}


void
_free_group_list(SysmanGroupArg *ga_p, int cnt)
{

	int	i;


	if (ga_p == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		_free_group(ga_p + i);
	}

	free((void *)ga_p);
}


static
int
gid_sorter(const void *gid_p1, const void *gid_p2)
{

	gid_t	gid_1 = *(gid_t *)gid_p1;
	gid_t	gid_2 = *(gid_t *)gid_p2;


	if (gid_1 > gid_2) {
		return (1);
	}
	if (gid_1 < gid_2) {
		return (-1);
	}
	return (0);
}


gid_t
_get_next_avail_gid(gid_t min_gid)
{

	int		i;
	gid_t		*sorted_gids;
	gid_t		group_id;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*groupname;
	char		*passwd;
	char		*gid;
	char		*members;


	tbl = table_of_type(DB_GROUP_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, 0, &err, tbl, NULL, NULL);

	if (cnt < 0) {
		return (min_gid);
	}

	sorted_gids = (gid_t *)malloc(cnt * sizeof (gid_t));

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &groupname, &passwd, &gid, &members);

		sorted_gids[i] = atoi(gid);
	}

	qsort((void *)sorted_gids, cnt, sizeof (gid_t), gid_sorter);

	group_id = min_gid;

	for (i = 0; i < cnt; i++) {

		if (sorted_gids[i] < min_gid) {
			continue;
		}

		if (group_id == sorted_gids[i]) {
			/*
			 * "group_id" already in use, increment it and
			 * try again, otherwise break out and return
			 */
			group_id++;
		} else {
			break;
		}
	}

	return (group_id);
}
