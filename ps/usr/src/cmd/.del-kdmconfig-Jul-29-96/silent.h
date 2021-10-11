/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * silent.h: External defintitions for silent mode configuration
 *
 * Description:
 *  This file defines the interface to the silent mode configuration
 *  action routines. These routines attempt to configure the various
 *  aspects of window system configuration without user interaction.
 *
 * The following exported routines are found in this file
 *
 *  int get_display_silent(void)
 *  int get_keyboard_silent(void)
 *  int get_pointer_silent(void)
 *
 * This file also holds the interface definitions for these routines.
 *
 */

#ifndef _INC_SILENT_H_
#define	_INC_SILENT_H_
#pragma ident "@(#)silent.h 1.3 94/08/10 SMI"

int get_display_silent(void);

int get_keyboard_silent(void);

int get_pointer_silent(void);

int get_xin_silent(void);

#endif /* _INC_SILENT_H_ */
