
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 ****************************************************************************
 *
 *	This file contains the private definitions for administrative
 *	framework errors.  It includes definitions for:
 *
 *		o Miscellaneous constants.
 *		o Internal error handling routines.
 *
 *      NOTE: Routines similar to adm_err_fmt() and adm_err_fmtf() 
 *            should be added that take localization file keys instead
 *            of text messages.
 *
 ****************************************************************************
 */

#ifndef _adm_err_impl_h
#define _adm_err_impl_h

#pragma	ident	"@(#)adm_err_impl.h	1.8	93/05/18 SMI"

#include <stdio.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_err.h"

/*
 *--------------------------------------------------------------------
 * Miscellaneous constants.
 *--------------------------------------------------------------------
 */

#define ADM_MAXERRHDR	((2*ADM_MAXINTLEN) + 2)	/* Max. length of STDFMT */
						/* error message header */
#define ADM_ERR_SNMHDR2	"[%d,%u,%u]%n"		/* Format of SNM error msg */
						/* header containing admin */
						/* error type and cleanup */
#define ADM_ERR_SNMFMT	"[%d,%u,%u]%s%s\n"	/* Format of SNM error msg */
						/* encapsulating admin error */
						/* type, cleanup, and msg */

/*
 *--------------------------------------------------------------------
 * Routines for internal use within administrative framework.
 *--------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	   adm_err_fmt(Adm_error *, int, u_int, u_int, char *);
extern	int	   adm_err_fmtf(Adm_error *, int, u_int, u_int, char *, ...);
extern	int	   adm_err_fmt2(Adm_error *, int, u_int, u_int, char *);
extern	int	   adm_err_hdr2str(int, u_int, char *, u_int *);
extern	boolean_t  adm_err_isentry(Adm_error *);
extern	char   	  *adm_err_msg(int);
extern	Adm_error *adm_err_newh();
extern	int	   adm_err_reset(Adm_error *);
extern	int	   adm_err_set2(FILE *, char *, int, u_int, char *);
extern	int	   adm_err_snm2str(int, u_int, u_int, char *, char **, u_int *);
extern	int	   adm_err_str2cln(char *, u_int, u_int *);
extern	int	   adm_err_str2err(char *, u_int, int *, u_int *, char **);
extern	int	   adm_err_str2snm(char *, int *, u_int *, u_int *, char **);
extern	int	   adm_err_unfmt(Adm_error *, u_int, caddr_t);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_err_impl_h */

