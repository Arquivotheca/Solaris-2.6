/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef __DPTIOCTL_H_
#define	__DPTIOCTL_H_

#pragma ident	"@(#)dptioctl.h	1.4	95/03/22 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/* DPT ioctl defines and structures					*/

#define	DPT_MAX_CONTROLLERS	18

/* High order bit in dpt_found to signify controllers in ioctl		*/
#define	DPT_CTRL_HELD		0x8000000

typedef struct {
	ushort dc_number;
	ushort dc_addr[DPT_MAX_CONTROLLERS];
} dpt_get_ctlrs_t;

/* ReadConfig data structure - this structure contains the EATA Configuration */
typedef struct {
	unchar ConfigLength[4];	/* Len in bytes after this field.	*/
	unchar EATAsignature[4];
	unchar EATAversion;

	unchar OverLapCmds:1;	/* TRUE if overlapped cmds supported	*/
	unchar TargetMode:1;	/* TRUE if target mode supported	*/
	unchar TrunNotNec:1;
	unchar MoreSupported:1;
	unchar DMAsupported:1;	/* TRUE if DMA Mode Supported		*/
	unchar DMAChannelValid:1; /* TRUE if DMA Channel is Valid	*/
	unchar ATAdevice:1;
	unchar HBAvalid:1;	/* TRUE if HBA field is valid		*/

	unchar PadLength[2];
	unchar HBA[4];
	unchar CPlength[4];	/* Command Packet Length		*/
	unchar SPlength[4];	/* Status Packet Length			*/
	unchar QueueSize[2];	/* Controller Que depth			*/
	unchar SG_Size[4];

	unchar IRQ_Number:4;	/* IRQ Ctlr is on ... ie 14,15,12	*/
	unchar IRQ_Trigger:1;	/* 0 =Edge Trigger, 1 =Level Trigger	*/
	unchar Secondary:1;	/* TRUE if ctlr not parked on 0x1F0	*/
	unchar DMA_Channel:2;	/* DMA Channel used if PM2011		*/

	unchar	Reserved0;	/*	Reserved Field			*/

	unchar	Disable:1;	/* Secondary I/O Address Disabled	*/
	unchar	ForceAddr:1;	/* PCI Forced To An EISA/ISA Address    */
	unchar	Reserved1:6;	/* Reserved Field			*/

	unchar	MaxScsiID:5;	/* Maximun SCSI Target ID Supported	*/
	unchar	MaxChannel:3;	/* Maximum Channel Number Supported	*/

	unchar	MaxLUN;		/* Maximun LUN Supported		*/

	unchar	Reserved2:6;	/* Reserved Field			*/
	unchar	PCIbus:1;	/* PCI Adapter Flag			*/
	unchar	EISAbus:1;	/* EISA Adapter				*/

	unchar	RaidNum;	/* RAID HBA Number For Stripping	*/
	unchar	Reserved3;	/* Reserved Field			*/
}	dpt_ReadConfig_t;

typedef	struct {
	u_int	rcf_base;
	dpt_ReadConfig_t	rcf_config;
}	dpt_readconfig_t;

/*	dpt ioctls		*/
#define	DPT_EATA_USR_CMD	0x01
#define	DPT_GET_SIG		0x02
#define	DPT_GET_CTLRS		0x03
#define	DPT_READ_CONFIG		0x04

#define	DPT_CORE_CCB_SIZ	24

#ifdef	__cplusplus
}
#endif

#endif /* __DPTIOCTL_H_ */
