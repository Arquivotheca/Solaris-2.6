#ifndef lint
#pragma ident "@(#)svc_updateserial.c 1.3 95/12/05 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	svc_updateserial.c
 * Group:	libspmisvc
 * Description:	This module is responsible for updating the serial
 *		number for Intel systems.
 */

#include <elf.h>
#include <fcntl.h>
#include <utime.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/systeminfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include "spmisvc_lib.h"

/* internal prototypes */

int		_setup_hostid(void);

/* private prototypes */

static long		setser(char *);
static int	srd(int, off_t, void *, u_int);
static u_long	localize_elf_half(Elf32_Half);
static u_long	localize_elf_word(Elf32_Off);
static u_long	localize_elf_off(Elf32_Off);
static u_long	x86ize_long(unsigned long);

/* constants */

#define	V1	0x38d4419a
#define	A	16807
#define	M	2147483647
#define	Q	127773
#define	R	2836
#define	x()	if ((s = ((A*(s%Q))-(R*(s/Q)))) <= 0) s += M

/*---------------------- public functions -----------------------*/

/*
 * Function:	_setup_hostid
 * Description:	Set the hostid on any system supporting the i386 model of
 *		hostids.
 * Scope:	internal
 * Parameters:	none
 * Return:	NOERR	- set successful
 *		ERROR	- set failed
 */
int
_setup_hostid(void)
{
	char	buf[32] = "";
	char	orig[64] = "";
	char	path[MAXPATHLEN] = "";

	/* cache client hostids are set by hostmanager */
	if (get_machinetype() == MT_CCLIENT)
		return (NOERR);

	/* take no action when running dry-run */
	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	(void) sprintf(orig, "/tmp/root%s", IDKEY);
	(void) sprintf(path, "%s%s", get_rootdir(), IDKEY);

	/* only set if the original was not saved */
	if (access(orig, F_OK) < 0 &&
			access(path, F_OK) == 0 &&
			(sysinfo(SI_HW_SERIAL, buf, 32) < 0 ||
				buf[0] == '0')) {
		if (setser(path) < 0)
			return (ERROR);
	}

	return (NOERR);
}

/*---------------------- private functions -----------------------*/

/*
 * Function:	setser
 * Description: Generate a hardware serial number in the range 1 to (10**9-1)
 *		and sets appropriate constants in the sysinit module with name
 *		fn.
 * Scope:	private 
 * Parameters:	fn	- module file name
 * Return:	# >= 0	- random serial number
 *		 -1	- system call error
 *		 -2	- logic error
 */
static long
setser(char *fn)
{
	Elf32_Ehdr Ehdr;
	Elf32_Shdr Shdr;
	int fd;
	int rc;
	char name[6];
	off_t offset;
	off_t shstrtab_offset;
	off_t data_offset;
	int i;
	long t[3];
	long s;
	struct timeval tv;
	struct stat statbuf;
	struct utimbuf utimbuf;

	rc = -1;	/* default return code for system call error */

	/* open the module file */
	if ((fd = open(fn, O_RDWR)) < 0)
		return (rc);

	/* get file status for times */
	if (fstat(fd, &statbuf) < 0)
		goto out;

	/* read the elf header */
	offset = 0;
	if (srd(fd, offset, &Ehdr, sizeof (Ehdr)) < 0)
		goto out;

	/* read the section header for the section string table */
	offset = localize_elf_off(Ehdr.e_shoff) +
	    (localize_elf_half(Ehdr.e_shstrndx) *
	    localize_elf_half(Ehdr.e_shentsize));
	if (srd(fd, offset, &Shdr, sizeof (Shdr)) < 0)
		goto out;

	/* save the offset of the section string table */
	shstrtab_offset = localize_elf_off(Shdr.sh_offset);

	/* find the .data section header */
	/*CSTYLED*/
	for (i = 1; ; ) {
		offset = localize_elf_off(Ehdr.e_shoff) +
		    (i * localize_elf_half(Ehdr.e_shentsize));
		if (srd(fd, offset, &Shdr, sizeof (Shdr)) < 0)
			goto out;
		offset = shstrtab_offset + localize_elf_word(Shdr.sh_name);
		if (srd(fd, offset, name, sizeof (name)) < 0)
			goto out;
		if (strcmp(name, ".data") == 0)
			break;
		if (++i >= localize_elf_half(Ehdr.e_shnum)) {
			/* reached end of table */
			rc = -2;
			goto out;
		}
	}

	/* save the offset of the data section */
	data_offset = localize_elf_off(Shdr.sh_offset);

	/* read and check the version number */
	offset = data_offset;
	if (srd(fd, offset, &t[0], sizeof (t[0])) < 0)
		goto out;
	if (t[0] != x86ize_long(V1)) {
		rc = -2;
		goto out;
	}

	/* generate constants and serial number */
	(void) gettimeofday(&tv, (void *)NULL);
	s = tv.tv_sec + tv.tv_usec - (22*365*24*60*60);
	do {
		x();
		t[1] = x86ize_long(s);
		x();
		t[2] = x86ize_long(s);
		x();
		s %= 1000000000;
	} while (s == 0);

	/* store constants */
	offset = data_offset + sizeof (t[0]);
	if (lseek(fd, offset, SEEK_SET) < 0)
		goto out;
	if (write(fd, &t[1], (2 * sizeof (t[1]))) < 0)
		goto out;

	(void) close(fd);	/* close module file */

	/* restore file access and modification times */
	utimbuf.actime = statbuf.st_atime;
	utimbuf.modtime = statbuf.st_mtime;
	if (utime(fn, &utimbuf) < 0)
		return (-1);

	return (s);	/* return serial number */

	/* close file and return error code */
out:    (void) close(fd);
	return (rc);
}

/*
 * Function:	localize_elf_half
 * Description: Convert data type "Elf32_Half" from little-endian format to
 *		local byte order.
 * Scope:	private
 * Parameters:	x	- the value to be converted
 * Return:	#	- the value of x as interpreted by little-endian machines
 */
static u_long
localize_elf_half(Elf32_Half x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8));
}

/*
 * Function:	localize_elf_word
 * Description: Convert data type "Elf32_Half" from little-endian format to
 *		local byte order
 * Scope:	private
 * Parameters:	x	- the value to be converted
 * Return:	#	- the value of x as interpreted by little-endian machines
 */
static u_long
localize_elf_word(Elf32_Off x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

/*
 * Function:	localize_elf_off
 * Description:	Convert data type "Elf32_Half" from little-endian format
 *		to local byte order.
 * Scope:	private
 * Parameters:	x	- the value to be converted
 * Return:	#	- the value of x as interpreted by little-endian machines
 */
static u_long
localize_elf_off(Elf32_Off x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

/*
 * Function:	x86ize_long
 * Description:	Convert data type "long" from local format to little-endian
 *		byte order
 * Scope:	private
 * Parameters:	x	- the value to be converted (in local byte order)
 * Return:	#	- x in little-endian byte order
 */
static u_long
x86ize_long(u_long x)
{
	u_long	y;
	u_char	*p = (u_char *)&y;

	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
	return (y);
}

/*
 * Function:	srd
 * Description: Seek to a designated offset in the specified file and
 *		read 'nbyte' bytes into the buffer supplied.
 * Scope:	private
 * Parameters:	fd	- file descriptor for serial module
 *		offset	- offset in 'fd' in which to read
 *		buf	- buffer to read into
 *		nbyte	- number of bytes to read
 * Return:	#	- number of bytes actually read
 */
static int
srd(int fd, off_t offset, void *buf, u_int nbyte)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
		return (-1);

	return (read(fd, buf, nbyte));
}
