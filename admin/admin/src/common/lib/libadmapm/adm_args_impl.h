
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the private administrative argument definitions
 *	from the administrative framework.  The file contains definitions
 *	for:
 *
 *		o Administrative data handle containing method input.
 *		o Administrative data handle manipulation routines not
 *		  exported to the world.
 *
 *******************************************************************************
 */

#ifndef _adm_args_impl_h
#define _adm_args_impl_h

#pragma	ident	"@(#)adm_args_impl.h	1.9	93/05/18 SMI"

/*
 *----------------------------------------------------------------------
 * Private administrative variables.
 *----------------------------------------------------------------------
 */

extern Adm_data *adm_input_args;	/* Pointer to handle containing */
					/* formatted input to method */

/*
 *----------------------------------------------------------------------
 * Private administrative argument handling interfaces
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_args_a2hdr(char *, u_int, u_int, caddr_t, char *,
			       u_int, u_int *);
extern	int	adm_args_a2str(char *, u_int, u_int, caddr_t, char *, u_int, u_int *);
extern	u_int	adm_args_hdrsize(char *, u_int, u_int, caddr_t);
extern	int	adm_args_delr(Adm_data *, Adm_rowlink *, Adm_rowlink *);
extern	int	adm_args_eor(Adm_data *);
extern	int	adm_args_finda(Adm_data *, char *, Adm_arglink **, Adm_arglink **);
extern	int	adm_args_freea(Adm_arglink *);
extern	int	adm_args_freer(Adm_rowlink *);
extern	u_int	adm_args_hdrsize(char *, u_int, u_int, caddr_t);
extern	int	adm_args_init();
extern	int	adm_args_insa(Adm_data *, char *, u_int, u_int, caddr_t);
extern	int	adm_args_insr(Adm_data *);
extern	int	adm_args_remv(Adm_data *, Adm_arglink *, Adm_arglink *);
extern	int	adm_args_str2a(char **, char*, u_int *, u_int *, char **);
extern	int	adm_args_str2h(char *, Adm_data **);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_args_impl_h */

