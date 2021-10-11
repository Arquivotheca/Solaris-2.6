#ifndef lint
#pragma ident "@(#)soft_version.c 1.2 96/04/23 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
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
#include "spmisoft_lib.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Public Function Prototypes */

int	swi_prod_vcmp(char *, char *);
int	swi_pkg_vcmp(char *, char *);
int	swi_is_patch(Modinfo *);
int	swi_is_patch_of(Modinfo *, Modinfo *);

/* library function */
int	pkg_fullver_cmp(Modinfo *m1, Modinfo *m2);

/* Local Function Prototypes */

static  int	prod_tokanize(char **, char *);
static  int	chk_prod_toks(char **);
static  int	pkg_tokanize(char **, char *);
static  int	chk_pkg_toks(char **);
static  int	vstrcoll(char *, char *);
static  int	is_empty(char *);
static	void	strip_trailing_blanks(char **);
static	int	patch_in_list(struct patch_num *, struct patch_num *);
#ifdef DEBUG_V
static	void	print_tokens(char **);
#endif


#define	MAX_VERSION_LEN		256
#define	MAX_TOKENS		10

#define	PROD_SUN_NAME_TOK	0
#define	PROD_SUN_VER_TOK	1
#define	PROD_SUN_IVER_TOK	2
#define	PROD_VENDOR_NAME_TOK	3
#define	PROD_VENDOR_VER_TOK	4
#define	PROD_VENDOR_IVER_TOK	5

#define	PKG_SUN_VER_TOK		0
#define	PKG_SUN_IVER_TOK	1
#define	PKG_VENDOR_NAME_TOK	2
#define	PKG_VENDOR_VER_TOK	3
#define	PKG_VENDOR_IVER_TOK	4
#define	PKG_PATCH_TOK		5


#ifdef MAIN
main(int argc, char *argv[])
{
	int 	ret;
	int	c;
	char	*str1, *str2;

	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
		(void) printf("USAGE: %s -[P(roduct)p(ackage)] ver1 ver2\n",
		    argv[0]);
		exit(1);
	}
	if (*argv[1] != '-') {
		(void) printf("USAGE: %s -[P(roduct)p(ackage)] ver1 ver2\n",
		    argv[0]);
		exit(1);
	}

	c = (*(argv[1] + 1));

	str1 = argv[2];
	str2 = argv[3];

	switch (c)
	{
	case 'P':
		ret = prod_vcmp(str1, str2);
		(void) printf("prod_vcmp return = %d\n", ret);
		break;
	case 'p':
		ret = pkg_vcmp(str1, str2);
		(void) printf("pkg_vcmp return = %d\n", ret);
		break;
	default:
		(void) printf("USAGE: %s -[P(roduct)p(ackage)] ver1 ver2\n",
		    argv[0]);
		exit(1);
	}
	exit(0);
}
#endif

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * prod_vcmp() - compare two product version strings
 *
 * Parameters:
 *	v1	- first version string
 *	v2	- second version string
 * Returns:
 *   V_EQUAL_TO - v1 and v2 are equal
 *   V_GREATER_THEN - v1 is greater than v2.
 *   V_LESS_THEN - v1 is less than v2.
 *   V_NOT_UPGRADEABLE - v1 and v2 don't have a clear order relationship.
 *	This would be the case if we were comparing, say, Cray's
 *	version of Solaris to Toshiba's version of Solaris.  Since
 *	neither of them is a descendent of the other, we can't upgrade
 *	one to the other.
 *
 * Status:
 *	public
 */
int
swi_prod_vcmp(char * v1, char * v2)
{
	int	ret, i, state;
	char	v1_buf[MAX_VERSION_LEN + 1];
	char	v2_buf[MAX_VERSION_LEN + 1];
	char	*v1_tokens[MAX_TOKENS + 2], *v2_tokens[MAX_TOKENS + 2];

	(void) memset(v1_buf, '\0', sizeof (v1_buf));
	(void) memset(v2_buf, '\0', sizeof (v2_buf));
	(void) memset(v1_tokens, '\0', sizeof (v1_tokens));
	(void) memset(v2_tokens, '\0', sizeof (v2_tokens));

	(void) strcpy(v1_buf, v1);
	if ((ret = prod_tokanize(v1_tokens, v1_buf)) < 0)
		return (ret);

	(void) strcpy(v2_buf, v2);
	if ((ret = prod_tokanize(v2_tokens, v2_buf)) < 0)
		return (ret);


#ifdef DEBUG_V
	print_tokens(v1_tokens);
	(void) printf("\n");
	print_tokens(v2_tokens);
#endif

	for (i = 0; v1_tokens[i]; i++) {
		if (is_empty(v1_tokens[i]))
			continue;

		switch (i) {
		case PROD_SUN_NAME_TOK:
			state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			if (state != V_EQUAL_TO)
				return (V_NOT_UPGRADEABLE);
			break;

		case PROD_SUN_VER_TOK:
			if (is_empty(v2_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			break;

		case PROD_SUN_IVER_TOK:
			/* Solaris_2.0.1_5.0  Solaris_2.0.1 */
			if (is_empty(v2_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			if (state == V_EQUAL_TO) {
				state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;

		case PROD_VENDOR_NAME_TOK:
			/* Solaris_2.0.1_5.0  Solaris_2.0.1_Dell_A */
			if (!is_empty(v2_tokens[PROD_SUN_IVER_TOK]))
				return (V_NOT_UPGRADEABLE);

			if (is_empty(v2_tokens[i])) {
				/* Solaris_2.0.1_Dell_A  Solaris_2.0.1 */
				if (state == V_EQUAL_TO)
					return (V_GREATER_THEN);
				i = MAX_TOKENS;
				continue;
			}
			ret = strcoll(v1_tokens[i], v2_tokens[i]);
			if (ret != 0) {
				/* Solaris_2.0.1_Soulbourne_A	Solaris_2.0.1_Dell_A */
				if (state == V_EQUAL_TO)
					return (V_NOT_UPGRADEABLE);
				/* Solaris_2.0.1_Soulbourne_A	Solaris_2.0.2_Dell_A */
				else
					return (state);
			}
			break;

		case PROD_VENDOR_VER_TOK:
			/* Solaris_2.0.1_Dell_A  Solaris_2.0.1_Dell */
			if (is_empty(v2_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			if (state == V_EQUAL_TO) {
				state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;

		case PROD_VENDOR_IVER_TOK:
			/* Solaris_2.0.1_Dell_A_1.0  Solaris_2.0.1_Dell_A */
			if (is_empty(v2_tokens[i]))
				return (V_NOT_UPGRADEABLE);

			if (state == V_EQUAL_TO) {
				state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;
		default:
			return (V_NOT_UPGRADEABLE);
		}
	}

	for (i = 0; v2_tokens[i]; i++) {

		if (is_empty(v2_tokens[i]))
			continue;
		switch (i) {
		case PROD_SUN_NAME_TOK:
		case PROD_SUN_VER_TOK:
		case PROD_SUN_IVER_TOK:
			if (is_empty(v1_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			break;

		case PROD_VENDOR_NAME_TOK:
			/* Solaris_2.0.1_Dell_A  Solaris_2.0.1_5.1 */
			if (!is_empty(v1_tokens[PROD_SUN_IVER_TOK]))
				return (V_NOT_UPGRADEABLE);
			if (is_empty(v1_tokens[i])) {
				/* Solaris_2.0.1  Solaris_2.0.1_Dell_A */
				if (state == V_EQUAL_TO)
					return (V_LESS_THEN);
				else
					i = MAX_TOKENS;
			}
			break;

		case PROD_VENDOR_VER_TOK:
			/* Solaris_2.0.1_Dell  Solaris_2.0.1_Dell_A */
			if (is_empty(v1_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			break;

		case PROD_VENDOR_IVER_TOK:
			/* Solaris_2.0.1_Dell_A  Solaris_2.0.1_Dell_A_1.0 */
			if (is_empty(v1_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			break;
		}
	}
	return (state);
}

/*
 * pkg_vcmp() - compare package versions
 *
 * Parameters:
 *	v1	- first package VERSION string
 *	v2	- second package VERSION string
 * Return:
 *   V_EQUAL_TO - v1 and v2 are equal
 *   V_GREATER_THEN - v1 is greater than v2.
 *   V_LESS_THEN - v1 is less than v2.
 *   V_NOT_UPGRADEABLE - v1 and v2 don't have a clear order relationship.
 *	This would be the case if we were comparing, say, Cray's
 *	version of a package to Toshiba's version of that package.  Since
 *	neither of them is a descendent of the other, we can't upgrade
 *	one to the other.
 *
 * Status:
 *	public
 */
int
swi_pkg_vcmp(char * v1, char * v2)
{
	int	ret, i, state;
	char	v1_buf[MAX_VERSION_LEN + 1];
	char	v2_buf[MAX_VERSION_LEN + 1];
	char	*v1_tokens[MAX_TOKENS + 2], *v2_tokens[MAX_TOKENS + 2];

	(void) memset(v1_buf, '\0', sizeof (v1_buf));
	(void) memset(v2_buf, '\0', sizeof (v2_buf));
	(void) memset(v1_tokens, '\0', sizeof (v1_tokens));
	(void) memset(v2_tokens, '\0', sizeof (v2_tokens));

	(void) strcpy(v1_buf, v1);
	if ((ret = pkg_tokanize(v1_tokens, v1_buf)) < 0)
		return (ret);

	(void) strcpy(v2_buf, v2);
	if ((ret = pkg_tokanize(v2_tokens, v2_buf)) < 0)
		return (ret);

#ifdef DEBUG_V
	print_tokens(v1_tokens);
	(void) printf("\n");
	print_tokens(v2_tokens);
#endif

	for (i = 0; v1_tokens[i]; i++) {
		if (is_empty(v1_tokens[i]))
			continue;
		switch (i) {
		case PKG_SUN_VER_TOK:
			state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			break;
		case PKG_SUN_IVER_TOK:
			if (state == V_EQUAL_TO) {
				/* 2.1.1,REV=5.0  2.1.1 */
				if (is_empty(v2_tokens[i]))
					state = vstrcoll(v1_tokens[i], "0");
				else
					state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;
		case PKG_VENDOR_NAME_TOK:
			/* 2.1.1_Dell_A  2.1.1,REV=5.0 */
			if (!is_empty(v2_tokens[PKG_SUN_IVER_TOK]))
				return (V_NOT_UPGRADEABLE);

			if (is_empty(v2_tokens[i])) {
				/* 2.1.1_Dell_A  2.1.1 */
				if (state == V_EQUAL_TO)
					return (V_NOT_UPGRADEABLE);
				i = MAX_TOKENS;
				continue;
			}
			ret = strcoll(v1_tokens[i], v2_tokens[i]);
			if (ret != 0) {
				/* 2.0.1_Soulbourne_A   2.0.1_Dell_A */
				if (state == V_EQUAL_TO)
					return (V_NOT_UPGRADEABLE);
				/* 2.0.1_Solbourne_A   2.0.2_Dell_A */
				else
					return (state);
			}
			break;

		case PKG_VENDOR_VER_TOK:
			/* 2.0.1_Dell_A  2.0.1_Dell */
			if (is_empty(v2_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			if (state == V_EQUAL_TO) {
				state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;

		case PKG_VENDOR_IVER_TOK:
			/* 2.0.1_Dell_A,REV=1.0  2.0.1_Dell_A */
			if (state == V_EQUAL_TO) {
				if (is_empty(v2_tokens[i]))
					state = vstrcoll(v1_tokens[i], "0");
				else
					state = vstrcoll(v1_tokens[i], v2_tokens[i]);
			}
			break;
		case PKG_PATCH_TOK:
			break;
		}
	}

	for (i = 0; v2_tokens[i]; i++) {
		if (is_empty(v2_tokens[i]))
			continue;

		switch (i) {
		case PKG_SUN_VER_TOK:
			break;
		case PKG_SUN_IVER_TOK:
			if (state == V_EQUAL_TO) {
				/* 2.0.1  2.0.1,REV=5.0 */
				if (is_empty(v1_tokens[i]))
					state = vstrcoll("0", v2_tokens[i]);
			}
			break;

		case PKG_VENDOR_NAME_TOK:
			/* 2.1.1,REV=5.0  2.1.1_Dell_A */
			if (!is_empty(v1_tokens[PKG_SUN_IVER_TOK]))
				return (V_NOT_UPGRADEABLE);

			if (is_empty(v1_tokens[i])) {
				/* 2.1.1  2.1.1_Dell_A */
				if (state == V_EQUAL_TO)
					return (V_NOT_UPGRADEABLE);
				i = MAX_TOKENS;
				continue;
			}
			break;

		case PKG_VENDOR_VER_TOK:
			/* 2.0.1_Dell  2.0.1_Dell_A */
			if (is_empty(v1_tokens[i]))
				return (V_NOT_UPGRADEABLE);
			break;

		case PKG_VENDOR_IVER_TOK:
			/* 2.0.1_Dell_A  2.0.1_Dell_A,REV=5.0 */
			if (is_empty(v1_tokens[i]))
				state = vstrcoll("0", v2_tokens[i]);
			break;

		case PKG_PATCH_TOK:
			break;
		}
	}
	return (state);
}

/*
 * pkg_fullver_cmp - compare the versions of two packages, comparing
 * 	both the VERSION and the PATCHLIST values.
 *
 * Parameters,
 *	m1 - modinfo struct for first package
 *	m2 - modinfo struct for second package
 *
 * Return:
 *	Assume v(m) is the VERSION string of package m and that
 *	p(m) is the patch list of m.
 *
 *	V_EQUAL_TO : v(m1) == v(m2) and p(m1) == p(m2)
 *	V_GREATER_THEN :  v(m1) > v(m2) OR (v(m1) == v(m2) AND
 *		p(m1) is a strict superset of p(m2))
 *	V_LESS_THEN :  v(m1) < v(m2) OR (v(m1) == v(m2) AND
 *		p(m1) is a strict subset of p(m2))
 *	V_NOT_UPGRADEABLE :
 *	    no order relationship exists between v(m1) and v(m2) OR
 *	    (v(m1) == v(m1) AND neither p(m1) nor p(m2) is a strict
 *	    subset of the other)
 */
int
pkg_fullver_cmp(Modinfo *m1, Modinfo *m2)
{
	int 	result;
	struct patch_num *p;
	int p1_contains_patch_not_in_p2 = FALSE;
	int p2_contains_patch_not_in_p1 = FALSE;

	result = pkg_vcmp(m1->m_version, m2->m_version);

	if (result != V_EQUAL_TO)
		return (result);

	for (p = m1->m_newarch_patches; p != NULL; p = p->next)
		if (!patch_in_list(p, m2->m_newarch_patches)) {
			p1_contains_patch_not_in_p2 = TRUE;
			break;
		}

	for (p = m2->m_newarch_patches; p != NULL; p = p->next)
		if (!patch_in_list(p, m1->m_newarch_patches)) {
			p2_contains_patch_not_in_p1 = TRUE;
			break;
		}

	if (p1_contains_patch_not_in_p2 && !p2_contains_patch_not_in_p1)
		return (V_GREATER_THEN);
	if (p2_contains_patch_not_in_p1 && !p1_contains_patch_not_in_p2)
		return (V_LESS_THEN);
	if (p2_contains_patch_not_in_p1 && p1_contains_patch_not_in_p2)
		return (V_NOT_UPGRADEABLE);
	return (V_EQUAL_TO);
}

/*
 * is_patch()
 *
 * Parameters:
 *	mi	-
 * Return:
 *	TRUE	-
 *	FALSE	-
 * Status:
 *	public
 */
int
swi_is_patch(Modinfo * mi)
{
	if (mi->m_version == NULL)
		return (FALSE);

	if (strstr(mi->m_version, "PATCH"))
		return (TRUE);

	return (FALSE);
}

/*
 * is_patch_of()
 *
 * Parameters:
 *	mi1	-
 *	mi2	-
 * Return:
 *
 * Status:
 *	public
 */
int
swi_is_patch_of(Modinfo *mi1, Modinfo *mi2)
{
	int	i;
	char    v1_buf[MAX_VERSION_LEN + 1];
	char    v2_buf[MAX_VERSION_LEN + 1];
	char    *v1_tokens[MAX_TOKENS + 2], *v2_tokens[MAX_TOKENS + 2];

	if (is_patch(mi1) == FALSE)
		return (FALSE);

	/* No patches for patches */
	if (is_patch(mi2) == TRUE)
		return (FALSE);

	(void) memset(v1_buf, '\0', sizeof (v1_buf));
	(void) memset(v2_buf, '\0', sizeof (v2_buf));
	(void) memset(v1_tokens, '\0', sizeof (v1_tokens));
	(void) memset(v2_tokens, '\0', sizeof (v2_tokens));

	(void) strcpy(v1_buf, mi1->m_version);
	if (pkg_tokanize(v1_tokens, v1_buf) < 0)
		return (FALSE);
	(void) strcpy(v2_buf, mi2->m_version);
	if (pkg_tokanize(v2_tokens, v2_buf) < 0)
		return (FALSE);

#ifdef DEBUG_V
	print_tokens(v1_tokens); printf("\n"); print_tokens(v2_tokens);
#endif

	for (i = 0; v1_tokens[i]; i++) {
		if (i == PKG_PATCH_TOK)
			break;
		if (strcoll(v1_tokens[i], v2_tokens[i]) != 0)
			return (FALSE);
	}

	/*
	 * Adding extra checking to break the assumption that all
	 * patches are installed. What this check will do is make sure
	 * that spooled packages are associated with spooled
	 * patches.
	 */
	if (mi1->m_shared != mi2->m_shared)
		return (FALSE);

	/* Also add checking to verify the the archectures match */
	if (strcmp(mi1->m_arch, mi2->m_arch) != 0)
		return (FALSE);

	return (TRUE);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * prod_tokanize()
 * Parameters:
 *	toks	-
 *	buf	-
 * Return:
 * Status:
 *	private
 */
static int
prod_tokanize(char * toks[], char buf[])
{
	static	char	*empty_str = "";
	char		*bp, *cp;
	int		len, i;

	len = (int) strlen(buf);
	if (len > MAX_VERSION_LEN)
		return (ERR_STR_TOO_LONG);

	toks[PROD_SUN_NAME_TOK] = empty_str;
	toks[PROD_SUN_VER_TOK] = empty_str;
	toks[PROD_SUN_IVER_TOK] = empty_str;
	toks[PROD_VENDOR_NAME_TOK] = empty_str;
	toks[PROD_VENDOR_VER_TOK] = empty_str;
	toks[PROD_VENDOR_IVER_TOK] = empty_str;

	bp = buf;
	if (!isalpha((unsigned)*bp))
		return (V_NOT_UPGRADEABLE);
	toks[PROD_SUN_NAME_TOK] = bp;

	for (i = 1; (cp = strchr(bp, '_')); i++) {
		*cp = '\0';
		bp = cp + 1;
		if (bp > (buf + len)) {
			return (V_NOT_UPGRADEABLE);
		}
		switch (i) {
		case PROD_SUN_VER_TOK:
			if (!isdigit((unsigned)(*bp))) {
				return (V_NOT_UPGRADEABLE);
			}
			toks[i] = bp;
			break;
		case PROD_SUN_IVER_TOK:
			if (isdigit((unsigned)(*bp))) {
				toks[i] = bp;
			} else {
				i++;
				toks[i] = bp;	/* Vendor name */
			}
			break;
		case PROD_VENDOR_NAME_TOK:
			if (isdigit((unsigned)(*bp))) {
				return (V_NOT_UPGRADEABLE);
			}
			toks[i] = bp;
			break;
		case PROD_VENDOR_VER_TOK:
			if (!isalpha((unsigned)(*bp))) {
				return (V_NOT_UPGRADEABLE);
			}
			toks[i] = bp;
			break;

		case PROD_VENDOR_IVER_TOK:
			if (!isdigit((unsigned)(*bp))) {
				return (V_NOT_UPGRADEABLE);
			}
			toks[i] = bp;
			break;
		}
	}

	if (i < 2)
		return (V_NOT_UPGRADEABLE);

	for (i = 0; toks[i]; i++) {
		cp = toks[i];
		while (*cp) {
			if (isalpha((unsigned) *cp)) {
				*cp = toupper((unsigned)*cp);
			}
			cp++;
		}
	}
	strip_trailing_blanks(toks);

	return (chk_prod_toks(toks));
}

/*
 * chk_prod_toks()
 *
 * Parameters:
 *	toks	-
 * Return:
 *
 * Status:
 *	private
 */
static int
chk_prod_toks(char *toks[])
{
	int	i;
	char	*cp;

	for (i = 0; toks[i]; i++) {
		if (*toks[i] == '\0')
			continue;

		switch (i) {
		case PROD_SUN_NAME_TOK:
			if ((strcoll(toks[i], "SOLARIS") == 0))
				break;
			else
				return (V_NOT_UPGRADEABLE);

		case PROD_SUN_VER_TOK:
		case PROD_SUN_IVER_TOK:
		case PROD_VENDOR_IVER_TOK:
			for (cp = toks[i]; *cp; cp++) {
				if (*cp == '.')
					continue;
				if (!isdigit((unsigned)(*cp)))
					return (V_NOT_UPGRADEABLE);
			}
			break;
		case PROD_VENDOR_NAME_TOK:
		case PROD_VENDOR_VER_TOK:
			for (cp = toks[i]; *cp; cp++) {
				if (!isalpha((unsigned)(*cp)))
					return (V_NOT_UPGRADEABLE);
			}
			break;
		}
	}
	return (0);
}

/*
 * pkg_tokanize()
 * Parameters:
 *	toks	-
 *	buf	-
 * Return:
 *
 * Status:
 *	private
 */
static int
pkg_tokanize(char * toks[], char buf[])
{
	static	char *empty_str = "";
	char	*bp, *cp;
	int	i, len;

	toks[PKG_SUN_VER_TOK] = empty_str;
	toks[PKG_SUN_IVER_TOK] = empty_str;
	toks[PKG_VENDOR_NAME_TOK] = empty_str;
	toks[PKG_VENDOR_VER_TOK] = empty_str;
	toks[PKG_VENDOR_IVER_TOK] = empty_str;
	toks[PKG_PATCH_TOK] = empty_str;

	len = (int) strlen(buf);
	if (len > MAX_VERSION_LEN)
		return (ERR_STR_TOO_LONG);

	bp = buf;
	if (!isdigit((unsigned)(*bp)))
		return (V_NOT_UPGRADEABLE);
	else
		toks[PKG_SUN_VER_TOK] = bp;

	cp = strchr(bp, '_');
	if (cp != NULL) {
		*cp = '\0';
		bp = cp + 1;
		if (!isalpha((unsigned)(*bp))) {
			return (V_NOT_UPGRADEABLE);
		}
		toks[PKG_VENDOR_NAME_TOK] = bp;

		cp = strchr(bp, '_');
		if (cp == NULL) {
			return (V_NOT_UPGRADEABLE);
		}
		*cp = '\0';
		bp = cp + 1;
		if (!isalpha((unsigned)(*bp))) {
			return (V_NOT_UPGRADEABLE);
		}
		toks[PKG_VENDOR_VER_TOK] = bp;
	}

	cp = strchr(bp, ',');
	if (cp != NULL) {
		*cp = '\0';
		bp = cp + 1;
		while (isspace((unsigned)*bp)) bp++;
		if (strncmp(bp, "REV=", 4) == 0) {
			bp += 4;
			if (!isalnum((unsigned)*bp))
				return (V_NOT_UPGRADEABLE);

			if (*toks[PKG_VENDOR_NAME_TOK] == '\0')
				toks[PKG_SUN_IVER_TOK] = bp;
			else
				toks[PKG_VENDOR_IVER_TOK] = bp;
			while (*bp) {
				if (isspace((unsigned)*bp)) {
					*bp = '\0';
					break;
				}
				if (*bp == ',') {
					*bp = '\0';
					break;
				}
				bp++;
			}
			bp++;
			while (isspace((unsigned)*bp)) bp++;
		}
		cp = strstr(bp, "PATCH=");
		if (cp != NULL) {
			bp = cp + 6;
			if (!isalnum((unsigned)*bp))
				return (V_NOT_UPGRADEABLE);
			toks[PKG_PATCH_TOK] = bp;
		}
	}

	for (i = 0; toks[i]; i++) {
		cp = toks[i];
		while (*cp) {
			if (isalpha((unsigned) *cp)) {
				*cp = toupper((unsigned)*cp);
			}
			cp++;
		}
	}
	strip_trailing_blanks(toks);

	return (chk_pkg_toks(toks));
}

/*
 * chk_pkg_toks()
 *
 * Parameters:
 *	toks	-
 * Return:
 *
 * Status:
 *	private
 */
static int
chk_pkg_toks(char *toks[])
{
	char	*cp;
	int	i;

	for (i = 0; toks[i]; i++) {

		if (*toks[i] == '\0')
			continue;
		switch (i) {
		case PKG_SUN_VER_TOK:
		case PKG_SUN_IVER_TOK:
		case PKG_VENDOR_IVER_TOK:
			for (cp = toks[i]; *cp; cp++) {
				if (*cp == '.')
					continue;
				if (!isdigit((unsigned)(*cp)))
					return (V_NOT_UPGRADEABLE);
			}
			break;

		case PKG_VENDOR_NAME_TOK:
		case PKG_VENDOR_VER_TOK:
			for (cp = toks[i]; *cp; cp++) {
				if (!isalpha((unsigned)(*cp)))
					return (V_NOT_UPGRADEABLE);
			}
			break;

		case PKG_PATCH_TOK:
			for (cp = toks[i]; *cp; cp++) {
				if (*cp == '-')
					continue;
				if (!isalnum((unsigned)(*cp)))
					return (V_NOT_UPGRADEABLE);
			}
		}
	}
	return (0);
}

/*
 * vstrcoll()
 *
 * Parameters:
 *	s1	-
 *	s2	-
 * Return:
 *
 * Status:
 *	private
 */
static int
vstrcoll(char *s1, char *s2) {
	int 	ret, i;
	int 	s1num[20], s2num[20];
	char	*cp_beg, *cp_end;

	if (isalpha((u_char)*s1)) {
		ret = strcoll(s1, s2);
		if (ret < 0)
			return (V_LESS_THEN);
		else if (ret > 0)
			return (V_GREATER_THEN);

		return (V_EQUAL_TO);
	}

	for (i = 0; i < 20; i++) {
		s1num[i] = -1;
		s2num[i] = -1;
	}

	i = 0; cp_beg = s1; cp_end = s1;
	while (cp_beg != NULL) {
		cp_end = strchr(cp_beg, '.');
		if (cp_end != NULL) {
			*cp_end = '\0';
			cp_end++;
		}
		s1num[i++] = atoi(cp_beg);
		cp_beg = cp_end;
	}

	i = 0; cp_beg = s2; cp_end = s2;
	while (cp_beg != NULL) {
		cp_end = strchr(cp_beg, '.');
		if (cp_end != NULL) {
			*cp_end = '\0';
			cp_end++;
		}
		s2num[i++] = atoi(cp_beg);
		cp_beg = cp_end;
	}

	for (i = 0; s1num[i] != -1; i++) {
		if (s2num[i] == -1) break;
		if (s1num[i] > s2num[i])
			return (V_GREATER_THEN);
		if (s1num[i] < s2num[i])
			return (V_LESS_THEN);
	}

	if ((s1num[i] != -1) || (s2num[i] != -1)) {
		if (s1num[i] == -1) {
			while (s2num[i] == 0) i++;
			if (s2num[i] != -1)
				return (V_LESS_THEN);
		} else {
			while (s1num[i] == 0) i++;
			if (s1num[i] != -1)
				return (V_GREATER_THEN);
		}
	}
	return (V_EQUAL_TO);
}

/*
 * strip_trailing_blanks()
 *
 * Parameters:
 *	toks	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
strip_trailing_blanks(char * toks[])
{
	int	i;
	char	*cp;

	for (i = 0; toks[i]; i++) {
		if (is_empty(toks[i]))
			continue;
		cp = toks[i] + (strlen(toks[i]) - 1);
		if (isspace((unsigned)*cp)) {
			while (isspace((unsigned)*cp)) cp--;
			cp++;
			*cp = '\0';
		}
	}
	return;
}

/*
 * is_empty()
 *
 * Parameters:
 *	cp	-
 * Return:
 *	0	-
 *	1	-
 * Status:
 *	private
 */
static int
is_empty(char * cp)
{
	if (*cp == '\0')
		return (1);
	return (0);
}

static int
patch_in_list(struct patch_num *p, struct patch_num *plist)
{
	while (plist) {
		if (strcmp(p->patch_num_id, plist->patch_num_id) == 0 &&
		    p->patch_num_rev <= plist->patch_num_rev)
			return (TRUE);
		plist = plist->next;
	}
	return (FALSE);
}

#ifdef DEBUG_V
static void
print_tokens(char *toks[])
{
	int i;

	for (i = 0; toks[i]; i++) {
		(void) printf("%s\n", toks[i]);
	}
}
#endif
