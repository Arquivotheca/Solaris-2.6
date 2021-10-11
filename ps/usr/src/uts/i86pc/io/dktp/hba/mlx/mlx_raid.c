/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_raid.c	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>

/*
 * The ccb allocated here is used only during mlx_attach() to be used
 * only in mlx_init_cmd() and will be discarded at the end of instance
 * initialization.
 */
int
mlx_ccbinit(register mlx_hba_t *const hba)
{
	size_t mem = sizeof (mlx_ccb_t);
	mlx_ccb_t *ccb;
	register ddi_dma_lim_t *dma_lim;

	ASSERT(hba != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));

	dma_lim = &mlx_dac_dma_lim;
	if (MLX_SCSI(hba))
		mem += sizeof (mlx_cdbt_t);

	if (ddi_iopb_alloc(hba->dip, dma_lim, mem,
	    (caddr_t *)&ccb) == DDI_FAILURE)
		return (DDI_FAILURE);
	bzero((caddr_t)ccb, mem);			/* needed */

	if (MLX_SCSI(hba))
		ccb->ccb_cdbt = (mlx_cdbt_t *)(ccb + 1);

	hba->ccb = ccb;
	ccb->paddr = MLX_KVTOP(ccb);
	return (DDI_SUCCESS);
}

/*
 * Finds MAX_TGT, max number of targets per channel and NCHN, the number
 * of real physical channels on the card and assigns them to mlx->max_tgt
 * and mlx->nchn respectively.
 *
 * Returns 1 on success and 0 on failure.
 */
int
mlx_getnchn_maxtgt(register mlx_t *const mlx, register mlx_ccb_t *const ccb)
{
	register int chn;
	register int tgt;
	u_char device_state[12];

	ASSERT(mlx != NULL);
	ASSERT(ccb != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));

	/*
	 * Prepare a ccb for a GET-DEVICE-STATE command.
	 * This command is a lot faster than the SCSI INQUIRY command.
	 */
	ccb->type = MLX_DAC_CTYPE2;
	ccb->ccb_opcode = MLX_DAC_GSTAT;

	/*
	 * ccb->ccb_xferpaddr is not used.  However, it is better be
	 * set to some valid paddr, otherwise the controller will
	 * transfer data to some arbitrary physical address which
	 * can be very dangerous.
	 */
	ccb->ccb_xferpaddr = MLX_KVTOP(&device_state);

	/*
	 * Let's find MAX_TGT first.  There is at least one channel on
	 * the card and we know it is channel 0.  So we set channel to
	 * 0 and change targets in the reverse order as there is a good
	 * chance that fewer commands need to be sent this way.
	 */
	ccb->ccb_chn = 0;
	for (tgt = 7; tgt >= 0; tgt--) {
		if (tgt == mlx->initiatorid)
			continue;

		ccb->ccb_tgt = (u_char)tgt;
		if (mlx_init_cmd(mlx, ccb) == DDI_FAILURE)
			return (0);

		if (ccb->ccb_status == MLX_SUCCESS)
			break;			/* found a valid tgt */
	}
	if (!tgt) {
		cmn_err(CE_WARN, "mlx_getnchn_maxtgt: MAX_TGT == 0");
		return (0);
	}
	mlx->max_tgt = (u_char)(tgt + 1);

	/*
	 * Now let's find NCHN:  We know that target tgt is valid on all
	 * channels so we set the target to be that.  Furthermore, we can
	 * skip channel 0 that we have already looped through.
	 */
	ccb->ccb_tgt = (u_char)tgt;
	for (chn = 1; chn <= MLX_DAC_CHN_NUM; chn++) {
		ccb->ccb_chn = (u_char)chn;
		if (mlx_init_cmd(mlx, ccb) == DDI_FAILURE)
			return (0);

		if (ccb->ccb_status == MLX_E_INVALCHN)
			break;			/* found an invalid chn */
	}
	if (chn == 0xff) {
		cmn_err(CE_WARN, "mlx_getnchn_maxtgt: NCHN == MLX_DAC_CHN_NUM");
		return (0);
	}
	mlx->nchn = (u_char)chn;

	MDBG5(("mlx_getnchn_maxtgt: nchn %d max_tgt %d",
		mlx->nchn, mlx->max_tgt));

	return (1);
}

/* Get System Drive configuration table from DAC960 EEPROM. */
int
mlx_getconf(register mlx_t *const mlx, register mlx_ccb_t *const ccb)
{
	register mlx_dac_conf_t *conf;
	size_t mem;

	ASSERT(mlx != NULL);
	ASSERT(ccb != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));

	/* Prepare a ccb for a READ-ROM-CONFIGURATION command */
	ccb->type = MLX_DAC_CTYPE5;
	ccb->ccb_opcode = MLX_DAC_RDCONFIG;

	mem = sizeof (*conf) - sizeof (mlx_dac_tgt_info_t) +
		((mlx->nchn * mlx->max_tgt) * sizeof (mlx_dac_tgt_info_t));
	conf = (mlx_dac_conf_t *)kmem_zalloc(mem, KM_NOSLEEP);
	if (conf == NULL)
		return (DDI_FAILURE);

	mlx->conf = conf;
	ccb->ccb_xferpaddr = MLX_KVTOP(mlx->conf);

	if (mlx_init_cmd(mlx, ccb) == DDI_FAILURE)
		return (DDI_FAILURE);

	if (ccb->ccb_status) {
		cmn_err(CE_WARN, "mlx_getconf: failed to read DAC960 EEPROM");
		return (DDI_FAILURE);
	}

	MDBG5(("mlx_getconf: %d System Drive(s)", conf->nsd));

	return (DDI_SUCCESS);
}

/*
 * Issues a MLX_DAC_ENQUIRY command and preserves the received info for
 * future use.  However, the max_cmd field is used immediately to set
 * up mlx->ccb_stk.
 */
int
mlx_getenquiry(register mlx_t *const mlx, register mlx_ccb_t *const ccb)
{
	register mlx_dac_enquiry_t *enq;

	ASSERT(mlx != NULL);
	ASSERT(ccb != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));

	enq = (mlx_dac_enquiry_t *)kmem_zalloc(sizeof (mlx_dac_enquiry_t),
	    KM_NOSLEEP);
	if (enq == NULL)
		return (DDI_FAILURE);

	mlx->enq	= enq;
	ccb->type	= MLX_DAC_CTYPE5;
	ccb->ccb_opcode	= MLX_DAC_ENQUIRY;
	ccb->ccb_xferpaddr	= MLX_KVTOP(mlx->enq);
	if (mlx_init_cmd(mlx, ccb) == DDI_FAILURE)
		return (DDI_FAILURE);

	mlx_getenq_info(mlx);

	return (DDI_SUCCESS);
}

/*
 * Extracts the info needed from the Enquiry Info and preserves it
 * it as part of the current s/w state.
 */
void
mlx_getenq_info(register mlx_t *const mlx)
{
	register mlx_dac_enquiry_t *enq;

	ASSERT(mlx != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex) || mutex_owned(&mlx->mutex));
	enq = mlx->enq;
	ASSERT(enq != NULL);
	ASSERT(mlx->conf != NULL);

	/* To ensure that mlx->enq and mlx->conf are always in sync */
	ASSERT(mlx->conf->nsd == enq->nsd);

	mlx->max_cmd = enq->max_cmd;
	/*
	 * F/w versions are either 1.22, 1.23, 1.3x, 1.5x or above.
	 * The ones before 1.22 are UNKNOWN, versions 1.3x and 1.5x
	 * are identical as far the Solaris driver is concerned,
	 * versions equal or greater than 1.5x are promised to be
	 * backward compatible with 1.5x.
	 */
	switch (enq->fw_num.release) {
	case 0:
		mlx->fw_version = UNKNOWN;
		break;
	case 1:
		if (enq->fw_num.version < 22)
			mlx->fw_version = UNKNOWN;
		else {
			switch (enq->fw_num.version) {
			case 22:
				mlx->fw_version = R1V22;
				break;
			case 23:
				mlx->fw_version = R1V23;
				break;
			default:
				mlx->fw_version = R1V5x;
				break;
			}
		}
		break;
	default:
		mlx->fw_version = R1V5x;
		break;
	}

	switch (mlx->fw_version) {
	case UNKNOWN:
	case R1V22:
		mlx->flags &= ~MLX_SUPPORTS_SG;
		mlx->sgllen = 1;
		mlx->flags &= ~MLX_NO_HOT_PLUGGING;
		break;
	case R1V23:
		mlx->flags |= MLX_SUPPORTS_SG;
		mlx->sgllen = MLX_MAX_NSG;
		mlx->flags &= ~MLX_NO_HOT_PLUGGING;
		break;
	case R1V5x:
		mlx->flags |= MLX_SUPPORTS_SG;
		mlx->sgllen = MLX_MAX_NSG;
		mlx->flags |= MLX_NO_HOT_PLUGGING;
		break;
	default:
		ASSERT(0);	/* bad f/w version! */
		break;
	}
	MDBG5(("mlx_getenq_info: f/w ver %d.%d (%d), max %d concurrent "
		"cmds, flags=0x%x", enq->fw_num.release, enq->fw_num.version,
		mlx->fw_version, mlx->max_cmd, mlx->flags));
	/*
	 * Do not use mlx->fw_version after this point.  It is better
	 * be used ONLY in the above to set up mlx->flags (create more
	 * flags if need be), otherwise it is not easy to keep track
	 * of different versions and how they effect this driver.
	 */
}

int
mlx_ccb_stkinit(register mlx_t *const mlx)
{
	u_char stk_size;
	register int i;
	register mlx_ccb_stk_t *ccb_stk;

	ASSERT(mlx != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));
	ASSERT(mlx->flags & MLX_GOT_ENQUIRY);

	stk_size = mlx->max_cmd + 1; /* the extra one flags the end of stack */

	ccb_stk = (mlx_ccb_stk_t *)kmem_zalloc(stk_size * sizeof (*ccb_stk),
	    KM_NOSLEEP);
	if (ccb_stk == NULL)
		return (DDI_FAILURE);
	mlx->free_ccb = mlx->ccb_stk = ccb_stk;

	/*
	 * Set up the free list:  The next field of every element on
	 * the stack contains the index of the next free element.
	 */
	for (i = 1; i < stk_size; i++, ccb_stk++) {
		ccb_stk->next = (short)i;
		ccb_stk->ccb = NULL;
	}
	/* Indicating the end of free ccb's and the end of stack. */
	ccb_stk->next = MLX_INVALID_CMDID;
	ccb_stk->ccb = NULL;

	/*
	 * mlx->free_ccb is guaranteed to point to a free element in
	 * the stack, iff (mlx->free_ccb->next != MLX_INVALID_CMDID).
	 * Otherwise there is no free element on the stack and we have
	 * to wait until some element becomes free.
	 *
	 * ccb_stk_t is deliberately chosen to be a struct and not
	 * a union so that the ccb field can be set to NULL for the
	 * free ccb's.  Because mlx_intr() has enough checking means
	 * to safe-guard itself from the spurious interrupts.
	 */

	return (DDI_SUCCESS);
}

/*
 * If the target is part of any System-Drive, it returns the address of
 * that System Drive structure.  Otherwise, it returns NULL.
 */
mlx_dac_sd_t *
mlx_in_any_sd(register mlx_t *const mlx, const u_char chn, const u_char tgt)
{
	u_char nsd;
	register int s;
	register mlx_dac_conf_t *conf;

	ASSERT(mlx != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex) || mutex_owned(&mlx->mutex));
	conf = mlx->conf;
	ASSERT(conf != NULL);

	nsd = conf->nsd;
	if (!nsd)
		return (NULL);

	/*
	 * Mylex guarantees that the System-Drive id's will always be in
	 * ascending order starting from 0.  And so will the arm and disk
	 * id's.
	 */
	ASSERT(nsd <= MLX_DAC_MAX_SD);
	for (s = 0; s < nsd; s++) {
		register int a;
		register mlx_dac_sd_t *sd = &conf->sd[s];
		u_char narm;

		ASSERT(sd != NULL);
		narm = sd->narm;
		if (!narm)
			continue;
		ASSERT(narm <= MLX_DAC_MAX_ARM);
		for (a = 0; a < narm; a++) {
			register int d;
			register mlx_dac_arm_t *arm = &sd->arm[a];
			u_char ndisk;

			ASSERT(arm != NULL);
			ndisk = arm->ndisk;
			if (!ndisk)
				continue;
			ASSERT(ndisk <= MLX_DAC_MAX_DISK);
			for (d = 0; d < ndisk; d++) {
				register mlx_dac_disk_t *disk = &arm->disk[d];

				ASSERT(disk != NULL);
				if (disk->tgt == tgt && disk->chn == chn) {
					MDBG5(("mlx_in_any_sd: chn %d, tgt %d"
						" => System Drive %d "
						"arm %d disk %d",
						chn, tgt, s, a, d));
					return (sd);
				}
			}
		}
	}
	return (NULL);
}

/*
 * Returns 1 if the target should not be accessed for any reason:
 *	It is a STANDBY drive or
 *	It is part of a System Drive.
 * Otherwise it returns 0.
 */
int
mlx_dont_access(register mlx_t *const mlx, const u_char chn, const u_char tgt)
{
	register mlx_dac_tgt_info_t *tgt_info;

	ASSERT(mlx != NULL);
	ASSERT(tgt < mlx->max_tgt);
	ASSERT(mutex_owned(&mlx_global_mutex) || mutex_owned(&mlx->mutex));
	ASSERT(mlx->conf != NULL);

	if (tgt == mlx->initiatorid || mlx_in_any_sd(mlx, chn, tgt) != NULL)
		return (1);

	tgt_info = &mlx->conf->tgt_info[(chn * mlx->max_tgt) + tgt];

	/*
	 * Unfortunately, the DAC960 f/w does not distinguish between
	 * a DEAD disk and a non-System Drive.  Otherwise, we ought to
	 * have tested for DEAD disks here and disallowed access to
	 * them if the f/w version did not support hot-plugging.
	 *
	 * The only reliable way to tell between DEAD disks from
	 * non-System Drives is to send a SCSI inquiry command to the
	 * device.  Therefore, DEAD disks are deliberately considered
	 * accessible here, allowing mlx_getinq() to send the
	 * SCSI inquiry command and have the inq_dtype field set to
	 * DTYPE_NOTPRESENT for the DEAD disks. This info will be used
	 * later in mlx_tran_tgt_init().
	 */

	if (tgt_info->state == MLX_DAC_TGT_STANDBY) {
		cmn_err(CE_CONT, "?Access denied to STANDBY drive "
			"SCSI id %d on channel %d\n", tgt, chn);
		return (1);
	}

	return (0);
}
