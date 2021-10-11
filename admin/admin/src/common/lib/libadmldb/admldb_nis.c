/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)admldb_nis.c	1.13	95/08/16 SMI"


/*
 * This module has only limited Cstyle conformance since Cstyle has
 * trouble with shell commands as part of strings.
 *
 * NIS_DEBUG is enabled by default.  If the file /tmp/NIS_DEBUG exists,
 * then debug information is output to this file.  If this file does not
 * exist, then no debug info is produced.
 *
 * Although the code refers to a lock file, the code has been enhanced to
 * instead use a directory as the locking mechanism.  The Unix command
 * equivalent of a test&set is 'mkdir'.
 */

#include <sys/param.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <admldb.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>

extern char *	basename(char *);
extern int	format_entry();
extern void	db_err_set();
extern int	list_table_impl();

int		shadow_map_exists(char **, char *);
int		verify_groups_yp( Db_error **, char *, char *);

static int	_servername(Db_error **, Table *, char *, char []);
static int	_server_probe(Db_error **, Table *, char *, char []);
static int	_edit_map(Db_error **, Table *, char *, char *, char *,
		    int, char **, struct tbl_trans_data	 *);
static char *	_yp_mapname(char *,Table *);
static void	_unlock_yp(char *, Table *);
static int	_popen(Db_error **, char *, char *, char [], int);
static int	_yp_match_key(Db_error **, char *, char ***, int, Table *);
static int	_create_policy_table(Db_error **, char *, char *);
static void	_set_group_member_buf(char [], char *, char *, int);
static char *	_locate_group_member( char *, char *);
static int 	_mk_regx_word(Db_error **, char [], char [], char [], char []);

#define LOCK_CREATE_RETRY	6	/* Number of retries */
#define LOCK_CREATE_TIMEOUT	10	/* Time (sec) to wait between retries */
#define LOCK_DIR		"lock_dir"
#define EDIT_CMD		"edcmdfile"

#define	LOCK_FILE_EXT	"ADM_LOCK_YPMAP"
#define	SMALLBUF	256
#define	LARGEBUF	2048

#define	NIS_ADD		0x01
#define	NIS_REMOVE 	0x02
#define	NIS_MODIFY	0x04
#define	NIS_NOMAP	0x08
#define	NIS_YESMAP	0x10

#define NIS_DEBUG
#ifdef NIS_DEBUG
#  define debug_fprintf		if(dfp) fprintf
	FILE	*dfp;
#endif

/*
 * Global storage for error messages.
 */
static char	errmsg[LARGEBUF];

/*
 * Function change a nis table
 */
int
set_nis_db(
	char			*host,
	char			*domain,
	ulong_t			 flags,
	Db_error	        **db_err,
	Table			*tbl,
	char		       **iargs,
	char		      ***oargs,
	struct tbl_trans_data	*ttp,
	int			 action,
	uid_t			*uidp)
{
	int			 edit_flag = 0;
	char			 new_line[LARGEBUF];
	char			 servername[SMALLBUF];
	char			 server_mappath[MAXPATHLEN];
	int			 retry = LOCK_CREATE_RETRY;

	(void) memset(new_line, '\0', sizeof (new_line));

	if ((geteuid() == (uid_t)0) && (uidp != NULL)) {
	    if (setuid(*uidp) < 0) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
		    "set_nis_db", "setuid", strerror(errno));
		return (-1);
	    }
	}
 
#ifdef NIS_DEBUG
#  define DEBUG_FILE "/tmp/NIS_DEBUG"

	if (access(DEBUG_FILE, W_OK) == 0) {
	    dfp = fopen(DEBUG_FILE, "a+");
	    (void) system("/bin/chmod 777 /tmp/NIS_DEBUG");
	    (void) setvbuf(dfp, (char *)NULL, _IONBF, (size_t) 0);
	}
#endif
	
	if (_servername(db_err, tbl, domain, servername) < 0)
		return (-1);
	/*
	 * Retry obtaining the lock directory.
	 */
	while (retry != 0) {
	    edit_flag = _server_probe(db_err, tbl, servername, server_mappath);
	    if (edit_flag < 0) {
		if (edit_flag == -2) {
		    /*
		     * Failed because lock dir already exists.
		     */
#ifdef NIS_DEBUG
		    debug_fprintf(dfp,"Lock dir already exists.  Sleep/retry %d\n",
			LOCK_CREATE_RETRY - retry + 1);
#endif
		    if (--retry) {	/* No use to sleep on last retry */
			sleep(LOCK_CREATE_TIMEOUT);
		    }
		} else {
		    _unlock_yp(servername, tbl);
		    return (-1);
		}
	    } else {
		break;	/* Got the lock dir */
	    }
	}
	/*
	 * If we have not obtained the lock after all the retries, return
	 * and error.
	 */
	if (retry == 0) {
	    return (-1);
	}

	if ((edit_flag & NIS_NOMAP) && (tbl->type == DB_POLICY_TBL)) {
	    if (_create_policy_table(db_err, servername, server_mappath) < 0) {
		_unlock_yp(servername, tbl);
		return (-1);
	    }
	    edit_flag &= ~NIS_NOMAP; edit_flag |= NIS_YESMAP;
	} else {
	    if ((edit_flag & NIS_NOMAP) &&
	       ((action == DB_REMOVE) || (flags & DB_MODIFY))) {

#ifdef NIS_DEBUG
		debug_fprintf(dfp, "server = %s\nserver_mappath= %s\n", servername, server_mappath);
#endif
		if (tbl->type == DB_MAIL_ALIASES_TBL)
		    strcpy(errmsg, ADMLDB_MSGS(DB_ERR_BAD_NIS_ALIASES));
		else
		    strcpy(errmsg, ADMLDB_MSGS(DB_ERR_BAD_NIS_DIR));
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    errmsg, servername);
		_unlock_yp(servername, tbl);
		return (-1);
	    }
	}
#ifdef NIS_DEBUG
	if (domain != NULL) debug_fprintf(dfp, "domain = %s\n", domain);
	debug_fprintf(dfp, "server = %s\nserver_mappath= %s\n",
		servername, server_mappath);
	debug_fprintf(dfp, "map name = %s\n", _yp_mapname(servername, tbl));
#endif

	/*
	 * list_table_impl is destructive to oargs so format new line and
	 * do special passwd/networks check in _yp_match_key() first.
	 */
	if (action == DB_REMOVE) {
		edit_flag |= NIS_REMOVE;
	} else {
		if (format_entry(new_line, sizeof (new_line),
		    oargs, ttp) != 0) {
			db_err_set(db_err, DB_ERR_TOO_BIG, ADM_FAILCLEAN,
			    "set_nis_db");
			(void) _unlock_yp(servername, tbl);
			return (-1);
		}

		if (ttp->type == DB_PASSWD_TBL) {
		    char* shadow_error_string;

		    if (!shadow_map_exists(&shadow_error_string, domain)) {
			char tail[256];
			char* pos                = strchr(new_line, ':') + 1;
			char* after_password_pos = strchr(pos, ':');

			strcpy(tail, after_password_pos);
			*pos = '\0';
			strcat(new_line, *oargs[1]); /* actual password */
			strcat(new_line, tail);
		    }
		}

		if (_yp_match_key(db_err, domain, oargs, flags, tbl) < 0) {
			(void) _unlock_yp(servername, tbl);
			return (-1);
		}

		flags |= DB_LIST_SINGLE;
		if (list_table_impl(DB_NS_NIS, host, domain, flags,
		    db_err, tbl, iargs, oargs, ttp, action, uidp) < 0) {
			if ((*db_err)->errno == DB_ERR_NO_ENTRY || \
			    (*db_err)->errno == DB_ERR_NO_TABLE) {
				db_err_free(db_err);
				edit_flag |= NIS_ADD;
			} else {
				(void) _unlock_yp(servername, tbl);
				return (-1);
			}
		} else {
			/* Entry found */
			if (flags & DB_MODIFY) {
				edit_flag |= NIS_MODIFY;
			} else {
				db_err_set(db_err, DB_ERR_ENTRY_EXISTS,
				    ADM_FAILCLEAN, "set_nis_db",
				    new_line, tbl->tn.nis);
				(void) _unlock_yp(servername, tbl);
				return (-1);
			}
		}
	}

#ifdef NIS_DEBUG
	if (edit_flag) {
		char	fbuf[BUFSIZ];

		fbuf[0] = '\0';
		if (edit_flag & NIS_ADD) strcat(fbuf, "NIS_ADD  ");
		if (edit_flag & NIS_REMOVE) strcat(fbuf, "NIS_REMOVE  ");
		if (edit_flag & NIS_MODIFY) strcat(fbuf, "NIS_MODIFY  ");
		if (edit_flag & NIS_NOMAP) strcat(fbuf, "NIS_NOMAP  ");
		if (edit_flag & NIS_YESMAP) strcat(fbuf, "NIS_YESMAP  ");
		debug_fprintf(dfp, "edit_flags = %s\n", fbuf);
	}
	debug_fprintf(dfp, "new_line = %s", new_line);
#endif

	if (_edit_map(db_err, tbl, servername, server_mappath,
		    new_line, edit_flag, iargs, ttp) < 0) {
		(void) _unlock_yp(servername, tbl);
		return (-1);
	}
#ifdef NIS_DEBUG
	debug_fprintf(dfp,"------------------------------------------------------------------------------\n");
	(void) fclose(dfp);
#endif
	return (0);
}

/*
 * This routine returns the field number we are working with as well
 * as the field itself (the field may be a concatenation of two fields).
 */
int
_get_field_info(
	Db_error		**db_err,
	Table			 *tbl,
	int			 *field_num,
	char			 *field,
	char			**iargs,
	struct tbl_trans_data	 *ttp)
{
	char			 *sp;
	static char	 	  buf[LARGEBUF];
	int		 	  i;
	struct sockaddr_in	  sin;

	memset(buf, '\0', sizeof (buf));
	for (i = 0; i < ttp->match_args.cnt; i++) {
	    if ((iargs[i] == NULL) || !strlen(iargs[i]))
		continue;
	    if ((tbl->type == DB_SERVICES_TBL) && (i == 1)) {
		if ((sp = (char *)malloc((strlen(iargs[1]) +\
			strlen(iargs[2]) + 2))) == NULL) {
		    db_err_set(db_err, DB_ERR_NO_MEMORY, \
				ADM_FAILCLEAN, "set_nis_db");
		    return (-1);
		}
		(void) sprintf(sp, "%s/%s", iargs[1], iargs[2]);
		(void) strcpy(buf, sp);
		i++;
	    } else {
		    (void) strcpy(buf, iargs[i]);
	    }
	    break;	/* we can only look for one key */
	}
	strcpy(field, buf);

	/*
	 * Given the map, figure out which field we are working with.
	 */
	switch(tbl->type) {
	    case DB_AUTO_HOME_TBL:
	    case DB_MAIL_ALIASES_TBL:
	    case DB_BOOTPARAMS_TBL:
	    case DB_NETMASKS_TBL:
	    case DB_NETGROUP_TBL:
	    case DB_NETWORKS_TBL:
	    case DB_LOCALE_TBL:
	    case DB_SHADOW_TBL:
	    case DB_PASSWD_TBL:
		*field_num = 1;
		break;
	    case DB_PROTOCOLS_TBL:
	    case DB_SERVICES_TBL:
	    case DB_TIMEZONE_TBL:
	    case DB_RPC_TBL:
		*field_num = 2;
		break;
	    /*
	     * Group names cannot start with a digit.  If a digit, then field 3
	     * else field 1.
	     */
	    case DB_GROUP_TBL:
		if (isdigit(*buf)) {
		    *field_num = 3;
		} else {
		    *field_num = 1;
		}
		break;
	    /*
	     * If the field is an IP address then field 1 else field 2.
	     */
	    case DB_HOSTS_TBL:
		sin.sin_addr.s_addr = inet_addr(buf);
		if (sin.sin_addr.s_addr == -1 || sin.sin_addr.s_addr == 0) {
		    *field_num = 2;
		} else {
		    *field_num = 1;
		}
		break;
	    /*
	     * If the field contains a ":" then it is field 1 else field 2.
	     */
	    case DB_ETHERS_TBL:
		if (strchr(buf, ':')) {
		    *field_num = 1;
		} else {
		    *field_num = 2;
		}
		break;
	    /*
	     * Most generic case.
	     */
	    default:
		*field_num = 1;
		break;
	}
	return (0);
}

/*
 * Define some macros for dealing with file updates.
 */
#define AWK_CNT "BEGIN {cnt = 0}" \
		"{if(substr($1,1,1) !=  \"#\" && " \
		"$%d  == \"%s\") cnt++ }" \
		"END {print cnt}"

#define AWK_MOD "{if(substr($1,1,1) !=  \"#\" && " \
		"$%d  == \"%s\") print \"%s\"; else print $0}"

#define AWK_DEL "{if(substr($1,1,1) ==  \"#\" || " \
		"$%d  != \"%s\") print $0}"

#define AWK_ADD "{print $0} END {print \"%s\"}"

/*
 * Deal with long awk lines by folding every 70 characters
 * This works around an awk bug which limits the length
 * of an awk command to ~256 characters.
 */
#define FOLD_SIZE 70
char *
awk_fold(char *line) {
	int		len;
	static char	new_line[LARGEBUF+((LARGEBUF/FOLD_SIZE+1)*5)];
	char	*c, *nc;;

	if (line == NULL) return("");
	/*
	 * Strip off trailing CR
	 */
	len = strlen(line);
	if (line[len - 1] == '\n') line[len - 1] = '\0';

	/*
	 * Break line up into little pieces of ~70 chars each
	 * Note: len = length of fragment we are working with.
	 */
	len = 0;
	memset(new_line, 0, LARGEBUF);
	c = line;
	nc = new_line;
	while( *c ) {
	    while(*c && len < FOLD_SIZE) {
	       *nc++ = *c++;
	       len++;
	    }
	    if (*c) {
		*nc++ = '"';
		*nc++ = ' ';
		*nc++ = '\\';
		*nc++ = '\n';
		*nc++ = '"';
	    }
	    len = 0;
	}
	return(new_line);
}

int
_edit_map(
	Db_error		**db_err,
	Table			 *tbl,
	char			 *servername,
	char			 *server_mappath,
	char			 *new_line,
	int			  edit_flag,
	char			**iargs,
	struct tbl_trans_data	 *ttp)
{
	char    		  pipecmd[LARGEBUF];
	int			  field_num;
	char			  field[LARGEBUF];
	char			  awk_opt[5];
	char			  awk_cmd_file[SMALLBUF];
	char			  awk_cmd[BUFSIZ] = { 0 };
	FILE			 *awk_fp = NULL;
	char			  inbuf[BUFSIZ];
	char			  lock_file[SMALLBUF];
	char			  make_mapname[BUFSIZ];

	if (_get_field_info(db_err, tbl, &field_num, field, iargs, ttp) < 0) {
	    return (-1);
	}

	/*
	 * If the column seperator is not the default (white space),
	 * then we will need an appropriate argument to 'awk'
	 */
	if (strcmp(tbl->tn.column_sep, DEFAULT_COLUMN_SEP) != 0) {
	    (void) sprintf(awk_opt, "-F%s", tbl->tn.column_sep);
	} else {
	    strcpy(awk_opt, "");
	}

	inbuf[0] = NULL;
	(void) sprintf(lock_file, "/tmp/%s.%s", _yp_mapname(servername, tbl),
	    LOCK_FILE_EXT);

	new_line = awk_fold(new_line);

	/*
	 * Go out and count how many matches there are.
	 */
	sprintf(awk_cmd_file, "%s.awk", lock_file);
	sprintf(awk_cmd, AWK_CNT, field_num, field);
	(void) sprintf(pipecmd,
	    "/bin/cat %s | "
	    "rsh -l root %s \"/bin/sh -c '/bin/cat > %s ; "
	    "/bin/awk %s -f %s %s"
	    "'\"",
	    awk_cmd_file,			/* cat		*/
	    servername,				/* rsh		*/
	    awk_cmd_file,			/* cat		*/
	    awk_opt,				/* awk		*/
	    awk_cmd_file,			/* awk		*/
	    server_mappath);			/* awk		*/
	if ((awk_fp = fopen(awk_cmd_file, "w")) == NULL) {
	    sprintf(errmsg,
		ADMLDB_MSGS(DB_ERR_BAD_NIS_TEMP), awk_cmd_file);
	    db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		"", errmsg);
	    return (-1);
	}
	fprintf(awk_fp, awk_cmd);
	fclose(awk_fp);

#ifdef NIS_DEBUG
	debug_fprintf(dfp, "Counting key matches.  Command = \n%s\n",
	    pipecmd);
	debug_fprintf(dfp, "AWK CMD BEGIN\n%s\nEND AWK CMD\n", awk_cmd);
#endif

	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0) {
	    return (-1);
	}
	unlink(awk_cmd_file);

	/*
	 * If a modify or remove make sure there is only 1 instance of
	 * the key because we don't know which line to modify/remove if
	 * there is more than one match.
	 */
	if ((edit_flag & NIS_MODIFY) || (edit_flag & NIS_REMOVE)) {
	    if (strcmp(inbuf, "0") == 0) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_EDIT_DEL), server_mappath, field);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    "", errmsg, servername);
		return (-1);
	    } else {
		if (strcmp(inbuf, "1") != 0) {
		    (void) sprintf(errmsg,
			ADMLDB_MSGS(DB_ERR_NIS_EDIT_UNIQ), server_mappath,
			field);
		    db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
			"", errmsg, servername);
		    return (-1);
		}
	    }
	} else {
	    /*
	     * If we are adding what we think is a new entry, make sure there
	     * is not a match.  If there is, then someone has editted
	     * the file and either not remade the map or we caught them in the
	     * middle of making the map.
	     */
	    if ((edit_flag & NIS_ADD) && (strcmp(inbuf, "0") != 0)) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_EDIT_DUP), server_mappath, field);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    "", errmsg, servername);
		return (-1);
	    }
	}

	inbuf[0] = NULL;
	awk_cmd[0] = NULL;

	/*
	 * Perform the actual edit.
	 */
	if (edit_flag & NIS_ADD) {
	    /*
	     * Add a new entry.  Do this by making a copy of the file
	     * and append the new line to the end of the tmp file.
	     * If the file doesn't exist, then we will just create it.
	     */
	    sprintf(awk_cmd, AWK_ADD, new_line);
	} else if (edit_flag & NIS_REMOVE) {
	    /*
	     * Remove an entry.  Do this by copying all non-matching
	     * lines to the tmp file.
	     */
	    sprintf(awk_cmd, AWK_DEL, field_num, field);
	} else if (edit_flag & NIS_MODIFY) {
	    /*
	     * Edit an entry.  Do this by replacing the matching line
	     * with the new one.
	     */
	    sprintf(awk_cmd, AWK_MOD, field_num, field, new_line);
	} else {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    ADMLDB_MSGS(DB_ERR_NIS_INTERNAL),
		    ADMLDB_MSGS( DB_ERR_NIS_BAD_FUNC));
		return (-1);
	}

	(void) sprintf(pipecmd,
	    "/bin/cat %s | "
	    "rsh -l root %s \"/bin/sh -c '/bin/cat > %s ; "
	    "/bin/awk %s -f %s %s > %s ; "
	    "/bin/echo \\$?; "
	    "'\"",
	    awk_cmd_file,				/* cat		*/
	    servername,					/* rsh		*/
	    awk_cmd_file,				/* cat		*/
	    awk_opt,					/* awk		*/
	    awk_cmd_file,				/* awk		*/
	    server_mappath,				/* awk		*/
	    lock_file);					/* awk		*/

	if ((awk_fp = fopen(awk_cmd_file, "w")) == NULL) {
	    sprintf(errmsg,
		ADMLDB_MSGS(DB_ERR_BAD_NIS_TEMP),
		awk_cmd_file);
	    db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		"", errmsg);
	    return (-1);
	}
	fprintf(awk_fp, awk_cmd);
	fclose(awk_fp);

#ifdef NIS_DEBUG
	debug_fprintf(dfp, "BEGIN PIPE COMMAND\n%s\nEND PIPE COMMAND\n", pipecmd);
	if (awk_cmd[0] != 0) {
	    debug_fprintf(dfp, "AWK CMD BEGIN\n%s\nEND AWK CMD\n", awk_cmd);
	}
#endif
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);
#ifdef NIS_DEBUG
	debug_fprintf(dfp, "edit cmd returned = %s\n", inbuf);
#endif
	/*
	 * Check the return status from the edit.  Since all edits do
	 * an 'echo $?' we make sure the exit status is 0 (success).
	 */
	if (strcmp(inbuf, "0") != 0) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_EDIT_FAILED), server_mappath,
		    (edit_flag & NIS_ADD) ? "add" :
		    (edit_flag & NIS_MODIFY) ? "modify" :
		    (edit_flag & NIS_REMOVE) ? "delete" : "edit",
		    field);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    errmsg, _yp_mapname(servername, tbl));
		return (-1);
	}

	/*
	 * Make expects auto.home whether 4.x or 5.x
	 */
	if (tbl->type == DB_AUTO_HOME_TBL)
		strcpy(make_mapname, "auto.home");
	else
		strcpy(make_mapname, basename(server_mappath));

	/*
	 * All done, so move the editted file back and remake the NIS
	 * maps.  When done remove the lock directory.
	 */
	unlink(awk_cmd_file);	/* rm local awk file */
	inbuf[0] = '\0';
	(void) sprintf(pipecmd, 
	    "rsh -l root %s \"/bin/sh -c '\
	    PATH=/bin:/usr/ccs/bin; \
	    /bin/mv -f %s %s;\
	    cd /var/yp; \
	    make %s 2>/dev/null 1>&2 || echo FAIL ; \
	    /bin/rm -rf %s.*' \"",
	    servername,				/* rsh		*/
	    lock_file,				/* mv		*/
	    server_mappath,			/* mv		*/
	    make_mapname,			/* make		*/
	    lock_file);				/* rm		*/
#ifdef NIS_DEBUG
	debug_fprintf(dfp, "BEGIN PIPE COMMAND\n%s\nEND PIPE COMMAND\n", pipecmd);
#endif
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);
#ifdef NIS_DEBUG
	debug_fprintf(dfp, "make returned = %s\n", inbuf);
#endif
	if (strcmp(inbuf, "FAIL") == 0) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    "make", ADMLDB_MSGS(DB_ERR_NIS_MAKE_FAILED));
		return (-1);
	}

	return (0);
}

int
_servername(
	Db_error **db_err,
	Table *tbl,
	char *domain,
	char server[])
{
	int	status;
	char	*servername;
	char	pipecmd[BUFSIZ], inbuf[BUFSIZ];

	inbuf[0] = NULL;

	if ((domain == NULL) || !strlen(domain)) {
		if ((status = yp_get_default_domain(&domain)) != 0) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
			    "set_nis_db", "yp_get_default_domain",
			    yperr_string(status));
			return (-1);
		}
	}

	status = yp_master(domain, tbl->tn.nis, &servername);
	if (status != 0) {
		if (status != YPERR_MAP) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
			    "set_nis_db", "yp_get_default_domain",
			    yperr_string(status));
			return (-1);
		}
	} else {
		(void) strcpy(server, servername);
		free(servername);
		return (0);
	}

	/*
	 * Try ypwhich without a map name
	 */
	(void) sprintf(pipecmd, 
		       "/bin/sh -c \"/bin/ypwhich -d %s 2>/dev/null\"", 
		       domain);
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);
	if (inbuf[0] != NULL) {
		(void) strcpy(server, inbuf);
		return (0);
	}

	/*
	 * Try to grab the error message.
	 */
	(void) sprintf(pipecmd, 
		       "/bin/sh -c \"/bin/ypwhich -d %s 2>&1\"",
		       domain);
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);
	db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
	    "ypwhich", inbuf);

	return (-1);
}

int
_server_probe(
	Db_error **db_err,
	Table	*tbl,
	char *servername,
	char server_mappath[])
{
	char    cmd1[BUFSIZ], cmd2[BUFSIZ], cmd3[BUFSIZ], cmd4[BUFSIZ];
	char    pipecmd[LARGEBUF], inbuf[BUFSIZ];
	char	lock_file[SMALLBUF];

	inbuf[0] = '\0';
	(void) sprintf(lock_file, "/tmp/%s.%s", _yp_mapname(servername, tbl),
		LOCK_FILE_EXT);

	if (tbl->type != DB_MAIL_ALIASES_TBL) {
		/* Extract location of maps from Makefile */
		(void) sprintf(cmd1,
		    "dir=`/bin/grep \"^DIR\" /var/yp/Makefile "
		    "| /bin/sed -e \"s/.*=[ 	]*//\"`; ");

		/*
		 * Check if DIR is set in Makefile and set map location.
		 * Note that we eval dir to strip any quotes.
		 */
		(void) sprintf(cmd2,
		    "if [ -z \"$dir\" ]; then "
		    "	/bin/echo \"NODIR:\"; "
		    "	exit; "
		    "else "
		    "	dir=`eval echo $dir`; "
		    "	map=\"${dir}/%s\"; "
		    "	op=\"${map}:\"; "
		    "fi ; ",
		    _yp_mapname(servername, tbl));
	} else {
		(void) sprintf(cmd1,
		    "dir=`/bin/grep \"^ALIASES\" /var/yp/Makefile "
		    "| /bin/sed -e \"s/.*=[ 	]*//\"`; ");

		(void) sprintf(cmd2,
		    "if [ -z \"$dir\" ]; then "
		    "   /bin/echo \"NODIR:\"; "
		    "   exit; "
		    "else "
		    "   dir=`eval echo $dir`; "
		    "   map=\"${dir}\"; "
		    "   op=\"${map}:\"; "
		    "fi ; ");
	}

	/* Check if the map exists. */
	(void) sprintf(cmd3,
	    "cd /var/yp; "
	    "if [ -f \"$map\" ]; then "
	    "   op=\"${op}MAPEXIST:\" ;"
	    "else "
	    "   op=\"${op}MAPNOEXIST:\"; "
	    "fi; ");

	/* Check for lock dir */
	(void) sprintf(cmd4, 
	    "/bin/mkdir %s.%s >/dev/null 2>&1; "
	    "if test $? -eq 0 ; then "
	    "   op=\"${op}NEWLOCK:\" ; "
	    "else "
	    "   op=\"${op}OLDLOCK:\" ;"
	    "fi; "
	    "echo $op;",
	    lock_file, LOCK_DIR);	/* make lock dir		*/

	(void) sprintf(pipecmd,
	    "/bin/echo '%s %s %s %s' | "
	    "rsh -l root %s 'cat | "
	    "   /bin/sh -s'",
	    cmd1, cmd2, cmd3, cmd4, servername);

#ifdef NIS_DEBUG
 debug_fprintf(dfp, "probe_cmd=%s\n", pipecmd);
#endif
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);

	if (strlen(inbuf) == 0) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_RSH_FAILED), servername);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
		    "set_nis_db", errmsg, "");
		return (-1);
	}

	if (strstr(inbuf, "NODIR:")) {
		if (tbl->type == DB_MAIL_ALIASES_TBL)
		    strcpy(errmsg, ADMLDB_MSGS(DB_ERR_BAD_NIS_ALIASES));
		else
		    strcpy(errmsg, ADMLDB_MSGS(DB_ERR_BAD_NIS_DIR));
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    errmsg, servername);
		return (-1);
	}
	if (strstr(inbuf, "OLDLOCK:")) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_LOCKED),
		    tbl->tn.nis, lock_file, LOCK_DIR);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
		    "set_nis_db", errmsg, servername);
		/*
		 * If the lock dir already exists, return something different
		 * so the caller can detect this situation and can
		 * do some retries.
		 */
		return (-2);
	}

	if (*inbuf != '/')
		(void) strcpy(server_mappath, "/var/yp/");
	else
		server_mappath[0] = '\0';
	(void) strcat(server_mappath, inbuf);
	*(strchr(server_mappath, ':')) = '\0';

	if (strstr(inbuf, "MAPNOEXIST:"))
		return (NIS_NOMAP);
	return (NIS_YESMAP);
}

char *
_yp_mapname(
	char *servername,
	Table *tbl)
{
	static char *auto_home = "auto_home";
	static int firstime = 1, is_fivex = 0;

	if (tbl->type == DB_AUTO_HOME_TBL) {
		if (firstime) {
			char inbuf[BUFSIZ], pipecmd[BUFSIZ];

			firstime = 0; *inbuf = '\0';
			/*
		 	 * The name of the auto.home map changed to auto_home
			 * in 5.x
		 	 */
			(void) sprintf(pipecmd,
			    "rsh -l root %s \"/bin/uname -r\"",
			    servername);
			_popen((Db_error **) NULL, pipecmd, "set_nis_db", inbuf,
			    sizeof (inbuf));
			if (*inbuf == '5')
				is_fivex = 1;
		}
		if (is_fivex && strcmp(tbl->tn.nis, "auto.home") == 0) 
			return (auto_home);
		return (tbl->tn.nis);
	}
	return ((char *)basename(tbl->tn.ufs));
}

void
_unlock_yp(
	char *servername,
	Table *tbl)
{
	char	command[BUFSIZ];
	char	lock_file[SMALLBUF];

	(void) sprintf(lock_file, "/tmp/%s.%s", _yp_mapname(servername, tbl), LOCK_FILE_EXT);
	(void) sprintf(command,
	    "rsh -l root %s \"/bin/rm -rf %s %s.%s %s.%s %s.awk\"",
	    servername, lock_file, lock_file, EDIT_CMD, lock_file, LOCK_DIR,
	    lock_file);
	(void) system(command);

	/*
	 * Clean up any local files that got created.
	 */
	(void) sprintf(command, "/bin/rm -rf %s.awk", lock_file);
	(void) system(command);
}


int
_popen(
	Db_error **db_err,
	char	*pipecmd,
	char	*func,
	char	inbuf[],
	int	bufsize)
{
	FILE *pp;
	char buf[PATH_MAX];

	(void) sprintf( buf, "set -f ; %s", pipecmd );
	if ((pp = popen(buf, "r")) == NULL) {
		if (db_err != NULL)
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, func,
			    "popen", strerror(errno));
		return (-1);
	}
	while (fgets(inbuf, bufsize, pp) != NULL) {
#ifdef NIS_DEBUG
    debug_fprintf(dfp, "======= %s\n", inbuf);
#endif
	}
	inbuf[strlen(inbuf)-1] = NULL;
	(void) pclose(pp);
	return (0);
}


/* The following struct and function support looking up a host in the yp
 * maps without matching entries in the DNS maps.  yp_match will match
 * DNS entries so we'll use yp_all for hosts
 */

/* User data passed in to the yp_all foreach function.  "key" is the
 * key we want to match, found is a flag that is set if the key matches.
 */

typedef struct ypkey {
    int   found;
    char* key;
    int   keylen;
} YpKey;


/* Try to match a key passed in with a key in the map.  If found, set flag
 * and return 1 to avoid being called again.
 */

int
_host_lookup(
	     int   instatus,
	     char* hkey,
	     int   hkeylen,
	     char* hval,
	     int   hvallen,
	     char* data
)
{
    YpKey* keystruct = (YpKey*)data;

    if (hkeylen != keystruct->keylen) {
	return (0);
    }

    if (strncmp(hkey, keystruct->key, hkeylen) == 0) {
	keystruct->found = 1;
	return (1);
    } else {
	return (0);
    }
}
	

/*
 * This function does extra checks for uniqueness.
 */
int
_yp_match_key(
	Db_error **db_err,
	char *domain,
	char ***oargs,
	int flags,
	Table *tbl)
{
	char *table, *key, *buf = NULL;
	int  status, vallen;

	if (((domain == NULL) || !strlen(domain)) &&
	    ((status = yp_get_default_domain(&domain)) != 0)) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
		    "set_nis_db", "yp_get_default_domain",
		    yperr_string(status));
		return (-1);
	}

	if (flags & DB_MODIFY) {
		char	*cp, *cp2;

		/*
		 * If the entry has the same name but different uid's then
		 * it's a duplicate.
		 */
		switch (tbl->type) {
		    case DB_GROUP_TBL:
			table = "group.byname";
			key = *oargs[0];
			status = yp_match(domain, table, key,
			    strlen(key), &buf, &vallen);
			if (buf == NULL)
				return (0);
			cp = strchr(buf, ':'); cp++; if (cp == NULL) return (0);
			cp = strchr(cp, ':'); cp++;  if (cp == NULL) return (0);
			cp2 = strchr(cp, ':'); if (cp2 == NULL) return (0);
			*cp2 = '\0';
			if (strcmp(cp, *oargs[2]) == 0) {
				free(buf);
				return(0);
			}
			free(buf);
			break;
		    case DB_PASSWD_TBL:
			table = "passwd.byname";
			key = *oargs[0];
			status = yp_match(domain, table, key,
			    strlen(key), &buf, &vallen);
			if (buf == NULL)
				return (0);
			cp = strchr(buf, ':'); cp++; if (cp == NULL) return (0);
			cp = strchr(cp, ':'); cp++;  if (cp == NULL) return (0);
			cp2 = strchr(cp, ':'); if (cp2 == NULL) return (0);
			*cp2 = '\0';
			if (strcmp(cp, *oargs[2]) == 0) {
				free(buf);
				return(0);
			}
			free(buf);
			break;
		    default:
			return (0);
		}

		db_err_set(db_err, DB_ERR_ENTRY_EXISTS, ADM_FAILCLEAN,
		    "set_nis_db", key, table);
		return (-1);
	} else {
		switch (tbl->type) {
		    case DB_PASSWD_TBL:
			table = tbl->tn.nis;
			key = *oargs[2];
			break;
		    case DB_NETWORKS_TBL:
			table = tbl->tn.nis;
			key = *oargs[1];
			break;
		    case DB_GROUP_TBL:
			table = "group.byname";
			key = *oargs[0];
			break;
		    case DB_HOSTS_TBL:
			table = "hosts.byname";
			key = *oargs[1];

			/* special handling for hosts to avoid returning DNS
			   entries.  Use yp_all instead of yp_match */

			if (key == NULL) {
			    return (0);
			} else {

			    YpKey                 keystruct;
			    struct ypall_callback callback;

			    keystruct.found  = 0;
			    keystruct.key    = key;
			    keystruct.keylen = strlen(key);

			    callback.foreach = _host_lookup;
			    callback.data    = (char*)&keystruct;

			    yp_all(domain, table, &callback);

			    if (keystruct.found != 0) {
				db_err_set(db_err, DB_ERR_ENTRY_EXISTS, 
					   ADM_FAILCLEAN,
					   "set_nis_db", key, table);
				return (-1);
			    } else {
				return (0);
			    }
			}
			    
			break;
		    case DB_ETHERS_TBL:
			table = "ethers.byaddr";
			key = *oargs[0];
			break;
		    default:
			return (0);
		}
		if (key == NULL)
			return (0);

		status = yp_match(domain, table, key, strlen(key), &buf, &vallen);
		if (buf != NULL)
			free(buf);
		if (status == 0) {
			db_err_set(db_err, DB_ERR_ENTRY_EXISTS, ADM_FAILCLEAN,
			   "set_nis_db", key, table);
			return (-1);
		}
	}
	return (0);
}

/*
 * These get appended below
 */
#define DEFAULT_ADMINPATH	"/opt/SUNWadm/2.1/"
#define	POLICY_MK_TARGET	"/etc/policy_mk_target"
#define	DEFAULT_POLICY_TEMPLATE	"/etc/policy.defaults"

int
_create_policy_table(
	Db_error **db_err,
	char *servername,
	char *server_mappath)
{
	char *cp, p_target[MAXPATHLEN], p_template[MAXPATHLEN];
	char pipecmd[MAXPATHLEN + BUFSIZ], inbuf[BUFSIZ];

	if ((cp = getenv("ADMINPATH")) != NULL) {
		(void) strcpy(p_target, cp); strcpy(p_template, cp);
	} else {
		(void) strcpy(p_target,   DEFAULT_ADMINPATH); 
		(void) strcpy(p_template, DEFAULT_ADMINPATH);
	}

	(void) strcat(p_target, POLICY_MK_TARGET);
	(void) strcat(p_template, DEFAULT_POLICY_TEMPLATE); 

	if (access(p_template, R_OK) < 0) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_POLICY_ACC), p_template);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    ADMLDB_MSGS(DB_ERR_NIS_POLICY_CRE), errmsg);
		return (-1);
	}

	if (access(p_target, R_OK) < 0) {
		(void) sprintf(errmsg,
		    ADMLDB_MSGS(DB_ERR_NIS_POLICY_MAKE), p_target);
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_nis_db",
		    ADMLDB_MSGS(DB_ERR_NIS_POLICY_CRE), errmsg);
		return (-1);
	}

	(void) sprintf(pipecmd,
	    "/bin/sed 's/=/	/' %s | "
	    "rsh -l root %s "
	    "   '/bin/cat > %s; "
	    "   /bin/chmod 644 %s'",
	    p_template, servername, server_mappath, server_mappath);
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);

	(void) sprintf(pipecmd, 
	    "/bin/cat %s | "
	    "rsh -l root %s "
	    "   '/bin/cat >> /var/yp/Makefile'",
	    p_target, servername);
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf,sizeof (inbuf)) < 0)
		return (-1);

	inbuf[0] = '\0';
	(void) sprintf(pipecmd,
	    "rsh -l root %s "
	    "   \"/bin/sh -c '"
	    "       PATH=/bin:/usr/ccs/bin; "
	    "       cd /var/yp; "
	    "       make %s 2>/dev/null 1>&2 || echo FAIL ; ' \"",
	    servername, basename(server_mappath));
	if (_popen(db_err, pipecmd, "set_nis_db", inbuf, sizeof (inbuf)) < 0)
		return (-1);
	if (strcmp(inbuf, "FAIL") == 0) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
		    "set_nis_db",
		    "make", ADMLDB_MSGS(DB_ERR_NIS_MAKE_FAILED));
		return (-1);
	}

	return (0);
}

int
shadow_map_exists(
	char **yp_error,
	char *domain)
{
	int	status, ret = 1;
	char	*servername;

	if ((domain == NULL) || !strlen(domain)) {
		if ((status = yp_get_default_domain(&domain)) != 0) {
			*yp_error = yperr_string(status);
			return (-1);
		}
	}

	status = yp_master(domain, "shadow.byname", &servername);
	if (status != 0) {
		if (status != YPERR_MAP) {
			*yp_error = yperr_string(status);
			return (-1);
		}
		ret = 0;
	} else
		free(servername);

	return (ret);
}
