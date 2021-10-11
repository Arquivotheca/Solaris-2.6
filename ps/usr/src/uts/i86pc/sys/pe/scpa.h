/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)scpa.h	1.2	93/11/02 SMI"

#define SCPA

#define uchar unsigned char
#define ulong unsigned long

/* scpacb, scpa Control Block. One is allocated for each instance of a
 * driver using a port.
 */

struct scpacb {
	int scpa_flags;			/* state flags */
	int scpa_mode;			/* Mode, used as index into spca_funcs*/
	int scpa_port;			/* lpt port number */
};

#define SPCA_FREE	0		/* Not in use */
#define SPCA_OPEN	1		/* in use */


typedef struct scpacb * scpa_cookie_t;


scpa_cookie_t scpa_open(int port);
void scpa_close(struct scpacb *scb);
int scpa_access_method(scpa_cookie_t, int pp_type, 
	char *protocol, char *muxtype);
int scpa_lock(scpa_cookie_t, long timeout);
int scpa_unlock(scpa_cookie_t);
int scpa_ctl(scpa_cookie_t, int option, void *param);

unsigned char scpa_inb(scpa_cookie_t, int offset);
int scpa_outb(scpa_cookie_t, int offset, int val);

ulong scpa_get_reg(scpa_cookie_t, int handle, ulong reg);
int scpa_set_reg(scpa_cookie_t, int handle, ulong reg, ulong val);
int scpa_set_ctrl_reg(scpa_cookie_t, int handle, ulong reg, ulong val);
int scpa_setup_get_block(scpa_cookie_t, int handle);
int scpa_finish_get_block(scpa_cookie_t, int handle);
int scpa_get_block(scpa_cookie_t,int handle, char far *buf, int count);
int scpa_setup_put_block(scpa_cookie_t, int handle);
int scpa_finish_put_block(scpa_cookie_t, int handle);
int scpa_put_block(scpa_cookie_t, int handle, char far *buf, int count);

struct scpa_func {
	int (*put_reg)(int handle, int reg, int val);
	int (*get_reg)(int handle, int reg);

	int (*put_block)(int handle, char far *buf, int count);
	int (*get_block)(int handle, char far *buf, int count);

	int (*put_ctrl_reg)(int handle, int reg, int val);
	int (*setup_get_block)(int handle);

	int (*setup_put_block)(int handle);
	int (*finish_get_block)(int handle);

	int (*finish_put_block)(int handle);
};

#define SCPA_MAXPORTS 3

#define SCPA_MAXSPCB 10			/* Max number of handles */

#define SCPA_PE3_DQNT 0 
#define SCPA_PE3_WBT 1
#define SCPA_PE3_PS2 2
#define SCPA_PE3_EPP 3
#define SCPA_PE3_COMPAQ 4
#define SCPA_PE2_DQNT 5
#define SCPA_PE2_WBT 6
#define SCPA_PE2_PS2 7
#define SCPA_PE2_EPP 8
#define SCPA_PE2_COMPAQ 9

#define SCPA_MAXMODE 9
