/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_parms.c	1.18	92/10/28 SMI"

/*
 * FILE:  parms.c
 *
 *	Admin Framework routines for handling parameters from the SNM
 *	request RPC and to the SNM callback RPC.
 */

#include <string.h>
#include <malloc.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare static functions */
static int get_char_system_parm(struct amsl_req *, Netmgt_arg *, char **);
static int get_int_system_parm(struct amsl_req *, Netmgt_arg *, int *);
static int get_reqid_system_parm(struct amsl_req *, Netmgt_arg *,
	    Adm_requestID *);
static int get_vers_1_parms(struct amsl_req *);
static int put_system_header(struct amsl_req *);
static int put_system_trailer(struct amsl_req *);
static int put_unfmt_parms(struct amsl_req *, struct bufctl *, char *);
static int put_vers_1_parms(struct amsl_req *);

/*
 * -------------------------------------------------------------------
 *  get_system_parms - Retrieve the request RPC system parameters.
 *	Accepts a pointer to the AMSL request structure.
 *	This routine sequentially retrieves the system parameters from
 *	the request RPC via calls to the SNM Agent Services library.
 *	Each system parameter value is stored in the associated field
 *	in the request structure.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_system_parms"

int
get_system_parms(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Netmgt_arg nmarg;	/* Temporary SNM argument structure */
	int  stat;		/* Status return code and loop switch */

	/*
	 * The Admin framework system parameter request protocol is...
	 *	ADM_VERSION		Request framework version number
	 *	ADM_REQUESTID		Request identification number
	 *	ADM_CLASS		Request class name
	 *	ADM_CLASS_VERS		Request class version number
	 *	ADM_METHOD		Request method name
	 *	ADM_HOST		Target host name
	 *	ADM_DOMAIN		Target domain name
	 *	ADM_CLIENT_HOST		(Optional) Client host name
	 *	ADM_CLIENT_DOMAIN	(Optional) Client domain name
	 *	ADM_CLIENT_GROUP	(Optional) Client group name
	 *	ADM_CATEGORIES		(Optional) Client diag categories
	 *	ADM_TIMEOUT_PARMS	(Optional) Client max timeout for
	 *					   the method
	 *	ADM_LANG		(Optional) Client language prefer-
	 *					   ence for messages
	 *	ADM_UNFMT		(Optional) unformatted input
	 *	ADM_FENCE		End of system parameters
	 *
	 * The class agent depends on ADM_VERSION being the first
	 * system parameter, and ADM_FENCE being the last system
	 * parameter before the formatted input parameters.
	 */

	stat = 0;

	/*
	 * Position to the first system argument, ADM_VERSION
	 * Must exist and contain a valid integer version number,
	 * since it may be required in subsequent processing of
	 * the Admin framework protocol.
	 *
	 * NOTE: We do not generate an error if we cannot handle the
	 *	version number.  Instead, we simply store the version
	 *	number in the request structure, ignore processing
	 *	any system parameters, and return without an error.
	 *	Its up to the verification routine to decide what to do.
	 */

	if (netmgt_fetch_argument(ADM_VERSION_NAME, &nmarg))
		stat = get_int_system_parm(reqp, &nmarg,
		    &reqp->request_version);
	else {
		(void) amsl_err_netmgt(&stat, (char **)NULL);
		stat = amsl_err(reqp, ADM_ERR_BADSNMFETCH, stat,
		    ADM_VERSION_NAME);
	}
	if ((stat == 0) && (reqp->request_version <= 0))
		stat = amsl_err(reqp, ADM_ERR_BADVERSION,
		    reqp->request_version);

	/*
	 * Handle the request protocol according to its version number.
	 * In general, there could be different code for each version,
	 * so we have a switch statement based on version number.  The
	 * actual request handling code is functionalized so that the
	 * same function can be used be different versions.
	 * Remember, the point of this exercise is to map the request
	 * system parameters into fields in the current agent version of
	 * the amsl request structure.
	 */

	if (stat == 0)
		switch (reqp->request_version) {

			case 1:				/* Version 1 */
				stat = get_vers_1_parms(reqp);
				break;

			default:

				break;
		}					/* End of switch */

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  get_vers_1_parms - Retrieve version 1 request RPC system parameters.
 *	Accepts a pointer to the AMSL request structure and the request
 *	protocol version number.
 *	This routine sequentially retrieves the system parameters from
 *	the request RPC via calls to the SNM Agent Services library.
 *	Each system parameter value is stored in the associated field
 *	in the request structure.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_vers_1_parms"

static int
get_vers_1_parms(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Netmgt_arg nmarg;	/* Temporary SNM argument structure */
	int  stat;		/* Status return code and loop switch */

	/*
	 * The version 1 framework system parameter request protocol is...
	 *	ADM_VERSION		Request framework version number
	 *	ADM_REQUESTID		Request identification number
	 *	ADM_CLASS		Request class name
	 *	ADM_CLASS_VERS		Request class version number
	 *	ADM_METHOD		Request method name
	 *	ADM_METHOD_VERS		Request method version number
	 *	ADM_HOST		Target host name
	 *	ADM_DOMAIN		Target domain name
	 *	ADM_CLIENT_HOST		(Optional) Client host name
	 *	ADM_CLIENT_DOMAIN	(Optional) Client domain name
	 *	ADM_CLIENT_GROUP	(Optional) Client group name
	 *	ADM_CATEGORIES		(Optional) Client diag categories
	 *	ADM_TIMEOUT_PARMS	(Optional) Client max timeout for
	 *					   the method
	 *	ADM_LANG		(Optional) Client language prefer-
	 *					   ence for messages
	 *	ADM_UNFMT		(Optional) unformatted input
	 *	ADM_FENCE		End of system parameters
	 *
	 * The class agent depends on ADM_VERSION being the first
	 * system parameter, and ADM_FENCE being the last system
	 * parameter before the formatted input parameters.
	 */

	/* First, position to start of system parameters */
	stat = 0;
	if (! (netmgt_fetch_argument(ADM_VERSION_NAME, &nmarg)))
		stat = amsl_err(reqp, ADM_ERR_BADSNMPROTO);

	/*
	 * Retrieve system parameters sequentially.  For each parameter,
	 * validate its type, length, and (optionally) value; then store
	 * its value into the request structure.  Note that missing
	 * system parameters result in null values in the request
	 * structure.  If a null value is passed for a string format
	 * system parameter, we simply leave the value null in the
	 * request structure.  The verification routine can then decide
	 * if this is a fatal error.
	 *
	 * Note that errors in type and length cause a fatal error
	 * return from this routine.
	 */

	while (stat == 0) {

		/* Get next sequential argument */
		if (! (netmgt_fetch_argument((char *)NULL, &nmarg))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADSNMFETCH, stat,
			    "");
			break;
		}

		/* Check for end of sequential argument scan */
		if (! (strcmp(NETMGT_ENDOFARGS, nmarg.name))) {
			stat = amsl_err(reqp, ADM_ERR_BADSNMPROTO);
			break;
		}
		if (! (strcmp(ADM_FENCE_NAME, nmarg.name)))
			break;

		/* Check for request identifier parameter */
		if (! (strcmp(ADM_REQUESTID_NAME, nmarg.name))) {
			stat = get_reqid_system_parm(reqp, &nmarg,
			    reqp->reqIDp);
		}

		/* Check for class name parameter */
		else if (! (strcmp(ADM_CLASS_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->class_name);
		}

		/* Check for class version parameter */
		else if (! (strcmp(ADM_CLASS_VERS_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->class_version);
		}

		/* Check for method name parameter */
		else if (! (strcmp(ADM_METHOD_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->method_name);
		}

		/* Check for host name parameter */
		else if (! (strcmp(ADM_HOST_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->agent_host);
		}

		/* Check for domain name parameter */
		else if (! (strcmp(ADM_DOMAIN_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->agent_domain);
		}

		/* Check for client host name parameter */
		else if (! (strcmp(ADM_CLIENT_HOST_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_host);
		}

		/* Check for client domain name parameter */
		else if (! (strcmp(ADM_CLIENT_DOMAIN_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_domain);
		}

		/* Check for client group name parameter */
		else if (! (strcmp(ADM_CLIENT_GROUP_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_group_name);
		}

		/* Check for client diagnostic categories parameter */
		else if (! (strcmp(ADM_CATEGORIES_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_diag_categories);
		}

		/* Check for client timeout parameter */
		else if (! (strcmp(ADM_TIMEOUT_PARMS_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_timeout_parm);
		}

		/* Check for client message language parameter */
		else if (! (strcmp(ADM_LANG_NAME, nmarg.name))) {
			stat = get_char_system_parm(reqp, &nmarg,
			    &reqp->client_lang_parm);
		}

		/* Check for unformatted input parameter */
		else if (! (strcmp(ADM_UNFMT_NAME, nmarg.name))) {
			stat = 0;		/* Ignore it here */
		}

		/* Unexpected system parameter; ignore it */
		else {
			ADM_DBG("ei", ("Invoke: Unexpected system parmeter: %s",
			    nmarg.name));
		}
	}					/* End of while loop */

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  get_int_system_parm - Process integer system parameter.
 *	Accepts a pointer to the AMSL request structure, a pointer to
 *	the Netmgt_arg argument structure, and a pointer to the field
 *	in the AMSL request structure to contain the integer value.
 *	This routine validates that the system parameter is an integer
 *	argument and stores the integer value into the request structure
 *	field.
 *	A non-zero status code is returned if an error occurs.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_int_system_parm"

int
get_int_system_parm(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	Netmgt_arg *nmargp,	/* Pointer to netmgt argument structure */
	int *req_int)		/* Pointer to request integer field */
{
	int   stat;		/* Return status code */

	/* If valid integer argument, store it into request struct field */
	stat = 0;
	if (nmargp->type == NETMGT_INT)
		if ((nmargp->length == sizeof (int)) && (nmargp->value
		    != (caddr_t)NULL))
			*req_int = *(int *)nmargp->value;
		else
			stat = amsl_err(reqp, ADM_ERR_BADSNMVALUE,
			    nmargp->name, nmargp->type);
	else
		stat = amsl_err(reqp, ADM_ERR_BADSNMTYPE, nmargp->type,
		    nmargp->name);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  get_char_system_parms - Process character string system parameter.
 *	Accepts a pointer to the AMSL request structure, a pointer to
 *	the Netmgt_arg argument structure, and the address of the field
 *	in the AMSL request structure to contain the string pointer.
 *	This routine validates that the system parameter is a string
 *	argument, duplicates the string in memory, and stores the
 *	address of the duplicated string in the specified filed in the
 *	request structure.
 *	A non-zero status code is returned if an error occurs.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_char_system_parm"

int
get_char_system_parm(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	Netmgt_arg *nmargp,	/* Pointer to netmgt argument structure */
	char **spp)		/* Address of pointer to string value */
{
	char *sp;		/* Temp string pointer */
	int   stat;		/* Return status code */

	/* If valid string argument, allocate memory and store pointer */
	stat = 0;
	if (nmargp->type == NETMGT_STRING)
		if ((nmargp->length != 0) && (nmargp->value != (caddr_t)NULL))
			if ((sp = strdup(nmargp->value)) != (char *)NULL)
				*spp = sp;
			else
				stat = amsl_err(reqp, ADM_ERR_NOMEMORY,
				    PROG_NAME, 1);
		else
			*spp = (char *)NULL;
	else
		stat = amsl_err(reqp, ADM_ERR_BADSNMTYPE, nmargp->type,
		    nmargp->name);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  get_reqid_system_parm - Process request identifier system parameter.
 *	Accepts a pointer to the AMSL request structure, a pointer to
 *	the Netmgt_arg argument structure, and a pointer to the request
 *	identifier structure in the AMSL request structure.
 *	This routine validates that the system parameter is a string
 *	argument, converts the string to a request identifier,  and
 *	stores the converted request identifier in the specified field
 *	in the request structure.
 *	A non-zero status code is returned if an error occurs.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_reqid_system_parm"

int
get_reqid_system_parm(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	Netmgt_arg *nmargp,	/* Pointer to netmgt argument structure */
	Adm_requestID *reqidp)	/* Pointer to request id structure */
{
	u_int len;		/* Temporary integer length */
	int stat;		/* Return status code */

	/* If valid string argument, allocate memory and store pointer */
	stat = 0;
	if (nmargp->type == NETMGT_STRING)
		if ((nmargp->length != 0) && (nmargp->value != (caddr_t)NULL))
			if ((adm_reqID_str2rid(nmargp->value, reqidp, &len))
			    == ADM_SUCCESS)
				stat = 0;
			else
				stat = amsl_err(reqp, ADM_ERR_BADSNMVALUE,
				    nmargp->name, nmargp->type);
		else
			(void) adm_reqID_blank(reqidp);
	else
		stat = amsl_err(reqp, ADM_ERR_BADSNMTYPE, nmargp->type,
		    nmargp->name);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  get_input_parms - Retrieve the request RPC input parameters.
 *	Accepts a pointer to the AMSL request structure and a pointer
 *	to the input parameter Admin IO handle.
 *	This routine sequentially retrieves the input parameters from
 *	the request RPC via calls to the SNM Agent Services library.
 *	An Admin_arg structure is built for each input parameter and
 *	it is linked onto the end of the argument linked list anchored
 *	by the Admin data handle.  Additionally, optional unformatted
 *	character data is handled (via the ADM_UNFMT argument).
 *	Returns an error status code and adds the error message to the
 *	formatted error structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"get_input_parms"

int
get_input_parms(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct Adm_data *inp)	/* Pointer to input argument data handle */
{
	Netmgt_arg nmarg;	/* Temporary SNM argument structure */
	char *bp;		/* Temporary buffer pointer */
	int  stat;		/* Local status code and loop switch */

	/*
	 * NOTE:  SNM string arguments always include the "\0" in the
	 *	  string's length.  If we get a string with length zero,
	 *	  it implies the value pointer should be NULL.  If we
	 *	  get a string with length one, it implies a valid
	 *	  string buffer, but containing only the \0 character.
	 *	  In this case, we set the length to zero, but still
	 *	  point to the string buffer.  For strings with length
	 *	  greater than zero, back off one byte on the length.
	 */

	/* Get the unformatted input argument; if it exists */
	stat = 0;
	if (netmgt_fetch_argument(ADM_UNFMT_NAME, &nmarg))
		if ((nmarg.length != 0) && (nmarg.value != (caddr_t)NULL))
			if ((bp = (char *)malloc((size_t)nmarg.length))
			    != NULL) {
				ADM_DBG("a", ("Args:   Method input: %d bytes of unformatted data",
				    nmarg.length));
				(void) memcpy(bp, (char *)nmarg.value,
				    (size_t)nmarg.length);
				inp->unformatted_len = nmarg.length - 1;
				inp->unformattedp = bp;
				}
			else
				return (amsl_err(reqp, ADM_ERR_NOMEMORY,
				    PROG_NAME, 2));

	/* Position to the start of the input arguments */
	if (stat == 0)
		if (! (netmgt_fetch_argument(ADM_FENCE_NAME, &nmarg)))
			stat = amsl_err(reqp, ADM_ERR_BADSNMPROTO);

	/* Loop calling SNM Agent Services for next input argument */
	while (stat == 0) {

		/* Get next sequential argument */
		if (! (netmgt_fetch_argument((char *)NULL, &nmarg))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADSNMFETCH, stat,
			    "");
			break;
		}

		/* Check for end of sequential argument scan */
		if (! (strcmp(NETMGT_ENDOFARGS, nmarg.name))) {
			break;
		}

		/* Check for valid string argument and add to arg list */
		if (nmarg.type == NETMGT_STRING) {
			ADM_DBG("a", ("Args:   Method input: %s(%d)=%s",
			    nmarg.name, nmarg.length,
			    (nmarg.value != NULL?nmarg.value:"(nil)")));
			if (nmarg.length != 0)
				nmarg.length -= 1; /* Remove \0 */
			else
				nmarg.value = (char *)NULL;
			if ((adm_args_puta(inp, nmarg.name, ADM_STRING,
			    nmarg.length, nmarg.value)) != 0)
				stat = amsl_err(reqp, ADM_ERR_BADINPUTPARM,
				    nmarg.name);
		}
		else
			stat = amsl_err(reqp, ADM_ERR_BADSNMTYPE,
			    nmarg.type, nmarg.name);
	}					/* End of while loop */

	/* Return success */
	return (0);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_output_parms - Add output parameters to callback RPC message.
 *	Accepts a pointer to the AMSL request structure and a pointer
 *	to the output argument handle.
 *	This routine sequentially adds the output parameters from
 *	the method program to the SNM callback RPC messsage via calls
 *	to the SNM Agent Services library.
 * 	This routine assumes the callback RPC system parameters have
 * 	already been built, with the last system parameter being the
 * 	ADM_FENCE parameter.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_output_parms"

int
put_output_parms(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct Adm_data *outp)	/* Pointer to output argument data handle */
{
	Adm_arg *admargp;	/* Pointer to Admin argument structure */
	Netmgt_data nmarg;	/* Temporary SNM report argument struct */
	int  brksw;		/* Loop break switch */
	int  sw;		/* Temporary switch variable */
	int  stat;		/* Status code */
	int  temp;		/* Temp integer */
	bool_t eventflag = FALSE;	/* Data vs event flag */

	/* Check for and process formatted output arguments */
	stat = 0;
	if ((adm_args_htype(outp) & ADM_FORMATTED) != ADM_FORMATTED)
		return (stat);
	stat = adm_args_reset(outp);

	/*
	 * Walk list of output arguments anchored by output IO handle.
	 * For each argument, check if end of row or an output argument.
	 * If end of row, call SNM Agent Services to mark end of row.
	 * If output arg, call SNM Agent Services to add report argument.
	 * Exit loop when end of args encountered.
	 */

	brksw = 0;
	while ((stat == 0) && (brksw == 0)) {
		sw = adm_args_nexta(outp, ADM_ANYTYPE, &admargp);
		switch (sw) {

		case ADM_SUCCESS:		/* Got next argument */
			ADM_DBG("a", ("Args:   Method output: %s(%d)=%s",
			    admargp->name, admargp->length,
			    (admargp->valuep != NULL?admargp->valuep:"(nil)")));
			(void) strcpy(nmarg.name, admargp->name);
			nmarg.type = admargp->type;
			nmarg.length = admargp->length;
			nmarg.value = admargp->valuep;
			if ((admargp->length == 0) && (admargp->valuep
			    == (caddr_t)NULL))
				nmarg.value = "";
			else
				nmarg.length += 1;	/* Adjust for \0 */
			if (! (netmgt_build_report(&nmarg, &eventflag))) {
				(void) amsl_err_netmgt(&stat, (char **)NULL);
				stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM,
				    stat, nmarg.name);
				brksw = 1;
			}
			break;

		case ADM_ERR_ENDOFROW:		/* Hit end of a row */
			ADM_DBG("a", ("Args:   Method output: (End of row)"));
			netmgt_mark_end_of_row();
			if ((temp = adm_args_nextr(outp)) == 0) {
				break;

			} else if (temp == ADM_ERR_ENDOFTABLE)
					brksw = 1;
				else {
					stat = amsl_err(reqp, ADM_ERR_BADNEXTR,
					    temp);
					brksw = 1;
				}
			break;

		case ADM_ERR_ENDOFTABLE:	/* Hit end of table */
			ADM_DBG("a", ("Args:   Method output: (End of table)"));
			brksw = 1;
			break;

		default:			/* Fatal error */
			(void) amsl_err(reqp, ADM_ERR_NEXTARG, sw);
			stat = sw;
			brksw = 1;
			break;
		}				/* End of switch */
	}				/* End of infinite loop */

	/* If empty table, return success */
	if (stat == ADM_ERR_NOFRMT)
		stat = 0;

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_action_header - Build the action callback RPC system parameters.
 *	Accepts a pointer to the AMSL request structure and pointers to
 *	the unformatted output and error buffer control structures.
 *	This routine sequentially writes the system parameters for
 *	the callback RPC via calls to the SNM Agent Services library.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_action_header"

int
put_action_header(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	int  stat;		/* Status return code and loop switch */

	/*
	 * The Admin framework system parameter protocol for
	 * an action request callback is...
	 *	ADM_VERSION	Protocol version number (from request
	 *	ADM_REQUESTID	Request identification number
	 * followed by per version parameters.
	 * For version 1, the additional parameters include...
	 *	ADM_UNFMT	(Optional) unformatted output
	 *	ADM_UNFERR	(Optional) unformatted error
	 * followed by the system parameter termination argument...
	 *	ADM_FENCE	End of system parameters
	 *
	 * The client AMCL depends on ADM_VERSION being the first
	 * system parameter, and ADM_FENCE being the last system
	 * parameter before the formatted output parameters.
	 */

	/* Build the request version number and request id parameters */
	stat = put_system_header(reqp);

	/*
	 * Handle the rest of the callback protocol per version number.
	 * In general, there could be different code for each version,
	 * so we have a switch statement based on version number.  The
	 * actual callback handling code is functionalized so that the
	 * same function can be used by different versions.
	 * Remember, the point of this exercise is to map the fields
	 * from the amsl request structure to the appropriate protocol
	 * for system parameters based upon the same protocol as the
	 * original request.
	 */

	if (stat == 0)
		switch (reqp->request_version) {

			case 1:				/* Version 1 */
				stat = put_vers_1_parms(reqp);
				break;

			default:			/* Default = bad */

				stat = amsl_err(reqp, ADM_ERR_BADVERSION,
				    reqp->request_version);
				break;
		}					/* End of switch */


	/* Build the end of system parameters parameter */
	if (stat == 0)
		stat = put_system_trailer(reqp);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_version_header - Build version range callback RPC system parameters.
 *	Accepts a pointer to the AMSL request structure.
 *	This routine sequentially builds the system parameters for
 *	the version range callback RPC via calls to the SNM Agent
 *	Services library.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_version_header"

int
put_version_header(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Netmgt_data nmarg;	/* Temporary SNM argument structure */
	bool_t eventflag = FALSE;	/* Data vs event flag */
	int  temp;		/* Temporary integer */
	int  stat;		/* Status return code and loop switch */

	/*
	 * The version range system parameter callback protocol
	 * includes...
	 *	ADM_VERSION	Original request version number
	 *	ADM_REQUESTID	Original request identifier number
	 *	ADM_LOW_VERSION	Class agent lowest protocol version
	 *			number handled
	 *	ADM_HIGH_VERSION Class agent highest protocol version
	 *			number handled
	 *	ADM_FENCE	End of protocol header
	 *
	 * The client AMCL depends on ADM_VERSION being the first
	 * system parameter, and ADM_FENCE being the last system
	 * parameter.
	 *
	 */

	/* Build the request version number and request id parameters */
	stat = put_system_header(reqp);

	/* Build the lowest protocol version number handled */
	if (stat == 0) {
		temp = ADM_LOW_VERSION;
		(void) strcpy(nmarg.name, ADM_LOW_VERSION_NAME);
		nmarg.type = NETMGT_INT;
		nmarg.length = sizeof (int);
		nmarg.value = (caddr_t)&temp;
		if (! (netmgt_build_report(&nmarg, &eventflag))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
			    nmarg.name);
		}
	}

	/* Build the highest protocol version number handled */
	if (stat == 0) {
		temp = ADM_HIGH_VERSION;
		(void) strcpy(nmarg.name, ADM_HIGH_VERSION_NAME);
		nmarg.type = NETMGT_INT;
		nmarg.length = sizeof (int);
		nmarg.value = (caddr_t)&temp;
		if (! (netmgt_build_report(&nmarg, &eventflag))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
			    nmarg.name);
		}
	}

	/* Build the end of system parameters parameter */
	if (stat == 0)
		stat = put_system_trailer(reqp);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_weakauth_header - Build weak auth callback RPC system parameters.
 *	Accepts a pointer to the AMSL request structure and a buffer
 *	of authentication flavor names.
 *	This routine sequentially builds the system parameters for the
 *	weak auth callback RPC via calls to the SNM Agent Services library.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_weakauth_header"

int
put_weakauth_header(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	char *flavors)		/* Pointer to auth flavor names string */
{
	Netmgt_data nmarg;	/* Temporary SNM argument structure */
	bool_t eventflag = FALSE;	/* Data vs event flag */
	int  stat;		/* Status return code and loop switch */

	/*
	 * The weak auth system parameter callback protocol includes:
	 *
	 *	ADM_VERSION	 Original request version number
	 *	ADM_REQUESTID	 Original request identifier number
	 *	ADM_AUTH_FLAVORS Names of valid auth flavors for retry
	 *	ADM_FENCE	 End of protocol header
	 *
	 * The client AMCL depends on ADM_VERSION being the first
	 * system parameter, and ADM_FENCE being the last system
	 * parameter.
	 *
	 */

	/* Build the request version number and request id parameters */
	stat = put_system_header(reqp);

	/* Build the authentication flavor names parameter */
	if (stat == 0) {
		(void) strcpy(nmarg.name, ADM_AUTH_FLAVORS_NAME);
		nmarg.type = NETMGT_STRING;
		nmarg.length = strlen(flavors) + 1;
		nmarg.value = (caddr_t)flavors;
		if (! (netmgt_build_report(&nmarg, &eventflag))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
			    nmarg.name);
		}
	}

	/* Build the end of system parameters parameter */
	if (stat == 0)
		stat = put_system_trailer(reqp);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_system_header - Build common callback RPC system parameters.
 *	Accepts a pointer to the AMSL request structure.
 *	This routine sequentially builds the system parameters for
 *	the start of all callback RPC's via calls to the SNM Agent
 *	Services library.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_system_header"

static int
put_system_header(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Netmgt_data nmarg;	/* Temporary SNM argument structure */
	char ridbuf[ADM_MAXRIDLEN+1];	/* Temp requestID buffer */
	bool_t eventflag = FALSE;	/* Data vs event flag */
	u_int  len;		/* Temporary length */
	int  stat;		/* Status return code and loop switch */

	stat = 0;

	/* Send the original request version number as the first argument */
	(void) strcpy(nmarg.name, ADM_VERSION_NAME);
	nmarg.type = NETMGT_INT;
	nmarg.length = sizeof (int);
	nmarg.value = (caddr_t)&reqp->request_version;
	if (! (netmgt_build_report(&nmarg, &eventflag))) {
		(void) amsl_err_netmgt(&stat, (char **)NULL);
		stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
		    nmarg.name);
	}

	/* Send the request identifier as the second argument */
	if (stat == 0) {
		if ((adm_reqID_rid2str(*(reqp->reqIDp), ridbuf,
		    ADM_MAXRIDLEN + 1, &len)) != 0) {
			ridbuf[0] = '\0';
			len = 0;
		}
		(void) strcpy(nmarg.name, ADM_REQUESTID_NAME);
		nmarg.type = NETMGT_STRING;
		nmarg.length = len +1;
		nmarg.value = (caddr_t)ridbuf;
		if (! (netmgt_build_report(&nmarg, &eventflag))) {
			(void) amsl_err_netmgt(&stat, (char **)NULL);
			stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
			    nmarg.name);
		}
	}

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_system_trailer - Build common callback RPC system parameter
 *	header termination parameter.
 *	Accepts a pointer to the AMSL request structure.
 *	This routine sequentially builds the system parameter marking
 *	the end of all callback RPC system parameters via a call to the
 *	SNM Agent Services library.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_system_trailer"

static int
put_system_trailer(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	Netmgt_data nmarg;	/* Temporary SNM argument structure */
	bool_t eventflag = FALSE; /* Data vs event flag */
	int  stat;		/* Status return code and loop switch */

	/* Send the special fence argument as the last system argument */
	stat = 0;
	(void) strcpy(nmarg.name, ADM_FENCE_NAME);
	nmarg.type = NETMGT_STRING;
	nmarg.length = 1;
	nmarg.value = "";
	if (! (netmgt_build_report(&nmarg, &eventflag))) {
		(void) amsl_err_netmgt(&stat, (char **)NULL);
		stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
		    nmarg.name);
	}

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * -------------------------------------------------------------------
 *  put_vers_1_parms - Build version 1 callback RPC system parameters.
 *	Accepts a pointer to the AMSL request structure and the request
 *	protocol version number.
 *	This routine sequentially builds the system parameters for
 *	the callback RPC via calls to the SNM Agent Services library.
 *	Each system parameter value is retrieved from the associated
 *	field in the request structure.
 *	Returns an error status code and adds the error message to the
 *	formatted error structure in the request structure.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"put_vers_1_parms"

static int
put_vers_1_parms(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	int  stat;		/* Status return code and loop switch */

	/*
	 * The version 1 framework system parameter callback protocol
	 * includes...
	 *	ADM_UNFMT	(Optional) unformatted output
	 *	ADM_UNFERR	(Optional) unformatted errors
	 *
	 * This version depends on ADM_VERSION being the first system
	 * parameter built, ADM_REQUESTID being the second system
	 * parameter built, and ADM_FENCE being the last system
	 * parameter built (after return from this function).
	 */

	/* Build the unformatted output well-known argument */
	stat = put_unfmt_parms(reqp, reqp->outbuff, ADM_UNFMT_NAME);

	/* Build the unformatted error well-known argument */
	if (stat == 0)
		stat = put_unfmt_parms(reqp, reqp->errbuff, ADM_UNFERR_NAME);

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * ---------------------------------------------------------------------
 * put_unfmt_parms - Process informatted output arguments.
 *	Accepts a pointer to the amsl request structure, a pointer to
 *	the linked list of unformatted output buffers, and the well-known
 *	name of the output argument.  Builds an argument containing all the
 *	unformatted output concatenated into a single string value.
 *	Returns an error status code if errors and places the error
 *	message in the formatted error structure.
 * ---------------------------------------------------------------------
 */

#define	PROG_NAME	"put_unfmt_parms"

static int
put_unfmt_parms(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct bufctl *buffp,	/* Pointer to unformatted output buffers */
	char  *argname)		/* Name of well-known Admin argument */
{
	Netmgt_data nmarg;	/* Temporary netmgt argument structure */
	int  stat;		/* Error return status */
	bool_t eventflag = FALSE;	/* Data vs event flag */

	/* Set up well-known netmgt argument */
	(void) strcpy(nmarg.name, argname);
	nmarg.type = NETMGT_STRING;
	nmarg.length = 0;
	nmarg.value = (caddr_t)NULL;

	/* See if any unformatted output.  If not, do NOT return argument */
	if (buffp == (struct bufctl *)NULL)
		return (0);
	if ((buffp->startp == (char *)NULL) || (buffp->size == buffp->left))
		return (0);

	/* Set to return the unformatted output buffer */
	nmarg.length = buffp->size - buffp->left;
	nmarg.value = (caddr_t)buffp->startp;
	ADM_DBG("a", ("Args:   Method output: %d bytes of unformatted data",
	    nmarg.length));
	if (buffp->left > 0) {
		*(buffp->currp) = '\0';
		nmarg.length += 1;
	}

	/* Add the argument to the SNM callback RPC message */
	stat = 0;
	if (! (netmgt_build_report(&nmarg, &eventflag))) {
		(void) amsl_err_netmgt(&stat, (char **)NULL);
		stat = amsl_err(reqp, ADM_ERR_BADOUTPUTPARM, stat,
		    nmarg.name);
	}

	/* Return status */
	return (stat);
}

#undef PROG_NAME

/*
 * ----------------------------------------------------------------------
 * put_callback - Routine to make the callback message RPC to the client.
 *	Accepts pointer to an amsl_req structure and the child process's
 *	exit status code.
 *	Returns result parameters to the client via an RPC callback message.
 * ----------------------------------------------------------------------
 */

int
put_callback(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	int    xstat)		/* Non-fatal child exit status */
{
	int    nmcode;		/* SNM system error code */
	char  *nmmsgp;		/* SNM system error message */
	struct timeval delta_time; /* Time structure */
	u_int  status;		/* Netmgt return status code */
	u_int  flags;		/* Local flags for callback messages */
	int    stat;		/* Local status code */

	ADM_DBG("i", ("Invoke: Making callback RPC..."));

	/* Check if a non-fatal error has occurred.  Send error if so */
	stat = 0;
	if (xstat == 0)
		status = NETMGT_SUCCESS;
	else
		status = NETMGT_WARNING;

	/* Action request callback supported as data report callback */
	delta_time.tv_sec = 0;
	delta_time.tv_usec = 0;
	flags = NETMGT_LAST;
	if (! (netmgt_send_report(delta_time, status, flags))) {
		(void) amsl_err_netmgt(&nmcode, &nmmsgp);
		stat = amsl_err(reqp, ADM_ERR_BADSNMSEND, nmcode, nmmsgp);
		ADM_DBG("ei", ("Invoke: Error %d sending report: %s",
		    nmcode, nmmsgp));
	}

	/* Return status */
	return (stat);
}

#undef PROG_NAME
