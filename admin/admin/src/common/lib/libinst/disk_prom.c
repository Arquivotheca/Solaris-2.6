#ifndef lint
#pragma ident   "@(#)disk_prom.c 1.10 95/06/06 SMI"
#endif
/*
 * Copyright (c) 1992-1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
/*
 * This module contains routines which manipulate the disk structure or one
 * of its unit subcomponents as a complete entity
 */
#include "disk_lib.h"
#include "ibe_lib.h"
#include <sys/openpromio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

/* Constants and Globals */

#define	MAXPROPSIZE	128
#define	MAXVALSIZE	(4096 - MAXPROPSIZE - sizeof (u_int))
#define	PROPBUFSIZE	(MAXPROPSIZE + MAXVALSIZE + sizeof (u_int))

/* Data structures */

typedef union {
	char			buf[PROPBUFSIZE];
	struct openpromio	opp;
} OpenProm;

/*
 * openprom parameters required for evaluation
 */
typedef struct {
	char	device[MAXNAMELEN];
	char	targets[32];
} PromParam;

/* external prototypes */

extern int	devfs_path_to_drv(char *, char *);

#ifdef TEST
/*
 *		TEST SCENARIO
 *
 *	1. Make disk_prom.c with the the TEST macro defined.
 *	This will compile in main and some test functions
 *	that have been written.
 *
 *  2. Run "disk_prom -t".  This will run a set of dry
 *	run scenarios.  The first set exercise the sunmon
 *	style boot device specification (eg,  sd, xy,...).
 *	The next 2 sets run white box tests on the absolute_to_canon
 *	and _standard_to_canon functions to verify that they work.
 *	The results will go to stdout.
 *
 *	3. The following scenarios need to be tested.
 *	a.  On a system with a boot prom with version 2.5
 *	or greater run:
 *		- in the bootparam create a new alias, foo
 *		setenv boot-device foo
 *		run disk_prom, if the path specified is
 *		valid it should return the appropriate device
 *		specification otherwise it should return an
 *		error
 *
 *		- in the bootparam redefine the alias, disk
 *		to some other path
 *		setenv boot-device disk
 *		run disk_prom, if the path specified is
 *		valid it should return the appropriate device
 *		specification otherwise it should return an
 *		error
 *
 *		- for each disk alias (disk, disk0, ...)
 *		setenv boot-device <diskalias>
 *		Run disk_prom, if the device exists then
 *		it should return the appropriate device,
 *		otherwise it will return an error
 *
 *	b.  On a system with a boot prom with version 2.x
 *		but less than 2.5 run:
 *		- in the bootparam create a new alias, foo
 *		setenv boot-device foo
 *		run disk_prom
 *		This test should always fail since there is
 *		no way to pick up new aliases in pre-2.5
 *		OBP.  Additionally, aliases which are
 *		preexisting can be changed with devalias
 *		but the value used by boot is always the
 *		original hardcoded definition so it is not
 *		test changes made to these.
 *
 *		- for each disk alias (disk, disk0, ...)
 *		setenv boot-device <diskalias>
 *		Run disk_prom, if the device exists then
 *		it should return the appropriate device,
 *		otherwise it will return an error
 *
 *	c.  Run the following tests on both OBP 1.x boot
 *		prom and sunmon boot prom.
 *
 *		- for each value in the list below, change the
 *		value of boot-from to it and make sure that
 *		disk_prom returns the appropriate device or
 *		errors out:
 *		  sd() - this should return either c0t3d0 or
 *			c0t0d0 depending on the value of the
 *			variable sd-targets.  This variable
 *			gives a translation from the internal
 *			target number to the external device
 *			target specification.  It is a list of
 *			external device targets.  The internal
 *			target is used as an index into that list
 *			to derive the external target.  An example
 *			of this follows:
 *
 *			sd-targets 31204567
 *			boot-from sd(0,0,0)
 *			The resulting device number would be c0t3d0.
 *			 sd(,,) - same as above
 *			 sd(,1) - should return c0t1d0
 *			 sd(,2,2) - should return c0t2d2
 */


/*
 * The following are used to test the sunmon_to_canon
 * function.  The other functions have to be tested in a
 * live environment since they query the file system for
 * the disk devices connected to the system.
 */
static PromParam test_proms[] = {
		"()", "",
		"(,,)", "",
		"()", "31204567",
		"(,,)", "31204567",
		"sd()", "",
		"sd()", "31204567",
		"sd(,,)", "31204567",
		"sd(,10,)", "01234567",
		"sd(0)", "",
		"sd(,3)", "31204567",
		"sd(,8,)", "31204567",
		"sd(,18,2)", "31204567",
		"sd(0,20)", "",
		"xy()", "",
		"xd()", "",
		"ip()", "",
		"xy(,,)", "",
		"/iommu/sbus/esp/sd", "",
		"/iommu/sbus@f,e0001000/esp/sd@3,0", "",
		"/iommu/sbus/esp@f,800000/sd@0,0:a", "31204567",
		"/iommu/sbus%1/esp/sd", "",
		"/iommu/sbus/espdma/sd@0,0", "",
		"/iommu/sbus/espdma/cmdk@6,0:2,\\solaris.elf", "",
		"/iommu/sbus/espdma/cmdk@6,0:,\\solaris.elf", "",
		"/iommu/sbus/espdma@f,400000/esp@f,800000/sd@3,0", "",
		"/iommu/sbus/espdma/esp@f,800000/sd@3,0", "",
		"/iommu/sbus/espdma/esp@f,800000/sd@0,0:i", "",
		"/iommu/sbus/espdma/esp@f,800000/sd@3,1", "",
		"/iommu/sbus/espdma/esp/sd@0,0:h", "",
		"/iommu/sbus/espdma/esp/sd@3,0:c", "",
		"", ""
};
static int test_prom_offset = -1;

static char *test_abs[] = {
	"disk",
	"disk0",
	"disk1",
	"disk2",
	"disk3",
	"disk:a",
	"disk3:",
	"disk0:z",
	""
};
#endif

/* Public Function Prototypes */

/* Library Function Prototypes */

char *			_eeprom_default(void);

/* Local Function Prototypes */

static PromParam *	_prom_dflt_boot_device(void);
static char *		_generic_dir(char *);
static char *		_search_dir(char *);
static char *		_sunmon_to_canon(char *, char *);
static char *		_absolute_to_canon(char *);
static char *		_standard_to_canon(char *);
static char *		_obp_alias_lookup(char *);
static int		_obp_child(int, int);
static int		_obp_next(int, int);
static void		_obp_walk(int, int, int, char *);
static int		_obp_search_node(int, char *);
static char *		_obp_next_component(char **);
static int		_is_openprom(int fd);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

#ifdef TEST

/* Local Test Function Prototypes */

static void	test_set_prom_struct(int);
static void	print_test_prom_struct(int);

/*
 * test_set_prom_struct()
 *
 * Parameters:
 *	prom_offset	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
test_set_prom_struct(int prom_offset)
{
	test_prom_offset = prom_offset;
}

/*
 * print_test_prom_struct()
 *
 * Parameters:
 *	prom_offset	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
print_test_prom_struct(int prom_offset)
{
	(void) printf("\nBoot Device:  %s\n",
		test_proms[prom_offset].device);
	(void) printf("SD Target List:  %s\n",
		test_proms[prom_offset].targets);
}

main(int argc, char *argv[])
{
	int	test = 0;
	char * 	cp;
	int	i;
	char *  abs_out;

	while ((i = getopt(argc, argv, "tx:")) != -1) {
		switch (i) {
		    case 't':	test++;
				break;
		    case 'x':	(void) set_trace_level(atoi(optarg));
				break;
		    default:	(void) fprintf(stderr,
					"Usage: prom_test [-t] [-x <#>]\n");
				exit(1);
		}
	}

	if (test == 0) {
		cp = _eeprom_default();
		if (cp == NULL)
			(void) printf("Cannot determine device\n");
		else
			(void) printf("Device is %s\n", cp);
	} else {
		/* Test absolute_to_canon and sunmon_to_canon */
		for (i = 0; test_proms[i].device[0] != '\0'; i++) {
			test_set_prom_struct(i);
			print_test_prom_struct(i);
			cp = _eeprom_default();
			if (cp == NULL)
				(void) printf("Cannot determine device\n");
			else
				(void) printf("Device is %s\n", cp);
		}

		/* Test _standard_to_canon */
		(void) printf(
			"\nStandard Device to Canonical Translation Test\n\n");
		for (i = 0; test_abs[i][0] != '\0'; i++) {
			abs_out = _standard_to_canon(test_abs[i]);
			if (abs_out != NULL) {
				(void) printf("Input: %s  Output: %s\n",
						test_abs[i], abs_out);
			} else {
				(void) printf(
				"Input: %s  Output: Cannot determine device\n",
						test_abs[i]);
			}
		}
	}
}
#endif

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _eeprom_default()
 *	Query the eeprom for the default boot-device field. This routine
 *	always returns NULL if the disk list was built from a disk file,
 *	since the eeprom on the current system will not necessarily reflect
 *	the disks in the file. NULL is also returned on systems which do
 *	not support the eeprom command (e.g. Intel).
 * Parameters:
 *	none
 * Return:
 *	NULL	- no default boot disk could be determined
 *	char *	- drive name (e.g. "c0t0d0")
 * Status:
 *	private
 * Note:
 *	This routine does not recognize non-default controller
 *	boot devices and assumes the customer has not changed
 *	the default disk specifier aliases.
 *
 * Note:
 *  No attempt is made in this function to verify the existence
 *  of the device returned by the call to eeprom.  That is the
 *  responsibility of the calling function.
 */
char *
_eeprom_default(void)
{
	PromParam	*param;
	char		*obpd;
	char		*p;
	char		*cp;

	/* if this is dryrun then return NULL */
	if (_diskfile_load == 1)
		return (NULL);

	/* query the PROM for default boot device related parameters */
	if ((param = _prom_dflt_boot_device()) == NULL)
		return (NULL);

	/*
	 * loop through the default boot device list looking for a device
	 * which can be translated and matches a device name in the
	 * /devices directory tree
	 */
	for (p = strtok(param->device, " "); p != NULL; p = strtok(NULL, " ")) {
		/*
		 * look for a sunmon device specifier
		 */
		if (strchr(p, '(') != NULL) {
			if ((cp = _sunmon_to_canon(p, param->targets)) != NULL)
				return (cp);
			continue;
		}

		/*
		 * look for an absolute device path specifier
		 */
		if (*p == '/') {
			if ((cp = _absolute_to_canon(p)) != NULL)
				return (cp);
			continue;
		}

		/*
		 * look for an alias node traslation on SPARC EEPROM
		 * revisions >= 2.5 IEEE 1275 compliant proms
		 */
		if ((obpd = _obp_alias_lookup(p)) != NULL) {
			if ((cp = _absolute_to_canon(obpd)) != NULL)
				return (cp);
			continue;
		}

		/*
		 * look for a SPARC EEPROM device translations on prom
		 * revisions < 2.5 (default)
		 */
		if ((cp = _standard_to_canon(p)) != NULL)
			return (cp);
	}

	return (NULL);
}

/* ******************************************************************** */
/*			PRIVATE FUNCTIONS				*/
/* ******************************************************************** */
/*
 * _absolute_to_canon
 *	Convert an absolute device name from PROM path format to file
 *	system nomenclature.
 * Parameters:
 *	device	- device name
 * Return:
 *	NULL	- cannot map device to system
 *	char *	- pointer to local buffer with disk translation
 * Status:
 *	private
 */
static char *
_absolute_to_canon(char *device)
{
	static char	disk[MAXNAMELEN];
	struct stat	sbuf;
	struct dk_cinfo	dkc;
	char		buf[MAXNAMELEN];
	char		tmpbuf[MAXNAMELEN];
	char		drv_buf[MAXNAMELEN];
	char		odir[MAXNAMELEN];
	char		elink[MAXNAMELEN];
	char		element[MAXNAMELEN];
	struct dirent	*dp;
	DIR		*dirp;
	char		*dir_chp;
	char		*next;
	int		len;
	int		fd;
	int		n;
	char		*sp;
	char		*cp;
	char		*tmpsp;

	disk[0] = '\0';
	(void) strcpy(buf, device);

	if (getcwd(odir, MAXNAMELEN) == NULL)
		return (NULL);

	/*
	 * if there is a ':' character in between the first '/' and
	 * the last '/', we can't resolve the path name
	 */
	if ((cp = strchr(buf, '/')) != NULL &&
			(cp = strchr(cp, ':')) != NULL &&
			strchr(cp, '/') != NULL)
		return (NULL);

	/*
	 * strip off any trailing boot options specifiers (part of
	 * PPC IEEE 1275 extension)
	 */
	if ((cp = strrchr(buf, '/')) != NULL &&
			(cp = strchr(cp, ':')) != NULL &&
			(cp = strchr(cp, ',')) != NULL)
		*cp = '\0';

	/*
	 * if there is no ':' after the last '/' character, or if there is
	 * a ':' with no specifier, append the default segment specifier
	 * ":a"; if there is a ':' followed by a digit, this indicates
	 * a partition number (which does not map into the /devices name
	 * space), so strip the number and replace it with the letter
	 * that represents the partition index
	 */
	if ((cp = strrchr(buf, '/')) != NULL) {
		if ((cp = strchr(cp, ':')) == NULL)
			(void) strcat(buf, ":a");
		else if (*++cp == '\0')
			(void) strcat(buf, "a");
		else if (isdigit(*cp)) {
			n = atoi(cp);
			/* make sure to squash the digit */
			*cp = '\0';
			switch (n) {
			    case 0:	(void) strcat(buf, "q");
					break;
			    case 1:	(void) strcat(buf, "r");
					break;
			    case 2:	(void) strcat(buf, "s");
					break;
			    case 3:	(void) strcat(buf, "t");
					break;
			    case 4:	(void) strcat(buf, "u");
					break;
			    default:	(void) strcat(buf, "a");
					break;
			}
		}
	}

	if (get_trace_level() > 4) {
		write_status(SCR, LEVEL0|CONTINUE,
			"OBP device name cleaned up: %s", buf);
	}

	/*
	 * the device name should now be translated; go to the /devices
	 * directory and hunt for the device
	 */
	if (chdir("/devices") < 0)
		return (NULL);

	(void) strcpy(element, "../../devices/");

	/*
	 * store a copy of the original pathname in tmpbuf.  The contents
	 * of buf will end up being changed by _obp_next_component().
	 * tmpsp is a pointer to data within tmpbuf.  It moves each
	 * time we change directories to another level.
	 * We'll use tmpbuf when we need to ask for the driver name of
	 * a given /devices node.
	 */
	(void)strcpy(tmpbuf, buf);
	tmpsp  = tmpbuf;

	for (sp = buf, next = _obp_next_component(&sp);
			next != NULL; next = _obp_next_component(&sp)) {
		/*
		 * update tmpsp to point to the latest component in tmpbuf
		 */
		tmpsp++;
		tmpsp = strchr(tmpsp, '/');

		/*
		 * if a directory exists with this component name,
		 * change directory to this component and continue
		 * the loop
		 */
		if (_search_dir(next) != NULL) {
			(void) chdir(next);
			(void) strcat(element, next);
			(void) strcat(element, "/");
			continue;
		}
		/*
		 * On a system with generic device names, it is possible that
		 * the device node name in the prom is generic (e.g. disk)
		 * in the prom, but is specific (e.g. sd) in the /devices
		 * tree.  So if we searched a directory and found nothing,
		 * then ask for the driver name and use this to search
		 * the directory.
		 */
		*tmpsp = '\0';		/* NULL terminate tmpbuf */
		if (devfs_path_to_drv(tmpbuf, drv_buf) == 0) {
			if ((cp = strchr(next, '@')) != NULL)
				(void)strcat(drv_buf, cp);
			/*
			 * we got a driver name - make sure that it's
			 * not the same name we just looked for - if not
			 * go ahead and take look.
			 */
			if ((strcmp(drv_buf, next) != 0) &&
			    (_search_dir(drv_buf) != NULL)) {
				(void) chdir(drv_buf);
				(void) strcat(element, drv_buf);
				(void) strcat(element, "/");
				*tmpsp = '/';
				continue;
			}
		} else
			drv_buf[0] = '\0';

		*tmpsp = '/';	/* restore tmpbuf */
		/*
		 * If the component string includes an "@" sign,
		 * return an error because you can't match the name
		 * to an installed driver pathame (or you would have
		 * already in _search_dir())
		 *
		 * If there is exactly one entry that contains the
		 * component string followed by @*, that must be it,
		 * so change directory to it and continue
		 *
		 * Also check for the specific name (drv_buf) if it is not
		 * the same as 'next'
		 */
		if ((strchr(next, '@') == NULL) &&
		    (dir_chp = _generic_dir(next)) != NULL) {
			(void) chdir(dir_chp);
			(void) strcat(element, dir_chp);
			(void) strcat(element, "/");
			continue;
		} else if ((strcmp(next, drv_buf) != 0) &&
		    (strchr(drv_buf, '@') == NULL) &&
		    (dir_chp = _generic_dir(drv_buf)) != NULL) {
			(void) chdir(dir_chp);
			(void) strcat(element, dir_chp);
			(void) strcat(element, "/");
			continue;
		} else {
			(void) chdir(odir);
			return (NULL);
		}
	}

	/*
	 * if the final component is a block device, then find the
	 * canonical name for this device in /dev/dsk
	 *
	 * Note we also handle the condition mentioned above where
	 * the node name in the prom is generic but the node name
	 * in /devices appears as the specific (driver) name.
	 * If the prom name does not appear in /devices then we check
	 * for the specific form of the name (driver name)
	 */
	if (stat(sp, &sbuf) != 0) {
		if (devfs_path_to_drv(tmpbuf, drv_buf) == 0) {
			if (((cp = strchr(sp, '@')) != NULL) ||
			    ((cp = strchr(sp, ':')) != NULL))
				(void)strcat(drv_buf, cp);
			if (stat(drv_buf, &sbuf) != 0)
				sp = NULL;
			else
				sp = drv_buf;
		}
	}
	if (sp != NULL) {
		if (S_ISBLK(sbuf.st_mode)) {
			(void) strcat(element, sp);
			(void) chdir("/dev/dsk");
			if ((dirp = opendir(".")) != NULL) {
				while ((dp = readdir(dirp)) != NULL) {
					if (streq(dp->d_name, ".") ||
							streq(dp->d_name, ".."))
						continue;

					(void) sprintf(buf, "/dev/dsk/%s",
							dp->d_name);
					if ((len = readlink(buf, elink,
							MAXNAMELEN)) < -1) {
						(void) closedir(dirp);
						(void) chdir(odir);
						return (NULL);
					}

					elink[len] = '\0';
					if (strneq(element, elink))
						continue;

					/*
					 * the ioctl() used to check to see if
					 * the device is a cdrom must be run on
					 * the raw device
					 */
					(void) sprintf(buf,
						"/dev/rdsk/%s", dp->d_name);
					if ((fd = open(buf, O_RDONLY)) >= 0) {
						n = ioctl(fd, DKIOCINFO, &dkc);
						if (n == 0 && dkc.dki_ctype !=
								DKC_CDROM) {
							/*
							 * strip the slice/part
							 * specifier off device
							 * before returning and
							 * break
							 */
							(void) strcpy(disk,
								dp->d_name);

							if ((cp = strrchr(disk,
								    's'))
								    != NULL ||
								(cp = strrchr(
								    disk, 'p'))
								    != NULL)
							    *cp = '\0';
							break;
						}

						(void) close(fd);
					}
				}

				(void) closedir(dirp);
			}

		}
	}

	(void) chdir(odir);
	if (disk[0] == '\0')
		return (NULL);

	return (disk);
}

/*
 * _obp_next_component()
 *	Search for the next device component in the specified string
 * Parameters:
 *	pname	- address of pointer to device path string
 * Return:
 *	NULL	- no more components
 *	char *	- pointer to string containing next component
 * Status:
 *	private
 */
static char *
_obp_next_component(char **pname)
{
	char	*pp;
	char	*cp;

	if (pname == NULL || *pname == NULL)
		return (NULL);

	if (**pname == '/')
		++*pname;

	if (**pname == '\0')
		return (NULL);

	if ((cp = strchr(*pname, '/')) == NULL)
		return (NULL);

	*cp = '\0';
	pp = *pname;
	*pname = ++cp;
	return (pp);
}

/*
 * _search_dir()
 *	Search the current directory for a specified file name.
 * Parameters:
 *	component	- the name of file for search
 * Return:
 *	 char * - directory found; returning pointer to the name
 *	 NULL   - directory not found
 * Status:
 *	private
 */
static char *
_search_dir(char *component)
{
	struct dirent	*dp;
	DIR		*dirp;

	if ((dirp = opendir(".")) == NULL)
		return (NULL);

	while ((dp = readdir(dirp)) != NULL) {
		if (streq(dp->d_name, component))
			return (component);
	}

	(void) closedir(dirp);
	return (NULL);
}

/*
 * _generic_dir()
 *	Perform a generic compare on a directory component name
 *	and the current directory.  If the current directory is
 *	readable it is opened and its name is returned.  The name
 *	is compared to the component until the compare matches or
 *	the component has no more characters and the next character
 *	to match is '@' in the directory name.  If successful the
 *	full name of the current directory is returned.
 * Parameters:
 *	component	- component name to match
 * Return:
 *	char *	- success
 *	NULL	- failure
 * Status:
 *	private
 */
static char *
_generic_dir(char *component)
{
	static char	dir[MAXNAMELEN];
	struct dirent	*dp;
	DIR		*dirp;
	int		len;

	dir[0] = '\0';
	len = strlen(component);

	if ((dirp = opendir(".")) != NULL) {
		while ((dp = readdir(dirp)) != NULL) {
			if (strncmp(dp->d_name, component, len) == 0 &&
					dp->d_name[len] == '@') {
				(void) strcpy(dir, dp->d_name);
			}
		}

		(void) closedir(dirp);
	}

	if (dir[0] == '\0')
		return (NULL);

	return (dir);
}

/*
 * _prom_dflt_boot_device()
 *	Use /usr/sbin/eeprom to obtain the default boot device parameter
 *	from the PROM (any other data needed to determine the device).
 * Parameters:
 *	none
 * Return:
 *	NULL		- can't determine default device parameter
 *	prom_info *	- default boot prom data fields
 * Status:
 *	private
 */
static PromParam *
_prom_dflt_boot_device(void)
{
	static PromParam	param;
	char	buf[MAXNAMELEN];
	FILE	*pp;
	char	*sp;
	char	*cp;
	void	(*sig)(int);

#ifdef TEST
	if (test_prom_offset >= 0) {
		param = test_proms[test_prom_offset];
		return (&param);
	}
#endif

	/* initialize prom parameter structure */
	(void) strcpy(param.device, "disk");
	param.targets[0] = '\0';

	(void) sprintf(buf, "/usr/platform/%s/sbin/eeprom 2>&1",
			get_default_platform());

	if ((pp = (FILE *)popen(buf, "r")) == NULL)
		return (NULL);

	if (get_trace_level() > 4)
		write_status(SCR, LEVEL0, "EEPROM Parameters");

	sig = signal(SIGPIPE, SIG_IGN);
	while (!feof(pp) && fgets(buf, sizeof (buf), pp) != NULL) {
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';

		if ((cp = strstr(buf, "boot-device=")) != NULL ||
				(cp = strstr(buf, "bootdev=")) != NULL ||
				(cp = strstr(buf, "boot-from=")) != NULL) {
			if (get_trace_level() > 4)
				write_status(SCR, LEVEL1|LISTITEM,
					"Boot device:  %s", buf);
			/*
			 * "..." appears due to Bug 1123205 in which
			 * NVRAM < 2.12 cannot display parameter values
			 * > 27 characters in length; only if this does
			 * not appear do we accept the field
			 */
			if (strstr(buf, "...") == NULL) {
				sp = strchr(buf, '=');
				(void) strcpy(param.device, ++sp);
			}
		}

		/*
		 * extract the first token in the sd-targets list (if
		 * one is defined)
		 */
		if ((cp = strstr(buf, "sd-targets=")) != NULL) {
			if (get_trace_level() > 4)
				write_status(SCR, LEVEL1|LISTITEM,
					"SCSI targets:  %s", buf);
			cp = strchr(cp, '=');
			for (sp = ++cp; *sp != NULL && !isspace(*sp); ++sp);
			*sp = '\0';
			(void) strcpy(param.targets, cp);
		}

		/*
		 * look for the diag-switch? parameter (if one is
		 * defined) and make sure that, if it exists, it is
		 * set to "false"
		 */
		if ((cp = strstr(buf, "diag-switch?=")) != NULL) {
			if (get_trace_level() > 4)
				write_status(SCR, LEVEL1|LISTITEM,
					"Diagnostics switch: %s\n", buf);
			sp = strchr(cp, '=');
			++sp;
			if (strneq(sp, "false"))
				return (NULL);
		}
	}

	(void) pclose(pp);
	(void) signal(SIGPIPE, sig);
	return (&param);
}

/*
 * _sunmon_to_canon()
 *	Translate the string from sunmon-ese to canonical
 *	disk nomenclature (xx() -> cXtXdX). The syntax is:
 *		xx(A,B,C)
 *		A 	- controller number
 *		B	- target number
 *		C	- slice index (ie and le ignored)
 *	NOTE:
 *
 *	'B' may either be old style (hex representation of target
 *	x 8), or compatibility value (actual target). This value
 *	is in turn an index into the sd-targets property of the
 *	eeprom and should then be used to obtain the actual target
 *	number.
 *
 *	NOTE:
 *
 *	The only device specifiers (xx) supported are:
 *		id 	- IPI
 *		xy	- SMD
 *		xd	- SMDD
 *		sd	- SCSI
 *	All other specifiers will be assumed non-disk. The 'xy'
 *	and 'xd' specifiers also translate into a canonical form
 *	that does not have a 'tX' value.
 *
 *  NOTE:
 *	Legal sunmon strings include "" = "sd(0,0,0)"
 *				"()" = "sd(0,0,0)"
 *				"(,,)" = "sd(0,0,0)"
 *
 * Parameters:
 *	device	- device name
 *	targets	- value for sd_targets (NULL if none defined)
 * Return:
 *	NULL	- not a sunmon device name
 *	char *	- pointer to local string with canonical translation
 * Status:
 *	private
 */
static char *
_sunmon_to_canon(char *device, char *targets)
{
	static char	result[MAXNAMELEN];
	char		buf[MAXNAMELEN] = "";
	int 		data[3] = {0, 0, 0};
	int 		field;
	char		*cp;

	result[0] = '\0';

	/* if the device field is missing insert the default "sd" */
	if (device[0] == '(') {
		(void) sprintf(buf, "sd%s", device);
		(void) strcpy(device, buf);
	}

	/* check the device field for either a known disk type */
	if (strncmp(device, "id", 2) != 0 &&
			strncmp(device, "sd", 2) != 0 &&
			strncmp(device, "xd", 2) != 0 &&
			strncmp(device, "xy", 2) != 0)
		return (NULL);

	/*
	 * extract the controller, target (optional), and disk number
	 * from the parenthetic token
	 */
	if ((cp = strchr(device, '(')) == NULL)
		return (NULL);

	if (*++cp != ')') {
		for (field = 0; *cp != NULL && field < 3; field++) {
			if (*cp != ',')
				data[field] = atoi(cp);

			if ((cp = strchr(cp, ',')) == NULL)
				break;

			cp++;
		}
	}

	/*
	 * assemble the canonical form of the device; Xylogics controllers
	 * have no target number; old-style SCSI addresses must convert target
	 * to hex and divide by 8 to get the real target; compatibility SCSI
	 * addresses use target as an offset into sd_targets and use result as
	 * the target
	 */
	if (strncmp(device, "xy", 2) == 0 ||
			strncmp(device, "xd", 2) == 0) {
		(void) sprintf(result, "c%dd%d", data[0], data[2]);
	} else if (strncmp(device, "sd", 2) == 0) {
		if (data[1] >= 8) {
			(void) sprintf(buf, "%d", data[1]);
			data[1] = (int)strtol(buf, NULL, 16) / 8;
			(void) sprintf(result, "c%dt%dd%d",
					data[0], data[1], data[2]);
		} else {
			if (strneq(targets, "")) {
				buf[0] = targets[data[1]];
				buf[1] = '\0';
				data[1] = atoi(buf);
			}

			(void) sprintf(result, "c%dt%dd%d",
					data[0], data[1], data[2]);
		}
	} else
		(void) sprintf(result, "c%dt%dd%d",
				data[0], data[1], data[2]);

	return (result);
}

/*
 * _standard_to_canon()
 *	The 'device' string is an old OBP default alias. If the
 *	device is "disk" we will need to scan the "sbus-probe-list"
 *	to find the default target (should be the first in the list).
 *	Otherwise, use the default translations. Note that 'devalias'
 *	is not available from this program. Note also that optional
 *	slice specifiers are supported.
 *
 *	NOTE:	the boot device may translate into cXtXdX or cXdX
 *		depending on the disk type (e.g. SMD). The disk
 *		type cannot be determined since we can't get to the
 *		device alias translation for "disk*", so we'll have
 *		to try both.
 * Parameters:
 *	device	- device alias name
 * Return:
 *	NULL	- could not translate or not a disk device
 *	char *	- OBP name for device (pointer to local
 *		  static string)
 */
static char *
_standard_to_canon(char *device)
{
	static char	disk[16];
	static char	xydisk[16];
	char		odir[MAXNAMELEN];
	int		target;
	char		*cp;
	struct dirent	*dp;
	DIR		*dirp;
	char		slice[4];
	char    	full_disk[16];
	char		full_xydisk[16];

	disk[0] = '\0';
	target = -1;

	if (device == NULL || device[0] == '\0')
		return (NULL);

	if (strncmp(device, "disk0", 5) == 0)
		target = 3;
	else if (strncmp(device, "disk1", 5) == 0)
		target = 1;
	else if (strncmp(device, "disk2", 5) == 0)
		target = 2;
	else if (strncmp(device, "disk3", 5) == 0)
		target = 0;
	else if (strncmp(device, "disk", 4) == 0) {
		target = 3;
	}

	if (target >= 0) {
		(void) sprintf(disk, "c0t%dd0", target);
		(void) sprintf(xydisk, "c0t%d", target);

		/* look for optional slice specifier */
		cp = strchr(device, ':');

		if (cp != NULL) {
			++cp;
			switch (*cp) {
				case 'a': (void) strcpy(slice, "1"); break;
				case 'b': (void) strcpy(slice, "1"); break;
				case 'c': (void) strcpy(slice, "2"); break;
				case 'd': (void) strcpy(slice, "3"); break;
				case 'e': (void) strcpy(slice, "4"); break;
				case 'f': (void) strcpy(slice, "5"); break;
				case 'g': (void) strcpy(slice, "6"); break;
				case 'h': (void) strcpy(slice, "7"); break;
				default: (void) strcpy(slice, "0"); break;
			}
		} else
			(void) strcpy(slice, "0");

		/*
		 * Create the fully specified device to check
		 * for existence of the device but return the
		 * device specifier minus the slice
		 */
		(void) sprintf(full_xydisk, "%ss%s", xydisk, slice);
		(void) sprintf(full_disk, "%ss%s", disk, slice);

		if (getcwd(odir, MAXNAMELEN) == NULL ||
				chdir("/dev/dsk") < 0)
			return (NULL);

		if ((dirp = opendir(".")) == NULL)
			return (NULL);

		while ((dp = readdir(dirp)) != (struct dirent *)0) {
			if (streq(dp->d_name, ".") || streq(dp->d_name, ".."))
				continue;

			if (streq(dp->d_name, full_disk)) {
				(void) closedir(dirp);
				(void) chdir(odir);
				return (disk);
			} else if (streq(dp->d_name, full_xydisk)) {
				(void) closedir(dirp);
				(void) chdir(odir);
				return (xydisk);
			}
		}

		(void) closedir(dirp);
		(void) chdir(odir);
	}

	/* non-standard disk device */
	return (NULL);
}

/*
 * _obp_alias_lookup()
 *	/dev/openprom alias lookup.
 * Parameters:
 *	device	- device alias name
 * Return:
 *	NULL	- alias lookup failed
 *	char *	- pointer to value found in alias block
 * Status:
 *	private
 */
static char *
_obp_alias_lookup(char *device)
{
	static char	alias[MAXNAMELEN];
	char		options[16] = "";
	char		*cp;
	char		*sp;
	int		fd;

	/*
	 * make sure the system supports an openprom device
	 */
	if ((fd = open("/dev/openprom", O_RDONLY)) < 0 ||
			_is_openprom(fd) == 0)
		return (NULL);

	/*
	 * check the alias name for an explicit options specifier (':')
	 */
	(void) strcpy(alias, device);
	if ((cp = strchr(alias, ':')) != NULL) {
		*cp = '\0';
		(void) strcpy(options, ++cp);
	}

	/*
	 * walk the alias tree looking for translation for the alias
	 */
	_obp_walk(fd, _obp_next(fd, 0), 0, alias);
	(void) close(fd);

	if (alias[0] != '/')
		return (NULL);

	/*
	 * if the alias had a options specifier, then override the
	 * translated options field with the one found on the alias
	 */
	if ((cp = strrchr(alias, '/')) != NULL) {
		if ((sp = strchr(cp, ':')) == NULL) {
			(void) strcat(alias, ":a");
		} else {
			if (options[0] != '\0') {
				*++sp = '\0';
				(void) strcat(cp, options);
			}
		}
	}

	return (alias);
}


/*
 * _obp_walk()
 *	Recursive function which walks the openprom device tree
 *	looking for an alias match.
 * Parameters:
 *	fd	- open file descriptor for /dev/openprom
 *	id	- pointer to current node
 *	level	-
 *	node	- pointer to node containing alias to match; also
 *		  used to retrieve the value of the alias (if found)
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_obp_walk(int fd, int id, int level, char *node)
{
	int	current;

	if (_obp_search_node(fd, node) != 0) {
		if ((current = _obp_child(fd, id)) != 0)
			_obp_walk(fd, current, level + 1, node);

		if ((current = _obp_next(fd, id)) != 0)
			_obp_walk(fd, current, level, node);
	}
}

/*
 * _obp_child()
 *	Find the pointer to the child node.
 * Parameters:
 *	fd	- open file descriptor for /dev/openprom
 *	id	- pointer to current node
 * Return:
 *	int	- pointer to child node in the tree
 * Status:
 *	private
 */
static int
_obp_child(int fd, int id)
{
	OpenProm	  pbuf;
	struct openpromio *opp = &(pbuf.opp);
	/*LINTED [alignment ok]*/
	int		  *ip = (int *)(opp->oprom_array);

	(void) memset(pbuf.buf, 0, PROPBUFSIZE);
	opp->oprom_size = MAXVALSIZE;
	*ip = id;

	if (ioctl(fd, OPROMCHILD, opp) < 0)
		return (0);

	/*LINTED [alignment ok]*/
	return (*(int *)opp->oprom_array);
}

/*
 * _obp_next()
 *	Find the next node in the tree.
 * Parameters:
 *	fd	- open file descriptor for /dev/openprom
 *	id	- pointer to current node
 * Return:
 *	int	- pointer to next node in the tree
 * Status:
 *	private
 */
static int
_obp_next(int fd, int id)
{
	OpenProm	  pbuf;
	struct openpromio *opp = &(pbuf.opp);
	/*LINTED [alignment ok]*/
	int		  *ip = (int *)(opp->oprom_array);

	(void) memset(pbuf.buf, 0, PROPBUFSIZE);
	opp->oprom_size = MAXVALSIZE;
	*ip = id;

	if (ioctl(fd, OPROMNEXT, opp) < 0)
		return (0);

	/*LINTED [alignment ok]*/
	return (*(int *)opp->oprom_array);
}

/*
 * _obp_search_node()
 *	Search a particular node in the openprom tree for an alias
 *	which matches the specified node, and if found, return the
 *	value associated with the alias in the node parameter.
 * Parameters:
 *	fd	- open file descriptor for /dev/openprom
 *	node	- pointer to node containing alias to match; also
 *		  used to retrieve the value of the alias (if found)
 * Return:
 *	1	- alias found and value returned
 *	0	- alias not found
 * Status:
 *	private
 */
static int
_obp_search_node(int fd, char *node)
{
	OpenProm	  pbuf;
	OpenProm	  Pbuf;
	struct openpromio *opp = &(pbuf.opp);
	struct openpromio *ppp = &(Pbuf.opp);
	int		  anode = 0;
	char		  str[MAXPATHLEN] = "";

	(void) memset(pbuf.buf, 0, PROPBUFSIZE);

	for (;;) {
		opp->oprom_size = MAXPROPSIZE;
		if (ioctl(fd, OPROMNXTPROP, opp) < 0 ||
				opp->oprom_size == 0)
			break;

		if (anode == 0 && streq(opp->oprom_array, "name")) {
			ppp->oprom_size = MAXVALSIZE;
			(void) strcpy(ppp->oprom_array, "name");

			if (ioctl(fd, OPROMGETPROP, ppp) < 0)
				break;

			if (streq(ppp->oprom_array, "aliases"))
				anode++;
		}

		if (streq(opp->oprom_array, node)) {
			ppp->oprom_size = MAXVALSIZE;
			(void) strcpy(ppp->oprom_array, node);
			if (ioctl(fd, OPROMGETPROP, ppp) < 0)
				break;

			(void) strcpy(str, ppp->oprom_array);
		}
	}

	if (anode > 0) {
		(void) strcpy(node, str);
		return (0);
	}

	return (1);
}

/*
 * _is_openprom()
 *	Boolean test to see if a device is an openprom device. The
 *	test is based on whether the OPROMGETCONS ioctl() works,
 *	and if the OPROMCONS_OPENPROM bits are set.
 * Parameters:
 *	fd	- open file descriptor for /dev/openprom device
 * Return:
 *	1	- openprom device verified
 *	0	- device is not an openprom
 * Status:
 *	private
 */
static int
_is_openprom(int fd)
{
	OpenProm	  pbuf;
	struct openpromio *opp = &(pbuf.opp);
	u_char		  mask;

	opp->oprom_size = MAXVALSIZE;

	if (ioctl(fd, OPROMGETCONS, opp) == 0) {
		mask = (u_char)opp->oprom_array[0];
		if ((mask & OPROMCONS_OPENPROM) == OPROMCONS_OPENPROM)
			return (1);
	}

	return (0);
}
