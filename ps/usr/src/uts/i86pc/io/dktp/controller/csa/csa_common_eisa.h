/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_COMMON_EISA_H
#define	_CSA_CSA_COMMON_EISA_H

#pragma	ident	"@(#)csa_common_eisa.h	1.1	95/05/17 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	EISA_CFG0	0xc80	/* EISA configuration port 0 */
#define	EISA_CFG1	0xc81	/* EISA configuration port 1 */
#define	EISA_CFG2	0xc82	/* EISA configuration port 2 */
#define	EISA_CFG3	0xc83	/* EISA configuration port 3 */


Bool_t	eisa_probe(dev_info_t *devi, ushort ioaddr);

typedef	struct	product_id_spec {
	ulong	id;
	ulong	mask;
} pid_spec_t;


#ifdef	__cplusplus
}
#endif

#endif	/* _CSA_CSA_COMMON_EISA_H */
