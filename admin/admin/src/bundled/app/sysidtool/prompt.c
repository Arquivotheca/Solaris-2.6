/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)prompt.c 1.19 96/02/05"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/conf.h>
#include <stdarg.h>
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "message.h"
#include "prompt.h"

/*
 * NB:  These three static variables restrict the
 * program to a single user interface connection.
 * If this is undesirable, encapsulate the variables
 * in a structure, dynamically allocated and returned
 * by prompt_open, and passed in to and destroyed by
 * prompt_close.
 */
static pid_t	pid;		/* process-ID of user interface */
static int	to_ui;		/* file descriptor for UI's input */
static int	from_ui;	/* file descriptor for UI's output */

static void	prompt_check(char *);
static void	prompt_string(Sysid_op, Sysid_attr, char *, int);
static int	prompt_integer(Sysid_op, Sysid_attr, int);
static int	prompt_menu(
			Sysid_op, Sysid_attr, char **, int, int, char *, int);

static int	create_fifos(char *);
static int	open_fifos(char *, int *, int *);
static void	remove_fifos(void);

static char	*prompt_error_string(Sysid_err, va_list);
static void send_ui_set_locale(char *);

#ifdef DEV
static void
onintr(int sig)
{
	(void) kill(pid, sig);
}
#endif

/*
 * onchld is installed to catch "abnormal"
 * UI server terminations
 */
/*ARGSUSED*/
static void
onchld(int sig)
{
	int	status;

	while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
		;
	remove_fifos();
	exit(WEXITSTATUS(status));
}

void
die(char *message)
{
	int	force_exit = 1;
	int	status;

	if (pid != 0) {
		/* parent process */
		(void) signal(SIGCHLD, SIG_IGN);
		if (pid > 0) {
			(void) kill(pid, SIGTERM);	/* kill UI process */
			while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
				;
		}
		remove_fifos();
		(void) fprintf(stderr, "%s\n", message);
		(void) fflush(stderr);
	} else {
		/* ui process */
		extern	void dl_do_cleanup(char *, int *);	/* XXX */

		dl_do_cleanup(message, &force_exit);
	}
	exit(1);
}

static void
send_ui_set_locale(char *locale)
{
	MSG	*mp;

	/*
	 * Send the UI a "set locale" message
	 * (no response expected)
	 */
	mp = msg_new();
	(void) msg_set_type(mp, SET_LOCALE);
	(void) msg_add_arg(mp, ATTR_LOCALE, VAL_STRING,
			(void *)locale, strlen(locale) + 1);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
}

int
prompt_open(int *argcp, char **argv)
{
	FILE	*fp;
	char	path[MAXPATHLEN];
	char	*fifo_dir;
	int	status;

	fprintf(debugfp, "prompt_open %s\n", argv[0]);

	(void) sighold(SIGUSR1);		/* block "start" signal */
	(void) sighold(SIGPOLL);		/* and "stop/hangup" signal */
	/*
	 * Check for presence of persistent server.
	 * Its process id is in the file SYSI_UI_PID
	 * fork if not already running.
	 */
#ifdef DEV
	/*
	 * This code enables developers to pass
	 * in the fifo directory as a command-line
	 * argument and thus not interfere with
	 * one another.
	 */
	{
		int	ac = *argcp;
		char	**av = argv;

		fifo_dir = (char *)0;

		while (--ac && **++av == '-') {
			if ((*av)[1] == 'f') {
				ac--;
				fifo_dir = *++av;
			}
		}

		if (fifo_dir == (char *)0)
			fifo_dir = get_fifo_dir();
	}
#else
	fifo_dir = get_fifo_dir();
#endif
	if (fifo_dir == (char *)0)
		return (-1);
	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_PID);
	fp = fopen(path, "r");
	if (fp == (FILE *)0 ||			/* if pid file doesn't exist */
	    fscanf(fp, "%ld", &pid) != 1 ||	/* or contents are invalid */
	    kill(pid, 0) < 0) {			/* or process isn't running */
		if (fp != (FILE *)0)
			(void) fclose(fp);	/* reopen it later */
		/*
		 * Create FIFOs for communication
		 * between server and sysid*.  If
		 * they already exist, we'll jsut
		 * use the old ones.
		 */
		if (create_fifos(fifo_dir) < 0)
			return (-1);

		fflush(debugfp);
		/*
		 * Fork child UI server
		 */
		pid = fork();
		if (pid < 0)
			return (-1);
		if (pid == 0) {
			/* Child UI process */
			test_disable();
			fclose(debugfp);
			init_display(argcp, argv, fifo_dir);
			/*NOTREACHED*/
		}
		(void) signal(SIGCHLD, onchld);
		/*
		 * Record server's PID in file
		 */
		fp = fopen(path, "w");
		if (fp == (FILE *)0) {
			perror("fopen");
			exit(-1);		/* XXX ? */
		}
		(void) fprintf(fp, "%lu", (u_long)pid);
	}
	(void) fclose(fp);
#ifdef DEV
	(void) signal(SIGINT, onintr);
#endif
	if (open_fifos(fifo_dir, &to_ui, &from_ui) < 0) {
		(void) kill(pid, SIGTERM); /* can't talk to it, so kill it */
		status = -1;
	} else {
		(void) kill(pid, SIGUSR1); /* send "start" signal */
		status = 0;
	}

	/*
	 * Send UI process our stdin
	 */
	if (ioctl(to_ui, I_SENDFD, 0) < 0) {
		(void) kill(pid, SIGTERM);
		status = -1;
	}

	send_ui_set_locale(setlocale(LC_MESSAGES, (char *)0));

	return (status);
}

int
prompt_close(char *text, int do_exit)
{
	MSG	*mp;
	char	*arg;
	int	childpid;
	int	status;

	/*
	 * For all the other prompt_* routines it is
	 * an error not to have previously called
	 * prompt_open.  In the case of prompt_close,
	 * it is not an error and we either just return
	 * or shut down the UI process, depending on
	 * the do_exit argument.  To shut down the UI
	 * process, we first connect to it (w/prompt_open)
	 * then follow the normal shutdown procedure.
	 */
	if (pid == (pid_t)0) {
		if (do_exit) {
			int	argc = 1;	/* XXX MUST BE 1! */
			char	*argv = "";	/* value doesn't matter */

			(void) prompt_open(&argc, &argv);
		} else
			return (0);
	}

	if (text)
		arg = text;
	else
		arg = "";

	(void) signal(SIGCHLD, SIG_IGN);

	mp = msg_new();
	(void) msg_set_type(mp, CLEANUP);
	(void) msg_add_arg(mp, ATTR_STRING, VAL_STRING, arg, strlen(arg) + 1);
	(void) msg_add_arg(mp, ATTR_DOEXIT, VAL_INTEGER,
					(void *)&do_exit, sizeof (do_exit));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	/*
	 * Wait for response (avoid race conditions)
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&do_exit, sizeof (do_exit));
		msg_delete(mp);
	}

	(void) close(to_ui);
	(void) close(from_ui);

	/*
	 * Signal UI process to shut down
	 */
	(void) kill(pid, SIGPOLL);

	if (do_exit) {
		while ((childpid = waitpid(pid, &status, 0)) < 0 &&
		    errno == EINTR)
			;

		if (childpid != pid)	/* ??? */
			return (-1);
		else
			return (WEXITSTATUS(status));
	} else
		return (0);
}

/*
 *	Interface:
 *		void prompt_terminal(char chosen_termtype[MAX_TERM+2]);
 *
 *	Expected behavior:
 *		This routine should present the user with a numbered list
 *		of terminal types passed. If the user chooses all but the
 *		last entry (which is guaranteed to by "Other"), then
 *		the interface should set chosen_termtype to point to
 *		the chosen entry.
 *		If "Other" is chosen, the interface should provide a
 *		text field where the user can enter the terminal type.
 *
 *	Current validation routine:
 *		The current validation routine for this field (in terminal.c):
 *
 *		static struc term_id *verify_selection(char *buf)
 *
 *	Note:
 *		This routine will not be called unless the DISPLAY
 *		environment variable is not set, the TERM variable is
 *		not set, and the program is not running on a framebuffer
 *		console.
 */
void
prompt_terminal(char	*chosen_termtype)
{
	MSG	*mp;

	prompt_check("prompt_terminal");

	if (chosen_termtype[0] == '\0')
		prompt_string(GET_TERMINAL, ATTR_TERMINAL,
		    chosen_termtype, MAX_TERM+1);
	/*
	 * Send the UI a "set term" message
	 * (no response expected)
	 */
	mp = msg_new();
	(void) msg_set_type(mp, SET_TERM);
	(void) msg_add_arg(mp, ATTR_TERMINAL, VAL_STRING,
			(void *)chosen_termtype, strlen(chosen_termtype) + 1);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
}

/*
 *	Interface:
 *		void prompt_hostname(char *hostname);
 *
 *	Expected behavior:
 *		This routine should put up the current help
 *		information that sysidnet currently has. This
 *		help text specifies the valid combinations of hostname.
 *
 *	Current validation routine:
 *		(in host.c)
 *
 *		static int valid_hostname(FIELD *f)
 *
 *		Should be changed to
 *
 *		static int valid_hostname(char *hostname);
 *
 *	NB:  caller is responsible for ensuring hostname
 *	is a pointer to at least MAXHOSTNAMELEN bytes
 */
void
prompt_hostname(char *hostname)
{
	prompt_check("prompt_hostname");

	prompt_string(GET_HOSTNAME, ATTR_HOSTNAME, hostname, MAXHOSTNAMELEN);
}

/*
 *	Interface:
 *		int prompt_isit_standalone(int current_pick)
 *
 *	Expected behavior:
 *		This routine should prompt the user whether to use
 *		the network or not.
 *
 *		If the user answers yes, this routine should return 0.
 *		If the user answers no, this routine should return 1.
 *
 *	Validation routine:
 *		Not applicable
 */
int
prompt_isit_standalone(int current_pick)
{
	int	networked = current_pick;

	prompt_check("prompt_isit_standalone");

	if (current_pick != NO_INTEGER_VALUE)
		networked ^= 1;		/* XXX sense of question is reversed */

	networked = prompt_integer(GET_NETWORKED, ATTR_NETWORKED, networked);

	if (networked != NO_INTEGER_VALUE)
		networked ^= 1;

	return (networked);
}

/*
 *	Interface:
 *		void prompt_primary_net(char **interface_list, int ninterfaces,
 *			int current_interface);
 *
 *	Expected behavior:
 *		This routine should prompt the user to choose the primary
 *		network interface from the list passed.
 *
 *		The interface_chosen should contain the string representing
 *		the chosen interface when this function returns.
 *
 *	Validation routine:
 *		Not applicable
 */
int
prompt_primary_net(char	**interface_list,
		int	ninterfaces,
		int	current_pick)
{
	prompt_check("prompt_primary_net");

	return (prompt_menu(GET_PRIMARY_NET, ATTR_PRIMARY_NET,
		interface_list, ninterfaces, current_pick, (char *)0, 0));
}

/*
 *	Interface:
 *		void prompt_hostIP(char *ipaddr);
 *
 *	Expected behavior:
 *
 *		This routine should prompt the user for the ip address
 *		of the host (i.e. ip address of primary interface)
 *
 *		The ipaddr buffer should be filled in with the
 *		ascii representation of the ip address upon return.
 *
 *	Validation routine:
 *		The current validation routine (in host.c) is:
 *
 *		static int valid_host_ip_addr(FIELD *f)
 *
 *		which should be changed to
 *
 *		int valid_host_ip_addr(char *ipaddr_to_verify);
 *
 *	NB:  caller is responsible for ensuring hostip
 *	is a pointer to at least MAX_IPADDR bytes
 */
void
prompt_hostIP(char *hostIP)
{
	prompt_check("prompt_hostIP");

	prompt_string(GET_HOSTIP, ATTR_HOSTIP, hostIP, MAX_IPADDR+1);
}

/*
 *	Interface:
 *		int prompt_confirm(Confirm_data list_data[num_items],
 *		    int num_items)
 *
 *	Expected behavior:
 *		This routine should display the (field_id, value) pairs
 *		with some mechanism for the user to confirm or not confirm
 *		the given data.
 *
 *		If the user confirms the data, the routine should return TRUE;
 *
 *		If the user doesn't confirm the data, the routine should
 *		return FALSE;
 */
int
prompt_confirm(Confirm_data *list_data, int num_items)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i;
	int	confirmed;

	prompt_check("prompt_confirm");

	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, GET_CONFIRM);
	for (i = 0; i < num_items; i++) {
		Confirm_data	*data = &list_data[i];

		(void) msg_add_arg(mp, data->field_attr, VAL_INTEGER,
			(void *)&data->field_attr, sizeof (data->field_attr));
		(void) msg_add_arg(mp, ATTR_ARG, VAL_STRING,
			(void *)data->value, strlen(data->value) + 1);
		(void) msg_add_arg(mp, ATTR_ARG, VAL_INTEGER,
			(void *)&data->flags, sizeof (data->flags));
	}
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &attr, (Val_t *)0,
				(void *)&confirmed, sizeof (confirmed));
		msg_delete(mp);

		if (attr != ATTR_CONFIRM)
			confirmed = NO_INTEGER_VALUE;
	} else
		confirmed = NO_INTEGER_VALUE;

	return (confirmed);
}

/*
 *	Interface:
 *		void prompt_error(Err_t errcode, ...)
 *
 *	Expected behavior:
 *		The passed message code specifies a
 *		template that should be used to construct
 *		the error message to be displayed
 *
 *	Note:
 *		Some of the arguments to prompt_error are strings
 *		that point to error messages returned by SNAG.
 *
 */
void
prompt_error(Sysid_err errcode, ...)
{
	va_list	ap;
	char	*errmsg;
	MSG	*mp;

	prompt_check("prompt_error");

	mp = msg_new();
	(void) msg_set_type(mp, ERROR);

	va_start(ap, errcode);
	errmsg = prompt_error_string(errcode, ap);
	va_end(ap);

	(void) msg_add_arg(mp, ATTR_ERROR, VAL_STRING,
					(void *)errmsg, strlen(errmsg));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response (sent when user confirms error)
	 */
	mp = msg_receive(from_ui);
	msg_delete(mp);
}

/*
 *	Interface:
 *		int prompt_name_service(char ns_type[40], int current_pick)
 *
 *		int prompt_name_service(char **service_list, int nservices,
 *			int current_pick)
 *
 *	Expected behavior:
 *		This routine should display a menu of name services to
 *		choose from (i.e. NIS, NIS+ or none). The current pick
 *		passed in specifies the ordinal value of
 *		previous menu item that was picked.
 *
 *		The ns_type array should contain the ascii name of
 *		the nameservice picked upon return (This could easily
 *		be changed to a int type)
 *
 *	Validation routine:
 *		Not applicable
 *
 */
int
prompt_name_service(char **service_list,
		int	nservices,
		int	current_pick)
{
	prompt_check("prompt_name_service");

	return (prompt_menu(GET_NAME_SERVICE, ATTR_NAME_SERVICE,
		service_list, nservices, current_pick, (char *)0, 0));
}

/*
 *	Interface:
 *		void prompt_domain(char domain[MAX_DOMAINNAME+2])
 *
 *	Expected behavior:
 *		This routine should prompt the user for the domain name.
 *
 *		The domain array should be filled in with the domain name
 *		upon return
 *
 *	Validation routine:
 *		(in name_service.c)
 *
 *		static int valid_domainname(FIELD *f)
 *
 *		should be
 *
 *		static int valid_domainname(char *domainname)
 */
void
prompt_domain(char *domain)
{
	prompt_check("prompt_domain");

	prompt_string(GET_DOMAIN, ATTR_DOMAIN, domain, MAX_DOMAINNAME+1);
}

/*
 *	Interface:
 *		int prompt_broadcast(int current_pick)
 *
 *	Expected behavior:
 *		This routine should query the user on whether to
 *		find the name server by broadcasting or by specifying
 *		the name server.
 *
 *		The values of current_pick and the returned int should be:
 *
 *		0 - specifies broadcast
 *		1 - specifies "Specify Hostname of name server"
 *
 *	Validation routine:
 *		None
 */
int
prompt_broadcast(int current_pick)
{
	prompt_check("prompt_broadcast");

	return (prompt_integer(GET_BROADCAST, ATTR_BROADCAST, current_pick));
}

/*
 *	Interface:
 *		void prompt_nisservers(char ns_server_name[MAX_HOSTNAME+2],
 *			ns_server_addr[MAX_IPADDR+2])
 *
 *	Expected behavior:
 *		This routine should prompt the user for the hostname and
 *		ipaddr of the name server. The ns_server_name array should
 *		contain the hostname of the server and the ns_server_addr
 *		should contain the ip address of the hostname upon return.
 *
 *	Validation routines:
 *		The validation routine for ns_server_name (in name_service.c):
 *
 *		static int valid_hostname(FIELD *f)
 *
 *		which should be
 *
 *		static int valid_hostname(char *hostname);
 *
 *		The validation routine for ns_server_addr (in name_service.c):
 *
 *		static int valid_host_ip_addr(FIELD *f)
 *
 *		which should be
 *
 *		static int valid_host_ip_addr(char *ipaddr);
 */
void
prompt_nisservers(char	*ns_server_name,
		char	*ns_server_addr)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i;

	prompt_check("prompt_nisservers");

	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, GET_NISSERVERS);
	(void) msg_add_arg(mp, ATTR_NISSERVERNAME, VAL_STRING,
			(void *)ns_server_name, strlen(ns_server_name) + 1);
	(void) msg_add_arg(mp, ATTR_NISSERVERADDR, VAL_STRING,
			(void *)ns_server_addr, strlen(ns_server_addr) + 1);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	*ns_server_name = '\0';
	*ns_server_addr = '\0';

	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	for (i = 0; i < msg_get_nargs(mp); i++) {
		char	buf[MAX_FIELDVALLEN+1];

		(void) msg_get_arg(mp, &attr, (Val_t *)0,
					(void *)buf, sizeof (buf));

		switch (attr) {
		case ATTR_NISSERVERNAME:
		(void) strncpy(ns_server_name, buf, MAX_HOSTNAME+1);
			ns_server_name[MAX_HOSTNAME] = '\0';
			break;
		case ATTR_NISSERVERADDR:
			(void) strncpy(ns_server_addr, buf, MAX_IPADDR+1);
			ns_server_addr[MAX_IPADDR] = '\0';
			break;
		case ATTR_ERROR:
		default:
			break;
		}
	}
	msg_delete(mp);
}

/*
 *	Interface:
 *		int prompt_isit_subnet(int subnetted);
 *
 *	Expected behavior:
 *		This routine should query the user whether they have subnets
 *		or not. The value returned and the value passed in should
 *		have the following semantics:
 *
 *		0 -- Not subnetted
 *		1 -- Subnetted
 *
 *	Validation routine:
 *		Not applicable
 */
int
prompt_isit_subnet(int subnetted)
{
	prompt_check("prompt_isit_subnet");

	return (prompt_integer(GET_SUBNETTED, ATTR_SUBNETTED, subnetted));
}

/*
 *	Interface:
 *		int prompt_netmask(char netmask[MAX_NETMASK+3])
 *
 *	Expected behavior:
 *		This routine should prompt the user for the netmask.
 *		It will only be called if the return value from
 *		prompt_isit_subnetted is 1.
 *		The array netmask should be filled in with an ascii
 *		representation of the netmask upon return.
 *
 *	Validation routine:
 *		(in netmask.c)
 *
 *		static int valid_ip_netmask(FIELD *f)
 *
 *		should be
 *
 *		static int valid_ip_netmask(char *netmask)
 */
void
prompt_netmask(char *netmask)
{
	prompt_check("prompt_netmask");

	prompt_string(GET_NETMASK, ATTR_NETMASK, netmask, MAX_NETMASK+1);
}

/*
 *	Interface:
 *		int prompt_bad_nis(Sysid_err errcode, ...)
 *
 *	Expected behavior:
 *		This routine should display a popup that contains the
 *		passed prompt as part of the popup. It should ask whether
 *		the user wants to continue or reselect the name
 *		service.
 *
 *		The return value should be:
 *
 *		0 - specifies that system configuration should continue
 *		1 - specifies that the user wishes to reselect the name
 *			service info
 *
 *	Validation routine:
 *		Not applicable
 *
 *	Notes:
 *		The passed in prompt should already be localized.
 */
int
prompt_bad_nis(Sysid_err errcode, ...)
{
	va_list	ap;
	Sysid_attr attr;
	int	reselect;
	char	*errmsg;
	MSG	*mp;

	prompt_check("prompt_bad_nis");

	mp = msg_new();
	(void) msg_set_type(mp, GET_BAD_NIS);

	va_start(ap, errcode);
	errmsg = prompt_error_string(errcode, ap);
	va_end(ap);

	(void) msg_add_arg(mp, ATTR_ERROR, VAL_STRING,
					(void *)errmsg, strlen(errmsg));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response (sent when user confirms error)
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &attr, (Val_t *)0,
					(void *)&reselect, sizeof (reselect));
		msg_delete(mp);

		if (attr != ATTR_BAD_NIS)
			reselect = NO_INTEGER_VALUE;
	} else
		reselect = NO_INTEGER_VALUE;

	return (reselect);
}

/*
 *	Interface:
 *		void prompt_timezone(char timezone[MAX_TZ+2], int *region_pick,
 *		    int *tz_pick);
 *
 *	Expected behavior:
 *		This routine is a bit complicated:
 *
 *		First, the routine should display a menu of the timezone
 *		regions available. Once the user picks a region, the
 *		integer pointed to by region_pick should be set to
 *		the "index" of the picked region.
 *
 *		If the region picked is "GMT", another screen should come
 *		up letting the user pick the offset from GMT (should
 *		be in the range of -12 to 13).
 *
 *		If the region picked is "Filename", then another screen
 *		should come up that allows the user to specify a filename
 *
 *		Otherwise, a submenu of timezones for the specified region
 *		should be displayed. The integer pointed to by tz_pick should
 *		be set to the "index" of the picked "sub" timezone.
 *
 *		In all cases, the timezone array should be filled with
 *		the value of the given input.
 *
 *	Validation routine:
 *		In the case that "Filename" is picked, the filename entered
 *		is validated by the routine (in ui_timezone.c):
 *
 *		static Sysid_err ui_valid_tz(Field_desc *f);
 */
void
prompt_timezone(char	*timezone,
		int	*region_pick,
		int	*tz_pick)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i;

	prompt_check("prompt_timezone");

	mp = msg_new();
	(void) msg_set_type(mp, GET_TIMEZONE);
	(void) msg_add_arg(mp, ATTR_TIMEZONE, VAL_STRING,
				(void *)timezone, MAX_TZ+1);
	(void) msg_add_arg(mp, ATTR_TZ_REGION, VAL_INTEGER,
				(void *)region_pick, sizeof (*region_pick));
	(void) msg_add_arg(mp, ATTR_TZ_INDEX, VAL_INTEGER,
				(void *)tz_pick, sizeof (*tz_pick));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	for (i = 0; i < msg_get_nargs(mp); i++) {
		char	buf[MAX_FIELDVALLEN+1];

		(void) msg_get_arg(mp, &attr, (Val_t *)0,
						(void *)buf, sizeof (buf));
		switch (attr) {
		case ATTR_TIMEZONE:
			(void) strncpy(timezone, buf, MAX_TZ+1);
			timezone[MAX_TZ] = '\0';
			break;
		case ATTR_TZ_REGION:
			(void) memcpy((void *)region_pick, (void *)buf,
							sizeof (*region_pick));
			break;
		case ATTR_TZ_INDEX:
			(void) memcpy((void *)tz_pick, (void *)buf,
							sizeof (*tz_pick));
			break;
		case ATTR_ERROR:
		default:
			break;
		}
	}
	msg_delete(mp);
}

/*
 *	Interface:
 *		void prompt_locale(char *lang,
 *		    char locale_picked[MAX_LOCALE+1])
 *
 *		The user interface will not be called to pick a locale
 *		unless the number of locales (nlocales) is greater than 1.
 *
 *		The locale_picked parameter should contain the string
 *		representing the localed picked upon return
 *
 *		This is the first possible question that sysidnet will ask.
 *		After the locale is picked (or autodetermined), the locale
 *		is set via set_locale.
 *
 *	Expected behavior:
 *		prompt_locale should present the user with a list of
 *		locales with each locale printed in the language for that
 *		locale (i.e. there should be a set_locale(LC_ALL, locales[i])
 *		before each locale is displayed).
 *
 *		The locale_picked variable should point to the string
 *		representing the chosen locale.
 *
 *	Validation routine:
 *		There is no current validation routine for locale stuff
 *
 */
void
prompt_locale(char	*locale,
		int	*lang_pick,
		int	*loc_pick)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i, j;
	int	nlangs;
	int	nlocales;
	char **langp;
	char **localep;

	prompt_check("prompt_locale");

	nlangs = get_lang_strings(&langp);

	if (nlangs > 0) {

		mp = msg_new();
		(void) msg_set_type(mp, GET_LOCALE);
		(void) msg_add_arg(mp, ATTR_NLANGS, VAL_INTEGER,
					(void *)&nlangs, sizeof (int));
		for (i = 0; i < nlangs; i++) {
			(void) msg_add_arg(mp, ATTR_LANG, VAL_STRING,
					(void *)langp[i], MAX_LANG+1);
			nlocales = get_lang_locale_strings(langp[i], &localep);
			(void) msg_add_arg(mp, ATTR_NLOCALES, VAL_INTEGER,
						(void *)&nlocales, sizeof (int));
			for (j = 0; j < nlocales; j++) {
				(void) msg_add_arg(mp, ATTR_LOCALE, VAL_STRING,
					(void *)localep[j], MAX_LOCALE+1);
			}
		}
		(void) msg_add_arg(mp, ATTR_LOCALEPICK, VAL_INTEGER,
					(void *)locale, sizeof (*lang_pick));
		(void) msg_add_arg(mp, ATTR_LOC_LANG, VAL_INTEGER,
					(void *)lang_pick, sizeof (*lang_pick));
		(void) msg_add_arg(mp, ATTR_LOC_INDEX, VAL_INTEGER,
					(void *)loc_pick, sizeof (*loc_pick));
		(void) msg_send(mp, to_ui);
		msg_delete(mp);
	
		/*
		 * Wait for response
		 */
		mp = msg_receive(from_ui);
		for (i = 0; i < msg_get_nargs(mp); i++) {
			char	buf[MAX_FIELDVALLEN+1];
	
			(void) msg_get_arg(mp, &attr, (Val_t *)0,
						(void *)buf, sizeof (buf));
			switch (attr) {
			case ATTR_LOCALEPICK:
				(void) strncpy(locale, buf, MAX_LOCALE+1);
				locale[MAX_LOCALE] = '\0';
				break;
			case ATTR_LOC_LANG:
				(void) memcpy((void *)lang_pick, (void *)buf,
							sizeof (*lang_pick));
				break;
			case ATTR_LOC_INDEX:
				(void) memcpy((void *)loc_pick, (void *)buf,
							sizeof (*loc_pick));
				break;
			case ATTR_ERROR:
			default:
				break;
			}
		}
		msg_delete(mp);

	}
	send_ui_set_locale(setlocale(LC_MESSAGES, locale));
}


/*
 *	Interface:
 *		void prompt_date(char year[MAX_YEAR+2],
 *		    char month[MAX_MONTH+2], char day[MAX_DAY+2],
 *		    char hour[MAX_HOUR+2], char minute[MAX_MINUTE+2]);
 *
 *	Expected behavior:
 *		This routine should prompt the user for the year, month,
 *		day, hour and minute.
 *		The values entered by the user for the year, month, day,
 *		hour and minute should be copied into the appropriate
 *		arrays.
 *
 *		Expected ranges for each of the fields are as follows:
 *
 *		year - [1900, undetermined]
 *		month - [1, 12]
 *		day - [1,31]
 *		hour - [0,23]
 *		minute - [0,59]
 *
 *	Validation routine:
 *		None available, though obvious further checking would
 *		be nice (i.e. limit the range of the day depending on
 *		the month).
 *
 *	Notes:
 *		Note that there are localized representations of the
 *		date (year,month,day) as well as the time (hour, minute),
 *		so either make each a separate field, or create
 *		localized versions of a date field and a time field.
 */
void
prompt_date(char *year,
	char	*month,
	char	*day,
	char	*hour,
	char	*minute)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i;

	prompt_check("prompt_date");

	mp = msg_new();
	(void) msg_set_type(mp, GET_DATE_AND_TIME);
	(void) msg_add_arg(mp, ATTR_YEAR, VAL_STRING,
				(void *)year, MAX_YEAR+1);
	(void) msg_add_arg(mp, ATTR_MONTH, VAL_STRING,
				(void *)month, MAX_MONTH+1);
	(void) msg_add_arg(mp, ATTR_DAY, VAL_STRING,
				(void *)day, MAX_DAY+1);
	(void) msg_add_arg(mp, ATTR_HOUR, VAL_STRING,
				(void *)hour, MAX_HOUR+1);
	(void) msg_add_arg(mp, ATTR_MINUTE, VAL_STRING,
				(void *)minute, MAX_MINUTE+1);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	for (i = 0; i < msg_get_nargs(mp); i++) {
		char	buf[MAX_FIELDVALLEN+1];

		(void) msg_get_arg(mp, &attr, (Val_t *)0,
					(void *)buf, sizeof (buf));
		switch (attr) {
		case ATTR_YEAR:
			(void) strncpy(year, buf, MAX_YEAR+1);
			year[MAX_YEAR] = '\0';
			break;
		case ATTR_MONTH:
			(void) strncpy(month, buf, MAX_MONTH+1);
			month[MAX_MONTH] = '\0';
			break;
		case ATTR_DAY:
			(void) strncpy(day, buf, MAX_DAY+1);
			day[MAX_DAY] = '\0';
			break;
		case ATTR_HOUR:
			(void) strncpy(hour, buf, MAX_HOUR+1);
			hour[MAX_HOUR] = '\0';
			break;
		case ATTR_MINUTE:
			(void) strncpy(minute, buf, MAX_MINUTE+1);
			minute[MAX_MINUTE] = '\0';
			break;
		default:
			break;
		}
	}
	msg_delete(mp);
}

/*
 *	Interface:
 *		int prompt_password(char password[MAX_PASSWORD+1],
 *			char epassword[MAX_PASSWORD+1])
 *
 *	Expected behavior:
 *		This routine should prompt the user for the root
 *		password.  The array password should be filled in
 *		the clear-text password and the array epassword
 *		should be filled in with the encrypted password.
 */
void
prompt_password(char *password, char *epassword)
{
	Sysid_attr attr;
	MSG	*mp;
	int	i;

	prompt_check("prompt_password");

	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, GET_PASSWORD);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	*password = '\0';
	*epassword = '\0';

	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	for (i = 0; i < msg_get_nargs(mp); i++) {
		char	buf[MAX_FIELDVALLEN];
		(void) msg_get_arg(mp, &attr, (Val_t *)0,
					(void *)buf, sizeof (buf));
		switch (attr) {
		case ATTR_PASSWORD:
			(void) strncpy(password, buf, MAX_PASSWORD+1);
			password[MAX_PASSWORD] = '\0';
			break;
		case ATTR_EPASSWORD:
			(void) strncpy(epassword, buf, MAX_PASSWORD+1);
			epassword[MAX_PASSWORD] = '\0';
			break;
		case ATTR_ERROR:
		default:
			break;
		}
	}
	msg_delete(mp);
}

/*
 *	Interface:
 *		Prompt_t prompt_message(Prompt_t handle, char *text)
 *
 *	Expected behavior:
 *		This routine displays its argument message (which is
 *		expected to be localized by the caller).  The mechanism
 *		by which the message is displayed is dependent on the
 *		interface type in use (tty or gui); thus the use of an
 *		opaque "prompt handle".  If the prompt handle is null,
 *		a new message is generated (tty:  clear screen, gui:
 *		create popup); if non null, an existing message is used
 *		(tty: print after existing message, gui: change contents
 *		of existing popup).
 *
 *		This routine returns a handle on the prompt message for
 *		use in subsequent prompt_message and prompt_dismiss calls.
 *
 */
Prompt_t
prompt_message(Prompt_t handle, char *text)
{
	Sysid_attr attr;
	Prompt_t newh;
	MSG	*mp;
	char	*arg;

	prompt_check("prompt_message");

	if (text)
		arg = text;
	else
		arg = "";

	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, DISPLAY_MESSAGE);
	(void) msg_add_arg(mp, ATTR_PROMPT, VAL_INTEGER,
					(void *)&handle, sizeof (handle));
	(void) msg_add_arg(mp, ATTR_STRING, VAL_STRING,
					(void *)arg, strlen(arg) + 1);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);

	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &attr, (Val_t *)0,
					(void *)&newh, sizeof (newh));
		msg_delete(mp);

		if (attr != ATTR_PROMPT)
			newh = (Prompt_t)0;
	} else
		newh = (Prompt_t)0;

	return (newh);
}

/*
 *	Interface:
 *		void prompt_dismiss(Prompt_t handle)
 *
 *	Expected behavior:
 *		This routine dismiss the message referenced by the
 *		argument handle.  The mechanism by which the message
 *		is dismissed is dependent on the interface type in use
 *		(tty or gui); thus the use of an opaque "prompt handle".
 *		If the tty interface is in use, a clear-screen sequence
 *		is generated; if the gui is in use, the opaque handle
 *		will contain the widget-id of a pop-up dialog, which is
 *		subsequently popped down.
 */
void
prompt_dismiss(Prompt_t handle)
{
	prompt_check("prompt_dismiss");

	(void) prompt_integer(DISMISS_MESSAGE, ATTR_PROMPT, (int)handle);
}

static void
prompt_check(char *func)
{
	fprintf(debugfp, "%s\n", func);

	if (pid == (pid_t)0) {
		fprintf(stderr, dgettext(TEXT_DOMAIN,
		    "Fatal internal error:  %s called before prompt_open!\n"),
			func);
		abort();
	}
}

/*
 * The following are the generic prompt routines used to
 * ask the simple questions, generally, those supplying a
 * single value (in the case of the menu routine, think of
 * the "array" of choices as a single value) and expecting
 * a single response.
 */
static void
prompt_string(Sysid_op op, Sysid_attr attr, char *string, int len)
{
	Sysid_attr ret_attr;
	MSG	*mp;
	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, op);
	if (string != NO_STRING_VALUE)
		(void) msg_add_arg(mp, attr, VAL_STRING,
					(void *)string, strlen(string) + 1);
	else
		(void) msg_add_arg(mp, attr, VAL_STRING, (void *)"", 0);
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &ret_attr, (Val_t *)0,
						(void *)string, len);
		msg_delete(mp);

		if (attr != ret_attr)
			(void) strcpy(string, "");
	} else
		(void) strcpy(string, "");
}

static int
prompt_integer(Sysid_op op, Sysid_attr attr, int val)
{
	Sysid_attr ret_attr;
	MSG	*mp;
	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, op);
	(void) msg_add_arg(mp, attr, VAL_INTEGER,
					(void *)&val, sizeof (int));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &ret_attr, (Val_t *)0,
					(void *)&val, sizeof (int));
		msg_delete(mp);

		if (attr != ret_attr)
			val = NO_INTEGER_VALUE;
	} else
		val = NO_INTEGER_VALUE;

	return (val);
}

static int
prompt_menu(Sysid_op op,
	Sysid_attr attr,
	char	**ptrs,
	int	nptrs,
	int	initial_pick,	/* can be NO_INTEGER_VALUE if not defined */
	char	*string,	/* optional RETURN string value */
	int	len)		/* optional length of RETURN buffer */
{
	Sysid_attr ret_attr;
	MSG	*mp;
	int	i, index;

	/*
	 * Create message, send to UIM
	 */
	mp = msg_new();
	(void) msg_set_type(mp, op);
	(void) msg_add_arg(mp, ATTR_SIZE, VAL_INTEGER,
					(void *)&nptrs, sizeof (int));
	for (i = 0; i < nptrs; i++)
		(void) msg_add_arg(mp, attr, VAL_STRING,
					(void *)ptrs[i], strlen(ptrs[i]) + 1);
	(void) msg_add_arg(mp, ATTR_INDEX, VAL_INTEGER,
					(void *)&initial_pick, sizeof (int));
	(void) msg_send(mp, to_ui);
	msg_delete(mp);
	/*
	 * Wait for response
	 */
	mp = msg_receive(from_ui);
	if (mp != (MSG *)0) {
		(void) msg_get_arg(mp, &ret_attr, (Val_t *)0,
					(void *)&index, sizeof (int));
		msg_delete(mp);

		if (attr != ret_attr)
			index = NO_INTEGER_VALUE;
	} else
		index = NO_INTEGER_VALUE;

	if (index >= 0 && index < nptrs) {
		if (string != (char *)0)
			(void) strncpy(string, ptrs[index], len);
	} else {
		if (string != (char *)0)
			*string = '\0';
	}
	return (index);
}

char *
get_fifo_dir(void)
{
	static	char **fifo_dirp;
	static	char *dirs[] = {
		"/usr/tmp",
		"/tmp",
		NULL
	};
	char	path[MAXPATHLEN];

	if (fifo_dirp == (char **)0) {
		for (fifo_dirp = dirs; *fifo_dirp; fifo_dirp++) {
			(void) sprintf(path, "%s/%s", *fifo_dirp, SYSID_UI_PID);
			if (access(path, F_OK) == 0)
				break;
		}
		if (*fifo_dirp == (char *)0) {
			for (fifo_dirp = dirs; *fifo_dirp; fifo_dirp++) {
				if (access(*fifo_dirp, W_OK) == 0)
					break;
			}
		}
	}
	return (*fifo_dirp);
}

static int
create_fifos(char *fifo_dir)
{
	char	path[MAXPATHLEN];
	int	status;
	mode_t	mode = 0600;

#ifdef DEV
	if (geteuid() != 0) {
		(void) umask(0);
		mode = 0666;
	}
#endif

	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_IN);
	status = mkfifo(path, mode);
	if (status == 0 || errno == EEXIST) {
		(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_OUT);
		status = mkfifo(path, mode);
		if (status == 0 || errno == EEXIST)
			status = 0;
	}
	return (status);
}

static int
open_fifos(char *prefix, int *to, int *from)
{
	char	path[MAXPATHLEN];

	(void) sprintf(path, "%s/%s", prefix, SYSID_UI_FIFO_IN);
	*to = open(path, O_WRONLY);
	if (*to < 0)
		return (-1);

	(void) sprintf(path, "%s/%s", prefix, SYSID_UI_FIFO_OUT);
	*from = open(path, O_RDONLY);
	if (*from < 0)
		return (-1);

	return (0);
}

static void
remove_fifos(void)
{
	char	path[MAXPATHLEN];
	char	*fifo_dir;

	fifo_dir = get_fifo_dir();
	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_PID);
	(void) unlink(path);

	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_IN);
	(void) unlink(path);

	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_OUT);
	(void) unlink(path);
}

static char *
prompt_error_string(Sysid_err errcode, va_list ap)
{
	static char buf[BUFSIZ];
	char	*fmt;

	switch (errcode) {
	case SYSID_ERR_BAD_TZ_FILE_NAME:
		fmt = BAD_TZ_FILE_NAME;
		break;
	case SYSID_ERR_BAD_LOOPBACK:
		fmt = BAD_LOOPBACK;
		break;
	case SYSID_ERR_BAD_HOSTS_ENT:
		fmt = BAD_HOSTS_ENT;
		break;
	case SYSID_ERR_BAD_IP_ADDR:
		fmt = BAD_IP_ADDR;
		break;
	case SYSID_ERR_BAD_UP_FLAG:
		fmt = BAD_UP_FLAG;
		break;
	case SYSID_ERR_NO_IPADDR:
		fmt = NO_IPADDR;
		break;
	case SYSID_ERR_STORE_NETMASK:
		fmt = STORE_NETMASK;
		break;
	case SYSID_ERR_BAD_NETMASK:
		fmt = BAD_NETMASK;
		break;
	case SYSID_ERR_BAD_DOMAIN:
		fmt = BAD_DOMAIN;
		break;
	case SYSID_ERR_NSSWITCH_FAIL1:
		fmt = NSSWITCH_FAIL1;
		break;
	case SYSID_ERR_NSSWITCH_FAIL2:
		fmt = NSSWITCH_FAIL2;
		break;
	case SYSID_ERR_BAD_NISSERVER_ENT:
		fmt = BAD_NISSERVER_ENT;
		break;
	case SYSID_ERR_BAD_YP_ALIASES:
		fmt = BAD_YP_ALIASES;
		break;
	case SYSID_ERR_NO_NETMASK:
		fmt = NO_NETMASK;
		break;
	case SYSID_ERR_BAD_TIMEZONE:
		fmt = BAD_TIMEZONE;
		break;
	case SYSID_ERR_BAD_DATE:
		fmt = BAD_DATE;
		break;
	case SYSID_ERR_GET_ETHER:
		fmt = GET_ETHER;
		break;
	case SYSID_ERR_BAD_ETHER:
		fmt = BAD_ETHER;
		break;
	case SYSID_ERR_BAD_BOOTP:
		fmt = BAD_BOOTP;
		break;
	case SYSID_ERR_BAD_NETMASK_ENT:
		fmt = BAD_NETMASK_ENT;
		break;
	case SYSID_ERR_BAD_TIMEZONE_ENT:
		fmt = BAD_TIMEZONE_ENT;
		break;
	case SYSID_ERR_CANT_DO_PASSWORD_PLUS:
		fmt = CANT_DO_PASSWORD_PLUS;
		break;
	case SYSID_ERR_CANT_DO_PASSWORD:
		fmt = CANT_DO_PASSWORD;
		break;
	case SYSID_ERR_CANT_DO_KEYLOGIN:
		fmt = CANT_DO_KEYLOGIN;
		break;
	case SYSID_ERR_BAD_YP_BINDINGS1:
		fmt = BAD_YP_BINDINGS1;
		break;
	case SYSID_ERR_BAD_YP_BINDINGS2:
		fmt = BAD_YP_BINDINGS2;
		break;
	case SYSID_ERR_BAD_NIS_SERVER1:
		fmt = BAD_NIS_SERVER1;
		break;
	case SYSID_ERR_BAD_NIS_SERVER2:
		fmt = BAD_NIS_SERVER2;
		break;
	case SYSID_ERR_BAD_NODENAME:
		fmt = BAD_NODENAME;
		break;
	case SYSID_ERR_NIS_SERVER_ACCESS:
		fmt = NIS_SERVER_ACCESS;
		break;
	default:
		break;
	}

	(void) vsprintf(buf, fmt, ap);

	return (buf);
}
