/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * exists.h: Convenience routines for node exiatance tests
 *
 * Description:
 *
 *
 */

#ifndef _INC_EXISTS_H
#define	_INC_EXISTS_H
#pragma ident "@(#)exists.h 1.3 94/07/13 SMI"

#include "windvc.h"

#define	display_exists (cat_exists(DISPLAY_CAT) == CAT_EXISTS_YES)
#define	pointer_exists (cat_exists(POINTER_CAT) == CAT_EXISTS_YES)
#define	keyboard_exists (cat_exists(KEYBOARD_CAT) == CAT_EXISTS_YES)
#define	monitor_exists (cat_exists(MONITOR_CAT) == CAT_EXISTS_YES)

#endif /* _INC_EXISTS_H_ */
