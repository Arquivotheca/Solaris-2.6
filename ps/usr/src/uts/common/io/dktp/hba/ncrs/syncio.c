/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)syncio.c	1.6	96/04/25 SMI"

#include <sys/dktp/ncrs/ncr.h>


/*
 * establish a new sync i/o state for all the luns on a target
 */

void
ncr_syncio_state(	ncr_t	*ncrp,
			npt_t	*nptp,
			unchar	 state,
			unchar	 sxfer,
			unchar	 sscfX10 )
{
	ushort	 target;
	ushort	 lun;

	/* change state of all LUNs on this target */
	NSYNCSTATE(ncrp, nptp) = state;
	target = nptp->nt_target;
	for (lun = 0; lun < NLUNS_PER_TARGET; lun++) {
		/* store new sync i/o parms in each per-target-struct */
		if ((nptp = NTL2UNITP(ncrp, target, lun)) != NULL) {
			nptp->nt_selectparm.nt_sxfer = sxfer;
			nptp->nt_sscfX10 = sscfX10;
		}
	}

	return;
}


void
ncr_syncio_disable( ncr_t *ncrp )
{
	ushort	 target;

	NDBG31(("ncr_syncio_disable: ioaddr=0x%x\n", ncrp->n_ioaddr));

	for (target = 0; target < NTARGETS; target++)
		ncrp->n_syncstate[target] = NSYNC_SDTR_REJECT;

	return;
}


void
ncr_syncio_reset_target(	ncr_t	*ncrp,
				int	 target )
{
	npt_t	*nptp;
	ushort	 lun;

	/* check if sync i/o negotiation disabled on this target */
	if (target == ncrp->n_initiatorid)
		ncrp->n_syncstate[target] = NSYNC_SDTR_REJECT;
	else if (ncrp->n_syncstate[target] != NSYNC_SDTR_REJECT)
		ncrp->n_syncstate[target] = NSYNC_SDTR_NOTDONE;

	for (lun = 0; lun < NLUNS_PER_TARGET; lun++) {
		if ((nptp = NTL2UNITP(ncrp, target, lun)) != NULL) {
			nptp->nt_selectparm.nt_sxfer = 0;
		}
	}

}


void
ncr_syncio_reset(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	ushort	target;

	NDBG31(("ncr_syncio_reset: ioaddr=0x%x\n", ncrp->n_ioaddr));

	if (nptp != NULL) {
		/* only reset the luns on this target */
		ncr_syncio_reset_target(ncrp, nptp->nt_target);
		return;
	}

	/* set the max offset to zero to disable sync i/o */
	for (target = 0; target < NTARGETS; target++) {
		ncr_syncio_reset_target(ncrp, target);
	}
	return;
}

void
ncr_syncio_msg_init(	ncr_t	*ncrp,
			npt_t	*nptp )
{

	nptp->nt_sendmsg.count = 6;
	nptp->nt_syncobuf[0] = 0x01;
	nptp->nt_syncobuf[1] = 0x03;
	nptp->nt_syncobuf[2] = 0x01;
	nptp->nt_syncobuf[3] = ncrp->n_minperiod[nptp->nt_target] / 4;
	nptp->nt_syncobuf[4] = 8;

	return;
}


static bool_t
ncr_syncio_enable(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	unchar	 sxfer;
	unchar	 sscfX10;
	int	 time_ns;
	int	 offset;
	int	 initiator_time_ns;
	int	 initiator_offset;


	/* units for transfer period factor are 4 nsec. */
	time_ns = nptp->nt_syncibuf[1] * 4;
	initiator_time_ns = nptp->nt_syncobuf[3] * 4;

	offset = nptp->nt_syncibuf[2];
	initiator_offset = nptp->nt_syncobuf[4];

	nptp->nt_fastscsi = FALSE;

	/* check for 0 offset from target meaning "async" */
	if (offset == 0) {
		/* set asynchronous mode for this target */
		ncr_syncio_state(ncrp, nptp, NSYNC_SDTR_DONE, 0, 0);
		return (TRUE);
	}

	/* check the period returned by the target */
	/* target shouldn't try to decrease my period */
	if (time_ns < initiator_time_ns
	|| !ncr_max_sync_divisor(ncrp, time_ns, &sxfer, &sscfX10)) {
		NDBG31(("ncr_syncio_enable: invalid period: %d,%d\n"
				, time_ns, offset));
		return (FALSE);
	}

	/* check the offset returned by the target */
	if (offset > initiator_offset) {
		NDBG31(("ncr_syncio_enable: invalid offset: %d,%d\n"
				, time_ns, offset));
		return (FALSE);
	}

	/* encode the divisor and offset values */
	sxfer = ((sxfer - 4) << 5) + offset;

	nptp->nt_fastscsi = (time_ns < 200) ? TRUE : FALSE;

	/* set the max offset and clock divisor for all LUNs on this target */
	ncr_syncio_state(ncrp, nptp, NSYNC_SDTR_DONE, sxfer, sscfX10);
	return (TRUE);
}

/*
 * The target started the synchronous i/o negotiation sequence by
 * sending me a SDTR message. Look at the target's parms and the
 * HBA's defaults and decide on the appropriate compromise. Send the
 * larger of the two transfer periods and the smaller of the two offsets.
 */
static bool_t
ncr_syncio_respond(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	unchar	sxfer;
	unchar	sscfX10;
	int	time_ns;
	int	offset;
	int	initiator_time_ns;
	int	initiator_offset;

	/* use the smallest offset */
	offset = nptp->nt_syncibuf[2];
	initiator_offset = 8;
	if (offset > initiator_offset)
		offset = initiator_offset;

	/* units for transfer period factor are 4 nsec. */
	time_ns = nptp->nt_syncibuf[1] * 4;
	initiator_time_ns = ncrp->n_minperiod[nptp->nt_target];

	/* use largest time period */
	if (time_ns < initiator_time_ns) {
		time_ns = initiator_time_ns;
	}

	if (!ncr_max_sync_divisor(ncrp, time_ns, &sxfer, &sscfX10)) {
		NDBG31(("ncr_syncio_respond: invalid period: %d,%d\n"
				, time_ns, offset));
		return (FALSE);
	}

	sxfer = ((sxfer - 4) << 5) + offset;

	/* set the max offset and clock divisor for all LUNs on this target */
	ncr_syncio_state(ncrp, nptp, NSYNC_SDTR_DONE, sxfer, sscfX10);

	/* report to target the adjusted period */
	if ((time_ns = ncr_period_round(ncrp, time_ns)) == -1) {
		NDBG31(("ncr_syncio_respond: round failed time=%d\n"
			, time_ns));
		return (FALSE);
	}

	nptp->nt_syncobuf[0] = 0x01;
	nptp->nt_syncobuf[1] = 0x03;
	nptp->nt_syncobuf[2] = 0x01;
	nptp->nt_syncobuf[3] = time_ns / 4;
	nptp->nt_syncobuf[4] = offset;

	nptp->nt_fastscsi = (time_ns < 200) ? TRUE : FALSE;

	return (TRUE);
}

ulong
ncr_syncio_decide(	ncr_t	*ncrp,
			npt_t	*nptp,
			ulong	 action )
{
	if (action & (NACTION_SIOP_HALT | NACTION_SIOP_REINIT
			| NACTION_BUS_FREE)) {
		/* set all LUNs on this target to renegotiate syncio */
		ncr_syncio_reset(ncrp, nptp);
		return (action);
	}

	if (action & (NACTION_DONE | NACTION_ERR)) {
		/* the target finished without responding to SDTR */
		/* set all LUN's on this target to async mode */
		ncr_syncio_state(ncrp, nptp, NSYNC_SDTR_DONE, 0, 0);
		return (action);
	}

	if (action & (NACTION_MSG_PARITY | NACTION_INITIATOR_ERROR)) {
		/* allow target to try to do error recovery */
		return (action);
	}

	if ((action & NACTION_SDTR) == 0) {
		return (action);
	}
		
	/* if got good SDTR response, enable sync i/o */
	switch (NSYNCSTATE(ncrp, nptp)) {
	case NSYNC_SDTR_SENT:
		if (ncr_syncio_enable(ncrp, nptp)) {
			/* reprogram the sxfer register */
			NCR_SET_SYNCIO(ncrp, nptp);
			return (NACTION_ACK | action);
		}
		break;

	case NSYNC_SDTR_RCVD:
		/* if target initiated SDTR handshake, send response */
		if (ncr_syncio_respond(ncrp, nptp)) {
			/* reprogram the sxfer register */
			NCR_SET_SYNCIO(ncrp, nptp);
			return (NACTION_SDTR_OUT | action);
		}
		break;

	case NSYNC_SDTR_REJECT:
		return(NACTION_MSG_REJECT | action);
	}

	/* target and hba counldn't agree on sync i/o parms, so */
	/* set all LUN's on this target to async mode until */
	/* next bus reset */
	ncr_syncio_state(ncrp, nptp, NSYNC_SDTR_DONE, 0, 0);
	return (NACTION_MSG_REJECT | action);
}
