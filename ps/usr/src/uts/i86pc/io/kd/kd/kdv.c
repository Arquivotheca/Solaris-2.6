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

#pragma ident	"@(#)kdv.c	1.24	96/08/05 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/sysinfo.h"
#include "sys/buf.h"
#include "sys/cram.h"
#include "sys/cmn_err.h"
#include <sys/debug.h>
#include "sys/kd.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/ws/tcl.h"
#include "sys/ws/ws.h"
#include "sys/vid.h"
#include "sys/vdc.h"
#include "sys/kmem.h"
#include "sys/archsystm.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

/* function prototypes for tcl_ functions used here */
extern void	tcl_reset(wstation_t *, channel_t *, termstate_t *);



extern struct font_info fontinfo[];
extern struct font_info kd_romfonts[];
extern struct modeinfo kd_modeinfo[];
extern ushort kd_iotab[][MKDBASEIO];
extern struct b_param kd_inittab[];
extern struct reginfo kd_regtab[];
extern unchar kd_ramdactab[];
extern unchar	*egafont_p[5];
extern struct cgareginfo kd_cgaregtab[];
extern struct m6845init kd_cgainittab[];
extern struct m6845init kd_monoinittab[];
extern long kdmonitor[];
extern int kd_font_offset[];
extern char kd_swmode[];
extern wstation_t	Kdws;
extern channel_t	*ws_activechan(), *ws_getchan();
extern unchar	kd_adtconv[];
struct b_param *kdv_getparam();

extern struct vdc_info	Vdc;

extern int	kdioaddrcnt,
		kdvmemcnt;
extern struct kd_range	kd_basevmem[],
			kdvmemtab[];
extern ushort	kdconfiotab[];
extern caddr_t	kd_va;
extern caddr_t	p0_va;

caddr_t kd_romfont_mod = 0;

#ifdef EVGA
extern struct at_disp_info disp_info[];
extern int evga_inited;
extern int cur_mode_is_evga;
extern unchar saved_misc_out;
#endif	/* EVGA */

static void kdv_ldfont(vidstate_t *vp, unchar fnt);
static void kdv_params(void);
static void kdv_xfer(ushort *srcp, ushort *dstp, int cnt, char dir);
static void kdv_clrgraph(vidstate_t *vp, unchar mode);
static void kdv_ldramdac(vidstate_t *vp);
static void kdv_setmode0(unchar mode);
static void kdv_setall(vidstate_t *vp, unchar mode);
static ushort kdv_mem(int op, ushort *addr, ushort word);
static void kdv_disp(int on);
static void kdv_rstega(termstate_t *tsp, vidstate_t *vp);
static void kdv_setregs(char *tabp, int type);
static int kdv_rdsw(void);
static int kdv_ckherc(vidstate_t *vp);

/*
 *
 */

void
kdv_init(chp)
channel_t	*chp;
{
	unchar		adptype;
	int		indx, cnt;
	vidstate_t	*vp = &Kdws.w_vstate;
	termstate_t	*tsp = &chp->ch_tstate;

	Kdws.w_vstate.v_nfonts = 5; /* number of VGA fonts */
	Kdws.w_vstate.v_fontp = fontinfo;
	fontinfo[FONT8x8].f_fontp = (unchar *)phystokv(egafont_p[F8x8_INDX]);
	fontinfo[FONT8x14].f_fontp = (unchar *)phystokv(egafont_p[F8x14_INDX]);
	fontinfo[FONT8x14m].f_fontp = (unchar *)phystokv(egafont_p[F9x14_INDX]);
	fontinfo[FONT8x16].f_fontp = (unchar *)phystokv(egafont_p[F8x16_INDX]);
	fontinfo[FONT9x16].f_fontp = (unchar *)phystokv(egafont_p[F9x16_INDX]);
	kd_romfonts[FONT8x8] = fontinfo[FONT8x8];	/* structure copy */
	kd_romfonts[FONT8x14] = fontinfo[FONT8x14];	/* structure copy */
	kd_romfonts[FONT8x14m] = fontinfo[FONT8x14m];	/* structure copy */
	kd_romfonts[FONT8x16] = fontinfo[FONT8x16];	/* structure copy */
	kd_romfonts[FONT9x16] = fontinfo[FONT9x16];	/* structure copy */

	vp->v_modesp = kd_modeinfo;

	vp->v_font = 0;

	/* get equipment byte */
	adptype = (CMOSread(EB) >> 4) & 0x03;

	/*
	 * probe the address space of the adapter to see if there is really
	 * something out there.
	 */
	/*
	 * look in the system's BIOS data area to see if an adapter was
	 * found by the BIOS at system boot.
	 */
	vp->v_adapter_present = *(ulong *)(phystokv(0x4a8));

	if (vp->v_adapter_present == 0)		/* No adapter */
		adptype = MCAP_EGA;

	kdv_adptinit(adptype);
	vp->v_cmos = adptype;
	switch (adptype) {
	case MCAP_MONO:
		if (kdv_ckherc(vp)) {
			vp->v_type = KD_HERCULES;
			indx = KD_HERCINDX;
		} else {
			vp->v_type = KD_MONO;
			indx = KD_MONOINDX;
		}
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[indx][cnt];
		break;
	case MCAP_COLOR40:
		vp->v_cmos = MCAP_COLOR;
		/*FALLTHROUGH*/
	case MCAP_COLOR:
		vp->v_type = KD_CGA;
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[KD_COLRINDX][cnt];
		break;
	case MCAP_EGA:
		kdv_params();
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[KD_EGAINDX][cnt];
		break;
	}
	/* v_dvmode set by kdv_adptinit */
	vp->v_cvmode = vp->v_dvmode;
	kdv_rst(tsp, vp);
	kdv_enable(vp);

	if (DTYPE(Kdws, KD_VGA)) {
#ifdef	EVC
		if (evc_info(vp))
			;
		else
#endif	/* EVC */
		vdc_info(vp);
		if (Vdc.v_info.dsply == KD_MULTI_M ||
		    Vdc.v_info.dsply == KD_STAND_M) {
			vp->v_cvmode = vp->v_dvmode = DM_VGAMONO80x25;
			kdv_rst(tsp, vp);
			kdv_enable(vp);
		}
	}

	/* set up legal I/O port and video memory addresses */
	if (kdioaddrcnt > 0 && kdioaddrcnt <= MKDCONFADDR) {
		for (cnt = 0; cnt < MKDCONFADDR; cnt++) {
			if (vp->v_ioaddrs[cnt] == 0) {
				bcopy((caddr_t)kdconfiotab,
					(caddr_t)&vp->v_ioaddrs[cnt],
					(sizeof (ushort) * kdioaddrcnt));
				break;
			}
		}
	}
#ifndef EVC
	if (kdvmemcnt > 0 && kdvmemcnt <= MKDCONFADDR)
		bcopy((caddr_t)kdvmemtab, (caddr_t)&kd_basevmem[1],
				(sizeof (struct kd_range) * kdvmemcnt));
#endif
}

/*
 *
 */

void
kdv_adptinit(unchar atype)
{
	vidstate_t	*vp = &Kdws.w_vstate;

	switch (atype) {
	case MCAP_MONO:
		vp->v_dvmode = DM_EGAMONO80x25;	/* equiv. EGA mode */
		vp->v_scrmsk = MONO_SCRMASK;
		vp->v_modesel = M_ALPHA80 | M_BLINK | M_ENABLE;
		break;
	case MCAP_COLOR40:
	case MCAP_COLOR:
		vp->v_dvmode = (atype == MCAP_COLOR) ? DM_C80x25 : DM_C40x25;
		vp->v_scrmsk = COLOR_SCRMASK;
		vp->v_modesel = kd_cgaregtab[vp->v_dvmode].cga_mode;
		if (!VTYPE(V750))
			vdc_check(atype);	/* CGA or AT&T Rite-Vu	? */
		if (VTYPE(V400 | V750)) /* Is AT&T Adapter ? */
			Vdc.v_mode2sel = M_UNDRLINE;
		break;
	case MCAP_EGA:

#ifdef	EVC
		if (evc_info(vp)) {	/* EVC-1 is superset of VGA */
			vp->v_dvmode = DM_VGA_C80x25;
			vp->v_type = KD_VGA;
			vp->v_scrmsk = EGA_SCRMASK;
			break;
		}
#endif	/* EVC */
		vdc_check(atype);
		outb(0x3c4, 0x5); /* test for VGA */
		if (inb(0x3c4) != 0x5 && vp->v_adapter_present) {
			vp->v_dvmode = kdv_rdsw();	/* EGA */
			vp->v_type = KD_EGA;
		} else {
			/* It is a VGA or there's no adapter present */
			vp->v_dvmode = DM_VGA_C80x25;
			vp->v_type = KD_VGA;
			if (VTYPE(V600))
				WSMODE(vp, DM_ATT_640)->m_offset = 4;
			else if (VTYPE(CAS2)) {
				WSMODE(vp, DM_ATT_640)->m_params = KD_CAS2;
				WSMODE(vp, DM_ATT_640)->m_offset = 1;
				WSMODE(vp, DM_VDC800x600E)->m_params = KD_CAS2;
				WSMODE(vp, DM_VDC800x600E)->m_offset = 9;
			}
		}
		vp->v_scrmsk = EGA_SCRMASK;	/* set screen mask */
		if (VTYPE(V750)) {
			if (!VSWITCH(ATTCGAMODE))	/* set CGA mode */
				vp->v_dvmode = DM_ENH_CGA;
			else if (VSWITCH(ATTDISPLAY) && DMODE(Kdws, DM_EGAMONO80x25))
				vp->v_dvmode = DM_B80x25;
		}
		break;
	}
}

/*
 *
 */

void
kdv_setmode(chp, newmode)
channel_t	*chp;
unchar	newmode;
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;

	if (vp->v_cvmode == newmode)
		return;
	while (Kdws.w_flags & WS_NOMODESW)
		sleep((caddr_t)&Kdws.w_flags, PZERO);
	Kdws.w_flags |= WS_NOCHANSW;

#ifdef EVGA
	evga_ext_rest(vp->v_cvmode);
#endif	/* EVGA */

	kdv_setdisp(chp, vp, tsp, newmode);

#ifdef EVGA
	/* kdv_setdisp() sets vp->v_cvmode */
	evga_ext_init(vp->v_cvmode);
#endif	/* EVGA */

	if (WSCMODE(vp)->m_color) {	/* color mode? */
		vp->v_undattr = tsp->t_curattr;
	}
	if (WSCMODE(vp)->m_font) {	/* text mode? */
		tcl_reset(&Kdws, chp, tsp);
	}
	Kdws.w_flags &= ~WS_NOCHANSW;
	wakeup((caddr_t)&Kdws.w_flags);
}

/*
 *
 */
/*ARGSUSED*/
void
kdv_cursortype(wsp, chp, tsp)
wstation_t	*wsp;
channel_t	*chp;
termstate_t	*tsp;
{
	ushort cur_end, new_val;
	ushort reg_base = 0x3d0;

	if (chp != ws_activechan(&Kdws))
		return;

	if (DTYPE(Kdws, KD_EGA)) {
		if (VTYPE(V750))
			vdc_lktime(0);
		switch (chp->ch_vstate.v_font) {
		case FONT8x8:
			cur_end = 0x7;
			if (tsp->t_curtyp == 0)
				new_val = 0x7;
			else if (tsp->t_curtyp == 1)
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
			break;

		case FONT8x16:
		case FONT9x16:
			cur_end = 0xd;
			if (tsp->t_curtyp == 0)
				new_val = 0xd;
			else if (tsp->t_curtyp == 1)
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
			break;

		case FONT8x14:
		case FONT8x14m:
			cur_end = 0xd;
			if (tsp->t_curtyp == 0)
				new_val = 0xd;
			else if (tsp->t_curtyp == 1)
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
			break;

		default:
			return;
		}

		outb(reg_base + 4, 0x0b);
		outb(reg_base + 5, cur_end);
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);
		if (VTYPE(V750))
			vdc_lktime(1);
		return;
	}

	if (DTYPE(Kdws, KD_VGA)) {
		if (!(inb(0x3cc) & 0x01))
			reg_base = 0x3b0;
		outb(reg_base + 4, 0x0b);
		cur_end = inb(reg_base + 5);
		if (tsp->t_curtyp == 0)
			new_val = cur_end - 1;
		else if (tsp->t_curtyp == 1)
			new_val = 0x00;
		else
			new_val = cur_end + 1;
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);
		return;
	}

	if (DTYPE(Kdws, KD_CGA) || DTYPE(Kdws, KD_MONO)) {
		switch (chp->ch_vstate.v_font) {
		case FONT8x8:
		case FONT8x16:
		case FONT9x16:
			cur_end = 0x7;
			if (tsp->t_curtyp == 0)
				new_val = 0x6;
			else if (tsp->t_curtyp == 1)
				new_val = 0x00;
			else {
				cur_end = 0x20;
				new_val = 0x20;
			}
			break;

		case FONT8x14:
		case FONT8x14m:
			cur_end = 0x6;
			if (tsp->t_curtyp == 0)
				new_val = 0x5;
			else if (tsp->t_curtyp == 1)
				new_val = 0x00;
			else {
				cur_end = 0x20;
				new_val = 0x20;
			}
			break;

		default:
			return;
		}
		outb(reg_base + 4, 0x0b);
		outb(reg_base + 5, cur_end);
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);
	}
}

/*
 *
 */

int
kdv_notromfont(vp)
vidstate_t *vp;
{
	if (kd_romfont_mod)
		return (1);
	if (vp->v_fontp[FONT8x8].f_fontp != kd_romfonts[FONT8x8].f_fontp)
		return (1);
	if (vp->v_fontp[FONT8x14].f_fontp != kd_romfonts[FONT8x14].f_fontp)
		return (1);
	if (vp->v_fontp[FONT8x16].f_fontp != kd_romfonts[FONT8x16].f_fontp)
		return (1);
	return (0);
}

void
kdv_setdisp(chp, vp, tsp, newmode)
channel_t	*chp;
vidstate_t	*vp;
termstate_t	*tsp;
unchar	newmode;
{
	int newfont;

	vp->v_cvmode = newmode;

	/* WSCMODE(vp) == (vp->v_modesp + vp->v_cvmode) */

	newfont = WSCMODE(vp)->m_font;
	if (newfont) {	/* text mode? */
		chp->ch_dmode = KD_TEXT0;
		tsp->t_rows = WSCMODE(vp)->m_rows;
		tsp->t_cols = WSCMODE(vp)->m_cols;
		tsp->t_scrsz = tsp->t_cols * tsp->t_rows;
		vp->v_font = 0; /* force a font load */
	} else
		chp->ch_dmode = KD_GRAPHICS;
	if (chp == ws_activechan(&Kdws)) {
		kdv_rst(tsp, vp);
		kdv_enable(vp);
		kdv_cursortype(&Kdws, chp, tsp);
		if (WSCMODE(vp)->m_font) {	/* text? */
			tsp->t_cursor -= tsp->t_origin;
			tsp->t_origin = 0;
		}
	}
}

/*
 *
 */

void
kdv_rst(tsp, vp)
termstate_t	*tsp;
vidstate_t	*vp;
{
	if (CMODE(Kdws, DM_ENH_CGA) && VTYPE(V750)) {
		vdc_750cga(tsp, vp);
		return;
	}
	if (WSCMODE(vp)->m_color) {	/* color mode ? */
		vp->v_regaddr = COLOR_REGBASE;	/* register address */
	} else {				/* monochrome adapter mode */
		vp->v_regaddr = MONO_REGBASE;	/* register address */
		vp->v_undattr = UNDERLINE;	/* Set underline */
	}
	switch (vp->v_cmos) {
	case MCAP_MONO:
		outb(vp->v_regaddr + MODE_REG, M_ALPHA80);
		kdv_setregs((char *)kd_monoinittab, I_6845MONO);
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
		break;
	case MCAP_COLOR:
		kdv_disp(0);		/* Turn off display */
		kdv_setregs((char *)&kd_cgainittab[kd_cgaregtab[vp->v_cvmode].cga_index], I_6845COLOR);	/* Set regs */
		outb(vp->v_regaddr + COLOR_REG,
		    kd_cgaregtab[vp->v_cvmode].cga_color);	/* Set color */
		vp->v_modesel = kd_cgaregtab[vp->v_cvmode].cga_mode;	/* Save new mode */
		vp->v_font = WSCMODE(vp)->m_font;
		break;
	case MCAP_EGA:
		if (vp->v_adapter_present != 0)
		    kdv_rstega(tsp, vp);
		break;
	}
	vp->v_rscr = (caddr_t)WSCMODE(vp)->m_base;	/* Set base */
	vp->v_scrp = (ushort *)phystokv(vp->v_rscr);	/* Set base */
#ifdef EVGA
	if (vp->v_cvmode != ENDNONEVGAMODE+15 &&
	    vp->v_cvmode != ENDNONEVGAMODE+16 &&
	    vp->v_cvmode != ENDNONEVGAMODE+34 &&
	    vp->v_cvmode != ENDNONEVGAMODE+35) {
#endif
		if (!WSCMODE(vp)->m_font) {
			if (vp->v_rscr == (caddr_t)EGA_BASE &&
			    WSCMODE(vp)->m_color) {
				outb(0x3ce, 0x05); outb(0x3cf, 0x02);
			}
			bzero((caddr_t)vp->v_scrp, WSCMODE(vp)->m_size);
			if (vp->v_rscr == (caddr_t)EGA_BASE &&
			    WSCMODE(vp)->m_color) {
				struct b_param	*initp;

				initp = kdv_getparam(vp->v_cvmode);
				out_reg(&kd_regtab[I_GRAPH], 0x5,
				    ((char *)initp->graphtab)[0x5]);
			}
		}
#ifdef EVGA
	}
#endif
}

/*
 *
 */

void
kdv_enable(vp)
register vidstate_t	*vp;
{
	switch (vp->v_cmos) {
	case MCAP_COLOR:
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);	/* Set mode */
		if (VTYPE(V400))
			outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);
		break;
	case MCAP_EGA:
		(void) inb(vp->v_regaddr + IN_STAT_1);
		outb(kd_regtab[I_ATTR].ri_address, PALETTE_ENABLE);
		kdv_disp(1);
		break;
	}
}

/*
 *
 */

static void
kdv_disp(int on)
{
	register vidstate_t	*vp = &Kdws.w_vstate;

	if (on) {
		vp->v_modesel |= M_ENABLE;
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
	} else if (vp->v_modesel & M_ENABLE) {
		/* wait for vertical sync */
		while (!(inb(vp->v_regaddr + STATUS_REG) & S_VSYNC) &&
				vp->v_adapter_present)
			;
		vp->v_modesel &= ~M_ENABLE;
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
	}
}

/*
 *
 */

static void
kdv_setregs(char *tabp, int type)
{
	register int	index, count;
	struct reginfo	*regp;

	regp = &kd_regtab[type];
	count = regp->ri_count;
	index = (type == I_SEQ) ? 1 : 0;
	for (; index < count; index++, tabp++) {
		out_reg(regp, index, *tabp); /* out_reg is a multi-line macro */
	}
}

/*
 *
 */

static void
kdv_rstega(termstate_t *tsp, vidstate_t *vp)
{
	unchar	fnt;		/* Font type for EGA mode */

	fnt = WSCMODE(vp)->m_font;	/* get font type for mode */

	/* v_cvmode set in kdv_setdisp */

	kdv_setall(vp, vp->v_cvmode);
	if (fnt != 0 && fnt != vp->v_font) {
		kdv_ldfont(vp, fnt);
	}
	vp->v_font = fnt;	/* remember which font is loaded */
	/* Adjust cursor start and end positions */
	if (!VTYPE(V600 | CAS2)) {
		if (fnt == FONT8x8) {
			outb(0x3d4, 0xa); outb(0x3d5, 0x6);
			outb(0x3d4, 0xb); outb(0x3d5, 0x7);
		} else {
			outb(0x3d4, 0xa); outb(0x3d5, 0xb);
			outb(0x3d4, 0xb); outb(0x3d5, 0xc);
		}
	}
	if (Kdws.w_init > 1) {
		/* Initialization was successful */
		if (vp->v_regaddr == MONO_REGBASE) {
			tsp->t_attrmskp[1].attr = 0;
			tsp->t_attrmskp[4].attr = 1;
			tsp->t_attrmskp[34].attr = 7;
		} else {
			tsp->t_attrmskp[1].attr = BRIGHT;
			tsp->t_attrmskp[4].attr = 0;
			tsp->t_attrmskp[34].attr = 1;
		}
	}
}

void
kdv_setcursor_type(int curtype)
{
	/* Adjust cursor start and end positions */
	unsigned char startsline, endsline;

	startsline = curtype&0xff;
	endsline = (curtype>>16)&0xff;

	outb(0x3d4, 0xa); outb(0x3d5, startsline);
	outb(0x3d4, 0xb); outb(0x3d5, endsline);
}

/*
 *
 */

int
kdv_stchar(chp, dest, ch, count)
channel_t	*chp;
ushort	dest,
	ch;
int	count;
{
	ushort	scrmsk, *dstp;
	vidstate_t	*vp = &chp->ch_vstate;
	register int	cnt, avail;

	scrmsk = vp->v_scrmsk;
	do {
		dest &= scrmsk;
		dstp = vp->v_scrp + dest;
		avail = scrmsk - dest + 1;
		if (count < avail)
			avail = count;
		count -= avail;
		if (chp == ws_activechan(&Kdws) && ATYPE(Kdws, MCAP_COLOR) &&
		    !VTYPE(V400 | V750)) {
			if (avail < 8) {
				for (cnt = 0; cnt < avail; cnt++)
					(void) kdv_mem(KD_WRVMEM, dstp++, ch);
			} else {
				kdv_disp(0);
				for (cnt = 0; cnt < avail; cnt++)
					*dstp++ = ch;
				kdv_disp(1);
			}
		} else
			for (cnt = 0; cnt < avail; cnt++)
				*dstp++ = ch;
		dest += avail;
	} while (count != 0);
	return (0);
}

/*
 *
 */

static void
kdv_setall(vidstate_t *vp, unchar mode)
{
	struct b_param	*initp;
	unchar	tmp_reg;

	/*
	 * Turn the display off.
	 * kdv_setall is only called by kdv_rstega, which is only
	 * called by kdv_rst. Every call to kdv_rst is followed by
	 * a call to kdv_enable, which turns the display back
	 * on (kdv_disp(1)).
	 */

	kdv_disp(0);

	initp = kdv_getparam(mode);
#ifdef	EVC
	mode = evc_init(mode);	/* pass through if no EVC-1 */
#endif	/* EVC */

	if (VTYPE(V750) && VSWITCH(ATTDISPLAY))
		(void) kdv_setmode0(mode); /* sets mode0 appropriately */
	/*
	 * set the Palette Address Source bit to 0 to clear the screen
	 * while we reprogram all the registers.
	 */
	(void) inb(vp->v_regaddr + IN_STAT_1);	/* Initialize flip-flop */

	outb(kd_regtab[I_ATTR].ri_address, 0);

	switch (Vdc.v_type) {

	case V750:
		if (mode == DM_ENH_CGA) {
			/* set Super-Vu to CGA mode */
			inb(0x3d8);
			inb(0x3d8);	/* unmask status 1/misc out register */
			outb(0x3c2, 0x23);
			vdc_lktime(0);	/* unlock timing registers */
			vdc_scrambler(0);	/* turn off scrambler logic */
		}
		break;

	case V600:
		tmp_reg = vdc_unlk600();
		outb(0x3ce, 0x0d); outb(0x3cf, 0x00);
		outb(0x3ce, 0x0e); outb(0x3cf, 0x00);
		break;

	case CAS2:
		vdc_unlkcas2();		/* Cascade 2-specific diddling */
		break;

	default:
		break;
	}

	/*
	 * Reset sequencer.
	 */
	out_reg(&kd_regtab[I_SEQ], 0, SEQ_RST); /* sequencer reset */
	/*
	 * Set miscellaneous register.
	 */
	outb(MISC_OUT, initp->miscreg);

	if (VTYPE(V750) && VSWITCH(ATTDISPLAY))
		vdc_lk750(mode);

	/*
	 * Initialize CRT controller.
	 */
	if (WSMODE(vp, mode)->m_color) {
		if (DTYPE(Kdws, KD_VGA)) {
			out_reg(&kd_regtab[I_EGACOLOR], 0x11,
			    ((char *)&initp->egatab)[0x11] & ~0x80);
		}
		kdv_setregs((char *)&initp->egatab, I_EGACOLOR);
	} else {
		if (DTYPE(Kdws, KD_VGA)) {
			out_reg(&kd_regtab[I_EGAMONO], 0x11,
			    ((char *)&initp->egatab)[0x11] & ~0x80);
		}
		kdv_setregs((char *)&initp->egatab, I_EGAMONO);
	}
	/*
	 * Initialize graphics registers.
	 */
	if (DTYPE(Kdws, KD_VGA)) {
		outb(GRAPH_1_POS, GRAPHICS1);
		outb(GRAPH_2_POS, GRAPHICS2);
	}
	kdv_setregs((char *)initp->graphtab, I_GRAPH);

	/*
	 * Program sequencer registers.
	 */
	kdv_setregs((char *)initp->seqtab, I_SEQ);
	out_reg(&kd_regtab[I_SEQ], 0, SEQ_RUN);

	/* turn display off */

	kdv_disp(0);	/* loading the sequencer registers turns on display */
	/*
	 * Initialize attribute registers.
	 */
	(void) inb(vp->v_regaddr + IN_STAT_1);	/* Initialize flip-flop */
	kdv_setregs((char *)initp->attrtab, I_ATTR);

	/* v_border gets set via KDSBORDER ioctl */

	out_reg(&kd_regtab[I_ATTR], 0x11, vp->v_border);
	if (DTYPE(Kdws, KD_VGA)) {
		outb(0x3c0, 0x14); outb(0x3c0, 0x00);
		switch (Vdc.v_type) {

		case V600:
			outb(0x3ce, 0x0e);
			if (mode == DM_VDC640x400V)
				outb(0x3cf, 0x01);
			else
				outb(0x3cf, 0x00);
			outb(0x3ce, 0x0f); outb(0x3cf, tmp_reg);
			break;

		case CAS2:
			out_reg(&kd_regtab[I_SEQ], 0, SEQ_RST);
			vdc_cas2extregs(vp, mode);
			out_reg(&kd_regtab[I_SEQ], 0, SEQ_RUN);
			break;

#ifdef	EVC
		case VEVC:
			evc_finish(mode);
			break;
#endif	/* EVC */

		default:
			break;

		}
		kdv_ldramdac(vp);
	}

	if (!WSMODE(vp, mode)->m_font) /* graphics */
		/* clear screen, do something special for DM_VDC640x400V */
		kdv_clrgraph(vp, mode);
}

/*
 *
 */

/* should never be called for evga */

static void
kdv_setmode0(unchar mode)
{
	unchar	monid;

	vdc_scrambler(0);			/* disable scrambler logic */
	monid = vdc_rdmon(mode);
	if (monid & 0x02)		/* non-multimode */
		outb(0x3de, 0x00);	/* reset mode0 */
	else if (mode == DM_ATT_640)	/* multimode */
		outb(0x3de, 0x00);	/* reset mode0 */
	else
		outb(0x3de, 0x10);	/* set mode0 */
	vdc_scrambler(1);			/* reenable scrambler logic */
}

/*
 *
 */

static void
kdv_ldramdac(vidstate_t *vp)
{
	register unchar	*start, *end;
	register int	s;

#ifdef EVGA
	/* Don't assume all vga are using m_ramdac */
	if (WSCMODE(vp)->m_ramdac <= 3) {
#endif
		while ((inb(vp->v_regaddr + STATUS_REG) & S_VSYNC) &&
					vp->v_adapter_present)
			;
		s = clear_int_flag();
		while (!(inb(vp->v_regaddr + STATUS_REG) & S_VSYNC) &&
					vp->v_adapter_present)
			;
		start = &kd_ramdactab[0] + (WSCMODE(vp)->m_ramdac * 0x300);
		end = start + 0x300;
		outb(0x3c6, 0xff);
		outb(0x3c8, 0x00);
		while (start != end)
			outb(0x3c9, *start++);
		restore_int_flag(s); /* restore interrupt state */
#ifdef EVGA
	}
#endif
}

/*
 *
 */

static void
kdv_clrgraph(vidstate_t *vp, unchar mode)
{
	int	size, offset;
	unchar	tmp_pr5;
	caddr_t	addr;

#ifdef EVGA
	if (mode != ENDNONEVGAMODE+15 && mode != ENDNONEVGAMODE+16 &&
	    mode != ENDNONEVGAMODE+34 && mode != ENDNONEVGAMODE+35) {
#endif
		addr = (caddr_t)phystokv(WSMODE(vp, mode)->m_base);
		size = (int)WSMODE(vp, mode)->m_size;
		bzero(addr, size);
		if (mode == DM_VDC640x400V) {
			tmp_pr5 = vdc_unlk600();
			for (offset = 1; offset < 4; offset++) {
				outb(0x3ce, 0x09);
				outb(0x3cf, (0x10 * offset));
				bzero(addr, size);
			}
			outb(0x3ce, 0x09); outb(0x3cf, 0x00);
			outb(0x3ce, 0x0f); outb(0x3cf, tmp_pr5);
		}
#ifdef EVGA
	}
#endif
}

/*
 *
 */

static ushort
kdv_mem(int op, ushort *addr, ushort word)
{
	register vidstate_t	*vp = &Kdws.w_vstate;
	register ushort	tmp = 0;
	register int	s;

	while ((inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK) &&
				vp->v_adapter_present)
		;
	s = clear_int_flag();
	while (!(inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK) &&
				vp->v_adapter_present)
		;
	if (op == KD_RDVMEM)
		tmp = *addr;
	else
		*addr = word;
	restore_int_flag(s);
	return (tmp);
}

/*
 *
 */

static void
kdv_xfer(ushort *srcp, ushort *dstp, int cnt, char dir)
{
	if (dir == UP) {
		while (cnt--)
			*dstp-- = *srcp--;
	} else {
		while (cnt--)
			*dstp++ = *srcp++;
	}
}

/*
 *
 */

int
kdv_mvword(chp, from, to, count, dir)
channel_t	*chp;
ushort	from, to;
int	count;
char	dir;
{
	vidstate_t	*vp = &chp->ch_vstate;
	register ushort	scrmsk, *srcp, *dstp;
	register int	avail;

	scrmsk = vp->v_scrmsk;
	if (dir == UP) {
		from += count - 1;
		to += count - 1;
	}
	while (count > 0) {
		from &= scrmsk;
		to &= scrmsk;
		if (dir == UP)
			avail = ((from < to) ? from : to) + 1;
		else
			avail = scrmsk - ((from > to) ? from : to) + 1;
		if (count < avail)
			avail = count;
		srcp = vp->v_scrp + from;
		dstp = vp->v_scrp + to;
		if (dir == UP) {
			if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750)) {
				if (avail >= 8) {
					kdv_disp(0);
					vcopy((char *)srcp, (char *)dstp,
					    avail, dir);
					kdv_disp(1);
				} else {
					while (avail--)
						(void) kdv_mem(KD_WRVMEM, dstp--, kdv_mem(KD_RDVMEM, srcp--, 0));
				}
			} else
				kdv_xfer(srcp, dstp, avail, UP);
			from -= avail;
			to -= avail;
		} else {
			if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750)) {
				if (avail >= 8) {
					kdv_disp(0);
					vcopy((char *)srcp, (char *)dstp,
					    avail, dir);
					kdv_disp(1);
				} else {
					while (avail--)
						(void) kdv_mem(KD_WRVMEM, dstp++, kdv_mem(KD_RDVMEM, srcp++, 0));
				}
			} else
				kdv_xfer(srcp, dstp, avail, DOWN);
			from += avail;
			to += avail;
		}
		count -= avail;
	}
	return (0);
}

/*
 *	Check for Hercules display adapter.
 */

static int
kdv_ckherc(vidstate_t *vp)
{
	register ushort *basep, save;
	register int	rv = 0;	/* assume it is not a hercules */

	outb(0x3bf, 0x02);	/* map in second page of hercules memory */
	basep = (ushort *)phystokv(WSCMODE(vp)->m_base);	/* Base */
	save = *basep;
	*basep = 0;		/* set first word of memory to zero */
	*(basep + 0x4000) = 1;	/* set first word of 2nd page to one */
	if (*(basep + 0x4000) == 1 && *basep == 0) { /* probably a hercules */
		outb(0x3bf, 0x00);	/* unmap second page of memory */
		rv = 1;
	}
	*basep = save;
	return (rv);
}

/*
 * Read the switches.
 */

static int
kdv_rdsw(void)
{
	register int	cnt;
	register unchar	sw = 0;

	outb(MISC_OUT, 0x1);
	for (cnt = 3; cnt >= 0; cnt--) {
		outb(MISC_OUT, ((cnt << CLKSEL) + 1));
		if (inb(IN_STAT_0) & SW_SENSE)
			sw |= (1 << (3 - cnt));
	}
	return (kd_swmode[sw & 0x0f]);
}

/*
 *
 */

static void
kdv_params(void)
{
	Kdws.w_vstate.v_parampp =
	    (unchar **)phystokv(ftop(*(ulong *)phystokv(0x4a8)));
}

/*
 *
 */

static void
kdv_ldfont(vidstate_t *vp, unchar fnt)
{
	register unchar	*from,		/* Source in ROM */
			*to;		/* Destination in generator */
	ushort		skip;		/* Bytes to skip when loading */
	register ushort	i;		/* Counter */
	ushort	bpc;
	ulong	count;
	unchar	ccode;		/* Character code to change */
	int pervtflag = 0;

	outb(0x3c4, 0x02); outb(0x3c5, 0x04);
	outb(0x3c4, 0x04); outb(0x3c5, 0x06);
	outb(0x3ce, 0x05); outb(0x3cf, 0x00);
	outb(0x3ce, 0x06); outb(0x3cf, 0x04);

	switch (fnt) {
	case FONT8x14m:	/* load 8x14 font first */
		from = vp->v_fontp[FONT8x14].f_fontp;
		bpc = vp->v_fontp[FONT8x14].f_bpc;
		count = vp->v_fontp[FONT8x14].f_count;
		pervtflag = ! (from == kd_romfonts[FONT8x14].f_fontp);
		break;
	case FONT9x16:	/* load 8x16 font first */
		from = vp->v_fontp[FONT8x16].f_fontp;
		bpc = vp->v_fontp[FONT8x16].f_bpc;
		count = vp->v_fontp[FONT8x16].f_count;
		pervtflag = ! (from == kd_romfonts[FONT8x16].f_fontp);
		break;
	default:
		from = vp->v_fontp[fnt].f_fontp;	/* point to table */
		bpc = vp->v_fontp[fnt].f_bpc;	/* bytes / character */
		count = vp->v_fontp[fnt].f_count;	/* character count */
		pervtflag = ! (from == kd_romfonts[fnt].f_fontp);
		break;
	}
	to = (unchar *)phystokv(CHGEN_BASE);	/* Point to generator base */
	skip = 0x20 - bpc;		/* Calculate bytes to skip in gen */
	while (count-- != 0) {	/* Copy all characters from ROM */
		for (i = bpc; i != 0; i--)	/* Copy valid character */
			*to++ = *from++;	/* Copy byte of character */
		to += skip;		/* Skip over unneeded space */
	}
	/*
	 * Only apply ROM font changes for 8x14m or 9x16 if we got base font
	 * the ROM
	 */
	if ((fnt == FONT8x14m && kd_romfonts[FONT8x14].f_fontp == vp->v_fontp[FONT8x14].f_fontp) ||
	    (fnt == FONT9x16 && kd_romfonts[FONT8x16].f_fontp == vp->v_fontp[FONT8x16].f_fontp)) {
		from = vp->v_fontp[fnt].f_fontp;
		while ((ccode = *from++) != 0) {
			to = (unchar *)(phystokv(CHGEN_BASE) + ccode * 0x20);
			for (i = bpc; i != 0; i--)
				*to++ = *from++;
		}
	}

	/*
	 * This code is used to overlay any defined user changes.  Only
	 * do this if we see that VT has not set up its own private font
	 * but rather is using the ROM font
	 */

	if (kd_romfont_mod != NULL && !pervtflag) {	/* apply font changes */
		struct char_def *cdp;
		unsigned int numchar, j;
		rom_font_t *rfp;

		rfp = (rom_font_t *)kd_romfont_mod;
		numchar = rfp->fnt_numchar;
		j = 0;
		cdp = &rfp->fnt_chars[0];
		while (j < numchar) {
			from = (unchar *) cdp + kd_font_offset[fnt];
			to = (unchar *)(phystokv(CHGEN_BASE) +
			    cdp->cd_index * 0x20);

			for (i = bpc; i != 0; i--)
				*to++ = *from++;
			j++;
			cdp++;
		}
	}

	outb(0x3c4, 0x02); outb(0x3c5, 0x03);
	outb(0x3c4, 0x04); outb(0x3c5, 0x02);
	outb(0x3ce, 0x04); outb(0x3cf, 0x00);
	outb(0x3ce, 0x05); outb(0x3cf, 0x10);
	outb(0x3ce, 0x06);
	if (WSCMODE(vp)->m_color)
		outb(0x3cf, 0x0e);
	else
		outb(0x3cf, 0x0a);
}

/* assumes called by active channel */

void
kdv_shiftset(vp, dir)
vidstate_t	*vp;
int dir;
{

	if (ATYPE(Kdws, MCAP_COLOR) && VTYPE(V400)) {
		if (dir)
			Vdc.v_mode2sel |= M_ALTCHAR;
		else
			Vdc.v_mode2sel &= ~M_ALTCHAR;
		outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);
	} else if (ATYPE(Kdws, MCAP_EGA)) {
		if (dir) {
			out_reg(&kd_regtab[I_SEQ], 3, 0x04);
		} else {
			out_reg(&kd_regtab[I_SEQ], 3, 0x00);
		}
	}
}

/*
 *
 */

void
kdv_setuline(vp, on)
vidstate_t *vp;
int on;
{
	unchar	tmp, tmp_pr5;

	if (on)
		Vdc.v_mode2sel |= M_UNDRLINE;
	else
		Vdc.v_mode2sel &= ~M_UNDRLINE;
	switch (Vdc.v_type) {
	case V750:
		(void) inb(0x3d8);
		(void) inb(0x3d8);
		outb(0x3de, 5);
		break;
	case V600:
		tmp_pr5 = vdc_unlk600();
		outb(0x3ce, 0xc);
		tmp = (inb(0x3cf) | 0x80);
		outb(0x3ce, 0xc);
		outb(0x3cf, tmp);
		outb(0x3ce, 0xf);
		outb(0x3cf, tmp_pr5);
		break;

	case CAS2:
		(void) inb(vp->v_regaddr + IN_STAT_1);
		outb(0x3c0, 0x01);	/* change blue attribute palette reg */
		outb(0x3c0, (on) ? 0x07 : 0x01);
		outb(0x3c0, 0x20);	/* turn palette back on */
		break;

	default:
		break;
	}

	if ((!Vdc.v_type && (DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)))
#ifdef EVGA
	    || (Vdc.v_type == VEVGA)
#endif /* EVGA */
	   ) {

		/*
		 * Either display is unknown and it's of type EGA or VGA,
		 * or it's an EVGA.
		 */

		(void) inb(vp->v_regaddr + IN_STAT_1);
		outb(0x3c0, 0x01);	/* change blue attribute palette reg */
		outb(0x3c0, (on) ? 0x07 : 0x01);
		outb(0x3c0, 0x20);	/* turn palette back on */
	}

	outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);
}

/*
 *
 */

void
kdv_mvuline(vp, up)
vidstate_t	*vp;
int up;
{
	if (VTYPE(V750) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
		if (VTYPE(V750))
			vdc_lktime(0);
		outb(vp->v_regaddr, 0x14); /* CRTC index register */

		switch (vp->v_cvmode) {
		case DM_B40x25:
		case DM_C40x25:
		case DM_B80x25:
		case DM_C80x25:
			outb(vp->v_regaddr + 1, up? 0x07: 0x08); /* data reg. */
			break;
		default:
			outb(vp->v_regaddr + 1, up? 0x0d: 0x11); /* data reg. */
		}
		if (VTYPE(V750))
			vdc_lktime(1);
	}
}

/*
 * enable or disable white char underline feature of AT&T video cards
 */

int
kdv_undattrset(wsp, chp, curattrp, param)
wstation_t *wsp;
channel_t *chp;
ushort	*curattrp;
short	param;
{
	register int activeflag;
	channel_t *achp;			/* active channel */
	termstate_t *tsp = &chp->ch_tstate;
	vidstate_t *vp = &chp->ch_vstate;
	unsigned char tmp;

	achp = ws_activechan(wsp);
	activeflag = (chp == achp);
	if (DTYPE(Kdws, KD_VGA) && vp->v_regaddr == MONO_REGBASE)
		return (0);
	switch (param) {
	case 4:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
			if (vp->v_undattr == UNDERLINE) {
				if (activeflag) {
					kdv_setuline(vp, 1);
					kdv_mvuline(vp, 1);
				}
				*curattrp |= vp->v_undattr;
			}
			else
				*curattrp = tsp->t_curattr;
		} else if (DTYPE(Kdws, KD_CGA))
			*curattrp = tsp->t_curattr;
		break;
	case 5:
	case 6:
		/*
		 * 5 and 6 toggle between blink and bright background
		 */
		if (param == 5)
			tsp->t_flags &= ~T_BACKBRITE;
		else
			tsp->t_flags |= T_BACKBRITE;
		if (activeflag) {
			(void) inb(vp->v_regaddr + IN_STAT_1);
			outb(0x3c0, 0x10); /* attribute mode control register */
			if (DTYPE(Kdws, KD_VGA))
				tmp = inb(0x3c1);
			else
				tmp = 0x00 | WSCMODE(vp)->m_color << 1;
			if (param == 5)
				outb(0x3c0, (tmp | 0x08));
			else
				outb(0x3c0, (tmp & ~0x08));
			outb(0x3c0, 0x20);	/* turn palette back on */
		}
		break;
	case 38:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
			tsp->t_attrmskp[34].attr = 0x03;
			vp->v_undattr = UNDERLINE;
		}
		break;
	case 39:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) { /* disable underline */
			tsp->t_attrmskp[34].attr = 0x01;
			vp->v_undattr = tsp->t_curattr;
			if (activeflag) {
				kdv_setuline(vp, 0);
				kdv_mvuline(vp, 0);
			}
		}
		break;
	}
	return (0);
}

/*
 *
 */

/*ARGSUSED*/
int
kdv_disptype(qp, mp, iocp, chp, arg, copyflag)
queue_t	*qp;
mblk_t	*mp;
struct iocblk	*iocp;
channel_t	*chp;
int	arg;
int	*copyflag;
{
	vidstate_t	*vp = &chp->ch_vstate;
	struct kd_disparam	disp;
	mblk_t	*tmp;

	disp.type = vp->v_type;
	disp.addr = (char *)vp->v_rscr;
	bcopy((caddr_t)vp->v_ioaddrs, (caddr_t)disp.ioaddr,
				MKDIOADDR * sizeof (ushort));
	if (!(tmp = allocb(sizeof (struct kd_disparam), BPRI_MED))) {
		cmn_err(CE_NOTE, "!kdv_disptype: can't get msg for reply to VT_OPENQRY");
		ws_iocnack(qp, mp, iocp, ENOMEM);
	}
	*(struct kd_disparam *)tmp->b_rptr = disp;
	tmp->b_wptr += sizeof (struct kd_disparam);
	ws_copyout(qp, mp, tmp, sizeof (struct kd_disparam));
	*copyflag = 1;
	return (0);
}

/*
 *
 */

int
kdv_colordisp(void)
{
	return (!(DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)));
}

/*
 *
 */

int
kdv_xenixctlr(void)
{
	return ((int)kd_adtconv[Kdws.w_vstate.v_type]);
}

/*
 *
 */

int
kdv_xenixmode(chp, cmd, rvalp)
channel_t	*chp;
int	cmd,
	*rvalp;
{
	vidstate_t	*vp = &chp->ch_vstate;

	switch (cmd) {
	case MCA_GET:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return (EINVAL);
		break;
	case CGA_GET:
		if (!DTYPE(Kdws, KD_CGA))
			return (EINVAL);
		break;
	case EGA_GET:
		if (!DTYPE(Kdws, KD_EGA))
			return (EINVAL);
		break;
	case VGA_GET:
		if (!DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;
	case CONS_GET:
		break;
	default:
		return (EINVAL);
	}
	*rvalp = vp->v_cvmode;
	if (*rvalp == DM_EGAMONO80x25 &&
	    (DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)))
		*rvalp = M_MCA_MODE;	/* Xenix equivalent of mode */
	else if (*rvalp == DM_ENH_B80x43 || *rvalp == DM_ENH_C80x43)
		*rvalp = *rvalp + OFFSET_80x43;	/* Xenix equivalent */
	return (0);
}

/*
 *
 */

int
kdv_sborder(chp, arg)
channel_t	*chp;
long	arg;
{
	vidstate_t	*vp = &chp->ch_vstate;

	if (chp->ch_dmode == KD_GRAPHICS || DTYPE(Kdws, KD_CGA))
		return (ENXIO);
	if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) || VTYPE(V750))
		return (ENXIO);
	if (chp == ws_activechan(&Kdws)) {
		(void) inb(vp->v_regaddr + IN_STAT_1);
		out_reg(&kd_regtab[I_ATTR], 0x11, (char)arg);
		outb(kd_regtab[I_ATTR].ri_address, PALETTE_ENABLE);
	}
	vp->v_border = (unchar)arg;
	return (0);
}

/*
 *
 */

void
kdv_scrxfer(chp, dir)
channel_t	*chp;
int	dir;
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	int	tmp;
	ushort	avail, scrmsk, *srcp, *dstp;

	scrmsk = vp->v_scrmsk;
	avail = scrmsk - (tsp->t_origin & scrmsk) + 1;
	if (dir == KD_SCRTOBUF) {
		srcp = vp->v_scrp;
		dstp = *(Kdws.w_scrbufpp + chp->ch_id);
	} else {	/* KD_BUFTOSCR */
		srcp = *(Kdws.w_scrbufpp + chp->ch_id);
		dstp = vp->v_scrp;
	}
	if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750))
		kdv_disp(0);
	if (tsp->t_scrsz > avail) {	/* we must wrap, copy first chunk */
		kdv_xfer(srcp, dstp, avail, DOWN);
		srcp = vp->v_scrp;
		dstp += avail;
		avail = tsp->t_scrsz - avail;
	} else
		avail = tsp->t_scrsz;
	kdv_xfer(srcp, dstp, avail, DOWN);
	if (dir != KD_SCRTOBUF)	{	/* clean the screen */
		tmp = KD_MAXSCRSIZE;
		srcp = *(Kdws.w_scrbufpp + chp->ch_id);
		while (--tmp)
			*srcp = (tsp->t_curattr << 8 | ' ');
	}
	if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750))
		kdv_disp(1);
}

/*
 *
 */

void
kdv_textmode(chp)
channel_t	*chp;
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	int	noscreen;
	unchar	newmode;

	noscreen = !WSCMODE(vp)->m_font;	/* no screen to restore? */
	newmode = noscreen ? Kdws.w_vstate.v_dvmode : vp->v_cvmode;
#ifdef EVGA
	evga_ext_rest(cur_mode_is_evga);
#endif
	kdv_setdisp(chp, vp, tsp, newmode);
#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif
	if (noscreen)
		kdclrscr(chp, 0, tsp->t_scrsz);
	else if (chp == ws_activechan(&Kdws)) {
		kdv_scrxfer(chp, KD_BUFTOSCR);
		kdsetcursor(chp, tsp);
	}
}

/*
 *
 */

void
kdv_text1(chp)
channel_t	*chp;
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	register int	s;
	ushort	cursor, origin, port, row, col;

	port = vp->v_regaddr;
	/* read the current cursor position */
	s = clear_int_flag();
	outb(port, R_CURADRH);
	cursor = (inb(port + DATA_REG) << 8);
	outb(port, R_CURADRL);
	cursor += (inb(port + DATA_REG) & 0xff);
	/* read the current origin */
	if (DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
		outb(port, R_STARTADRH);
		origin = (inb(port + DATA_REG) << 8);
		outb(port, R_STARTADRL);
		origin += (inb(port + DATA_REG) & 0xff);
	} else
		origin = 0;
	restore_int_flag(s);
	/* calculate the cooresponding tcl row and column representation */
	row = ((ushort) (cursor - origin) / tsp->t_cols);
	col = cursor - origin - (row * tsp->t_cols);
	/* update the tcl */
	tsp->t_row = row;
	tsp->t_col = col;
	tsp->t_cursor = cursor;
	tsp->t_origin = origin;
	vp->v_font = 0;

#ifdef EVGA
	evga_ext_rest(cur_mode_is_evga);
#endif	/* EVGA */

	kdv_setdisp(chp, vp, tsp, vp->v_cvmode);

#ifdef EVGA
	/*
	 * New mode shouldn't be evga but have this
	 * here in case any evga text modes are ever added.
	 */
	evga_ext_init(vp->v_cvmode);
#endif

	kdsetcursor(chp, tsp);
	kdv_enable(vp);
}

unchar goodspeedstar64[] = {
    0x50, 0x18, 0x10, 0x00, 0x10, 0x00, 0x03, 0x00,
    0x02, 0x67, 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81,
    0xbf, 0x1f, 0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00,
    0x00, 0x00, 0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96,
    0xb9, 0xa3, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x14, 0x07, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
    0x3d, 0x3e, 0x3f, 0x0c, 0x00, 0x0f, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00, 0xff
};


struct b_param *
kdv_getparam(mode)
unchar	mode;
{
	struct b_param	*initp;
	struct modeinfo *modep;
	register vidstate_t *vp = &Kdws.w_vstate;

	modep = WSMODE(vp, mode);
	switch (modep->m_params) {
	case KD_BIOS:
		/* locate the video mode parameter table */
		if (!bcmp((char *)phystokv(ftop((ulong)0xc0000038)),
		    "SpeedStar 64\0\0\0\0\0Version 2.", 27)) {
			/*
			 * appears to be a "broken" ss64, in that 
			 * the VMP information is incorrect.  In-
			 * stead, use the timings from the 1.2 ver-
			 * sion of the card.
			 */
			initp = (struct b_param *)&goodspeedstar64;
		} else
			initp = (struct b_param *)
			    (phystokv(ftop((ulong)*vp->v_parampp)))
			    + modep->m_offset;
		break;
	case KD_CAS2:
		/* locate the extended video mode standard parameter table */
		initp = (struct b_param *)
		    (phystokv(ftop((ulong)*(vp->v_parampp - 0x3)))) +
		    modep->m_offset;
		break;
	default:	/* KD_TBLE */
		initp = &kd_inittab[modep->m_offset];
		break;
	}
	return (initp);
}

/*
 * State to pass from kdv_setxenixfont() to kdv_setxenixfont_bottom().
 */
static struct sxf_state {		/* SetXenixFont_state */
	int font, bpc, size;
	unsigned char *oldfont, *curfont;
	vidstate_t *vp;
} sxf_state;

/*ARGSUSED*/
int
kdv_setxenixfont(qp, mp, chp, font, useraddr, copyflag)
queue_t *qp;
mblk_t *mp;
channel_t *chp;
int font;
caddr_t useraddr;
int *copyflag;
{
	int bpc;            /*  Bytes per character  */
	int size;           /*  Size of data area  */
	unchar *curfont,*oldfont;
	vidstate_t *vp = &Kdws.w_vstate;    /*  Global video information  */

	switch (font) {
	case FONT8x8:
		bpc = F8x8_BPC;
		size = F8x8_SIZE;
		curfont = vp->v_fontp[FONT8x8].f_fontp;
		break;
	case FONT8x14:
		bpc = F8x14_BPC;
		size = F8x14_SIZE;
		curfont = vp->v_fontp[FONT8x14].f_fontp;
		break;
	case FONT8x16:
		bpc = F8x16_BPC;
		size = F8x16_SIZE;
		curfont = vp->v_fontp[FONT8x16].f_fontp;
		break;
	default:
		return (EINVAL);
	}

	/*
	 * check to see if the default font is the ROM font
	 * If so, allocate space for new font.
	 */
	oldfont = curfont;
	if (kd_romfonts[font].f_fontp == curfont) {
		/* fail ioctl if useraddr is NULL -- no XENIX font to release */
		if (useraddr == NULL)
			return (EFAULT);
		curfont = kmem_alloc(size, KM_SLEEP);
	} else { /* maybe a reset to the ROM font if useraddr is NULL */
		if (useraddr == NULL) {
			curfont = kd_romfonts[font].f_fontp;
			bpc = kd_romfonts[font].f_bpc;
			size = kd_romfonts[font].f_count * bpc;
		}
	}

	/* Free current font info if useraddr is NULL */
	if (useraddr == NULL) {
		struct font_info *fp;
		fp = &vp->v_fontp[font];
		kmem_free(fp->f_fontp, fp->f_count * fp->f_bpc);
	} else {
		/*
		 * Copyin new font, but first record the state to be
		 * passed to kdv_setxenixfont_bottom()
		 */
		sxf_state.font = font;
		sxf_state.bpc = bpc;
		sxf_state.size = size;
		sxf_state.oldfont = oldfont;
		sxf_state.curfont = curfont;
		sxf_state.vp = vp;

		ws_copyin(qp, mp, useraddr, size, (mblk_t *) &sxf_state);
		*copyflag = 0;

		/*
		 * The reset of the processing will be done in
		 * kdv_setxenixfont_bottom().  As we have to wait
		 * until the response to the ws_copyin() comes back
		 * from the streams head.
		 */
	}
	return (0);
}

int
kdv_setxenixfont_bottom(queue_t *qp, mblk_t *mp, mblk_t *private)
{
	int font, bpc, size;
	size_t resid, nbyte;
	unsigned char *oldfont, *curfont, *tmpfont;
	vidstate_t *vp;
	channel_t *tchp;
	mblk_t *tmp;
	struct iocblk *iocp;
	struct sxf_state *statep;

	ASSERT(qp != NULL);
	ASSERT(mp != NULL);
	ASSERT(mp->b_cont != NULL);
	ASSERT(private != NULL);

	iocp = (struct iocblk *) mp->b_rptr;

	/*
	 * Restore the state from where we left off in kdv_setxenixfont().
	 */
	statep = (struct sxf_state *) private;
	font = statep->font;
	bpc = statep->bpc;
	size = statep->size;
	oldfont = statep->oldfont;
	curfont = statep->curfont;
	vp = statep->vp;

	/*
	 * Construct curfont from the user provided data.
	 * NB. curfont is already kmem_alloc'd size bytes.
	 */
	tmpfont = curfont;
	resid = size;
	for (tmp = mp->b_cont; tmp != NULL && resid != 0;
	     tmp = tmp->b_cont) {
		nbyte = tmp->b_wptr - tmp->b_rptr;
		bcopy((caddr_t) tmp->b_rptr, (caddr_t) tmpfont, nbyte);
		resid -= nbyte;
		tmpfont += nbyte;
	}
	freemsg(mp->b_cont);
	mp->b_cont = NULL;

	/* update global workstation vid structure */
	vp->v_fontp[font].f_fontp = curfont;
	vp->v_fontp[font].f_count = size/bpc;
	if (kd_romfont_mod)
		kdv_release_fontmap();

	if (oldfont != curfont) {
		/* update channels pointing to oldfont */
		int i;

		for (i = 0; i < WS_MAXCHAN; i++) {
			tchp = ws_getchan(&Kdws, i);
			if (!tchp)
				continue;
			if (tchp->ch_vstate.v_fontp[font].f_fontp != oldfont)
				continue;
			if (tchp == ws_activechan(&Kdws))
				continue; /* skip for the moment */
			tchp->ch_vstate.v_fontp[font].f_fontp = curfont;
			tchp->ch_vstate.v_fontp[font].f_count = size / bpc;
		}
	}

	/*
	 * Now do the active channel. Font changed for it if its font
	 * points to oldfont.
	 */

	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	if (vp->v_fontp[font].f_fontp != oldfont &&
	    vp->v_fontp[font].f_fontp != curfont) {
		ws_iocack (qp, mp, iocp);
		return (0);
	}

	/* reset font information on channel */
	vp->v_fontp[font].f_fontp = curfont;
	vp->v_fontp[font].f_count = size / bpc;

	if (WSCMODE(vp)->m_font == 0) {		/* graphics mode */
		ws_iocack (qp, mp, iocp);
		return (0);
	}

	kdv_ldfont(vp, WSCMODE(vp)->m_font);
	ws_iocack (qp, mp, iocp);
	return (0);
}

/*
 *  Function to get the font for the given font type according to XENIX ioctl
 *  interface. Note that we always get the default font, never the per-VT
 *  font.
 */
/*ARGSUSED*/
int
kdv_getxenixfont(qp, mp, chp, font, useraddr, copyflag)
queue_t *qp;
mblk_t *mp;
channel_t *chp;
int font;
caddr_t useraddr;
int *copyflag;
{
	register int size;		/*  No of bytes per character  */
	register unchar *from;		/*  Address to copy from  */
	mblk_t *tmp;
	struct iocblk *iocp = (struct iocblk *) mp->b_rptr;
	vidstate_t *vp = &Kdws.w_vstate;	/*  Video information  */
	int bpc;

	/*  Check for duff argument  */
	if (useraddr == NULL)
		return (EINVAL);

	/*  Decide where we're going to copy from and how much  */
	switch (font) {
	case FONT8x8:
	case FONT8x16:
	case FONT8x14:
		from = vp->v_fontp[font].f_fontp;
		bpc = vp->v_fontp[font].f_bpc;
		size = vp->v_fontp[font].f_count * bpc;
		break;
	default:
		return (EINVAL);
	}

	if (mp->b_cont != NULL &&
	    (mp->b_cont->b_wptr - mp->b_cont->b_rptr < sizeof (caddr_t))) {
		freemsg(mp->b_cont);		/* not big enough */
		mp->b_cont = NULL;
	}

	if (mp->b_cont == NULL &&
	    !(mp->b_cont = allocb(sizeof (caddr_t), BPRI_MED))) {
		cmn_err(CE_NOTE, "!kdv_getxenixfont: failed to allocate "
		    "msgb for useraddr");
		ws_iocnack(qp, mp, iocp, ENOMEM);
		return (ENOMEM);
	}
	*(caddr_t *) mp->b_cont->b_rptr = useraddr;
	mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof (caddr_t);

	if (!(tmp = allocb(size, BPRI_MED))) {
		cmn_err(CE_NOTE, "!kdv_getxenixfont: failed to allocate "
		    "msgb to copyout font");
		return (ENOMEM);
	}
	bcopy((caddr_t) from, (caddr_t) tmp->b_rptr, size);
	tmp->b_wptr = tmp->b_rptr + size;
	ws_copyout(qp, mp, tmp, size);
	*copyflag = 0;

	/*
	 * XXX follow overlay list and copyup appropriate overlays
	 */
	if (kd_romfont_mod) {
		int i;
		unchar *from, *to;
		rom_font_t *rfp;

		rfp = (rom_font_t *)kd_romfont_mod;
		for (i = 0; i < rfp->fnt_numchar; i++) {
			mblk_t *ovly;

			from = (unchar *)&rfp->fnt_chars[i];
			from += kd_font_offset[font];
			to = (unchar *) useraddr +
			    bpc*rfp->fnt_chars[i].cd_index;
			mp->b_cont->b_rptr = to;
			if (!(ovly = allocb(bpc, BPRI_MED))) {
				cmn_err(CE_NOTE, "!kdv_getxenixfont: failed "
					"to allocate overlay msgb");
				ws_iocnack(qp, mp, iocp, ENOMEM);
				return (ENOMEM);
			}
			*(unchar **) ovly->b_rptr = from;
			ovly->b_wptr += bpc;
			ws_copyout(qp, mp, ovly, bpc);
			*copyflag = 0;
		}
	}

	return (0);
}

/*ARGSUSED*/
int
kdv_modromfont(caddr_t buf, unsigned int numchar)
{
	int font;
	vidstate_t *vp = &Kdws.w_vstate;
	channel_t *tchp;

	if (kd_romfont_mod) {
		int size;
		caddr_t oldbuf;
		rom_font_t *rfp;

		rfp = (rom_font_t *)kd_romfont_mod;
		size = rfp->fnt_numchar * sizeof (struct char_def)
			+ sizeof (numchar);
		oldbuf = kd_romfont_mod;
		kd_romfont_mod = buf;
		kmem_free(oldbuf, size);
	}
	else
		kd_romfont_mod = buf;

	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	font = WSCMODE(vp)->m_font;
	if (font == 0) { /* graphics mode */
		return (0);
	}
	/* does VT have its own font? */
	if (vp->v_fontp[font].f_fontp != kd_romfonts[font].f_fontp) {
		return (0);
	}

	kdv_ldfont(vp, font);
	return (0);
}

int
kdv_release_fontmap(void)
{
	int font;
	channel_t *tchp;
	vidstate_t *vp;

	if (kd_romfont_mod) {
		int size;
		caddr_t oldbuf;
		rom_font_t *rfp;

		rfp = (rom_font_t *)kd_romfont_mod;
		size = rfp->fnt_numchar * sizeof (struct char_def) +
			sizeof (int);
		oldbuf = kd_romfont_mod;
		kd_romfont_mod = NULL;
		kmem_free(oldbuf, size);
	}
	else
		return (0);

	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	font = WSCMODE(vp)->m_font;
	if (font == 0) { /* graphics mode */
		return (0);
	}
	/* does VT have its own font? */
	if (vp->v_fontp[font].f_fontp != kd_romfonts[font].f_fontp) {
		return (0);
	}
	kdv_ldfont(vp, font);
	return (0);
}
