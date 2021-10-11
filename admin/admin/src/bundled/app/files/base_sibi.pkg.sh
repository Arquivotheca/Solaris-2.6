/*
 *
 *	@(#)base_sibi.pkg.sh 1.2 96/01/24
 *
 * Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 *
 *
 *
 * root_pkgs - list of packages that go into install "root"
 * for 5.X cdrom install (really "bootarch" dependent pkgs).
 * With KBI, these may be of type "root" or "usr" as long as the 
 * pkginfo BASEDIR is correct.
 *
 * take care to list this in dependant order, see pkgs/xxx/install/depend files
 *
 */

#ifdef sparc
SUNWcar.c	/* Sun4c Root */
SUNWkvm.c	/* Sun4c Kvm */
#endif

#ifdef sparc
SUNWcar.m	/* Sun4m Root */
SUNWkvm.m	/* Sun4m Kvm */
#endif

#ifdef sparc
SUNWcar.d	/* Sun4d Root */
SUNWkvm.d	/* Sun4d Kvm */
#endif

#ifdef sparc
SUNWcar.u	/* Sun4u Root */
SUNWkvm.u	/* Sun4u Kvm */
#endif

#ifdef i386
SUNWcar.i	/* x86 Root */
SUNWkvm.i	/* x86 Kvm */
#endif  /* i386 */

#ifdef __ppc
SUNWcar.p	/* ppc Root */
SUNWkvm.p	/* ppc Kvm */
#endif

SUNWcsr		/* Core Sparc Root */
SUNWcsd		/* Core Sparc Devices */

SUNWnisr	/* Network Information System, (Root) */
#ifdef INCLUDE_RM_PKGS
SUNWscpr	/* Source Compatibility, (Root) */
#endif

#ifdef sparc
SUNWcg6.c	/* cg6 drivers */
SUNWdfb.c	/* Dumb Frame Buffer Drivers */
#endif

#ifdef sparc
SUNWssadv	/* SPARCstorage Array Drivers */
SUNWssaop	/* SPARCstorage Array Utility */
SUNWcg6.m	/* cg6 drivers */
SUNWdfb.m	/* Dumb Frame Buffer Drivers */
SUNWsxr.m	/* Dumb Frame Buffer Drivers */
SUNWtcx.m       /* TCX Frame Buffer Drivers */
#endif

#ifdef sparc
SUNWcg6.d	/* cg6 drivers */
SUNWdfb.d	/* Dumb Frame Buffer Drivers */
#endif

#ifdef sparc
SUNWcg6.u	/* cg6 drivers */
SUNWdfb.u	/* Dumb Frame Buffer Drivers */
#endif

#ifdef sparc
SUNWleor	/* Leo drivers; see BugId's 1193971 & 1201363 */
SUNWleo.d	/* Leo drivers; see BugId's 1193971 & 1201363 */
SUNWleo.m	/* Leo drivers; see BugId's 1193971 & 1201363 */
#endif

#ifdef __ppc
SUNWdfb.p       /* Dumb Frame Buffer Drivers */
#endif  /* __ppc */

#ifdef i386
SUNWdfb.i       /* Dumb Frame Buffer Drivers */
#endif  /* i386 */

SUNWadmr

SUNWxwdv	/* X Windows Kernel Drivers */
SUNWxwmod	/* OpenWindows kernel modules */

#ifdef i386
SUNWos86r       /* i386 root install package */
SUNWcdx86       /* i386 CDROM package */
SUNWpsdcr       /* common drivers, usable on any PC bus */
SUNWpsder       /* drivers for EISA devices */
SUNWpsdir       /* ISA bus PC device drivers */
SUNWpsdmr       /* MicroChannel device drivers */
#endif  /* i386 */

#ifdef __ppc
SUNWos86r       /* PPC root install package */
SUNWcdx86       /* PPC CDROM package */
SUNWpsdcr       /* common drivers, usable on any PC bus */
SUNWpsdir       /* ISA bus PC device drivers */
#endif  /* __ppc */

#ifdef sparc
AXILvplr.c	/* Axil platform links */
AXILvplr.m	/* Axil platform links */
AXILvplu.c	/* Axil usr/platform links */
AXILvplu.m	/* Axil usr/platform links */
PFUvplr.m	/* Fujitsu platform links */
PFUvplu.m	/* Fujitsu usr/platform links */
#endif

#ifdef sparc
SUNWhmd		/* HappyMeal ASIC Drivers */
SUNWffb.u	/* System Software support for the FFB graphics hardware  */
SUNWpd	        /* Drivers for SPARC platforms with the PCI bus  */
#endif /* sun4_boots and electron */

#ifdef sparc
TSBWvplu.m	/* Toshiba usr/platform links */
TSBWvplr.m	/* Toshiba platform links */
#endif /* sun4_boots */

SUNWcsu		/* Core Sparc Usr */
#ifdef INCLUDE_RM_PKGS
SUNWarc		/* Archive Libraries */
#endif

#ifdef INCLUDE_RM_PKGS
#ifdef sparc
SUNWbcp		/* Binary Compatability */
SUNWscbcp	/* Binary Compatability */
#endif	/* sparc */
#endif

SUNWesu		/* Extended System Utilities */
SUNWipc		/* Interprocess Communications */
#ifdef INCLUDE_RM_PKGS
SUNWfac		/* FACE */
#endif
SUNWloc		/* System Localization */
SUNWploc	/* European Partial Locales */
SUNWkey		/* Keyboard configuration tables */
SUNWnisu	/* Network Information System, (Usr) */
#ifdef INCLUDE_RM_PKGS
SUNWdoc		/* Doctools */
SUNWscpu	/* Source Compatibility, (Usr) */
#endif
SUNWter		/* Terminal Info */
SUNWtoo		/* Tools */
SUNWmfrun	/* MOTIF runtime files */

#ifdef INCLUDE_RM_PKGS
SUNWadmfw
#endif
SUNWadmap
SUNWadmc

#ifdef i386
SUNWpmi		/* X support */
SUNWxwpls	/* X support */
#endif	/* i386 */

#ifdef __ppc
SUNWxwpls	/* X support */
#endif /* __ppc */

SUNWxwfnt	/* X Windows font software */
SUNWxwice	/* X Windows software */
SUNWxwplt	/* X Windows platform software */
SUNWolrte	/* OpenLook toolkits runtime environment */
SUNWoldst	/* OpenLook deskset tools */
SUNWoldte	/* OpenLook desktop environment */
SUNWxwopt	/* nonessential MIT core clients and server extensions */

SUNWtltk	/* Tool Talk */

#ifdef sparc
SUNWsxow
SUNWsx
SUNWtcxow       /* TCX DDX Support */
SUNWlibC
SUNWleow        /* Leo drivers; see BugId's 1193971 & 1201363 */
#endif	/* sparc */

SUNWlibCf

SUNWlibms
SUNWenise

#ifdef INCLUDE_RM_PKGS
SUNWrdm		/* OI&LBN */
#endif

#if defined(i386) || defined(__ppc)
SUNWos86u       /* i386 or ppc usr install package */
#endif  /* defined i386 or __ppc */

SUNWinst	/* The install software */

SUNWadmgn	/* get netmask package */

SUNWdthj	/* HotJava browser package which includes JAVA VM */

#ifdef sparc
SUNWffbw	/* DDX software support for the FFB graphics hardware */
#endif /* sparc */

SUNWsibi

#ifdef sparc
SHWPcdrom	/* cd-rom installation add-on for SHWP */
#endif /* sparc */

