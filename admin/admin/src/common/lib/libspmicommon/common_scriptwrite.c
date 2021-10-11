#ifndef lint
#pragma ident   "@(#)common_scriptwrite.c 1.2 96/06/06 SMI"
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

#include <string.h>
#include "spmicommon_lib.h"
#include "common_strings.h"

#define	SCRIPTBUFSIZE 1024

static int	g_seq = 1;

/* Public Function Prototypes */

void		scriptwrite(FILE *, uint, char **, ...);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * scriptwrite()
 *	Write out specified script fragment to output file, replacing
 *	specified tokens by values.
 *
 *	expected arguments:
 *		FILE	*fp;			file pointer
 *		u_int	format			format flag for write_message()
 *		char	**cmds;			array of shell commands
 *		{char	*token, *value}*	up to 10 token-value pairs
 *		char	*(0)			to mark end of list
 *
 *	The array of shell commands will contain 2 sets of strings delimeted
 *	by a null string.
 *	The first will be used to generate the actual upgrade shell script,
 *	and will contain token placeholders identified by @TOKEN@.  These will
 *	be replaced by their actual values before writing.
 *	The second set of strings will begin with:
 *		"DryRun @TOKEN0@ @TOKEN1@ ..."
 *	Followed by any text that will be printed on the screen.
 *	    If the text needs translation, it will be of the form:
 *		"gettext SUNW_INSTALL_LIBSVC The actual text...`"
 *	    Any tokens will be represented by $0n to avoid confusion during
 *	    localization, and will be replaced by ther actual values before
 *	    writing.
 *	    Also, the string "gettext SUNW_INSTALL_LIBSVC" will be
 *	    stripped along with the trailing "`"
 *
 *	If DryRun AND trace_level is set > 0, we'll still write the script.
 *
 * WARNING:
 *	The output string must NOT be longer than SCRIPTBUFSIZE
 * Return:
 *	none
 * Status:
 *	public
 */
void
scriptwrite(FILE *fp, u_int format, char **cmdarray, ...)
{
	va_list ap;
	char	*token[10], *value[10];
	int	TokenIndex[10];
	char	thistoken[20];
	char	buf[SCRIPTBUFSIZE], ibuf[SCRIPTBUFSIZE];
	int	count = 0;
	int	DryRun = 0;
	int	DryRunHdr = 0;
	int	PrintDryRun = 0;
	int	PassCount = 1;
	int	i, j, len, slen, TokI;
	char	c, *cp, *dst;
	char	*bp;

	va_start(ap, cmdarray);
	while ((token[count] = va_arg(ap, char *)) != (char *)0) {
		value[count++] = va_arg(ap, char *);
	}
	va_end(ap);

	i = TokI = 0;
	if (GetSimulation(SIM_EXECUTE)) {
		PassCount = 2;
		if (get_trace_level() == 0) {
			/*
			 * If we're in DryRun, but not tracing, skip down to
			 * the "DryRun" eyecatcher.
			 */
			for (i = 0; *(cp = cmdarray[i]) != '\0'; i++)
				;
		}
	}
	while (PassCount) {
	    for (; *(cp = cmdarray[i]) != '\0'; i++, DryRunHdr++) {
		if (DryRun && (strncmp(cp, "gettext", 7) == 0)) {
			/*
			 * Translate it before doing token replacements:
			 * Strip all chars up to "'",
			 * Strip trailing char (assumed to be "'"
			 */
			dst = strchr(cp, '\'');
			if (*dst != '\0') {
				dst++;
				strcpy(ibuf, dst);
				ibuf[strlen(ibuf)-1] = NULL;
				cp = dgettext("SUNW_INSTALL_LIBSVC", ibuf);
			}
		}
		/*
		 * cp is now pointing to the source string
		 * bp is pointing to the destination buffer
		 */
		bp = buf;
		while ((c = *cp++) != '\0') {
			switch (c) {

			case '@':
				dst = thistoken;
				while ((*dst++ = *cp++) != '@')
					;
				*--dst = '\0';
				if (strcmp(thistoken, "SEQ") == 0) {
					len = sprintf(bp, "%d", g_seq);
					if (len > 0)
						bp += len;
					break;
				}
				for (j = 0; j < count; j++) {
					/*
					 * If the token matches
					 *   If this is a dryrun && we're
					 *   processing the header
					 *	Note the index of this token
					 *	for later substitution
					 *   else
					 *	replace the token by its value
					 */
					if (strcmp(thistoken, token[j]) == 0) {
						if (!DryRun || DryRunHdr) {
							slen = strlen(value[j]);
							strncpy(bp, value[j],
							    slen);
							bp += slen;
						} else if (
						    DryRun && !DryRunHdr) {
							TokenIndex[TokI++] = j;
						}
						break;
					}
				}
				if (j == count) {
					write_message(SCR, WARNMSG, LEVEL0,
					    MSG1_BAD_TOKEN, thistoken);
				}
				break;

			case '$':
				if (!DryRun) {
					*bp++ = c;
					break;
				}
				/*
				 * Make sure it's '$0n'
				 */
				if (*cp != '0') {
					*bp++ = c;
					break;
				}
				if (!isdigit(*(cp+1))) {
					*bp++ = c;
					break;
				}
				/*
				 * Got it, copy value if it's valid
				 */
				j = TokenIndex[atoi(cp+1)];
				if (j >= 0) {
					slen = strlen(value[j]);
					strncpy(bp, value[j], slen);
					bp += slen;
					cp += 2;
				} else {
					*bp++ = c;
				}
				break;

			default:
				*bp++ = c;
				break;
			}
		}
		*bp = NULL;
		if (PrintDryRun) {
			/*
			 * If we're printing DryRun info, and we're past
			 * the eyecatcher, write_message() the buffer
			 */
			if (DryRunHdr)
				write_message(LOGSCR, STATMSG, format, buf);
		} else {
			(void) fprintf(fp, "%s\n", buf);
		}
	    }
	    PassCount--;
	    /*
	     * If we're not in DryRun, PassCount should be 0 now
	     */
	    if (GetSimulation(SIM_EXECUTE) && (PassCount == 1)) {
		i++;
		if (strncmp(cmdarray[i], "DryRun", 6)) {
			write_message(SCR, WARNMSG, LEVEL0,
			    dgettext("SUNW_INSTALL_LIBSVC",
			    "Internal error: Dry Run message missing"));
			/*
			 * Just use the shell script string
			 */
			i = 0;
			DryRunHdr = 1;
		} else {
			DryRunHdr = 0;
			DryRun = 1;
			TokI = 0;
			for (j = 0; j < 10; j++) {
				TokenIndex[j] = -1;
			}
		}
		PrintDryRun = 1;
	    }
	}
	g_seq++;
}
