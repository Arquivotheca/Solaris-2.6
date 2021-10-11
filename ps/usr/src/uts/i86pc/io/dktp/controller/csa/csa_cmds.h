/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_CMDS_H
#define	_CSA_CSA_CMDS_H

#pragma	ident	"@(#)csa_cmds.h	1.2	95/06/06 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Drive Array Controller Command codes
 */
#define	CSA_ID_LDRIVE		0x10	/* identify logical drive */
#define	CSA_ID_CTLR		0x11	/* identify controller */
#define	CSA_ID_LDSTATUS		0x12	/* identify logical drive status */
#define	CSA_START_RECOV		0x13	/* start recover */
#define	CSA_READ_SECTOR		0x20	/* read one or more sectors */
#define	CSA_WRITE_SECTOR	0x30	/* write one or more sectors */
#define	CSA_DIAG_MODE		0x40	/* go to diagnostic mode */
#define	CSA_SENSE_CONFIG	0x50	/* sense config of controller */
#define	CSA_SET_CONFIG		0x51	/* set config of controller */
#define	CSA_SET_CACHE_CONFIG	0xc0	/* set cache configuration */
#define	CSA_FLUSH_CACHE		0xc2	/* flush/disable posted-write cache */

#define	BYTE	unchar
#define	WORD	ushort
#define	DOUBLE	ulong


/*
 * Need to "pack" the following structure declarations because some of
 * them "unaligned" members.
 */
#pragma pack(1)




/*
 * 0x11: Identify Controller
 *	This command is used to determine various information about the
 *	controller itself and the number of configured logical drives.
 */

struct identify_controller {
	BYTE	configured_logical_drives;
	DOUBLE	configuration_signature;
	BYTE	ascii_firmware_revision[4];
	BYTE	rom_firmware_revision[4];
	BYTE	hardware_revision;		/* IDA, IDA-2, SMART */
	BYTE	boot_block_revision[4];		/* SMART only */
	DOUBLE	drive_present_bit_map;		/* SMART only */
	DOUBLE	external_drive_bit_map;		/* SMART only */
	DOUBLE	eisa_id;			/* SMART only */
	BYTE	reserved[482];			/* IDA, IDA-2 */
};



/*
 * 0x10: Identify Logical Drive
 *	This command is used to determine the size and fault tolerance
 *	mode of a logically configured drive.
 */

struct identify_logical_drive {
	WORD	block_size_in_bytes;
	DOUBLE	blocks_available;
	BYTE	logical_drive_parameter_table[16];
	BYTE	fault_tolerance;
	BYTE	reserved[489];
};



/*
 * 0x12: Sense Logical Drive Status
 *	This command is used to see error and problem information for a
 *	logical drive.
 */

struct identify_logical_drive_status {
	BYTE	unit_status;
	BYTE	drive_failure_map[4];
	WORD	read_error_count[32];
	WORD	write_error_count[32];
	BYTE	drive_error_data[8*32];
	BYTE	drq_timeout_count[32];
	DOUBLE	blocks_left_to_recover;
	BYTE	drive_recovering;
	WORD	remap_count[32];
	DOUBLE	replacement_drive_map;
	DOUBLE	active_spare_map;
	BYTE	spare_status;
	BYTE	spare_to_replace_map[32];
	DOUBLE	replaced_marked_ok_map;
	BYTE	media_has_been_exchanged;
	BYTE	reserved[488];
};

/* unit_status values: */
#define	LDR_OPER	0	/* Logical drive operable */
#define	LDR_FAIL	1	/* Logical drive failure */
#define	LDR_CONFIG	2	/* Logical drive needs to be configured */
#define	LDR_REGEN	3	/* Logical drive operating in regenerate mode */
#define	LDR_RECOV	4	/* Logical drive ready to start the */
				/*	Recover process */
#define	LDR_RESREC	5	/* Logical drive will resume Recover process */
				/*	after a power-off */

/* *********************************************************************** */



/*
 * 0x50: Sense Configuration
 *	This command is used to sense the configuration parameters for
 *	a logical drive.
 *
 * 0x51: Set Configuration
 *	This command is used to set the configuration parameters for
 *	a logical drive.
 */

struct configuration {
	DOUBLE	configuration_signature;
	WORD	compatibility_port_address;
	BYTE	data_distribution_mode;
	BYTE	surface_analysis_control;
	WORD	controller_physical_drive_count;
	WORD	logical_unit_physical_drive_count;
	WORD	fault_tolerance_mode;
	BYTE	physical_drive_parameter_table[16];
	BYTE	logical_drive_parameter_table[16];
	DOUBLE	drive_assignment_map;
	WORD	distribution_factor;
	DOUBLE	spare_assignment_map;
	BYTE	reserved[6];
	WORD	operating_system;
	BYTE	controller_order;
	BYTE	additional_information;
	WORD	reserved_2[446];
};



/*
 * 0xc0: Set Cache Configuration
 *	This command is used for configuring and controllering read cache
 *	and posted write operations parameters.
 */

struct set_posted_write_buffer {
	DOUBLE	posted_writes_drive_bit_map;
	WORD	kbytes_for_read_cache;
	WORD	kbytes_for_posted_write_memory;
	BYTE	disable_flag;
	BYTE	reserved[503];
};



/*
 * 0xc2: Flush/Disable Posted-Write Cache
 *	This command is used for configuring and controllering read cache
 *	and posted write operations parameters.
 */
struct flush_disable {
	WORD	disable_flag;
	BYTE	reserved[510];
};




/* The following is logical drive parameter table, 16 bytes in total. */

struct logical_parameter_table {
	WORD	cylinders;
	BYTE	heads;
	BYTE	xsig;
	BYTE	psectors;
	WORD	wpre;
	BYTE	maxecc;
	BYTE	drive_control;
	WORD	pcyls;
	BYTE	pheads;
	WORD	landz;
	BYTE	sectors_per_track;
	BYTE	check_sum;
};


/* disable_flag values: */
#define	CSA_FLUSH_N_DISABLE	1 /* flush and temporarily disable cache */
#define	CSA_FLUSH_N_ENABLE	0 /* flush and (re)enable cache */

#ifdef	__cplusplus
}
#endif

#endif /* _CSA_CSA_CMDS_H */
