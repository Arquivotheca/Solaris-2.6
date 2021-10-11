#ifndef lint
#pragma ident "@(#)v_filesys.h 1.7 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1992-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_filesys.h
 * Group:	ttinstall
 * Description:
 */

/*
 * Space overhead required by ufs/ffs
 */
#define DSK_OVERHEAD .15

/*
 * error returns from filesys_ok() used in install.c and filesys.c
 */
#define NO_ROOT			1 << 1
#define NO_SWAP			1 << 2
#define EXPORT_TOO_SMALL	1 << 3
#define ROOT_TOO_SMALL		1 << 4
#define USR_TOO_SMALL		1 << 5
#define OPT_TOO_SMALL		1 << 6
#define USR_OWN_TOO_SMALL	1 << 7
#define EXPORT_EXEC_TOO_SMALL	1 << 8
#define EXPORT_ROOT_TOO_SMALL	1 << 9
#define EXPORT_SWAP_TOO_SMALL	1 << 10

extern char   **lfs;

/*
 * functions exported by filesys.c
 */
#ifdef __cplusplus
extern          "C" {
#endif

    extern int      any_filesys(void);

#ifdef __cplusplus
}
#endif
