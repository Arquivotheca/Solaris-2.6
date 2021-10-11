#ifndef lint
#pragma ident "@(#)v_types.h 1.14 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_types.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_TYPES_H
#define	_V_TYPES_H

#if !defined(TRUE) || ((TRUE) != 1)
#define	TRUE (1)
#endif
#if !defined(FALSE) || ((FALSE) != 0)
#define	FALSE (0)
#endif

#if !defined(MBYTE)
#define	MBYTE 1048576.0
#endif

#if !defined(KBYTE)
#define	KBYTE 1024.0
#endif

/* Installation type */
typedef enum {
	V_QUICK_INSTALL = 0,
	V_CUSTOM_INSTALL = 1,
	V_UPGRADE = 2
} V_InstallType_t;

/* Custom installation machine types */
typedef enum {
	V_STANDALONE = 0,
	V_SERVER = 1
} V_SystemType_t;

/* v_* function return values */
typedef enum {
	V_PRODUCT = 0,
	V_METACLUSTER = 1,
	V_CLUSTER = 2,
	V_PACKAGE = 3
} V_ModType_t;

/* v_* function return values */
typedef enum {
	V_OK = 0,
	V_FAILURE = 1
} V_Status_t;

typedef enum {
	V_NOT_TESTED = 0,
	V_TEST_SUCCESS = 1,
	V_TEST_FAILURE = 2
} V_TestMount_t;

#endif				/* _V_TYPES_H */
