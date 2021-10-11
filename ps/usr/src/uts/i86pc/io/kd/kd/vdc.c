/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved 					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)vdc.c	1.10	96/07/30 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/stream.h"
#include "sys/kd.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/ws/ws.h"
#include "sys/vid.h"
#include "sys/vdc.h"
#include "sys/archsystm.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

extern struct vdc_info	Vdc;
extern int AssumeVDCType;
#ifdef EVGA
extern int evga_inited;
#endif	/* EVGA */

long	vdcmonitor[] = {
	KD_MULTI_C,
	KD_MULTI_M,
	KD_STAND_C,
	KD_STAND_M,
};

/*
 *
 */

static int vdc_ckidstr(char *addrp, char *strp, int cnt);
static void vdc_wrnpar(int value);
static void vdc_wrnpar2(int value);
unchar vdc_rdmon(unchar mode);

extern caddr_t	kd_va;
extern caddr_t	p0_va;
extern struct vertim kd_l400[];
extern struct vertim kd_l350[];

/*ARGSUSED*/
int
vdc_disptype(qp, mp, iocp, chp, arg, copyflag)
queue_t	*qp;
mblk_t	*mp;
struct iocblk	*iocp;
channel_t	*chp;
int	arg;
int	*copyflag;
{
	vidstate_t	*vp = &chp->ch_vstate;
	mblk_t	*tmp;

	/*
	 *  Hopefully evc_info() returns 0 if an evga board is present.
	 *  If not, then evga won't work.
	 */

#ifdef	EVC
	if (evc_info(vp))
		;
	else
#endif	/* EVC */
	vdc_info(vp);
	if (!(tmp = allocb(sizeof (struct kd_vdctype), BPRI_MED))) {
		cmn_err(CE_NOTE,
		    "!vdc_disptype: can't get msg for reply to KDVDCTYPE");
		ws_iocnack(qp, mp, iocp, ENOMEM);
	}
	*(struct kd_vdctype *)tmp->b_rptr = Vdc.v_info;
	tmp->b_wptr += sizeof (struct kd_vdctype);
	ws_copyout(qp, mp, tmp, sizeof (struct kd_vdctype));
	*copyflag = 1;
	return (0);
}

/*
 *
 */

void
vdc_lktime(int dir)
{
	outb(0x3d4, 0x00);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, (dir) ? 0xaa : 0x55);
}

/*
 *
 */

void
vdc_scrambler(int dir)
{
	outb(0x3d4, 0x1f);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, (dir) ? 0x55 : 0xaa);
}

/* XXX NEW IMPLEMENTED */
void
vdc_lk750(int mode)
{
	register int indx;

	vdc_lktime(0);
	if (mode == DM_ATT_640) {	/* lock 400 line mode */
		for (indx = 0; indx < 15; indx++) {
			outb(0x3d4, kd_l400[indx].v_ind);
			outb(0x3d5, kd_l400[indx].v_val);
		}
	} else {			/* lock 350 line mode */
		vdc_scrambler(0);
		for (indx = 0; indx < 5; indx++) {
			outb(0x3d4, kd_l350[indx].v_ind);
			outb(0x3d5, kd_l350[indx].v_val);
		}
		vdc_scrambler(1);
	}
	vdc_lktime(1);
}

/*
 *
 */

unchar
vdc_unlk600(void)
{
	unchar tmp_pr5;

	outb(0x3ce, 0xf);
	tmp_pr5 = inb(0x3cf);
	outb(0x3ce, 0xf);
	outb(0x3cf, 0x5);
	return (tmp_pr5);
}


void
vdc_unlkcas2(void)
{
	unchar	tmp_reg;
	register int	s;

	s = clear_int_flag();
	outb(0x3c4, 0x06); outb(0x3c5, 0xec);
	outb(0x3c4, 0xaf); outb(0x3c5, 0x04);
	outb(0x3c4, 0x84); outb(0x3c5, 0x00);
	outb(0x3c4, 0xa7); outb(0x3c5, 0x00);
	outb(0x3c4, 0x91);
	tmp_reg = (inb(0x3c5) & 0x7f);
	outb(0x3c4, 0x91);
	outb(0x3c5, tmp_reg);
	restore_int_flag(s);
}


/*
 *
 */

static void
vdc_wrnpar2(int value)
{
	outb(0x3d4, 0x1a);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, value);
}

/*
 *
 */

static void
vdc_wrnpar(int value)
{
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3db, value);
}

/*
 *
 */

unchar
vdc_rd750sw()
{
	unchar sw = 0;

	vdc_wrnpar(0x80);		/* set to EGA mode */
	outb(MISC_OUT, 0x01);
	inb(0x3d8);
	inb(0x3d8);
	if (inb(IN_STAT_0) & SW_SENSE)
		sw = 1;
	outb(MISC_OUT, 0x05);
	inb(0x3d8);
	inb(0x3d8);
	if (inb(IN_STAT_0) & SW_SENSE)
		sw |= 0x02;
	outb(MISC_OUT, 0x01);
	return (sw);
}

/*
 *
 */

/*ARGSUSED*/
void
vdc_750cga(tsp, vp)
termstate_t	*tsp;
vidstate_t	*vp;
{
	vdc_lktime(1);			/* lock timing registers */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3c2, 0xa7);
	vdc_wrnpar2(0x94);
	vdc_scrambler(0);		/* disable scrambler logic */
	outb(0x3de, 0x10);
	outb(0x3c2, 0xa7);
	inb(0x3da);
	outb(0x3c0, 0x10);		/* set non-flicker mode */
	outb(0x3c0, 0x00);
	vdc_wrnpar2(0x94);
	vdc_wrnpar(0xc1);		/* set non-ega mode */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3de, 0x55);		/* unlock AT&T mode2 register */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d8, 0x2d);
	outb(0x3de, 0x00);		/* clear Mode 0 bit in mode2 reg */
	vp->v_cmos = MCAP_COLOR;
	kdv_adptinit(MCAP_COLOR);
	vp->v_type = KD_CGA;
	vp->v_cvmode = vp->v_dvmode;
	kdv_rst(vp);			/* Set display state */
}

/*
 * called from kdv_adptinit
 */

void
vdc_check(unchar adptype)
{
	register char	*addrp;
	ushort	*addr1p,
		*addr2p,
		save1,
		save2;

	if (adptype == MCAP_EGA) {
		addrp = (char *)phystokv(V750_IDADDR);
		if (AssumeVDCType == 1 || vdc_ckidstr(addrp, "22790", 5)) {
			Vdc.v_type = V750;
			/* read sw's 5, 6 on Super-Vu */
			if ((Vdc.v_switch = vdc_rd750sw()) & ATTDISPLAY) {
				/*
				 * use fast clock lock positive sync
				 * polarity alpha double scan enabled
				 */
				vdc_wrnpar(0x81);
				inb(0x3d8);
				inb(0x3d8);
				outb(0x3c2, 0x80);
				vdc_wrnpar2(0xe0);
			}
			return;
		}
		addrp = (char *)phystokv(V600_IDADDR);
		if (AssumeVDCType == 2 || vdc_ckidstr(addrp, "003116", 6)) {
			Vdc.v_type = V600;
			return;
		}
		addrp = (char *)phystokv(CAS2_IDADDR);
		if (AssumeVDCType == 3 ||
		    (vdc_ckidstr(addrp, "C02000", 6) && !(inb(0x78) & 0x8))) {
			Vdc.v_type = CAS2;
			return;
		}
	} else if (adptype == MCAP_COLOR || adptype == MCAP_COLOR40) {
		addr1p = (ushort *)phystokv(COLOR_BASE + 0x0001);
		addr2p = (ushort *)phystokv(COLOR_BASE + 0x4001);
		save1 = *addr1p;
		save2 = *addr2p;
		*addr2p = 0xa5;		/* Set a word in second page */
		*addr1p = 0;		/* Will overwrite if no second page */
		if (*addr2p != 0xa5) {
			*addr2p = save2;
			return;
		}
		/* Is a Rite-Vu (probably) */
		*addr1p = save1;
		*addr2p = save2;
		/*
		 * Have to hard code base register address because the
		 * v_regaddr field isn't set yet.  We know it's a color card.
		 */
		outb(COLOR_REGBASE + STATUS_REG, 0x00);
		drv_usecwait(10);
		if ((inb(COLOR_REGBASE + STATUS_REG) & 0xc0) == 0xc0)
			Vdc.v_type = V400;
	}
}

/*
 *
 */

#ifdef notdef
void
vdc_600regs(char *tabp)
{
	register int	indx;

	for (indx = 0x9; indx < 0xf; indx++, tabp++) {
		outb(0x3ce, indx);
		outb(0x3cf, *tabp);
	}
}
#endif

/*
 *
 */

/*ARGSUSED*/
vdc_mon_type(vp, color)
vidstate_t	*vp;
unchar	color;
{
	int	vgacolor;

	/*
	 * See what kind of display is attatched to a VGA adapter.  There are
	 * two ways to do this:
	 *
	 * 1)  We use the sense bit of the input status register 0 the way it's
	 * supposed to be used on the VGA.  On a VGA, we can sense a monochrome 
	 * monitor by playing around with the Video DAC, which is the device 
	 * that deals with the color registers.  A monochrome monitor uses only 
	 * the Green * signal, so we do the following:
	 *	Save current RGB registers for location 0
	 *	Program R=0, G=0, B=12 for location 0
	 *	Wait for the display active, then look at the sense bit.
	 *	If sense bit == 0 we've got a monochrome display (return 1)
	 *	otherwise, we've got a color display. (return 0)
	 *
	 * 2)  Knowing that when the system is booting, the VGA bios has
	 * already been through the method given above, we can cheat.  Though
	 * we can't use the video bios directly since we're in protected mode,
	 * we can look at the Miscellaneous Output Register (read at 0x3cc on a
	 * VGA) and see what the I/O Address Select bit is set to.
	 *
	 * We choose method 2.  Note that this must be called before reading
	 * the switches, which blindly writes into the same register we need
	 * for this function.  Everything works out because we only call this
	 * function if we discover that we're running on a VGA.
	 */
	vgacolor = inb(MISC_OUT_READ) & IO_ADDR_SEL;

	if ((color == LOAD_COLOR && vgacolor) ||
	    (color == LOAD_MONO && !vgacolor))
		return(1);
	else
		return(0);
}

/*
 * called from kdv_init and to satisfy KDVDCTYPE ioctls.
 */

void
vdc_info(vp)
vidstate_t	*vp;
{
	/*
	 *  If kd has been initialized for EVGA, then the Vdc information is
	 *  already established and immutable, so leave it alone.
	 */

#ifdef EVGA
    if (! evga_inited) {
#endif	/* EVGA */

	Vdc.v_info.cntlr = vp->v_type;
	Vdc.v_info.dsply = KD_UNKNOWN;
	switch (Vdc.v_info.cntlr) {
	case KD_CGA:
	case KD_EGA:
		switch (Vdc.v_type) {
		case V400:
		case V750:
			vdc_scrambler(0);
			Vdc.v_info.dsply = vdcmonitor[vdc_rdmon(vp->v_dvmode)];
			switch (Vdc.v_type) {
			case V400:
				Vdc.v_info.cntlr = KD_VDC400;
				break;
			case V750:
				Vdc.v_info.cntlr = KD_VDC750;
				break;
			}
			vdc_scrambler(1);
			break;
		default:
			break;
		}
			break;
	case KD_VGA:
		/*
		 * vdc_mon_type() returns non-zero if the dac test
		 * indicates that a monitor of the requested type was found.
		 */
		if (vdc_mon_type(vp, LOAD_COLOR))	{
			if (VTYPE(V600 | CAS2)) {
				Vdc.v_info.cntlr = KD_VDC600;
				Vdc.v_info.dsply = KD_MULTI_C;
			} else
				Vdc.v_info.dsply = KD_STAND_C;
		} else if (vdc_mon_type(vp, LOAD_MONO))
			Vdc.v_info.dsply = KD_STAND_M;
		else
			if (VTYPE(CAS2))
				Vdc.v_info.dsply = KD_STAND_M;
			else
				Vdc.v_info.dsply = KD_MULTI_C;
		break;
	default:
		break;
	}
#ifdef EVGA
    }   /* endif ( ! evga_inited ) */
#endif	/* EVGA */
}

/*
 *
 */

static int
vdc_ckidstr(char *addrp, char *strp, int cnt)
{
	register int	tcnt;

	for (tcnt = 0; tcnt < cnt; tcnt++) {
		if (*addrp++ != strp[tcnt])
			return (0);
	}
	return (1);
}

/*
 *
 */

unchar
vdc_rdmon(unchar mode)
{
	unchar	monbits;

	outb(0x3de, 0x10);
	monbits = (inb(0x3da) >> 4);		/* read id bits 4, 5 */
	if (monbits & 0x02) { /* non-multimode */
		outb(0x3de, 0x00);	/* reset mode0 */
	} else {	/* multimode */
		if (mode == DM_ATT_640)
			outb(0x3de, 0x00);	/* reset mode0 */
		else
			outb(0x3de, 0x10);	/* set mode0 */
	}
	return (monbits & 0x03);
}

void
vdc_cas2extregs(vp, mode)
vidstate_t	*vp;
unchar	mode;
{
	register unchar *regp;
	register ulong	*tabp;
	register int	offset;

	offset = WSMODE(vp, mode)->m_offset;
	switch (mode) {
	case DM_ATT_640:
	case DM_VDC800x600E:
		tabp = (ulong *)vp->v_parampp - 4;
		break;
	default:
		tabp = (ulong *)((unchar *)vp->v_parampp - 6);
		break;
	}
	regp = (unchar *)phystokv(ftop(*tabp)) + (0x1c * offset);
	outb(0x3c4, 0x86); outb(0x3c5, *(regp + 0x05));
	outb(0x3c4, 0xa4); outb(0x3c5, *(regp + 0x17));
}
