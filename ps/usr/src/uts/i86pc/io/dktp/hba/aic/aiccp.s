/*
/* Copyright (c) 1993-94 Sun Microsystems, Inc.
/*
/#ident   "@(#)aiccp.s 1.2     94/09/08 SMI"
/------------------------------------------------------------
/*"@(#)aiccp.s 1.1 dated Apr 9, 1994
/* Version 1.0 R2 Dated Feb 16,1994
/* Version 1.0 R1 Dated Feb 4, 1994
/*
/* Developed by 
/* Wipro Infotech Ltd.,
/* 88, M.G. Road, Bangalore 560 001
/* INDIA
/* for acceptance by SunSoft.
/*
/------------------------------------------------------------
	.text
	.align	4
/==========================================================================
/ aiccopy( port, dest, count )
/ 	This routine is for doing a block move/burst xfer from the
/   TMC data register to the required address or from some virtual address
/	to the TMC data register.
/==========================================================================
	.globl	blockinb
blockinb:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %edi
	movl	0x10(%ebp), %ecx
	cld
	rep
	insb
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	

	.globl	blockoutb
blockoutb:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %esi
	movl	0x10(%ebp), %ecx
	cld
	rep
	outsb
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	
/==========================================================================
	.globl	blockinw
blockinw:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %edi
	movl	0x10(%ebp), %ecx
	cld
	rep
	insw
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	

	.globl	blockoutw
blockoutw:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %esi
	movl	0x10(%ebp), %ecx
	cld
	rep
	outsw
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	
/==========================================================================
	.globl	blockind
blockind:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %edi
	movl	0x10(%ebp), %ecx
	cld
	rep
	insl
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	

	.globl	blockoutd
blockoutd:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	movl 	0x8(%ebp), %edx
	movl	0xc(%ebp), %esi
	movl	0x10(%ebp), %ecx
	cld
	rep
	outsl
	popl	%edi
	popl	%esi
	popl	%ecx
	leave	
	ret	
/==========================================================================
