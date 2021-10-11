/*
 * Copyright 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_P9000REG_H
#define	_P9000REG_H

#pragma ident	"@(#)p9000reg.h	1.6	95/07/28 SMI"

/*
 * The following describes the structure of the  Weitek  Power  9000
 * User Interface Controller.
 *
 * Some of the addresses of the P9000/P9100 depend on the byte and bit
 * ordering of the host.  The values _LITTLE_ENDIAN and _BIG_ENDIAN
 * from isa_defs.h should set the right  addresses.   The endianess
 * can be forced by setting P9000_LITTLE_ENDIAN to 0 or 1.  The
 * values P9000_ENDIAN* can be set to a constant if they are wrong
 * for your machine.  If its desired to determine this at runtime,
 * set the #define P9000_ENDIAN* to the runtime variable containing
 * a 0-7 for the value that is in bits 16-18 of the address
 * (P9000_ADDR_SWAP_*).
 *
 * In the comments below, a hex digit of 'z' indicates that  bit  19
 * is 0, and bits 16-18 are the bit swap, byte swap, half swap
 * values.  Hence, 'z' will generally be '0' and 's' will be '8'.
 */

#include <sys/types.h>
#include <sys/isa_defs.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef volatile struct p9000_control_regs {

	/* System Control Registers */

	u_long : 32;                            /* 0x000z0000    */
	u_long volatile p9000_cr_sysconfig;                        /* 0x000z0004 rw */
	u_long volatile p9000_cr_interrupt;               /* 0x000z0008 rw */
	u_long volatile p9000_cr_interrupt_en;            /* 0x000z000c rw */
	u_long volatile p9000_cr_filler1[0xf0/4];         /* 0x000z0010    */

	/* Video Control Registers */

	u_long volatile p9000_cr_filler2;                 /* 0x000z0100 hg */
	u_long volatile p9000_cr_hrzc;                    /* 0x000z0104 ro */
	u_long volatile p9000_cr_hrzt;                    /* 0x000z0108 rw */
	u_long volatile p9000_cr_hrzsr;                   /* 0x000z010c rw */
	u_long volatile p9000_cr_hrzbr;                   /* 0x000z0110 rw */
	u_long volatile p9000_cr_hrzbf;                   /* 0x000z0114 rw */
	u_long volatile p9000_cr_prehrzc;                 /* 0x000z0118 rw */
	u_long volatile p9000_cr_vrtc;                    /* 0x000z011c ro */
	u_long volatile p9000_cr_vrtt;                    /* 0x000z0120 rw */
	u_long volatile p9000_cr_vrtsr;                   /* 0x000z0124 rw */
	u_long volatile p9000_cr_vrtbr;                   /* 0x000z0128 rw */
	u_long volatile p9000_cr_vrtbf;                   /* 0x000z012c rw */
	u_long volatile p9000_cr_prevrtc;                 /* 0x000z0130 rw */
	u_long volatile p9000_cr_sraddr;                  /* 0x000z0134 ro */
	u_long volatile p9000_cr_srtctl;                  /* 0x000z0138 rw */
	u_long volatile p9000_cr_qsfcounter;              /* 0x000z013c ro */
	u_long volatile p9000_cr_filler3[0x40/4];         /* 0x000z0140 hg */

	/* Vram Control Registers */

	u_long volatile : 32;                            /* 0x000z0180    */
	u_long volatile p9000_cr_mem_config;              /* 0x000z0184 rw */
	u_long volatile p9000_cr_rfperiod;                /* 0x000z0188 rw */
	u_long volatile p9000_cr_rfcount;                 /* 0x000z018c ro */
	u_long volatile p9000_cr_rlmax;                   /* 0x000z0190 rw */
	u_long volatile p9000_cr_rlcur;                   /* 0x000z0194 ro */
	u_long volatile p9000_cr_filler4[0x68/4];         /* 0x000z0198    */

	/* Ramdac Control Registers (used only on 9100) */

	u_long volatile p9000_cr_ramdac[16];              /* 0x000z0200 rw */

	u_long volatile p9000_cr_filler5[0xfdc0/4];       /* 0x000z0210    */
} p9000_control_regs_t;                                   /* 0x000s0000    */


typedef volatile struct p9000_drawing_regs  {
	u_long volatile p9000_dr_status;                  /* 0x000s0000 ro */
	u_long volatile p9000_dr_blit;                    /* 0x000s0004 ro */
	u_long volatile p9000_dr_quad;                    /* 0x000s0008 ro */
	u_long volatile p9000_dr_pixel8;                  /* 0x000s000c wo */
	u_long volatile : 32;                            /* 0x000s0010    */
	u_long volatile p9000_dr_next_pixels;             /* 0x000s0014 wo */
	u_long volatile p9000_dr_filler6[0x68/4];         /* 0x000s0018    */
	u_long volatile p9000_dr_pixel1[32];              /* 0x000s0080 wo */
	u_long volatile p9000_dr_filler7[0x80/4];         /* 0x000s0100    */

	/* Parameter Engine Control Registers */

	u_long volatile : 32;                            /* 0x000s0180    */
	u_long volatile p9000_dr_oor;                     /* 0x000s0184 ro */
	u_long volatile : 32;                            /* 0x000s0188    */
	u_long volatile p9000_dr_cindex;                  /* 0x000s018c rw */
	long   volatile p9000_dr_w_off_xy;                /* 0x000s0190 rw */
	long   volatile p9000_dr_pe_w_min;                /* 0x000s0194 ro */
	long   volatile p9000_dr_pe_w_max;                /* 0x000s0198 ro */
	long   : 32;                            /* 0x000s019c    */
	u_long volatile p9000_dr_yclip;                   /* 0x000s01a0 ro */
	u_long volatile p9000_dr_xclip;                   /* 0x000s01a4 ro */
	u_long volatile p9000_dr_xedge_lt;                /* 0x000s01a8 ro */
	u_long volatile p9000_dr_xedge_gt;                /* 0x000s01ac ro */
	u_long volatile p9000_dr_yedge_lt;                /* 0x000s01b0 ro */
	u_long volatile p9000_dr_yedge_gt;                /* 0x000s01b4 ro */
	u_long volatile p9000_dr_filler8[0x48/4];         /* 0x000s01b8    */

	/* Drawing Engine Pixel Processing Registers */

	u_long volatile p9000_dr_fground;                 /* 0x000s0200 rw */
	u_long volatile p9000_dr_bground;                 /* 0x000s0204 rw */
	u_long volatile p9000_dr_pmask;                   /* 0x000s0208 rw */
	u_long volatile p9000_dr_draw_mode;               /* 0x000s020c rw */
	long   volatile p9000_dr_pat_originx;             /* 0x000s0210 rw */
	long   volatile p9000_dr_pat_originy;             /* 0x000s0214 rw */
	u_long volatile p9000_dr_raster;                  /* 0x000s0218 rw */
	u_long volatile p9000_dr_pixel8_reg;              /* 0x000s021c rw */
	long   volatile p9000_dr_w_min;                   /* 0x000s0220 rw */
	long   volatile p9000_dr_w_max;                   /* 0x000s0224 rw */
	long   volatile p9000_dr_filler9[0x58/4];         /* 0x000s0228    */
	u_long volatile p9000_dr_pattern[8];              /* 0x000s0280 rw */
	long   volatile p9000_dr_filler10[0xd60/4];       /* 0x000s02a0    */

	/* Parameter Engine Coordinate Registers */

	long   : 32;                            /* 0x000s1000    */
	long   : 32;                            /* 0x000s1004    */
	long   volatile p9000_dr_x0;                      /* 0x000s1008 rw */
	long   : 32;                            /* 0x000s100c    */
	long   volatile p9000_dr_y0;                      /* 0x000s1010 rw */
	long   : 32;                            /* 0x000s1014    */
	long   volatile p9000_dr_xy0;                     /* 0x000s1018 rw */
	long   : 32;                            /* 0x000s101c    */
	long   : 32;                            /* 0x000s1020    */
	long   : 32;                            /* 0x000s1024    */
	long   volatile p9000_dr_rel_x0;                  /* 0x000s1028 wo */
	long   : 32;                            /* 0x000s102c    */
	long   volatile p9000_dr_rel_y0;                  /* 0x000s1030 wo */
	long   : 32;                            /* 0x000s1034    */
	long   volatile p9000_dr_rel_xy0;                 /* 0x000s1038 wo */
	long   : 32;                            /* 0x000s103c    */
	long   : 32;                            /* 0x000s1040    */
	long   : 32;                            /* 0x000s1044    */
	long   volatile p9000_dr_x1;                      /* 0x000s1048 rw */
	long   : 32;                            /* 0x000s104c    */
	long   volatile p9000_dr_y1;                      /* 0x000s1050 rw */
	long   : 32;                            /* 0x000s1054    */
	long   volatile p9000_dr_xy1;                     /* 0x000s1058 rw */
	long   : 32;                            /* 0x000s105c    */
	long   : 32;                            /* 0x000s1060    */
	long   : 32;                            /* 0x000s1064    */
	long   volatile p9000_dr_rel_x1;                  /* 0x000s1068 wo */
	long   : 32;                            /* 0x000s106c    */
	long   volatile p9000_dr_rel_y1;                  /* 0x000s1070 wo */
	long   : 32;                            /* 0x000s1074    */
	long   volatile p9000_dr_rel_xy1;                 /* 0x000s1078 wo */
	long   : 32;                            /* 0x000s107c    */
	long   : 32;                            /* 0x000s1080    */
	long   : 32;                            /* 0x000s1084    */
	long   volatile p9000_dr_x2;                      /* 0x000s1088 rw */
	long   : 32;                            /* 0x000s108c    */
	long   volatile p9000_dr_y2;                      /* 0x000s1090 rw */
	long   : 32;                            /* 0x000s1094    */
	long   volatile p9000_dr_xy2;                     /* 0x000s1098 rw */
	long   : 32;                            /* 0x000s109c    */
	long   : 32;                            /* 0x000s10a0    */
	long   : 32;                            /* 0x000s10a4    */
	long   volatile p9000_dr_rel_x2;                  /* 0x000s10a8 wo */
	long   : 32;                            /* 0x000s10ac    */
	long   volatile p9000_dr_rel_y2;                  /* 0x000s10b0 wo */
	long   : 32;                            /* 0x000s10b4    */
	long   volatile p9000_dr_rel_xy2;                 /* 0x000s10b8 wo */
	long   : 32;                            /* 0x000s10bc    */
	long   : 32;                            /* 0x000s10c0    */
	long   : 32;                            /* 0x000s10c4    */
	long   volatile p9000_dr_x3;                      /* 0x000s10c8 rw */
	long   : 32;                            /* 0x000s10cc    */
	long   volatile p9000_dr_y3;                      /* 0x000s10d0 rw */
	long   : 32;                            /* 0x000s10d4    */
	long   volatile p9000_dr_xy3;                     /* 0x000s10d8 rw */
	long   : 32;                            /* 0x000s10dc    */
	long   : 32;                            /* 0x000s10e0    */
	long   : 32;                            /* 0x000s10e4    */
	long   volatile p9000_dr_rel_x3;                  /* 0x000s10e8 wo */
	long   : 32;                            /* 0x000s10ec    */
	long   volatile p9000_dr_rel_y3;                  /* 0x000s10f0 wo */
	long   : 32;                            /* 0x000s10f4    */
	long   volatile p9000_dr_rel_xy3;                 /* 0x000s10f8 wo */
	long   : 32;                            /* 0x000s10fc    */
	long   volatile p9000_dr_filler11[0x100/4];       /* 0x000s1100    */

	/* Meta Coordinate Pseudo Registers */

	long   : 32;                            /* 0x000s1200    */
	long   : 32;                            /* 0x000s1204    */
	long   volatile p9000_dr_point_x;                 /* 0x000s1208 wo */
	long   : 32;                            /* 0x000s120c    */
	long   volatile p9000_dr_point_y;                 /* 0x000s1210 wo */
	long   : 32;                            /* 0x000s1214    */
	long   volatile p9000_dr_point_xy;                /* 0x000s1218 wo */
	long   : 32;                            /* 0x000s121c    */
	long   : 32;                            /* 0x000s1220    */
	long   : 32;                            /* 0x000s1224    */
	long   volatile p9000_dr_point_rel_x;             /* 0x000s1228 wo */
	long   : 32;                            /* 0x000s122c    */
	long   volatile p9000_dr_point_rel_y;             /* 0x000s1230 wo */
	long   : 32;                            /* 0x000s1234    */
	long   volatile p9000_dr_point_rel_xy;            /* 0x000s1238 wo */
	long   : 32;                            /* 0x000s123c    */
	long   : 32;                            /* 0x000s1240    */
	long   : 32;                            /* 0x000s1244    */
	long   volatile p9000_dr_line_x;                  /* 0x000s1248 wo */
	long   : 32;                            /* 0x000s124c    */
	long   volatile p9000_dr_line_y;                  /* 0x000s1250 wo */
	long   : 32;                            /* 0x000s1254    */
	long   volatile p9000_dr_line_xy;                 /* 0x000s1258 wo */
	long   : 32;                            /* 0x000s125c    */
	long   : 32;                            /* 0x000s1260    */
	long   : 32;                            /* 0x000s1264    */
	long   volatile p9000_dr_line_rel_x;              /* 0x000s1268 wo */
	long   : 32;                            /* 0x000s126c    */
	long   volatile p9000_dr_line_rel_y;              /* 0x000s1270 wo */
	long   : 32;                            /* 0x000s1274    */
	long   volatile p9000_dr_line_rel_xy;             /* 0x000s1278 wo */
	long   : 32;                            /* 0x000s127c    */
	long   : 32;                            /* 0x000s1280    */
	long   : 32;                            /* 0x000s1284    */
	long   volatile p9000_dr_tri_x;                   /* 0x000s1288 wo */
	long   : 32;                            /* 0x000s128c    */
	long   volatile p9000_dr_tri_y;                   /* 0x000s1290 wo */
	long   : 32;                            /* 0x000s1294    */
	long   volatile p9000_dr_tri_xy;                  /* 0x000s1298 wo */
	long   : 32;                            /* 0x000s129c    */
	long   : 32;                            /* 0x000s12a0    */
	long   : 32;                            /* 0x000s12a4    */
	long   volatile p9000_dr_tri_rel_x;               /* 0x000s12a8 wo */
	long   : 32;                            /* 0x000s12ac    */
	long   volatile p9000_dr_tri_rel_y;               /* 0x000s12b0 wo */
	long   : 32;                            /* 0x000s12b4    */
	long   volatile p9000_dr_tri_rel_xy;              /* 0x000s12b8 wo */
	long   : 32;                            /* 0x000s12bc    */
	long   : 32;                            /* 0x000s12c0    */
	long   : 32;                            /* 0x000s12c4    */
	long   volatile p9000_dr_quad_x;                  /* 0x000s12c8 wo */
	long   : 32;                            /* 0x000s12cc    */
	long   volatile p9000_dr_quad_y;                  /* 0x000s12d0 wo */
	long   : 32;                            /* 0x000s12d4    */
	long   volatile p9000_dr_quad_xy;                 /* 0x000s12d8 wo */
	long   : 32;                            /* 0x000s12dc    */
	long   : 32;                            /* 0x000s12e0    */
	long   : 32;                            /* 0x000s12e4    */
	long   volatile p9000_dr_quad_rel_x;              /* 0x000s12e8 wo */
	long   : 32;                            /* 0x000s12ec    */
	long   volatile p9000_dr_quad_rel_y;              /* 0x000s12f0 wo */
	long   : 32;                            /* 0x000s12f4    */
	long   volatile p9000_dr_quad_rel_xy;             /* 0x000s12f8 wo */
	long   : 32;                            /* 0x000s12fc    */
	long   : 32;                            /* 0x000s1300    */
	long   : 32;                            /* 0x000s1304    */
	long   volatile p9000_dr_rect_x;                  /* 0x000s1308 wo */
	long   : 32;                            /* 0x000s130c    */
	long   volatile p9000_dr_rect_y;                  /* 0x000s1310 wo */
	long   : 32;                            /* 0x000s1314    */
	long   volatile p9000_dr_rect_xy;                 /* 0x000s1318 wo */
	long   : 32;                            /* 0x000s131c    */
	long   : 32;                            /* 0x000s1320    */
	long   : 32;                            /* 0x000s1324    */
	long   volatile p9000_dr_rect_rel_x;              /* 0x000s1328 wo */
	long   : 32;                            /* 0x000s132c    */
	long   volatile p9000_dr_rect_rel_y;              /* 0x000s1330 wo */
	long   : 32;                            /* 0x000s1334    */
	long   volatile p9000_dr_rect_rel_xy;             /* 0x000s1338 wo */
	long   : 32;                            /* 0x000s133c    */
	long   volatile p9000_dr_filler12[0xecc0/4];      /* 0x000s1340    */
} p9000_drawing_regs_t;                                   /* 0x00200000    */

typedef struct {
	p9000_control_regs_t volatile p9000_control_regs[8];
} control_t;

typedef struct {
	p9000_drawing_regs_t volatile p9000_drawing_regs[8];
} drawing_t;

#ifdef _KERNEL
typedef struct p9000 {
    control_t *p9000_control_regp;
    drawing_t *p9000_drawing_regp;
    uchar_t *p9000_frame_bufferp;
    uchar_t *p9000_bt485_basep;
    uchar_t *p9000_seq_basep;
} p9000_t;
#else
typedef volatile struct p9000   {
    u_long volatile p9000_nonpower9000[0x100000/4];      /* 0x00000000 rw */
    p9000_control_regs_t volatile p9000_control_regs[8]; /* 0x001z0000    */
    p9000_drawing_regs_t volatile p9000_drawing_regs[8]; /* 0x001s0000    */
    uchar_t volatile p9000_frame_buffer[0x200000];       /* 0x00200000 rw */
    }   p9000_t;                                         /* 0x00400000    */
#endif

typedef p9000_t volatile *p9000p_t;

	/* Address Endian Bits */

#define P9000_ADDR_SWAP_HALF_WORDS      4                /* 0x00040000 */
#define P9000_ADDR_SWAP_BYTES           2                /* 0x00020000 */
#define P9000_ADDR_SWAP_BITS            1                /* 0x00010000 */

#ifndef P9000_LITTLE_ENDIAN
#if defined (_LITTLE_ENDIAN) && !defined (_BIG_ENDIAN)
#define P9000_LITTLE_ENDIAN 1
#elif defined (_BIT_ENDIAN) && !defined (_LITTLE_ENDIAN)
#define P9000_LITTLE_ENDIAN 0
#endif
#endif

#if P9000_LITTLE_ENDIAN

#ifndef P9000_ENDIAN_PIXEL1
#define P9000_ENDIAN_PIXEL1  \
       (P9000_ADDR_SWAP_HALF_WORDS | \
       P9000_ADDR_SWAP_BYTES | \
       P9000_ADDR_SWAP_BITS)                           /* 0x00070000 */
#endif

#ifndef P9000_ENDIAN_PIXEL8
#define P9000_ENDIAN_PIXEL8  \
       (P9000_ADDR_SWAP_HALF_WORDS | \
       P9000_ADDR_SWAP_BYTES)                          /* 0x00060000 */
#endif

#ifndef P9000_ENDIAN_PATTERN
#define P9000_ENDIAN_PATTERN \
       (P9000_ADDR_SWAP_HALF_WORDS | \
       P9000_ADDR_SWAP_BYTES | \
       P9000_ADDR_SWAP_BITS)                           /* 0x00070000 */
#endif

#else   /* !P9000_LITTLE_ENDIAN */

#ifndef P9000_ENDIAN_PIXEL1
#define P9000_ENDIAN_PIXEL1     0
#endif

#ifndef P9000_ENDIAN_PIXEL8
#define P9000_ENDIAN_PIXEL8     0
#endif

#ifndef P9000_ENDIAN_PATTERN
#define P9000_ENDIAN_PATTERN    0
#endif

#endif

#ifdef _KERNEL
#ifndef P9000CR
#define P9000CR p9000_control_regp->p9000_control_regs[0]
#endif

#ifndef P9000DR
#define P9000DR p9000_drawing_regp->p9000_drawing_regs[0]
#endif

#ifndef P9000DRP1
#define P9000DRP1 p9000_drawing_regp->p9000_drawing_regs[P9000_ENDIAN_PIXEL1]
#endif

#ifndef P9000DRP8
#define P9000DRP8 p9000_drawing_regp->p9000_drawing_regs[P9000_ENDIAN_PIXEL8]
#endif

#ifndef P9000DRPT
#define P9000DRPT p9000_drawing_regp->p9000_drawing_regs[P9000_ENDIAN_PATTERN]
#endif
#else
#ifndef P9000CR
#define P9000CR p9000_control_regs[0]
#endif

#ifndef P9000DR
#define P9000DR p9000_drawing_regs[0]
#endif

#ifndef P9000DRP1
#define P9000DRP1 p9000_drawing_regs[P9000_ENDIAN_PIXEL1]
#endif

#ifndef P9000DRP8
#define P9000DRP8 p9000_drawing_regs[P9000_ENDIAN_PIXEL8]
#endif

#ifndef P9000DRPT
#define P9000DRPT p9000_drawing_regs[P9000_ENDIAN_PATTERN]
#endif
#endif

#define p9000_bground      P9000DR.p9000_dr_bground      /* 0x000s0204 rw */
#define p9000_blit         P9000DR.p9000_dr_blit         /* 0x000s0004 ro */
#define p9000_cindex       P9000DR.p9000_dr_cindex       /* 0x000s018c rw */
#define p9000_draw_mode    P9000DR.p9000_dr_draw_mode    /* 0x000s020c rw */
#define p9000_fground      P9000DR.p9000_dr_fground      /* 0x000s0200 rw */
#define p9000_hrzbf        P9000CR.p9000_cr_hrzbf        /* 0x000z0114 rw */
#define p9000_hrzbr        P9000CR.p9000_cr_hrzbr        /* 0x000z0110 rw */
#define p9000_hrzc         P9000CR.p9000_cr_hrzc         /* 0x000z0104 ro */
#define p9000_hrzsr        P9000CR.p9000_cr_hrzsr        /* 0x000z010c rw */
#define p9000_hrzt         P9000CR.p9000_cr_hrzt         /* 0x000z0108 rw */
#define p9000_interrupt    P9000CR.p9000_cr_interrupt    /* 0x000z0008 rw */
#define p9000_interrupt_en P9000CR.p9000_cr_interrupt_en /* 0x000z000c rw */
#define p9000_line_rel_x   P9000DR.p9000_dr_line_rel_x   /* 0x000s1268 wo */
#define p9000_line_rel_xy  P9000DR.p9000_dr_line_rel_xy  /* 0x000s1278 wo */
#define p9000_line_rel_y   P9000DR.p9000_dr_line_rel_y   /* 0x000s1270 wo */
#define p9000_line_x       P9000DR.p9000_dr_line_x       /* 0x000s1248 wo */
#define p9000_line_xy      P9000DR.p9000_dr_line_xy      /* 0x000s1258 wo */
#define p9000_line_y       P9000DR.p9000_dr_line_y       /* 0x000s1250 wo */
#define p9000_mem_config   P9000CR.p9000_cr_mem_config   /* 0x000z0184 rw */
#define p9000_next_pixels  P9000DR.p9000_dr_next_pixels  /* 0x000s0014 wo */
#define p9000_oor          P9000DR.p9000_dr_oor          /* 0x000s0184 ro */
#define p9000_pat_originx  P9000DR.p9000_dr_pat_originx  /* 0x000s0210 rw */
#define p9000_pat_originy  P9000DR.p9000_dr_pat_originy  /* 0x000s0214 rw */
#define p9000_pattern      P9000DRPT.p9000_dr_pattern    /* 0x000s0280 rw */
#define p9000_pe_w_max     P9000DR.p9000_dr_pe_w_max     /* 0x000s0198 ro */
#define p9000_pe_w_min     P9000DR.p9000_dr_pe_w_min     /* 0x000s0194 ro */
#define p9000_pixel1       P9000DRP1.p9000_dr_pixel1     /* 0x000s0080 wo */
#define p9000_pixel8       P9000DRP8.p9000_dr_pixel8     /* 0x000s000c wo */
#define p9000_pixel8_reg   P9000DRP8.p9000_dr_pixel8_reg /* 0x000s021c rw */
#define p9000_pmask        P9000DR.p9000_dr_pmask        /* 0x000s0208 rw */
#define p9000_point_rel_x  P9000DR.p9000_dr_point_rel_x  /* 0x000s1228 wo */
#define p9000_point_rel_xy P9000DR.p9000_dr_point_rel_xy /* 0x000s1238 wo */
#define p9000_point_rel_y  P9000DR.p9000_dr_point_rel_y  /* 0x000s1230 wo */
#define p9000_point_x      P9000DR.p9000_dr_point_x      /* 0x000s1208 wo */
#define p9000_point_xy     P9000DR.p9000_dr_point_xy     /* 0x000s1218 wo */
#define p9000_point_y      P9000DR.p9000_dr_point_y      /* 0x000s1210 wo */
#define p9000_prehrzc      P9000CR.p9000_cr_prehrzc      /* 0x000z0118 rw */
#define p9000_prevrtc      P9000CR.p9000_cr_prevrtc      /* 0x000z0130 rw */
#define p9000_qsfcounter   P9000CR.p9000_cr_qsfcounter   /* 0x000z013c ro */
#define p9000_quad         P9000DR.p9000_dr_quad         /* 0x000s0008 ro */
#define p9000_quad_rel_x   P9000DR.p9000_dr_quad_rel_x   /* 0x000s12e8 wo */
#define p9000_quad_rel_xy  P9000DR.p9000_dr_quad_rel_xy  /* 0x000s12f8 wo */
#define p9000_quad_rel_y   P9000DR.p9000_dr_quad_rel_y   /* 0x000s12f0 wo */
#define p9000_quad_x       P9000DR.p9000_dr_quad_x       /* 0x000s12c8 wo */
#define p9000_quad_xy      P9000DR.p9000_dr_quad_xy      /* 0x000s12d8 wo */
#define p9000_quad_y       P9000DR.p9000_dr_quad_y       /* 0x000s12d0 wo */
#define p9000_ramdac       P9000CR.p9000_cr_ramdac       /* 0x000z0200 rw */
#define p9000_raster       P9000DR.p9000_dr_raster       /* 0x000s0218 rw */
#define p9000_rect_rel_x   P9000DR.p9000_dr_rect_rel_x   /* 0x000s1328 wo */
#define p9000_rect_rel_xy  P9000DR.p9000_dr_rect_rel_xy  /* 0x000s1338 wo */
#define p9000_rect_rel_y   P9000DR.p9000_dr_rect_rel_y   /* 0x000s1330 wo */
#define p9000_rect_x       P9000DR.p9000_dr_rect_x       /* 0x000s1308 wo */
#define p9000_rect_xy      P9000DR.p9000_dr_rect_xy      /* 0x000s1318 wo */
#define p9000_rect_y       P9000DR.p9000_dr_rect_y       /* 0x000s1310 wo */
#define p9000_rel_x0       P9000DR.p9000_dr_rel_x0       /* 0x000s1028 wo */
#define p9000_rel_x1       P9000DR.p9000_dr_rel_x1       /* 0x000s1068 wo */
#define p9000_rel_x2       P9000DR.p9000_dr_rel_x2       /* 0x000s10a8 wo */
#define p9000_rel_x3       P9000DR.p9000_dr_rel_x3       /* 0x000s10e8 wo */
#define p9000_rel_xy0      P9000DR.p9000_dr_rel_xy0      /* 0x000s1038 wo */
#define p9000_rel_xy1      P9000DR.p9000_dr_rel_xy1      /* 0x000s1078 wo */
#define p9000_rel_xy2      P9000DR.p9000_dr_rel_xy2      /* 0x000s10b8 wo */
#define p9000_rel_xy3      P9000DR.p9000_dr_rel_xy3      /* 0x000s10f8 wo */
#define p9000_rel_y0       P9000DR.p9000_dr_rel_y0       /* 0x000s1030 wo */
#define p9000_rel_y1       P9000DR.p9000_dr_rel_y1       /* 0x000s1070 wo */
#define p9000_rel_y2       P9000DR.p9000_dr_rel_y2       /* 0x000s10b0 wo */
#define p9000_rel_y3       P9000DR.p9000_dr_rel_y3       /* 0x000s10f0 wo */
#define p9000_rfcount      P9000CR.p9000_cr_rfcount      /* 0x000z018c ro */
#define p9000_rfperiod     P9000CR.p9000_cr_rfperiod     /* 0x000z0188 rw */
#define p9000_rlcur        P9000CR.p9000_cr_rlcur        /* 0x000z0194 ro */
#define p9000_rlmax        P9000CR.p9000_cr_rlmax        /* 0x000z0190 rw */
#define p9000_sraddr       P9000CR.p9000_cr_sraddr       /* 0x000z0134 ro */
#define p9000_srtctl       P9000CR.p9000_cr_srtctl       /* 0x000z0138 rw */
#define p9000_status       P9000DR.p9000_dr_status       /* 0x000s0000 ro */
#define p9000_sysconfig    P9000CR.p9000_cr_sysconfig    /* 0x000z0004 rw */
#define p9000_tri_rel_x    P9000DR.p9000_dr_tri_rel_x    /* 0x000s12a8 wo */
#define p9000_tri_rel_xy   P9000DR.p9000_dr_tri_rel_xy   /* 0x000s12b8 wo */
#define p9000_tri_rel_y    P9000DR.p9000_dr_tri_rel_y    /* 0x000s12b0 wo */
#define p9000_tri_x        P9000DR.p9000_dr_tri_x        /* 0x000s1288 wo */
#define p9000_tri_xy       P9000DR.p9000_dr_tri_xy       /* 0x000s1298 wo */
#define p9000_tri_y        P9000DR.p9000_dr_tri_y        /* 0x000s1290 wo */
#define p9000_vrtbf        P9000CR.p9000_cr_vrtbf        /* 0x000z012c rw */
#define p9000_vrtbr        P9000CR.p9000_cr_vrtbr        /* 0x000z0128 rw */
#define p9000_vrtc         P9000CR.p9000_cr_vrtc         /* 0x000z011c ro */
#define p9000_vrtsr        P9000CR.p9000_cr_vrtsr        /* 0x000z0124 rw */
#define p9000_vrtt         P9000CR.p9000_cr_vrtt         /* 0x000z0120 rw */
#define p9000_w_max        P9000DR.p9000_dr_w_max        /* 0x000s0224 rw */
#define p9000_w_min        P9000DR.p9000_dr_w_min        /* 0x000s0220 rw */
#define p9000_w_off_xy     P9000DR.p9000_dr_w_off_xy     /* 0x000s0190 rw */
#define p9000_x0           P9000DR.p9000_dr_x0           /* 0x000s1008 rw */
#define p9000_x1           P9000DR.p9000_dr_x1           /* 0x000s1048 rw */
#define p9000_x2           P9000DR.p9000_dr_x2           /* 0x000s1088 rw */
#define p9000_x3           P9000DR.p9000_dr_x3           /* 0x000s10c8 rw */
#define p9000_xclip        P9000DR.p9000_dr_xclip        /* 0x000s01a4 ro */
#define p9000_xedge_gt     P9000DR.p9000_dr_xedge_gt     /* 0x000s01ac ro */
#define p9000_xedge_lt     P9000DR.p9000_dr_xedge_lt     /* 0x000s01a8 ro */
#define p9000_xy0          P9000DR.p9000_dr_xy0          /* 0x000s1018 rw */
#define p9000_xy1          P9000DR.p9000_dr_xy1          /* 0x000s1058 rw */
#define p9000_xy2          P9000DR.p9000_dr_xy2          /* 0x000s1098 rw */
#define p9000_xy3          P9000DR.p9000_dr_xy3          /* 0x000s10d8 rw */
#define p9000_y0           P9000DR.p9000_dr_y0           /* 0x000s1010 rw */
#define p9000_y1           P9000DR.p9000_dr_y1           /* 0x000s1050 rw */
#define p9000_y2           P9000DR.p9000_dr_y2           /* 0x000s1090 rw */
#define p9000_y3           P9000DR.p9000_dr_y3           /* 0x000s10d0 rw */
#define p9000_yclip        P9000DR.p9000_dr_yclip        /* 0x000s01a0 ro */
#define p9000_yedge_gt     P9000DR.p9000_dr_yedge_gt     /* 0x000s01b4 ro */
#define p9000_yedge_lt     P9000DR.p9000_dr_yedge_lt     /* 0x000s01b0 ro */

#define p9000_ramdac0      P9000CR.p9000_cr_ramdac[0]    /* 0x000z0200 rw */
#define p9000_ramdac1      P9000CR.p9000_cr_ramdac[1]    /* 0x000z0204 rw */
#define p9000_ramdac2      P9000CR.p9000_cr_ramdac[2]    /* 0x000z0208 rw */
#define p9000_ramdac3      P9000CR.p9000_cr_ramdac[3]    /* 0x000z020c rw */
#define p9000_ramdac4      P9000CR.p9000_cr_ramdac[4]    /* 0x000z0210 rw */
#define p9000_ramdac5      P9000CR.p9000_cr_ramdac[5]    /* 0x000z0214 rw */
#define p9000_ramdac6      P9000CR.p9000_cr_ramdac[6]    /* 0x000z0218 rw */
#define p9000_ramdac7      P9000CR.p9000_cr_ramdac[7]    /* 0x000z021c rw */
#define p9000_ramdac8      P9000CR.p9000_cr_ramdac[8]    /* 0x000z0220 rw */
#define p9000_ramdac9      P9000CR.p9000_cr_ramdac[9]    /* 0x000z0224 rw */
#define p9000_ramdac10     P9000CR.p9000_cr_ramdac[10]   /* 0x000z0228 rw */
#define p9000_ramdac11     P9000CR.p9000_cr_ramdac[11]   /* 0x000z022c rw */
#define p9000_ramdac12     P9000CR.p9000_cr_ramdac[12]   /* 0x000z0230 rw */
#define p9000_ramdac13     P9000CR.p9000_cr_ramdac[13]   /* 0x000z0234 rw */
#define p9000_ramdac14     P9000CR.p9000_cr_ramdac[14]   /* 0x000z0238 rw */
#define p9000_ramdac15     P9000CR.p9000_cr_ramdac[15]   /* 0x000z023c rw */

        /* Coordinate Registers 0x000s1??? rw */

#define P9000_COORD_BITS                14
#define P9000_COORD_SIGN_BIT            (1 << (P9000_COORD_BITS - 1))
#define P9000_COORD_MIN                 (-P9000_COORD_SIGN_BIT)
#define P9000_COORD_MAX                 (P9000_COORD_SIGN_BIT - 1)
#define P9000_COORD_MASK                ((1 << P9000_COORD_BITS) - 1)
#define P9000_COORD_SIGN_EXTEND(x)      (((x) & P9000_COORD_SIGN_BIT) ? \
        ((x) | P9000_COORD_MIN) : ((x) & P9000_COORD_MAX))

        /* Draw Mode Register 0x000s020c rw */

#define P9000_DRAWMODE_PICK_CTRL        0x00000008
#define P9000_DRAWMODE_PICK             0x00000004
#define P9000_DRAWMODE_DEST_BUFFER_CTRL 0x00000002
#define P9000_DRAWMODE_DEST_BUFFER      0x00000001

        /* Interrupt Register 0x000z0008 rw */

#define P9000_INT_VBLANKED_CTRL         0x00000020
#define P9000_INT_VBLANKED              0x00000010
#define P9000_INT_PICKED_CTRL           0x00000008
#define P9000_INT_PICKED                0x00000004
#define P9000_INT_DE_IDLE_CTRL          0x00000002
#define P9000_INT_DE_IDLE               0x00000001

        /* Interrupt Enable Register 0x000z000c rw */

#define P9000_INTEN_MEN_CTRL            0x00000080
#define P9000_INTEN_MEN                 0x00000040
#define P9000_INTEN_VBLANKED_EN_CTRL    0x00000020
#define P9000_INTEN_VBLANKED_EN         0x00000010
#define P9000_INTEN_PICKED_EN_CTRL      0x00000008
#define P9000_INTEN_PICKED_EN           0x00000004
#define P9000_INTEN_DE_IDLE_EN_CTRL     0x00000002
#define P9000_INTEN_DE_IDLE_EN          0x00000001

        /* Memory Configuration Register 0x000z0184 rw */

#define P9000_MEMCFG_MEMORY_CONFIG_1    0x00000000  /* 1MB     (two cycle)  */
#define P9000_MEMCFG_MEMORY_CONFIG_2    0x00000001  /* 1MB     (one cycle)  */
#define P9000_MEMCFG_MEMORY_CONFIG_3    0x00000002  /* 2MB     (single buf) */
#define P9000_MEMCFG_MEMORY_CONFIG_4    0x00000003  /* 1MB * 2 (double buf) */
#define P9000_MEMCFG_MEMORY_CONFIG_5    0x00000004  /* 2MB * 2 (double buf) */
#define P9000_MEMCFG_MEMORY_CONFIG_MASK 0x00000007

        /* Out Of Range Register 0x000s0184 ro */

#define P9000_OOR_X3_OOR                0x00000080
#define P9000_OOR_X2_OOR                0x00000040
#define P9000_OOR_X1_OOR                0x00000020
#define P9000_OOR_X0_OOR                0x00000010
#define P9000_OOR_Y3_OOR                0x00000008
#define P9000_OOR_Y2_OOR                0x00000004
#define P9000_OOR_Y1_OOR                0x00000002
#define P9000_OOR_Y0_OOR                0x00000001

        /* Raster Register 0x000s0218 rw */

#define P9000_RASTER_USE_PATTERN        0x00020000
#define P9000_RASTER_QUAD_DRAW_MODE     0x00010000
#define P9000_RASTER_ROP_MINTERMS       0x0000ffff
#define P9000_RASTER_ROP_F_MASK         0x0000ff00
#define P9000_RASTER_ROP_B_MASK         0x0000f0f0
#define P9000_RASTER_ROP_S_MASK         0x0000cccc
#define P9000_RASTER_ROP_D_MASK         0x0000aaaa

        /* Screen Repaint Timing Control 0x000z0138 rw */

#define P9000_SRTCTL_INTERNAL_VSYNC     0x00000100
#define P9000_SRTCTL_INTERNAL_HSYNC     0x00000080
#define P9000_SRTCTL_COMPOSITE          0x00000040
#define P9000_SRTCTL_ENABLE_VIDEO       0x00000020
#define P9000_SRTCTL_HBLNK_RELOAD       0x00000010
#define P9000_SRTCTL_DISPLAY_BUFFER     0x00000008
#define P9000_SRTCTL_QSFSELECT          0x00000007

        /* Status Register 0x000s0000 ro */

#define P9000_STATUS_ISSUE_QB           0x80000000
#define P9000_STATUS_BUSY               0x40000000
#define P9000_STATUS_QUAD_OR_BUSY       (P9000_STATUS_ISSUE_QB | \
        P9000_STATUS_BUSY)
#define P9000_STATUS_PICKED             0x00000080
#define P9000_STATUS_PIXEL_SOFTWARE     0x00000040
#define P9000_STATUS_BLIT_SOFTWARE      0x00000020
#define P9000_STATUS_QUAD_SOFTWARE      0x00000010
#define P9000_STATUS_QUAD_CONCAVE       0x00000008
#define P9000_STATUS_QUAD_HIDDEN        0x00000004
#define P9000_STATUS_QUAD_VISIBLE       0x00000002
#define P9000_STATUS_QUAD_INTERSECTS    0x00000001

        /* System Configuration Register 0x000z0004 rw */

#define P9000_SYSCONF_DRIVELOAD2        0x01000000
#define P9000_SYSCONF_HRES1_MASK        0x00700000
#define P9000_SYSCONF_HRES1_SHIFT       20
#define P9000_SYSCONF_HRES2_MASK        0x000e0000
#define P9000_SYSCONF_HRES2_SHIFT       17
#define P9000_SYSCONF_HRES3_MASK        0x0001c000
#define P9000_SYSCONF_HRES3_SHIFT       14
#define P9000_SYSCONF_PIXEL_SWAP_HALF   0x00002000
#define P9000_SYSCONF_PIXEL_SWAP_BYTE   0x00001000
#define P9000_SYSCONF_PIXEL_SWAP_BITS   0x00000800
#define P9000_SYSCONF_PIXEL_BUF_READ    0x00000400
#define P9000_SYSCONF_PIXEL_BUF_WRITE   0x00000200
#define P9000_SYSCONF_VERSION_MASK      0x00000007
#define P9000_SYSCONF_HRES_NOADD        0x00000000
#define P9000_SYSCONF_HRES_ADD32        0x00000001
#define P9000_SYSCONF_HRES_ADD64        0x00000002
#define P9000_SYSCONF_HRES_ADD128       0x00000003
#define P9000_SYSCONF_HRES_ADD256       0x00000004
#define P9000_SYSCONF_HRES_ADD512       0x00000005
#define P9000_SYSCONF_HRES_ADD1024      0x00000006
#define P9000_SYSCONF_HRES_ADD2048      0x00000007

        /* Video Control Mask */

#define P9000_VIDCTRL_VALUE_MASK        0x00000fff

        /* Window Minimum and Maximum Mask */

#define P9000_WINDOW_MASK               0x00001fff
#define P9000_WINDOW_XY_MASK            0x1fff1fff

        /* Xclip Register 0x000s01a4 ro */

#define P9000_XCLIP_X3_LT_MIN           0x00000080
#define P9000_XCLIP_X2_LT_MIN           0x00000040
#define P9000_XCLIP_X1_LT_MIN           0x00000020
#define P9000_XCLIP_X0_LT_MIN           0x00000010
#define P9000_XCLIP_X3_GT_MAX           0x00000008
#define P9000_XCLIP_X2_GT_MAX           0x00000004
#define P9000_XCLIP_X1_GT_MAX           0x00000002
#define P9000_XCLIP_X0_GT_MAX           0x00000001

        /* Xedge_gt Register 0x000s01ac ro */

#define P9000_XEDGEGT_X0_LT_X2          0x00000020
#define P9000_XEDGEGT_X1_LT_X3          0x00000010
#define P9000_XEDGEGT_X3_LT_X0          0x00000008
#define P9000_XEDGEGT_X2_LT_X3          0x00000004
#define P9000_XEDGEGT_X1_LT_X2          0x00000002
#define P9000_XEDGEGT_X0_LT_X1          0x00000001

        /* Xedge_lt Register 0x000s01a8 ro */

#define P9000_XEDGELT_X0_GT_X2          0x00000020
#define P9000_XEDGELT_X1_GT_X3          0x00000010
#define P9000_XEDGELT_X3_GT_X0          0x00000008
#define P9000_XEDGELT_X2_GT_X3          0x00000004
#define P9000_XEDGELT_X1_GT_X2          0x00000002
#define P9000_XEDGELT_X0_GT_X1          0x00000001

        /* Yclip Register 0x000s01a0 ro */

#define P9000_YCLIP_Y3_LT_MIN           0x00000080
#define P9000_YCLIP_Y2_LT_MIN           0x00000040
#define P9000_YCLIP_Y1_LT_MIN           0x00000020
#define P9000_YCLIP_Y0_LT_MIN           0x00000010
#define P9000_YCLIP_Y3_GT_MAX           0x00000008
#define P9000_YCLIP_Y2_GT_MAX           0x00000004
#define P9000_YCLIP_Y1_GT_MAX           0x00000002
#define P9000_YCLIP_Y0_GT_MAX           0x00000001

        /* Yedge_gt Register 0x000s01b4 ro */

#define P9000_YEDGEGT_Y0_LT_Y2          0x00000020
#define P9000_YEDGEGT_Y1_LT_Y3          0x00000010
#define P9000_YEDGEGT_Y3_LT_Y0          0x00000008
#define P9000_YEDGEGT_Y2_LT_Y3          0x00000004
#define P9000_YEDGEGT_Y1_LT_Y2          0x00000002
#define P9000_YEDGEGT_Y0_LT_Y1          0x00000001

        /* Yedge_lt Register 0x000s01b0 ro */

#define P9000_YEDGELT_Y0_GT_Y2          0x00000020
#define P9000_YEDGELT_Y1_GT_Y3          0x00000010
#define P9000_YEDGELT_Y3_GT_Y0          0x00000008
#define P9000_YEDGELT_Y2_GT_Y3          0x00000004
#define P9000_YEDGELT_Y1_GT_Y2          0x00000002
#define P9000_YEDGELT_Y0_GT_Y1          0x00000001

#define P9000_PTR(p9000)    (P9000_NOOP (), (p9000))

#define P9000_XY(x, y)                \
        (((long) (x) << 16) | (long) (y) & 0xffff)

#define        P9000_PAT_COPY(d, s)        \
        (d)[0] = (s)[0],        \
        (d)[1] = (s)[1],        \
        (d)[2] = (s)[2],        \
        (d)[3] = (s)[3],        \
        (d)[4] = (s)[4],        \
        (d)[5] = (s)[5],        \
        (d)[6] = (s)[6],        \
        (d)[7] = (s)[7]

#define P9000_LINEBYTES(sysconfig)  \
        (((sysconfig) & P9000_SYSCONF_HRES3_MASK ?          \
        1 << ((((sysconfig) & P9000_SYSCONF_HRES3_MASK) >>  \
        P9000_SYSCONF_HRES3_SHIFT) + 4) : 0) +              \
        ((sysconfig) & P9000_SYSCONF_HRES2_MASK ?           \
        1 << ((((sysconfig) & P9000_SYSCONF_HRES2_MASK) >>  \
        P9000_SYSCONF_HRES2_SHIFT) + 4) : 0) +              \
        ((sysconfig) & P9000_SYSCONF_HRES1_MASK ?           \
        1 << ((((sysconfig) & P9000_SYSCONF_HRES1_MASK) >>  \
        P9000_SYSCONF_HRES1_SHIFT) + 4) : 0))

#define P9000_GET_ENABLE_VIDEO(p9000)   \
        (((p9000) -> p9000_srtctl & P9000_SRTCTL_ENABLE_VIDEO) != 0)

#define P9000_SET_VBLANK_INTR(p9000)    \
        (p9000) -> p9000_interrupt_en = P9000_INTEN_MEN_CTRL | \
        P9000_INTEN_MEN | P9000_INTEN_VBLANKED_EN_CTRL | \
        P9000_INTEN_VBLANKED_EN

#define P9000_STOP_VBLANK_INTR(p9000)   \
        (p9000) -> p9000_interrupt_en = P9000_INTEN_VBLANKED_EN_CTRL

#define P9000_CLEAR_VBLANK_INTR(p9000)  \
        (p9000) -> p9000_interrupt = P9000_INT_VBLANKED_CTRL

#define P9000_NOT_INLINE 1
#if P9000_NOT_INLINE

#define P9000_NOOP(p9000)   (p9000)

#define        P9000_WAIT_BUSY(p9000)        \
        while ((p9000) -> p9000_status & P9000_STATUS_BUSY)

#define        P9000_DRAW_QUAD(p9000)        \
        while ((p9000) -> p9000_quad & P9000_STATUS_ISSUE_QB)

#define        P9000_DRAW_BLIT(p9000)        \
        while ((p9000) -> p9000_blit & P9000_STATUS_ISSUE_QB)

#define P9000_WAIT_VSYNC(p9000) \
        while ((p9000) -> p9000_vrtsr & P9000_VIDCTRL_VALUE_MASK) != \
        (p9000) -> p9000_vrtc & P9000_VIDCTRL_VALUE_MASK))

#define P9000_SET_ENABLE_VIDEO(p9000) \
        (p9000) -> p9000_srtctl |= P9000_SRTCTL_ENABLE_VIDEO

#define P9000_CLEAR_ENABLE_VIDEO(p9000) \
        (p9000) -> p9000_srtctl &= ~P9000_SRTCTL_ENABLE_VIDEO

#define P9000_SET_BUFFER_0(p9000)  \
        p9000regs -> p9000_sysconfig &= ~(P9000_SYSCONF_PIXEL_BUF_READ | \
          P9000_SYSCONF_PIXEL_BUF_WRITE)

#define P9000_SET_BUFFER_1(p9000)  \
        p9000regs -> p9000_sysconfig |= P9000_SYSCONF_PIXEL_BUF_READ | \
          P9000_SYSCONF_PIXEL_BUF_WRITE

#else /* P9000_NOT_INLINE */

extern  void P9000_NOOP (void);
extern  void P9000_WAIT_BUSY (p9000p_t);
extern  void P9000_DRAW_QUAD (p9000p_t);
extern  void P9000_DRAW_BLIT (p9000p_t);
extern  void P9000_WAIT_VSYNC (p9000p_t);
extern  void P9000_SET_ENABLE_VIDEO (p9000p_t);
extern  void P9000_CLEAR_ENABLE_VIDEO (p9000p_t);
extern  void P9000_SET_BUFFER_0 (p9000p_t);
extern  void P9000_SET_BUFFER_1 (p9000p_t);
extern  long P9000_WAIT_SUBR (p9000p_t);
extern  long P9000_QUAD_SUBR (p9000p_t);
extern  long P9000_BLIT_SUBR (p9000p_t);

#endif /* P9000_NOT_INLINE */

        /* These addresses hang the cpu.  I suspect its  a  hardware */
        /* bug of the Weitek p9000.                                  */

#define P9000_BAD_ADDRESSES(addr) \
        (((addr) & 0xfff81ffc) == \
        (long) &((p9000p_t)0)->p9000_control_regs[0].p9000_cr_filler2 || \
        ((addr) & 0xfff81fc0) == \
        (long) &((p9000p_t)0)->p9000_control_regs[0].p9000_cr_filler3)

/*
 * P9000 Physical address map exported by the hardware.
 */
#define P9000_CONTROL_BASE              0x00100000
#define P9000_CONTROL_SIZE              0x00080000
#define P9000_DRAWING_BASE              0x00180000
#define P9000_DRAWING_SIZE              0x00080000
#define P9000_FRAME_BUFFER_BASE         0x00200000
#define P9000_FRAME_BUFFER_SIZE         0x00200000

/*
 * Logical address map exported by the driver (made to work with the stock
 * DDX libraries.
 */
#define P9000_VBASE                     0x00000000
#define P9000_REGISTER_BASE             0x00100000
#define P9000_REGISTER_SIZE             0x00100000
#define P9000_CONTROL_BASE              0x00100000
#define P9000_CONTROL_SIZE              0x00080000
#define P9000_DRAWING_BASE              0x00180000
#define P9000_DRAWING_SIZE              0x00080000
#define P9000_FRAME_BUFFER_BASE         0x00200000
#define P9000_FRAME_BUFFER_SIZE         0x00200000
#define P9000_SIZE                      0x00400000

#define P9000_VRT_VADDR                 0x10000000
#define P9000_VRT_SIZE                  4096

#define P9000_NONE                      0xFFFFFFFF

#ifdef        __cplusplus
}
#endif

#endif        /* !_P9000REG_H */
