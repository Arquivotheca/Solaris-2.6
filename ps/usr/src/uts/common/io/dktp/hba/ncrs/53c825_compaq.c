/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)53c825_compaq.c	1.2	95/12/14 SMI"

#include <sys/dktp/ncrs/ncr.h>

#include <sys/dktp/ncrs/ncr.h>
#include <sys/dktp/ncrs/53c8xx.h>
#include <sys/dktp/ncrs/script.h>

/*
 * EISA product ids accepted by this driver for slots 0x1000 to 0xf000
 */
int	ncr53c825_eisa_board_ids[] = {
		0x3044110e	/* Compaq part ?????, rev 0 */
};

static bool_t
ncr53c825_compaq_probe(
			dev_info_t	*dip,
			int	*regp,
			int	 len,
			int	*pidp,
			int	 pidlen,
			bus_t	 bus_type,
			bool_t	 probing)
{
	ioadr_t	slotadr = *regp;
	int	cnt;


	if (bus_type != BUS_TYPE_EISA)
		return (FALSE);

	/* allow product-id property to override the default EISA id */
	if (pidp == NULL) {
		if (slotadr < 0x1000)
			return (FALSE);

		/* Handle normal EISA cards in a normal EISA slot */
		pidp = ncr53c825_eisa_board_ids;
		pidlen = sizeof (ncr53c825_eisa_board_ids);
	}

	/* check the product id of the current slot for a match */
	for (cnt = pidlen / sizeof (int); cnt > 0; cnt--, pidp++) {
		if (eisa_probe(slotadr, *pidp)) {
			NDBG4(("ncr53c825_probe success %d %d\n", *pidp, cnt));
			return (TRUE);
		}
	}
	return (FALSE);
}

void	ncr53c810_reset(ncr_t *ncrp);
void	ncr53c810_init(ncr_t *ncrp);
void	ncr53c810_enable(ncr_t *ncrp);
void	ncr53c810_disable(ncr_t *ncrp);
unchar	ncr53c810_get_istat(ncr_t *ncrp);
void	ncr53c810_halt(ncr_t *ncrp);
void	ncr53c810_set_sigp(ncr_t *ncrp);
void	ncr53c810_reset_sigp(ncr_t *ncrp);
ulong	ncr53c810_get_intcode(ncr_t *ncrp);
void	ncr53c810_check_error(npt_t *nptp, struct scsi_pkt *pktp);
ulong	ncr53c810_dma_status(ncr_t *ncrp);
ulong	ncr53c810_scsi_status(ncr_t *ncrp);
bool_t	ncr53c810_save_byte_count(ncr_t *ncrp, npt_t *nptp);
bool_t	ncr53c810_get_target(ncr_t *ncrp, unchar *tp);
unchar	ncr53c810_encode_id(unchar id);
void	ncr53c810_set_syncio(ncr_t *ncrp, npt_t *nptp);
void	ncr53c810_setup_script(ncr_t *ncrp, npt_t *nptp);
void	ncr53c810_start_script(ncr_t *ncrp, int script);
void	ncr53c810_bus_reset(ncr_t *ncrp);

int	ncr53c710_geometry(ncr_t *ncrp, struct scsi_address *ap);

nops_t	ncr53c825_nops_compaq = {
	"53c825_compaq",
	ncr_script_init,
	ncr_script_fini,
	ncr53c825_compaq_probe,
	ncr_get_irq_eisa,
	ncr_xlate_irq_no_sid,
#ifdef	PCI_DDI_EMULATION
	ncr_get_ioaddr_eisa,
#else
	NCR_EISA_RNUMBER,
#endif
	ncr53c810_reset,
	ncr53c810_init,
	ncr53c810_enable,
	ncr53c810_disable,
	ncr53c810_get_istat,
	ncr53c810_halt,
	ncr53c810_set_sigp,
	ncr53c810_reset_sigp,
	ncr53c810_get_intcode,
	ncr53c810_check_error,
	ncr53c810_dma_status,
	ncr53c810_scsi_status,
	ncr53c810_save_byte_count,
	ncr53c810_get_target,
	ncr53c810_encode_id,
	ncr53c810_setup_script,
	ncr53c810_start_script,
	ncr53c810_set_syncio,
	ncr53c810_bus_reset,

	/* the Compaq 32-Bit Fast-Wide SCSI-2 uses the 710 geometry function */
	ncr53c710_geometry
};
