/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)scpa.c	1.3	93/12/01 SMI"

#include "scpa.h"

int scpa_ports[] = { 0x378, 0x278, 0x3bc };
struct scpacb scpacbs[SCPA_MAXSPCB];

int PE3_SH_Set_Ctrl_Reg(int h,int Reg, int Val);
int PE3_SH_Put_Register(int h, int Reg, int Val);
int PE3_SH_Block_Write(int h, char far * Buffer, int Count);
int PE3_DQNT_Get_Register(int h,int Reg);
int PE3_DQNT_Block_Read(int h,char far * Buffer, int Count);
int PE3_WBT_Get_Register(int h, int Reg);
int PE3_WBT_Block_Read(int h, char far * Buffer, int Count);
int PE3_PS2_Block_Write(int h, char far * Buffer, int Count);
int PE3_PS2_Get_Register(int h, int Reg);
int PE3_HH_Set_Ctrl_Reg(int h, int Reg, int Val);
int PE3_HH_Put_Register(int h, int Reg, int Val);
int PE3_HH_Get_Register(int h, int Reg);
int PE3_HH_Block_Write(int h, char far * Buffer, int Count);
int PE3_HH_Block_Read(int h, char far * Buffer, int Count);
int PE3_Setup_Block_Read(int h);
int PE3_Setup_Block_Write(int h);

int Set_PGA_Ctrl_Reg(int h, int Reg, int Value);
int PE2_SH_Put_Register(int h, int Reg, int Val);
int PE2_SH_Block_Write(int h, char far *Data, int Count);
int PE2_WBT_Get_Register(int h, int Reg);
int PE2_WBT_Block_Read(int h, char far * Data, int Count);
int PE2_PS2_Block_Write(int h, char far * Data, int Count);
int PE2_HH_Block_Write(int h, char far * Data, int Count);
int PE2_HH_Put_Register(int h, int Reg, int Value);
int PE2_HH_Get_Register(int h, int Reg);
int PE2_HH_Block_Read(int h, char far * Data, int Count);
int PE2_DQNT_Block_Read(int h, char far * Data, int Count);
int PE2_DQNT_Get_Register(int h, int Reg);
int PE3_CPQ_Get_Register(int h, int Reg);
int PE3_CPQ_Block_Read(int h, char far * Data, int Count);
int PE2_CPQ_Get_Register(int h, int Reg);
int PE2_CPQ_Block_Read(int h, char far * Data, int Count);
int PE3_Finish_Block_Read(int h);
int PE3_Finish_Block_Write(int h);

int null_func(int handle) {
	return -1;
}

struct scpa_func scpa_funcs[] = {
/* PE3_DQNT 0 */{	PE3_SH_Put_Register,PE3_DQNT_Get_Register,
			PE3_SH_Block_Write, PE3_DQNT_Block_Read,
			PE3_SH_Set_Ctrl_Reg,PE3_Setup_Block_Read,
			PE3_Setup_Block_Write, PE3_Finish_Block_Read,
			PE3_Finish_Block_Write },

/* PE3_WBT 1 */{	PE3_SH_Put_Register, PE3_WBT_Get_Register, 
			PE3_SH_Block_Write, PE3_WBT_Block_Read, 
			PE3_SH_Set_Ctrl_Reg,PE3_Setup_Block_Read,
			PE3_Setup_Block_Write, PE3_Finish_Block_Read,
			PE3_Finish_Block_Write },

/* PE3_PS2 2 */{	PE3_SH_Put_Register, PE3_PS2_Get_Register, 
			PE3_PS2_Block_Write, PE3_WBT_Block_Read, 
			PE3_SH_Set_Ctrl_Reg, PE3_Setup_Block_Read,
			PE3_Setup_Block_Write, PE3_Finish_Block_Read,
			PE3_Finish_Block_Write },

/* PE3_EPP 3 */{	PE3_HH_Put_Register, PE3_HH_Get_Register, 
			PE3_HH_Block_Write, PE3_HH_Block_Read, 
			PE3_HH_Set_Ctrl_Reg, PE3_Setup_Block_Read,
			PE3_Setup_Block_Write, PE3_Finish_Block_Read,
			PE3_Finish_Block_Write },

/* PE3_COMPAQ 4 */{	PE3_SH_Put_Register, PE3_CPQ_Get_Register,
			PE3_SH_Block_Write, PE3_CPQ_Block_Read,
			PE3_SH_Set_Ctrl_Reg, PE3_Setup_Block_Read,
			PE3_Setup_Block_Write, PE3_Finish_Block_Read,
			PE3_Finish_Block_Write },

/* PE2_DQNT 5 */{	PE2_SH_Put_Register, PE2_DQNT_Get_Register,
			PE2_SH_Block_Write, PE2_DQNT_Block_Read,
			Set_PGA_Ctrl_Reg, null_func,
			null_func, null_func, null_func},

/* PE2_WBT 6 */{	PE2_SH_Put_Register, PE2_WBT_Get_Register,
			PE2_SH_Block_Write, PE2_WBT_Block_Read,
			Set_PGA_Ctrl_Reg, null_func,
			null_func, null_func, null_func},

/* PE2_PS2 7 */{	PE2_SH_Put_Register, PE2_WBT_Get_Register,
			PE2_PS2_Block_Write, PE2_WBT_Block_Read,
			Set_PGA_Ctrl_Reg, null_func,
			null_func, null_func, null_func},

/* PE2_EPP 8 */{	PE2_HH_Put_Register, PE2_HH_Get_Register,
			PE2_HH_Block_Write, PE2_HH_Block_Read,
			Set_PGA_Ctrl_Reg, null_func,
			null_func, null_func, null_func},

/* PE2_COMPAQ 9 */ {	PE2_SH_Put_Register, PE2_CPQ_Get_Register,
			PE2_SH_Block_Write, PE2_CPQ_Block_Read,
			Set_PGA_Ctrl_Reg, null_func,
			null_func, null_func, null_func},
};

struct scpacb *scpacb_alloc() {
	int i;

	for (i = 0; i < SCPA_MAXSPCB; i++)
		if (scpacbs[i].scpa_flags == SPCA_FREE)
			break;

	if (i >= SCPA_MAXSPCB)
		return (struct scpacb *) 0;

	scpacbs[i].scpa_flags = SPCA_OPEN;
	scpacbs[i].scpa_mode = -1;

	return &scpacbs[i];
}

scpa_cookie_t scpa_open(int port) {
	struct scpacb *scb = scpacb_alloc();

	scb->scpa_port = port;

	return scb;
}

void scpa_close(struct scpacb *scb) {
	if (scb && scb->scpa_flags & SPCA_OPEN)
		scb->scpa_flags = SPCA_FREE;
}

int scpa_access_method(scpa_cookie_t scb, int pp_type, 
	char *protocol, char *muxtype) {

	if (pp_type < 0 || pp_type > SCPA_MAXMODE)
		return -1;

	scb->scpa_mode = pp_type;

	return 0;
}

int scpa_lock(scpa_cookie_t scb, long timeout){
	return -1;
}

int scpa_unlock(scpa_cookie_t scb){
	return -1;
}

int scpa_ctl(scpa_cookie_t scb, int option, void *param){
	return -1;
}


ulong scpa_get_reg(scpa_cookie_t scb, int handle, ulong reg) {
	return (*scpa_funcs[scb->scpa_mode].get_reg)(handle, reg);
}

int scpa_set_reg(scpa_cookie_t scb, int handle, ulong reg, ulong val) {
	return (*scpa_funcs[scb->scpa_mode].put_reg)(handle, reg, val);
}

int scpa_set_ctrl_reg(scpa_cookie_t scb, int handle, ulong reg, ulong val){
	return (*scpa_funcs[scb->scpa_mode].put_ctrl_reg)(handle, reg, val);
}

int scpa_setup_get_block(scpa_cookie_t scb, int handle) {
	return (*scpa_funcs[scb->scpa_mode].setup_get_block)(handle);
}

int scpa_finish_get_block(scpa_cookie_t scb, int handle) {
	return (*scpa_funcs[scb->scpa_mode].finish_get_block)(handle);
}

int scpa_get_block(scpa_cookie_t scb, int handle, char far *buf, int count) {
	return (*scpa_funcs[scb->scpa_mode].get_block)(handle, buf, count);
}

int scpa_setup_put_block(scpa_cookie_t scb, int handle) {
	return (*scpa_funcs[scb->scpa_mode].setup_put_block)(handle);
}

int scpa_finish_put_block(scpa_cookie_t scb, int handle) {
	return (*scpa_funcs[scb->scpa_mode].finish_put_block)(handle);
}

int scpa_put_block(scpa_cookie_t scb, int handle, char far *buf, int count) {
	return (*scpa_funcs[scb->scpa_mode].put_block)(handle, buf, count);
}

unsigned char scpa_inb(scpa_cookie_t scb, int offset) {
	return inb((scb->scpa_port + offset) & 0xffff);
}

int scpa_outb(scpa_cookie_t scb, int offset, int val) {
	outb((scb->scpa_port + offset) & 0xffff, val & 0xff);

	return 0;
}
