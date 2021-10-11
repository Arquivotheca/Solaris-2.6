/*
 * Copyright (c) 1992-1994, Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sccd_audio.c	1.7	96/05/27 SMI"

#include <sys/scsi/scsi.h>
#include <sys/cdio.h>

#include <sys/dktp/objmgr.h>
#include <sys/dktp/tgcd.h>
#include <sys/dktp/tgpassthru.h>
#include <sys/dktp/cdtypes.h>

/*
 *	Object Management
 */
static opaque_t cdstd_create();
static opaque_t cdsony_create();

/*
 *	This is the loadable module wrapper
 */
#include <sys/modctl.h>

extern struct mod_ops mod_miscops;

static struct modlmisc modlmisc = {
	&mod_miscops,	/* Type of module */
	"SCSI CDROM Audio Objects"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

char _depends_on[] = "drv/objmgr";

int
_init(void)
{
	objmgr_ins_entry("sccd_std",  (opaque_t)cdstd_create, "audio");
	objmgr_ins_entry("sccd_sony", (opaque_t)cdsony_create, "audio");
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	if ((objmgr_del_entry("sccd_std")  == DDI_FAILURE) ||
	    (objmgr_del_entry("sccd_sony") == DDI_FAILURE))
		return (EBUSY);
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


/*
 * Local static data
 */
#ifdef	CDSONY_DEBUG
#define	DENT	0x0001
#define	DERR	0x0002
#define	DIO	0x0004
static	int	cdstd_debug = DENT|DERR|DIO;

#endif	/* CDSONY_DEBUG */

/*
 * Local Function Prototypes
 */
static opaque_t cd_create(opaque_t ops);

static int cdstd_pause_resume(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, u_char data);
static int cdstd_play_msf(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
static int cdstd_play_trkind(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
static int cdstd_volume_ctrl(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t  data, int flag);
static int cdstd_read_subchannel(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
static int cdstd_read_mode1(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
static int cdstd_read_tochdr(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
static int cdstd_read_tocentry(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);

static int cdstd_init(register struct cd_data *cddp, opaque_t tgpassthru_objp);
static int cdstd_free(struct tgcd_obj *tgcdobjp);
static int cdstd_identify(struct cd_data *cddp, struct scsi_inquiry *inqp);
static int cdstd_ioctl(struct cd_data *cddp, opaque_t cmdp, dev_t dev, int cmd,
	int arg, int flag);

struct 	tgcd_objops cdstd_ops = {
	cdstd_init,
	cdstd_free,
	cdstd_identify,
	cdstd_ioctl,
	0, 0
};

static opaque_t
cd_create(opaque_t ops)
{
	register struct	tgcd_obj *tgcdobjp;
	register struct	cd_data *cddp;

	tgcdobjp = kmem_zalloc((sizeof (*tgcdobjp) + sizeof (*cddp)), KM_SLEEP);
	cddp = (struct cd_data *)(tgcdobjp+1);
	tgcdobjp->cd_data = (opaque_t)cddp;
	tgcdobjp->cd_ops  = ops;

	return ((opaque_t)tgcdobjp);
}

static opaque_t
cdstd_create()
{
	return (cd_create((opaque_t)&cdstd_ops));
}

static int
cdstd_init(register struct cd_data *cddp, opaque_t tgpassthru_objp)
{
	cddp->cd_tgpt_objp = tgpassthru_objp;

	TGPASSTHRU_INIT(tgpassthru_objp);
	return (DDI_SUCCESS);
}

static int
cdstd_free(struct tgcd_obj *tgcdobjp)
{
	register struct cd_data *cddp;

	cddp = (struct cd_data *)tgcdobjp->cd_data;
	if (cddp->cd_tgpt_objp)
		TGPASSTHRU_FREE(cddp->cd_tgpt_objp);
	kmem_free(tgcdobjp, (sizeof (*tgcdobjp) + sizeof (*cddp)));
	return (0);
}

/*ARGSUSED*/
static int
cdstd_identify(struct cd_data *cddp, struct scsi_inquiry *inqp)
{
	return (DDI_SUCCESS);
}

static int
cdstd_ioctl(struct cd_data *cddp, opaque_t cmdp, dev_t dev, int cmd,
	int arg, int flag)
{
	switch (cmd) {
	case CDROMPAUSE:
		return (cdstd_pause_resume(cddp, cmdp, dev, (u_char)0));
	case CDROMRESUME:
		return (cdstd_pause_resume(cddp, cmdp, dev, (u_char)1));
	case CDROMPLAYMSF:
		return (cdstd_play_msf(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMPLAYTRKIND:
		return (cdstd_play_trkind(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMVOLCTRL:
		return (cdstd_volume_ctrl(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMSUBCHNL:
		return (cdstd_read_subchannel(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMREADMODE1:
		return (cdstd_read_mode1(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMREADTOCHDR:
		return (cdstd_read_tochdr(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMREADTOCENTRY:
		return (cdstd_read_tocentry(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMREADMODE2:
	default:
		return (ENOTTY);
	}
}

/*
 * This routine does a pause or resume to the cdrom player. Only affect
 * audio play operation.
 */
static int
cdstd_pause_resume(struct cd_data *cddp, opaque_t cmdp, dev_t dev, u_char data)
{
	register u_char	*cdb;
	register struct	uscsi_cmd *com = (struct uscsi_cmd *)cmdp;

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_PAUSE_RESUME;
	cdb[8] = data;
	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;

	return (TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE));
}

/*
 * This routine plays audio by msf
 */
static int
cdstd_play_msf(struct cd_data *cddp, opaque_t cmdp, dev_t dev, caddr_t data,
	int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_msf		msf_struct;
	register struct cdrom_msf	*msf = &msf_struct;

	if (ddi_copyin((caddr_t)data, (caddr_t)msf, sizeof (struct cdrom_msf),
	    flag))
		return (EFAULT);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_PLAYAUDIO_MSF;
	cdb[3] = msf->cdmsf_min0;
	cdb[4] = msf->cdmsf_sec0;
	cdb[5] = msf->cdmsf_frame0;
	cdb[6] = msf->cdmsf_min1;
	cdb[7] = msf->cdmsf_sec1;
	cdb[8] = msf->cdmsf_frame1;
	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;
	return (TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE));
}

/*
 * This routine plays audio by track/index
 */
static int
cdstd_play_trkind(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_ti			ti_struct;
	register struct cdrom_ti	*ti = &ti_struct;

	if (ddi_copyin((caddr_t)data, (caddr_t)ti, sizeof (struct cdrom_ti),
	    flag))
		return (EFAULT);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_PLAYAUDIO_TI;
	cdb[4] = ti->cdti_trk0;
	cdb[5] = ti->cdti_ind0;
	cdb[7] = ti->cdti_trk1;
	cdb[8] = ti->cdti_ind1;
	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;
	return (TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE));
}

/*
 * This routine control the audio output volume
 */
static int
cdstd_volume_ctrl(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t  data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_volctrl		volume;
	register struct cdrom_volctrl	*vol = &volume;
	caddr_t buffer;
	int	rtn;


	if (ddi_copyin((caddr_t)data, (caddr_t)vol,
		sizeof (struct cdrom_volctrl), flag))
		return (EFAULT);

	buffer = kmem_zalloc(20, KM_SLEEP);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_MODE_SELECT;
	cdb[1] = 0x10;
	cdb[4] = 20;

	/*
	 * fill in the input data. Set the output channel 0, 1 to
	 * output port 0, 1 respestively. Set output channel 2, 3 to
	 * mute. The function only adjust the output volume for channel
	 * 0 and 1.
	 */
	buffer[4] = 0xe;
	buffer[5] = 0xe;
	buffer[6] = 0x4;	/* set the immediate bit to 1 */
	buffer[12] = 0x01;
	buffer[13] = vol->channel0;
	buffer[14] = 0x02;
	buffer[15] = vol->channel1;

	com->uscsi_cdblen = CDB_GROUP0;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 20;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;

	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	kmem_free(buffer, 20);
	return (rtn);
}

static int
cdstd_read_subchannel(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct	cdrom_subchnl		subchanel;
	register struct	cdrom_subchnl	*subchnl = &subchanel;
	caddr_t buffer;
	int	rtn;

	if (ddi_copyin((caddr_t)data, (caddr_t)subchnl,
	    sizeof (struct cdrom_subchnl), flag))
		return (EFAULT);

	buffer = kmem_zalloc(16, KM_SLEEP);
	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_READ_SUBCHANNEL;
	cdb[1] = (subchnl->cdsc_format & CDROM_LBA) ? 0 : 0x02;
	/*
	 * set the Q bit in byte 2 to 1.
	 */
	cdb[2] = 0x40;
	/*
	 * This byte (byte 3) specifies the return data format. Proposed
	 * by Sony. To be added to SCSI-2 Rev 10b
	 * Setting it to one tells it to return time-data format
	 */
	cdb[3] = 0x01;
	cdb[8] = 0x10;

	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x10;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	if (rtn) {
		kmem_free(buffer, 16);
		return (rtn);
	}

	subchnl->cdsc_audiostatus = buffer[1];
	subchnl->cdsc_trk = buffer[6];
	subchnl->cdsc_ind = buffer[7];
	subchnl->cdsc_adr = buffer[5] & 0xF0;
	subchnl->cdsc_ctrl = buffer[5] & 0x0F;
	if (subchnl->cdsc_format & CDROM_LBA) {
		subchnl->cdsc_absaddr.lba =
		    ((u_char)buffer[8] << 24) +
		    ((u_char)buffer[9] << 16) +
		    ((u_char)buffer[10] << 8) +
		    ((u_char)buffer[11]);
		subchnl->cdsc_reladdr.lba =
		    ((u_char)buffer[12] << 24) +
		    ((u_char)buffer[13] << 16) +
		    ((u_char)buffer[14] << 8) +
		    ((u_char)buffer[15]);
	} else {
		subchnl->cdsc_absaddr.msf.minute = buffer[9];
		subchnl->cdsc_absaddr.msf.second = buffer[10];
		subchnl->cdsc_absaddr.msf.frame = buffer[11];
		subchnl->cdsc_reladdr.msf.minute = buffer[13];
		subchnl->cdsc_reladdr.msf.second = buffer[14];
		subchnl->cdsc_reladdr.msf.frame = buffer[15];
	}
	kmem_free(buffer, 16);
	if (ddi_copyout((caddr_t)subchnl, (caddr_t)data,
	    sizeof (struct cdrom_subchnl), flag))
		return (EFAULT);
	return (0);
}

static int
cdstd_read_mode1(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct	cdrom_read		mode1_struct;
	register struct	cdrom_read	*mode1 = &mode1_struct;

	if (ddi_copyin((caddr_t)data, (caddr_t)mode1,
	    sizeof (struct cdrom_read), flag))
		return (EFAULT);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_READ;
	cdb[1] = (u_char)((mode1->cdread_lba >> 16) & 0XFF);
	cdb[2] = (u_char)((mode1->cdread_lba >> 8) & 0xFF);
	cdb[3] = (u_char)(mode1->cdread_lba & 0xFF);
	cdb[4] = mode1->cdread_buflen >> 11;

	com->uscsi_cdblen = CDB_GROUP0;
	com->uscsi_bufaddr = mode1->cdread_bufaddr;
	com->uscsi_buflen = mode1->cdread_buflen;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	return (TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_USERSPACE));
}


static int
cdstd_read_tochdr(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_tochdr		header;
	register struct cdrom_tochdr	*hdr = &header;
	caddr_t buffer;
	int	rtn;

	buffer = kmem_zalloc(4, KM_SLEEP);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_READ_TOC;
	cdb[6] = 0x00;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 bytes.
	 */
	cdb[8] = 0x04;
	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x04;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	if (rtn) {
		kmem_free(buffer, 4);
		return (rtn);
	}

	hdr->cdth_trk0 = buffer[2];
	hdr->cdth_trk1 = buffer[3];
	kmem_free(buffer, 4);
	if (ddi_copyout((caddr_t)hdr, (caddr_t)data,
	    sizeof (struct cdrom_tochdr), flag))
		return (EFAULT);
	return (0);
}

/*
 * This routine read the toc of the disc and returns the information
 * of a particular track. The track number is specified by the ioctl
 * caller.
 */
static int
cdstd_read_tocentry(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_tocentry		toc_entry;
	register struct cdrom_tocentry	*entry = &toc_entry;
	caddr_t buffer;
	int	rtn;
	int	lba;

	if (ddi_copyin((caddr_t)data, (caddr_t)entry,
	    sizeof (struct cdrom_tocentry), flag))
		return (EFAULT);

	if (!(entry->cdte_format & (CDROM_LBA | CDROM_MSF))) {
		return (EINVAL);
	}

	buffer = kmem_zalloc(12, KM_SLEEP);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_READ_TOC;
	/* set the MSF bit of byte one */
	cdb[1] = (entry->cdte_format & CDROM_LBA) ? 0 : 2;
	cdb[6] = entry->cdte_track;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 + 8
	 * = 12 bytes, since we only need one entry.
	 */
	cdb[8] = 0x0C;
	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 0x0C;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	if (rtn) {
		kmem_free(buffer, 12);
		return (rtn);
	}

	entry->cdte_adr = (buffer[5] & 0xF0) >> 4;
	entry->cdte_ctrl = (buffer[5] & 0x0F);
	if (entry->cdte_format & CDROM_LBA) {
		entry->cdte_addr.lba =
		    ((u_char)buffer[8] << 24) +
		    ((u_char)buffer[9] << 16) +
		    ((u_char)buffer[10] << 8) +
		    ((u_char)buffer[11]);
	} else {
		entry->cdte_addr.msf.minute = buffer[9];
		entry->cdte_addr.msf.second = buffer[10];
		entry->cdte_addr.msf.frame = buffer[11];
	}

	/*
	 * Now do a readheader to determine which data mode it is in.
	 * ...If the track is a data track
	 */
	if ((entry->cdte_ctrl & CDROM_DATA_TRACK) &&
	    (entry->cdte_track != CDROM_LEADOUT)) {
		if (entry->cdte_format & CDROM_LBA) {
			lba = entry->cdte_addr.lba;
		} else {
			lba =
			    (((entry->cdte_addr.msf.minute * 60) +
			    (entry->cdte_addr.msf.second)) * 75) +
			    entry->cdte_addr.msf.frame;
		}
		bzero((caddr_t)cdb, CDB_GROUP1);
		cdb[0] = SCMD_READ_HEADER;
		cdb[2] = (u_char)((lba >> 24) & 0xFF);
		cdb[3] = (u_char)((lba >> 16) & 0xFF);
		cdb[4] = (u_char)((lba >> 8) & 0xFF);
		cdb[5] = (u_char)(lba & 0xFF);
		cdb[8] = 0x08;
		com->uscsi_buflen = 0x08;

		rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev,
					UIO_SYSSPACE);
		if (rtn) {
			kmem_free(buffer, 12);
			return (rtn);
		}
		entry->cdte_datamode = buffer[0];

	} else
		entry->cdte_datamode = (u_char) -1;

	kmem_free(buffer, 12);

	if (ddi_copyout((caddr_t)entry, data,
	    sizeof (struct cdrom_tocentry), flag))
		return (EFAULT);

	return (0);
}


/*
 * 	SONY SCSI CD support object
 */
static int cdsony_volume_ctrl(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t  data, int flag);

static int cdsony_ioctl(struct cd_data *cddp, opaque_t cmdp, dev_t dev, int cmd,
	int arg, int flag);
static int cdsony_identify(struct cd_data *cddp, struct scsi_inquiry *inqp);

#ifdef CDROMREADOFFSET
static int cdstd_read_sony_session_offset(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag);
#endif

struct 	tgcd_objops cdsony_ops = {
	cdstd_init,
	cdstd_free,
	cdsony_identify,
	cdsony_ioctl,
	0, 0
};

static opaque_t
cdsony_create()
{
	return (cd_create((opaque_t)&cdsony_ops));
}

static char *sony_vid = "SONY";

/*ARGSUSED*/
static int
cdsony_identify(struct cd_data *cddp, struct scsi_inquiry *inqp)
{
	if (strncmp(sony_vid, inqp->inq_vid, 4))
		return (DDI_FAILURE);
	return (DDI_SUCCESS);
}

static int
cdsony_ioctl(struct cd_data *cddp, opaque_t cmdp, dev_t dev, int cmd,
	int arg, int flag)
{
	switch (cmd) {
	case CDROMPAUSE:
		return (cdstd_pause_resume(cddp, cmdp, dev, (u_char)0));
	case CDROMRESUME:
		return (cdstd_pause_resume(cddp, cmdp, dev, (u_char)1));
	case CDROMPLAYMSF:
		return (cdstd_play_msf(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMPLAYTRKIND:
		return (cdstd_play_trkind(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMVOLCTRL:
		return (cdsony_volume_ctrl(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMSUBCHNL:
		return (cdstd_read_subchannel(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMREADMODE1:
		return (cdstd_read_mode1(cddp, cmdp, dev, (caddr_t)arg, flag));
	case CDROMREADTOCHDR:
		return (cdstd_read_tochdr(cddp, cmdp, dev, (caddr_t)arg,
			flag));
	case CDROMREADTOCENTRY:
		return (cdstd_read_tocentry(cddp, cmdp, dev, (caddr_t)arg,
			flag));
#ifdef CDROMREADOFFSET
	case CDROMREADOFFSET:
		return (cdstd_read_sony_session_offset(cddp, cmdp, dev,
			(caddr_t)arg, flag));
#endif
	case CDROMREADMODE2:
	default:
		return (ENOTTY);
	}
}

/*
 * This routine control the audio output volume
 */
static int
cdsony_volume_ctrl(struct cd_data *cddp, opaque_t cmdp, dev_t dev,
	caddr_t  data, int flag)
{
	register u_char			*cdb;
	register struct	uscsi_cmd 	*com = (struct uscsi_cmd *)cmdp;
	struct cdrom_volctrl		volume;
	register struct cdrom_volctrl	*vol = &volume;
	caddr_t buffer;
	int	rtn;


	if (ddi_copyin((caddr_t)data, (caddr_t)vol,
		sizeof (struct cdrom_volctrl), flag))
		return (EFAULT);

	buffer = kmem_zalloc(18, KM_SLEEP);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = 0xc9;	/* vendor unique command */
	cdb[8] = 0x12;

	/*
	 * fill in the input data. Set the output channel 0, 1 to
	 * output port 0, 1 respestively. Set output channel 2, 3 to
	 * mute. The function only adjust the output volume for channel
	 * 0 and 1.
	 */
	buffer[10] = 0x01;
	buffer[11] = vol->channel0;
	buffer[12] = 0x02;
	buffer[13] = vol->channel1;

	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = 18;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT;
	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	kmem_free(buffer, 18);
	return (rtn);
}

#ifdef CDROMREADOFFSET

#define	SONY_SESSION_OFFSET_LEN 12
#define	SONY_SESSION_OFFSET_KEY 0x40
#define	SONY_SESSION_OFFSET_VALID	0x0a

static int
cdstd_read_sony_session_offset(struct cd_data *cddp, opaque_t cmdp,
	dev_t dev, caddr_t data, int flag)
{
	register u_char			*cdb;
	register struct uscsi_cmd	*com = (struct uscsi_cmd *)cmdp;

	caddr_t buffer;
	int	rtn;
	int	session_offset;

	buffer = kmem_zalloc((size_t)SONY_SESSION_OFFSET_LEN, KM_SLEEP);

	cdb = (u_char *)com->uscsi_cdb;
	cdb[0] = SCMD_READ_TOC;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 bytes.
	 */
	cdb[8] = SONY_SESSION_OFFSET_LEN;
	cdb[9] = SONY_SESSION_OFFSET_KEY;

	com->uscsi_cdblen = CDB_GROUP1;
	com->uscsi_bufaddr = buffer;
	com->uscsi_buflen = SONY_SESSION_OFFSET_LEN;
	com->uscsi_flags = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_READ;

	rtn = TGPASSTHRU_TRANSPORT(TGPTOBJP(cddp), cmdp, dev, UIO_SYSSPACE);
	if (rtn) {
		kmem_free(buffer, (size_t)SONY_SESSION_OFFSET_LEN);
		return (rtn);
	}

	session_offset = 0;
	if (buffer[1] == SONY_SESSION_OFFSET_VALID) {
		session_offset = ((u_char) buffer[8] << 24) +
		    ((u_char) buffer[9] << 16) +
		    ((u_char) buffer[10] << 8) +
		    ((u_char) buffer[11]);
		session_offset = session_offset / 4;
	}
	kmem_free(buffer, SONY_SESSION_OFFSET_LEN);

	if (ddi_copyout((caddr_t)&session_offset, (caddr_t)data,
	    sizeof (int), flag))
		return (EFAULT);

	return (rtn);
}
#endif
