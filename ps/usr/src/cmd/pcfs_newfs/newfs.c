/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)newfs.c 1.3     95/10/18 SMI"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/fdio.h>
#ifdef i386
#include <sys/dktp/fdisk.h>
#endif
#include <sys/dkio.h>
#include <sys/sysmacros.h>
#include "pcfs_newfs.h"
#include <sys/fs/pc_fs.h>
#include <sys/fs/pc_dir.h>
#include <sys/fs/pc_label.h>
#include <macros.h>

/*
 *	newfs (for pcfs)
 *
 *	Install a boot block, FAT, and (if desired) the first resident
 *	of the new fs.
 *
 *	XXX -- no localization done!!!!
 *	XXX -- floppy opens need O_NDELAY?
 */

char	Firstfileattr = 0x20;
int	Notreally = 0;
int	Fatentsize = 0;
int	Outputtofile = 0;
int	SunBPBfields = 0;
int	Verbose = 0;
int	Imagesize = 3;

void
usage(char *arg0, int reason)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "%s [-NvSrhs] [-B filename] [-b label] ", arg0);
	fprintf(stderr, "[-i filename] device_name\n");
	fprintf(stderr, "%s [-NvSrhs] [-B filename] [-b label] ", arg0);
	fprintf(stderr, "[-i filename] [-d 3|5] -f imagefile_name\n");
	fprintf(stderr, "\n");

	if (reason == 1) {
		fprintf(stderr, "The 'imagefile_name' argument should be ");
		fprintf(stderr, "the name of an\nordinary file. The file ");
		fprintf(stderr, "will store a generic boot image.\nAt ");
		fprintf(stderr, "present, this is only useful as a ");
		fprintf(stderr, "debugging tool.\n");
		fprintf(stderr, "\n");
	} else if (reason == 2) {
		fprintf(stderr, "The 'device_name' argument should specify ");
		fprintf(stderr, "a raw diskette or\nhard disk device.  Hard ");
		fprintf(stderr, "disks should be further qualified\nwith a ");
		fprintf(stderr, "partition specifying suffix.  Examples ");
		fprintf(stderr, "are:\n    /dev/rdiskette and ");
		fprintf(stderr, "/dev/rdsk/c0t0d0p0:c \n\n");
		fprintf(stderr, "NOTE: The hard disk support ");
		fprintf(stderr, "capabilities of this\nprogram ");
		fprintf(stderr, "are only supported on x86 machines.\n");
		fprintf(stderr, "\n");
	} else if (reason == 3) {
		fprintf(stderr, "If the -S option (build Solaris boot media)");
		fprintf(stderr, "is specified,\n");
		fprintf(stderr, "a compatible boot block file must also ");
		fprintf(stderr, "be specified\nwith the -B option.\n");
		fprintf(stderr, "\n");
	} else if (reason == 4) {
		fprintf(stderr, "The r,h, and s options specify that the ");
		fprintf(stderr, "initial file should be\nmarked as ");
		fprintf(stderr, "read-only, hidden, or system, ");
		fprintf(stderr, "respectively.\nThese options are only ");
		fprintf(stderr, "valid if an initial file has been\n");
		fprintf(stderr, "specified with the -i option.\n");
	} else if (reason == 5) {
		fprintf(stderr, "The imagesize specified with -d must either ");
		fprintf(stderr, "be\n3 (for a 1.44MB 3.5\" floppy image) ");
		fprintf(stderr, "or 5 (for a 1.2MB 5.25\" floppy image)\n");
	}

	exit(1);
}

/*
 *  parse_drvnum
 *	Convert a partition name into a drive number.
 */
int
parse_drvnum(char *pn)
{
	int drvnum;

	/*
	 * Determine logical drive to seek after.
	 */
	if (strlen(pn) == 1 && *pn >= 'c' && *pn <= 'z') {
		drvnum = *pn - 'c' + 1;
	} else if (*pn >= '0' && *pn <= '9') {
		char *d;
		int v, m, c;

		v = 0;
		d = pn;
		while (*d && *d >= '0' && *d <= '9') {
			c = strlen(d);
			m = 1;
			while (--c)
				m *= 10;
			v += m * (*d - '0');
			d++;
		}

		if (*d || v > 24) {
			fprintf(stderr,
			    "%s: bogus partition specification.\n", pn);
			return (-1);
		}
		drvnum = v;
	} else if (strcmp(pn, "boot") == 0) {
		drvnum = 99;
	}

	return (drvnum);
}

#ifdef i386
/*
 *  seek_partn
 *
 *	Seek to the beginning of the partition where we need to install
 *	the new FAT.  Zero return for any error, but print error
 *	messages here.
 */
int
seek_partn(int fd, char *pn, struct _bios_param_blk *wbpb,
    struct _sun_bpb_extensions *sbpb)
{
	struct ipart part[FD_NUMPART];
	struct mboot mb;
	ulong bootsectseek;
	ulong drvbasesec;
	int drvnum;
	int rval = 0;
	int i;

	if ((drvnum = parse_drvnum(pn)) < 0)
		return (rval);

	if (read(fd, &mb, sizeof (mb)) != sizeof (mb)) {
		fprintf(stderr, "Couldn't read a Master Boot Record?!\n");
		return (rval);
	}

	memcpy(part, mb.parts, sizeof (part));

	if (ltohs(mb.signature) != BOOTSECSIG) {
		fprintf(stderr, "Bad Sig on master boot record!\n");
		return (rval);
	}

	/*
	 *  Wade through the fdisk table(s) looking for the specified
	 *  logical drive.
	 */
	if (drvnum == 99) {
		for (i = 0; i < FD_NUMPART; i++) {
			if (part[i].systid == SUNIXOSBOOT)
				break;
		}
		if (i == FD_NUMPART) {
			fprintf(stderr, "No boot partition found on drive!\n");
			return (rval);
		} else {
			bootsectseek = ltohl(part[i].relsect) * BPSEC;
			if (ltohl(part[i].numsect) > 0xffff)
				wbpb->bpb.sectors_in_volume = 0;
			else
				wbpb->bpb.sectors_in_volume =
				    (short)ltohl(part[i].numsect);
			wbpb->bpb.sectors_in_logical_volume =
			    ltohl(part[i].numsect);
			sbpb->bs_offset_high = ltohl(part[i].relsect) >> 16;
			sbpb->bs_offset_low = ltohl(part[i].relsect) & 0xFFFF;
		}
	} else if (drvnum == 1) {
		for (i = 0; i < FD_NUMPART; i++) {
			if (part[i].systid == DOSOS12 ||
			    part[i].systid == DOSOS16 ||
			    part[i].systid == DOSHUGE)
				break;
		}
		if (i == FD_NUMPART) {
			fprintf(stderr, "No primary dos partition found ");
			fprintf(stderr, "on drive!\n");
			return (rval);
		} else {
			bootsectseek = ltohl(part[i].relsect) * BPSEC;
			if (ltohl(part[i].numsect) > 0xffff)
				wbpb->bpb.sectors_in_volume = 0;
			else
				wbpb->bpb.sectors_in_volume =
				    (short)ltohl(part[i].numsect);
			wbpb->bpb.sectors_in_logical_volume =
			    ltohl(part[i].numsect);
			sbpb->bs_offset_high = ltohl(part[i].relsect) >> 16;
			sbpb->bs_offset_low = ltohl(part[i].relsect) & 0xFFFF;
		}
	} else {
		struct mboot extmboot;
		u_char xsysid;
		ulong nextseek, partbias, xnumsect;

		for (i = 0; i < FD_NUMPART; i++) {
			if (part[i].systid == EXTDOS)
				break;
		}
		if (i == FD_NUMPART) {
			fprintf(stderr, "No primary dos partition found ");
			fprintf(stderr, "on drive!\n");
			return (rval);
		}

		partbias = nextseek = ltohl(part[i].relsect);
		xsysid = part[i].systid;
		xnumsect = ltohl(part[i].numsect);
		while (--drvnum && xsysid == EXTDOS) {
			lseek(fd, nextseek * BPSEC, SEEK_SET);
			if (read(fd, &extmboot, sizeof (extmboot)) !=
			    sizeof (mb)) {
				fprintf(stderr, "Couldn't read extended "
				    "partition record?!\n");
				return (rval);
			}
			memcpy(part, extmboot.parts, sizeof (part));
			xsysid = part[1].systid;
			drvbasesec = nextseek;
			nextseek = ltohl(part[1].relsect) + partbias;
		}
		bootsectseek = ltohl(part[0].relsect) + drvbasesec;
		if (drvnum) {
			fprintf(stderr, "No such partition found on drive!\n");
			return (rval);
		} else if (xnumsect < (bootsectseek - partbias)) {
			fprintf(stderr, "Bogus extended partition info!\n");
			return (rval);
		} else {
			if (ltohl(part[0].numsect) > 0xffff)
				wbpb->bpb.sectors_in_volume = 0;
			else
				wbpb->bpb.sectors_in_volume =
				    (short)ltohl(part[0].numsect);
			wbpb->bpb.sectors_in_logical_volume =
			    ltohl(part[0].numsect);
			sbpb->bs_offset_high = bootsectseek >> 16;
			sbpb->bs_offset_low = bootsectseek & 0xFFFF;
			bootsectseek *= BPSEC;
		}
	}

	if (Verbose)
		printf("Requested partition's offset: Sector %x.\n",
		    bootsectseek/BPSEC);

	if (lseek(fd, bootsectseek, SEEK_SET) < 0) {
		fprintf(stderr, "Partition %s", pn);
		perror("");
		return (rval);
	}

	return (++rval);
}

#endif	/* i386 */

/*
 *  prepare_image_file
 *
 *	Open the file that will hold the image (as opposed to the image
 *	being written to the boot sector of an actual disk).
 */
int
prepare_image_file(char *fn, struct _bios_param_blk *wbpb,
    struct _sun_bpb_extensions *sbpb)
{
	int fd;
	char zerobyte = '\0';

	if ((fd = open(fn, O_RDWR | O_CREAT | O_EXCL, 0666)) < 0) {
		perror(fn);
		exit(2);
	}

	if (Imagesize == 5) {
		/* Disk image of a 1.2M floppy */
		wbpb->bpb.sectors_in_volume = 2 * 80 * 15;
		wbpb->bpb.sectors_in_logical_volume = 2 * 80 * 15;
		wbpb->bpb.sectors_per_track = 15;
		wbpb->bpb.heads = 2;
		wbpb->bpb.media = 0xF9;
		wbpb->bpb.num_root_entries = 224;
		wbpb->bpb.sectors_per_cluster = 1;
		wbpb->bpb.sectors_per_fat = 7;
	} else {
		/* Disk image of a 1.44M floppy */
		wbpb->bpb.sectors_in_volume = 2 * 80 * 18;
		wbpb->bpb.sectors_in_logical_volume = 2 * 80 * 18;
		wbpb->bpb.sectors_per_track = 18;
		wbpb->bpb.heads = 2;
		wbpb->bpb.media = 0xF0;
		wbpb->bpb.num_root_entries = 224;
		wbpb->bpb.sectors_per_cluster = 1;
		wbpb->bpb.sectors_per_fat = 9;
	}

	/*
	 * Make a holey file, with length the exact
	 * size of the floppy image.
	 */
	if (lseek(fd, (wbpb->bpb.sectors_in_volume * BPSEC)-1, SEEK_SET) < 0) {
		close(fd);
		perror(fn);
		exit(2);
	}

	if (write(fd, &zerobyte, 1) != 1) {
		close(fd);
		perror(fn);
		exit(2);
	}

	if (lseek(fd, 0, SEEK_SET) < 0) {
		close(fd);
		perror(fn);
		exit(2);
	}

	Fatentsize = 12;  /* Size of fat entry in bits */
	strncpy(wbpb->ebpb.type, FAT12_TYPE_STRING, strlen(FAT12_TYPE_STRING));
	wbpb->ebpb.phys_drive_num = 0;

	sbpb->bs_offset_high = 0;
	sbpb->bs_offset_low = 0;

	return (fd);
}

/*
 *  partn_lecture
 *
 *	Give a brief sermon on dev_name user should pass to
 *	the program from the command line.
 *
 */
void
partn_lecture(char *dn)
{
	fprintf(stderr, "\nSorry, I had assumed %s was a diskette.\n\n", dn);
	fprintf(stderr, "However, a diskette specific operation ");
	fprintf(stderr, "failed to work on this device.\n");
	fprintf(stderr, "If it is actually a hard disk you meant ");
	fprintf(stderr, "to specify, please be sure to\n");
	fprintf(stderr, "provide the name of the full physical disk, and ");
	fprintf(stderr, "further qualify that name\n");
	fprintf(stderr, "with a partition name.\n\n");
	fprintf(stderr, "Hint: the device is usually something similar to\n");
	fprintf(stderr, "    /dev/rdsk/c0d0p0 or /dev/rdsk/c0t0d0p0\n");
	fprintf(stderr, "and the partition name is appended to the ");
	fprintf(stderr, "device name. For example:\n");
	fprintf(stderr, "    /dev/rdsk/c0d0p0:c or /dev/rdsk/c0t2d0p0:boot\n");
}

/*
 *  lookup_floppy
 *
 *	Look up a media descriptor byte and other crucial BPB values
 *	based on floppy characteristics.
 */
void
lookup_floppy(struct fd_char *fdchar, struct _bios_param_blk *wbpb,
    struct _sun_bpb_extensions *sbpb)
{
	ulong tsize;
	int cyls, spt, hds, diam;

	cyls = fdchar->fdc_ncyl;
	diam = fdchar->fdc_medium;
	spt = fdchar->fdc_secptrack;
	hds = fdchar->fdc_nhead;

	tsize = cyls * hds * spt;

	wbpb->bpb.sectors_in_volume = (short)tsize;
	wbpb->bpb.sectors_in_logical_volume = tsize;
	wbpb->bpb.sectors_per_track = spt;
	wbpb->bpb.heads = hds;

	Fatentsize = 12;  /* Size of fat entry in bits */
	strncpy(wbpb->ebpb.type, FAT12_TYPE_STRING, strlen(FAT12_TYPE_STRING));
	wbpb->ebpb.phys_drive_num = 0;

	sbpb->bs_offset_high = 0;
	sbpb->bs_offset_low = 0;

	switch (diam) {
	case 3:
		switch (hds) {
		case 2:
			switch (spt) {
			case 9:
				wbpb->bpb.media = 0xF9;
				wbpb->bpb.num_root_entries = 112;
				wbpb->bpb.sectors_per_cluster = 2;
				wbpb->bpb.sectors_per_fat = 3;
				break;
			case 18:
				wbpb->bpb.media = 0xF0;
				wbpb->bpb.num_root_entries = 224;
				wbpb->bpb.sectors_per_cluster = 1;
				wbpb->bpb.sectors_per_fat = 9;
				break;
			case 36:
				wbpb->bpb.media = 0xF0;
				wbpb->bpb.num_root_entries = 240;
				wbpb->bpb.sectors_per_cluster = 2;
				wbpb->bpb.sectors_per_fat = 9;
				break;
			default:
				fprintf(stderr, "Unknown diskette params! ");
				fprintf(stderr,
				    "3.5'' diskette with %d heads ", hds);
				fprintf(stderr, "and %d sectors/track.\n",
				    spt);
				exit(4);
			}
			break;
		case 1:
		default:
			fprintf(stderr, "Unknown diskette params!  ");
			fprintf(stderr, "3.5'' diskette with %d heads ", hds);
			exit(4);
		}
		break;
	case 5:
		switch (hds) {
		case 2:
			switch (spt) {
			case 15:
				wbpb->bpb.media = 0xF9;
				wbpb->bpb.num_root_entries = 224;
				wbpb->bpb.sectors_per_cluster = 1;
				wbpb->bpb.sectors_per_fat = 7;
				break;
			case 9:
				wbpb->bpb.media = 0xFD;
				wbpb->bpb.num_root_entries = 112;
				wbpb->bpb.sectors_per_cluster = 2;
				wbpb->bpb.sectors_per_fat = 2;
				break;
			case 8:
				wbpb->bpb.media = 0xFF;
				wbpb->bpb.num_root_entries = 112;
				wbpb->bpb.sectors_per_cluster = 1;
				wbpb->bpb.sectors_per_fat = 2;
				break;
			default:
				fprintf(stderr, "Unknown diskette params! ");
				fprintf(stderr,
				    "5.25'' diskette with %d heads ", hds);
				fprintf(stderr, "and %d sectors/track.\n",
				    spt);
				exit(4);
			}
			break;
		case 1:
			switch (spt) {
			case 9:
				wbpb->bpb.media = 0xFC;
				wbpb->bpb.num_root_entries = 64;
				wbpb->bpb.sectors_per_cluster = 1;
				wbpb->bpb.sectors_per_fat = 2;
				break;
			case 8:
				wbpb->bpb.media = 0xFE;
				wbpb->bpb.num_root_entries = 64;
				wbpb->bpb.sectors_per_cluster = 1;
				wbpb->bpb.sectors_per_fat = 1;
				break;
			default:
				fprintf(stderr, "Unknown diskette params! ");
				fprintf(stderr,
				    "5.25'' diskette with %d heads ", hds);
				fprintf(stderr, "and %d sectors/track.\n",
				    spt);
				exit(4);
			}
			break;
		default:
			fprintf(stderr, "Unknown diskette params! ");
			fprintf(stderr, "5.25'' diskette with %d heads.", hds);
			exit(4);
		}
		break;
	default:
		fprintf(stderr, "\nSorry, I only know about ");
		fprintf(stderr, "5.25'' and 3.5'' diskettes.\n");
		exit(4);
	}
}

/*
 *  compute_hd_clustersize
 *
 *	Compute a not too wasteful combination of sectors/fat and
 *	sectors/cluster for a hard drive.
 *
 *	Algorithm arrived at after examining tables from 'DOS internals',
 *	plus some on-the-side computing to make sure it wasn't too
 *	wasteful.  I believe that if any sectors are wasted on FAT, that
 *	number of sectors is always less that the number of sectors required
 *	to form a cluster.
 */
void
compute_hd_clustersize(struct _bios_param_blk *wbpb)
{
	ulong lost, left, cleft;
	ulong spc, spf;

	lost = wbpb->bpb.resv_sectors;
	lost += (wbpb->bpb.num_root_entries) * sizeof (struct pcdir) / BPSEC;
	left = wbpb->bpb.sectors_in_logical_volume - lost;

	if (left >= 0x400000) {
		fprintf(stderr, "Partition too large for a FAT!\n");
		exit(4);
	} else if (left <= 0x40000) {
		spc = 4;
	} else if (left <= 0x80000) {
		spc = 8;
	} else if (left <= 0x100000) {
		spc = 16;
	} else if (left <= 0x200000) {
		spc = 32;
	} else {
		spc = 64;
	}

	/* Assuming a 16 bit fat, compute a sector/fat figure */
	spf = idivceil(left, (FAT16_ENTSPERSECT * spc + 2));

	cleft = idivceil((left - 2*spf), spc);
	if (cleft <= DOS_F12MAXC) {
		/*
		 * Can't use a 16-bit fat as PCFS would expect 12 bit
		 * FATs for this few a number of clusters.  We'll go
		 * ahead and keep the same sectors per fat and just
		 * end up losing about a cluster (which if we added
		 * back would bump us back up to needing the 16 bit
		 * fat!)
		 */
		Fatentsize = 12;  /* Size of fat entry in bits */
		strncpy(wbpb->ebpb.type, FAT12_TYPE_STRING,
		    strlen(FAT12_TYPE_STRING));
	} else {
		Fatentsize = 16;  /* Size of fat entry in bits */
		strncpy(wbpb->ebpb.type, FAT16_TYPE_STRING,
		    strlen(FAT16_TYPE_STRING));
	}

	wbpb->bpb.sectors_per_cluster = spc;
	wbpb->bpb.sectors_per_fat = (u_short)spf;
}

/*
 *  open_and_seek
 *
 *	Open the requested 'dev_name'.  Seek to point where
 *	we'll write boot sectors, etc., based on any ':partition'
 *	attachments to the dev_name.
 *
 *	By the time we are finished here, the entire BPB will be
 *	filled in, excepting the volume label.
 */
int
open_and_seek(char *dn, struct _bios_param_blk *wbpb,
    struct _sun_bpb_extensions *sbpb)
{
	struct dk_geom dginfo;
	struct fd_char fdchar;
	struct stat di;
	char *actualdisk = NULL;
	char *suffix = NULL;
	int fd;

	if (Verbose)
		printf("Opening destination device/file.\n");

	/*
	 * We hold these truths to be self evident, all BPBs we create
	 * will have these values in these fields.
	 */
	wbpb->bpb.num_fats = 2;
	wbpb->bpb.bytes_sector = BPSEC;
	wbpb->bpb.resv_sectors = 1;
	wbpb->bpb.hidden_sectors = 0;

	wbpb->ebpb.ext_signature = 0x29; /* Magic number for modern format */
	wbpb->ebpb.volume_id = 0;

	/*
	 * If all output goes to a simple file, call a routine to setup
	 * that scenario. Otherwise, try to find the device.
	 */
	if (Outputtofile) {
		return (fd = prepare_image_file(dn, wbpb, sbpb));
	} else if (stat(dn, &di)) {
		/*
		 *  Device named on command line doesn't exist.  That
		 *  probably means there is a partition-specifying
		 *  suffix attached to the actual disk name.
		 */
		actualdisk = strtok(strdup(dn), ":");
		if (suffix = strchr(dn, ':'))
			suffix++;

		if (stat(actualdisk, &di)) {
			perror(actualdisk);
			exit(2);
		}
	} else {
		actualdisk = strdup(dn);
	}

	/*
	 *  Destination exists, now find more about it.
	 */
	if (!(S_ISCHR(di.st_mode))) {
		fprintf(stderr, "\n%s: Sorry, dev_name must indicate a ",
		    actualdisk);
		fprintf(stderr, "character special device.\n");
		exit(2);
	} else if ((fd = open(actualdisk, O_RDWR | O_EXCL)) < 0) {
		perror(actualdisk);
		exit(2);
	}

	/*
	 * Find appropriate partition if we were requested to do so.
	 */
#ifdef i386
	if (suffix && !(seek_partn(fd, suffix, wbpb, sbpb))) {
		close(fd);
		exit(2);
	}
#else
	if (suffix) {
		fprintf(stderr, "Sorry, hard disk newfs (for pcfs) is ");
		fprintf(stderr, "only supported on x86.\n");
		exit(2);
	}
#endif

	if (!suffix) {
		/*
		 * Likely we have a floppy drive.  We can do a floppy
		 * specific ioctl to get the remainder of our info. Now,
		 * if the ioctl fails, we have a good idea that they
		 * aren't really on a floppy and they should have given
		 * us a partition specifier.
		 */
		if (ioctl(fd, FDIOGCHAR, &fdchar) == -1) {
			if (errno == ENOTTY) {
				partn_lecture(actualdisk);
				close(fd);
				exit(2);
			}
		} else {
#ifdef sparc
			fdchar.fdc_medium = 3;
#endif
			lookup_floppy(&fdchar, wbpb, sbpb);
		}
#ifdef i386
	} else {
		/*
		 *  Look up the last remaining bits of info we need
		 *  that is specific to the hard drive using a disk ioctl.
		 */
		if ((ioctl(fd, DKIOCG_VIRTGEOM, &dginfo)) == -1) {
			close(fd);
			perror("Drive geometry lookup");
			exit(2);
		} else {
			wbpb->bpb.heads = dginfo.dkg_nhead;
			wbpb->bpb.sectors_per_track = dginfo.dkg_nsect;
			wbpb->bpb.media = 0xF8;
			wbpb->bpb.num_root_entries = 512;
			wbpb->ebpb.phys_drive_num = 0x80;
			compute_hd_clustersize(wbpb);
		}
	}
#else
	}
#endif
	return (fd);
}

/*
 * The following is a copy of MS-DOS 4.0 boot block.
 * It consists of the BIOS parameter block, and a disk
 * bootstrap program.
 *
 * The BIOS parameter block contains the right values
 * for the 3.5" high-density 1.44MB floppy format.
 *
 * This will be our default boot sector, if the user
 * didn't point us at a different one.
 *
 */
u_char defbootsec[512] = {
	0xeb, 0x3c, 0x90, 	/* 8086 short jump + displacement + NOP */
	'M', 'S', 'D', 'O', 'S', '4', '.', '0',	/* OEM name & version */
	0x00, 0x02, 0x01, 0x01, 0x00,
	0x02, 0xe0, 0x00, 0x40, 0x0b,
	0xf0, 0x09, 0x00, 0x12, 0x00,
	0x02, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
	0x29, 0x00, 0x00, 0x00, 0x00,
	'N', 'O', 'N', 'A', 'M', 'E', ' ', ' ', ' ', ' ', ' ',
	'F', 'A', 'T', '1', '2', ' ', ' ', ' ',
	0xfa, 0x33,
	0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x16, 0x07,
	0xbb, 0x78, 0x00, 0x36, 0xc5, 0x37, 0x1e, 0x56,
	0x16, 0x53, 0xbf, 0x3e, 0x7c, 0xb9, 0x0b, 0x00,
	0xfc, 0xf3, 0xa4, 0x06, 0x1f, 0xc6, 0x45, 0xfe,
	0x0f, 0x8b, 0x0e, 0x18, 0x7c, 0x88, 0x4d, 0xf9,
	0x89, 0x47, 0x02, 0xc7, 0x07, 0x3e, 0x7c, 0xfb,
	0xcd, 0x13, 0x72, 0x7c, 0x33, 0xc0, 0x39, 0x06,
	0x13, 0x7c, 0x74, 0x08, 0x8b, 0x0e, 0x13, 0x7c,
	0x89, 0x0e, 0x20, 0x7c, 0xa0, 0x10, 0x7c, 0xf7,
	0x26, 0x16, 0x7c, 0x03, 0x06, 0x1c, 0x7c, 0x13,
	0x16, 0x1e, 0x7c, 0x03, 0x06, 0x0e, 0x7c, 0x83,
	0xd2, 0x00, 0xa3, 0x50, 0x7c, 0x89, 0x16, 0x52,
	0x7c, 0xa3, 0x49, 0x7c, 0x89, 0x16, 0x4b, 0x7c,
	0xb8, 0x20, 0x00, 0xf7, 0x26, 0x11, 0x7c, 0x8b,
	0x1e, 0x0b, 0x7c, 0x03, 0xc3, 0x48, 0xf7, 0xf3,
	0x01, 0x06, 0x49, 0x7c, 0x83, 0x16, 0x4b, 0x7c,
	0x00, 0xbb, 0x00, 0x05, 0x8b, 0x16, 0x52, 0x7c,
	0xa1, 0x50, 0x7c, 0xe8, 0x87, 0x00, 0x72, 0x20,
	0xb0, 0x01, 0xe8, 0xa1, 0x00, 0x72, 0x19, 0x8b,
	0xfb, 0xb9, 0x0b, 0x00, 0xbe, 0xdb, 0x7d, 0xf3,
	0xa6, 0x75, 0x0d, 0x8d, 0x7f, 0x20, 0xbe, 0xe6,
	0x7d, 0xb9, 0x0b, 0x00, 0xf3, 0xa6, 0x74, 0x18,
	0xbe, 0x93, 0x7d, 0xe8, 0x51, 0x00, 0x32, 0xe4,
	0xcd, 0x16, 0x5e, 0x1f, 0x8f, 0x04, 0x8f, 0x44,
	0x02, 0xcd, 0x19, 0x58, 0x58, 0x58, 0xeb, 0xe8,
	0xbb, 0x00, 0x07, 0xb9, 0x03, 0x00, 0xa1, 0x49,
	0x7c, 0x8b, 0x16, 0x4b, 0x7c, 0x50, 0x52, 0x51,
	0xe8, 0x3a, 0x00, 0x72, 0xe6, 0xb0, 0x01, 0xe8,
	0x54, 0x00, 0x59, 0x5a, 0x58, 0x72, 0xc9, 0x05,
	0x01, 0x00, 0x83, 0xd2, 0x00, 0x03, 0x1e, 0x0b,
	0x7c, 0xe2, 0xe2, 0x8a, 0x2e, 0x15, 0x7c, 0x8a,
	0x16, 0x24, 0x7c, 0x8b, 0x1e, 0x49, 0x7c, 0xa1,
	0x4b, 0x7c, 0xea, 0x00, 0x00, 0x70, 0x00, 0xac,
	0x0a, 0xc0, 0x74, 0x29, 0xb4, 0x0e, 0xbb, 0x07,
	0x00, 0xcd, 0x10, 0xeb, 0xf2, 0x3b, 0x16, 0x18,
	0x7c, 0x73, 0x19, 0xf7, 0x36, 0x18, 0x7c, 0xfe,
	0xc2, 0x88, 0x16, 0x4f, 0x7c, 0x33, 0xd2, 0xf7,
	0x36, 0x1a, 0x7c, 0x88, 0x16, 0x25, 0x7c, 0xa3,
	0x4d, 0x7c, 0xf8, 0xc3, 0xf9, 0xc3, 0xb4, 0x02,
	0x8b, 0x16, 0x4d, 0x7c, 0xb1, 0x06, 0xd2, 0xe6,
	0x0a, 0x36, 0x4f, 0x7c, 0x8b, 0xca, 0x86, 0xe9,
	0x8a, 0x16, 0x24, 0x7c, 0x8a, 0x36, 0x25, 0x7c,
	0xcd, 0x13, 0xc3, 0x0d, 0x0a, 0x4e, 0x6f, 0x6e,
	0x2d, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20,
	0x64, 0x69, 0x73, 0x6b, 0x20, 0x6f, 0x72, 0x20,
	0x64, 0x69, 0x73, 0x6b, 0x20, 0x65, 0x72, 0x72,
	0x6f, 0x72, 0x0d, 0x0a, 0x52, 0x65, 0x70, 0x6c,
	0x61, 0x63, 0x65, 0x20, 0x61, 0x6e, 0x64, 0x20,
	0x70, 0x72, 0x65, 0x73, 0x73, 0x20, 0x61, 0x6e,
	0x79, 0x20, 0x6b, 0x65, 0x79, 0x20, 0x77, 0x68,
	0x65, 0x6e, 0x20, 0x72, 0x65, 0x61, 0x64, 0x79,
	0x0d, 0x0a, 0x00, 0x49, 0x4f, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x53, 0x59, 0x53, 0x4d, 0x53,
	0x44, 0x4f, 0x53, 0x20, 0x20, 0x20, 0x53, 0x59,
	0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa
};

/*
 *  verify_bootblk
 *
 *	We were provided with the name of a file containing the bootblk
 *	to install.  Verify it has a valid boot sector as best we can. Any
 *	errors and we return a bad file descriptor.  Otherwise we fill up the
 *	provided buffer with the boot sector, return the file
 *	descriptor for later use and leave the file pointer just
 *	past the boot sector part of the boot block file.
 */
int
verify_bootblkfile(char *fn, u_char *bsbuf, ulong *blkfilesize)
{
	struct _boot_sector *bsp;
	struct stat fi;
	int bsfd = -1;

	if (stat(fn, &fi)) {
		perror(fn);
	} else if (fi.st_size < BPSEC) {
		fprintf(stderr, "%s: Too short to be a boot sector.\n", fn);
	} else if ((bsfd = open(fn, O_RDONLY)) < 0) {
		perror(fn);
	} else if (read(bsfd, bsbuf, BPSEC) < BPSEC) {
		close(bsfd);
		bsfd = -1;
		perror("Bootblk read");
	} else {
		bsp = (struct _boot_sector *)bsbuf;
		if ((bsp->bs_jump_code[0] != OPCODE1 &&
		    bsp->bs_jump_code[0] != OPCODE2) ||
		    ltohs(bsp->bs_signature) != BOOTSECSIG) {
			close(bsfd);
			bsfd = -1;
			fprintf(stderr, "Boot block (%s) bogus.\n", fn);
		}
		*blkfilesize = fi.st_size;
	}
	return (bsfd);
}

/*
 *  verify_firstfile
 *
 *	We were provided with the name of a file to be the first file
 *	installed on the disk.  We just need to verify it exists and
 *	find out how big it is.  If it doesn't exist, we print a warning
 *	message about how the file wasn't found.  We don't exit fatally,
 *	though, rather we return a size of 0 and the FAT will be built
 *	without installing any first file.  They can then presumably
 *	install the correct first file by hand.
 */
int
verify_firstfile(char *fn, ulong *filesize)
{
	struct stat fi;
	int fd = -1;

	*filesize = 0;
	if (stat(fn, &fi) || (fd = open(fn, O_RDONLY)) < 0) {
		perror(fn);
		fprintf(stderr, "Sorry, first file will not be installed.\n");
		fprintf(stderr, "The FAT will be installed, though, so ");
		fprintf(stderr, "the proper\nfirst file should be ");
		fprintf(stderr, "installable by hand.\n");
	} else
		*filesize = fi.st_size;

	return (fd);
}

/*
 *  label_volume
 *
 *	Fill in BPB with volume label.
 */
void
label_volume(char *lbl, struct _bios_param_blk *wbpb)
{
	int ll, i;

	/* Put a volume label into our BPB. */
	if (!lbl)
		lbl = "NONAME";

	ll = min(11, (int)strlen(lbl));
	for (i = 0; i < ll; i++) {
		wbpb->ebpb.volume_label[i] = toupper(lbl[i]);
	}
	for (; i < 11; i++) {
		wbpb->ebpb.volume_label[i] = ' ';
	}
}

int
copy_bootblk(char *fn, u_char *bootsect, u_long *bootblksize)
{
	int bsfd = -1;

	if (Verbose && fn)
		printf("Request to install boot block file %s.\n", fn);
	else if (Verbose)
		printf("Request to install DOS boot block.\n");

	/*
	 *  If they want to install their own boot block, sanity check
	 *  that block.
	 */
	if (fn) {
		bsfd = verify_bootblkfile(fn, bootsect, bootblksize);
		if (bsfd < 0) {
			exit(3);
		}
		*bootblksize = roundup(*bootblksize, BPSEC);
	} else {
		memcpy(bootsect, defbootsec, BPSEC);
		*bootblksize = BPSEC;
	}

	return (bsfd);
}

/*
 *  mark_cluster
 *
 *	This routine fills a FAT entry with the value supplied to it as an
 *	argument.  The fatp argument is assumed to be a pointer to the FAT's
 *	0th entry.  The clustnum is the cluster entry that should be updated.
 *	The value is the new value for the entry.
 */
void
mark_cluster(u_char *fatp, pc_cluster_t clustnum, u_short value)
{
	u_char *ep;
	u_long idx;

	idx = (Fatentsize == 16) ? clustnum * 2 : clustnum + clustnum/2;
	ep = fatp + idx;

	if (Fatentsize == 16) {
		*(u_short *)ep = htols(value);
	} else {
		if (clustnum & 1) {
			*ep = (*ep & 0x0f) | ((value << 4) & 0xf0);
			ep++;
			*ep = (value >> 4) & 0xff;
		} else {
			*ep++ = value & 0xff;
			*ep = (*ep & 0xf0) | ((value >> 8) & 0x0f);
		}
	}
}

u_char *
build_fat(struct _bios_param_blk *wbpb, u_long bootblksize,
    u_long *fatsize, char *ffn, int *fffd, u_long *ffsize,
    pc_cluster_t *ffstartclust)
{
	pc_cluster_t cn;
	u_char *fatp;
	u_short numclust, numsect;

	/* Alloc space for a FAT and then null it out. */
	if (Verbose) {
		printf("BUILD FAT.\n");
		printf("%d sectors per fat.\n", wbpb->bpb.sectors_per_fat);
	}
	*fatsize = BPSEC * wbpb->bpb.sectors_per_fat;
	if (!(fatp = (u_char *)malloc(*fatsize))) {
		perror("FAT table alloc");
		exit(4);
	} else {
		memset(fatp, 0, *fatsize);
	}

	/* Build in-memory FAT */
	*fatp = wbpb->bpb.media;
	*(fatp + 1) = 0xFF;
	*(fatp + 2) = 0xFF;

	if (Fatentsize == 16)
		*(fatp + 3) = 0xFF;

	/*
	 * Get info on first file to install, if any.
	 */
	if (ffn)
		*fffd = verify_firstfile(ffn, ffsize);

	/*
	 * Compute number of clusters to preserve for bootblk overage.
	 * Remember that we already wrote the first sector of the boot block.
	 * These clusters are marked BAD to prevent them from being deleted
	 * or used.  The first available cluster is 2, so we always offset
	 * the clusters.
	 */
	numsect = idivceil((bootblksize - BPSEC), BPSEC);
	numclust = idivceil(numsect, wbpb->bpb.sectors_per_cluster);

	if (Verbose && numclust)
		printf("Hiding %d excess bootblk cluster(s).\n", numclust);
	for (cn = 0; cn < numclust; cn++)
		mark_cluster(fatp, cn + 2, PCF_BADCLUSTER);

	/*
	 * Compute and preserve number of clusters for first file.
	 */
	if (*fffd >= 0) {
		*ffstartclust = cn + 2;
		numsect = idivceil(*ffsize, BPSEC);
		numclust = idivceil(numsect, wbpb->bpb.sectors_per_cluster);

		if (Verbose)
			printf("Reserving %d first file cluster(s).\n",
			    numclust);
		for (cn = 0; (int)cn < (int)(numclust-1); cn++)
			mark_cluster(fatp, *ffstartclust + cn,
			    *ffstartclust + cn + 1);
		mark_cluster(fatp, *ffstartclust + cn, PCF_LASTCLUSTER);
	}

	return (fatp);
}

void
dirent_time_fill(struct pcdir *dep)
{
	struct  timeval tv;
	struct	tm	*tp;
	u_short	dostime;
	u_short	dosday;

	(void) gettimeofday(&tv, (struct timezone *)0);
	tp = localtime(&tv.tv_sec);
	/* get the time & day into DOS format */
	dostime = tp->tm_sec / 2;
	dostime |= tp->tm_min << 5;
	dostime |= tp->tm_hour << 11;
	dosday = tp->tm_mday;
	dosday |= (tp->tm_mon + 1) << 5;
	dosday |= (tp->tm_year - 80) << 9;
	dep->pcd_mtime.pct_time = htols(dostime);
	dep->pcd_mtime.pct_date = htols(dosday);
}

void
dirent_fname_fill(struct pcdir *dep, char *fn)
{
	char *fname, *fext;
	int nl, i;

	if (fname = strrchr(fn, '/')) {
		fname++;
	} else {
		fname = fn;
	}

	if (fext = strrchr(fname, '.')) {
		fext++;
	} else {
		fext = "";
	}

	fname = strtok(fname, ".");

	nl = min(PCFNAMESIZE, (int)strlen(fname));
	for (i = 0; i < nl; i++) {
		dep->pcd_filename[i] = toupper(fname[i]);
	}
	for (; i < PCFNAMESIZE; i++) {
		dep->pcd_filename[i] = ' ';
	}

	nl = min(PCFEXTSIZE, (int)strlen(fext));
	for (i = 0; i < nl; i++) {
		dep->pcd_ext[i] = toupper(fext[i]);
	}
	for (; i < PCFEXTSIZE; i++) {
		dep->pcd_ext[i] = ' ';
	}
}

u_char *
build_rootdir(struct _bios_param_blk *wbpb, char *ffn, int fffd,
    u_long ffsize, pc_cluster_t ffstart, u_long *rdirsize)
{
	u_char *rdirp;
	struct pcdir *entry;

	/*
	 * Build a root directory.  It will be empty if we don't have
	 * a first file we are installing.
	 */
	*rdirsize = wbpb->bpb.num_root_entries * sizeof (struct pcdir);
	if (!(rdirp = (u_char *)malloc(*rdirsize))) {
		perror("Root Dir alloc");
		exit(4);
	} else {
		memset(rdirp, 0, *rdirsize);
		entry = (struct pcdir *)rdirp;
	}

	/* Create directory entry for first file, if there is one */
	if (fffd >= 0) {
		dirent_fname_fill(entry, ffn);
		entry->pcd_attr = Firstfileattr;
		dirent_time_fill(entry);
		entry->pcd_scluster = htols(ffstart);
		entry->pcd_size = htoll(ffsize);
	}

	return (rdirp);
}

/*
 * write_rest
 *
 *	Write all the bytes from the current file pointer to end of file
 *	in the source file out to the destination file.  The writes should
 *	be padded to whole clusters with 0's if necessary.
 */
void
write_rest(struct _bios_param_blk *wbpb, char *efn,
    int dfd, int sfd, int remaining)
{
	char buf[BPSEC];
	u_short numsect, numclust;
	u_short wnumsect, s;
	int doneread = 0;
	int rstat;

	/*
	 * Compute number of clusters required to contain remaining bytes.
	 */
	numsect = idivceil(remaining, BPSEC);
	numclust = idivceil(numsect, wbpb->bpb.sectors_per_cluster);

	wnumsect = numclust * wbpb->bpb.sectors_per_cluster;
	for (s = 0; s < wnumsect; s++) {
		if (!doneread) {
			if ((rstat = read(sfd, buf, BPSEC)) < 0) {
				perror(efn);
				doneread = 1;
				rstat = 0;
			} else if (rstat == 0) {
				doneread = 1;
			}
			memset(&(buf[rstat]), 0, BPSEC - rstat);
		}
		if (write(dfd, buf, BPSEC) != BPSEC) {
			fprintf(stderr, "Copying ");
			perror(efn);
		}
	}
}

#ifndef i386
/*
 *  swap_pack_{bpb,sebpb}cpy
 *
 *	If not on an x86 we assume the structures making up the bpb
 *	were not packed and that longs and shorts need to be byte swapped
 *	(we've kept everything in host order up until now).  A new architecture
 *	might not need to swap or might not need to pack, in which case
 *	new routines will have to be written.  Of course if an architecture
 *	supports both packing and little-endian host order, it can follow the
 *	same path as the x86 code.
 */
void
swap_pack_bpbcpy(struct _boot_sector *bsp, struct _bios_param_blk *wbpb)
{
	u_char *fillp;

	fillp = (u_char *) &(bsp->bs_filler[ORIG_BPB_START_INDEX]);

	*fillp++ = getbyte(wbpb->bpb.bytes_sector, 1);
	*fillp++ = getbyte(wbpb->bpb.bytes_sector, 0);
	*fillp++ = wbpb->bpb.sectors_per_cluster;
	*fillp++ = getbyte(wbpb->bpb.resv_sectors, 1);
	*fillp++ = getbyte(wbpb->bpb.resv_sectors, 0);
	*fillp++ = wbpb->bpb.num_fats;
	*fillp++ = getbyte(wbpb->bpb.num_root_entries, 1);
	*fillp++ = getbyte(wbpb->bpb.num_root_entries, 0);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_volume, 1);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_volume, 0);
	*fillp++ = wbpb->bpb.media;
	*fillp++ = getbyte(wbpb->bpb.sectors_per_fat, 1);
	*fillp++ = getbyte(wbpb->bpb.sectors_per_fat, 0);
	*fillp++ = getbyte(wbpb->bpb.sectors_per_track, 1);
	*fillp++ = getbyte(wbpb->bpb.sectors_per_track, 0);
	*fillp++ = getbyte(wbpb->bpb.heads, 1);
	*fillp++ = getbyte(wbpb->bpb.heads, 0);
	*fillp++ = getbyte(wbpb->bpb.hidden_sectors, 3);
	*fillp++ = getbyte(wbpb->bpb.hidden_sectors, 2);
	*fillp++ = getbyte(wbpb->bpb.hidden_sectors, 1);
	*fillp++ = getbyte(wbpb->bpb.hidden_sectors, 0);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_logical_volume, 3);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_logical_volume, 2);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_logical_volume, 1);
	*fillp++ = getbyte(wbpb->bpb.sectors_in_logical_volume, 0);
	*fillp++ = wbpb->ebpb.phys_drive_num;
	*fillp++ = wbpb->ebpb.reserved;
	*fillp++ = wbpb->ebpb.ext_signature;
	*fillp++ = getbyte(wbpb->ebpb.volume_id, 3);
	*fillp++ = getbyte(wbpb->ebpb.volume_id, 2);
	*fillp++ = getbyte(wbpb->ebpb.volume_id, 1);
	*fillp++ = getbyte(wbpb->ebpb.volume_id, 0);

	strncpy((char *)fillp, wbpb->ebpb.volume_label, 11);
	fillp += 11;
	strncpy((char *)fillp, wbpb->ebpb.type, 8);
}

void
swap_pack_sebpbcpy(struct _boot_sector *bsp, struct _sun_bpb_extensions *sbpb)
{
	u_short *fillp;

	fillp = (u_short *) &(bsp->bs_sebpb);
	*fillp++ = htols(sbpb->bs_offset_high);
	*fillp	 = htols(sbpb->bs_offset_low);
}
#endif	/* ! i386 */

void
write_fat(int fd, char *fn, char *lbl, char *ffn, struct _bios_param_blk *wbpb,
    struct _sun_bpb_extensions *sbpb)
{
	struct _boot_sector *bsp;
	pc_cluster_t ffsc;
	u_char bootsect[BPSEC];
	u_char *fatp, *rdirp;
	ulong bootblksize, fatsize, rdirsize, ffsize;
	int bsfd = -1;
	int fffd = -1;

	bsfd = copy_bootblk(fn, bootsect, &bootblksize);
	label_volume(lbl, wbpb);

	/* Copy our BPB into bootsec structure */
	bsp = (struct _boot_sector *)bootsect;
#ifdef i386
	memcpy(&(bsp->bs_bpb), wbpb, sizeof (*wbpb));
#else
	swap_pack_bpbcpy(bsp, wbpb);
#endif

	/* Copy SUN BPB extensions into bootsec structure */
	if (SunBPBfields)
#ifdef i386
		memcpy(&(bsp->bs_sebpb), sbpb, sizeof (*sbpb));
#else
		swap_pack_sebpbcpy(bsp, sbpb);
#endif

	/* Write boot sector */
	if (!Notreally) {
		if (write(fd, bootsect, sizeof (bootsect)) != BPSEC) {
			perror("Bootsector write");
			exit(4);
		}
	}

	if (Verbose)
		printf("Building FAT.\n");
	fatp = build_fat(wbpb, bootblksize, &fatsize,
	    ffn, &fffd, &ffsize, &ffsc);

	/* Write FAT */
	if (Verbose)
		printf("Writing FAT(s). %d bytes times %d.\n",
		    fatsize, wbpb->bpb.num_fats);
	if (!Notreally) {
		int nf, wb;
		for (nf = 0; nf < (int)wbpb->bpb.num_fats; nf++)
			if ((wb = write(fd, fatp, fatsize)) != fatsize) {
				perror("FAT write");
				exit(4);
			} else {
				if (Verbose)
					printf("Wrote %d bytes\n", wb);
			}
	}

	free(fatp);

	if (Verbose)
		printf("Building root directory.\n");
	rdirp = build_rootdir(wbpb, ffn, fffd, ffsize, ffsc, &rdirsize);

	if (Verbose)
		printf("Writing root directory. %d bytes.\n", rdirsize);
	if (!Notreally) {
		if (write(fd, rdirp, rdirsize) != rdirsize) {
			perror("Root directory write");
			exit(4);
		}
	}

	free(rdirp);

	/*
	 * Now write anything that needs to be in the file space.
	 */
	if (bootblksize > BPSEC) {
		if (Verbose)
			printf("Writing remainder of boot block.\n");
		if (!Notreally)
			write_rest(wbpb, fn, fd, bsfd, bootblksize - BPSEC);
	}

	if (fffd >= 0) {
		if (Verbose)
			printf("Writing first file.\n");
		if (!Notreally)
			write_rest(wbpb, ffn, fd, fffd, ffsize);
	}
}

void
main(int argc, char **argv)
{
	struct _bios_param_blk dskparamblk;
	struct _sun_bpb_extensions sunblk;
	char *bootblkfn = NULL;
	char *diskname = NULL;
	char *firstfn = NULL;
	char *label = NULL;
	int  fd;
	int  c;

	while ((c = getopt(argc, argv, "NvSshrB:b:f:i:d:")) != EOF) {
		switch (c) {
		case 'N':
			Notreally++;
			continue;
		case 'v':
			Verbose++;
			continue;
		case 'S':
			SunBPBfields = 1;
			continue;
		case 'B':
			bootblkfn = optarg;
			continue;
		case 'f':
			diskname = optarg;
			Outputtofile = 1;
			continue;
		case 'b':
			label = optarg;
			continue;
		case 'i':
			firstfn = optarg;
			continue;
		case 'd':
			Imagesize = atoi(optarg);
			continue;
		case 'r':
			Firstfileattr |= 0x01;
			continue;
		case 'h':
			Firstfileattr |= 0x02;
			continue;
		case 's':
			Firstfileattr |= 0x04;
			continue;
		default:
			usage(argv[0], 0);
		}
	}

	if (Outputtofile && (argc - optind))
		usage(argv[0], 0);
	else if (Outputtofile && !diskname)
		usage(argv[0], 1);
	else if (!Outputtofile && (argc - optind != 1))
		usage(argv[0], 2);
	else if (SunBPBfields && !bootblkfn)
		usage(argv[0], 3);
	else if (Firstfileattr != 0x20 && !firstfn)
		usage(argv[0], 4);
	else if (Imagesize != 3 && Imagesize != 5)
		usage(argv[0], 5);

	if (!Outputtofile)
		diskname = argv[optind];

	memset(&dskparamblk, 0, sizeof (dskparamblk));
	memset(&sunblk, 0, sizeof (sunblk));

	fd = open_and_seek(diskname, &dskparamblk, &sunblk);
	write_fat(fd, bootblkfn, label, firstfn, &dskparamblk, &sunblk);
	close(fd);
	exit(0);
}
