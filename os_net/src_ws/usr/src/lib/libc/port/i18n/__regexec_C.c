/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)__regexec_C.c 1.6	96/07/15  SMI"

/*
 * COMPONENT_NAME: (LIBCPAT) Standard C Library Pattern Functions
 *
 * FUNCTIONS: __regexec_C
 *
 * ORIGINS: 27
 *
 * This module contains IBM CONFIDENTIAL code. -- (IBM
 * Confidential Restricted when combined with the aggregated
 * modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 *
 * (C) COPYRIGHT International Business Machines Corp. 1991, 1992
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "reglocal.h"
#include <regex.h>
#include "patlocal.h"



/*
 * External data defined in regexec()
 */

extern int	__reg_bits[];	/* bitmask for [] bitmap */


/*
 * Internal definitions
 */

typedef struct {	/* subexpression addresses */
	uchar_t	*sa;	/* start subexpression address */
	uchar_t	*ea;	/* end subexpression address + 1 */
} ea_t;

typedef	struct {		/* structure for data during recursion */
	int	flags;			/* combined cflags and eflags */
	unsigned wander;	/* flag: OK to wander */
	uchar_t	*string;	/* ptr to start of string */
	uchar_t	*pend_string;	/* ptr to byte beyond match in string */
	uchar_t *start_match;	/* ^ to beginning of match */
	_LC_collate_t *pcoltbl;	/* ptr to collation methods table */
	int	submatch[_REG_SUBEXP_MAX+1]; /* # subexp matches */
	ea_t	tmatch[_REG_SUBEXP_MAX+1]; /* subexp addresses */
} EXEC;

typedef	struct {	/* bitmap recursion data */
	int	bit_max;	/* max allowed [] matches */
	int	bit_count;	/* number of [] matches	*/
} BIT;


/*
 * Internal function prototypes
 */

/* match entire pattern against string */
static int	match_re_C(
			uchar_t *,
			uchar_t *,
			regex_t	*,
			EXEC	*);

/* match ILS [] against string */
static	int	match_bit_C(
			uchar_t *,
			uchar_t *,
			regex_t	*,
			EXEC	*,
			BIT	*);

/* match ILS . against string */
static	int	match_dot_C(
			uchar_t *,
			uchar_t *,
			regex_t	*,
			EXEC	*,
			BIT	*);

/*
 * __regexec_C() - Determine if RE pattern matches string
 *		   - valid for only C locale
 *
 *		   - hdl	ptr to __lc_collate table
 *		   - preg	ptr to structure with compiled pattern
 *		   - string	string to be matched
 *		   - nmatch	# of matches to report in pmatch
 *		   - pmatch	reported subexpression offsets
 *		   - eflags	regexec() flags
 */

int
__regexec_C(_LC_collate_t *hdl, const regex_t *preg, const char *string,
	size_t nmatch, regmatch_t pmatch[], int eflags)
{
	EXEC	e;		/* initial data passing structure	*/
	int	i;		/* loop index				*/
	uchar_t	*pmap;		/* ptr to character map table		*/
	uchar_t	*pstr;		/* ptr to next string byte		*/
	int	stat;		/* match_re() return status		*/
	static const EXEC zero;	/* faster than bzero() call		*/

/*
 * Return error if RE pattern is undefined
 */
	if (preg->re_comp == NULL)
		return (REG_BADPAT);

/*
 * optimisation:
 *	if the pattern doesn't start with "^",
 *	trim off prefix of pstr that has no hope of matching.  If we
 *	exhaust pstr via this method, we may already be a wiener!
 */
	pmap = (uchar_t *)preg->re_sc->re_map;
	pstr = (uchar_t *) string;

	if (*(uchar_t *) preg->re_comp != CC_BOL) {
		while (*pstr && pmap[*pstr] == 0)
			++pstr;

		if (!*pstr && !pmap[0])
			return (REG_NOMATCH);
	}

/*
 * Initialize data recursion buffer
 */
	e = zero;
	e.flags = preg->re_cflags | eflags;
	e.pcoltbl = hdl;
	e.string = (uchar_t *)string;
	e.wander = 1;
	e.start_match = pstr;
/*
 * Attempt to match entire compiled RE pattern starting at current
 *     position in string
 */
	stat = match_re_C((uchar_t *)preg->re_comp, pstr, (regex_t *)preg, &e);

/*
 * Return offsets of entire pattern match
 * Return subexpression offsets, zero-length changed to -1
 */
	if (stat == 0) {
		pstr = e.start_match;
		if (nmatch > 0 && (preg->re_cflags & REG_NOSUB) == 0) {
			pmatch[0].rm_sp = (char *) pstr;
			pmatch[0].rm_ep = (char *) e.pend_string;
			pmatch[0].rm_so = pstr - (uchar_t *)string;
			pmatch[0].rm_eo = e.pend_string - (uchar_t *)string;
			for (i = 1; i < nmatch && i <= _REG_SUBEXP_MAX; i++) {
				if (e.tmatch[i].sa != NULL) {
					pmatch[i].rm_sp =
						(char *)e.tmatch[i].sa;
					pmatch[i].rm_ep =
						(char *)e.tmatch[i].ea;
					pmatch[i].rm_so = e.tmatch[i].sa -
						(uchar_t *)string;
					pmatch[i].rm_eo = e.tmatch[i].ea -
						(uchar_t *)string;
				} else {
					pmatch[i].rm_sp = NULL;
					pmatch[i].rm_ep = NULL;
					pmatch[i].rm_so = (off_t)-1;
					pmatch[i].rm_eo = (off_t)-1;
				}
			}
		}
	} else if (stat == REG_EBOL)
		return (REG_NOMATCH);
	return (stat);
}


/*
 * match_re()	- Match entire RE pattern to string
 *		- Note: CC_codes are in specific order to promote
 *		-       performance.  Do not change without proof
 *		-       that performance is improved - fms 07/23/91
 *			(Sorry `fms'. The compiler generates a jump
 *			 table for the CC_codes switch, so your comments
 *			 about "promoting performance" are misleading)
 *
 *		- ppat		ptr to pattern
 *		- pstr		ptr to string
 *		- preg		ptr to caller's regex_t structure
 *		- pe		ptr to recursion data structure
 */

static int
match_re_C(uchar_t *ppat, uchar_t *pstr, regex_t *preg, EXEC *pe)
{
	uchar_t	*best_alt;	/* best alternative pend_string		*/
	size_t	count;		/* # bytes to backtrack each time	*/
	size_t	count2;		/* ignore case backreference counter	*/
	int	cp;		/* pattern character			*/
	int	cp2;		/* opposite case pattern character	*/
	int	cs;		/* string character			*/
	int	idx;		/* subexpression index			*/
	int	max;		/* maximum repetition count - min	*/
	int	min;		/* minimum repetition count		*/
	uchar_t	*pback;		/* ptr to subexpression backreference	*/
	uchar_t	*pea;		/* ptr to subexpression end address	*/
	uchar_t	*psa;		/* ptr to subexpression start address	*/
	uchar_t	*pstop;		/* ptr to backtracking string point	*/
	uchar_t	*ptemp;		/* ptr to string during backreference	*/
	uchar_t	*sav_pat;	/* saved pattern			*/
	uchar_t	*sav_str;	/* saved string				*/
	uchar_t	*pmap;		/* ptr to character map table		*/
	int	stat;		/* match_re() recursive status		*/
	int	wclen;		/* # bytes in character			*/
	int	wander;		/* copy of EXEC.wander			*/
	int	wc_p;
	int	wc_s;
	EXEC	r;		/* another copy of *pe for recursion	*/
	EXEC	rbest;		/* best alternative recursion data	*/

	wander = pe->wander;
	pmap = preg->re_sc->re_map;
	sav_pat = ppat;
	sav_str = pstr;
	pe->wander = 0;

	if (0) {
	    no_match:
		/*
		 * NOTE: the only way to come here is via a goto.
		 */
		if (wander) {
			/*
			 * we come here if we fail to match, and ok to wander
			 * down the string looking for a match.
			 *	- restore the pattern to the start
			 *	- restore string to one past where we left off
			 *	  and trim unmatchables
			 */
			if (*sav_str == '\0')
				return (REG_NOMATCH);

			ppat = sav_pat;		/* restore patterm	*/
			pstr = sav_str + 1;

			while (*pstr && pmap[*pstr] == 0)
				++pstr;

		/*
		 * If at end of string, and it isn't possible for '$'
		 * to start an expression, then no match.  It is possible
		 * for '$' to start an expression as in "x*$", since
		 * "x*$" is equivalent to "$" when 0 x's precede the end
		 * of line, so we have to check one more time to see if
		 * the pattern matches the empty string (i.e. empty string
		 * is equivalent to end of line).  Note that this way,
		 * "yx*$" won't match "", but "x*$" will.
		 */
			if (*pstr == 0 && !pmap[0])
				return (REG_NOMATCH);

			pe->start_match = sav_str = pstr;
		} else
			return (REG_NOMATCH);
	}

/*
 * Perform each compiled RE pattern code until end-of-pattern or non-match
 * Break to bottom of loop to match remaining pattern/string when extra
 *   expressions have been matched
 */
	while (1) {
		count = 1;
		switch (*ppat++) {
/*
 * a single character, no repetition
 *   continue if pattern character matches next string character
 *   otherwise return no match
 */

		case CC_CHAR:
			if (*ppat != *pstr)
				goto no_match;
			ppat++;
			pstr++;
			continue;
/*
 * any single character, no repetition
 *   continue if next string character is anything but <nul>
 *   otherwise return no match
 */

		case CC_DOT:
			if (*pstr++ != '\0')
				continue;
			return (REG_NOMATCH);
/*
 * end-of-pattern
 *   update forward progress of matched location in string
 *   return success
 */

		case CC_EOP:
			pe->pend_string = pstr;
			return (0);
/*
 * bracket expression, no repetition
 *   continue if next string character has bit set in bitmap
 *   otherwise return no match
 */

		case CC_BITMAP:
			cs = *pstr++;
			if ((*(ppat + (cs >> 3)) & __reg_bits[cs & 7]) != 0) {
				ppat += BITMAP_LEN;
				continue;
			}
			goto no_match;
/*
 * character string, no repetition
 * single multibyte character, no repetition
 *   continue if next n pattern characters matches next n string characters
 *   otherwise return no match
 */

		case CC_STRING:
		case CC_WCHAR:
			count = *ppat++;
			do
				if (*ppat++ != *pstr++)
					goto no_match;
			while (--count > 0);
			continue;
/*
 * end subexpression, no repetition
 *   save subexpression ending address
 *   continue in all cases
 */

		case CC_SUBEXP_E:
			idx = *ppat++;
			pe->tmatch[idx].ea = pstr;
			continue;
/*
 * subexpression backreference, no repetition
 *   continue if next n string characters matches what was previously
 *     matched by the referenced subexpression
 *   otherwise return no match
 */
		case CC_BACKREF:
			idx = *ppat++;
			pback = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			while (pback < pea)
				if (*pback++ != *pstr++)
					goto no_match;
			continue;
/*
 * begin subexpression
 *   generate new copy of recursion data
 *   preserve subexpression starting address
 *   match remaining pattern against remaining string
 *   if remaining pattern match succeeds, update recursion data with
 *   new copy and return success
 *   if remaining pattern match fails and zero length subexpression is ok,
 *   continue with pattern immediately following CC_SUBEXP_E
 *   otherwise return fatal error
 */

		case CC_SUBEXP:
			idx = *ppat++;
			r = *pe;
			r.tmatch[idx].sa = pstr;
			stat = match_re_C(ppat, pstr, preg, &r);
			if (stat == 0) {
				*pe = r;
				return (0);
			}
			if (((cp2 = (*(uchar_t *)preg->re_sc->re_esub[idx] &
				CR_MASK)) == CR_QUESTION || cp2 == CR_STAR) ||
				((cp2 == CR_INTERVAL ||
				cp2 == CR_INTERVAL_ALL) &&
				*(((uchar_t *)preg->re_sc->re_esub[idx])+1)
				== 0)) {
				ppat = preg->re_sc->re_esub[idx];
				if ((*ppat != (CC_SUBEXP_E | CR_INTERVAL)) &&
					(*ppat !=
					(CC_SUBEXP_E | CR_INTERVAL_ALL)))
					ppat += 2;
				else
					ppat += 4;
				continue;
			}
			goto no_match;
/*
 * any single ILS character, no repetition
 *   continue if next string character is anything but <nul>
 *     or <newline> and REG_NEWLINE is set
 *   otherwise return no match
 */

		case CC_WDOT:
			if (*pstr == 0 || (*pstr == '\n' &&
				(pe->flags & REG_NEWLINE) != 0))
				goto no_match;
			pstr++;
			continue;
/*
 * ILS bracket expression, no repetition
 *   if ignoring case, get lowercase version of collating element
 *   continue if next string collating element has bit set in bitmap
 *   otherwise return no match
 */

		case CC_WBITMAP:
		{
			wchar_t	delta;	/* character offset into bitmap */
			wchar_t	ucoll;	/* unique collation weight */

			ucoll = (wchar_t)*pstr;
			ptemp = pstr + 1;
			if (ucoll >= preg->re_sc->re_ucoll[0] &&
				ucoll <= preg->re_sc->re_ucoll[1]) {
				delta = ucoll - preg->re_sc->re_ucoll[0];
				if ((*(ppat + (delta >> 3)) &
					__reg_bits[delta & 7]) != 0) {
					pstr = ptemp;
					ppat += ((preg->re_sc->re_ucoll[1] -
						preg->re_sc->re_ucoll[0]) /
						NBBY) + 1;
					continue;
				}
			}
			goto no_match;
		}
/*
 * beginning-of-line anchor
 *   REG_NEWLINE allows ^ to match null string following a newline
 *   REG_NOTBOL means first character is not beginning of line
 *
 *   REG_NOTBOL   REG_NEWLINE   at BOL   ACTION
 *   ----------   -----------   ------   -------------------------
 *        N            N           Y     continue
 *        N            N           N     return REG_EBOL
 *        N            Y           Y     continue
 *        N            Y           N     continue if \n, else return REG_NOMATCH
 *        Y            N           Y     return REG_EBOL
 *        Y            N           N     return REG_EBOL
 *        Y            Y           Y     continue if \n, else return REG_NOMATCH
 *        Y            Y           N     continue if \n, else return REG_NOMATCH
 */

		case CC_BOL:
			if ((pe->flags & REG_NOTBOL) == 0) {
				if (pstr == pe->string)
					continue;
				else if ((pe->flags & REG_NEWLINE) == 0)
					return (REG_EBOL);
			} else if ((pe->flags & REG_NEWLINE) == 0)
				return (REG_EBOL);
			if (pstr > pe->string && *(pstr-1) == '\n')
				continue;
			goto no_match;
/*
 * end-of-line anchor
 *   REG_NEWLINE allows $ to match null string preceeding a newline
 *   REG_NOTEOL means last character is not end of line
 *
 *   REG_NOTEOL   REG_NEWLINE   at EOL   ACTION
 *   ----------   -----------   ------   --------------------------
 *        N            N           Y     continue
 *        N            N           N     return REG_NOMATCH
 *        N            Y           Y     continue
 *        N            Y           N     continue if \n, else return REG_NOMATCH
 *        Y            N           Y     return REG_NOMATCH
 *        Y            N           N     return REG_NOMATCH
 *        Y            Y           Y     continue if \n, else return REG_NOMATCH
 *        Y            Y           N     continue if \n, else return REG_NOMATCH
 */

		case CC_EOL:
			if ((pe->flags & REG_NOTEOL) == 0) {
				if (*pstr == '\0')
					continue;
				else if ((pe->flags & REG_NEWLINE) == 0)
					goto no_match;
			} else if ((pe->flags & REG_NEWLINE) == 0)
				goto no_match;
			if (*pstr == '\n')
				continue;
			goto no_match;

/*
 * CC_WORDB has been added to support word boundaries (ie, \< and \>
 * for ex) to the AIX implementation. *ppat has '<' or '>' to
 * distinguish beginning or end of a word. The OSF/1 ex/vi has its own
 * regular expression so it doesn't need to invoke the regex API.
 */
		case CC_WORDB: {
			uchar_t c;

			if (*ppat++ == (uchar_t) '<') {
				if (pstr == pe->string) {
					if ((pe->flags & REG_NOTBOL) == 0)
						continue;
					else
						goto no_match;
				}
				c = *(pstr - 1);
				if (!IS_WORDCHAR(c) && IS_WORDCHAR(*pstr))
					continue;
			} else {
				if (pstr == pe->string)
					goto no_match;
				c = *(pstr - 1);
				if (!IS_WORDCHAR(*pstr) && IS_WORDCHAR(c))
					continue;
			}
		}
			goto no_match;
/*
 * start alternative
 *   try each alternate
 *   select best alternative or the one which gets to EOP first
 */

		case CC_ALTERNATE:
			best_alt = NULL;
			do {
				idx = *ppat++ << 8;
				idx += *ppat++;
				r = *pe;
				stat = match_re_C(ppat, pstr, preg, &r);
				if (stat == 0 && best_alt < r.pend_string) {
					if (*r.pend_string == '\0') {
						*pe = r;
						return (0);
					}
					best_alt = r.pend_string;
					rbest = r;
				}
				if (idx == 0)
					break;
				ppat += idx + 1;
			} while (1);
			if (best_alt != NULL) {
				*pe = rbest;
				return (0);
			}
			goto no_match;
/*
 * any single character except <newline>, no repetition
 *   continue if next string character is anything but <nul>
 *     or <newline> and REG_NEWLINE is set
 *   otherwise return no match
 */

		case CC_DOTREG:
			if (*pstr == 0 || (*pstr++ == '\n' &&
				(pe->flags & REG_NEWLINE) != 0))
				goto no_match;
			continue;
/*
 * end alternative
 *   skip over any other alternative patterns and continue matching
 *     pattern to string
 */

		case CC_ALTERNATE_E:
			idx = *ppat++;
			ppat = preg->re_sc->re_esub[idx];
			continue;
/*
 * invalid compiled RE code
 *   return fatal error
 */

		default:
			return (REG_BADPAT);
/*
 * ignore case single character, no repetition
 *   continue if next string character matches pattern character or
 *     opposite case of pattern character
 *   otherwise return no match
 */

		case CC_I_CHAR:
			if (*ppat++ == *pstr) {
				ppat++;
				pstr++;
				continue;
			}
			if (*ppat++ == *pstr++)
				continue;
			goto no_match;
/*
 * ignore case character string, no repetition
 * ignore case single multibyte character, no repetition
 *   continue if next n string characters match next n pattern characters or
 *     opposite case of next n pattern characters
 *   otherwise return no match
 */

		case CC_I_STRING:
			count = *ppat++;
			do {
				if (*ppat++ == *pstr) {
					ppat++;
					pstr++;
				} else if (*ppat++ != *pstr++)
					goto no_match;
			} while (--count > 0);
			continue;
/*
 * ignore case subexpression backreference, no repetition
 *   continue if next n string characters matches what was previously
 *     matched by the referenced subexpression
 *   otherwise return no match
 */

		case CC_I_BACKREF:
			idx = *ppat++;
			pback = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			while (pback < pea) {
				if ((*pback != *pstr) &&
					(tolower(*pback) != tolower(*pstr)))
					goto no_match;
				pback++;
				pstr++;
			}
			continue;
/*
 * ignore case single ILS character, no repetition
 *   continue if next n string characters match next n pattern characters or
 *     opposite case of next n pattern characters
 */

		case CC_I_WCHAR:
			count = *ppat++;
			if (strncmp((const char *)ppat,
				(const char *)pstr, count) != 0)
				if (strncmp((const char *)(ppat+count),
					(const char *)pstr, count) != 0)
					goto no_match;
			ppat += count * 2;
			pstr += count;
			continue;

/*
 * ignore case ILS subexpression backreference, no repetition
 *   continue if next n string characters or their opposite case matches
 *     what was previosly matched by the referenced subexpression
 *   otherwise return no match
 */

		case CC_I_WBACKREF:
			idx = *ppat++;
			pback = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			while (pback < pea) {
				wc_p = (int)*pback;
				wc_s = (int)*pstr;
				if ((wc_p != wc_s) &&
					(tolower(wc_p) != tolower(wc_s)))
					goto no_match;
				pback++;
				pstr++;
			}
			continue;
/*
 * ignore case ILS subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_WBACKREF processing
 */

		case CC_I_WBACKREF | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_iwbackref;
/*
 * ignore case ILS subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_WBACKREF processing
 */

		case CC_I_WBACKREF | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_iwbackref;
/*
 * ignore case ILS subexpression backreference, zero or more occurances "*"
 *   define min/max and jump to common CC_I_WBACKREF processing
 */

		case CC_I_WBACKREF | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_iwbackref;
/*
 * ignore case ILS subexpression backreference - variable number of matches
 *   continue if subexpression match was zero-length
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_iwbackref:
			idx = *ppat++;
			psa = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			count = pea - psa;
			if (count == 0)
				continue;
			while (min-- > 0) {
				pback = psa;
				while (pback < pea) {
					wc_p = (int)*pback;
					wc_s = (int)*pstr;
					if ((wc_p != wc_s) && (tolower(wc_p) !=
						tolower(wc_s)))
						goto no_match;
					pback++;
					pstr++;
				}
			}
			pstop = pstr;
			while (max-- > 0) {
				pback = psa;
				while (pback < pea) {
					wc_p = (int)*pback;
					wc_s = (int)*pstr;
					if ((wc_p != wc_s) && (tolower(wc_p) !=
						tolower(wc_s)))
						goto no_match;
					pback++;
					pstr++;
				}
				if (pback < pea)
					break;
			}
			break;
/*
 * ignore case subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_BACKREF processing
 */

		case CC_I_BACKREF | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_ibackref;
/*
 * ignore case subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_BACKREF processing
 */

		case CC_I_BACKREF | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_ibackref;
/*
 * ignore case subexpression backreference, zero or more occurances "*"
 *   define min/max and jump to common CC_I_BACKREF processing
 */

		case CC_I_BACKREF | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_ibackref;
/*
 * ignore case subexpression backreference - variable number of matches
 *   continue if subexpression match was zero-length
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_ibackref:
			idx = *ppat++;
			psa = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			count = pea - psa;
			if (count == 0)
				continue;
			while (min-- > 0) {
				pback = psa;
				while (pback < pea) {
					if ((*pback !=  *pstr) &&
						(tolower(*pback) !=
						tolower(*pback)))
						goto no_match;
					pback++;
					pstr++;
				}
			}
			pstop = pstr;
			while (max-- > 0) {
				pback = psa;
				ptemp = pstr;
				count2 = count;
				do {
					if ((*pback !=  *pstr) &&
						(tolower(*pback) !=
						tolower(*pback)))
						break;
					pback++;
					pstr++;
				} while (--count2 > 0);
				if (count2 == 0)
					pstr = ptemp;
				else
					break;
			}
			break;
/*
 * ignore case single multibyte character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_WCHAR processing
 */

		case CC_I_WCHAR | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_iwchar;
/*
 * ignore case single multibyte character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_WCHAR processing
 */

		case CC_I_WCHAR | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_iwchar;
/*
 * ignore case single multibyte character, one or more occurances "+"
 *   define min/max and jump to common CC_I_WCHAR processing
 */

		case CC_I_WCHAR | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_iwchar;
/*
 * ignore case single multibyte character, zero or one occurances "?"
 *   define min/max and jump to common CC_I_WCHAR processing
 */

		case CC_I_WCHAR | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_iwchar;
/*
 * ignore case single multibyte character, zero or more occurances "*"
 *   define min/max and jump to common CC_I_WCHAR processing
 */

		case CC_I_WCHAR | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_iwchar;
/*
 * ignore case single multibyte character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_iwchar:
			count = *ppat++;
			while (min-- > 0) {
				if (strncmp((const char *)ppat,
					(const char *)pstr, count) != 0)
					if (strncmp((const char *)(ppat+count),
						(const char *)pstr, count) != 0)
						goto no_match;
				pstr += count;
			}
			pstop = pstr;
			while (max-- > 0) {
				if (strncmp((const char *)ppat,
					(const char *)pstr, count) != 0)
					if (strncmp((const char *)(ppat+count),
						(const char *)pstr, count) != 0)
						break;
				pstr += count;
			}
			ppat += count * 2;
			break;
/*
 * ignore case single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_CHAR processing
 */

		case CC_I_CHAR | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_ichar;
/*
 * ignore case single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_I_CHAR processing
 */

		case CC_I_CHAR | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_ichar;
/*
 * ignore case single character, one or more occurances "+"
 *   define min/max and jump to common CC_I_CHAR processing
 */

		case CC_I_CHAR | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_ichar;
/*
 * ignore case single character, zero or one occurances "?"
 *   define min/max and jump to common CC_I_CHAR processing
 */

		case CC_I_CHAR | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_ichar;
/*
 * ignore case single character, zero or more occurances "*"
 *   define min/max and jump to common CC_I_CHAR processing
 */

		case CC_I_CHAR | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_ichar;
/*
 * ignore case single character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_ichar:
			cp = *ppat++;
			cp2 = *ppat++;
			while (min-- > 0) {
				if (cp != *pstr && cp2 != *pstr)
					goto no_match;
				pstr++;
			}
			pstop = pstr;
			while (max-- > 0 && (cp == *pstr || cp2 == *pstr))
				pstr++;
			break;
/*
 * any single character except <newline> , min/max occurances "{m,n}"
 *   define min/max and jump to common CC_DOTREG processing
 */

		case CC_DOTREG | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_dotreg;
/*
 * any single character except <newline> , min/max occurances "{m,n}"
 *   define min/max and jump to common CC_DOTREG processing
 */

		case CC_DOTREG | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_dotreg;
/*
 * any single character except <newline>, one or more occurances "+"
 *   define min/max and jump to common CC_DOTREG processing
 */

		case CC_DOTREG | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_dotreg;
/*
 * any single character except <newline>, zero or one occurances "?"
 *   define min/max and jump to common CC_DOTREG processing
 */

		case CC_DOTREG | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_dotreg;
/*
 * any single character except <newline>, zero or more occurances "*"
 *   define min/max and jump to common CC_DOTREG processing
 */

		case CC_DOTREG | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_dotreg;
/*
 * any single character except <newline> - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_dotreg:
			while (min-- > 0)
				if (*pstr == '\0' || (*pstr++ == '\n' &&
					(pe->flags & REG_NEWLINE) != 0))
					return (REG_NOMATCH);
			pstop = pstr;
			while (max-- > 0)
				if (*pstr == '\0')
					break;
				else if (*pstr++ == '\n' &&
					(pe->flags & REG_NEWLINE) != 0) {
					pstr--;
					break;
				}
			break;
/*
 * ILS bracket expression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WBITMAP processing
 */

		case CC_WBITMAP | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_wbitmap;
/*
 * ILS bracket expression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WBITMAP processing
 */

		case CC_WBITMAP | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_wbitmap;
/*
 * ILS bracket expression, one or more occurances "+"
 *   define min/max and jump to common CC_WBITMAP processing
 */

		case CC_WBITMAP | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_wbitmap;
/*
 * ILS bracket expression, zero or one occurances "?"
 *   define min/max and jump to common CC_WBITMAP processing
 */

		case CC_WBITMAP | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_wbitmap;
/*
 * ILS bracket expression, zero or more occurances "*"
 *   define min/max and jump to common CC_WBITMAP processing
 */

		case CC_WBITMAP | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_wbitmap;
/*
 * ILS bracket expression - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   call match_bit to match remaining pattern
 */

cc_wbitmap:
		{
			BIT	b;		/* bitmap recursion data */

			while (min-- > 0) {
				/* character offset into bitmap */
				wchar_t	delta;
				/* unique collation weight */
				wchar_t	ucoll;

				ucoll = (wchar_t)*pstr;
				ptemp = pstr + 1;
				if (ucoll >= preg->re_sc->re_ucoll[0] &&
					ucoll <= preg->re_sc->re_ucoll[1]) {
					delta = ucoll -
						preg->re_sc->re_ucoll[0];
					if ((*(ppat + (delta >> 3)) &
						__reg_bits[delta & 7]) == 0)
						goto no_match;
					pstr = ptemp;
				} else
					goto no_match;
			}
			b.bit_count = 0;
			b.bit_max = max;
			if (match_bit_C(ppat, pstr, preg, pe, &b))
				goto no_match;
			else
				return (0);
		}
/*
 * any single ILS character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WDOT processing
 */

		case CC_WDOT | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_wdot;
/*
 * any single ILS character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WDOT processing
 */

		case CC_WDOT | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_wdot;
/*
 * any single ILS character, one or more occurances "+"
 *   define min/max and jump to common CC_WDOT processing
 */

		case CC_WDOT | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_wdot;
/*
 * any single ILS character, zero or one occurances "?"
 *   define min/max and jump to common CC_WDOT processing
 */

		case CC_WDOT | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_wdot;
/*
 * any single ILS character, zero or more occurances "*"
 *   define min/max and jump to common CC_WDOT processing
 */

		case CC_WDOT | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_wdot;
/*
 * any single ILS character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   call match_dot to match remaining pattern
 */

cc_wdot:
		{
			BIT	b;		/* period recursion data */

			while (min-- > 0) {
				if (*pstr == '\0' || (*pstr == '\n' &&
					(pe->flags & REG_NEWLINE) != 0))
					return (REG_NOMATCH);
				pstr++;
			}
			b.bit_count = 0;
			b.bit_max = max;
			if (match_dot_C(ppat, pstr, preg, pe, &b))
				goto no_match;
			else
				return (0);
		}
/*
 * single multibyte character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WCHAR processing
 */

		case CC_WCHAR | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_wchar;
/*
 * single multibyte character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_WCHAR processing
 */

		case CC_WCHAR | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_wchar;
/*
 * single multibyte character, one or more occurances "+"
 *   define min/max and jump to common CC_WCHAR processing
 */

		case CC_WCHAR | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_wchar;
/*
 * single multibyte character, zero or one occurances "?"
 *   define min/max and jump to common CC_WCHAR processing
 */

		case CC_WCHAR | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_wchar;
/*
 * single multibyte character, zero or more occurances "*"
 *   define min/max and jump to common CC_WCHAR processing
 */

		case CC_WCHAR | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_wchar;
/*
 * single multibyte character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_wchar:
			count = *ppat++;
			while (min-- > 0)
				for (wclen = 0, ptemp = ppat;
					wclen < count; wclen++)
					if (*ptemp++ != *pstr++)
						goto no_match;
			pstop = pstr;
			while (max-- > 0) {
				for (wclen = 0, ptemp = ppat, psa = pstr;
					wclen < count; wclen++)
					if (*ptemp++ != *psa++)
						break;
				if (wclen < count)
					break;
				else
					pstr += count;
			}
			ppat += count;
			break;
/*
 * end subexpression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_SUBEXP_E processing
 */

		case CC_SUBEXP_E | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_subexpe;
/*
 * end subexpression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_SUBEXP_E processing
 */

		case CC_SUBEXP_E | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - min - 1;
			goto cc_subexpe;
/*
 * end subexpression, one or more occurances "+"
 *   define min/max and jump to common CC_SUBEXP_E processing
 */

		case CC_SUBEXP_E | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_subexpe;
/*
 * end subexpression, zero or one occurances "?"
 *   define min/max and jump to common CC_SUBEXP_E processing
 */

		case CC_SUBEXP_E | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_subexpe;
/*
 * end subexpression, zero or more occurances "*"
 *   define min/max and jump to common CC_SUBEXP_E processing
 */

		case CC_SUBEXP_E | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_subexpe;
/*
 * end subexpression - variable number of matches
 *   save subexpression ending address
 *   if zero-length match, continue with remaining pattern if
 *     at or below minimum # of required matches
 *     otherwise return an error so that the last previous string
 *     matching locations can be used
 *   increment # of subexpression matches
 *   if the maximum # of required matches have not been found,
 *     reexecute the subexpression
 *     if it succeeds or fails without reaching the minimum # of matches
 *       return with the appropriate status
 *   if maximum number of matches found or the last match_re() failed and
 *     the minimum # of matches have been found, continue matching the
 *     remaining pattern against the remaining string
 */

cc_subexpe:
			idx = *ppat++;
			pe->tmatch[idx].ea = pstr;
			if (pe->tmatch[idx].ea == pe->tmatch[idx].sa)
				if (pe->submatch[idx] < min)
					continue;
				else
					goto no_match;
			pe->submatch[idx]++;
			if (pe->submatch[idx] < min + max) {
				r = *pe;
				stat = match_re_C(
					(uchar_t *)preg->re_sc->re_lsub[idx],
					pstr, preg, &r);
				if (stat != REG_NOMATCH ||
					pe->submatch[idx] < min) {
					if (stat == 0)
						*pe = r;
					return (stat);
				}
			}
			continue;
/*
 * subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_BACKREF processing
 */

		case CC_BACKREF | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_backref;
/*
 * subexpression backreference, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_BACKREF processing
 */

		case CC_BACKREF | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX -1;
			goto cc_backref;
/*
 * subexpression backreference, zero or more occurances "*"
 *   define min/max and jump to common CC_BACKREF processing
 */

		case CC_BACKREF | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_backref;
/*
 * subexpression backreference - variable number of matches
 *   continue if subexpression match was zero-length
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_backref:
			idx = *ppat++;
			psa = pe->tmatch[idx].sa;
			pea = pe->tmatch[idx].ea;
			count = pea - psa;
			if (count == 0)
				continue;
			while (min-- > 0) {
				pback = psa;
				while (pback < pea)
					if (*pback++ != *pstr++)
						goto no_match;
			}
			pstop = pstr;
			while (max-- > 0) {
				if (strncmp((const char *)psa,
					(const char *)pstr, count) != 0)
					break;
				pstr += count;
			}
			break;
/*
 * bracket expression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_BITMAP processing
 */

		case CC_BITMAP | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_bitmap;
/*
 * bracket expression, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_BITMAP processing
 */

		case CC_BITMAP | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_bitmap;
/*
 * bracket expression, one or more occurances "+"
 *   define min/max and jump to common CC_BITMAP processing
 */

		case CC_BITMAP | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_bitmap;
/*
 * bracket expression, zero or one occurances "?"
 *   define min/max and jump to common CC_BITMAP processing
 */

		case CC_BITMAP | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_bitmap;
/*
 * bracket expression, zero or more occurances "*"
 *   define min/max and jump to common CC_BITMAP processing
 */

		case CC_BITMAP | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_bitmap;
/*
 * bracket expression - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_bitmap:
			while (min-- > 0) {
				cs = *pstr++;
				if ((*(ppat + (cs >> 3)) &
					__reg_bits[cs & 7]) == 0)
					goto no_match;
			}
			pstop = pstr;
			while (max-- > 0) {
				cs = *pstr;
				if ((*(ppat + (cs >> 3)) &
					__reg_bits[cs & 7]) != 0)
					pstr++;
				else
					break;
			}
			ppat += BITMAP_LEN;
			break;
/*
 * any single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_DOT processing
 */

		case CC_DOT | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_dot;
/*
 * any single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_DOT processing
 */

		case CC_DOT | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_dot;
/*
 * any single character, one or more occurances "+"
 *   define min/max and jump to common CC_DOT processing
 */

		case CC_DOT | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_dot;
/*
 * any single character, zero or one occurances "?"
 *   define min/max and jump to common CC_DOT processing
 */

		case CC_DOT | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_dot;
/*
 * any single character, zero or more occurances "*"
 *   define min/max and jump to common CC_DOT processing
 */

		case CC_DOT | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_dot;
/*
 * any single character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_dot:
			while (min-- > 0)
				if (*pstr++ == '\0')
					return (REG_NOMATCH);
			pstop = pstr;
			while (max-- > 0)
				if (*pstr++ == '\0') {
					pstr--;
					break;
				}
			break;
/*
 * single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_CHAR processing
 */

		case CC_CHAR | CR_INTERVAL:
			min = *ppat++;
			max = *ppat++;
			goto cc_char;
/*
 * single character, min/max occurances "{m,n}"
 *   define min/max and jump to common CC_CHAR processing
 */

		case CC_CHAR | CR_INTERVAL_ALL:
			min = *ppat++;
			ppat++;
			max = INT_MAX - 1;
			goto cc_char;
/*
 * single character, one or more occurances "+"
 *   define min/max and jump to common CC_CHAR processing
 */

		case CC_CHAR | CR_PLUS:
			min = 1;
			max = INT_MAX - 1;
			goto cc_char;
/*
 * single character, zero or one occurances "?"
 *   define min/max and jump to common CC_CHAR processing
 */

		case CC_CHAR | CR_QUESTION:
			min = 0;
			max = 1;
			goto cc_char;
/*
 * single character, zero or more occurances "*"
 *   define min/max and jump to common CC_CHAR processing
 */

		case CC_CHAR | CR_STAR:
			min = 0;
			max = INT_MAX - 1;
			goto cc_char;
/*
 * single character - variable number of matches
 *   match minimum number of required times
 *     return no match if cannot meet minimum count
 *   save new string position for backtracking
 *   match maximum number of required times, where max is
 *     number of remaining matches
 *   break-out to match remaining pattern/string
 */

cc_char:
			cp = *ppat++;
			while (min-- > 0)
				if (cp != *pstr++)
					goto no_match;
			pstop = pstr;
			while (max-- > 0 && cp == *pstr)
				pstr++;
			break;
		} /* switch */
		break;
	} /* while */
/*
 * surplus matched expressions end up here
 * generate new copy of recursion data
 * match remaining pattern against remaining string
 * if remaining pattern match fails, forfeit one extra matched
 *   character and try again until no spare matches are left
 * return success and new recursion data if entire remaining pattern matches
 * otherwise return no match
 */
	while (1) {
		r = *pe;
		stat = match_re_C(ppat, pstr, preg, &r);
		if (stat != REG_NOMATCH) {
			if (stat == 0)
				*pe = r;
			return (stat);
		}
		if (pstr <= pstop)
			break;
		pstr -= count;
	}
	goto no_match;
}


/*
 * match_dot_C()	- Match period to remainder of string
 *		- used for C locale
 *
 *		- ppat		ptr to pattern
 *		- pstr		ptr to string
 *		- preg		ptr to caller's regex_t structure
 *		- pe		ptr to recursion data structure
 *		- pb		ptr to recursion period data
 */

static int
match_dot_C(uchar_t *ppat, uchar_t *pstr, regex_t *preg, EXEC *pe, BIT *pb)
{
	int	i;		/* # bytes in character			*/
	EXEC	r;		/* another copy of *pe for recursion	*/

/*
 * Attempt another . match if maximum not reached yet
 * If successful, attempt next match via recursion
 */

	if (*pstr != '\0' && (*pstr != '\n' ||
		(pe->flags & REG_NEWLINE) == 0) &&
		pb->bit_count < pb->bit_max) {
		pb->bit_count++;
		if (match_dot_C(ppat, pstr + 1, preg, pe, pb) == 0)
			return (0);
	}
	r = *pe;
	if ((i = match_re_C(ppat, pstr, preg, &r)) == 0)
		*pe = r;
	return (i);
}


/*
 * match_bit_C()	- Match bracket expression [] to remainder of string
 *		- used for C locale
 *
 *		- ppat		ptr to pattern
 *		- pstr		ptr to string
 *		- preg		ptr to caller's regex_t structure
 *		- pe		ptr to recursion data structure
 *		- pb		ptr to recursion bitmap data
 */

static int
match_bit_C(uchar_t *ppat, uchar_t *pstr, regex_t *preg, EXEC *pe, BIT *pb)
{
	wchar_t	delta;		/* character offset into bitmap		*/
	wchar_t	ucoll;		/* unique collation weight		*/
	uchar_t	*pnext;		/* ptr to next collation symbol		*/
	int	stat;		/* match_re() return status		*/
	EXEC	r;		/* another copy of *pe for recursion	*/

/*
 * Attempt another [] match if maximum not reached yet
 * If successful, attempt next match via recursion
 */
	if (*pstr != '\0' && pb->bit_count < pb->bit_max) {
		ucoll = (wchar_t)*pstr;
		pnext = pstr + 1;
		if (ucoll >= preg->re_sc->re_ucoll[0] &&
			ucoll <= preg->re_sc->re_ucoll[1]) {
			delta = ucoll - preg->re_sc->re_ucoll[0];
			if ((*(ppat + (delta >> 3)) &
				__reg_bits[delta & 7]) != 0) {
				pb->bit_count++;
				if (match_bit_C(ppat, pnext, preg, pe, pb) == 0)
					return (0);
			}
		}
	}
	r = *pe;
	ppat += ((preg->re_sc->re_ucoll[1] - preg->re_sc->re_ucoll[0]) /
		NBBY) + 1;
	if ((stat = match_re_C(ppat, pstr, preg, &r)) == 0)
		*pe = r;
	return (stat);
}
