	.ident	"@(#)lchown.s	1.7	96/06/01 SMI"


	.file	"lchown.s"

	.text

	.globl  __cerror

_fwdef_(`lchown'):
	MCOUNT
	movl	$LCHOWN,%eax
	lcall   $SYSCALL_TRAPNUM,$0
	jae	noerror
	_prologue_
_m4_ifdef_(`DSHLIB',
	`pushl	%eax',
	`'
)
	jmp	_fref_(__cerror)
noerror:
	xorl	%eax, %eax
	ret
