/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ncr_sni.c	1.6	96/02/02 SMI"

#include <sys/dktp/ncrs/ncr.h>
#include "ncr_sni.h"


extern	bool_t	ncr53c710_probe(int *, int, int *, int, bus_t, bool_t);

/*
 * EISA product ids accepted by this driver for I/O addresses 0x800 to 0x8ff 
 */
int	ncr53c710_SNI_board_ids[] = {
		0xC1AAC94D	/* Siemens/Nixdorf Inc PCE-5S */
};

/*
 * This is the special probe routine for the Siemens/Nixdorf Inc PCE-5S
 *
 * For the normal EISA slot addresses the ncr53c710_probe() routine
 * is called via the normal ncr53c710_nops table.  For the two
 * embedded 53c710 controllers it verifies the motherboard product
 * ID number and check the CMOS RAM to determine if the controller
 * has been enabled via the CMOS setup or the EISA config utility.
 */

bool_t
ncr53c710_probe_SNI(	dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing )
{
	ioadr_t	slotadr = *regp;
	int	cnt;
	unchar	config_byte;
	
	if (bus_type != BUS_TYPE_EISA)
		return (FALSE);

	/* the SNI PCE-5S can have two embedded 710 controllers */
	if (slotadr != NCR53C710_SNI_CTL1_ADDR
	&&  slotadr != NCR53C710_SNI_CTL2_ADDR) {
		return (FALSE);
	}
	
	/* allow product-id property to override the default EISA id */
	if (pidp == NULL) {
		/* Handle embedded SNI 53c710 controllers */
		pidp = ncr53c710_SNI_board_ids;
		pidlen = sizeof ncr53c710_SNI_board_ids;
	}

	/* check the product id of the motherboard for a match */
	for (cnt = pidlen / sizeof (int); cnt > 0; cnt--, pidp++) {
		if (eisa_probe(0, *pidp)) {
			NDBG4(("ncr53c710_probe success %d %d\n", *pidp, cnt));
			goto got_it;
		}
	}
	return (FALSE);

    got_it:
	/* on the SNI PCE-5S need to check the CMOS ram configuration */
	/* byte to determine if the controller is enabled */
	config_byte = GET_CMOS_BYTE(0x29);
	config_byte &= SNI_CMOS_ENABLE_MASK;

	if (*regp == NCR53C710_SNI_CTL1_ADDR) {
		return (config_byte != SNI_CMOS_SCSI_DISABLED);
	}

	if (*regp == NCR53C710_SNI_CTL2_ADDR) {
		if (config_byte == SNI_CMOS_SCSI_1_2
		||  config_byte == SNI_CMOS_SCSI_2_1)
			return (TRUE);
		return (FALSE);
	}
	return (FALSE);
}


bool_t
ncr_get_irq_eisa_SNI(	ncr_t		*ncrp,
			int		*regp,
			int		 reglen )
{
	unchar	config_byte;
	unchar	tmp;
	bool_t	swapped_config = FALSE;

	/* Check the CMOS ram configuration byte to determine the
	/* IRQs for the two embedded 710s */

	config_byte = GET_CMOS_BYTE(0x29);

	/* If controller #1 is on IRQ14 then the config bits are swapped */
	if ((config_byte & SNI_CMOS_IRQ1_MASK) == SNI_CMOS_SCSI_IRQ1_14)
		swapped_config = TRUE;

	if (*regp == NCR53C710_SNI_CTL1_ADDR) {
		if (swapped_config)
			goto config2;
		goto config1;
	}
	if (*regp == NCR53C710_SNI_CTL2_ADDR) {
		if (swapped_config)
			/* I know it's IRQ 14 but goto the switch anyways */
			goto config1;
		goto config2;
	}
	return (FALSE);

    config1:
	switch (config_byte & SNI_CMOS_IRQ1_MASK) {
	case SNI_CMOS_SCSI_IRQ1_10:
		ncrp->n_irq = 10;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ1_11:
		ncrp->n_irq = 11;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ1_14:
		ncrp->n_irq = 14;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ1_15:
		ncrp->n_irq = 15;
		return (TRUE);
	}
	return (FALSE);

    config2:
	switch (config_byte & SNI_CMOS_IRQ2_MASK) {
	case SNI_CMOS_SCSI_IRQ2_10:
		ncrp->n_irq = 10;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ2_11:
		ncrp->n_irq = 11;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ2_14:
		ncrp->n_irq = 14;
		return (TRUE);
	case SNI_CMOS_SCSI_IRQ2_15:
		ncrp->n_irq = 15;
		return (TRUE);
	}
	return (FALSE);
}
