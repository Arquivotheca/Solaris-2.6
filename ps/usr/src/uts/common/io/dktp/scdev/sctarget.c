/*
 * Copyright (c) 1994 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sctarget.c	1.4	96/05/27 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dktp/sctarget.h>
#include <sys/byteorder.h>

/*
 * Local static data
 */

/*
 * functions to convert between host format and scsi format
 */
ulong
scsi_stoh_3byte(unchar *ap)
{
	register ulong av = *(ulong *)ap;

#if defined(_LITTLE_ENDIAN)
	return (((av & 0xff) << 16) | (av & 0xff00) | ((av & 0xff0000) >> 16));
#elif defined(_BIG_ENDIAN)
	return (av);
#else
#error _LITTLE/_BIG ENDIAN type must be defined!
#endif
}

ulong
scsi_stoh_long(ulong ai)
{
	return (ntohl(ai));
}

ushort
scsi_stoh_short(ushort as)
{
	return (ntohs(as));
}

void
scsi_incmplmsg(struct scsi_device *devp, char *label, struct scsi_pkt *pktp)
{
	switch (pktp->pkt_reason) {
				/* cleanup already done: */
	case CMD_RESET:		/* SCSI bus reset destroyed command */
	case CMD_ABORTED:	/* Command tran aborted on request */
				/* the following may need cleanup: */
	case CMD_DMA_DERR:	/* dma direction error occurred */
	case CMD_UNX_BUS_FREE:	/* Unexpected Bus Free Phase occurred */
	case CMD_TIMEOUT:	/* Command timed out */
	case CMD_TRAN_ERR:	/* unspecified transport error */
	case CMD_NOMSGOUT:	/* Targ refused to go to Msg Out phase */
	case CMD_CMD_OVR:	/* Command Overrun */
	case CMD_DATA_OVR:	/* Data Overrun */

	case CMD_INCOMPLETE:	/* tran stopped with not normal state */
	case CMD_STS_OVR:	/* Status Overrun */
	case CMD_BADMSG:	/* Message not Command Complete */
	case CMD_XID_FAIL:	/* Extended Identify message rejected */
	case CMD_IDE_FAIL:	/* Initiator Detected Err msg rejected */
	case CMD_ABORT_FAIL:	/* Abort message rejected */
	case CMD_REJECT_FAIL:	/* Reject message rejected */
	case CMD_NOP_FAIL:	/* No Operation message rejected */
	case CMD_PER_FAIL:	/* Msg Parity Error message rejected */
	case CMD_BDR_FAIL:	/* Bus Device Reset message rejected */
	case CMD_ID_FAIL:	/* Identify message rejected */
		scsi_log(devp->sd_dev, label,  CE_WARN,
			"transport completed with %s\n",
			scsi_rname(pktp->pkt_reason));
		break;

	default:
		scsi_log(devp->sd_dev, label,  CE_WARN,
			"transport completed for unknown reason\n");
		break;
	}
}

void
scsi_inqfill(char *p, int l, char *s)
{
	register unsigned i = 0, c;

	if (!p)
		return;
	while (i++ < l) {
/* 		clean strings of non-printing chars			*/
		if ((c = *p++) < ' ' || c > 0176) {
			c = ' ';
		}
		*s++ = (char)c;
	}
	*s++ = 0;
}

int
scsi_exam_arq(opaque_t scdevp, struct scsi_pkt *pktp, int (*rqshdl)(),
	dev_info_t dev, char *name)
{
	register struct	scsi_arq_status *arqp;
	int 	amt;

	arqp = (struct scsi_arq_status *)SCBP(pktp);

	if (arqp->sts_rqpkt_status.sts_busy) {
		scsi_log(dev, name, CE_WARN, "Busy Status on REQUEST SENSE\n");
		return (QUE_COMMAND);
	}

	if (arqp->sts_rqpkt_status.sts_chk) {
		scsi_log(dev, name, CE_WARN,
			"Check Condition on REQUEST SENSE\n");
		return (QUE_COMMAND);
	}

	amt = SENSE_LENGTH - arqp->sts_rqpkt_resid;
	if ((arqp->sts_rqpkt_state & STATE_XFERRED_DATA) == 0 || amt == 0) {
		scsi_log(dev, name, CE_WARN,
			"Request Sense couldn't get sense data\n");
		return (QUE_COMMAND);
	}

	return (rqshdl(scdevp, pktp, &arqp->sts_sensedata, amt));
}
