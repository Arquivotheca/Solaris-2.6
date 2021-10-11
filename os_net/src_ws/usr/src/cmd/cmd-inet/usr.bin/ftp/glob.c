/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)glob.c	1.8	96/05/15 SMI"	/* SVr4.0 1.4	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 * 		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *		All rights reserved.
 *
 */


/*
 * C-shell glob for random programs.
 */

#include "ftp_var.h"

#ifndef NCARGS
#define	NCARGS	5120
#endif

#define	QUOTE 0200
#define	TRIM 0177
#define	eq(a, b)	(strcmp(a, b) == 0)

/*
 * According to the person who wrote the C shell "glob" code, a reasonable
 * limit on number of arguments would seem to be the maximum number of
 * characters in an arg list / 6.
 *
 * XXX:	With the new VM system, NCARGS has become enormous, making
 *	it impractical to allocate arrays with NCARGS / 6 entries on
 *	the stack.  The proper fix is to revamp code elsewhere (in
 *	sh.dol.c and sh.glob.c) to use a different technique for handling
 *	command line arguments.  In the meantime, we simply fall back
 *	on using the old value of NCARGS.
 */
#ifdef	notyet
#define	GAVSIZ	(NCARGS / 6)
#else	notyet
#define	GAVSIZ	(10240 / 6)
#endif	notyet

static	char **gargv;		/* Pointer to the (stack) arglist */
static	char **agargv;
static	int agargv_size;
static	long gargc;		/* Number args in gargv */
static	short gflag;
static char *strspl();
static char *strend(char *cp);
static char *strspl(char *cp, char *dp);
static int tglob(char c);
static char **copyblk(char **v);
static void ginit(char **agargv);
static void addpath(char c);
static int any(int c, char *s);
static void Gcat(char *s1, char *s2);
static void collect(char *as);
static void acollect(char *as);
static void sort(void);
static void expand(char *as);
static void matchdir(char *pattern);
static int execbrc(char *p, char *s);
static int ftp_fnmatch(wchar_t t_ch, wchar_t t_fch, wchar_t t_lch);
static int gethdir(char *home);
static void xfree(char *cp);
static void rscan(char **t, int (*f)(char));
static int letter(char c);
static int digit(char c);
static int match(char *s, char *p);
static int amatch(char *s, char *p);
static int blklen(char **av);
static char **blkcpy(char **oav, char **bv);

static	int globcnt;

static char	*globchars = "`{[*?";

static	char *gpath, *gpathp, *lastgpathp;
static	int globbed;
static	char *entp;
static	char **sortbas;

char **
glob(char *v)
{
	char agpath[BUFSIZ];
	char *vv[2];

	if (agargv == NULL) {
		agargv = (char **)malloc(GAVSIZ * sizeof (char *));
		agargv_size = GAVSIZ;
		if (agargv == NULL) {
			globerr = "Arguments too long.";
			return (0);
		}
	}
	vv[0] = v;
	vv[1] = 0;
	gflag = 0;
	rscan(vv, tglob);
	if (gflag == 0)
		return (copyblk(vv));

	globerr = 0;
	gpath = agpath;
	gpathp = gpath;
	*gpathp = 0;
	lastgpathp = &gpath[sizeof (agpath) - 2];
	ginit(agargv);
	globcnt = 0;
	collect(v);
	if (globcnt == 0 && (gflag&1)) {
		blkfree(gargv);
		gargv = 0;
		return (0);
	} else
		return (gargv = copyblk(gargv));
}

static void
ginit(char **agargv)
{

	agargv[0] = 0;
	gargv = agargv;
	sortbas = agargv;
	gargc = 0;
}

static void
collect(char *as)
{
	if (eq(as, "{") || eq(as, "{}")) {
		Gcat(as, "");
		sort();
	} else
		acollect(as);
}

static void
acollect(char *as)
{
	register long ogargc = gargc;

	gpathp = gpath; *gpathp = 0; globbed = 0;
	expand(as);
	if (gargc != ogargc)
		sort();
}

static void
sort(void)
{
	register char **p1, **p2, *c;
	char **Gvp = &gargv[gargc];

	p1 = sortbas;
	while (p1 < Gvp-1) {
		p2 = p1;
		while (++p2 < Gvp)
			if (strcmp(*p1, *p2) > 0)
				c = *p1, *p1 = *p2, *p2 = c;
		p1++;
	}
	sortbas = Gvp;
}

static void
expand(char *as)
{
	register char *cs;
	register char *sgpathp, *oldcs;
	struct stat stb;

	sgpathp = gpathp;
	cs = as;
	if (*cs == '~' && gpathp == gpath) {
		addpath('~');
		cs++;
		while (letter(*cs) || digit(*cs) || *cs == '-')
			addpath(*cs++);
		if (!*cs || *cs == '/') {
			if (gpathp != gpath + 1) {
				*gpathp = 0;
				if (gethdir(gpath + 1))
					globerr = "Unknown user name after ~";
				(void) strcpy(gpath, gpath + 1);
			} else
				(void) strcpy(gpath, home);
			gpathp = strend(gpath);
		}
	}
	while (!any(*cs, globchars)) {
		if (*cs == 0) {
			if (!globbed)
				Gcat(gpath, "");
			else if (stat(gpath, &stb) >= 0) {
				Gcat(gpath, "");
				globcnt++;
			}
			goto endit;
		}
		addpath(*cs++);
	}
	oldcs = cs;
	while (cs > as && *cs != '/')
		cs--, gpathp--;
	if (*cs == '/')
		cs++, gpathp++;
	*gpathp = 0;
	if (*oldcs == '{') {
		(void) execbrc(cs, ((char *)0));
		return;
	}
	matchdir(cs);
endit:
	gpathp = sgpathp;
	*gpathp = 0;
}

static void
matchdir(char *pattern)
{
	struct stat stb;
	register struct dirent *dp;
	DIR *dirp;

	/*
	 * BSD/SunOS open() system call maps a null pathname into
	 * "." while System V does not.
	 */
	if (*gpath == (char)0) {
		dirp = opendir(".");
	} else
		dirp = opendir(gpath);
	if (dirp == NULL) {
		if (globbed)
			return;
		goto patherr2;
	}
	if (fstat(dirp->dd_fd, &stb) < 0)
		goto patherr1;
	if (!S_ISDIR(stb.st_mode)) {
		errno = ENOTDIR;
		goto patherr1;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_ino == 0)
			continue;
		if (match(dp->d_name, pattern)) {
			Gcat(gpath, dp->d_name);
			globcnt++;
		}
	}
	closedir(dirp);
	return;

patherr1:
	closedir(dirp);
patherr2:
	globerr = "Bad directory components";
}

static int
execbrc(char *p, char *s)
{
	char restbuf[BUFSIZ + 2];
	register char *pe, *pm, *pl;
	int brclev = 0;
	char *lm, savec, *sgpathp;
	int	len;

	for (lm = restbuf; *p != '{'; *lm += len, p += len) {
		if ((len = mblen(p, MB_CUR_MAX)) <= 0)
			len = 1;
		memcpy(lm, p, len);
	}

	for (pe = ++p; *pe; pe += len) {
		if ((len = mblen(pe, MB_CUR_MAX)) <= 0)
			len = 1;

		switch (*pe) {

		case '{':
			brclev++;
			continue;

		case '}':
			if (brclev == 0)
				goto pend;
			brclev--;
			continue;

		case '[':
			for (pe++; *pe && *pe != ']'; pe += len) {
				if ((len = mblen(pe, MB_CUR_MAX)) <= 0)
					len = 1;
			}
			len = 1;
			continue;
		}
	}
pend:
	brclev = 0;
	for (pl = pm = p; pm <= pe; pm += len) {
		if ((len = mblen(pm, MB_CUR_MAX)) <= 0)
			len = 1;

		switch (*pm & (QUOTE|TRIM)) {

		case '{':
			brclev++;
			continue;

		case '}':
			if (brclev) {
				brclev--;
				continue;
			}
			goto doit;

		case ','|QUOTE:
		case ',':
			if (brclev)
				continue;
doit:
			savec = *pm;
			*pm = 0;
			(void) strcpy(lm, pl);
			(void) strcat(restbuf, pe + 1);
			*pm = savec;
			if (s == 0) {
				sgpathp = gpathp;
				expand(restbuf);
				gpathp = sgpathp;
				*gpathp = 0;
			} else if (amatch(s, restbuf))
				return (1);
			sort();
			pl = pm + 1;
			if (brclev)
				return (0);
			continue;

		case '[':
			for (pm++; *pm && *pm != ']'; pm += len) {
				if ((len = mblen(pm, MB_CUR_MAX)) <= 0)
					len = 1;
			}
			len = 1;
			if (!*pm)
				pm--;
			continue;
		}
	}
	if (brclev)
		goto doit;
	return (0);
}

static int
match(char *s, char *p)
{
	register int c;
	register char *sentp;
	char sglobbed = globbed;

	if (*s == '.' && *p != '.')
		return (0);
	sentp = entp;
	entp = s;
	c = amatch(s, p);
	entp = sentp;
	globbed = sglobbed;
	return (c);
}

static int
amatch(char *s, char *p)
{
	wchar_t scc;
	int ok;
	wchar_t lc1, lc2;
	char *sgpathp;
	struct stat stb;
	wchar_t c, cc;
	int	len_s, len_p;

	globbed = 1;
	for (;;) {
		if ((len_s = mbtowc(&scc, s, MB_CUR_MAX)) <= 0) {
			scc = (unsigned char)*s;
			len_s = 1;
		}
		/* scc = *s++ & TRIM; */
		s += len_s;

		if ((len_p = mbtowc(&c, p, MB_CUR_MAX)) <= 0) {
			c = (unsigned char)*p;
			len_p = 1;
		}
		p += len_p;
		switch (c) {

		case '{':
			return (execbrc(p - len_p, s - len_s));

		case '[':
			ok = 0;
			lc1 = 0;
			while ((cc = *p) != '\0') {
				if ((len_p = mbtowc(&cc, p, MB_CUR_MAX)) <= 0) {
					cc = (unsigned char)*p;
					len_p = 1;
				}
				p += len_p;
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if ((len_p = mbtowc(&lc2, p,
					    MB_CUR_MAX)) <= 0) {
						lc2 = (unsigned char)*p;
						len_p = 1;
					}
					p += len_p;
					if (ftp_fnmatch(scc, lc1, lc2))
						ok++;
				} else
					if (scc == (lc1 = cc))
						ok++;
			}
			if (cc == 0)
				if (!ok)
					return (0);
			continue;

		case '*':
			if (!*p)
				return (1);
			if (*p == '/') {
				p++;
				goto slash;
			}
			s -= len_s;
			do {
				if (amatch(s, p))
					return (1);
			} while (*s++);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if (c != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		case '/':
			if (scc)
				return (0);
slash:
			s = entp;
			sgpathp = gpathp;
			while (*s)
				addpath(*s++);
			addpath('/');
			if (stat(gpath, &stb) == 0 && S_ISDIR(stb.st_mode))
				if (*p == 0) {
					Gcat(gpath, "");
					globcnt++;
				} else
					expand(p);
			gpathp = sgpathp;
			*gpathp = 0;
			return (0);
		}
	}
}

#ifdef notdef
static
Gmatch(s, p)
	register char *s, *p;
{
	register int scc;
	int ok, lc;
	int c, cc;

	for (;;) {
		scc = *s++ & TRIM;
		switch (c = *p++) {

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0)
				if (ok)
					p--;
				else
					return (0);
			continue;

		case '*':
			if (!*p)
				return (1);
			for (s--; *s; s++)
				if (Gmatch(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		}
	}
}
#endif

static void
Gcat(char *s1, char *s2)
{
	if (gargc >= agargv_size - 1) {
		char **tmp;

		if (globerr) {
			return;
		}
		tmp = (char **)realloc(agargv,
		    (agargv_size + GAVSIZ) * sizeof (char *));
		if (tmp == NULL) {
			globerr = "Arguments too long";
			return;
		} else {
			agargv = tmp;
			agargv_size += GAVSIZ;
		}
		gargv = agargv;
		sortbas = agargv;
	}
	gargc++;
	gargv[gargc] = 0;
	gargv[gargc - 1] = strspl(s1, s2);
}

static void
addpath(char c)
{

	if (gpathp >= lastgpathp)
		globerr = "Pathname too long";
	else {
		*gpathp++ = c;
		*gpathp = 0;
	}
}

static void
rscan(char **t, int (*f)(char))
{
	register char *p, c;
	int	len;

	while (p = *t++) {
		if (f == tglob)
			if (*p == '~')
				gflag |= 2;
			else if (eq(p, "{") || eq(p, "{}"))
				continue;
		while ((c = *p) != '\0') {
			(void) (*f)(c);
			if ((len = mblen(p, MB_CUR_MAX)) <= 0)
				len = 1;
			p += len;
		}
	}
}

static int
tglob(char c)
{
	if (any(c, globchars))
		gflag |= c == '{' ? 2 : 1;
	return (c);
}

static int
letter(char c)
{
	return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_');
}

static int
digit(char c)
{
	return (c >= '0' && c <= '9');
}

static int
any(int c, char *s)
{
	int	len;

	while (*s) {
		if (*s == c)
			return (1);
		if ((len = mblen(s, MB_CUR_MAX)) <= 0)
			len = 1;
		s += len;
	}
	return (0);
}

static int
blklen(char **av)
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

static char **
blkcpy(char **oav, char **bv)
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

void
blkfree(char **av0)
{
	register char **av = av0;

	while (*av)
		xfree(*av++);
	free(av0);
}

static void
xfree(char *cp)
{
	extern char end[];

	if (cp >= end && cp < (char *)&cp)
		free(cp);
}

static char *
strspl(char *cp, char *dp)
{
	register char *ep = malloc((unsigned)(strlen(cp) + strlen(dp) + 1));

	if (ep == (char *)0)
		fatal("Out of memory");
	(void) strcpy(ep, cp);
	(void) strcat(ep, dp);
	return (ep);
}

static char **
copyblk(char **v)
{
	register char **nv = (char **)malloc((unsigned)((blklen(v) + 1) *
	    sizeof (char **)));

	if (nv == (char **)0)
		fatal("Out of memory");

	return (blkcpy(nv, v));
}

static char *
strend(char *cp)
{

	while (*cp)
		cp++;
	return (cp);
}
/*
 * Extract a home directory from the password file
 * The argument points to a buffer where the name of the
 * user whose home directory is sought is currently.
 * We write the home directory of the user back there.
 */
gethdir(char *home)
{
	register struct passwd *pp = getpwnam(home);

	if (!pp || home + strlen(pp->pw_dir) >= lastgpathp)
		return (1);
	(void) strcpy(home, pp->pw_dir);
	return (0);
}

static int
ftp_fnmatch(wchar_t t_ch, wchar_t t_fch, wchar_t t_lch)
{
	char	t_char[MB_LEN_MAX + 1];
	char	t_patan[MB_LEN_MAX * 2 + 8];
	char	*p;
	int	i;

	if ((t_ch == t_fch) || (t_ch == t_lch))
		return (1);

	p = t_patan;
	if ((i = wctomb(t_char, (wchar_t)t_ch)) <= 0)
		return (0);
	t_char[i] = 0;

	*p++ = '[';
	if ((i = wctomb(p, (wchar_t)t_fch)) <= 0)
		return (0);
	p += i;
	*p++ = '-';
	if ((i = wctomb(p, (wchar_t)t_lch)) <= 0)
		return (0);
	p += i;
	*p++ = ']';
	*p = 0;

	if (fnmatch(t_patan, t_char, FNM_NOESCAPE))
		return (0);
	return (1);
}