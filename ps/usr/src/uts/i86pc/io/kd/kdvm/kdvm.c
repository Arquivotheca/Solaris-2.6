/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kdvm.c	1.17	96/07/30 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/user.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "sys/cmn_err.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/uio.h"
#include "sys/kd.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/kd.h"
#include "sys/ws/chan.h"
#include "sys/ws/ws.h"
#include "sys/vid.h"
#include "sys/vdc.h"
#include "sys/cred.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "sys/mman.h"
#include "sys/ddi.h"
#include "sys/tss.h"
#include "sys/sunddi.h"
#include "sys/v86.h"

#define	CH_MAPMX	10

extern wstation_t	Kdws;
extern struct vdc_info	Vdc;

extern int		kdvmemcnt;
extern struct kd_range	kdvmemtab[];
extern unchar		kdkb_krr_dt;

extern channel_t	*ws_getchan(),
			*ws_activechan();


struct kd_range	kd_basevmem[MKDIOADDR] = {
	{ 0xa0000, 0xc0000 },
#ifdef	EVC
	{ (unsigned long)0xD0000000, (unsigned long)0xD00FFFFF },
#endif	/* EVC */
};


#ifdef EVGA
extern int		evga_inited;
extern int		new_mode_is_evga;
extern int		evga_mode;
extern int		evga_num_disp;
extern struct at_disp_info disp_info[];
extern unsigned long	evga_type;
#endif /* EVGA */

/* function prototypes */
int	kdvm_modeioctl(channel_t *, int);
int	kdvm_xenixioctl(queue_t *, mblk_t *, int, int, int *, int *, int *, int *);
int	kdvm_xenixdoio(queue_t *, mblk_t *, int, int, int *);
void	kdvm_xenixdoio_bottom(queue_t *, mblk_t *);
void	ws_pio_romfont_middle(queue_t *, mblk_t *);
void	ws_pio_romfont_bottom(queue_t *, mblk_t *);
int	kdvm_xenixmap(queue_t *, mblk_t *, channel_t *, struct _kthread *, int cmd, int *);

#ifdef EVGA
int evga_modeioctl(channel_t *, int);
#endif


int kdvm_mapdisp(queue_t *, mblk_t *);
int kdvm_map(struct _kthread *, channel_t *, struct map_info *, struct kd_memloc *, cred_t *);
int kdvm_unmapdisp(struct _kthread *, channel_t *, struct map_info *);

/* in external source files */
extern int kdvt_ioctl(queue_t *, mblk_t *, struct iocblk *, channel_t *, int, int, int *);
extern int kdkb_mktone(queue_t *, mblk_t *, struct iocblk *, int, caddr_t, int *);


/*
 *	Called from kdmioctlmsg in ../kd/kdstr.c to handle various ioctls.
 */

kdvm_ioctl(
queue_t	*qp,
mblk_t	*mp,
int	*hit,		/* set to 1 if ioctl is handled here */
int	*rvalp,		/* ptr to return value */
int	*copyflag,	/* if 1, rvalp needs to be copied to user-space arg */
int	*reply)		/* if 1, kdmioctlmsg should reply upstream */
{
	termstate_t	*tsp;
	vidstate_t	*vp;
	struct iocblk	*iocp;
	int		indx, cnt, rv = 0;
	int		cmd;
	int		arg;
	channel_t	*chp;

	chp = (channel_t *)qp->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;
	tsp = &chp->ch_tstate;
	vp = &chp->ch_vstate;

	*reply = 1;	/* assume a reply should be sent unless changed below */

	arg = *(int *)mp->b_cont->b_rptr;

	switch (cmd) {

/* Enhanced Application Compatibility Support */

	case CONS_BLANKTIME:
		if (arg < 0)
			rv = EINVAL;
		break;

	case CONSADP:
		if ((chp = ws_activechan(&Kdws)) == NULL) {
			*rvalp = rv = -1;
		} else {
			rv = 0;
			*rvalp = chp->ch_id;
		}

		break;

/* End Enhanced Application Compatibility Support */

	case KDSBORDER:	/* set border in ega color text mode */
		rv = kdv_sborder(chp, arg);
		break;

	case KDSETRAD:	/* set keyboard key-repeat rate/delay time */
		kdkb_krr_dt = (unchar) arg;
		kdkb_cmd(TYPE_WARN, FROM_DRIVER);
		rv = 0;
		break;

	case KDSCROLL:		/* For IUS binary compatibility */
		/* this is supposed to enable/disable hardware scrolling */
		rv = 0;
		break;

	case KDDISPTYPE:	/* get display information to user */
		/* this function handles the copyout itself */
		rv = kdv_disptype(qp, mp, iocp, chp, arg, copyflag);
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;

	case KDVDCTYPE:		/* return VDC controller/display information */
		/*XXX this function isn't in IUS XXX*/
		/* this function handles the copyout itself */
		rv = vdc_disptype(qp, mp, iocp, chp, arg, copyflag);
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;

	case KIOCSOUND:
		if (chp == ws_activechan(&Kdws))
			kdkb_sound(arg);
		rv = 0;
		break;

	case KDSETMODE:
		/* Here mode pertains to graphics or text. */
		switch ((int)arg) {
		case KD_TEXT0:
		case KD_TEXT1:
			if (!CHNFLAG(chp, CHN_XMAP) && CHNFLAG(chp, CHN_UMAP)) {
				rv = EIO;
				break;
			}
			if (chp->ch_dmode == arg)
				break;
			chp->ch_dmode = (unchar)arg;
			if (arg == KD_TEXT0)
				kdv_textmode(chp);
			else
				kdv_text1(chp);
			break;
		case KD_GRAPHICS:
			if (chp->ch_dmode == KD_GRAPHICS)
				break;
			kdv_scrxfer(chp, KD_SCRTOBUF);
			chp->ch_dmode = KD_GRAPHICS;
			/*
			 * If start address has changed, we must re-zero
			 * the screen buffer so as to not confuse VP/ix.
			 */
			if (tsp->t_origin) {
				tsp->t_cursor -= tsp->t_origin;
				tsp->t_origin = 0;
				kdsetbase(chp, tsp);
				kdsetcursor(chp, tsp);
				kdv_scrxfer(chp, KD_BUFTOSCR);
			}
		}
		break;

	case KDGETMODE:		/* get current mode - KD_TEXT, KD_GRAPHICS */
		*rvalp = chp->ch_dmode;
		break;

	case KDMKTONE:		/* turn on the speaker for a bit */
		if (chp == ws_activechan(&Kdws)) {
			rv = kdkb_mktone(qp, mp, iocp, arg, (caddr_t)0, reply);
		}
		break;

	case KDMAPDISP:
	    {
		int arg;
		struct v86blk *v86p;

		/* The streams head inserts a "v86blk" structure, allowing a
		 * streams driver to access the user process context.
		 */
		if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL) {
			/* the v86blk is missing - bail out now */
			rv = EINVAL;
			break;
		}

		v86p = (struct v86blk *) mp->b_cont->b_rptr;
		arg = *(int *)mp->b_cont->b_cont->b_rptr;

		/* save the process pointer from the v86p into the
		 * workstation's "private" field so that it can be used
		 * later.
		 */
		/* arrange to copy in the user's "struct kd_memloc" struct,
		 * passing the process kernel thread pointer from the
		 * v86p structure for use in the bottom half of this ioctl
		 * handling code.
		 */
		ws_copyin (qp, mp, (caddr_t)arg, sizeof (struct kd_memloc),
		    (mblk_t *)v86p->v86b_t);
		*reply = 0;	/* upper level doesn't need to reply */

		/* when the M_IOCDATA response to our copyin request comes,
		 * kdmiocdatamsg() will call kdvm_mapdisp() to do the real
		 * work.
		 */

		break;
	    }

	case KDUNMAPDISP:
	    {
		struct proc *procp;
		struct v86blk *v86p;

		/* The streams head inserts a "v86blk" structure, allowing a
		 * streams driver to access the user process context.
		 */
		if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL) {
			/* the v86blk is missing - bail out now */
			rv = EINVAL;
			break;
		}

		v86p = (struct v86blk *) mp->b_cont->b_rptr;

		/*
		 *	Determine the current process.
		 */
		procp = ttoproc(v86p->v86b_t);

		if (Kdws.w_map.m_procp != procp) {
			/* this isn't the process that has it mapped */
			rv = EACCES;
			break;
		}

		if (!kdvm_unmapdisp(v86p->v86b_t, chp, &Kdws.w_map)) {
			rv = EFAULT;
			break;
		}
		break;
	    }

	case KDENABIO:		/* enable ins and outs to adapter ports */
	case KDDISABIO:		/* disable ins and outs to adapter ports */
	    {
		/* struct v86blk *v86p; */

		/* The streams head inserts a "v86blk" structure, allowing a
		 * streams driver to access the user process context.
		 */
		if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL) {
			/* the v86blk is missing - bail out now */
			rv = EINVAL;
			break;
		}

		/* v86p = (struct v86blk *) mp->b_cont->b_rptr; */

		/*
		 * Pass the process pointer from the v86p so that it can be
		 * used by the enableio/disableio functions.
		 */
		if (cmd == KDENABIO)	/* enable ins and outs */
			rv = enableio(chp->ch_vstate.v_ioaddrs);
		else			/* KDDISABIO - disable ins and outs */
			rv = disableio(chp->ch_vstate.v_ioaddrs);
		break;
	    }


	case KDADDIO:		/* add a port to the list of allowed ports */
		/* check for permission using iocblk-supplied credentials */
		if ((rv = drv_priv(iocp->ioc_cr)) != 0)
			return (rv);
		if ((ushort)arg > MAXTSSIOADDR)
			return (ENXIO);
		for (indx = 0; indx < MKDIOADDR; indx++)
			if (!vp->v_ioaddrs[indx]) {
				vp->v_ioaddrs[indx] = (ushort)arg;
				break;
			}

		if (indx == MKDIOADDR)
			return (EIO);
		break;

	case KDDELIO:	/* remove a port from the list of allowed ports */
		/* check for permission using iocblk-supplied credentials */
		if ((rv = drv_priv(iocp->ioc_cr)) != 0)
			return (rv);
		for (indx = 0; indx < MKDIOADDR; indx++) {
			if (vp->v_ioaddrs[indx] != (ushort)arg)
				continue;
			for (cnt = indx; cnt < (MKDIOADDR - 1); cnt++)
				vp->v_ioaddrs[cnt] = vp->v_ioaddrs[cnt + 1];
			vp->v_ioaddrs[cnt] = (ushort)0;
			break;
		}
		break;

	case WS_PIO_ROMFONT:
		ws_copyin (qp, mp, (caddr_t)arg, sizeof(unsigned int),
		    (mblk_t *)arg);
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;

#ifdef EVGA
	case KDEVGA:
		if (! DTYPE(Kdws, KD_VGA)) {
			/*
			 * The card has already been successfully identified;
			 * it can't be evga.
			 */
			rv = EINVAL;
		} else {
			/* Card was a vanilla-type VGA, could
			 * be an evga.
			 */
			ws_copyin (qp, mp, (caddr_t)arg,
			   sizeof(unsigned long), (mblk_t *)NULL);
			/*
			 * evga_init will be called later when the data gets
			 * copied in
			 */
			*copyflag = 1;
		}
		break;
#endif /* EVGA */

	default:
		/* Mode change hits the default case. Cmd is new mode
		 * or'ed with MODESWITCH.
		 */
		switch (cmd & 0xffffff00) {
		case VTIOC:		/* VT ioctl */
			rv = kdvt_ioctl(qp, mp, iocp, chp, cmd, arg, reply);
			break;

		case USL_OLD_MODESWITCH: /* old USL mode switch ioctl */
		case SCO_OLD_MODESWITCH: /* old SCO mode switch ioctl */
			cmd = (cmd & ~IOCTYPE) | MODESWITCH;
			/*FALLTHROUGH*/
		case MODESWITCH:	/* UNIX mode switch ioctl */
			rv = kdvm_modeioctl(chp, cmd);
			break;
#ifdef EVGA
		case EVGAIOC:		/* evga mode switch ioctl */
			if (evga_inited)
				rv = evga_modeioctl(chp, cmd);
		    	else
				rv = ENXIO;
			break;
#endif
		default:
			rv = kdvm_xenixioctl(qp, mp, cmd, arg, rvalp, copyflag, hit, reply);
			if (*hit == 1)		/* Xenix ioctl */
				break;
		}
		break;
	}
	return (rv);
}


/*
 *	Handle display mode switch ioctl commands.
 *	Called from kdvm_ioctl() above.
 */

int
kdvm_modeioctl(channel_t *chp, int cmd)
{
	int	rv = 0;
	channel_t *achp;
	struct proc *procp;
#ifdef EVGA
	int generic_mode;
#endif

	cmd &= ~O_MODESWITCH;
	cmd |= MODESWITCH;

#ifdef EVGA
	if (evga_inited) {
		generic_mode = 0;
	}
#endif

	/*
	 * If kd has been initialized for evga, then DTYPE is still KD_VGA so
	 * modes that require only DTYPE of KD_VGA should succeed. Requests
	 * for modes that require other DTYPEs or have additional requirements
	 * may fail.
	 */

	switch (cmd) {
	/* have to check for Xenix modes */
	case SW_ATT640:
		if (!(VTYPE(V750) && VSWITCH(ATTDISPLAY)) && !VTYPE(V600 | CAS2))
			rv = ENXIO;
		break;
	case SW_ENHB80x43:
	case SW_ENHC80x43:
		/*XXX can VGA handle this, or just EGA? XXX*/
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			rv = ENXIO;
		cmd -= OFFSET_80x43;
		break;
	case SW_VGAB40x25:
	case SW_VGAB80x25:
	case SW_VGAC40x25:
	case SW_VGAC80x25:
	case SW_VGAMONO80x25:
	case SW_VGA640x480C:
	case SW_VGA640x480E:
	case SW_VGA320x200:
	case SW_VGA_B132x25:
	case SW_VGA_C132x25:
	case SW_VGA_B132x43:
	case SW_VGA_C132x43:
		if (!DTYPE(Kdws, KD_VGA))
			rv = ENXIO;
		break;
	case SW_MCAMODE:
		if (!(DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)))
			rv = ENXIO;
		break;
	case SW_VDC640x400V:
		if (!VTYPE(V600))
			rv = ENXIO;
		break;
	case SW_VDC800x600E:
		if (!VTYPE(V600 | CAS2) ||
		    (Vdc.v_info.dsply != KD_MULTI_M &&
		     Vdc.v_info.dsply != KD_MULTI_C)) {
			/* not VDC-600 or CAS-2 ? */
#ifdef EVGA
			if (evga_inited) {
				generic_mode = SW_GEN_800x600;
			} else {
#endif
				rv = ENXIO;
#ifdef EVGA
			}
#endif
		}
		break;

	case SW_CG320_D:
	case SW_CG640_E:
	case SW_ENH_MONOAPA2:
	case SW_VGAMONOAPA:
	case SW_VGA_CG640:
	case SW_ENH_CG640:
	case SW_ENHB40x25:
	case SW_ENHC40x25:
	case SW_ENHB80x25:
	case SW_ENHC80x25:
	case SW_EGAMONO80x25:
		if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)))
			rv = ENXIO;
		break;
	case SW_CG640x350:
	case SW_EGAMONOAPA:
		if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)))
			rv = ENXIO;
		/* for all VGA and the VDC 750, switch from F to F*
		 * since we know we have enough memory
		 * For other EGA cards that we can't identify, keep your
		 * fingers crossed and hope that mode F works
		 */
		if (VTYPE(V750) || DTYPE(Kdws, KD_VGA))
			cmd += 2;
		break;
	case SW_B40x25:
	case SW_C40x25:
	case SW_B80x25:
	case SW_C80x25:
	case SW_BG320:
	case SW_CG320:
	case SW_BG640:
		if (DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES))
			rv = ENXIO;
		break;
#ifdef	EVC
	case SW_EVC1024x768E:
		if (!VTYPE(VEVC) ||
		    (Vdc.v_info.dsply != KD_MULTI_M &&
		     Vdc.v_info.dsply != KD_MULTI_C)) {
			/* EVC-1 with hi-res monitor only */
#ifdef EVGA
			if (evga_inited) {
				generic_mode = SW_GEN_1024x768;
			} else
#endif
				rv = ENXIO;
		}
		break;
	case SW_EVC1024x768D:
		if (!VTYPE(VEVC) ||
		    (Vdc.v_info.dsply != KD_MULTI_M &&
		     Vdc.v_info.dsply != KD_MULTI_C))
			rv = ENXIO;	/* EVC-1 with hi-res monitor only */
		break;
	case SW_EVC640x480V:
		if (!VTYPE(VEVC))
			rv = ENXIO;
		break;
#endif	/* EVC */

#ifdef  EVGA
	/* temporary kludge for X server */

	case TEMPEVC1024x768E:
		if (evga_inited)
			generic_mode = SW_GEN_1024x768;
		else
			rv = ENXIO;
		break;
#endif	/* EVGA */

	default:
		rv = ENXIO;
		break;
	}

#ifdef EVGA
	if (evga_inited && generic_mode) {
		rv = evga_modeioctl(chp, generic_mode);
		return (rv);
	}
#endif	/* EVGA */

	if (!rv) {
		achp = ws_activechan(&Kdws);
		if (!achp) {
			cmn_err(CE_WARN,
			    "kdvm_modeioctl: Could not find active channel");
			return (ENXIO);
		}
		ws_mapavail(achp, &Kdws.w_map);

		/*XXX this isn't good enough -- this function is called by
		 *XXX the STREAMS ioctl code, and drv_getparm uses "curthread",
		 *XXX which isn't guaranteed to be valid!
		 */
		drv_getparm (UPROCP, (unsigned long *)&procp);

		if ((Kdws.w_map.m_procp != procp) &&
		    (!CHNFLAG(chp, CHN_XMAP) && CHNFLAG(chp, CHN_UMAP)))
			return (EIO);

		if (cmd == SW_MCAMODE) {  /* Use bogus mode to force reset */
			chp->ch_vstate.v_dvmode = 0xff;
			kdv_setmode(chp, DM_EGAMONO80x25);
		} else
			kdv_setmode(chp, (unchar)(cmd & KDMODEMASK));
	}
	return (rv);
}


#ifdef EVGA
/*
 *	Handle EVGA display mode switching.
 *	Called from kdvm_ioctl() above.
 */
int
evga_modeioctl(channel_t *chp, int cmd)
{
	int	rv = 0;
	int	i, gen_mode;
	int	color, x, y;
	int	newmode;
	channel_t *achp;
	struct proc *procp;
	struct at_disp_info *disp;

	gen_mode = (cmd & EVGAMODEMASK);

	color = 16;	/* for most modes */

	switch (gen_mode) {

	case GEN_640x350:
		x = 640;
		y = 350;
		break;
	case GEN_640x480:
		x = 640;
		y = 480;
		break;
	case GEN_720x540:
		x = 720;
		y = 540;
		break;
	case GEN_800x560:
		x = 800;
		y = 560;
		break;
	case GEN_800x600:
		x = 800;
		y = 600;
		break;
	case GEN_960x720:
		x = 960;
		y = 720;
		break;
	case GEN_1024x768:
	case GEN_1024x768x2:
	case GEN_1024x768x4:
		x = 1024;
		y = 768;
		switch (gen_mode) {
		case GEN_1024x768x2:
			color = 2;
			break;
		case GEN_1024x768x4:
			color = 4;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	/* Look for a match in disp_info, given the board type	*/
	/* present, the resolution requested, and the number of */
	/* (implicit) colors requested.				*/

	for (i = 0, disp = disp_info; i < evga_num_disp; i++, disp++) {
		if (evga_type == disp->type && x == disp->xpix &&
		    y == disp->ypix && color == disp->colors) {
			/* Found a match */
			break;
		}
	}

	if (i >= evga_num_disp)		/* failure */
		rv = ENXIO;

	if (!rv) {
		achp = ws_activechan(&Kdws);
		if (!achp) {
			cmn_err(CE_WARN,
			    "evga_modeioctl: Could not find active channel");
			return (ENXIO);
		}
		ws_mapavail(achp, &Kdws.w_map);

		/*XXX this isn't good enough -- this function is called by
		 *XXX the STREAMS ioctl code, and drv_getparm uses "curthread",
		 *XXX which isn't guaranteed to be valid!
		 */
		drv_getparm(UPROCP, (unsigned long *)&procp);

		if ((Kdws.w_map.m_procp != procp) &&
		    (!CHNFLAG(chp, CHN_XMAP) && CHNFLAG(chp, CHN_UMAP))) {
			return (EIO);
		}

		/* Convert relative offset (from beginning of evga modes)
		 * to absolute offset into kd_modeinfo.
		 */
		newmode = i + ENDNONEVGAMODE + 1;

		kdv_setmode(chp, (unchar)newmode);
	}
	return (rv);
}
#endif /* EVGA */


/*
 *	Called from kdvm_ioctl() to handle the XENIX-flavoured ioctl commands.
 */

int
kdvm_xenixioctl(queue_t *qp, mblk_t *mp, int cmd, int arg, int *rvalp, int *copyout, int *hit, int *reply)
{
	struct iocblk	*iocp;
	channel_t	*chp;
	mblk_t		*tmp_buf;
	int		tmp, rv;
	int		error = 0;

	chp = (channel_t *)qp->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;

	switch (cmd) {
	case GIO_COLOR:
		if (kdv_colordisp())		/* display is color */
			*rvalp = 0;		/* 0 means color */
		else
			*rvalp = 1;		/* 1 means monochrome */
		*copyout = 0;	/* "rvalp" should be ioctl return value */
		break;

	case CONS_CURRENT:
		*rvalp = kdv_xenixctlr();
		*copyout = 0;	/* "rvalp" should be ioctl return value */
		break;

	case MCA_GET:
	case CGA_GET:
	case EGA_GET:
	case VGA_GET:
	case CONS_GET:
		/* get the current video mode and fill in rvalp */
		if ((error = kdv_xenixmode(chp, cmd, rvalp)) == 0)
			*copyout = 0; /* "rvalp" should be ioctl return value */
		break;

	case MAPMONO:
	case MAPCGA:
	case MAPEGA:
	case MAPVGA:
	case MAPSPECIAL:
	case MAPCONS:
	    {
		struct v86blk *v86p;

		/* The streams head inserts a "v86blk" structure, allowing a
		 * streams driver to access the user process context.
		 */
		if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL) {
			/* the v86blk is missing - bail out now */
			rv = EINVAL;
			break;
		}

		v86p = (struct v86blk *) mp->b_cont->b_rptr;

		/*
		 *	Call kdvm_xenixmap, passing the thread pointer
		 *	that was passed in the v86blk.
		 */
		if ((error = kdvm_xenixmap(qp, mp, chp, v86p->v86b_t, cmd, rvalp)) == 0)
			*copyout = 0; /* "rvalp" should be ioctl return value */
		break;
	    }

	case MCAIO:
	case CGAIO:
	case EGAIO:
	case VGAIO:
	case CONSIO:
		error = kdvm_xenixdoio(qp, mp, cmd, arg, reply);
		break;

	case SWAPMONO:
		if (DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES))
			break;
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			error = EINVAL;
			break;
		}
		switch (chp->ch_vstate.v_cvmode) {
		case DM_EGAMONO80x25:
		case DM_EGAMONOAPA:
		case DM_ENHMONOAPA2:
			break;
		default:
			error = EINVAL;
		}
		break;
	case SWAPCGA:
		if (DTYPE(Kdws, KD_CGA))
			break;
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			error = EINVAL;
			break;
		}
		switch (chp->ch_vstate.v_cvmode) {
		case DM_B40x25:
		case DM_C40x25:
		case DM_B80x25:
		case DM_C80x25:
		case DM_BG320:
		case DM_CG320:
		case DM_BG640:
		case DM_CG320_D:
		case DM_CG640_E:
			break;
		default:
			error = EINVAL;
		}
		break;

	case SWAPEGA:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			error = EINVAL;
			break;
		}
		/* fail for any VGA mode or non-standard EGA mode */
		if (chp->ch_vstate.v_cvmode >= DM_VGA_C40x25)
			error = EINVAL;
		break;

	case SWAPVGA:
		if (!DTYPE(Kdws, KD_VGA)) {
			error = EINVAL;
			break;
		}
		/* fail for these non-standard VGA modes */
		switch (chp->ch_vstate.v_cvmode) {
		case DM_ENH_CGA:
		case DM_ATT_640:
		case DM_VDC800x600E:
		case DM_VDC640x400V:
			error = EINVAL;
			break;
		default:
			break;
		}
		break;

	case PIO_FONT8x8:
		tmp = FONT8x8;
		goto piofont;

	case PIO_FONT8x14:
		tmp = FONT8x14;
		goto piofont;

	case PIO_FONT8x16:
		tmp = FONT8x16;
piofont:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		error = kdv_setxenixfont(qp, mp, chp, tmp, arg, reply);
		break;

	case GIO_FONT8x8:
		tmp = FONT8x8;
		goto giofont;

	case GIO_FONT8x14:
		tmp = FONT8x14;
		goto giofont;

	case GIO_FONT8x16:
		tmp = FONT8x16;
giofont:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		error = kdv_getxenixfont(qp, mp, chp, tmp, arg, reply);
		break;

	case KDDISPINFO: {
		struct kd_dispinfo dinfo;

		switch (Kdws.w_vstate.v_type) {

		case KD_MONO:
		case KD_HERCULES:
			dinfo.vaddr = (char *) MONO_BASE;
			dinfo.physaddr = (paddr_t) MONO_BASE;
			dinfo.size = MONO_SIZE;
			break;
		case KD_CGA:
			dinfo.vaddr = (char *) COLOR_BASE;
			dinfo.physaddr = (paddr_t) COLOR_BASE;
			dinfo.size = COLOR_SIZE;
			break;
		case KD_VGA:
		case KD_EGA:
			/*
			 *  Assume as we do for MONOAPA2 that the EGA card
			 *  has 128K of RAM. Still have fingers crossed.
			 *  Not an issue for VGA -- we always have 128K.
			 */
			dinfo.vaddr = (char *) EGA_BASE;
			dinfo.physaddr = (paddr_t) EGA_BASE;
			dinfo.size = EGA_LGSIZE;
			break;
		default:
			return (EINVAL);
		}

		if (!(tmp_buf = allocb(sizeof (struct kd_dispinfo), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to KDDISPINFO");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(struct kd_dispinfo *)tmp_buf->b_rptr = dinfo; /* structure copy */
		tmp_buf->b_wptr += sizeof (struct kd_dispinfo);
		ws_copyout(qp, mp, tmp_buf, sizeof (struct kd_dispinfo));
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;
	}

	case CONS_GETINFO: {
		struct vid_info vinfo;
		termstate_t *tsp;

		tsp = &chp->ch_tstate;
		vinfo.size = sizeof (struct vid_info);
		vinfo.m_num = chp->ch_id;
		vinfo.mv_row = tsp->t_row;
		vinfo.mv_col = tsp->t_col;
		vinfo.mv_rsz = tsp->t_rows;
		vinfo.mv_csz = tsp->t_cols;
		vinfo.mv_norm.fore = tsp->t_curattr & 0x07;
		vinfo.mv_norm.back = (unchar)((int)(tsp->t_curattr & 0x70) >> 4);
		vinfo.mv_rev.fore = 0; /* reverse always black on white */
		vinfo.mv_rev.back = 7;
		vinfo.mv_grfc.fore = 7; /* match graphics with background info */
		vinfo.mv_grfc.back = 0;
		vinfo.mv_ovscan =  chp->ch_vstate.v_border;
		vinfo.mk_keylock = chp->ch_kbstate.kb_state / CAPS_LOCK;
		if (!(tmp_buf = allocb(sizeof (struct vid_info), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to CONS_GETINFO");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(struct vid_info *)tmp_buf->b_rptr = vinfo; /* structure copy */
		tmp_buf->b_wptr += sizeof (struct vid_info);
		ws_copyout(qp, mp, tmp_buf, sizeof (struct vid_info));
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;
	}

	case CONS_6845INFO: {

		struct m6845_info minfo;

		minfo.size = sizeof (struct m6845_info);
		minfo.cursor_type = chp->ch_tstate.t_curtyp;
		minfo.screen_top = chp->ch_tstate.t_origin;
		if (!(tmp_buf = allocb(sizeof (struct m6845_info), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmtmp_buf: can't get msg for reply to CONS_6845INFO");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(struct m6845_info *)tmp_buf->b_rptr = minfo; /* structure copy */
		tmp_buf->b_wptr += sizeof (struct m6845_info);
		ws_copyout(qp, mp, tmp_buf, sizeof (struct m6845_info));
		*reply = 0;	/* we have handled it - upper doesn't need to reply */
		break;
	}

	case EGA_IOPRIVL:	/* SCO - allow access to standard EGA ports */
		if (!DTYPE(Kdws, KD_EGA))
			return (EINVAL);
		goto check;

	case VGA_IOPRIVL:	/* SCO - allow access to standard VGA ports */
		if (!DTYPE(Kdws, KD_VGA))
			return (EINVAL);
check:
		/* check for permission using iocblk-supplied credentials */
		if ((rv = drv_priv(iocp->ioc_cr)) != 0)
			return (rv);		/* not super-user */

		/* verify that the current list of ports is allowable for
		 * this ioctl -- this won't allow the ioctl if additional
		 * ports have been added to the list (using KDADDIO)
		 */
		if (checkio(chp->ch_vstate.v_ioaddrs) == 1) {
			/* struct v86blk *v86p; */

			/* The streams head inserts a "v86blk" structure,
			 * allowing a streams driver to access the user
			 * process context.
			 */
			if (mp->b_cont == NULL || mp->b_cont->b_cont == NULL) {
				/* the v86blk is missing - bail out now */
				rv = EINVAL;
				break;
			}

			/* v86p = (struct v86blk *) mp->b_cont->b_rptr; */

			/* access is okay - enable the ports, passing the
			 * thread pointer.
			 */
			rv = enableio(chp->ch_vstate.v_ioaddrs);
		} else
			rv = EINVAL;
		*copyout = 0;
		break;

	case PGAIO:
	case PGAMODE:
	case MAPPGA:
	case PGA_GET:
	case SWAPPGA:
	case CGAMODE:
	case EGAMODE:
	case VGAMODE:
	case MCAMODE:
#ifdef DONT_INCLUDE
	case KIOCDOSMODE:
#endif
	case KIOCNONDOSMODE:
	case SPECIAL_IOPRIVL:
	case INTERNAL_VID:
	case EXTERNAL_VID:
		error = EINVAL;
		break;

	default:
		*hit = 0;
		break;
	}
	return (error);
}


/*
 *	Map the display into user address space.
 *	Called from kdmiocdatamsg() in ../kd/kdstr.c when the M_IOCDATA message
 *	is received in response to our copyin request (which was from
 *	kdvm_ioctl() above.)
 *
 *	At this point, the user process context has been stored into the
 *	copyresp structure (originally passed by the stream head) so we can
 *	now check the validity of the user's map request (struct kd_memloc),
 *	then perform the actual mapping.
 *
 *	[Note that this function is really only needed for V.3 compatibility,
 *	as V.4 applications are expected to use mmap() to map the display.]
 */
int
kdvm_mapdisp (queue_t *qp, mblk_t *mp)
{
	int		rv;
	channel_t	*chp;
	struct copyresp	*csp;
	struct kd_memloc *memlocp;
	struct iocblk	*iocp;
	struct map_info	*map_p = &Kdws.w_map;
	struct proc	*procp;
	struct _kthread	*threadp;	/* current thread pointer, passed in private */

	chp = (channel_t *)qp->q_ptr;	/* find channel given q pointer */

	iocp = (struct iocblk *)mp->b_rptr;	/* point at ioctl info */

	/* get at copyin response information so we can get our saved
	 * "currthread" stored in csp->csp_private.
	 */
	csp = (struct copyresp *)mp->b_rptr;
	threadp = (struct _kthread *)csp->cp_private;

	/* point "memlocp" at the copy of the user's kd_memloc structure */
	memlocp = (struct kd_memloc *)mp->b_cont->b_rptr;


	/* channel must be the currently-active channel, and should be in
	 * graphics mode.  (XXX why must it be in graphics mode? XXX)
	 */
	if (chp != ws_activechan(&Kdws) /* || chp->ch_dmode != KD_GRAPHICS */ ) {
		ws_iocnack(qp, mp, iocp, EACCES);
		return (EACCES);
	}

	/* find the mapping information for this display */
	map_p = &Kdws.w_map;

	/* update the mapping information for this channel */
	ws_mapavail(chp, map_p);

	/*
	 *	Determine the current process.
	 */
	procp = ttoproc(threadp);

	/* If the channel is mapped already, then this process must be the
	 * same process, or else fail with EBUSY.
	 */
	if ((map_p->m_procp && map_p->m_procp != procp) ||
	    map_p->m_cnt == CH_MAPMX) {
		ws_iocnack(qp, mp, iocp, EBUSY);
		return (EBUSY);
	}

	/* verify the correctness of the user's memloc data and do it. */
	/* (we use the iocblk-supplied credentials) */
	if ((rv = kdvm_map(threadp, chp, map_p, memlocp, iocp->ioc_cr)) != 0) {
		ws_iocnack(qp, mp, iocp, rv);
		return (rv);
	}

	ws_iocack(qp, mp, iocp);		/* everything went okay */
	return (0);
}


int
kdvm_map(struct _kthread *threadp, channel_t *chp, struct map_info *map_p, struct kd_memloc *memp, cred_t *cred)
{
	register ulong	start, end;
	int		cnt;
	int		rv;
	struct proc	*procp;
	struct as	*as;
	unsigned long	l;
	extern major_t	kd_major;	/* in kd/kdstr.c */

	if (memp->length <= 0)		/* length is too short */
		return (0);		/* failure ... */

	/* check requested physical range for validity */
	start = (ulong)memp->physaddr;
	end = (ulong)memp->physaddr + memp->length;

	for (cnt = 0; cnt < kdvmemcnt + 1; cnt++) {
		if (start >= kd_basevmem[cnt].start &&
		    end <= kd_basevmem[cnt].end)
			break;
	}

	if (cnt == kdvmemcnt + 1)	/* didn't find a valid memory range */
		return (EINVAL);	/* failure ... */

	map_p->m_addr[map_p->m_cnt] = *memp;	/* --structure copy-- */

	/*
	 *	Determine the current process.
	 */
	procp = ttoproc(threadp);

	/* the address space must have been created by now; if not, panic */
	if ((as = procp->p_as) == NULL)
		cmn_err(CE_PANIC, "kdvm_map: no as allocated");

	/* must free any pages user already has within the requested range */
	if (memp->vaddr) {
		if (as_unmap(as, memp->vaddr, memp->length))
			return (EINVAL);
	}

	/* map in the virtual memory as a set of device pages. */

	rv = ddi_segmap(
		makedevice (kd_major, chp->ch_id),	/* dev_t dev */
		(off_t)start,				/* off_t offset */
		as,					/* struct as *asp */
		(caddr_t *)&memp->vaddr,		/* caddr_t *addrp */
		(off_t)memp->length,			/* off_t len */
		PROT_READ | PROT_WRITE | PROT_USER,	/* u_int prot */
		PROT_READ | PROT_WRITE | PROT_USER,	/* u_int maxprot */
		MAP_SHARED | (memp->vaddr ? MAP_FIXED : 0), /* u_int flags */
		cred					/* cred_t *credp */
		);

	if (rv)			/* ddi_segmap failed */
		return (rv);

	/* enable i/o port access if requested */
	if (memp->ioflg)
		enableio(chp->ch_vstate.v_ioaddrs);

	if (!map_p->m_cnt) {
		map_p->m_procp = procp;
		drv_getparm(PPID, (unsigned long *)&l);
		map_p->m_pid = (pid_t) l;
		map_p->m_chan = chp->ch_id;
		chp->ch_flags |= CHN_UMAP;
	}
	map_p->m_cnt++;
	return (0);
}



/*
 *	Unmap the display.
 */
int
kdvm_unmapdisp(struct _kthread *threadp, channel_t *chp, struct map_info *map_p)
{
	int		cnt;
	struct proc	*procp;
	struct as	*as;

	chp->ch_vstate.v_font = 0;	/* assume the font has been munged */

	/*
	 *	Determine the process pointer given the kernel thread ptr.
	 */
	procp = ttoproc(threadp);

	/* get at the process address space information */
	if ((as = procp->p_as) == NULL)
		cmn_err(CE_PANIC, "kdvm: no as allocated");

	/* remove each memory region that was mapped */
	for (cnt = 0; cnt < map_p->m_cnt; cnt++) {
		as_unmap(as, map_p->m_addr[cnt].vaddr,
		    map_p->m_addr[cnt].length);

		/* also disable any i/o ports associated */
		if (map_p->m_addr[cnt].ioflg)
			disableio(chp->ch_vstate.v_ioaddrs);
	}

	map_p->m_procp = (struct proc *)0;
	map_p->m_pid = (pid_t) 0;
	chp->ch_flags &= ~CHN_MAPPED;
	map_p->m_cnt = 0;
	map_p->m_chan = 0;
	return (1);
}


/*
 *
 */

int
kdvm_xenixmap(queue_t *qp, mblk_t *mp, channel_t *chp, struct _kthread *threadp, int cmd, int *rvalp)
{
	struct map_info	*map_p = &Kdws.w_map;
	struct kd_memloc memloc;
	vidstate_t	*vp = &chp->ch_vstate;
	struct iocblk	*iocp;
	struct proc	*procp;
	int		rv;

	/*
	 *	Check that the map command makes sense for the video adapter
	 *	type in the system.
	 */
	switch (cmd) {
	case MAPMONO:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return (EINVAL);
		break;

	case MAPCGA:
		if (!DTYPE(Kdws, KD_CGA))
			return (EINVAL);
		break;

	case MAPEGA:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;

	case MAPVGA:
		if (!DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;

	case MAPSPECIAL:		/* not supported */
		return (EINVAL);

	default:
		break;
	}

	if (chp != ws_activechan(&Kdws))
		return (EACCES);
	ws_mapavail(chp, map_p);

	/*
	 *	Determine the process, given the thread pointer.
	 */
	procp = ttoproc(threadp);

	if (map_p->m_procp && map_p->m_procp != procp || map_p->m_cnt == CH_MAPMX)
		return (EBUSY);

	/*
	 * find the physical address and size of screen memory.
	 */
	memloc.physaddr = (char *)WSCMODE(vp)->m_base;
	memloc.length = WSCMODE(vp)->m_size;
	memloc.vaddr = 0;		/* not assigned yet */
	memloc.ioflg = 0;		/* doesn't include i/o privilege */

	iocp = (struct iocblk *)mp->b_rptr;	/* point at ioctl info */

	/* let kdvm_map do the hard part */
	if ((rv = kdvm_map(threadp, chp, map_p, &memloc, iocp->ioc_cr)) != 0) {
		ws_iocnack(qp, mp, iocp, rv);
		return (rv);
	}

	chp->ch_flags |= CHN_XMAP;
	*rvalp = (int)memloc.vaddr;	/* fill in the chosen address */
	return (0);
}


/*
 *	Called from kdvm_xenixioctl() to handle the MCAIO, CGAIO, EGAIO,
 *	VGAIO, and CONSIO ioctls.
 */
int
kdvm_xenixdoio(queue_t *qp, mblk_t *mp, int cmd, int arg, int *reply)
{
	switch (cmd) {
	case MCAIO:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return (EINVAL);
		break;
	case CGAIO:
		if (!DTYPE(Kdws, KD_CGA))
			return (EINVAL);
		break;
	case EGAIO:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;
	case VGAIO:
		if (!DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;
	case CONSIO:
		break;
	default:
		return (EINVAL);
	}
	/*
	 * Pass the user's address to be saved in the copyreq's 'private' field
	 * so we can get the address later if we need to do a copyout back to
	 * user space.
	 */
	ws_copyin (qp, mp, (caddr_t)arg, sizeof (struct port_io_arg),
	    (mblk_t *)arg);
	*reply = 0;
	return (0);
}


/*
 *	Called from kdmiocdatamsg() in ../kd/kdstr.c when the response to
 *	the copyin arrives, and we should now handle the actual i/o for
 *	the MCAIO, CGAIO, EGAIO, VGAIO, and CONSIO ioctls.
 *
 *	Note that this function is also called in response to our M_COPYOUT
 *	message (generated by ws_copyout after we've done the inb), so we
 *	need to check the "copyresp" structure for our flag indicating that
 *	this is the COPYOUT response.)
 */
void
kdvm_xenixdoio_bottom (queue_t *qp, mblk_t *mp)
{
	struct port_io_arg	portio;
	struct iocblk	*iocp;
	struct copyresp	*csp;
	channel_t	*chp;
	mblk_t		*tmp, *new;
	int		cnt, indone = 0;
	caddr_t		addr;

	csp = (struct copyresp *)mp->b_rptr;

	if (csp->cp_private == (mblk_t *)1) {	/* this is the response */
		/*
		 * This message is the response to our previous M_COPYOUT
		 * (as indicated by csp->cp_private being 1), so we simply
		 * need to send a normal M_IOCACK response to the original
		 * ioctl, so it will complete.
		 */
		ws_iocack(qp, mp, (struct iocblk *)mp->b_rptr);
		return;
	}

	chp = (channel_t *)qp->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;

	tmp = mp->b_cont;
	portio = *(struct port_io_arg *)tmp->b_rptr;	/* structure copy */
	freemsg (tmp);

	/* Handle each of the four possible port operations in the request */
	for (cnt = 0; cnt < 4 && portio.args[cnt].port; cnt++) {
		if (!ws_ck_kd_port(&chp->ch_vstate, portio.args[cnt].port)) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		}
		switch (portio.args[cnt].dir) {
		case IN_ON_PORT:
			portio.args[cnt].data = inb(portio.args[cnt].port);
			indone++;
			break;
		case OUT_ON_PORT:
			outb(portio.args[cnt].port, portio.args[cnt].data);
			break;
		default:
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		}
	}

	/*
	 *	If any port input was done, we need to send the results back
	 *	upstream, to be eventually copied out to user space.
	 */
	if (indone) {
		if (!(tmp = allocb(sizeof (portio), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to DOIO");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			return;
		}
		*(struct port_io_arg *)tmp->b_rptr = portio; /* structure copy */
		tmp->b_wptr += sizeof (portio);

		/*
		 * need to get a new mblock in order to pass the user address
		 * back up for the copyout.
		 */
		addr = (caddr_t) csp->cp_private;	/* saved user addr */
		if (!(new = allocb(sizeof (caddr_t), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get 2nd msg for DOIO reply");
			freemsg (tmp);
			ws_iocnack(qp, mp, iocp, ENOMEM);
			return;
		}
		*(caddr_t *)new->b_rptr = addr;	/* fill in the address */
		new->b_wptr += sizeof (addr);
		mp->b_cont = new;		/* link the mblk on */

		ws_copyout(qp, mp, tmp, sizeof (portio));
	} else {
		ws_iocack(qp, mp, iocp);	/* "out" only - done now */
	}
}


/*
 * middle part of WS_PIO_ROMFONT ioctl
 * The data should be present now after the copyin.
 */

void
ws_pio_romfont_middle (queue_t *qp, mblk_t *mp)
{
	struct copyreq	*cpq;
	struct iocblk	*iocp;
	mblk_t	*tmp;
	unsigned int	numchar;
	int	size;
	caddr_t	arg;

	iocp = (struct iocblk *)mp->b_rptr;
	tmp = mp->b_cont;

	numchar = *(unsigned int *)tmp->b_rptr;
	freemsg (tmp);
	mp->b_cont = (mblk_t *)NULL;

	if (numchar > MAX_ROM_CHAR) {
		ws_iocnack (qp, mp, iocp, EINVAL);
		return;
	}
	if (numchar == 0) {
		iocp->ioc_rval = kdv_release_fontmap();
		return;
	}
	size = sizeof (numchar) + numchar*sizeof (struct char_def);
	cpq = (struct copyreq *)mp->b_rptr;
	arg = (caddr_t)(cpq->cq_private);
	ws_copyin (qp, mp, arg, size, (mblk_t *)1);
}


/*
 * bottom part of WS_PIO_ROMFONT ioctl
 * The data should be present now after the copyin.
 */

void
ws_pio_romfont_bottom (queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;
	mblk_t	*tmp;
	unsigned int	numchar;
	caddr_t buf;
	caddr_t newbuf;
	int	size;

	iocp = (struct iocblk *)mp->b_rptr;
	tmp = mp->b_cont;

	numchar = *(unsigned int *)tmp->b_rptr;
	buf = (caddr_t)tmp->b_rptr;
	freemsg (tmp);
	mp->b_cont = (mblk_t *)NULL;

	size = sizeof(numchar) + numchar*sizeof(struct char_def);
	newbuf = kmem_alloc(size, KM_SLEEP);
	bcopy (buf, newbuf, size);
	iocp->ioc_rval = kdv_modromfont(newbuf, numchar);
	ws_iocack (qp, mp, iocp);
}
