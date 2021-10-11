/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_GEN_VTOC_H
#define	_SYS_DKTP_GEN_VTOC_H

#pragma ident	"@(#)gen_vtoc.h	1.5	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	VTOC_SEC	29		/* VTOC sector number on disk */

/* Sanity word for the physical description area */
#define	VALID_PD		0xCA5E600D

struct gen_vtoc {
	unsigned long v_sanity;			/* to verify vtoc sanity */
	unsigned long v_version;		/* layout version */
	char v_volume[8];			/* volume name */
	ushort v_nparts;			/* number of partitions */
	ushort v_pad;				/* pad for 286 compiler */
	unsigned long v_reserved[10];		/* free space */
	struct partition v_part[16];		/* partition headers */
	time_t timestamp[16];			/* SCSI time stamp */
};


struct pdinfo	{
	unsigned long driveid;		/* identifies the device type */
	unsigned long sanity;		/* verifies device sanity */
	unsigned long version;		/* version number */
	char serial[12];		/* serial number of the device */
	unsigned long cyls;		/* number of cylinders per drive */
	unsigned long tracks;		/* number tracks per cylinder */
	unsigned long sectors;		/* number sectors per track */
	unsigned long bytes;		/* number of bytes per sector */
	unsigned long logicalst;	/* sector address of logical sector 0 */
	unsigned long errlogst;		/* sector address of error log area */
	unsigned long errlogsz;		/* size in bytes of error log area */
	unsigned long mfgst;		/* sector address of mfg. defect info */
	unsigned long mfgsz;		/* size in bytes of mfg. defect info */
	unsigned long defectst;		/* sector address of the defect map */
	unsigned long defectsz;		/* size in bytes of defect map */
	unsigned long relno;		/* number of relocation areas */
	unsigned long relst;		/* sector address of relocation area */
	unsigned long relsz;		/* size in sectors of relocation area */
	unsigned long relnext;		/* address of next avail reloc sector */
/*
 * the previous items are left intact from AT&T's 3b2 pdinfo.  Following
 * are added for the 80386 port
 */
	unsigned long vtoc_ptr;		/* byte offset of vtoc block */
	unsigned short vtoc_len;	/* byte length of vtoc block */
	unsigned short vtoc_pad;	/* pad for 16-bit machine alignment */
	unsigned long alt_ptr;		/* byte offset of alternates table */
	unsigned short alt_len;		/* byte length of alternates table */
		/* new in version 3 */
	unsigned long pcyls;		/* physical cylinders per drive */
	unsigned long ptracks;		/* physical tracks per cylinder */
	unsigned long psectors;		/* physical sectors per track */
	unsigned long pbytes;		/* physical bytes per sector */
	unsigned long secovhd;		/* sector overhead bytes per sector */
	unsigned short interleave;	/* interleave factor */
	unsigned short skew;		/* skew factor */
	unsigned long pad[8];		/* space for more stuff */
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_GEN_VTOC_H */
