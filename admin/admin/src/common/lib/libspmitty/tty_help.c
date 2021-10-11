#ifndef lint
#pragma ident "@(#)tty_help.c 1.2 96/05/10 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_help.c
 * Group:	libspmitty
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/bitmap.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <unistd.h>
#include <locale.h>
#include <libintl.h>

#include "spmitty_api.h"
#include "tty_utils.h"
#include "tty_strings.h"

/*
 * adminhelp is a simple form of online help.  it can display information
 * in the following categories:
 *	Topics		provides conceptual information about the
 *			program or task
 *	How To		provides step by step instructions for a task
 *	Reference	defines terms, describes values, etc.
 *
 * adminhelp expects the following files to live in the directory
 * indicated by the ADMINHELPHOME environment variable:
 *
 *		    $ADMINHELPHOME
 *			  |
 *	topics		howto		reference
 *	  |		  |		    |
 *	Topics		Howto		Reference
 *	datafiles	datafiles	datafiles
 *
 * Topics, Howto, and Reference are index files that contain a list of
 * subject and filename pairs:
 *
 *	subject_string
 *		filename
 *
 * datafiles are the text files containing the help text - the filenames
 * must correspond to the filenames specified in the index files.
 *
 * adminhelp takes 2 parameters:
 *	a char		which is one of 'C', 'P', or 'R' and indicates
 *			the help category (Topics, How To, and
 *			Reference, respectively).
 *	a char *	which is the filename of the file that contains
 *			text on the desired subject.
 */

static char	*helpdir = (char *) NULL;

static char	*helpdir_dflt = "/usr/lib/locale/$LANG/help/adminhelp";


/*
 * implemented as 3 screens:
 *
 * Category Browser -> selects the category of help (Topic, Howto, Reference)
 * Subject Browser -> selects a title within a category
 * Help Viewer -> displays the text associated with Category/Subject chosen.
 *
 *
 *
 */

/*
 * export a `pointer' to the top level help index
 */
static HelpEntry	_toplevel_help;

/* these are used for the previous button */
static int	last_category = 0;
static int	last_subject = 0;

static WINDOW  *hwin;

typedef struct h_index {
	char		*title;
	char		*fname;
	char		*text;
	struct h_index	*next;
} h_index_t;

static h_index_t *topic = (h_index_t *) NULL;
static h_index_t *howto = (h_index_t *) NULL;
static h_index_t *refer = (h_index_t *) NULL;

/* forward declarations: */
static int	view_help(help_t, h_index_t *);
static int	read_help_text(char *, h_index_t *);
static int	count_items(h_index_t *);
static int	fmt_text(char *, char ***);
static void	show_help_text(WINDOW *, char **, int, int, int);
static char	*get_help_subject(h_index_t *, char *);
static void	syntax(char *, int, char *);
static help_t	get_help_category(void);
static h_index_t *load_index(char *, char *);

/*
 * Function: HelpInitialize
 * Description:
 *	Set up the information that tells where to look for the help
 *	files.
 *	The helpdir is set to the value of the environment variable
 *	ADMINHELPHOME.  If ADMINHELPHOME is not set, then the value
 *	of the parameter help_dir is used. If that is null, then use
 *	a hard-coded default.
 * Scope:       PUBLIC
 * Parameters:
 *	help_dir - Help path to use if ADMINHELPHOME is not set.
 *		   May be NULL.
 * Return:	none
 * Notes:
 */
void
HelpInitialize(char *help_dir)
{
	char *tmp;

	/*
	 * setup default online help location
	 */

	if (!(tmp = getenv("ADMINHELPHOME"))) {
		if (help_dir)
			tmp = help_dir;
		else
			tmp = helpdir_dflt;
	}
	helpdir = xstrdup(tmp);
}

/*
 * Function: HelpGetTopLevelEntry
 * Description:
 *	Get the top level help entry.
 * Scope:       PUBLIC
 * Parameters:  none
 * Return:	the top level help entry
 * Notes:
 */
HelpEntry
HelpGetTopLevelEntry(void)
{
	return (_toplevel_help);
}

/*
 * show_help(void *h, void *foo)
 *
 * function:    callback function to be passed into wmenu() for displaying
 *		online help attached to menu selection items
 */
/*ARGSUSED1*/
int
show_help(void *h, void *foo)
{

	do_help_index(((HelpEntry *) h)->win, ((HelpEntry *) h)->type,
	    ((HelpEntry *) h)->title);

	return (0);

}

/*
 * do_help_index(help_t cat, char *title)
 *
 * function:  entry point into the AdminHelp system which displays the
 *		online help for the specified category and title.
 *
 *		if cat and title are HELP_NONE and (char *)NULL respectively,
 *		the help system is entered at the top level help index
 *
 * input:
 *	  cat:	  help type {HELP_NONE | HELP_TOPIC | HELP_HOWTO | HELP_REFER}
 *	  title:  pointer to character string representing desired help topic
 *
 * return:
 *	  no return
 */
void
do_help_index(WINDOW *parent, help_t icat, char *ititle)
{

	h_index_t	*first_subject;
	h_index_t	*tmp;
	help_t		cat;
	char		*title;
	char		*cat_dir;
	char		*cat_title;
	int		done = 0;
	int		curx, cury;
	static int	first = 1;

	first_subject = tmp = (h_index_t *) NULL;

	cat = icat;		/* category passed in */
	title = ititle;		/* title passed in */

	/* initialize indexes if necessary */
	if (first) {

		_toplevel_help.win = (WINDOW *) stdscr;
		_toplevel_help.type = HELP_NONE;
		_toplevel_help.title = (char *) NULL;

		topic = load_index("topics", "Topics");
		howto = load_index("howto", "Howto");
		refer = load_index("reference", "Reference");

		if (topic == (h_index_t *) NULL &&
		    howto == (h_index_t *) NULL &&
		    refer == (h_index_t *) NULL) {
			(void) simple_notice(stdscr, F_OKEYDOKEY,
			    TTY_HELP_NOHELP_TITLE,
			    TTY_HELP_NOHELP_NOTICE);
			return;
		} else
			first = 0;

	}

	(void) getsyx(cury, curx);

	/* set up help window */
	if (hwin == (WINDOW *) NULL) {

		hwin = newwin(LINES, COLS, 0, 0);
		wcolor_set_bkgd(hwin, BODY);
		keypad(hwin, 1);	/* blech!  why isn't this inherited? */

	}
	(void) wclear(hwin);
	(void) werase(hwin);

	while (done == 0) {

		/*
		 * if no category, ask user for one...
		 *
		 * get_help_category returns a help_t enum indicating which
		 * help category the user selected.
		 *
		 * set up a couple of pointers based on this enum.
		 *
		 * first_subject is pointer to the list of available subjects
		 * under that category
		 *
		 * cat_dir	 is a pointer to the subdirectory name
		 *		containing the help files corresponding to
		 *		the help subjects
		 *
		 * cat_title	 is a pointer to the string which names the
		 *		 category, this is just used to display the
		 * 		current category in the next menu
		 */
		if (cat == HELP_NONE)
			cat = get_help_category();

		switch ((int) cat) {
		    case HELP_TOPIC:
			first_subject = topic;
			cat_dir = "topics";
			cat_title = "Topics";
			break;

		    case HELP_HOWTO:
			first_subject = howto;
			cat_dir = "howto";
			cat_title = "How To";
			break;

		    case HELP_REFER:
			first_subject = refer;
			cat_dir = "reference";
			cat_title = "Reference";
			break;

		    case HELP_NONE:
		    default:
			done = 1;
			break;
		}

		/* loop until done with viewing subjects */
		while (done == 0) {

			/*
			 * if no title given, ask user for one
			 *
			 * get_help_subject() returns a pointer to a string
			 * representing the title of the help the user
			 * selected.
			 *
			 * If title is non-null, need to look it up in the list
			 * of available help titles to find the help node
			 * which matches the title.
			 *
			 */
			if (title == (char *) NULL) {
				last_subject = 0;
				title = get_help_subject(
						first_subject, cat_title);
			}

			/* if no title, go back to categories */
			if (title == (char *) NULL) {
				cat = HELP_NONE;
				break;
			} else if (title[0] == '\0') {
				done = 1;
				break;
			}
			/* find first node matching 'title' */
			for (tmp = first_subject;
			    tmp && (strcmp(title, tmp->title) != 0);
			    tmp = tmp->next);

			if (tmp) {

				/*
				 * found help!	read in the help text and
				 * save a pointer to it in the help node
				 * structure.
				 *
				 * once it has been read in, go display the
				 * help.
				 */

				(void) werase(hwin);

				if (read_help_text(cat_dir, tmp) == -1) {
					char *msg;
					msg = (char *) xmalloc(strlen(
						TTY_HELP_NOHELPTOPIC_TEXT) +
						strlen(tmp->title) + 1);
					(void) sprintf(msg,
						TTY_HELP_NOHELPTOPIC_TEXT,
						tmp->title);
					(void) simple_notice(stdscr,
						F_OKEYDOKEY,
						TTY_HELP_NOHELPTOPIC_TITLE,
						msg);
					free(msg);

					/* return to the topics list */
					title = (char *) NULL;
				} else if (view_help(cat, tmp) == DONE) {
					/* choose another subject */
					title = (char *) NULL;
				} else {
					done = 1;
				}

			} else {

				/* title not found, choose another */
				title = (char *) NULL;
				continue;

			}
		}
	}

	/* all done... clean up */
	(void) delwin(hwin);
	(void) touchwin(parent);
	(void) wrefresh(parent);
	(void) setsyx(cury, curx);
	(void) doupdate();
	hwin = (WINDOW *) NULL;

}


static	  help_t
get_help_category(void)
{
	static char	*intro;
	static char	*opts[3];
	int		n_opt = sizeof (opts) / sizeof (char *);

	int		ch;
	int		row;
	int		top_row;
	int		cur;
	int		keys;

	/* load up choices */
	if (intro == (char *)0) {
		intro = TTY_HELP_CATEGORY;
		opts[0] = DESC_F_TOPICS;
		opts[1] = DESC_F_HOWTO;
		opts[2] = DESC_F_REFER;
	}

	(void) werase(hwin);
	(void) wclear(hwin);

	/* show title  */
	wheader(hwin, TTY_HELP_MAIN_INDEX);

	row = 2;

	/* show blurb about this screen */
	row = wword_wrap(hwin, row, 2, 75, intro);
	++row;

	/* remember first row of menu, count options, display them */
	top_row = row;

	keys = F_GOTO | F_EXITHELP;
	wfooter(hwin, keys);

#define	cat_to_item_num(x) \
	(((x == HELP_TOPIC) ? 0 : \
	    ((x == HELP_HOWTO) ? 1 : \
	    (x == HELP_REFER) ? 2 : 0)))

#define	item_num_to_cat(x) \
	(((x == 0) ? HELP_TOPIC : \
	    ((x == 1) ? HELP_HOWTO : \
	    (x == 2) ? HELP_REFER : HELP_TOPIC)))

	/* set selected status on current selected category */
	if (last_category)
		cur = cat_to_item_num(last_category);
	else
		cur = cat_to_item_num(HELP_TOPIC);

	ch = wmenu(hwin, top_row, INDENT1,
		LINES - top_row - 5, COLS - (2 * INDENT1),
		(Callback_proc *)0, (void *)0,		/* help proc */
		(Callback_proc *)0, (void *)0,		/* select proc */
		(Callback_proc *)0, (void *)0,		/* deselect proc */
		NULL,
		opts, n_opt, (void *)&cur,
		M_RADIO | M_RADIO_ALWAYS_ONE, keys);

	if (is_continue(ch)) {
		last_category = item_num_to_cat(cur);
		return (last_category);
	} else
		return (HELP_NONE);

}

/*ARGSUSED*/
static char *
get_help_subject(h_index_t *cat, char *cat_title)
{
	static char	subject[BUFSIZ];	/* scratch buf to selected */
						/* subject */
	char		**subjects;
	int		nsubjects;	/* number of help subjects */

	h_index_t	*tmp;		/* scratch pointer for help nodes */

	int		i;		/* scratch counter */
	int		top_row;	/* first row of menu */
	int		cur;		/* remember last selection */
	int		keys;

	int		ch;

	char		*ptr;

	/* set up array of subject titles */
	nsubjects = count_items(cat);

	subjects = (char **) xmalloc(nsubjects * sizeof (char *));
	(void) memset((void *)subjects, 0, nsubjects * sizeof (char *));

	for (i = 0, tmp = cat; i < nsubjects && tmp; i++, tmp = tmp->next) {

		subjects[i] = (char *) tmp->title;

	}

	(void) werase(hwin);
	(void) wclear(hwin);

	/* show title */
	wheader(hwin, TTY_HELP_SUBJECTS);

	keys = F_GOTO | F_MAININDEX | F_EXITHELP;
	wfooter(hwin, keys);

	/* set up for first page */
	cur = last_subject;
	top_row = 3;

	ch = wmenu(hwin, top_row, INDENT1,
		LINES - top_row - 5, COLS - (2 * INDENT1),
		(Callback_proc *)0, (void *)0,		/* help proc */
		(Callback_proc *)0, (void *)0,		/* select proc */
		(Callback_proc *)0, (void *)0,		/* deselect proc */
		NULL,
		subjects, nsubjects, (void *)&cur,
		M_RADIO | M_RADIO_ALWAYS_ONE, keys);

	if (is_continue(ch) != 0) {
		last_subject = cur;
		(void) strcpy(subject, subjects[cur]);
		ptr = subject;
	} else {
		(void) memset(subject, 0, sizeof (subject));
		if (is_helpindex(ch) != 0)
			ptr = (char *) NULL;
		else if (is_exithelp(ch) != 0)
			ptr = subject;
	}

	if (subjects)
		free((void *) subjects);

	return (ptr);
}

static int
view_help(help_t cat, h_index_t * node)
{
	int nlines;		/* n lines of help  */
	int lines_per_page;	/* n lines per displayed page */
	int ch;			/* input character  */
	int dirty;
	int top_line;
	int top_row = 3;
	int row;
	int exit_status;

	u_long keys;

	char *cp;		/* scratch pointer to help text */
	char **text;		/* pointer to array of lines */

	/* determine screen dimensions */
	lines_per_page = LINES - 5;	/* 5 for white space & title & */
	/* prompts */

	cp = xstrdup(node->text);
	nlines = fmt_text(cp, &text);

	top_line = 0;
	row = top_row = 2;

	/* show title */
	wheader(hwin, node->title);

	/*
	 * show options
	 */
	switch ((int) cat) {
	case HELP_TOPIC:
		keys = F_TOPICS | F_EXITHELP;
		break;
	case HELP_HOWTO:
		keys = F_HOWTO | F_EXITHELP;
		break;
	case HELP_REFER:
		keys = F_REFER | F_EXITHELP;
		break;
	case HELP_NONE:
	default:
		/* "can't happen" */
		keys = F_OKEYDOKEY;
		break;
	}
	wfooter(hwin, keys);

	dirty = 1;

	for (;;) {

		if (dirty) {

			show_help_text(hwin, text, top_line, top_row,
			    lines_per_page);

			scroll_prompts(hwin, top_row, 1, top_line, nlines,
			    lines_per_page);

		}
		(void) wmove(hwin, row, INDENT1);
		(void) wnoutrefresh(hwin);
		(void) doupdate();

		ch = wzgetch(hwin, keys);

		if (is_helpindex(ch)) {

			(void) werase(hwin);
			exit_status = DONE;
			break;

		} else if (is_exithelp(ch)) {

			exit_status = RETURN;
			break;

		} else if (ch == CTRL_D) {

			/* page down */
			if ((top_line + lines_per_page) < nlines) {

				/* advance a page */
				top_line += lines_per_page;
				dirty = 1;

			} else if ((top_line + row) < nlines) {

				/* advance to last line */
				row += (nlines - top_line) - 1;
				top_line = row - 2;
				dirty = 1;

			} else
				beep();	/* at end */

		} else if (ch == CTRL_U) {

			/* page up */
			if ((top_line - lines_per_page) >= 0) {

				/* reverse a page */
				top_line = (top_line > lines_per_page ?
				    top_line - lines_per_page : 0);
				dirty = 1;

			} else if (row > top_row) {

				/* back to first line */
				top_line = 0;
				row = top_row;
				dirty = 1;

			} else
				beep();	/* at top */

		} else if (ch == D_ARROW || ch == CTRL_N || ch == CTRL_F) {

			if (row == top_row + lines_per_page - 1) {

				/* last row */
				if (top_line + lines_per_page < nlines) {

					/* more lines, scroll up */
					top_line++;
					dirty = 1;

				} else
					beep();	/* bottom */

			} else if (row < (top_row + lines_per_page - 1)) {

				/* down a row, more lines? */
				if ((top_line + row) < nlines)
					row++;
				else
					beep();	/* last, no wrap */

			} else
				beep();	/* last, no wrap */


		} else if (ch == U_ARROW || ch == CTRL_P || ch == CTRL_B) {

			if (row == top_row) {

				if (top_line) {	/* scroll down */
					--top_line;
					dirty = 1;
				} else
					beep();	/* very top_line */

			} else
				row--;

		} else if (ch != ESCAPE)
			beep();

	}

	/* free extra copy of text */
	if (text)
		free(text);

	return (exit_status);
}

/*
 * show npp lines of text from `lines' array, starting with `first'
 * and in row r on window 'w'
 */
static void
show_help_text(WINDOW * w, char **text, int first, int r, int npp)
{
	int		last;
	int		row;
	int		i;

	last = r + npp - 1;

	for (row = r; row <= last; ++row) {
		(void) wmove(w, row, 0);
		(void) wclrtoeol(w);
	}

	for (row = r, i = first; text[i] && row <= last; ++row, ++i) {
		(void) mvwprintw(w, row, INDENT1, "%.*s", COLS - INDENT1,
		    text[i]);
	}
	wnoutrefresh(w);

}

/*
 * break text pointed at by cp into lines terminated by NULL.
 * save pointer to beginning of each line into a malloc'ed array
 * and return the array in text, the array must be freed by the caller.
 * return number of lines.
 */
static int
fmt_text(char *cp, char ***text)
{
	int		i;
	int		last;
	char	   *nl;
	char	  **tmp;

	tmp = (char **) xmalloc(32 * sizeof (char *));
	(void) memset((void *)tmp, 0, 32 * sizeof (char *));

	i = 0;
	last = 32;

	while (cp) {

		if ((nl = strchr(cp, '\n')) != (char *) NULL)
			*nl = '\0';

		tmp[i++] = (char *) cp;

		if (i == last) {	/* need more buckets */
			tmp = (char **) xrealloc((void *) tmp,
			    (last + 32) * sizeof (char *));
			(void) memset((void *)(tmp + last), 0,
			    32 * sizeof (char *));
			last += 32;
		}

		if (nl != (char *) NULL) {
			cp = (nl + 1);
		} else
			cp = nl;	/* terminate loop */
	}

	*text = tmp;
	return (i);
}

/*
 * load the help text corresponding to category/node->title.
 * uses mmap() instead of creating a copy of the text.
 */
static int
read_help_text(char *cat, h_index_t * node)
{
	int		fd = -1;
	struct stat	status;
	caddr_t		pa = NULL;
	char		fname[MAXPATHLEN];

	/* if this file hasn't been processed in yet, mmap() it */
	if (node->text == (char *) NULL) {

		(void) sprintf(fname, "%s/%s/%s", helpdir, cat, node->fname);

		if ((fd = open(fname, O_RDONLY, 0)) == -1)
			return (-1);

		if (fstat(fd, &status) == -1) {
			return (-1);
		}

		if ((pa = mmap((caddr_t) NULL, status.st_size, PROT_READ,
			    MAP_SHARED, fd, (off_t) 0)) == (caddr_t) - 1) {
			return (-1);
		}

		if (close(fd) == -1) {
			return (-1);
		}

		/* mmap'ed OK.	Save off the pointer...	 */
		node->text = (char *) pa;
	}
	return (0);
}
/*
 * parse and read help index file.  code lifted from the GUI version of
 * admin help....
 */
static h_index_t *
load_index(char *cat, char *fname)
{
	char	tocfile[1024], label[1024], filename[1024], buf[1024];

	h_index_t	*head = (h_index_t *) NULL, *tail;
	int		i;
	int		count;

	char	   *r;
	FILE	   *fp;

	(void) sprintf(tocfile, "%s/%s/%s", helpdir, cat, fname);

	if ((fp = fopen(tocfile, "r")) == (FILE *) NULL) {
		(void) fprintf(stderr, TTY_HELP_OPEN_FILE_ERR, tocfile);
		return ((h_index_t *) NULL);
	}
	count = 0;
	for (i = 1; r = fgets(label, 1024, fp); i++) {

		if (!r) {

			syntax(tocfile, i, TTY_HELP_EOF_ERR);
			break;

		}
		if (*label == '!')
			continue;	/* it's a comment */

		if (*label == '\t') {

			syntax(tocfile, i, TTY_HELP_LEADING_TAB_ERR);
			continue;

		}
		if (*label == '\n') {	/* skip blank lines too */

			syntax(tocfile, i, TTY_HELP_BLANK_LINE_ERR);
			continue;

		}
		*strchr(label, '\n') = '\0';
		i++;

		if (!fgets(buf, 1024, fp)) {

			syntax(tocfile, i, TTY_HELP_EOF_ERR);
			break;

		}
		/* keep reading 'til a valid filename line is found */
		while (*buf != '\t') {

			i++;
			if (!(r = fgets(buf, 1024, fp))) {

				syntax(tocfile, i, TTY_HELP_EOF_ERR);
				break;

			}
		}
		if (!r)
			break;

		if (sscanf(buf, "%s", filename) != 1)
			syntax(tocfile, i, TTY_HELP_EXCESS_TOKENS_ERR);

		if (!head) {

			head = tail = (h_index_t *) xmalloc(sizeof (h_index_t));
			head->fname = xstrdup(filename);
			head->title = xstrdup(label);
			head->text = (char *)0;
			head->next = (h_index_t *)0;

		} else {

			tail->next = (h_index_t *) xmalloc(sizeof (h_index_t));
			tail = tail->next;
			tail->fname = xstrdup(filename);
			tail->title = xstrdup(label);
			tail->text = (char *)0;

		}

		tail->next = NULL;
		count++;
	}

	(void) fclose(fp);
	return (head);
}

static void
syntax(char *filename, int line, char *string)
{
	(void) fprintf(stderr, TTY_HELP_SYNTAX_ERR,
	    filename, line, string);
}

/* utility to count number of elements in `list' */
static int
count_items(h_index_t * list)
{
	register h_index_t *tmp;
	register int    i;

	for (tmp = list, i = 0;
	    tmp != (h_index_t *) NULL;
	    tmp = tmp->next, i++);

	return (i);
}
