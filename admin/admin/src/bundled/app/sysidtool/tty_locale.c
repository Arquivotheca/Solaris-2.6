/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)tty_locale.c 1.15 96/09/26"

/*
 *	File:		locale.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user for the desired locale.
 */

#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "tty_defs.h"
#include "tty_msgs.h"

/*
 * Assume a minimal 24 x 80 screen
 */
#define	SCR_LINES 23
#define	COL_WIDTH 38

#define	is_odd(x) ((x) & 1)

static void	print_lang_list(Menu *, int);
static void	print_locale_list(Menu *, int, int);
static int	get_num(void);
static void	print_l10n(int, int, int, char *);

/*
 * do_get_locale:
 *
 *	This routine is the client interface routine used for
 *	retrieving the desired locale from the user.
 *	*_get_locale() should be called first to see if we need to prompt
 *	the user and to create the menu list.
 *
 */

void
do_get_locale(
	char		*locale,
	Field_desc	*languages,
	int		reply_to)
{
	int		loc_select;
	int		select;
	int		i, j;
	Menu		*language_menu;
	int		nlanguages;
	int		nlocales;
	Menu		*loc_menu;
	int		n;
	int		loc_pick;
	MSG		*mp;

	language_menu = (Menu *)languages->value;
	nlanguages = language_menu->nitems;


	/*
	 * If we're on an Asian image, don't prompt for Asian locales since
	 * we can't print those characters in the CUI.  In this case,
	 * always return English (select == 0) as the locale choice.
	 */
	for (i = 0; i < nlanguages; i++)
		for (j = 0; mb_locales[j]; j++)
			if (strcmp(language_menu->labels[i],
				mb_locales[j]) == 0) {

				/* Cleanup */
				locale = "C";
				n = 0;
				loc_pick = 0;

				mp = msg_new();
				(void) msg_set_type(mp, REPLY_OK);
				(void) msg_add_arg(mp,
					ATTR_LOCALEPICK, VAL_STRING,
					(void *)locale, MAX_TZ+1);
				(void) msg_add_arg(mp,
					ATTR_LOC_LANG, VAL_INTEGER,
					(void *)&n, sizeof (n));
				(void) msg_add_arg(mp,
					ATTR_LOC_INDEX, VAL_INTEGER,
					(void *)&loc_pick, sizeof (loc_pick));
				(void) msg_send(mp, reply_to);
				msg_delete(mp);
				return;
			}

	/* main prompt loop */
	do {
		if (nlanguages > 1) {
			/*
			 * only display the language list if there
			 * is more than one item to display
			 */
			print_lang_list(language_menu, nlanguages);
			select = get_num();
		} else {
			/*
			 * if there is only one language, select it for
			 * the user so they don't get prompted to select
			 * something from a list that only contains one item
			 */
			select = 0;
		}
		if (select >= 0 && select < nlanguages) {
			loc_menu = ((Menu **)language_menu->values)[select];
			nlocales = loc_menu->nitems;
		}
	} while (select < 0 || select >= nlanguages);

	setlocale(LC_MESSAGES, ((char **)language_menu->client_data)[select]);

	do {
		print_locale_list(loc_menu, nlanguages, nlocales);
		loc_select = get_num();
		if (loc_select == nlocales) {
			/*
			 * return to previous was selected, prompt for
			 * language again
			 */
			do {
				if (nlanguages > 1) {
					print_lang_list(language_menu, nlanguages);
					select = get_num();
				} else {
					select = 0;
				}
				if (select >= 0 && select < nlanguages) {
					loc_menu = ((Menu **)language_menu->
						values)[select];
					nlocales = loc_menu->nitems;
				}
				loc_select = -1;
			} while (select < 0 || select >= nlanguages);


	setlocale(LC_MESSAGES, ((char **)language_menu->client_data)[select]);

		}
	} while (loc_select < 0 || loc_select >= nlocales);

	(void) strcpy(locale,
			((char **)loc_menu->client_data)[loc_select]);
	n = select;
	loc_pick = loc_select;
	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_LOCALEPICK, VAL_STRING,
				(void *)locale, MAX_TZ+1);
	(void) msg_add_arg(mp, ATTR_LOC_LANG, VAL_INTEGER,
				(void *)&n, sizeof (n));
	(void) msg_add_arg(mp, ATTR_LOC_INDEX, VAL_INTEGER,
				(void *)&loc_pick, sizeof (loc_pick));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);

}

/*
 * print_list
 *
 *	Print the list of available l10n's
 */

static  void
print_lang_list(Menu *menu, int n_langs)
{
	char	current_locale[MAX_LOCALE];
	int	i;

	/* clear the screen (don't know term type yet) */
	for (i = 0; i < 66; i++)
		(void) putchar('\n');

	(void) strcpy(current_locale, setlocale(LC_MESSAGES, (char *)0));
	
	(void) printf("%s\n\n", SELECT_LANG_TITLE);

	if (n_langs < SCR_LINES) {
		for (i = 0; i < n_langs; i++) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[i] =
				xstrdup(menu->labels[i]);

			/*
			 * print the l10n string for the label
			 */
			print_l10n(1, 1, i, menu->labels[i]);
		}
	} else {
		int cnt, offset, num;

		cnt = n_langs / 2;

		offset = is_odd(n_langs) ? cnt + 1: cnt;

		for (i = 0; i < cnt; i++) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[i] =
				xstrdup(menu->labels[i]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(1, 2, i, menu->labels[i]);

			num = i + offset;

			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[num] =
				xstrdup(menu->labels[num]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(1, 2, num, menu->labels[num]);
			putchar('\n');
		}

		if (is_odd(n_langs)) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[cnt] =
				xstrdup(menu->labels[cnt]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(1, 2, cnt, menu->labels[cnt]);
			putchar('\n');
		}
	}

	(void) setlocale(LC_MESSAGES, current_locale);

	/* Then, print the selection prompt */
	(void) printf("\n?");
	(void) fflush(stdout);
}

static  void
print_locale_list(Menu *menu, int n_languages, int n_locales)
{
	char	current_locale[MAX_LOCALE];
	int	i;
	char	buff[512];

	/* clear the screen (don't know term type yet) */
	for (i = 0; i < 66; i++)
		(void) putchar('\n');

	(void) strcpy(current_locale, setlocale(LC_MESSAGES, (char *)0));

	(void) printf("%s\n\n", SELECT_LOCALE_TITLE);
	(void) printf("%s\n\n", SELECT_LOCALE_TEXT);

	if (n_locales < SCR_LINES) {
		for (i = 0; i < n_locales; i++) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[i] =
				xstrdup(menu->labels[i]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(0, 1, i, menu->labels[i]);
		}

		if (n_languages > 1) {
			(void) sprintf(buff, RETURN_TO_PREVIOUS, n_locales);
			(void) printf("%s", buff);
		}
	} else {
		int cnt, offset, num;

		cnt = n_locales / 2;

		offset = is_odd(n_locales) ? cnt + 1: cnt;

		for (i = 0; i < cnt; i++) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[i] =
				xstrdup(menu->labels[i]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(0, 2, i, menu->labels[i]);

			num = i + offset;

			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[num] =
				xstrdup(menu->labels[num]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(0, 2, num, menu->labels[num]);
			putchar('\n');
		}

		if (is_odd(n_locales)) {
			/*
			 * save the old label in the client data
			 */
			((char **)menu->client_data)[cnt] =
				xstrdup(menu->labels[cnt]);
			/*
			 * print the l10n string for the label
			 */
			print_l10n(0, 2, cnt, menu->labels[cnt]);

			if (n_languages > 1) {
				(void) sprintf(buff, RETURN_TO_PREVIOUS, n_locales);
				(void) printf("%s", buff);
			}
			putchar('\n');
		}
	}

	(void) setlocale(LC_MESSAGES, current_locale);

	/* Then, print the selection prompt */
	(void) printf("\n\n%s", gettext(ENTER_A_NUMBER));
	(void) fflush(stdout);
}

/*
 * get_num
 *
 *	Read stdin, and return int value, return -1 if error
 *
 */

static	int
get_num(void)
{
	int	i;
	char	buf[BUFSIZ];

	/* Read stdin, return err if nothing entered */
	i = read(0, buf, BUFSIZ);
	buf[i] = NULL;

	if (i == 0)
		return (-1);

	/* Verify selection syntax */
	for (i = 0; buf[i]; i++)
		if (!isdigit(buf[i]))
			switch (buf[i]) {
			case '\n':
			case  ' ':
				break;
			default:
				return (-1);
			}

	return (atoi(buf));
}

static void
print_l10n(int lang_only, int col, int num, char *dom)
{
	int l1, l2, status;
	char buff[512], buff2[512], buff3[512], buff4[512];

	(void) setlocale(LC_MESSAGES, dom);
	status = get_l10n_string(lang_only, dom, buff3, buff4, 512);
	switch (status) {
	case 1: /* locales without translation */
		(void) sprintf(buff2, ENTER_THIS_ITEM, num, num);
		(void) sprintf(buff, "%s %s", buff2, buff4);
		break;
	case 0: /* locales with translated strings */
		(void) sprintf(buff2, gettext(ENTER_THIS_ITEM), num, num);
		(void) sprintf(buff, "%s %s (%s)", buff2, buff3, buff4);
		break;
	case -1: /* no locale description */
		(void) sprintf(buff2, gettext(ENTER_THIS_ITEM), num, num);
		(void) sprintf(buff, "%s %s", buff2, dom);
		break;
	}
	
	/*
	 * make sure that the output string is left justified
	 * and only prints out in as much space as it has.
	 */
	if (col == 1) {
		printf("%*.*s ", -(2 * COL_WIDTH), (2 * COL_WIDTH), buff);
		putchar('\n');
	} else {
		printf("%*.*s ", -COL_WIDTH, COL_WIDTH, buff);
	}
}
