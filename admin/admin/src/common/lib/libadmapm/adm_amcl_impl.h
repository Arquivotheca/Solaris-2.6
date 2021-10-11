
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the private administrative framework definitions
 *	for the adm_perf_method() routine.
 *
 *******************************************************************************
 */

#ifndef _adm_amcl_impl_h
#define _adm_amcl_impl_h

#pragma	ident	"@(#)adm_amcl_impl.h	1.14	95/05/05 SMI"

#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 * Miscellaneous constants.
 *----------------------------------------------------------------------
 */

#define RENDEZ_VERS	(u_long)10	/* RPC version number for receiving results */

/*
 *----------------------------------------------------------------------
 * Global variables.
 *----------------------------------------------------------------------
 */

extern boolean_t is_rpc_allocated;
extern int	 t_udp_sock;
extern int	 t_tcp_sock;
extern u_long	 t_rendez_prog;

/*
 *----------------------------------------------------------------------
 * Exported interfaces.
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" int adm_amcl_cleanup(int, int, u_long, u_long);
#else
extern int adm_amcl_cleanup(int, int, u_long, u_long);
#endif

#endif /* !_adm_amcl_impl_h */

