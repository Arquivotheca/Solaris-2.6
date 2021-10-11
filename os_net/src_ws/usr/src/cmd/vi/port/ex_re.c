/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)ex_re.c	1.17	96/07/31 SMI"	/* SVr4.0 1.21	*/

#include "ex.h"
#include "ex_re.h"
char *_compile();

/* 
 * The compiled-regular-expression storage areas (re, scanre, and subre)
 * have been changed into dynamically allocated memory areas, in both the
 * Solaris and XPG4 versions.
 * 
 * In the Solaris version, which uses the original libgen(3g) compile()
 * and step() calls, these areas are allocated once, and then data are
 * copied between them subsequently, as they were in the original
 * implementation.  This is possible because the compiled information is
 * a self-contained block of bits.
 *
 * In the XPG4 version, the expr:compile.o object is linked in as a
 * simulation of these functions using the new regcomp() and regexec()
 * functions.  The problem here is that the resulting
 * compiled-regular-expression data contain pointers to other data, which
 * need to be freed, but only when we are quite sure that we are done
 * with them - and certainly not before.  There was an earlier attempt to
 * handle these differences, but that effort was flawed.
 */

extern char	getchar();
#ifdef XPG4
void regex_comp_free(void *);
extern size_t regexc_size;	/* compile.c: size of regex_comp structure */
#endif /* XPG4 */

/*
 * Global, substitute and regular expressions.
 * Very similar to ed, with some re extensions and
 * confirmed substitute.
 */
global(k)
	bool k;
{
	register unsigned char *gp;
	register int c;
	register line *a1;
	unsigned char globuf[GBSIZE], *Cwas;
	int nlines = lineDOL();
	int oinglobal = inglobal;
	unsigned char *oglobp = globp;
	char	multi[MB_LEN_MAX + 1];
	wchar_t	wc;
	int	len;
	

	Cwas = Command;
	/*
	 * States of inglobal:
	 *  0: ordinary - not in a global command.
	 *  1: text coming from some buffer, not tty.
	 *  2: like 1, but the source of the buffer is a global command.
	 * Hence you're only in a global command if inglobal==2. This
	 * strange sounding convention is historically derived from
	 * everybody simulating a global command.
	 */
	if (inglobal==2)
		error(value(vi_TERSE) ? gettext("Global within global") :
gettext("Global within global not allowed"));
	markDOT();
	setall();
	nonzero();
	if (skipend())
		error(value(vi_TERSE) ? gettext("Global needs re") :
gettext("Missing regular expression for global"));
	c = getchar();
	(void)vi_compile(c, 1);
	savere(&scanre);
	gp = globuf;
	while ((c = peekchar()) != '\n') {
		if (!isascii(c)) {
			if (c == EOF) {
				c = '\n';
				ungetchar(c);
				goto out;
			}

mb_copy:
			if ((len = _mbftowc(multi, &wc, getchar, &peekc)) > 0) {
				if ((gp + len) >= &globuf[GBSIZE - 2])
					error(gettext("Global command too long"));
				strncpy(gp, multi, len);
				gp += len;
				continue;
			}
		}

		(void) getchar();
		switch (c) {

		case EOF:
			c = '\n';
			ungetchar(c);
			goto out;

		case '\\':
			c = peekchar();
			if (!isascii(c)) {
				*gp++ = '\\';
				goto mb_copy;
			}

			(void) getchar();
			switch (c) {

			case '\\':
				ungetchar(c);
				break;

			case '\n':
				break;

			default:
				*gp++ = '\\';
				break;
			}
			break;
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE - 2])
			error(gettext("Global command too long"));
	}

out:
	donewline();
	*gp++ = c;
	*gp++ = 0;
	saveall();
	inglobal = 2;
	for (a1 = one; a1 <= dol; a1++) {
		*a1 &= ~01;
		if (a1 >= addr1 && a1 <= addr2 && execute(0, a1) == k)
			*a1 |= 01;
	}
#ifdef notdef
/*
 * This code is commented out for now.  The problem is that we don't
 * fix up the undo area the way we should.  Basically, I think what has
 * to be done is to copy the undo area down (since we shrunk everything)
 * and move the various pointers into it down too.  I will do this later
 * when I have time. (Mark, 10-20-80)
 */
	/*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
	if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
#endif
	if (inopen)
		inopen = -1;
	/*
	 * Now for each marked line, set dot there and do the commands.
	 * Note the n^2 behavior here for lots of lines matching.
	 * This is really needed: in some cases you could delete lines,
	 * causing a marked line to be moved before a1 and missed if
	 * we didn't restart at zero each time.
	 */
	for (a1 = one; a1 <= dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands(1, 1);
			a1 = zero;
		}
	}
	globp = oglobp;
	inglobal = oinglobal;
	endline = 1;
	Command = Cwas;
	netchHAD(nlines);
	setlastchar(EOF);
	if (inopen) {
		ungetchar(EOF);
		inopen = 1;
	}
}

/*
 * gdelete: delete inside a global command. Handles the
 * special case g/r.e./d. All lines to be deleted have
 * already been marked. Squeeze the remaining lines together.
 * Note that other cases such as g/r.e./p, g/r.e./s/r.e.2/rhs/,
 * and g/r.e./.,/r.e.2/d are not treated specially.  There is no
 * good reason for this except the question: where to you draw the line?
 */
gdelete()
{
	register line *a1, *a2, *a3;

	a3 = dol;
	/* find first marked line. can skip all before it */
	for (a1=zero; (*a1&01)==0; a1++)
		if (a1>=a3)
			return;
	/* copy down unmarked lines, compacting as we go. */
	for (a2=a1+1; a2<=a3;) {
		if (*a2&01) {
			a2++;		/* line is marked, skip it */
			dot = a1;	/* dot left after line deletion */
		} else
			*a1++ = *a2++;	/* unmarked, copy it */
	}
	dol = a1-1;
	if (dot>dol)
		dot = dol;
	change();
}

bool	cflag;
int	scount, slines, stotal;

substitute(c)
	int c;
{
	register line *addr;
	register int n;
	int gsubf, hopcount;

	gsubf = compsub(c);
	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	stotal = 0;
	slines = 0;
	for (addr = addr1; addr <= addr2; addr++) {
		scount = hopcount = 0;
		if (dosubcon(0, addr) == 0)
			continue;
		if (gsubf) {
			/*
			 * The loop can happen from s/\</&/g
			 * but we don't want to break other, reasonable cases.
			 */
			hopcount = 0;
			while (*loc2) {
				if (++hopcount > sizeof linebuf)
					error(gettext("substitution loop"));
				if (dosubcon(1, addr) == 0)
					break;
			}
		}
		if (scount) {
			stotal += scount;
			slines++;
			putmark(addr);
			n = append(getsub, addr);
			addr += n;
			addr2 += n;
		}
	}
	if (stotal == 0 && !inglobal && !cflag)
		error(value(vi_TERSE) ? gettext("Fail") :
gettext("Substitute pattern match failed"));
	snote(stotal, slines);
	return (stotal);
}

compsub(ch)
{
	register int seof, c, uselastre; 
	static int gsubf;
	static unsigned char remem[RHSSIZE];
	static int remflg = -1;

	if (!value(vi_EDCOMPATIBLE))
		gsubf = cflag = 0;
	uselastre = 0;
	switch (ch) {

	case 's':
		(void)skipwh();
		seof = getchar();
		if (endcmd(seof) || any(seof, "gcr")) {
			ungetchar(seof);
			goto redo;
		}
		if (isalpha(seof) || isdigit(seof))
			error(value(vi_TERSE) ? gettext("Substitute needs re") :
gettext("Missing regular expression for substitute"));
		seof = vi_compile(seof, 1);
		uselastre = 1;
		comprhs(seof);
		if (!value(vi_EDCOMPATIBLE))
			gsubf = cflag = 0;
		break;

	case '~':
		uselastre = 1;
		/* fall into ... */
	case '&':
	redo:
		if (re == NULL || re->Expbuf[1] == 0)
			error(value(vi_TERSE) ? gettext("No previous re") :
gettext("No previous regular expression"));
		if (subre == NULL || subre->Expbuf[1] == 0)
			error(value(vi_TERSE) ? gettext("No previous substitute re") :
gettext("No previous substitute to repeat"));
		break;
	}
	for (;;) {
		c = getchar();
		switch (c) {

		case 'g':
			gsubf = !gsubf;
			continue;

		case 'c':
			cflag = !cflag;
			continue;

		case 'r':
			uselastre = 1;
			continue;

		default:
			ungetchar(c);
			setcount();
			donewline();
			if (uselastre)
				savere(&subre);
			else
				resre(subre);

			/*
			 * The % by itself on the right hand side means
			 * that the previous value of the right hand side
			 * should be used. A -1 is used to indicate no
			 * previously remembered search string.
			 */

			if (rhsbuf[0] == '%' && rhsbuf[1] == 0)
				if (remflg == -1)
					error(gettext("No previously remembered string"));
			        else
					strcpy(rhsbuf, remem);
			else {
				strcpy(remem, rhsbuf);
				remflg = 1;
			}
			return (gsubf);
		}
	}
}

comprhs(seof)
	int seof;
{
	register unsigned char *rp, *orp;
	register int c;
	unsigned char orhsbuf[RHSSIZE];
	char	multi[MB_LEN_MAX + 1];
	int	len;
	wchar_t	wc;

	rp = rhsbuf;
	CP(orhsbuf, rp);
	for (;;) {
		c = peekchar();
		if (c == seof) {
			(void) getchar();
			break;
		}

		if (!isascii(c) && c != EOF) {
			if ((len = _mbftowc(multi, &wc, getchar, &peekc)) > 0) {
				if ((rp + len) >= &rhsbuf[RHSSIZE - 1])
					goto toobig;
				strncpy(rp, multi, len);
				rp += len;
				continue;
			}
		}

		(void) getchar();
		switch (c) {

		case '\\':
			c = peekchar();
			if (c == EOF) {
				(void) getchar();
				error(gettext("Replacement string ends with \\"));
			}

			if (!isascii(c)) {
				*rp++ = '\\';
				if ((len = _mbftowc(multi, &wc, getchar, &peekc)) > 0) {
					if ((rp + len) >= &rhsbuf[RHSSIZE - 1])
						goto over_flow;
					strncpy(rp, multi, len);
					rp += len;
					continue;
				}
			}

			(void) getchar();
			if (value(vi_MAGIC)) {
				/*
				 * When "magic", \& turns into a plain &,
				 * and all other chars work fine quoted.
				 */
				if (c != '&') {
					if(rp >= &rhsbuf[RHSSIZE - 1]) {
						*rp=0;
						error(value(vi_TERSE) ?
gettext("Replacement pattern too long") :
gettext("Replacement pattern too long - limit 256 characters"));
					}
					*rp++ = '\\';
				}
				break;
			}
magic:
			if (c == '~') {
				for (orp = orhsbuf; *orp; *rp++ = *orp++)
					if (rp >= &rhsbuf[RHSSIZE - 1])
						goto toobig;
				continue;
			}
			if(rp >= &rhsbuf[RHSSIZE - 1]) {
over_flow:
				*rp=0;
				error(value(vi_TERSE) ?
gettext("Replacement pattern too long") :
gettext("Replacement pattern too long - limit 256 characters"));
			}
			*rp++ = '\\';
			break;

		case '\n':
		case EOF:
			if (!(globp && globp[0])) {
				ungetchar(c);
				goto endrhs;
			}

		case '~':
		case '&':
			if (value(vi_MAGIC))
				goto magic;
			break;
		}
		if (rp >= &rhsbuf[RHSSIZE - 1]) {
toobig:
			*rp = 0;
			error(value(vi_TERSE) ?
gettext("Replacement pattern too long") :
gettext("Replacement pattern too long - limit 256 characters"));
		}
		*rp++ = c;
	}
endrhs:
	*rp++ = 0;
}

getsub()
{
	register unsigned char *p;

	if ((p = linebp) == 0)
		return (EOF);
	strcLIN(p);
	linebp = 0;
	return (0);
}

dosubcon(f, a)
	bool f;
	line *a;
{

	if (execute(f, a) == 0)
		return (0);
	if (confirmed(a)) {
		dosub();
		scount++;
	}
	return (1);
}

confirmed(a)
	line *a;
{
	register int c, cnt, ch;

	if (cflag == 0)
		return (1);
	pofix();
	pline(lineno(a));
	if (inopen)
		putchar('\n' | QUOTE);
	c = lcolumn(loc1);
	ugo(c, ' ');
	ugo(lcolumn(loc2) - c, '^');
	flush();
	cnt = 0;
bkup:	
	ch = c = getkey();
again:
	if (c == '\b') {
		if ((inopen)
		 && (cnt > 0)) {
			putchar('\b' | QUOTE);
			putchar(' ');
			putchar('\b' | QUOTE), flush();
			cnt --;
		} 
		goto bkup;
	}
	if (c == '\r')
		c = '\n';
	if (inopen && MB_CUR_MAX == 1 || c < 0200) {
		putchar(c);
		flush();
		cnt++;
	}
	if (c != '\n' && c != EOF) {
		c = getkey();
		goto again;
	}
	noteinp();
	return (ch == 'y');
}

ugo(cnt, with)
	int with;
	int cnt;
{

	if (cnt > 0)
		do
			putchar(with);
		while (--cnt > 0);
}

int	casecnt;
bool	destuc;

dosub()
{
	register unsigned char *lp, *sp, *rp;
	int c;
	int	len;

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < (unsigned char *)loc1)
		*sp++ = *lp++;
	casecnt = 0;
	/*
	 * Caution: depending on the hardware, c will be either sign
	 * extended or not if C&QUOTE is set.  Thus, on a VAX, c will
	 * be < 0, but on a 3B, c will be >= 128.
	 */
	while (c = *rp) {
		if ((len = mblen((char *)rp, MB_CUR_MAX)) <= 0)
			len = 1;
		/* ^V <return> from vi to split lines */
		if (c == '\r')
			c = '\n';

		if (c == '\\') {
			rp++;
			if ((len = mblen((char *)rp, MB_CUR_MAX)) <= 0)
				len = 1;
			switch (c = *rp++) {

			case '&':
				sp = place(sp, loc1, loc2);
				if (sp == 0)
					goto ovflo;
				continue;

			case 'l':
				casecnt = 1;
				destuc = 0;
				continue;

			case 'L':
				casecnt = LBSIZE;
				destuc = 0;
				continue;

			case 'u':
				casecnt = 1;
				destuc = 1;
				continue;

			case 'U':
				casecnt = LBSIZE;
				destuc = 1;
				continue;

			case 'E':
			case 'e':
				casecnt = 0;
				continue;
			}
			if(re != NULL && c >= '1' && c < re->Nbra + '1') {
				sp = place(sp, braslist[c - '1'] , braelist[c - '1']);
				if (sp == 0)
					goto ovflo;
				continue;
			}
			rp--;
		}
		if (len > 1) {
			if ((sp + len) >= &genbuf[LBSIZE])
				goto ovflo;
			strncpy(sp, rp, len);
		} else {
			if (casecnt)
				*sp = fixcase(c);
			else
				*sp = c;
		}
		sp += len; rp += len;
		if (sp >= &genbuf[LBSIZE])
ovflo:
			error(value(vi_TERSE) ? gettext("Line overflow") :
gettext("Line overflow in substitute"));
	}
	lp = (unsigned char *)loc2;
	loc2 = (char *)(linebuf + (sp - genbuf));
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE])
			goto ovflo;
	strcLIN(genbuf);
}

fixcase(c)
	register int c;
{

	if (casecnt == 0)
		return (c);
	casecnt--;
	if (destuc) {
		if (islower(c))
			c = toupper(c);
	} else
		if (isupper(c))
			c = tolower(c);
	return (c);
}

unsigned char *
place(sp, l1, l2)
	register unsigned char *sp, *l1, *l2;
{

	while (l1 < l2) {
		*sp++ = fixcase(*l1++);
		if (sp >= &genbuf[LBSIZE])
			return (0);
	}
	return (sp);
}

snote(total, nlines)
	register int total, nlines;
{

	if (!notable(total))
		return;
	if (nlines != 1 && nlines != total)
		printf(mesg(value(vi_TERSE) ?
			/*
			 * TRANSLATION_NOTE
			 *	Reference order of arguments must not
			 *	be changed using '%digit$', since vi's
			 *	printf() does not support it.
			 */
			    gettext("%d subs on %d lines") :
			/*
			 * TRANSLATION_NOTE
			 *	Reference order of arguments must not
			 *	be changed using '%digit$', since vi's
			 *	printf() does not support it.
			 */
			    gettext("%d substitutions on %d lines")),
		       total, nlines);
	else
		printf(mesg(value(vi_TERSE) ?
			    gettext("%d subs") :
			    gettext("%d substitutions")),
		       total);
	noonl();
	flush();
}

#ifdef XPG4
#include <regex.h>

extern int regcomp_flags;	/* use to specify cflags for regcomp() */
#endif /* XPG4 */

vi_compile(eof, oknl)
	int eof;
	int oknl;
{
	register int c;
	register unsigned char *gp, *p1;
	unsigned char *rhsp;
	unsigned char rebuf[LBSIZE];
	char	multi[MB_LEN_MAX + 1];
	int	len;
	wchar_t	wc;

#ifdef XPG4
	/*
	 * reset cflags to plain BRE
	 * if \< and/or \> is specified, REG_WORDS is set.
	 */
	regcomp_flags = 0;
#endif /* XPG4 */

	gp = genbuf;
	if (isalpha(eof) || isdigit(eof))
error(gettext("Regular expressions cannot be delimited by letters or digits"));
	if(eof >= 0200 && MB_CUR_MAX > 1)
error(gettext("Regular expressions cannot be delimited by multibyte characters"));
	c = getchar();
	if (eof == '\\')
		switch (c) {

		case '/':
		case '?':
			if (scanre == NULL || scanre->Expbuf[1] == 0)
error(value(vi_TERSE) ? gettext("No previous scan re") :
gettext("No previous scanning regular expression"));
			resre(scanre);
			return (c);

		case '&':
			if (subre == NULL || subre->Expbuf[1] == 0)
error(value(vi_TERSE) ? gettext("No previous substitute re") :
gettext("No previous substitute regular expression"));
			resre(subre);
			return (c);

		default:
error(value(vi_TERSE) ? gettext("Badly formed re") :
gettext("Regular expression \\ must be followed by / or ?"));
		}
	if (c == eof || c == '\n' || c == EOF) {
		if (re == NULL || re->Expbuf[1] == 0)
error(value(vi_TERSE) ? gettext("No previous re") :
gettext("No previous regular expression"));
		if (c == '\n' && oknl == 0)
error(value(vi_TERSE) ? gettext("Missing closing delimiter") :
gettext("Missing closing delimiter for regular expression"));
		if (c != eof)
			ungetchar(c);
		return (eof);
	}
	gp = genbuf;
	if (c == '^') {
		*gp++ = c;
		c = getchar();
	}
	ungetchar(c);
	for (;;) {
		c = getchar();
		if (c == eof || c == EOF) {
			if (c == EOF)
				ungetchar(c);
			goto out;
		}
		if (gp >= &genbuf[LBSIZE - 3])
complex:
			cerror(value(vi_TERSE) ? gettext("Re too complex") :
gettext("Regular expression too complicated"));

		if (!(isascii(c) || MB_CUR_MAX == 1)) {
			ungetchar(c);
			if ((len = _mbftowc(multi, &wc, getchar, &peekc)) >= 1) {
				if ((gp + len) >= &genbuf[LBSIZE - 3])
					goto complex;
				strncpy(gp, multi, len);
				gp += len;
				continue;
			}
			(void) getchar();
		}

		switch (c) {

		case '\\':
			c = getchar();
			if (!isascii(c)) {
				ungetchar(c);
				if ((len = _mbftowc(multi, &wc, getchar, &peekc)) >= 1) {
					if ((gp + len) >= &genbuf[LBSIZE - 3])
						goto complex;
					*gp++ = '\\';
					strncpy(gp, multi, len);
					gp += len;
					continue;
				}
				(void) getchar();
			}
				
			switch (c) {

			case '<':
			case '>':
#ifdef XPG4
				regcomp_flags = REG_WORDS;
				/*FALLTHRU*/
#endif /* XPG4 */
			case '(':
			case ')':
			case '{':
			case '}':
			case '$':
			case '^':
			case '\\':
				*gp++ = '\\';
				*gp++ = c;
				continue;
			
			case 'n':
				*gp++ = c;
				continue;
			}
			if(c >= '0' && c <= '9') {
				*gp++ = '\\';
				*gp++ = c;
				continue;
			}
			if (value(vi_MAGIC) == 0)
magic:
			switch (c) {

			case '.':
				*gp++ = '.';
				continue;

			case '~':
				rhsp = rhsbuf;
				while (*rhsp) {
					if (!isascii(*rhsp)) {
						if ((len = mbtowc((wchar_t *)0, (char *)rhsp, MB_CUR_MAX)) > 1) {
							if ((gp + len) >= &genbuf[LBSIZE-2])
								goto complex;
							strncpy(gp, rhsp, len);
							rhsp += len; gp += len;
							continue;
						}
					}
					len = 1;
					if (*rhsp == '\\') {
						c = *++rhsp;
						if (c == '&')
cerror(value(vi_TERSE) ? gettext("Replacement pattern contains &") :
gettext("Replacement pattern contains & - cannot use in re"));
						if (c >= '1' && c <= '9')
cerror(value(vi_TERSE) ? gettext("Replacement pattern contains \\d") :
gettext("Replacement pattern contains \\d - cannot use in re"));
						if ((len = mbtowc((wchar_t *)0, (char *)rhsp, MB_CUR_MAX)) <= 1) {
							len = 1;
							if(any(c, ".\\*[$"))
								*gp++ = '\\';
						}
					}

					if ((gp + len) >= &genbuf[LBSIZE-2])
						goto complex;
					if (len == 1) {
						c = *rhsp++;
						*gp++ = (value(vi_IGNORECASE) ? tolower(c) : c);
					} else {
						strncpy(gp, rhsp, len);
						gp += len; rhsp += len;
					}
				}
				continue;

			case '*':
				*gp++ = '*';
				continue;

			case '[':
				*gp++ = '[';
				c = getchar();
				if (c == '^') {
					*gp++ = '^';
					c = getchar();
				}

				do { 
					if (!isascii(c) && c != EOF) {
						ungetchar(c);
						if ((len = _mbftowc(multi, &wc, getchar, &peekc)) >= 1) {
							if ((gp + len)>= &genbuf[LBSIZE-4])
								goto complex;
							strncpy(gp, multi, len);
							gp += len;
							c = getchar();
							continue;
						}
						(void) getchar();
					}
				
					if (gp >= &genbuf[LBSIZE-4])
						goto complex;
					if(c == '\\' && peekchar() == ']') {
						(void)getchar();
						*gp++ = '\\';
						*gp++ = ']';
					}
					else if (c == '\n' || c == EOF)
						cerror(gettext("Missing ]"));
					else
						*gp++ = (value(vi_IGNORECASE) ? tolower(c) : c);
					c = getchar();
				} while(c != ']');
				*gp++ = ']';
				continue;
			}
			if (c == EOF) {
				ungetchar(EOF);
				*gp++ = '\\';
				*gp++ = '\\';
				continue;
			}
			if (c == '\n')
cerror(value(vi_TERSE) ? gettext("No newlines in re's") :
gettext("Can't escape newlines into regular expressions"));
			*gp++ = '\\';
			*gp++ = (value(vi_IGNORECASE) ? tolower(c) : c);
			continue;

		case '\n':
			if (oknl) {
				ungetchar(c);
				goto out;
			}
cerror(value(vi_TERSE) ? gettext("Badly formed re") :
gettext("Missing closing delimiter for regular expression"));

		case '.':
		case '~':
		case '*':
		case '[':
			if (value(vi_MAGIC))
				goto magic;
			if(c != '~')
				*gp++ = '\\';
defchar:
		default:
			*gp++ = (value(vi_IGNORECASE) ? tolower(c) : c);
			continue;
		}
	}
out:
	*gp++ = '\0';

#ifdef XPG4
	/* see if our compiled RE's will fit in the re structure:	*/
	if (regexc_size > EXPSIZ) {
		/*
		 * this should never happen. but it's critical that we
		 * check here, otherwise .bss would get overwritten.
		 */
		cerror(value(vi_TERSE) ? gettext("RE's can't fit") :
			gettext("Regular expressions can't fit"));
		return(eof);
	}

	/*
	 * We create re each time we need it.
	 */

	if (re == NULL || re == scanre || re == subre) {
		if ((re = calloc(1, sizeof(struct regexp))) == NULL) {
			error(gettext("out of memory"));
			exit(errcnt);
		}
	} else {
		regex_comp_free(&re->Expbuf);
		memset(re, 0, sizeof(struct regexp));
	}

	compile((char *) genbuf, (char *) re->Expbuf, (char *) re->Expbuf
	    + regexc_size);
#else /* !XPG4 */
	(void)_compile(genbuf, re->Expbuf, re->Expbuf +
	    sizeof(re->Expbuf)); 
#endif /* XPG4 */

	if(regerrno)
		switch(regerrno) {
	
		case 42:
cerror(gettext("\\( \\) Imbalance"));
		case 43:
cerror(value(vi_TERSE) ? gettext("Awash in \\('s!") :
gettext("Too many \\('d subexpressions in a regular expression"));
		case 50:
			goto complex;
		case 67:
cerror(value(vi_TERSE) ? gettext("Illegal byte sequence") :
gettext("Regular expression has illegal byte sequence"));
		}
	re->Nbra = nbra;
	return(eof);
}

cerror(s)
	unsigned char *s;
{
	if (re) {
		re->Expbuf[0] = re->Expbuf[1] = 0;
	}
	error(s);
}

execute(gf, addr)
	line *addr;
{
	register unsigned char *p1, *p2;
	char *start;
	register int c, i;
	int ret;
	int	len;

	if (gf) {
		if (re == NULL || re->Expbuf[0])
			return (0);
		if(value(vi_IGNORECASE)) {
			p1 = genbuf;
			p2 = (unsigned char *)loc2;
			while(c = *p2) {
				if ((len = mblen((char *)p2, MB_CUR_MAX)) <= 0)
					len = 1;
				if (len == 1) {
					*p1++ = tolower(c);
					p2++;
					continue;
				}
				strncpy(p1, p2, len);
				p1 += len; p2 += len;
			}
			*p1 = '\0';
			locs = (char *)genbuf;
			p1 = genbuf;
			start = loc2;
		} else {
			p1 = (unsigned char *)loc2;
			locs = loc2;
		}
	} else {
		if (addr == zero)
			return (0);
		p1 = linebuf;
		getline(*addr);
		if(value(vi_IGNORECASE)) {
			p1 = genbuf;
			p2 = linebuf;
			while(c = *p2) {
				if ((len = mblen((char *)p2, MB_CUR_MAX)) <= 0)
					len = 1;
				if (len == 1) {
					*p1++ = tolower(c);
					p2++;
					continue;
				}
				strncpy(p1, p2, len);
				p1 += len; p2 += len;
			}
			*p1 = '\0';
			p1 = genbuf;
			start = (char *)linebuf;
		}
		locs = (char *)0;
	}

	ret = step((char *)p1, (char *)re->Expbuf);

	if(value(vi_IGNORECASE) && ret) {
		loc1 = start + (loc1 - (char *)genbuf);
		loc2 = start + (loc2 - (char *)genbuf);
		for(i = 0; i < NBRA; i++) {
			braslist[i] = start + (braslist[i] - (char *)genbuf);
			braelist[i] = start + (braelist[i] - (char *)genbuf);
		}
	}
	return ret;
}

/*
 *  Initialize the compiled regular-expression storage areas (called from
 *  main()).
 */

void init_re (void)
{
#ifdef XPG4
	re = scanre = subre = NULL;
#else /* !XPG4 */
	if ((re = calloc(1, sizeof(struct regexp))) == NULL) {
		error(gettext("out of memory"));
		exit(errcnt);
	}

	if ((scanre = calloc(1, sizeof(struct regexp))) == NULL) {
		error(gettext("out of memory"));
		exit(errcnt);
	}

	if ((subre = calloc(1, sizeof(struct regexp))) == NULL) {
		error(gettext("out of memory"));
		exit(errcnt);
	}
#endif /* XPG4 */
}

/*
 *  Save what is in the special place re to the named alternate
 *  location.  This means freeing up what's currently in this target
 *  location, if necessary.
 */

void savere(struct regexp ** a)
{
#ifdef XPG4
	if (a == NULL || re == NULL) {
		return;
	}

	if (*a == NULL) {
		*a = re;
		return;
	}

	if (*a != re) {
		regex_comp_free(&((*a)->Expbuf));
		free(*a);

		if(*a == (struct regexp *) &scanre->Expbuf[0])
			scanre = re;
		if(*a == (struct regexp *) &subre->Expbuf[0])
			subre = re;

		*a = re;
	}
#else /* !XPG4 */
	memcpy(*a, re, sizeof(struct regexp));
#endif /* XPG4 */
} 


/*
 *  Restore what is in the named alternate location to the special place
 *  re.  This means first freeing up what's currently in re, if necessary.
 */

void resre(struct regexp * a)
{
#ifdef XPG4
	if (a == NULL) {
		return;
	}

	if (re == NULL) {
		re = a;
		return;
	}

	if (a != re) {
		regex_comp_free(&re->Expbuf);
		free(re);
		re = a;
	}
#else /* !XPG4 */
	memcpy(re, a, sizeof(struct regexp));
#endif /* XPG4 */
}
