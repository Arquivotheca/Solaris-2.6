/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)db_entry.c	1.33	96/08/01 SMI"

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "db_entry.h"

#define	LCK_WAIT_TIME	2	/* Wait 2 seconds for locks */
#define	PASSWD_TABLE	"/etc/passwd"
#define	SHADOW_TABLE	"/etc/shadow"

int locking_disabled = 0;
int recognize_plus = 0;

void 
remove_component(char *path)
{
	char *p;

	p = strrchr(path, '/'); 		/* find last '/' 	*/
	if (p == NULL) {
		*path = '\0';			/* set path to null str	*/
	} else {
		*p = '\0';			/* zap it 		*/
	}
}

char *
basename(char *path)
{
	char *p;

	p = strrchr(path, '/');
	if (p == NULL) 
		p = path;
	else
		++p;
	return (p);
}

/*
 * Function to traverse a symlink path to find the real file at the end of
 * the rainbow.
 */
int
trav_link(char **path)
{
	static char newpath[MAXPATHLEN];
	char lastpath[MAXPATHLEN];
	int len;
	char *tp;

	strcpy(lastpath, *path);
	while ((len = readlink(*path, newpath, sizeof(newpath))) != -1) {
		newpath[len] = '\0';
		if (newpath[0] != '/') {
			tp = strdup(newpath);
			remove_component(lastpath);
			sprintf(newpath, "%s/%s", lastpath, tp);
			free(tp);
		}
		strcpy(lastpath, newpath);
		*path = newpath;
	}

	/*
	 * ENOENT or EINVAL is the normal exit case of the above loop.
	 */
	if ((errno == ENOENT) || (errno == EINVAL))
		return (0);
	else
		return (-1);
}

/*
 * Function to generate a temporary pathname to use.  based on tempnam(3).
 */

static char *seed="AAA";

char *
tempfile(const char *dir)
{
	char *buf, *cp;

	buf = (char *) malloc(MAXPATHLEN);
	strcpy(buf, dir);
	(void) strcat(buf, "/");
	(void) strcat(buf, seed);
	(void) strcat(buf, "XXXXXX");
	for (cp = seed; *cp == 'Z'; *cp++ = 'A');
	if (*cp != '\0')
		++* cp;
	if (*mktemp(buf) == '\0')
		return (NULL);
	return (buf);
}

static void
almhdlr(int sig)
{
}

/*
 * Function to lock a database against conflicting accesses.  Based on 
 * lckpwdf().
 */
int
lock_db(
	char *db, 
	int type,
	int *fdp)
{
	int status;
	char *lock_path;
	char *cp;
	struct flock flock;
	char *bp;
	mode_t mode, cmask = 0;
	
	if (locking_disabled)
	        return (0);

	lock_path = (char *) malloc(strlen(db) + 7);
	/*
	 * Handle passwd/shadow files specially so that we interact with
	 * lckpwdf().
	 */
	if ((strcmp(db, PASSWD_TABLE) == 0) || 
	    (strcmp(db, SHADOW_TABLE) == 0)) {
		bp = "pwd";
		cp = strdup(db);
		remove_component(cp);
		mode = 0600;
	} else {
		cp = "/etc";
		bp = basename(db);
		mode = 0600;
	}

	sprintf(lock_path, "%s/.%s.lock", cp, bp);
	cmask = umask(cmask);
	*fdp = open(lock_path, (O_RDWR | O_CREAT | O_TRUNC), mode);
	cmask = umask(cmask);
	if (*fdp == -1) {
		free(lock_path);
		return (-1);
	} else {
		flock.l_type = type;
		flock.l_whence = flock.l_start = flock.l_len = 0;
		(void) sigset(SIGALRM, almhdlr);
		(void) alarm(LCK_WAIT_TIME);
		status = fcntl(*fdp, F_SETLKW, (int)&flock); 
		(void) alarm(0);
		(void) sigset(SIGALRM, SIG_DFL);
		free(lock_path);
		return (status);
	}
}

/*
 * Function to unlock database previously locked with lock_db
 */
int
unlock_db(int *fdp)
{
	struct flock flock;

	if (locking_disabled)
		return (0);

	if (*fdp == -1) 
		return (-1);
	else {
		flock.l_type = F_UNLCK;
		flock.l_whence = flock.l_start = flock.l_len = 0;
		(void) fcntl(*fdp, F_SETLK, (int)&flock);
		(void) close(*fdp);
		*fdp = -1;
		return (0);
	}
}	


int
get_env_var(
	char *db,		/* File variable is set in */
	char *vname,		/* Variable name */
	char *vval)		/* Variable value */
{
	FILE *ifp;
	char buff[1024];
	int status;
	char *sp;

	if ((ifp = fopen(db, "r")) == NULL)
		return (errno);
	
	while (fgets(buff, sizeof(buff), ifp) != NULL) {
		if (strncmp(buff, vname, strlen(vname)) == 0) {
		        if (*(sp = &buff[strlen(vname)]) == '=') {
				strcpy(vval, (sp+1));
				if ((sp = strchr(vval, ';')) != NULL)
				        *sp = '\0';
				else if ((sp = strchr(vval, '\n')) != NULL)
					*sp = '\0';
				(void) fclose(ifp);
				return (0);
			}
		}
	}

	(void) fclose(ifp);
	return (-1);
}
			
int
set_env_var(
	char *db,		/* File variable is set in */
	char *vname,		/* Variable name */
	char *vval)		/* Variable value */
{
	FILE *ifp, *ofp;	/* Input & output files */
	char *tmpdir, *tmp;	/* Temp file name and location */
	int serrno;		/* Saved errno for cleanup cases */
	char buff[1024];
	int replaced = 0;
	char *tdb;
	struct stat sb;
	int status;

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tdb = db;
	if (trav_link(&tdb) == -1)
		return (errno);
	tmpdir = strdup(tdb);
	remove_component(tmpdir);
	if (strlen(tmpdir) == 0) 
		strcat(tmpdir, ".");
	tmp = tempfile(tmpdir);
	(void) free(tmpdir);
	if ((ofp = fopen(tmp, "w")) == NULL) {
		serrno = errno;
		(void) free(tmp);
		return (serrno);
	}
	/*
	 * Preserve permissions of current file if it exists; otherwise it's
	 * up to the caller to set umask or do a chmod later.
	 */
	if ((status = stat(tdb, &sb)) == 0) {
		if (fchmod(ofp->_file, sb.st_mode) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}
		if (fchown(ofp->_file, sb.st_uid, sb.st_gid) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}	        
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}

	if ((ifp = fopen(db, "r+")) != NULL) {
		while (fgets(buff, sizeof(buff), ifp) != NULL) {
			if (!replaced && 
			    (strncmp(buff, vname, strlen(vname)) == 0) &&
			    (buff[strlen(vname)] == '=')) {
				sprintf(buff, "%s=%s\n", 
				    vname, vval, vname);
				replaced = 1;
			}
			if (fputs(buff, ofp) == EOF) {
				serrno = errno;
				(void) fclose(ofp);
				(void) fclose(ifp);
				(void) unlink(tmp);
				(void) free(tmp);
				return (serrno);
			}
		}

	(void) fclose(ifp);
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	if (!replaced && 
	    (fprintf(ofp, "%s=%s; export %s\n", vname, vval, vname) == EOF)) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	if (fsync(ofp->_file)) {
		serrno = errno;
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	(void) fclose(ofp);
	if (rename(tmp, tdb) != 0) {
		serrno = errno;
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	} else {
		(void) free(tmp);
		return (0);
	}
}


int 
replace_db(
	char *db,		/* File to act upon */
	char *newent)		/* New entry to place in file */
{
	FILE *ifp, *ofp;	/* Output file */
	char *tmpdir, *tmp;	/* Temp file name and location */
	int serrno;		/* Saved errno for cleanup cases */
	char *tdb;
	struct stat sb;
	int status;

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tdb = db;
	if (trav_link(&tdb) == -1)
		return (errno);
	tmpdir = strdup(tdb);
	remove_component(tmpdir);
	if (strlen(tmpdir) == 0) 
		strcat(tmpdir, ".");
	tmp = tempfile(tmpdir);
	(void) free(tmpdir);
	if ((ofp = fopen(tmp, "w")) == NULL) {
		(void) free(tmp);
		return (errno);
	}
	/*
	 * Preserve permissions of current file if it exists; otherwise it's
	 * up to the caller to set umask or do a chmod later.
	 */
	if ((status = stat(tdb, &sb)) == 0) {
		if (fchmod(ofp->_file, sb.st_mode) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}
		if (fchown(ofp->_file, sb.st_uid, sb.st_gid) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}	        
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}

	/* Quick check to make sure we have read & write rights to the file */
	if ((ifp = fopen(db, "r+")) != NULL)
		(void) fclose(ifp);
	else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}
		
	/*
	 * Write out the data, close and then rename to overwrite old file.
	 */
	if (fprintf(ofp, "%s\n",newent) == EOF) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	if (fsync(ofp->_file)) {
		serrno = errno;
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	(void) fclose(ofp);

	if (rename(tmp, tdb) != 0) {
		serrno = errno;
		(void) unlink(tmp);
		return (serrno);
	} else {
		(void) free(tmp);
		return (0);
	}
}

int 
read_db(
	char *db,	/* File to read */
	char *buffer,	/* Buffer to use */
	int buffsize)	/* Size of buffer */
{
	FILE *ifp;
	int serrno;
	char *cp;
	
	if ((ifp = fopen(db, "r")) == NULL) {
		serrno = errno;
		return (serrno);
	} else if (fgets(buffer, buffsize, ifp) == NULL) {
		serrno = errno;
		(void) fclose(ifp);
		return (serrno);
	} else {
		if ((cp = strchr(buffer, '\n')) != NULL)
			*cp = '\0';
		(void) fclose(ifp);
		return (0);
	}
}

static char *
get_db_line(
	char *buff,		/* Where to stuff the line */
	int buffsize,		/* Size of buffer */
	FILE *fp)		/* File to read from */
{
	char *cp = buff;
	int l;
	char *status;

	while ((status = fgets(cp, (buffsize - (cp - buff)), fp)) != NULL) {
#ifdef DEBUG
printf("Get: buffer is %s", cp);
fflush(stdout);
#endif
		if (((l = strlen(cp)) < 2) || ((cp[l - 2]) != '\\'))
			break;
		cp += l - 2;
	}
	return (status);
}

/*
 * Function to create a new column list and initialize the fields
 */

int
new_col_list(
	Col_list **listpp, 	/* Ptr to list ptr */
	char *col_sep,		/* The separator string for columns */
	char *comment_sep,	/* The separator string for comment */
	char *comment)		/* Comment */
{
	if (((*listpp = (Col_list *)malloc(sizeof(Col_list))) == NULL))
		return (-1);

	if ((((*listpp)->column_sep = col_sep) != NULL) &&
	    (((*listpp)->column_sep = strdup(col_sep)) == NULL)) {
		free(*listpp);
		return (-1);
	}

	if ((((*listpp)->comment_sep = comment_sep) != NULL) &&
	    (((*listpp)->comment_sep = strdup(comment_sep)) == NULL)) {
		free((*listpp)->column_sep);
		free(*listpp);
		return (-1);
	}

	if ((((*listpp)->comment = comment) == NULL) ||
	    (((*listpp)->comment = strdup(comment)) != NULL)) {
		(*listpp)->start = (*listpp)->end = NULL;
		return (0);
	} else
		return (-1);
}

/*
 * Function to free a column list.  Frees any columns attached to list.
 */
void 
free_col_list(Col_list **listpp)
{
	Col_list *lp = *listpp;
	Column *cp;

	if (lp != NULL) {
		while (lp->start != NULL) {
			cp = lp->start->next;
			free(lp->start->match_val);
			free(lp->start->replace_val);
			free(lp->start);
			lp->start = cp;
		}

		free(lp->comment);
		free(lp->comment_sep);
		free(lp->column_sep);
		free(*listpp);
		*listpp = NULL;
	}

	return;
}

/*
 * Function to add a column to a column list.  Columns are maintained in
 * sorted order by num.
 */
int
new_column(
	Col_list *listp,
	ushort_t num,
	ushort_t first_match,
	ushort_t last_match,
	char *match_val,
	char *replace_val,
	ushort_t case_flag)
{
	Column *cp, *tp;
	
	if ((cp = malloc(sizeof(Column))) == NULL)
		return (-1);
	cp->num = num;
	cp->first_match = first_match;
	cp->last_match = last_match;
	cp->case_flag = case_flag;
	cp->match_flag = 0;
	if (((cp->match_val = match_val) != NULL) &&
	    ((cp->match_val = strdup(match_val)) == NULL)) {
		free(cp);
		return (-1);
	}

	if (((cp->replace_val = replace_val) != NULL) &&
	    ((cp->replace_val = strdup(replace_val)) == NULL)) {
		free(cp->match_val);
		free(cp);
		return(-1);
	}
	cp->prev = NULL;
	cp->next = NULL;

	for (tp = listp->end; ((tp != NULL) && (tp->num > cp->num)); 
	    tp = tp->prev);

	if (tp == NULL) {
		cp->next = listp->start;
		listp->start = cp;
		if (listp->end == NULL)
			listp->end = cp;
		else
			cp->next->prev = cp;
	} else {
		cp->next = tp->next;
		tp->next = cp;
		cp->prev = tp;
		if (cp->next != NULL)
			cp->next->prev = cp;
		else
			listp->end = cp;
	}

	return (0);
}

/*
 * Function to free a column struct.
 */
static int
free_column(
	Col_list *listp, 
	ushort_t num)
{
	Column *cp;

	if ((cp = find_column(listp, num)) == NULL)
		return (-1);
	else {
		if (cp->prev == NULL)
			listp->start = cp->next;
		else
			cp->prev->next = cp->next;
		if (cp->next == NULL)
			listp->end = cp->prev;
		else
			cp->next->prev = cp->prev;
		free(cp->match_val);
		free(cp->replace_val);
		free(cp);
		return (0);
	}
}

/*
 * Function to locate column 'num'.
 */
Column *
find_column(
	Col_list *clp,
	ushort_t num)
{
	Column *cp;

	for (cp = clp->start; cp != NULL; cp = cp->next)
		if (cp->num == num)
			return (cp);
	return (NULL);
}

/*
 * Function to do a match on two column lists.  Each column in searchp is 
 * allowed to match a range of columns in entryp.  If each column in searchp
 * which has a match_val specified matches with a column in entryp from the 
 * range specified, then the match is an EXACT_MATCH.  If any columns which have
 * a match_val specified don't match, while others do, then it is a MIX_MATCH.
 * Failure to match any columns is a NO_MATCH.
 */
 
#define	COL_NOT_MATCHED	2
#define	COL_MATCHED	1
#define	EXACT_MATCH	COL_MATCHED
#define	MIX_MATCH	(COL_NOT_MATCHED | COL_MATCHED)
#define	NO_MATCH	0

int
match_entry(
	Col_list *searchp,
	Col_list *entryp)
{
	Column *sp, *ep;
	int ret = 0;
	int status;

	for (sp = searchp->start; sp != NULL; sp = sp->next) {
		if (sp->match_val != NULL) {
#ifdef DEBUG
printf("Looking to match %s\n", sp->match_val);
fflush(stdout);
#endif
			for (ep = entryp->start; ep != NULL; ep = ep->next) {
#ifdef DEBUG
printf("Col: %d\tVal: %s\n", ep->num, ep->match_val);
fflush(stdout);
#endif
				if ((ep->num >= sp->first_match) && 
				    (ep->num <= sp->last_match)) {
					if (sp->case_flag == DBE_CASE_INSENSITIVE)
						status = strcasecmp(sp->match_val,
								    ep->match_val);
					else
						status = strcmp(sp->match_val,
								ep->match_val);
					if (status == 0) {
						sp->match_flag = 1;
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
 * Function to retrieve a token from a line; similar to strtok but considers
 * multiple consecutive non-whitespace separators to define empty columns
 * rather than being a single separator.  Also strips leading whitespace.
 */

#define	WHITESPACE	" \t\r\n\f\v"
#define	WHITESPACELEN	sizeof(WHITESPACE)

char *
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

/*
 * Function to parse a line from a table and place it into a column list.
 */
int
parse_db_buffer(
	char *buff,
	char *col_sep,
	char *comment_sep,
	Col_list **clp)
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
#ifdef DEBUG
printf("Parse: buffer is %s", buff);
fflush(stdout);
#endif
	if ((status = new_col_list(clp, col_sep, comment_sep, NULL)) != 0)
		return (status);

	/* 
	 * Duplicate for working copy and then remove trailing newline left from fgets.
	 */
	cp = strdup(buff);
	if (cp[l - 1] == '\n')
		cp[l - 1] = '\0';
	
	/*
	 * Search for comment if a separator was given.
	 */
	if ((comment_sep != NULL) &&
	    ((t = strstr(cp, comment_sep)) != NULL)) {
		(*clp)->comment = strdup((t + strlen(comment_sep)));
		*t = '\0';
	}

	/*
	 * Now break out each column and stick it in a struct.
	 */
	for (tok = gettok(cp, col_sep); tok != NULL; 
	    tok = gettok(NULL, col_sep)) {
#ifdef DEBUG
printf("Parse: token = %s\n", tok);
fflush(stdout);
#endif
		if ((status = new_column(*clp, colnum, colnum, colnum, tok, 
		    NULL, DBE_CASE_SENSITIVE)) != 0) {	
			free_col_list(clp);
			free(cp);
			return (status);
		}
		++colnum;
	}

	free(cp);
	return (colnum);
}

/*
 * Function to construct a table entry from a column list.
 */
int
construct_db_buffer(
	char *buff,
	int buffsize,
	Col_list *listp)
{
	Column *colp;
	ushort_t next_col = 0;
	char *cp = buff;
	int len;

	buff[0] = '\0';
	for (colp = listp->start; colp != NULL; colp = colp->next) {
		while (next_col < colp->num) {
			*cp = listp->column_sep[0];
			++cp;
			++next_col;
		}
		if ((colp->replace_val != NULL) && 
		    ((len = strlen(colp->replace_val)) != 0)) {
			if ((cp + len) < (buff + buffsize)) {
				strcpy(cp, colp->replace_val);
				cp += len;
			} else
				return (-1);
		}
	}

	*cp = '\0';
	if ((listp->comment != NULL) && 
	    ((cp + strlen(listp->comment) + strlen(listp->comment_sep + 1) <
	      (buff + buffsize))))
		sprintf(cp, "\t%s%s", listp->comment_sep, listp->comment);
	strcat(buff, "\n");
	return (0);
}

/*
 * Function to find a requested entry in a table.
 */
int
get_db_entry(
	char *db,
	Col_list *listp,
	char *buff,
	int bufflen)
{
	FILE *fp;
	Column *tp;
	int found = 0;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */
	Col_list *clp = NULL;
	int status;

	if (lock_db(db, F_RDLCK, &fd) == -1)
		return (-ETXTBSY);
	if ((fp = fopen(db, "r")) == NULL) {
		(void) unlock_db(&fd);
		return (errno);
	}

	while (get_db_line(buff, bufflen, fp) != NULL) {
		if ((status = parse_db_buffer(buff, listp->column_sep,
		    listp->comment_sep, &clp)) > 0) {
			if (match_entry(listp, clp) == EXACT_MATCH) {
				found = 1;
				break;
			} else
				free_col_list(&clp);
		}
		else if (status < 0)
			break;
	}

	(void) fclose(fp);
	(void) unlock_db(&fd);

	if (found) {
		/*
		 * Copy the comment, null it out from the temp struct so
		 * it doesn't get freed from under us.  Then exchange list
		 * pointers so we free up mem the caller allocated.
		 */
		listp->comment = clp->comment;
		clp->comment = NULL;
		tp = listp->start;
		listp->start = clp->start;
		clp->start = tp;
		tp = listp->end;
		listp->end = clp->end;
		clp->end = tp;
		free_col_list(&clp);
	}
	return ((found - 1));
}

/*
 * Macro to do the buffer writes in replace_db_entry
 */

#define	write_buff {							\
	if (fputs(buff, ofp) == EOF) {					\
		serrno = errno;						\
		(void) fclose(ifp);					\
		(void) unlock_db(&fd);					\
		(void) fclose(ofp);					\
		(void) unlink(tmp);					\
		(void) free(tmp);					\
		return (serrno);					\
	}								\
}

/*
 * Macro to do error cleanup of open files in replace_db_entry
 */
#define	cleanup_files {							\
	(void) fclose(ifp);						\
	(void) unlock_db(&fd);						\
	(void) fclose(ofp);						\
	(void) unlink(tmp);						\
	(void) free(tmp);						\
}
/*
 * Function to add/replace an entry in a table.
 */
int
replace_db_entry(
	char *db,		/* Database to work on */
	Col_list *listp,	/* Column list representing entry */
	int flags)		/* Flag bits controlling routine behavior */
{
	FILE *ifp, *ofp;
	char *tmpdir, *tmp;
	char buff[2048];
	Col_list *entryp = NULL;
	int replaced = 0;
	int serrno;
	int status;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */
	char *tdb;
	struct stat sb;
	int nis_entry_seen = 0;
	long cur_pos, nis_pos, this_pos, save_pos;
	int ypcompat = (flags & DBE_YP_COMPAT);
	
	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tdb = db;
	if (trav_link(&tdb) == -1)
		return (errno);
	tmpdir = strdup(tdb);
	remove_component(tmpdir);
	if (strlen(tmpdir) == 0) 
		strcat(tmpdir, ".");
	tmp = tempfile(tmpdir);
	(void) free(tmpdir);
	if ((ofp = fopen(tmp, "w")) == NULL)
		return (errno);
	/*
	 * Preserve permissions of current file if it exists; otherwise it's
	 * up to the caller to set umask or do a chmod later.
	 */
	if ((status = stat(tdb, &sb)) == 0) {
		if (fchmod(ofp->_file, sb.st_mode) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}
		if (fchown(ofp->_file, sb.st_uid, sb.st_gid) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}	        
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}
	if (lock_db(db, F_WRLCK, &fd) == -1) {
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (-ETXTBSY);
	}

	/*
	 * Process file, line at a time.  When we know that we've done the
	 * replacement, just pass the rest of the data through.
	 */
	if ((ifp = fopen(db, "r+")) != NULL) {
		for (cur_pos = 0; 
		    get_db_line(buff, sizeof(buff), ifp) != NULL;
		    cur_pos = ftell(ifp)) {
			if (replaced) {
				write_buff;
				continue;
			}
			if (ypcompat && !nis_entry_seen)
				if (strchr("+-", buff[0]) != NULL) {
					nis_pos = cur_pos;
					nis_entry_seen = 1;
				}
			if ((status = parse_db_buffer(buff, listp->column_sep,
			    listp->comment_sep, &entryp)) < 0) {
				cleanup_files;
				return (-ENOMEM);
			} else if ((status == 0) && !nis_entry_seen) {
				write_buff;
				continue;
			}
			
			/*
			 * We only do the overwrite if this is an exact match.  We
			 * only do the add if no columns matched.
			 */
			if ((status = match_entry(listp, entryp)) == EXACT_MATCH) {
				if (flags & DBE_OVERWRITE) {
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
						     (get_db_line(buff, sizeof(buff), ifp)
						      != NULL);
						     this_pos = ftell(ifp))
							write_buff;
						if ((serrno != 0) ||
						    (this_pos != cur_pos)) {
							cleanup_files;
							return (errno);
						} else {
							nis_entry_seen = 0;
							ypcompat = 0;
							if (fseek(ifp, save_pos, SEEK_SET) 
							    != 0) {
								cleanup_files;
								return (errno);
							}
						}
					}
					if (construct_db_buffer(buff, sizeof(buff),
								listp) != 0) {
						cleanup_files;
						free_col_list(&entryp);
						return (-E2BIG);
					}
					write_buff;
					replaced = 1;
					free_col_list(&entryp);
				} else {
					cleanup_files;
					free_col_list(&entryp);
					return (-EEXIST);
				}
			} else if ((status == MIX_MATCH) && 
			           !(flags & DBE_IGNORE_MIX_MATCH)) {
				cleanup_files;
				free_col_list(&entryp);
				return (-EEXIST);
			} else if (!nis_entry_seen) {
				write_buff;
				free_col_list(&entryp);
			}
		}
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) unlock_db(&fd);
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}
	/*
	 * Didn't find what we were looking for, so add the entry at the end. 
	 */
	if (!replaced && (flags & DBE_ADD)) {
		if (construct_db_buffer(buff, sizeof(buff), listp) != 0) {
			cleanup_files;
			return (-E2BIG);
		}
		write_buff;
		replaced = 1;
	}
	/*
	 * If we paused copying because of NIS+, rewind and bulk copy the remaining
	 * entries.
	 */
	if (replaced && nis_entry_seen) {
		if (fseek(ifp, nis_pos, SEEK_SET) != 0) {
			serrno = errno;
			cleanup_files;
			return (serrno);
		}
		while ((status = fread(buff, sizeof(char), sizeof(buff), ifp)) > 0)
			if (fwrite(buff, sizeof(char), status, ofp) != status)
				break;
		if ((!feof(ifp) && ferror(ifp)) || ferror(ofp)) {
			cleanup_files;
			return (EIO);
		}
	}

	(void) fclose(ifp);

	if (fsync(ofp->_file)) {
		serrno = errno;
		(void) unlock_db(&fd);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	(void) fclose(ofp);

	if (replaced) {
		if (rename(tmp, tdb) != 0) {
			serrno = errno;
			(void) unlock_db(&fd);
			(void) unlink(tmp);
			(void) free(tmp);
			return (serrno);
		} else {
			(void) unlock_db(&fd);
			(void) free(tmp);
			return (0);
		}
	} else {
		(void) unlock_db(&fd);
		(void) unlink(tmp);
		(void) free(tmp);
		return (-ENOENT);
	}
}

/*
 * Function to remove an entry from a table.
 */
int
remove_db_entry(
	char *db,
	Col_list *listp)
{
	FILE *ifp, *ofp;
	char *tmpdir, *tmp;
	char buff[2048];
	Col_list *entryp = NULL;
	int replaced = 0;
	int serrno;
	int status;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */
	char *tdb;
	struct stat sb;

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tdb = db;
	if (trav_link(&tdb) == -1)
		return (errno);
	tmpdir = strdup(tdb);
	remove_component(tmpdir);
	if (strlen(tmpdir) == 0) 
		strcat(tmpdir, ".");
	tmp = tempfile(tmpdir);
	(void) free(tmpdir);
	if ((ofp = fopen(tmp, "w")) == NULL)
		return (errno);
	/*
	 * Preserve permissions of current file if it exists; otherwise it's
	 * up to the caller to set umask or do a chmod later.
	 */
	if ((status = stat(tdb, &sb)) == 0) {
		if (fchmod(ofp->_file, sb.st_mode) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}
		if (fchown(ofp->_file, sb.st_uid, sb.st_gid) == -1) {
			serrno = errno;
			(void) fclose(ofp);
			(void) unlink(tmp);
			return (serrno);
		}	        
	} else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}
	if (lock_db(db, F_WRLCK, &fd) == -1) {
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (-ETXTBSY);
	}

	/*
	 * Process file, line at a time.  When we know that we've done the
	 * replacement, just pass the rest of the data through.
	 */
	if ((ifp = fopen(db, "r+")) != NULL) {
		while (get_db_line(buff, sizeof(buff), ifp) != NULL) {
			status = 0;
			if (!replaced &&
			    ((status = parse_db_buffer(buff, listp->column_sep,
		    	      listp->comment_sep, &entryp)) > 0) && 
			    (match_entry(listp, entryp) == EXACT_MATCH))
				replaced = 1;
			else if (status < 0)
				break;
			else if (fputs(buff, ofp) == EOF) {
				serrno = errno;
				(void) fclose(ifp);
				(void) unlock_db(&fd);
				(void) fclose(ofp);
				(void) unlink(tmp);
				(void) free(tmp);
				free_col_list(&entryp);
				return (serrno);
			}

			free_col_list(&entryp);
		}
	} else {
		serrno = errno;
		(void) unlock_db(&fd);
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}
		

	(void) fclose(ifp);

	if (fsync(ofp->_file)) {
		serrno = errno;
		(void) unlock_db(&fd);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	(void) fclose(ofp);

	if (replaced) {
		serrno = status = rename(tmp, tdb);
		if (status != 0) {
			serrno = errno;
			(void) unlink(tmp);
		}
	} else {
		serrno = -ENOENT;
		(void) unlink(tmp);
	}

	(void) unlock_db(&fd);
	(void) free(tmp);
	return (serrno);
}

/*
 * Function to list an entire database.  Uses a callback scheme similar to
 * that of yp_all().
 */
int
list_db(
	char *db,
	char *column_sep,
	char *comment_sep,
	struct list_db_callback *cb)
{
	FILE *ifp;
	char buff[2048];
	Col_list *entryp = NULL;
	int status = 0;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */

	if (lock_db(db, F_RDLCK, &fd) == -1)
		return (-ETXTBSY);
	/*
	 * Process file, line at a time, calling the callback routine for each
	 * entry found.
	 */
	if ((ifp = fopen(db, "r")) != NULL) {
		while (get_db_line(buff, sizeof(buff), ifp) != NULL) {
			if ((status = parse_db_buffer(buff, column_sep,
		    	      comment_sep, &entryp)) > 0) {
				if ((status = (*cb->foreach)(status, entryp,
				    cb->data)) != 0) {
					free_col_list(&entryp);
					break;
				}
			} else if (status < 0) {
				status = -ENOMEM;
				break;
			}
			free_col_list(&entryp);
			status = 0;
		}
	} else {
		(void) unlock_db(&fd);
		return (errno);
	}
	
	(void) fclose(ifp);
	(void) unlock_db(&fd);
	return (status);
}
