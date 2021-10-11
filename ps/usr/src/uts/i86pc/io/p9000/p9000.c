/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)p9000.c	1.13	96/07/30 SMI"

/*
 * P9000 theory of operation:
 *
 * Most P9000 operations are done by mapping the P9000 components into
 * user process memory.  User processes that share mappings (typically
 * pixrect programs) must cooperate among themselves to prevent damaging
 * the state of the P9000  User processes may also acquire private
 * mappings (MAP_PRIVATE flag to mmap(2)), in which case the P9000
 * driver will preserve device state for each mapping.
 *
 * The Weitek P9000 maps in 4 megabytes into the address space.
 * The first 2 megabytes are the P9000 registers.  The second
 * 2 megabytes are the frame buffer.
 *
 * Mappings to the registers may be MAP_PRIVATE, in which case
 * the driver keeps a per-context copy of the fbc and tec registers in
 * local memory.  Only one context at a time may have valid mappings.
 * If a process tries to access the registers through an invalid
 * mapping, the driver in invoked to swap register state and validate
 * and validate the mappings.
 *
 *
 * The P9000 driver also provides a "lockpage" mechanism.  A requesting
 * process uses the GRABPAGEALLOC ioctl to create a lock page.  The ioctl
 * returns a device offset unique to that lock page.  Any process may then
 * map that offset twice in order to get lock and unlock mappings.  When a
 * process writes a '1' to its lock mapping, it acquires a lock and no
 * other process may access the lock page -- if they try they will get a
 * page fault and go to sleep.  When the locking process writes a '0' to
 * the unlock mapping, the driver wakes up any sleeping process(es).
 *
 *
 * Finally, processes have the option of mapping the "vertical retrace
 * page".  This is a page in shared memory containing a 32-bit integer
 * that is incremented each time a vertical retrace interrupt occurs.  It
 * is used so that programs may synchronize themselves with vertical
 * retrace.
 */
 
#define ALWAYS_NO_INTERRUPTS 0

#define CDELAY(c, n)    \
{ \
        register int N = n; \
        while (--N > 0) { \
                if (c) \
                        break; \
                drv_usecwait(1); \
        } \
}

/*
 * Weitek P9000 8 bit color frame buffer driver
 */


#include <sys/debug.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/cred.h>
#include <sys/open.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/devops.h>
#include <sys/sunddi.h>
/* #include <sys/sysmacros.h> */
#include <sys/modctl.h>
#include <sys/pci.h>

#include <sys/visual_io.h>
#include <sys/fbio.h>
#include "sys/p9000reg.h"
#include "sys/bt485reg.h"

#define BT485_BASE	0x03c8
#define BT485_A0	0x0001
#define BT485_A1	-0x0002
#define BT485_A2	0x4000
#define BT485_A3	0x8000

#include "sys/viperreg.h"
#include "sys/viperio.h"

/* configuration options */

#ifndef lint
char _depends_on[] = "misc/seg_mapdev";
#endif /* lint */

#define P9000DEBUG	0
#if P9000DEBUG >= 1
#define	ASSERT2(e) \
{\
    if (!(e)) \
	cmn_err(CE_NOTE, \
		"p9000: assertion failed \"%s\", line %d\n", #e, __LINE__); }
#else
#define	ASSERT2(e)		/* nothing */
#endif

#if P9000DEBUG >= 2
int	p9000_debug = P9000DEBUG;

#define	DEBUGF(level, args) \
    { if (p9000_debug >= (level)) cmn_err args; }
#define	DUMP_SEGS(level, s, c) \
    { if (p9000_debug == (level)) dump_segs(s, c); }
#else
#define	DEBUGF(level, args)	/* nothing */
#define	DUMP_SEGS(level, s, c)	/* nothing */
#endif

/* Data access requirements. */
#ifdef  _LITTLE_ENDIAN
static struct ddi_device_acc_attr endian_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_STRUCTURE_LE_ACC,
	DDI_STRICTORDER_ACC
};
#else /* _LITTLE_ENDIAN */
static struct ddi_device_acc_attr endian_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_STRUCTURE_BE_ACC,
	DDI_STRICTORDER_ACC
};
#endif /* _LITTLE_ENDIAN */

static struct ddi_device_acc_attr nosw_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STRICTORDER_ACC
};

#define	getprop(devi, name, def)	\
		ddi_getprop(DDI_DEV_T_ANY, (devi), \
		DDI_PROP_DONTPASS, (name), (def))

/* config info */

static int	p9000_open(dev_t *, int, int, cred_t *);
static int	p9000_close(dev_t, int, int, cred_t *);
static int	p9000_ioctl(dev_t, int, int, int, cred_t *, int *);
static int	p9000_mmap(dev_t, off_t, int);
static int	p9000_segmap(dev_t, off_t, struct as *, caddr_t *, off_t,
				u_int, u_int, u_int, cred_t *);

static struct cb_ops p9000_cb_ops = {
    p9000_open,		/* open */
    p9000_close,		/* close */
    nodev,		/* strategy */
    nodev,		/* print */
    nodev,		/* dump */
    nodev,	 	/* read */
    nodev,		/* write */
    p9000_ioctl,		/* ioctl */
    nodev,		/* devmap */
    p9000_mmap,		/* mmap */
    p9000_segmap,	/* segmap */
    nochpoll,		/* poll */
    ddi_prop_op,	/* cb_prop_op */
    0,			/* streamtab  */
    D_NEW | D_MP	/* Driver compatibility flag */
};

static int	p9000_info(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int	p9000_identify(dev_info_t *);
static int	p9000_attach(dev_info_t *, ddi_attach_cmd_t);
static int	p9000_detach(dev_info_t *, ddi_detach_cmd_t);
static int	p9000_probe(dev_info_t *);
#ifdef POWER
static int	p9000_power(dev_info_t *, int, int);
#endif

static struct dev_ops p9000_ops = {
    DEVO_REV,			/* devo_rev, */
    0,				/* refcnt  */
    p9000_info,			/* info */
    p9000_identify,		/* identify */
    p9000_probe,			/* probe */
    p9000_attach,		/* attach */
    p9000_detach,		/* detach */
    nodev,			/* reset */
    &p9000_cb_ops,		/* driver operations */
    (struct bus_ops *) 0,	/* bus operations */
#ifdef POWER
    p9000_power,			/* power operations */
#else
    nulldev,
#endif
};

static struct p9000_map_pvt {
    struct p9000_softc	*softc;
    ddi_mapdev_handle_t handle;		/* handle of mapdev object	*/
    u_int		offset;		/* starting offset of this map	*/
    u_int		len;		/* length of this map		*/
    struct p9000_cntxt	*context;	/* associated context		*/
    struct p9000_map_pvt	*next;		/* List of associated pvt's for
					 * this context
					 */
};

#define P9000_CMAP_ENTRIES	256
#define P9000_CURSOR_ENTRIES	2

/*
 * Per-context info:
 *	many registers in the tec and fbc do
 *	not need to be saved/restored.
 */
struct p9000_cntxt {
    struct p9000_cntxt	*link;
    struct p9000_map_pvt	*p9000_ctx_pvt;		/* List of associated pvt's */
						/* for this context */
    int		pid;				/* owner of mapping */
    int		p9000_ctx_flag;
    u_long	p9000_ctx_interrupt;		/* 0x000z0008 rw */
    u_long	p9000_ctx_interrupt_en;		/* 0x000z000c rw */
    ulong_t	p9000_ctx_cindex;		/* 0x000s018c rw */
    long	p9000_ctx_w_off_xy;		/* 0x000s0190 rw */
    ulong_t	p9000_ctx_fground;		/* 0x000s0200 rw */
    ulong_t	p9000_ctx_bground;		/* 0x000s0204 rw */
    ulong_t	p9000_ctx_pmask;			/* 0x000s0208 rw */
    ulong_t	p9000_ctx_draw_mode;		/* 0x000s020c rw */
    long	p9000_ctx_pat_originx;		/* 0x000s0210 rw */
    long	p9000_ctx_pat_originy;		/* 0x000s0214 rw */
    ulong_t	p9000_ctx_raster;		/* 0x000s0218 rw */
    ulong_t	p9000_ctx_pixel8_reg;		/* 0x000s021c rw */
    long	p9000_ctx_w_min;			/* 0x000s0220 rw */
    long	p9000_ctx_w_max;			/* 0x000s0224 rw */
    ulong_t	p9000_ctx_pattern[8];		/* 0x000s0280 rw */
    long	p9000_ctx_x0;			/* 0x000s1008 rw */
    long	p9000_ctx_y0;			/* 0x000s1010 rw */
    long	p9000_ctx_x1;			/* 0x000s1048 rw */
    long	p9000_ctx_y1;			/* 0x000s1050 rw */
    long	p9000_ctx_x2;			/* 0x000s1088 rw */
    long	p9000_ctx_y2;			/* 0x000s1090 rw */
    long	p9000_ctx_x3;			/* 0x000s10c8 rw */
    long	p9000_ctx_y3;			/* 0x000s10d0 rw */
};

/* per-unit data */
struct p9000_softc {
    int		w;
    int		h;
    int		size;		/* total size of frame buffer */
    kmutex_t	interlock;	/* interrupt locking */
    p9000_t	p9000;		/* p9000 hardware mapping structure */
    ddi_acc_handle_t	p9000_dr_handle;		/* handle to regs */
    ddi_acc_handle_t	p9000_cr_handle;		/* handle to regs */
    ddi_acc_handle_t	p9000_fb_handle;		/* handle to regs */
    ddi_acc_handle_t	p9000_bt_handle;		/* handle to regs */
    ddi_acc_handle_t	p9000_seq_handle;	/* handle to regs */
    u_char *	ndvram;		/* for non display VRAM while suspended */
    int		ndvramsz;	/* size of non display VRAM */
    
    struct softcur {
	short	enable;		/* cursor enable */
	short	pad1;
	struct fbcurpos	pos;	/* cursor position */
	struct fbcurpos	hot;	/* cursor hot spot */
	struct fbcurpos	size;	/* cursor bitmap size */
	u_long	image[32];	/* cursor image bitmap */
	u_long	mask[32];	/* cursor mask bitmap */
    } cur;
    
    u_char	omap[P9000_CURSOR_ENTRIES][3];
    u_char	cmap[P9000_CMAP_ENTRIES][3];/* shadow color map */
    u_short	omap_update;		/* overlay colormap update flag */
    
    u_short	cmap_index;		/* colormap update index */
    u_short	cmap_count;		/* colormap update count */
    
#define P9000_VRTIOCTL	1		/* FBIOVERTICAL in effect */
#define P9000_VRTCTR	2		/* OWGX vertical retrace counter */

    int		*vrtpage;		/* pointer to VRT page */
    int		*vrtalloc;		/* pointer to VRT allocation */
    int		vrtmaps;		/* number of VRT page maps */
    int		vrtflag;		/* vrt interrupt flag */
    struct p9000_cntxt	*curctx;	/* context switching */
    struct p9000_cntxt	shared_ctx;	/* shared context */
    struct p9000_cntxt	*pvt_ctx;	/* list of non-shared contexts */
    int		pci_rev;		/* bios revision # */
    int		emulation;		/* emulation type */
    dev_info_t	*devi;			/* back pointer */
    int		have_intr;		/* do interrupts work */
    ddi_iblock_cookie_t	iblock_cookie;	/* block interrupts */
    kmutex_t	mutex;			/* mutex locking */
    kcondvar_t	vrtsleep;		/* for sleeping */
    int		suspended;		/* true if driver is suspended */
    viper_port_t	viperport;
    viper_init_t	viperinit;
    int         viperoutctrl;           /* saved copy of outctrl port */
    int		bus_type;
    struct	vis_identifier p9000_ident;
};

static int	p9000_map_access(ddi_mapdev_handle_t, void *, off_t);
static void	p9000_map_free(ddi_mapdev_handle_t, void *);
static int	p9000_map_dup(ddi_mapdev_handle_t, void *, 
				ddi_mapdev_handle_t, void **);

static struct ddi_mapdev_ctl p9000_map_ops = {
    MAPDEV_REV,		/* mapdev_ops version number	*/
    p9000_map_access,	/* mapdev access routine	*/
    p9000_map_free,	/* mapdev free routine		*/
    p9000_map_dup	/* mapdev dup routine		*/
};

static void	*p9000_softc_head;
static u_int	p9000_pagesize;
#if ALWAYS_NO_INTERRUPTS
static u_int	p9000_pageoffset;
#endif

/* default structure for FBIOGATTR ioctl */
static struct fbgattr	p9000_attr;

/*
 * handy macros
 */

#define getsoftc(unit) \
    ((struct p9000_softc *)ddi_get_soft_state(p9000_softc_head, (unit)))

#define	btob(n)			ctob(btoc(n))

#define P9000_WAIT      500000  /* .5 seconds */

#define	p9000_int_enable(p9000)	P9000_SET_VBLANK_INTR(p9000)

#define p9000_int_disable_intr(p9000)	P9000_STOP_VBLANK_INTR(p9000)

#define p9000_int_clear(p9000)	P9000_CLEAR_VBLANK_INTR(p9000)

/* check if color map update is pending */
#define	p9000_update_pending(softc) \
    ((softc)->cmap_count || (softc)->omap_update)

/*
 * forward references
 */
static u_int	p9000_intr(caddr_t);
static void	p9000_reset_cmap(u_char *, register u_int);
static void	p9000_update_cmap(struct p9000_softc *, u_int, u_int);
static void	p9000_update_omap(struct p9000_softc *);
static void     p9000_cmap_copyout (u_char *, u_char *, u_int);
static void     p9000_cmap_copyin (u_char *, u_char *, u_int);
#ifdef POWER
static void	p9000_set_sync(struct p9000_softc *, int);
#endif
static void	p9000_set_video(struct p9000_softc *, int);
static void	p9000_reset(struct p9000_softc *);
static int	p9000_cntxsave(p9000p_t, struct p9000_cntxt *);
static int	p9000_cntxrestore(p9000p_t, struct p9000_cntxt *);
static struct	p9000_cntxt *p9000_ctx_map_insert(struct p9000_softc *, int);
static int      p9000_getpid(void);

static int	viper_vlb_probe(dev_info_t *);
static int	viper_enable(register ddi_acc_handle_t,
		    p9000p_t, viper_port_t *, int *);
static int	viper_disable(register ddi_acc_handle_t,
		    viper_port_t *, int);
static int	viper_ioctl(dev_t, int, int, int, cred_t *, int *);
static void	viper_setcurpos(struct p9000_softc *);
static void	viper_setcurshape(struct p9000_softc *);
static void	viper_setcurcolor(struct p9000_softc *);
static void	viper_setpalet(struct p9000_softc *);
static int	viper_enter_graph_mode(struct p9000_softc *);
static int	viper_enter_text_mode(struct p9000_softc *);
static int      viper_unlock_regs(register ddi_acc_handle_t,
			viper_port_t *);
static int      viper_lock_regs(register ddi_acc_handle_t,
			viper_port_t *);
static void	viper_write_ic2061a(struct p9000_softc *, u_long);

/* Loadable Driver stuff */

extern struct mod_ops	mod_driverops;

static struct modldrv	p9000_modldrv = {
    &mod_driverops,		/* Type of module.  This one is a driver */
    "weitek p9000 driver v95/07/24",	/* Name of the module. */
    &p9000_ops,			/* driver ops */
};

static struct modlinkage	p9000_modlinkage = {
    MODREV_1,
    (void *) &p9000_modldrv,
    NULL,
};


static unsigned char const p9000_bitflip[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


int
_init(void)
{
    register int	e;
    
    DEBUGF(1, (CE_CONT, "p9000: compiled %s, %s\n", __TIME__, __DATE__));
    
    if ((e = ddi_soft_state_init(&p9000_softc_head,
				 sizeof (struct p9000_softc), 1)) != 0) {
	DEBUGF(1, (CE_CONT, "done\n"));
	return (e);
    }
    
    e = mod_install(&p9000_modlinkage);
    
    if (e) {
	ddi_soft_state_fini(&p9000_softc_head);
	DEBUGF(1, (CE_CONT, "done\n"));
    }
    DEBUGF(1, (CE_CONT, "p9000: _init done rtn=%d\n", e));
    return (e);
}

int
_fini(void)
{
    register int	e;

    if ((e = mod_remove(&p9000_modlinkage)) != 0)
	return (e);

    ddi_soft_state_fini(&p9000_softc_head);

    return (0);
}

int
_info(struct modinfo *const modinfop)
{
    return (mod_info(&p9000_modlinkage, modinfop));
}

static
p9000_identify(register dev_info_t *const devi)
{
    register char *const	name = ddi_get_name(devi);

    if (strncmp(name, "p9000", 5) == 0 ||
	strncmp(name, "SUNW,p9000", 10) == 0 ||
	strncmp(name, "pci", 3) == 0)
	return (DDI_IDENTIFIED);
    else
	return (DDI_NOT_IDENTIFIED);
}

static int
p9000_probe(register dev_info_t *const devi)
{
    DEBUGF(1, (CE_CONT, "p9000_probe (%s) unit=%d\n",
	       ddi_get_name(devi), ddi_get_instance(devi)));

    if (ddi_dev_is_sid(devi) == DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT, "p9000_probe dev is sid\n"));
	return (DDI_PROBE_DONTCARE);
    }
    return(viper_vlb_probe(devi));
}


/*
 * search the entries of the "reg" property for one which has
 * both the nonrelocatable bit set and the desired base address.
 */
static u_int
get_reg_index(dev_info_t *const devi, caddr_t addrp)
{

    int			length, index;
    pci_regspec_t	*pcireg;
	    
    if (ddi_getlongprop(DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
	"reg", (caddr_t)&pcireg, &length) != DDI_PROP_SUCCESS) {
	DEBUGF(2, (CE_CONT, "get_reg_index: can't get reg prop\n"));
	/* take a guess */
	return (5);
    }

    for (index = 0; index < length / sizeof(pci_regspec_t); index++)
    {

	/* PCI_RELOCAT_B */
	if ((pcireg[index].pci_phys_hi & PCI_REG_REL_M) && 
	    ((pcireg[index].pci_phys_low & PCI_CONF_ADDR_MASK)
	    == (u_int)addrp))
	{
	    DEBUGF(2, (CE_CONT, "get_reg_index: 0x%x\n", index));
	    kmem_free(pcireg, (size_t)length);
	    return(index);
	}
    }
    DEBUGF(2, (CE_CONT, "get_reg_index: can't find index\n"));
    kmem_free(pcireg, (size_t)length);

    /* take a guess */
    return (5);
}

static int
p9000_attach(register dev_info_t *const devi,
	register ddi_attach_cmd_t const cmd)
{
    register struct p9000_softc	*softc;
    register int const		unit = ddi_get_instance(devi);
    p9000p_t    p9000reg;
    char	name[16];
#ifdef POWER
    int		proplen;
    ulong_t	timestamp[2];
    int		power[2];
#endif
    int         length;
    
    DEBUGF(1, (CE_CONT, "p9000_attach unit=%d cmd=%d\n", unit, (int) cmd));

    switch (cmd)
    {
    case DDI_ATTACH:
	break;

    case DDI_RESUME:
	if (!(softc = (struct p9000_softc *)ddi_get_driver_private(devi)))
	    return (DDI_FAILURE);
	if (!softc->suspended)
	    return (DDI_SUCCESS);
	mutex_enter(&softc->mutex);

	p9000_reset(softc);

	if (softc->curctx) {
	    u_int	display_size = softc->w * softc->h;

	    p9000reg = &softc->p9000;

	    /* Restore non display RAM */
	    ddi_rep_putb(softc->p9000_fb_handle, softc->ndvram,
		  ((u_char *) &p9000reg->p9000_frame_bufferp[display_size]),
		  softc->ndvramsz, DDI_DEV_AUTOINCR);
	    kmem_free(softc->ndvram, softc->ndvramsz);
	    
	    /* Restore other frame buffer state */
	    (void)p9000_cntxrestore(p9000reg, softc->curctx);
	    viper_setcurpos(softc);
	    viper_setcurshape(softc);
	    p9000_update_cmap(softc, (u_int)0, P9000_CMAP_ENTRIES);
	    p9000_update_omap(softc);

	    p9000_int_enable(p9000reg);	/* Schedule the update */
	}

	softc->suspended = 0;
	mutex_exit(&softc->mutex);
#ifdef POWER
	/* Restore brightness level */
	if (ddi_getlongprop_buf(DDI_DEV_T_ANY, devi, 0,
			"pm_norm_pwr", (caddr_t)power, &proplen)
			== DDI_PROP_SUCCESS)
	
	    if (p9000_power(devi, 1, power[1]) != DDI_SUCCESS)
		return (DDI_FAILURE);
#endif

	return (DDI_SUCCESS);

    default:
	return (DDI_FAILURE);
    }
    
    DEBUGF(1, (CE_CONT, "p9000_attach unit=%d\n", unit));
    
    p9000_pagesize = ddi_ptob(devi, 1);
#if ALWAYS_NO_INTERRUPTS
    p9000_pageoffset = p9000_pagesize - 1;
#endif
    
    /* Allocate softc struct */
    if (ddi_soft_state_zalloc(p9000_softc_head, unit))
	return(DDI_FAILURE);
    
    softc = getsoftc(unit);
    
    /* link it in */
    softc->devi = devi;

    DEBUGF(1, (CE_CONT, "p9000_attach devi=0x%x unit=%d\n", devi, unit));

    ddi_set_driver_private(devi, (caddr_t) softc);
    
    p9000reg = &softc->p9000;

    /* Check the bus, either PCI or VLB */
    length = sizeof(name);
    if (ddi_getlongprop_buf(DDI_DEV_T_NONE, ddi_get_parent(devi),
	    DDI_PROP_DONTPASS, "device_type", (caddr_t)name,
	    &length) != DDI_PROP_SUCCESS) {
	if (ddi_dev_is_sid(devi) == DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT, "p9000_attach dev is sid\n"));
	    softc->bus_type = VIPER_PORT_BUS_PCI;
	} else {
	    DEBUGF(2, (CE_CONT, "p9000_attach dev is not sid\n"));
	    softc->bus_type = VIPER_PORT_BUS_VLB;
	}
    } else if (strcmp(name, "pci") == 0) {
	softc->bus_type = VIPER_PORT_BUS_PCI;
	DEBUGF(2, (CE_CONT,
		"p9000_attach: bus type is pci\n"));
    } else {
	softc->bus_type = VIPER_PORT_BUS_VLB;
	DEBUGF(2, (CE_CONT,
		"p9000_attach: bus type is vlb\n"));
    }

    if (softc->bus_type == VIPER_PORT_BUS_PCI) {
	ddi_acc_handle_t	config_handle;	/* handle to PCI regs */

	(void)strcpy((caddr_t)&softc->p9000_ident, "SUNWp9000");
	if (pci_config_setup(devi, &config_handle)) {
	    DEBUGF(2, (CE_CONT,
		    "p9100_attach: pci_config_setup FAILED\n"));
	    return (DDI_FAILURE);
	}
	softc->pci_rev = pci_config_getb(config_handle,
	    PCI_CONF_REVID);
	pci_config_teardown(&config_handle);

	/*
	 * Map in the device registers.
	 */
	if (ddi_regs_map_setup(devi, 1,
		(caddr_t *)&p9000reg->p9000_control_regp,
		P9000_CONTROL_BASE, sizeof (control_t), &endian_attr,
		&softc->p9000_cr_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map control registers\n"));
	    return (DDI_FAILURE);
	}

	if (ddi_regs_map_setup(devi, 1,
		(caddr_t *)&p9000reg->p9000_drawing_regp,
		P9000_DRAWING_BASE, sizeof (drawing_t), &endian_attr,
		&softc->p9000_dr_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map drawing registers\n"));
	    return (DDI_FAILURE);
	}

	/* map in the framebuffer */
	if (ddi_regs_map_setup(devi, 1,
		(caddr_t *)&p9000reg->p9000_frame_bufferp,
		P9000_FRAME_BUFFER_BASE, P9000_FRAME_BUFFER_SIZE, &nosw_attr,
		&softc->p9000_fb_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map framebuffer\n"));
	    return (DDI_FAILURE);
	}

	/* map in all of the I/O space exported by the device */
	if (ddi_regs_map_setup(devi, 2,
		(caddr_t *)&p9000reg->p9000_bt485_basep,
		0, 0, &endian_attr, &softc->p9000_bt_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map 485 dac\n"));
	    return (DDI_FAILURE);
	}

	/* map in the "hard-decode" addresses for VGA */
	if (ddi_regs_map_setup(devi, get_reg_index(devi, 0x3c0),
		(caddr_t *)&p9000reg->p9000_seq_basep, 0, 0, &endian_attr,
		&softc->p9000_seq_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map VGA registers\n"));
	    return (DDI_FAILURE);
	}

	softc->viperport.vp_bustype = VIPER_PORT_BUS_PCI;

	/*
	 * VGA devices are suppose to "hard-decode" aliases of registers
	 * located in the range 0x100-0x3ff, so we should use them, but
	 * don't.
	 */
	softc->viperport.vp_bt485_ram_write = p9000reg->p9000_seq_basep +
	    (BT485_RAM_WRITE & 0xf);                /* 3c0 + 8 */
	softc->viperport.vp_bt485_palet_data = p9000reg->p9000_seq_basep +
	    (BT485_PALET_DATA & 0xf);               /* 3c0 + 9 */
	softc->viperport.vp_bt485_pixel_mask = p9000reg->p9000_seq_basep +
	    (BT485_PIXEL_MASK & 0xf);               /* 3c0 + 6 */
	softc->viperport.vp_bt485_ram_read = p9000reg->p9000_seq_basep +
	    (BT485_RAM_READ & 0xf);                 /* 3c0 + 7 */

	/* these registers are offsets from base register 1 */
	softc->viperport.vp_bt485_color_write = p9000reg->p9000_bt485_basep +
	    (0x400 | (BT485_COLOR_WRITE & 0xff));   /* base + 4c8 */
	softc->viperport.vp_bt485_color_data = p9000reg->p9000_bt485_basep +
	    (0x400 | (BT485_COLOR_DATA & 0xff));    /* base + 4c9 */
	softc->viperport.vp_bt485_comreg0 = p9000reg->p9000_bt485_basep +
	    (0x400 | (BT485_COMREG0 & 0xff));       /* base + 4c6 */
	softc->viperport.vp_bt485_color_read = p9000reg->p9000_bt485_basep +
	    (0x400 | (BT485_COLOR_READ & 0xff));    /* base + 4c7 */

	softc->viperport.vp_bt485_comreg1 = p9000reg->p9000_bt485_basep +
	    (0x800 | (BT485_COMREG1 & 0xff));       /* base + 8c8 */
	softc->viperport.vp_bt485_comreg2 = p9000reg->p9000_bt485_basep +
	    (0x800 | (BT485_COMREG2 & 0xff));       /* base + 8c9 */
	softc->viperport.vp_bt485_stat_reg = p9000reg->p9000_bt485_basep +
	    (0x800 | (BT485_STAT_REG & 0xff));      /* base + 8c6 */
	softc->viperport.vp_bt485_cursor_data = p9000reg->p9000_bt485_basep +
	    (0x800 | (BT485_CURSOR_DATA & 0xff));   /* base + 8c7 */

	softc->viperport.vp_bt485_cursor_x_low = p9000reg->p9000_bt485_basep +
	    (0xc00 | (BT485_CURSOR_X_LOW & 0xff));  /* base + cc8 */
	softc->viperport.vp_bt485_cursor_x_high =
	    p9000reg->p9000_bt485_basep +
	    (0xc00 | (BT485_CURSOR_X_HIGH & 0xff)); /* base + cc9 */
	softc->viperport.vp_bt485_cursor_y_low =
	    p9000reg->p9000_bt485_basep +
	    (0xc00 | (BT485_CURSOR_Y_LOW & 0xff));  /* base + cc6 */
	softc->viperport.vp_bt485_cursor_y_high =
	    p9000reg->p9000_bt485_basep +
	    (0xc00 | (BT485_CURSOR_Y_HIGH & 0xff)); /* base + cc7 */

	if (softc->pci_rev == 0x1)
	    goto rev_1;

	/* "hard-decode" registers in the VGA space */
	softc->viperport.vp_miscout =
	    p9000reg->p9000_seq_basep + (VIPER_MISCOUT & 0xf);
	softc->viperport.vp_miscin =
	    p9000reg->p9000_seq_basep + (VIPER_MISCIN & 0xf);
	softc->viperport.vp_seq_index_port =
	    p9000reg->p9000_seq_basep + (VIPER_SEQ_INDEX_PORT & 0xf);
	softc->viperport.vp_seq_data_port =
	    p9000reg->p9000_seq_basep + (VIPER_SEQ_DATA_PORT & 0xf);

    } else {

	/* P9000 on VLB
	 *
	 * NOTE: this assumes that VLB regspecs conform to the proposed
	 * format:
	 *
	 *  <drv-reg=i,address,size>,<drv-intr=irq>,<drv-dmachan=dmachan>,
	 *      <drv-ipl=interrupt-priority>
	 *           where i=0 for Memory space
	 *                 i=1 for I/O space
	 * viper-reg=0,0xa0000000,0x40000,
	 *           1,0x0,0x10000
	 *       viper-intr=11 viper-ipl=5
	 */

	(void)strcpy((caddr_t)&softc->p9000_ident, "SUNWp9000");

	/*
	 * Map in the device registers.
	 */
	if (ddi_regs_map_setup(devi, 4,
		(caddr_t *)&p9000reg->p9000_control_regp,
		P9000_CONTROL_BASE, sizeof (control_t), &endian_attr,
		&softc->p9000_cr_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map control registers\n"));
	    return (DDI_FAILURE);
	}

	if (ddi_regs_map_setup(devi, 4,
		(caddr_t *)&p9000reg->p9000_drawing_regp,
		P9000_DRAWING_BASE, sizeof (drawing_t), &endian_attr,
		&softc->p9000_dr_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map drawing registers\n"));
	    return (DDI_FAILURE);
	}

	if (ddi_regs_map_setup(devi, 4,
		(caddr_t *)&p9000reg->p9000_frame_bufferp,
		P9000_FRAME_BUFFER_BASE, P9000_FRAME_BUFFER_SIZE, &nosw_attr,
		&softc->p9000_fb_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map framebuffer\n"));
	    return (DDI_FAILURE);
	}

	if (ddi_regs_map_setup(devi, 1, (caddr_t *)&p9000reg->p9000_bt485_basep,
		0, 0, &endian_attr, &softc->p9000_bt_handle) != DDI_SUCCESS) {
	    DEBUGF(2, (CE_CONT,
		    "p9000_attach: couldn't map 485 dac\n"));
	    return (DDI_FAILURE);
	}

	/*
	 * In this case both the RAMDAC and SEQ share the same base
	 * which is zero.
	 */
	p9000reg->p9000_seq_basep = p9000reg->p9000_bt485_basep = 0;
	softc->p9000_seq_handle = softc->p9000_bt_handle;

	softc->viperport.vp_bustype = VIPER_PORT_BUS_VLB;

	softc->viperport.vp_bt485_ram_write = p9000reg->p9000_bt485_basep +
	    BT485_RAM_WRITE;     /*  3c8 */
	softc->viperport.vp_bt485_palet_data = p9000reg->p9000_bt485_basep +
	    BT485_PALET_DATA;    /*  3c9 */
	softc->viperport.vp_bt485_pixel_mask = p9000reg->p9000_bt485_basep +
	    BT485_PIXEL_MASK;    /*  3c6 */
	softc->viperport.vp_bt485_ram_read = p9000reg->p9000_bt485_basep +
	    BT485_RAM_READ;      /*  3c7 */
	softc->viperport.vp_bt485_color_write = p9000reg->p9000_bt485_basep +
	    BT485_COLOR_WRITE;   /* 43c8 */
	softc->viperport.vp_bt485_color_data = p9000reg->p9000_bt485_basep +
	    BT485_COLOR_DATA;    /* 43c9 */
	softc->viperport.vp_bt485_comreg0 = p9000reg->p9000_bt485_basep +
	    BT485_COMREG0;       /* 43c6 */
	softc->viperport.vp_bt485_color_read = p9000reg->p9000_bt485_basep +
	    BT485_COLOR_READ;    /* 43c7 */
	softc->viperport.vp_bt485_comreg1 = p9000reg->p9000_bt485_basep +
	    BT485_COMREG1;       /* 83c8 */
	softc->viperport.vp_bt485_comreg2 = p9000reg->p9000_bt485_basep +
	    BT485_COMREG2;       /* 83c9 */
	softc->viperport.vp_bt485_stat_reg = p9000reg->p9000_bt485_basep +
	    BT485_STAT_REG;      /* 83c6 */
	softc->viperport.vp_bt485_cursor_data = p9000reg->p9000_bt485_basep +
	    BT485_CURSOR_DATA;   /* 83c7 */
	softc->viperport.vp_bt485_cursor_x_low = p9000reg->p9000_bt485_basep +
	    BT485_CURSOR_X_LOW;  /* c3c8 */
	softc->viperport.vp_bt485_cursor_x_high = p9000reg->p9000_bt485_basep +
	    BT485_CURSOR_X_HIGH; /* c3c9 */
	softc->viperport.vp_bt485_cursor_y_low = p9000reg->p9000_bt485_basep +
	    BT485_CURSOR_Y_LOW;  /* c3c6 */
	softc->viperport.vp_bt485_cursor_y_high = p9000reg->p9000_bt485_basep +
	    BT485_CURSOR_Y_HIGH; /* c3c7 */

rev_1:
	softc->viperport.vp_miscout = p9000reg->p9000_seq_basep + VIPER_MISCOUT;
	softc->viperport.vp_miscin = p9000reg->p9000_seq_basep + VIPER_MISCIN;
	softc->viperport.vp_seq_index_port =
	    p9000reg->p9000_seq_basep + VIPER_SEQ_INDEX_PORT;
	softc->viperport.vp_seq_data_port =
	    p9000reg->p9000_seq_basep + VIPER_SEQ_DATA_PORT;

    }
    if (viper_enable(softc->p9000_seq_handle,
	    p9000reg, &softc->viperport,
	    &softc->viperoutctrl) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT, "p9000_attach: viper_enable failed\n"));
	return (DDI_FAILURE);
    }

    softc->vrtpage = NULL;
    softc->vrtalloc = NULL;
    softc->vrtmaps = 0;
    softc->vrtflag = 0;
 
    p9000_reset(softc);

#if ALWAYS_NO_INTERRUPTS
    /* attach interrupt, notice the dance... see 1102427 */
    if (ddi_add_intr(devi, 0, &softc->iblock_cookie, 0,
		    (u_int (*)()) nulldev, (caddr_t)0) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT, "p9000_attach%d add_intr failed\n", unit));
	(void) p9000_detach(devi, DDI_DETACH);
	return (DDI_FAILURE);
    }
    
    mutex_init(&softc->interlock, "p9000_interlock", MUTEX_DRIVER,
    softc->iblock_cookie);
    mutex_init(&softc->mutex, "p9000", MUTEX_DRIVER, softc->iblock_cookie);
    cv_init(&softc->vrtsleep, "p9000", CV_DRIVER, softc->iblock_cookie);
    
    ddi_remove_intr(devi, 0, softc->iblock_cookie);
    
    if (ddi_add_intr(devi, 0, &softc->iblock_cookie, 0,
		    p9000_intr, (caddr_t) softc) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT,
		    "p9000_attach%d add_intr failed\n", unit));
	(void) p9000_detach(devi, DDI_DETACH);
	return (DDI_FAILURE);
    }
    
    softc->have_intr = 1;

#else /* ALWAYS_NO_INTERRUPTS */

    softc->have_intr = 0;
    mutex_init(&softc->interlock, "p9000_interlock", MUTEX_DRIVER,
    softc->iblock_cookie);
    mutex_init(&softc->mutex, "p9000", MUTEX_DRIVER, softc->iblock_cookie);
    cv_init(&softc->vrtsleep, "p9000", CV_DRIVER, softc->iblock_cookie);

#endif /* ALWAYS_NO_INTERRUPTS */
    
    /*
     * Initialize hardware colormap and software colormap images. It might
     * make sense to read the hardware colormap here.
     */
    p9000_reset_cmap(softc->cmap[0], P9000_CMAP_ENTRIES);
    p9000_reset_cmap(softc->omap[0], P9000_CURSOR_ENTRIES);
    p9000_update_cmap(softc, (u_int) 0, P9000_CMAP_ENTRIES);
    p9000_update_omap(softc);
    
    DEBUGF(2, (CE_CONT,
	       "p9000_attach%d just before create_minor node\n", unit));

    (void)sprintf(name, "p9000_%d", unit);

    if (ddi_create_minor_node(devi, name, S_IFCHR,
			      unit, DDI_NT_DISPLAY, 0) == DDI_FAILURE) {
	ddi_remove_minor_node(devi, NULL);
	DEBUGF(2, (CE_CONT,
		   "p9000_attach%d create_minor node failed\n", unit));
	return (DDI_FAILURE);
    }

    ddi_report_dev(devi);
    
    /*
     * Initialize the shared context for this unit
     */
    softc->shared_ctx.p9000_ctx_pvt = NULL;
    softc->pvt_ctx = NULL;
    
#ifdef POWER
    /* Create power management properties */
    timestamp[0] = 0;
    drv_getparm(TIME, timestamp+1);
    (void) ddi_prop_create(DDI_DEV_T_NONE, devi, DDI_PROP_CANSLEEP,
			   "pm_timestamp", (caddr_t)timestamp,
			   sizeof (timestamp));
    power[0] = 1;
    power[1] = 255;
    (void) ddi_prop_create(DDI_DEV_T_NONE, devi, DDI_PROP_CANSLEEP,
			   "pm_norm_pwr", (caddr_t)power, sizeof (power));
#endif

    return (DDI_SUCCESS);
}


static int
p9000_detach(register dev_info_t *const devi,
	register ddi_detach_cmd_t const cmd)
{
    register int const	unit = ddi_get_instance(devi);
    register struct p9000_softc *const	softc = getsoftc(unit);
    register p9000p_t const p9000reg = &softc->p9000;

    DEBUGF(1, (CE_CONT, "p9000_detach softc=%x, devi=0x%x\n", softc, devi));

    switch (cmd) {
    case DDI_DETACH:
	break;

    case DDI_SUSPEND:
	if (softc == NULL)
	    return (DDI_FAILURE);
	if (softc->suspended)
	    return (DDI_FAILURE);
	
	mutex_enter(&softc->mutex);
	
	if (softc->curctx) {
	    register u_int  const display_size = softc->w * softc->h;

	    /* Save non display RAM */ 
	    /* 2Mb - displayable memory */
	    /* NUST be changed for other boards or worked out dynamically */
	    softc->ndvramsz = (2 * 1024 * 1024) - display_size;
	    if ((softc->ndvram = kmem_alloc(softc->ndvramsz,
					    KM_NOSLEEP)) == NULL) {
		mutex_exit(&softc->mutex);
		return (DDI_FAILURE);
	    }

	    ddi_rep_getb(softc->p9000_fb_handle, softc->ndvram,
		(u_char *) &p9000reg->p9000_frame_bufferp[display_size],
		softc->ndvramsz, DDI_DEV_AUTOINCR);
	    
	    /* Save other frame buffer state */
	    (void) p9000_cntxsave(p9000reg, softc->curctx);
	}

	softc->suspended = 1;

	mutex_exit(&softc->mutex);

	return (DDI_SUCCESS);

    default:
	return (DDI_FAILURE);
    }
    
    /* shut off video */
    
    P9000_CLEAR_ENABLE_VIDEO (p9000reg);
    
    mutex_enter(&softc->mutex);
    mutex_enter(&(softc)->interlock);

    p9000_int_disable_intr(p9000reg);

    mutex_exit(&(softc)->interlock);
    mutex_exit(&softc->mutex);
    
    if (softc->have_intr)
	ddi_remove_intr(devi, 0, softc->iblock_cookie);
    
    (void)viper_disable(softc->p9000_seq_handle,
	    &softc->viperport, softc->viperoutctrl);
    if (softc->bus_type == VIPER_PORT_BUS_PCI) {
	/* P9000 on the PCI */

	/*
	 * Unmap the device registers.
	 */
	ddi_regs_map_free(&softc->p9000_cr_handle);

	ddi_regs_map_free(&softc->p9000_dr_handle);

	/* unmap the framebuffer */
	ddi_regs_map_free(&softc->p9000_fb_handle);

	/* unmap all of the I/O space exported by the device */
	ddi_regs_map_free(&softc->p9000_bt_handle);

	/* unmap the "hard-decode" addresses for VGA */
	ddi_regs_map_free(&softc->p9000_seq_handle);

    } else {

	/*
	 * Map in the device registers.
	 */
	ddi_regs_map_free(&softc->p9000_cr_handle);

	ddi_regs_map_free(&softc->p9000_dr_handle);

	/* unmap the framebuffer */
	ddi_regs_map_free(&softc->p9000_fb_handle);

	/* unmap all of the I/O space exported by the device */
	ddi_regs_map_free(&softc->p9000_bt_handle);

	/* In this case both the RAMDAC and SEQ share the same base
	 * which is zero.
	 */
	softc->p9000_seq_handle = NULL;
    }
    
    if (softc->vrtalloc != NULL)
	kmem_free(softc->vrtalloc, p9000_pagesize * 2);
    
    mutex_destroy(&softc->mutex);
    
    cv_destroy(&softc->vrtsleep);
    
    ASSERT2(softc->curctx == NULL);
    
    ddi_soft_state_free(p9000_softc_head, unit);
    
    return (0);
}


#ifdef POWER
static int
p9000_power(dev_info_t *dip, int cmpt, int level)
{
    int		power[2];
    register struct p9000_softc	*softc;
    
    if (cmpt != 1 || 0 > level || level > 255 ||
	    !(softc = (struct p9000_softc *) ddi_get_driver_private(dip)))
	return (DDI_FAILURE);
    
    if (level) {
	/*
	p9000_set_sync(softc, FBVIDEO_ON);
	 */
	p9000_set_video(softc, FBVIDEO_ON);
	power[0] = 1;
	power[1] = level;
	ddi_prop_modify(DDI_DEV_T_NONE, dip, DDI_PROP_CANSLEEP,
			"pm_norm_pwr", (caddr_t)power, sizeof (power));
    } else {
	p9000_set_video(softc, FBVIDEO_OFF);
	/*
	p9000_set_sync(softc, FBVIDEO_OFF);
	*/
    }
    
    (void) ddi_power(dip, cmpt, level);
    
    return (DDI_SUCCESS);
}
#endif

/* ARGSUSED */
static int
p9000_info(dev_info_t * dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
    register int	instance;
    register int	error = DDI_SUCCESS;
    register struct p9000_softc	*softc;
    
    instance = getminor((dev_t)arg);
    switch (infocmd) {
    case DDI_INFO_DEVT2DEVINFO:
	softc = getsoftc(instance);
	if (softc == NULL) {
	    error = DDI_FAILURE;
	} else {
	    *result = (void *)softc->devi;
	    error = DDI_SUCCESS;
	}
	break;
    case DDI_INFO_DEVT2INSTANCE:
	*result = (void *)instance;
	error = DDI_SUCCESS;
	break;
    default:
	error = DDI_FAILURE;
    }
    return (error);
}

/*ARGSUSED*/
static
p9000_open(register dev_t *const devp, register int const flag,
	register int const otyp, register cred_t *const cred)
{
    register int const	unit = getminor(*devp);
    struct p9000_softc	*softc = getsoftc(unit);
    int	error = 0;
    
    DEBUGF(2, (CE_CONT, "p9000_open(%d)\n", unit));
    
    /*
     * is this gorp necessary?
     */
    if (otyp != OTYP_CHR) {
	error = EINVAL;
    }
    else if (softc == NULL) {
	error = ENXIO;
    }
    return (error);
}


/*ARGSUSED*/
static
p9000_close(register dev_t const dev, register int const flag,
	register int const otyp, register cred_t *const cred)
{
    register int const	unit = getminor(dev);
    struct p9000_softc	*softc = getsoftc(unit);
    int	error = 0;
    
    DEBUGF(2, (CE_CONT, "p9000_close(%d, %d, %d)\n", unit, flag, otyp));
    
    if (otyp != OTYP_CHR) {
	error = EINVAL;
    }
    else if (softc == NULL) {
	error = ENXIO;
    }
    else {
	mutex_enter(&softc->mutex);
	softc->cur.enable = 0;
	softc->curctx = NULL;
	p9000_reset(softc);
	mutex_exit(&softc->mutex);
    }
    return (error);
}

/*ARGSUSED*/
static
p9000_mmap(register dev_t const dev, register off_t const off,
	register int const prot)
{
    register struct p9000_softc *const	softc = getsoftc(getminor(dev));
    register p9000p_t const p9000reg = &softc->p9000;
    register int	diff;
    register caddr_t	vaddr;
    register int	rval;
    
    DEBUGF(off ? 5 : 1, (CE_CONT, "p9000_mmap(%d, 0x%x)\n",
			 getminor(dev), (u_int) off));

    if ((diff = off - P9000_CONTROL_BASE) >= 0 && diff < P9000_CONTROL_SIZE)
	vaddr = (caddr_t)p9000reg->p9000_control_regp + diff;

    else if ((diff = off - P9000_DRAWING_BASE) >= 0 &&
	    diff < P9000_DRAWING_SIZE)
	vaddr = (caddr_t)p9000reg->p9000_drawing_regp + diff;

    else if ((diff = off - P9000_FRAME_BUFFER_BASE) >= 0 &&
	    diff < P9000_FRAME_BUFFER_SIZE)
	vaddr = (caddr_t)p9000reg->p9000_frame_bufferp + diff;

#if ALWAYS_NO_INTERRUPTS

    else if ((diff = off - P9000_VRT_VADDR) >= 0 && diff < P9000_VRT_SIZE)
	vaddr = softc->vrtpage ?
	       (caddr_t) softc->vrtpage + diff : (caddr_t) - 1;

#endif /* ALWAYS_NO_INTERRUPTS */

    else
	vaddr = (caddr_t) - 1;

    if (vaddr != (caddr_t) - 1)
	rval = hat_getkpfnum(vaddr);
    else
	rval = -1;

    DEBUGF(5, (CE_CONT, "p9000_mmap returning 0x%x\n", rval));

    return (rval);
}


/*ARGSUSED*/
static
p9000_ioctl(dev_t dev, int cmd, int data, int mode, cred_t *cred, int *rval)
{
    register struct p9000_softc *const	softc = getsoftc(getminor(dev));
    register p9000p_t	const p9000reg = &softc->p9000;
    register int	cursor_cmap;
    int		i;
    
    u_char	*iobuf_cmap_red;
    u_char	*iobuf_cmap_green;
    u_char	*iobuf_cmap_blue;
    u_char	*stack_cmap;
    
    struct fbcmap	*cmap, cma;
    struct fbcursor	cp;
    
    u_int	index;
    u_int	count;
    u_char	*map;
    u_int	entries;
    
    DEBUGF(3, (CE_CONT, "p9000_ioctl(%d, 0x%x)\n", getminor(dev), cmd));
    
    /* default to updating normal colormap */
    cursor_cmap = 0;
    
    switch (cmd) {
    case VIS_GETIDENTIFIER:
	if (ddi_copyout((caddr_t)&softc->p9000_ident,
			(caddr_t)data,
			sizeof (struct vis_identifier),
			mode))
	    return (EFAULT);
	break;
    
    case FBIOPUTCMAP:
    case FBIOGETCMAP:{
    
cmap_ioctl:
	
	if (ddi_copyin((caddr_t)data,
		       (caddr_t)&cma,
		       sizeof (struct fbcmap),
		       mode))
	    return (EFAULT);
    
	cmap = &cma;
	index = (u_int) cmap->index;
	count = (u_int) cmap->count;
    
	if (count == 0) {
	    return (0);
	}
    
	if (cursor_cmap == 0) {
	    map = softc->cmap[0];
	    entries = P9000_CMAP_ENTRIES;
	}
	else {
	    map = softc->omap[0];
	    entries = P9000_CURSOR_ENTRIES;
	}
    
	if (index >= entries ||
	    index + count > entries) {
	    return (EINVAL);
	}
	/*
	 * Allocate memory for color map RGB entries.
	 */
	stack_cmap = kmem_alloc((P9000_CMAP_ENTRIES * 3), KM_SLEEP);
    
	iobuf_cmap_red = stack_cmap;
	iobuf_cmap_green = stack_cmap + P9000_CMAP_ENTRIES;
	iobuf_cmap_blue = stack_cmap + (P9000_CMAP_ENTRIES * 2);
    
	if (cmd == FBIOPUTCMAP) {
	    int error;
    
	    DEBUGF(3, (CE_CONT, "FBIOPUTCMAP\n"));
	    if (error = ddi_copyin((caddr_t)cmap->red,
				   (caddr_t)iobuf_cmap_red,
				   count,
				   mode)) {
		kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		return (error);
	    }
    
	    if (error = ddi_copyin((caddr_t)cmap->green,
				   (caddr_t)iobuf_cmap_green,
				   count,
				   mode)) {
		kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		return (error);
	    }
	    if (error = ddi_copyin((caddr_t)cmap->blue,
				   (caddr_t)iobuf_cmap_blue,
				   count,
				   mode)) {
		kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		return (error);
	    }
    
	    mutex_enter(&softc->mutex);
	    map += index * 3;
	    if (p9000_update_pending(softc)) {
		mutex_enter(&(softc)->interlock);
		p9000_int_disable_intr(p9000reg);
		mutex_exit(&(softc)->interlock);
	    }
	    
	    /*
	     * Copy color map entries from stack to the color map
	     * table in the softc area.
	     */
	    
	    p9000_cmap_copyin(map++, iobuf_cmap_red, count);
	    p9000_cmap_copyin(map++, iobuf_cmap_green, count);
	    p9000_cmap_copyin(map, iobuf_cmap_blue, count);
	    
	    /* cursor colormap update */
	    if (entries < P9000_CMAP_ENTRIES)
		p9000_update_omap(softc);
	    else
		p9000_update_cmap(softc, index, count);
	    p9000_int_enable(p9000reg);
	    mutex_exit(&softc->mutex);
	    
	    }
	    else {
		/* FBIOGETCMAP */
		DEBUGF(3, (CE_CONT, "FBIOGETCMAP\n"));
		
		mutex_enter(&softc->mutex);
		map += index * 3;
		
		/*
		 * Copy color map entries from soft area to
		 * local storage and prepare for a copyout
		 */
		
		p9000_cmap_copyout(iobuf_cmap_red, map++, count);
		p9000_cmap_copyout(iobuf_cmap_green, map++, count);
		p9000_cmap_copyout(iobuf_cmap_blue, map, count);
		
		mutex_exit(&softc->mutex);
		
		if (ddi_copyout((caddr_t)iobuf_cmap_red,
				(caddr_t)cmap->red,
				count,
				mode)) {
		    kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		    return (EFAULT);
		}
		if (ddi_copyout((caddr_t)iobuf_cmap_green,
				(caddr_t)cmap->green,
				count,
				mode)) {
		    kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		    return (EFAULT);
		}
		if (ddi_copyout((caddr_t)iobuf_cmap_blue,
				(caddr_t)cmap->blue,
				count,
				mode)) {
		    kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
		    return (EFAULT);
		}
	    }
	    kmem_free(stack_cmap, (P9000_CMAP_ENTRIES * 3));
	    if (!softc->have_intr) {
		(void)p9000_intr((caddr_t) softc);
	    }
	}
	break;
    
    
	case FBIOSATTR:{
	    struct fbsattr	attr;
	
	    if (ddi_copyin((caddr_t)data,
			   (caddr_t)&attr,
			   sizeof (attr),
			   mode))
		return (EFAULT);
	    DEBUGF(3, (CE_CONT, "FBIOSATTR, type=%d\n", attr.emu_type));
	    if (attr.emu_type != -1)
		return (EINVAL);
	    /* ignore device-dependent stuff */
	}
	break;
	
	
	case FBIOGATTR:{
	    struct fbgattr	attr;

	    DEBUGF(3, (CE_CONT, "FBIOGATTR, emu_type=%d\n", softc->emulation));
	    bcopy((caddr_t)&p9000_attr, (caddr_t)&attr, sizeof (attr));
	    mutex_enter(&softc->mutex);
	    attr.fbtype.fb_type = softc->emulation;
	    attr.fbtype.fb_width = softc->w;
	    attr.fbtype.fb_height = softc->h;
	    attr.fbtype.fb_size = softc->size;
	    attr.sattr.emu_type = softc->emulation;
	    mutex_exit(&softc->mutex);
	    
	    if (ddi_copyout((caddr_t)&attr,
			    (caddr_t)data,
			    sizeof (struct fbgattr),
			    mode))
		return (EFAULT);
	}
	break;
    
	case FBIOGTYPE:{
	    struct fbtype	fb;
	    
	    mutex_enter(&softc->mutex);
	    bcopy((caddr_t)&p9000_attr.fbtype, (caddr_t)&fb,
		  sizeof (struct fbtype));
	    DEBUGF(3, (CE_CONT, "FBIOGTYPE\n"));
	    fb.fb_width = softc->w;
	    fb.fb_height = softc->h;
	    fb.fb_size = softc->size;
	    mutex_exit(&softc->mutex);
	    
	    if (ddi_copyout((caddr_t)&fb,
			    (caddr_t)data,
			    sizeof (struct fbtype),
			    mode))
		return (EFAULT);
	}
	break;
	
	case FBIOSVIDEO:
	    DEBUGF(3, (CE_CONT, "FBIOSVIDEO\n"));
	    if (ddi_copyin((caddr_t)data,
			   (caddr_t)&i,
			   sizeof (int),
			   mode))
		return (EFAULT);
	    mutex_enter(&softc->mutex);
	    p9000_set_video(softc, i);
	    mutex_exit(&softc->mutex);
	break;
	
#define ddi_getb(h, p)		inb((int)(p))
#define ddi_putb(h, p, v)	outb((int)(p),(v))

	case FBIOGVIDEO:
	    DEBUGF(3, (CE_CONT, "FBIOGVIDEO\n"));
	    mutex_enter(&softc->mutex);
	    i = (ddi_getb(softc->p9000_bt_handle,
		softc->viperport.vp_bt485_comreg0) &
		BT485_POWER_DOWN_ENABLE) ? FBVIDEO_OFF : FBVIDEO_ON;
	    mutex_exit(&softc->mutex);
	    
	    if (ddi_copyout((caddr_t)&i,
			    (caddr_t)data,
			    sizeof (int),
			    mode))
		return (EFAULT);
	break;
#undef ddi_putb
#undef ddi_getb
	
#if ALWAYS_NO_INTERRUPTS
	/* vertical retrace interrupt */
	
	case FBIOVERTICAL:
	
	    if (softc->have_intr) {
		mutex_enter(&softc->mutex);
		softc->vrtflag |= P9000_VRTIOCTL;
		p9000_int_enable(p9000reg);
		cv_wait(&softc->vrtsleep, &softc->mutex);
		mutex_exit(&softc->mutex);
	    }
	    else
		P9000_WAIT_VSYNC (p9000reg);
	return (0);
	
	case FBIOVRTOFFSET:
	    i = P9000_VRT_VADDR;
	    
	    if (ddi_copyout((caddr_t)&i,
			    (caddr_t)data,
			    sizeof (int),
			    mode))
		return (EFAULT);
	    return (0);

#endif /* ALWAYS_NO_INTERRUPTS */
	
	/* HW cursor control */
	case FBIOSCURSOR:{
	    int		set;
	    int		cbytes;
	    u_long	stack_image[32];
	    u_long	stack_mask[32];
	    
	    if (ddi_copyin((caddr_t)data,
			   (caddr_t)&cp,
			   sizeof (struct fbcursor),
			   mode))
		return (EFAULT);
	    
	    set = cp.set;
	    softc->cur.size = cp.size;
	    
	    /* compute cursor bitmap bytes */
	    cbytes = softc->cur.size.y * sizeof (softc->cur.image[0]);
	    if (set & FB_CUR_SETSHAPE) {
	    
		if ((u_int) cp.size.x > 32 || (u_int) cp.size.y > 32) {
		    return (EINVAL);
		}
	    
		/* copy cursor image into softc */
		if (cp.image != NULL) {
		    if (ddi_copyin((caddr_t)cp.image,
				   (caddr_t)&stack_image,
				   cbytes,
				   mode))
			return (EFAULT);
		}
		if (cp.mask != NULL) {
		    if (ddi_copyin((caddr_t)cp.mask,
				   (caddr_t)&stack_mask,
				   cbytes,
				   mode))
			return (EFAULT);
		}
	    }
	    
	    mutex_enter(&softc->mutex);
	    if (set & FB_CUR_SETCUR)
		softc->cur.enable = cp.enable;
	    
	    if (set & FB_CUR_SETPOS)
		softc->cur.pos = cp.pos;
	    
	    if (set & FB_CUR_SETHOT)
		softc->cur.hot = cp.hot;
	    
	    /* update hardware */
	    viper_setcurpos(softc);
	    
	    
	    if (set & FB_CUR_SETSHAPE) {
	    
	    if (cp.image != NULL) {
		bzero((caddr_t)softc->cur.image, sizeof (softc->cur.image));
		bcopy((caddr_t)&stack_image,
		      (caddr_t)softc->cur.image,
		      cbytes);
	    }
	    if (cp.mask != NULL) {
		bzero((caddr_t)softc->cur.mask, sizeof (softc->cur.mask));
		bcopy((caddr_t)&stack_mask,
		      (caddr_t)softc->cur.mask,
		      cbytes);
	    }
	    /* load into hardware */
	    viper_setcurshape(softc);
	    }
	    mutex_exit(&softc->mutex);
	    
	    /* load colormap */
	    if (set & FB_CUR_SETCMAP) {
		cursor_cmap = 1;
		cmd = FBIOPUTCMAP;
		data = (int) &cp.cmap;
		goto cmap_ioctl;
	    }
	    break;
	}
	
	case FBIOGCURSOR:{
	    int		cbytes;
	    u_long	stack_image[32];
	    u_long	stack_mask[32];
	    
	    if (ddi_copyin((caddr_t)data,
			   (caddr_t)&cp,
			   sizeof (struct fbcursor),
			   mode))
		return (EFAULT);
	    
	    mutex_enter(&softc->mutex);
	    cp.set = 0;
	    cp.enable = softc->cur.enable;
	    cp.pos = softc->cur.pos;
	    cp.hot = softc->cur.hot;
	    cp.size = softc->cur.size;
	    cp.cmap.index = 0;
	    cp.cmap.count = P9000_CURSOR_ENTRIES;
	    
	    /* compute cursor bitmap bytes */
	    cbytes = softc->cur.size.y * sizeof (softc->cur.image[0]);
	    
	    bcopy((caddr_t)softc->cur.image, (caddr_t)&stack_image, cbytes);
	    bcopy((caddr_t)softc->cur.mask, (caddr_t)&stack_mask, cbytes);
	    
	    mutex_exit(&softc->mutex);
	    
	    if (ddi_copyout((caddr_t)&cp,
			    (caddr_t)data,
			    sizeof (struct fbcursor),
			    mode))
		return (EFAULT);
	    
	    /* if image pointer is non-null copy both bitmaps */
	    if (cp.image != NULL) {
		if (ddi_copyout((caddr_t)&stack_image,
				(caddr_t)cp.image,
				cbytes,
				mode))
		    return (EFAULT);
		if (ddi_copyout((caddr_t)&stack_mask,
				(caddr_t)cp.mask,
				cbytes,
				mode))
		    return (EFAULT);
	    }
	    
	    /* if red pointer is non-null copy colormap */
	    if (cp.cmap.red) {
		cursor_cmap = 1;
		cmd = FBIOGETCMAP;
		data = (int) &cp.cmap;
		goto cmap_ioctl;
	    }
	}
	break;
	
	case FBIOSCURPOS: {
	
	    struct fbcurpos	stack_curpos;   /* cursor position */
	    
	    if (ddi_copyin((caddr_t)data,
			   (caddr_t)&stack_curpos,
			   sizeof (struct fbcurpos),
			   mode))
		return (EFAULT);
	    
	    mutex_enter(&softc->mutex);
	    bcopy((caddr_t)&stack_curpos,
		  (caddr_t)&softc->cur.pos,
		  sizeof (struct fbcurpos));
	    viper_setcurpos(softc);
	    mutex_exit(&softc->mutex);
	}
	break;
	
	case FBIOGCURPOS: {
	
	    struct fbcurpos	stack_curpos;   /* cursor position */
	    
	    mutex_enter(&softc->mutex);
	    bcopy((caddr_t)&softc->cur.pos,
		  (caddr_t)&stack_curpos,
		  sizeof (struct fbcurpos));
	    mutex_exit(&softc->mutex);
	    if (ddi_copyout((caddr_t)&stack_curpos,
			    (caddr_t)data,
			    sizeof (struct fbcurpos),
			    mode))
		return (EFAULT);
	}
	break;
	
	case FBIOGCURMAX:{
	    static struct fbcurpos curmax = {32, 32};
	    
	    if (ddi_copyout((caddr_t)&curmax,
			    (caddr_t)data,
			    sizeof (struct fbcurpos),
			    mode))
		return (EFAULT);
	}
	break;
    
#if P9000DEBUG >= 3
	case 253:
	    break;
	
	case 255:
	    p9000_debug = (int) data;
	    if (p9000_debug == -1)
		p9000_debug = P9000DEBUG;
	    cmn_err(CE_CONT, "p9000_debug is now %d\n", p9000_debug);
	    break;

#endif /* P9000DEBUG */
	
	default:
	    return (viper_ioctl (dev, cmd, data, mode, cred, rval));
    }				/* switch(cmd) */
    return (0);
}


static  u_int
p9000_intr(caddr_t arg)
{
    register struct p9000_softc *const softc = (struct p9000_softc *) arg;
    register p9000p_t	const p9000reg = &softc->p9000;
    
    DEBUGF(7, (CE_CONT,
	       "p9000_intr: softc=%x, vrtflag=%x\n", softc, softc->vrtflag));
    
    mutex_enter(&softc->mutex);
    mutex_enter(&softc->interlock);
    
    if (!(p9000_update_pending(softc) || (softc)->vrtflag)) {

	/* TODO catch stray interrupts? */
	p9000_int_disable_intr(p9000reg);
	p9000_int_clear(p9000reg);
	mutex_exit(&softc->interlock);
	mutex_exit(&softc->mutex);

	return (DDI_INTR_CLAIMED);
    }

#if ALWAYS_NO_INTERRUPTS

    if (softc->vrtflag & P9000_VRTCTR) {
	if (softc->vrtmaps == 0) {
	    softc->vrtflag &= ~P9000_VRTCTR;
	}
	else
	    *softc->vrtpage += 1;
    }

    if (softc->vrtflag & P9000_VRTIOCTL) {
	softc->vrtflag &= ~P9000_VRTIOCTL;
	cv_broadcast(&softc->vrtsleep);
    }

#endif /* ALWAYS_NO_INTERRUPTS */

    if (p9000_update_pending(softc)) {

	/* load cursor color map */
	if (softc->omap_update) {
	    viper_setcurcolor(softc);
	    softc->omap_update = 0;
	}

	/* load main color map */
	if (softc->cmap_count) {
	    viper_setpalet(softc);
	    softc->cmap_count = 0;
	}
    }

    p9000_int_disable_intr(p9000reg);
    p9000_int_clear(p9000reg);

    if (softc->vrtflag)
	p9000_int_enable(p9000reg);
    
#ifdef	COMMENT

    if (!softc->vrtflag)
	p9000_int_disable_intr(p9000reg);

#endif	/* COMMENT */
    
    mutex_exit(&softc->interlock);
    mutex_exit(&softc->mutex);

    return (DDI_INTR_CLAIMED);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
p9000_reset_cmap(register u_char *const cmap, register u_int const entries)
{
    bzero((char *) cmap, entries * 3);
    cmap[0] = 255;
    cmap[1] = 255;
    cmap[2] = 255;
}

/*
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 * Zero count argument indicates cursor color map update desired.
 */
static void
p9000_update_cmap(register struct p9000_softc *softc, register u_int index,
	register u_int count)
{
    register u_int	high, low;
    
    if (count == 0) {
	softc->omap_update = 1;
	return;
    }

    high = softc->cmap_count;

    if (high != 0) {
	high += (low = softc->cmap_index);
    
	if (index < low)
	    softc->cmap_index = low = index;
	
	if (index + count > high)
	    high = index + count;
    
	softc->cmap_count = high - low;
    }
    else {
	softc->cmap_index = index;
	softc->cmap_count = count;
    }
}


static void
p9000_update_omap(register struct p9000_softc *const softc)
{
    softc->omap_update = 1;
}

/*
 * Copy colormap entries between red, green, or blue array to
 * interspersed rgb array.
 */
static  void
p9000_cmap_copyin (register u_char *rgb, register u_char *buf,
	register u_int count)
{
    while (count != 0) {
	*rgb = *buf;
	rgb += 3;
	buf++;
	count--;
    }
}

/*
 * Copy colormap entries between interspersed rgb array to
 * red, green, or blue array.
 */
static  void
p9000_cmap_copyout (register u_char *buf, register u_char *rgb,
	register u_int count)
{
    while (count != 0) {
	*buf = *rgb;
	rgb += 3;
	buf++;
	count--;
    }
}

/*ARGSUSED*/
static void
p9000_reset(register struct p9000_softc *const softc)
{
}

/*
 * P9000/P9100 Context Management
 *
 * This virtualizes the P9000/P9100 register file by associating a register
 * save area with each mapping of the device and using the ddi_mapdev
 * mechanism to allow us to detect mapping changes.
 * Only one of these contexts is valid at any time; an access to one
 * of the invalid contexts saves the current P9000/P9100 context, invalidates
 * the current valid mapping, restores the former register contents
 * appropriate to the faulting mapping, and then validates it.
 *
 * This implements a graphical context switch that is transparent to
 * the user.
 *
 * the REGS contain the interesting context registers.
 *
 * Per-mapping info:
 *	Some, but not all, mappings are part of a context.
 *      Any mapping that is MAP_PRIVATE to the REGS will be
 *	part of a unique context.  MAP_SHARED mappings are part
 *	of the shared context and all such programs must arbitrate among
 *	themselves to keep from stepping on each other's register settings.
 *	Mappings to the framebuffer may or may not be part of a context,
 *	depending on exact hardware type.
 */

#define P9000_MAP_SHARED   0x02    /* shared context */
#define P9000_MAP_LOCK     0x04    /* lock page */
#if ALWAYS_NO_INTERRUPTS
#define P9000_MAP_VRT      0x08    /* vrt page */
#endif
#define P9000_MAP_REGS     0x10    /* mapping includes registers */
#define P9000_MAP_FB       0x40    /* mapping includes framebuffer */

#define P9000_MAP_CTX      (P9000_MAP_REGS | P9000_MAP_FB) /* needs context */

static struct p9000_map_pvt *
p9000_pvt_alloc(register struct p9000_cntxt *const ctx, register u_int const off,
	register u_int const len, register struct p9000_softc *const softc)
{
    register struct p9000_map_pvt	*pvt;
    
    /*
     * create the private data portion of the mapdev object
     */
    pvt = (struct p9000_map_pvt *)kmem_zalloc(sizeof (struct p9000_map_pvt), 
					     KM_SLEEP);
    pvt->offset  = off;
    pvt->len     = len;
    pvt->context = ctx;
    pvt->softc = softc;
    
    /*
     * Link this pvt into the list of associated pvt's for this
     * context
     */
    pvt->next = ctx->p9000_ctx_pvt;
    ctx->p9000_ctx_pvt = pvt;
    
    return(pvt);
}

/*
 * This routine is called through the cb_ops table to handle
 * p9000 mmap requests.
 */
/*ARGSUSED*/
static int
p9000_segmap(register dev_t const dev, register off_t const off,
	register struct as *const as, register caddr_t *const addrp,
	register off_t const len, register u_int const prot,
	register u_int const maxprot, register u_int flags,
	register cred_t *const cred)
{
    register struct p9000_softc *const	softc = getsoftc(getminor(dev));
    register struct p9000_cntxt	*ctx  = (struct p9000_cntxt *)NULL;
    register struct p9000_cntxt *const	shared_ctx = &softc->shared_ctx;
    register struct p9000_map_pvt	*pvt;
    register u_int	maptype = 0;
    register int	error;
    register int	i;
    
    DEBUGF(3, (CE_CONT, "segmap: off=%x, len=%x\n", off, len));
    
    mutex_enter(&softc->mutex);
    
    /*
     * Validate and categorize the map request.  Valid mmaps to
     * the p9000 driver are to device hardware or to the vertical
     * retrace counter page
     *
     * VRT page mapping?  If so, be sure to count VRT events
     */
#if ALWAYS_NO_INTERRUPTS
    if (off == P9000_VRT_VADDR) {
	if (!softc->have_intr)
	    return (EIO);
    
	if (len != p9000_pagesize) {
	    mutex_exit(&softc->mutex);
	    DEBUGF(3, (CE_CONT,
		       "rejecting because off=vrt and len=%x\n", len));
	    return (EINVAL);
	}
	
	maptype = P9000_MAP_VRT;
	
	if (softc->vrtmaps++ == 0) {
	    if (softc->vrtpage == NULL)
		softc->vrtalloc = (int *) kmem_alloc(p9000_pagesize * 2,
						     KM_SLEEP);
	    softc->vrtpage = (int *)
			     (((ulong_t)softc->vrtalloc + p9000_pagesize) &
			     ~p9000_pageoffset);
	    *softc->vrtpage = 0;
	    softc->vrtflag |= P9000_VRTCTR;
	    p9000_int_enable(9000);
	}
    }
    
    else {

#else /* ALWAYS_NO_INTERRUPTS */

    {

#endif /* ALWAYS_NO_INTERRUPTS */

	/*
	 * Check to insure that the entire range is legal and we are
	 * not trying to map more than defined by the device.
	 */
	for (i = 0; i < len; i += p9000_pagesize) {
	    if (p9000_mmap(dev, (off_t)  off + i, (int) maxprot) == -1) {
		mutex_exit(&softc->mutex);
		DEBUGF(3, (CE_CONT,
			   "rejecting because mmap returns -1, off=%x\n",
			   off+i));
		return (ENXIO);
	    }
	}
    
	/* classify it */
	if (off + len > P9000_REGISTER_BASE &&
	    off < P9000_REGISTER_BASE + P9000_REGISTER_SIZE) {
	    maptype |= P9000_MAP_CTX;
	}
    }
    
    
    /*
     * Is this mapping for part of the p9000 context?
     * If so, splice it into the softc context list.
     */
    if (maptype & P9000_MAP_CTX) {
    
	/*
	 * Is this a shared mapping.  If so use the shared_ctx.
	 */
	if (flags & MAP_SHARED) {
	    ctx = shared_ctx;
	    ctx->p9000_ctx_flag = P9000_MAP_CTX;   /* XXX: move to attach */
	}
	else {
	    ctx = p9000_ctx_map_insert(softc, maptype);
	    ctx->p9000_ctx_flag |= maptype;
	}
	
	pvt = p9000_pvt_alloc(ctx, 0, (u_int) len, softc);
	
	/*
	 * create the mapdev object
	 */
	error = ddi_mapdev(dev, off, as, addrp, len, prot, maxprot,
		flags, cred, &p9000_map_ops, &pvt->handle,
		(void *) pvt); 
	
	if (error != 0) {
	    cmn_err(CE_WARN, "ddi_mapdev failure\n");
	}
	
    }
    else {
    
	/*
	 * Yet another kludge: some programs may decide to map random
	 * parts of the device MAP_PRIVATE, even those that don't form
	 * part of the context. This has the effect of potentially asking
	 * ddi_segmap (aka spec_segmap) to create a MAP_PRIVATE mapping,
	 * which it will not do. We turn all gratuitous private mappings
	 * into shared ones here.
	 */
	
	if (flags & MAP_PRIVATE)
    	flags = (flags & ~MAP_TYPE) | MAP_SHARED;
	
	error = ddi_segmap(dev, off, as, addrp, len, prot,
		maxprot, flags, cred);
    }
    mutex_exit(&softc->mutex);
    return (error);
}

/*
 * An access has been made to a context other than the current one
 */
/*ARGSUSED*/
static int
p9000_map_access(register ddi_mapdev_handle_t const handle,
	register void *const pvt,
	register off_t const offset)
{
    register struct p9000_map_pvt *const	p = (struct p9000_map_pvt *)pvt;
    register struct p9000_map_pvt	*pvts;
    register struct p9000_softc *const	softc = p->softc;
    register p9000p_t	const p9000reg = &softc->p9000;
    register int	err = 0;
    
    ASSERT(pvt);
    
    mutex_enter(&softc->mutex);
    
    /*
     * Do we need to switch contexts?
     */
    if (softc->curctx != p->context) {
	
	/*
	 * If there's a current context, save it
	 */
	if (softc->curctx != (struct p9000_cntxt *)NULL) {
	    /*
	     * Set mapdev for current context and all associated handles
	     * to intercept references to their addresses
	     */
	    ASSERT(softc->curctx->p9000_ctx_pvt);
	    for (pvts = softc->curctx->p9000_ctx_pvt; pvts != NULL;
		     pvts=pvts->next) {
		err = ddi_mapdev_intercept(pvts->handle,
		pvts->offset, pvts->len);
		if (err)
		    return (err);
	    }
	    
	    if (p9000_cntxsave(p9000reg, softc->curctx) == 0) {
		DEBUGF(1, (CE_CONT, "p9000: context save failed\n"));
		/*
		 * At this point we have no current context.
		 */
		softc->curctx = NULL;
		mutex_exit(&softc->mutex);
		    return (-1);
	    }
	}
	
	/*
	 * Idle the chips
	 */
	CDELAY(!(p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY), P9000_WAIT);

	if (p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY) {
	    DEBUGF(1, (CE_CONT, "p9000: idle_p9000: status = %x\n",
		   p9000reg->p9000_status));

	    /*
	     * At this point we have no current context.
	     */
	    softc->curctx = NULL;
	    mutex_exit(&softc->mutex);
		return (-1);
	}

	DEBUGF(4, (CE_CONT, "loading context %x\n", p->context));
	
	if (p->context->p9000_ctx_flag & P9000_MAP_REGS)
	    if (!p9000_cntxrestore(p9000reg, p->context)) {
		DEBUGF(1, (CE_CONT, "p9000: context restore failed\n"));
	    /*
	     * At this point we have no current context.
	     */
	    softc->curctx = NULL;
	    mutex_exit(&softc->mutex);
		return (-1);
	}

	/*
	 * switch software "context"
	 */
	softc->curctx = p->context;
    }
    
    ASSERT(p->context->p9000_ctx_pvt);

    for (pvts = p->context->p9000_ctx_pvt; pvts != NULL; pvts = pvts->next) {
	if ((err = ddi_mapdev_nointercept(pvts->handle, 
					  pvts->offset, pvts->len)) != 0) {
	    mutex_exit(&softc->mutex);
	    return(err);
	}
    }
    
    mutex_exit(&softc->mutex);
    return (err);
}


/*ARGSUSED*/
static void
p9000_map_free(register ddi_mapdev_handle_t const handle,
	register void *const pvt)
{
    register struct p9000_map_pvt *const	p = (struct p9000_map_pvt *)pvt;
    register struct p9000_map_pvt	*pvts;
    register struct p9000_map_pvt	*ppvts;
    register struct p9000_cntxt *const	ctx = p->context;
    register struct p9000_softc *const	softc = p->softc;
    register struct p9000_cntxt *const	shared_ctx = &softc->shared_ctx;
    
    mutex_enter(&softc->mutex);
    
    DEBUGF(4, (CE_CONT, "p9000_map_free: cleaning up pid %d\n", ctx->pid));

    /*
     * Remove the pvt data
     */
    ppvts = NULL;
    for (pvts = ctx->p9000_ctx_pvt; pvts != NULL; pvts = pvts->next) {
	if (pvts == pvt) {
	    if (ppvts == NULL) {
		ctx->p9000_ctx_pvt = pvts->next;
	    }
	    else {
		ppvts->next = pvts->next;
	    }
	    kmem_free(pvt, sizeof(struct p9000_map_pvt));
	    break;
	}
	ppvts = pvt;
    }
    
    /*
     * Remove the context if this is not the shared context and there are
     * no more associated pvt's
     */
    if ((ctx != shared_ctx) && (ctx->p9000_ctx_pvt == NULL)) {
	register struct p9000_cntxt *ctxptr;
	
	if (ctx == softc->curctx)
	    softc->curctx = NULL;

	/*
	 * Scan private context list for entry to remove.
	 * Check first to see if it's the head of our list.
	 */
	if (softc->pvt_ctx == ctx) {
	    softc->pvt_ctx = ctx->link;
	    kmem_free(ctx, sizeof (struct p9000_cntxt));
	}
	else {
	    for (ctxptr = softc->pvt_ctx; ctxptr != NULL;
		     ctxptr = ctxptr->link) {
		if (ctxptr->link == ctx) {
		    ctxptr->link = ctx->link;
		    kmem_free(ctx, sizeof (struct p9000_cntxt));
		}
	    }
	}
    }
    
    /*
     * If the curctx is the shared context, and there are no 
     * more pvt's for the shared context, set the curctx to 
     * NULL to force a context switch on the next device access.
     */
    if ((softc->curctx == shared_ctx) &&
	    (softc->curctx->p9000_ctx_pvt == NULL)) {
	softc->curctx = NULL;
    }

    mutex_exit(&softc->mutex);
}


/*ARGSUSED*/
static int 
p9000_map_dup(register ddi_mapdev_handle_t const old_handle,
	register void *const oldpvt, register ddi_mapdev_handle_t new_handle,
	register void **const newpvt)
{
    register struct p9000_map_pvt *const	p = (struct p9000_map_pvt *)oldpvt;
    register struct p9000_softc *const	softc = p->softc;
    register struct p9000_map_pvt	*pvt;
    register struct p9000_cntxt		*ctx;
    
    mutex_enter(&softc->mutex);
    if (p->context != &softc->shared_ctx) {
	ctx = (struct p9000_cntxt *)
	    kmem_zalloc(sizeof (struct p9000_cntxt), KM_SLEEP);
	
	*ctx = *p->context;
	ctx->p9000_ctx_pvt = NULL;
    }
    else
	ctx = &softc->shared_ctx;
    
    pvt = p9000_pvt_alloc(ctx, 0, p->len, softc);
    
    pvt->handle = new_handle;
    *newpvt = pvt;
    
#if ALWAYS_NO_INTERRUPTS

    if (p->context && (p->context->p9000_ctx_flag & P9000_MAP_VRT)) {
	softc->vrtflag |= P9000_VRTCTR;
	if (softc->vrtmaps == 0)
	    p9000_int_enable(9000);
	softc->vrtmaps++;
    }

#endif /* ALWAYS_NO_INTERRUPTS */
    
    mutex_exit(&softc->mutex);
    return(0);
}

#ifdef POWER
static void
p9000_set_sync(struct p9000_softc *softc, int v)
{
}
#endif

static void
p9000_set_video(struct p9000_softc *softc, int v)
{
    register ddi_acc_handle_t handle = softc->p9000_bt_handle;

#define ddi_getb(h, p)		inb((int)(p))
#define ddi_putb(h, p, v)	outb((int)(p),(v))

    if (v & FBVIDEO_ON)
	ddi_putb(handle, softc->viperport.vp_bt485_comreg0,
	     ddi_getb(handle, softc->viperport.vp_bt485_comreg0) & ~
	     BT485_POWER_DOWN_ENABLE);
    else
	ddi_putb(handle, softc->viperport.vp_bt485_comreg0,
	     ddi_getb(handle, softc->viperport.vp_bt485_comreg0) |
	     BT485_POWER_DOWN_ENABLE);
#undef ddi_putb
#undef ddi_getb
}

static int
p9000_cntxsave(register p9000p_t p9000reg, struct p9000_cntxt *const saved)
{
    DEBUGF(5, (CE_CONT, "saving registers\n"));
    
    CDELAY(!(p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY), P9000_WAIT);
    if (p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY) {
	DEBUGF(1, (CE_CONT,
		"p9000: cntxsave: status = %x\n", p9000reg->p9000_status));
	return (0);
    }
    
    /*
     * start dumping stuff out.
     */
    
    saved->p9000_ctx_interrupt = p9000reg->p9000_interrupt;
    saved->p9000_ctx_interrupt_en = p9000reg->p9000_interrupt_en;
    saved->p9000_ctx_cindex = p9000reg->p9000_cindex;
    saved->p9000_ctx_w_off_xy = p9000reg->p9000_w_off_xy;
    saved->p9000_ctx_fground = p9000reg->p9000_fground;
    saved->p9000_ctx_bground = p9000reg->p9000_bground;
    saved->p9000_ctx_pmask = p9000reg->p9000_pmask;
    saved->p9000_ctx_draw_mode = p9000reg->p9000_draw_mode;
    saved->p9000_ctx_pat_originx = p9000reg->p9000_pat_originx;
    saved->p9000_ctx_pat_originy = p9000reg->p9000_pat_originy;
    saved->p9000_ctx_raster = p9000reg->p9000_raster;
    saved->p9000_ctx_pixel8_reg = p9000reg->p9000_pixel8_reg;
    saved->p9000_ctx_w_min = p9000reg->p9000_w_min;
    saved->p9000_ctx_w_max = p9000reg->p9000_w_max;
    P9000_PAT_COPY (saved->p9000_ctx_pattern, p9000reg->p9000_pattern);
    saved->p9000_ctx_x0 = p9000reg->p9000_x0;
    saved->p9000_ctx_y0 = p9000reg->p9000_y0;
    saved->p9000_ctx_x1 = p9000reg->p9000_x1;
    saved->p9000_ctx_y1 = p9000reg->p9000_y1;
    saved->p9000_ctx_x2 = p9000reg->p9000_x2;
    saved->p9000_ctx_y2 = p9000reg->p9000_y2;
    saved->p9000_ctx_x3 = p9000reg->p9000_x3;
    saved->p9000_ctx_y3 = p9000reg->p9000_y3;
    return (1);
}

static int
p9000_cntxrestore(register p9000p_t const p9000reg,
	register struct p9000_cntxt *const saved)
{
    DEBUGF(5, (CE_CONT, "restoring registers\n"));
    
    CDELAY(!(p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY), P9000_WAIT);
    if (p9000reg->p9000_status & P9000_STATUS_QUAD_OR_BUSY) {
	DEBUGF(1, (CE_CONT,
		"p9000: cntxrestore: status = %x\n", p9000reg->p9000_status));
	return (0);
    }
    
    /*
     * start restoring stuff in.
     */
    
    p9000reg->p9000_interrupt = saved->p9000_ctx_interrupt |
			     P9000_INT_VBLANKED_CTRL |
			     P9000_INT_PICKED_CTRL |
			     P9000_INT_DE_IDLE_CTRL;
    p9000reg->p9000_interrupt_en = saved->p9000_ctx_interrupt_en |
				P9000_INTEN_MEN_CTRL |
				P9000_INTEN_VBLANKED_EN_CTRL |
				P9000_INTEN_PICKED_EN_CTRL |
				P9000_INTEN_DE_IDLE_EN_CTRL;
    p9000reg->p9000_cindex = saved->p9000_ctx_cindex;
    p9000reg->p9000_w_off_xy = saved->p9000_ctx_w_off_xy;
    p9000reg->p9000_fground = saved->p9000_ctx_fground;
    p9000reg->p9000_bground = saved->p9000_ctx_bground;
    p9000reg->p9000_pmask = saved->p9000_ctx_pmask;
    p9000reg->p9000_draw_mode = saved->p9000_ctx_draw_mode;
    p9000reg->p9000_pat_originx = saved->p9000_ctx_pat_originx;
    p9000reg->p9000_pat_originy = saved->p9000_ctx_pat_originy;
    p9000reg->p9000_raster = saved->p9000_ctx_raster;
    p9000reg->p9000_pixel8_reg = saved->p9000_ctx_pixel8_reg;
    p9000reg->p9000_w_min = saved->p9000_ctx_w_min;
    p9000reg->p9000_w_max = saved->p9000_ctx_w_max;
    P9000_PAT_COPY (p9000reg->p9000_pattern, saved->p9000_ctx_pattern);
    p9000reg->p9000_x0 = saved->p9000_ctx_x0;
    p9000reg->p9000_y0 = saved->p9000_ctx_y0;
    p9000reg->p9000_x1 = saved->p9000_ctx_x1;
    p9000reg->p9000_y1 = saved->p9000_ctx_y1;
    p9000reg->p9000_x2 = saved->p9000_ctx_x2;
    p9000reg->p9000_y2 = saved->p9000_ctx_y2;
    p9000reg->p9000_x3 = saved->p9000_ctx_x3;
    p9000reg->p9000_y3 = saved->p9000_ctx_y3;
    return (1);
}


/*
 * p9000_ctx_map_insert()
 *
 * Insert a mapping into the mapping list of a private context.  First
 * determine if there's an existing context (e.g. one with the same PID
 * as the current  one and that does not already have a mapping of this
 * type yet).  If not, allocate a new one.  Then insert mapping into this
 * context's list.
 *
 * The softc mutex must be held across calls to this routine.
 */
static
struct p9000_cntxt *
p9000_ctx_map_insert(struct p9000_softc *softc, int maptype)
{
    register struct p9000_cntxt	*ctx;
    register struct p9000_cntxt	*pvt_ctx_list;
    int         curpid = p9000_getpid();
    
    DEBUGF(4, (CE_CONT, "p9000_ctx_map_insert: maptype=0x%x curpid=%d\n",
	       maptype, curpid));
    
    /*
     * If this is the first time we're here, then alloc space
     * for new context and depart.
     */
    if (softc->pvt_ctx == NULL) {
	ctx = (struct p9000_cntxt *)
	kmem_zalloc(sizeof (struct p9000_cntxt), KM_SLEEP);
	ctx->pid = curpid;
	ctx->link = NULL;
	softc->pvt_ctx = ctx;
	return (ctx);
    }
    
    pvt_ctx_list = softc->pvt_ctx;
    
    /*
     * Find existing context if one exists.  We have a match if
     * we're the same process *and* there's not already a
     * mapping of this type assigned.
     */
    for (ctx = pvt_ctx_list; ctx != NULL; ctx = ctx->link) {
	if (ctx->pid == curpid &&
	    (maptype & ctx->p9000_ctx_flag & P9000_MAP_CTX) == 0)
	break;
    }
    
    /* no match, create a new one and add to softc list */
    if (ctx == NULL) {
	ctx = (struct p9000_cntxt *)
	kmem_zalloc(sizeof (struct p9000_cntxt), KM_SLEEP);
	ctx->pid = curpid;
	ctx->link = softc->pvt_ctx;
	softc->pvt_ctx = ctx;
    }
    
    return (ctx);
}

/*
 * p9000_getpid()
 *
 * Simple wrapper around process ID call to drv_getparm(9f).
 */
static int
p9000_getpid(void)
{
    u_long	rval;
    
    if (drv_getparm(PPID, &rval) == -1)
	return (0);
    else
	return ((int)rval);
}


/*
 * Viper specific routines
 */

/*
 * This function identifies P9000 on VLB.
 */

#define P9000_PROBE_DID_MAP_DR  0x0001
#define P9000_PROBE_DID_MAP_FB  0x0002
#define P9000_PROBE_DID_MAP_BT  0x0004
#define P9000_PROBE_DID_ENABLE  0x0008
#define P9000_PROBE_MATCH       0x0400
#define P9000_PROBE_VALUE_X     0x1234L
#define P9000_PROBE_VALUE_Y     0x3678L
#define P9000_PROBE_VALUE_XY    0x1234f678L

viper_vlb_probe(dev_info_t *devi)
{
    register p9000p_t	p9000reg;
    long		x;
    long		y;
    long		xy;
    long		newxy = 0;
    register int	status = 0;
    ddi_acc_handle_t	p9000_dr_handle;
    ddi_acc_handle_t	p9000_bt_handle;
    ddi_acc_handle_t	p9000_fb_handle;
    viper_port_t	viperport;
    p9000_t		p9000;
    int			viperoutctrl;
    volatile int	temp;

    DEBUGF(1, (CE_CONT, "viper_vlb_probe (%s) unit=%d\n",
	       ddi_get_name(devi), ddi_get_instance(devi)));

    if (ddi_getprop (DDI_DEV_T_NONE, devi, DDI_PROP_DONTPASS,
	"present", 0) <= 0)
	return (DDI_PROBE_FAILURE);

    p9000reg = &p9000;

    if (ddi_regs_map_setup(devi, 0,
	    (caddr_t *)&p9000reg->p9000_drawing_regp,
	    P9000_DRAWING_BASE, sizeof (drawing_t), &endian_attr,
	    &p9000_dr_handle) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT,
		"viper_vlb_probe: couldn't map drawing regs\n"));
	goto failure;
    }
    status |= P9000_PROBE_DID_MAP_DR;

    /* map in the framebuffer */
    if (ddi_regs_map_setup(devi, 0,
	    (caddr_t *)&p9000reg->p9000_frame_bufferp,
	    P9000_FRAME_BUFFER_BASE, P9000_FRAME_BUFFER_SIZE, &nosw_attr,
	    &p9000_fb_handle) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT,
		"viper_vlb_probe: couldn't map framebuffer\n"));
	goto failure;
    }
    status |= P9000_PROBE_DID_MAP_FB;

    if (ddi_regs_map_setup(devi, 1, (caddr_t *)&p9000reg->p9000_bt485_basep,
	    0, 0, &endian_attr, &p9000_bt_handle) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT,
		"viper_vlb_probe: couldn't map 485 dac\n"));
	goto failure;
    }
    status |= P9000_PROBE_DID_MAP_BT;

    p9000reg->p9000_seq_basep = p9000reg->p9000_bt485_basep = 0;
    viperport.vp_seq_index_port =
	p9000reg->p9000_seq_basep + VIPER_SEQ_INDEX_PORT;
    viperport.vp_seq_data_port =
	p9000reg->p9000_seq_basep + VIPER_SEQ_DATA_PORT;

    if (viper_enable(p9000_bt_handle, p9000reg, &viperport,
	    &viperoutctrl) != DDI_SUCCESS) {
	DEBUGF(2, (CE_CONT, "viper_vlb_probe: viper_enable failed\n"));
	goto failure;
    }
    status |= P9000_PROBE_DID_ENABLE;

    p9000reg->p9000_xy0 = 0L;
    temp = p9000reg->p9000_xy0;
    p9000reg->p9000_x0 = P9000_PROBE_VALUE_X;
    temp = p9000reg->p9000_x0;
    p9000reg->p9000_y0 = P9000_PROBE_VALUE_Y;
    temp = p9000reg->p9000_y0;
    if ((p9000reg->p9000_xy0 & (P9000_COORD_MASK | (P9000_COORD_MASK << 16)))
	  != (P9000_PROBE_VALUE_XY & (P9000_COORD_MASK | (P9000_COORD_MASK << 16))))
    {
	DEBUGF(2, (CE_CONT,"  p9000 mismatch of values.\n"));
	DEBUGF(2, (CE_CONT,"  Got 0x%x, expected 0x%x.\n",
	    newxy, P9000_PROBE_VALUE_XY));
	goto failure;
    }
    status |= P9000_PROBE_MATCH;
    
failure:
    if (status & P9000_PROBE_DID_ENABLE)
	(void)viper_disable(p9000_bt_handle, &viperport, viperoutctrl);

    if (status & P9000_PROBE_DID_MAP_DR)
	ddi_regs_map_free(&p9000_dr_handle);

    if (status & P9000_PROBE_DID_MAP_FB)
	ddi_regs_map_free(&p9000_fb_handle);

    if (status & P9000_PROBE_DID_MAP_BT)
	ddi_regs_map_free(&p9000_bt_handle);

    if (!(status & P9000_PROBE_MATCH))
	return (DDI_PROBE_FAILURE);
    
    DEBUGF(2, (CE_CONT, "viper_vlb_probe returning success\n"));
    return (DDI_PROBE_SUCCESS);
}

#define ddi_getb(h, p)		inb((int)(p))
#define ddi_putb(h, p, v)	outb((int)(p),(v))
#define ddi_rep_getb(h, f, t, l, i)	\
			(void)repinsb((int)(t),(u_char *)(f),(l))
#define ddi_rep_putb(h, f, t, l, i)	\
			(void)repoutsb((int)(t),(u_char *)(f),(l))

static int
viper_enable (register ddi_acc_handle_t seq_handle, p9000p_t p9000reg,
	viper_port_t *viperport, int *viperoutctrl)
{
    register u_char     newoutctrl;
    register u_char     basemem;
    register u_char     oldseq;
    u_long  physaddr;
    
    if (!viper_unlock_regs(seq_handle, viperport))
	return (DDI_FAILURE);

    if (viperport->vp_bustype != VIPER_PORT_BUS_PCI) {

	physaddr =
	    hat_getkpfnum((caddr_t)p9000reg->p9000_frame_bufferp);
    
	physaddr <<= 12;
	physaddr -= P9000_FRAME_BUFFER_BASE;
	/* compare the page numbers */
	switch (physaddr) {
	case (u_long) 0xa0000000:
	    basemem = VIPER_OUTCTRL_MEM_A0000000;
	    break;
	
	case (u_long) 0x20000000:
	    basemem = VIPER_OUTCTRL_MEM_20000000 ;
	    break;
	
	case (u_long) 0x80000000:
	    basemem = VIPER_OUTCTRL_MEM_80000000 ;
	    break;
	
	default:
	    return (DDI_FAILURE);
	}
	
	oldseq = ddi_getb (seq_handle, viperport->vp_seq_index_port);

	ddi_putb (seq_handle,
	    viperport->vp_seq_index_port, VIPER_SEQ_OUTCTRL_INDEX);

	if ((ddi_getb (seq_handle,
		viperport->vp_seq_index_port) & VIPER_SEQ_MASK)
		!= (VIPER_SEQ_OUTCTRL_INDEX & VIPER_SEQ_MASK)) {
	    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);
	    (void)viper_lock_regs(seq_handle, viperport);
	    return (DDI_FAILURE);
	}

	*viperoutctrl = ddi_getb (seq_handle,
	    viperport->vp_seq_data_port);

	/* use the reserve bits to enable memory for viper on VLB */
	newoutctrl = (*viperoutctrl & ~VIPER_OUTCTRL_MEM_BITS) | basemem;

	ddi_putb (seq_handle, viperport->vp_seq_data_port, newoutctrl);
	if (ddi_getb (seq_handle,
		viperport->vp_seq_data_port) != newoutctrl) {
	    ddi_putb (seq_handle, viperport->vp_seq_data_port,
		*viperoutctrl);
	    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);
	    (void)viper_lock_regs(seq_handle, viperport);
	    return (DDI_FAILURE);
	}
	ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);

    }

    if (!viper_lock_regs(seq_handle, viperport))
	return (DDI_FAILURE);

    return (DDI_SUCCESS);
}

static int
viper_disable (register ddi_acc_handle_t seq_handle,
	viper_port_t *viperport, int viperoutctrl)
{
    register u_char  oldseq;

    if (!viper_unlock_regs(seq_handle, viperport))
	return (DDI_FAILURE);

    if (viperport->vp_bustype != VIPER_PORT_BUS_PCI) {
	oldseq = ddi_getb (seq_handle, viperport->vp_seq_index_port);

	ddi_putb (seq_handle,
	    viperport->vp_seq_index_port, VIPER_SEQ_OUTCTRL_INDEX);
	if ((ddi_getb (seq_handle,
		viperport->vp_seq_index_port) & VIPER_SEQ_MASK)
		!= (VIPER_SEQ_OUTCTRL_INDEX & VIPER_SEQ_MASK)) {
	    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);
	    (void)viper_lock_regs(seq_handle, viperport);
	    return (DDI_FAILURE);
	}

	/* restore the original value in the VIPER MISC register */
	ddi_putb (seq_handle, viperport->vp_seq_data_port, viperoutctrl);

	if (ddi_getb (seq_handle,
		viperport->vp_seq_data_port) != viperoutctrl) {
	    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);
	    (void)viper_lock_regs(seq_handle, viperport);
	    return (DDI_FAILURE);
	}

	ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);
    }
    if (!viper_lock_regs(seq_handle, viperport))
	return (DDI_FAILURE);

    return (DDI_SUCCESS);
}


/*ARGSUSED*/
static int
viper_ioctl(register dev_t const dev, register int cmd, register int data,
	register int mode, register cred_t *const cred,
	register int *const rval)
{
    register struct p9000_softc *const	softc = getsoftc(getminor(dev));
    register int	status;
    
    switch (cmd) {
    case VIPERIO_GET_INIT:
	mutex_enter(&softc->mutex);
	status = ddi_copyout((caddr_t)&softc->viperinit,
			     (caddr_t)data,
			     sizeof (viper_init_t),
			     mode);
	mutex_exit(&softc->mutex);
	if (status)
	    return (EFAULT);
	break;
	
    case VIPERIO_PUT_INIT:
	mutex_enter(&softc->mutex);
	status = ddi_copyin((caddr_t)data,
			    (caddr_t)&softc->viperinit,
			    sizeof (viper_init_t),
			    mode);
	if (!status) {
	    softc->w = softc->viperinit.vi_width;
	    softc->h = softc->viperinit.vi_height;
	    softc->size = softc->h *
		P9000_LINEBYTES(softc->viperinit.vi_sysconfig);
	}
	mutex_exit(&softc->mutex);
	if (status)
	    return (EFAULT);
	break;
	
    case VIPERIO_GET_PORTS:
	status = ddi_copyout((caddr_t)&softc->viperport,
			     (caddr_t)data,
			     sizeof (viper_port_t),
			     mode);
	if (status)
	    return (EFAULT);
	break;
	
    case VIPERIO_GRAPH_MODE:
	mutex_enter(&softc->mutex);
	status = viper_enter_graph_mode (softc);
	mutex_exit(&softc->mutex);
	if (status)
	    return (status);
	break;
	
    case VIPERIO_TEXT_MODE:
	mutex_enter(&softc->mutex);
	status = viper_enter_text_mode (softc);
	mutex_exit(&softc->mutex);
	if (status)
	    return (status);
	break;
	
    case 250:       /* for compatibility */
	case VIPERIO_SET_ICD_CLOCK:
	mutex_enter(&softc->mutex);
	viper_write_ic2061a (softc, data);
	mutex_exit(&softc->mutex);
	break;
	
    default:
	return (ENOTTY);
    }
    return (0);
}


/*
 * enable/disable/update HW cursor
 */
static void
viper_setcurpos(register struct p9000_softc *const softc)
{
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;

    if (softc->cur.enable) {
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_low,
	      (softc->cur.pos.x - softc->cur.hot.x + 32) & 0xff);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_high,
	      (softc->cur.pos.x - softc->cur.hot.x + 32) >> 8);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_low,
	      (softc->cur.pos.y - softc->cur.hot.y + 32) & 0xff);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_high,
	      (softc->cur.pos.y - softc->cur.hot.y + 32) >> 8);
    }
    else {
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_low,
	      BT485_CURSOR_OFFPOS & 0xff);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_high,
	      BT485_CURSOR_OFFPOS >> 8);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_low,
	      BT485_CURSOR_OFFPOS & 0xff);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_high,
	      BT485_CURSOR_OFFPOS >> 8);
    }
}

/*
 * load HW cursor bitmaps
 */
static void
viper_setcurshape(register struct p9000_softc *const softc)
{
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;
    register u_long     edge = 0xffffffffUL;
    register u_long	*image, *mask;
    register int	i;
    u_long		tmp;
    
    /* compute right edge mask */
    if (softc->cur.size.x < 32)
	edge = (1 << softc->cur.size.x) - 1;
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_ram_write, 0);
    for (i = 0, image = softc->cur.image;
	 i < 32; i++, image++) {
	tmp = *image;
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[tmp & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 8) & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 16) & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 24) & 0xff]);
    }
    
    for (i = 0, mask = softc->cur.mask; i < 32; i++, mask++) {
	tmp = *mask & edge;
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[tmp & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 8) & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 16) & 0xff]);
	ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_data,
	      p9000_bitflip[(tmp >> 24) & 0xff]);
    }
}


static void
viper_setcurcolor(register struct p9000_softc *const softc)
{
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;

    ddi_putb (bt_handle,
	softc->viperport.vp_bt485_color_write, BT485_CURSOR1_COLOR);

    ddi_rep_putb (bt_handle, (u_char *) softc->omap,
	softc->viperport.vp_bt485_color_data, sizeof (softc->omap),
	DDI_DEV_NO_AUTOINCR);
}


static void
viper_setpalet(register struct p9000_softc *const softc)
{
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;

    ddi_putb (bt_handle,
	softc->viperport.vp_bt485_ram_write, softc->cmap_index);

    ddi_rep_putb (bt_handle, (u_char *) &softc->cmap[softc->cmap_index],
	softc->viperport.vp_bt485_palet_data, softc->cmap_count * 3,
	DDI_DEV_NO_AUTOINCR);
}


static  int
viper_enter_graph_mode(register struct p9000_softc *const softc)
{
    register p9000p_t	const p9000reg = &softc->p9000;
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;
    register ddi_acc_handle_t seq_handle = softc->p9000_seq_handle;
    register int	value;
    
    if (!softc->viperinit.vi_valid)
	return (EIO);
    
    DEBUGF(1, (CE_CONT, "viper_enter_graph_mode\n"));
    /* enable Command Reg3 & 8bit DAC & off */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg0,
	  BT485_COMMAND_REGISTER_3_ENABLE | BT485_DAC_8_BIT_RESOLUTION |
	  BT485_POWER_DOWN_ENABLE);
    
    ddi_putb (bt_handle,
	softc->viperport.vp_bt485_ram_write, BT485_COMREG3_SELECT);
    
    /* Set clock multiplier */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg3,
	  softc->viperinit.vi_vclk & VIPER_INIT_VCLK_DOUBLER ?
	  BT485_2X_CLOCK_MULTIPLIER_ENABLED : 0);
    
    /* Select Viper video clock, x windows cursor */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg2,
	  BT485_PORTSEL_UNMASKED | BT485_PCLK1_SELECTED |
	  BT485_TWO_COLOR_X_WINDOWS_CURSOR);

    switch (softc->viperinit.vi_depth) {
    case 4:     /* 4 bit pixels, use palette */
	ddi_putb (bt_handle,
	    softc->viperport.vp_bt485_comreg1, BT485_4_BIT_PIXELS);
	break;
    
    case 8:     /* 8 bit pixels, use palette */
    default:
	ddi_putb (bt_handle,
	    softc->viperport.vp_bt485_comreg1, BT485_8_BIT_PIXELS);
	break;
    
    case 16:    /* 16 bit (5:5:5) pixels, bypass palette */
	ddi_putb (bt_handle,
	    softc->viperport.vp_bt485_comreg1,
	    BT485_16_BIT_PIXELS | BT485_TRUE_COLOR_ENABLE);
	break;
    
    case 24:    /* 24 bit (8:8:8) pixels, bypass palette */
    case 32:
	ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg1,
	    BT485_24_BIT_PIXELS | BT485_TRUE_COLOR_ENABLE);
	break;
    }
    
    /* pixel read mask */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_pixel_mask, 0xff);
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_low,
	  BT485_CURSOR_OFFPOS & 0xff);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_high,
	  BT485_CURSOR_OFFPOS >> 8);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_low,
	  BT485_CURSOR_OFFPOS & 0xff);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_high,
	  BT485_CURSOR_OFFPOS >> 8);
    
    viper_unlock_regs(seq_handle, &softc->viperport);
    
    if (softc->viperport.vp_bustype != VIPER_PORT_BUS_PCI) {
    
	/* select output reg */
	ddi_putb (seq_handle, softc->viperport.vp_seq_index_port,
	    VIPER_SEQ_OUTCTRL_INDEX);
	
	value = ddi_getb (seq_handle, softc->viperport.vp_seq_data_port);
	
	value &= VIPER_OUTCTRL_MEM_BITS | VIPER_OUTCTRL_RESERVED_BITS;
	if (softc->viperinit.vi_vclk & VIPER_INIT_VCLK_HSYNC)
	    value |= VIPER_OUTCTRL_P9000_HSYNC_POLARITY;
	if (softc->viperinit.vi_vclk & VIPER_INIT_VCLK_VSYNC)
	    value |= VIPER_OUTCTRL_P9000_VSYNC_POLARITY;
	value |= VIPER_OUTCTRL_P9000_VIDEO_ENABLE;
	
	ddi_putb (seq_handle, softc->viperport.vp_seq_data_port, value);
    }
    
    else {      /* PCI */
	
	/* select output reg */
	ddi_putb (seq_handle, softc->viperport.vp_seq_index_port,
	    VIPER_SEQ_OUTCTRL_INDEX);
	
	value = ddi_getb (seq_handle, softc->viperport.vp_seq_data_port);
	
	value |= VIPER_OUTCTRL_P9000_VIDEO_ENABLE;
	
	ddi_putb (seq_handle, softc->viperport.vp_seq_data_port, value);
	
	value = ddi_getb (seq_handle, softc->viperport.vp_miscin);
	
	value &= ~(VIPER_MISC_VSYNC_POLARITY | VIPER_MISC_HSYNC_POLARITY);
	
	if (softc->viperinit.vi_vclk & VIPER_INIT_VCLK_HSYNC)
	    value |= VIPER_MISC_HSYNC_POLARITY;
	if (softc->viperinit.vi_vclk & VIPER_INIT_VCLK_VSYNC)
	    value |= VIPER_MISC_VSYNC_POLARITY;
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout, value);
    }
    
    (void)viper_lock_regs(seq_handle, &softc->viperport);
    
    viper_write_ic2061a (softc, softc->viperinit.vi_vclk >>
			 VIPER_INIT_VCLK_DOT_FREQ_SHIFT);
    
    viper_write_ic2061a (softc, softc->viperinit.vi_memspeed);
    
    /* note: this needs to be at least 20milliseconds?*/
    delay (drv_usectohz (20000));
    
    p9000reg->p9000_sysconfig = softc->viperinit.vi_sysconfig;
    
    /* INTERRUPT_EN = disabled */
    p9000reg->p9000_interrupt_en = P9000_INTEN_MEN_CTRL |
    P9000_INTEN_VBLANKED_EN_CTRL | P9000_INTEN_PICKED_EN_CTRL |
    P9000_INTEN_DE_IDLE_EN_CTRL;
    
    p9000reg->p9000_interrupt = P9000_INT_VBLANKED_CTRL |
    P9000_INT_PICKED_CTRL | P9000_INT_DE_IDLE_CTRL;
    
    p9000reg->p9000_prehrzc = softc->viperinit.vi_prehrzc;
    p9000reg->p9000_prevrtc = softc->viperinit.vi_prevrtc;
    p9000reg->p9000_hrzsr = softc->viperinit.vi_hrzsr;
    p9000reg->p9000_hrzbr = softc->viperinit.vi_hrzbr;
    p9000reg->p9000_hrzbf = softc->viperinit.vi_hrzbf;
    p9000reg->p9000_hrzt = softc->viperinit.vi_hrzt;
    p9000reg->p9000_vrtsr = softc->viperinit.vi_vrtsr;
    p9000reg->p9000_vrtbr = softc->viperinit.vi_vrtbr;
    p9000reg->p9000_vrtbf = softc->viperinit.vi_vrtbf;
    p9000reg->p9000_vrtt = softc->viperinit.vi_vrtt;
    p9000reg->p9000_rfperiod = 0x186;
    p9000reg->p9000_rlmax = 0xfa;
    p9000reg->p9000_mem_config = softc->viperinit.vi_memconfig;
    p9000reg->p9000_srtctl = softc->viperinit.vi_srtctl;
    
    /* disable Command Reg3 & 8bit DAC & on */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg0,
	BT485_DAC_8_BIT_RESOLUTION);
    
    return (0);
}


static  int
viper_enter_text_mode(register struct p9000_softc *const softc)
{
    register p9000p_t	const p9000reg = &softc->p9000;
    register ddi_acc_handle_t bt_handle = softc->p9000_bt_handle;
    register ddi_acc_handle_t seq_handle = softc->p9000_seq_handle;
    register int	value;
    
    /* Select viper video clock, cursor off! */
    ddi_putb (bt_handle,
	softc->viperport.vp_bt485_comreg2, BT485_PORTSEL_UNMASKED |
	  BT485_PCLK1_SELECTED | BT485_CURSOR_DISABLED);
    
    /* disable video in the video controller */
    p9000reg->p9000_srtctl = P9000_SRTCTL_INTERNAL_VSYNC |
			  P9000_SRTCTL_INTERNAL_HSYNC |
			  P9000_SRTCTL_COMPOSITE | 4;
    
    viper_unlock_regs(seq_handle, &softc->viperport);
    
    if (softc->viperport.vp_bustype != VIPER_PORT_BUS_PCI) {
    
	/* select output reg */
	ddi_putb (seq_handle, softc->viperport.vp_seq_index_port,
	    VIPER_SEQ_OUTCTRL_INDEX);
	
	value = ddi_getb (seq_handle, softc->viperport.vp_seq_data_port);
	value &= VIPER_OUTCTRL_MEM_BITS | VIPER_OUTCTRL_RESERVED_BITS;
	value |= VIPER_OUTCTRL_5186_VIDEO_ENABLE;
	
	ddi_putb (seq_handle, softc->viperport.vp_seq_data_port, value);
    }
    
    else {      /* PCI */
	/* select output reg */
	ddi_putb (seq_handle, softc->viperport.vp_seq_index_port,
	    VIPER_SEQ_OUTCTRL_INDEX);
	
	value = ddi_getb (seq_handle, softc->viperport.vp_seq_data_port);
	value &= ~VIPER_OUTCTRL_P9000_VIDEO_ENABLE;
	ddi_putb (seq_handle, softc->viperport.vp_seq_data_port, value);
    }
    
    (void)viper_lock_regs(seq_handle, &softc->viperport);
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout, 0x67);
    
    /* wait for ICD to settle some */
    /* note: this needs to be at least 20milliseconds?*/
    delay (drv_usectohz (20000));
    
    /* enable Command Reg3 */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg0,
	BT485_COMMAND_REGISTER_3_ENABLE);
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_ram_write,
	BT485_COMREG3_SELECT);
    
    /* set clock multiplier */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg3, 0);
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg1, 0);
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_comreg2, 0);
    
    /* pixel read mask */
    ddi_putb (bt_handle, softc->viperport.vp_bt485_pixel_mask, 0xff);
    
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_low,
	  BT485_CURSOR_OFFPOS & 0xff);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_x_high,
	  BT485_CURSOR_OFFPOS >> 8);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_low,
	  BT485_CURSOR_OFFPOS & 0xff);
    ddi_putb (bt_handle, softc->viperport.vp_bt485_cursor_y_high,
	  BT485_CURSOR_OFFPOS >> 8);
    
    /* select vga clock mode register */
    ddi_putb (seq_handle, softc->viperport.vp_seq_index_port, 1);

    /* reenable vga video */
    ddi_putb (seq_handle, softc->viperport.vp_seq_data_port,
	ddi_getb (seq_handle, softc->viperport.vp_seq_data_port) & ~0x20);
    
    return (0);
}


#define P9000_UNLOCK_DID_GET_CLOCK  0x0001
#define P9000_UNLOCK_SUCCESS        0x0002

static int
viper_unlock_regs (register ddi_acc_handle_t seq_handle,
		    viper_port_t *viperport)
{
    register u_char     oldseq;
    register u_char     oldmisc;
    register u_char     oldclockmode;
    register u_char     newmisc;
    register int        status = 0;

    oldseq = ddi_getb (seq_handle, viperport->vp_seq_index_port);

    ddi_putb (seq_handle, viperport->vp_seq_index_port, 0x01);
    if ((ddi_getb (seq_handle,
	    viperport->vp_seq_index_port) & VIPER_SEQ_MASK) != 0x01) {
	DEBUGF(2,
	    (CE_CONT, " setting clockmode seq failed, returning failure\n"));
	goto failure;
    }

    oldclockmode = ddi_getb (seq_handle, viperport->vp_seq_data_port);
    status |= P9000_UNLOCK_DID_GET_CLOCK;

    /* select misc reg */

    ddi_putb (seq_handle,
	viperport->vp_seq_index_port, VIPER_SEQ_MISC_INDEX);

    if ((ddi_getb (seq_handle,
	    viperport->vp_seq_index_port) & VIPER_SEQ_MASK)
	    != (VIPER_SEQ_MISC_INDEX & VIPER_SEQ_MASK)) {
	DEBUGF(2,
	    (CE_CONT, " setting viper seq misc failed, returning failure\n"));
	goto failure;
    }

    /* save misc reg in case its unlocked already */
    oldmisc = ddi_getb (seq_handle, viperport->vp_seq_data_port);

    /* unlock misc reg via 2 writes */

    ddi_putb (seq_handle, viperport->vp_seq_data_port, oldmisc);
    ddi_putb (seq_handle, viperport->vp_seq_data_port, oldmisc);

    /* get the value */

    newmisc = ddi_getb (seq_handle, viperport->vp_seq_data_port);
    newmisc &= ~VIPER_MISC_CRLOCK;

    /* clear bit 5 to unlock extended regs */

    ddi_putb (seq_handle, viperport->vp_seq_data_port, newmisc);
    if (ddi_getb (seq_handle, viperport->vp_seq_data_port) != newmisc) {
	DEBUGF(2, (CE_CONT, " setting viper misc failed, returning failure\n"));
	ddi_putb (seq_handle, viperport->vp_seq_data_port, oldmisc);
	goto failure;
    }

    status |= P9000_UNLOCK_SUCCESS;

failure:

    if (status & P9000_UNLOCK_DID_GET_CLOCK) {
	ddi_putb (seq_handle, viperport->vp_seq_index_port, 0x01);
	ddi_putb (seq_handle, viperport->vp_seq_data_port, oldclockmode);
    }

    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);

    return ((status & P9000_UNLOCK_SUCCESS) != 0);
}


#define P9000_LOCK_DID_GET_CLOCK  0x0001
#define P9000_LOCK_SUCCESS        0x0002

static int
viper_lock_regs (register ddi_acc_handle_t seq_handle,
		    viper_port_t *viperport)
{
    register u_char     oldseq;
    register u_char     oldmisc;
    register u_char     oldclockmode;
    register int        status = 0;

    oldseq = ddi_getb (seq_handle, viperport->vp_seq_index_port);

    ddi_putb (seq_handle, viperport->vp_seq_index_port, 0x01);
    if ((ddi_getb (seq_handle,
	    viperport->vp_seq_index_port) & VIPER_SEQ_MASK) != 0x01) {
	DEBUGF(2,
	    (CE_CONT, " setting clockmode seq failed, returning failure\n"));
	goto failure;
    }

    oldclockmode = ddi_getb (seq_handle, viperport->vp_seq_data_port);
    status |= P9000_LOCK_DID_GET_CLOCK;

    /* select misc reg */

    ddi_putb (seq_handle,
	viperport->vp_seq_index_port, VIPER_SEQ_MISC_INDEX);

    if ((ddi_getb (seq_handle,
	    viperport->vp_seq_index_port) & VIPER_SEQ_MASK)
	    != (VIPER_SEQ_MISC_INDEX & VIPER_SEQ_MASK)) {
	DEBUGF(2,
	    (CE_CONT, " setting viper seq misc failed, returning failure\n"));
	goto failure;
    }

    /* get the value */
    oldmisc = ddi_getb (seq_handle, viperport->vp_seq_data_port);

    /* set bit 5 to lock extended regs */

    ddi_putb (seq_handle, viperport->vp_seq_data_port,
	oldmisc | VIPER_MISC_CRLOCK);

    status |= P9000_LOCK_SUCCESS;

failure:

    if (status & P9000_LOCK_DID_GET_CLOCK) {
	ddi_putb (seq_handle, viperport->vp_seq_index_port, 0x01);
	ddi_putb (seq_handle, viperport->vp_seq_data_port, oldclockmode);
    }

    ddi_putb (seq_handle, viperport->vp_seq_index_port, oldseq);

    return ((status & P9000_LOCK_SUCCESS) != 0);
}


static  void
viper_write_ic2061a (register struct p9000_softc *const softc,
	register u_long const clockval)
{
    register ddi_acc_handle_t seq_handle = softc->p9000_seq_handle;
    register u_long	data;
    register int	i;
    register u_char	oldstate;
    register int	flags;
    
    
    oldstate = ddi_getb(seq_handle,
	softc->viperport.vp_miscin) & ~(VIPER_MISC_DATA | VIPER_MISC_CLOCK);
    flags = ddi_enter_critical();
    
    /*
     * The programming requirements of the icd2061a appear to be
     * that  the  time between outs must be at least 30ns (setup
     * 20ns and hold time 10ns),  and  must  be  less  than  the
     * timeout  specification  2ms.   Hence  we will sprinkle in
     * calls  to   delay   1us.    In   reality,   the   current
     * implementation of drv_usecwait will delay 10us.
     */
     
    /* First, send the "Unlock sequence" to the clock chip */
    
    /* Raise the data bit */
    
    ddi_putb (seq_handle,
	softc->viperport.vp_miscout, oldstate | VIPER_MISC_DATA);
    drv_usecwait (1);
    
    /* Send at least 5 unlock bits */
    
    for (i = 0; i < 5; i++) {
    
	/* Hold the data on while lowering and rasing the clock */
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	    oldstate | VIPER_MISC_DATA);

	drv_usecwait (1);

	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	    oldstate | VIPER_MISC_DATA | VIPER_MISC_CLOCK);

	drv_usecwait (1);
    }
    
    /* Then turn the data and clock off */
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout, oldstate);
    drv_usecwait (1);
    
    /* And turn the clock on one more time */
    
    ddi_putb (seq_handle,
	softc->viperport.vp_miscout, oldstate | VIPER_MISC_CLOCK);
    drv_usecwait (1);
    
    /* Now send start bit */
    
    /* Leave data off, and lower the clock */
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout, oldstate);
    drv_usecwait (1);
    
    /* Leave data off, and raise the clock */
    
    ddi_putb (seq_handle,
	softc->viperport.vp_miscout, oldstate | VIPER_MISC_CLOCK);
    drv_usecwait (1);
    
    /* Next, send the 24 data bits */
    
    for (i = 0, data = clockval << 3; i < 24; i++, data >>= 1) {
    
	/* Leaving the clock high, raise the inverse of the data bit. */
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	      oldstate | VIPER_MISC_CLOCK | (~data & VIPER_MISC_DATA));
	drv_usecwait (1);
	
	/* Leaving the inverse data in place, lower the clock */
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	    oldstate | (~data & VIPER_MISC_DATA));
	drv_usecwait (1);
	
	/* Leaving the clock low, raise the data bit */
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	    oldstate | (data & VIPER_MISC_DATA));
	drv_usecwait (1);
	
	/* Leaving the data bit in place, raise the clock */
	
	ddi_putb (seq_handle, softc->viperport.vp_miscout,
	      oldstate | VIPER_MISC_CLOCK | (data & VIPER_MISC_DATA));
	drv_usecwait (1);
	
	/* Get the next bit of data */
    }
    
    /* Leaving the clock high, raise the data bit */
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout,
	oldstate | VIPER_MISC_DATA | VIPER_MISC_CLOCK);
    drv_usecwait (1);
    
    /* Leaving the data high, clock the clock low, then high again */
    
    ddi_putb (seq_handle,
	softc->viperport.vp_miscout, oldstate | VIPER_MISC_DATA);
    drv_usecwait (1);
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout,
	oldstate | VIPER_MISC_DATA | VIPER_MISC_CLOCK);
    drv_usecwait (1);
    
    /* Selecting both serial control bits to 1 will */
    /* select register 2 frequency		  */
    
    ddi_putb (seq_handle, softc->viperport.vp_miscout,
	oldstate | VIPER_MISC_DATA | VIPER_MISC_CLOCK);
    
    ddi_exit_critical(flags);
    
    /* The clock settings become  effective  when  the  watchdog */
    /* timer times out, 2ms.				     */
    
    delay (drv_usectohz (2000));
}
#undef ddi_getb
#undef ddi_putb
#undef ddi_rep_putb
#undef ddi_rep_putb
