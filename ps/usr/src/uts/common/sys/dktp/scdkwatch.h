/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_SYS_DKTP_SCDKWATCH_H
#define	_SYS_DKTP_SCDKWATCH_H

#pragma ident	"@(#)scdkwatch.h	1.4	96/09/20 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

struct scdk_watch_result {
	struct scsi_status		*statusp;
	struct scsi_extended_sense	*sensep;
	u_char				actual_sense_length;
	struct scsi_pkt			*pkt;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_SCDKWATCH_H */
