#ifndef lint
#ident   "@(#)xgetsh.c 1.4 93/12/16 SMI"
#endif				/* lint */
/*
 * xgetsh is a version of xgettext which works for shell scripts.
 *
 * The -m and -d parameters work the same as for xgettext.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "xgetsh.h"
#include "nhash.h"

#define	USAGE	"Usage: %s [-m string] [ -d domainname ] fname [fname]\n"

static FILE *open_file(char *);
static void chk_len(char []);
static void do_file(FILE *fp);
static void set_ofp();
static int is_continuation(char *);
static int is_private_function(char *);

static FILE *ofp;
static char *domainname = "messages";
static char *msgstrtag = NULL;
static char buf[MAX_LEN], string[MAX_LEN];

/* ncmsg.c */
extern int	cmsg(const char *msgid);

main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	FILE *fp;
	int c;

	while ((c = getopt(argc, argv, "d:m:")) != EOF) {
		switch (c) {
		case 'd':
			domainname = optarg;
			break;
		case 'm':
			msgstrtag = optarg;
			break;
		case '?':
			(void) fprintf(stderr, "Unknown option\n");
			exit(1);
		}
	}

	if (argv[optind] == NULL) {
		(void) fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	set_ofp();

	(void) fprintf(ofp, "domain \"%s\"\n", domainname);

	for (; optind < argc; optind++) {
		fp = open_file(argv[optind]);
		do_file(fp);
	}

	exit(0);
	/*NOTREACHED*/
#ifdef lint
	return (0);
#endif	/* lint */
}

static void
do_file(FILE *fp)
{
	char			*cp;
	char			*begin;
	char			ch;
	int			continuation = 0;
	int			quotes_found = 0;
	int			i;
	int			j;

	while (fgets(buf, MAX_LEN, fp)) {
		chk_len(buf);

		cp = buf;
		if (*cp == '#')
			continue;

		if (!continuation) {
			/*
			 * Look for gettext.
			 */
			while (strncmp(cp, "gettext", 7) != 0) {
				cp++;
				if (!*cp) break;
			}
			if (!*cp)
				continue;
		}

		if (is_private_function(cp))
			continue;

		continuation = 0;
		if (quotes_found == 0) {
			/*
			 * Find first quote after gettext.
			 */
			while (*cp && *cp != '"' && *cp != '\'') {
				if (is_continuation(cp)) {
					continuation = 1;
					break;
				}
				cp++;
			}
			if (!*cp) {
				(void) fprintf(stderr,
				    "Cannot find quote after gettext():\n");
				(void) fprintf(stderr, "%s\n", buf);
				continue;
			}
			if (continuation)
				continue;

			/*
			 * We have our first quote.
			 */
			ch = *cp;
			quotes_found = 1;
			cp++;
			if (!*cp) {
				(void) fprintf(stderr,
				    "String ended unexpectedly: %s\n", buf);
				continue;
			}

			/*
			 * Remember the start of our string.
			 */
			begin = cp;
		}
		/*
		 * Look for second quote.
		 */
		while (*cp) {
			/*
			 * Allow backslash quotes in the string.
			 */
			if (*cp == '\\') {
				cp++;
				if (cp && *cp && *cp == ch) {
					cp++;
				} else
					cp--;
			}
			if (is_continuation(cp)) {
				/*
				 * Remove backslash and newline
				 */
				buf[cp - buf] = '\0';
				continuation = 1;
				break;
			}
			/*
			 * Is this our second quote.
			 */
			if (*cp == ch) {
				*cp = '\0';
				(void) strcat(string, begin);
				quotes_found = 2; continuation = 0;
				break;
			}
			cp++;
		}
		if (continuation) {
			(void) strcat(string, begin);
			begin = buf;
			continue;
		}
		if (quotes_found != 2) {
			(void) fprintf(stderr, "Missing matching quote:\n");
			(void) fprintf(stderr, "%s\n", buf);
			continuation = 0;
			quotes_found = 0;
			continue;
		}


		for (i = 0; i < (int) strlen(string); i++) {
			if (string[i] == '"' && i > 0 && string[i-1] != '\\') {
				string[strlen(string) + 1] = '\0';
				for (j = strlen(string); j > i; j--)
					string[j] = string[j - 1];
				string[i] = '\\';
			}
		}

		if (!cmsg(string)) {
			(void) fprintf(ofp, "msgid  \"%s\"\n", string);
			if (msgstrtag) {
				(void) fprintf(ofp, "msgstr \"%s%s\"\n",
				    msgstrtag, string);
			} else
				(void) fprintf(ofp, "msgstr\n");
		} else {
			(void) fprintf(ofp, "# msgid  \"%s\"\n", string);
			if (msgstrtag) {
				(void) fprintf(ofp, "# msgstr \"%s%s\"\n",
				    msgstrtag, string);
			} else
				(void) fprintf(ofp, "# msgstr\n");
		}
		(void) memset(string, '\0', MAX_LEN);
		(void) memset(buf, '\0', MAX_LEN);
		quotes_found = 0;
	}
}

static FILE *
open_file(char *fname)
{
	FILE *fp;

	if (access(fname, F_OK) < 0) {
		(void) fprintf(stderr, "Can't access %s\n", fname);
		exit(1);
	}

	if ((fp = fopen(fname, "r")) == NULL) {
		(void) fprintf(stderr, "fopen() failed for %s !\n", fname);
		exit(1);
	}
	return (fp);
}

static void
chk_len(char buf[])
{
	if (buf[MAX_LEN - 2]) {
		(void) fprintf(stderr,
		    "Error: string exceeds limit of %d characters:\n", MAX_LEN);
		(void) fprintf(stderr, "%s\n", buf);
		exit(1);
	}
}

static int
is_continuation(char *cp)
{
	if (*cp == '\\' && *(cp+1) == '\n')
		return (1);
	return (0);
}

static void
set_ofp()
{
	char ofile[256];

	(void) sprintf(ofile, "%s.po", domainname);
	if ((ofp = fopen(ofile, "w")) == NULL) {
		(void) fprintf(stderr, "fopen() failed for %s !\n", ofile);
		exit(1);
	}
}

/*
 * Is this a private version of gettext(). Used in scripts designed to work both
 * under 4.x and 5.0. Look for () before quotes.
 */
static int
is_private_function(char *cp)
{
	while (*cp) {
		if (*cp == '\'' || *cp == '"')
			return (0);
		if (*(cp+1) == '\0')
			return (0);
		if ((*cp == '(') && (*(cp+1) == ')'))
			return (1);
		cp++;
	}
	return (0);
}
