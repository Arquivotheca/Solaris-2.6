#ifndef lint
#pragma ident "@(#)atconfig.c 1.1 95/07/26"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include <sys/wait.h>
#include <varargs.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>
#include <elf.h>

/* magic values used by the sysinit module */
#define	V1	0x38d4419a
#define	V1_K1   0x7a5fd043
#define	V1_K2   0x65cb612e

/* Local Function Prototypes */
static int srd(int, off_t, void *, u_int);
static int ser_exists(char *fn);
static int tryunmount(char *dir);
static int trymount(char *dev, char *dir);
static int run();
void tolog(char *str);
extern void exit(int val);

/*
 * some install-specific things:
 *	OLDROOT is a directory we can use as a temporary mount point
 *	SAVEPLACE is where we'll squirrel away the sysinit module
 *	ROOTPROG is a program who output tells us the name of the root device
 *	INITFILE[12] are the possible sysinit modules we'll try to copy
 *	LOGFILE is where we'll append all our output
 */
#define	OLDROOT "/a"
#define	SAVEPLACE "/tmp/root/kernel/misc"
#define	ROOTPROG "/usr/sbin/install.d/rootdev"
#define	INITFILE1 OLDROOT "/.atconfig"
#define	INITFILE2 OLDROOT "/kernel/misc/sysinit"
#define	LOGFILE "/dev/null"

/* maximum expected command line/pathname */
#define	MAXLINE	1024

/*
 *	main -- the atconfig command
 *
 *	this program does pretty much the same thing as the old
 *	atconfig script except that it checks for the sysinit module
 *	in two places (INITFILE1 and INITFILE2) instead of one and
 *	it refuses to copy a sysinit module that hasn't been patched.
 *
 *	note: any failures along the way just cause us to quietly exit.
 */

main(int argc, char *argv[])
{
	char rootdev[MAXLINE];	/* output from "rootdev" command */
	int len;		/* length of rootdev's output */
	FILE *p;		/* pipe from rootdev command */

	if (argc != 1) {
		(void) fprintf(stderr, "usage: %s\n", argv[0]);
		exit(1);
	}

	/* make the save place to hold the sysinit module */
	if ((mkdirp(SAVEPLACE, 0777) < 0) && (access(SAVEPLACE, F_OK) < 0)) {
		tolog("mkdirp failed");
		exit(1);
	}

	/* get the root dev and create the raw dev name from it */
	if ((p = popen(ROOTPROG " 2>> " LOGFILE, "r")) == NULL) {
		tolog("popen failed");
		exit(1);
	}

	/* take the output of the rootdev command */
	if (fgets(rootdev, MAXLINE, p) == NULL) {
		tolog("fgets failed");
		exit(1);
	}

	(void) pclose(p);

	/* make sure we got something back */
	if ((len = strlen(rootdev)) <= 1) {
		tolog("short rootdev");
		exit(1);
	}

	/* chop off the newline */
	rootdev[len - 1] = '\0';

	/* see if we can mount the old root filesystem */
	if (trymount(rootdev, OLDROOT)) {
		/* got it mounted, check for sysinit modules */
		if (ser_exists(INITFILE1)) {
			tolog("copy" INITFILE1);
			(void) run("cp %s %s", INITFILE1, SAVEPLACE);
		} else if (ser_exists(INITFILE2)) {
			tolog("copy" INITFILE2);
			(void) run("cp %s %s", INITFILE2, SAVEPLACE);
		}

		(void) tryunmount(OLDROOT);
	} else
		tolog("trymount failed");

	exit(0);
	/*NOTREACHED*/
}

/*
 *	trymount -- try to mount a ufs filesystem read-only
 *
 *	if the mount fails the first time, this routine runs fsck
 *	and tries it again.  zero is returned on success, the exit
 *	value from the mount command on failure.
 *
 *	in order to make this routine act like the old atconfig.sh script,
 *	it prints "Stand by..." when running the fsck.
 */

static int
trymount(char *dev, char *dir)
{
	char rawdev[MAXLINE];
	char *dskptr;
	char *srcptr;
	char *dstptr;

	tolog(dev);
	/* try to mount the "old" root */
	if (run("mount -F ufs %s %s", dev, dir)) {
		/* change "dsk" to "rdsk" */
		if ((dskptr = strstr(dev, "dsk")) == NULL)
			return (0);

		srcptr = dev;
		dstptr = rawdev;

		/* copy stuff before the word "dsk" */
		while (srcptr < dskptr)
			*dstptr++ = *srcptr++;

		/* add the 'r' */
		*dstptr++ = 'r';

		/* copy the rest */
		while (*dstptr++ = *srcptr++)
			;

		/* non-zero exit value means mount failed.  try harder. */
		(void) printf("Stand by...\n");
		(void) fflush(stdout);
		tolog(rawdev);
		(void) run("fsck -y %s", rawdev);

		if (run("mount -F ufs -o ro %s %s", dev, dir)) {
			tolog("give up mount");
			return (0);	/* still couldn't mount it, give up */
		}
	}

	return (1);
}

/*
 *	tryunmount -- attempt to un-mount the given directory
 *
 *	returns the exit value from the umount command.
 */

static int
tryunmount(char *dir)
{
	return (run("umount %s", dir));
}

/*
 *	run -- run a command, logging output to LOGFILE, return exit value
 *
 *	this routine takes a command to run in printf-style (i.e. a format
 *	string followed by a variable number of args).  we didn't use
 *	system() to runs things since it is not guaranteed to give us the
 *	exit value of the command.
 *
 *	the command run by this routine gets /dev/null for stdin and
 *	appends to LOGFILE for stdout and stderr.
 */

/*VARARGS*/
static int
run(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	char cmd[MAXLINE];
	int pid;
	int wstat;
	int fd;

	va_start(args);
	fmt = va_arg(args, char *);

	if (fmt == NULL)
		return (1);	/* error return */

	(void) vsprintf(cmd, fmt, args);
	va_end(args);

	tolog(cmd);

	if ((pid = fork()) < 0)
		return (1);
	else if (pid) {
		/* parent */
		if (waitpid(pid, &wstat, 0) < 0) {
			tolog("waitpid failed");
			return (1);
		}
		if (WIFEXITED(wstat)) {
			sprintf(cmd, "child had exit status %d",
			    WEXITSTATUS(wstat));
			tolog(cmd);
			return (WEXITSTATUS(wstat));
		} else {
			tolog("child died status");
			return (1);	/* must have died with some error */
		}
	} else {
		/* child */
		if ((fd = open("/dev/null", O_RDONLY)) >= 0) {
			(void) dup2(fd, 0);	/* stdin is /dev/null */
			(void) close(fd);
		}

		if ((fd = open(LOGFILE, O_RDWR|O_APPEND)) >= 0) {
			(void) dup2(fd, 1);	/* stdout/stderr is LOGFILE */
			(void) dup2(fd, 2);
			(void) close(fd);
		}

		(void) execlp("sh", "sh", "-c", cmd, 0);
		_exit(1);
	}
	/*NOTREACHED*/
}

/*
 *	ser_exists -- returns true if fn exists & contains a hostid seed
 *
 *	this routine is just a slightly modified version of the set_ser()
 *	routine used by the install to set the hostid seed.
 */

static int
ser_exists(char *fn)
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

	rc = 0;	/* assume module doesn't exist */

	/* open the module file */
	if ((fd = open(fn, O_RDONLY)) < 0) {
		tolog("couldn't open module");
		return (rc);
	}

	/* read the elf header */
	offset = 0;
	if (srd(fd, offset, &Ehdr, sizeof (Ehdr)) < 0) {
		tolog("couldn't read elf header");
		goto out;
	}

	/* read the section header for the section string table */
	offset = Ehdr.e_shoff + (Ehdr.e_shstrndx * Ehdr.e_shentsize);
	if (srd(fd, offset, &Shdr, sizeof (Shdr)) < 0) {
		tolog("couldn't read sections");
		goto out;
	}

	/* save the offset of the section string table */
	shstrtab_offset = Shdr.sh_offset;

	/* find the .data section header */
	/*CSTYLED*/
	for (i = 1; ; ) {
		offset = Ehdr.e_shoff + (i * Ehdr.e_shentsize);
		if (srd(fd, offset, &Shdr, sizeof (Shdr)) < 0) {
			tolog("srd error 1");
			goto out;
		}
		offset = shstrtab_offset + Shdr.sh_name;
		if (srd(fd, offset, name, sizeof (name)) < 0) {
			tolog("srd error 2");
			goto out;
		}
		if (strcmp(name, ".data") == 0)
			break;
		if (++i >= (int)Ehdr.e_shnum) {
			/* reached end of table */
			tolog("end of elf table");
			goto out;
		}
	}

	/* save the offset of the data section */
	data_offset = Shdr.sh_offset;

	/* read and check the version number and initial seed values */
	offset = data_offset;
	if (srd(fd, offset, &t[0], sizeof (t[0]) * 3) < 0) {
		tolog("srd error 3");
		goto out;
	}
	if (t[0] != V1) {
		static char buf[MAXLINE];
		sprintf(buf, "bad ver: v1=%x k1=%x k2=%x\n", V1, V1_K1, V1_K2);
		tolog(buf);
		goto out;
	}
	if ((t[1] == V1_K1) || (t[2] == V1_K2)) {
		static char buf[MAXLINE];
		sprintf(buf, "zero hostid: v1=%x k1=%x k2=%x\n",
		    V1, V1_K1, V1_K2);
		tolog(buf);
		goto out;
	}

	(void) close(fd);	/* close module file */

	return (1);	/* passed all our checks... return true */

	/* close file and return error code */
out:    (void) close(fd);
	return (rc);
}

/*
 *	srd -- seek to offset and read
 *
 *	this routine came from set_ser.c also.  it has not been modified.
 */
static int
srd(int fd, off_t offset, void *buf, u_int nbyte)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
		return (-1);

	return (read(fd, buf, nbyte));
}

void
tolog(char *str)
{
	static FILE *fp = NULL;

	if (fp == NULL)
		fp = fopen(LOGFILE, "w");

	if (fp != NULL) {
		fprintf(fp, "atconfig: %s\n", str);
		fflush(fp);
	}
}
