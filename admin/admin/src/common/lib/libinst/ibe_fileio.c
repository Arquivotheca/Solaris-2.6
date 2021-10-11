#ifndef lint
#pragma ident "@(#)ibe_fileio.c 1.52 95/01/30"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "disk_lib.h"
#include "ibe_lib.h"
#include <stdarg.h>
#include <errno.h>

/* Public Function Prototypes */

void 		write_message(u_char, u_int, u_int, char *, ...);
void 		write_status(u_char, u_int, char *, ...);
void		write_notice(u_int, char *, ...);
void 		(*register_func(u_int, void (*)(u_int, char *)))();

/* Library Function Prototypes */

int 		_create_inst_release(Product *);
int		_open_product_file(Product *);

/* Private Function Prototypes */

static char *	_dflt_status_format(u_int, char *);
static void	_dflt_status_func(u_int, char *);
static char *	_dflt_error_format(u_int, char *);
static void	_dflt_error_func(u_int, char *);
static char *	_dflt_warning_format(u_int, char *);
static void	_dflt_warning_func(u_int, char *);
static void	_dflt_log_func(u_int, char *);
static void	_write_message(u_char, u_int, u_int, char *);

/* Local variables */

static void	(*_status_func)(u_int, char *) = _dflt_status_func;
static void	(*_error_func)(u_int, char *) = _dflt_error_func;
static void	(*_warning_func)(u_int, char *) = _dflt_warning_func;
static void	(*_log_func)(u_int, char *) = _dflt_log_func;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * write_message()
 * 	Write text data to the log file and/or the display.
 *	The text should already be internationalized by the calling
 *	routine. If the application has registered a display routine,
 *	that routine should be used to present the text, otherwise
 *	the display will be assumed to be stdout.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	dest	- specifies where the message should be recorded. Valid
 *		  values are:
 *			LOG	- write only to the log file
 *			SCR	- write only to the display
 *			LOGSCR	- write to both the file and display
 *	type	- Notification type. Valid values:
 *			STATMSG	- status message
 *			ERRMSG	- non-reconcilable problem
 *			WARNMSG	- advisory warning
 *	format	- format of the message (used by the formatting routine).
 *		  Valid values are:
 *			LEVEL0	- base level message
 *			LEVEL1	- first level message
 *			LEVEL2	- second level message
 *			LEVEL3	- third level message
 *			CONTINUE - message continuation from preceding message
 *			LISTITEM - item type message
 *			PARTIAL  - part of a message (more to come)
 *	string	- print format string
 *	...	- optional print arguments
 * Return:
 *	none
 * Status:
 *	public
 */
void
/*VARARGS4*/
write_message(u_char dest, u_int type, u_int format, char *string, ...)
{
	va_list	ap;
	char	buf[MAXPATHLEN + 1] = "";

	va_start(ap, string);
	(void) vsprintf(buf, string, ap);
	_write_message(dest, type, format, buf);
	va_end(ap);
}

/*
 * write_status()
 *	Write a status message.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	dest	- specifies where the message should be recorded. Valid
 *		  values are:
 *			LOG	- write only to the log file
 *			SCR	- write only to the display
 *			LOGSCR	- write to both the file and display
 *	format	- format of the message (used by the formatting routine).
 *		  Valid values are:
 *			LEVEL0	- base level message
 *			LEVEL1	- first level message
 *			LEVEL2	- second level message
 *			LEVEL3	- third level message
 *			CONTINUE - message continuation from preceding message
 *			LISTITEM - item type message
 *			PARTIAL  - part of a message (more to come)
 *	string	- print format string
 *	...	- optional printf arguments
 * Return:
 */
void
/*VARARGS3*/
write_status(u_char dest, u_int format, char *string, ...)
{
	va_list		ap;
	char		buf[MAXPATHLEN + 1] = "";

	va_start(ap, string);
	(void) vsprintf(buf, string, ap);
	_write_message(dest, STATMSG, format, buf);
	va_end(ap);
}

/*
 * write_notice()
 * 	Write text data to the log file and the display.
 *	The text should already be internationalized by the calling
 *	routine. If the application has registered a display routine,
 *	that routine should be used to present the text, otherwise
 *	the display will be assumed to be stderr.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	type	- Notification type. Valid values:
 *			ERRMSG	- non-reconcilable problem
 *			WARNMSG - advisory warning
 *	string	- print format string
 *	...	- optional printf arguments
 * Return:
 *	none
 * Status:
 *	public
 */
void
/*VARARGS2*/
write_notice(u_int type, char *string, ...)
{
	va_list	ap;
	char	buf[MAXPATHLEN + 1] = "";

	va_start(ap, string);
	(void) vsprintf(buf, string, ap);
	_write_message(LOG|SCR, type, LEVEL0, buf);
	va_end(ap);
}

/*
 * register_func()
 *	Register a STATUS, ERRMSG, or WARNMSG notification handler
 *	routine. This routine allows applications to supply their
 *	own routines which are called when posting status, error,
 *	and warning messages to the display. There are three separate
 *	handlers which can be supplied , one for each message type.
 * Parameters:
 *	type	- Notification type. Valid values:
 *			ERRMSG	- non-reconcilable problem
 *			WARNMSG - advisory warning
 *	func	- pointer to a function which is to be used for
 *		  posting display messages of type 'type'
 * Return:
 *	pointer to the previously registered handler function
 * Status:
 *	public
 */
void (*
register_func(u_int type, void (*func)(u_int, char *)))()
{
	void 	(*oldfunc)(u_int, char *) = NULL;

	switch (type) {
	    case ERRMSG:
		oldfunc = _error_func;
		_error_func = func;
		break;
	    case WARNMSG:
		oldfunc = _warning_func;
		_warning_func = func;
		break;
	    case STATMSG:
		oldfunc = _status_func;
		_status_func = func;
		break;
	}

	return (oldfunc);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _create_inst_release()
 *	Create the softinfo INST_RELEASE file on the image being
 *	created and log the current Solaris release, version, and
 *	revision representing the product installed on the system.
 * Parameters:
 *	prod	- non-NULL product structure pointer for the Solaris
 *		  product
 * Return:
 *	NOERR	- action completed (or skipped if debugging is turned on)
 *	ERROR	- unable to create INST_RELEASE file
 * Status:
 *	semi-private (internal library use only)
 */
int
_create_inst_release(Product *prod)
{
	FILE 	*fp;
	char	entry[256];

	(void) sprintf(entry, "%s%s/INST_RELEASE",
			get_rootdir(), SYS_ADMIN_DIRECTORY);

	if (get_install_debug() == 0) {
		if ((fp = fopen(entry, "w")) == NULL) {
			return (ERROR);
		} else {
			(void) fprintf(fp, "OS=%s\nVERSION=%s\nREV=%s\n",
				prod->p_name, prod->p_version, prod->p_rev);
			(void) fclose(fp);
		}
	}

	return (NOERR);
}

/*
 * _open_product_file()
 *	Open/create the product release file on the targetted install
 *	image for appended writing. Log the current product information.
 *	The softinfo directory is also created if one does not already
 *	exist. The file is in the softinfo directory, and has a name of
 *	the form:
 *
 *		<PRODUCT>_<VERSION>
 *
 *	The file is set to no buffering to avoid the need to
 *	close the file upon completion. The file format is:
 *
 *		OS=<product name>
 *		VERSION=<product version>
 *		REV=<product revision>
 * Parameters:
 *	prod	- non-NULL Product structure pointer
 * Return:
 *	NOERR	- product file open
 *	ERROR	- product file open failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_open_product_file(Product *prod)
{
	char	path[256];
	FILE	*fp;

	if (get_install_debug() > 0)
		return (NOERR);

	(void) sprintf(path, "%s%s/%s_%s",
		get_rootdir(),
		SYS_SERVICES_DIRECTORY, prod->p_name,
		prod->p_version);

	if ((fp = fopen(path, "a")) != NULL) {
		(void) fprintf(fp, "OS=%s\nVERSION=%s\nREV=%s\n",
			prod->p_name, prod->p_version, prod->p_rev);
		(void) fclose(fp);
		return (NOERR);
	}

	return (ERROR);
}

/* ******************************************************************** */
/*			PRIVATE SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _dflt_status_format()
 *	Default status message formatting function which adds format
 *	data to the string passed in and returns a pointer to a local
 *	buffer containing the fully formatted string.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	format	- format of the message (used by the formatting routine).
 *		  Valid values are:
 *			LEVEL0	- base level message
 *			LEVEL1	- first level message
 *			LEVEL2	- second level message
 *			LEVEL3	- third level message
 *			CONTINUE - message continuation from preceding message
 *			LISTITEM - item type message
 *			PARTIAL  - part of a message (more to come)
 *	string	- print format string
 * Return:
 *	char *
 * Status:
 *	private
 */
static char *
_dflt_status_format(u_int format, char *string)
{
	static char 	buf[MAXPATHLEN + 1];

	buf[0] = '\0';

	/* assemble the format leading characters based on the format */
	/* block separation */
	if (format & LEVEL0) {
		if (((format & CONTINUE) == 0) && (string[0] != '\0'))
			(void) strcat(buf, "\n");
	} else if (format & LEVEL1)
		(void) strcat(buf, "\t");
	else if (format & LEVEL2)
		(void) strcat(buf, "\t\t");
	else if (format & LEVEL3)
		(void) strcat(buf, "\t\t\t");

	/* class demarcation */
	if (format & LISTITEM) {
		if (format & CONTINUE)
			(void) strcat(buf, "  ");
		else
			(void) strcat(buf, "- ");
	}

	(void) strcat(buf, string);
	if ((format & PARTIAL) == 0)
		(void) strcat(buf, "\n");

	return (buf);
}

/*
 * _dflt_status_func()
 *	The default status logging function which converts a
 *	status message into the default format and prints it
 *	out to stdout.
 * Parameters:
 *	format	- format parameter for format function
 *	string	- buffer containing message to be logged
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_dflt_status_func(u_int format, char *string)
{
	(void) fprintf(stdout, _dflt_status_format(format, string));
	(void) fflush(stdout);
}

/*
 * _dflt_error_func()
 *	The default error logging function which converts a
 *	notice message into the default format and prints it
 *	out to stderr.
 * Parameters:
 *	format	- format of the message (used by the formatting routine)
 *	string	- buffer containing notice message to be logged
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_dflt_error_func(u_int format, char *string)
{
	(void) setbuf(stderr, NULL);
	(void) fprintf(stderr, "%s", _dflt_error_format(format, string));
}

/*
 * _dflt_error_format()
 *	Default error formatting routine.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	format	- format of the message (used by the formatting routine).
 *	string	- buffer containing notice message to be logged
 * Return:
 *	char *	- pointer to buffer containing formatted data
 * Status:
 *	private
 */
static char *
/*ARGSUSED0*/
_dflt_error_format(u_int format, char *string)
{
	static char 	buf[MAXPATHLEN + 1] = "";

	buf[0] = '\0';
	(void) sprintf(buf, "\n%s%s\n", MSG_LEADER_ERROR, string);
	return (buf);
}

/*
 * _dflt_warning_format()
 *	Defautl warning format routine.
 *
 *	WARNING:	A single message when formatted should not exceed
 *			1000 bytes or it will cause corruptions and core dumps.
 *			If a message needs to be longer than this, the PARTIAL
 *			format flag should be used on consecutive message calls.
 * Parameters:
 *	format	- format of the message (used by the formatting routine).
 *	string	- buffer containing notice message to be logged
 * Return:
 *	char *	- pointer to buffer containing formatted data
 * Status:
 *	private
 */
static char *
/*ARGSUSED0*/
_dflt_warning_format(u_int format, char *string)
{
	static char 	buf[MAXPATHLEN + 1] = "";

	buf[0] = '\0';
	(void) sprintf(buf, "%s%s\n", MSG_LEADER_WARNING, string);
	return (buf);
}

/*
 * _dflt_warning_func()
 *	The default warning logging function which converts a
 *	notice message into the default format and prints it
 *	out to stderr.
 * Parameters:
 *	format	- format of the message (used by the formatting routine).
 *	string	- buffer containing notice message to be logged
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_dflt_warning_func(u_int format, char *string)
{
	(void) fprintf(stderr, "%s", _dflt_warning_format(format, string));
	(void) fflush(stderr);
}

/*
 * _dflt_log_func()
 *	Default function which writes the message to the log file. Messages
 *	are only logged if debugging is not active. The log file is created
 *	and open for append if this hasn't been done previously.
 * Parameters:
 *	format	- format of the message (used by the formatting routine).
 *	buf	- string to be logged
 * Return:
 *	none
 * Status:
 *	private
 */
static void
/*VARARGS1*/
_dflt_log_func(u_int format, char *buf)
{
	static FILE 	*lfp = NULL;

	if (get_install_debug() == 0) {
		if (lfp == NULL) {
			if ((lfp = fopen(TMPLOGFILE, "a")) == NULL)
				lfp = stderr;

			(void) setbuf(lfp, NULL);
		}

		if (lfp != NULL)
			(void) fprintf(lfp,  buf);
	}
}

/*
 * _write_message()
 * Parameters:
 *	dest	- specifies where the message should be recorded. Valid
 *		  values are:
 *			LOG	- write only to the log file
 *			SCR	- write only to the display
 *			LOGSCR	- write to both the file and display
 *	type	- Notification type. Valid values:
 *			STATMSG	- status message
 *			ERRMSG	- non-reconcilable problem
 *			WARNMSG	- advisory warning
 *	format	- format of the message (used by the formatting routine).
 *		  Valid values are:
 *			LEVEL0	- base level message
 *			LEVEL1	- first level message
 *			LEVEL2	- second level message
 *			LEVEL3	- third level message
 *			CONTINUE - message continuation from preceding message
 *			LISTITEM - item type message
 *			PARTIAL  - part of a message (more to come)
 *	buf	- assembled message, unformatted
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_write_message(u_char dest, u_int type, u_int format, char *buf)
{
	/*
	 * if the text is targetted for the log file, write it
	 * using the currently registered file logging function
	 */
	if (dest & LOG) {
		switch (type) {
		    case STATMSG:
			(*_log_func)(format,
				_dflt_status_format(format, buf));
			break;

		    case WARNMSG:
			(*_log_func)(format, _dflt_warning_format(format, buf));
			break;

		    case ERRMSG:
			(*_log_func)(format, _dflt_error_format(format, buf));
			break;
		}
	}

	/*
	 * if the text is targetted for the display, write it
	 * using the currently registered display logging function
	 */
	if (dest & SCR) {
		switch (type) {
		    case ERRMSG:
			(*_error_func)(format, buf);
			break;
		    case STATMSG:
			(*_status_func)(format, buf);
			break;
		    case WARNMSG:
			(*_warning_func)(format, buf);
			break;
		}
	}
}
