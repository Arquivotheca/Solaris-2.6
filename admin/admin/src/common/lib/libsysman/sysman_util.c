/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)sysman_util.c	1.6	96/09/09 SMI"


#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include "sysman_types.h"
#include "sysman_codes.h"

#include <nl_types.h> 
#define TRUE	1
#define FALSE	0
nl_catd	_catlibsysman;  /* for sysman catgets() */

void
open_sysman_cat()
{
static catopened = FALSE;
if (!catopened) {
                _catlibsysman = catopen("libsysman.cat", 0);
                catopened = TRUE;
        }
}

void
init_err_msg(char *buf, int bufsiz, const char *msg)
{

	if (buf != NULL && bufsiz > 0) {
		strncpy(buf, msg, (size_t)bufsiz);
		buf[bufsiz - 1] = '\0';
	}
}


int
check_ns_user_conflicts(const char *username, uid_t uid)
{

	struct passwd	*pwnam;
	struct passwd	*pwuid;
	boolean_t	name_ok = B_TRUE;
	boolean_t	id_ok = B_TRUE;


	if (username != NULL && (pwnam = getpwnam(username)) != NULL) {
		name_ok = B_FALSE;
	}

	if ((pwuid = getpwuid(uid)) != NULL) {
		id_ok = B_FALSE;
	}

	if (name_ok == B_TRUE && id_ok == B_TRUE) {
		return (SYSMAN_CONFLICT_NONE);
	}

	if (name_ok == B_FALSE && id_ok == B_FALSE) {
		if (pwnam->pw_uid == pwuid->pw_uid &&
		    strcmp(pwnam->pw_name, pwuid->pw_name) == 0) {
			return (SYSMAN_CONFLICT_BOTH_USED);
		}
	}

	if (name_ok == B_FALSE) {
		return (SYSMAN_CONFLICT_NAME_USED);
	} else {
		return (SYSMAN_CONFLICT_ID_USED);
	}
}


int
check_ns_group_conflicts(const char *groupname, gid_t gid)
{

	struct group	*grnam;
	struct group	*grgid;
	boolean_t	name_ok = B_TRUE;
	boolean_t	id_ok = B_TRUE;


	if (groupname != NULL && (grnam = getgrnam(groupname)) != NULL) {
		name_ok = B_FALSE;
	}

	if ((grgid = getgrgid(gid)) != NULL) {
		id_ok = B_FALSE;
	}

	if (name_ok == B_TRUE && id_ok == B_TRUE) {
		return (SYSMAN_CONFLICT_NONE);
	}

	if (name_ok == B_FALSE && id_ok == B_FALSE) {
		if (grnam->gr_gid == grgid->gr_gid &&
		    strcmp(grnam->gr_name, grgid->gr_name) == 0) {
			return (SYSMAN_CONFLICT_BOTH_USED);
		}
	}

	if (name_ok == B_FALSE) {
		return (SYSMAN_CONFLICT_NAME_USED);
	} else {
		return (SYSMAN_CONFLICT_ID_USED);
	}
}


int
check_ns_host_conflicts(const char *hostname, const char *ip_addr)
{

	struct hostent	*hnam;
	struct hostent	*hid;
	u_long		addr;
	boolean_t	name_ok = B_TRUE;
	boolean_t	id_ok = B_TRUE;


	if (hostname != NULL && (hnam = gethostbyname(hostname)) != NULL) {
		name_ok = B_FALSE;
	}

	addr = inet_addr(ip_addr);
	if ((hid =
	    gethostbyaddr((char *)&addr, sizeof (addr), AF_INET)) != NULL) {
		id_ok = B_FALSE;
	}

	if (name_ok == B_TRUE && id_ok == B_TRUE) {
		return (SYSMAN_CONFLICT_NONE);
	}

	if (name_ok == B_FALSE && id_ok == B_FALSE) {
		if (strcmp(hnam->h_name, hid->h_name) == 0) {
			return (SYSMAN_CONFLICT_BOTH_USED);
		}
	}

	if (name_ok == B_FALSE) {
		return (SYSMAN_CONFLICT_NAME_USED);
	} else {
		return (SYSMAN_CONFLICT_ID_USED);
	}
}


/*
 * A couple of utilities for manipualting the jobsched cron time value lists.
 */


int
cp_time_a_from_l(j_time_elt_t *a, int a_len, j_time_elt_t *l)
{

	int		i;
	j_time_elt_t	*l_p;


	for (i = 0, l_p = l; i < a_len && l_p != NULL; i++, l_p = l_p->next) {
		memcpy((void *)&a[i], (void *)l_p, sizeof (j_time_elt_t));
	}

	if (i == a_len && l_p != NULL) {
		return (SYSMAN_JOBSCHED_OVFLW);
	}

	return (SYSMAN_SUCCESS);
}


j_time_elt_t *
mk_time_l_from_a(j_time_elt_t *a)
{

	int		i;
	j_time_elt_t	*ret = NULL;
	j_time_elt_t	*t;
	j_time_elt_t	*prev = NULL;


	if (a == NULL) {
		return (NULL);
	}

	for (i = 0; ; i++) {

		t = (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		if (t == NULL) {
			return (NULL);
		}

		if (ret == NULL) {
			ret = t;
		}

		memcpy((void *)t, (void *)&a[i], sizeof (j_time_elt_t));

		if (prev != NULL) {
			prev->next = t;
		}

		prev = t;

		if (a[i].next == NULL) {
			break;
		}
	}

	return (ret);
}


char *
cron_list_to_field_string(j_time_elt_t *list)
{

	char		val_buf[128];
	char		buf[512];
	int		len;
	j_time_elt_t	*ptr;


	if (list == NULL) {
		return (strdup("*"));
	}

	buf[0] = '\0';

	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		switch (ptr->type_tag) {
		case j_atom:
			(void) sprintf(val_buf, "%d", ptr->value.atom);
			(void) strcat(buf, val_buf);
			(void) strcat(buf, ",");
			break;
		case j_range:
			(void) sprintf(val_buf, "%d-%d",
			    ptr->value.range.range_low,
			    ptr->value.range.range_high);
			(void) strcat(buf, val_buf);
			(void) strcat(buf, ",");
			break;
		case j_wildcard:
			(void) strcat(buf, "*");
			(void) strcat(buf, ",");
			break;
		}
	}

	/* NULL out last , return */

	if ((len = strlen(buf)) > 0) {
		buf[len - 1] = '\0';
		return (strdup(buf));
	}

	return (NULL);
}


j_time_elt_t *
field_string_to_malloc_cron_list(const char *str)
{

	j_time_elt_t	*ret = NULL;
	j_time_elt_t	*t;
	j_time_elt_t	*prev = NULL;
	const char	*p;
	const char	*p1;
	const char	*p2;
	char		strbuf[128];
	char		buf[128];


	if (strchr(str, ',') != NULL) {

		/* field string is a list -- count elements */

		p1 = str;

		for ( ; (p2 = strchr(p1, ',')) != NULL; p1 = p2 + 1) {

			t = (j_time_elt_t *)malloc(sizeof (j_time_elt_t));

			if (prev != NULL) {
				prev->next = t;
			}

			if (ret == NULL) {
				ret = t;
			}

			(void) strncpy(strbuf, p1, p2 - p1);
			strbuf[p2 - p1] = 0;

			if (strcmp(strbuf, "*") == 0) {
				t->type_tag = j_wildcard;
			} else if ((p = strchr(strbuf, '-')) != NULL) {
				t->type_tag = j_range;
				(void) strncpy(buf, strbuf, p - strbuf);
				buf[p - strbuf] = 0;
				t->value.range.range_low = atoi(buf);
				t->value.range.range_high = atoi(p + 1);
			} else {
				t->type_tag = j_atom;
				t->value.atom = atoi(strbuf);
			}

			prev = t;
		}

		t = (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		t->next = NULL;
		
		prev->next = t;

		(void) strcpy(strbuf, p1);

		if (strcmp(strbuf, "*") == 0) {
			t->type_tag = j_wildcard;
		} else if ((p = strchr(strbuf, '-')) != NULL) {
			t->type_tag = j_range;
			(void) strncpy(buf, strbuf, p - strbuf);
			buf[p - strbuf] = 0;
			t->value.range.range_low = atoi(buf);
			t->value.range.range_high = atoi(p + 1);
		} else {
			t->type_tag = j_atom;
			t->value.atom = atoi(strbuf);
		}
	} else {
		ret = (j_time_elt_t *)malloc(sizeof (j_time_elt_t));

		if (ret == NULL) {
			return (NULL);
		}

		if (strcmp(str, "*") == 0) {
			ret->type_tag = j_wildcard;
		} else if ((p = strchr(str, '-')) != NULL) {
			ret->type_tag = j_range;
			(void) strncpy(buf, str, p - str);
			buf[p - str] = 0;
			ret->value.range.range_low = atoi(buf);
			ret->value.range.range_high = atoi(p + 1);
		} else {
			ret->type_tag = j_atom;
			ret->value.atom = atoi(str);
		}

		ret->next = NULL;
	}

	return (ret);
}


void
free_malloc_cron_list(j_time_elt_t *l)
{

	j_time_elt_t	*n;


	if (l == NULL) {
		return;
	}

	for (n = l->next; n != NULL; l = n, n = n->next) {
		free((void *)l);
	}

	/* last one */
	free((void *)l);
}


int
field_string_to_cron_list(const char *str, j_time_elt_t *t_p, int t_p_len)
{

	int		cnt;
	j_time_elt_t	*t;
	j_time_elt_t	*prev = NULL;
	const char	*p;
	const char	*p1;
	const char	*p2;
	char		strbuf[128];
	char		buf[128];


	if (str == NULL || t_p == NULL || t_p_len == 0) {
		return (SYSMAN_BAD_INPUT);
	}

	if (strchr(str, ',') != NULL) {

		/* field string is a list -- count elements */

		p1 = str;

		for (cnt = 0; (p2 = strchr(p1, ',')) != NULL, cnt < t_p_len;
		    p1 = p2 + 1, cnt++) {

			t = &t_p[cnt];

			if (prev != NULL) {
				prev->next = t;
			}

			(void) strncpy(strbuf, p1, p2 - p1);
			strbuf[p2 - p1] = 0;

			if (strcmp(strbuf, "*") == 0) {
				t->type_tag = j_wildcard;
			} else if ((p = strchr(strbuf, '-')) != NULL) {
				t->type_tag = j_range;
				(void) strncpy(buf, strbuf, p - strbuf);
				buf[p - strbuf] = 0;
				t->value.range.range_low = atoi(buf);
				t->value.range.range_high = atoi(p + 1);
			} else {
				t->type_tag = j_atom;
				t->value.atom = atoi(strbuf);
			}

			prev = t;
		}

		if (cnt == t_p_len) {
			return (SYSMAN_JOBSCHED_OVFLW);
		}

		t = &t_p[cnt];
		t->next = NULL;
		
		prev->next = t;

		(void) strcpy(strbuf, p1);

		if (strcmp(strbuf, "*") == 0) {
			t->type_tag = j_wildcard;
		} else if ((p = strchr(strbuf, '-')) != NULL) {
			t->type_tag = j_range;
			(void) strncpy(buf, strbuf, p - strbuf);
			buf[p - strbuf] = 0;
			t->value.range.range_low = atoi(buf);
			t->value.range.range_high = atoi(p + 1);
		} else {
			t->type_tag = j_atom;
			t->value.atom = atoi(strbuf);
		}
	} else {
		t = &t_p[0];

		if (strcmp(str, "*") == 0) {
			t->type_tag = j_wildcard;
		} else if ((p = strchr(str, '-')) != NULL) {
			t->type_tag = j_range;
			(void) strncpy(buf, str, p - str);
			buf[p - str] = 0;
			t->value.range.range_low = atoi(buf);
			t->value.range.range_high = atoi(p + 1);
		} else {
			t->type_tag = j_atom;
			t->value.atom = atoi(str);
		}

		t->next = NULL;
	}

	return (SYSMAN_SUCCESS);
}
