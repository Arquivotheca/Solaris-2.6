/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma ident   "@(#)local_args.c 1.6     95/05/08 SMI"

/*
 * This file contains local RMTC enhancements
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <admin/adm_err_msgs.h>
#include <admin/adm_args.h>
#include <admin/adm_err.h>
#include <admin/adm_fw.h>

static Adm_arg	*argp;
static int	 ret;

/*
 * This routine gets the specified input parameter 'par' and assigns 'value'
 * to it.  If 'required', then an appropriate error is returned.
 */

int
_get_input_arg(char *par, char **value, char *method_id, char *default_value,
	int required)
{
	ret = adm_args_geti(par, ADM_STRING, &argp);
	return (__get_arg(par, value, method_id, default_value, required,
		ADM_ERR_REQ_INPUT_PAR));
}

/*
 * This routine gets the specified return parameter 'par' and assigns 'value'
 * to it.  If 'required', then an appropriate error is returned.
 */

int
_get_returned_arg(Adm_data *outargs, char *par, char **value,
	char *method_id, char *default_value, int required)
	{
	ret = adm_args_geta(outargs, par, ADM_STRING, &argp);
	return (__get_arg(par, value, method_id, default_value, required,
		ADM_ERR_REQ_RET_PAR));
	}

int
__get_arg(char *par, char **value, char *method_id, char *default_value,
	int required, int err)
{
	if (ret == ADM_SUCCESS) {
		/*
		 * Although the arg may be defined, it may be empty (NULL).
		 */
		if (argp->valuep && *(argp->valuep)) {
			*value = argp->valuep;
			return (PARAM_FOUND);
		} else {
			if (default_value == NULL) *value = "";
			else *value = default_value;
			return (PARAM_FOUND_NOT_DEF);
		}
	} else {
		/*
		 * No arg defined.
		 */
		if (required == REQUIRED_ARG) {
			adm_err_setf(ADM_NOCATS,
				err, ADM_ERR_MSGS(err),
				method_id, par);
			exit(ADM_FAILURE);
		}
		/*
		 * else assume caller set 'value' to an appropriate value.
		*/
		return (PARAM_NOT_FOUND);
	}
}

static FILE *nowhere = NULL;

int
sprintf_alloc(char **buf, char *message, ...) {
    va_list	vp;		/* Varargs argument list pointer.	*/
    int		len;		/* Number of chars printed.		*/

    va_start(vp, message);
    len = vsprintf_alloc(buf, message, vp);
    va_end(vp);
    return(len);
}

int
vsprintf_alloc(char **buf, char *message, va_list vp) {
    int		len;		/* Number of chars printed.		*/

    /*
     * For performance reasons, we only want to open /dev/null once.
     * Assume the fopen succeeds.
     */
    if (nowhere == NULL) {
	nowhere = fopen("/dev/null", "w");
    }
    len = vfprintf(nowhere, message, vp);
    *buf = (char *)malloc(len + 1);
    if (*buf == NULL) return(0);
    vsprintf(*buf, message, vp);
    return(len);
}
