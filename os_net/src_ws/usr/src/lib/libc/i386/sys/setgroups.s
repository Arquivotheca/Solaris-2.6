	.ident	"@(#)setgroups.s	1.7	96/06/01 SMI"

	.file	"setgroups.s"

	.globl	__cerror

_fwdef_(`setgroups'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETGROUPS,%eax
	lcall   $SYSCALL_TRAPNUM,$0
	jae	noerror
	_prologue_
_m4_ifdef_(`DSHLIB',
	`pushl	%eax',
	`'
)
	jmp	_fref_(__cerror)
noerror:
	xorl	%eax,%eax
	ret
