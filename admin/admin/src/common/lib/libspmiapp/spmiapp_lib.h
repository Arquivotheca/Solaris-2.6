#ifndef lint
#pragma ident "@(#)spmiapp_lib.h 1.1 95/10/20 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmiapp_lib.h
 * Group:	libspmiapp
 * Description:
 */

#ifndef _SPMIAPP_LIB_H
#define	_SPMIAPP_LIB_H

#include "spmiapp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* app_profile.c */
int		configure_dfltmnts(Profile *);
int		configure_sdisk(Profile *);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMIAPP_LIB_H */
