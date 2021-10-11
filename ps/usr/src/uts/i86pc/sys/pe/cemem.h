/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)cemem.h	1.2	93/11/02 SMI"

#ifdef DOS
extern char far *CE_Make_Pointer();
#define MP_READ(offset)		((unsigned char) CE_Dos_Read(offset) & 0xff)
#define MP_WRITE(offset, value)	CE_Dos_Write(offset, value)
#define MP_Make_Pnt(offset)	CE_Make_Pointer(offset)
#endif

#ifdef SHELL
#define MP_READ(offset)		sio_mem_read(offset)
#define MP_WRITE(offset, value)	sio_mem_write(offset, value)
#define MP_Make_Pnt(offset)	(char *)0
#endif

#ifdef KERNEL
extern char *CC_Memory;
#define MP_READ(offset)		CC_Memory[offset]
#define MP_WRITE(offset, value)	CC_Memory[offset] = value
#define MP_Make_Pnt(offset)	&CC_Memory[offset]
#endif

