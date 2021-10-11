/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)admldb_ufs.c	1.3	95/06/06 SMI"

#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <admldb.h>
#include <admldb_impl.h>
#include <admldb_msgs.h>
#include "admutil.h"
#include "nis_plus_ufs_policy.h"

/*
**  If a new table must be created we use this file as a template source
**  for obtaining ownership and permissions.
*/
#define TPLATE_SRC "/etc/passwd"

char *
ldb_get_db_line(
	char *buff,		/* Where to stuff the line */
	int buffsize,		/* Size of buffer */
	FILE *fp,		/* File to read from */
	struct tbl_trans_data *ttp)	/* Translation/format structure for table */
{
	char *cp = buff;
	int l;
	char *status;
	long pos = -1;
	
	while ((status = fgets(cp, (buffsize - (cp - buff)), fp)) != NULL) {
		if (ttp->type == DB_MAIL_ALIASES_TBL) {
			if (pos == -1) {
				pos = ftell(fp);
				cp += strlen(cp) - 1;
			} else {
				if (isspace(*cp))
					cp += strlen(cp) - 1;
				else {
					(void)fseek(fp, pos, SEEK_SET);
					strcpy(cp, "\n");
					break;
				}
			}		
		} else if (((l = strlen(cp)) < 2) || ((cp[l - 2]) != '\\'))
			break;
		else
			cp += l - 2;
	}
	if ((status == NULL) && (pos != -1)) {
		(void)fseek(fp, pos, SEEK_SET);
		strcpy(cp, "\n");
		status = cp;
	}
	return (status);
}

/*
 * Function to retrieve a token from a line; similar to strtok but considers
 * multiple consecutive non-whitespace separators to define empty columns
 * rather than being a single separator.  Also strips leading whitespace.
 */

#define	WHITESPACE	" \t\r\n\f\v"
#define	WHITESPACELEN	sizeof(WHITESPACE)

static char *
gettok(
	char *buff,
	char *span)
{
	static char *cp = "";
	char *rp;
	int len;

	if (buff != NULL)
		cp = buff;

	cp += strspn(cp, WHITESPACE);	/* Get past leading whitespace */
	if (*cp == '\0')
		return (NULL);

	len = strcspn(cp, span);	/* Find next separator */
	rp = cp;
	cp += len;
	if (*cp != '\0') {
		*cp = '\0';
		++cp;
	}
	return (rp);
}

int
_parse_db_buffer(
	char *buff,
	Row **rp,
	char *column_sep,
	char *comment_sep,
	struct tbl_fmt *tfp,
	ulong_t type)
{
	ushort_t colnum = 0;
	char *s, *t, *tok;
	char *cp;
	int status;
	int l;

	/*
	 * Don't try parsing empty lines
	 */
	if ((l = strlen(buff)) == 0)
		return (0);

	if ((*rp = new_row()) == NULL)
		return (-ENOMEM);

	/* 
	 * Duplicate for working copy and remove trailing newline
	 */
	cp = strdup(buff);
	if (cp[l - 1] == '\n')
		cp[l - 1] = '\0';
	
	/*
	 * Search for comment if a separator was given.
	 */
	if ((comment_sep != NULL) &&
	    ((t = strstr(cp, comment_sep)) != NULL)) {
		if ((tfp->comment_col >= 0) &&
		    (new_numbered_column(*rp, tfp->comment_col, 
		    (t + strlen(comment_sep)), NULL, DB_CASE_SENSITIVE) 
		    == NULL)) {
			free_row(*rp);
			return (-ENOMEM);
		}
		*t = '\0';
	}

	/*
	 * Now break out each column and stick it in a struct.
	 */
	for (tok = gettok(cp, column_sep); tok != NULL; 
	    tok = gettok(NULL, column_sep)) {
		if (new_numbered_column(*rp, colnum, tok, NULL, 
		    tfp->data_cols[colnum].case_flag)  == NULL) {	
			free_row(*rp);
			free(cp);
			return (-ENOMEM);
		}
		if (colnum != tfp->alias_col)
		        ++colnum;
		if (type == DB_MAIL_ALIASES_TBL || type == DB_AUTO_HOME_TBL) {
			tok = gettok(NULL, "\n");
			if (new_numbered_column(*rp, colnum, tok, NULL,
			    tfp->data_cols[colnum].case_flag) == NULL) {
			    	free_row(*rp);
			    	free(cp);
			    	return (-ENOMEM);
			} else
				break;
		}
	}

	free(cp);
	return (colnum);
}

/*
 * Function to do a match on two rows.  Each column in searchp is 
 * allowed to match a range of columns in entryp.  If each column in searchp
 * which has a match_val specified matches with a column in entryp from the 
 * range specified, then the match is an EXACT_MATCH.  If any columns which have
 * a match_val specified don't match, while others do, then it is a MIX_MATCH.
 * Failure to match any columns is a NO_MATCH.
 */
int
_match_entry(
	Row *searchp, 
	Row *entryp,
	struct tbl_fmt *tfp)
{
	Column *sp, *ep;
	int ret = 0;
	int status;
	int (*fn)();
	extern int strcasecmp();
	char *tok;
	char *cp;
	
	for (sp = searchp->start; sp != NULL; sp = sp->next) {
		if (sp->up->match_val != NULL) {
#ifdef DEBUG
printf("Looking to match %s\n", sp->up->match_val);
fflush(stdout);
#endif
			for (ep = entryp->start; ep != NULL; ep = ep->next) {
#ifdef DEBUG
printf("Col: %d\tVal: %s\n", ep->up->num, ep->up->match_val);
fflush(stdout);
#endif
				if (((int)ep->up->num >= 
				    tfp->data_cols[sp->up->num].first_match) && 
				    ((int)ep->up->num <= 
				    tfp->data_cols[sp->up->num].last_match)) {
					if (tfp->data_cols[sp->up->num].case_flag ==
					    DB_CASE_INSENSITIVE)
					    	fn = strcasecmp;
					else
						fn = strcmp;
					if (ep->up->num == tfp->alias_col) {
						cp = strdup(ep->val);
						for (tok = gettok(cp, 
						    DEFAULT_COLUMN_SEP);
						    tok != NULL; 
						    tok = gettok(NULL, 
						    DEFAULT_COLUMN_SEP))
						    	if ((status = 
						    	    (*fn)(sp->up->match_val, tok)) 
						    	    == 0)
								break;
						free(cp);
					} else
						status = (*fn)(sp->up->match_val, ep->val);
					if (status == 0) {
						sp->up->match_flag = 1;
						ret |= COL_MATCHED;
						break;
					}
				}
			}
			if (ep == NULL) {
				ret |= COL_NOT_MATCHED;
				break;
			}
		}
	}

	return (ret);
}

/*
 * Function to construct a table entry from the data arg list.
 */
int
format_entry(
	char *buff,
	int buffsize,
	char ***oa,
	struct tbl_trans_data *ttp)
{
	char *cp = buff;
	int len;
	int i, j;
	char *sp;
	
	buff[0] = '\0';
	for (i = 0; i < ttp->fmts[DB_UFS_INDEX].cnt; ++i) {
		if (!strcmp(ttp->fmts[DB_UFS_INDEX].data_cols[i].param, DB_NULL_PAR)) {
			if (i != 0)
				*cp++ = ttp->tn.column_sep[0];
			*cp++ = 'x';
			continue;
		}
		if ((ttp->type == DB_SERVICES_TBL) && (i == 1)) {
			if ((sp = (char *)malloc((strlen(*oa[1]) + strlen(*oa[2]) + 2)))
			    == NULL)
			    	return (-1);
			sprintf(sp, "%s/%s", *oa[1], *oa[2]);
			*oa[1] = sp;
			*oa[2] = NULL;
		}
		for (j = 0; j < ttp->io_args.cnt; ++j) {
			if (!strcmp(ttp->fmts[DB_UFS_INDEX].data_cols[i].param,
			    ttp->io_args.at[j].name)) {
				if ((oa[j] != NULL) && (*oa[j] != NULL) &&
				    ((len = strlen(*oa[j])) != 0)) {
				    	if (i == ttp->fmts[DB_UFS_INDEX].comment_col) {
				    		if ((i != 0) &&
				    		    isspace(ttp->tn.column_sep[0])) {
				    			*cp++ = ttp->tn.column_sep[0];
						}
						*cp++ = ttp->tn.comment_sep[0];
					} else if (i != 0)
						*cp++ = ttp->tn.column_sep[0];
					if ((cp + len) < (buff + buffsize)) {
						strcpy(cp, *oa[j]);
						cp += len;
					} else {
						return (-1);
					}
				} else if ((i != 0) && !isspace(ttp->tn.column_sep[0]) &&
				    (ttp->type != DB_MAIL_ALIASES_TBL))
					*cp++ = ttp->tn.column_sep[0];
				break;
			}
		}
		if ((i != 0) && (j == ttp->io_args.cnt) && 
		    !isspace(ttp->tn.column_sep[0]))
			*cp++ = ttp->tn.column_sep[0];

	}
/*
	for (i = 0; i < (ttp->type == DB_PASSWD_TBL ? 7 : ttp->io_args.cnt); ++i) {
		if (i != 0)
			*cp++ = ttp->tn.column_sep[0];
		if ((ttp->type == DB_PASSWD_TBL) && (i == 1)) {
			*cp++ = 'x';
			continue;
		}
		if ((ttp->type == DB_SERVICES_TBL) && (i == 1)) {
			if ((sp = (char *)malloc((strlen(*oa[1]) + strlen(*oa[2]) + 2)))
			    == NULL)
			    	return (-1);
			sprintf(sp, "%s/%s", *oa[1], *oa[2]);
			*oa[1] = sp;
			*oa[2] = NULL;
		}
		if ((oa[i] != NULL) && (*oa[i] != NULL) &&
		    ((len = strlen(*oa[i])) != 0)) {
			if ((ttp->tn.comment_sep != NULL) && 
			    (!strcmp(ttp->io_args.at[i].name, DB_COMMENT_PAR)))
				*cp++ = ttp->tn.comment_sep[0];
			if ((cp + len) < (buff + buffsize)) {
				strcpy(cp, *oa[i]);
				cp += len;
			} else {
				return (-1);
			}
		}
	}
*/
	*cp++ = '\n';
	*cp = '\0';
	return (0);
}

int
compare_ufs_col0(
	Row **ra,
	Row **rb)
{
	Column *ca, *cb;

	if ((ca = column_num_in_row(*ra, 0)) == NULL)
	        return (-1);
	if ((cb = column_num_in_row(*rb, 0)) == NULL)
	        return (1);
	if (ca->up->case_flag == DB_CASE_INSENSITIVE)
		return (strcasecmp(ca->val, cb->val));
	else
		return (strcmp(ca->val, cb->val));
}

int
compare_ufs_col1(
	Row **ra,
	Row **rb)
{
	Column *ca, *cb;

	if ((ca = column_num_in_row(*ra, 1)) == NULL)
	        return (-1);
	if ((cb = column_num_in_row(*rb, 1)) == NULL)
	        return (1);
	if (ca->up->case_flag == DB_CASE_INSENSITIVE)
		return (strcasecmp(ca->val, cb->val));
	else
		return (strcmp(ca->val, cb->val));
}

/*
 * Function to list an entire database. 
 */
int
list_ufs_db(
	ulong_t flags,
	Db_error **db_err,
	Table *tbl,
	char **iargs,
	char ***oargs,
	struct tbl_trans_data *ttp)
{
	FILE *ifp;
	char buff[2048];
	Row *rowp = NULL, **rlp = NULL, *mr = NULL;
	int i, cn;
	int rows = 0, rl_size = 500;
	int status = 0;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */
	int found = 0;
	Column *cp;
	char *sp = NULL;


	if ((!(flags & DB_DISABLE_LOCKING) && 
	    (lock_db(tbl->tn.ufs, F_RDLCK, &fd) == -1))) {
		db_err_set(db_err, DB_ERR_DB_BUSY, ADM_FAILCLEAN, 
		    "list_ufs_db", tbl->tn.ufs);
		return (-1);
	}

	/*
	 * Process file, line at a time.
	 */
	if ((ifp = fopen(tbl->tn.ufs, "r")) != NULL) {
		if ((flags & DB_LIST_SINGLE)) {
			if ((mr = new_row()) == NULL) {
				db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
				    "list_ufs_db");
				(void) fclose(ifp);
				(void) unlock_db(&fd);
				return (-1);
			}
			for (i = 0; i < ttp->match_args.cnt; ++i) {
				if ((iargs[i] == NULL) || !strlen(iargs[i]))
					continue;
				if ((tbl->type == DB_SERVICES_TBL) &&
				    (i == 1) && (iargs[i] != NULL) && 
				    (iargs[i+1] != NULL)) {
					if ((sp = (char *)malloc((strlen(iargs[i]) + 
					    strlen(iargs[i+1]) + 2))) == NULL) {
						db_err_set(db_err, 
						    DB_ERR_NO_MEMORY, 
						    ADM_FAILCLEAN,
						    "list_ufs_db");
						(void) fclose(ifp);
						(void) unlock_db(&fd);
						free_row(mr);
						return(-1);
					}
					sprintf(sp, "%s/%s", iargs[i], 
					    iargs[i+1]);
					iargs[i] = sp;
					iargs[i+1] = NULL;
				}
				cn = ttp->match_args.at[i].colnum[DB_UFS_INDEX];
				if (new_numbered_column(mr, cn, NULL,
				    iargs[i], 
				    ttp->fmts[DB_UFS_INDEX].data_cols[cn].case_flag) 
				    == NULL) {
					db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
					    "list_ufs_db");
					(void) fclose(ifp);
					(void) unlock_db(&fd);
					free_row(mr);
					return (-1);
				}
			}
		}
		while (ldb_get_db_line(buff, sizeof(buff), ifp, ttp) != NULL) {
			if ((status = _parse_db_buffer(buff, &rowp, 
			    ttp->tn.column_sep, ttp->tn.comment_sep, 
			    &ttp->fmts[DB_UFS_INDEX], ttp->type)) > 0) {
			    	if ((flags & DB_LIST_SINGLE)) {
			    		if (_match_entry(mr, rowp, 
			    		    &ttp->fmts[DB_UFS_INDEX]) == EXACT_MATCH) {
			    			found = 1;
			    			break;
			    		} else {
			    			free_row(rowp);
			    			continue;
			    		}
			    	}
				if ((rlp == NULL) || (rows == rl_size)) {
					/* Ran out of space, allocate more */
					rl_size = rl_size * 2;
					if ((rlp = (Row **) realloc(rlp,
					    (rl_size * sizeof(Row *)))) == NULL) {
						db_err_set(db_err, 
						    DB_ERR_NO_MEMORY, 
						    ADM_FAILCLEAN, 
						    "list_ufs_db");
						(void) fclose(ifp);
						(void) unlock_db(&fd);
						for (i = 0; i < rows; ++i)
						        free_row(rlp[i]);
						free(rlp);
						return (-1);
					}
				}
				rlp[rows++] = rowp;			        
			} else if (status < 0) {
			        db_err_set(db_err, DB_ERR_NO_MEMORY, 
				    ADM_FAILCLEAN, "list_ufs_db");
			        (void) fclose(ifp);
			        (void) unlock_db(&fd);
				for (i = 0; i < rows; ++i)
				        free_row(rlp[i]);
			        free(rlp);
			        return (-1);
			}
			status = 0;
		}
		if (ferror(ifp)) {
			db_err_set(db_err, DB_ERR_READ_ERROR, ADM_FAILCLEAN, 
			    "list_ufs_db", tbl->tn.ufs);
			(void) fclose(ifp);
			(void) unlock_db(&fd);
			for (i = 0; i < rows; ++i)
			        free_row(rlp[i]);
			free(rlp);
			return (-1);
		}
	} else { 
	    /*
	    ** Special case DB_POLICY_TBL since it gets created in the
	    ** read path.  All other tables are created in the write
	    ** path (SET_UFS_DB.  A missing table is treated as a value 
	    ** not found.
	    */
	    if ((ttp->type == DB_POLICY_TBL) || (errno != ENOENT)) {
	        db_err_set(db_err, DB_ERR_NO_TABLE, ADM_FAILCLEAN, 
	            "list_ufs_db", tbl->tn.ufs);
		(void) unlock_db(&fd);
		return (-1);
	    }
	}

	(void) fclose(ifp);
	(void) unlock_db(&fd);
	
	if ((flags & DB_LIST_SINGLE)) {
		if (found) {
			for (cp = rowp->start; cp != NULL; cp = cp->next) {
				if ((tbl->type == DB_SERVICES_TBL) &&
				    (cp->up->num == 1) &&
				    ((sp = strchr(cp->val, '/')) != NULL))
					*sp++ = '\0';
				for (i = 0; i < ttp->io_args.cnt; ++i)
					if ((oargs[i] != NULL) &&					    !strcmp(ttp->fmts[DB_UFS_INDEX].data_cols[cp->up->num].param, ttp->io_args.at[i].name)) 
					    	*oargs[i] = strdup(cp->val);
			}
			if ((sp != NULL) && (oargs[2] != NULL))
				*oargs[2] = strdup(sp);
			free_row(rowp);
			free_row(mr);
			return (0);
		} else {
			for (i = 0; i < ttp->match_args.cnt; ++i)
				if ((iargs[i] != NULL) && strlen(iargs[i]))
					break;
			db_err_set(db_err, DB_ERR_NO_ENTRY, ADM_FAILCLEAN, "list_ufs_db",
			    iargs[i], ttp->tn.ufs);
			free_row(mr);
			return (-1);
		}
	}
	if ((flags & DB_SORT_LIST))
	        qsort(rlp, rows, sizeof(Row *), 
		    ttp->fmts[DB_UFS_INDEX].sort_function);
	for (i = 0; i < rows; ++i)
		if (append_row(tbl, rlp[i]) != 0) {
			db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN, 
			    "list_ufs_db");
			for (i = 0; i < rows; ++i)
			        free_row(rlp[i]);
			free(rlp);
			free_tdh(tbl);
			return (-1);
		}
	free(rlp);
	return (0);
}

/*
 * Macro to do error cleanup of open files in set_ufs_db
 */
#define	cleanup_files {							\
	(void) fclose(ifp);						\
	(void) unlock_db(&fd);						\
	(void) fclose(ofp);						\
	(void) unlink(tmp);						\
	free(tmp);							\
	free_row(mr);							\
}

/*
 * Macro to do the buffer writes in set_ufs_db/set_groups_ufs
 */

#define	write_buff {							\
	if (fputs(buff, ofp) == EOF) {					\
		db_err_set(db_err, DB_ERR_WRITE_ERROR, ADM_FAILCLEAN,	\
		    FUNCTION, db);					\
		cleanup_files;						\
		return (-1);						\
	}								\
}

/*
 * Function to add/replace an entry in a table.
 */
int
set_ufs_db(
	ulong_t flags,
	Db_error **db_err,
	Table *tbl,
	char **iargs,
	char ***oargs,
	int action,
	struct tbl_trans_data *ttp)
{

#define	FUNCTION	"set_ufs_db"

	FILE *ifp, *ofp;
	char *tmpdir, *tmp;
	char buff[2048];
	Row *rp = NULL, *mr = NULL;
	Column *cp;
	int replaced = 0;
	int status, serrno;
	int i, cn;
	char *tdb, *db;
	struct stat sb;
	int nis_entry_seen = 0;
	long cur_pos, nis_pos, this_pos, save_pos;
	char *sp;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tdb = db = tbl->tn.ufs;
	if (trav_link(&tdb) == -1) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
		    "set_ufs_db", "readlink", strerror(errno));
		return (-1);
	}
	tmpdir = strdup(tdb);
	remove_component(tmpdir);
	if (strlen(tmpdir) == 0) 
		strcat(tmpdir, ".");
	tmp = tempfile(tmpdir);
	free(tmpdir);
	if ((ofp = fopen(tmp, "w")) == NULL) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_ufs_db",
		    "fopen", strerror(errno));
		free(tmp);
		return (-1);
	}
	(void) setbuf(ofp, NULL);	/* Make stream unbuffered */
	
	/*
	 * Preserve permissions of current file if it exists; otherwise it's
	 * up to the caller to set umask or do a chmod later.
	 */
	if ((status = stat(tdb, &sb)) == 0) {
		if (fchmod(fileno(ofp), sb.st_mode) == -1) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
			    "set_ufs_db", "fchmod", strerror(errno));
			(void) fclose(ofp);
			(void) unlink(tmp);
			free(tmp);
			return (-1);
		}
		if (fchown(fileno(ofp), sb.st_uid, sb.st_gid) == -1) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
			    "set_ufs_db", "fchown", strerror(errno));
			(void) fclose(ofp);
			(void) unlink(tmp);
			free(tmp);
			return (-1);
		}	        
	} else if (errno != ENOENT) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
		    "set_ufs_db", "stat", strerror(errno));
		(void) fclose(ofp);
		(void) unlink(tmp);
		free(tmp);
		return (-1);
	}
	if (!(flags & DB_DISABLE_LOCKING) &&
	    (lock_db(db, F_WRLCK, &fd) == -1)) {
		db_err_set(db_err, DB_ERR_DB_BUSY, ADM_FAILCLEAN, 
		    "set_ufs_db", tbl->tn.ufs);
		(void) fclose(ofp);
		(void) unlink(tmp);
		free(tmp);
		return (-1);
	}

	/*
	 *  If the db does not exist create it.  
	 */
	if (access(db, F_OK)  != 0) {
	    /*   Do special processing for /etc/Policy_defaults.  If it does
	     *   not exist, copy the fallback file from 
	     *   /opt/SUNWadm/version/usr/snadm/etc/policy.defaults
	     */
	    if (ttp->type == DB_POLICY_TBL) {
	        install_fallback_file(db);

	    } else {
	        /*
		**  obtain permissions and ownership from an existing
		**  database.  Use /etc/passwd since it must always
		**  exist.
		*/
	        if ((status = stat(TPLATE_SRC, &sb)) == 0) {
		    int cfd;

		    
		    if (cfd = creat(db, sb.st_mode) == -1) {
		        db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
			     "set_ufs_db", "creat", strerror(errno));
			return (-1);
		    }

		    if (fchown(cfd, sb.st_uid, sb.st_gid) == -1) {
		        db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, 
				   "set_ufs_db", "fchown", strerror(errno));
			(void) close(cfd);
			return (-1);
		    }	        

		    (void) close(cfd);

		}

            }
	}

	/*
	 * Process file, line at a time.  When we know that we've done the
	 * replacement, just pass the rest of the data through.
	 */
	if ((ifp = fopen(db, "r+")) != NULL) {
		if ((mr = new_row()) == NULL) {
			db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
			    "list_ufs_db");
			cleanup_files;
			return (-1);
		}
		for (i = 0; i < ttp->match_args.cnt; ++i) {
			if ((iargs[i] == NULL) || !strlen(iargs[i]))
				continue;
			if ((tbl->type == DB_SERVICES_TBL) &&
			    (i == 1) && (iargs[i] != NULL) && (iargs[i+1] != NULL)) {
				if ((sp = (char *)malloc((strlen(iargs[i]) + 
				    strlen(iargs[i+1]) + 2))) == NULL) {
					db_err_set(db_err, 
					    DB_ERR_NO_MEMORY, 
					    ADM_FAILCLEAN,
					    "list_ufs_db");
					cleanup_files;
					return(-1);
				}
				sprintf(sp, "%s/%s", iargs[i], 
				    iargs[i+1]);
				iargs[i] = sp;
				iargs[i+1] = NULL;
			}
			cn = ttp->match_args.at[i].colnum[DB_UFS_INDEX];
			if (new_numbered_column(mr, cn, NULL,
			    iargs[i], 
			    ttp->fmts[DB_UFS_INDEX].data_cols[cn].case_flag) 
			    == NULL) {
				db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
				    "list_ufs_db");
				cleanup_files;
				return (-1);
			}
		}
		for (cur_pos = 0; 
		    ldb_get_db_line(buff, sizeof(buff), ifp, ttp) != NULL;
		    cur_pos = ftell(ifp)) {
			if (replaced) {
				write_buff;
				continue;
			}
			if (tbl->tn.yp_compat && !nis_entry_seen)
				if (strchr("+-", buff[0]) != NULL) {
					nis_pos = cur_pos;
					nis_entry_seen = 1;
				}
			if ((status = _parse_db_buffer(buff, &rp, ttp->tn.column_sep,
			    ttp->tn.comment_sep, &ttp->fmts[DB_UFS_INDEX], ttp->type)) < 0) {
			    	db_err_set(db_err, DB_ERR_NO_MEMORY, ADM_FAILCLEAN,
			    	    "set_ufs_db");
				cleanup_files;
				return (-1);
			} else if ((status == 0) && !nis_entry_seen) {
				write_buff;
				continue;
			}
			
			/*
			 * We only do the overwrite if this is an exact match.  We
			 * only do the add if no columns matched.
			 */
			if ((status = _match_entry(mr, rp, &ttp->fmts[DB_UFS_INDEX])) 
			    == EXACT_MATCH) {
				if ((action == DB_REMOVE) || (flags & DB_MODIFY)) {
				        /*
				         * If this DB has the YP '+' convention and
				         * we have stopped copying because we saw one,
				         * it's time to backup & copy the stuff we
				         * "buffered" before writing this entry.
				         */
					if (nis_entry_seen) {
						save_pos = ftell(ifp);
						for (serrno = fseek(ifp, nis_pos, SEEK_SET),
						     this_pos = nis_pos;
						     (serrno == 0) &&
						     (this_pos != cur_pos) &&
						     (ldb_get_db_line(buff, sizeof(buff), ifp, ttp)
						      != NULL);
						     this_pos = ftell(ifp))
							write_buff;
						if ((serrno != 0) ||
						    (this_pos != cur_pos)) {
						    	db_err_set(db_err, 
						    	    DB_ERR_READ_ERROR,
						    	    ADM_FAILCLEAN,
						    	    "set_ufs_db",
						    	    tbl->tn.ufs);
							cleanup_files;
							return (-1);
						} else {
							nis_entry_seen = 0;
							if (fseek(ifp, save_pos, SEEK_SET) 
							    != 0) {
								db_err_set(db_err, 
								    DB_ERR_READ_ERROR,
								    ADM_FAILCLEAN,
								    "set_ufs_db",
								    tbl->tn.ufs);
							    cleanup_files;
							    return (-1);
							}
						}
					}
					if (action == DB_SET) {
						if (format_entry(buff, sizeof(buff),
						    oargs, ttp) != 0) {
							    db_err_set(db_err, 
							        DB_ERR_TOO_BIG,
								ADM_FAILCLEAN, 
								"set_ufs_db");
							cleanup_files;
							free_row(rp);
							return (-1);
						}
						write_buff;
					}
					replaced = 1;
					free_row(rp);
				} else {
					for (cp = mr->start; cp != NULL; cp = cp->next)
						if (cp->up->match_flag)
							break;
					db_err_set(db_err, DB_ERR_ENTRY_EXISTS,
					    ADM_FAILCLEAN, "set_ufs_db", 
					    cp->up->match_val, db);
					cleanup_files;
					free_row(rp);
					return (-1);
				}
			} else if ((action == DB_SET) && (status == MIX_MATCH)) {
				for (cp = mr->start; cp != NULL; cp = cp->next)
					if (cp->up->match_flag)
						break;
				db_err_set(db_err, DB_ERR_ENTRY_EXISTS,
				    ADM_FAILCLEAN, "set_ufs_db", 
				    cp->up->match_val, db);
				cleanup_files;
				free_row(rp);
				return (-1);
			} else if (!nis_entry_seen) {
				write_buff;
				free_row(rp);
			}
		}
	} else if (errno != ENOENT || ttp->type == DB_POLICY_TBL) {
		db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN, "set_ufs_db",
		    "fopen", strerror(errno));
		(void) unlock_db(&fd);
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (-1);
	}
	/*
	 * Didn't find what we were looking for, so add the entry at the end. 
	 */
	if (!replaced && (action == DB_SET) && (flags & DB_ADD)) {
		if (format_entry(buff, sizeof(buff), oargs, ttp) != 0) {
			    db_err_set(db_err, DB_ERR_TOO_BIG,
				ADM_FAILCLEAN, "set_ufs_db");
			cleanup_files;
			return (-1);
		}
		write_buff;
		replaced = 1;
	}
	/*
	 * If we paused copying because of NIS, rewind and bulk copy the remaining
	 * entries.
	 */
	if (replaced && nis_entry_seen) {
		if (fseek(ifp, nis_pos, SEEK_SET) != 0) {
		    	db_err_set(db_err, DB_ERR_READ_ERROR,
		    	    ADM_FAILCLEAN, "set_ufs_db", tbl->tn.ufs);
			cleanup_files;
			return (-1);
		}
		while ((status = fread(buff, sizeof(char), sizeof(buff), ifp)) > 0)
			if (fwrite(buff, sizeof(char), status, ofp) != status)
				break;
		if (!feof(ifp) && ferror(ifp)) {
		    	db_err_set(db_err, DB_ERR_READ_ERROR,
		    	    ADM_FAILCLEAN, "set_ufs_db", tbl->tn.ufs);
			cleanup_files;
			return (-1);
		} else if (ferror(ofp)) {
			db_err_set(db_err, DB_ERR_WRITE_ERROR, ADM_FAILCLEAN,
			    "set_ufs_db", db);
			cleanup_files;
			return (-1);
		}
	}

	(void) fclose(ifp);
	(void) fsync(fileno(ofp));
	(void) fclose(ofp);

	if (replaced) {
		if (rename(tmp, tdb) != 0) {
			db_err_set(db_err, DB_ERR_IN_LIB, ADM_FAILCLEAN,
			    "set_ufs_db", "rename", strerror(errno));
			(void) unlock_db(&fd);
			(void) unlink(tmp);
			free(tmp);
			free_row(mr);
			return (-1);
		} else {
			(void) unlock_db(&fd);
			free(tmp);
			free_row(mr);
			return (0);
		}
	} else {
		(void) unlock_db(&fd);
		(void) unlink(tmp);
		(void) free(tmp);
		for (i = 0; i < ttp->match_args.cnt; ++i)
			if (iargs[i] != NULL)
				break;
		db_err_set(db_err, DB_ERR_NO_ENTRY, ADM_FAILCLEAN,
		    "set_ufs_db", iargs[i], tbl->tn.ufs);
		free_row(mr);
		return (-1);
	}
}
