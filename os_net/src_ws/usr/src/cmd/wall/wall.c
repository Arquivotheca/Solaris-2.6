/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)wall.c	1.11	95/07/13 SMI"	/* SVr4.0 1.13.1.7	*/

#define	FAILURE	(-1)
#define	TRUE	1
#define	FALSE	0

#include <signal.h>

char	mesg[3000];

#include <stdio.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utmp.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <locale.h>
#include <syslog.h>

int	entries;
extern	int errno;
extern	char *sys_errlist[];
char	*infile;
int	group;
struct	group *pgrp;
char	*grpname;
char	line[MAXNAMLEN+1] = "???";
char	*systm;
long	tloc;
unsigned int usize;
struct	utmp *utmp;
struct	utsname utsn;
char	who[9]	= "???";
static char time_buf[50];
int	aflag;
#define DATE_FMT	"%a %b %e %H:%M:%S"

#define equal(a,b)		(!strcmp( (a), (b) ))

main(argc, argv)
int	argc;
char	*argv[];
{
	int	i=0, fd;
	register	struct utmp *p;
	FILE	*f;
	struct	stat statbuf;
	register	char *ptr;
	struct	passwd *pwd;
	char	*term_name;
	void readargs(), sendmes();

	(void)setlocale(LC_ALL, "");

	if(uname(&utsn) == -1) {
		fprintf(stderr, "wall: uname() failed, %s\n", sys_errlist[errno]);
		exit(2);
	}
	systm = utsn.nodename;

	if(stat(UTMP_FILE, &statbuf) < 0) {
		fprintf(stderr, "stat failed on %s\n", UTMP_FILE);
		exit(1);
	}
	/*
		get usize (an unsigned int) for malloc call
 		and check that there is no truncation (for those 16 bit CPUs)
 	*/
	usize = statbuf.st_size;
	if(usize != statbuf.st_size) {
		fprintf(stderr, "'%s' too big.\n", UTMP_FILE);
		exit(1);
	}
	entries = usize / sizeof(struct utmp);
	if((utmp=(struct utmp *)malloc(usize)) == NULL) {
		fprintf(stderr, "cannot allocate memory for '%s'.\n", UTMP_FILE);
		exit(1);
	}

	if((fd=open(UTMP_FILE, O_RDONLY)) < 0) {
		fprintf(stderr,"cannot open '%s'\n", UTMP_FILE);
		exit(1);
	}
	if(read(fd, (char *) utmp, usize) != usize) {
		fprintf(stderr, "cannot read '%s'\n", UTMP_FILE);
		exit(1);
	}
	close(fd);
	readargs(argc, argv);

	/*
		Get the name of the terminal wall is running from.
	*/

	if ((term_name = ttyname(fileno(stderr))) != NULL)
	/*
		skip the leading "/dev/" in term_name
	*/
		strncpy(line, &term_name[5], sizeof(line) - 1);
	if (who[0] == '?') {
		if (pwd = getpwuid(getuid()))
			strncpy(&who[0],pwd->pw_name,sizeof(who));
	}

	f = stdin;
	if(infile) {
		f = fopen(infile, "r");
		if(f == NULL) {
			fprintf(stderr,"%s??\n", infile);
			exit(1);
		}
	}
	for(ptr= &mesg[0]; fgets(ptr,&mesg[sizeof(mesg)]-ptr, f) != NULL
		; ptr += strlen(ptr));
	fclose(f);
	time(&tloc);
	cftime(time_buf, DATE_FMT,&tloc);
	for(i=0;i<entries;i++) {
		if((p=(&utmp[i]))->ut_type != USER_PROCESS) continue;
		/*
		 * if (-a option OR NOT pty window login), send the message
		 */
		if (aflag || !nonuser(*p))
			sendmes(p);
	}
	alarm(60);
	do {
		i = (int)wait((int *)0);
	} while(i != -1 || errno != ECHILD);
	exit(0); /* NOTREACHED */
}

void
sendmes(p)
struct utmp *p;
{
	register i;
	register char *s;
	static char device[] = "/dev/123456789012";
	register char *bp;
	int ibp;
	FILE *f;
	int fd;

	if(group)
		if(!chkgrp(p->ut_user))
			return;
	while((i=(int)fork()) == -1) {
		alarm(60);
		wait((int *)0);
		alarm(0);
	}

	if(i)
		return;

	signal(SIGHUP, SIG_IGN);
	alarm(60);
	s = &device[0];
	sprintf(s, "/dev/%.*s", sizeof(p->ut_line), p->ut_line);

	/* check if the device is really a tty */
	if ((fd = open(s, O_WRONLY|O_NOCTTY|O_NONBLOCK)) == -1) {
		fprintf(stderr,"Cannot send to %.*s on %s\n",
			sizeof(p->ut_user), p->ut_user, s);
		perror("open");
		exit(1);
	} else {
		if (!isatty(fd)) {
			fprintf(stderr, "Cannot send to device %.*s %s\n",
				sizeof(p->ut_line), p->ut_line,
				"because it's not a tty");
			openlog("wall", 0, LOG_AUTH);
			syslog(LOG_CRIT, "%.*s in utmp is not a tty\n",
				sizeof(p->ut_line), p->ut_line);
			closelog();
			exit(1);
		}
	}
#ifdef DEBUG
	close(fd);
	f = fopen("wall.debug", "a");
#else
	f = fdopen(fd, "w");
#endif
	if(f == NULL) {
		fprintf(stderr,"Cannot send to %.-8s on %s\n", &p->ut_user[0],s);
		perror("open");
		exit(1);
	}
	fprintf(f, "\07\07\07Broadcast Message from %s (%s) on %s %19.19s",
		 who, line, systm, time_buf);
	if(group)
		fprintf(f, " to group %s", grpname);
	fprintf(f, "...\n");
#ifdef DEBUG
	fprintf(f,"DEBUG: To %.8s on %s\n", p->ut_user, s);
#endif
	i = strlen(mesg);
	for (bp = mesg; --i >= 0; bp++) {
		ibp = (unsigned int)((unsigned char) *bp);
		if (*bp == '\n')
			putc('\r', f);
		if (isprint(ibp) || *bp == '\r' || *bp == '\013' ||
                    *bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\007') {
			putc(*bp, f);
		} else {
			if (!isascii(*bp)) {
                                fputs("M-", f);
                                *bp = toascii(*bp);
			}
			if (iscntrl(*bp)) {
                                putc('^', f);
                                putc(*bp + 0100, f);
			}
			else
                                putc(*bp, f);
		}

		if (*bp == '\n')
			fflush(f);

		if (ferror(f) || feof(f)) {
			printf("\n\007Write failed\n");
			exit(1);
		}
	}
	fclose(f);
	close(fd);
	exit(0);
}

void
readargs(ac, av)
int ac;
char **av;
{
	register int i;

	for(i = 1; i < ac; i++) {
		if(equal(av[i], "-g")) {
			if(group) {
				fprintf(stderr, "Only one group allowed\n");
				exit(1);
			}
			i++;
			if((pgrp=getgrnam(grpname= av[i])) == NULL) {
				fprintf(stderr, "Unknown group %s\n", grpname);
				exit(1);
			}
			endgrent();
			group++;
		} else if (equal(av[i],"-a")) {
			i++;
			aflag++;
		} else
			infile = av[i];
	}
}
#define BLANK		' '
chkgrp(name)
register char *name;
{
	register int i;
	register char *p;

	for(i = 0; pgrp->gr_mem[i] && pgrp->gr_mem[i][0]; i++) {
		for(p=name; *p && *p != BLANK; p++);
		*p = 0;
		if(equal(name, pgrp->gr_mem[i]))
			return(1);
	}

	return(0);
}
