#pragma ident	"@(#)syslog.c	1.7	96/08/01 SMI"  /* from UCB 5.9 5/7/86 */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by /etc/syslogd.
 *
 * Author: Eric Allman
 * Modified to use UNIX domain IPC by Ralph Campbell
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <netdb.h>
#include <strings.h>
#include <varargs.h>
#include <vfork.h>
#include <stdio.h>

#define	MAXLINE	1024			/* max message size */
#define	NULL	0			/* manifest */

#define	PRIMASK(p)	(1 << ((p) & LOG_PRIMASK))
#define	PRIFAC(p)	(((p) & LOG_FACMASK) >> 3)
#define	IMPORTANT 	LOG_ERR

static char	*logname = "/dev/log";
static char	*ctty = "/dev/console";

static struct _syslog {
	int	_LogFile;
	int	_LogStat;
	char	*_LogTag;
	int	_LogMask;
	struct 	sockaddr _SyslogAddr;
	char	*_SyslogHost;
	int	_LogFacility;
} *_syslog;
#define	LogFile (_syslog->_LogFile)
#define	LogStat (_syslog->_LogStat)
#define	LogTag (_syslog->_LogTag)
#define	LogMask (_syslog->_LogMask)
#define	SyslogAddr (_syslog->_SyslogAddr)
#define	SyslogHost (_syslog->_SyslogHost)
#define	LogFacility (_syslog->_LogFacility)

extern	int errno;

extern char *calloc();
extern char *strerror(int);
extern time_t time();
extern unsigned int alarm();

static int
allocstatic()
{
	_syslog = (struct _syslog *)calloc(1, sizeof (struct _syslog));
	if (_syslog == 0)
		return (0);	/* can't do it */
	LogFile = -1;		/* fd for log */
	LogStat	= 0;		/* status bits, set by openlog() */
	LogTag = "syslog";	/* string to tag the entry with */
	LogMask = 0xff;		/* mask of priorities to be logged */
	LogFacility = LOG_USER;	/* default facility code */
	return (1);
}

/*VARARGS2*/
syslog(pri, fmt, va_alist)
	int pri;
	char *fmt;
	va_dcl
{
	va_list ap;

	va_start(ap);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

vsyslog(pri, fmt, ap)
	int pri;
	char *fmt;
	va_list ap;
{
	char buf[MAXLINE + 1], outline[MAXLINE + 1];
	register char *b, *f, *o;
	register int c;
	long now;
	int pid, olderrno = errno;
	int retsiz, outsiz = MAXLINE + 1;

	if (_syslog == 0 && !allocstatic())
		return;
	/* see if we should just throw out this message */
	if (pri <= 0 || PRIFAC(pri) >= LOG_NFACILITIES ||
	    (PRIMASK(pri) & LogMask) == 0)
		return;
	if (LogFile < 0)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* build the message */
	o = outline;
	retsiz = snprintf(o, outsiz, "<%d>", pri);
	outsiz -= retsiz;
	o += retsiz;
	(void) time(&now);
	retsiz = snprintf(o, outsiz, "%.15s ", ctime(&now) + 4);
	outsiz -= retsiz;
	o += retsiz;
	if (LogTag) {
		retsiz = snprintf(o, outsiz, "%s", LogTag);
		outsiz -= retsiz;
		if (outsiz < 0)
			outsiz = 0;
		o += retsiz;
	}
	if (LogStat & LOG_PID) {
		retsiz = snprintf(o, outsiz, "[%d]", getpid());
		outsiz -= retsiz;
		if (outsiz < 0)
			outsiz = 0;
		o += strlen(o);
	}
	if (LogTag && (outsiz > 2)) {
		(void) strcpy(o, ": ");
		outsiz -= 2;
		if (outsiz < 0)
			outsiz = 0;
		o += 2;
	}

	b = buf;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && b < &buf[MAXLINE]) {
		char *errstr;

		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}
		if ((errstr = strerror(olderrno)) == NULL)
			(void) sprintf(b, "error %d", olderrno);
		else
			(void) strcpy(b, errstr);
		b += strlen(b);
	}
	*b++ = '\n';
	*b = '\0';
	retsiz = vsnprintf(o, outsiz, buf, ap);	/* copy resulting string */
	o += retsiz;
	outsiz -= retsiz;
	c = o - outline;
	if (c > MAXLINE)
		c = MAXLINE;

	/* output the message to the local logger */
	if (sendto(LogFile, outline, c, 0, &SyslogAddr,
	    sizeof (SyslogAddr)) >= 0)
		return;
	if (!(LogStat & LOG_CONS))
		return;

	/* output the message to the console */
	pid = vfork();
	if (pid == -1)
		return;
	if (pid == 0) {
		int fd;

		(void) signal(SIGALRM, SIG_DFL);
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGALRM));
		(void) alarm(5);
		fd = open(ctty, O_WRONLY);
		(void) alarm(0);
		if (outsiz > 2) {	/* Just in case */
			(void) strcat(o, "\r\n");
			c += 2;
		}
		o = index(outline, '>') + 1;
		(void) write(fd, o, c - (o - outline));
		(void) close(fd);
		_exit(0);
	}
	if (!(LogStat & LOG_NOWAIT))
		while ((c = wait((int *)0)) > 0 && c != pid)
			;
}

/*
 * OPENLOG -- open system log
 */

openlog(ident, logstat, logfac)
	char *ident;
	int logstat, logfac;
{
	if (_syslog == 0 && !allocstatic())
		return;
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0)
		LogFacility = logfac & LOG_FACMASK;
	if (LogFile >= 0)
		return;
	SyslogAddr.sa_family = AF_UNIX;
	(void) strncpy(SyslogAddr.sa_data, logname,
	    sizeof (SyslogAddr.sa_data));
	if (LogStat & LOG_NDELAY) {
		LogFile = socket(AF_UNIX, SOCK_DGRAM, 0);
		(void) fcntl(LogFile, F_SETFD, 1);
	}
}

/*
 * CLOSELOG -- close the system log
 */

closelog()
{

	if (_syslog == 0)
		return;
	(void) close(LogFile);
	LogFile = -1;
}

/*
 * SETLOGMASK -- set the log mask level
 */
setlogmask(pmask)
	int pmask;
{
	int omask;

	if (_syslog == 0 && !allocstatic())
		return (-1);
	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}

/*
 * snprintf/vsnprintf -- These routines are here
 * temporarily to solve bugid 1220257. Perhaps
 * they could become a public interface at some
 * point but not for now.
 */

extern int _doprnt();

/*VARARGS3*/
static int
snprintf(string, n, format, va_alist)
char *string, *format;
size_t n;
va_dcl
{
	register int count;
	FILE siop;
	va_list ap;

	if (n == 0)
		return (0);
	siop._cnt = n - 1;
	siop._base = siop._ptr = (unsigned char *)string;
	siop._flag = _IOWRT+_IOSTRG;
	va_start(ap);
	count = _doprnt(format, ap, &siop);
	va_end(ap);
	*siop._ptr = '\0';	/* plant terminating null character */
	return (count);
}

/*VARARGS3*/
static int
vsnprintf(string, n, format, ap)
char *string, *format;
size_t n;
va_list ap;
{
	register int count;
	FILE siop;

	if (n == 0)
		return (0);
	siop._cnt = n - 1;
	siop._base = siop._ptr = (unsigned char *)string;
	siop._flag = _IOWRT+_IOSTRG;
	count = _doprnt(format, ap, &siop);
	*siop._ptr = '\0';	/* plant terminating null character */
	return (count);
}
