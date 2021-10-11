/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _PCFS_NEWFS_H
#define	_PCFS_NEWFS_H

#pragma ident	"@(#)pcfs_newfs.h	1.3	96/04/10 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	BPSEC		512	/* Assumed # of bytes per sector */

#define	OPCODE1		0xE9
#define	OPCODE2		0xEB
#define	BOOTSECSIG	0xAA55

#define	uppercase(c)	((c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 'A' : (c))

#define	FAT12_TYPE_STRING	"FAT12   "
#define	FAT16_TYPE_STRING	"FAT16   "

#define	FAT16_ENTSPERSECT	256

#ifndef	SUNIXOSBOOT
#define	SUNIXOSBOOT	190	/* Solaris UNIX boot partition */
#endif

/*
 *  A macro implementing a ceiling function for integer divides.
 */
#define	idivceil(dvend, dvsor) \
	((dvend)/(dvsor) + (((dvend)%(dvsor) == 0) ? 0 : 1))

/*
 *	MS-DOS Disk layout:
 *
 *	---------------------
 *	|    Boot sector    |
 *	|-------------------|
 *	|   Reserved area   |
 *	|-------------------|
 *	|	FAT #1      |
 *	|-------------------|
 *	|	FAT #2      |
 *	|-------------------|
 *	|   Root directory  |
 *	|-------------------|
 *	|                   |
 *	|     File area     |
 *	|___________________|
 */

#ifdef i386
#pragma	pack(1)
#endif
struct _orig_bios_param_blk {
	short   bytes_sector;
	u_char	sectors_per_cluster;
	short   resv_sectors;
	u_char  num_fats;
	short   num_root_entries;
	short   sectors_in_volume;
	u_char  media;
	short   sectors_per_fat;
	short   sectors_per_track;
	short   heads;
	long    hidden_sectors;
	long    sectors_in_logical_volume;
};
#ifdef i386
#pragma pack()
#endif

#ifdef i386
#pragma	pack(1)
#endif
struct _bpb_extensions {
	u_char  phys_drive_num;
	u_char  reserved;
	u_char  ext_signature;
	long    volume_id;
	char    volume_label[11];
	char	type[8];
};
#ifdef i386
#pragma pack()
#endif

#ifdef i386
#pragma	pack(1)
#endif
struct _sun_bpb_extensions {
	u_short	bs_offset_high;
	u_short	bs_offset_low;
};
#ifdef i386
#pragma pack()
#endif

#ifdef i386
#pragma	pack(1)
#endif
struct _bios_param_blk {
	struct _orig_bios_param_blk	bpb;
	struct _bpb_extensions		ebpb;
};
#ifdef i386
#pragma pack()
#endif

#ifdef i386
#pragma	pack(1)
struct _boot_sector {
	u_char	bs_jump_code[3];
	char    bs_oem_name[8];
	struct _bios_param_blk	bs_bpb;
	struct _sun_bpb_extensions bs_sebpb;
	char	bs_bootstrap[444];
	u_short bs_signature;
};
#pragma pack()
#else
#define	ORIG_BPB_START_INDEX	9	/* index into filler field */
struct _boot_sector {
	u_char	bs_jump_code[2];
	u_char  bs_filler[60];
	struct _sun_bpb_extensions bs_sebpb;
	char    bs_bootstrap[444];
	u_short bs_signature;
};
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _PCFS_NEWFS_H */
