#ifndef lint
#pragma ident "@(#)v_fdisk.c 1.27 96/10/09 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc. All rights reserved.
 */
/*
 * Module:	v_fdisk.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "spmistore_api.h"
#include "spmisvc_api.h"

#include "v_types.h"
#include "v_disk.h"
#include "v_disk_private.h"

/*
 * This file contains the View interface layer to the `fdisk' functionality
 * in the underlying install disk library.
 */

/*
 * how may physical partitions are supported on an fdisk label.
 */
#ifdef FD_NUMPART
int N_Partitions = FD_NUMPART;

#else
int N_Partitions = 5;

#endif

/*
 * provide UI a mechanism for knowing the number of different fdisk
 * partition types... per SunSoftSouth, there are 4 partition types exposed
 * to the user: <unused>, Solaris, DOS, and Other.
 *
 * N_PartitionTypes = 10
 */
int N_PartitionTypes = 5;

/*
 * int v_boot_disk_selected(void):
 *
 * determines if the *required* boot disk for i386 systems is selected.
 * SPARC and PowerPC systems can boot from any disk any are no impacted
 * by this constraint.
 *
 * returns 0 if not selected or no boot drive returns non-zero if bootdrive is
 * selected
 */
int
v_boot_disk_selected(void)
{
	Disk_t *tmp;

	if (IsIsa("i386")) {
		if (DiskobjFindBoot(CFG_CURRENT, &tmp) != D_OK || tmp == NULL)
			return (0);

		return (disk_selected(tmp));
	}

	return (1);
}

/*
 * int v_fdisk_get_space_avail(int disk):
 *
 * get `unused' space in disk's fdisk partitions
 *
 * return size in terms of current units (e.g., MB or Cyls), returns 0 on
 * error.
 */
int
v_fdisk_get_space_avail(int disk)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_disks[disk].info,
		fdisk_space_avail(_disks[disk].info), ROUNDDOWN));

}

/*
 * int v_fdisk_flabel_req(int disk):
 *
 * Boolean function, does disk require an fdisk label?
 * (a/k/a - Is this an X86 box?)
 *
 * XXX this may be a mis-interpretation of the disk_fdisk_req() function. It
 * may not really describe if an fdisk is needed, but rather if the disk has
 * one or does not...
 *
 */
int
v_fdisk_flabel_req(int disk)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (disk_fdisk_req(_disks[disk].info));

}

/*
 * int v_fdisk_flabel_exist(int disk):
 *
 * Boolean function: does this disk have an fdisk label?
 *
 * returns 0 if no fdisk label
 * non-zero if there is one
 */
int
v_fdisk_flabel_exist(int disk)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (fdisk_no_flabel(_disks[disk].info) == 0);

}

/*
 * int v_fdisk_set_default_flabel(int disk)
 *
 * Puts default fdisk label onto disk drive.
 *
 * returns 0 on error
 * returns 1 if succesful
 */
int
v_fdisk_set_default_flabel(int disk)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	(void) FdiskobjConfig(LAYOUT_DEFAULT, _disks[disk].info, "");
	(void) commit_disk_config(_disks[disk].info);

	return (1);

}

/*
 * int v_fdisk_flabel_has_spart(int disk)
 *
 * Boolean function, Does disk's fdisk label have a Solaris partition?
 *
 * returns 0 if not
 * returns 1 if does
 *
 */
int
v_fdisk_flabel_has_spart(int disk)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (get_solaris_part(_disks[disk].info, CFG_CURRENT));

}

/*
 * couple of statics for remembering/dealing with creating a default Solaris
 * partition.
 *
 * Need to know where largest 'free' partition is.. so go find the holes, and
 * remember where they are and their sizes.
 *
 */

static struct {
	u_int size;
	int inuse;
} parts[FD_NUMPART];

static int lrg_part;		/* largest `free' partition index */

/*
 * int v_fdisk_get_part_maxsize(int part)
 *
 * computes and returns largest possible size for partition 'part' (`part' is a
 * relative index, (0 - 3) and must be mapped into actual part number if
 * passed into disk lib.)
 *
 * return size in terms of current units (e.g., MB or Cyls)
 *
 */
int
v_fdisk_get_part_maxsize(int part)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	/*
	 * compute all free partition's max sizes
	 */
	(void) v_fdisk_get_max_partsize_free(_current_disk_index);

	/*
	 * partition is in use, max size for it is it's current size plus
	 * any free space around it.
	 *
	 * if partition is not in use, then it's max size is already known. if
	 * (part_id(_current_disk, FD_PART_NUM(part)) != UNUSED) { maxsize =
	 * part_size(_current_disk, FD_PART_NUM(part)); if (part != 0)
	 * maxsize += parts[part-1].size; if (part+1 < N_Partitions) maxsize
	 * += parts[part+1].size; } else maxsize = parts[part].size;
	 */

	/*
	 * return partition's maximum size
	 */
	return (blocks2size(_current_disk, parts[part].size, ROUNDDOWN));

}

/*
 * int v_fdisk_get_max_partsize(int disk)
 *
 * compute and return largest possible partition size on this fdisk label
 * regardless of partitions is use.
 *
 * return size in terms of current units (e.g., MB or Cyls)
 *
 */
int
v_fdisk_get_max_partsize(int disk)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_disks[disk].info,
		usable_disk_blks(_disks[disk].info), ROUNDDOWN));

}

/*
 * int v_fdisk_set_solaris_max_partsize(int disk)
 *
 * creates a solaris partition spanning entire disk,
 * regardless of partitions in use.
 *
 * return 1 if successful
 * return 0 if error
 *
 */
int
v_fdisk_set_solaris_max_partsize(int i)
{

	if (_num_disks == -1 && i == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	/*
	 * reset the fdisk partition table and layout the largest
	 * Solaris partition possible
	 */
	(void) FdiskobjConfig(LAYOUT_RESET, _disks[i].info, "");
	(void) FdiskobjConfig(LAYOUT_DEFAULT, _disks[i].info, "");
	(void) validate_fdisk(_disks[i].info);
	(void) commit_disk_config(_disks[i].info);
	_disks[i].status = V_DISK_EDITED;

	return (1);
}

/*
 * int v_fdisk_get_max_partsize_free(int disk)
 *
 * Get largest possible partition size on disk's fdisk label given
 * existing partitions (e.g., largest free chunk before, between, or after)
 *
 * return 0 if no space, or no free partition
 * return size in current units (MB or Cyls)
 */
int
v_fdisk_get_max_partsize_free(int disk)
{
	int j;
	int maxholesize = 0;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	/* get info on partitions */
	lrg_part = -1;
	for (j = 0; j < N_Partitions; j++) {

		if (part_id(_disks[disk].info, FD_PART_NUM(j)) == UNUSED) {

			parts[j].inuse = 0;
			parts[j].size = max_size_part_hole(_disks[disk].info,
			    FD_PART_NUM(j));

			if (parts[j].size > maxholesize) {
				lrg_part = j;	/* remember this part # */
				maxholesize = parts[j].size;
			}
		} else {
			parts[j].inuse = 1;
			parts[j].size = max_size_part_hole(_disks[disk].info,
			    FD_PART_NUM(j));

		}
	}

	/*
	 * should either have a partition index representing the largest
	 * free hole, or no holes at all.  Which is it?
	 */
	if (lrg_part == -1) {
		return (0);	/* no free partition */
	} else {
		return (blocks2size(_disks[disk].info, parts[lrg_part].size,
			ROUNDUP));
	}

}

/*
 * int v_fdisk_set_solaris_free_partsize(int disk)
 *
 * create a solaris partition in the largest free chunk of the fdisk
 *
 */
int
v_fdisk_set_solaris_free_partsize(int disk)
{
	int part;

	if (_num_disks == -1 && disk == -1)
		(void) v_init_disks();

	if (disk > N_Partitions || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	/*
	 * first, preserve any existing partitions.
	 */
	for (part = 0; part < N_Partitions; part++) {
		if (parts[part].inuse == 1) {

			/*
			 * check errors?  This should *not* fail
			 */
			(void) set_part_preserve(_disks[disk].info,
			    FD_PART_NUM(part), PRES_YES);
		}
	}

	/*
	 * now make largest partition, Solaris partition
	 */
	if (lrg_part != -1) {

		/* set size */
		if ((v_errno = set_part_geom(_disks[disk].info,
			FD_PART_NUM(lrg_part), GEOM_IGNORE,
			parts[lrg_part].size))
		    != D_OK)
			return (0);

		/* set type to Solaris & make it active */
		(void) set_part_attr(_disks[disk].info, FD_PART_NUM(lrg_part),
		    SUNIXOS, GEOM_IGNORE);

		/* slide partition's start cyl into hole */
		if ((v_errno = adjust_part_starts(_disks[disk].info)) != D_OK)
			return (0);
	}
	(void) validate_fdisk(_disks[disk].info);
	(void) commit_disk_config(_disks[disk].info);
	_disks[disk].status = V_DISK_EDITED;

	return (1);
}

/*
 * routines to get/set partition's `active' status, partition number is
 * relative to the current disk.
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual part
 * number when passed into disk lib.)
 *
 */
int
v_fdisk_part_is_active(int part)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	return (part_is_active(_current_disk, FD_PART_NUM(part)));

}

int
v_fdisk_set_active_part(int part)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	if (v_errno = set_part_attr(_current_disk, FD_PART_NUM(part),
		GEOM_IGNORE, GEOM_IGNORE))
		return (V_FAILURE);

	return (V_OK);
}

/*
 * get number of partition types
 */
int
v_fdisk_get_n_part_types()
{
	return (N_PartitionTypes);
}

/*
 * routines to get/set partition's type, partition
 * number is relative to the current disk.
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual
 *  part number when passed into disk lib.)
 *
 * Per SunSoftSouth, there are 4 supported parititon types:
 *	Solaris (SUNIXOS),
 *	PRI DOS (DOSHUGE, DOSOS12, DOSOS16),
 *	EXT DOS (DOSEXT),
 *	<unused> (UNUSED),
 *	Other (all others).
 *
 */
V_DiskPart_t
v_fdisk_get_part_type(int part)
{
	V_DiskPart_t ret;

	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_UNUSED);

	if (_current_disk == (Disk_t *) NULL)
		return (V_UNUSED);

	switch (part_id(_current_disk, FD_PART_NUM(part))) {

	case SUNIXOS:
		ret = V_SUNIXOS;
		break;

	case DOSHUGE:
	case DOSOS12:
	case DOSOS16:
		ret = V_DOSPRIMARY;
		break;

	case EXTDOS:
		ret = V_DOSEXT;
		break;

	case UNUSED:
		ret = V_UNUSED;
		break;

	default:
		ret = V_OTHER;
		break;
	}

	return (ret);

}

/*
 * get partitions type by index.  (eg: what is the type of the 6th one?)
 *
 * ordering of partition types in the next two routines is
 * important and must be maintained consistently;
 *
 * Per SunSoftSouth, there are 4 supported parititon types:
 *	Solaris (SUNIXOS),
 *	PRI DOS (DOSHUGE, DOSOS12, DOSOS16),
 *	EXT DOS (DOSEXT),
 *	<unused> (UNUSED),
 *	Other (all others)
 */
V_DiskPart_t
v_fdisk_get_type_by_index(int type)
{
	V_DiskPart_t ret;

	switch (type) {
	case 0:
		ret = V_SUNIXOS;
		break;

	case 1:
		ret = V_DOSPRIMARY;
		break;

	case 2:
		ret = V_DOSEXT;
		break;

	case 3:
		ret = V_UNUSED;
		break;

	case 4:
		ret = V_OTHER;
		break;

	default:
		ret = V_OTHER;
		break;

	}

	return (ret);
}

/*
 * get partition type as a string, (eg: what is the name of the 6th type?)
 */
char *
v_fdisk_get_type_str_by_index(int i)
{
	char *cp = (char *) NULL;

	switch (i) {
	case 0:
		cp = "SOLARIS";
		break;

	case 1:
		cp = "PRI DOS";
		break;

	case 2:
		cp = "EXT DOS";
		break;

	case 3:
		cp = "<unused>";
		break;

	case 4:
		cp = "Other";
		break;

	default:
		cp = "Other";
		break;
	}

	return (cp);

}

/*
 * get partition's type as a string (what is the name of partition's part
 * type?)
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual part
 * number when passed into disk lib.)
 *
 */
char *
v_fdisk_get_part_type_str(int part)
{
	char *cp;

	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return ("");

	if (_current_disk == (Disk_t *) NULL)
		return ("");

	switch (part_id(_current_disk, FD_PART_NUM(part))) {
	case SUNIXOS:
		cp = "SOLARIS";
		break;

	case DOSHUGE:
	case DOSOS12:
	case DOSOS16:
		cp = "PRI DOS";
		break;

	case EXTDOS:
		cp = "EXT DOS";
		break;

	case UNUSED:
		cp = "<unused>";
		break;

	default:
		cp = "Other";
		break;
	}

	return (cp);

}

/*
 * set partition's type to type.
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual part
 * number when passed into disk lib.)
 */
int
v_fdisk_set_part_type(int part, V_DiskPart_t type)
{
	int parttype;

	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	switch (type) {

	case V_SUNIXOS:
		parttype = SUNIXOS;
		break;

	case V_DOSPRIMARY:
		parttype = DOSHUGE;
		break;

	case V_UNUSED:
		parttype = UNUSED;
		break;

	default:
		parttype = UNUSED;
		break;
	}

	if (v_errno = set_part_attr(_current_disk, FD_PART_NUM(part),
		parttype, GEOM_IGNORE))
		return (V_FAILURE);

	return (V_OK);
}


/*
 * routines to get/set part size, on current disk
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual part
 * number when passed into disk lib.)
 *
 */
int
v_fdisk_get_part_size(int part)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_current_disk,
		part_size(_current_disk, FD_PART_NUM(part)), ROUNDDOWN));

}

int
v_fdisk_set_part_size(int part, int size)
{
	int osize;

	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	osize = part_size(_current_disk, FD_PART_NUM(part));

	v_errno = set_part_geom(_current_disk, FD_PART_NUM(part), GEOM_IGNORE,
	    size2blocks(_current_disk, size));

	if (v_errno != D_OK)
		return (V_FAILURE);
	else {

		if (size != 0) {

			if (osize == 0) {
				/* move new partition into hole */
				part_stuck_off(_current_disk,
					FD_PART_NUM(part));
					adjust_part_starts(_current_disk);
			}
		} else {

			/*
			 * partition's starting cylinder is `unstuck' until
			 * partition has a size > 0
			 */
			part_stuck_off(_current_disk, FD_PART_NUM(part));
		}

		return (V_OK);
	}
}

/*
 * routines to get/set starting cylinder or sector for i'th partition
 *
 * (`part' is a relative index, (0 - 3) and must be mapped into actual part
 * number when passed into disk lib.)
 *
 */
int
v_fdisk_get_part_startsect(int part)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

/*
	This now uses HBA geometry!!!!
*/
	return ((part_startsect(_current_disk, FD_PART_NUM(part)) +
		disk_geom_hbacyl(_current_disk) - 1) /
		disk_geom_hbacyl(_current_disk));
}

int
v_fdisk_set_part_startcyl(int part, int cyl)
{
	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	v_errno = set_part_geom(_current_disk, FD_PART_NUM(part), cyl,
	    GEOM_IGNORE);

	if (v_errno != D_OK)
		return (V_FAILURE);
	else {
		adjust_part_starts(_current_disk);
		return (V_OK);
	}
}

/*
 * returns ending cylinder for partition... for display only
 */
int
v_fdisk_get_part_endcyl(int part)
{

	if (FD_PART_NUM(part) > N_Partitions || FD_PART_NUM(part) < 1)
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (blocks_to_cyls(_current_disk,
		(part_startsect(_current_disk, FD_PART_NUM(part)) +
		    part_size(_current_disk, FD_PART_NUM(part)))));
}

/*
 * convert to/from units, dependent on disk geometry, so need to know which
 * disk.
 */
int
v_cyls_to_mb(int disk, int cyls)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (blocks_to_mb(_disks[disk].info,
		cyls_to_blocks(_disks[disk].info, cyls)));
}

int
v_mb_to_cyls(int disk, int mb)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (0);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (0);

	return (blocks_to_cyls(_disks[disk].info,
		mb_to_blocks(_disks[disk].info, mb)));
}

/*
 * v_fdisk_get_err_buf()
 *
 * Validates that the F-Disk configuration in the CURRENT state is sane.  The
 * S-disk pointer is established if the F-disk is legal and there is a valid
 * Solaris partition.
 *
 * also make sure that the Solaris partition is non-zero.
 */
int
v_fdisk_validate(int disk)
{
	int part;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (V_OK);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (V_OK);

	/*
	 * is FDISK OK?
	 */
	if ((v_errno = validate_fdisk(_disks[disk].info)) != D_OK) {

		if (v_errno == D_OVER || v_errno == D_BADORDER ||
		    v_errno == D_NOSOLARIS || v_errno == D_OUTOFREACH)
			return (V_FAILURE);
		else
			return (V_OK);

	}
	/*
	 * is there a Solaris partition?
	 */
	part = get_solaris_part(_disks[disk].info, CFG_CURRENT);
	if (part == 0) {
		v_errno = D_NOSOLARIS;
		return (V_FAILURE);
	}
	/*
	 * is the Solaris partition `big enough'?
	 */
	if (v_fdisk_get_part_size(FD_PART_IDX(part)) <
	    blocks2size(_disks[disk].info, MINFSSIZE, ROUNDUP)) {
		v_errno = V_TOOSMALL;
		return (V_FAILURE);
	}
	return (V_OK);

}

/*
 * v_fdisk_get_err_buf()
 *
 * function: this is only called in response to a v_fdisk_validate() call which
 * produced a non-negative error code.  We want to create a reasonably
 * detailed error message containing the disk library's abbreviated message.
 * Also want to include some ideas for how to resolve the problem. returns:
 * current disk error text contained in buf.
 */
char *
v_fdisk_get_err_buf(void)
{
	static char buf[BUFSIZ];
	static char tmp[BUFSIZ];
	int	total, correct, part_num;

#define	FDISK_ERROR_OVERLAP	gettext(\
	"This FDISK configuration is invalid and cannot be used in its "\
	"current form.  There appear to be two partitions which overlap.\n\n"\
	"Partitions may not occupy the same physical cylinders.  You can "\
	"resolve this problem by deleting one of these partitions and "\
	"recreating it so that it no longer overlaps.")

#define	FDISK_ERROR_BADORDER	gettext(\
	"This FDISK configuration is invalid and cannot be used in its "\
	"current form.  There appear to be two partitions which are out of "\
	"order.\n\nThis means that a higher numbered partition (such as 2) "\
	"has beginning and ending cylinders which come before a lower "\
	"numbered partition (such as 1).")

/*
 * #define	FDISK_ERROR_OUTOFREACH	gettext(\
 *	"This FDISK configuration is invalid. Root (/) and other critical "\
 *	"slices must end before the first 1023 cylinders of the disk. To "\
 *	"fix the problem, change the Solaris fdisk partition so the root "\
 *	"slice ends before physical disk cylinder 1023.")
 */
#define	FDISK_ERROR_OUTOFREACH gettext(\
	"The root (/) slice of the Solaris partition must end before the " \
	"first 1023 cylinders of the disk.\n\n" \
	"> If there is an EXT DOS and/or Other partition listed above \n " \
	"   the Solaris partition on the screen, delete one or both \n " \
	"   partitions.\n\n" \
	"> If there is a PRI DOS partition listed above the Solaris \n " \
	"   partition on the screen, reduce the size of PRI DOS by %d MB \n " \
	"   and continue.")

#define	FDISK_ERROR_TOOSMALL	gettext(\
	"This FDISK configuration is invalid and cannot be used in its "\
	"current form.  The Solaris partition which is configured is not "\
	"not large enough to be used.  The Solaris partition must be at "\
	"least 10MB in size and should really be at least 200MB.")

#define	FDISK_ERROR_UNKNOWN	gettext(\
	"Unknown error during fdisk Validation(This message must be changed).")

#define	FDISK_ERROR_NOSOLARIS	gettext(\
	"This FDISK configuration is invalid and cannot be used in its "\
	"current form.  There is no Solaris partition.")

	(void) memset(buf, '\0', BUFSIZ);
	switch (v_get_v_errno()) {
	case V_OVER:
		(void) sprintf(buf, "%s", FDISK_ERROR_OVERLAP);
		break;

	case V_BADORDER:
		(void) sprintf(buf, "%s", FDISK_ERROR_BADORDER);
		break;

	case V_NOSOLARIS:
		(void) sprintf(buf, "%s", FDISK_ERROR_NOSOLARIS);
		break;

	case V_OUTOFREACH:
		part_num = get_solaris_part(_current_disk, CFG_CURRENT);
		total = slice_size(_current_disk, BOOT_SLICE) +
				slice_size(_current_disk, ALT_SLICE) +
				get_default_fs_size(ROOT, NULL, DONTROLLUP);
		correct = part_startsect(_current_disk, part_num) -
			(1023 * disk_geom_hbacyl(_current_disk)) + total;
		correct = (correct + 2047) / 2048;
		(void) sprintf(tmp, FDISK_ERROR_OUTOFREACH, correct);
		(void) sprintf(buf, "%s", tmp);
		break;

	case V_TOOSMALL:
		(void) sprintf(buf, "%s", FDISK_ERROR_TOOSMALL);
		break;

	default:
		(void) sprintf(buf, "%s", FDISK_ERROR_UNKNOWN);
		break;
	}

	return (buf);

}
