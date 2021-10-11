/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)user_impl.c	1.43	96/08/27 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <crypt.h>
#include <limits.h>
#include <fcntl.h>
#include <shadow.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "sysman_impl.h"
#include "admldb.h"


#define	MAX_S_GRP	32


static const char	*group_err_msg =
	"The following groups do not exist in /etc/group.  You must add "
	" these groups using the Group Management function before users "
	" can be created as members of the group: ";


/*
 * The following ZSTR macros and encrypt_password were ripped from
 * unbundled/classes/libadmusr
 */

#define	ZSTRCMP(s1, s2) (s1 == NULL ? (s2 == NULL ? 0 : -1) \
				    : (s2 == NULL ? 1 : strcmp(s1, s2)))

#define	ZSTRCPY(s1, s2)	if (s1 != NULL) {			\
				if (s2 != NULL) {		\
					(void) strcpy(s1, s2);	\
				} else {			\
					*s1 = NULL;		\
				}				\
			}

#define _USR_2ATOU(cp)   (((*(cp) - '0') * 10) + (*((cp) + 1) - '0'))

void
_usr_u2a(u_int u, int min_len, char *p)
{
	/* Like itoa, but we use knowledge that u is positive */

	char temp[100];
	int i = 0;
	int j;

	do {
		temp[i++] = (u % 10) + '0';
	} while ( (u /=10) > 0);

		/* If needed, pad the string with zeros */
	for (j = i; j < min_len; j++, p++)
		*p = '0';

		/* Digits are stored in temp in reverse order */
	for (i--; i >= 0; i--, p++)
		*p = temp[i];

	*p = NULL;
}

/*
 * _USR_DATE2DAYS:
 *
 *	Convert a date specified in the form DDMMYYYY to the
 *	number of days since 01-January-1970 UTC.
 */

int
_usr_date2days(
	int *err,
	const char *cdate,
	char *ndays
	)
{
	u_int days;
	struct tm tm_cdate;
	time_t nsecs;

	/* Convert date to number of seconds since 01-Jan-1970 UTC */

	tm_cdate.tm_sec  = 1;
	tm_cdate.tm_min  = 0;
	tm_cdate.tm_hour = 0;
	tm_cdate.tm_mday = _USR_2ATOU(cdate);
	tm_cdate.tm_mon  = _USR_2ATOU(cdate + 2) - 1;
	tm_cdate.tm_year = ((_USR_2ATOU(cdate + 4) * 100)
			 +   _USR_2ATOU(cdate + 6))
			 - 1900;
	tm_cdate.tm_isdst = -1;

	nsecs = mktime(&tm_cdate);
	if (nsecs == (time_t) -1) {
		*err = errno;
		return(-1);
	}

	/* Days = Seconds / Seconds-per-Day */

	days = nsecs / DAY;
	_usr_u2a(days, -1, ndays);

	return(days);
}

/*
 * _USR_DAYS2DATE
 *
 *	Convert a date specified in number of days since 01-January-1970
 *	to the form DDMMYYYY.
 */

int
_usr_days2date(
	int *err,
	char *ndays,
	char *cdate
	)
{
	int days;
	char *ptr;
	struct tm *tm_p;
	time_t nsecs;

	tzset();

	days = strtol(ndays, &ptr, 10);
	if ((ptr == ndays) || (*ptr != NULL)) {
		*err = 0;
		return(-1);
	}
	if (*ndays == '-') {
	    strcpy(cdate, "");
	    return(0);
	}

	/* Convert days to number of seconds since 01-Jan-1970. */

	nsecs = (days * DAY) + 1;

	tm_p = gmtime(&nsecs);
	if (tm_p == NULL) {
		*err = errno;
		return(-1);
	}

	_usr_u2a(tm_p->tm_mday,        2, cdate);
	_usr_u2a(tm_p->tm_mon + 1,     2, (char *)(cdate + 2));
	_usr_u2a(tm_p->tm_year + 1900, 4, (char *)(cdate + 4));

	return(days);
}

static
void
encrypt_passwd(const char *passwd, char *epasswd)
{

	time_t	salt;
	char	saltc[2];
	char	*e_pw;
	int	i;
	int	m;


	if (passwd == NULL || epasswd == NULL) {
		return;
	}

	/*
	 * If passwd is blank, locked, or no passwd, just copy it
	 * straight over without encrypting.
	 */

	if (strcmp(passwd, "") == 0 ||
	    strcmp(passwd, "*LK*") == 0 ||
	    strcmp(passwd, "NP") == 0) {
		strcpy(epasswd, passwd);
	} else {
		(void) time((time_t *)&salt);
		salt += (long)getpid();
		saltc[0] = salt & 077;
		saltc[1] = (salt >> 6) & 077;
		for (i = 0; i < 2; i++) {
			m = saltc[i] + '.';
			if (m > '9') m += 7;
			if (m > 'Z') m += 6;
			saltc[i] = m;
		}
		e_pw = crypt(passwd, saltc);
		(void) strcpy(epasswd, e_pw);
	}
}



static
boolean_t
is_all_digits(const char *group)
{

	int	i;
	int	len;
	int	retval;


	len = strlen(group);

	for (i = 0; i < len; i++) {
		if (isdigit(group[i]) == 0) {
			break;
		}
	}

	if (i == len) {
		retval = B_TRUE;
	} else {
		retval = B_FALSE;
	}

	return (retval);
}


static
const char *
group_to_gid(const char *group)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	const char	*groupname;
	const char	*passwd;
	const char	*members;
	const char	*gid;


	if (group == NULL) {
		return (NULL);
	}

	if (is_all_digits(group) != B_FALSE) {
		return (strdup(group));
	}

	tbl = table_of_type(DB_GROUP_TBL);

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_LIST_SINGLE,
	    &err, tbl,
	    group, NULL,
	    &groupname, &passwd, &gid, &members);

	free_table(tbl);

	if (status == 0) {
		return (strdup(gid));
	} else {
		return (NULL);
	}
}


static
int
validate_user_input(SysmanUserArg *ua_p, char *buf, int len)
{

	int		status;
	int		retval = 0;
	char		tmp_buf[1024];
	char		*passwd;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tmp_buf[0] = '\0';

	/* Make sure passwd isn't NULL */

	if (ua_p->passwd == NULL) {
		return (SYSMAN_USER_NO_PASSWD);
	}

	status = validate_user_groups(ua_p, buf, len);

	if (status != 0) {
		return (status);
	}

	if (ua_p->path != NULL && ua_p->path[0] != '\0') {

		if (ua_p->path[0] != '/') {

			if (tmp_buf[0] == '\0') {
				strncpy(buf,
				    "The home directory must be an "
				    "absolute path", len);
				buf[len - 1] = '\0';
				retval = SYSMAN_USER_BAD_HOME;
			} else {
				strcat(tmp_buf,
				    "\nThe home directory must be an "
				    "absolute path");
				strncpy(buf, tmp_buf, len);
				buf[len - 1] = '\0';
				retval = SYSMAN_USER_BAD_HOME;
			}
		}
	}

	return (retval);
}

static
int
validate_user_groups(SysmanUserArg *ua_p, char *buf, int len)
{

	int		i;
	int		j;
	int		retval = 0;
	char		tmp_buf[1024];
	Table		*tbl;
	Db_error	*err;
	int		cnt;
	char		*ptr;
	char		*c_ptr;		/* "comma" pointer */
	int		pgv = 0;	/* primary group valid? */
	char		*s_groups = NULL;
	char		*sg[MAX_S_GRP];	/* secondary group pointers */
	int		sgv[MAX_S_GRP];	/* secondary group valid? */
	int		nsg = 0;	/* number of secondary groups */
	int		nsgv = 0;	/* number of sg's found valid */
	char		*groupname;
	char		*passwd;
	char		*gid;
	char		*members;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tmp_buf[0] = '\0';
	/* Make sure that input groups are all valid, existing groups. */

	if (ua_p->second_grps != NULL && ua_p->second_grps[0] != '\0') {

		s_groups = strdup(ua_p->second_grps);

		if (s_groups == NULL) {

			/* out of memory */

			strncpy(buf, "Out of memory, can't continue", len);
			buf[len - 1] = '\0';
			return (SYSMAN_MALLOC_ERR);
		}

		ptr = s_groups;

		while (ptr != NULL) {

			sg[nsg] = ptr;
			c_ptr = strchr(sg[nsg], ',');

			if (c_ptr != NULL) {
				*c_ptr = '\0';
				ptr = c_ptr + 1;
			} else {
				ptr = NULL;
			}

			sgv[nsg] = 0;

			nsg++;
		}

	}

	tbl = table_of_type(DB_GROUP_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, 0, &err, tbl, NULL, NULL);

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &groupname, &passwd, &gid, &members);

		if (pgv == 0) {
			/* Check primary group */

			if (strcmp(ua_p->group, groupname) == 0 ||
			    strcmp(ua_p->group, gid) == 0) {

				/* found primary group in /etc/group */
				pgv = 1;
			}
		}

		if (nsgv < nsg) {
			for (j = 0; j < nsg; j++) {
				if (sgv[j] == 0) {

					/* Check j'th secondary group */

					if (strcmp(sg[j], groupname) == 0 ||
					    strcmp(sg[j], gid) == 0) {

						/* found group */
						sgv[j] = 1;
						nsgv++;
					}
				}
			}
		}
	}

	free_table(tbl);

	/*
	 * "If primary group isn't marked valid OR the number of secondary
	 *  groups found to be valid is less then the number of secondary
	 *  groups that were passed in" ...
	 */

	if (pgv == 0 || nsgv < nsg) {

		strcpy(tmp_buf, group_err_msg);

		if (pgv == 0) {
			strcat(tmp_buf, ua_p->group);
			strcat(tmp_buf, ", ");
		}

		for (i = 0; i < nsg; i++) {
			if (sgv[i] == 0) {
				strcat(tmp_buf, sg[i]);
				strcat(tmp_buf, ", ");
			}
		}

		/* get rid of last ", " */
		tmp_buf[strlen(tmp_buf) - 2] = '\0';

		strncpy(buf, tmp_buf, len);
		buf[len - 1] = '\0';
		retval = SYSMAN_USER_BAD_GROUP;
	}

	if (s_groups != NULL) {
		free(s_groups);
	}
	return (retval);

}


static
int
add_user_to_groups(const char *username, const char *second_grps)
{

	int		status;
	int		retval;
	Table		*tbl;
	Db_error	*err;
	char		group_buf[1024];
	char		other_key_buf[1024];
	const char	*groupname_key;
	const char	*gid_key;
	const char	*groupname;
	const char	*passwd;
	const char	*gid;
	const char	*members;
	const char	*sp;
	const char	*ep;
	char		*p;
	char		new_members[1024];


	retval = 0;

	if (second_grps == NULL || second_grps[0] == '\0') {
		return (retval);
	}

	/*
	 * For each secondary group, get the group table entry, append
	 * this user's name to the members list, and modify the entry
	 * with the new members list.
	 */

	tbl = table_of_type(DB_GROUP_TBL);

	sp = second_grps;

	group_buf[0] = '\0';

	while (sp != NULL) {

		ep = strchr(sp, ',');

		if (ep != NULL) {
			strncpy(group_buf, sp, ep - sp);
			group_buf[ep - sp] = '\0';
			sp = ep + 1;
		} else {
			strcpy(group_buf, sp);
			sp = NULL;
		}

		if (is_all_digits(group_buf) == B_FALSE) {
			groupname_key = group_buf;
			gid_key = NULL;
		} else {
			groupname_key = NULL;
			gid_key = group_buf;
		}

		status = lcl_list_table(DB_NS_UFS, NULL, NULL,
		    DB_LIST_SINGLE, &err, tbl, groupname_key, gid_key,
		    &groupname, &passwd, &gid, &members);

		if (status != 0) {
			if (retval == 0) {
				retval = SYSMAN_USER_GROUP_FAILED;
			}
		}

		/*
		 * If user is already a member of the group,
		 * continue onto next group.  This shouldn't
		 * happen, since this is a NEW user, but
		 * maybe somebody updated the group file
		 * first with the username.
		 */

		if (members != NULL && strstr(members, username) != NULL) {
			continue;
		}

		if (members != NULL && members[0] != '\0') {
			strcpy(new_members, members);
			strcat(new_members, ",");
		} else {
			new_members[0] = '\0';
		}
		strcat(new_members, username);

		if (groupname_key == NULL) {
			strcpy(other_key_buf, groupname);
			groupname_key = other_key_buf;
		} else {
			strcpy(other_key_buf, gid);
			gid_key = other_key_buf;
		}

		p = new_members;

		status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL,
		    DB_MODIFY, &err, tbl, groupname_key, gid_key,
		    &groupname, &passwd, &gid, &p);

		if (status != 0) {
			retval = SYSMAN_USER_GROUP_FAILED;
		}
	}

	free_table(tbl);

	return (retval);
}


static
int
copy_file(const char *infile, const char *outfile, mode_t mode)
{

	int		retval = 0;
	int		fdin;
	int		fdout;
	char		*src;
	char		*dst;
	struct stat	stat_buf;


	/*
	 * From Stevens,
	 * "Advanced Programming in the UNIX Environment", p. 412
	 */

	if ((fdin = open(infile, O_RDONLY)) < 0) {
		return (-1);
	}

	if ((fdout = open(outfile, O_RDWR | O_CREAT | O_TRUNC, mode)) < 0) {
		close(fdin);
		return (-1);
	}

	if (fstat(fdin, &stat_buf) < 0) {
		retval = -1;
		goto finish;
	}

	/* set size of output file */

	if (lseek(fdout, stat_buf.st_size - 1, SEEK_SET) == -1) {
		retval = -1;
		goto finish;
	}

	if (write(fdout, "", 1) != 1) {
		retval = -1;
		goto finish;
	}

	if ((src = mmap(0, stat_buf.st_size, PROT_READ, MAP_SHARED,
	    fdin, 0)) == (caddr_t)-1) {
		retval = -1;
		goto finish;
	}

	if ((dst = mmap(0, stat_buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,
	    fdout, 0)) == (caddr_t)-1) {
		retval = -1;
		goto finish;
	}

	(void) memcpy(dst, src, stat_buf.st_size);

	finish:

	close(fdin);
	close(fdout);

	return (retval);
}


static
int
remove_home_dir(const char *path)
{

	char	cmd[1024];


	if (path == NULL) {
		return (0);
	}

	sprintf(cmd, "set -f ; /bin/rm -r %s 1>/dev/null 2>&1", path);
	return (system(cmd));
}


static
int
make_home_dir(const char *path, int uid, int gid, const char *shell)
{

	char		path_comp[PATH_MAX];
	char		*backout_path = NULL;
	const char	*slash_ptr;
	char		*working_path;
	char		*end_ptr;
	struct stat	stat_buf;
	char		outfile[PATH_MAX];


	if (path == NULL || path[0] == '\0' || path[0] != '/' ||
	    shell == NULL || shell[0] == '\0') {
		return (SYSMAN_BAD_INPUT);
	}

	if (stat(path, &stat_buf) == 0) {
		/*
		 * user asked to create a home directory, but the 
		 * directory already exists, return failure 
		 */
		return (SYSMAN_USER_HOME_DIR_FAILED);
	}

	working_path = strdup(path);

	slash_ptr = working_path;

	/* eliminate trailing '/' characters */
	end_ptr = working_path + strlen(working_path) - 1;
	while (*end_ptr == '/') {
		--end_ptr;
	}
	*end_ptr = '\0';

	while (slash_ptr != NULL) {

		/* gobble extra '/' characters */
		while (*slash_ptr == '/') {
			slash_ptr++;
		}

		strncpy(path_comp, path, slash_ptr - working_path);
		path_comp[slash_ptr - working_path] = '\0';

		if (stat(path_comp, &stat_buf) != 0) {

			if (errno == ENOENT) {
				/* path component doesn't exist, create it */
				if (mkdir(path_comp, 0755) == -1) {
					(void) remove_home_dir(backout_path);
					return (SYSMAN_USER_HOME_DIR_FAILED);
				}

				if (backout_path == NULL) {
					backout_path = strdup(path_comp);
				}
			} else {
				/* hosed, return failure */
				(void) remove_home_dir(backout_path);
				return (SYSMAN_USER_HOME_DIR_FAILED);
			}
		}

		slash_ptr = strchr(slash_ptr, '/');
	}

	free(working_path);

	/* finished components, now make the home dir */

	if (mkdir(path, 0755) == -1) {
		(void) remove_home_dir(backout_path);
		return (SYSMAN_USER_HOME_DIR_FAILED);
	}
	(void) chown(path, (uid_t)uid, (gid_t)gid);

	/* copy .login and appropriate shell init file from /etc/skel */


	if (strcmp(shell, "/bin/csh") == 0) {
		sprintf(outfile, "%s/.login", path);
		(void) copy_file("/etc/skel/local.login", outfile, 0644);
		(void) chown(outfile, (uid_t)uid, (gid_t)gid);

		sprintf(outfile, "%s/.cshrc", path);
		(void) copy_file("/etc/skel/local.cshrc", outfile, 0644);
		(void) chown(outfile, (uid_t)uid, (gid_t)gid);
	} else if (strcmp(shell, "/bin/sh") == 0 ||
	    strcmp(shell, "/bin/ksh") == 0 ||
	    strcmp(shell, "/bin/jsh") == 0) {
		sprintf(outfile, "%s/.profile", path);
		(void) copy_file("/etc/skel/local.profile", outfile, 0644);
		(void) chown(outfile, (uid_t)uid, (gid_t)gid);
	}

	return (SYSMAN_SUCCESS);
}


int
_root_add_user(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanUserArg	*ua_p = (SysmanUserArg *)arg_p;
	int		uid_int;
	int		gid_int;
	const char	*gid;
	const char	*e;
	char		epasswd[64];
	struct stat	stat_buf;
    	char 		expire_days[100];
	char 		*expirep;
    	int 		arg2;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	status = validate_user_input(ua_p, buf, len);

	if (status != 0) {
		return (status);
	}

	gid = group_to_gid(ua_p->group);

	encrypt_passwd(ua_p->passwd, epasswd);
	e = epasswd;

	tbl = table_of_type(DB_PASSWD_TBL);

    	if ((ua_p->expire != NULL) &&
		(strcmp(ua_p->expire, "") != 0)) {
		status = _usr_date2days(&arg2, ua_p->expire, expire_days);
		if (status == -1) {
	    		/* ERROR - date conversion error */
			strncpy(buf,
			    "Failure converting expiration date.", len - 1);
			buf[len - 1] = '\0';
			return (SYSMAN_USER_ADD_FAILED);
		}
		expirep = expire_days;
    	} else {
		expirep = (char *) ua_p->expire;
    	}


	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_ADD, &err, tbl,
	    ua_p->username_key,
	    &ua_p->username, &e, &ua_p->uid, &gid,
	    &ua_p->comment, &ua_p->path, &ua_p->shell, &ua_p->lastchanged,
	    &ua_p->minimum, &ua_p->maximum, &ua_p->warn, &ua_p->inactive,
	    &expirep, &ua_p->flag);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		buf[len - 1] = '\0';
		free_table(tbl);
		return (SYSMAN_USER_ADD_FAILED);
	}

	free_table(tbl);

	/* Add user to secondary groups */

	if (ua_p->second_grps != NULL && ua_p->second_grps[0] != '\0') {
		status = add_user_to_groups(ua_p->username, ua_p->second_grps);
		if (status != 0) {
			strncpy(buf,
			    "Failure adding user to secondary groups", len - 1);
			buf[len - 1] = '\0';
			return (status);
		}
	}

	/* Create the user's home directory */

	if (ua_p->home_dir_flag != B_FALSE) {

		uid_int = atoi(ua_p->uid);
		gid_int = atoi(gid);

		free((void *)gid);

		status = make_home_dir(ua_p->path, uid_int, gid_int, 
				ua_p->shell);

		if (status != 0) {
			ua_p->home_dir_flag = B_FALSE;
			(void) _root_delete_user(arg_p, buf, len);
			(void) strncpy(buf, "Could not create users Home Directory.", len - 1);
			return (status);
		} 
	} else {
		if (ua_p->path != NULL && ua_p->path[0] != '\0') {
			/*
			 * check ownership of existing home directory.
			 * If added user does not own the directory
			 * give an informational message.
			 * If path does not exist, give an informational
			 * message.
			 */
			if (stat(ua_p->path, &stat_buf) == 0) {
				if (stat_buf.st_uid != (uid_t)uid_int) {
					(void) strncpy(buf, "Home Directory is owned by a different user.", len - 1);
					return (SYSMAN_INFO);
				} 
			} else {
				(void) strncpy(buf, "Home Directory does not yet exist.", len - 1);
				return(SYSMAN_INFO);
			}

		}

	}

	return (SYSMAN_SUCCESS);
}


/*
 * is_user_in_member_list -- takes a username, a comma separated list
 * of group members, and returns TRUE or FALSE to indicate whether the
 * username is found in the list.  If found, sets ret_ptr to point to
 * the first occurance of the username in the list.
 * Returns FALSE on any memory allocation failure.
 */

static
boolean_t
is_user_in_member_list(const char *username, const char *list, char **ret_ptr)
{

	char		*p;
	char		*l;
	char		*new_l;
	boolean_t	retval = B_FALSE;


	if (username == NULL || list == NULL) {
		return (B_FALSE);
	}

	l = new_l = strdup(list);

	if (new_l == NULL) {
		return (B_FALSE);
	}

	p = strtok(new_l, ",");
	if (strcmp(username, p) == 0) {

		/* found the user */

		if (ret_ptr != NULL) {
			*ret_ptr = (char *)list;
		}

		retval = B_TRUE;
	}

	while ((p = strtok(NULL, ",")) != NULL) {
		if (strcmp(username, p) == 0) {

			/* found the user */

			if (ret_ptr != NULL) {
				*ret_ptr = (char *)(list + (p - l));
			}

			retval = B_TRUE;

			break;
		}
	}

	free((void *)l);

	return (retval);
}


static
int
remove_user_from_groups(const char *username)
{

	int		i;
	int		len;
	int		retval = 0;
	int		status;
	char		*ptr = NULL;
	Table		*tbl;
	Db_error	*err;
	int		cnt;
	const char	*groupname;
	const char	*passwd;
	const char	*gid;
	const char	*members;
	char		new_members[1024];


	tbl = table_of_type(DB_GROUP_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, 0,
	    &err, tbl, NULL, NULL);

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &groupname, &passwd, &gid, &members);

		if (is_user_in_member_list(username, members, &ptr) == B_TRUE) {

			/*
			 *first, copy the members that precede the
			 * member being removed
			 */

			new_members[0] = '\0';

			if (ptr != members) {
				strncpy(new_members, members, ptr - members);
				new_members[ptr - members] = '\0';
			}

			/*
			 * Now look for a comma following the deleted
			 * member name.  If found, there are member
			 * names that follow it, and they need to be
			 * copied; otherwise, we're done.
			 */

			if ((ptr = strchr(ptr, ',')) != NULL) {
				ptr++;
				if (ptr != '\0') {
					strcat(new_members, ptr);
				}
			} else {
				len = strlen(new_members);
				if (new_members[len - 1] == ',') {
					new_members[len - 1] = '\0';
				}
			}

			ptr = new_members;

			status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL,
			    DB_MODIFY, &err, tbl, groupname, gid,
			    &groupname, &passwd, &gid, &ptr);

			if (status != 0) {
				retval = SYSMAN_USER_GROUP_FAILED;
			}
		}
	}

	free_table(tbl);

	return (retval);
}


int
_root_delete_user(void *arg_p, char *buf, int len)
{

	int		status;
	Table		*tbl;
	Db_error	*err;
	SysmanUserArg	*ua_p = (SysmanUserArg *)arg_p;
	const char	*username;
	const char	*passwd;
	const char	*uid;
	const char	*gid;
	const char	*comment;
	const char	*path;
	const char	*shell;
	const char	*lastchanged;
	const char	*maximum;
	const char	*minimum;
	const char	*warn;
	const char	*inactive;
	const char	*expire;
	const char	*flag;
	int		uid_int;
	struct stat	stat_buf;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_PASSWD_TBL);

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_LIST_SINGLE,
	    &err, tbl, ua_p->username_key,
	    &username, &passwd, &uid, &gid, &comment,
	    &path, &shell, &lastchanged, &maximum, &minimum,
	    &warn, &inactive, &expire, &flag);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_USER_NO_USER);
	}

	status = lcl_remove_table_entry(DB_NS_UFS, NULL, NULL, 0L, &err, tbl,
	    ua_p->username_key);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_USER_DEL_FAILED);
	}

	free_table(tbl);


	if (ua_p->home_dir_flag != B_FALSE) {
		if (stat(path, &stat_buf) == 0) {
			uid_int = atoi(ua_p->uid);
			if (stat_buf.st_uid != (uid_t)uid_int) {
				(void) strncpy(buf, "Home Directory is owned by a different user.  Directory NOT removed.", len - 1);
				return (SYSMAN_INFO);
			} else {
				(void) remove_home_dir(path);
			}
		} 
	}

	status = remove_user_from_groups(username);

	if (status != 0) {
		return (status);
	}

	return (SYSMAN_SUCCESS);
}


int
_root_modify_user(void *arg_p, char *buf, int len)
{

	int		status;
	int		retval = 0;
	const char	*gid;
	Table		*tbl;
	Db_error	*err;
	char		epasswd[64];
	const char	*new_passwd;
	SysmanSharedUserArg	ua;
	char		local_buf[256];
	char		last_change[8];	/* number of days since 1/1/70 */
	SysmanUserArg	*ua_p = (SysmanUserArg *)arg_p;
	struct timeval	tod;
	int		uid_int;
	struct stat	stat_buf;
    	char 		expire_days[100];
	char 		*expirep;
    	int 		arg2;


	if (ua_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	status = validate_user_groups(ua_p, buf, len);

	if (status != 0) {
		return (status);
	}

	/*
	 * first get the current user entry; we need to do this to
	 * determine of the passwd is being modified.
	 */

	strcpy(ua.username_key, ua_p->username_key);
	status = common_get_user(&ua, local_buf, sizeof (local_buf), B_TRUE);

	if (status != 0) {
		(void) strncpy(buf, local_buf, len - 1);
		return (SYSMAN_USER_MOD_FAILED);
	}

	/*
	 * If current passwd is the same as the passwd being passed in
	 * with the modify info, then it isn't being changed, and we
	 * already have the encrypted passwd.  If it's different, the
	 * user's passwd is being changed, and we need to encrypt it.
	 * If the modify info passwd is NULL, pass it down, that will
	 * be a "clear password" modification.
	 */

	if (ua_p->passwd == NULL) {
		/* requesting a clear passwd */
		new_passwd = NULL;
		if (ua.passwd != NULL) {  /* is this a change? if so.. */
			/* set lastchg for /etc/shaddow, days since 1/1/70 */
			if (gettimeofday(&tod, (struct timezone *)0) < 0)
				return (SYSMAN_USER_MOD_FAILED);
			sprintf (last_change, "%d", tod.tv_sec/(24*60*60));
			ua_p->lastchanged = last_change;
		}
	} else {
		/* a real passwd ... */
		if ((ua.passwd != NULL) &&
		    (strcmp(ua_p->passwd, ua.passwd) == 0)) {

			/*
			 * has a passwd, but it's not changing; already
			 * encrypted, pass it right through.
			 */

			new_passwd = ua_p->passwd;
		} else {

			/* This is a new passwd, needs to be encrypted. */
			encrypt_passwd(ua_p->passwd, epasswd);
			new_passwd = epasswd;
			/* set lastchg for /etc/shaddow, days since 1/1/70 */
			if (gettimeofday(&tod, (struct timezone *)0) < 0)
				return (SYSMAN_USER_MOD_FAILED);
			sprintf (last_change,"%d", tod.tv_sec/(24*60*60) );
			ua_p->lastchanged = last_change;
		}
	}

	gid = group_to_gid(ua_p->group);

	tbl = table_of_type(DB_PASSWD_TBL);

    	if ((ua_p->expire != NULL) &&
		(strcmp(ua_p->expire, "") != 0)) {
		status = _usr_date2days(&arg2, ua_p->expire, expire_days);
		if (status == -1) {
	    		/* ERROR - date conversion error */
			strncpy(buf,
			    "Failure converting expiration date.", len - 1);
			buf[len - 1] = '\0';
			return (SYSMAN_USER_MOD_FAILED);
		}
		expirep = expire_days;
    	} else {
		expirep = (char *) ua_p->expire;
    	}

	status = lcl_set_table_entry(DB_NS_UFS, NULL, NULL, DB_MODIFY,
	    &err, tbl,
	    ua_p->username_key,
	    &ua_p->username, &new_passwd, &ua_p->uid, &gid,
	    &ua_p->comment, &ua_p->path, &ua_p->shell, &ua_p->lastchanged,
	    &ua_p->minimum, &ua_p->maximum, &ua_p->warn, &ua_p->inactive,
	    &expirep, &ua_p->flag);

	free((void *)gid);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		retval = SYSMAN_USER_MOD_FAILED;
	}

	free_table(tbl);

	/*
	 * To effectively modify the secondary group memberships,
	 * remove the (old) user from all groups and add the (new)
	 * user back to the specified groups.  This is a lot of
	 * overhead if the group membership and username haven't
	 * changed, but since there's no way to find that out
	 * without traversing the table and checking it's probably
	 * not too expensive.  Look here for a speedup if performance
	 * problems exist.
	 */

	if (remove_user_from_groups(ua_p->username_key) != SYSMAN_SUCCESS) {
		retval = SYSMAN_USER_GROUP_FAILED;
	}
	if (add_user_to_groups(ua_p->username, ua_p->second_grps) !=
	    SYSMAN_SUCCESS) {
		retval = SYSMAN_USER_GROUP_FAILED;
	}

	if (ua_p->path != NULL && ua_p->path[0] != '\0') {
		/*
		 * check ownership of existing home directory.
		 * If  user does not own the directory
		 * give an informational message.
		 * If path does not exist, give an informational
		 * message.
		 */
		uid_int = atoi(ua_p->uid);

		if (stat(ua_p->path, &stat_buf) == 0) {
			if (stat_buf.st_uid != (uid_t)uid_int) {
				(void) strncpy(buf, "Home Directory is owned by a different user.", len - 1);
				return (SYSMAN_INFO);
			} 
		} else {
			(void) strncpy(buf, "Home Directory does not yet exist.", len - 1);
			return(SYSMAN_INFO);
		}

	}


	return (retval);
}


static
int
common_get_user(
	SysmanSharedUserArg	*ua_p,
	char			*buf,
	int			len,
	boolean_t		do_shadow)
{

	int			status;
	int			retval = SYSMAN_SUCCESS;
	int			i;
	int			cnt;
	Table			*tbl;
	Db_error		*err;
	char			*username;
	char			*passwd;
	char			*uid;
	char			*gid;
	char			*comment;
	char			*path;
	char			*shell;
	char			*lastchanged;
	char			*minimum;
	char			*maximum;
	char			*warn;
	char			*inactive;
	char			*expire;
	char			*flag;
	char			*groupname;
	char			*members;
	char			grp_buf[1024];
	ulong_t			tbl_flags;
	char			date[100];
	int			arg2;


	if (ua_p == NULL || ua_p->username_key == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_PASSWD_TBL);

	if (do_shadow == B_FALSE) {
		tbl_flags = DB_LIST_SINGLE;
	} else {
		tbl_flags = DB_LIST_SINGLE | DB_LIST_SHADOW;
	}

	status = lcl_list_table(DB_NS_UFS, NULL, NULL, tbl_flags,
	    &err, tbl,
	    ua_p->username_key,
	    &username, &passwd, &uid, &gid, &comment, &path, &shell,
	    &lastchanged, &minimum, &maximum, &warn, &inactive,
	    &expire, &flag);

	if (status != 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_USER_GET_FAILED);
	}

	if (username != NULL) {
		strcpy(ua_p->username, username);
	} else {
		ua_p->username[0] = '\0';
	}
	if (passwd != NULL) {
		strcpy(ua_p->passwd, passwd);
	} else {
		ua_p->passwd[0] = '\0';
	}
	if (uid != NULL) {
		strcpy(ua_p->uid, uid);
	} else {
		ua_p->uid[0] = '\0';
	}
	if (gid != NULL) {
		strcpy(ua_p->group, gid);
	} else {
		ua_p->group[0] = '\0';
	}
	if (comment != NULL) {
		strcpy(ua_p->comment, comment);
	} else {
		ua_p->comment[0] = '\0';
	}
	if (path != NULL) {
		strcpy(ua_p->path, path);
	} else {
		ua_p->path[0] = '\0';
	}
	if (shell != NULL) {
		strcpy(ua_p->shell, shell);
	} else {
		ua_p->shell[0] = '\0';
	}
	if (lastchanged != NULL) {
		strcpy(ua_p->lastchanged, lastchanged);
	} else {
		ua_p->lastchanged[0] = '\0';
	}
	if (minimum != NULL) {
		strcpy(ua_p->minimum, minimum);
	} else {
		ua_p->minimum[0] = '\0';
	}
	if (maximum != NULL) {
		strcpy(ua_p->maximum, maximum);
	} else {
		ua_p->maximum[0] = '\0';
	}
	if (warn != NULL) {
		strcpy(ua_p->warn, warn);
	} else {
		ua_p->warn[0] = '\0';
	}
	if (inactive != NULL) {
		strcpy(ua_p->inactive, inactive);
	} else {
		ua_p->inactive[0] = '\0';
	}
	if (expire != NULL) {
		if (strcmp(expire, "") != 0) {
			status = _usr_days2date(&arg2, expire, date);
			if (status == -1) {
				(void) strncpy(buf, "Error converting password expiration date.", len - 1);
				free_table(tbl);
				return (SYSMAN_USER_GET_FAILED);
			}
			strcpy(ua_p->expire, date);
		} else {
			strcpy(ua_p->expire, expire);
		}
	} else {
		ua_p->expire[0] = '\0';
	}
	if (flag != NULL) {
		strcpy(ua_p->flag, flag);
	} else {
		ua_p->flag[0] = '\0';
	}

	free_table(tbl);

	grp_buf[0] = '\0';

	tbl = table_of_type(DB_GROUP_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, 0,
	    &err, tbl, NULL, NULL);

	if (cnt < 0) {
		retval = SYSMAN_USER_GROUP_FAILED;
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &groupname, &passwd, &gid, &members);

		if (members != NULL &&
		    (strstr(members, ua_p->username_key)) != NULL) {

			strcat(grp_buf, groupname);
			strcat(grp_buf, ",");
		}
	}

	if (grp_buf[0] != '\0') {
		/* truncate last comma */
		grp_buf[strlen(grp_buf) - 1] = '\0';
		strcpy(ua_p->second_grps, grp_buf);
	} else {
		ua_p->second_grps[0] = '\0';
	}

	free_table(tbl);

	return (retval);
}


int
_get_user(SysmanSharedUserArg *arg_p, char *buf, int len)
{
	return (common_get_user(arg_p, buf, len, B_FALSE));
}


int
_root_get_user(void *arg_p, char *buf, int len)
{
	return (common_get_user((SysmanSharedUserArg *)arg_p,
	    buf, len, B_TRUE));
}


int
_list_user(SysmanUserArg **ua_pp, char *buf, int len)
{

	int		i;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*username;
	char		*passwd;
	char		*uid;
	char		*gid;
	char		*comment;
	char		*path;
	char		*shell;
	char		*lastchanged;
	char		*minimum;
	char		*maximum;
	char		*warn;
	char		*inactive;
	char		*expire;
	char		*flag;


	if (ua_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	tbl = table_of_type(DB_PASSWD_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, DB_SORT_LIST,
	    &err, tbl, NULL, NULL);

	if (cnt < 0) {
		(void) strncpy(buf, err->msg, len - 1);
		free_table(tbl);
		return (SYSMAN_USER_GET_FAILED);
	}

	*ua_pp =
	    (SysmanUserArg *)malloc((unsigned)(cnt * sizeof (SysmanUserArg)));

	if (*ua_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &username, &passwd, &uid, &gid, &comment,
		    &path, &shell, &lastchanged, &maximum, &minimum,
		    &warn, &inactive, &expire, &flag);

		(*ua_pp)[i].username_key = NULL;
		(*ua_pp)[i].username = username ? strdup(username) : NULL;
		(*ua_pp)[i].passwd = passwd ? strdup(passwd) : NULL;
		(*ua_pp)[i].uid = uid ? strdup(uid) : NULL;
		(*ua_pp)[i].group = gid ? strdup(gid) : NULL;
		(*ua_pp)[i].comment = comment ? strdup(comment) : NULL;
		(*ua_pp)[i].path = path ? strdup(path) : NULL;
		(*ua_pp)[i].shell = shell ? strdup(shell) : NULL;
		(*ua_pp)[i].lastchanged =
		    lastchanged ? strdup(lastchanged) : NULL;
		(*ua_pp)[i].minimum = minimum ? strdup(minimum) : NULL;
		(*ua_pp)[i].maximum = maximum ? strdup(maximum) : NULL;
		(*ua_pp)[i].warn = warn ? strdup(warn) : NULL;
		(*ua_pp)[i].inactive = inactive ? strdup(inactive) : NULL;
		(*ua_pp)[i].expire = expire ? strdup(expire) : NULL;
		(*ua_pp)[i].flag = flag ? strdup(flag) : NULL;
	}

	free_table(tbl);

	return (cnt);
}


void
_free_user(SysmanUserArg *ua_p)
{

	if (ua_p->username != NULL) {
		free((void *)ua_p->username);
	}
	if (ua_p->passwd != NULL) {
		free((void *)ua_p->passwd);
	}
	if (ua_p->uid != NULL) {
		free((void *)ua_p->uid);
	}
	if (ua_p->group != NULL) {
		free((void *)ua_p->group);
	}
	if (ua_p->comment != NULL) {
		free((void *)ua_p->comment);
	}
	if (ua_p->path != NULL) {
		free((void *)ua_p->path);
	}
	if (ua_p->shell != NULL) {
		free((void *)ua_p->shell);
	}
	if (ua_p->lastchanged != NULL) {
		free((void *)ua_p->lastchanged);
	}
	if (ua_p->minimum != NULL) {
		free((void *)ua_p->minimum);
	}
	if (ua_p->maximum != NULL) {
		free((void *)ua_p->maximum);
	}
	if (ua_p->warn != NULL) {
		free((void *)ua_p->warn);
	}
	if (ua_p->inactive != NULL) {
		free((void *)ua_p->inactive);
	}
	if (ua_p->expire != NULL) {
		free((void *)ua_p->expire);
	}
	if (ua_p->flag != NULL) {
		free((void *)ua_p->flag);
	}
}


void
_free_user_list(SysmanUserArg *ua_p, int cnt)
{

	int	i;


	if (ua_p == NULL) {
		return;
	}
	for (i = 0; i < cnt; i++) {
		_free_user(ua_p + i);
	}

	free((void *)ua_p);
}


static
int
uid_sorter(const void *uid_p1, const void *uid_p2)
{

	uid_t	uid_1 = *(uid_t *)uid_p1;
	uid_t	uid_2 = *(uid_t *)uid_p2;


	if (uid_1 > uid_2) {
		return (1);
	}
	if (uid_1 < uid_2) {
		return (-1);
	}
	return (0);
}


uid_t
_get_next_avail_uid(uid_t min_uid)
{

	int		i;
	uid_t		*sorted_uids;
	uid_t		user_id;
	int		cnt;
	Table		*tbl;
	Db_error	*err;
	char		*username;
	char		*passwd;
	char		*uid;
	char		*gid;
	char		*comment;
	char		*path;
	char		*shell;
	char		*lastchanged;
	char		*minimum;
	char		*maximum;
	char		*warn;
	char		*inactive;
	char		*expire;
	char		*flag;


	tbl = table_of_type(DB_PASSWD_TBL);

	cnt = lcl_list_table(DB_NS_UFS, NULL, NULL, 0, &err, tbl, NULL, NULL);

	if (cnt < 0) {
		return (min_uid);
	}

	sorted_uids = (uid_t *)malloc(cnt * sizeof (uid_t));

	if (sorted_uids == NULL) {
		return (min_uid);
	}

	for (i = 0; i < cnt; i++) {

		(void) get_next_entry(&err, tbl,
		    &username, &passwd, &uid, &gid, &comment,
		    &path, &shell, &lastchanged, &maximum, &minimum,
		    &warn, &inactive, &expire, &flag);

		sorted_uids[i] = atoi(uid);
	}

	qsort((void *)sorted_uids, cnt, sizeof (uid_t), uid_sorter);

	user_id = min_uid;

	for (i = 0; i < cnt; i++) {

		if (sorted_uids[i] < min_uid) {
			continue;
		}

		if (user_id == sorted_uids[i]) {
			/*
			 * "user_id" already in use, increment it and
			 * try again, otherwise break out and return
			 */
			user_id++;
		} else {
			break;
		}
	}

	return (user_id);
}
