
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the adm_perf_method() routine (and supporting cast).
 *	It is the administrative framework interface through which (possibly
 *	remote) administrative methods are accessed.  The routine is based on
 *	the SunNet Manager manager services library.
 *
 *	NOTE: svc_run() was adapted to accomodate timeouts and to return
 *	      after the last report from a data request is received from
 *	      the agent involved.
 *
 *	NOTE: adm_perf_method() is EXTREMELY thread-UNSAFE!  It can only
 *	      handle one method request at a time for each process.  A
 *	      table of active requests (gaurded by a mutex lock) will need
 *	      to be added in order for the routines to handle multiple
 *	      concurrent requests (this table could be indexed by SNM
 *	      timestamp).
 *
 *	NOTE: adm_perf_method() should be changed to lookup the admin class
 *	      agent RPC program number from the portmapper.
 *
 *******************************************************************************
 */

#ifndef _adm_amcl_c
#define _adm_amcl_c

#pragma	ident	"@(#)adm_amcl.c	1.37	94/06/07 SMI"

#include <rpc/rpc.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <netdb.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <malloc.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "netmgt/netmgt.h"
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_auth_impl.h"

/*
 * NOTE: The following definition should be removed if it becomes a
 *	 a standard part of the libadmcom include files.
 */

#define NETMGT_ACTION_REPORT	(u_int) 13

/*
 * Request callback RPC info.
 */

boolean_t  is_rpc_allocated=B_FALSE;	/* Has a temp. RPC # already */
					/* been allocated for callbacks? */
int	   t_udp_sock;			/* Temp. RPC UDP socket descriptor */
int	   t_tcp_sock;			/* Temp. RPC TCP socket descriptor */
u_long	   t_rendez_prog;		/* Temp. RPC program number */

/*
 * External routines not declared in included header files.
 */

extern struct timeval *_netmgt_get_request_time();
extern struct timeval *_netmgt_request_action(char *, u_long, u_long, char *,
				u_long, u_long, struct timeval, u_int,
				struct in_addr *);
extern bool_t	       _netmgt_check_activity(struct timeval, struct in_addr,
				 u_long, u_long, struct timeval);
extern bool_t		netmgt_set_argument (Netmgt_arg *);
extern bool_t	       _netmgt_set_auth_flavor (int);

/*
 * Static globals.
 */

static struct timeval *snm_tsp = NULL;	/* SNM timestamp for method request */
static boolean_t  adm_last_report;	/* Last report received from agent? */
static Adm_data	  *outputp;		/* Output from method */
static Adm_error  *errp;		/* Error information about request */
static int	  estat;		/* Method exit status */
static boolean_t  is_request_try;	/* Should request be tried or retried? */
static int	  tries;		/* # of time we tried a request */
static char	 *flavor_names;		/* List of authentication names to */
					/* use when re-trying a request. */
static u_int auth_flavors[ADM_AUTH_MAXFLAVORS]; /* Acceptable auth flavors */
static u_int auth_flav_num;		/* # of acceptable flavors in list */

/*
 * Performance measures.
 */

#ifdef ADM_DEBUG

static ADM_TMG_T adm_ru_init;		/* initialization performance */
static ADM_TMG_T adm_ru_call;		/* adm_perf_method() performance */
static ADM_TMG_T adm_ru_build[10];	/* request build performances */

#endif ADM_DEBUG

/*
 * Forward declarations.
 */

static char  *pv(char *);
static int    adm_build_args(Adm_error *, u_long, Adm_requestID, char *,
			     char *, char *, char *, char *, char *, char *,
			     struct timeval, struct timeval, u_int,
			     struct timeval, Adm_data *);
static int    adm_build_fence(Adm_error *, Adm_requestID, char *, char *, char *,
			      char *, char *, char *, char *, struct timeval,
			      struct timeval, u_int, struct timeval, Adm_data *);
static int    adm_build_methargs(Adm_error *, Adm_data *);
static int    adm_get_results(Adm_error *, struct in_addr, u_long, u_long,
				struct timeval *, struct timeval,
				struct timeval, u_int, struct timeval);
static int    adm_init_request(Adm_error *, u_long, u_long *, int *, int *,
			       Adm_requestID *);
static int    adm_make_request(Adm_error *, u_long, struct timeval *,
			       Adm_requestID, char *, char *, char *, char *,
			       char *, char *, u_long, u_long, u_long,
			       struct timeval, struct timeval, struct timeval,
			       u_int, struct timeval, Adm_data *);
static int    adm_parse_opts(struct timeval *, struct timeval *, u_long *,
			     u_long *, char **, char **, char **, char **,
			     u_int *, u_int [], char **, u_int *,
			     struct timeval *, struct timeval *, char **,
			     u_long *, int *, va_list);

static boolean_t adm_ping_agent(struct timeval, struct in_addr, u_long, u_long,
			     u_int, struct timeval);
static boolean_t adm_proc_fence(Adm_error *, Adm_data *);
static boolean_t adm_proc_methargs(Adm_error *, Adm_data *);
static boolean_t adm_rec_data(Adm_error *, Adm_data *);
static boolean_t adm_rec_error(int *, Adm_error *);
static void   adm_rendez(u_int, char *, char *, char *, u_int, struct timeval,
			 u_int);
static int    adm_set_auth(u_int *, Adm_error *, u_long, u_int, u_int[], char *);
static int    adm_set_snm_arg(Adm_error *, boolean_t, char *, u_int, u_int,
			      caddr_t);
static int    adm_svc_run(struct timeval *);

/*
 *-------------------------------------------------------------------------
 *
 * PV( s ):
 *
 *	Return a printable value for string s.  If s is not NULL, a pointer
 *	to s is returned.  If s is NULL, a pointer to the string "??" is
 *	returned.
 *
 *-------------------------------------------------------------------------
 */

static
char *
pv(char *s)
{
	if (s != NULL) {
		return(s);
	} else {
		return(dgettext(ADM_TEXT_DOMAIN, ADM_NULLSTRING));
	}
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_AMCL_CLEANUP( udp_sock, tcp_sock, rendez_prog, rendez_vers ):
 *
 *	Clean up the temporary RPC number used to handle class agent
 *	report callbacks.  The specified UDP and TCP socket descriptors
 *	are closed and the specified temporary RPC program and version
 *	numbers are unregistered.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_amcl_cleanup(
	int udp_sock,
	int tcp_sock,
	u_long rendez_prog,
	u_long rendez_vers
	)
{
	pid_t tpid;

	tpid = getpid();

	/*
	 * Close rendezvous sockets.
	 */

	if (udp_sock != RPC_ANYSOCK) {
		ADM_DBG("i", ("Invoke: closing rendezvous UDP socket"));
		close(udp_sock);
	}
	if (tcp_sock != RPC_ANYSOCK) {
		ADM_DBG("i", ("Invoke: closing rendezvous TCP socket"));
		close(tcp_sock);
	}

	/*
	 * Unregister the temporary RPC number used for the rendezvous.
	 * Only do this if the RPC number was allocated from this
	 * process.  Otherwise, this process was forked from the one
	 * that allocated the RPC number, and so does not own the RPC number.
	 */

	if (tpid == adm_pid) {
	    if (rendez_prog != 0L) {
		ADM_DBG("i", ("Invoke: unregistering rendezvous RPC"));
		netmgt_unregister_callback(rendez_prog, rendez_vers);
	    }
	} else {
	    adm_pid = tpid;
	}

	is_rpc_allocated = B_FALSE;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_BUILD_ARGS( errorp, flags, reqID, adm_class, class_vers, method, host,
 *		   domain, client_group, categories, rep_timeout,
 *		   ping_timeout, ping_cnt, ping_delay, in_handlep ):
 *
 *	This routine sets up all of the neccessary SNM argument
 *	necessary to make an administrative method request.  It
 *	sets up both the system arguments (see adm_build_fence())
 *	and the method arguments (see adm_build_methargs()).
 *
 *	The flags argument should be the framework control flags
 *	being used for this method invocation.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *	If an error occurs, the specified error structure will be filled
 *	in with details concerning the error, and an appropriate error
 *	code will be returned.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_build_args(
	Adm_error *errorp,
	u_long flags,
	Adm_requestID reqID,
	char *adm_class,
	char *class_vers,
	char *method,
	char *host,
	char *domain,
	char *client_group,
	char *categories,
	struct timeval rep_timeout,
	struct timeval ping_timeout,
	u_int ping_cnt,
	struct timeval ping_delay,
	Adm_data *in_handlep
	)
{
	int stat;

	if ((flags & ADM_LOCAL_REQUEST_FLAG) != 0) {
		return(ADM_SUCCESS);
	}

	ADM_DBG("cia", ("Call: building SNM argument list..."));

	stat = adm_build_fence(errorp, reqID, adm_class, class_vers, method, host,
			       domain, client_group, categories, rep_timeout,
			       ping_timeout, ping_cnt, ping_delay, in_handlep);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	stat = adm_build_methargs(errorp, in_handlep);

	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_BUILD_FENCE( errorp, reqID, adm_class, class_vers, method, host,
 *		    domain, client_group, categories, rep_timeout,
 *		    ping_timeout, ping_cnt, ping_delay, in_handlep ):
 *
 *	Build a fence in the current SNM request argument list to
 *	contain the special administrative framework system arguments
 *	for the method request.  The fence will include the following
 *	arguments in the order shown:
 *
 *		Name	 	  Used For:
 *		----		  ---------
 *
 *		ADM_VERSION	  Internal protocol version being used
 *				  between AMCL and AMSL.
 * 
 *		ADM_LANG	  Client's language preference.
 *
 *		ADM_REQUESTID	  Request ID for this method invocation.
 *
 *		ADM_CLASS	  Name of class being invoked.
 *
 *		ADM_CLASS_VERS	  Class version number.  This argument will
 *				  only be set if the "class_vers" specification
 *				  is not ADM_DEFAULT_VERS.
 *
 *		ADM_METHOD	  Name of method being invoked.
 *
 *		ADM_HOST	  Target of method invocation.
 *				  This argument will only be set if
 *				  the "host" specification is non-null.
 *
 *		ADM_DOMAIN	  Name of domain in which to process request.
 *				  This argument will only be set if
 *				  the "domain" specification is non-null.
 *
 *		ADM_CLIENT_DOMAIN SecureRPC domain of the caller.
 *
 *		ADM_CLIENT_GROUP  Preferred group under which to exec
 *				  the method runfile.
 *
 *		ADM_CATEGORIES	  Additional non-standard category names
 *				  to include on the automatic tracing message
 *				  associated with this method request.
 *
 *		ADM_TIMEOUT_PARMS Timeout parameters used in this method
 *				  request.  The format of this string is:
 *
 *					TTL=<#> PTO=<#> PCNT=<#> PDLY=<#>
 *
 *				  TTL specifies the maximum time-to-live
 *				  (in seconds) for this method request,
 *				  PTO specifies the timeout to wait for a
 *				  ping acknowledgement, PCNT specifies the
 *				  number of times to ping a class agent
 *				  before assuming it has crashed, and PDLY
 *				  specifies the inital delay before
 *				  beginning pinging activities.
 *				  
 *		NETMGT_OPTSTRING  Unformatted input to the method.
 *				  This argument will only be set if
 *				  the input handle specification (in_handlep)
 *				  contains unformatted data.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *	If an error occurs, the specified error structure will be filled
 *	in with details concerning the error, and an appropriate error
 *	code will be returned.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_build_fence(
	Adm_error *errorp,
	Adm_requestID reqID,
	char *adm_class,
	char *class_vers,
	char *method,
	char *host,
	char *domain,
	char *client_group,
	char *categories,
	struct timeval rep_timeout,
	struct timeval ping_timeout,
	u_int ping_cnt,
	struct timeval ping_delay,
	Adm_data *in_handlep
	)
{
	char *lang;			    /* Language preference */
	char srpc_dom[SYS_NMLN+1];	    /* Secure RPC domain name */
	long srpc_dom_len;		    /* Secure RPC domain name length */
	char rid[ADM_MAXRIDLEN+1];	    /* Request ID in string format */
	u_int ridlen;			    /* Length of request ID */
	char timeouts[sizeof(ADM_TIMEOUTS_FMT)+(3*ADM_MAXLONGLEN)+
		      ADM_MAXINTLEN];	    /* Request timeout specifications */
	int tlen;			    /* Length of above string */
	int version;			    /* AMCL/ASML protocol version */
	int stat;

	ADM_DBG("ia", ("Invoke: creating system argument list"));

	/*
	 * AMCL/AMSL internal protocol version.
	 */

	version = ADM_VERSION;
	stat = adm_set_snm_arg(errorp, B_TRUE, ADM_VERSION_NAME,
			       (u_int) NETMGT_INT, (u_int) sizeof(int),
			       (caddr_t) &version);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Language preference.
	 */

	lang = setlocale(LC_ALL, NULL);
	if (lang != NULL) {
		stat = adm_set_snm_arg(errorp, B_TRUE, ADM_LANG_NAME,
				NETMGT_STRING, (u_int)(strlen(lang) + 1),
				lang);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
	}

	/*
	 * Request ID of method invocation.
	 */

	stat = adm_reqID_rid2str(reqID, rid, (u_int)(ADM_MAXRIDLEN+1), &ridlen);
	if (stat != ADM_SUCCESS) {
		adm_err_fmtf(errorp, ADM_ERR_RIDCVT, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_RIDCVT), stat,
			     reqID.clnt_pid, reqID.clnt_time, reqID.clnt_count,
			     adm_err_msg(stat));
		return(ADM_ERR_RIDCVT);
	} else {
		stat = adm_set_snm_arg(errorp, B_TRUE, ADM_REQUESTID_NAME,
				       NETMGT_STRING, (u_int)(ridlen + 1), rid);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
	}

	/*
	 * Class name.
	 */

	stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CLASS_NAME, NETMGT_STRING,
			       (u_int)(strlen(adm_class) + 1), adm_class);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Class version.
	 */

	if (class_vers != ADM_DEFAULT_VERS) {
	    stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CLASS_VERS_NAME,
 				   NETMGT_STRING,
			       	   (u_int)(strlen(class_vers) + 1), class_vers);
	    if (stat != ADM_SUCCESS) {
		return(stat);
	    }
	}

	/*
	 * Method name.
	 */

	stat = adm_set_snm_arg(errorp, B_TRUE, ADM_METHOD_NAME, NETMGT_STRING,
			       (u_int)(strlen(method) + 1), method);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Host name.
	 */

	if (host != NULL) {
	    stat = adm_set_snm_arg(errorp, B_TRUE, ADM_HOST_NAME, NETMGT_STRING,
			       	   (u_int)(strlen(host) + 1), host);
	    if (stat != ADM_SUCCESS) {
		return(stat);
	    }
	}

	/*
	 * Domain name.
	 */

	if (domain != NULL) {
	    stat = adm_set_snm_arg(errorp, B_TRUE, ADM_DOMAIN_NAME, NETMGT_STRING,
			       	   (u_int)(strlen(domain) + 1), domain);
	    if (stat != ADM_SUCCESS) {
		return(stat);
	    }
	}

	/*
	 * Client's host name.
	 */

	if (adm_host[0] != NULL) {
		stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CLIENT_HOST_NAME,
				       NETMGT_STRING,
				       (u_int)(strlen(adm_host)+1),
			       	       adm_host);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
	}

	/*
	 * Secure RPC domain name.
	 */

	srpc_dom_len = sysinfo(SI_SRPC_DOMAIN, srpc_dom, (long)(SYS_NMLN+1));
	if ((srpc_dom_len != -1) && (srpc_dom_len <= (u_int)(SYS_NMLN+1))) {
		stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CLIENT_DOMAIN_NAME,
				       NETMGT_STRING,
			       	       (u_int)(srpc_dom_len), srpc_dom);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
	}

	/*
	 * Client's preferred group.
	 */

	if (client_group != NULL) {
	    stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CLIENT_GROUP_NAME,
				   NETMGT_STRING,
			       	   (u_int)(strlen(client_group) + 1),
				   client_group);
	    if (stat != ADM_SUCCESS) {
		return(stat);
	    }
	}

	/*
	 * Additional non-standard tracing message categories.
	 */

	if (categories != ADM_NOCATS) {
	    stat = adm_set_snm_arg(errorp, B_TRUE, ADM_CATEGORIES_NAME,
				   NETMGT_STRING,
			       	   (u_int)(strlen(categories) + 1),
				   categories);
	    if (stat != ADM_SUCCESS) {
		return(stat);
	    }
	}

	/*
	 * Timeout specifications for request.
	 */

	tlen = sprintf(timeouts, ADM_TIMEOUTS_FMT, rep_timeout.tv_sec,
			ping_timeout.tv_sec, ping_cnt, ping_delay.tv_sec);
	if (tlen < 0) {
		adm_err_fmtf(errorp, ADM_ERR_SYSARGFMT, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_SYSARGFMT),
			     ADM_TIMEOUT_PARMS_NAME, errno);
		return(ADM_ERR_SYSARGFMT);
	}
	stat = adm_set_snm_arg(errorp, B_TRUE, ADM_TIMEOUT_PARMS_NAME,
			       NETMGT_STRING, (u_int)(tlen + 1), timeouts);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Unformatted input.
	 */

	if (in_handlep != NULL) {
	  if (in_handlep->unformattedp != NULL) {
		stat = adm_set_snm_arg(errorp, B_TRUE, ADM_UNFMT_NAME,
				NETMGT_STRING, (u_int)(in_handlep->unformatted_len + 1),
				in_handlep->unformattedp);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
	  }
	}

	/*
	 * End-of-fence indiciation argument.
	 */

	stat = adm_set_snm_arg(errorp, B_TRUE, ADM_FENCE_NAME, NETMGT_STRING,
			       (u_int) 0, "");
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_BUILD_METHARGS( errorp, in_handlep ):
 *
 *	This routine sets the administrative method argument values
 *	in a SNM agent request.  The arguments are taken from the
 *	specified administrative data handle.  It is assumed that
 *	the handle contains only one row of data.
 *
 *	Upon successful completion, the routine returns ADM_SUCCESS.
 *	If an error occurs, the specified error structure will be
 *	filled in with details concerning the error and an appropriate
 *	error code will be returned.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_build_methargs(
	Adm_error *errorp,
	Adm_data *in_handlep
	)
{
	Adm_arglink *alinkp;		/* Link to method argument */
	Adm_arg *argp;			/* Method argument */
	int stat;

	if (in_handlep == NULL) {
		ADM_DBG("ia", ("Invoke: no input arguments to method"));
		return(ADM_SUCCESS);
	}
	if (in_handlep->first_rowp == NULL) {
		ADM_DBG("ia", ("Invoke: no input arguments to method"));
		return(ADM_SUCCESS);
	}
	if (in_handlep->first_rowp != in_handlep->last_rowp) {
		adm_err_fmtf(errorp, ADM_ERR_ROWS, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_ROWS));
		return(ADM_ERR_ROWS);
	}

	ADM_DBG("ia", ("Invoke: setting method arguments"));

	alinkp = in_handlep->first_rowp->first_alinkp;
	while(alinkp != NULL) {
		argp = alinkp->argp;
		stat = adm_set_snm_arg(errorp, B_FALSE, argp->name, NETMGT_STRING,
			    (u_int)(argp->valuep == NULL ? 0 : argp->length + 1),
			    (argp->valuep == NULL ? "" : argp->valuep));
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
		alinkp = alinkp->next_alinkp;
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_INIT_REQUEST( errorp, flags, rendezp, udp_sockp, tcp_sockp, reqIDp ):
 *
 *	Initialize a new administrative class method request.  This
 *	routine generates a new request ID for the method request, and
 *	initializes SunNet Manager for the new request.  The standard
 *	administrative framework callback routine (specified by the
 *	adm_rendez variable) is registered as the RPC server for the
 *	error and data reports from the method.
 *
 *	This routine returns the registered callback RPC number in
 *	*rendezp, the UDP and TCP socket descriptors associated with
 *	the RPC number in *udp_sockp and *tcp_sockp.  It also returns
 *	the new request ID in *reqIDp.
 *
 *	The flags argument should specify the framework control flags
 *	being used for this method invocation.
 *
 *	Upon successful completion, the routine returns ADM_SUCCESS.
 *	If an error occurs, details about the error will be placed in
 *	the specified error structure and an appropriate error code will
 *	be returned.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_init_request(
	Adm_error *errorp,
	u_long flags,
	u_long *rendezp,
	int *udp_sockp,
	int *tcp_sockp,
	Adm_requestID *reqIDp
	)
{
	u_long rendez_prog;		/* RPC program number for results */
	int stat;

	*udp_sockp = RPC_ANYSOCK;
	*tcp_sockp = RPC_ANYSOCK;
	*rendezp = 0L;

	/*
	 * Generate a request ID for the new request.
	 */

	stat = adm_reqID_new(reqIDp);
	if (stat != ADM_SUCCESS) {
		ADM_DBG("cie", ("Error: unable to generate request ID"));
		adm_err_fmtf(errorp, ADM_ERR_RIDGEN, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_RIDGEN), stat,
			     adm_err_msg(stat));
		return(ADM_ERR_RIDGEN);
	}
	ADM_DBG("ci", ("Call: initializing method request, ID = %ld:%ld:%lu",
		reqIDp->clnt_pid, reqIDp->clnt_time, reqIDp->clnt_count));

	/*
	 * If the method is being dispatched locally, there is no need
	 * to register a callback RPC number for receiving results.
	 */

	if ((flags & ADM_LOCAL_REQUEST_FLAG) != 0) {
		return(ADM_SUCCESS);
	}

	/*
	 * Register temporary RPC to receive results.  If one is already
	 * registered, then just re-use it.  Otherwise, save the RPC
	 * number for re-use.
	 */

	if (is_rpc_allocated) {

	    ADM_DBG("i", ("Invoke: re-using previous callback sockets"));

	    rendez_prog = t_rendez_prog;
	    *udp_sockp = t_udp_sock;
	    *tcp_sockp = t_tcp_sock;

	} else {

	    rendez_prog = netmgt_register_callback(adm_rendez, udp_sockp,
				tcp_sockp, (u_long) RENDEZ_VERS,
				(u_long) (IPPROTO_UDP | IPPROTO_TCP));
	    if (rendez_prog == NULL) {

		ADM_DBG("ie", ("Error: unable to allocate callback sockets"));

		adm_err_fmtf(errorp, ADM_ERR_NORENDEZ, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_NORENDEZ),
			     netmgt_error.service_error, pv(netmgt_sperror()));
		adm_amcl_cleanup(*udp_sockp, *tcp_sockp, rendez_prog,
				(u_long) RENDEZ_VERS);
		return(ADM_ERR_NORENDEZ);

	    } else {

		ADM_DBG("i", ("Invoke: allocated callback RPC %ul, "
			      "UdpSock=%d, TcpSock=%d", rendez_prog,
			      *udp_sockp, *tcp_sockp));

		t_rendez_prog = rendez_prog;
		t_udp_sock = *udp_sockp;
		t_tcp_sock = *tcp_sockp;
		is_rpc_allocated = B_TRUE;
	    }
	}
	*rendezp = rendez_prog;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_GET_RESULTS( errorp, agent_addr, agent_prog, agent_vers, req_tsp,
 *		    rep_timeout, ping_timeout, ping_cnt, ping_delay ):
 *
 *	Retrieve the results of a method invocation.  This routine
 *	drops into the framework's special version of svc_run()
 *	(adm_svc_run()) to wait for and process result callbacks
 *	from the class agent.  "agent_addr", "agent_prog", and
 *	"agent_vers" should be the host IP address, RPC program number,
 *	and RPC version number of the class agent processing the method
 *	request.  "req_tsp" should point to the SNM timestamp for the
 *	request.
 *
 *	This routine is primarily concerned with monitoring the
 *	request to determine if it crashes.  It periodically pings
 *	the class agent to determine if the request is still being
 *	processed.  Pinging activities begin after the delay specified
 *	by "ping_delay".  Each ping attempt will be retried up to
 *	"ping_cnt" number of times, and each ping times-out after
 *	the interval specified by "ping_timeout".  The maximum
 *	time-to-live for the request is specified by "rep_timeout".
 *	After this period, the method or agent are assumed to have
 *	hung and the overall request times-out.  "rep_timeout" may
 *	be set to zero to indicate an infinite time-to-live for the
 *	request.
 *
 *	Upon completion of the method, this routine returns ADM_SUCCESS.
 *	If an error occurs, or if the method request times-out, this
 *	routine fills in the error structure pointed to by "errorp"
 *	and returns an appropriate error code.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_get_results(
	Adm_error *errorp,
	struct in_addr agent_addr,
	u_long agent_prog,
	u_long agent_vers,
	struct timeval *req_tsp,
	struct timeval rep_timeout,
	struct timeval ping_timeout,
	u_int ping_cnt,
	struct timeval ping_delay
	)
{
	int next_backoff;	     /* Index of next backoff factor to use */
	struct timeval current;	     /* Current time */
	struct timeval future;	     /* Time next set of ping attempts occurs */
	struct timeval next_ping;    /* Timeout to next set of ping attempts */
	struct timeval max_delay;    /* Max. allowable interval between pings */
	struct timeval expire;	     /* Time at which request is assumed */
				     /* to be hung - 0 indicates never */
	u_long s;
	int stat;

	/*
	 * The following array is used to iteratively increase the
	 * delay between ping attempts as a method keeps running.
	 * After each successful ping attempt, the delay is increased
	 * by one of the factors in the array (each factor repesents
	 * hundredths), until the maximum delay threshhold is reached.
	 * Entries in the array are used cyclicly.
	 */

static u_long adm_backoff[] = { 30, 50, 80, 100, 100, 0 };

	/*
	 * The following macros are used in this routine to manipulate
	 * time values:
	 *
	 *	o Add time to a base time.
	 *	o Subtract time from a base time.
	 *	o Multiply a base time by a fraction (in hundredths).
	 *	o Determine if a timeout represents an infinite value.
	 */

#define ADM_ADD_TIME(base_time, add_time)			\
								\
		base_time.tv_sec  += add_time.tv_sec;		\
		base_time.tv_usec += add_time.tv_usec;		\
		if (base_time.tv_usec > (long) MICROSEC) {	\
			base_time.tv_sec  += (long) 1;		\
			base_time.tv_usec %= (long) MICROSEC;	\
		}

#define ADM_SUB_TIME(base_time, sub_time)			\
								\
		base_time.tv_sec -= sub_time.tv_sec;		\
		if (base_time.tv_usec >= sub_time.tv_usec) {	\
			base_time.tv_usec -= sub_time.tv_usec;	\
		} else {					\
			base_time.tv_sec  -= 1;			\
			base_time.tv_usec = (long) MICROSEC	\
					  - sub_time.tv_usec	\
					  + base_time.tv_usec;	\
		}
	
#define ADM_MULT_TIME(base_time, hunds)				\
								\
		base_time.tv_usec += (hunds * base_time.tv_usec) / (long) 100; \
		s = base_time.tv_sec * hunds;			\
		base_time.tv_sec += s / (long) 100;		\
		base_time.tv_usec += (s % (long) 100) * (long) 10000;	 \
		base_time.tv_sec += base_time.tv_usec / (long) MICROSEC; \
		base_time.tv_usec %= (long) MICROSEC;

#define ADM_IS_INFINITE_TIMEOUT(timeout)			\
								\
		((timeout.tv_sec == 0L) && (timeout.tv_usec == 0L))

	/*
	 * Determine expiration time for receiving results of request.
	 */

	if (ADM_IS_INFINITE_TIMEOUT(rep_timeout)) {
		expire = rep_timeout;
	} else {
		stat = gettimeofday(&expire, NULL);
		if (stat == -1) {
			adm_err_fmtf(errorp, ADM_ERR_BADTIME, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADTIME));
			return(ADM_ERR_BADTIME);
		}
		ADM_ADD_TIME(expire, rep_timeout);
	}

	/*
	 * Specify the initial timeout before beginning pinging.
	 */

	if (ADM_IS_INFINITE_TIMEOUT(ping_delay)) {

		next_ping = rep_timeout;

	} else if ((!ADM_IS_INFINITE_TIMEOUT(rep_timeout)) &&
		    (timercmp(&rep_timeout, &ping_delay, <))) {

		next_ping = rep_timeout;

	} else {

		next_ping = ping_delay;
	}

	/*
	 * Determine the maximum delay between ping attempts.
	 */

	max_delay = ping_delay;
	ADM_MULT_TIME(max_delay, ADM_MAX_PING_DELAY_FACTOR);

	/*
	 * Iteratively switch between waiting for the method request's
	 * results using adm_svc_run(), and pinging the class agent to
	 * determine if the method is still running.  Do this until
	 * either the results are received, the maximum time-to-live
	 * for the method expires, or a ping is not acknowledged.
	 */

	next_backoff = 0;
	while (B_TRUE) {

		ADM_DBG("ci", ("Call: waiting for method callback..."));

		stat = adm_svc_run((ADM_IS_INFINITE_TIMEOUT(next_ping) ?
						NULL : &next_ping));

		/* Finish if the method results were received or an */
		/* unexpected error occurred */

		if (stat == ADM_SUCCESS) {
			ADM_DBG("ci", ("Call: request completed..."));
			break;
		}
		if (stat == ADM_ERR_LOSTREG) {
			adm_err_fmtf(errorp, ADM_ERR_LOSTREG, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_LOSTREG));
			break;
		}

		/* Finish if the maximum time-to-live for the method */
		/* has been exceeded */

		stat = gettimeofday(&current, NULL);
		if (stat == -1) {
			adm_err_fmtf(errorp, ADM_ERR_BADTIME, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADTIME));
			stat = ADM_ERR_BADTIME;
			break;
		}
		if (!ADM_IS_INFINITE_TIMEOUT(expire)) {
		    if (timercmp(&current, &expire, >) ||
			timercmp(&current, &expire, ==)) {
			ADM_DBG("ci", ("Call: request timed out..."));
			adm_err_fmtf(errorp, ADM_ERR_TIMEOUT, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_TIMEOUT));
			stat = ADM_ERR_TIMEOUT;
			break;
		    }
		}

		/* Ping the class agent to determine if the method is */
		/* still running.  If it is not, then finish the request */

		if (!adm_ping_agent(*req_tsp, agent_addr, agent_prog,
				    agent_vers, ping_cnt, ping_timeout)) {
			adm_err_fmtf(errorp, ADM_ERR_TIMEOUT, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_TIMEOUT));
			stat = ADM_ERR_TIMEOUT;
			break;
		}

		/* Compute the delay until the next pinging attempt */
		/* should be made.  Backoff the delay up to the maximum */
		/* possible delay, but do not exceed the expiration */
		/* tie for the request */

		if (timercmp(&next_ping, &max_delay, !=)) {
			ADM_MULT_TIME(next_ping, adm_backoff[next_backoff++]);
			if (adm_backoff[next_backoff] == 0) {
				next_backoff = 0;
			}
			if (timercmp(&next_ping, &max_delay, >)) {
				next_ping = max_delay;
			}
		}
		if (!ADM_IS_INFINITE_TIMEOUT(expire)) {
			future = current;
			ADM_ADD_TIME(future, next_ping);
			if (timercmp(&future, &expire, >)) {
				next_ping = expire;
				ADM_SUB_TIME(next_ping, current);
			}
		}
	}

	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_MAKE_REQUEST( errorp, flags, req_tsp, reqID, adm_class, class_vers,
 *		     method, client_group, categories, hostname, agent_prog,
 *		     agent_vers, rendez_prog, ack_timeout, rep_timeout,
 *		     ping_timeout, ping_cnt, ping_delay, in_handlep ):
 *
 *	Make a method request to a class agent.  The request (which
 *	should previously have been built before calling this routine)
 *	is sent to the class agent on host "hostname" at RPC program
 *	number "agent_prog" and version "agent_vers".  "req_tsp" should
 *	be a point to the previously built request's SNM timestamp.
 *
 *	Before this routine is called, the caller should have registered
 *	a SNM rendezvous at which to collect the request results.
 *	"rendez_prog" should be set to the RPC program number of this
 *	rendezvous.  "ack_timeout" and "rep_timeout" should be the
 *	timeout to use to wait for the initial request acknowledgement
 *	and request results, respectively.  "ping_timeout", "ping_cnt",
 *	and "ping_delay" specify the timeout to wait for an acknowledgement
 *	to a ping request, the number of times to try pinging an agent
 *	before assuming that it has crashed, and the delay at the start
 *	of a request before beginnning pinging activities, respectively.
 *
 *	If the ADM_LOCAL_REQUEST_FLAG flag is set in the "flags" argument,
 *	the requested method is dispatched locally, without using RPC
 *	or a class agent.  This mode of operation should be useful for
 *	debugging methods and for invoking methods under conditions
 *	where a class agent is not available (such as at boot-time).
 *	When dispatching a method locally, "reqID" should be set to the
 *	request ID of the method invocation, "class" "class_vers" and
 *	"method" should be set to the name of the class, version, and
 *	method being invoked, client_group should be set to the preferred
 *	group name under which to execute the request, and "categories"
 *	should be set to any additional categories (beyond the standard
 *	ones) with which to tag the diagnostic message for this request.
 *
 *	If successful, this routine returns ADM_SUCCESS and returns
 *	the method results in the appropriate global variables.
 *	If unsuccessful, this routine fills the error structure
 *	"pointed to by "errorp" with information regarding the error,
 *	and returns an appropriate error code.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_make_request(
	Adm_error *errorp,
	u_long flags,
	struct timeval *req_tsp,
	Adm_requestID reqID,
	char *adm_class,
	char *class_vers,
	char *method,
	char *client_group,
	char *categories,
	char *hostname,
	u_long agent_prog,
	u_long agent_vers,
	u_long rendez_prog,
	struct timeval ack_timeout,
	struct timeval rep_timeout,
	struct timeval ping_timeout,
	u_int ping_cnt,
	struct timeval ping_delay,
	Adm_data *in_handlep
	)
{
	struct in_addr agent_addr;	/* IP address of destination host */
	struct timeval *tsp;		/* SNM request timestamp */
	u_int type;			/* Error type */
	u_int clean;			/* Clean or dirty failure */
	char *messagep;			/* Error message */
	boolean_t print_pid;		/* Print method PID if local dispatch? */
	u_int ctl_flags;		/* AMSL control flags */
	Adm_rowlink *crowp;		/* Current row of input arg handle */
	Adm_arglink *calinkp;		/* Current arg in input arg handle */
	int stat;

	/*
	 * If dispatching the method locally, invoke a special interface
	 * into the AMSL, and return the results it returns.  (Note: we're
	 * careful to preserve the current row and argument pointers in
	 * the input argument handle in case the AMSL changes them.
	 */

	if ((flags & ADM_LOCAL_REQUEST_FLAG) != 0) {
		ADM_DBG("i", ("Invoke: request is local dispatch"));
		adm_get_local_dispatch_info(&print_pid);
		ctl_flags = (print_pid ? AMSL_DEBUG_CHILD : 0);
		crowp   = in_handlep->current_rowp;
		calinkp = in_handlep->current_alinkp;
		estat = amsl_local_dispatch(ctl_flags, &reqID, adm_class,
			    class_vers, method, in_handlep, outputp, errp,
			    client_group, categories);
		in_handlep->current_rowp   = crowp;
		in_handlep->current_alinkp = calinkp;
		return(ADM_SUCCESS);
	}

	/*
	 * Normal method dispatch though a (possibly remote) class agent.
	 */

	/*
	 * Make the request.
	 */

	ADM_DBG("ci", ("Call: making method request to host %s", hostname));

	tsp = _netmgt_request_action(hostname, agent_prog, agent_vers,
			             adm_host, rendez_prog, (u_long) RENDEZ_VERS,
			             ack_timeout, 0, &agent_addr);
	if (tsp == NULL) {
	    switch(netmgt_error.service_error) {

		case NETMGT_WARNING:
		case NETMGT_FATAL:

		    ADM_DBG("ie", ("Error: SNM action request failed"));

		    messagep = NULL;
		    stat = adm_err_str2snm(netmgt_error.message, &estat, &type,
					   &clean, &messagep);
		    if (stat == ADM_SUCCESS) {
			adm_err_fmtf(errorp, netmgt_error.agent_error,
				     type, clean, messagep);
		    } else {
		    	adm_err_fmtf(errorp, ADM_ERR_SNMAGT, ADM_ERR_SYSTEM,
			     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_SNMAGT),
			     netmgt_error.agent_error, pv(netmgt_error.message));
			estat = ADM_FAILURE;
		    }
		    if (messagep != NULL) {
			free(messagep);
		    }
		    return(errorp->code);

		case NETMGT_CANTCREATECLIENT:	     /* Agent not registered */

		    ADM_DBG("ie", ("Error: agent not registered"));

		    adm_err_fmtf(errorp, ADM_ERR_NOAGENT, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_NOAGENT),
			     hostname, agent_prog, agent_vers,
			     adm_host, hostname);
		    return(ADM_ERR_NOAGENT);

		case NETMGT_RPCFAILED:		    /* RPC failure */
		case NETMGT_CANTCREATEDESAUTH:	    /* Can't create DES cred */

		    if (netmgt_error.service_error == NETMGT_RPCFAILED) {
		    	ADM_DBG("ie", ("Error: RPC failure"));
		        adm_err_fmtf(errorp, ADM_ERR_SNMSRV, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_SNMSRV),
			     netmgt_error.service_error, pv(netmgt_sperror()));
		    } else {
		    	ADM_DBG("ie", ("Error: Can't create DES credentials"));
		        adm_err_fmtf(errorp, ADM_ERR_CREDFAIL, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_CREDFAIL));
		    }

		    auth_flavors[0] = ADM_RETRY_FLAVOR;
		    auth_flav_num = 1;
		    if (flavor_names != NULL) {
			free(flavor_names);
		    }
		    flavor_names = NULL;
		    is_request_try = B_TRUE;
		    return(ADM_ERR_CREDFAIL);

		case NETMGT_UNKNOWNHOST:
		    ADM_DBG("ie", ("rpc timeout"));
		    adm_err_fmtf(errorp, ADM_ERR_SNMBADHOST, ADM_ERR_SYSTEM,
			     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_SNMBADHOST),
			     hostname, adm_host);
		    return(NETMGT_UNKNOWNHOST);

		case NETMGT_RPCTIMEDOUT:
		    ADM_DBG("ie", ("rpc timeout"));
		    adm_err_fmtf(errorp, ADM_ERR_SNMTIMEOUT, ADM_ERR_SYSTEM,
			     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_SNMTIMEOUT),
			     hostname);
		    return(NETMGT_RPCTIMEDOUT);

		default:

		    ADM_DBG("ie", ("Error: server error"));

		    adm_err_fmtf(errorp, ADM_ERR_SNMSRV, ADM_ERR_SYSTEM,
			     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_SNMSRV),
			     netmgt_error.service_error, pv(netmgt_sperror()));
		    return(ADM_ERR_SNMSRV);
	    }
	}

	ADM_DBG("i", ("Invoke: request acknowledged"));

	if ((req_tsp == NULL) || (req_tsp->tv_sec != tsp->tv_sec) ||
		(req_tsp->tv_usec != tsp->tv_usec)) {
		    adm_err_fmtf(errorp, ADM_ERR_TWOTS, ADM_ERR_SYSTEM,
			     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_TWOTS),
			     (req_tsp == NULL ? -1 : req_tsp->tv_sec),
			     (req_tsp == NULL ? -1 : req_tsp->tv_usec),
			     (tsp == NULL ? -1 : tsp->tv_sec),
			     (tsp == NULL ? -1 : tsp->tv_usec));
		    return(ADM_ERR_TWOTS);
	}

	/*
	 * Wait for and process the results of the method request.
	 */

	stat = adm_get_results(errorp, agent_addr, agent_prog, agent_vers,
			       req_tsp, rep_timeout, ping_timeout, ping_cnt,
			       ping_delay);

	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_PARSE_OPTS( ack_timeoutp, rep_timeoutp, agent_progp, agent_versp,
 *		   classp, class_versp, hostp, domainp, num_flavorsp,
 *		   flavors[], categoriesp, ping_cntp, ping_timeoutp,
 *		   ping_delayp, client_groupp, flagsp, max_triesp,
 *		   opt_args ):
 *
 *	Parse a sequence of framework control options.  This routine is
 *	used to parse the options portion of a request to adm_perf_method().
 *	opt_args should be a stdarg(5) va_list variable pointing to the
 *	sequence of framework *	control options.  The valid framework control
 *	options are:
 *
 *	    ADM_CLASS, (char *) adm_class
 *
 *		Name of class in which the invoked method resides.
 *		A pointer to the class name is returned in *classp.
 *
 *	    ADM_CLASS_VERS, (char *) version
 *
 *		Version number of class being invoked.  If the caller does
 *		no wish to specify a version number, this option may be
 *		omitted, or the version number may be set to ADM_DEFAULT_VERS.
 *		A pointer to the version number is returned in *class_versp.
 *
 *	    ADM_HOST, (char *) host
 *
 *		Name of host on which to perform the requested method.
 *		The host specification may be ADM_LOCALHOST in order to
 *		specify method execution on the local host.  A pointer
 *		to the host specification is returned in *hostp.
 *
 *	    ADM_DOMAIN, (char *) domain
 *
 *		Name of domain in which to perform the requested method.
 *		A pointer to the domain specification is returned in
 *		*domainp.
 *
 *	    ADM_CLIENT_GROUP, (char *) client_group
 *
 *		Specify the preferred group under which to exec the
 *		requested method runfile.  A pointer to the preferred
 *		group name is returned in *client_groupp.
 *
 *	    ADM_AUTH_TYPE, (u_int) auth_type
 *
 *		Type of authentication to use when making request
 *		to agent.  The specified flavor list (flavors[]) is filled
 *		in with a list of flavors from this type, and *num_flavorsp
 *		is set to the number of flavors in this list.
 *
 *	    ADM_AUTH_FLAVOR, (u_int) auth_flavor
 *
 *		Flavor of authentication to use when making request
 *		to agent.  The specified authentication flavor is recorded
 *		in flavors[0] and *num_flavorsp is set to 1.
 *
 *	    ADM_ACK_TIMEOUT, (long) secs, (long) usecs
 *
 *		Timeout in seconds and microseconds to wait for an
 *		acknowledgement of an initial method request.  These values
 *		are placed in the timeval structure *ack_timeoutp.
 *
 *	    ADM_REP_TIMEOUT, (long) secs, (long) usecs
 *
 *		Timeout in seconds and microseconds to wait for a data
 *		report containing the results of a method request.  These
 *		values are placed in the timeval structure *rep_timeoutp.
 *		A value of zero indicates an infinite timeout.
 *
 *	    ADM_PINGS, (u_int) retries
 *
 *		Number of times to retry pinging the agent before
 *		assuming that the request failed.  The "retries" value
 *	        is returned in *pint_cntp.
 *
 *	    ADM_PING_TIMEOUT, (long) secs, (long) usecs
 *
 *		Timeout in seconds and microseconds to wait for an
 *		acknowledgement to a ping request.  These values are placed
 *		in the timeval structure *ping_timeoutp.
 *
 *	    ADM_PING_DELAY, (long) secs, (long) usecs
 *
 *		Time to delay (at the start of method request) before
 *		beginning to ping the class agent to determine if the
 *		method is running.  These values are placed in the
 *		timeval structure *ping_delayp.  A value of zero indicates
 *		that pinging should not take place.
 *
 *	    ADM_AGENT, (u_long) prog_num, (u_long) vers_num
 *
 *		RPC program and version number of agent serving the requested
 *		method.  These values are placed in *agent_progp and
 *		*agent_versp.
 *
 *	    ADM_ALLOW_AUTH_NEGO, (boolean_t) allow_nego
 *
 *		If allow_nego is B_TRUE, authentication negotiation is
 *		allowed (*max_triesp is set to 2).  If not, negotiation is
 *		disabled (*max_triesp is set to 1).
 *
 *	    ADM_LOCAL_DISPATCH, (boolean_t) local_dispatch
 *
 *		If local_dispatch is B_TRUE, the requested method, and any
 *		of its sub-methods, are dispatched locally (under the
 *		caller's UID and GID) without using SNM to send the
 *		request to a class agent.  This option is intended for
 *		use when debugging methods and for use under unusual
 *		circumstances when normal dispatch is unavailable (such
 *		as at boot-time).  This option sets the ADM_LOCAL_REQUEST_FLAG
 *		flag in the variable *flagsp.
 *
 *		NOTE: This option overrides any host specification.
 *
 *	    ADM_CATEGORIES, (char *) categories
 *
 *		Comma-separated list of categories (in addition to the
 *		default ones) in which to place the automatic tracing
 *		message that is generated by this routine.  A pointer
 *		to the categories specification is returned in
 *		*categoriesp.
 *
 *	If the sequence of framework control options does not specify a value
 *	for one of the above framework controls, a default value will be
 *	supplied.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	NOTE: An option should be added where an application can specify
 *	      the RPC program number to use as the callback from the
 *	      requested method.  This would allow the re-use of such an
 *	      RPC number between successive method requests, in order to
 *	      avoid the overhead of having the framework allocate and
 *	      register a new callback RPC for each request.
 *
 *	NOTE: This routine takes an argument of type va_list.  This may
 *	      not be very portable.  Optionally, this routine could
 *	      be merged with adm_perf_method().
 *
 *--------------------------------------------------------------------
 */

static
int
adm_parse_opts(
	struct timeval *ack_timeoutp,
	struct timeval *rep_timeoutp,
	u_long *agent_progp,
	u_long *agent_versp,
	char **classp,
	char **class_versp,
	char **hostp,
	char **domainp,
	u_int *num_flavorsp,
	u_int flavors[],
	char **categoriesp,
	u_int *ping_cntp,
	struct timeval *ping_timeoutp,
	struct timeval *ping_delayp,
	char **client_groupp,
	u_long *flagsp,
	int *max_triesp,
	va_list opt_args
	)
{
	int option;			/* Framework control option */
	long secs;			/* Timeout seconds */
	long usecs;			/* Timeout microseconds */
	u_long prog_num;		/* Agent RPC program number */
	u_long vers_num;		/* Agent RPC version number */
	u_int auth_type;		/* Authentication type */
	u_int auth_flavor;		/* Authentication flavor */
	boolean_t allow_nego;		/* Allow authentication negotiation? */
	boolean_t local_dispatch;	/* Invoke method locally w/o RPC? */
	int stat;

	/* Set up default values */

	ack_timeoutp->tv_sec   = ADM_ACK_TSECS;
	ack_timeoutp->tv_usec  = ADM_ACK_TUSECS;
	rep_timeoutp->tv_sec   = adm_rep_timeout;
	rep_timeoutp->tv_usec  = (long) 0;
	ping_timeoutp->tv_sec  = adm_ping_timeout;
	ping_timeoutp->tv_usec = (long) 0;
	ping_delayp->tv_sec    = adm_ping_delay;
	ping_delayp->tv_usec   = (long) 0;
	*ping_cntp     = adm_ping_cnt;
	*agent_progp   = ADM_CLASS_AGENT_PROG;
	*agent_versp   = ADM_CLASS_AGENT_VERS;
	*classp        = NULL;
	*class_versp   = ADM_DEFAULT_VERS;
	*hostp         = ADM_LOCALHOST;
	*domainp       = NULL;
	auth_type      = adm_auth_init_type;
	auth_flavor    = adm_auth_init_flavor;
	*flagsp	       = adm_flags;
	*max_triesp    = (ADM_DEFAULT_NEGO ? 2 : 1);
	*categoriesp   = ADM_NOCATS;
	*client_groupp = NULL;

	/* Parse the framework control options sequence */

	option = va_arg(opt_args, int);
	while (option != ADM_ENDOPTS) {
	    switch (option) {

		case ADM_CLASS:			/* Class name */

			*classp = va_arg(opt_args, char *);
			break;

		case ADM_CLASS_VERS:		/* Class version number */

			*class_versp = va_arg(opt_args, char *);
			break;

		case ADM_HOST:			/* Host name */

			*hostp = va_arg(opt_args, char *);
			break;

		case ADM_DOMAIN:		/* Domain name */

			*domainp = va_arg(opt_args, char *);
			break;

		case ADM_AUTH_TYPE:		/* Authentication type */

			auth_type = va_arg(opt_args, u_int);
			break;

		case ADM_AUTH_FLAVOR:		/* Authentication flavor */

			auth_flavor = va_arg(opt_args, u_int);
			auth_type = ADM_AUTH_UNSPECIFIED;
			break;

		case ADM_ALLOW_AUTH_NEGO:	/* Allow auth negotiation */

			allow_nego = va_arg(opt_args, boolean_t);
			*max_triesp = (allow_nego ? 2 : 1);
			break;

		case ADM_ACK_TIMEOUT:		/* Timeout */
		case ADM_REP_TIMEOUT:
		case ADM_PING_TIMEOUT:
		case ADM_PING_DELAY:

			secs = va_arg(opt_args, long);
			usecs = va_arg(opt_args, long);
			if ((secs < 0L) || (usecs < 0L)) {
				return(ADM_ERR_BADTIMEOUT);
			}
			if (usecs > 0L) {    /* Round to next highest second */
				secs += 1L;
				usecs = 0L;
			}
			switch (option) {
			    case ADM_ACK_TIMEOUT:
				ack_timeoutp->tv_sec = secs;
				ack_timeoutp->tv_usec = usecs;
				break;
			    case ADM_REP_TIMEOUT:
				rep_timeoutp->tv_sec = secs;
				rep_timeoutp->tv_usec = usecs;
				break;
			    case ADM_PING_TIMEOUT:
				ping_timeoutp->tv_sec = secs;
				ping_timeoutp->tv_usec = usecs;
				break;
			    case ADM_PING_DELAY:
				ping_delayp->tv_sec = secs;
				ping_delayp->tv_usec = usecs;
				break;
			}
			break;

		case ADM_PINGS:			/* Ping retry limit */

			*ping_cntp = va_arg(opt_args, u_int);
			break;

		case ADM_AGENT:			/* Agent RPC number */

			prog_num = va_arg(opt_args, u_long);
			vers_num = va_arg(opt_args, u_long);
			if ((prog_num == 0) || (vers_num == 0)) {
				return(ADM_ERR_BADAGENT);
			}
			*agent_progp = prog_num;
			*agent_versp = vers_num;
			break;

		case ADM_CATEGORIES:		/* Tracing message categories */

			*categoriesp = va_arg(opt_args, char *);
			break;

		case ADM_CLIENT_GROUP:		/* Client's preferred group */

			*client_groupp = va_arg(opt_args, char *);
			break;

		case ADM_LOCAL_DISPATCH:	/* Invoke method w/o RPC? */

			local_dispatch = va_arg(opt_args, boolean_t);
			if (local_dispatch) {
				*flagsp |= ADM_LOCAL_REQUEST_FLAG;
			} else if ((adm_flags & ADM_LOCAL_REQUEST_FLAG) == 0L) {
				*flagsp &= ~ADM_LOCAL_REQUEST_FLAG;
			}
			break;

		default:

			return(ADM_ERR_BADOPT);
	    }
	    option = va_arg(opt_args, int);
	}

	if ((*flagsp & ADM_LOCAL_REQUEST_FLAG) != 0) {
		*max_triesp = 1;
	}

	if (auth_type != ADM_AUTH_UNSPECIFIED) {
		stat = adm_auth_chktype(auth_type, num_flavorsp, flavors);
		if (stat != ADM_AUTH_OK) {
			return(stat);
		}
		if (*num_flavorsp == (u_int)0) {
			return(ADM_ERR_NOAUTHFLAV);
		}
	} else {
		flavors[0] = auth_flavor;
		*num_flavorsp = 1;
	}
	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_PING_AGENT( request_ts, IP_address, prog_num, vers_num,
 *		   retries, timeout ):
 *
 *	Ping an administrative agent to determine if a specific request
 *	is being processed.  "IP_address", "prog_num", and "vers_num"
 *	specify the host IP address, RPC program number, and RPC
 *	version number of the agent to ping, respectively.  The specified
 *	agent is pinged to determine if it is currently processing a
 *	request with the specified timestamp ("request_ts").  The
 *	routine will attempt to ping the agent up to the specified
 *	number of times ("retries"), or until it receives a response.
 *	Each retry will have the specified timeout before the next
 *	ping is attempted.
 *	
 *	If a negative response is received, or if all ping retries
 *	timeout, this routine will return B_FALSE.  If a positive
 *	response is received on any of the ping attempts, the routine
 *	will return B_TRUE.
 *
 *	NOTE: Exponential backoff should be added to this routine.
 *
 *--------------------------------------------------------------------
 */

static
boolean_t
adm_ping_agent(
	struct timeval request_ts,
	struct in_addr IP_address,
	u_long prog_num,
	u_long vers_num,
	u_int retries,
	struct timeval timeout
	)
{
	u_int attempt;		/* Number of ping requests attempted */
	Netmgt_error error;

	attempt = 1;
	while (attempt++ <= retries) {

		/*
		 * Ping the agent.  Return B_TRUE if receive a positive
		 * ack.  Return B_FALSE if receive an explicit negative
		 * ack.
		 */

		ADM_DBG("i", ("Invoke: pinging agent, attempt %d...",
			attempt - 1));

		if (_netmgt_check_activity(request_ts, IP_address, prog_num,
					   vers_num, timeout)) {
			ADM_DBG("i", ("Invoke: positive ack received"));
			return(B_TRUE);
		}

		if (netmgt_fetch_error(&error)) {
			if (error.service_error == NETMGT_UNKNOWNREQUEST) {
				ADM_DBG("ie", ("Error: negative ack received"));
				return(B_FALSE);
			}
		}
		ADM_DBG("ie", ("Error: ping ack timeout"));
	}

	return(B_FALSE);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_PROC_FENSE( errorp, out_handlep ):
 *
 *	Process the system arguments in a SunNet Manager data report.
 *	For the most part, system arguments are ignored.  This routine
 *	does process the following arguments, however:
 *
 *		Name			Used for:
 *		----			---------
 *
 *		ADM_VERSION		Checked for compatibility with AMCL
 *					protocol version.  This should always
 *					be the first argument in a report.
 *
 *		NETMGT_OPTSTRING	The value of this argument is
 *					placed in the admin data handle
 *					as the unformatted output from
 *					the method.
 *
 *		ADM_UNFERR		The value of this argument is
 *					placed in the error structure
 *					as the unformatted error information
 *					from the method.
 *
 *		ADM_AUTH_FLAVORS	If present, this argument indicates
 *					that the auentication flavor provided
 *					with the request was insufficient,
 *					and that the flavor names included
 *					in the value list of this argument
 *					would be acceptable.  This information
 *					is recorded so that the request
 *					can be retried with one of the
 *					acceptable authentication flavors.
 *
 *	If successful, this routine will returns B_TRUE and will
 *	have read all of the report arguments up to (and including)
 *	the special marker argument (ADM_FENCE).  If an error occurs,
 *	this routine will fill in the formatted error fields of the
 *	error structure with information about the error, and will
 *	return B_FALSE.
 *
 *--------------------------------------------------------------------
 */

static
boolean_t
adm_proc_fence(
	Adm_error *errorp,
	Adm_data *out_handlep
	)
{
	Netmgt_data data;
	int version;
	int stat;

	/*
	 * Check for correct protocol version.
	 */

	ADM_DBG("i", ("Invoke: processing system arguments from callback..."));

	stat = netmgt_fetch_data(&data);
	if (!stat) {
		adm_err_fmtf(errorp, ADM_ERR_BADATA, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADATA),
			      netmgt_error.service_error, pv(netmgt_sperror()));
		return(B_FALSE);
	}
	if (strcmp(data.name, ADM_VERSION_NAME) != 0) {
		adm_err_fmtf(errorp, ADM_ERR_NOVERSION, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_NOVERSION));
		return(B_FALSE);
	}
	if (data.value != NULL) {
		memcpy((char *) &version, data.value, sizeof(int));
	}
	if ((data.value == NULL) || (version != ADM_VERSION)) {
		adm_err_fmtf(errorp, ADM_ERR_BADVERSION, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADVERSION),
			      (data.value == NULL ? -1 : version));
		return(B_FALSE);
	}

	/*
	 * Process other system arguments.
	 */

	while (stat = netmgt_fetch_data(&data)) {

							/* End of data report */
	    if (strcmp(data.name, NETMGT_ENDOFARGS) == 0) {

		break;
							/* Unformatted results */
	    } else if (strcmp(data.name, ADM_UNFMT_NAME) == 0) {

		stat = adm_args_putu(out_handlep, data.value,
			(data.length == 0 ? 0 : data.length - 1));
		if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_BADUNF, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADUNF), stat,
				adm_err_msg(stat));
			return(B_FALSE);
		}
							/* Unfmt error info */
	    } else if (strcmp(data.name, ADM_UNFERR_NAME) == 0) {

		stat = adm_err_unfmt(errorp,
			(data.length == 0 ? 0 : data.length - 1), data.value);
		if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_BADUNFERR, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADUNFERR), stat,
				adm_err_msg(stat));
			return(B_FALSE);
		}
							/* Auth retry list*/
	    } else if (strcmp(data.name, ADM_AUTH_FLAVORS_NAME) == 0) {

		if (flavor_names != NULL) {
			free(flavor_names);
		}
		flavor_names = strdup(data.value);
		if (flavor_names == NULL) {
			adm_err_fmtf(errorp, ADM_ERR_NOMEM, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_NOMEM));
			return(B_FALSE);
		}
		is_request_try = B_TRUE;
							/* End-of-fence */
	    } else if (strcmp(data.name, ADM_FENCE_NAME) == 0) {

		return(B_TRUE);

	    } /* Ignore all other system arguments in this version. */

	}

	if (stat) {
		adm_err_fmtf(errorp, ADM_ERR_NOFENCE, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_NOFENCE));
	} else {
		adm_err_fmtf(errorp, ADM_ERR_BADATA, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADATA),
			      netmgt_error.service_error, pv(netmgt_sperror()));
	}

	return(B_FALSE);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_PROC_METHARGS( errorp, out_handlep ):
 *
 *	Process the method result arguments from a SunNet Manager
 *	data report.  The output arguments are placed in the formatted
 *	table of the specified administrative data structure.
 *
 *	Upon normal completion, this routine returns B_TRUE.  If an error
 *	occurs processing the report, an appropriate error is placed in
 *	the specified error structure and B_FALSE is returned.
 *
 *--------------------------------------------------------------------
 */

static
boolean_t
adm_proc_methargs(
	Adm_error *errorp,
	Adm_data *out_handlep
	)
{
	boolean_t new_row;	/* Need new row in output argument table */
	Netmgt_data data;
	int stat;

	ADM_DBG("i", ("Invoke: processing method output arguments from callback..."));

	new_row = B_TRUE;
	while (stat = netmgt_fetch_data(&data)) {

							/* End of data report */
	    if (strcmp(data.name, NETMGT_ENDOFARGS) == 0) {

		break;
							/* End of table row */
	    } else if (strcmp(data.name, NETMGT_ENDOFROW) == 0) {

		if (new_row) {
		    stat = adm_args_insr(out_handlep);
		    if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_BADINSR, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADINSR), stat,
				adm_err_msg(stat));
			return(B_FALSE);
		    }
		}
		new_row = B_TRUE;
	    } else {
							/* Formatted argument */
		if (new_row) {
		    stat = adm_args_insr(out_handlep);
		    if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_BADINSR, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADINSR), stat,
				adm_err_msg(stat));
			return(B_FALSE);
		    }
		}
		stat = adm_args_puta(out_handlep, data.name, ADM_STRING,
				  (u_int)(data.length == 0 ? 0 : data.length-1),
				  (data.length == 0 ? NULL : data.value));
		if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_BADARG, ADM_ERR_SYSTEM,
				ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADARG), stat,
				adm_err_msg(stat));
			return(B_FALSE);
		}
		new_row = B_FALSE;

	    }
	}

	if (!stat) {
		adm_err_fmtf(errorp, ADM_ERR_BADATA, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADATA),
			      netmgt_error.service_error, pv(netmgt_sperror()));
	}

	return(stat);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_REC_DATA( errorp, out_handlep ):
 *
 *	Process a SunNet Manager data report from the administrative
 *	class agent.  This routine processes both the system arguments
 *	and the method arguments in the report.  Result arguments from
 *	the method are placed in the formatted table of the specified
 *	administrative data structure.
 *
 *	Upon normal completion, this routine returns B_TRUE.  If an error
 *	occurs processing the report, an appropriate error is placed in
 *	the specified error structure and B_FALSE is returned.
 *
 *--------------------------------------------------------------------
 */

static
boolean_t
adm_rec_data(
	Adm_error *errorp,
	Adm_data *out_handlep
	)
{
	ADM_DBG("i", ("Invoke: received data report from agent"));

	if (!adm_proc_fence(errorp, out_handlep)) {
		return(B_FALSE);
	}

	return(adm_proc_methargs(errorp, out_handlep));
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_REC_ERROR( exit_statp, errorp ):
 *
 *	Process an error report.  Warning and fatal errors are assumed
 *	to indicate errors from the framework or method.  The format of
 *	these errors are:
 *
 *	    Netmgt Error Fields	    Value:
 *	    -------------------	    ------
 *
 *	       service_error	    NETMGT_WARNING / NETMGT_FATAL
 *
 *	       agent_error	    Framework or method error code.
 *
 *	       message		    String indicating the exit status,
 *				    error type (ADM_ERR_SYSTEM / ADM_ERRCLASS),
 *				    cleanliness status (ADM_FAILCLEAN /
 *				    ADM_FAILDIRTY), and optional error
 *				    message.  Formats for string:
 *
 *					[exit,type,cleanup]\n
 *					[exit,type,cleanup] message\n
 *
 *	All other errors are assumed to be SunNet Manager services errors.
 *	The exit status is returned in *exit_statp while the other
 *	values from the error report are filled into the specified
 *	error structure.
 *
 *	Upon normal completion, this routine returns B_TRUE.  If a fatal
 *	error is detected (one that should signal the termination of the
 *	invocation), the routine returns B_FALSE.
 *
 *--------------------------------------------------------------------
 */

static
boolean_t
adm_rec_error(
	int *exit_statp,
	Adm_error *errorp
	)
{
	u_int type;		/* Error type */
	u_int cleanup;		/* Error cleanliness indication */
	char *msgp;		/* Administrative error message */
	int stat;

	ADM_DBG("ie", ("Invoke: received error report from agent"));

	switch(netmgt_error.service_error) {

	    case NETMGT_WARNING:			
	    case NETMGT_FATAL:			/* Framework or method error */

		stat = adm_err_str2snm(netmgt_error.message, exit_statp, &type,
					&cleanup, &msgp);
		if (stat != ADM_SUCCESS) {
			adm_err_fmtf(errorp, ADM_ERR_ERRPARSE, ADM_ERR_SYSTEM,
				     ADM_FAILDIRTY, adm_err_msg(ADM_ERR_ERRPARSE),
				     stat, adm_err_msg(stat));
			return(B_FALSE);
		}
		adm_err_fmt2(errorp, netmgt_error.agent_error, type, cleanup,
				msgp);
		break;

	    default:				/* SNM error */

		adm_err_fmtf(errorp, ADM_ERR_UNKERR, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_UNKERR),
			      netmgt_error.service_error);
		return(B_FALSE);
	}

	return((netmgt_error.service_error == NETMGT_WARNING ? B_TRUE
							     : B_FALSE));
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_RENDEZ( type, system, group, key, count, interval, flags ):
 *
 *	This is the callback routine for processing reports from the
 *	administrative class agent.  The appropriate report type
 *	(data or error) is processed and, if the report is the last
 *	from the agent request, adm_svc_run() is signalled that this
 *	is the end of the reports.
 *
 *	The arguments to this routine (as required by the SunNet Manager
 *	manager services library) are largely ignored.
 *
 *--------------------------------------------------------------------
 */

static
void
adm_rendez(
	u_int type,
	char *system,
	char *group,
	char *key,
	u_int count,
	struct timeval interval,
	u_int flags
	)
{
	boolean_t error_free;		/* No errors in processing report? */
	Netmgt_msginfo msginfo;		/* SNM report information */

	/*
	 * Verify that the report is from the current method request.
	 */

	if (!netmgt_fetch_msginfo(&msginfo)) {
		return;
	}
	if (snm_tsp == NULL) {
		return;
	}
	if (timercmp(snm_tsp, &msginfo.request_time, !=)) {
		ADM_DBG("ie", ("Error: received a stray report...discarding"));
		return;
	}

	/*
	 * Process report.
	 */

	switch (type) {

	    case NETMGT_ACTION_REPORT:			/* Method Data */

		error_free = adm_rec_data(errp, outputp);
		break;

	    case NETMGT_ERROR_REPORT:			/* AMSL Error */

		error_free = adm_rec_error(&estat, errp);
		break;

	    default:					/* Unknown */

		adm_err_fmtf(errp, ADM_ERR_BADREP, ADM_ERR_SYSTEM,
			      ADM_FAILDIRTY, adm_err_msg(ADM_ERR_BADREP), type);
		error_free = B_FALSE;
		break;
	}

	/*
	 * Determine if request is terminating, and signal if so.
	 */

	if ((!error_free) || ((flags & NETMGT_LAST) != 0)) {
		adm_last_report = B_TRUE;
	}
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_SET_AUTH( flav, errorp, flags, num_flavors, flavors[], flavor_names ):
 *
 *	Set the flavor of authentication to use when making a
 *	method request to the class agent.  If flavor_names is
 *	not a NULL pointer, it should point to a list of authentication
 *	flavor names that are acceptable for this request.  If
 *	possible, this routine will set the SNM authentication
 *	flavor to the first possible flavor (known to SNM) in this list.
 *
 *	If flavor_names is NULL, flavors[] should be an array of
 *	authentication flavor numbers to try (as specified in
 *	<rpc/auth.h>.  num_flavors should be set to the number of
 *	flavors listed in this array.  This routine will then set
 *	the SNM authentication flavor to the first entry in the
 *	array known to SNM.
 *
 *	The flags arguments should be the framework control flags
 *	being used in this method invocation.
 *
 *	If successful, this routine returns ADM_SUCCESS and set *flav
 *	to the authentication flavor selected.  If unsuccessful,
 *	the error structure pointed to by errorp will be filled in
 *	with informaiton concerning the error and an appropriate
 *	error code will be returned.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_set_auth(
	u_int *flav,
	Adm_error *errorp,
	u_long flags,
	u_int num_flavors,
	u_int flavors[],
	char *flavor_names
	)
{
	char *flav_name;		/* Flavor name */
	char *next_flavor;		/* Next flavor name */
	char *fdup;			/* Duplicate of flavors list, so that */
					/* adm_strtok() can write into it. */
	boolean_t is_auth_set;		/* Has auth. flavor been set? */
	int i;
	int stat;

	if ((flags & ADM_LOCAL_REQUEST_FLAG) != 0) {
		return(ADM_SUCCESS);
	}

	is_auth_set = B_FALSE;

	/*
	 * If a list of authentication flavor names is provided, set the
	 * authentication flavor from that list.
	 */

	if (flavor_names != NULL) {

		/* Duplicate the list so that adm_strtok can write into it. */

		fdup = strdup(flavor_names);
		if (fdup == NULL) {
			adm_err_fmtf(errorp, ADM_ERR_NOMEM, ADM_ERR_SYSTEM,
				     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_NOMEM));
			return(ADM_ERR_NOMEM);
		}

		/* Loop through the flavor names to find an acceptable one. */

		next_flavor = fdup;
		while ((flav_name = adm_strtok(&next_flavor, " ")) != NULL) {
			stat = adm_auth_str2flavor(flav_name, flav);
                        if (stat == ADM_AUTH_OK) {
				if (_netmgt_set_auth_flavor((int)*flav)) {
					ADM_DBG("is", ("Auth: set auth flavor to %d",
						(int)*flav));

					is_auth_set = B_TRUE;
					break;
				}
			}
		}
		free(fdup);

	} else {

	/*
 	 * No list of flavor names was specified.  Loop through the
 	 * list of authentication flavor numbers to find one that is
 	 * acceptable.
 	 */

		for (i = 0; i < num_flavors; i++) {
			*flav = flavors[i];
			if (_netmgt_set_auth_flavor((int)*flav)) {
				ADM_DBG("is", ("Auth: set auth flavor to %d",
						(int)*flav));
				is_auth_set = B_TRUE;
				break;
			}
		}
	}

	/*
	 * Has authentication been successfully set?
	 */

	if (is_auth_set) {
		return(ADM_SUCCESS);
	} else {
		ADM_DBG("iae", ("Auth: unable to find acceptable auth flavor"));
		adm_err_fmtf(errorp, ADM_ERR_SETAUTH, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_SETAUTH));
		return(ADM_ERR_SETAUTH);
	}
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_SET_SNM_ARG( errorp, sys_arg, arg_name, arg_type, arg_length,
 *		    arg_valuep ):
 *
 *	This routine creates a new SNM argument (in the current request)
 *	having the specified name (arg_name), type, length, and value.
 *	sys_arg should be set to B_TRUE if the specified argument is
 *	an administrative framework system argument, and B_FALSE otherwise.
 * 
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *	If an error occurs setting the SNM argument, the routine
 *	fills in the specified error structure (errorp) with details
 *	concerning the error and returns an appropriate error code.
 *
 *--------------------------------------------------------------------
 */

static
int
adm_set_snm_arg(
	Adm_error *errorp,
	boolean_t sys_arg,
	char *arg_name,
	u_int arg_type,
	u_int arg_length,
	caddr_t arg_valuep
	)
{
	int err_code;
	Netmgt_arg netarg;

	strcpy(netarg.name, arg_name);
	netarg.type = arg_type;
	netarg.length = arg_length;
	netarg.value = arg_valuep;

	ADM_DBG("a", ("Args: setting SNM argument %s", arg_name));

	if (!netmgt_set_argument(&netarg)) {
		err_code = (sys_arg ? ADM_ERR_SETSYSARG : ADM_ERR_SETMETHARG);
		adm_err_fmtf(errorp, err_code, ADM_ERR_SYSTEM, ADM_FAILCLEAN,
			     adm_err_msg(err_code), netmgt_error.service_error,
			     pv(netmgt_sperror()));
		return(err_code);
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_SVC_RUN( timeoutp ):
 *
 *	This routine is used in place of svc_run(3N) to wait for the
 *	results from an administrative method (agent) request.  It
 *	provides the following needed functionality not present in the
 *	standard svc_run(3N):
 *
 *	    1. Termination
 *
 *		After the last report for the present SunNet Manager data
 *		request is processed, this routine will return to the
 *		caller.
 *
 *	    2. Timeout
 *
 *		The specified timeout value is used to judge a crash
 *		of the server of a request.  If the timeout pointer is
 *		NULL, the routine will block indefinitely waiting
 *		for the final server callback.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *	A timeout is signalled by the status ADM_ERR_TIMEOUT.
 *
 *	NOTE: The adm_perf_method() callback routine is expected to set
 *	      the variable adm_last_report to B_TRUE after it has processed
 *	      the last report from an agent.
 *
 *	NOTE: This routine is not thread-safe.  Multiple concurrent invocations
 *	      of this routine yield undefined results.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
 *	  All Rights Reserved  	
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *	The copyright notice above does not evidence any   
 *	actual or intended publication of such source code.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *	PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *	Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *          All rights reserved.
 *
 *--------------------------------------------------------------------
 */

struct pollfd	svc_pollset[FD_SETSIZE];

static
int
adm_svc_run(struct timeval *timeoutp)
{
	int nfds;
	int dtbsize = __rpc_dtbsize();
	int ms_timeout;			/* Timeout in milliseconds */
	int i;

	extern int   __rpc_select_to_poll(int, fd_set *, struct pollfd *);
	extern void    svc_getreq_poll(struct pollfd *, int);

	/*
	 * Compute the result timeout value in milliseconds.
	 */

	if (timeoutp == NULL) {
		ms_timeout = -1;
	} else {
		ms_timeout = (timeoutp->tv_sec * 1000)
			   + (timeoutp->tv_usec / 1000)
			   + 1;
	}

	/*
	 * Receive the request results.
	 */

	adm_last_report = B_FALSE;
	while (!adm_last_report) {
		/*
		 * Check whether there is any server fd on which we may
		 * to wait.
		 */
		nfds = __rpc_select_to_poll(dtbsize, &svc_fdset, svc_pollset);
		if (nfds == 0) {
			return(ADM_ERR_LOSTREG);	/* None waiting */
		}

		switch (i = poll(svc_pollset, nfds,
				(timeoutp == NULL ? -1 : ms_timeout))) {
		case -1:
			/*
			 * We ignore all errors, continuing with the assumption
			 * that it was set by the signal handlers (or any
			 * other outside event) and not caused by poll().
			 */
			continue;
		case 0:
			return(ADM_ERR_TIMEOUT);
		default:
			svc_getreq_poll(svc_pollset, i);
		}
	}

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_PERF_METHOD( method, in_handlep, out_handlepp, errorpp,
 *		    opt1, opt2, ..., ADM_ENDOPTS ):
 *
 *	Perform an administrative class method.  The caller specifies
 *	the name of the method to invoke, an administrative data handle
 *	containing the input to the method (in_handlep), the address of
 *	a pointer in which to place the results of the method invocation
 *	(out_handlepp), and the address of a pointer to hold an error
 *	information from the method (errorpp).
 *
 *	The user may also specify a set of framework control options (see
 *	adm_parse_opts() above for a complete list of the valid options).
 *	The list of options should include at least the name of the
 *	method's class (option ADM_CLASS).
 *
 *	Upon completion, the routine returns the results of the method and
 *	any error information at *out_handlepp and *errorpp, respectively.
 *	The exit status of the method is also normally returned as
 *	the value of the function  (ADM_SUCCESS normally indicates the
 *	successful completion of the requested method).  If a framework
 *	error occurs during the invocation, ADM_FAILURE is returned
 *	as the value of the routine and additional information about
 *	the error is returned in *errorpp.
 *
 *--------------------------------------------------------------------
 */
int
adm_perf_method(
	char *method,
	Adm_data *in_handlep,
	Adm_data **out_handlepp,
	Adm_error **errorpp,
	...
	)
{
	va_list req_args;		/* Varargs arguments to method request */
	char *adm_class;			/* Class name of requested method */
	char *class_vers;		/* Class version number */
	char *host;			/* Host on which to execute method */
	char *domain;			/* Domain in which to exec method */
	char *client_group;		/* Client's preferred group */
	char *categories;		/* Additional trace message categories */
	struct timeval ack_timeout;	/* Timeout for initial request */
	struct timeval rep_timeout;	/* Timeout for reports from agent */
	struct timeval ping_timeout;	/* Timeout to wait for ping ack */
	struct timeval ping_delay;	/* Delay before beginning pinging */
	u_int ping_cnt;			/* Number of time to retry ping */
	u_long agent_prog;		/* RPC program number of server agent */
	u_long agent_vers;		/* RPC version number of server agent */
	u_long rendez_prog;		/* Temporary RPC number for results */
	int udp_sock;			/* Rendezvous UDP socket descriptor */
	int tcp_sock;			/* Rendezvous TCP socket descriptor */
	char hostname[MAXHOSTNAMELEN+1];/* Host on which to execute method */
	Adm_requestID reqID;		/* Request ID */
	u_int flav;			/* Auth flavor selected for request */
	int ret_stat;			/* Return status from method request */
	u_long flags;			/* Framework control flags */
	int max_tries;			/* Max. times to try request */
	int stat;
	int i;

	/*
	 * Macros used to free structures allocated by this routine.
	 * These macros free the error structure and output argument
	 * structure if they are not going to be returned to the caller.
	 * The last one also frees the authentication flavor name list
	 * used for retrying under-authenticated requests.
	 */

#define ADM_AMCL_FREE_ERR	if (errorpp == NULL) {			\
					adm_err_freeh(errp);		\
				}
#define ADM_AMCL_FREE_OUT	if (out_handlepp == NULL) {		\
					adm_args_freeh(outputp);	\
				}
#define ADM_AMCL_FREE		ADM_AMCL_FREE_ERR; ADM_AMCL_FREE_OUT;	\
				if (flavor_names != NULL) {		\
					free(flavor_names);		\
				}

	ADM_TMG_START(adm_ru_call);
	ADM_TMG_START(adm_ru_init);

	flavor_names = NULL;		/* Must init first thing so that */
					/* ADM_AMCL_FREE doesn't mess up */
	va_start(req_args, errorpp);

	/*
	 * Create error structure and output handle.
	 */

	errp = adm_err_newh();
	if (errorpp != NULL) {
		*errorpp = errp;
	}
	if (errp == NULL) {
		va_end(req_args);
		return(ADM_FAILURE);
	}

	outputp = adm_args_newh();
	if (out_handlepp != NULL) {
		*out_handlepp = outputp;
	}
	if (outputp == NULL) {
		adm_err_fmtf(errp, ADM_ERR_NOHANDLE, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_NOHANDLE));
		va_end(req_args);
		ADM_AMCL_FREE_ERR;
		return(ADM_FAILURE);
	}

	/*
	 * Initialize the framework, if not already initialized.
	 */

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		adm_err_fmtf(errp, ADM_ERR_INITFAIL, ADM_ERR_SYSTEM, 
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_INITFAIL), stat,
			     adm_err_msg(stat));
		va_end(req_args);
		return(ADM_FAILURE);
	}

	/*
	 * Select framework controls.
	 */

	auth_flav_num = ADM_AUTH_MAXFLAVORS;
	stat = adm_parse_opts(&ack_timeout, &rep_timeout, &agent_prog,
			      &agent_vers, &adm_class, &class_vers, &host, &domain,
			      &auth_flav_num, auth_flavors, &categories, &ping_cnt,
			      &ping_timeout, &ping_delay, &client_group,
			      &flags, &max_tries, req_args);
	va_end(req_args);
	if (stat != ADM_SUCCESS) {
		adm_err_fmtf(errp, ADM_ERR_MALOPT, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_MALOPT), stat,
			     adm_err_msg(stat));
		ADM_AMCL_FREE;
		return(ADM_FAILURE);
	}

	/*
	 * Verify request parameters.
	 */

	if (adm_class == NULL) {
		adm_err_fmtf(errp, ADM_ERR_MALCLASS, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_MALCLASS));
		ADM_AMCL_FREE;
		return(ADM_FAILURE);
	}
	if (method == NULL) {
		adm_err_fmtf(errp, ADM_ERR_MALMETHOD, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_MALMETHOD));
		ADM_AMCL_FREE;
		return(ADM_FAILURE);
	}

	if (host != NULL) {
		if (strlen(host) > (size_t) MAXHOSTNAMELEN) {
		    adm_err_fmtf(errp, ADM_ERR_MALHOST, ADM_ERR_SYSTEM,
			     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_MALHOST));
		    ADM_AMCL_FREE;
		    return(ADM_FAILURE);
		}
		strcpy(hostname, host);
	} else {
		strcpy(hostname, adm_host);
	}
	
	/*
	 * Initialize the method request for the agent.
	 */

	stat = adm_init_request(errp, flags, &rendez_prog, &udp_sock, &tcp_sock,
				&reqID);
	if (stat != ADM_SUCCESS) {
		ADM_AMCL_FREE;
		return(ADM_FAILURE);
	}
	snm_tsp = NULL;

	ADM_TMG_END(adm_ru_init);

	/*
	 * Execute request and collect results.  If initial request
	 * fails due to insufficient authentication, retry the request
	 * with an acceptable level of authentication.
	 */

	tries = 0;
	is_request_try = B_TRUE;
	while (is_request_try && (tries < max_tries)) {

		/* Reset request results */

		ADM_DBG("i", ("Invoke: Beginning request attempt %d...", tries));

		estat = ADM_SUCCESS;
		adm_err_reset(errp);

		tries++;
		is_request_try = B_FALSE;

		/*
		 * Set request instance to reflect agent host, class,
		 * and method being invoked.
		 */

		if ((flags & ADM_LOCAL_REQUEST_FLAG) == 0) {
		    if (!netmgt_set_instance(hostname, adm_class, method)) {

			ADM_DBG("ie", ("Error: unable to set up SNM request instance"));

			adm_err_fmtf(errp, ADM_ERR_NINST, ADM_ERR_SYSTEM,
				     ADM_FAILCLEAN, adm_err_msg(ADM_ERR_NINST),
				     netmgt_error.service_error,
				     pv(netmgt_sperror()));
			ADM_AMCL_FREE;
			return(ADM_FAILURE);
		    }

		    ADM_DBG("i", ("Invoke: set up SNM request instance"));

		    snm_tsp = _netmgt_get_request_time();
		}

		/*
		 * Build request arguments (system and method).
		 */

		ADM_TMG_START(adm_ru_build[tries]);
		stat = adm_build_args(errp, flags, reqID, adm_class, class_vers,
				method, hostname, domain, client_group,
				categories, rep_timeout, ping_timeout,
				ping_cnt, ping_delay, in_handlep);
		ADM_TMG_END(adm_ru_build[tries]);
		if (stat != ADM_SUCCESS) {
			ADM_AMCL_FREE;
			return(ADM_FAILURE);
		}

		/* Set authentication flavor */

		stat = adm_set_auth(&flav, errp, flags, auth_flav_num, auth_flavors,
					flavor_names);
		if (stat != ADM_SUCCESS) {
			ADM_AMCL_FREE;
			return(ADM_FAILURE);
		}

		/* Try request */

		stat = adm_make_request(errp, flags, snm_tsp, reqID, adm_class,
					class_vers, method, client_group,
					categories, hostname, agent_prog,
					agent_vers, rendez_prog, ack_timeout,
					rep_timeout, ping_timeout, ping_cnt,
					ping_delay, in_handlep);
		if (stat == ADM_ERR_CREDFAIL) {
			if (tries == 1) {
				max_tries += 1;
			}
		} else if (stat != ADM_SUCCESS) {
			adm_args_reset(outputp);
			ADM_AMCL_FREE;
			return(ADM_FAILURE);
		}
	}

	/*
	 * Complete the method invocation.
	 */

	if ((errp->code != ADM_SUCCESS) && (errp->type == ADM_ERR_SYSTEM)) {
		ret_stat = ADM_FAILURE;
	} else {
		ret_stat = estat;
		adm_auth_init_type = ADM_AUTH_UNSPECIFIED;
		adm_auth_init_flavor = flav;
	}
	adm_args_reset(outputp);
	ADM_AMCL_FREE;

	ADM_TMG_END(adm_ru_call);
	ADM_DBG("p", ("perf: init: e=%ld u=%ld s=%ld",
		ADM_TMG_ELAPSED(adm_ru_init), ADM_TMG_USER(adm_ru_init),
		ADM_TMG_SYSTEM(adm_ru_init)));
#ifdef ADM_DEBUG
	for (i = 1; i <= tries; i++) {
	    ADM_DBG("p",
	      ("perf: build %d: e=%ld u=%ld s=%ld", i,
	       ADM_TMG_ELAPSED(adm_ru_build[i]), ADM_TMG_USER(adm_ru_build[i]),
	       ADM_TMG_SYSTEM(adm_ru_build[i])));
	}
#endif ADM_DEBUG
	ADM_DBG("p", ("perf: call: e=%ld u=%ld s=%ld",
		ADM_TMG_ELAPSED(adm_ru_call), ADM_TMG_USER(adm_ru_call),
		ADM_TMG_SYSTEM(adm_ru_call)));
	return(ret_stat);
}

#endif /* !_adm_amcl_c */

