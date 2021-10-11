/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)unixglue.c	1.4	94/04/18 SMI"

void
pe_delay(h,len) 
int len;
{
	drv_usecwait(len);
}


int
LPT_Num_To_Port(h,lpt_num) 
int lpt_num;
{
	static int ports[] = { 0x378, 0x278, 0x3bc };

	if (lpt_num > 2 || lpt_num < 0)
		return -1;

	return ports[lpt_num];
}


void 
Hardware_Unhook(h) {}

void
Hardware_Setup_ISR(h) {}



int
Enable_EPP_Mode(h) {
	extern int Allow_EPP;

	if (!Allow_EPP) return 1;

	if (!Check_If_EPP_Capable(h))
		return 1;

	EPP_Enable(h,5);

	return 0;
}

int
Disable_EPP_Mode(h) {
	EPP_Disable(h);

	return 0;
}


void
PS2_Setup(h) {}
	
int
Check_Compaq_Signature(h) {
	return 1;
}

void
Find_IRQ_Number(h) {}
int
Check_Already_Loaded(h) {
	return 1;
}
