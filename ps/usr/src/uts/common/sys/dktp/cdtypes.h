/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_CDTYPES_H
#define	_SYS_DKTP_CDTYPES_H

#pragma ident	"@(#)cdtypes.h	1.2	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

struct cd_data {
	opaque_t	cd_tgpt_objp;
};

#define	TGPTOBJP(X) ((X)->cd_tgpt_objp)

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_CDTYPES_H */
