
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _adm_sec_h
#define _adm_sec_h

#pragma	ident	"@(#)adm_sec.h	1.4	93/05/18 SMI"

#include "adm_auth.h"

/*
 * FILE:  adm_sec.h
 *
 *      Admin Framework general purpose security information
 *      exported to methods.
 */


/* Admin security common principal name for external API's */
typedef	struct	Adm_cpn {
	u_int   type;		/* Type of common principal name */
	uid_t   id;		/* User or group identifier */
	char   *name;		/* User or group name */
	char   *role;		/* Role or host name */
	char   *domain;		/* Domain name */
} Adm_cpn;

/* Admin security common principal name type definitions */
#define	ADM_CPN_NONE	0	/* Cpn has no type (empty) */
#define	ADM_CPN_USER	1	/* Cpn is for a user */
#define	ADM_CPN_GROUP	2	/* Cpn is for a group */
#define	ADM_CPN_OTHER	3	/* Cpn is for an others entry */

/* Security information function prototype definitions */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	adm_auth_client(u_int *, u_int *, uid_t *, Adm_cpn **);
extern	int	adm_cpn_cpn2str(Adm_cpn *, char *, u_int);
extern	int	adm_cpn_free(Adm_cpn *);

#ifdef __cplusplus
}
#endif

#endif /* !_adm_sec_h */

