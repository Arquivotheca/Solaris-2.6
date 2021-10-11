#ifndef lint
#pragma ident "@(#)setser.c 1.16 95/03/20 SMI"
#endif
/*
 * Copyright (c) 1992-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>
#include <elf.h>

#define	V1	0x38d4419a

#define	A	16807
#define	M	2147483647
#define	Q	127773
#define	R	2836
#define	x()	if ((s = ((A*(s%Q))-(R*(s/Q)))) <= 0) s += M

/* Externals and Globals */

extern int	gettimeofday();

/* Public Function Prototypes */

long		setser(char *);

/* Local Function Prototypes */

static int	srd(int, off_t, void *, u_int);
static u_long	localize_elf_half(Elf32_Half);
static u_long	localize_elf_word(Elf32_Off);
static u_long	localize_elf_off(Elf32_Off);
static u_long	x86ize_long(unsigned long);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * setser()
 *	Generate a hardware serial number in the range 1 to (10**9-1)
 *	and sets appropriate constants in the sysinit module with name fn.
 * Parameters:
 *	fn	- module file name
 * Return:
 *	# >= 0	- random serial number
 *	 -1	- system call error
 *	 -2	- logic error
 * Status:
 *	public
 */
long
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

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */

/*
 * srd()
 *	Seek to a designated offset in the specified file and
*	read 'nbyte' bytes into the buffer supplied.
 * Parameters:
 *	fd	- file descriptor for serial module
 *	offset	- offset in 'fd' in which to read
 *	buf	- buffer to read into
 *	nbyte	- number of bytes to read
 * Return:
 *	#	- number of bytes actually read
 * Status:
 *	private
 */
static int
srd(int fd, off_t offset, void *buf, u_int nbyte)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
		return (-1);

	return (read(fd, buf, nbyte));
}

/*
 * localize_elf_half()
 *	Convert data type "Elf32_Half" from little-endian format to local byte
*	order.
 * Parameters:
 *	x	- the value to be converted
 * Return:
 *	#	- the value of x as interpreted by little-endian machines
 * Status:
 *	private
 */
static u_long
localize_elf_half(Elf32_Half x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8));
}

/*
 * localize_elf_word()
 *	convert data type "Elf32_Half" from little-endian format to local byte
 *	order
 * Parameters:
 *	x	- the value to be converted
 * Return:
 *	#	- the value of x as interpreted by little-endian machines
 * Status:
 *	private
 */
static u_long
localize_elf_word(Elf32_Off x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

/*
 * localize_elf_off()
 *	convert data type "Elf32_Half" from little-endian format to local byte
 *	order
 * Parameters:
 *	x	- the value to be converted
 * Return:
 *	#	- the value of x as interpreted by little-endian machines
 * Status:
 *	private
 */
static u_long
localize_elf_off(Elf32_Off x)
{
	u_char	*p = (u_char *)&x;

	return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

/*
 * x86ize_long()
 *	convert data type "long" from local format to little-endian byte order
 * Parameters:
 *	x	- the value to be converted (in local byte order)
 * Return:
 *	#	- x in little-endian byte order
 * Status:
 *	private
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
