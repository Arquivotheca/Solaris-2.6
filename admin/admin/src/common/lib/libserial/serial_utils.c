#ifndef lint
#ident "@(#)serial_utils.c	1.15	96/03/26 SMI"
#endif

/*
 *  Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/systeminfo.h>	/* for sysinfo/SI_MACHINE */
#include <sys/types.h>
#include <sys/stat.h>
#include "serial_impl.h"


static
int
build_eeprom_path(char *path)
{

	FILE		*pp;
	struct stat	buf;
	int		status;
	char		platform[MEDBUF];


	if (stat("/usr/sbin/eeprom", &buf) == 0) {
		strcpy(path, "/usr/sbin/eeprom");
	} else {
		/* figure that it's a kbi system */
		if ((pp = popen("/bin/uname -i", "r")) == NULL) {
			return (SERIAL_FAILURE);
		}

		fgets(platform, sizeof (platform), pp);

		status = pclose(pp);

		if (status == 0) {
			/* strip off the newline */
			platform[strlen(platform) - 1] = '\0';
			sprintf(path, "/usr/platform/%s/sbin/eeprom", platform);
			if (stat(path, &buf) != 0) {
				return (SERIAL_FAILURE);
			}
		} else {
			return (SERIAL_FAILURE);
		}
	}

	return (SERIAL_SUCCESS);
}


/*
 * We pass in port and true/false.
 * TRUE if we want to ignore cd
 * False if we don't
 */
int
do_eeprom(const char *port, const char *truefalse)
{

	char		arch[MEDBUF];
	char		eeprom_path[MEDBUF];
	char		cmd[MEDBUF];


	if (sysinfo(SI_MACHINE, arch, MEDBUF-1) <= 0)
		return (SERIAL_SUCCESS);	/* ought to always succeed */
	else if ((strcmp("sun4", arch) == 0) || (strcmp("i86pc", arch) == 0)
		 || (strcmp("prep", arch) == 0))
		return (SERIAL_SUCCESS);	/* no EEPROM on this model */

	if ((strcmp(port, "a") != 0) && (strcmp(port, "b") != 0)) {
		return (SERIAL_SUCCESS);
	}

	if (build_eeprom_path(eeprom_path) == SERIAL_FAILURE) {
		return (SERIAL_FAILURE);
	}
	sprintf(cmd, "%s tty%s-ignore-cd=%s\n", eeprom_path, port, truefalse);

	if (system(cmd) != 0) {
		return (SERIAL_FAILURE);
	}
	return (SERIAL_SUCCESS);
}


/*
 * Call a UNIX command via popen() and return SERIAL_SUCCESS or SERIAL_FAILURE.
 * If success, return the command's output.
 */
int
do_popen_cmd(char *cmd, char *retbuf)
{
	FILE		*cmd_results;
	char		buf[MEDBUF];

	memset(retbuf, '\0', LGBUF);	/* clear the buffer */

	close(1);
	close(2);
	/* limit security leaks */
	putenv("PATH=/usr/sbin:/usr/bin");
	putenv("IFS= \t");

	sprintf(buf, "set -f ; %s", cmd);
	if ((cmd_results = popen(buf, "r")) == NULL) {
		return (SERIAL_FAILURE);
	} else {
		if (fgets(retbuf, LGBUF, cmd_results) == NULL) {
			pclose(cmd_results);
			return (SERIAL_FAILURE);
		} else {
			pclose(cmd_results);
			return (SERIAL_SUCCESS);
		}
	}
}

/*
 * The following code was added to solve bug 1152777 which allowed a user
 * to change the eeprom "ignore-cd" for the console (ttya for example)
 * causing the system to not boot after the next 'reset' of the system.
 *
 * The specific problem is that the eeprom was changed for the console.
 * A partial solution would be to not change the eeprom if it is the console.
 * Although solving bug 1152777, it does not solve the general problem
 * which is that serial port manager gives the impression that it can
 * change the parameters for the console when in fact it can't.  Console
 * parameters are changed in /etc/inittab.
 *
 * A problem in determining what port the console is attached to is that
 * different architectures behave differently.  The only way to determine
 * if the console is on a tty port is to look at the eeprom.  Unfortunately
 * x86 machines don't have an eeprom so the only way to get the needed
 * information is to make a kernel dive.  This is the reason the function
 * "is_console" is different for sparc and x86 machines.
 */

#if defined(i386) || defined(__ppc)
/*
 * ---------------------------------- x86 ----------------------------------
 * Since x86 machines don't have the eeprom command, we have to make a
 * kernel dive.  The kernel variable 'console' contains the informatino
 * we needed:
 *	console = 0	=> means the console is the tube
 *	console = 1	=> means the console is ttya
 *	console = 2	=> means the console is ttyb
 */
#include <kvm.h>
#include <nlist.h>
#include <sys/systeminfo.h>	/* for sysinfo/SI_MACHINE */

/*
 * 'port' is "/dev/term/<port>".
 */

int
is_console(const char *port) {
	kvm_t	*kvm;
	struct nlist nl[2];
	int	console;

	memset(nl, 0, sizeof (nl));
	kvm = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kvm == NULL) {
		return (FALSE);
	}
	nl[0].n_name = "console";
	kvm_nlist(kvm, nl);
	kvm_read(kvm, nl[0].n_value, (char *)&console, sizeof (console));
	kvm_close(kvm);
	if (((*port - 'a') + 1) == console)
		return (TRUE);
	else return (FALSE);
}

/*
 * --------------------------------- SPARC ---------------------------------
 * On sparc machines we use the eeprom command to determine where the console
 * is located.  Since there are different types of sparc machines with
 * different revisions of the eeprom, we have to look for two different
 * eeprom variables.  On old sun4 machines the variable is 'console' while
 * on newer machines the variable is 'output-device'.
 */

#else /* sparc */
int
is_console(const char *port) {
    char	*cmd = "/usr/sbin/prtconf -vp | /usr/bin/grep stdout-path";
    char	 ret[LGBUF];
    int		 len;

    do_popen_cmd(cmd, ret);
    len = strlen(ret);
    if (len != 0) {
	if( ret[(len -3)] == *port) return (TRUE);
	else return (FALSE);
    } else {
	/*
	 * Probably an older sparc which doesn't know what a 'stdout-path'
	 * is.  In this case, fall back to looking in the eeprom to
	 * see if we can figure things out.
	 */
	 return (is_console_old_sparc(port));
    }
}
char *console_strings[] = {
	"console",
	"output-device",
	NULL
};

int
is_console_old_sparc(const char *port) {
	FILE		*eeprom;
	char		**cons = console_strings;
	char		eeprom_path[MEDBUF];
	char		cmd[MAXPATHLEN];
	char		buffer[BUFSIZ];
	char		*c;
	char		*tmp;


	if (build_eeprom_path(eeprom_path) == SERIAL_FAILURE) {
		return (FALSE);
	}

	/*
	 * Loop through all possible eeprom symbols.
	 */
	while (*cons) {
		sprintf(cmd, "%s %s", eeprom_path, *cons);
		eeprom = popen(cmd, "r");
		if (eeprom) {
			if (fgets(buffer, BUFSIZ, eeprom)) {
				c = strchr(buffer, '=');
				/*
				 * If the eeprom symbol was found, look
				 * to see if the 4th character is the
				 * same as the port.  The reason for the
				 * 4th character is it is assumed that
				 * eeprom will return something of the
				 * form "tty?" where "?" will either be
				 * "a" or "b"
				 */
				if (c) {
					c++; /* skip over '=' */
					if (strncmp(c, "tty", 3) == 0)
						c += 3;
					tmp = strchr(c,'\n');
					if (*tmp) *tmp = 0;
					if (strcmp(c, port) == 0)
						return (TRUE);
				}
			}
		}
		pclose(eeprom);
		cons++;
	}
	/*
	 * If we none of the eeprom symbols contain 'tty?' where '?' is
	 * the same as the port we are looking for, then we return FALSE.
	 */
	return (FALSE);
}
#endif /* i386 */
