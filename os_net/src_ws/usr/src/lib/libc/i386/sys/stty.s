	.ident	"@(#)stty.s	1.7	96/06/01 SMI"


	.file	"stty.s"

	.text

	.globl	__cerror

_fwdef_(`stty'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$STTY,%eax
	lcall   $SYSCALL_TRAPNUM,$0
	jae	noerror
	_prologue_
_m4_ifdef_(`DSHLIB',
	`pushl	%eax',
	`'
)
	jmp	_fref_(__cerror)
noerror:
	ret
