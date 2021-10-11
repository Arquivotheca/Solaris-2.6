/*LINTLIBRARY*/
/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma	ident	"@(#)ui_format.c 1.4 94/06/16"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#ifdef USE_XPG4_WCS
#define WCSWIDTH(x)		wcswidth(x)
#define WCSLEN(x)		wcslen(x)
#define WCSCAT(x, y)		wcscat(x, y)
#define WCSCPY(x, y)		wcscpy(x, y)
#else
#define WCSWIDTH(x)		wscol(x)
#define WCSLEN(x)		wslen(x)
#define WCSCAT(x, y)		wscat(x, y)
#define WCSCPY(x, y)		wscpy(x, y)
#endif

static	wchar_t	*fmt(wchar_t *, int);
static	int	any(wchar_t, wchar_t	*);
static	void	prefix(wchar_t *);
static	void	split(wchar_t *);
static	void	pack(wchar_t *);
static	void	oflush(void);
static	void	tabulate(wchar_t *);
static	void	leadin(void);
static	int	 ws_segment_col(wchar_t *, wchar_t *);

extern	void	*xmalloc(size_t);

/*
 * Routine to format messages.
 * The only public routine in this file is format_text() which has
 * mb string as input parameter and return value (an array). 
 * However, internally wchar_t's are used for ease of string operations.
 */

/*
 * format_text(string, width) formats the string according the width by
 * returning an array of pointers with each array element points to
 * the beginning of a new line.
 * For i18n level 4 work, we convert the string to wide characters
 * (wchar_t) and use wchar_t as the internal structure for the rest
 * of this file, i.e. fmt() and other functions.  After fmt() returns
 * back to format_text(), we convert wchar_t string back to mb string
 * and set up the return array.
 */
char **
format_text(
	char	*string,	/* message to format */
	int	width)		/* desired indentation of formatted text */
{
	int n;
	char *s;
	char **return_array;
	wchar_t		*wcsptr, wcs_string[BUFSIZ];
	char		str_buffer[BUFSIZ];

	/*
	 * Format the message.
	 * The formatted text is returned in a new buffer.
	 */

	/* convert the string to wchar_t string before calling fmt() */
	(void) mbstowcs(wcs_string, string, BUFSIZ);
	wcsptr = fmt(wcs_string, width);

	/* now formating is done, convert the wchar_t string back to mb */
	(void) wcstombs(str_buffer, wcsptr, BUFSIZ);
	string = str_buffer;

	/*
	 * count all the newlines so we know how much to
	 * allocate
	 */

	n = 0;
	s = string;
	while (*s) {
		if (*s++ == '\n')
			n++;
	}

	return_array = (char **)xmalloc((n + 1) * sizeof (char *));

	/*
	 * now go down the string and whenever a newline
	 * is found, it becomes a null to terminate the current line
	 * and the next character is pointed to as the beginning of the
	 * next line
	 */

	n = 0;
	s = string;
	return_array[n++] = s;	/* first one */

	while (*s) {
		if (*s == '\n') {
			*s++ = '\0';
			return_array[n++] = s;
		} else
			s++;
	}

	return_array[--n] = NULL;

	return (return_array);

}




/*
 * BELOW HERE IS AN ADAPTATION OF THE TEXT FORMATTER USED BY MAIL
 * AND VI, MODIFIED TO DO I/O ON STRINGS, RATHER THAN ON FILES.
 * Maybe some older version of mail and vi, I couldn't locate
 * even similar sections in Solaris 2.3.  (Ray Cheng)
 */




/*
 * Is ch any of the characters in str?
 */

static int
any(wchar_t ch, wchar_t *str)
{
	register wchar_t *f;
	register wchar_t c;

	f = str;
	c = ch;
	while (*f)
		if (c == *f++)
			return (1);
	return (0);
}



/*
 * fmt -- format the concatenation of strings
 */

/* string equivalents for stdin, stdout */
static	wchar_t *sstdout;
static	int  _soutptr;
#define	sgetc(fi)	(_ptr < (int)WCSLEN(fi) ? fi[_ptr++] : EOF)
#define	sputchar(c)	sstdout[_soutptr++] = c
#define	sputc(c, f)	sputchar(c)


#define	NOSTR	((wchar_t *) 0)	/* Null string pointer for lint */

static	wchar_t	outbuf[BUFSIZ];	/* Sandbagged output line image */
static	wchar_t	*outp;		/* Pointer in above */
static	int	filler;		/* Filler amount in outbuf */

static	int	pfx;		/* Current leading blank count */
static	int	lineno;		/* Current input line */
static	int	nojoin = 1;	/* split lines only, don't join */
				/* short ones */
static	int	width = 80;	/* Width that we will not exceed */

enum crown_type	{c_none, c_reset, c_head, c_lead, c_fixup, c_body};
static	enum crown_type	crown_state;	/* Crown margin state */
static	int	crown_head;		/* The header offset */
static	int	crown_body;		/* The body offset */



/*
 * Drive the whole formatter by managing input string.  Also,
 * cause initialization of the output stuff and flush it out
 * at the end.
 */


/*
 * Read up characters from the passed input string, forming lines,
 * doing ^H processing, expanding tabs, stripping trailing blanks,
 * and sending each line down for analysis.
 */

static wchar_t *
fmt(wchar_t *fi, int cols)
{
	wchar_t linebuf[BUFSIZ], canonb[BUFSIZ];
	register wchar_t *cp, *cp2;
	register int c, col;
	register int _ptr = 0;
	wchar_t		wcstmp[4];

	width = cols;
	if (width > cols)
		width = 80;


	/* open format buffer */
	sstdout = (wchar_t *)xmalloc(BUFSIZ * sizeof(wchar_t));
	sstdout[0] = (wchar_t)0;
	_soutptr = 0;

	c = (int)sgetc(fi);
	while (c != EOF) {

		/*
		 * Collect a line, doing ^H processing.
		 * Leave tabs for now.
		 */

		cp = linebuf;
		while (c != '\n' && c != EOF && cp-linebuf < BUFSIZ-1) {
			if (c == '\b') {
				if (cp > linebuf)
					cp--;
				c = (int)sgetc(fi);
				continue;
			}
			if (!iswprint(c) && c != '\t') {
				c = (int)sgetc(fi);
				continue;
			}
			*cp++ = (wchar_t)c;
			c = (int)sgetc(fi);
		}
		*cp = (wchar_t)'\0';

		/*
		 * Toss anything remaining on the input line.
		 */

		while (c != '\n' && c != EOF)
			c = (int)sgetc(fi);

		/*
		 * Expand tabs on the way to canonb.
		 */

		col = 0;
		cp = linebuf;
		cp2 = canonb;
		while ((c = *cp++) != NULL) {
			if (c != (wchar_t)'\t') {
				col++;
				if (cp2-canonb < BUFSIZ-1)
					*cp2++ = (wchar_t)c;
				continue;
			}
			do {
				if (cp2-canonb < BUFSIZ-1)
					*cp2++ = (wchar_t)' ';
				col++;
			} while ((col & 07) != 0);
		}

		/*
		 * Swipe trailing blanks from the line.
		 */

		for (cp2--; cp2 >= canonb && *cp2 == (wchar_t)' '; cp2--)
			;
		*++cp2 = (wchar_t)'\0';
		prefix(canonb);
		if (c != EOF)
			c = (int)sgetc(fi);
	}

	/*
	 * If anything partial line image left over,
	 * send it out now.
	 */
	if (outp) {
		*outp = (wchar_t)'\0';
		outp = NOSTR;
		(void) WCSCAT(sstdout, outbuf);
		(void) mbstowcs(wcstmp, "\n", 1);
		(void) WCSCAT(sstdout, wcstmp);
		outbuf[0] = (wchar_t)'\0';
	}

	return (sstdout);
}

/*
 * Take a line devoid of tabs and other garbage and determine its
 * blank prefix.  If the indent changes, call for a linebreak.
 * If the input line is blank, echo the blank line on the output.
 * Finally, if the line minus the prefix is a mail header, try to keep
 * it on a line by itself.
 */

static void
prefix(wchar_t line[])
{
	register wchar_t *cp;
	register int np, h = 0;

	if (WCSLEN(line) == 0) {
		oflush();
		sputchar((wchar_t)'\n');
		sstdout[_soutptr] = (wchar_t)'\0';
		if (crown_state != c_none)
			crown_state = c_reset;
		return;
	}
	for (cp = line; *cp == (wchar_t)' '; cp++)
		;
	np = cp - line;

	/*
	 * The following horrible expression attempts to avoid linebreaks
	 * when the indent changes due to a paragraph.
	 */

	if (crown_state == c_none && np != pfx && (np > pfx || abs(pfx-np) > 8))
		oflush();
	if (nojoin) {
		h = 1;
		oflush();
	}
	if (!h && (h = (*cp == (wchar_t)'.')))
		oflush();
	pfx = np;
	switch (crown_state) {
	case c_reset:
		crown_head = pfx;
		crown_state = c_head;
		break;
	case c_lead:
		crown_body = pfx;
		crown_state = c_body;
		break;
	case c_fixup:
		crown_body = pfx;
		crown_state = c_body;
		if (outp) {
			wchar_t s[BUFSIZ];

			*outp = (wchar_t)'\0';
			(void) WCSCPY(s, &outbuf[crown_head]);
			outp = NOSTR;
			split(s);
		}
	}
	split(cp);
	if (h)
		oflush();
	lineno++;
}

/*
 * Split up the passed line into output "words" which are
 * maximal strings of non-blanks with the blank separation
 * attached at the end.  Pass these words along to the output
 * line packer.
 */

static void
split(wchar_t line[])
{
	register wchar_t *cp, *cp2;
	wchar_t word[BUFSIZ];
	wchar_t wcstmp[10];

	cp = line;
	while (*cp) {
		cp2 = word;

		/*
		 * Collect a 'word,' allowing it to contain escaped
		 * white space.
		 */

		while (*cp && !iswspace(*cp)) {
			if (*cp == (wchar_t)'\\' && iswspace(cp[1]))
				*cp2++ = *cp++;
			*cp2++ = *cp++;
		}

		/*
		 * Guarantee a space at end of line.
		 * Two spaces after end of sentence punctuation.
		 */

		if (*cp == (wchar_t)'\0' && !iswspace(cp[-1])) {
			*cp2++ = (wchar_t)' ';
			(void) mbstowcs(wcstmp, ".:!?", 4);
			if (any(cp[-1], wcstmp))
				*cp2++ = (wchar_t)' ';
		}
		while (iswspace(*cp))
			*cp2++ = *cp++;
		*cp2 = (wchar_t)'\0';
		pack(word);
	}
}

/*
 * Output section.
 * Build up line images from the words passed in.  Prefix
 * each line with correct number of blanks.  The buffer "outbuf"
 * contains the current partial line image, including prefixed blanks.
 * "outp" points to the next available space therein.  When outp is NOSTR,
 * there ain't nothing in there yet.  At the bottom of this whole mess,
 * leading tabs are reinserted.
 */


/*
 * Pack a word onto the output line.  If this is the beginning of
 * the line, push on the appropriately-sized string of blanks first.
 * If the word won't fit on the current line, flush and begin a new
 * line.  If the word is too long to fit all by itself on a line,
 * just give it its own and hope for the best.
 */

static void
pack(wchar_t word[])
{
	register wchar_t *cp;
	register int s, t, w; /* in units of columns */

	if (outp == NOSTR)
		leadin();
	t = WCSWIDTH(word);
	s = ws_segment_col(outbuf, outp);
	if (t+s <= width) {

		/*
		 * In like flint!
		 */

		for (cp = word; *cp; *outp++ = *cp++)
			;
		return;
	}
	if (t <= width-filler) {
		/* 
		 * fits by itself so start new line
		 * Fillers are ' ' and occupies one columne per wchar_t.
		 */
		oflush();
		leadin();
		s = ws_segment_col(outbuf, outp);
	}
	w = width - (s + 1);			/* allow for '-' at end */
	for (cp = word, t = 1; *cp; *outp++ = *cp++, t++) {
		if (t > w && cp[1]) {
			*outp++ = (wchar_t)'-';	/* hyphenate line */
			oflush();		/* start new line */
			leadin();		/* add leading white space */
			t = 1;			/* reset count */
			s = ws_segment_col(outbuf, outp);
			w = width - (s + 1);
		}
	}
}

/*
 * If there is anything on the current output line, send it on
 * its way.  Set outp to NOSTR to indicate the absence of the current
 * line prefix.
 */

static void
oflush(void)
{
	if (outp == NOSTR)
		return;
	*outp = (wchar_t)'\0';
	tabulate(outbuf);
	outp = NOSTR;
}

/*
 * Take the passed line buffer, insert leading tabs where possible, and
 * output on standard output (finally).
 */

static void
tabulate(wchar_t line[])
{
	register wchar_t *cp;
	register int b, t;
	wchar_t		wcstmp[10];

	/*
	 * Toss trailing blanks in the output line.
	 */

	cp = line + WCSLEN(line) - 1;
	while (cp >= line && iswspace(*cp))
		cp--;

	/*
	 * Add a single blank after anything that
	 * might be a user prompt.
	 */

	(void) mbstowcs(wcstmp, ":?", 2);
	if (any(*cp++, wcstmp))
		*cp++ = (wchar_t)' ';
	*cp = '\0';

	/*
	 * Count the leading blank space and tabulate.
	 */

	for (cp = line; iswspace(*cp); cp++)
		;
	b = cp-line;
	t = b >> 3;
	b &= 07;
	if (t > 0)
		do
			sputc((wchar_t)'\t', stdout);
		while (--t);
	if (b > 0)
		do
			sputc((wchar_t)' ', stdout);
		while (--b);
	while (*cp)
		sputc(*cp++, stdout);
	sputc((wchar_t)'\n', stdout);
	sstdout[_soutptr] = (wchar_t)'\0';
}

/*
 * Initialize the output line with the appropriate number of
 * leading blanks.
 */

static void
leadin(void)
{
	register int b;
	register wchar_t *cp;
	register int l;

	switch (crown_state) {
	case c_head:
		l = crown_head;
		crown_state = c_lead;
		break;

	case c_lead:
	case c_fixup:
		l = crown_head;
		crown_state = c_fixup;
		break;

	case c_body:
		l = crown_body;
		break;

	default:
		l = pfx;
		break;
	}
	filler = l;
	for (b = 0, cp = outbuf; b < l; b++)
		*cp++ = (wchar_t)' ';
	outp = cp;
}

/*
 * ws_segment_col(wchar_t *ws, wchar_t *p) returns the number of columns
 * of the wchar_t string from ws to p.  p may or may not points to
 * (wchar_t *)NULL.  We save *p, replace it with NULL, measure columns,
 * and restore *p.
 */
static int
ws_segment_col(wchar_t *ws, wchar_t *p)
{
	wchar_t		saved_wc = *p;
	int		col;

	*p = (wchar_t)NULL;
	col = WCSWIDTH(ws);
	*p = saved_wc;
	return (col);
}
