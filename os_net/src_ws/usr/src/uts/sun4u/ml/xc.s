/*
 *      Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma ident   "@(#)xc.s	1.44	96/08/19 SMI"

#if defined(lint)
#include <sys/types.h>
#include <sys/cpuvar.h>
#else	/*lint */
#include "assym.s"
#endif	/* lint */

#include <sys/param.h>
#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include <sys/intreg.h>
#include <sys/intr.h>
#include <sys/x_call.h>
#include <sys/spitasi.h>
#include <sys/privregs.h>
#include <sys/machthread.h>
#include <sys/machtrap.h>
#include <sys/xc_impl.h>

#ifdef TRAPTRACE
#include <sys/traptrace.h>
#endif /* TRAPTRACE */


#if defined(lint)

/* ARGSUSED */
void
send_self_xcall(struct cpu *cpu, u_int arg1, u_int arg2, u_int arg3, u_int arg4,
    u_int func)
{}

#else

/*
 * For a x-trap request to the same processor, just send a fast trap
 */
	ENTRY_NP(send_self_xcall)
	ta ST_SELFXCALL
	retl
	nop
	SET_SIZE(send_self_xcall)

#endif	/* lint */


#if defined(lint)

/* ARGSUSED */
void
self_xcall(struct cpu *cpu, u_int arg1, u_int arg2, u_int arg3, u_int arg4,
    u_int func)
{}

#else

/*
 *	
 * Entered by the software trap (TT=ST_SELFXCALL, TL>0) thru send_self_xcall().
 * Emulate the mondo handler - vec_interrupt().
 *
 * Global registers are the Alternate Globals.
 *	Entries:
 *		%o0 - CPU
 *		%o5 - function or inumber to call
 *		%o1, %o2, %o3, %o4  - arguments
 *		
 */
	ENTRY_NP(self_xcall)
	!
	! Verify what to do -
	! It could be a fast trap handler address (pc > KERNELBASE)
	! or an interrupt number.
	!
	set	KERNELBASE, %g4
	cmp	%o5, %g4
	bl,pt	%xcc, 0f			! an interrupt number found
	mov	%o5, %g5

	!
	! TL>0 handlers are expected to do "retry"
	! prepare their return PC and nPC now
	!
	rdpr	%tnpc, %g1
	wrpr	%g1, %tpc			!  PC <- TNPC[TL]
 	add	%g1, 4, %g1
	wrpr	%g1, %tnpc			! nPC <- TNPC[TL] + 4

#ifdef TRAPTRACE
	TRACE_PTR(%g4, %g6)
	rdpr	%tick, %g6
	stxa	%g6, [%g4 + TRAP_ENT_TICK]%asi
	rdpr	%tl, %g6
	stha	%g6, [%g4 + TRAP_ENT_TL]%asi
	rdpr	%tt, %g6
	stha	%g6, [%g4 + TRAP_ENT_TT]%asi
	sta	%g5, [%g4 + TRAP_ENT_TR]%asi ! pc of the TL>0 handler
	rdpr	%tpc, %g6
	sta	%g6, [%g4 + TRAP_ENT_TPC]%asi
	rdpr	%tstate, %g6
	stxa	%g6, [%g4 + TRAP_ENT_TSTATE]%asi
	sta	%sp, [%g4 + TRAP_ENT_SP]%asi
	sta	%o1, [%g4 + TRAP_ENT_F1]%asi
	sta	%o3, [%g4 + TRAP_ENT_F2]%asi
	sta	%o2, [%g4 + TRAP_ENT_F3]%asi
	sta	%o4, [%g4 + TRAP_ENT_F4]%asi
	TRACE_NEXT(%g4, %g6, %g3)
#endif TRAPTRACE
	!
	! Load the arguments for the fast trap handler.
	!
	mov	%o1, %g1
	mov	%o2, %g2
	mov	%o3, %g3
	jmp	%g5				! call the fast trap handler
	mov	%o4, %g4			! and won't be back
	/* Not Reached */

0:
	!
	! Fetch data from intr_vector[] table according to the interrupt number.
	!
	! We have an interrupt number.
	! Put the request on the cpu's softint list,
	! and set %set_softint.
	!
	! Register usage
	!	%g5 - inumber
	!	%g2 - requested pil
	!	%g3 - intr_req
	!	%g4 - *cpu
	!	%g1, %g6 - temps
	!
	! allocate an intr_req from the free list
	!
	ld	[%o0 + INTR_HEAD], %g3
	mov	%o0, %g4		! use g4 to match vec_interrupt()'s code
	!
	! if intr_req == NULL, it will cause TLB miss
	! TLB miss handler (TL>0) will call panic
	!
	! get pil from intr_vector table
	!
1:
	set	intr_vector, %g1
#if INTR_VECTOR == 0x10
	sll	%g5, 0x4, %g6
#else
	Error INTR_VECTOR has been changed
#endif
	add	%g1, %g6, %g1		! %g1 = &intr_vector[IN]
	lduh	[%g1 + IV_PIL], %g2
	!
	! fixup free list
	!
	ld	[%g3 + INTR_NEXT], %g6
	st	%g6, [%g4 + INTR_HEAD]
	!
	! fill up intr_req
	!
	st	%g5, [%g3 + INTR_NUMBER]
	st	%g0, [%g3 + INTR_NEXT]
	!
	! move intr_req to appropriate list
	!
	sll	%g2, 2, %g5
	add	%g4, INTR_TAIL, %g6
	ld	[%g6 + %g5], %g1	! current tail
	brz,pt	%g1, 2f			! branch if list empty
	st	%g3, [%g6 + %g5]	! make intr_req new tail
	!
	! there's pending intr_req already
	!
	ba,pt	%xcc, 3f
	st	%g3, [%g1 + INTR_NEXT]	! update old tail
2:
	!
	! no pending intr_req; make intr_req new head
	!
	add	%g4, INTR_HEAD, %g6
	st	%g3, [%g6 + %g5]
#ifdef TRAPTRACE
	TRACE_PTR(%g1, %g6)
	rdpr	%tick, %g6
	stxa	%g6, [%g1 + TRAP_ENT_TICK]%asi
	rdpr	%tl, %g6
	stha	%g6, [%g1 + TRAP_ENT_TL]%asi
	rdpr	%tt, %g6
	stha	%g6, [%g1 + TRAP_ENT_TT]%asi
	rdpr	%tpc, %g6
	sta	%g6, [%g1 + TRAP_ENT_TPC]%asi
	rdpr	%tstate, %g6
	stxa	%g6, [%g1 + TRAP_ENT_TSTATE]%asi
	sta	%sp, [%g1 + TRAP_ENT_SP]%asi
	ld	[%g3 + INTR_NUMBER], %g6
	sta	%g6, [%g1 + TRAP_ENT_TR]%asi
	add	%g4, INTR_HEAD, %g6
	ld	[%g6 + %g5], %g6		! intr_head[pil]
	sta	%g6, [%g1 + TRAP_ENT_F1]%asi
	add	%g4, INTR_TAIL, %g6
	ld	[%g6 + %g5], %g6		! intr_tail[pil]
	sta	%g6, [%g1 + TRAP_ENT_F2]%asi
	sta	%g2, [%g1 + TRAP_ENT_F3]%asi
	sta	%g3, [%g1 + TRAP_ENT_F4]%asi
	TRACE_NEXT(%g1, %g6, %g5)
#endif TRAPTRACE
3:
	!
	! Write %set_softint with (1<<pil) to cause a "pil" level trap
	!
	mov	1, %g1
	sll	%g1, %g2, %g1
	wr	%g1, SET_SOFTINT
	done
	SET_SIZE(self_xcall)

#endif	/* lint */

#if defined(lint)

/* ARGSUSED */
void
xt_sync_tl1(void) {}

#else
/*
 *	This dummy tl1 function is there to ensure that previously called
 *	xtrap handlers have exececuted. The hardware (mondo dispatch mechanism)
 *	is such that return from xtrap doesn't guarantee execution of xtrap
 *	handler. So, callers can call this xtrap-handler to ensure that
 *	the previous one is complete. This is because the hardware only
 *	can handle 1 mondo at a time - when this mondo is handled, we are
 *	sure that the mondo for the previous xtrap must have been handled.
*/
	ENTRY_NP(xt_sync_tl1)
	retry
	SET_SIZE(xt_sync_tl1)

#endif	/* lint */


#if defined(lint)

/* ARGSUSED */
void
xc_trace(u_int traptype, cpuset_t cpu_set, u_int func,
		u_int arg1, u_int arg2, u_int arg3) 
{}

#else
#ifdef  TRAPTRACE
	ENTRY(xc_trace)
	rdpr	%pstate, %g1
	andn	%g1, PSTATE_IE | PSTATE_AM, %g2
	wrpr	%g0, %g2, %pstate		/* disable interrupts */
	TRACE_PTR(%g3, %g4)
	rdpr	%tick, %g4
	stxa	%g4, [%g3 + TRAP_ENT_TICK]%asi
	stha	%g0, [%g3 + TRAP_ENT_TL]%asi
	set	TT_XCALL, %g2
	or	%o0, %g2, %g4
	stha	%g4, [%g3 + TRAP_ENT_TT]%asi
	sta	%o7, [%g3 + TRAP_ENT_TPC]%asi
	sta	%o1, [%g3 + TRAP_ENT_SP]%asi		/* sp = cpuset */
	sta	%o2, [%g3 + TRAP_ENT_TR]%asi		/* tr = func */
	sta	%o3, [%g3 + TRAP_ENT_F1]%asi		/* f1 = arg1 */
	sta	%o4, [%g3 + TRAP_ENT_F2]%asi		/* f2 = arg2 */
	sta	%o5, [%g3 + TRAP_ENT_F3]%asi		/* f3 = arg3 */
	sta	%i7, [%g3 + TRAP_ENT_F4]%asi		/* f4 = xcall caller */
	stxa	%g1, [%g3 + TRAP_ENT_TSTATE]%asi	/* tstate = pstate */
	TRACE_NEXT(%g2, %g3, %g4)
/*
 * In the case of a cpuset of greater size than an int we
 * grab an extra trace buffer just to store the cpuset
 * int's. Seems like a waste but popular opinion opted for this 
 * rather than increase the size of the buffer.
 */
#if NCPU > 32
	TRACE_PTR(%g3, %g4)
	ld	[%o1], %g5
	sta	%g5, [%g3 + TRAP_ENT_F1]%asi
	ld	[%o1+4], %g5
	sta	%g5, [%g3 + TRAP_ENT_F2]%asi
	ld	[%o1+8], %g5
	sta	%g5, [%g3 + TRAP_ENT_F3]%asi
	ld	[%o1+0xc], %g5
	sta	%g5, [%g3 + TRAP_ENT_F4]%asi
	TRACE_NEXT(%g2, %g3, %g4)
#endif	/* NCPU */
	retl
	wrpr	%g0, %g1, %pstate			/* enable interrupts */
	SET_SIZE(xc_trace)
#else	TRAPTRACE
#define	xc_trace(a, b, c, d, e, f)
#endif	TRAPTRACE

#endif	/* lint */

#if defined(lint)
void
idle_stop_xcall(void)
{}
#else
/*
 * idle or stop xcall handler.
 *
 * Called in response to an xt_some initiated by idle_other_cpus
 * and stop_other_cpus.
 *
 *	Entry:
 *		%g1    - handler at TL==0
 *		%g2    - arg1
 *		%g3    - arg2
 *		%g4    - arg3
 *
 * 	Register Usage:
 *		%g1    - handler at TL==0
 *		%g2.lo - arg1
 *		%g3    - arg2
 *		%g2.hi - arg3
 *		%g4    - pil
 *
 * %g1 will either be cpu_idle_self or cpu_stop_self and is
 * passed to sys_trap, to run at TL=0. No need to worry about
 * the regp passed to cpu_idle_self/cpu_stop_self, since
 * neither require arguments.
 */
	ENTRY_NP(idle_stop_xcall)
	sllx	%g4, 32, %g4		! high 32 bit of %g2
	or	%g4, %g2, %g2
	rdpr	%pil, %g4
	cmp	%g4, XCALL_PIL
	ba,pt	%xcc, sys_trap
	movl	%xcc, XCALL_PIL, %g4
	SET_SIZE(idle_stop_xcall)
#endif	/* lint */
