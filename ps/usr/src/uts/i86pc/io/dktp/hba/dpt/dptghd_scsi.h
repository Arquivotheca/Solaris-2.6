
/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _DPTGHD_SCSI_H
#define	_DPTGHD_SCSI_H

#pragma	ident	"@(#)dptghd_scsi.h	1.1	96/06/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>

void	scsi_htos_3byte(unchar *ap, ulong nav);
void	scsi_htos_long(unchar *ap, ulong niv);
void	scsi_htos_short(unchar *ap, ushort nsv);
ulong	scsi_stoh_3byte(unchar *ap);
ulong	scsi_stoh_long(ulong ai);
ushort	scsi_stoh_short(ushort as);


#ifdef	__cplusplus
}
#endif

#endif  /* _DPTGHD_SCSI_H */
