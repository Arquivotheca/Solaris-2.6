/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#ifndef _ADM_LOCAL_H
#define	_ADM_LOCAL_H

#pragma ident   "@(#)adm_local.h 1.3     95/05/08 SMI"

#ifdef  __cplusplus
extern "C" {
#endif
#include "adm_args.h"
#include <stdarg.h>

#define	OPTIONAL_ARG	0
#define	REQUIRED_ARG	1
#define	NODEFAULT	""

#define	PARAM_FOUND		0	/* Parameter found and defined.	    */
#define	PARAM_FOUND_NOT_DEF	1	/* Parameter found and not defined. */
#define	PARAM_NOT_FOUND		2	/* Parameter not found.		    */

/*
 * Assumptions:
 *	Assumed global variables/#defines:
 *		method_id
 *
 * Returns:
 *	PARAM_FOUND
 *	PARAM_FOUND_NOT_DEF
 *	PARAM_NOT_FOUND
 */

/*
 * Macros for dealing with args passed into a method.
 */
#define	get_opt_arg(param, value) \
		_get_input_arg(param, value, method_id, NODEFAULT, \
			OPTIONAL_ARG)

#define	get_req_arg(param, value, default) \
		_get_input_arg(param, value, method_id, default, \
			REQUIRED_ARG)

/*
 * Macros for dealing with paramaters returned from a method.
 */
#define	get_opt_ret(outargs, param, value) \
		_get_returned_arg(outargs, param, value, method_id, \
			NODEFAULT, OPTIONAL_ARG)

#define	get_req_ret(outargs, param, value, default) \
		_get_returned_arg(outargs, param, value, method_id, \
			default, REQUIRED_ARG)

/*
 * Macros for defining args to be passed to a method.
 */
#define	put_arg(arglist, param, value) \
		adm_args_puta(arglist, param, ADM_STRING, \
		strlen(value), value)

		/* Only put arg if it is defined (contains something) */
#define put_arg_ck(arglist, param, value) \
		if (strlen(value)) put_arg(arglist, param, value)

/*
 * Defines used by the above macros
 */
extern int	_get_input_arg(char *param, char **variable, char *method_id,
			char *default_value, int required);

extern int	_get_returned_arg(Adm_data *outargs, char *param,
			char **variable, char *method_id,
			char *default_value, int required);

extern int	sprintf_alloc(char **, char *, ...);
extern int	vsprintf_alloc(char **, char *, va_list);

#ifdef  __cplusplus
}
#endif

#endif /* _ADM_LOCAL_H */
