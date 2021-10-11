#ifndef lint
#pragma ident "@(#)pf_util.c 2.21 96/09/13"
#endif
/*
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmisvc_api.h"
#include "pf_strings.h"
#include "profile.h"

/* private prototypes */

static void	_print_sdisk_layout(Disk_t *);
static void	_print_fdisk_layout(Disk_t *);

/* ---------------------------- public functions ------------------------ */

/*
 * Function:	print_disk_layout
 * Description:	Print the disk layout for all disks. Selected disks are
 *		always reported. Unselected disks are also reported if we
 *		are running in simulation mode, or get_trace_level() > 1.
 * Scope:	public
 * Parameters:	none
 * Return:	none
 */
void
print_disk_layout(void)
{
	Disk_t	*dp;
	int	first = 1;
	char	*str;

	/* print selected disks */
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {
			/* print a message header if this is the first disk */
			if (first) {
				write_status(LOGSCR, LEVEL0,
					MSG0_DISK_LAYOUT_SELECTED);
				first = 0;
			}

			str = MSG0_DISK_TABLE_DISK;
			write_status(LOGSCR, LEVEL0, "    %s %s",
				str, disk_name(dp));

			/*
			 * print out the Fdisk configuration for disks which
			 * support fdisk
			 */
			if (disk_fdisk_req(dp))
				_print_fdisk_layout(dp);

			/*
			 * if Fdisk is supported and there is no Solaris
			 * partition we are done with this disk
			 */
			if (disk_fdisk_req(dp) &&
					get_solaris_part(dp, CFG_CURRENT) == 0)
				continue;

			/*
			 * print out the vtoc configuration
			 */
			_print_sdisk_layout(dp);
		}
	}

	/*
	 * if running in simulation mode or with active tracing, print out
	 * unselected disks
	 */
	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 1) {
		first = 1;
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp))
				continue;

			/* print a message header if this is the first disk */
			if (first) {
				write_status(LOGSCR, LEVEL0,
					MSG0_DISK_LAYOUT_UNSELECTED);
				first = 0;
			}

			str = MSG0_DISK_TABLE_DISK;
			write_status(LOGSCR, LEVEL0, "    %s %s",
				str, disk_name(dp));

			/*
			 * print out the Fdisk configuration for disks which
			 * support fdisk
			 */
			if (disk_fdisk_req(dp))
				_print_fdisk_layout(dp);

			/*
			 * if Fdisk is supported and there is no Solaris
			 * partition we are done with this disk
			 */
			if (disk_fdisk_req(dp) &&
					get_solaris_part(dp, CFG_CURRENT) == 0)
				continue;

			/*
			 * print out the vtoc configuration
			 */
			_print_sdisk_layout(dp);

			/* print spacers between disk records */
			if (first == 0 && next_disk(dp) != NULL)
				write_status(LOGSCR, LEVEL0, "");
		}
	}
}

/*
 * Function:	fatal_exit
 * Description:	Exit routine called when an irreconcilable error has occurred.
 *		If the caller provides a message, it is printed as an error
 *		notification. Stdout and stderr are flushed, and the process
 *		is terminated.
 * Scope:	public
 * Parameters:	msg	- message string format
 *		...	- message string parameters
 * Return:	none
 */
void
fatal_exit(char *msg, ...)
{
	va_list		ap;
	char		buf[256];

	if (msg != NULL && *msg != NULL) {
		buf[0] = '\0';
		va_start(ap, msg);
		(void) vsprintf(buf, msg, ap);
		va_end(ap);
		write_notice(ERRMSG, buf);
	}

	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0)
		write_status(SCR, LEVEL0, MSG1_EXIT_STATUS, 2);

	(void) fflush(stderr);
	(void) fflush(stdout);
	exit(2);
}

/*
 * Function:	print_space_layout
 * Description:	Print out software error message pertinent to the partitioning
 *		type.
 * Scope:	public
 * Parameters:	status	- space table returned by ResobjComplete() call
 * Return:	none
 */
void
/*ARGSUSED1*/
print_space_layout(Space *status, Label_t part_opt)
{
	ResobjHandle	res;
	SliceKey *	key;
	int		explicit;
	int		j = 0;
	long		minimum_total = 0;
	long		default_total = 0;
	int		minimum_size = 0;
	int		default_size = 0;
	int		actual_size = 0;
	int		n = percent_free_space();
	int		i;
	int		flag = 1;
	char		*str;
	int		expcount = 0;
	char		rname[MAXNAMELEN];
	int		rinstance;
	ResStat_t	rstatus;

	sw_lib_init(NULL);
	set_percent_free_space(DEFAULT_FS_FREE);

	/*
	 * print out the list of resources which didn't meet
	 * space requirements
	 */
	for (j = 0; status[j].name[0] != '\0'; j++) {
		flag = 0;
		if (status[j].allocated == 0 &&
				!streq(status[j].name, SWAP)) {
			write_status(LOGSCR, LEVEL1|LISTITEM|FMTPARTIAL,
				MSG_STD_ERROR);
			write_status(LOGSCR, LEVEL0|CONTINUE,
				MSG1_SPACE_NOT_FIT,
				status[j].name);
		} else {
			write_status(LOGSCR, LEVEL1|LISTITEM|FMTPARTIAL,
				MSG_STD_ERROR);
			write_status(LOGSCR, LEVEL0|CONTINUE,
				MSG1_SPACE_TOO_SMALL,
				status[j].name);
		}
	}

	/*
	 * print out the actual/minimum/required space table
	 */
	if (flag == 0) {
		write_status(LOGSCR, LEVEL0, "");

		/* print the table header centered for I18N */
		/* i18n: 34 character maximum */
		str = MSG0_SPACE_REQUIREMENTS;
		i = ((36 - (int) strlen(str) + 1) / 2) + 27;
		write_status(LOGSCR, LEVEL1,
			"%*s%s", i, " ", str);

		/* i18n: 25 character maximum */
		str = MSG0_SPACE_DIRECTORY;
		write_status(LOGSCR, LEVEL1|FMTPARTIAL,
			"%-25.25s", str);

		/* i18n: 9 character maximum */
		str = MSG0_SPACE_ACTUAL;
		write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%11.9s", str);

		/* i18n: 9 character maximum */
		str = MSG0_SPACE_REQUIRED;
		write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%12.9s", str);

		/* i18n: 9 character maximum */
		str = MSG0_SPACE_DEFAULT;
		write_status(LOGSCR, LEVEL0|CONTINUE, "%12.9s", str);

		write_status(LOGSCR, LEVEL1,
		"-------------------------  ----------  ----------  ----------");

		WALK_DIRECTORY_LIST(res) {
			if (ResobjGetAttribute(res,
					RESOBJ_NAME,	 rname,
					RESOBJ_INSTANCE, &rinstance,
					RESOBJ_STATUS,   &rstatus,
					NULL) != D_OK)
				continue;

			if (rstatus != RESSTAT_INDEPENDENT)
				continue;

			actual_size = 0;
			explicit = 0;

			if ((key = SliceobjFindUse(CFG_CURRENT, NULL,
					rname, rinstance, FALSE)) != NULL) {
				actual_size = Sliceobj_Size(CFG_CURRENT,
						key->dp, key->slice);
				if (SliceobjIsExplicit(CFG_CURRENT,
						key->dp, key->slice) ||
						SliceobjIsPreserved(CFG_CURRENT,
							key->dp, key->slice))
					explicit = 1;
					expcount++;
			}

			/*
			 * obtain both the minimum and default sizes
			 */
			minimum_size = ResobjGetContent(res, ADOPT_ALL,
					RESSIZE_MINIMUM);
			default_size = ResobjGetContent(res, ADOPT_ALL,
					RESSIZE_DEFAULT);

			/* convert the values from sectors to megabytes */
			if (actual_size < default_size)
				actual_size = sectors_to_mb_trunc(actual_size);
			else
				actual_size = sectors_to_mb(actual_size);

			minimum_size = sectors_to_mb(minimum_size);
			default_size = sectors_to_mb(default_size);

			if (default_size > 0) {
				if (actual_size > 0) {
					write_status(SCR, LEVEL1,
						"%-25.25s%11d%c%11d%12d",
						rname,
						actual_size,
						(explicit == 1 ? '*' : ' '),
						minimum_size,
						default_size);
				} else {
					write_status(SCR, LEVEL1,
						"%-25.25s%11s%c%11d%12d",
						rname,
						"",
						' ',
						minimum_size,
						default_size);
				}

				minimum_total += minimum_size;
				default_total += default_size;
			}
		}

		actual_size = SliceobjSumSwap(NULL, SWAPALLOC_ALL);
		minimum_size = ResobjGetSwap(RESSIZE_MINIMUM);
		default_size = ResobjGetSwap(RESSIZE_DEFAULT);

		/* convert the values from sectors to megabytes */
		if (actual_size < default_size)
			actual_size = sectors_to_mb_trunc(actual_size);
		else
			actual_size = sectors_to_mb(actual_size);

		minimum_size = sectors_to_mb(minimum_size);
		default_size = sectors_to_mb(default_size);

		if (actual_size > 0) {
			write_status(SCR, LEVEL1,
				"%-25.25s%11d%c%11d%12d",
				SWAP,
				actual_size,
				' ',
				minimum_size,
				default_size);
		} else {
			write_status(SCR, LEVEL1,
				"%-25.25s%11s%c%11d%12d",
				SWAP,
				"",
				' ',
				minimum_size,
				default_size);
		}
		minimum_total += minimum_size;
		default_total += default_size;

		write_status(LOGSCR, LEVEL0, "");
		/* i18n: 25 character maximum */
		str = MSG0_SPACE_TOTAL;
		write_status(LOGSCR, LEVEL1,
			"%25.25s%12s%11d%12d",
			str,
			"",
			minimum_total,
			default_total);
	}

	write_status(LOGSCR, LEVEL0, "");

	if (expcount > 0) {
		/* i18n: 10 character maximum */
		str = MSG0_SPACE_NOTE;
		write_status(LOGSCR, LEVEL1|FMTPARTIAL,
			"%-10.10s", str);

		/* i18n: 50 character maximum */
		str = MSG0_SPACE_NOTE_LINE1;
		write_status(LOGSCR, LEVEL0|CONTINUE,
			"%-52.50s", str);

		/* i18n: 50 character maximum */
		str = MSG0_SPACE_NOTE_LINE2;
		write_status(LOGSCR, LEVEL1,
			"%10.10s%-52.50s", " ", str);

		/* i18n: 50 character maximum */
		str = MSG0_SPACE_NOTE_LINE3;
		write_status(LOGSCR, LEVEL1,
			"%10.10s%-52.50s", " ", str);
	}

	sw_lib_init(NULL);
	if (n == 0)
		set_percent_free_space(NO_EXTRA_SPACE);
	else
		set_percent_free_space(n);
}

/* ----------------------------- private functions ---------------------- */

/*
 * Function:	_print_sdisk_layout
 * Description:	Print out the slice configuration.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	none
 */
static void
_print_sdisk_layout(Disk_t *dp)
{
	int	i;
	char	*str;

	/* validate parameters */
	if (dp == NULL)
		return;

	/* print out table header */
	str = MSG0_DISK_TABLE_HEADER;
	i = (60 - (int)strlen(str) + 1) / 2;
	write_status(LOGSCR, LEVEL1, "%*s%s", i, " ", str);

	/*
	 * only print a layout if there is at least one slice
	 * which has either a defined size of mount point
	 */
	WALK_SLICES_STD(i) {
		if (slice_size(dp, i) ||
				slice_mntpnt_exists(dp, i))
			break;
	}

	if (i > LAST_STDSLICE)
		return;

	/* print out table header (must be I18N aligned) */
	/* i18n: 8 characters maximum */
	str = MSG0_DISK_TABLE_SLICE;
	write_status(LOGSCR, LEVEL1|FMTPARTIAL, "%7.5s", str);

	/* i18n: 8 characters maximum */
	str = MSG0_DISK_TABLE_START;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%10.8s", str);

	/* i18n: 8 characters maximum */
	str = MSG0_DISK_TABLE_CYLS;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%10.8s", str);

	/* i18n: 4 characters maximum */
	str = MSG0_DISK_TABLE_MB;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%6.4s", str);

	/* i18n: 8 characters maximum */
	str = MSG0_DISK_TABLE_PRESERVED;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%10.8s", str);

	/* i18n: 25 characters maximum */
	str = MSG0_DISK_TABLE_MOUNT;
	write_status(LOGSCR, LEVEL0|CONTINUE, "  %-25.25s", str);

	write_status(LOGSCR, LEVEL1,
	"-------  --------  --------  ----  --------  ----------");

	/* print slices */
	if (get_trace_level() > 0) {
		WALK_SLICES(i) {
			if (slice_size(dp, i) == 0)
				continue;
			write_status(LOGSCR, LEVEL1|CONTINUE,
				"%7d%10d%10d%6d%10.8s  %-25.25s",
				i,
				slice_start(dp, i),
				blocks_to_cyls(dp, slice_size(dp, i)),
				blocks_to_mb(dp, slice_size(dp, i)),
				(slice_preserved(dp, i) ?
					MSG_STD_YES : MSG_STD_NO),
				slice_mntpnt(dp, i));
		}
	} else {
		WALK_SLICES_STD(i) {
			if (slice_size(dp, i) == 0)
				continue;
			write_status(LOGSCR, LEVEL1|CONTINUE,
				"%7d%10d%10d%6d%10.8s  %-25.25s",
				i,
				slice_start(dp, i),
				blocks_to_cyls(dp, slice_size(dp, i)),
				blocks_to_mb(dp, slice_size(dp, i)),
				(slice_preserved(dp, i) ?
					MSG_STD_YES : MSG_STD_NO),
				slice_mntpnt(dp, i));
		}
	}

	/*
	 * print Solaris statistics
	 */
	write_status(LOGSCR, LEVEL0|CONTINUE, "");
	write_status(LOGSCR, LEVEL1,
		MSG3_DISK_TABLE_STATISTICS,
		blocks_to_mb(dp, usable_sdisk_blks(dp)),
		usable_sdisk_cyls(dp),
		blocks_to_mb(dp, sdisk_space_avail(dp)));

}

/*
 * Function:	_print_fdisk_layout
 * Description:	Print out the fdisk partition table for simulation mode.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	none
 */
static void
_print_fdisk_layout(Disk_t *dp)
{
	int	p;
	int	c;
	char	*str;
	int	i;

	/*
	 * if there is no disk object, or there is no Fdisk data
	 * on the disk object, then we're done
	 */
	if (dp == NULL || disk_no_fdisk_req(dp))
		return;

	/* print out table header */
	str = MSG0_FDISK_TABLE_HEADER;
	i = (40 - (int)strlen(str) + 1) / 2;
	write_status(LOGSCR, LEVEL1, "%*s%s", i, " ", str);

	/* i18n: 15 character maximum */
	str = MSG0_FDISK_TABLE_PARTITION;
	write_status(LOGSCR, LEVEL1|FMTPARTIAL, "%11s", str);

	/* i18n: 10 character maximum */
	str = MSG0_FDISK_TABLE_OFFSET;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%10.8s", str);

	/* i18n: 4 character maximum */
	str = MSG0_FDISK_TABLE_MB;
	write_status(LOGSCR, LEVEL0|CONTINUE|FMTPARTIAL, "%6.4s", str);

	/* i18n: 15 character maximum */
	str = MSG0_FDISK_TABLE_TYPE;
	write_status(LOGSCR, LEVEL0|CONTINUE, "%8.6s", str);

	write_status(LOGSCR, LEVEL1|CONTINUE,
		"-----------  --------  ----  ------");

	/*
	 * print out entries for each partition in their original
	 * partition order (must be I18N aligned)
	 */
	WALK_PARTITIONS(c) {
		WALK_PARTITIONS(p) {
			if (part_orig_partnum(dp, p) == c) {
				write_status(LOGSCR, LEVEL1,
					"%11d%10d%6d%8d",
					c,
					part_startsect(dp, p),
					blocks_to_mb(dp, part_size(dp, p)),
					part_id(dp, p));
				break;
			}
		}
	}

	write_status(LOGSCR, LEVEL0, "");
}
