	.ident	"@(#)link.s	1.7	96/06/01 SMI"

	.file	"link.s"

	.text

	.globl	__cerror

_fwdef_(`link'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LINK,%eax
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
