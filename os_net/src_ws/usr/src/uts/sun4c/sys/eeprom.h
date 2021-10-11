/*
 * Copyright (c) 1990-1996, by Sun Microsystems, Inc.
 */

#ifndef _SYS_EEPROM_H
#define	_SYS_EEPROM_H

#pragma ident	"@(#)eeprom.h	1.9	96/02/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The EEPROM is part of the Mostek MK48T02 clock chip. The EEPROM
 * is 2K, but the last 8 bytes are used as the clock, and the 32 bytes
 * before that emulate the ID prom. There is no
 * recovery time necessary after writes to the chip.
 */
#ifndef _ASM
#include <sys/types.h>

struct ee_soft {
	u_short	ees_wrcnt[3];		/* write count (3 copies) */
	u_short	ees_nu1;		/* not used */
	u_char	ees_chksum[3];		/* software area checksum (3 copies) */
	u_char	ees_nu2;		/* not used */
	u_char	ees_resv[0xd8-0xc];	/* XXX - figure this out sometime */
};
#endif /* !_ASM */

#define	OBIO_EEPROM_ADDR 0xF2000000	/* address of eeprom in obio space */

#define	EEPROM_ADDR	0xFFFF8000	/* virtual address we map eeprom to */
#define	EEPROM_SIZE	0x7D8		/* size of eeprom in bytes */
#define	EEPROM		((struct eeprom *)EEPROM_ADDR)

/*
 * ID prom constants. They are included here because the ID prom is
 * emulated by stealing 20 bytes of the eeprom.
 */
#define	IDPROM_ADDR	(EEPROM_ADDR+EEPROM_SIZE) /* virtual addr of idprom */
#define	IDPROMSIZE	0x20			/* size of ID prom, in bytes */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_EEPROM_H */
