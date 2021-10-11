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
#ident	"@(#)file.c 1.6 93/04/09"
#endif

#include "defs.h"
#include "ui.h"
#include <errno.h>
#include <unistd.h>

#define	TEXTVIEW_DEFAULT	"textedit -read_only"
#define	PSVIEW_DEFAULT		"imagetool"
#define	DEMO_DEFAULT		"screendemo"

int
runfile(file)
	File	*file;
{
	char	*prog, *cp;
	char	cwd[MAXPATHLEN];
	char	demodir[MAXPATHLEN+1];
	char	cmd[BUFSIZ];
	char	progbuf[MAXPATHLEN];
	int	status = SUCCESS;

	switch (file->f_type) {
	case ASCII:
		prog = getenv("TEXTEDIT");
		if (prog == (char *)0) {
			(void) sprintf(progbuf, "%s/bin/%s",
				openwinhome, TEXTVIEW_DEFAULT);
			prog = progbuf;
		}
		(void) sprintf(cmd, "%s %s %s",
			prog, file->f_args ? file->f_args : "",
			file->f_path);
		msg(Basescreen, 1, gettext(
			"Launching text viewer (%s)...\n"), prog);
		if (fork() == 0)
			exit(system(cmd));
		msg(Basescreen, 1, "");
		break;
	case POSTSCRIPT:
		prog = getenv("POSTVIEW");
		if (prog == (char *)0) {
			(void) sprintf(progbuf, "%s/bin/%s",
				openwinhome, PSVIEW_DEFAULT);
			prog = progbuf;
		}
		(void) sprintf(cmd, "%s %s %s",
			prog, file->f_args ? file->f_args : "",
			file->f_path);
		msg(Basescreen, 1, gettext(
			"Launching Postscript page viewer (%s)...\n"), prog);
		if (fork() == 0)
			exit(system(cmd));
		msg(Basescreen, 1, "");
		break;
	case ROLLING:
		prog = getenv("SCREENDEMO");
		if (prog == (char *)0)
			prog = DEMO_DEFAULT;
		(void) sprintf(cmd, "%s %s %s",
			prog, file->f_args ? file->f_args : "",
			file->f_path);
		(void) getcwd(cwd, sizeof (cwd));
		(void) strncpy(demodir, file->f_path,
			sizeof (demodir) - 1);
		demodir[sizeof (demodir) - 1] = '\0';
		cp = strrchr(demodir, '/');
		if (cp != (char *)0)	/* cp CAN'T be null... */
			*cp = '\0';
		if (chdir(demodir) < 0)
			msg(Termscreen, 1,
			    gettext("Warning:  cannot cd to %s:  %s"),
				demodir, strerror(errno));
		msg(Basescreen, 1, gettext(
			"Launching rolling demo executive (%s)...\n"), prog);
		if (fork() == 0)
			exit(system(cmd));
		msg(Basescreen, 1, "");
		if (chdir(cwd) < 0)
			msg(Termscreen, 1,
			    gettext("Warning:  cannot cd back to %s:  %s"),
				cwd, strerror(errno));
		break;
	case EXECUTABLE:
		(void) sprintf(cmd, "%s %s",
			file->f_path, file->f_args ? file->f_args : "");
		(void) getcwd(cwd, sizeof (cwd));
		(void) strncpy(demodir, file->f_path,
			sizeof (demodir) - 1);
		demodir[sizeof (demodir) - 1] = '\0';
		cp = strrchr(demodir, '/');
		if (cp != (char *)0)	/* cp CAN'T be null... */
			*cp = '\0';
		if (chdir(demodir) < 0)
			msg(Termscreen, 1,
			    gettext("Warning:  cannot cd to %s:  %s"),
				demodir, strerror(errno));
		status = tty_exec_cmd(cmd, (Exit_func)0, (caddr_t)0);
		if (chdir(cwd) < 0)
			msg(Termscreen, 1,
			    gettext("Warning:  cannot cd back to %s:  %s"),
				cwd, strerror(errno));
		break;
	default:
		asktoproceed(Loadscreen, gettext(
		    "Sorry, I don't know what to do\nwith the file `%s'.\n"),
			file->f_path);
		status = ERR_INVALIDTYPE;
		break;
	}
	return (status);
}
