/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * ui.h: User Interface control function definitions
 *
 * Description:
 *  This file provides the interface definition for user interface
 *  functions. These are the routines that provide the interface to the user
 *  in the event that some or all of the config parameters cannot
 *  be discovered automatically.
 *
 * The following exported routines are found in this file
 *
 *  int get_display_interactive(int)
 *  int get_keyboard_interactive(int)
 *  int get_pointer_interactive(int)
 *  int get_config_confirm(int)
 *
 * This file also provides interface definitions for these routines.
 *
 */

#ifndef _INC_UI_H_
#define	_INC_UI_H_

#pragma ident "@(#)ui.h 1.7 94/07/13 SMI"

/*
 * CONTROL functions - called from the main program to build displays and
 *			elicit responses from the user.
 */

/*
 * Functions:	get_display_interactive(int)
 *		get_keyboard_interactive(int)
 *		get_pointer_interactive(int)
 *
 * Returns:
 *	An integer indicating desired navigational direction (either
 *	forward to next screen or backwards to previous screen).
 *
 * Parameters:
 *	An integer indicating the current navigational direction
 *	(either forwards or backwards).
 *
 * Function:
 *  These routines are called by the controlling routine in order to
 *  interactively detrmine what the user desired selections will be.
 *  There is one function for the retrieval of each data element: display,
 *  keyboard and mouse.
 *
 *  These routines should call the appropriate access functions to build the
 *  lists of selections, should build the screens and get the responses, and
 *  should use the selection functions to set the selected data.
 *
 *  After making the selection, the user should have the opportunity to
 *  test the selection interactively.
 *
 *  These control functions must set some value using the selection mechanism
 *  before exiting.
 *
 * Interfaces:
 *   get_category_list();
 *   get_device_info();
 *   set_selected_device();
 */

int get_display_interactive(int);
int get_keyboard_interactive(int);
int get_pointer_interactive(int);

/*
 * Function: confirm_what
 *
 * Returns:
 *	CONFIRM_OK or CONFIRM_NO
 *
 *	The user will indicate if any of the manually selected items
 *	are incorrect.
 *
 * Parameters:
 *	  (int) confirm_what
 *		Range: CONFIRM_DIS | CONFIRM_KBD | CONFIRM_PTR in combination
 *
 * Function:
 *   Displays selected items, offers final confirmation.
 *   This screen will display only the items that were
 *   selected interactively. Those can be determined by looking
 *   at the appropriate bits in the passed parameter.
 *
 * Interfaces:
 *   get_selected_device();
 *
 */

#define	CONFIRM_DIS	0x01
#define	CONFIRM_KBD 0x02
#define	CONFIRM_PTR 0x04
#define CONFIRM_MON 0x08
#define	CONFIRM_NONE 0x0
#define	CONFIRM_ALL CONFIRM_DIS|CONFIRM_KBD|CONFIRM_PTR|CONFIRM_MON

#define	CONFIRM_OK 0
#define	CONFIRM_NO 1

int get_config_confirm(int);

#define	NAV_FORWARD	0
#define	NAV_BACKWARD	1

void	ui_init(void);
int		ui_intro(void);
void	ui_wintro(void);
void	ui_error(char *, int);

#endif /* _INC_UI_H_ */
