/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)xhs.h	1.2	93/11/02 SMI"

/***************************************************************************************************************; */
/* Exports to the Media Control Module										; */
/*--------------------------------------------------------------------------------------------------------------; */
int	Hardware_Initialize(),
	Hardware_Setup_ISR(),
	Hardware_Unhook();

void	Hardware_Enable_Int(),
	Hardware_Disable_Int();


/***************************************************************************************************************; */
/* Data Exported to Media Control Module									; */
/*--------------------------------------------------------------------------------------------------------------; */
/*NEEDSWORK	Hardware_Configuration, is defined elsewhere */
#ifdef notdef
int 	Hardware_Status,
	Hardware_Memory_Address,
	Hardware_IRQ_Number;
#endif

/***************************************************************************************************************; */
/* Imports from Media Control Module										; */
/*--------------------------------------------------------------------------------------------------------------; */
void	Media_ISR(),
	Media_Poll(),
	Media_Timer_ESR();

extern	Media_Status,
	Media_Memory_Address,
	Media_IO_Address,
	Media_IRQ_Number;

/*// Media Status Bit Map */
#define	MEDIA_INITIALIZED		0x01
#define	MEDIA_DRIVER_ENABLED		0x02
#define	MEDIA_INTERRUPTS_DISABLED	0x04
#define	MEDIA_IN_ISR			0x08
#define	MEDIA_IN_SEND			0x10
