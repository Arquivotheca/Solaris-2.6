#ifndef lint
#pragma ident "@(#)v_upgrade.h 1.21 96/07/02 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_upgrade.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_UPGRADE_H
#define	_V_UPGRADE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int v_is_upgrade(void);

/* interface for dealing with diskless clients to be upgraded */
extern int v_client_to_be_upgraded(int);
extern char *v_get_client_rootpath(int);
extern char *v_get_client_release(int);
extern int v_get_n_upgradeable_clients(void);

#ifdef __cplusplus
}

#endif

#endif				/* _V_UPGRADE_H */
