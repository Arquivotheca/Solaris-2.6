/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#ifndef lint
#ident	"@(#)xv_admin.c 1.7 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include "swmtool.h"

#define	NCHOICES	14		/* XXX must match GUI */

static	Xv_opaque items[NCHOICES];	/* array of panel items */
static 	Config	ui_config;		/* displayed configuration */

#ifdef __STDC__
static void GetMail(void);
static void SetMail(void);
#else
static void GetMail();
static void SetMail();
#endif

void
#ifdef __STDC__
GetAdmin(void)
#else
GetAdmin()
#endif
{
	Choice	*chp;
	register int i;

	/*
	 * Copy in program config parameters.
	 * We are supposed to treat the strings we
	 * get as read-only, so make our own copy
	 * of the mail and host strings.
	 */
	free(ui_config.admin.mail);
	free(ui_config.hosts);

	config_get(&ui_config);

	ui_config.admin.mail = xstrdup(ui_config.admin.mail);
	ui_config.hosts = xstrdup(ui_config.hosts);

	for (i = 0; i < NCHOICES; i++) {
		chp = (Choice *)xv_get(items[i], PANEL_CLIENT_DATA);
		if (chp->value == -1 || chp->value >= chp->nchoice)
			chp->value = 0;
		xv_set(items[i], PANEL_VALUE, chp->value, NULL);
	}
	GetMail();
}

void
#ifdef __STDC__
SetAdmin(void)
#else
SetAdmin()
#endif
{
	Choice	*chp;
	Config	config;
	register int i;

	/*
	 * The host data is maintained by the
	 * host properties code, so we need to
	 * be careful to preserve it.  We free
	 * our old copy, then grab and duplicate
	 * the new data.
	 */
	free(ui_config.hosts);
	config_get(&config);
	ui_config.hosts = xstrdup(config.hosts);

	for (i = 0; i < NCHOICES; i++) {
		chp = (Choice *)xv_get(items[i], PANEL_CLIENT_DATA);
		if (chp->value == -1 || chp->value >= chp->nchoice)
			chp->value = 0;
		chp->value = (int)xv_get(items[i], PANEL_VALUE);
	}
	SetMail();
	config_set(&ui_config);
}

static void
#ifdef __STDC__
GetMail(void)
#else
GetMail()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	int	nrows = (int)xv_get(ip->ConfigMailList, PANEL_LIST_NROWS);
	char	*cp, *next;
	char	namebuf[256];
	register int i;

	if (nrows)
		xv_set(ip->ConfigMailList,
		    PANEL_LIST_DELETE_ROWS,	0,	nrows,
		    NULL);
#ifdef lint
	next = namebuf;
#endif

	for (cp = ui_config.admin.mail, i = 0; *cp; cp = next) {
		while (*cp && (isspace((u_int)*cp) || *cp == ','))
			cp++;
		next = cp;
		while (*next && !isspace((u_int)*next) && *next != ',')
			next++;
		if (next != cp) {
			(void) strncpy(namebuf, cp, next - cp);
			namebuf[next - cp] = '\0';
			xv_set(ip->ConfigMailList,
			    PANEL_LIST_INSERT,	i,
			    PANEL_LIST_STRING,	i,	namebuf,
			    PANEL_LIST_SELECT,	i,	TRUE,
			    NULL);
			++i;
		}
	}
}

static void
#ifdef __STDC__
SetMail(void)
#else
SetMail()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	int	nrows = (int)xv_get(ip->ConfigMailList, PANEL_LIST_NROWS);
	char	mailbuf[MAXMAIL + 1], *name;
	int	used, left;
	register int i;

	mailbuf[0] = '\0';
	mailbuf[sizeof (mailbuf) - 1] = '\0';
	used = 0;
	left = sizeof (mailbuf);
	for (i = 0; i < nrows; i++) {
		name = (char *)xv_get(ip->ConfigMailList, PANEL_LIST_STRING, i);
		left -= (strlen(name) + 2);	/* ' ' and '\0' */
		if (left < 0) {
			asktoproceed(Adminscreen, gettext(
				"Maximum length of mail (%s) string\n\
would be exceeded; truncating before\nrecipient \"%s\""),
				MAXMAIL, name);
			break;
		}
		(void) sprintf(&mailbuf[used], " %s", name);
		used += (strlen(name) + 1);	/* add 1 for space */
	}
	if (used > (int)strlen(ui_config.admin.mail))
		ui_config.admin.mail = xrealloc(ui_config.admin.mail, used + 1);
	(void) strncpy(ui_config.admin.mail, mailbuf, used);
	ui_config.admin.mail[used] = '\0';
}

void
#ifdef __STDC__
AddMail(void)
#else
AddMail()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	char	*name = (char *)xv_get(ip->ConfigRecip, PANEL_VALUE);
	int	row =
		    (int)xv_get(ip->ConfigMailList, PANEL_LIST_FIRST_SELECTED);

	/*
	 * Add a host with the given name to the
	 * list after the selected position (or
	 * bottom, if none selected).
	 */
	if (row == -1)
		row = (int)xv_get(ip->ConfigMailList, PANEL_LIST_NROWS);
	else
		row += 1;	/* for append */

	if (name == (char *)0 || name[0] == '\0') {
		asktoproceed(Adminscreen, gettext(
		    "First enter a recipient address, then press \"Add\"."));
		return;
	}

	xv_set(ip->ConfigMailList,
	    PANEL_LIST_INSERT,		row,
	    PANEL_LIST_STRING,		row,	name,
	    PANEL_LIST_SELECT,		row,	TRUE,
	    NULL);
}

void
#ifdef __STDC__
DeleteMail(void)
#else
DeleteMail()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	int	nrows = (int)xv_get(ip->ConfigMailList, PANEL_LIST_NROWS);
	int	row =
	    (int)xv_get(ip->ConfigMailList, PANEL_LIST_FIRST_SELECTED);

	if (row == -1) {
		asktoproceed(Adminscreen, gettext(
			"You have not selected a recipient."));
		return;
	}
	xv_set(ip->ConfigMailList, PANEL_LIST_DELETE, row, NULL);

	/*
	 * Just deleted a row.  If we deleted the
	 * last row and there are rows left, select
	 * the new last row.  If we deleted something
	 * other than the last row, select the new
	 * occupant of the deleted position.
	 */
	if (--nrows > 0) {
		if (row >= nrows)
			xv_set(ip->ConfigMailList,
				PANEL_LIST_SELECT, nrows - 1, TRUE, NULL);
		else
			xv_set(ip->ConfigMailList,
				PANEL_LIST_SELECT, row, TRUE, NULL);
	}
}

void
#ifdef __STDC__
ChangeMail(void)
#else
ChangeMail()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	char	*name = (char *)xv_get(ip->ConfigRecip, PANEL_VALUE);
	int	row =
		    (int)xv_get(ip->ConfigMailList, PANEL_LIST_FIRST_SELECTED);

	if (row == -1) {
		asktoproceed(Adminscreen, gettext(
			"To change an entry, select it from\n"
			"the list, enter a new recipient name,\n"
			"then press \"Change\"."));
		return;
	}
	if (name == (char *)0 || name[0] == '\0') {
		asktoproceed(Adminscreen, gettext(
	    "First enter a recipient name, then press \"Change\"."));
		return;
	}
	xv_set(Props_PropsWin->ConfigMailList,
	    PANEL_LIST_STRING,		row,	name,
	    NULL);
}

void
InitConfig(caddr_t instance)
{
	/*LINTED [alignment ok]*/
	Props_PropsWin_objects *ip = (Props_PropsWin_objects *)instance;
	File_FileWin_objects *sw = File_FileWin;
	Admin	*admin = &ui_config.admin;
	Choice	*chp;
	register int	i, j;
	char	pathbuf[MAXPATHLEN], *cp;

	/*
	 * Copy in program config parameters
	 */
	config_get(&ui_config);

	/*
	 * We are supposed to treat the strings we
	 * get as read-only, so make our own copy
	 * of the mail and host strings.
	 */
	ui_config.admin.mail = xstrdup(ui_config.admin.mail);
	ui_config.hosts = xstrdup(ui_config.hosts);

	i = 0;
	xv_set(ip->ConfigConflict, PANEL_CLIENT_DATA, &admin->conflict, NULL);
	items[i++] = ip->ConfigConflict;

	xv_set(ip->ConfigInstance, PANEL_CLIENT_DATA, &admin->instance, NULL);
	items[i++] = ip->ConfigInstance;

	xv_set(ip->ConfigPartial, PANEL_CLIENT_DATA, &admin->partial, NULL);
	items[i++] = ip->ConfigPartial;

	xv_set(ip->ConfigSetuid, PANEL_CLIENT_DATA, &admin->setuid, NULL);
	items[i++] = ip->ConfigSetuid;

	xv_set(ip->ConfigAction, PANEL_CLIENT_DATA, &admin->action, NULL);
	items[i++] = ip->ConfigAction;

	xv_set(ip->ConfigIdepend, PANEL_CLIENT_DATA, &admin->idepend, NULL);
	items[i++] = ip->ConfigIdepend;

	xv_set(ip->ConfigRdepend, PANEL_CLIENT_DATA, &admin->rdepend, NULL);
	items[i++] = ip->ConfigRdepend;

	xv_set(ip->ConfigRunlevel, PANEL_CLIENT_DATA, &admin->runlevel, NULL);
	items[i++] = ip->ConfigRunlevel;

	xv_set(ip->ConfigSpace, PANEL_CLIENT_DATA, &admin->space, NULL);
	items[i++] = ip->ConfigSpace;

	xv_set(ip->ConfigVerbose, PANEL_CLIENT_DATA, &ui_config.showcr, NULL);
	items[i++] = ip->ConfigVerbose;

	xv_set(ip->ConfigInteract, PANEL_CLIENT_DATA, &ui_config.askok, NULL);
	items[i++] = ip->ConfigInteract;

	/*
	 * Display parameters
	 */
	xv_set(ip->DispIconic, PANEL_CLIENT_DATA, &ui_config.gui_mode, NULL);
	items[i++] = ip->DispIconic;

	xv_set(ip->DispName, PANEL_CLIENT_DATA, &ui_config.namelen, NULL);
	items[i++] = ip->DispName;

	xv_set(ip->DispCols, PANEL_CLIENT_DATA, &ui_config.ncols, NULL);
	items[i++] = ip->DispCols;

	for (i = 0; i < NCHOICES; i++) {
		chp = (Choice *)xv_get(items[i], PANEL_CLIENT_DATA);
		for (j = 0; j < chp->nchoice; j++) {
			chp->prompts[j] = xstrdup(gettext(chp->cprompts[j]));
			xv_set(items[i],
			    PANEL_CHOICE_STRING, j, chp->prompts[j], NULL);
		}
	}

	GetAdmin();
	ResetDisplayModes();

	(void) strcpy(pathbuf, config_file((char *)0));
	cp = strrchr(pathbuf, '/');
	if (cp != (char *)0) {
		*cp = '\0';
		xv_set(sw->FileDir, PANEL_VALUE, pathbuf, NULL);
		xv_set(sw->FileName, PANEL_VALUE, cp+1, NULL);
	} else {
		xv_set(sw->FileDir, PANEL_VALUE, (char *)0, NULL);
		xv_set(sw->FileName, PANEL_VALUE, pathbuf, NULL);
	}
}

void
ConfigFile(CF_mode mode)
{
	File_FileWin_objects *ip = File_FileWin;
	Config	config;
	char	filename[MAXPATHLEN];
	char	*dircomp, *filecomp, *str;

	switch (mode) {
	case CONFIG_SAVEAS:
		/*
		 * Display the window and
		 * indicate "save"
		 */
		xv_set(ip->FileDoit,
		    PANEL_LABEL_STRING,	gettext("Save"),
		    PANEL_CLIENT_DATA,	CONFIG_SAVE,
		    NULL);
		xv_set(ip->FileWin,
		    XV_LABEL,	gettext("Software Manager:  Save As"),
		    XV_SHOW,	TRUE,
		    NULL);
		return;
		/* NOTREACHED */
	case CONFIG_SAVE:
		dircomp = (char *)xv_get(ip->FileDir, PANEL_VALUE);
		filecomp = (char *)xv_get(ip->FileName, PANEL_VALUE);
		if (dircomp == (char *)0 || *dircomp == '\0')
			dircomp = ".";
		/*
		 * Strip leading white-space from both
		 * the directory and file name components
		 */
		while (*dircomp && isspace((u_char)*dircomp))
			dircomp++;
		while (*filecomp && isspace((u_char)*filecomp))
			filecomp++;
		(void) sprintf(filename, "%s/%s", dircomp, filecomp);
		if (filename[0] == '\0') {
			asktoproceed(ip->FileWin, gettext(
			    "Attempt to save current configuration aborted.\n\
You did not specify a file name."));
			return;
		}
		config_get(&config);

		if (path_is_writable(filename) == SUCCESS &&
		    confirm(ip->FileWin,
			xstrdup(gettext("Cancel")),
			xstrdup(gettext("Overwrite")),
			xstrdup(gettext("File `%s' exists.\n\
Do you wish to overwrite it with your new\n\
configuration or cancel the save attempt?")), filename) == 1) {
				str = gettext("Configuration not saved");
		} else {
			xv_set(ip->FileWin, PANEL_BUSY, TRUE, NULL);
			if (config_write(filename, &config) < 0) {
				asktoproceed(ip->FileWin, gettext(
			    "Warning:  your configuration could not be\n\
saved to file `%s':  %s\n"),
					filename, strerror(errno));
				str = gettext("Configuration not saved");
			} else {
				str = gettext("Configuration saved");
				(void) config_file(filename);
			}
			xv_set(ip->FileWin, PANEL_BUSY, FALSE, NULL);
		}
		if (xv_get(ip->FileWin, XV_SHOW) == TRUE)
			msg(ip->FileWin, 1, str);
		break;
	case CONFIG_LOAD:
		/*
		 * Display the window and
		 * indicate "load"
		 */
		xv_set(ip->FileDoit,
		    PANEL_LABEL_STRING,	gettext("Load"),
		    PANEL_CLIENT_DATA,	mode,
		    NULL);
		xv_set(ip->FileWin,
		    XV_LABEL,	gettext("Software Manager:  Load"),
		    XV_SHOW,	TRUE,
		    NULL);
		return;
		/* NOTREACHED */
	}

}
