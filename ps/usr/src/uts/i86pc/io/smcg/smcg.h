/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_SMCG_H
#define	_SMCG_H

#pragma	ident	"@(#)smcg.h	1.1	95/07/18 SMI"

/*
 * Driver declarations for the SMC Generic UMAC	driver
 */

#ifdef	__cplusplus
extern "C" {
#endif


/* debug flags */
#define	SMCGTRACE	0x01
#define	SMCGERRS	0x02
#define	SMCGRECV	0x04
#define	SMCGDDI		0x08
#define	SMCGSEND	0x10
#define	SMCGINT		0x20
#define	SMCGALAN	0x40

/* Misc	*/
#define	SMCGHIWAT	32768		/* driver flow control high water */
#define	SMCGLOWAT	4096		/* driver flow control low water */
#define	SMCGMAXPKT	1500		/* maximum media frame size */

/* Definitions for the field bus_type */
#define	SMCG_AT_BUS	0x00
#define	SMCG_MCA_BUS	0x01
#define	SMCG_EISA_BUS	0x02

/* Function declarations */
int LM_Nextcard(Adapter_Struc *);
int LM_Get_Addr(Adapter_Struc *);
int LM_GetCnfg(Adapter_Struc *);
int LM_Initialize_Adapter(Adapter_Struc *);
int LM_Open_Adapter(Adapter_Struc *);
int LM_Close_Adapter(Adapter_Struc *);
int LM_Add_Multi_Address(Adapter_Struc *);
int LM_Delete_Multi_Address(Adapter_Struc *);
int LM_Change_Receive_Mask(Adapter_Struc *);
int LM_Send(Data_Buff_Structure *, Adapter_Struc *, int);
int LM_Service_Events(Adapter_Struc *);
int LM_Disable_Adapter(Adapter_Struc *);
int LM_Enable_Adapter(Adapter_Struc *);
int LM_Receive_Copy(int, int, Data_Buff_Structure *, Adapter_Struc *, int);
int UM_Receive_Packet(char *, unsigned short, Adapter_Struc *, int);
int UM_Status_Change(Adapter_Struc *);
int UM_Receive_Copy_Complete(Adapter_Struc *);
int UM_Send_Complete(int, Adapter_Struc *);
int UM_Interrupt(Adapter_Struc *);

/* SMC Generic UMAC structure */
typedef
struct smcg_info {
	gld_mac_info_t	*smcg_macinfo;
	Adapter_Struc	*smcg_pAd;
	struct smcg_info *smcg_first;		/* first port of card */
	int		smcg_multicount;	/* current multicast count */
	int		smcg_ready;		/* driver is ready for ints */
	int		smcg_numchannels;	/* number of ports on card */
	kmutex_t	smcg_dual_port_lock;	/* used if multiple ports */
} smcg_t;


#ifdef	__cplusplus
}
#endif

#endif	/* _SMCG_H */
