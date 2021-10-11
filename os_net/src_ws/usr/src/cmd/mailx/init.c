/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)init.c	1.8	95/07/19 SMI"	/* from SVr4.0 1.2.1.3 */

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * A bunch of global variable declarations lie herein.
 *
 * All global externs are declared in def.h. All variables are initialized
 * here!
 *
 * !!!!!IF YOU CHANGE (OR ADD) IT HERE, DO IT THERE ALSO !!!!!!!!
 *
 */

#include	"def.h"
#include	<grp.h>
#include	<pwd.h>
#include	<utmp.h>
#include	<sys/utsname.h>

int	Fflag = 0;			/* -F option (followup) */
int	Hflag = 0;			/* print headers and exit */
char	*Tflag;				/* -T temp file for netnews */
int	UnUUCP = 0;			/* -U flag */
char	**altnames;			/* List of alternate names for user */
int	askme;
int	baud;				/* Output baud rate */
char	*bflag;				/* Bcc given from non tty */
char	*binmsg = "*** Message content is not printable: pipe to command or save to a file ***";
char	*cflag;				/* Cc given from non tty */
int	cond;				/* Current state of conditional exc. */
NODE	*curptr = NOFP;
int	debug;				/* Debug flag set */
struct	message	*dot;			/* Pointer to current message */
int	edit;				/* Indicates editing a file */
char	*editfile;			/* Name of file being edited */
int	exitflg = 0;			/* -e for mail test */
NODE	*fplist = NOFP;
struct	grouphead	*groups[HSHSIZE];/* Pointer to active groups */
int	hflag;				/* Sequence number for network -h */
char	homedir[PATHSIZE];		/* Name of home directory */
struct	ignore		*ignore[HSHSIZE];/* Pointer to ignored fields */
int	image;				/* File descriptor for image of msg */
FILE	*input;				/* Current command input file */
int	intty;				/* True if standard input a tty */
int	issysmbox;			/* mailname is a system mailbox */
FILE	*itf;				/* Input temp file buffer */
int	lexnumber;			/* Number of TNUMBER from scan() */
char	lexstring[STRINGLEN];		/* String from TSTRING, scan() */
int	loading;			/* Loading user definitions */
char	*lockname;			/* named used for locking in /var/mail */
#ifdef	USR_SPOOL_MAIL
char	*maildir = "/usr/spool/mail/";	/* directory for mail files */
#else
# ifdef preSVr4
char	*maildir = "/usr/mail/";	/* directory for mail files */
# else
char	*maildir = "/var/mail/";	/* directory for mail files */
# endif
#endif
char	mailname[PATHSIZE];		/* Name of /var/mail system mailbox */
off_t	mailsize;			/* Size of system mailbox */
int	maxfiles;			/* Maximum number of open files */
struct	message	*message;		/* The actual message structure */
int	msgCount;			/* Count of messages read in */
gid_t	myegid;
uid_t	myeuid;
char	myname[PATHSIZE];		/* My login id */
pid_t	mypid;				/* Current process id */
gid_t	myrgid;
uid_t	myruid;
int	newsflg = 0;			/* -I option for netnews */
char	noheader;			/* Suprress initial header listing */
int	noreset;			/* String resets suspended */
char	nosrc;				/* Don't source /etc/mail/mailx.rc */
int	nretained;			/* Number of retained fields */
int	numberstack[REGDEP];		/* Stack of regretted numbers */
char	origname[PATHSIZE];		/* Name of mailfile before expansion */
FILE	*otf;				/* Output temp file buffer */
int	outtty;				/* True if standard output a tty */
FILE	*pipef;				/* Pipe file we have opened */
char	*progname;			/* program name (argv[0]) */
char	*prompt = NOSTR;		/* prompt string */
int	rcvmode;			/* True if receiving mail */
int	readonly;			/* Will be unable to rewrite file */
int	regretp;			/* Pointer to TOS of regret tokens */
int	regretstack[REGDEP];		/* Stack of regretted tokens */
struct	ignore		*retain[HSHSIZE];/* Pointer to retained fields */
char	*rflag;				/* -r address for network */
int	rmail;				/* Being called as rmail */
int	sawcom;				/* Set after first command */
int	selfsent;			/* User sent self something */
int	senderr;			/* An error while checking */
char	*sflag;				/* Subject given from non tty */
int	sourcing;			/* Currently reading variant file */
int	space;				/* Current maximum number of messages */
jmp_buf	srbuf;
int	tflag;				/* Read headers from text */
/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */
struct strings stringdope[NSPACE];
char	*stringstack[REGDEP];		/* Stack of regretted strings */
char	tempEdit[14];
char	tempMail[14];
char	tempMesg[14];
char	tempQuit[14];
char	tempResid[PATHSIZE];		/* temp file in :saved */
char	tempZedit[14];
uid_t	uid;				/* The invoker's user id */
static struct utimbuf	utimeb;
struct utimbuf	*utimep = &utimeb;
struct	var	*variables[HSHSIZE];	/* Pointer to active var list */
