/*
 * Copyright (c) 1991-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident  "@(#)modstubs.s	1.33	96/10/23 SMI"

#include <sys/asm_linkage.h>

#if defined(lint)

char stubs_base[1], stubs_end[1];

#else	/* lint */

/*
 * !!!!!!!! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! !!!!!!!!
 *
 *	For functions which are either STUBs or WSTUBs the actual function
 *	need to be called using 'blrl' instruction because of preamble and
 *	postamble (i.e mod_hold_stub and mod_release_stub) around the
 *	function call.  Due to this we need to copy arguments for the
 *	real function.  On PowerPC we can't tell how many arguments are there
 *	on the stack so we have to save everything during the call to
 *	mod_hold_stub, and then restore them before calling the real
 *	function.  Also, arguments beyond the first 8 (r3-r10) need to
 *	be copied from the original stack frame to the new one set up for
 *	the installed function.  Additionally we copy 2 words from the
 *	argument overflow area, thus supporting 10 argument words as is
 *	done on the x86.  If more than 10 argument words needs to be
 *	supported, this code must change.  There is room in the newly
 *	built stack frame for 6 more argument words.
 *
 *	NOTE: Use NO_UNLOAD_STUBs if the module is NOT unloadable once it is
 *	      loaded.
 */

/*
 * WARNING: there is no check for forgetting to write END_MODULE,
 * and if you do, the kernel will most likely crash.  Be careful
 *
 * This file assumes that all of the contributions to the data segment
 * will be contiguous in the output file, even though they are separated
 * by pieces of text.  This is safe for all assemblers I know of now...
 */

/*
 * This file uses ansi preprocessor features:
 *
 * 1. 	#define mac(a) extra_ ## a     -->   mac(x) expands to extra_a
 * The old version of this is
 *      #define mac(a) extra_/.*.*./a
 * but this fails if the argument has spaces "mac ( x )"
 * (Ignore the dots above, I had to put them in to keep this a comment.)
 *
 * 2.   #define mac(a) #a             -->    mac(x) expands to "x"
 * The old version is
 *      #define mac(a) "a"
 *
 * For some reason, the 5.0 preprocessor isn't happy with the above usage.
 * For now, we're not using these ansi features.
 *
 * The reason is that "the 5.0 ANSI preprocessor" is built into the compiler
 * and is a tokenizing preprocessor. This means, when confronted by something
 * other than C token generation rules, strange things occur. In this case,
 * when confronted by an assembly file, it would turn the token ".globl" into
 * two tokens "." and "globl". For this reason, the traditional, non-ANSI
 * prepocessor is used on assembly files.
 *
 * It would be desirable to have a non-tokenizing cpp (accp?) to use for this.
 */

/*
 * This file contains the stubs routines for modules which can be autoloaded.
 * stub_install_common is a small assembly routine that calls
 * the c function install_stub.
 */
#define MODULE(module,namespace) \
	.data; \
module/**/_modname: \
	.string	"namespace/module" ; \
	.align	2; \
	.globl	module/**/_modinfo ; \
module/**/_modinfo: ; \
	.long module/**/_modname; \
	.long 0		/* storage for modctl pointer */

	/* then stub_info structures follow until a mods_func_adr is 0 */

/* this puts a 0 where the next mods_func_adr would be */
#define END_MODULE(module) .long 0

#define STUB(module, fcnname, retfcn) STUB_COMMON(module, fcnname, \
						mod_hold_stub, retfcn, 0)

/* "weak stub", don't load on account of this call */
#define WSTUB(module, fcnname, retfcn) STUB_COMMON(module, fcnname, \
						retfcn, retfcn, 1)

/* "weak stub" for non-unloadable module, don't load on account of this call */
#define NO_UNLOAD_WSTUB(module, fcnname, retfcn) STUB_UNLOADABLE(module, \
				fcnname, retfcn, retfcn, 1)

/*
 * "unloadable stub", don't bother 'holding' module if it's already loaded
 * since the module cannot be unloaded.
 *
 * User *MUST* guarentee the module is not unloadable (no _fini routine).
 */
#define NO_UNLOAD_STUB(module, fcnname, retfcn) STUB_UNLOADABLE(module, \
						fcnname,  retfcn, retfcn, 0)

/*
 * The data section in the stub_common macro is the
 * mod_stub_info structure for the stub function
 */

#define STUB_COMMON(module, fcnname, install_fcn, retfcn, weak) \
	.text; \
	ENTRY(fcnname); \
	lis	%r11,fcnname/**/_info@ha; \
	lwzu	%r0,fcnname/**/_info@l(%r11); \
	lwz	%r12,0x10(%r11); \
	cmpi	%r12,0;			/* weak?? */ \
	beq	stubs_common_code;	/* not weak */ \
	lwz	%r12,0xc(%r11); \
	cmp	%r12,%r0;		/* installed? */ \
	bne	stubs_common_code;	/* yes, so do the mod_hold thing */ \
	mtctr	%r0; \
	bctr	;			/* no, just jump to retfcn */ \
	.data; \
	.align	 2; \
fcnname/**/_info: \
	.long	install_fcn; \
	.long	module/**/_modinfo; \
	.long	fcnname; \
	.long	retfcn; \
	.long   weak

#define STUB_UNLOADABLE(module, fcnname, install_fcn, retfcn, weak)\
	.text; \
	ENTRY(fcnname); \
	lis	%r11,fcnname/**/_info@ha; \
	lwzu	%r0,fcnname/**/_info@l(%r11); \
	lwz	%r12,0xc(%r11); \
	cmp	%r12,%r0;		/* installed? */ \
	bne	stubs_bctr;		/* installed! */ \
	lwz	%r12,0x10(%r11); \
	cmpi	%r12,0;			/* weak?? */ \
	beq	stubs_common_code;	/* no, so do the mod_hold thing */ \
	mtctr	%r0; \
	bctr	;			/* yes, just jump to retfcn */ \
	.data; \
	.align	2; \
fcnname/**/_info: \
	.long	install_fcn; \
	.long	module/**/_modinfo; \
	.long	fcnname; \
	.long	retfcn; \
	.long   weak

/*
 * We branch here with the fcnname_info pointer in r11
 */
	.text
	.globl	mod_hold_stub
	.globl	mod_release_stub
stubs_common_code:
	mflr	%r0
	stwu	%r1,-0x30(%r1)
	stw	%r11,0x8(%r1)		! save the info pointer
	stw	%r0,0x34(%r1)		! save the return address
	stw	%r3,0x10(%r1)		! save arguments ...
	stw	%r4,0x14(%r1)
	stw	%r5,0x18(%r1)
	stw	%r6,0x1c(%r1)
	stw	%r7,0x20(%r1)
	stw	%r8,0x24(%r1)
	stw	%r9,0x28(%r1)
	stw	%r10,0x2c(%r1)
	mr	%r3,%r11		! arg 1
	bl	mod_hold_stub		! mod_hold_stub(mod_stub_info *)
	cmpi	%r3,-1			! error?
	beq	.error
	stw	%r3,0xc(%r1)		! using 'reserved' stack frame loc.
					! to save return value
	! restore and copy arguments
	lwz	%r4,0x14(%r1)		! arg 2
	lwz	%r3,0x10(%r1)		! arg 1
	lwz	%r12,0x40(%r1)		! fetch arg 9
	lwz	%r5,0x18(%r1)		! arg 3
	stw	%r12,0x10(%r1)		! store arg 9
	lwz	%r6,0x1c(%r1)		! arg 4
	lwz	%r12,0x44(%r1)		! fetch arg 10
	lwz	%r7,0x20(%r1)		! arg 5
	stw	%r12,0x14(%r1)		! store arg 10
	lwz	%r11,0x8(%r1)		! fetch modinfo
	lwz	%r8,0x24(%r1)		! arg 6
	lwz	%r0,0(%r11)		! fetch install_fcn
	lwz	%r9,0x28(%r1)		! arg 7
	mtlr	%r0
	lwz	%r10,0x2c(%r1)		! arg 8
	blrl
	stw	%r3,0x10(%r1)		! save return value(s)
	stw	%r4,0x14(%r1)
	lwz	%r3,0x8(%r1)		! arg 1
	lwz	%r4,0xc(%r1)		! arg 2
	bl	mod_release_stub
	lwz	%r0,0x34(%r1)
	lwz	%r3,0x10(%r1)		! return value 1
	lwz	%r4,0x14(%r1)		! return value 2
.tail:
	mtlr	%r0
	addi	%r1,%r1,0x30
	blr

.error:
	lwz	%r11,0x8(%r1)		! This should never happen.
	lwz	%r0,0x34(%r1)
	lwz	%r3,0xc(%r11)
	b	.tail

stubs_bctr:
	mtctr	%r0
	bctr

! this is just a marker for the area of text that contains stubs 
	.text
	.globl stubs_base
stubs_base:
	nop

/*
 * WARNING WARNING WARNING!!!!!!
 * 
 * On the MODULE macro you MUST NOT use any spaces!!! They are
 * significant to the preprocessor.  With ansi c there is a way around this
 * but for some reason (yet to be investigated) ansi didn't work for other
 * reasons!  
 *
 * When zero is used as the return function, the system will call
 * panic if the stub can't be resolved.
 */

/*
 * Stubs for specfs
 */

#ifndef SPEC_MODULE
	MODULE(specfs,fs);
	NO_UNLOAD_STUB(specfs, common_specvp,  	nomod_zero);
	NO_UNLOAD_STUB(specfs, makectty,		nomod_zero);
	NO_UNLOAD_STUB(specfs, makespecvp,      	nomod_zero);
	NO_UNLOAD_STUB(specfs, smark,           	nomod_zero);
	NO_UNLOAD_STUB(specfs, spec_segmap,     	nomod_einval);
	NO_UNLOAD_STUB(specfs, specfind,        	nomod_zero);
	NO_UNLOAD_STUB(specfs, specvp,          	nomod_zero);
	NO_UNLOAD_STUB(specfs, stillreferenced, 	nomod_zero);
	NO_UNLOAD_STUB(specfs, devi_stillreferenced, 	nomod_zero);
	NO_UNLOAD_STUB(specfs, spec_getvnodeops,	nomod_zero);
	END_MODULE(specfs);
#endif


/*
 * Stubs for sockfs. A non-unloadable module.
 */
#ifndef SOCK_MODULE
	MODULE(sockfs,fs);
	NO_UNLOAD_STUB(sockfs, so_socket,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, so_socketpair,	nomod_zero);
	NO_UNLOAD_STUB(sockfs, bind,  		nomod_zero);
	NO_UNLOAD_STUB(sockfs, listen,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, accept,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, connect,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, shutdown,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, recv,  		nomod_zero);
	NO_UNLOAD_STUB(sockfs, recvfrom,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, recvmsg,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, send,  		nomod_zero);
	NO_UNLOAD_STUB(sockfs, sendmsg,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, sendto,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, getpeername,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, getsockname,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, getsockopt,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, setsockopt,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, sockconfig,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, sock_getmsg,  	nomod_zero);
	NO_UNLOAD_STUB(sockfs, sock_putmsg,  	nomod_zero);
	END_MODULE(sockfs);
#endif


/*
 * Stubs for nfs common code.
 * XXX nfs_getvnodeops should go away with removal of kludge in vnode.c
 */
#ifndef NFS_MODULE
	MODULE(nfs,fs);
	WSTUB(nfs,	nfs_getvnodeops,	nomod_zero);
	WSTUB(nfs,	nfs_perror,		nomod_zero);
	WSTUB(nfs,	nfs_cmn_err,		nomod_zero);
	END_MODULE(nfs);
#endif

/*
 * Stubs for nfs_dlboot (diskless booting).
 */
#ifndef NFS_DLBOOT_MODULE
	MODULE(nfs_dlboot,misc);
	STUB(nfs_dlboot,	mount_root,	nomod_minus_one);
	STUB(nfs_dlboot,	mount3_root,	nomod_minus_one);
	END_MODULE(nfs_dlboot);
#endif

/*
 * Stubs for nfs server-only code.
 * nfs_fhtovp and nfs3_fhtovp are not listed here because of NFS
 * performance considerations.
 */
#ifndef NFSSRV_MODULE
	MODULE(nfssrv,misc);
        STUB(nfssrv,            exportfs,       nomod_minus_one);
        STUB(nfssrv,            nfs_getfh,      nomod_minus_one);
	NO_UNLOAD_STUB(nfssrv,	nfs_svc,	nomod_zero);
	END_MODULE(nfssrv);
#endif

/*
 * Stubs for kernel lock manager.
 */
#ifndef KLM_MODULE
	MODULE(klmmod,misc);
	NO_UNLOAD_STUB(klmmod, lm_svc,		nomod_zero);
	NO_UNLOAD_STUB(klmmod, lm_shutdown,	nomod_zero);
	END_MODULE(klmmod);
#endif
 
#ifndef KLMOPS_MODULE
	MODULE(klmops,misc);
	NO_UNLOAD_STUB(klmops, lm_frlock,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm4_frlock,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm_shrlock,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm4_shrlock,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm_nlm_dispatch,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm_nlm4_dispatch,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm_nlm_reclaim,	nomod_zero);
	NO_UNLOAD_STUB(klmops, lm_nlm4_reclaim,	nomod_zero);
	END_MODULE(klmops);
#endif


/*
 * Stubs for kernel TLI module
 *   XXX currently we never allow this to unload
 */
#ifndef TLI_MODULE
	MODULE(tlimod,misc);
	NO_UNLOAD_STUB(tlimod,	t_kopen,		nomod_minus_one);
	NO_UNLOAD_STUB(tlimod,	t_kunbind,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kadvise,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_krcvudata,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_ksndudata,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kalloc,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kbind,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kclose,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kspoll,		nomod_zero);
	NO_UNLOAD_STUB(tlimod,	t_kfree,		nomod_zero);
	END_MODULE(tlimod);
#endif

/*
 * Stubs for kernel RPC module
 *   XXX currently we never allow this to unload
 */
#ifndef RPC_MODULE
	MODULE(rpcmod,strmod);
	NO_UNLOAD_STUB(rpcmod,	clnt_tli_kcreate,	nomod_minus_one);
	NO_UNLOAD_STUB(rpcmod,	svc_tli_kcreate,	nomod_minus_one);
	NO_UNLOAD_STUB(rpcmod,	bindresvport,		nomod_minus_one);
	NO_UNLOAD_STUB(rpcmod,	xdrmblk_init,		nomod_zero);
	NO_UNLOAD_STUB(rpcmod,	xdrmem_create,		nomod_zero);
	END_MODULE(rpcmod);
#endif

/*
 * Stubs for des
 */
#ifndef DES_MODULE
	MODULE(des,misc);
	STUB(des, cbc_crypt, 	 	nomod_zero);
	STUB(des, ecb_crypt, 		nomod_zero);
	STUB(des, _des_crypt,		nomod_zero);
	END_MODULE(des);
#endif

/*
 * Stubs for procfs. An non-unloadable module.
 */
#ifndef PROC_MODULE
	MODULE(procfs,fs);
	NO_UNLOAD_STUB(procfs, prfree,		nomod_zero);
	NO_UNLOAD_STUB(procfs, prexit,		nomod_zero);
	NO_UNLOAD_STUB(procfs, prlwpexit,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prinvalidate,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prnsegs,		nomod_zero);
	NO_UNLOAD_STUB(procfs, prgetstatus,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prgetlwpstatus,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prgetpsinfo,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prgetlwpsinfo,	nomod_zero);
	NO_UNLOAD_STUB(procfs, oprgetstatus,	nomod_zero);
	NO_UNLOAD_STUB(procfs, oprgetpsinfo,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prnotify,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prexecstart,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prexecend,	nomod_zero);
	NO_UNLOAD_STUB(procfs, prrelvm,		nomod_zero);
	NO_UNLOAD_STUB(procfs, prbarrier,	nomod_zero);
	NO_UNLOAD_STUB(procfs, pr_getprot,	nomod_zero);
	NO_UNLOAD_STUB(procfs, pr_free_my_pagelist, nomod_zero);
	END_MODULE(procfs);
#endif

/*
 * Stubs for fifofs
 */
#ifndef FIFO_MODULE
	MODULE(fifofs,fs);
	STUB(fifofs, fifovp,      	0);
	STUB(fifofs, fifo_getinfo,	0);
	STUB(fifofs, fifo_vfastoff,	0);
	END_MODULE(fifofs);
#endif

/*
 * Stubs for ufs
 *
 * This is needed to support the old quotactl system call.
 * When the old sysent stuff goes away, this will need to be revisited.
 */
#ifndef UFS_MODULE
	MODULE(ufs,fs);
	STUB(ufs, quotactl, nomod_minus_one);
	END_MODULE(ufs);
#endif

/*
 * Stubs for namefs
 */
#ifndef NAMEFS_MODULE
	MODULE(namefs,fs);
	STUB(namefs, nm_unmountall, 	0);
	END_MODULE(namefs);
#endif

/*
 * Stubs for ts_dptbl
 */
#ifndef TS_DPTBL_MODULE
	MODULE(TS_DPTBL,sched);
	STUB(TS_DPTBL, ts_getdptbl,		0);
	STUB(TS_DPTBL, ts_getkmdpris,		0);
	STUB(TS_DPTBL, ts_getmaxumdpri,	0);
	END_MODULE(TS_DPTBL);
#endif

/*
 * Stubs for ia_dptbl
 */
#ifndef IA_DPTBL_MODULE
	MODULE(IA_DPTBL,sched);
	STUB(IA_DPTBL, ia_getdptbl,		nomod_zero);
	STUB(IA_DPTBL, ia_getkmdpris,		nomod_zero);
	STUB(IA_DPTBL, ia_getmaxumdpri,	nomod_zero);
	END_MODULE(IA_DPTBL);
#endif

/*
 * Stubs for rt_dptbl
 */
#ifndef RT_DPTBL_MODULE
	MODULE(RT_DPTBL,sched);
	STUB(RT_DPTBL, rt_getdptbl,		0);
	END_MODULE(RT_DPTBL);
#endif

/*
 * Stubs for kb (only needed for 'win')
 */
#ifndef KB_MODULE
	MODULE(kb,strmod);
	STUB(kb, strsetwithdecimal,	0);
	END_MODULE(kb);
#endif

/*
 * Stubs for swapgeneric
 */
#ifndef SWAPGENERIC_MODULE
	MODULE(swapgeneric,misc);
	STUB(swapgeneric, rootconf,     0);
	STUB(swapgeneric, getfstype,    0);
	STUB(swapgeneric, dumpinit,     0);
	STUB(swapgeneric, getswapdev,   0);
	STUB(swapgeneric, getrootdev,   0);
	STUB(swapgeneric, getfsname,    0);
	STUB(swapgeneric, loadrootmodules, 0);
	STUB(swapgeneric, getlastfrompath, 0);
	STUB(swapgeneric, loaddrv_hierarchy, 0);
	END_MODULE(swapgeneric);
#endif

/*
 * Stubs for bootdev
 */
#ifndef BOOTDEV_MODULE
	MODULE(bootdev,misc);
	STUB(bootdev, i_promname_to_devname,	0);
	END_MODULE(bootdev);
#endif
/*
 * stubs for strplumb...
 */
#ifndef STRPLUMB_MODULE
	MODULE(strplumb,misc);
	STUB(strplumb, strplumb,     0);
	STUB(strplumb, strplumb_get_driver_list, 0);
	END_MODULE(strplumb);
#endif

/*
 * Stubs for console configuration module
 */
#ifndef CONSCONFIG_MODULE
	MODULE(consconfig,misc);
	STUB(consconfig, consconfig,     0);
	END_MODULE(consconfig);
#endif

/* 
 * Stubs for accounting.
 */
#ifndef SYSACCT_MODULE
	MODULE(sysacct,sys);
	WSTUB(sysacct, acct,  		nomod_zero);
	END_MODULE(sysacct);
#endif

/*
 * Stubs for ipcsemaphore routines. sem.c
 */
#ifndef SEMSYS_MODULE
	MODULE(semsys,sys);
	WSTUB(semsys, semexit,		nomod_zero);
	END_MODULE(semsys);
#endif

/*
 * Stubs for ipcshmem routines. shm.c
 */
#ifndef SHMSYS_MODULE
	MODULE(shmsys,sys);
	WSTUB(shmsys, shmexit,		nomod_zero);
	WSTUB(shmsys, shmfork,		nomod_zero);
	END_MODULE(shmsys);
#endif

/*
 * Stubs for doors
 */
#ifndef DOOR_MODULE
	MODULE(doorfs,sys);
	WSTUB(doorfs, door_slam,	nomod_zero);
	NO_UNLOAD_STUB(doorfs, door_get_activation,	nomod_zero);
	NO_UNLOAD_STUB(doorfs, door_release_activation,	nomod_zero);
	END_MODULE(doorfs);
#endif

/*
 * Stubs for timod
 */
#ifndef TIMOD_MODULE
	MODULE(timod,strmod);
	STUB(timod, ti_doname,		nomod_minus_one);
	END_MODULE(timod)
#endif

/*
 * Stubs for the remote kernel debugger.
 */
#ifndef DSTUB4C_MODULE
	MODULE(dstub4c,misc);
	STUB(dstub4c,  _db_install,	nomod_zero);
	WSTUB(dstub4c, _db_poll,	nomod_zero);
	WSTUB(dstub4c, _db_scan_pkt,	nomod_zero);
	END_MODULE(dstub4c);
#endif

/*
 * Stubs for dma routines. dmaga.c
 * (These are only needed for cross-checks, not autoloading)
 */
#ifndef DMA_MODULE
	MODULE(dma,drv);
	WSTUB(dma, dma_alloc,		nomod_zero); /* (DMAGA *)0 */
	WSTUB(dma, dma_free,		nomod_zero); /* (DMAGA *)0 */
	END_MODULE(dma);
#endif

/*
 * Stubs for sunwindow routines.
 */
#ifndef WIN_MODULE
	MODULE(win,drv);
	WSTUB(win, mem_rop,			nomod_zero);
	WSTUB(win, mem_putcolormap,		nomod_zero);
	WSTUB(win, mem_putattributes,		nomod_zero);
	END_MODULE(win);
#endif

/*
 * Stubs for auditing.
 */
#ifndef C2AUDIT_MODULE
	MODULE(c2audit,sys);
	STUB(c2audit,  audit_init,		nomod_zero);
	STUB(c2audit,  _auditsys,		nomod_zero);
	WSTUB(c2audit, audit_free,		nomod_zero);
	WSTUB(c2audit, audit_start, 		nomod_zero);
	WSTUB(c2audit, audit_finish,		nomod_zero);
	WSTUB(c2audit, audit_suser,		nomod_zero);
	WSTUB(c2audit, audit_newproc,		nomod_zero);
	WSTUB(c2audit, audit_pfree,		nomod_zero);
	WSTUB(c2audit, audit_thread_free,	nomod_zero);
	WSTUB(c2audit, audit_thread_create,	nomod_zero);
	WSTUB(c2audit, audit_falloc,		nomod_zero);
	WSTUB(c2audit, audit_unfalloc,		nomod_zero);
	WSTUB(c2audit, audit_closef,		nomod_zero);
	WSTUB(c2audit, audit_copen,		nomod_zero);
	WSTUB(c2audit, audit_core_start,	nomod_zero);
	WSTUB(c2audit, audit_core_finish,	nomod_zero);
	WSTUB(c2audit, audit_stropen,		nomod_zero);
	WSTUB(c2audit, audit_strclose,		nomod_zero);
	WSTUB(c2audit, audit_strioctl,		nomod_zero);
	WSTUB(c2audit, audit_strputmsg,		nomod_zero);
	WSTUB(c2audit, audit_c2_revoke,		nomod_zero);
	WSTUB(c2audit, audit_savepath,		nomod_zero);
	WSTUB(c2audit, audit_anchorpath,	nomod_zero);
	WSTUB(c2audit, audit_addcomponent,	nomod_zero);
	WSTUB(c2audit, audit_exit,		nomod_zero);
	WSTUB(c2audit, audit_exec,		nomod_zero);
	WSTUB(c2audit, audit_symlink,		nomod_zero);
	WSTUB(c2audit, audit_vncreate_start,	nomod_zero);
	WSTUB(c2audit, audit_vncreate_finish,	nomod_zero);
	WSTUB(c2audit, audit_enterprom,		nomod_zero);
	WSTUB(c2audit, audit_exitprom,		nomod_zero);
	WSTUB(c2audit, audit_chdirec,		nomod_zero);
	WSTUB(c2audit, audit_getf,		nomod_zero);
	WSTUB(c2audit, audit_setf,		nomod_zero);
	WSTUB(c2audit, audit_sock,		nomod_zero);
	WSTUB(c2audit, audit_strgetmsg,		nomod_zero);
	END_MODULE(c2audit);
#endif

/*
 * Stubs for kernel rpc security service module
 */
#ifndef RPCSEC_MODULE
        MODULE(rpcsec,misc);
        NO_UNLOAD_STUB(rpcsec, sec_clnt_revoke,		nomod_zero);
        NO_UNLOAD_STUB(rpcsec, authkern_create,		nomod_zero);
        NO_UNLOAD_STUB(rpcsec, sec_svc_msg,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec, sec_svc_control,		nomod_zero);
        END_MODULE(rpcsec);
#endif
 
/*
 * Stubs for rpc RPCSEC_GSS security service module
 */
#ifndef RPCSEC_GSS_MODULE
	MODULE(rpcsec_gss,misc);
	NO_UNLOAD_STUB(rpcsec_gss, __svcrpcsec_gss,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_set_svc_name,	nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_getcred,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_set_callback,	nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_secget,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_secfree,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_seccreate,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_set_defaults,	nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_revauth,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_secpurge,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_cleanup,		nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_get_versions,        nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_max_data_length,     nomod_zero);
	NO_UNLOAD_STUB(rpcsec_gss, rpc_gss_svc_max_data_length, nomod_zero);
	END_MODULE(rpcsec_gss);
#endif


#ifndef SAD_MODULE
	MODULE(sad,drv);
	STUB(sad, sadinit, 0);
	STUB(sad, ap_free, 0);
	END_MODULE(sad);
#endif

#ifndef WC_MODULE
	MODULE(wc,drv);
	STUB(wc, wcvnget, 0);
	STUB(wc, wcvnrele, 0);
	END_MODULE(wc);
#endif

#ifndef IWSCN_MODULE
	MODULE(iwscn,drv);
	STUB(iwscn, srredirecteevp_lookup, 0);
	STUB(iwscn, srredirecteevp_rele, 0);
	STUB(iwscn, srpop, 0);
	END_MODULE(iwscn);
#endif

/*
 * Stubs for checkpoint-resume module
 */
#ifndef CPR_MODULE
        MODULE(cpr,misc);
        STUB(cpr, cpr, 0);
        END_MODULE(cpr);
#endif

/*
 * Stubs for kernel probes (tnf module).  Not unloadable.
 */
#ifndef TNF_MODULE
	MODULE(tnf,drv);
	NO_UNLOAD_STUB(tnf, tnf_ref32_1,        nomod_zero);
	NO_UNLOAD_STUB(tnf, tnf_string_1,       nomod_zero);
	NO_UNLOAD_STUB(tnf, tnf_opaque_array_1, nomod_zero);
	NO_UNLOAD_STUB(tnf, tnf_struct_tag_1,   nomod_zero);
	NO_UNLOAD_STUB(tnf, tnf_allocate,       nomod_zero);
	END_MODULE(tnf);
#endif

! this is just a marker for the area of text that contains stubs 
	.text
	.globl stubs_end
stubs_end:
	nop

#endif	/* lint */

/* From
 *	sun4/ml/subr_4.s and
 *	sparc/sys/asm_linkage.h
 * An "argument" is passed in %r11 by STUB_COMMON
 */

#if defined(lint)

unsigned int
stub_install_common(void)
{ return (0); }

#else	/* lint */

	ENTRY_NP(stub_install_common)
	mflr	%r0
	stw	%r0, 4(%r1)		! save the real LR
	stwu	%r1, -SA(MINFRAME)(%r1)	! setup stack frame
	mr	%r3, %r11		! pass the "argument" for C call
	bl	install_stub
	addi	%r1, %r1, SA(MINFRAME)	! restore stack frame
	lwz	%r0, 4(%r1)		! restore LR
	mtctr	%r3
	mtlr	%r0
	bctr
	SET_SIZE(stub_install_common)

#endif	/* lint */
