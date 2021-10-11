#ifndef lint
#ident   "@(#)find_modified.c 1.26 95/05/31 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "sw_lib.h"

#include <sys/mkdev.h>
#include <limits.h>
#include <dirent.h>
#include <pkgstrct.h>
#include <utime.h>
#include <grp.h>
#include <pwd.h>
#include <fcntl.h>
#include <ctype.h>



/* Public Function Prototypes */

void    	find_modified(Module *);
void    	canoninplace(char *);

/* Local Function Prototypes */

static int	checkmap(char *, Product *);
static void	canonize(char *);
static void	mappath(int, char *);
static void	basepath(char *, char *);
static int	srchcfile(struct cfent *, char *path, FILE *, FILE *);
static int	getnum(FILE *, int, long *, long);
static int	getstr(FILE *, char *, int, char *);
static int	getend(FILE *);
static int	eatwhite(FILE *);
static void	copycanon(char *dst, char *src);
static unsigned docksum(char *path);
static int	gpkgmap(struct cfent *ept, FILE *fp);
static void	free_filediff(struct filediff *statp);
static int	to_be_ignored(Modinfo *mi, struct filediff *statp);
static int	contents_only(struct filediff *statp);

static struct filediff *ckentry(int, struct cfent *);
static struct filediff *getfilediff(char *);
static struct filediff *cverify(char *path, struct cinfo *cinfo,
    struct filediff *statp);
static struct filediff *averify(char *ftype, char *path, struct ainfo *ainfo);

static void   add_statp_entry(struct filediff *, Modinfo *);
static void   add_tbl_entry(char *, char *);
static void   build_pkg_arch_tbl(void);
static void   chain_match_component(struct cfent *, Modinfo *);
static void   chain_new_class(char *);
static void   check_statp_chain(void);
#ifdef DEBUG2
static int    dump_chain(char *);
static void   dump_close(void);
static int    dump_open(char *);
static int    dump_tbl(char *);
#endif
static void   free_classes(void);
static void   free_pkg_arch_tbl(void);
static void   free_pkg_info_list(struct pkg_info *);
static void   free_statp_chain(void);
static void   free_modified_lists(void);
static Module *get_new_media_head(void);
static int    in_classes(char *class);

#define	logerr	printf

void spin(int);


extern int	errno;
extern int	in_final_upgrade_stage;

static	char	basedir[] = "/";

#define	nxtentry(p) (srchcfile(p, "*", fp, NULL))
#define	RELATIVE(x)	(x[0] != '/')
#define	streq(s1, s2) (strcmp((s1), (s2)) == 0)

struct missing_file {
	struct missing_file *next;
	int    misslen;
	char   missing_file_name[2];
};

static struct missing_file *missing_file_list = NULL;

static struct cfent entry;
static char	*install_root;
static char *srcherrstr;

/*
 * This routine checks all files which are referenced
 * in the pkgmap which is identified by mapfile arg
 */

static char baseroot[MAXPATHLEN];
static char lastmissing[MAXPATHLEN];
static int length_missing = 0;

static struct class_chain {
	struct class_chain *next_class;
	char *class;
};

static struct class_chain *class_head = NULL;
static struct filediff *real_modified_list = NULL;
static struct filediff *real_modified_list_tail = NULL;
static struct filediff *statp_chain_head = NULL;
static struct filediff *statp_chain_tail = NULL;
static struct filediff *tentative_modified_list = NULL;
static struct pkg_info *tbl_head = NULL;

void
find_modified(Module *mod)
{
	struct filediff *ptr, *next_ptr;
	struct filediff **statpp;
	char	mapfile[MAXPATHLEN];
	int	n;

	lastmissing[0] = '\0';
	if (*get_rootdir() == '\0') {
		strcpy(baseroot, mod->sub->info.prod->p_rootdir);
	} else {
		strcpy(baseroot, get_rootdir());
		strcat(baseroot, mod->sub->info.prod->p_rootdir);
	}
	n = strlen(baseroot);
	if (baseroot[n - 1] == '/')
		baseroot[n - 1] = '\0';
	install_root = baseroot;
	sprintf(mapfile, "%s/var/sadm/install/contents", install_root);
	/*
	 * Make sure these data structures are empty and set to NULL
	 */
	free_statp_chain();
	free_pkg_arch_tbl();
	free_classes();
	free_modified_lists();

	checkmap(mapfile, mod->sub->info.prod);
	/*
	 * code to parcel out everything currently on
	 * the real_modified_list to the owning packages.
	 */
	/*LINTED [var set before used]*/
	for (ptr = real_modified_list; ptr != NULL; ptr = next_ptr) {
		next_ptr = ptr->diff_next;

		statpp = &(ptr->owning_pkg->m_filediff);
		while (*statpp != (struct filediff *)NULL)
			statpp = &((*statpp)->diff_next);
		*statpp = ptr;
		ptr->diff_next = NULL;
	}

	real_modified_list = NULL;
	real_modified_list_tail = NULL;
	install_root = NULL;
}

static int
checkmap(mapfile, prod)
char	*mapfile;
Product *prod;
{
	FILE *fp;
	int	n, errflg;
	char 	*save_path = NULL;
	char	temppath[MAXPATHLEN];
	struct  filediff *statp;
	Node	*node;
	Modinfo *mi;
	char	pkgabbrev[16];
	char	*cp;
	struct  pinfo *pp;
#ifdef DEBUG2
	char	cpath[1024];
#endif

	if ((fp = fopen(mapfile, "r")) == NULL) {
		printf(gettext("Can't open contents file: %s"), mapfile);
		return (-1);
	}

	errflg = 0;
	while (n = nxtentry(&entry)) {

		spin(25);
		if (n < 0) {
			printf("garbled entry in contents file");
			printf("error = %s", srcherrstr);
			continue;
		}
		if (n == 0)
			break; /* done with file */

		if (install_root) {
			save_path = entry.path;
			(void) sprintf(temppath, "%s/%s", install_root,
				entry.path);
			entry.path = temppath;
		}
		if (((statp = ckentry(0, &entry))) != NULL) {
			if ((int)statp == -1)
				errflg++;
			else {
				for (pp = entry.pinfo; pp != NULL;
				    pp = pp->next) {
					strcpy(pkgabbrev, pp->pkg);
					cp = strchr(pkgabbrev, '.');
					if (cp)
						*cp = '\0';
					node = findnode(prod->p_packages,
					    pkgabbrev);
					if (node == NULL)
						continue;
					mi = (Modinfo *)(node->data);
					while (mi->m_shared == NULLPKG ||
					    mi->m_pkginst == NULL ||
					    !streq(mi->m_pkginst, pp->pkg)) {
						node = mi->m_instances;
						if (node == NULL)
							break;
						mi = (Modinfo *)(node->data);
					}
					statp->owning_pkg = mi;
					if (node != NULL)
						break;
				}
				if (pp == NULL) {
					free_filediff(statp);
					continue;
				}
#ifdef DEBUG2
				strcpy(cpath, statp->component_path);
#endif
				/*
				 * if component is on package's ignore
				 * list, skip it.
				 */
				if (mi->m_pkg_hist &&
				    mi->m_pkg_hist->ignore_list &&
				    to_be_ignored(mi, statp)) {
					free_filediff(statp);
					continue;
				}
				/*
				 * This statp entry is interesting,
				 * add it to the statp chain.
				 */
				add_statp_entry(statp, mi);
			}
		}
		if (install_root && save_path)
			entry.path = save_path;
	}
	/*
	 * Create a package/arch table with unique entries
	 * derived from the statp chain entries.
	 */
	build_pkg_arch_tbl();
	/*
	 * search pkgmap, for each entry in pkg tbl, for matches in the
	 * statp chain.  If a match is found take appropriate action.
	 */
	check_statp_chain();

	free_statp_chain();
	free_pkg_arch_tbl();

	(void) fclose(fp);
	return (0);
}

static struct filediff *
ckentry(envflag, ept)
int	envflag;
struct cfent	*ept;
{
	struct	filediff	*statp;
	struct  missing_file	*missp;
	int	bump;

	canonize(ept->path);
	if (strchr("sl", ept->ftype)) {
		if (!RELATIVE(ept->ainfo.local)) {
			if (envflag) {
				mappath(2, ept->ainfo.local);
				basepath(ept->ainfo.local, basedir);
			}
			canonize(ept->ainfo.local);
		}
	}

	statp = NULL;
	if (length_missing > 0 &&
	    strncmp(ept->path, lastmissing, length_missing) == 0) {
		return (statp);
	} else {
		lastmissing[0] = '\0';
		length_missing = 0;
	}

	if (ept->pinfo->status == '%')
		return (statp);
	if (!strchr("in", ept->ftype)) {
		/* validate attributes */
		statp = averify(&ept->ftype, ept->path, &ept->ainfo);
	}
	if (statp && statp->diff_flags & DIFF_MISSING) {
		strcpy(lastmissing, ept->path);
		strcat(lastmissing, "/");
		length_missing = strlen(lastmissing);
		if (strcmp(get_rootdir(), "/") == 0)
			bump = 0;
		else
			bump = strlen(get_rootdir());
		missp = (struct missing_file *)xcalloc(sizeof
		    (struct missing_file) + strlen(lastmissing) - bump + 1);
		strcat(missp->missing_file_name, lastmissing + bump);
		missp->misslen = length_missing - bump;
		missp->next = missing_file_list;
		missing_file_list = missp;
	}

	if (strchr("fev", ept->ftype) &&
	    (ept->ftype != 'v') &&
	    (ept->ftype != 'e')) {
		/* validate contents */
		return (cverify(ept->path, &ept->cinfo, statp));
	}
	return (statp);
}

#define	isdot(x)	((x[0] == '.') && (!x[1] || (x[1] == '/')))
#define	isdotdot(x)	((x[0] == '.') && (x[1] == '.') && (!x[2] || \
			    (x[2] == '/')))

static void
canonize(file)
char *file;
{
	char *pt, *last;
	int level;

	/* remove references such as './' and '../' and '//' */
	for (pt = file; *pt;) {
		if (isdot(pt))
			(void) strcpy(pt, pt[1] ? pt+2 : pt+1);
		else if (isdotdot(pt)) {
			level = 0;
			last = pt;
			do {
				level++;
				last += 2;
				if (*last)
					last++;
			} while (isdotdot(last));
			--pt; /* point to previous '/' */
			while (level--) {
				if (pt <= file)
					return;
				while ((*--pt != '/') && (pt > file))
					;
			}
			if (*pt == '/')
				pt++;
			(void) strcpy(pt, last);
		} else {
			while (*pt && (*pt != '/'))
				pt++;
			if (*pt == '/') {
				while (pt[1] == '/')
					(void) strcpy(pt, pt+1);
				pt++;
			}
		}
	}
	if ((--pt > file) && (*pt == '/'))
		*pt = '\0';
}

/* 0 = both upper and lower case */
/* 1 = lower case only */
/* 2 = upper case only */
#define	mode(flag, pt)	(!flag || ((flag == 1) && islower(pt[1])) ||\
			((flag == 2) && isupper(pt[1])))

static void
mappath(flag, path)
int flag;
char *path;
{
	char buffer[MAXPATHLEN];
	char varname[64];
	char *npt, *pt, *pt2, *copy;
	char *token;

	copy = buffer;
	for (pt = path; *pt;) {
		if ((*pt == '$') && isalpha(pt[1]) && mode(flag, pt) &&
		    ((pt == path) || (pt[-1] == '/'))) {
			pt2 = varname;
			for (npt = pt+1; *npt && (*npt != '/');)
				*pt2++ = *npt++;
			*pt2 = '\0';
			if ((token = getenv(varname)) != NULL && *token) {
				/* copy in parameter value */
				while (*token)
					*copy++ = *token++;
				pt = npt;
			} else
				*copy++ = *pt++;
		} else if (*pt == '/') {
			while (pt[1] == '/')
				pt++;
			if ((pt[1] == '\0') && (pt > path))
				break;
			*copy++ = *pt++;
		} else
			*copy++ = *pt++;
	}
	*copy = '\0';
	(void) strcpy(path, buffer);
}

static void
basepath(path, basedir)
char *path;
char *basedir;
{
	char buffer[MAXPATHLEN];

	if (*path != '/') {
		(void) strcpy(buffer, path);
		if (basedir && *basedir) {
			while (*basedir)
				*path++ = *basedir++;
			if (path[-1] == '/')
				path--;
		}
		*path++ = '/';
		(void) strcpy(path, buffer);
	}
}

void
canoninplace(char *src)
{
	char *dst;

	dst = src;
	while (*src) {
		if (*src == '/') {
			*dst++ = '/';
			while (*src == '/')
				src++;
		} else
			*dst++ = *src++;
	}
	if (*(dst - 1) == '/')
		dst--;
	*dst = '\0';
}

#define	ERROR(s) \
	{ \
		srcherrstr = (s); \
		(void) getend(fpin); \
		return (-1); \
	}

static char	mypath[MAXPATHLEN];
static char	mylocal[MAXPATHLEN];

static int
srchcfile(ept, path, fpin, fpout)
struct cfent *ept;
char	*path;
FILE	*fpin, *fpout;
{
	struct pinfo *pinfo, *lastpinfo;
	long	pos;
	char	*pt,
		pkgname[PKGSIZ+1],
		classname[CLSSIZ+1];
	int	c, n, rdpath, anypath;

	/*
	 * This code uses goto's instead of nested
	 * subroutines because execution time of this
	 * routine is especially critical to installation
	 */

	srcherrstr = NULL;
	ept->volno = 0;
	ept->ftype = BADFTYPE;
	(void) strcpy(ept->pkg_class, BADCLASS);
	ept->path = NULL;
	ept->ainfo.local = NULL;
	ept->ainfo.mode = BADMODE;
	(void) strcpy(ept->ainfo.owner, BADOWNER);
	(void) strcpy(ept->ainfo.group, BADGROUP);
	ept->cinfo.size = ept->cinfo.cksum = ept->cinfo.modtime = BADCONT;

	/* free up list of packages which reference this entry */
	while (ept->pinfo) {
		pinfo = ept->pinfo->next;
		free(ept->pinfo);
		ept->pinfo = pinfo;
	}
	ept->pinfo = NULL;
	ept->npkgs = 0;

	/*
	 * If path to search for is "*", then we will return
	 * the first path we encounter as a match, otherwise
	 * we return an error
	 */
	anypath = 0;
	if (path && (path[0] != '/')) {
		if (!strcmp(path, "*"))
			anypath++;
		else {
			srcherrstr = gettext("illegal search path specified");
			return (-1);
		}
	}

	rdpath = 0;
	for (;;) {
		if (feof(fpin))
			return (0); /* no more entries */

		/* save current position in file */
		pos = ftell(fpin);

		/* grab path from first entry */
		c = getc(fpin);
		if (c != '/') {
			/*
			 * We check for EOF inside this if statement
			 * to reduce normal execution time
			 */
			if (c == EOF)
				return (0); /* no more entries */
			else if (isspace(c) || (c == '#') || (c == ':')) {
				/* line is a comment */
				(void) getend(fpin);
				continue;
			}

			/*
			 * We need to read this entry in the
			 * format which specifies
			 *	ftype class path
			 * so we set the rdpath variable and
			 * immediately jump to the code which
			 * will parse this format.  When done,
			 * that code will return to Path_Done below
			 */
			(void) ungetc(c, fpin);
			rdpath = 1;
			break;
		}

		/* copy first token into path element of passed structure */
		pt = mypath;
		do {
			if (strchr("= \t\n", c))
				break;
			*pt++ = (char) c;
		} while ((c = getc(fpin)) != EOF);
		*pt = '\0';

		if (c == EOF)
			ERROR("incomplete entry")
		ept->path = mypath;

Path_Done:
		/*
		 * Determine if we have read the pathname which
		 * identifies the entry we are searching for
		 */
		if (anypath)
			n = 0; /* any pathname will do */
		else if (path)
			n = strcmp(path, ept->path);
		else
			n = 1; /* no pathname will match */

		if (n == 0) {
			/*
			 * We want to return information about this
			 * path in the structure provided, so
			 * parse any local path and jump to code
			 * which parses rest of the input line
			 */
			if (c == '=') {
				/* parse local path specification */
				if (getstr(fpin, NULL, MAXPATHLEN, mylocal))
					ERROR("unable to read local/link path")
				ept->ainfo.local = mylocal;
			}
			break; /* scan into a structure */
		} else if (n < 0) {
			/*
			 * The entry we want would fit BEFORE the
			 * one we just read, so we need to unread
			 * what we've read by seeking back to the
			 * start of this entry
			 */
			if (fseek(fpin, pos, 0)) {
				srcherrstr = gettext("failure attempting fseek");
				return (-1);
			}
			return (2); /* path would insert here */
		}

		if (fpout) {
			/*
			 * Copy what we've read and the rest of this
			 * line onto the specified output stream
			 */
			(void) fprintf(fpout, "%s%c", ept->path, c);
			if (rdpath) {
				(void) fprintf(fpout, "%c %s", ept->ftype,
					ept->pkg_class);
			}
			while ((c = getc(fpin)) != EOF) {
				putc(c, fpout);
				if (c == '\n')
					break;
			}
		} else {
			/*
			 * Since this isn't the entry we want, just read
			 * the stream until we find the end of this entry
			 * and then start this search loop again
			 */
			while ((c = getc(fpin)) != EOF) {
				if (c == '\n')
					break;
			}
			if (c == EOF)
				ERROR("missing newline at end of entry")
		}
	}

	if (rdpath < 2) {
		/*
		 * Since we are processing an oldstyle entry and
		 * we have already read ftype, class, and path
		 * we just jump into reading the other info
		 */

		switch (c = eatwhite(fpin)) {
		    case EOF:
			srcherrstr = gettext("incomplete entry");
			return (-1);

		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
			ERROR(gettext("volume number not expected"))

		    case 'i':
			ERROR(gettext("ftype <i> not expected"))

		    case '?':
		    case 'f':
		    case 'v':
		    case 'e':
		    case 'l':
		    case 's':
		    case 'p':
		    case 'c':
		    case 'b':
		    case 'd':
		    case 'x':
			ept->ftype = (char) c;
			if (getstr(fpin, NULL, CLSSIZ, ept->pkg_class))
				ERROR(gettext("unable to read class token"))
			if (!rdpath)
				break; /* we already read the pathname */

			if (getstr(fpin, "=", MAXPATHLEN, mypath))
				ERROR(gettext("unable to read pathname field"))
			ept->path = mypath;

			c = getc(fpin);
			rdpath++;
			goto Path_Done;

		    default:
			srcherrstr = gettext("unknown ftype");
	Error:
			(void) getend(fpin);
			return (-1);
		}
	}

	if (strchr("sl", ept->ftype) && (ept->ainfo.local == NULL))
		ERROR(gettext("no link source specified"));

	if (strchr("cb", ept->ftype)) {
#ifdef SUNOS41
		ept->ainfo.xmajor = BADMAJOR;
		ept->ainfo.xminor = BADMINOR;
		if (getnum(fpin, 10, (long *)&ept->ainfo.xmajor, BADMAJOR) ||
		    getnum(fpin, 10, (long *)&ept->ainfo.xminor, BADMINOR))
#else
		ept->ainfo.major = BADMAJOR;
		ept->ainfo.minor = BADMINOR;
		if (getnum(fpin, 10, (long *)&ept->ainfo.major, BADMAJOR) ||
		    getnum(fpin, 10, (long *)&ept->ainfo.minor, BADMINOR))
#endif
			ERROR(gettext("unable to read major/minor device numbers"))
	}

	if (strchr("cbdxpfve", ept->ftype)) {
		/* mode, owner, group should be here */
		if (getnum(fpin, 8, (long *)&ept->ainfo.mode, BADMODE) ||
		    getstr(fpin, NULL, ATRSIZ, ept->ainfo.owner) ||
		    getstr(fpin, NULL, ATRSIZ, ept->ainfo.group))
			ERROR(gettext("unable to read mode/owner/group"))
	}

	if (strchr("ifve", ept->ftype)) {
		/* look for content description */
		if (getnum(fpin, 10, (long *)&ept->cinfo.size, BADCONT) ||
		    getnum(fpin, 10, (long *)&ept->cinfo.cksum, BADCONT) ||
		    getnum(fpin, 10, (long *)&ept->cinfo.modtime, BADCONT))
			ERROR(gettext("unable to read content info"))
	}

	if (ept->ftype == 'i') {
		if (getend(fpin)) {
			srcherrstr = gettext("extra tokens on input line");
			return (-1);
		}
		return (1);
	}

	/* determine list of packages which reference this entry */
	lastpinfo = (struct pinfo *)0;
	while ((c = getstr(fpin, ":\\", PKGSIZ, pkgname)) <= 0) {
		if (c < 0)
			ERROR(gettext("package name too long"))
		else if (c == 0) {
			/* a package was listed */
			pinfo = (struct pinfo *)xcalloc(sizeof (struct pinfo));
			if (!lastpinfo)
				ept->pinfo = pinfo; /* first one */
			else
				lastpinfo->next = pinfo; /* link list */
			lastpinfo = pinfo;

			if (strchr("-+*~!%", pkgname[0])) {
				pinfo->status = pkgname[0];
				(void) strcpy(pinfo->pkg, pkgname+1);
			} else
				(void) strcpy(pinfo->pkg, pkgname);

			/* pkg/[:[ftype][:class] */
			c = getc(fpin);
			if (c == '\\') {
				/* get alternate ftype */
				pinfo->editflag++;
				c = getc(fpin);
			}

			if (c == ':') {
				/* get special classname */
				(void) getstr(fpin, "", 12, classname);
				(void) strcpy(pinfo->aclass, classname);
				c = getc(fpin);
			}
			ept->npkgs++;

			if ((c == '\n') || (c == EOF))
				return (1);
			else if (!isspace(c))
				ERROR(gettext("bad end of entry"))
		}
	}

	if (getend(fpin) && ept->pinfo) {
		srcherrstr = gettext("extra token(s) on input line");
		return (-1);
	}
	return (1);
}

static int
getnum(fp, base, d, bad)
FILE *fp;
int base;
long *d;
long bad;
{
	int c;

	/* leading white space ignored */
	c = eatwhite(fp);
	if (c == '?') {
		*d = bad;
		return (0);
	}

	if ((c == EOF) || (c == '\n') || !isdigit(c)) {
		(void) ungetc(c, fp);
		return (1);
	}

	*d = 0;
	while (isdigit(c)) {
		*d = (*d * base) + (c & 017);
		c = getc(fp);
	}
	(void) ungetc(c, fp);
	return (0);
}

static int
getstr(fp, sep, n, str)
FILE *fp;
int n;
char *sep, *str;
{
	int c;

	/* leading white space ignored */
	c = eatwhite(fp);
	if ((c == EOF) || (c == '\n')) {
		(void) ungetc(c, fp);
		return (1); /* nothing there */
	}

	/* fill up string until space, tab, or separator */
	while (!strchr(" \t", c) && (!sep || !strchr(sep, c))) {
		if (n-- < 1) {
			*str = '\0';
			return (-1); /* too long */
		}
		*str++ = (char) c;
		c = getc(fp);
		if ((c == EOF) || (c == '\n'))
			break; /* no more on this line */
	}
	*str = '\0';
	(void) ungetc(c, fp);
	return (0);
}

static int
getend(fp)
FILE *fp;
{
	int c;
	int n;

	n = 0;
	do {
		if ((c = getc(fp)) == EOF)
			return (n);
		if (!isspace(c))
			n++;
	} while (c != '\n');
	return (n);
}

static int
eatwhite(fp)
FILE *fp;
{
	int c;

	/* this test works around a side effect of getc() */
	if (feof(fp))
		return (EOF);
	do
		c = getc(fp);
	while ((c == ' ') || (c == '\t'));
	return (c);
}

#define	IO_BUFFER_SIZE	8192

extern char *install_root;
#define	reperr printf

#define	WDMSK 0177777L
#define	BUFSIZE 512
#define	DATEFMT	"%D %r"
#define	TDELTA 15*60

#define	ERR_GETWD	gettext("unable to determine current working directory")
#define	ERR_CHDIR	gettext("unable to change current working directory to <%s>")

/* checksum disable swicth */
static int	dochecksum = 1;

static int	cksumerr;

struct part { short unsigned hi, lo; };
static union hilo { /* this only works right in case short is 1/2 of long */
	struct part hl;
	long	lg;
} tempa, suma;

static struct filediff *
getfilediff(path)
char *path;
{
	struct filediff *statp;

	statp = (struct filediff *)xcalloc(sizeof (struct filediff) +
	    strlen(path) + 1);

	statp->diff_flags = 0;
	statp->linkptr = NULL;

	copycanon(statp->component_path, path);
	return (statp);
}

/*
 * copy path, converting to canonical form: remove leading
 * install_root value, compress multiple slashed to one.
 */
static void
copycanon(char *dst, char *src)
{
	int n;

	if (install_root) {
		n = strlen(install_root);
		if (n > 1)
			src += n;
	}
	while (*src) {
		if (*src == '/') {
			*dst++ = '/';
			while (*src == '/')
				src++;
		} else
			*dst++ = *src++;
	}
	if (*(dst - 1) == '/')
		dst--;
	*dst = '\0';
}

static struct filediff *
cverify(path, cinfo, statp)
char *path;
struct cinfo *cinfo;
struct filediff *statp;
{
	struct stat	status;	/* file status buffer */
	unsigned	mycksum;

	if (stat(path, &status) < 0) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_MISSING;
		return (statp);
	} else if (status.st_mtime != cinfo->modtime) {
		dochecksum = 1;
	} else {
		dochecksum = 0;
	}

	if (status.st_size != cinfo->size) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_CONTENTS;
		return (statp);
	}

/* checksum disable swicth */
	if (dochecksum == 1) {
		mycksum = docksum(path);
	} else {
		mycksum = cinfo->cksum;
	}

	if ((mycksum != cinfo->cksum) || cksumerr) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_CONTENTS;
		return (statp);
	}

	return (statp);
}

static unsigned
docksum(path)
char *path;
{
	register int fp;
	static	char io_buffer[IO_BUFFER_SIZE];
	char 	*cp;
	long	bytes_read;

	unsigned lsavhi, lsavlo;

	cksumerr = 0;
	if ((fp = open(path, O_RDONLY, 0)) == -1) {
		cksumerr++;
		return (0);
	}

	suma.lg = 0;

	while ((bytes_read = read(fp, io_buffer, sizeof (io_buffer))) > 0)
	    for (cp = io_buffer; cp < (io_buffer+bytes_read); cp++)
		    suma.lg += ((int) (*cp&0377)) & WDMSK;

	tempa.lg = (suma.hl.lo & WDMSK) + (suma.hl.hi & WDMSK);
	lsavhi = (unsigned) tempa.hl.hi;
	lsavlo = (unsigned) tempa.hl.lo;

	(void) close(fp);
	return (lsavhi+lsavlo);
}

static struct filediff *
averify(ftype, path, ainfo)
char	*ftype, *path;
struct ainfo *ainfo;
{
	struct stat	status, targ_status;	/* file status buffer */
	int		n;
	char		myftype;
	char		buf[MAXPATHLEN];
	char 		cwd[MAXPATHLEN];
	char 		*cd;
	char 		*c;
	struct filediff *statp;
	int		err;
	struct group	*grp;			/* group entry buffer */
	struct passwd	*pwd;

	statp = NULL;

	if (lstat(path, &status) < 0) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_MISSING;
		return (statp);
	}

	/* Determining actual type of existing object. */
	err = 0;
	switch (status.st_mode & S_IFMT) {
	case S_IFLNK:
		myftype = 's';
		if (*ftype != 's') err++;
		break;
	case S_IFIFO:
		myftype = 'p';
		if (*ftype != 'p') err++;
		break;
	case S_IFCHR:
		myftype = 'c';
		if (*ftype != 'c') err++;
		break;
	case S_IFDIR:
		myftype = 'd';
		if (!strchr("dx", *ftype)) err++;
		break;
	case S_IFBLK:
		myftype = 'b';
		if (*ftype != 'b') err++;
		break;
	case S_IFREG:
		myftype = 'f';
		if (!strchr("feilv", *ftype)) err++;
		break;
	case 0:
	default:
		return (NULL);
	}
	if (err) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_TYPE;
		statp->exp_type = *ftype;
		statp->actual_type = myftype;
		return (statp);
	}

	if (*ftype == 'l' || *ftype == 's') {
		/* Get copy of the current working directory */
		if (getcwd(cwd, MAXPATHLEN) == NULL) {
			reperr(ERR_GETWD);
			return ((struct filediff *)-1);
		}
		/*
		 * Change to the directory in which the link is
		 * to be created.
		 */
		cd = xstrdup(path);
		c = strrchr(cd, '/');
		if (c) {
			*c = NULL;
			if (chdir(cd) != 0) {
				if (statp == NULL)
					statp = getfilediff(path);
				statp->diff_flags |= DIFF_MISSING_LINK;
				free(cd);
				return (statp);
			}
		}
		free(cd);

		if (*ftype == 'l') {
			if ((status.st_nlink < 2) ||
			    (stat(ainfo->local, &targ_status) < 0)) {
				if (statp == NULL)
					statp = getfilediff(path);
				statp->diff_flags |= DIFF_MISSING_LINK;
			} else {

				if ((status.st_ino != targ_status.st_ino) ||
				    (status.st_dev != targ_status.st_dev)) {
					/*
					 *  Go back to previous working
					 *  directory.
					 */
					if (chdir(cwd) != 0) {
						reperr(ERR_CHDIR, cwd);
						return ((struct filediff *)-1);
					}
					if (statp == NULL)
						statp = getfilediff(path);
					statp->diff_flags |= DIFF_HLINK_TARGET;
					statp->linkptr =
					    (char *) xmalloc((size_t)
					    (strlen(ainfo->local) + 1));
					strcpy(statp->linkptr, ainfo->local);
				}
			}
		} else { /* ftype == 's' */
			char	pathbuf[MAXPATHLEN];

			if (*ainfo->local == '/')
				sprintf(pathbuf, "/a%s", ainfo->local);
			else
				strcpy(pathbuf, ainfo->local);

			if (stat(pathbuf, &targ_status) < 0) {
				if (statp == NULL)
					statp = getfilediff(path);
				statp->diff_flags |= DIFF_MISSING_LINK;
			}

			/* make sure that symbolic link is correct */
			n = readlink(path, buf, MAXPATHLEN);
			buf[n] = '\0';
			if (strcmp(buf, ainfo->local)) {
				if (statp == NULL)
					statp = getfilediff(path);
				statp->diff_flags |= DIFF_SLINK_TARGET;
				statp->linkptr =
				    (char *)xmalloc((size_t)
				    (strlen(ainfo->local) + 1));
				strcpy(statp->linkptr, ainfo->local);

				statp->link_found =
				    (char *)xmalloc((size_t) (strlen(buf) + 1));
				strcpy(statp->link_found, buf);
			}
		}

		/* Go back to previous working directory */
		if (chdir(cwd) != 0) {
			reperr(ERR_CHDIR, cwd);
			exit(1);
		}
		return (statp);
	}

	if (*ftype == 'i')
		return (NULL); /* don't check anything else */

#ifdef DONTUSE
	/*
	 * 10 Feb 93
	 * Don't track these problems now since the current state of ON
	 * would generate too many messages.
	 */
	if (strchr("cb", myftype)) {
		if (ainfo->major < 0)
			ainfo->major = ((status.st_rdev>>8)&0377);
		if (ainfo->minor < 0)
			ainfo->minor = (status.st_rdev&0377);
		/* check major & minor */
		if (status.st_rdev != makedev(ainfo->major, ainfo->minor)) {
			if (statp == NULL)
				statp = getfilediff(path);
			statp->diff_flags |= DIFF_MAJORMINOR;
			statp->majmin = makedev(ainfo->major, ainfo->minor);
		}
	}
#endif DONTUSE

	/* compare specified mode w/ actual mode excluding sticky bit */
	if ((ainfo->mode & 06777) != (status.st_mode & 06777)) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_PERM;
		statp->act_mode = status.st_mode & 07777;
	}

	/* rewind group file */
	setgrent();

	/* get group entry for specified group */
	grp = getgrnam(ainfo->group);
	if (grp == NULL || grp->gr_gid != status.st_gid) {
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_GID;
		statp->act_gid = status.st_gid;
	}

	/* rewind password file */
	setpwent();

	/* get password entry for specified owner */
	pwd = getpwnam(ainfo->owner);
	if (pwd == NULL || pwd->pw_uid != status.st_uid) {
		/* get owner name for actual UID */
		if (statp == NULL)
			statp = getfilediff(path);
		statp->diff_flags |= DIFF_UID;
		statp->act_uid = status.st_uid;
	}
	return (statp);
}

char		*errstr = NULL;
static int	attrpreset = 0;

static char	mypath[MAXPATHLEN];
static char	mylocal[MAXPATHLEN];

static int
gpkgmap(ept, fp)
struct cfent *ept;
FILE *fp;
{
	int	c;

	errstr = NULL;
	ept->volno = 0;
	ept->ftype = BADFTYPE;
	(void) strcpy(ept->pkg_class, BADCLASS);
	ept->path = NULL;
	ept->ainfo.local = NULL;
	if (!attrpreset) {
		/* default attributes were supplied, so don't reset */
		ept->ainfo.mode = BADMODE;
		(void) strcpy(ept->ainfo.owner, BADOWNER);
		(void) strcpy(ept->ainfo.group, BADGROUP);
#ifdef SUNOS41
		ept->ainfo.xmajor = BADMAJOR;
		ept->ainfo.xminor = BADMINOR;
#else
		ept->ainfo.major = BADMAJOR;
		ept->ainfo.minor = BADMINOR;
#endif
	}
	ept->cinfo.cksum = ept->cinfo.modtime = ept->cinfo.size = (-1L);

	ept->npkgs = 0;

	if (!fp)
		return (-1);
readline:
	switch (c = eatwhite(fp)) {
	    case EOF:
		return (0);

	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		if (ept->volno) {
			errstr = gettext("bad volume number");
			goto error;
		}
		do {
			ept->volno = (ept->volno*10)+c-'0';
			c = getc(fp);
		} while (isdigit(c));
		goto readline;

	    case ':':
	    case '#':
		(void) getend(fp);
		/* FALLTHROUGH */
	    case '\n':
		goto readline;

	    case 'i':
		ept->ftype = (char) c;
		c = eatwhite(fp);
		/* FALLTHROUGH */
	    case '.':
	    case '/':
		(void) ungetc(c, fp);

		if (getstr(fp, "=", MAXPATHLEN, mypath)) {
			errstr = gettext("unable to read pathname field");
			goto error;
		}
		ept->path = mypath;
		c = getc(fp);
		if (c == '=') {
			if (getstr(fp, NULL, MAXPATHLEN, mylocal)) {
				errstr = gettext("unable to read local pathname");
				goto error;
			}
			ept->ainfo.local = mylocal;
		} else
			(void) ungetc(c, fp);

		if (ept->ftype == 'i') {
			/* content info might exist */
			if (!getnum(fp, 10, (long *)&ept->cinfo.size, BADCONT) &&
			(getnum(fp, 10, (long *)&ept->cinfo.cksum, BADCONT) ||
			getnum(fp, 10, (long *)&ept->cinfo.modtime, BADCONT))) {
				errstr = gettext("unable to read content info");
				goto error;
			}
		}

		if (getend(fp)) {
			errstr = gettext("extra tokens on input line");
			return (-1);
		}
		return (1);

	    case '?':
	    case 'f':
	    case 'v':
	    case 'e':
	    case 'l':
	    case 's':
	    case 'p':
	    case 'c':
	    case 'b':
	    case 'd':
	    case 'x':
		ept->ftype = (char) c;
		if (getstr(fp, NULL, CLSSIZ, ept->pkg_class)) {
			errstr = gettext("unable to read class token");
			goto error;
		}
		if (getstr(fp, "=", MAXPATHLEN, mypath)) {
			errstr = gettext("unable to read pathname field");
			goto error;
		}
		ept->path = mypath;

		c = getc(fp);
		if (c == '=') {
			/* local path */
			if (getstr(fp, NULL, MAXPATHLEN, mylocal)) {
				errstr = (strchr("sl", ept->ftype) ?
					gettext("unable to read link specification") :
					gettext("unable to read local pathname"));
				goto error;
			}
			ept->ainfo.local = mylocal;
		} else if (strchr("sl", ept->ftype)) {
			if ((c != EOF) && (c != '\n'))
				(void) getend(fp);
			errstr = gettext("missing or invalid link specification");
			return (-1);
		} else
			(void) ungetc(c, fp);
		break;

	    default:
		errstr = gettext("unknown ftype");
error:
		(void) getend(fp);
		return (-1);
	}

	if (strchr("sl", ept->ftype) && (ept->ainfo.local == NULL)) {
		errstr = gettext("no link source specified");
		goto error;
	}

	if (strchr("cb", ept->ftype)) {
#ifdef SUNOS41
		ept->ainfo.xmajor = BADMAJOR;
		ept->ainfo.xminor = BADMINOR;
		if (getnum(fp, 10, (long *)&ept->ainfo.xmajor, BADMAJOR) ||
		    getnum(fp, 10, (long *)&ept->ainfo.xminor, BADMINOR)) {
#else
		ept->ainfo.major = BADMAJOR;
		ept->ainfo.minor = BADMINOR;
		if (getnum(fp, 10, (long *)&ept->ainfo.major, BADMAJOR) ||
		    getnum(fp, 10, (long *)&ept->ainfo.minor, BADMINOR)) {
#endif
			errstr = gettext("unable to read major/minor device numbers");
			goto error;
		}
	}

	if (strchr("cbdxpfve", ept->ftype)) {
		/*
		 * Links and information files don't
		 * have attributes associated with them
		 */
		if (getnum(fp, 8, (long *)&ept->ainfo.mode, BADMODE))
			goto end;

		/* mode, owner, group should be here */
		if (getstr(fp, NULL, ATRSIZ, ept->ainfo.owner) ||
		    getstr(fp, NULL, ATRSIZ, ept->ainfo.group)) {
			errstr = gettext("unable to read mode/owner/group");
			goto error;
		}
	}

	if (strchr("ifve", ept->ftype)) {
		/* look for content description */
		if (!getnum(fp, 10, (long *)&ept->cinfo.size, BADCONT) &&
		(getnum(fp, 10, (long *)&ept->cinfo.cksum, BADCONT) ||
		getnum(fp, 10, (long *)&ept->cinfo.modtime, BADCONT))) {
			errstr = gettext("unable to read content info");
			goto error;
		}
	}

	if (ept->ftype == 'i')
		goto end;

end:
	if (getend(fp) && ept->pinfo) {
		errstr = gettext("extra token on input line");
		return (-1);
	}

done:
	return (1);
}


/*
 *********************************************************
 * THIS ROUTINE TAKEN FROM LIB/LIBC/PORT/GEN/CFTIME.C    *
 *********************************************************
 */

#ifdef SUNOS41
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 * This routine converts time as follows.  The epoch is 0000  Jan  1
 * 1970  GMT.   The  argument  time  is  in seconds since then.  The
 * localtime(t) entry returns a pointer to an array containing:
 *
 *		  seconds (0-59)
 *		  minutes (0-59)
 *		  hours (0-23)
 *		  day of month (1-31)
 *		  month (0-11)
 *		  year
 *		  weekday (0-6, Sun is 0)
 *		  day of the year
 *		  daylight savings flag
 *
 * The routine corrects for daylight saving time and  will  work  in
 * any  time  zone provided "timezone" is adjusted to the difference
 * between Greenwich and local standard time (measured in seconds).
 *
 *	 ascftime(buf, format, t)	-> where t is produced by localtime
 *					   and returns a ptr to a character
 *					   string that has the ascii time in
 *					   the format specified by the format
 *					   argument (see date(1) for format
 *					   syntax).
 *
 *	 cftime(buf, format, t) 	-> just calls ascftime.
 *
 *
 *
 */

#ifdef __STDC__
	#pragma weak ascftime = _ascftime
	#pragma weak cftime = _cftime
#endif
#ifndef SUNOS41
#include	"synonyms.h"
#else
#define	const
#endif
#include	<stddef.h>
#include	<time.h>
#include	<limits.h>
#include	<stdlib.h>

int
cftime(buf, format, t)
char	*buf;
char	*format;
const time_t	*t;
{
	return (ascftime(buf, format, localtime(t)));
}

int
ascftime(buf, format, tm)
char	*buf;
const char	*format;
const struct tm *tm;
{
	/* Set format string, if not already set */
	if (format == NULL || *format == '\0')
		if (((format = getenv("CFTIME")) == 0) || *format == 0)
			format =  "%C";

	return (strftime(buf, LONG_MAX, format, tm));
}
#endif



static int
to_be_ignored(Modinfo *mi, struct filediff *statp)
{
	char *cp, *p, *subcomponent;

	if (streq(mi->m_basedir, "/"))
		/* skip over leading "/" */
		subcomponent = statp->component_path + 1;
	else
		subcomponent = statp->component_path +
		    strlen(mi->m_basedir) + 1;
	cp = mi->m_pkg_hist->ignore_list;
	while ((p = split_name(&cp)) != NULL)
		if (streq(subcomponent, p))
			return (1);
	return (0);
}

int
match_missing_file(char *path)
{
	int n;
	struct missing_file *missp;

	if (missing_file_list == NULL)
		return (0);
	n = strlen(path);
	path[n] = '/';
	for (missp = missing_file_list; missp != NULL; missp = missp->next) {
		if (strncmp(missp->missing_file_name, path, missp->misslen)
		    == 0) {
			path[n] = '\0';
			return (1);
		}
	}
	path[n] = '\0';
	return (0);
}

/* Utility that moves a spinner around on the screen */

static int spinCt = 0;
static char spinner[4] = { '|', '/', '-', '\\' };
static int spindex = 0;

void spin(int skip)
{
	if (!in_final_upgrade_stage)
		return;
	if (! ((spinCt++) % skip)) {
		(void) printf("\r%c", spinner[(spindex++)%4]);
		(void) fflush(stdout);
	}
}

static int
contents_only(struct filediff *statp)
{

	if ((statp->diff_flags & DIFF_MASK) == DIFF_CONTENTS)
		return (1);
	else
		statp->diff_flags &= ~DIFF_CONTENTS;
	return (0);
}

/*
 *  The statp() of a file has turned up differences.  The infomation
 *  stored in the statp structure will be added to a chain of similar
 *  statp structures for future use.
 */
static void
add_statp_entry(struct filediff * statp, Modinfo * mi)
{
	struct pkg_info	*pkg_entry;
	struct pkg_info	**last_entry;
	char *pkg_name, *ptr;

	statp->diff_next = NULL;
	if (statp_chain_head == NULL) {
		statp_chain_head = statp;
		statp_chain_tail = statp;
	} else {
		statp_chain_tail->diff_next = statp;
		statp_chain_tail = statp;
	}

	if ((mi->m_pkg_hist == NULL) ||
	    (mi->m_pkg_hist->replaced_by == NULL)) {
		pkg_entry = (struct pkg_info *)xmalloc(sizeof(struct pkg_info));
		statp->pkg_info_ptr = pkg_entry;
		pkg_entry->next = NULL;
		pkg_entry->name = xstrdup(mi->m_pkgid);
		pkg_entry->arch = xstrdup(mi->m_arch);
	} else {
		ptr = mi->m_pkg_hist->replaced_by;
		last_entry = &(statp->pkg_info_ptr);
		while ((pkg_name = split_name(&ptr)) != NULL) {
			pkg_entry = (struct pkg_info *)
					xmalloc(sizeof(struct pkg_info));
			pkg_entry->next = NULL;
			pkg_entry->name = xstrdup(pkg_name);
			/*
			 * This is the old arch value.  This may be
			 * changed later to figure the 'new' arch.
			 */
			pkg_entry->arch = xstrdup(mi->m_arch);

			*last_entry =  pkg_entry;
			last_entry  = &(pkg_entry->next);
		}
	}
}

/*
 *  Build a table, of pkg_info structures, that contains the name
 *  and arch of each package found on the statp_chain.
 */
static void
build_pkg_arch_tbl()
{
	struct filediff *link;
	struct pkg_info *info;

	for (link=statp_chain_head; link != NULL; link=link->diff_next)
		for (info=link->pkg_info_ptr; info != NULL; info=info->next)
			add_tbl_entry(info->name, info->arch);
}

/*
 *  If the name/arch pair is unique:
 *		1. allocate memory for a new table entry
 *		2. assigns values to each field
 *		3. link the new entry into the table
 */
static void
add_tbl_entry(char * name, char * arch)
{
	struct pkg_info *tbl_entry, **tblpp;

	tblpp = &tbl_head;
	while (*tblpp != (struct pkg_info *)NULL) {
		if (strcmp(name, (*tblpp)->name) == 0 &&
		    strcmp(arch, (*tblpp)->arch) == 0)
			return;
		tblpp = &((*tblpp)->next);
	}
	tbl_entry = (struct pkg_info *)xmalloc(sizeof(struct pkg_info));
	tbl_entry->name = xstrdup(name);
	tbl_entry->arch = xstrdup(arch);
	tbl_entry->next = NULL;
	*tblpp = tbl_entry;
}

/*
 * For each package in the pkg_arch_tbl, loop through its pkgmap
 * searching for matches in the statp_chain.  Depending on its file
 * type, the entry may, or may not, be added to the class_list,
 * the real_modified_list, the tentative_modified_list, or deleted.
 */
static void
check_statp_chain()
{
	char *cp, mapfile[MAXPATHLEN];
	Module *mod;
	Modinfo *m_new;
	Arch_match_type match;
	FILE *fp;
	int ndx;
	static struct cfent pentry;
	struct filediff *ptr;
	struct pkg_info *tbl;

	/*
	 *  Looking for new media head
	 */
	mod = get_new_media_head();
	for (tbl=tbl_head; tbl != NULL; tbl=tbl->next) {

		/* should check to see of arch matches */
		m_new = find_new_package(mod->sub->info.prod,
		    tbl->name, tbl->arch, &match);
		if (m_new == NULL)
			continue;

		sprintf(mapfile, "%s/%s/pkgmap",
			mod->sub->info.prod->p_pkgdir, m_new->m_pkg_dir);
		if ((fp=fopen(mapfile, "r")) == NULL)
			continue;

		while (ndx=gpkgmap(&pentry, fp)) {
			if (ndx < 0)   /* garbled pkgmap entry */
				continue;
			cp = pentry.path;
			if (pentry.ftype == 'i') {
				if (*cp++ == 'i' && *cp++ == '.')
					chain_new_class(cp);
			} else {
				chain_match_component(&pentry, m_new);
			}
		}
		fclose(fp);
		free_classes();

		/*
		 * The entries that remain on the tentative_modfied_list
		 * may now be moved to the real_modified_list.
		 */
		if (real_modified_list == NULL) {
		    real_modified_list = tentative_modified_list;
		    real_modified_list_tail = tentative_modified_list;
		} else
		    real_modified_list_tail->diff_next=tentative_modified_list;

		/*
		 * Find the end of the tentative_modified_list
		 */
		for (ptr=real_modified_list_tail;
		     ptr != NULL; ptr = ptr->diff_next)
			real_modified_list_tail = ptr;

		/*
		 * Set tentative_modified_list to NULL
		 */
		tentative_modified_list = NULL;
	}
}

static Module *
get_new_media_head()
{
	Module *mod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED)
		return (mod);
	}
	return ((Module *)NULL);
}

/*
 * Add new class to the list of classes.
 */
static void
chain_new_class(char *classname)
{
	struct filediff *statp, **statpp;
	struct class_chain *new_class;

	new_class = (struct class_chain *)xmalloc(sizeof(struct class_chain));
	new_class->class = xstrdup(classname);
	if (class_head == NULL)
		new_class->next_class = NULL;
	else
		new_class->next_class = class_head;
	class_head = new_class;

	statpp = &tentative_modified_list;
	while (*statpp != (struct filediff *)NULL) {
		statp = *statpp;
		if (streq(classname,statp->pkgclass) && contents_only(statp)) {
			*statpp = statp->diff_next;
			free_filediff(statp);
		} else
			statpp = &(statp->diff_next);
	}
}

/*
 * If a pkgmap entry matches a statp_chain entry, determine if
 * it should be added to the tentative_modified_list, or the
 * real_modified_list, or be deleted.
 */
static void
chain_match_component(struct cfent *pentry, Modinfo *m_new)
{
	struct filediff *statp, **statpp;
	char path_string[MAXPATHLEN];

	if (m_new->m_instdir)
		sprintf(path_string, "%s/%s", m_new->m_instdir, pentry->path);
	else
		sprintf(path_string, "%s/%s", m_new->m_basedir, pentry->path);
	canoninplace(path_string);

	statpp = &statp_chain_head;
	while (*statpp != (struct filediff *)NULL) {
		statp = *statpp;

		if (strcmp(path_string, statp->component_path) == 0) {
			/*
			 * remove entry from statp_chain
			 */
			statp->replacing_pkg = m_new;
			*statpp = statp->diff_next;

			if (strcmp(pentry->pkg_class, BADCLASS) != 0) {
				if (in_classes(pentry->pkg_class) &&
						contents_only(statp)) {
					free_filediff(statp);
				} else {
					/*
					 * move entry to the
					 * tentative_modified_list
					 */
					statp->diff_next =
					    tentative_modified_list;
					tentative_modified_list = statp;
					strcpy(statp->pkgclass,
					    pentry->pkg_class);
				}
			} else {
				/*
				 * move the entry to the end
				 * of the real_modified_list.
				 */
				if (real_modified_list == NULL)
				    real_modified_list = statp;
				else
				    real_modified_list_tail->diff_next = statp;
				real_modified_list_tail = statp;
				statp->diff_next = NULL;
			}
			return;
		} else
			statpp = &(statp->diff_next);
	}
}

/*
 * Is 'class' in the class_list?
 */
static int
in_classes(char *class)
{
	struct class_chain *ptr;

	for (ptr = class_head; ptr != NULL; ptr = ptr->next_class) {
		if (strcmp(class, ptr->class) == 0)
			return (1);
	}
	return (0);
}

/*************************************************************************
 ***
 ***   The next six functions are used to free malloc'ed space.
 ***
 *************************************************************************/
static void
free_filediff(struct filediff *statp)
{
	if (statp->linkptr != NULL)
		free(statp->linkptr);
	if (statp->link_found != NULL)
		free(statp->link_found);
	if (statp->pkg_info_ptr != NULL)
		free_pkg_info_list(statp->pkg_info_ptr);
	free(statp);
}

/*
 * In addition to freeing malloc'ed space, this function also makes one last
 * attempt to associate a statp entry to the owning package.
 */
static void
free_statp_chain()
{
	struct filediff *ptr, *next;
	Modinfo		*mi;

	/*LINTED [var set before used]*/
	for (ptr = statp_chain_head; ptr != NULL; ptr = next) {
		/*
		 * Making one last check to see if the entry can be
		 * associated with a package being replaced. We determine
		 * that the file can associated if:
		 * - the action is not TO_BE_PRESERVED and
		 * - if the contents of the package are not going away)
		 * then do the save file processing.
		 */
		mi = ptr->owning_pkg;
		if (mi != NULL && (mi->m_action != TO_BE_PRESERVED &&
			!(mi->m_flags & CONTENTS_GOING_AWAY))) {
			next = ptr->diff_next;
			/*
			 * move the entry to the end
			 * of the real_modified_list.
			 */
			if (real_modified_list == NULL)
			    real_modified_list = ptr;
			else
			    real_modified_list_tail->diff_next = ptr;
			real_modified_list_tail = ptr;
			ptr->diff_next = NULL;
		} else {
			/*
			 * Can't make an association, so just free the entry.
			 */
			next = ptr->diff_next;
			free_filediff(ptr);
		}
	}
	statp_chain_head = NULL;
	statp_chain_tail = NULL;
}

static void
free_pkg_arch_tbl()
{
	free_pkg_info_list(tbl_head);
	tbl_head = NULL;
}

static void
free_pkg_info_list(struct pkg_info *head_of_list)
{
	struct pkg_info *ptr, *next_ptr;

	/*LINTED [var set before used]*/
	for (ptr = head_of_list; ptr != NULL; ptr = next_ptr) {
		next_ptr = ptr->next;
		if (ptr->name != NULL)
			free(ptr->name);
		if (ptr->arch != NULL)
			free(ptr->arch);
		free(ptr);
	}
}

static void
free_classes()
{
	struct class_chain *ptr, *next_class;

	/*LINTED [var set before used]*/
	for (ptr = class_head; ptr != NULL; ptr = next_class) {
		next_class = ptr->next_class;
		free(ptr->class);
		free(ptr);
	}
	class_head = NULL;
}

static void
free_modified_lists()
{
	struct filediff *ptr, *next;

	/*LINTED [var set before used]*/
	for (ptr = tentative_modified_list; ptr != NULL; ptr = next) {
		next = ptr->diff_next;
		free_filediff(ptr);
	}

	for (ptr = real_modified_list; ptr != NULL; ptr = next) {
		next = ptr->diff_next;
		free_filediff(ptr);
	}

	tentative_modified_list = NULL;
	real_modified_list = NULL;
	real_modified_list_tail = NULL;
}

#ifdef DEBUG2
/*************************************************************************
 ***
 ***   These functions are used during debugging
 ***
 *************************************************************************/
static FILE *fptr;

static int
dump_chain(char * filename)
{
	struct filediff *link;
	struct pkg_info *info;
	int status;

	if ((status = dump_open(filename)) != 0)
		return (status);

	fprintf(fptr, "---   dump statp chain   ---\n");
	for(link = statp_chain_head; link != NULL; link = link->diff_next) {
		fprintf(fptr, "statp(%s):\n", link->component_path);
		for(info = link->pkg_info_ptr; info != NULL; info = info->next)
			fprintf(fptr, "\t  %s, %s\n",
						info->name, info->arch);
	}
	fprintf(fptr, "----------------------------\n");
	dump_close();
	return (0);
}

static int
dump_tbl(char *filename)
{
	struct pkg_info *ptr;
	int status;

	if ((status = dump_open(filename)) != 0)
		return (status);

	fprintf(fptr, "---   dump pkg info tbl  ---\n");
	for (ptr = tbl_head; ptr != NULL; ptr = ptr->next)
		fprintf(fptr, "name => %-15s arch => %s\n",
						ptr->name, ptr->arch);
	fprintf(fptr, "----------------------------\n");
	dump_close();
	return (0);
}

static int
dump_open(char *filename)
{
	if ((fptr = fopen(filename, "w")) == NULL)
		return (1);
	return (0);
}

static void
dump_close()
{
	if (fptr != (FILE *)NULL) {
		(void) fclose(fptr);
		fptr = (FILE *)NULL;
	}
	return;
}
#endif
