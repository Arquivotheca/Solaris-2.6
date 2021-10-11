/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_SYS_SCSI_GENERIC_DAD_MODE_H
#define	_SYS_SCSI_GENERIC_DAD_MODE_H

#pragma ident	"@(#)dad_mode.h	1.12	96/06/07 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Structures and defines for DIRECT ACCESS mode sense/select operations
 */

/*
 * Direct Access Device mode header device specific byte definitions.
 *
 * On MODE SELECT operations, the effect of the state of the WP bit is unknown,
 * else reflects the Write-Protect status of the device.
 *
 * On MODE SELECT operations, the the DPOFUA bit is reserved and must
 * be zero, else on MODE SENSE operations it reflects whether or not
 * DPO and FUA bits are supported.
 */

#define	MODE_DAD_WP	0x80
#define	MODE_DAD_DPOFUA	0x10

/*
 * Direct Access Device Medium Types (for non direct-access magentic tapes)
 */

#define	DAD_MTYP_DFLT	0x0 /* default (currently mounted) type */

#define	DAD_MTYP_FLXSS	0x1 /* flexible disk, single side, unspec. media */
#define	DAD_MTYP_FLXDS	0x2 /* flexible disk, double side, unspec. media */

#define	DAD_MTYP_FLX_8SSSD 0x05	/* 8", single side, single density, 48tpi */
#define	DAD_MTYP_FLX_8DSSD 0x06	/* 8", double side, single density, 48tpi */
#define	DAD_MTYP_FLX_8SSDD 0x09	/* 8", single side, double density, 48tpi */
#define	DAD_MTYP_FLX_8DSDD 0x0A	/* 8", double side, double density, 48tpi */
#define	DAD_MTYP_FLX_5SSLD 0x0D	/* 5.25", single side, single density, 48tpi */
#define	DAD_MTYP_FLX_5DSMD1 0x12 /* 5.25", double side, medium density, 48tpi */
#define	DAD_MTYP_FLX_5DSMD2 0x16 /* 5.25", double side, medium density, 96tpi */
#define	DAD_MTYP_FLX_5DSQD 0x1A	/* 5.25", double side, quad density, 96tpi */
#define	DAD_MTYP_FLX_3DSLD 0x1E	/* 3.5", double side, low density, 135tpi */


/*
 * Direct Access device Mode Sense/Mode Select Defined pages
 */

#define	DAD_MODE_ERR_RECOV	0x01
#define	DAD_MODE_FORMAT		0x03
#define	DAD_MODE_GEOMETRY	0x04
#define	DAD_MODE_FLEXDISK	0x05
#define	DAD_MODE_VRFY_ERR_RECOV	0x07
#define	DAD_MODE_CACHE		0x08
#define	DAD_MODE_MEDIA_TYPES	0x0B
#define	DAD_MODE_NOTCHPART	0x0C
#define	DAD_MODE_POWER_COND	0x0D

/*
 * Definitions of selected pages
 */

/*
 * Page 0x1 - Error Recovery Parameters
 *
 * Note:	This structure is incompatible with previous SCSI
 *		implementations. See <scsi/impl/mode.h> for an
 *		alternative form of this structure. They can be
 *		distinguished by the length of data returned
 *		from a MODE SENSE command.
 */

#define	PAGELENGTH_DAD_MODE_ERR_RECOV	0x0A

struct mode_err_recov {
	struct	mode_page mode_page;	/* common mode page header */
#if defined(_BIT_FIELDS_LTOH)
	u_char		dcr	: 1,	/* disable correction */
			dte	: 1,	/* disable transfer on error */
			per	: 1,	/* post error */
			eec	: 1,	/* enable early correction */
			rc	: 1,	/* read continuous */
			tb	: 1,	/* transfer block */
			arre	: 1,	/* auto read realloc enabled */
			awre	: 1;	/* auto write realloc enabled */
#elif defined(_BIT_FIELDS_HTOL)
	u_char		awre	: 1,	/* auto write realloc enabled */
			arre	: 1,	/* auto read realloc enabled */
			tb	: 1,	/* transfer block */
			rc	: 1,	/* read continuous */
			eec	: 1,	/* enable early correction */
			per	: 1,	/* post error */
			dte	: 1,	/* disable transfer on error */
			dcr	: 1;	/* disable correction */
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif	/* _BIT_FIELDS_LTOH */
	u_char	read_retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	reserved;
	u_char	write_retry_count;
	u_char	reserved_2;
	u_short	recovery_time_limit;
};

/*
 * Page 0x3 - Direct Access Device Format Parameters
 */

struct mode_format {
	struct	mode_page mode_page;	/* common mode page header */
	u_short	tracks_per_zone;	/* Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_bytes_sect;	/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew;
	u_short	cylinder_skew;
#if defined(_BIT_FIELDS_LTOH)
	u_char			: 3,
		_reserved_ins	: 1,	/* see <scsi/impl/mode.h> */
			surf	: 1,
			rmb	: 1,
			hsec	: 1,
			ssec	: 1;	/* Drive Type Field */
#elif defined(_BIT_FIELDS_HTOL)
	u_char		ssec	: 1,	/* Drive Type Field */
			hsec	: 1,
			rmb	: 1,
			surf	: 1,
		_reserved_ins	: 1,	/* see <scsi/impl/mode.h> */
				: 3;
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif	/* _BIT_FIELDS_LTOH */
	u_char	reserved[2];
};

/*
 * Page 0x4 - Rigid Disk Drive Geometry Parameters
 */

struct mode_geometry {
	struct	mode_page mode_page;	/* common mode page header */
	u_char	cyl_ub;			/* number of cylinders */
	u_char	cyl_mb;
	u_char	cyl_lb;
	u_char	heads;			/* number of heads */
	u_char	precomp_cyl_ub;		/* cylinder to start precomp */
	u_char	precomp_cyl_mb;
	u_char	precomp_cyl_lb;
	u_char	current_cyl_ub;		/* cyl to start reduced current */
	u_char	current_cyl_mb;
	u_char	current_cyl_lb;
	u_short	step_rate;		/* drive step rate */
	u_char	landing_cyl_ub;		/* landing zone cylinder */
	u_char	landing_cyl_mb;
	u_char	landing_cyl_lb;
#if defined(_BIT_FIELDS_LTOH)
	u_char		rpl	: 2,	/* rotational position locking */
				: 6;
#elif defined(_BIT_FIELDS_HTOL)
	u_char			: 6,
			rpl	: 2;	/* rotational position locking */
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif	/* _BIT_FIELDS_LTOH */
	u_char	rotational_offset;	/* rotational offset */
	u_char	reserved;
	u_short	rpm;			/* rotations per minute */
	u_char	reserved2[2];
};

#define	RPL_SPINDLE_SLAVE		1
#define	RPL_SPINDLE_MASTER		2
#define	RPL_SPINDLE_MASTER_CONTROL	3

/*
 * Page 0x8 - Caching Page
 *
 * Note:	This structure is incompatible with previous SCSI
 *		implementations. See <scsi/impl/mode.h> for an
 *		alternative form of this structure. They can be
 *		distinguished by the length of data returned
 *		from a MODE SENSE command.
 */

#define	PAGELENGTH_DAD_MODE_CACHE_SCSI3	0x12

struct mode_cache_scsi3 {
	struct	mode_page mode_page;	/* common mode page header */
#if defined(_BIT_FIELDS_LTOH)
	u_char		rcd	: 1,	/* Read Cache Disable */
			mf	: 1,	/* Multiplication Factor */
			wce	: 1,	/* Write Cache Enable */
			size	: 1,	/* Size Enable */
			disc	: 1,	/* Discontinuity */
			cap	: 1,	/* Caching Analysis Permitted */
			abpf	: 1,	/* Abort Pre-Fetch */
			ic	: 1;	/* Initiator Control */
	u_char	write_reten_pri	: 4,	/* Write Retention Priority */
		read_reten_pri	: 4;	/* Demand Read Retention Priority */
#elif defined(_BIT_FIELDS_HTOL)
	u_char		ic	: 1,	/* Initiator Control */
			abpf	: 1,	/* Abort Pre-Fetch */
			cap	: 1,	/* Caching Analysis Permitted */
			disc	: 1,	/* Discontinuity */
			size	: 1,	/* Size Enable */
			wce	: 1,	/* Write Cache Enable */
			mf	: 1,	/* Multiplication Factor */
			rcd	: 1;	/* Read Cache Disable */
	u_char	read_reten_pri	: 4,	/* Demand Read Retention Priority */
		write_reten_pri	: 4;	/* Write Retention Priority */
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif	/* _BIT_FIELDS_LTOH */
	u_short	dis_prefetch_len;	/* Disable prefetch xfer length */
	u_short	min_prefetch;		/* minimum prefetch length */
	u_short	max_prefetch;		/* maximum prefetch length */
	u_short	prefetch_ceiling;	/* max prefetch ceiling */
#if defined(_BIT_FIELDS_LTOH)
	u_char			: 3,	/* reserved */
			vu_123	: 1,	/* Vendor Specific, byte 12 bit 3 */
			vu_124	: 1,	/* Vendor Specific, byte 12 bit 4 */
			dra	: 1,	/* Disable Read-Ahead */
			lbcss	: 1,	/* Logical Block Cache Segment Size */
			fsw	: 1;	/* Force Sequential Write */
#elif defined(_BIT_FIELDS_HTOL)
	u_char		fsw	: 1,	/* Force Sequential Write */
			lbcss	: 1,	/* Logical Block Cache Segment Size */
			dra	: 1,	/* Disable Read-Ahead */
			vu_124	: 1,	/* Vendor Specific, byte 12 bit 4 */
			vu_123	: 1,	/* Vendor Specific, byte 12 bit 3 */
				: 3;	/* reserved */
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif	/* _BIT_FIELDS_LTOH */
	u_char	num_cache_seg;		/* Number of cache segments */
	u_short	cache_seg_size;		/* Cache segment size */
	u_char	reserved;
	u_char	non_cache_seg_size_ub;	/* Non cache segment size */
	u_char	non_cache_seg_size_mb;
	u_char	non_cache_seg_size_lb;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SCSI_GENERIC_DAD_MODE_H */
