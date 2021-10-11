/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _DPT_EISA_H
#define	_DPT_EISA_H
#pragma	ident	"@(#)dpt_eisa.h	1.1	96/06/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif


#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#define	EISA_CFG0	0xc80	/* EISA configuration port 0 */
#define	EISA_CFG1	0xc81	/* EISA configuration port 1 */
#define	EISA_CFG2	0xc82	/* EISA configuration port 2 */
#define	EISA_CFG3	0xc83	/* EISA configuration port 3 */


typedef	struct	product_id_spec {
	ulong	id;
	ulong	mask;
} pid_spec_t;


int eisa_probe(dev_info_t *devi, ushort ioaddr, pid_spec_t *default_pid,
		int   prod_idlen);


#ifdef	__cplusplus
}
#endif
#endif	/* _DPT_EISA_H */
