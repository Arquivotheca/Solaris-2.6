#ifndef lint
#pragma ident "@(#)store_initdisk.c 1.27 96/09/27 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_initdisk.c
 * Group:	libspmistore
 * Description:	Routines to initialize the disk list using live
 *		data from the existing system, or disk simulation data.
 */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/fs/ufs_fs.h>
#include "spmistore_lib.h"
#include "spmicommon_lib.h"

#ifndef	MODULE_TEST
/* public prototypes */
int		DiskobjInitList(char *);
Disk_t *	DiskobjCreate(char *);

/* private prototypes */
static char *	DiskGetNext(void);
static char *	DiskSeekNext(void);
static Disk_t * DiskobjInitialize(char *);
static int	DiskobjInitControllerLive(Disk_t *);
static int	DiskobjInitControllerSim(Disk_t *);

static void	FdiskobjInit(Disk_t *);

static void	PartobjSort(Disk_t *);
static int	PartobjShift(Disk_t *, int);
static void	PartobjSwap(Disk_t *, int, int);
static int	PartobjSizeUsable(Disk_t *, int, int, int);

static void	SdiskobjInit(Disk_t *);
static void	SdiskobjLockSlices(Disk_t *);
static void	SdiskobjMarkOverlap(Disk_t *);
static void	SdiskobjNullify(Disk_t *);
static int	SdiskobjInitLive(Disk_t *);
static int	SdiskobjInitSim(Disk_t *);

static int	NameIsWholeDisk(char *);
static int	NameIsDiskDevice(char *);
static char *	ParseField(char *, int);
static void	InputInit(char *);
static void	InputComplete(FILE *);
static char *	InputGetLine(void);
static void	InputSetReuse(void);
static void	InputSetFp(FILE *);
static char *	UfsGetLastmount(Disk_t *, int);

/* ioctl for HBA and physical geometry not in <sys/dkio.h> */
#ifndef	DKIOCG_VIRTGEOM
#define	DKIOCG_VIRTGEOM	(DKIOC|33)
#endif

#ifndef	DKIOCG_PHYGEOM
#define	DKIOCG_PHYGEOM	(DKIOC|32)
#endif

#endif /* MODULE_TEST */

/* module private globals */

static int	_Readreuse = 0;
static DIR *	_InputDir = NULL;
static FILE *	_InputFp = NULL;

/* --------------------- Test Interface ------------------------ */

#ifdef MODULE_TEST
main(int argc, char **argv, char **env)
{
	Disk_t *	dp;
	int		n;
	char *		file = NULL;

	while ((n = getopt(argc, argv, "x:d:h")) != -1) {
		switch (n) {
		case 'd':
			(void) SetSimulation(SIM_SYSDISK, 1);
			file = xstrdup(optarg);
			(void) printf("Using %s as an input file\n", file);
			break;
		case 'x':
			(void) set_trace_level(atoi(optarg));
			break;
		case 'h':
			(void) printf(
			    "Usage: %s [-x <debug level>] [-d <disk file>]\n",
			    basename(argv[0]));
			exit(1);
		}
	}

	(void) set_rootdir("/a");
	n = DiskobjInitList(file);
	if (n < 0) {
		(void) printf("Error %d returned from disk load\n", n);
		exit (1);
	}

	(void) printf("%d disks found\n\n", n);
	(void) printf("-----------------------------------\n");

	WALK_DISK_LIST(dp) {
		print_disk(dp, NULL);
		(void) printf("-----------------------------------\n");
	}

	exit (0);
}
#else
/* --------------------- public functions ---------------------- */

/*
 * Function:	DiskobjInitList
 * Description:	Create the primary disk object list, initializing data
 *		either directly from the system configuraiton, or from
 *		an optional dry-run disk configuariotn data file. If the
 *		primary disk object list has already been created, or if a
 *		dry-run file has been specified which is not readable, the
 *		function returns in failure. Otherwise, the function loads
 *		each disk according to the configuration data found, and
 *		returns the number of disk objects initialized in the disk
 *		list. If a dry-run file contains multiple specifications for
 *		a single disk, only the first instance of the specification
 *		is interpreted; remaining instances are considered redundant
 *		and are ignored.
 * Scope:	public
 * Parameters:	file	[RO,*RO] (optional)
 *			optional path name to readable dry-run dat file; NULL
 *			if live data is intended
 *
 * Return:	D_BADARG	a dry-run file was specified, but is
 *				not readable
 *		# >=0		number of disk objects initialized in
 *				the primary disk object list
 */
int
DiskobjInitList(char *file)
{
	int		count = 0;
	char *		disk;

	/* check to see if list is already initialized */
	if (first_disk() != NULL)
		return (0);

	if (GetSimulation(SIM_SYSDISK)) {
		if (file == NULL)
			return (0);

		/*
		 * dry-run: make sure the input file is readable and set the
		 * disk simulation flag
		 */
		if (access(file, R_OK) != 0)
			return (D_BADARG);
	}

	/* set the number of supported partitions */
	if (IsIsa("ppc") || IsIsa("i386"))
		numparts = 16;

	/* initialize the disk listing input stream */
	InputInit(file);

	/*
	 * create a disk object for each viable disk and add the object to
	 * the disk list
	 */
	while ((disk = DiskGetNext()) != NULL) {
		/* skip redundant entries */
		if (find_disk(disk) != NULL)
			continue;

		if (DiskobjInitialize(disk) != NULL)
			count++;
	}

	/* terminate all disk listing input streams */
	InputComplete(NULL);

	/* initialize the boot object */
	(void) BootobjInit();

	return (count);
}

/*
 * Function:	DiskobjCreate
 * Description:	Create a new disk object and initialize the name to
 *		the specified parameter.
 * Scope:	public
 * Parameters:	name	[RO, *RO]
 *			Name to assign to the disk object.
 * Return:	NULL	- creation failed
 *		!NULL	- pointer to newly created disk object
 */
Disk_t *
DiskobjCreate(char *name)
{
	Disk_t	*dp;
	char	*cp;

	if (name == NULL)
		return (NULL);

	/* allocate a new disk structure */
	if ((dp = (Disk_t *) xcalloc(sizeof (Disk_t))) == NULL)
		return (NULL);

	/* initialize the disk name as derived from the device name */
	if ((cp = strrchr(name, 'p')) != NULL ||
				(cp = strrchr(name, 's')) != NULL) {
		*cp = NULL;
		(void) strncpy(disk_name(dp), name, cp - name);
	}

	/* initialize the standard disk flags */
	disk_state_set(dp, DF_INITIAL);

	/* for platforms using fdisk, initialize the flag */
	if (IsIsa("ppc") || IsIsa("i386"))
		disk_state_set(dp, DF_FDISKEXISTS);

	/* for platforms exposing fdisk, initialize the flag */
	if (IsIsa("i386"))
		disk_state_set(dp, DF_FDISKREQ);

	/* set the flag for no sdisk label */
	SdiskobjSetBit(CFG_CURRENT, dp, SF_NOSLABEL);

	/* set the flag for no fdisk label */
	FdiskobjSetBit(CFG_CURRENT, dp, FF_NOFLABEL);

	return (dp);
}

/* --------------------- private functions ---------------------- */
/*
 * Function:	DiskobjInitialize
 * Description:	Create a new disk object with the name specified, and
 *		initialize all data either from a live configuration source,
 *		or from a disk simulation file. If the new disk object 
 *		is successfully created and initialized, it is added
 *		to the active disk object list in order of its name.
 * Scope:	private
 * Parameters:	name	[RO, *RO]
 *			Name of disk object to be initialized (e.g. c0t0d0)
 * Return:	NULL	- could not create a new disk object, or
 *			  controller initialization indicated the disk object
 *			  should not be retained
 *		!NULL	- pointer to the newly created and initialized disk
 *			  object
 */
static Disk_t *
DiskobjInitialize(char *name)
{
	Disk_t *	dp;
	int		n;

	/* create a new disk object and initialize the state */
	if ((dp = DiskobjCreate(name)) == NULL)
		return (NULL);

	/* initialize the data fields for the object */
	if (GetSimulation(SIM_SYSDISK))
		n = DiskobjInitControllerSim(dp);
	else
		n = DiskobjInitControllerLive(dp);

	if (n < 0) {
		DiskobjDestroy(dp);
		return (NULL);
	}

	/* if controller info is okay, initialize the fdisk data */
	if (disk_okay(dp))
		FdiskobjInit(dp);

	/* if controller info is okay, initialize the sdisk dasta */
	if (disk_okay(dp))
		SdiskobjInit(dp);

	/* sychronize all disk states */
	(void) DiskobjSave(CFG_EXIST, dp);
	(void) DiskobjSave(CFG_COMMIT, dp);

	/* mark the disk initialization as complete */
	disk_state_set(dp, DF_INIT);

	/* add the new disk object to the list of disk objects */
	DiskobjAddToList(dp);

	return (dp);
}

/*
 * Function:	FdiskobjInit
 * Description: Initialize the F-disk data structure for 'dp' according to what
 *		is found on the disk. If there is no Solaris partition at this
 *		point, be sure to NULL out the sdisk geometry pointer or it will
 *		incorrectly reference the main disk geometry.
 *
 *		The following sanity checks are performed:
 *
 *		(1) make sure that the numsect field is the same as the label
 *		    geometry's concept of the nsect/cyl * total cyl
 *		(2) make sure no partition projects off the end of the disk
 *
 *		If either of these tests fails, the NOPGEOM error flag which
 *		was previously cleared in _controller_info_init(), is reset.
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object.
 * Return:	none
 */
static void
FdiskobjInit(Disk_t *dp)
{
	char		device[MAXNAMELEN];
	struct dk_geom	dkg;
	char		cmd[64];
	FILE		*pp;
	int		i;
	int		numacyl;
	int		fd;
	int		origpart = 1;
	char *		cp;
	char **		ep;

	/* validate parameters */
	if (dp == NULL)
		return;

	/* this routine is unneccessary unless the system supports an fdisk */
	if (!disk_fdisk_exists(dp))
		return;

	FdiskobjReset(dp);

	if (GetSimulation(SIM_SYSDISK) == 0) {
		/* assemble the command to obtain the fdisk configuration */
		(void) sprintf(device, "/dev/rdsk/%sp0", disk_name(dp));
		if (IsIsa("ppc")) {
			/* fdisk simulator for the PowerPC in read-only mode */
			(void) sprintf(cmd,
			    "/usr/sbin/install.d/prep_partition %s 2>&1",
			    device);
		} else {
			(void) sprintf(cmd,
			    "/sbin/fdisk -R -v -W - %s 2>&1", device);
		}

		if ((pp = (FILE *)popen(cmd, "r")) == NULL)
			return;

		InputSetFp(pp);
	}

	/* read in fdisk table definition data printed in "fdisk -W" format */
	while ((cp = InputGetLine()) != NULL && strstr(cp, "Id") == NULL);

	for (i = 1, cp = InputGetLine();
			valid_fdisk_part(i) && cp != NULL;
			i++, cp = InputGetLine()) {
		/* make sure you have read in a real partition line */
		if (ParseBuffer(cp, &ep) != 10) {
			InputSetReuse();
			break;
		}

		/*
		 * set nsect, nhead, onecyl, and hbacyl from primary
		 * geometry
		 */
		part_geom_nsect(dp, i) = disk_geom_nsect(dp);
		part_geom_nhead(dp, i) = disk_geom_nhead(dp);
		part_geom_onecyl(dp, i) = disk_geom_onecyl(dp);
		part_geom_hbacyl(dp, i) = disk_geom_hbacyl(dp);

		/* set id, active, and the original partition number */
		Partobj_Id(CFG_CURRENT, dp, i) = atoi(ep[0]);
		Partobj_Active(CFG_CURRENT, dp, i) = atoi(ep[1]);
		Partobj_Origpart(CFG_CURRENT, dp, i) = origpart++;

		/* set rsect, tsect, tcyl on used partitions */
		if (part_id(dp, i) != UNUSED) {
			part_geom_rsect(dp, i) = atoi(ep[8]);
			part_geom_tsect(dp, i) = atoi(ep[9]);
			part_geom_tcyl(dp, i) = blocks_to_cyls(
				dp, part_geom_tsect(dp, i));
		}

		/* set firstcyl, lcyl, and lsect based on type */
		if (part_id(dp, i) == SUNIXOS) {
			/*
			 * set the first data cylinder to 1 for the
			 * offset to the boot slice cylinder
			 */
			part_geom_firstcyl(dp, i) = 1;
			numacyl = NUMALTCYL;
			/*
			 * 2.1 labels are only handled for live runs; dry-run
			 * is too messy and the user base too small to warrent
			 * the complexity
			 */
			if (GetSimulation(SIM_SYSDISK) == 0 && IsIsa("i386")) {
				if ((fd = open(device, O_RDONLY|O_NDELAY)) >= 0) {
					if (ioctl(fd, DKIOCGGEOM, &dkg) == 0) 
						numacyl = dkg.dkg_pcyl -
							dkg.dkg_ncyl;
					(void) close(fd);
				}
			}

			part_geom_lcyl(dp, i) = part_geom_tcyl(dp, i) - numacyl;
			part_geom_lsect(dp, i) =
				cyls_to_blocks(dp, part_geom_lcyl(dp, i));
		} else {
			/*
			 * non-Solaris partitions start at the beginning of
			 * the partition and have no alternate cylinders
			 */
			part_geom_firstcyl(dp, i) = 0;
			part_geom_lcyl(dp, i) = part_geom_tcyl(dp, i);
			part_geom_lsect(dp, i) = part_geom_tsect(dp, i);
		}

		/* set the dcyl and dsect values */
		part_geom_dcyl(dp, i) = part_geom_lcyl(dp, i) -
			part_geom_firstcyl(dp, i);
		part_geom_dsect(dp, i) = cyls_to_blocks(dp,
			part_geom_dcyl(dp, i));

		if (part_geom_dcyl(dp, i) < 0)
			part_geom_dcyl(dp, i) = 0;

		if (part_geom_dsect(dp, i) < 0)
			part_geom_dsect(dp, i) = 0;

		if (part_geom_lcyl(dp, i) < 0)
			part_geom_lcyl(dp, i) = 0;

		if (part_geom_lsect(dp, i) < 0)
			part_geom_lsect(dp, i) = 0;

		if (part_geom_firstcyl(dp, i) > part_geom_tcyl(dp, i))
			part_geom_firstcyl(dp, i) = 0;
	}

	/* terminate only the temporary input */
	if (GetSimulation(SIM_SYSDISK) == 0)
		InputComplete(pp);

	/*
	 * finish initializing the orig_partnum and geometry structure
	 * defaults for partitions which where not loaded (remember
	 * that the -W output only prints as many entries as there are
	 * defined partitions)
	 */
	for (; i <= FD_NUMPART; i++) {
		part_orig_partnum(dp, i) = origpart++;
		part_geom_nsect(dp, i) = disk_geom_nsect(dp);
		part_geom_nhead(dp, i) = disk_geom_nhead(dp);
		part_geom_onecyl(dp, i) = disk_geom_onecyl(dp);
		part_geom_hbacyl(dp, i) = disk_geom_hbacyl(dp);
	}

	/* sort the partitions in physically ascending order */
	PartobjSort(dp);

	/*
	 * PowerPC systems must have a Solaris partition; if they
	 * don't, pretend the disk is all around bad so it isn't
	 * presented to the user. This is part of the 2.5.1 hack
	 * and should be removed when full Fdisk partition support
	 * is engineered
	 */
	if (IsIsa("ppc") && get_solaris_part(dp, CFG_CURRENT) == 0)
		disk_state_set(dp, DF_NOPGEOM);

	FdiskobjClearBit(CFG_CURRENT, dp, FF_NOFLABEL);
}

/*
 * Function:	DiskGetNext
 * Description:	Get the next disk from the input stream. For live disk data,
 *		this is the next device in the /dev/rdsk directory which
 *		represents an s2 device. For disk simulation data, the
 *		next input line which represents the start of a data record
 *		is returned; this could be either an s2 or a p0 device,
 *		depending on the record format.
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	- no subsequent disk name found in input stream
 *		!NULL	- pointer to local buffer containing next disk name
 *			  found in input stream
 */
static char *
DiskGetNext(void)
{
	static char	name[MAXNAMELEN];
	struct dirent *	dent;
	char *		cp;
	char *		dev;

	name[0] = '\0';

	if (_InputDir != NULL) {
		while ((dent = readdir(_InputDir)) != NULL) {
			if ((cp = basename(dent->d_name)) == NULL)
				continue;

			if (NameIsWholeDisk(cp)) {
				(void) strcpy(name, cp);
				break;
			}
		}
	} else if (_InputFp != NULL) {
		while ((dev = DiskSeekNext()) != NULL) {
			if ((cp = basename(dev)) == NULL)
				continue;

			if (NameIsDiskDevice(cp)) {
				(void) strcpy(name, cp);
				break;
			}
		}
	}

	return (name[0] == '\0' ? NULL : name);
}

/*
 * Function:	DiskSeekNext
 * Description: Search the disk input data stream for the next valid
 *		disk name. For SPARC systems, the input stream pattern
 *		is "partition map" (part of the prtvtoc(9M) output
 *		format), and for Intel and PowerPC, the format is "fdisk
 *		table" (part of the fdisk(9M) output format).
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	- the input stream has not been initialized,
 *			  or there is no more input matching the
 *			  specified pattern
 *		!NULL	- pointer to a local static buffer containing
 *			  the name of the disk found in its full
 *			  absolute path name form
 */
static char *
DiskSeekNext(void)
{
	static char	name[MAXNAMELEN];
	char		pattern[32];
	char *		cp;
	char *		bufp;

	name[0] = '\0';

	if (_InputFp == NULL)
		return (NULL);

	if (IsIsa("sparc"))
		(void) strcpy(pattern, "partition map");
	else
		(void) strcpy(pattern, "fdisk table");

	while ((bufp = InputGetLine()) != NULL) {
		if (*bufp == '*' &&
				strstr(bufp, pattern) &&
				(cp = ParseField(bufp, 2)) != NULL) {
			(void) strcpy(name, cp);
			break;
		}
	}

	return (name[0] == '\0' ? NULL : name);
}

/*
 * Function:	InputGetLine
 * Description:	Retrieve a new-line terminated line of input from the
 *		data input source. If the input reuse flag is set, then the data
 *		currently residing in the input buffer should be returned; 
 *		otherwise, an input read is done and the data is returned.
 *		If the read re-use flag is set, it is reset before returning.
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	- new input is required and there is no more
 *			  input available from the input file currently set,
 *			  or the retrieval returned NULL data.
 *		!NULL	- pointer to local buffer containing next input line,
 *			  or last input line if the re-use flag is set.
 */
static char *
InputGetLine(void)
{
	static char	buf[MAXNAMELEN] = "";
	char *		cp = &buf[0];

	if (_Readreuse) {
		/* reuse the existing buffer */
		_Readreuse = 0;
	} else {
		if (_InputFp == NULL || feof(_InputFp) != 0)
			return (NULL);

		buf[0] = '\0';
		cp = fgets(buf, sizeof (buf), _InputFp);
		if ((cp != NULL) && (strlen(buf) > 0))
			buf[strlen(buf) - 1] = '\0';
	}

	return (buf);
}

/*
 * Function:	InputSetFp
 * Description:	Set the input data file pointer. The input data file
 *		is used for input data reads in other functions.  Reset
 *		the reuse flag if a new file is being used.
 * Scope:	private
 * Parameter:	fp	[RO, *RO]
 *			File point to be used as the set source value.
 * Return:	none
 */
static void
InputSetFp(FILE *fp)
{
	if (_InputFp != fp)
		_Readreuse = 0;

	_InputFp = fp;
}

/*
 * Function:	InputSetReuse
 * Description:	Set the input reuse flag to "on" to indicate that the
 *		value sitting in the current input buffer should be
 *		reused on the next input read attempt rather than
 *		fetching a new set of input data.
 * Scope:	private
 * Parameter:	none
 * Return:	none
 */
static void
InputSetReuse(void)
{
	_Readreuse = 1;
}

/*
 * Function:	ParseField
 * Description: Search through a buffer containing a string of tokens
 *		separated by white space (spaces and/or tabs), for the
 *		field index specified by the user. Field position ounting
 *		starts at '1'.
 * Scope:	private
 * Parameters:	buf	- [R0, *RW]
 *			  Non-NULL buffer containing a string of zero of more
 *			  tokens separated by spaces and/or tabs.
 *		field	- [RO]
 *			  Field position index >= 1 indicating which
 *			  field in the buffer is being parsed.
 * Return:	NULL	- invalid parameters, or field index exceeds
 *			  the string contents
 */
static char *
ParseField(char *buf, int field)
{
	char	*cp;

	if (buf == NULL || field <= 0)
		return (NULL);

	/*
	 * parse out the first field and decrement the loop initializer
	 * to account for that parsing; index to the position desired
	 */
	cp = strtok(buf, " \t");
	for (field--; field > 0 && cp != NULL; field--)
		cp = strtok(NULL, " \t");

	return (cp);
}

/*
 * Function:	NameIsWholeDisk
 * Description:	Boolean function checking to see if a string represents
 *		the strictly cannonical device name for an s2 special
 *		devices. Example: "c0t3d0s2" returns `1': "c0t3d0s1"
 *		returns `0'.
 * Scope:	private
 * Parameters:	name	- [RO, *RO]
 *			  String containing disk name to be evaluated;
 *			  this is the device name and not the path to
 *			  the device
 * Return:	0	- the string is not a whole disk name
 *		1	- the string does represent a whole disk name
 */
static int
NameIsWholeDisk(char *name)
{
	/* validate parameters */
	if (name == NULL)
		return (0);

	/* controller specifier */
	must_be(name, 'c');
        skip_digits(name);

        /* target specifier (optional) */
        if (*name == 't') {
		must_be(name, 't');
                skip_digits(name);
	}

        /* drive specifier */
	must_be(name, 'd');
        skip_digits(name);

        /* slice 2 specifier */
	must_be(name, 's');
	if (*name != '2')
		return (0);

        return (1);
}

/*
 * Function:	NameIsDiskDevice
 * Description:	Boolean function to determine if a given string
 *		represents a disk device name (e.g. c0t3d0s2 or
 *		c0d0p1).
 * Scope:	private
 * Parameters:	name	- [RO, *RO]
 *			  String containing disk name to be evaluated;
 *			  this is the device name and not the path to
 *			  the device
 * Return:	0	- the string is not a whole disk name
 *		1	- the string does represent a whole disk name
 */
static int
NameIsDiskDevice(char *name)
{
	/* validate parameters */
	if (name == NULL)
		return (0);
	
	/* controller specifier */
	must_be(name, 'c');
        skip_digits(name);

        /* target specifier (optional) */
        if (*name == 't') {
		must_be(name, 't');
                skip_digits(name);
	}

        /* drive specifier */
	must_be(name, 'd');
        skip_digits(name);
	if (*name != 's' && *name != 'p')
		return (0);

	++name;
	skip_digits(name);
	if (*name != '\0')
		return (0);

	return (1);
}

/*
 * Function:	InputInit
 * Description:	Initialize the input for obtaining disk device listing
 *		data. For live disk executions, this entails
 *		initializing access to the /dev/rdsk directory for reading.
 *		For disk simulation executions this entails opening the disk
 *		simulation data file for reading.
 * Scope:	private
 * Parameters:	file	- [RO, *RO] (optional)
 *			  Name of disk input file from which disk
 *			  simulation data will be drawn, or NULL to
 *			  initialize input for live disk configuration
 *			  collection.
 * Return:	none
 */
static void
InputInit(char *file)
{
	if (file == NULL) {
		/* live data loading */
		if (_InputDir == NULL) {
			_InputDir = opendir("/dev/rdsk");
		}
	} else {
		/* simulation data loading */
		_InputFp = fopen(file, "r");
	}
}

/*
 * Function:	InputComplete
 * Description:	Terminate the disk configuration input mechanism, either live
 *		or simulation, and reset the input data re-use flag so that
 *		stale data is not accidentally retrieved by subsequent reads.
 *		Reset the directory and/or input file pointers after closing.
 *		If the user specifies an explicit file pointer value as a 
 *		parameters, then only the input file will be closed (and
 *		the reuse flag reset).
 * Scope:	private
 * Parameters:	fp	[RO, *RO]
			Pointer to file being read indicating only
			the file attributes are to be closed, or NULL if both
			file and directory attributes are to be closed.
 * Return:	none
 */
static void
InputComplete(FILE *fp)
{
	/*
	 * only close directory input if this is not an explicit
	 * file-only close request
	 */
	if ((fp == NULL) && (_InputDir != NULL)) {
		(void) closedir(_InputDir);
		_InputDir = NULL;
	}

	if (_InputFp != NULL) {
		(void) fclose(_InputFp);
		_InputFp = NULL;
	}

	_Readreuse = 0;
}

/*
 * Function:	DiskobjInitControllerSim
 * Description: Initialize the disk object controller data, primary disk
 *		geometry, and HBA geometry (if available) by parsing
 *		the data from a disk input simulation file. This function
 *		will return a `0' status even if data initialization
 *		could not be completed; the state of completion is
 *		reflected in the state of the disk flags. This
 *		function handles both fdisk(9M) input following PSARC 1995/408,
 *		and prtvtoc input formats for SPARC (PSARC).
 * Scope:	private
 * Parameters:	dp	- [RO, *RO]
 *			  Non-NULL pointer to disk object in which to
 *			  initialize data
 * Return:	 0	- data initialization completed to the extent
 *			  possible and the disk object should be
 *			  retained for further processing
 *		-1	- data initialization failed and the disk object
 *			  should be destroyed
 */
static int
DiskobjInitControllerSim(Disk_t *dp)
{
	char **	ep;
	char *	cp;
	char *	sp;
	int	nsect = -1;
	int	nhead = -1;
	int	ctype;

	/* validate parameters */
	if (dp == NULL)
		return (-1);

	/* look for the CTYPE field */
	if ((cp = InputGetLine()) == NULL)
		return (-1);

	if (strstr(cp, "* CTYPE=") == NULL) {
		ctype = DKC_SCSI_CCS;
		InputSetReuse();
	} else {
		sp = strchr(cp, '=');
		/* 
		 * if the field has a non-numeric value, simulate
		 * an ioctl(DKIOCINFO) call failure
		 */
		++sp;
		ctype = atoi(sp);
		if (!is_allnums(sp) || ctype == DKC_CDROM)
			return (-1);
	}

	disk_state_unset(dp, DF_BADCTRL);
	disk_ctype_set(dp, ctype);

	/* unknown controller type */
	if (disk_ctype(dp) == DKC_UNKNOWN)
		return (0);

	disk_state_unset(dp, DF_UNKNOWN);

	/* fetch the primary geometry data */
	while ((cp = InputGetLine()) != NULL &&
			strstr(cp, "* Dimensions:") == NULL);

	if ((cp = InputGetLine()) == NULL ||
			ParseBuffer(cp, &ep) != 3 ||
			!streq(ep[2], "bytes/sector"))
		return (0);

	disk_geom_firstcyl(dp) = disk_state_test(dp, DF_FDISKEXISTS) ? 1 : 0;
	if ((cp = InputGetLine()) == NULL ||
			ParseBuffer(cp, &ep) != 3 ||
			!streq(ep[2], "sectors/track"))
		return (0);
	else
		disk_geom_nsect(dp) = atoi(ep[1]);

	if ((cp = InputGetLine()) == NULL ||
			ParseBuffer(cp, &ep) != 3 ||
			!streq(ep[2], "tracks/cylinder"))
		return (0);
	else
		disk_geom_nhead(dp) = atoi(ep[1]);

	disk_geom_onecyl(dp) = disk_geom_nhead(dp) * disk_geom_nsect(dp);

	/* skip the sectors/cylinder line if there is one */
	if ((cp = InputGetLine()) == NULL)
		return (0);
	else if (strstr(cp, "sectors/cylinder") == NULL)
		InputSetReuse();

	if ((cp = InputGetLine()) == NULL ||
			ParseBuffer(cp, &ep) != 3 ||
			!streq(ep[2], "cylinders"))
		return (0);

	disk_geom_tcyl(dp) = atoi(ep[1]);
	disk_geom_tsect(dp) = cyls_to_blocks(dp, disk_geom_tcyl(dp));

	if ((cp = InputGetLine()) != NULL &&
			ParseBuffer(cp, &ep) == 4 &&
			streq(ep[2], "accessible"))
		disk_geom_lcyl(dp) = atoi(ep[1]);
	else
		disk_geom_lcyl(dp) = disk_geom_tcyl(dp);

	disk_geom_lsect(dp) = cyls_to_blocks(dp, disk_geom_lcyl(dp));
	disk_geom_dcyl(dp) = disk_geom_lcyl(dp) - disk_geom_firstcyl(dp);
	disk_geom_dsect(dp) = cyls_to_blocks(dp, disk_geom_dcyl(dp));
	disk_geom_rsect(dp) = 0;
	disk_geom_hbacyl(dp) = disk_geom_onecyl(dp);

	/* set HBA geometry (if appropriate) */
	if (IsIsa("i386")) {
		while ((cp = InputGetLine()) != NULL) {
			if (ParseBuffer(cp, &ep) == 1 || !streq(ep[0], "*")) {
				InputSetReuse();
				break;
			}

			if (streq(ep[1], "HBA"))
				break;
		}

		if (cp != NULL && strstr(cp, "HBA")) {
			while ((cp = InputGetLine()) != NULL) {		
				if (ParseBuffer(cp, &ep) <= 1) {
					InputSetReuse();
					break;
				}

				if (strstr(cp, "sectors/track") != NULL &&
						ParseBuffer(cp, &ep) == 3)
					nsect = atoi(ep[1]);

				if (strstr(cp, "tracks/cylinder") != NULL &&
						ParseBuffer(cp, &ep) == 3)
					nhead = atoi(ep[1]);
			}

			if (nsect > 0 && nhead > 0)
				disk_geom_hbacyl(dp) = nsect * nhead;
		}
	}

	/* check if the geometry is bogus */
	if (disk_geom_tcyl(dp) == 0 || disk_geom_nsect(dp) == 0) {
		/* cannot format IDE disks */
		if (disk_ctype(dp) == DKC_DIRECT)
			disk_state_set(dp, DF_CANTFORMAT);

		return (0);
	}

	disk_state_unset(dp, DF_NOPGEOM);
	return (0);
}

/*
 * Function:	DiskobjInitControllerLive
 * Description: Determine the controller type on a live disk execution, and
 *		make sure the controller is actual responding. This routine
 *		will only "fail" if the drive is considered to be inoperable;
 *		this includes CDROM devices, which are not considered
 *		to be operable disk media.
 * Scope:	private
 * Parameters:	dp	- [RO, *RO]
 *			  Non-NULL pointer to a valid disk object.
 * Return:	 0	- data initialization completed to the extent
 *			  possible and the disk object should be
 *			  retained for further processing
 *		-1	- data initialization failed and the disk object
 *			  should be destroyed
 */
static int
DiskobjInitControllerLive(Disk_t *dp)
{
	struct dk_cinfo	dkc;
	struct dk_geom	dkg;
	int		fd;
	int		n;
	char		device[MAXNAMELEN];

	/* find the appropriate device for the system */
	if (disk_state_test(dp, DF_FDISKEXISTS))
		(void) sprintf(device, "/dev/rdsk/%sp0", disk_name(dp));
	else
		(void) sprintf(device, "/dev/rdsk/%ss2", disk_name(dp));

	/* try to access the controller */
	if ((fd = open(device, O_RDONLY|O_NDELAY)) < 0)
		return (-1);

	if (ioctl(fd, DKIOCINFO, &dkc) < 0 || dkc.dki_ctype == DKC_CDROM) {
		(void) close(fd);
		return (-1);
	} else {
		disk_state_unset(dp, DF_BADCTRL);
		disk_ctype_set(dp, dkc.dki_ctype);

		/* unknown controller type */
		if (dkc.dki_ctype == DKC_UNKNOWN) {
			(void) close(fd);
			return (0);
		}
	}

	disk_state_unset(dp, DF_UNKNOWN);

	/* fetch the primary geometry data */
	if (IsIsa("sparc"))
		n = ioctl(fd, DKIOCGGEOM, &dkg);
	else
		n = ioctl(fd, DKIOCG_PHYGEOM, &dkg);

	/* set the primary geometry */
	disk_geom_firstcyl(dp) = disk_state_test(dp, DF_FDISKEXISTS) ? 1 : 0;
	disk_geom_nhead(dp) = dkg.dkg_nhead;
	disk_geom_nsect(dp) = dkg.dkg_nsect;
	disk_geom_onecyl(dp) = dkg.dkg_nhead * dkg.dkg_nsect;
	disk_geom_tcyl(dp) = dkg.dkg_pcyl;
	disk_geom_tsect(dp) = cyls_to_blocks(dp, disk_geom_tcyl(dp));
	disk_geom_lcyl(dp) = dkg.dkg_ncyl;
	disk_geom_lsect(dp) = cyls_to_blocks(dp, disk_geom_lcyl(dp));
	disk_geom_dcyl(dp) = disk_geom_lcyl(dp) - disk_geom_firstcyl(dp);
	disk_geom_dsect(dp) = cyls_to_blocks(dp, disk_geom_dcyl(dp));
	disk_geom_rsect(dp) = 0;
	disk_geom_hbacyl(dp) = disk_geom_onecyl(dp);

	/* set HBA geometry (if appropriate) */
	if (IsIsa("i386") && ioctl(fd, DKIOCG_VIRTGEOM, &dkg) == 0)
		disk_geom_hbacyl(dp) = dkg.dkg_nhead * dkg.dkg_nsect;

	(void) close(fd);

	/* check if the geometry is bogus */
	if (n < 0 || dkg.dkg_pcyl == 0 || dkg.dkg_nsect == (u_short)0) {
		/* cannot format IDE disks */
		if (disk_ctype(dp) == DKC_DIRECT)
			disk_state_set(dp, DF_CANTFORMAT);
		return (0);
	}

	disk_state_unset(dp, DF_NOPGEOM);
	return (0);
}

/*
 * Function:	PartobjSort
 * Description: Make sure that blocks of unused space in the F-disk partition
 *		table have an UNUSED partition relative to their location
 *		on the disk. For example, if there is unused space at the
 *		beginning of the drive, make sure partition 1 is UNUSED so
 *		that install can allocate it.
 *
 *		ALGORITHM:
 *			Unused partitions are assigned to the largest holes
 *			to ensure maximum use of the drive.
 *
 *			(1) determine how many unused partitions there are.
 *			    If none or all are unused, no work to do.
 *			(2) for each used partition which doesn't have an
 *			    UNUSED partition preceding it, check to see if
 *			    it qualifies as "biggest" of remaining holes,
 *			    and if so, shift it down in the partition table
 *			    and create an UNUSED partition in front of it.
 *
 *		NOTE:	This sorting routine relies on the fact that
 *			Solaris fdisk(1M) displays partitions in
 *			ascending order based on their relative
 *			physical position on the disk (i.e. physical
 *			order == real order). For this reason, the
 *			input file is always sorted before processing
 *			for unused space.
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object loaded F-disk
 *		  	partition information.
 * Return:	none
 */
static void
PartobjSort(Disk_t *dp)
{
	int	part;
	int	start;
	int	count;
	int	size;
	int	next;

	start = one_cyl(dp);
	count = 0;

	/* sort the table (if necessary) */
	WALK_PARTITIONS(part) {
		if (part_id(dp, part) == UNUSED)
			continue;

		for (next = part + 1; next <= FD_NUMPART; next++) {
			if (part_id(dp, next) == UNUSED)
				continue;

			if (part_startsect(dp, next) < part_startsect(dp, part))
				PartobjSwap(dp, part, next);
		}
	}

	/* find how many unused partitions exist */
	WALK_PARTITIONS(part) {
		if (part_id(dp, part) == UNUSED)
			count++;
	}

	/* no work to do if no free partitions or all free partitions */
	if (count == 0 || count == FD_NUMPART)
		return;

	WALK_PARTITIONS(part) {
		if (count <= 0)
			break;

		if (part_id(dp, part) == UNUSED)
			continue;

		if (part > 1 && part_id(dp, part - 1) == UNUSED) {
			start = part_startsect(dp, part) +
					part_size(dp, part);
			continue;
		}

		size = part_startsect(dp, part) - start;

		/*
		 * if there is a hole before "part", try to shift
		 * part down in the partition table - may have to
		 * scoot others down too. If this fails, you're
		 * done, otherwise, decrement the UNUSED partition
		 * counter and skip over the next partition since
		 * you know you just shifted the current one to that
		 * position
		 */
		if (PartobjSizeUsable(dp, part, count, size)) {
			if (PartobjShift(dp, part) < 0)
				return;

			--count;
			++part;
		}

		/* advance the hole start to the end of this partition */
		start = part_startsect(dp, part) + part_size(dp, part);
	}
}

/*
 * Function:	PartobjShift
 * Description: Shift the specified F-disk partition entry to the next slot.
 *		If the next slot is not UNUSED, then shift that entry, and
 *		so forth, until you hit the end of the table, or you
 *		successfully shift.
 *
 *		ALGORITHM:
 *		- recursive
 *		- terminating condition: part == last valid partition
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a disk object.
 *		part	[RO]
 *			Valid partition index to be shifted down in the table.
 * Return:	 0	- shift successful
 *		-1	- no more partitions to shift to; shift failed
 */
static int
PartobjShift(Disk_t *dp, int part)
{
	Part_t		tmp;

	/* validate parameters */
	if (dp == NULL || !valid_fdisk_part(part))
		return (-1);

	/* terminating condition */
	if (part == FD_NUMPART)
		return (-1);

	if (part_id(dp, part + 1) != UNUSED) {
		if (PartobjShift(dp, part + 1) < 0)
			return (-1);
	}

	/* swap the UNUSED entry with the used entry */
	(void) memcpy(&tmp, fdisk_part_addr(dp, part + 1), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part + 1),
			fdisk_part_addr(dp, part), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part), &tmp, sizeof (Part_t));

	return (0);
}

/*
 * Function:	PartobjSwap
 * Description: Swap two partition entries in an Fdisk_t data structure
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object.
 *		next	[RO]
 *			Valid logical partition index to switch.
 *		part	[RO]
 *			Valid logical partition index to switch.
 * Return:	none
 */
static void
PartobjSwap(Disk_t *dp, int part, int next)
{
	Part_t	tmp;

	/* swapping a partition with itself; we're done */
	if (part == next)
		return;

	(void) memcpy(&tmp, fdisk_part_addr(dp, part), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part),
			fdisk_part_addr(dp, next), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, next), &tmp, sizeof (Part_t));
}

/*
 * Function:	PartobjSizeUsable
 * Description: Determine if a given partition on the disk is preceded by unused
 *		space, and if so, if that space is larger than gaps which occur
 *		later on the drive such that this partition is warranted being
 *		assigned one of the UNUSED partitions.
 *
 *		ASSUMPTIONS:
 *		- partitions MUST be ordered in physically ascending order
 *		  at this point
 *		NOTE:
 *		- we ignore partitions of size < one_cyl(), since that
 *		  space will not be usable in the Solaris domain
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object.
 *		part	[RO]
 *			Non-UNUSED partition number for which you want to find
 *			if there's a preceding hole
 *		count	[RO]
 *			Number of UNUSED partitions on the disk (# >= 0)
 *		size	[RO]
 *			Size of hole preceding the current partition
 *			(# >= one cylinder)
 * Return:	0	- there is no preceding hole, or the hole that
 *		  	  exists is not big enough relative to other
 *		  	  subsequent holes to warrant being allocated
 *		  	  an UNUSED partition
 *		1	- the preceding hole is large enough to warrant
 *		  	  being assigned an UNUSED partition
 */
static int
PartobjSizeUsable(Disk_t *dp, int part, int count, int size)
{
	int	start;
	int	hole;
	int	holecnt;
	int	bigger;
	int	i;

	/* validate parameters */
	if (dp == NULL || count < 0)
		return (0);

	holecnt = 0;
	bigger = 0;

	/*
	 * the hole is unusable if it is less than one cylinder,
	 * or the # of available UNUSED partitions is unusable
	 */
	if (size < one_cyl(dp) || count <= 0)
		return (0);

	/* start searching for holes after the current partition */
	start = part_startsect(dp, part) + part_size(dp, part);

	for (i = part + 1; i <= FD_NUMPART; i++) {
		if (part_id(dp, i) == UNUSED)
			continue;

		hole = part_startsect(dp, i) - start;

		if (hole > one_cyl(dp)) {
			holecnt++;
			if (size >= hole)
				bigger++;
		}

		start = part_startsect(dp, i) + part_size(dp, i);
	}
	/* check the end of the drive in case there is a hole there */
	hole = disk_geom_lsect(dp) - start;

	if (hole > one_cyl(dp)) {
		holecnt++;
		if (size >= hole)
			bigger++;
	}

	/*
	 * if the given hole is bigger than "N" other subsequent holes
	 * and "N" is the number of subsequent holes for which there
	 * are no UNUSED partitions available, then this hole should
	 * get an UNUSED partition
	 */
	if (bigger > (holecnt - count))
		return (1);

	return (0);
}

/*
 * Function:	SdiskobjLockSlices
 * Description: Set the lock bit on all restricted slices. This routine is
 *		used during disk initialization after slice configuration has
 *		completed.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (disk state: okay)
 *			Non-NULL pointer to a valid disk object.
 * Return:	none
 */
static void
SdiskobjLockSlices(Disk_t *dp)
{
	int	i;

	/* validate parameters */
	if (dp == NULL || !disk_okay(dp))
		return;

	WALK_SLICES(i) {
		if (i > LAST_STDSLICE) {
			(void) SliceobjSetAttributePriv(dp, i,
				SLICEOBJ_LOCKED,  TRUE,
				NULL);
		}
	}
}

/*
 * Functions:	SdiskobjNullify
 * Description:	Null out the entire current S-disk structure and set the lock
 *		flag on all locked system slices.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (disk state: okay)
 *			Non-NULL pointer to a disk object requiring
 *			sdisk structuring nullification.
 * Return:	none
 */
static void
SdiskobjNullify(Disk_t *dp)
{
	int	slice;

	/* validate parameters */
	if (dp == NULL || !disk_okay(dp))
		return;

	(void) memset(Sdiskobj_Addr(CFG_CURRENT, dp), 0, sizeof (Sdisk_t));
	/* set the instances to -1 for all slices at this point */
	WALK_SLICES(slice)
		Sliceobj_Instance(CFG_CURRENT, dp, slice) = VAL_UNSPECIFIED;

	SdiskobjLockSlices(dp);
}

/*
 * Function:	SdiskobjInit
 * Description:	Initialize the data in the sdisk portion of a disk object.
 *		All sdisk data areas are initialized with NULL data,
 *		and restricted slices are locked from user modification access.
 *		Data retrieval for systems supporting fdisk is only
 *		required if there is a Solaris partition. The sdisk geometry
 *		is initialized to the primary geometry for SPARC systems, 
 *		and to the fdisk Solaris partition geometry for
 *		systems possessing a Solaris partition. Successful execution
 *		of this function clears the SF_NOSLABEL sdisk flag.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (disk state: okay)
 *			Non-NULL pointer to the disk object requiring
 *			initialization.
 * Return:	none
 */
static void
SdiskobjInit(Disk_t *dp)
{
	int	p;
	int	n;

	/*
	 * clear the sdisk data structure and reset according to S-disk
	 * geometry
	 */
	SdiskobjNullify(dp);

	/* set the sdisk geometry pointer */
	if (disk_fdisk_exists(dp)) {
		/* only set fdisk systems with Solaris partitions */
		if ((p = get_solaris_part(dp, CFG_CURRENT)) == 0)
			return;

		Sdiskobj_Geom(CFG_CURRENT, dp) =
				Partobj_GeomAddr(CFG_CURRENT, dp, p);
	} else
		Sdiskobj_Geom(CFG_CURRENT, dp) = disk_geom_addr(dp);
	
	if (GetSimulation(SIM_SYSDISK))
		n = SdiskobjInitSim(dp);
	else
		n = SdiskobjInitLive(dp);

	/*
	 * if the load was successful, clear the NOLABEL flag;
	 * otherwise, clear the sdisk object and erase all data
	 * loaded in error
	 */
	if (n == 0)
		SdiskobjClearBit(CFG_CURRENT, dp, SF_NOSLABEL);
	else
		(void) _reset_sdisk(dp);
}

/*
 * Function:	SdiskobjInitLive
 * Description: Retrieve the disk label VTOC info from the drive and
 *		load the "start" and "size" information found there
 *		into the current slice table. Then scan the superblock
 *		area for each slice and, store the mount name for the
 *		slice. Overlapping slices will result in a "best fit"
 *		selection with the other slice marked "overlap".
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to disk object requiring
 *			sdisk data initialization
 * Return:	 0	- sdisk data was initialized successfully
 *		-1	- sdisk data initialization failed
 */
static int
SdiskobjInitLive(Disk_t *dp)
{
	struct vtoc	vtoc;
	int		slice;
	int		j;
	int		over;
	int		size;
	int		start;
	int		fd;
	int		realigned;
	char *		use;

	if (dp == NULL || (fd = open(make_char_device(disk_name(dp), 2),
			O_RDONLY|O_NDELAY)) < 0)
		return(-1);

	if (read_vtoc(fd, &vtoc) < 0) {
		(void) close(fd);
		return (-1);
	}

	/*
	 * set slice start, size, and mount point fields (based on VTOC tag
	 * information if possible)
	 */
	WALK_SLICES(slice) {
		realigned = FALSE;
		/*
		 * allow alternate sector slice lock override on fdisk systems
		 * if the existing size is > 0 in order to preserve alternate
		 * sectors situated in a different location from the current
		 * default
		 */
		if (SliceobjIsLocked(CFG_CURRENT, dp, slice) && slice != ALT_SLICE)
			continue;

		/* explicitly set the vtag field for slices 8 and 9 */
		if (slice == ALT_SLICE)
			vtoc.v_part[slice].p_tag = V_ALTSCTR;
		else if (slice == BOOT_SLICE)
			vtoc.v_part[slice].p_tag = V_BOOT;

		/* adjust the starting cylinder to next cylinder boundary */
		start = blocks_to_cyls(dp, vtoc.v_part[slice].p_start);

		/*
		 * set the slice size with appropriate adjustments 
		 * for cylinder rounding
		 */
		if (slice == ALT_SLICE && _disk_is_scsi(dp)) {
			size = 0;
		} else if ((over = vtoc.v_part[slice].p_start % one_cyl(dp)) > 0) {
			size = vtoc.v_part[slice].p_size - one_cyl(dp) + over;
			realigned = TRUE;
		} else {
			size = vtoc.v_part[slice].p_size;
		}

		/*
		 * fix for bug id 1266170, make sure that the size of
		 * the slice is set using SliceobjSetAttributePriv()
		 * before getting the name of the filesystem. This is
		 * UfsGetLastmount is dependent on the size of the slice
		 * being set.
		 */
		(void) SliceobjSetAttributePriv(dp, slice,
			SLICEOBJ_SIZE,	   size,
			NULL);

		/* only set the slice use for slices with size > 0 */
		if (size == 0) {
			use = "";
		} else {
			switch (vtoc.v_part[slice].p_tag) {
			    case V_ROOT:
				use = ROOT;
				break;
			    case V_SWAP:
				use = SWAP;
				break;
			    case V_BACKUP:
				use = OVERLAP;
				break;
			    case V_CACHE:
				use = CACHE;
				break;
			    case V_ALTSCTR:
				use = ALTSECTOR;
				break;
			    default:
				if ((use = UfsGetLastmount(dp, slice)) == NULL)
					use = "";
				break;
			}
		}

		/*
		 * set the slice attributes; the instance is '0' for
		 * file systems, and unspecified for all other slices; this
		 * is consistent with the resource object model which does
		 * allow multiple instances for non-directory resources, 
		 * but only allows instance 0 for directory resources
		 */
		(void) SliceobjSetAttributePriv(dp, slice,
			SLICEOBJ_USE,	   use,
			SLICEOBJ_INSTANCE, is_pathname(use) ? 0 :
						VAL_UNSPECIFIED,
			SLICEOBJ_START,	   start,
			SLICEOBJ_SIZE,	   size,
			SLICEOBJ_REALIGNED, realigned,
			NULL);
	}

	/*
	 * eliminate overlapping slices caused by concurrent superblocks;
	 * give credit to smaller (best fit)
	 */
	WALK_SLICES(slice) {
		if (!slice_mntpnt_is_fs(dp, slice))
			continue;

		for (j = slice + 1; j < numparts; j++) {
			if (!slice_mntpnt_is_fs(dp, j))
				continue;
			/*
			 * check for same mount points with matching
			 * superblock areas
			 */
			if (streq(slice_use(dp, slice), slice_use(dp, j)) &&
					slice_start(dp, slice) ==
						slice_start(dp, j)) {
				(void) strcpy(slice_use(dp,
					slice_size(dp, slice) <= slice_size(dp, j)
					? j : slice), "");
			}
		}
	}

	SdiskobjMarkOverlap(dp);
	(void) close(fd);
	return (0);
}

/*
 * Function:	SdiskobjMarkOverlap
 * Description: Scan through the slices and mark those which are unlabelled
 *		and which overlap other sized slices as OVERLAP. First mark
 *		unnamed slices which overlap named slices. Then mark slices
 *		which span the whole drive and are unnamed. Then, if two
 *		unlabelled slices occur which overlap, the latter of the
 *		two is marked.
 *
 *		NOTE:	Used at disk structure initialization time.
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to disk object.
 * Return:	none
 */
static void
SdiskobjMarkOverlap(Disk_t *dp)
{
	int	i;
	int	j;
	int	count;
	int	*sp;

	/* mark unnamed slices which overlap named slices */
	for (i = numparts - 1; i >= 0; i--) {
		if (slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) == 0 ||
				(SliceobjIsLocked(CFG_CURRENT, dp, i) &&
				    disk_initialized(dp)))
			continue;

		count = slice_overlaps(dp, i, slice_start(dp, i),
				slice_size(dp, i), &sp);

		if (count != 0) {
			for (j = 0; j < count; j++) {
				if (slice_mntpnt_is_fs(dp, sp[j])) {
					(void) strcpy(Sliceobj_Use(CFG_CURRENT,
							dp, i), OVERLAP);
					break;
				}
			}
		}
	}

	/* mark slice 2 if it's an unnamed slice and spans the drive */
	if (streq(Sliceobj_Use(CFG_CURRENT, dp, ALL_SLICE), "") &&
			Sliceobj_Instance(CFG_CURRENT, dp, ALL_SLICE) ==
				VAL_UNSPECIFIED &&
			Sliceobj_Size(CFG_CURRENT, dp, ALL_SLICE) ==
				accessible_sdisk_blks(dp)) {
		(void) strcpy(Sliceobj_Use(CFG_CURRENT,
			dp, ALL_SLICE), OVERLAP);
	}

	/* mark unnamed slices which overlap non-overlap slices */
	for (i = numparts - 1; i >= 0; i--) {
		if (slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) == 0 ||
				(SliceobjIsLocked(CFG_CURRENT, dp, i) &&
				    disk_initialized(dp)))
			continue;

		if (slice_overlaps(dp, i, slice_start(dp, i),
				slice_size(dp, i), (int **)0) != 0)
			(void) strcpy(Sliceobj_Use(CFG_CURRENT, dp, i),
				OVERLAP);
	}
}

/*
 * Function:	UfsGetLastmount
 * Description: Get the superblock from a slice (if available).
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a disk object.
 *		slice	[RO]
 *			Valid slice index number.
 * Return:	NULL	- no valid FS name found
 *		char *	- pointer to temporary structure containing FS name
 * Note:	Open the raw device, scan to the superblock offset, and read
 *		what should be the first superblock (assuming there was one -
 *		check the "magic" field to see). If the name given is "/a...",
 *		then strip off the leading "/a" to get the name of the real file
 *		system, otherwise, just copy the name.
 */
static char *
UfsGetLastmount(Disk_t *dp, int slice)
{
	static int		sblock[SBSIZE/sizeof (int)];
	static struct fs 	*fsp = (struct fs *) sblock;
	char			devpath[MAXNAMELEN];
	int			fd;

	(void) memset(fsp, 0, (SBSIZE/sizeof (int)) * sizeof (int));
	(void) strcpy(devpath, make_char_device(disk_name(dp), slice));

	/* attempt to open the disk; if it fails, skip it */
	if ((fd = open(devpath, O_RDONLY|O_NDELAY)) < 0)
		return (NULL);

	if (lseek(fd, SBOFF, SEEK_SET) == -1) {
		(void) close(fd);
		return (NULL);
	}

	if (read(fd, fsp, sizeof (sblock)) != sizeof (sblock)) {
		(void) close(fd);
		return (NULL);
	}

	(void) close(fd);

	/* make sure you aren't going to load bogus data */
	if (fsp->fs_magic != FS_MAGIC ||
			fsp->fs_fsmnt[0] != '/' ||
			strlen(fsp->fs_fsmnt) > (size_t)(MAXMNTLEN - 1))
		return (NULL);

	/*
	 * make sure the suprblock does not represent a file system bigger
	 * than the slice
	 */

	if (fsp->fs_ncyl > blocks_to_cyls(dp, slice_size(dp, slice)))
		return (NULL);
	if (strcmp(fsp->fs_fsmnt, "/a") == 0)
		return (ROOT);
	if (strncmp(fsp->fs_fsmnt, "/a/", 3) == 0)
		return (&fsp->fs_fsmnt[2]);
	return (fsp->fs_fsmnt);
}

/*
 * Functions:	SdiskobjInitSim
 * Description: Load the information for slices from the prtvtoc output. If
 *		there are no slice entries then the disk is considered to have
 *		no S-disk label.
 * Scope:	private
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object.
 * Return:	 0	- sdisk initialization completed successfully
 *		-1	- sdisk initialization failed
 */
static int
SdiskobjInitSim(Disk_t *dp)
{
	char *	cp;
	char **	ep;
	int	slice;
	int	n;
	int	start;
	int	size;
	int	over;
	int	count = 0;
	int	vtag;
	char *	use;
	int	realigned;

	/* validate parameters */
	if (dp == NULL)
		return (-1);

	/* find the beginning of the slice table in the input stream */
	while ((cp = InputGetLine()) != NULL) {
		if (strstr(cp, "Mount Directory") != NULL)
			break;
	}

	while ((cp = InputGetLine()) != NULL) {
		/* validate the input line as being a slice line */
		n = ParseBuffer(cp, &ep);
		if (n < 6 || n > 7) {
			InputSetReuse();
			break;
		}

		slice = atoi(ep[0]);
		if (!valid_sdisk_slice(slice)) {
			InputSetReuse();
			break;
		}

		realigned = FALSE;

		/*
		 * allow alternate sector slice lock override on fdisk systems
		 * if the existing size is > 0 in order to preserve alternate
		 * sectors situated in a different location from the current
		 * default
		 */
		if (SliceobjIsLocked(CFG_CURRENT, dp, slice) && slice != ALT_SLICE)
			continue;

		/* explicitly set the vtag field for slices 8 and 9 */
		if (slice == ALT_SLICE)
			vtag = V_ALTSCTR;
		else if (slice == BOOT_SLICE)
			vtag = V_BOOT;
		else
			vtag = atoi(ep[1]);

		/* adjust the starting cylinder to next cylinder boundary */
		start = blocks_to_cyls(dp, atoi(ep[3]));
		
		/*
		 * set the slice size with appropriate adjustments for
		 * cylinder rounding
		 */
		if (slice == ALT_SLICE && _disk_is_scsi(dp)) {
			size = 0;
		} else if ((over = atoi(ep[3]) % one_cyl(dp)) > 0) {
			size = atoi(ep[4]) - one_cyl(dp) + over;
			realigned = TRUE;
		} else {
			size = atoi(ep[4]);
		}

		/* only set the mount point name for slices with size > 0 */
		if (size == 0) {
			use = "";
		} else {
			switch (vtag) {
			    case V_ROOT:
				use = ROOT;
				break;
			    case V_SWAP:
				use = SWAP;
				break;
			    case V_BACKUP:
				use = OVERLAP;
				break;
			    case V_CACHE:
				use = CACHE;
				break;
			    case V_ALTSCTR:
				use = ALTSECTOR;
				break;
			    default:
				if (n == 7)
					use = ep[6];
				else
					use = "";
				break;
			}
		}

		(void) SliceobjSetAttributePriv(dp, slice,
			SLICEOBJ_USE,	    use,
			SLICEOBJ_INSTANCE,  VAL_UNSPECIFIED,
			SLICEOBJ_START,	    start,
			SLICEOBJ_SIZE,	    size,
			SLICEOBJ_REALIGNED, realigned,
			NULL);

		count++;
	}

	SdiskobjMarkOverlap(dp);
	if (count == 0)
		return (-1);

	return (0);
}
#endif /* MODULE_TEST */
