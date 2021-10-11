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

#pragma	ident	"@(#)tty_terminal.c 1.5 96/06/13"

/*
 *	File:		tty_get_terminal.c
 *
 *	Description:	This file contains the routines needed to get the
 *			terminal type.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "tty_defs.h"
#include "tty_msgs.h"

#ifdef sun386
#include <ctype.h>
#endif

struct  term_id {
	char	*prompt;	/* id of internationalized text message */
	char	*name;		/* actual value for TERM environment variable */
};

static	void		verify_term_list(void);
static	void		print_list(void);
static	void		print_other(void);

static	struct term_id  *verify_selection(char *);
static	void		get_stdin(char *);


/*
 * Any terminal that you want to display in the selection list,
 * need only be added to this list, with values as specified
 * by the `term_id' structure, above.
 */
static	struct term_id  term_list[] = {
	{	NULL,		"ansi"		},
	{	NULL,		"vt52"		},
	{	NULL,		"vt100"		},
	{	NULL,		"h19"		},
	{	NULL,		"adm31"		},
	{	NULL,		"AT386"		},
	{	NULL,		"sun-cmd"	},
	{	NULL,		"sun"		},
	{	NULL,		"tvi910+"	},
	{	NULL,		"925"		},
	{	NULL,		"wyse50"	},
	{	NULL,		"xterms"	},	/* new [shumway] */

	/*
	 * Add new terminal entries above here.  Don't touch
	 * anything below here.
	 */
	{	NULL,		NULL	},	/* used for manual entry */
};

static	int	*term_index;
static	int	term_size;	/* size of term_list */
static	int	num_terms;	/* # terminal entries in term_list that */
				/* exist in terminfo database		*/

/*
 * tty_get_terminal:
 *
 *	This routine is the client interface routine
 *	used for retrieving the console terminal type.
 *
 *	Input:  pointer to character buffers in which to place the
 *		terminal type.
 */

void
do_get_terminal(MSG *mp, int reply_to)
{
	char		termtype[MAX_TERM+1];
	char		buf[BUFSIZ];		/* input buffer */
	int		done = 0;		/* got terminal yet? */
	int		i;
	struct term_id	*selected;

	msg_delete(mp);

	i = 0;
	term_list[i++].prompt = xstrdup(ANSI);
	term_list[i++].prompt = xstrdup(VT52);
	term_list[i++].prompt = xstrdup(VT100);
	term_list[i++].prompt = xstrdup(H19);
	term_list[i++].prompt = xstrdup(ADM31);
	term_list[i++].prompt = xstrdup(SUN_PC);
	term_list[i++].prompt = xstrdup(SUN_CMDTOOL);
	term_list[i++].prompt = xstrdup(SUN_WORKSTATION);
	term_list[i++].prompt = xstrdup(TELEVIDEO_910);
	term_list[i++].prompt = xstrdup(TELEVIDEO_925);
	term_list[i++].prompt = xstrdup(WYSE_50);
	term_list[i++].prompt = xstrdup(XTERM);
	term_list[i++].prompt = xstrdup(OTHER_TERMINAL);

	/*
	 * Pick one of the entries and make sure we can open
	 * the terminfo file.  If we can't, we should be able to
	 * proceed with an ugly interface using printf.  That's
	 * the fallback strategy eventually, but doesn't work now.
	 */
	if (tgetent(buf, term_list[0].name) == -1) {
		termtype[0] = NULL;
		reply_string(ATTR_TERMINAL, termtype, reply_to);
		return;
	}

	/*
	 * For each terminal in term_list that has a terminfo entry,
	 * save its index.
	 */
	term_size = sizeof (term_list) / sizeof (struct term_id);
	term_index = (int *)xmalloc(term_size * sizeof (int));
	verify_term_list();

	/* clear the screen (d n't know term type yet) */
	for (i = 0; i < 66; i++)
		(void) putchar('\n');

	/* main loop */
	do {
		/*
		 * Print terminal selection list only if there are valid
		 * terminfo entries for the selection.  Otherwise, force
		 * manual entry of terminal.
		 */
		if ((num_terms <= 1) || (term_list[0].name == NULL)) {
			/* Force manual entry */
			for (i = 0; i < term_size; i++)
				if (term_list[i].name == (char *)0)
					break;
			selected = &term_list[i];
		} else {
			/* Present terminal selection list */
			print_list();
			get_stdin(buf);

			/*
			 * Make sure user entered only a number as specified
			 * in the list.  Valid selection will return pointer
			 * to corresponding term_list entry.
			 */
			selected = verify_selection(buf);
			if (!selected)
				continue;
		}

		/* Check for list terminal selection or manual entry */
		if (selected->name == (char *)0) {

			/* Prompt for manual entry */
			print_other();
			get_stdin(buf);
			(void) strcpy(termtype, buf);

			/* Error if nothing entered */
			if (buf[0] == NULL) {
				(void) wword_wrap(stdscr, 0, 0, MINCOLS,
						TERMINAL_NONE);
				continue;
			}


			/*
			 * Verify that there is a terminfo entry
			 */
			switch (tgetent(buf, termtype)) {

			case 0:		/* no terminfo entry */
				(void) wword_wrap(stdscr, 0, 0, MINCOLS,
						TERMINAL_NOT_FOUND);
				termtype[0] = NULL;
				break;

			case 1:		/* terminfo entry found */
				done = 1;
				break;
			}
		} else {
			/* Valid terminal selection from list */
			(void) strcpy(termtype, selected->name);
			done = 1;
		}

	} while (!done);

	/* Cleanup, and set TERM environment variable */
	free(term_index);
	for (i = 0; i < term_size; i++)
		free(term_list[i].prompt);
	reply_string(ATTR_TERMINAL, termtype, reply_to);
}



/*
 * get_stdin
 *
 *	Read stdin, and remove any leading or trailing blanks.
 *
 *	Input: buffer to receive user input.
 */

static	void
get_stdin(char	*buf)
{
	int	i;
	char	tmp[BUFSIZ];

	/* Read stdin, return if nothing entered */
	i = read(0, buf, BUFSIZ);
	buf[i] = NULL;
	if (i == 0)
		return;

	/* Delete leading blanks */
	for (i = 0; i < (int)strlen(buf); i++) {
		if (!isspace(buf[i]))
			break;
	}

	/* Refresh buf */
	(void) strcpy(tmp, &buf[i]);
	(void) strcpy(buf, tmp);

	/* Delete trailing blanks */
	for (i = strlen(buf)-1; i >= 0; i--)
		if (!isspace(buf[i]))
			break;

	if (i >= 0)
		buf[i+1] = NULL;
}



/*
 * verify_selection
 *
 *	Syntax check on user input.  Make sure user entered only
 *	a valid digit from the selection list.
 *
 *	Input:  pointer to buffer containing user character input.
 *
 *	Returns: pointer to `term_id' structure of selected terminal,
 *	otherwise returns NULL.
 */

static struct term_id *
verify_selection(char	*buf)
{
	int	i;

	/* Error if nothing entered */
	if (buf[0] == NULL) {
		(void) wword_wrap(stdscr, 0, 0, MINCOLS, NOTHING_SELECTED);
		return (NULL);
	}

	/* Verify selection syntax */
	for (i = 0; buf[i]; i++) {
		if (!isdigit(buf[i])) {
			(void) wword_wrap(stdscr, 0, 0, MINCOLS, NOT_A_DIGIT);
			return (NULL);
		}
	}

	/* Verify that the selection was proper */
	i = atoi(buf);
	if (i > num_terms) {
		(void) wword_wrap(stdscr, 0, 0, MINCOLS, INVALID_NUMBER);
		return (NULL);
	}

	/* return address of selected entry */
	return (&term_list[term_index[i-1]]);
}



/*
 * verify_term_list
 *
 *	For each entry in default terminal selection
 *	list, make sure it has a terminfo entry, other-
 *	wise, we don't display it.
 */

static void
verify_term_list(void)
{
	int	i;
	char	buf[BUFSIZ];

	for (num_terms = 0, i = 0; i < term_size; i++) {

		if (term_list[i].name != NULL) {

			switch (tgetent(buf, term_list[i].name)) {

			case 1:		/* terminfo entry found, record index */
				term_index[num_terms++] = i;
				break;

			case 0:		/* no terminfo entry */
				break;
			}
		}
		else
			/* We want the manual selection to always display */
			term_index[num_terms++] = i;
	}
}




/*
 * print_list
 *
 *	Print the list of supported terminal types.
 */

static  void
print_list(void)
{
	int	i;
	char	*buf, *p;

#define	LIST_FMT	" %d) %s"

	/* First, print the selection list. */
	(void) wword_wrap(stdscr, 0, 0, MINCOLS, TERMINAL_TEXT);

	for (i = 0; i < num_terms; i++) {
		p = term_list[term_index[i]].prompt;
		buf = (char *)xmalloc(strlen(LIST_FMT) + strlen(p) + 1);
		(void) sprintf(buf, LIST_FMT, i+1, p);
		(void) wword_wrap(stdscr, 0, 0, MINCOLS, buf);
		free(buf);
	}

	/* Then, print the manual selection entry */
	(void) wword_wrap(stdscr, 0, 0, MINCOLS, TERMINAL_PROMPT);
	(void) fflush(stdout);
}




/*
 * print_other
 *
 *	Print the manual entry prompt.
 */

static  void
print_other(void)
{
	(void) wword_wrap(stdscr, 0, 0, MINCOLS, TERMINAL_OTHER_TEXT);
	(void) wword_wrap(stdscr, 0, 0, MINCOLS, TERMINAL_OTHER_PROMPT);
	(void) fflush(stdout);
}
