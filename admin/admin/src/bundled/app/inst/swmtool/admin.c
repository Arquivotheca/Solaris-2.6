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
#ident	"@(#)admin.c 1.7 93/04/09"
#endif

#include "defs.h"
#include "hash.h"
#include "admin.h"
#include "ui.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct symbol {
	char	*name;		/* config file keyword */
	int	flags;		/* see below */
	caddr_t	value;		/* address of keyword data */
};

#define	SYM_ADMIN	0x1	/* gets written to admin file */
#define	SYM_CHOICE	0x10	/* value is Choice struct */
#define	SYM_STRING	0x20	/* value is character string */

/*
 * current configuration parameters
 */
static Config config = {
	/*
	 * current admin parameters -- these
	 * initial values are the defaults
	 */
	{
		/*
		 * mail -- initialize default w/xstrdup in config_init() XXX
		 */
		(char *)0,
		/*
		 * instance
		 */
		{
			0,		/* value */
			4,		/* nchoices */
			{		/* keywords */
				"ask",
				"overwrite",
				"unique",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Overwrite",
				"Install Unique",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0
			}
		},
		/*
		 * partial
		 */
		{
			0,		/* value */
			3,		/* nchoices */
			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Ignore",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * runlevel
		 */
		{
			0,		/* value */
			3,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Ignore",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * idepend
		 */
		{
			0,		/* value */
			3,		/* nchoices */
			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Ignore",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * rdepend
		 */
		{
			0,		/* value */
			3,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Ignore",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * space
		 */
		{
			0,		/* value */
			3,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Ignore",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * setuid
		 */
		{
			0,		/* value */
			4,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"nochange",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Yes",
				"No",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * action
		 */
		{
			0,		/* value */
			3,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Yes",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * conflict
		 */
		{
			0,		/* value */
			4,		/* nchoices */

			{		/* keywords */
				"ask",
				"nocheck",
				"nochange",
				"quit",
			}, {		/* C-locale prompts */
				"Ask",
				"Overwrite",
				"Skip",
				"Abort",
			}, {		/* translated prompts */
				0, 0, 0, 0,
			},
		},
		/*
		 * defualt basedir
		 */
		"default",
	},
	/*
	 * show cr
	 */
	{
		0,		/* value */
		2,		/* nchoices */

		{		/* keywords */
			"yes",
			"no",
		}, {		/* C-locale prompts */
			"Yes",
			"No",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * askok
	 */
	{
		0,		/* value */
		2,		/* nchoices */

		{		/* keywords */
			"yes",
			"no",
		}, {		/* C-locale prompts */
			"Yes",
			"No",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * eject
	 */
	{
		0,		/* value */
		2,		/* nchoices */

		{		/* keywords */
			"yes",
			"no",
		}, {		/* C-locale prompts */
			"Yes",
			"No",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * display params:  display mode
	 */
	{
		0,		/* value */
		3,		/* nchoices */

		{		/* keywords */
			"auto",
			"icon",
			"text",
		}, {		/* C-locale prompts */
			"Automatic",
			"Iconic",
			"Textual",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * display params:  name length
	 */
	{
		0,		/* value */
		2,		/* nchoices */

		{		/* keywords */
			"long",
			"short",
		}, {		/* C-locale prompts */
			"Long",
			"Short",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * display params:  number of columns
	 */
	{
		0,		/* value */
		3,		/* nchoices */

		{		/* keywords */
			"1",
			"2",
			"3",
		}, {		/* C-locale prompts */
			"1",
			"2",
			"3",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	/*
	 * display params:  startup-notice
	 */
	{
		0,		/* value */
		2,		/* nchoices */

		{		/* keywords */
			"yes",
			"no",
		}, {		/* C-locale prompts */
			"Displayed",
			"Not Displayed",
		}, {		/* translated prompts */
			0, 0, 0, 0,
		},
	},
	(char *)0,		/* list of spool directories */
	(char *)0,		/* remote mount point for pkg spool dir */
	(char *)0,		/* target host list */
	0			/* saved to file */
};

static Config configbuf;	/* scratch config struct */
static char configfile[MAXPATHLEN] = CONFIG_DEFAULT;

static struct symbol symtab[] = {
	{
		"showcr",
		SYM_CHOICE,
		(caddr_t)&configbuf.showcr
	},
	{
		"askok",
		SYM_CHOICE,
		(caddr_t)&configbuf.askok
	},
	{
		"eject",
		SYM_CHOICE,
		(caddr_t)&configbuf.eject
	},
	{
		"gui_mode",
		SYM_CHOICE,
		(caddr_t)&configbuf.gui_mode
	},
	{
		"namelen",
		SYM_CHOICE,
		(caddr_t)&configbuf.namelen
	},
	{
		"ncols",
		SYM_CHOICE,
		(caddr_t)&configbuf.ncols
	},
	{
		"notice",
		SYM_CHOICE,
		(caddr_t)&configbuf.notice
	},
	{
		"spooldir",
		SYM_STRING,
		(caddr_t)&configbuf.spooldirs
	},
	{
		"rmntdir",
		SYM_STRING,
		(caddr_t)&configbuf.rmntdir
	},
	{
		"hosts",
		SYM_STRING,
		(caddr_t)&configbuf.hosts
	},
	/*
	 * The following keywords are specified
	 * by the ABI for the package admin file
	 */
	{
		"mail",
		SYM_ADMIN|SYM_STRING,
		(caddr_t)&configbuf.admin.mail
	},
	{
		"instance",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.instance
	},
	{
		"partial",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.partial
	},
	{
		"runlevel",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.runlevel
	},
	{
		"idepend",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.idepend
	},
	{
		"rdepend",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.rdepend
	},
	{
		"space",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.space
	},
	{
		"setuid",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.setuid
	},
	{
		"conflict",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.conflict
	},
	{
		"action",
		SYM_ADMIN|SYM_CHOICE,
		(caddr_t)&configbuf.admin.action
	},
	{
		"basedir",
		SYM_ADMIN|SYM_STRING,
		(caddr_t)&configbuf.admin.basedir
	},
	{
		(char *)0,
		0,
		(caddr_t)0
	}
};

static List	*hashsym;	/* hashed list of above symbols */

extern char	*fpkgparam(FILE *, char *);		/* from libadm */
static int	choice_value(char *, char **, int);

/*
 * Initialize configuration file data structures.  The
 * main work here is to create a hashed list of all the
 * keyword structures for quick lookup.  We also initialize
 * the mail and spooldirs members of "config", because
 * they need to be dynamically allocated (they may be
 * realloc'ed later in the program).
 */
void
config_init(void)
{
	register struct symbol *sym;
	Node	*tnode;
	char	*cfname;
	int	nocfile = 0;

	if (hashsym != (List *)0)
		return;

	hashsym = getlist();
	if (hashsym == (List *)0)
		die(gettext("PANIC:  cannot allocate symbol table\n"));

	for (sym = symtab; sym->value != (caddr_t)0; sym++) {
		tnode = getnode();
		if (tnode == (Node *)0)
			die(gettext(
			    "PANIC:  cannot allocate symbol table entry\n"));
		tnode->key = sym->name;
		tnode->data = (void *)sym;
		(void) addnode(hashsym, tnode);
	}

	cfname = config_file((char *)0);
	if (access(cfname, R_OK) < 0) {
		cfname = ADMIN_DEFAULT;
		nocfile++;
	}

	if (config_read(cfname, &config) == 0 && !nocfile)
		config.saved = 1;
	else
		config.saved = 0;

	if (config.hosts == (char *)0)
		config.hosts = xstrdup(thishost);
	/*
	 * Initialize real host list from
	 * hosts listed in file
	 */
	host_init(config.hosts);

	if (config.admin.mail == (char *)0)
		config.admin.mail = xstrdup(MAIL_DEFAULT);
	if (config.spooldirs == (char *)0)
		config.spooldirs = xstrdup(SPOOL_DEFAULT);
	if (config.rmntdir == (char *)0)
		config.rmntdir = xstrdup(RMNT_DEFAULT);
}

int
config_read(filename, conf)
	char	*filename;
	Config	*conf;
{
	struct symbol *sym;
	Node	*tnode;
	FILE	*fp;
	int	mail = 0;	/* saw mail parameter */
	char	param[1024];
	char	*value;

	configbuf = config;		/* use current values as defaults */

	if (filename == (char *)0)
		filename = config_file((char *)0);

	fp = fopen(filename, "r");
	if (fp == (FILE *)0)
		return (-1);

	param[0] = '\0';
	while (value = fpkgparam(fp, param)) {
		if (strcmp(param, "mail") == 0)
			mail = 1;
		if (value[0] == '\0') {
			param[0] = '\0';
			continue; /* same as not being set at all */
		}
		tnode = findnode(hashsym, param);
		if (tnode != (Node *)0) {
			sym = tnode->data;
			if (sym->flags & SYM_CHOICE) {
				/*LINTED [alignment ok]*/
				Choice	*chp = (Choice *)sym->value;
				chp->value = choice_value(
					value, chp->keywords, chp->nchoice);
			} else if (sym->flags & SYM_STRING) {
				/*LINTED [alignment ok]*/
				*(char **)sym->value = value;
			} else
				msg(Adminscreen, 1, gettext(
				    "Warning:  unknown symbol type <%x>\n"),
					sym->flags);
		}
		param[0] = '\0';
	}
	(void) fclose(fp);

	if (!mail)
		/* if we don't assign anything to "mail" */
		configbuf.admin.mail = xstrdup(MAIL_DEFAULT);

	*conf = configbuf;
	return (0);
}

#define	EXT	".tmp"

int
config_write(filename, conf)
	char	*filename;
	Config	*conf;
{
	register struct symbol *sym;
	register char *cp;
	char	tmpfile[MAXPATHLEN];
	FILE	*fp;
	int	errs = 0;
	/*
	 * i18n:  The funny placement of the shell comments
	 * is necessary to work around a bug in xgettext
	 */
	char	*comment = gettext("#\n# "
		"This file is generated and maintained by the Software\n# "
		"Manager, swm.  You may edit this file if you wish;\n# "
		"however, swm will not preserve formatting changes.\n#"
		"\n");

	if (filename == (char *)0)
		filename = config_file((char *)0);

	(void) sprintf(tmpfile, "%.*s%s",
		MAXPATHLEN - (strlen(EXT) + 1), filename, EXT);
	fp = fopen(tmpfile, "w");
	if (fp == (FILE *)0)
		return (-1);

	if (fprintf(fp, "%s", comment) == EOF)
		errs++;

	if (conf == (Config *)0)
		conf = &config;
	configbuf = *conf;

	errs = 0;
	for (sym = symtab; sym->value != (caddr_t)0; sym++) {
		if (fprintf(fp, "%s=", sym->name) == EOF)
			errs++;
		if ((sym->flags & SYM_STRING) &&
		    /*LINTED [alignment ok]*/
		    *(char **)sym->value != (char *)0) {
			/*LINTED [alignment ok]*/
			for (cp = *(char **)sym->value; *cp != '\0'; cp++) {
				switch (*cp) {
				case '\n':
					/* escape newlines */
					if (fputs("\\\n", fp) == EOF)
						errs++;
					break;
				case '\\':
					/* escape backslashes */
					if (fputc('\\', fp) == EOF)
						errs++;
					/* FALLTHROUGH */
				default:
					/* print everything else */
					if (fputc(*cp, fp) == EOF)
						errs++;
				}
			}
		} else if (sym->flags & SYM_CHOICE) {
			/*LINTED [alignment ok]*/
			Choice *chp = (Choice *)sym->value;
			if (chp->value != -1 && chp->value < chp->nchoice &&
			    fprintf(fp, "%s", chp->keywords[chp->value]) == EOF)
				errs++;
		}
		if (fputc('\n', fp) == EOF)
			errs++;
	}

	if (fclose(fp) == 0 && errs == 0) {
		if (rename(tmpfile, filename) == 0) {
			conf->saved = 1;
			return (0);
		}
	}

	return (-1);	/* errors on write, close, or rename */
}

/*
 * Set program's idea of current configuration.
 * NB:  the mail and host members point to dynamically-
 * allocated data, so we need to reallocate the old
 * data and fill it in with the new data.  Because
 * of this, only the program's copy of the configuration
 * structure should be considered writable.
 */
void
config_set(uconf)
	Config	*uconf;
{
	char	*mptr = config.admin.mail;
	char	*hptr = config.hosts;

	config = *uconf;

	config.admin.mail = (char *)
		xrealloc(mptr, strlen(uconf->admin.mail) + 1);
	(void) strcpy(config.admin.mail, uconf->admin.mail);

	config.hosts = (char *)
		xrealloc(hptr, strlen(uconf->hosts) + 1);
	(void) strcpy(config.hosts, uconf->hosts);
}

/*
 * Get program's idea of current configuration.
 * NB:  the mail and host members point to dynamically-
 * allocated data, so we need to reallocate the old
 * data and fill it in with the new data.  Because
 * of this, only the program's copy of the configuration
 * structure should be considered writable.
 */
void
config_get(uconf)
	Config	*uconf;
{
	*uconf = config;
}

char *
get_spooldir(void)
{
	return (config.spooldirs);
}

char *
get_rmntdir(void)
{
	return (config.rmntdir);
}

int
get_interactive(void)
{
	return (strcmp("yes",
	    config.askok.keywords[config.askok.value]) == 0 ? 1 : 0);
}

int
get_showcr(void)
{
	return (strcmp("yes",
	    config.showcr.keywords[config.showcr.value]) == 0 ? 1 : 0);
}

/*
 * Get/set name of current configuration file.
 * Set if argument is non-null, get if null.
 * Returns name of current filename.
 */
char *
config_file(filename)
	char	*filename;
{
	if (filename != (char *)0)
		(void) strcpy(configfile, filename);
	return (configfile);
}

/*
 * Get program's idea of current admin file values.
 * This routine returns a pointer to static storage
 * that will be overwritten on the next call and must
 * therefore be copied by the caller if the values
 * are to be saved.
 */
Admin_file *
admin_get(void)
{
	static Admin_file adminf;
	Admin *admin = &config.admin;

	(void) memset((void *)&adminf, 0, sizeof (adminf));

	adminf.mail = admin->mail;
	adminf.instance = admin->instance.keywords[admin->instance.value];
	adminf.partial = admin->partial.keywords[admin->partial.value];
	adminf.runlevel = admin->runlevel.keywords[admin->runlevel.value];
	adminf.idepend = admin->idepend.keywords[admin->idepend.value];
	adminf.rdepend = admin->rdepend.keywords[admin->rdepend.value];
	adminf.space = admin->space.keywords[admin->space.value];
	adminf.setuid = admin->setuid.keywords[admin->setuid.value];
	adminf.conflict = admin->conflict.keywords[admin->conflict.value];
	adminf.action = admin->action.keywords[admin->action.value];
	adminf.basedir = admin->basedir;

	return (&adminf);
}


static int
choice_value(string, array, n)
	char	*string;
	char	*array[];
	int	n;
{
	register int i;

	for (i = 0; i < n; i++)
		if (strcmp(string, array[i]) == 0)
			return (i);
	return (-1);
}
