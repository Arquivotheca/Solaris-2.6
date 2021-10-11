#ident	"@(#)sparc.il	1.3	94/10/15 SMI"
!
! In-line functions for sparc kernels.
!

! return current thread pointer

	.inline	threadp,0
	mov	%g7, %o0
	.end

! needed by krtld

	.inline doflush,0
	flush	%o0
	.end
