/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#ifndef _SYS_DKTP_NCRS_INTERRUPT_H
#define	_SYS_DKTP_NCRS_INTERRUPT_H

#pragma	ident	"@(#)interrupt.h	1.3	94/06/30 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/* Interrupt vectors numbers that script may generate */
#define	NINT_OK		0xff00		/* device accepted the command */
#define	NINT_ILI_PHASE	0xff01		/* Illegal Phase */
#define	NINT_UNS_MSG	0xff02		/* Unsupported message */
#define	NINT_UNS_EXTMSG 0xff03		/* Unsupported extended message */
#define	NINT_MSGIN	0xff04		/* Message in expected */
#define	NINT_MSGREJ	0xff05		/* Message reject */
#define	NINT_RESEL	0xff06		/* C710 chip reselcted */
#define	NINT_SELECTED	0xff07		/* C710 chip selected */
#define	NINT_DISC	0xff09		/* Diconnect message received */
#define	NINT_RESEL_ERR	0xff0a		/* Reselect id error */
#define	NINT_SDP_MSG	0xff0b		/* Save Data Pointer message */
#define	NINT_RP_MSG	0xff0c		/* Restore Pointer message */
#define	NINT_SIGPROC	0xff0e		/* Signal Process */
#define	NINT_TOOMUCHDATA 0xff0f		/* Too much data to/from target */
#define	NINT_SDTR	0xff10		/* SDTR message received */
#define	NINT_SDTR_REJECT 0xff11		/* invalid SDTR exchange */
#define	NINT_REJECT	0xff12		/* failed to send msg reject */
#define	NINT_DEV_RESET	0xff13		/* bus device reset completed */


#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_NCRS_INTERRUPT_H */
