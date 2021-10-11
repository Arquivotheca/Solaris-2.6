/*
 * Copyright (c) 1995 by Sun Microsystems Inc.
 */

#pragma ident "@(#)intr.c	1.30	95/12/13 SMI"

#include <sys/cpuvar.h>
#include <sys/ivintr.h>
#include <sys/intreg.h>
#include <sys/membar.h>
#include <sys/intr.h>
#include <sys/cmn_err.h>
#include <sys/privregs.h>
#include <sys/debug.h>

kmutex_t soft_iv_lock;	/* protect software interrupt vector table */

u_int swinum_base;
u_int maxswinum;
u_int siron_inum;
u_int poke_cpu_inum;
int siron_pending;


static void sw_ivintr_init();

/*
 * intr_init() - interrutp initialization
 *	Initialize the system's software interrupt vector table and
 *	CPU's interrupt free list
 */
void
intr_init(cpu_t *cp)
{
	sw_ivintr_init(cp);
	init_intr_pool(cp);

	/* init the interrupt distribution mutex */
	mutex_init(&intr_dist_lock, "Interrupt Distribution lock",
		MUTEX_DEFAULT, DEFAULT_WT);
}

/*
 * sw_ivintr_init() - software interrupt vector initialization
 *	called after CPU is active
 *	the software interrupt vector table is part of the intr_vector[]
 */
static void
sw_ivintr_init(cpu_t *cp)
{
	int boot_upaid;
	extern u_int softlevel1();
	extern u_int poke_cpu_intr();

	/*
	 * initialize the software interrupt vector lock
	 */
	mutex_init(&soft_iv_lock, "software interrupt vector lock",
	    MUTEX_DEFAULT, NULL);

	/*
	 * use the boot processor's upa id as the index
	 * of the software inumber base
	 */
	boot_upaid = cpunodes[cp->cpu_id].upaid;
	swinum_base = boot_upaid * MAX_INO;

	/*
	 * the maximum software interrupt == MAX_INO
	 */
	maxswinum = swinum_base + MAX_INO;

	siron_inum = add_softintr(PIL_1, softlevel1, 0, 0);
	poke_cpu_inum = add_softintr(PIL_13, poke_cpu_intr, 0, 0);
	cp->cpu_m.poke_cpu_outstanding = B_FALSE;
}

/*
 * init_intr_pool()
 *	initialize the intr request pool for the cpu
 * 	should be called for each cpu
 */
void
init_intr_pool(cpu_t *cp)
{
	static int seqid = 0;			/* OK since we're not MP yet */
	extern int intr_add_pools;
	extern struct intr_req *intr_add_head;
#ifdef	DEBUG
	extern struct intr_req *intr_add_tail;
	extern int ncpunode;
#endif	/* DEBUG */
	int i;

	for (i = 0; i < INTR_PENDING_MAX-1; i++) {
		cp->cpu_m.intr_pool[i].intr_next =
		    &cp->cpu_m.intr_pool[i+1];
	}
	cp->cpu_m.intr_pool[INTR_PENDING_MAX-1].intr_next = NULL;

	cp->cpu_m.intr_head[0] = &cp->cpu_m.intr_pool[0];
	cp->cpu_m.intr_tail[0] = &cp->cpu_m.intr_pool[INTR_PENDING_MAX-1];

	if (intr_add_pools > 0) {

		/*
		 * If additional interrupt pools have been allocated,
		 * initialize those too and add them to the free list.
		 * We're not MP yet so we can count cpus with 'seqid'.
		 */

		struct intr_req *trace;

		ASSERT(seqid < ncpunode);

		trace = (seqid * INTR_PENDING_MAX * intr_add_pools) +
			intr_add_head;	/* not byte arithmetic */

		cp->cpu_m.intr_pool[INTR_PENDING_MAX-1].intr_next = trace;

		for (i = 1; i < intr_add_pools * INTR_PENDING_MAX; i++, trace++)
			trace->intr_next = trace + 1;
		trace->intr_next = NULL;

		ASSERT(trace >= intr_add_head && trace <= intr_add_tail);

		cp->cpu_m.intr_tail[0] = trace;
		seqid++;			/* cpu_seqid not yet init'd */
	}
}

/*
 * siron - primitive for sun/os/softint.c
 */
void
siron()
{
	extern void setsoftint();

	if (!siron_pending) {
		siron_pending = 1;
		setsoftint(siron_inum);
	}
}

/*
 * poke_cpu_intr - fall through when poke_cpu calls
 */

/* ARGSUSED */
u_int
poke_cpu_intr(caddr_t arg)
{
	CPU->cpu_m.poke_cpu_outstanding = B_FALSE;
	membar_stld_stst();
	return (1);
}

#ifdef DEBUG

/*
 * no_ivintr()
 * 	called by vec_interrupt() through sys_trap()
 *	vector interrupt received but not valid or not
 *	registered in intr_vector[]
 *	considered as a spurious mondo interrupt
 */
/* ARGSUSED */
void
no_ivintr(struct regs *rp, int inum, int pil)
{
	cmn_err(CE_WARN, "Invalid Vector Interrupt: number 0x%x, pil 0x%x\n",
		inum, pil);
#ifdef DEBUG_VEC_INTR
	prom_enter_mon();
#endif DEBUG_VEC_INTR
}

/*
 * no_intr_pool()
 * 	called by vec_interrupt() through sys_trap()
 *	vector interrupt received but no intr_req entries
 */
/* ARGSUSED */
void
no_intr_pool(struct regs *rp, int inum, int pil)
{
#ifdef DEBUG_VEC_INTR
	cmn_err(CE_WARN, "intr_req pool empty: number 0x%x, pil 0x%x\n",
		inum, pil);
	prom_enter_mon();
#else
	cmn_err(CE_PANIC, "intr_req pool empty: number 0x%x, pil 0x%x\n",
		inum, pil);
#endif /* DEBUG_VEC_INTR */
}

/*
 * no_intr_req()
 * 	called by pil_interrupt() through sys_trap()
 *	pil interrupt request received but not valid
 *	considered as a spurious pil interrupt
 */
/* ARGSUSED */
void
no_intr_req(struct regs *rp, u_int intr_req, int pil)
{
	cmn_err(CE_WARN, "invalid pil interrupt request 0x%x, pil 0x%x\n",
		intr_req, pil);
#ifdef DEBUG_PIL_INTR
	prom_enter_mon();
#endif DEBUG_PIL_INTR
}
#endif DEBUG
