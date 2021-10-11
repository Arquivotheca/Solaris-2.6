/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#ident "@(#)elx_media.c	1.10	96/05/21 SMI"

#include <sys/types.h>
#include <sys/stream.h>
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include <sys/kstat.h>
#include <sys/devops.h>
#include <sys/pci.h>
#if	defined PCI_DDI_EMULATION || COMMON_IO_EMULATION
#include <sys/xpci/sunddi_2.5.h>
#else	/* PCI_DDI_EMULATION */
#include <sys/sunddi.h>
#endif	/* PCI_DDI_EMULATION */
#include <sys/gld.h>

#include <sys/elx.h>

extern	void	elx_poll_cip(elx_t *, int, int, int);
extern	void    elx_msdelay(int);
extern	int	elx_saddr(gld_mac_info_t *);
extern	void	elx_discard(gld_mac_info_t *, int);

static void
med_xx_reset(gld_mac_info_t *macinfo, int port)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
}

static void
med_lbeat_set(ddi_acc_handle_t handle, int port, int on)
{
	ushort window;
	ushort value;

	SWTCH_WINDOW(port, 4, window);

	value = DDI_INW(port + ELX_MEDIA_STATUS);
	if (on)
		value |= ELD_MEDIA_LB_ENABLE;
	else
		value &= ~ELD_MEDIA_LB_ENABLE;
	DDI_OUTW(port + ELX_MEDIA_STATUS, value);

	RESTORE_WINDOW(port, window, 4);
}

static int
med_send_test(ddi_acc_handle_t handle, int port, unchar *addr)
{
	ushort window;
	ushort len;
	ushort pad;
	unchar padding[4];
	struct ether_header eh;

	bcopy((caddr_t)addr, (caddr_t)&eh.ether_dhost, ETHERADDRL);
	bcopy((caddr_t)addr, (caddr_t)&eh.ether_shost, ETHERADDRL);
	eh.ether_type = ETHERTYPE_IP;

	len = sizeof (eh);
	pad = (4 - (len & 0x3)) & 3;

	SWTCH_WINDOW(port, 1, window);
	DDI_OUTW(port + ELX_TX_PIO, len);
	DDI_OUTW(port + ELX_TX_PIO, 0);
	DDI_REPOUTSB(port + ELX_TX_PIO, (unchar *)&eh, len);
	DDI_REPOUTSB(port + ELX_TX_PIO, padding, pad);
	RESTORE_WINDOW(port, window, 1);

	return (len);
}

static int
med_recv_test(ddi_acc_handle_t handle, int port, unchar *addr,
	ushort len, int ver)
{
	int i;
	int rv;
	ushort window;
	ushort value;
	ushort rxerr;
	ushort rxlen;
	extern ushort elx_rxetab[];

	SWTCH_WINDOW(port, 1, window);

	rv = -1;

	for (i = 0; i < 3; i++) {
		value = DDI_INW(port + ELX_RX_STATUS);

		rxlen = ELRX_GET_LEN(ver, value);
		rxerr = ELX_GET_RXERR(ver, value, port);
		rxerr = ELRX_GET_ERR(ver, rxerr);

		if ((value & ELRX_ERROR) && (rxerr != ELRX_DRIBBLE))
			break;
		else if (rxlen >= len) {
			int j;
			unchar rxaddr[ETHERADDRL];

			DDI_REPINSB(port + ELX_RX_PIO, rxaddr, ETHERADDRL);

			for (j = 0; j < ETHERADDRL; j++)
				if (rxaddr[j] != addr[j])
					break;

			if (j == ETHERADDRL)
				rv = 0;
			break;
		}
	}

	RESTORE_WINDOW(port, window, 1);

	return (rv);
}


/*
 * Check whether a given media type is active.
 */
static int
med_verify(gld_mac_info_t *macinfo, int port, int media, int *speed, int ver)
{
	int rv;
	ushort window;
	ushort value;
	ushort lbmode;
	ushort len;
	static ushort mask = ELD_MEDIA_LB_DETECT | ELD_MEDIA_AUI_DISABLE;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	SWTCH_WINDOW(port, 4, window);

	if (media == GLDM_TP || media == GLDM_FIBER) {
		/*
		 * Enable Link Beat.  For 100BTX, 100BFX, & 10BT
		 * Link Beat Detect indicates media present.
		 */
		med_lbeat_set(handle, port, 1);
		med_xx_reset(macinfo, port);
		elx_msdelay(2000);
		value = DDI_INW(port + ELX_MEDIA_STATUS);
		RESTORE_WINDOW(port, window, 4);

		if ((value & mask) == mask)
			return (0);
		else
			return (-1);

	}

	/*
	 * It's either 10B5 (AUI) or 10B2 (BNC) ...
	 * Initialize the adapter.
	 */

	med_lbeat_set(handle, port, 0);

	(void) elx_saddr(macinfo);
	if (media == GLDM_BNC)
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_START_COAX, 0));
	else
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_STOP_COAX, 0));
	elx_msdelay(1);
	/*
	 * Set the appropriate loopback mode.
	 */
	lbmode = *speed == 10 ?  ELD_NET_EXT_LBACK : 0;
	DDI_OUTW(port + ELX_NET_DIAGNOSTIC, lbmode);
	med_xx_reset(macinfo, port);
	elx_msdelay(1);
	if (*speed == 100) {
		SET_WINDOW(port, 3);
		value = DDI_INW(port + ELX_MAC_CONTROL) | ELMC_FULL_DUPLEX;
		DDI_OUTW(port + ELX_MAC_CONTROL, value);
		med_xx_reset(macinfo, port);
		SET_WINDOW(port, 4);
		value = DDI_INW(port + ELX_PHYS_MGMT) & ~ELPM_LTEST_DEFEAT;
		DDI_OUTW(port + ELX_PHYS_MGMT, value);
	}
	/*
	 * Enable the transmitter/receiver.
	 */
	DDI_OUTW(port + ELX_COMMAND,
			COMMAND(ELC_SET_READ_ZERO, ELINTR_DEFAULT(0)));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_RX_FILTER, ELRX_IND_ADDR));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_ENABLE, 0));

	/*
	 * Transmit a frame to ourself.
	 */
	len = med_send_test(handle, port, macinfo->gldm_macaddr);
	elx_msdelay(2000);

	rv = 0;
	rv = med_recv_test(handle, port, macinfo->gldm_macaddr, len, ver);

	elx_discard(macinfo, port);

	/* clear the loopback bit */
	DDI_OUTW(port + ELX_NET_DIAGNOSTIC, 0);
	med_xx_reset(macinfo, port);

	RESTORE_WINDOW(port, window, 4);

	return (rv);
}

typedef	struct {
	ulong input;
	int output;
	int speed;
} convEntry;

/* resetOptions to internalConfig */
static convEntry roictab[] = {
	{	ELRO_MED_BT4,  ELICONF_MED_MII,		100	},
	{	ELRO_MED_BTX,  ELICONF_MED_100BTX,	100	},
	{	ELRO_MED_BFX,  ELICONF_MED_100BFX,	100	},
	{	ELRO_MED_10BT, ELICONF_MED_10BT,	10	},
	{	ELRO_MED_10B2, ELICONF_MED_10B2,	10	},
	{	ELRO_MED_AUI,  ELICONF_MED_10AUI,	10	},
	{	ELRO_MED_MII,  ELICONF_MED_MII,		0	}
};
static int roicEntries = sizeof (roictab) / sizeof (convEntry);

/* resetOptions to GLD */
static convEntry rogltab[] = {
	{	ELRO_MED_BTX,  GLDM_TP,    100	},
	{	ELRO_MED_BFX,  GLDM_FIBER, 100	},
	{	ELRO_MED_10BT, GLDM_TP,    10	},
	{	ELRO_MED_10B2, GLDM_BNC,   10	},
	{	ELRO_MED_AUI,  GLDM_AUI,   10	}
};
static int roglEntries = sizeof (rogltab) / sizeof (convEntry);

/* internalConfig to GLD */
static convEntry icgltab[] = {
	{	ELICONF_MED_10BT,   GLDM_TP,    10	},
	{	ELICONF_MED_100BTX, GLDM_TP,    100	},
	{	ELICONF_MED_100BFX, GLDM_FIBER, 100	},
	{	ELICONF_MED_10AUI,  GLDM_AUI,   10	},
	{	ELICONF_MED_10B2,   GLDM_BNC,   10	}
};
static int icglEntries = sizeof (icgltab) / sizeof (convEntry);

/*
 * Media type conversion routine.
 */
static int
med_cvt(convEntry *cvtab, int size, ulong sel, int *speed, int err)
{
	int i;

	for (i = 0; i < size; i++) {
		if (sel == cvtab[i].input) {
			if (speed)
				*speed = cvtab[i].speed;
			return (cvtab[i].output);
		}
	}

	return (err);
}

/*ARGSUSED*/
static int
med_sense1(gld_mac_info_t *macinfo, ushort mdef, int *speed)
{
	int i;
	int sel;
	int port;
	int media;
	ushort mavail;
	ushort window;
	ulong icm, icm_def;
	ulong icr;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	port = macinfo->gldm_port;

	SWTCH_WINDOW(port, 3, window);

	icr = DDI_INL(port + ELX_INTERNAL_CONFIG);

	/*
	 * For some reason the 3c592 does not properly return
	 * the media type when eisa_nvm is called (from elx_config).
	 * The value in mdef (elx_media) is always 0.
	 */
#ifdef ELXDEBUG
	cmn_err(CE_CONT, "med_sense1[icr:%x mdef:%x]", icr, mdef);
#endif
	icm_def = icr & ELICONF_MED_MASK;

	/*
	 * Don't autosense if user turned it off.
	 */
	if ((icr & ELICONF_AUTO_SEL) == 0) {
		media = med_cvt(icgltab, icglEntries, icm_def,
				speed, GLDM_UNKNOWN);
		RESTORE_WINDOW(port, window, 3);
		return (media);
	}

	mavail = DDI_INW(port + ELX_RESET_OPTIONS) & ELRO_MED_MASK;
	media = GLDM_UNKNOWN;

	icr &= ~(ELICONF_AUTO_SEL|ELICONF_MED_MASK);
	icm = 0;

	for (i = ELRO_MED_BTX; i < mavail; i <<= 1) {
		if ((sel = mavail & i) == 0)
			continue;

		/* test AUI before BNC */
		if (sel == ELRO_MED_10B2)
			sel = ELRO_MED_AUI & mavail;
		else if (sel == ELRO_MED_AUI)
			sel = ELRO_MED_10B2 & mavail;
		if (sel == 0)
			continue;

		if ((icm = med_cvt(roictab, roicEntries, sel, speed, 0xff))
			== 0xff)
			continue;

		/*
		 * Select media type and reset transmitter/receiver.
		 */
		SET_WINDOW(port, 3);
		DDI_OUTL(port + ELX_INTERNAL_CONFIG, icr | icm);
		med_xx_reset(macinfo, port);
		elx_msdelay(1);

		media = med_cvt(rogltab, roglEntries, sel, speed,
				GLDM_UNKNOWN);
		if (media == GLDM_UNKNOWN)
			continue;
		if (med_verify(macinfo, port, media, speed, 1) == 0)
			break;
		else
			media = GLDM_UNKNOWN;
	}

	if (media == GLDM_UNKNOWN)
		media = med_cvt(icgltab, icglEntries, icm_def, speed,
				GLDM_UNKNOWN);

	RESTORE_WINDOW(port, window, 3);

	return (media);
}

/* configControl to addressConfig */
static convEntry ccactab[] = {
	{	ELCONF_MED_10BT, ELAC_MED_10BT,	0	},
	{	ELCONF_MED_10B2, ELAC_MED_10B2,	0	},
	{	ELCONF_MED_AUI,  ELAC_MED_AUI,	0	}
};
static int ccacEntries = sizeof (ccactab) / sizeof (convEntry);

/* configControl to GLD */
static convEntry ccgltab[] = {
	{	ELCONF_MED_10BT, GLDM_TP,	0	},
	{	ELCONF_MED_10B2, GLDM_BNC,	0	},
	{	ELCONF_MED_AUI,  GLDM_AUI,	0	}
};
static int ccglEntries = sizeof (ccgltab) / sizeof (convEntry);

/* addressConfig to GLD */
static convEntry acgltab[] = {
	{	ELAC_MED_10BT, GLDM_TP,		10	},
	{	ELAC_MED_AUI,  GLDM_AUI,	10	},
	{	ELAC_MED_10B2, GLDM_BNC,	10	}
};
static int acglEntries = sizeof (acgltab) / sizeof (convEntry);

static int
med_sense0(gld_mac_info_t *macinfo, ushort mdef, int *speed, int can_sense)
{
	int i;
	int sel;
	int port;
	int media;
	ushort window;
	ushort mavail;
	ushort acr;
	ushort acm, acm_def;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	port = macinfo->gldm_port;

	SWTCH_WINDOW(port, 0, window);

	/*
	 * Extract media type from the Address Config Reg (previously
	 * set by the user with vendor supplied util).
	 */
	acr = DDI_INW(port + ELX_ADDRESS_CFG);
	acm = 0;
	acm_def = (mdef == (ushort)-1 ? acr : mdef) & ELAC_MED_MASK(0);

#ifdef ELXDEBUG
	SET_WINDOW(port, 0);
	cmn_err(CE_CONT, "med_sense0:%x %x ", acr, can_sense);
	cmn_err(CE_CONT, "mdef:%x ", acm_def);
	cmn_err(CE_CONT, "resource_cfg:%x ", DDI_INW(port+ELX_RESOURCE_CFG));
	cmn_err(CE_CONT, "cfg_ctl:%x ", DDI_INW(port+ELX_CONFIG_CTL));
	SET_WINDOW(port, 4);
	cmn_err(CE_CONT, "media_status:%x ", DDI_INW(port+ELX_MEDIA_STATUS));
	cmn_err(CE_CONT, "net_diag:%x ", DDI_INW(port+ELX_NET_DIAGNOSTIC));
	SET_WINDOW(port, 0);
#endif

	/*
	 * Don't autosense if board is not equipped to do so or
	 * if user turned it off.
	 */
	if ((can_sense == 0) || ((acr & ELAC_AUTO_SEL) == 0)) {
		media = med_cvt(acgltab, acglEntries, acm_def,
				speed, GLDM_UNKNOWN);
		RESTORE_WINDOW(port, window, 0);
		return (media);
	}

	acr &= ~ELAC_MED_MASK(0);
	mavail = DDI_INW(port + ELX_CONFIG_CTL) & ELCONF_MED_MASK;
	media = GLDM_UNKNOWN;

	*speed = 10;

	for (i = ELCONF_MED_10BT; i < mavail; i <<= 1) {
		if ((sel = mavail & i) == 0 ||
		    (acm = med_cvt(ccactab, ccacEntries, sel, NULL, 0xff))
			== 0xff)
			continue;

		med_lbeat_set(handle, port, 0);
		/*
		 * Select media type and reset transmitter/receiver.
		 */
		DDI_OUTW(port + ELX_ADDRESS_CFG, acr | acm);
		med_xx_reset(macinfo, port);
		elx_msdelay(1);

		media = med_cvt(ccgltab, ccglEntries, sel, NULL,
				GLDM_UNKNOWN);
		if (media == GLDM_UNKNOWN)
			continue;
		else if (med_verify(macinfo, port, media, speed, 0) == 0)
			break;
		else
			media = GLDM_UNKNOWN;
	}

	if (media == GLDM_UNKNOWN)
		media = med_cvt(acgltab, acglEntries, acm_def, NULL,
				GLDM_UNKNOWN);
	RESTORE_WINDOW(port, window, 0);

	return (media);
}

int
elx_med_sense(gld_mac_info_t *macinfo, ushort mdef, int sense)
{
	int speed;
	elx_t *elxp;
	int port;
	ddi_acc_handle_t handle;

	elxp = (elx_t *)macinfo->gldm_private;
	port = macinfo->gldm_port;
	handle = elxp->io_handle;

	if (NEW_ELX(elxp)) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0));
		elx_msdelay(50);
	}

	speed = 0;

	if (NEW_ELX(elxp))
		macinfo->gldm_media = med_sense1(macinfo, mdef, &speed);
	else
		macinfo->gldm_media = med_sense0(macinfo, mdef, &speed, sense);

	if (NEW_ELX(elxp)) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0));
		elx_msdelay(4);
	}

	return (speed);
}

void
elx_med_set(gld_mac_info_t *macinfo)
{
	int i;
	int port;
	int media;
	int speed;
	elx_t *elxp;
	ushort window;
	ddi_acc_handle_t handle;

	elxp = (elx_t *)macinfo->gldm_private;
	handle = elxp->io_handle;

	if ((media = macinfo->gldm_media) == GLDM_UNKNOWN ||
	    (speed = elxp->elx_speed) == 0)
		return;

	port = macinfo->gldm_port;

	/*
	 * Find the correct media type from the appropriate config
	 * register to gld translation table and set the media type.
	 */
	if (NEW_ELX(elxp)) {
		ulong icr, icm;

		for (i = 0; i < icglEntries; i++)
			if (media == icgltab[i].output && speed ==
			    icgltab[i].speed) {
				icm = icgltab[i].input;
				break;
			}
		SWTCH_WINDOW(port, 3, window);
		icr = DDI_INL(port + ELX_INTERNAL_CONFIG) &
			    ~(ELICONF_AUTO_SEL|ELICONF_MED_MASK);
		DDI_OUTL(port + ELX_INTERNAL_CONFIG, icr | icm);
		med_xx_reset(macinfo, port);
		elx_msdelay(1);
	} else {
		ushort acr, acm;

		for (i = 0; i < acglEntries; i++)
			if (media == acgltab[i].output) {
				acm = acgltab[i].input;
				break;
			}
		SWTCH_WINDOW(port, 0, window);
		acr = DDI_INW(port + ELX_ADDRESS_CFG);
		acr &= ~ELAC_MED_MASK(0);
		DDI_OUTW(port + ELX_ADDRESS_CFG, acr | acm);
	}

	switch (media) {
	case GLDM_TP: {
		ushort value;
		ushort jabber;

		if (elxp->elx_speed == 10)
			jabber = ELD_MEDIA_JABBER_ENB;
		else
			jabber = 0;

		SET_WINDOW(port, 4);
		value = DDI_INW(port + ELX_MEDIA_STATUS);
		if (elxp->elx_softinfo & ELS_LINKBEATDISABLE)
			value |= jabber;
		else
			value |= (ELD_MEDIA_LB_ENABLE | jabber);
		DDI_OUTW(port + ELX_MEDIA_STATUS, value);
		break;
	}
	case GLDM_BNC:
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_START_COAX, 0));
		break;
	default:
		break;
	}

	elx_msdelay(1);


#ifdef ELXDEBUG
	SET_WINDOW(port, 4);
	cmn_err(CE_CONT, "med_set:%x ", DDI_INW(port+ELX_MEDIA_STATUS));
#endif
	SET_WINDOW(port, window);
}
