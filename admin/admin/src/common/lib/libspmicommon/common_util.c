#ifndef lint
#pragma ident "@(#)common_util.c 1.11 96/07/17 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	common_util.c
 * Group:	libspmicommon
 * Description:
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "spmicommon_lib.h"
#include "common_strings.h"

/* constants */

#define	N_PROG_LOCKS		10
#define	_SC_PHYS_MB_DEFAULT	0x1000000	/* sixteen MB */

/* globals */

static char	blkdevdir[] = "/dev/dsk/";
static char	rawdevdir[] = "/dev/rdsk/";
static int	_library_trace_level = 0;

/* private prototypes */

static char *	_find_abs_path(char *);
static int	_map_node_to_devlink(char *, char *);
static int	_map_old_device_to_new(char *, char *);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	axtoi
 * Description:	Convert a hexidecimal, octal, or decimal string to integer
 *		form.
 * Scope:	public
 * Parameters:	str	- string to convert
 * Return:	# >= 0	- decimal conversion of a hexidecimal string
 */
int
axtoi(char *str)
{
	int		retval = 0;

	if (str && *str) {
		if (strlen(str) > 2U &&
		    str[0] == '0' && strchr("Xx", str[1])) {
			str += 2;
			if (sscanf(str, "%x", (u_int *) & retval) != 1)
				return (0);
		} else if (strlen(str) > 1U && str[0] == '0') {
			str++;
			if (sscanf(str, "%o", (u_int *) & retval) != 1)
				return (0);
		} else {
			if (sscanf(str, "%d", &retval) != 1)
				return (0);
		}

		return (retval);
	}
	return (0);
}

/*
 * Function:	_copy_file
 * Description:	Copy a file from one location in the file system to another
 *		location using the 'cp' command.
 * Scope:	public
 * Parameters:	dst	- copy destination file
 *		src	- copy source file
 * Return:	NOERR	- copy succeeded
 *		ERROR	- copy failed
 */
int
_copy_file(char *dst, char *src)
{
	char		cmd[MAXPATHLEN];

	/* validate parameters */
	if (dst == NULL || src == NULL)
		return (ERROR);

	if (!GetSimulation(SIM_ANY)) {
		(void) sprintf(cmd, "/usr/bin/cp %s %s >/dev/null 2>&1",
		    src, dst);
		if (system(cmd) != 0) {
			write_notice(ERRMSG, MSG_COPY_FAILED, src, dst);
			return (ERROR);
		}
	}
	return (NOERR);
}

/*
 * Function:	_create_dir
 * Description:	Create all the directories in the path, setting the modes,
 *		owners and groups along the way.  This is a recursive function.
 * Scope:	public
 * Parameters:	path	- directory pathname
 * Returns:	NOERR	- directory created successfully
 *		errno or FAIL (-1) on error
 */
int
_create_dir(char *path)
{
	int		status;
	char		*slash;

	/* validate parameters */
	if (*path == '\0' || access(path, X_OK) == 0)
		return (NOERR);

	/* return printing a tracer if running an execution simulation */
	if (GetSimulation(SIM_ANY)) {
		write_status(SCR, LEVEL1 | LISTITEM, CREATING_MNTPNT, path);
		return (NOERR);
	}
	slash = strrchr(path, '/');
	if (slash != NULL) {
		*slash = '\0';
		status = _create_dir(path);
		*slash = '/';
		if (status != NOERR) {
			write_notice(ERRMSG, CREATE_MNTPNT_FAILED, path);
			return (status);
		}
	}
	if (mkdir(path, 0775) == NOERR)
		return (NOERR);

	if (errno != EEXIST)
		return (errno);

	return (NOERR);
}

/*
 * Function:	_filesys_fiodio
 * Description:	Enable/disable/query asynchronous metadata writes for the
 *		specified file system. The caller's effective UID must be 0.
 * Scope:	public
 * Parameters:	name	- mount point path name
 *		set	- action to take on the above pathname
 *			  0 = disable asynchronous metadata writes
 *			  1 = enable asynchronous metadata writes
 *			  2 = query the status of the asynchronous metadata
 *				writes flag
 * Return:	none
 */
void
_filesys_fiodio(char *name, int set)
{
	int		mypid = getpid();
	int		fd;
	char		path[128];

	/* validate parameters */
	if (name == NULL || set > 2)
		return;

	/* return if running an execution simulation */
	if (GetSimulation(SIM_ANY))
		return;

	if (geteuid() != 0)
		return;

	/*
	 * create a temporary file in the designated file system and get/set
	 * the asynchronous write option using that file
	 */
	if (strcmp(name, "/") == 0)
		(void) sprintf(path, "%s%s%d", name, "....", mypid);
	else
		(void) sprintf(path, "%s/%s%d", name, "....", mypid);

	if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644)) >= 0) {
		if (set != 2 && ioctl(fd, _FIOSDIO, &set) < 0)
			write_notice(ERRMSG, SYNC_WRITE_SET_FAILED, path);

		if (set == 2 && ioctl(fd, _FIOGDIO, &set) < 0)
			write_notice(ERRMSG, SYNC_WRITE_SET_FAILED, path);

		(void) close(fd);

		/*
		 * remove the temporary file before you return
		 */
		(void) unlink(path);
	}
}

/*
 * Function:	get_trace_level
 * Description:	Get the current level of library tracing. This is used for
 *		internal tracing of application and library activity
 *		independent of the current debug status.
 * Scope:	public
 * Parameters:	none
 * Return:	# >= 0	- current trace level
 */
int
get_trace_level(void)
{
	return (_library_trace_level);
}

/*
 * Function:	_lock_prog
 * Description:	Lock the program specified by program into memory.  This
 *		function can be called up to 10 times to lock N_PROG_LOCKS
 *		(i.e. 10) programs into memory.
 * Scope:	public
 * Parameters:	program	- pathname of program to be locked
 * Return:	 0	- lock successful
 *		-1	- lock failed
 */
int
_lock_prog(char *program)
{
	static caddr_t	*pa = NULL;
	static int	locked_programs = 0;
	struct stat	sb;
	int		program_fd = -1;
	int		i;

	/* validate parameters */
	if (program == NULL)
		return (-1);

	/* if running an execution simulation, return */
	if (GetSimulation(SIM_ANY))
		return (0);

	if (pa == NULL) {
		if ((pa = (char **) malloc(N_PROG_LOCKS * sizeof (caddr_t)))
		    == NULL) {
			return (-1);
		}
		for (i = 0; i < N_PROG_LOCKS; i++)
			pa[i] = NULL;
	}
	if (locked_programs < N_PROG_LOCKS) {
		/* Open file descriptor to file program. */
		if ((program_fd = open(program, O_RDONLY, 0)) == -1)
			return (-1);

		if (fstat(program_fd, &sb) < 0)
			return (-1);

		/* Map it in and lock the pages into memory */
		if ((pa[locked_programs] = mmap((caddr_t) 0, sb.st_size,
			    PROT_READ, MAP_SHARED, program_fd,
			    (off_t) 0)) == (caddr_t) - 1)
			return (-1);

		(void) close(program_fd);
		if (mlock(pa[locked_programs], sb.st_size) == -1)
			return (-1);

		locked_programs++;

	}
	return (0);
}

/*
 * Function:	make_block_device
 * Description:	Assemble a complete block device name (e.g. /dev/dsk/c0t0d0s0)
 * Scope:	public
 * Parameters:	disk	- pointer to disk name (e.g. c0t0d0)
 *		slice	- slice number
 * Return:	char *	- pointer to local buffer containing assembled name
 */
char *
make_block_device(char *disk, int slice)
{
	static char	buf[32];

	buf[0] = '\0';
	(void) sprintf(buf, "/dev/dsk/%ss%d", disk, slice);
	return (buf);
}

/*
 * Function:	make_char_device
 * Description:	Assemble a complete character device name (e.g.
 *		/dev/rdsk/c0t0d0s0)
 * Scope:	public
 * Parameters:	disk	- pointer to disk name (e.g. c0t0d0)
 *		slice	- slice number
 * Return:	char *	- pointer to local buffer containing assembled name
 */
char *
make_char_device(char *disk, int slice)
{
	static char	buf[32];

	buf[0] = '\0';
	(void) sprintf(buf, "/dev/rdsk/%ss%d", disk, slice);
	return (buf);
}

/*
 * Function:	make_slice_name
 * Description:	Assemble a complete slice device name (e.g. c0t0d0s0)
 * Scope:	public
 * Parameters:	disk	- pointer to disk name (e.g. c0t0d0)
 *		slice	- slice number
 * Return:	char *	- pointer to local buffer containing assembled name
 */
char *
make_slice_name(char *disk, int slice)
{
	static char	buf[16];

	buf[0] = '\0';
	if ((int) strlen(disk) < 15)
		(void) sprintf(buf, "%ss%d", disk, slice);
	return (buf);
}

/*
 * Function:	make_device_name
 * Description:	Assemble a complete device name (e.g. c0t0d0s0 or c0t0d0p3).
 *		The device type identifier ('s' or 'p') is based on the isa
 *		type.
 * Scope:	public
 * Parameters:	disk	- pointer to disk name (e.g. c0t0d0)
 *		device	- device index number
 * Return:	char *	- pointer to local buffer containing assembled name
 */
char *
make_device_name(char *disk, int device)
{
	static char	buf[16];

	buf[0] = '\0';
	if ((int)  strlen(disk) < 15)
		(void) sprintf(buf, "%s%c%d",
			disk, IsIsa("sparc") ? 's' : 'p', device);
	return (buf);
}

/*
 * Function:	_map_to_effective_dev
 * Description:	Used during installation and upgrade to retrieve the local
 *		'/dev/<r>dsk' name which points to the same physical device
 *		(i.e. /devices/...) as 'dev' does in the <bdir> client device
 *		namespace.
 * Scope:	public
 * Parameters:	dev	- [RO] device name (e.g. /dev/rdsk/c0t0d0s3)
 *		edevbuf	- [WO] pathname of device on booted OS (e.g.
 *			      /devices/....). The space allocated for this
 *			      buffer is allocated by calling routine.
 * Return:	0	- search completed; edevbuf has whatever value was found
 *		1	- failure; error while scanning links in local /dev dir
 *		2	- failure; cannot read the link /<bdir>/<dev>
 * Note:	Check to see if '/dev' is a character or block device. Then
 *		translate /<bdir>/<dev> to its symbolic link value, and scan the
 *		local /dev/<r>dsk directory for a device that has the same
 *		symbolic link destination (e.g. /device/...). If not found,
 *		leave 'edevbuf' NULL, otherwise, copy the name of the local
 *		device (e.g. /dev/<r>dsk/...) into 'edevbuf'.
 */
int
_map_to_effective_dev(char *dev, char *edevbuf)
{
	static char	deviceslnk[] = "../devices/";
	static char	devlnk[] = "../dev/";
	char		linkbuf[MAXNAMELEN];
	char		mapped_name[MAXNAMELEN];
	char		ldev[MAXNAMELEN];
	char		*abs_path;
	int		len;

	edevbuf[0] = '\0';

	(void) sprintf(ldev, "%s%s", get_rootdir(), dev);
	if ((len = readlink(ldev, linkbuf, MAXNAMELEN)) == -1)
		return (2);
	linkbuf[len] = '\0';

	/*
	 * We now have the link (this could be to dev/ or ../devices. We now
	 * must make sure that we correctly map the BSD style devices.
	 */
	if (strncmp(dev, blkdevdir, strlen(blkdevdir)) != 0 &&
	    strncmp(dev, rawdevdir, strlen(rawdevdir)) != 0) {
		/* this must be a BSD style device */
		if (strncmp(linkbuf, deviceslnk, strlen(deviceslnk)) == 0) {

			/*
			 * A link to ../devices/, to be compatible with SVR4
			 * devices this link must be ../../devices
			 */
			(void) sprintf(linkbuf, "../%s", linkbuf);
		} else {
			if (strncmp(linkbuf, devlnk, strlen(devlnk)) == 0) {
				char		*tmpStr;

				/*
				 * This is a link to ../dev, we can just
				 * strip off the ../dev and use the logic
				 * below to get the linkbuf.
				 */
				if ((tmpStr = strstr(linkbuf, devlnk)) ==
				    NULL)
					return (1);
				/* Step past the ../dev/ */
				tmpStr = tmpStr + strlen(devlnk);
				/* copy the new path into linkbuf */
				(void) strcpy(linkbuf, tmpStr);
			}

			/*
			 * Here we have a link to dev/, we now need to map
			 * this to /a/dev/ and then read that link.
			 */
			(void) sprintf(ldev,
			    "%s/dev/%s", get_rootdir(), linkbuf);
			if ((len = readlink(ldev, linkbuf, MAXNAMELEN)) == -1)
				return (2);
			linkbuf[len] = '\0';
		}
	}

	/*
	 * Find the point in the linkbuf where the absolute pathname of the
	 * node begins (that is, skip over the "..[/..]*" part) and save the
	 * length of the leading relative part of the pathname.
	 */

	abs_path = _find_abs_path(linkbuf);
	len = abs_path - linkbuf;

	/*
	 * Now that we have the devices path to the device we need to search
	 * for that entry on the boot environment. (This is the effective
	 * device.
	 */

	if (access(abs_path, F_OK) == 0) {
		if (_map_node_to_devlink(linkbuf, edevbuf) == 0)
			return (0);
	}

	/*
	 * The device node in linkbuf either doesn't exist in the current
	 * effective root file system, or it exists but no link to it exists
	 * in /dev/dsk or /dev/rdsk.  The device may have a new name in the
	 * new release.	 Attempt to map the old name to a new name.
	 */

	/* copy the leading relative part of the link to the mapped name buf */
	(void) strncpy(mapped_name, linkbuf, len);

	/*
	 * Now fill the rest of the mapped_name buffer with the mapping of
	 * the absolute part of the name (if possible).
	 */
	if (_map_old_device_to_new(abs_path, mapped_name + len) == 0)
		return (_map_node_to_devlink(mapped_name, edevbuf));
	else
		return (1);

}

/*
 * Function:	reset_system_state
 * Description: Routine used to reset the state of the system. Resetting
 *		includes:
 *
 *		(1) halt all running newfs and fsck processes which
 *		    may still be active
 *		(2) unregister all currently active swap devices
 *		(3) unmount all filesystems registered in /etc/mnttab
 *		    to be mounted under /a (NOTE: this must be done after
 *		    killing fscks or a "busy" may be recorded)
 * Scope:	public
 * Parameters:	none
 * Return:	 0	- reset successful
 *		-1	- reset failed
 */
int
reset_system_state(void)
{
	/* if running an any simulation, return immediately */
	if (GetSimulation(SIM_ANY))
		return (0);

	if (system("ps -e | egrep newfs >/dev/null 2>&1") == 0) {
		(void) system("kill -9 `ps -e | egrep newfs | \
			awk '{print $1}'` \
			`ps -e | egrep mkfs | awk '{print $1}'` \
			`ps -e | egrep fsirand | awk '{print $1}'` \
			> /dev/null 2>&1");
	}
	if (system("ps -e | egrep fsck >/dev/null 2>&1") == 0) {
		(void) system("kill -9 `ps -e | egrep fsck | awk '{print $1}'` \
			> /dev/null 2>&1");
	}
	while (system("swap -l 2>&1 | egrep configured > \
			/dev/null 2>&1") != 0) {
		if (system("swap -d `swap -l | egrep -v swapfile | \
				head -1 | awk '{print $1}'` \
				>/dev/null 2>&1") != 0)
			return (-1);
	}

	if (DirUmountAll("/a") < 0)
		return (-1);

	return (0);
}

/*
 * Function:	slice_access
 * Description:	Keep a record of all slices which have been accessed
 *		explicitly. Return code indicates if the slice is already
 *		in the list of specified slices. The list of known slices
 *		is not kept in sorting order since the average chain will
 *		not have more than three or four entries.
 * Scope:	public
 * Parameters:	device	- string specifying device name (e.g. c0t0d0s2)
 *		alloc	- specifies if the entry should be added to the
 *			  list of access slices
 *			  (Valid values:  0 - do not add   1 - add)
 * Return:	0	- the specified slice has not already been used
 *		1	- the specified slice has already been used
 */
int
slice_access(char *device, int alloc)
{
	typedef struct access_entry {
		char		*device;
		short		slices[16];
		struct access_entry *next;
	}		Access_t;

	static Access_t	*Access_list = NULL;
	int		retval = 0;
	Access_t	*ap;
	char		buf[32];
	char		*cp;
	int		si;

	(void) strcpy(buf, device);
	cp = strrchr(buf, 's');
	*cp++ = '\0';
	si = atoi(cp);

	WALK_LIST(ap, Access_list) {
		if (streq(buf, ap->device)) {
			if (ap->slices[si] == 1) {
				retval = 1;
				break;
			}
		}
	}

	if (alloc == 1) {
		if (ap == NULL) {

			/*
			 * allocate and initialize new structure and add it
			 * to the list of known slices
			 */
			if ((ap = (Access_t *) xcalloc(
				    sizeof (Access_t))) == NULL)
				return (0);

			ap->device = (char *) strdup(buf);
			ap->next = Access_list;
			Access_list = ap;
		}
		ap->slices[si] = 1;
	}
	return (retval);
}

/*
 * Function:	set_trace_level
 * Description: Set the current level of library tracing. This is used for
 *		internal tracing of application and library activity
 *		independent of the current debug status.
 * Scope:	public
 * Parameters:	set	 - trace level:
 *			   0	 - off
 *			   # > 0 - trace level (the higher the more verbose)
 * Return:	# >= 0	- old trace level
 *		-1	- set failed
 */
int
set_trace_level(int set)
{
	int		old;

	if (set < 0)
		return (-1);

	old = _library_trace_level;
	_library_trace_level = set;
	return (old);
}

/*
 * Function:	simplify_disk_name
 * Description: Convert a conventional disk name into the internal canonical
 *		form. Remove the trailing index reference. The return status
 *		reflects whether or not the 'src' name is valid.
 *
 *				src			 dst
 *			---------------------------------------
 *			[/dev/rdsk/]c0t0d0s0	->	c0t0d0
 *			[/dev/rdsk/]c0t0d0p0	->	c0t0d0
 *			[/dev/rdsk/]c0d0s0	->	c0d0
 *			[/dev/rdsk/]c0d0p0	->	c0d0
 *
 * Scope:	public
 * Parameters:	dst	- used to retrieve cannonical form of drive name
 *			  ("" if not valid)
 *		src	- name of drive to be processed (see table above)
 * Return:	 0	- valid disk name
 *		-1	- invalid disk name
 */
int
simplify_disk_name(char *dst, char *src)
{
	char		name[128];
	char		*cp;

	*dst = '\0';

	/* look for the controller specifier */
	if ((cp = strrchr(src, 'c')) == NULL)
		return (-1);
	cp = strcpy(name, cp);
	cp++;
	skip_digits(cp);		/* must be at least 1 digit */

	/* the target specifier is optional */
	if (*cp == 't') {
		++cp;
		skip_digits(cp);	/* must be at least 1 digit */
	}
	/* look for the drive specifier */
	if (*cp++ != 'd')
		return (-1);
	skip_digits(cp);		/* must be at least 1 digit */

	/* look for the slice/partition specifier */
	if (*cp && *cp != 'p' && *cp != 's')
		return (-1);

	*cp = '\0';
	(void) strcpy(dst, name);
	return (0);
}

/*
 * Function:	_system_fs_ancestor
 * Description: Determine if a directory name is a child of the system
 *		directory namespace which is used during install. System
 *		directories include:
 *
 *			/dev	/usr	/etc	  /sbin
 *			/bin	/var	/opt	  /devices
 *			/lib	/export	/a	  /tmp
 *			/kernel /.cache /platform
 *
 * Scope:	public
 * Parameters:	fs	- non-NULL file system name to be analyzed
 * Return:	0	- 'fs' is not a system directory child
 *		1	- 'fs' is a system directory child
 */
int
_system_fs_ancestor(char *fs)
{
	int		i;
	int		n;
	static char	*_sysfs[] = {
		"/a",
		"/.cache",
		"/bin",
		"/dev",
		"/devices",
		"/etc",
		"/export",
		"/kernel",
		"/lib",
		"/opt",
		"/sbin",
		"/tmp",
		"/usr",
		"/var",
		"/platform",
	NULL};

	if (strcmp(fs, "/") == 0)
		return (1);

	for (i = 0; _sysfs[i]; i++) {
		if (strcmp(fs, _sysfs[i]) == 0)
			return (1);

		n = strlen(_sysfs[i]);
		if (strncmp(fs, _sysfs[i], n) == 0 &&
		    fs[n] == '/' &&
		    strncmp(fs, "/export/home", 12) != 0)
			return (1);
	}

	return (0);
}


static SimType	_Simulation[] = {0, 0, 0, 0, 0};

/*
 * Function:	SetSimulation
 * Description:	Set a specified simulation flag to 0 or 1 indicating
 *		that the simulation is not, or is supported,
 *		respectively. SIM_ANY is not a supported simulation
 *		flag type.
 * Scope:	public
 * Parameters:	type	- [RO]
 *			  specific simulation type defined by SimType
 *			  enumerated type (except SIM_ANY).
 *		value	- [RO] (0|1)
 *			  indicating if the simulation is "off" or "on";
 *			  any value which is not 0 is converted to 1
 *			  before being set
 * Return:	 0	- value of simluation flag before being set
 *		 1	- value of simluation flag before being set
 *		-1	- parameter value error
 */
int
SetSimulation(SimType type, int value)
{
	int		old;

	if (type == SIM_ANY || value != 0 && value != 1)
		return (-1);

	old = _Simulation[(int) type];
	_Simulation[(int) type] = value;
	return (old);
}

/*
 * Function:	GetSimulation
 * Description:	Get the current value of the specified simluation flag.
 * Scope:	public
 * Parameters:	type	- [RO]
 *			  specific simulation type defined by SimType
 *			  enumerated type.
 * Return:	0	- current value of specified simulation flag
 *		1	- current value of specified simulation flag
 */
int
GetSimulation(SimType type)
{
	int		value = 0;

	if (type != SIM_ANY) {
		value = _Simulation[(int) type];
	} else {
		if (GetSimulation(SIM_SYSDISK) ||
		    GetSimulation(SIM_MEDIA) ||
		    GetSimulation(SIM_SYSSOFT) ||
		    GetSimulation(SIM_EXECUTE))
			value = 1;
	}

	return (value);
}

/*
 * Function:	CatFile
 * Description:	Open a file and "cat" it using the write_message()
 *		interface.
 * Scope:	public
 * Parameters:	filename	- The file to read
 *		dest		- Write to logfile or display
 *		type		- Type of message
 *		format		- Message format (indent level)
 * Return:	none
 */
void
CatFile(char *filename, u_char dest, u_int type, u_int format)
{
	FILE *fd;
	char buf[MAXPATHLEN];
	int cont = 0;
	int slen;

	if ((fd = fopen(filename, "r")) == NULL)
		return;

	while (!feof(fd)) {
		if (fgets(buf, MAXPATHLEN-1, fd) != NULL) {
			/*
			 * Arbitrarily remove last character assuming it's a
			 * newline
			 */
			slen = strlen(buf);
			if (slen > 0)
				buf[slen-1] = '\0';
			if (!cont++)
				write_message(dest, type, format, buf);
			else
				write_message(dest, type, format|CONTINUE, buf);
		}
	}
	fclose(fd);
	return;
}

/*
 * Function:	ParseBuffer
 * Description:	Lexically parse a string into individually separate
 *		fields. The fields are kept in dynamically allocated
 *		data elemets which are local to this function, and
 *		which are reallocated upon each call (you must copy
 *		out needed data before subsequently calling this
 *		function).
 * Scope:	public
 * Parameters:	buf	- [RO, *RO]
 *			  buffer containing string to be parsed
 *		array	- [RO, *RO, **RW] (optional)
 *			  address of character pointer array which
 *			  will be set to point to locally stored static
 *			  buffer strings; if NULL, then no elements are
 *			  returned in the argument list
 * Return:	# >= 0	- number of elements in the returned array
 *			  which were parsed
 */
int
ParseBuffer(char *buf, char ***array)
{
	static char	*_Elements[30];
	char		tbuf[MAXNAMELEN] = "";
	char		*cp;
	int		i;

	if (buf == NULL)
		return (0);

	(void) strcpy(tbuf, buf);

	for (i = 0; i < 30; i++) {
		free(_Elements[i]);
		_Elements[i] = NULL;
	}

	for (i = 0, cp = strtok(tbuf, " \t");
	    i < 30 && cp != NULL;
	    i++, cp = strtok(NULL, " \t"))
		_Elements[i] = xstrdup(cp);

	if (array != NULL)
		*array = _Elements;

	return (i);
}

/* ---------------------- private functions ----------------------- */
/*
 * Function:	_find_abs_path
 * Description: Find the absolute part of a relative pathname (that is,
 *		find the part that starts after the "..[/..]*".	 If no "."
 *		or ".." pathname segments exist at the beginning of the path,
 *		just return the beginning of the input string.	Don't modify
 *		the input string. Just return a pointer to the character in the
 *		input string where the absolute part begins.
 * Scope:	private
 * Parameters:	path	- pointer to the pathname whose absolute portion is
 *			  to be found.
 * Return:	pointer to the absolute part of the pathname.
 */
static char    *
_find_abs_path(char *path)
{
	enum parse_state {
		AFTER_SLASH,
		AFTER_FIRST_DOT,
		AFTER_SECOND_DOT
	}		state;
	char		*cp;
	char		*last;

	for (cp = path, last = path, state = AFTER_SLASH; *cp; cp++) {
		switch (*cp) {
		case '.':
			if (state == AFTER_SLASH)
				state = AFTER_FIRST_DOT;
			else if (state == AFTER_FIRST_DOT)
				state = AFTER_SECOND_DOT;
			else if (state == AFTER_SECOND_DOT)
				return (last);
			break;

		case '/':
			if (state == AFTER_SLASH)
				last = cp;
			else if (state == AFTER_FIRST_DOT ||
			    state == AFTER_SECOND_DOT) {
				last = cp;
				state = AFTER_SLASH;
			}
			break;

		default:
			return (last);
		}
	}
	/* NOTREACHED */
}

/*
 * Function:	_map_node_to_devlink
 * Description:	Search the /dev/dsk or /dev/rdsk directory for a device link
 *		to the device node identified by linkbuf.  Copy the absolute
 *		pathname of that device link to the buffer pointed to by
 *		edevbuf.
 * Scope:	private
 * Parameters:	linkbuf	- [RO]
 *			  device node path
 *		edevbuf	- [WO]
 *			  pathname which is a symlink to the device node
 *			  identified by linkbuf.
 * Return:	0	- search completed; edevbuf has whatever value was found
 *		1	- failure; error while scanning links in local /dev dir
 *		2	- failure; cannot read the link /<bdir>/<dev>
 */
static int
_map_node_to_devlink(char *linkbuf, char *edevbuf)
{
	struct dirent	*dp;
	char		elink[MAXNAMELEN];
	DIR		*dirp;
	char		*dirname;
	int		len;

	if (strstr(linkbuf, ",raw") != NULL)
		/* This is a character device, search /dev/rdsk */
		dirname = rawdevdir;
	else
		/* This is a block device, search /dev/dsk */
		dirname = blkdevdir;

	if ((dirp = opendir(dirname)) == NULL)
		return (0);

	while ((dp = readdir(dirp)) != (struct dirent *) 0) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		(void) sprintf(edevbuf, "%s%s", dirname, dp->d_name);

		if ((len = readlink(edevbuf, elink, MAXNAMELEN)) == -1) {
			edevbuf[0] = '\0';
			(void) closedir(dirp);
			return (1);
		}
		elink[len] = '\0';

		if (strcmp(linkbuf, elink) == 0) {
			(void) closedir(dirp);
			return (0);
		}
	}
	edevbuf[0] = '\0';
	(void) closedir(dirp);
	return (1);
}

/*
 * Function:	_map_old_device_to_new
 * Description:	Uses the /tmp/physdevmap.nawk.* files (if any) to map the
 *		input device name to the new name for the same device.	If
 *		the name can be mapped, copy the mapped name into newdev.
 *		Otherwise, just copy olddev to newdev.
 * Scope:	private
 * Parameters:	iolddev	- [RO]
 *			  device name to be mapped (may have leading "../"*)
 *		newdev	- [WO]
 *			  new, equivalent name for same device.
 * Return:	 none
 * Note:	If this is the first call to this routine, use the
 *		/tmp/physdevmap.nawk.* files to build a mapping array.
 *		Once the mapping array is built, use it to map olddev
 *		to the new device name.
 */
static int
_map_old_device_to_new(char *olddev, char *newdev)
{
	static int	nawk_script_known_not_to_exist = 0;
	static char	nawkfile[] = "physdevmap.nawk.";
	static char	sh_env_value[] = "SHELL=/sbin/sh";
	char		cmd[MAXPATHLEN];
	DIR		*dirp;
	FILE		*pipe_fp;
	int		nawk_script_found;
	struct dirent	*dp;
	char		*envp;
	char		*shell_save = NULL;

	if (nawk_script_known_not_to_exist)
		return (1);

	if ((dirp = opendir("/tmp")) == NULL) {
		nawk_script_known_not_to_exist = 1;
		return (1);
	}
	nawk_script_found = 0;

	/*
	 * Temporarily set the value of the SHELL environment variable to
	 * "/sbin/sh" to ensure that the Bourne shell will interpret the
	 * commands passed to popen.  THen set it back to whatever it was
	 * before after doing all the popens.
	 */

	if ((envp = getenv("SHELL")) != NULL) {
		shell_save = malloc(strlen(envp) + 6 + 1);
		(void) strcpy(shell_save, "SHELL=");
		(void) strcat(shell_save, envp);
		(void) putenv(sh_env_value);
	}
	while ((dp = readdir(dirp)) != (struct dirent *) 0) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		if (strncmp(nawkfile, dp->d_name, strlen(nawkfile)) != 0)
			continue;

		nawk_script_found = 1;

		/*
		 * This is a nawk script for mapping old device names to new.
		 * Now use it to try to map olddev to a new name.
		 */

		(void) sprintf(cmd, "/usr/bin/echo \"%s\" | "
		    "/usr/bin/nawk -f /tmp/%s -v 'rootdir=\"%s\"' "
		    "2>/dev/null", olddev, dp->d_name,
		    streq(get_rootdir(), "") ? "/" : get_rootdir());

		if ((pipe_fp = (FILE *) popen(cmd, "r")) == NULL)
			continue;

		if (fgets(newdev, MAXPATHLEN, pipe_fp) != NULL) {
			/* remove the trailing new-line */
			newdev[strlen(newdev) - 1] = '\0';
			(void) pclose(pipe_fp);
			(void) closedir(dirp);
			if (shell_save != NULL)
				(void) putenv(shell_save);
			return (0);
		}
		(void) pclose(pipe_fp);
	}
	(void) closedir(dirp);

	if (shell_save != NULL)
		(void) putenv(shell_save);

	if (nawk_script_found == 0)
		nawk_script_known_not_to_exist = 1;
	return (1);
}

/*
 * *********************************************************************
 * FUNCTION NAME: CMNWiteBuffer
 *
 * DESCRIPTION:
 *  This function takes in a buffer of bytes, the number of bytes to
 *  write and the open file descriptor to write to.  This function then
 *  calls write() to write the buffer to the desired file descriptor.
 *  This function will not return until either an error occurs or ALL
 *  of the bytes have been written.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  int				     Upon success zero is returned.
 *				     Upon failure non-zero is returned.
 *
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  int				     The file descriptor to write the
 *				     buffer to.
 *  const void *		     A pointer to the buffer to be
 *				     written.
 *  size_t			     The number of bytes to be written.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

int
CMNWiteBuffer(int FileDes,
    const void *Buffer,
    size_t BytesToWrite)
{
	char		*CharPtr;
	ssize_t		BytesRemaining;
	ssize_t		BytesWritten;

	BytesRemaining = BytesToWrite;

	CharPtr = (char *) Buffer;
	while (BytesRemaining) {
		BytesWritten = write(FileDes,
		    &CharPtr[BytesToWrite - BytesRemaining],
		    BytesRemaining);

		if (BytesWritten == -1) {
			return (-1);
		}
		BytesRemaining = BytesRemaining - BytesWritten;
	}

	return (0);
}

/*
 * *********************************************************************
 * FUNCTION NAME: CMNModifyFileDesFlag
 *
 * DESCRIPTION:
 *   This routine allows the caller to set/clear options on the
 *   specified file descriptor.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  int				     Upon success zero is returned.
 *				     Upon failure non-zero is returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  int				     0 = Clear the flags on the descriptor
 *				     1 = Set the flags on the descriptor
 *  int				     The file descriptor to be modified.
 *  int				     The flags to be set on the file
 *				     file descriptor.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

int
CMNModifyFileDesFlag(int Set,
    int FileDes,
    int FlagsToSet)
{
	int		Flag;

	if ((Flag = fcntl(FileDes, F_GETFL, 0)) < 0) {
		return (-1);
	}
	if (Set) {
		Flag |= FlagsToSet;
	} else {
		Flag &= ~FlagsToSet;
	}

	if (fcntl(FileDes, F_SETFL, Flag) < 0) {
		return (-1);
	}
	return (0);
}

/*
 * *********************************************************************
 * Note that this implementation is taken stright out of Richard
 * Steven's Advanced Programming in the UNIX Environment book.
 * *********************************************************************
 */

/*
extern char *ptsname(int);
*/

/*
 * *********************************************************************
 * Function Name: CMNPTYMasterOpen				       *
 *								       *
 * Description:							       *
 *  This function opens up the master end of Pseudo Terminal.	       *
 *								       *
 * Return:							       *
 *  Type			     Description		       *
 *  int				     File Descriptor for pseudo	       *
 *				     terminal on Success.  Negative 1  *
 *				     on Failure.		       *
 *								       *
 * Parameters:							       *
 *  Type			     Description		       *
 *  char *			     The name if the pseudo terminal   *
 *				     that is opened.		       *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

int
CMNPTYMasterOpen(char *PTSName)
{
	char		*ptr;
	int		FDMaster;

	(void) strcpy(PTSName, "/dev/ptmx");

	/*
	 * Open up the pseudo device
	 */

	if ((FDMaster = open(PTSName, O_RDWR)) < 0) {
		return (-1);
	}

	/*
	 * Grant access to the slave
	 */

	if (grantpt(FDMaster) < 0) {
		(void) close(FDMaster);
		return (-1);
	}

	/*
	 * Clear the Slave's lock.
	 */

	if (unlockpt(FDMaster) < 0) {
		(void) close(FDMaster);
		return (-1);
	}

	/*
	 * Get the slave's name
	 */

	if ((ptr = ptsname(FDMaster)) == NULL) {
		(void) close(FDMaster);
		return (-1);
	}

	/*
	 * Copy the name into the supplied character string.
	 */

	(void) strcpy(PTSName, ptr);
	return (FDMaster);
}

/*
 * *********************************************************************
 * Function Name: CMNPTYSlaveOpen				       *
 *								       *
 * Description:							       *
 *  This function takes in the master end of a pseudo terminal and     *
 *  opens the slave end.  This function is called within the fork'd    *
 *  child to establish communications with the parent via the pseudo   *
 *  terminal.							       *
 *								       *
 * Return:							       *
 *  Type			     Description		       *
 *  int				     File Descriptor for pseudo	       *
 *				     terminal on Success.  Negative 1  *
 *				     on Failure.		       *
 *								       *
 * Parameters:							       *
 *  Type			     Description		       *
 *  int				     The file descriptor for the       *
 *				     master side of the pseudo	       *
 *				     terminal.			       *
 *  char *			     The name of the pseudo terminal.  *
 *				     This is returned by	       *
 *				     CMNPTYMasterOpen().	       *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

int
CMNPTYSlaveOpen(int FDMaster, char *PTSName)
{
	int		FDSlave;

	/*
	 * Open up the supplied pseudo device
	 */

	if ((FDSlave = open(PTSName, O_RDWR)) < 0) {
		(void) close(FDMaster);
		return (-1);
	}

	/*
	 * Set the pseudo terminal emulation mode.
	 */

	if (ioctl(FDSlave, I_PUSH, "ptem") < 0) {
		(void) close(FDMaster);
		(void) close(FDSlave);
		return (-1);
	}

	/*
	 * Set the terminal line discipline module
	 */

	if (ioctl(FDSlave, I_PUSH, "ldterm") < 0) {
		(void) close(FDMaster);
		(void) close(FDSlave);
		return (-1);
	}

	/*
	 * Set the terminal into compatiability mode
	 */

	if (ioctl(FDSlave, I_PUSH, "ttcompat") < 0) {
		(void) close(FDMaster);
		(void) close(FDSlave);
		return (-1);
	}

	/*
	 * We're out of here
	 */

	return (FDSlave);
}

/*
 * *********************************************************************
 * Function Name: CMNPTYFork					       *
 *								       *
 * Description:							       *
 *  This function forks a child process and sets up the I/O to the     *
 *  process via a pseudo terminal.				       *
 *								       *
 * Return:							       *
 *  Type			     Description		       *
 *  int				     > 0 : PID of child - Success      *
 *				     = 0 : Child process	       *
 *				     < 0 : Failure		       *
 *								       *
 * Parameters:							       *
 *  Type			     Description		       *
 *  int *			     The file descriptor for the       *
 *				     master side of the pseudo	       *
 *				     terminal.			       *
 *  char *			     The name of the pseudo terminal.  *
 *  const struct termios *	     Allows the calling application to *
 *				     set the terminal characteristics  *
 *				     for the pseudo terminal.  If a    *
 *				     NULL is provided, the defaults    *
 *				     will be used.		       *
 *  const struct winsize *	     Allows the calling appliation to  *
 *				     set the window size of the pseudo *
 *				     terminal.	If a NULL is provided, *
 *				     the dafaults are used.	       *
 *								       *
 * Designer/Programmer: Craig Vosburgh/RMTC (719)528-3647	       *
 * *********************************************************************
 */

pid_t
CMNPTYFork(int *FDMaster,
    char *PTSName,
    const struct termios * SlaveTermios,
    const struct winsize * SlaveWinSize)
{
	int		LocalFDMaster;
	int		LocalFDSlave;
	pid_t		LocalPID;
	char		LocalPTSName[20];

	/*
	 * Open up a new pseudo terminal
	 */

	if ((LocalFDMaster = CMNPTYMasterOpen(LocalPTSName)) < 0) {
		return (-1);
	}

	/*
	 * Copy the name of the pseudo terminal into the local string.
	 */

	if (PTSName != NULL) {
		(void) strcpy(PTSName, LocalPTSName);
	}

	/*
	 * fork a child
	 */

	if ((LocalPID = fork()) < 0) {
		return (-1);
	}

	/*
	 * If this is the Child
	 */

	else if (LocalPID == 0) {

		/*
		 * Get a new session id
		 */

		if (setsid() < 0) {
			return (-1);
		}

		/*
		 * Open up the slave side of the pseudo terminal
		 */

		if ((LocalFDSlave = CMNPTYSlaveOpen(LocalFDMaster,
							LocalPTSName)) < 0) {
			return (-1);
		}

		/*
		 * Were done with the master side of the pseudo terminal so
		 * close it.
		 */

		(void) close(LocalFDMaster);

		/*
		 * If the calling application provided a set of terminal
		 * attributes.
		 */

		if (SlaveTermios != NULL) {
			if (tcsetattr(LocalFDSlave,
					TCSANOW,
					SlaveTermios) < 0) {
				return (-1);
			}
		}

		/*
		 * If the calling application provided a terminal window size
		 */

		if (SlaveWinSize != NULL) {
			if (ioctl(LocalFDSlave, TIOCSWINSZ, SlaveWinSize) < 0) {
				return (-1);
			}
		}

		/*
		 * Set the slave side of the pseudo terminal as STDIN.
		 */

		if (dup2(LocalFDSlave, STDIN_FILENO) != STDIN_FILENO) {
			return (-1);
		}

		/*
		 * Set the slave side of the pseudo terminal as STDOUT.
		 */

		if (dup2(LocalFDSlave, STDOUT_FILENO) != STDOUT_FILENO) {
			return (-1);
		}

		/*
		 * Set the slave side of the pseudo terminal as STDERR.
		 */

		if (dup2(LocalFDSlave, STDERR_FILENO) != STDERR_FILENO) {
			return (-1);
		}

		/*
		 * If the slave file descriptor is above STDERR then go ahead
		 * and close it since it has been already dup'd.
		 */

		if (LocalFDSlave > STDERR_FILENO) {
			(void) close(LocalFDSlave);
		}

		/*
		 * Return a zero to signify that this is the child side.
		 */

		return (0);
	}

	/*
	 * Otherwise this is the Parent
	 */

	else {
		*FDMaster = LocalFDMaster;
		return (LocalPID);
	}
}

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	SystemGetMemsize
 * Description:	Return system memory size in 512 byte blocks. If sysconf()
 *		does not have a value for the number of pages on the system,
 *		use the localled defined constant _SC_PHYS_MB_DEFAULT (16 MB).
 * Scope:	internal
 * Parameters:	none
 * Return:	# > 0	- memory size in 512 byte blocks
 */
int
SystemGetMemsize(void)
{
	long		pages,
			size;
	char		*tmem;
	u_int		byte_calc;

	/* first check if an overrride memsize is set in memory */

	if ((tmem = getenv("SYS_MEMSIZE")) != NULL)
		byte_calc = atoi(tmem) * 0x100000;
	else {
		size = PAGESIZE; 			/* bytes per page  */
		pages = sysconf(_SC_PHYS_PAGES);	/* number of pages */

		/* required test i386 S1093 doesn't support _SC_PHYS_PAGES */
		if (pages <= 0)
			byte_calc = _SC_PHYS_MB_DEFAULT;
		else
			byte_calc = pages * size;
	}

	return (bytes_to_sectors(byte_calc));
}
