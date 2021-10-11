#ifndef lint
#ident   "@(#)buildhist.c 1.13 96/08/01 SMI"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "spmisoft_api.h"
#include "spmicommon_api.h"

#define	VER_LOW		1
#define	VER_HIGH	2

#define TOK_BUF_SIZE		256

#define	MAX_DELETED_FILES	100

/*
	Possible return codes.
	HISTORY_FILE_OK		Normal Return Code.
	EXCESS_DELETED_FILES	One or more packages had more than
				MAX_DELETED_FILES filed deleted from
				the old to the new versions. This return
				code indicates a warning condition, not
				a fatal error condition.
*/

#define	HISTORY_FILE_OK		0
#define	EXCESS_DELETED_FILES	1
#define	INSUFFICIENT_MEMORY	3
#define	INVALID_ENTRY		4

typedef struct hist {
	struct hist  *hist_next;
	char	*pkg_abbr;
	char	*ver_low;
	char	*ver_high;
	char	*arch;
	char	*replaced_by;
	char	*deleted_files;
	char	*cluster_rm_list;
	char	*ignore_list;
	int	to_be_removed;
	int	needs_pkgrm;
	int	prev_num_deletions;
} PkgHist;

struct excess_del_ok {
	struct excess_del_ok *next;
	char	*ed_pkg_abbr;
	char	*ed_arch;
};

/* local prototypes */

static void	parse_pkg_entry(char *, PkgHist **, int);
static char	*set_token_value(char *, char *);
static char	*tok_value(char *);
static char	*split_ver(char *, int);
static void	usage(void);
static int	dump_pkg_hist(PkgHist *, char *);
static int	entry_before(PkgHist *, PkgHist *);
static void	read_pkg_history_file(char *, PkgHist **, int);
static void	merge_pkg_histlist(PkgHist **, PkgHist *);
static void	insert_in_list(PkgHist *, PkgHist **);
static void	delete_from_list(char *, char *, PkgHist **);
static void	add_removed_files(PkgHist **, PkgHist **);
static void	create_new_ignore_entries(PkgHist **, PkgHist *);
static char	*trimCopy(char *);
static void	memfail(void);
static int	count_entries(char *);
static void	free_ph(PkgHist *);
static char	*chopstring(char **, int, int *);

/* global variables */

static PkgHist	*old_pkg_history = NULL;
static PkgHist	*new_pkg_history = NULL;
static PkgHist	*removed_files = NULL;
static PkgHist	*new_pkg_id = NULL;
static struct excess_del_ok *del_ok = NULL;
static char	*gtoksbuf;
static u_int	gtoksbuf_size;
static char	*glinebuf;
static int	warning_state = HISTORY_FILE_OK;
static int	BadHistoryRecord;		/* This record is bad */
static int	BadHistoryRecordS;		/* Count of bad records */

#if	!defined(FALSE) || ((FALSE) != 0)
#define	TRUE	1
#define	FALSE	0
#endif

/*ARGSUSED2*/
main(int argc, char **argv, char **environ)
{
	char	*oldphist = NULL;
	char	*newphist = NULL;
	char	*rmfilelist = NULL;
	char	*newpkglist = NULL;
	char	*outputfile = NULL;
	int	c;
	int	retCode;

	while ((c = getopt(argc, argv, "p:n:o:r:v:")) != EOF) {
		switch (c) {
		case 'p':
			oldphist = optarg;
			break;

		case 'n':
			newphist = optarg;
			break;

		case 'r':
			rmfilelist = optarg;
			break;

		case 'o':
			outputfile = optarg;
			break;

		case 'v':
			newpkglist = optarg;
			break;

		default:
			usage();
			break;
		}
	}

	if (oldphist == NULL || newpkglist == NULL || outputfile == NULL)
		usage();

	read_pkg_history_file(oldphist, &old_pkg_history, FALSE);
	read_pkg_history_file(newpkglist, &new_pkg_id, TRUE);
	if (newphist)
		read_pkg_history_file(newphist, &new_pkg_history, FALSE);
	if (rmfilelist)
		read_pkg_history_file(rmfilelist, &removed_files, FALSE);
	merge_pkg_histlist(&old_pkg_history, new_pkg_history);
	add_removed_files(&old_pkg_history, &removed_files);
	create_new_ignore_entries(&old_pkg_history, new_pkg_id);
	retCode = dump_pkg_hist(old_pkg_history, outputfile);

	if (retCode == HISTORY_FILE_OK)
		exit(warning_state);
	else
		exit(retCode);
}

static void
usage(void)
{
	(void) fprintf(stderr,
	    "Usage: buildhist -p <old_pkg_hist> -v <new_pkgs_list> \\\n");
	(void) fprintf(stderr,
	   "-o <output_file> [ -n <new_pkg_history_dir> -r <rm_file_list> ]\n");
	exit(1);
}

#define	ENTRY_BUF_SIZE 8192
static void
read_pkg_history_file(char *path, PkgHist **pkg_hist_list,
		int process_incomplete_entries)
{
	FILE	*fp;
	char	*line, *entry;
	u_int	entry_used, entry_size, n;

	if ((fp = fopen(path, "r")) == (FILE *)NULL) {
		(void) printf("Can't open file: %s, error = %d\n", path, errno);
		exit(1);
	}

	BadHistoryRecordS = 0;

	gtoksbuf = (char *) calloc(ENTRY_BUF_SIZE, 1);
	glinebuf = (char *) calloc(BUFSIZ, 1);
	gtoksbuf_size = ENTRY_BUF_SIZE;

	line = (char *) calloc(BUFSIZ, 1);

	entry = (char *) calloc(ENTRY_BUF_SIZE, 1);
	entry_size = ENTRY_BUF_SIZE;
	entry_used = 0;

	if (!gtoksbuf || !glinebuf || !line || !entry)
		memfail();

	while (fgets(line, BUFSIZ, fp)) {

		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		if (strstr(line, "PKG=")) {
			parse_pkg_entry(entry, pkg_hist_list,
			    process_incomplete_entries);
			(void) memset(entry, '\0', sizeof (entry));
			(void) strcpy(entry, line);
			entry_used = strlen(line);
		} else {
			n = strlen(line);
			if (entry_used + n >= entry_size) {
				entry = realloc(entry, entry_size +
				    ENTRY_BUF_SIZE);
				if (!entry)
					memfail();
				entry_size += ENTRY_BUF_SIZE;
			}
			(void) strcat(entry, line);
			entry_used += n;
		}
	}
	/*
	 * Last one
	 */
	parse_pkg_entry(entry, pkg_hist_list, process_incomplete_entries);

	(void) free(line);
	(void) free(entry);
	(void) free(gtoksbuf);
	(void) free(glinebuf);
	(void) fclose(fp);
	if (BadHistoryRecordS) {
		(void) fprintf(stderr, "** %d bad record(s) in %s\n",
		    BadHistoryRecordS, path);
	}
}

/*
 * The process_incomplete_entries flag is used to identify if the
 * pkghistory file is allowed to have incomplete entries. An incomplete
 * entry is one that is does not contain: REPLACE_BY, REMOVED_FILES,
 * REMOVE_FROM_CLUSTER, IGNORE_VALIDATION_ERRORS, or PKGRM directives.
 */
static void
parse_pkg_entry(char *entry, PkgHist **pkg_hist_list,
			int process_incomplete_entries)
{
	PkgHist	*ph;
	struct excess_del_ok *d_ok, **dpp;
	char	*tok_value;
	int	needs_saving = 0;
	int	excess_del_ok = 0;

	if (*entry == '\0')
		return;

	BadHistoryRecord = 0;

	ph = (PkgHist *) calloc(sizeof (PkgHist), 1);
	if (!ph)
		memfail();
	(void) memset(ph, '\0', sizeof (PkgHist));

	tok_value = set_token_value("PKG=", entry);
	if (tok_value != NULL) {
		ph->pkg_abbr = trimCopy(tok_value);
		if (strchr(tok_value, ' ') != NULL) {
			BadHistoryRecord++;
		}
	}

	tok_value = set_token_value("ARCH=", entry);
	if (tok_value != NULL) {
		ph->arch = trimCopy(tok_value);
		if (strchr(tok_value, ' ') != NULL) {
			BadHistoryRecord++;
		}
	}

	tok_value = set_token_value("VERSION=", entry);
	if (tok_value != NULL) {
		ph->ver_low = trimCopy(split_ver(tok_value, VER_LOW));
		ph->ver_high = trimCopy(split_ver(tok_value, VER_HIGH));
	}

	tok_value = set_token_value("REPLACED_BY=", entry);
	if (tok_value != NULL) {
		ph->replaced_by = trimCopy(tok_value);
		needs_saving = 1;
	} else {
		ph->replaced_by = NULL;
	}

	tok_value = set_token_value("REMOVED_FILES=", entry);
	if (tok_value != NULL) {
		ph->deleted_files = trimCopy(tok_value);
		ph->prev_num_deletions = count_entries(ph->deleted_files);
		needs_saving = 1;
	}

	tok_value = set_token_value("REMOVE_FROM_CLUSTER=", entry);
	if (tok_value != NULL) {
		ph->cluster_rm_list = trimCopy(tok_value);
		needs_saving = 1;
	}

	tok_value = set_token_value("IGNORE_VALIDATION_ERROR=", entry);
	if (tok_value != NULL) {
		ph->ignore_list = trimCopy(tok_value);
		needs_saving = 1;
	}

	tok_value = set_token_value("PKGRM=", entry);
	if (tok_value != NULL) {
		if ((*tok_value == 'y') || (*tok_value == 'Y'))
			ph->needs_pkgrm = 1;
		needs_saving = 1;
	}

	tok_value = set_token_value("EXCESS_DELETED_FILES_OK=", entry);
	if (tok_value != NULL) {
		if ((*tok_value == 'y') || (*tok_value == 'Y'))
			excess_del_ok = 1;
	}

	if (ph->replaced_by != NULL) {
		if (!strstr(ph->replaced_by, ph->pkg_abbr))
			ph->to_be_removed = 1;
	}

	if (!ph->arch || !ph->ver_high) {
		warning_state = INVALID_ENTRY;
		(void) fprintf(stderr, "WARNING:\n");
		(void) fprintf(stderr, "PKG=%s\n", ph->pkg_abbr);
		(void) fprintf(stderr,
		    "Entry lacks either an ARCH or VERSION field.\n");
		free_ph(ph);
		return;
	}

	if (excess_del_ok) {
		d_ok = (struct excess_del_ok *) calloc(sizeof
		    (struct excess_del_ok), 1);
		if (!d_ok)
			memfail();
		d_ok->ed_pkg_abbr = strdup(ph->pkg_abbr);
		d_ok->ed_arch = strdup(ph->arch);
		if (!d_ok->ed_pkg_abbr || !d_ok->ed_arch)
			memfail();
		/* chain d_ok entry to the del_ok queue */
		dpp = &del_ok;
		while (*dpp != (struct excess_del_ok *)NULL)
			dpp = &((*dpp)->next);
		*dpp = d_ok;
	}
	if (BadHistoryRecord) {
		warning_state = INVALID_ENTRY;
		BadHistoryRecordS++;
		(void) fprintf(stderr,
		    "** ERROR: Bad record ignored:\n%s", entry);
		free_ph(ph);
		return;
	}

	if (needs_saving || process_incomplete_entries)
		insert_in_list(ph, pkg_hist_list);
	else
		free_ph(ph);

	return;
}

static void
insert_in_list(PkgHist *ph, PkgHist **pkg_hist_list)
{
	PkgHist *curph, **last_ph;

	for (last_ph = pkg_hist_list, curph = *last_ph; curph != NULL;
	    last_ph = &((*last_ph)->hist_next), curph = *last_ph) {
		if (entry_before(ph, curph)) {
			ph->hist_next = curph;
			*last_ph = ph;
			break;
		}
	}

	if (curph == NULL) {
		ph->hist_next = NULL;
		*last_ph = ph;
	}

	return;
}

/*
 * Delete all entries from pkg_hist_list whose PKG and ARCH values
 * are equal to pkg and pkgarch.
 */
static void
delete_from_list(char *pkg, char *pkgarch, PkgHist **pkg_hist_list)
{
	PkgHist *curph, **last_ph;

	for (last_ph = pkg_hist_list, curph = *last_ph; curph != NULL; ) {
		if (streq(curph->pkg_abbr, pkg) &&
		    streq(curph->arch, pkgarch)) {
			*last_ph = curph->hist_next;
			free(curph);
		} else
			last_ph = &((*last_ph)->hist_next);
		curph = *last_ph;
	}

	return;
}

static int
entry_before(PkgHist *ph1, PkgHist *ph2)
{
	int status;

	status = strcmp(ph1->pkg_abbr, ph2->pkg_abbr);
	if (status < 0)
		return (1);	/* it's less than */
	else if (status > 0)
		return (0);

	status = strcmp(ph1->arch, ph2->arch);
	if (status < 0)
		return (1);	/* it's less than */
	else if (status > 0)
		return (0);

	status = pkg_vcmp(ph1->ver_high, ph2->ver_low);
	if (status < 0 || status == 0)
		return (1);
	status = pkg_vcmp(ph1->ver_low, ph2->ver_high);
	if (status < 0) {
		(void) printf(
		    "Overlapping package history entries not allowed.\n");
		(void) printf("%s %s %s,%s\n", ph1->pkg_abbr, ph1->arch,
		    ph1->ver_low, ph1->ver_high);
		(void) printf("%s %s %s,%s\n", ph2->pkg_abbr, ph2->arch,
		    ph2->ver_low, ph2->ver_high);
		exit(1);
	}
	return (0);
}

/*
 *  For every entry on the new pkghistory entry list pointed to by
 *  "new", remove all entries from "old" which have the same PKG
 *  and ARCH values.  Then, insert the new entry in the "old" list.
 */
static void
merge_pkg_histlist(PkgHist **old, PkgHist *new)
{
	PkgHist *ph, *next;
	char curpkg[256], curarch[256];

	ph = new;
	curpkg[0] = '\0';
	curarch[0] = '\0';
	while (ph) {
		next = ph->hist_next;
		ph->hist_next = NULL;
		if (!streq(curpkg, ph->pkg_abbr) ||
		    !streq(curarch, ph->arch)) {
			(void) strcpy(curpkg, ph->pkg_abbr);
			(void) strcpy(curarch, ph->arch);
			delete_from_list(ph->pkg_abbr, ph->arch, old);
		}
		insert_in_list(ph, old);
		ph = next;
	}
}


static char zerostring[] = "0";

static void
add_removed_files(PkgHist **old, PkgHist **rmlist)
{
	PkgHist *rmph, *next, *curph, *lastmatchph;
	char *last_hi_ver, *cp, *save_cp;

	rmph = *rmlist;
	*rmlist = NULL;
	while (rmph) {
		next = rmph->hist_next;
		rmph->hist_next = NULL;
		last_hi_ver = zerostring;
		lastmatchph = NULL;
		for (curph = *old; curph != NULL; curph = curph->hist_next) {
			if (streq(rmph->pkg_abbr, curph->pkg_abbr) &&
			    streq(rmph->arch, curph->arch)) {
				last_hi_ver = curph->ver_high;
				lastmatchph = curph;
				if (curph->needs_pkgrm)
					continue;
				if (curph->deleted_files == NULL) {
					cp = (char *) malloc(
					    strlen(rmph->deleted_files) + 2);
					if (!cp)
						memfail();
					*cp = '\0';
				} else {
					cp = (char *) malloc(
					    strlen(curph->deleted_files) +
					    strlen(rmph->deleted_files) + 2);
					if (!cp)
						memfail();
					(void) strcpy(cp, curph->deleted_files);
					(void) strcat(cp, " ");
				}
				(void) strcat(cp, rmph->deleted_files);
				save_cp = curph->deleted_files;
				curph->deleted_files = cp;
				free(save_cp);
			} else
				if (strcmp(last_hi_ver, zerostring) != 0)
					break;
		}
		if (pkg_vcmp(last_hi_ver, rmph->ver_high) < 0) {
			rmph->ver_low = trimCopy(last_hi_ver);
			if (lastmatchph && lastmatchph->ignore_list)
				rmph->ignore_list = trimCopy(
				    lastmatchph->ignore_list);
			insert_in_list(rmph, old);
		} else
			/* NOTE: need to individually free substrings */
			free(rmph);
		rmph = next;
	}
}

static void
create_new_ignore_entries(PkgHist **old, PkgHist *newlist)
{
	PkgHist *newph, *ph, *curph;

	for (curph = *old; curph != NULL; curph = curph->hist_next) {
		if (curph->ignore_list &&
		    (!streq(curph->pkg_abbr, (curph->hist_next)->pkg_abbr) ||
		    !streq(curph->arch, (curph->hist_next)->arch))) {
			for (newph = newlist; newph != NULL; newph =
			    newph->hist_next) {
				if (streq(newph->pkg_abbr, curph->pkg_abbr) &&
				    streq(newph->arch, curph->arch) &&
				    pkg_vcmp(curph->ver_high,
				    newph->ver_high) < 0) {
					if (curph->replaced_by == NULL &&
					    curph->deleted_files == NULL &&
					    curph->cluster_rm_list == NULL &&
					    !(curph->to_be_removed) &&
					    !(curph->needs_pkgrm))
						curph->ver_high =
						    trimCopy(newph->ver_high);
					else {
						ph = (PkgHist *) calloc(
						    sizeof (PkgHist), 1);
						if (!ph)
							memfail();
						(void) memset(ph, '\0',
						    sizeof (PkgHist));
						ph->pkg_abbr =
						    trimCopy(newph->pkg_abbr);
						ph->arch =
						    trimCopy(newph->arch);
						ph->ver_high =
						    trimCopy(newph->ver_high);
						ph->ver_low = curph->ver_high;
						ph->ignore_list =
						    trimCopy(
							curph->ignore_list);
						insert_in_list(ph, old);
					}
				}
			}
		}
	}
}

static char *
set_token_value(char *tok, char *entry)
{
	char	*cp, *tmp;
	int	tok_len;
	u_int	gtoksbuf_used;

	*gtoksbuf = '\0';
	gtoksbuf_used = 0;
	tok_len = strlen(tok);

	for (cp = strstr(entry, tok); cp != NULL;
	    cp = strstr((cp + tok_len), tok)) {
		tmp = tok_value(cp + tok_len);
		if (gtoksbuf_used + strlen(tmp) + 1 >= gtoksbuf_size) {
			gtoksbuf = realloc(gtoksbuf, gtoksbuf_size +
			    ENTRY_BUF_SIZE);
			if (!gtoksbuf)
				memfail();
			gtoksbuf_size += ENTRY_BUF_SIZE;
		}
		if (*gtoksbuf != '\0') {
			(void) strcat(gtoksbuf, " ");
			gtoksbuf_used++;
		}
		(void) strcat(gtoksbuf, tmp);
		gtoksbuf_used += strlen(tmp);
	}
	if (*gtoksbuf == '\0')
		return (NULL);
	return (gtoksbuf);
}

static char *
tok_value(char *cp)
{
	char	*lp;

	lp = glinebuf;

	while (*cp != '\n')
		*lp++ = *cp++;
	*lp = '\0';

	return (glinebuf);
}

static char *
split_ver(char *ver, int flag)
{
	static	char version[TOK_BUF_SIZE];
	char *zero = "0";
	char *cp;

	(void) strcpy(version, ver);
	cp = strchr(version, ':');

	if (flag == VER_LOW) {
		/* 
		 * "0" or the string before ":"
		 */
		if (cp == NULL)
			(void) strcpy(version, zero);
		else {
			cp--;
			while (isspace((u_char)*cp))
				cp--;
			*++cp = '\0';
		}
		cp = version;
	} else if (flag == VER_HIGH) {
		/*
		 * The string after the ":", or the whole string if no ":"
		 */
		if (cp != NULL) {
			cp++;
			while (isspace((u_char)*cp))
				cp++;
		} else {
			cp = version;
		}
	} else {
		cp = version;
	}
	/*
	 * Sanity check
	 * Can't be too long, and can't have imbedded blanks
	 */

	if (*cp != '\0' && strlen(cp) >= TOK_BUF_SIZE) {
		BadHistoryRecord++;
		cp = zero;
	}
	if (strchr(cp, ' ') != NULL) {
		BadHistoryRecord++;
		cp = zero;
	}
	return (cp);
}

static int
dump_pkg_hist(PkgHist *histlist, char *filename)
{
	PkgHist *ph;
	struct excess_del_ok *d;
	char *cp;
	FILE *dfp;
	int  delCount, byteCount, stringCount;
	int  returnCode = HISTORY_FILE_OK;

	dfp = fopen(filename, "w");
	if (dfp == NULL) {
		(void) printf("Can't write to output file: %s, error = %d\n",
		    filename, errno);
		exit(1);
	}
	for (ph = histlist; ph != NULL; ph = ph->hist_next) {
		delCount = byteCount = 0;
		byteCount += fprintf(dfp, "PKG=%s\n", ph->pkg_abbr);
		byteCount += fprintf(dfp, "ARCH=%s\n", ph->arch);
		if (strcmp(ph->ver_low, "0") == 0)
			byteCount += fprintf(dfp, "VERSION=%s\n", ph->ver_high);
		else
			byteCount += fprintf(dfp,
			    "VERSION=%s:%s\n", ph->ver_low, ph->ver_high);
		for (cp = ph->replaced_by; cp;) {
			byteCount += fprintf(dfp, "REPLACED_BY=%s\n",
			    chopstring(&cp, 80 -
			    strlen("REPLACED_BY="), &stringCount));
		}
		for (cp = ph->deleted_files; cp;) {
			byteCount += fprintf(dfp, "REMOVED_FILES=%s\n",
			    chopstring(&cp, 80 - strlen("REMOVED_FILES="),
				&stringCount));
			delCount += stringCount;
		}
		for (cp = ph->cluster_rm_list; cp;) {
			byteCount += fprintf(dfp, "REMOVE_FROM_CLUSTER=%s\n",
			    chopstring(&cp,
			    80 - strlen("REMOVE_FROM_CLUSTER="),
			    &stringCount));
		}
		for (cp = ph->ignore_list; cp;) {
			byteCount += fprintf(dfp,
			    "IGNORE_VALIDATION_ERROR=%s\n",
			    chopstring(&cp,
			    80 - strlen("IGNORE_VALIDATION_ERROR="),
			    &stringCount));
		}
		if (ph->needs_pkgrm) {
			byteCount += fprintf(dfp, "PKGRM=yes\n");
		}
		/*
		 *	Print a warning to the user if the number
		 *	of deleted files in a history entry exceeds
		 *	the MAX_DELETED_FILES definition.
		*/
		if (delCount > MAX_DELETED_FILES &&
		    delCount > ph->prev_num_deletions) {
			/*
			 *  See if this package is on the list of packages
			 *  for which excess deletions are OK.
			 */
			for (d = del_ok; d != NULL; d = d->next)
				if (streq(ph->pkg_abbr, d->ed_pkg_abbr) &&
				    streq(ph->arch, d->ed_arch))
					break;

			if (!d) {	/* not on excess-deletions_ok list */
				(void) fprintf(stderr, "WARNING:\n");
				(void) fprintf(stderr,
				    "PKG=%s\n", ph->pkg_abbr);
				(void) fprintf(stderr, "ARCH=%s\n", ph->arch);
				if (strcmp(ph->ver_low, "0") == 0)
					(void) fprintf(stderr,
						"VERSION=%s\n", ph->ver_high);
				else
					(void) fprintf(stderr,
					    "VERSION=%s:%s\n", ph->ver_low,
					    ph->ver_high);
				(void) fprintf(stderr,
				    "Excessive number of deleted");
				(void) fprintf(stderr,
				    " files: %d\n\n", delCount);
				returnCode = EXCESS_DELETED_FILES;
			}
		}
	}
	(void) fclose(dfp);
	return (returnCode);
}

static char *
chopstring(char **cpp, int maxlen, int *stringCount)
{
	char *cp;
	int i, last_start_of_string;

	*stringCount = 0;
	cp = *cpp;

	/* handle zero=length strings and all-blank strings */
	i = strlen(cp);
	if (i == 0) {
		*cpp = NULL;
		return (cp);
	}
	i--;
	while (i > 0 && cp[i] == ' ')
		i--;
	if (i == 0) {
		*cpp = NULL;
		cp[0] = '\0';
		return (cp);
	}

	/* cut off trailing blanks */
	cp[++i] = '\0';

	/* move leading cursor past leading blanks */
	while (*cp == ' ')
		cp++;

	i = 0;
	last_start_of_string = 0;
	++(*stringCount);

	/*CONSTCOND*/
	while (1) {
		while (cp[i] != ' ' && cp[i] != '\0')
			i++;
		if (cp[i] == '\0' || i > maxlen) {
			if (last_start_of_string == 0 || i <= maxlen) {
				if (cp[i] == '\0')
					*cpp = NULL;
				else {
					cp[i] = '\0';
					i++;
					while (cp[i] == ' ')
						i++;
					*cpp = cp + i;
				}
			} else {
				i = last_start_of_string;
				*cpp = cp + i;
				i--;
				while (cp[i] == ' ')
					i--;
				cp[i +1] = '\0';
				--(*stringCount);
			}
			return (cp);
		}
		while (cp[i] == ' ')
			i++;
		last_start_of_string = i;
		++(*stringCount);
	}
	/*NOTREACHED*/
	return (cp);
}

/*
	Given a string, make a copy of it and remove all trailing
	blanks, tabs, CRs and LFs. Null terminate.
	Returns:
		char* of a new string (malloc'd) when successful.
		NULL when unsuccessful.
*/
static char *
trimCopy(char * inStr)
{
	char	*retStr, *sPtr;

	retStr = strdup(inStr);
	if (!retStr)
		memfail();
	sPtr = strrspn(retStr, " \t\n\r\f\v");
	*sPtr = NULL;
	return (retStr);
}

static void
memfail(void)
{
	(void) fprintf(stderr, "FATAL ERROR: malloc() failed.\n");
	exit(INSUFFICIENT_MEMORY);
}

static int
count_entries(char *cp)
{
	int count = 0;
#define	IN_SPACE	0
#define	IN_WORD		1
	int state = IN_SPACE;

	if (cp == NULL)
		return (0);

	while (*cp) {
		if (state == IN_SPACE) {
			if (!isspace(*cp)) {
				count++;
				state = IN_WORD;
			}
		} else {
			if (isspace(*cp))
				state = IN_SPACE;
		}
		cp++;
	}
	return (count);
}

static void
free_ph(PkgHist *ph)
{
	if (ph->pkg_abbr)
		free(ph->pkg_abbr);
	if (ph->ver_low)
		free(ph->ver_low);
	if (ph->ver_high)
		free(ph->ver_high);
	if (ph->arch)
		free(ph->arch);
	if (ph->replaced_by)
		free(ph->replaced_by);
	if (ph->deleted_files)
		free(ph->deleted_files);
	if (ph->cluster_rm_list)
		free(ph->cluster_rm_list);
	if (ph->ignore_list)
		free(ph->ignore_list);
	free(ph);
}
