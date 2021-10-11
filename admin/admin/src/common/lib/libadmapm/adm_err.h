
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the exported definitions for handling
 *	errors in the administrative framework.  It includes the
 *	definitions for:
 *
 *		o Types (locations) of errors.
 *		o Miscellaneous constants.
 *		o Error handling structures.
 *		o Exported interfaces for handling errors.
 *
 *	    NOTE: Routines similar to adm_err_set() and adm_err_setf()
 *		  should be added that take an i18n key and map it to
 *		  the appropriate error text.
 *
 ****************************************************************************
 */

#ifndef _adm_err_h
#define _adm_err_h

#pragma	ident	"@(#)adm_err.h	1.14	95/06/02 SMI"

/*
 *--------------------------------------------------------------------
 * Types (locations) of administrative errors
 *--------------------------------------------------------------------
 */

#define ADM_ERR_SYSTEM	(u_int) 1	/* (Formatted) error from framework */
#define ADM_ERR_CLASS	(u_int) 2	/* (Formatted) error from class method */

/*
 *--------------------------------------------------------------------
 * Miscellaneous constants
 *--------------------------------------------------------------------
 */

#define ADM_MAXERRMSG	4096	/* Max. length of a method error message.
				   must match NETMGT_NAMESIZ in libadmcom*/
#define ADM_NULLSTRING	"??"

/*
 *--------------------------------------------------------------------
 * Administrative error structures.
 *--------------------------------------------------------------------
 */

typedef struct Adm_error Adm_error;	/* Administrative error */
struct Adm_error {
    int     code;	    /* Error code (ADM_SUCCESS if successful) */
    u_int   type;	    /* Error type (ADM_ERR_SYSTEM or ADM_ERR_CLASS) */
    u_int   cleanup;	    /* Cleanliness (ADM_FAILCLEAN or ADM_FAILDIRTY) */
    char   *message;	    /* Optional error message */
    u_int   unfmt_len;	    /* Length of unformatted error block */
    caddr_t unfmt_txt;	    /* Unformatted error block */
};

/*
 *--------------------------------------------------------------------
 * Export interfaces for managing administrative errors.
 *--------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_err_cleanup(char *, u_int);
extern	int	adm_err_freeh(Adm_error *);
extern	int	adm_err_set(char *, int, char *);
extern	int	adm_err_setf(char *, int, char*, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_err_h */

