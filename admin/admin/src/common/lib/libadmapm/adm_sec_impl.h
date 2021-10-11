
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _adm_sec_impl_h
#define _adm_sec_impl_h

#include "adm_sec.h"

/*
 * FILE:  adm_sec_impl.h
 *
 *      Admin Framework general purpose security information
 *      used by the AMCL and class agent.
 */

#pragma	ident	"@(#)adm_sec_impl.h	1.5	93/05/18 SMI"

/* Security information function prototype definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_auth_chkflavor(u_int, u_int *);
extern	int	adm_auth_chktype(u_int, u_int *, u_int[]);
extern	int	adm_auth_client2(char *, u_int *, u_int *, uid_t *, Adm_cpn **);
extern	int	adm_auth_type2str(char *, u_int, u_int);
extern	int	adm_auth_str2type(char *, u_int *);
extern	int	adm_auth_flavor2str(char *, u_int, u_int);
extern	int	adm_auth_str2flavor(char *, u_int *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_sec_impl_h */

