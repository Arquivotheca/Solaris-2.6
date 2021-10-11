/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * windvc.h: API layer over base dvc library to facilitate windows config.
 *
 * Description:
 *  This file provides the public interfaces for the windvc layer. This
 *  layer provides a simplification of the standard dvc routines, tailored
 *  to the OpenWindows config program (kdmconfig).
 *
 * The following exported routines are defined in this file
 *
 *  void dvc_init(void)
 *  char ** get_category_list(cat_t catname);
 *  NODE get_device_info(cat_t catname, int idx);
 *  void set_selected_device(NODE node);
 *  NODE get_selected_device(cat_t catname);
 *  void dvc_commit(void);
 *  NODE get_device_by_name(cat_t catname, char * nodename);
 *
 *  Attribute Management functions:

 *  ATTRIB get_attrib_name_list(NODE node);
 *  val_t get_attrib_type(NODE node, ATTRIB attr);
 *  char * get_attrib_title(NODE node, ATTRIB attr);
 *  VAL get_attrib_value(NODE node, ATTRIB attr);
 *  VAL get_selected_attrib_value(NODE node, ATTRIB attr);
 *  void set_selected_attrib_value(NODE node, ATTRIB attr, VAL val);
 *
 * This file also provides inteface definitions for these routines.
 */

#ifndef _INC_WINDVC_H_
#define	_INC_WINDVC_H_
#pragma ident "@(#)windvc.h 1.12 95/02/27 SMI"

#include "dvc.h"
typedef char *cat_t;
typedef device_info_t *NODE;
typedef char	*ATTRIB;
typedef void	*VAL;

#define	PCI_STR		"PCI"
#define	REG_STR		"reg"
#define	NUMERIC_STR	"numeric,0xA0000000"
#define	VLB_STR		"VLB"

/*
 * Function: dvc_init()
 *
 * Returns:
 *	void
 *
 * Parameters:
 *	void
 *
 * Function:
 *	This routine must be called one time initially to set up all
 *	internal data structures.
 *
 * Interfaces:
 *	fetch_dvc_info()
 */

void dvc_init(void);

/*
 * List Retrival - Routines used to get lists of possible devices,
 *			and attribute lists for those devs.
 */


/*
 * Functions: get_category_list()
 *		get_device_info()
 *
 * Returns:
 *	get_* function returns pointer to a NODE object which
 *	is the currently selected device.
 *
 *	If no object has yet been selected with a set_* call, then
 *	NULL is returned.
 *
 *	set_* function returns nothing (void).
 *
 * Parameters:
 *	set_* function passes a pointer to a NODE, with the
 *	selected attribute values in the dev_alist field. The most recent
 *	node item of a particular category type (display, keyboard, pointer)
 *	will overwrite any earlier call.
 *
 * Function:
 *  These routines are called to select a specific entry, or get the selected
 *  one. Setting is done by the interactive or the silent get_keyboard,
 *  get_display and get_pointer routines. Getting is done by the confirm
 *  routine, and by the commit routine.
 *
 * Interfaces:
 *   None
 */


char ** get_category_list(cat_t catname);
NODE get_device_info(cat_t catname, int idx);


/*
 * SELECTION functions - used to get selected node, or to select a
 *			specific node.
 */

/*
 * Functions: get_selected_device()
 *		set_selected_device()
 *
 * Returns:
 *	get_* function returns pointer to a NODE object which
 *	is the currently selected device.
 *
 *	If no object has yet been selected with a set_* call, then
 *	NULL is returned.
 *
 *	set_* function returns nothing (void).
 *
 * Parameters:
 *	set_* function passes a pointer to a NODE, with the
 *	selected attribute values in the dev_alist field. The most recent
 *	node item of a particular category type (display, keyboard, pointer)
 *	will overwrite any earlier call.
 *
 * Function:
 *  These routines are called to select a specific entry, or get the selected
 *  one. Setting is done by the interactive or the silent get_keyboard,
 *  get_display and get_pointer routines. Getting is done by the confirm
 *  routine, and by the commit routine.
 *
 *  The controlling routine should get the selected node, specifying an index
 *  using the order in which the node titles are output from get_category_list.
 *  The routine should modify the actual values, and then put the same
 *  node back as selected with set_selected_device.
 *
 * Interfaces:
 *   None
 */

#define	DISPLAY_CAT  "display"
#define	KEYBOARD_CAT "keyboard"
#define	POINTER_CAT  "pointer"
#define	MONITOR_CAT  "monitor"

NODE get_selected_device(cat_t catname);
void set_selected_device(NODE devnode);

/*
 * Functions: cat_exists()
 *
 * Returns:
 *	CAT_EXISTS_YES or CAT_EXISTS_NO
 *
 * Parameters:
 *	cat_t (DISPLAY_CAT | KEYBOARD_CAT | POINTER_CAT)
 *
 * Function:
 *	This routine eamines the list of already configured devices to
 *	determine if a device of a given category exists.
 *
 * Interfaces:
 *	get_dev_node()
 *	get_dev_cat()
 *
 */


typedef enum {
	CAT_EXISTS_NO,
	CAT_EXISTS_YES
} cat_exists_t;

cat_exists_t cat_exists(cat_t catname);


/*
 * Function: dvc_commit()
 *
 * Returns:
 *	void
 *
 * Parameters:
 *	void
 *
 * Function:
 *	This routine commits the selected devices, and updates the
 *	appropriate system configuration files.
 *
 * Interfaces:
 *	add_dev_node()
 *	modified_conf()
 *	update_conf()
 *
 */

void dvc_commit(void);

/*
 *  ATTRIBUTE MANAGEMENT Functions - Get lists, select values
 */


/*
 * Function: get_attrib_name_list()
 *
 * Returns:
 *	pointer to null-terminated array of attribute handles.
 *
 * Parameters:
 *	NODE node - node handle
 *
 * Function:
 *	When passed a node handle, this routine returns a list of strings
 *	identifying the attributes of this node. The strings are used to
 *	reference the attribute, as well as a label for attribute display.
 *	The last attribute is followed by a NULL value in the array.
 *
 * Interfaces:
 *	dvc NODE internals.
 *
 */

ATTRIB * get_attrib_name_list(NODE node);


/*
 * Function: get_attrib_type()
 *
 * Returns:
 *	VAL_NUMERIC or VAL_STRING
 *
 * Parameters:
 *	NODE node: node handle
 *	ATTRIB attrib: attribute handle
 *
 * Function:
 *
 *	This routine returns the type of the attribute. If it is a
 *	VAL_STRING attribute, then all calls to get_attrib_value will
 *	return pointers to character strings. If it is a VAL_NUMERIC,
 *	then those calls will return pointers to integers.
 *
 * Interfaces:
 *	dvc NODE internals.
 *
 */

val_t get_attrib_type(NODE node, ATTRIB attrib);

/*
 * Function: get_attrib_title()
 *
 * Returns:
 *	VAL_NUMERIC or VAL_STRING
 *
 * Parameters:
 *	NODE node: node handle
 *	ATTRIB attrib: attribute handle
 *
 * Function:
 *
 *	This routine returns the string title for the attribute handle
 *	passed. This is the string that should be used in the display
 *	of the attribute for user interface functions.
 *
 * Interfaces:
 *
 */

char * get_attrib_title(NODE node, ATTRIB attrib);

/*
 * Function: get_attrib_value()
 *
 * Returns:
 *	VAL pointer to next (or default) value for
 *	passed attribute and node handles.
 *	For VAL_NUMERIC attributes, it will return a pointer to an
 *	integer (int *), and for VAL_STRING, it will return a string
 *	(char *).
 *
 * Parameters:
 *	NODE  node: node handle
 *	ATTRIB attrib: attribute handle
 *
 * Function:
 *
 *	when this routine is called with a non-null attrib, it returns
 *	the default value for that attribute. On successive calls, if
 *	the attrib is NULL, it will return alternate values for that
 *	atribute, until it reaches the last one. Then, it will return
 *	pointer to NULL (not NULL itself) every time it is called, until
 *	a non NULL attrib is passed in the attrib parameter.
 *
 *	If NULL is passed as the attrib parameter, the node parameter
 *	will be ignored.
 *
 *	This will be used to get lists of possible values, as well as to
 *	validate a requested attribute against the list of possible vals.
 *
 *	When a new value is set via the set_attrib_value call, the
 *	behavior of this function WILL NOT CHANGE. It will always
 *	return the system default first. This default is specified in
 *	the device .cfinfo file. To retrieve the value that has been
 *	selected (initially the same as the default) use the routine
 *	'get_selected_attrib_value'.
 *
 *
 * Interfaces:
 *
 */

VAL get_attrib_value(NODE  node, ATTRIB attrib);

/*
 * Function: get_selected_attrib_value()
 *
 * Returns:
 *	VAL pointer to selected value for the
 *	passed attribute and node handles.
 *	For VAL_NUMERIC attributes, it will return a pointer to an
 *	integer (int *), and for VAL_STRING, it will return a string
 *	(char *).
 *
 * Parameters:
 *	NODE node: node handle
 *	ATTRIB attrib: attribute handle
 *
 * Function:
 *
 *	When this routine is called, it returns the selected value for
 *	that attribute. This attribute will be the one set by a call
 *	to 'set_attrib_value' or the default value if that function has
 *	never been called for this attribute and this node.
 *
 * Interfaces:
 *
 */


VAL get_selected_attrib_value(NODE node, ATTRIB attrib);

/*
 * Function: set_attrib_value()
 *
 * Returns:
 *	void.
 *
 * Parameters:
 *	NODE node: node handle
 *	ATTRIB attrib: attribute handle
 *	VAL pointer to value being set.
 *
 * Function:
 *	This routine will attempt to set the attribute in question to
 *	the passed value. If the value is not one of those allowed, then
 *	it will not be allowed. It will treat the value according to the
 *	type defined for the value, with no validation. The val parameter
 *	must be an integer (int) or a string (char *) in order for this
 *	function to operate correctly.
 *
 *	For string attributes, the library will make its own copy of
 *	the string passed.
 *
 * Interfaces:
 *	get_selected_attrib_value (to see if I need to do anything)
 *	get_attrib_value (for validation)
 *
 */

void set_attrib_value(NODE node, ATTRIB attrib, VAL val);


/*
 * Function : is_xinside
 *
 * Returns :
 *	int.
 *
 * Parameters:
 *	NODE dev: node handle;
 *
 * Function:
 * 	This routine determines if a display device is an xinside
 * 	device or not.  If it is an xinside display then kdmconfig
 * 	uses needs to find a monitor.
 *
 */
int is_xinside(NODE dev);


/*
 * SPECIFIC NODE SELECTION Function
 */

NODE get_node_by_name(cat_t catname, char * nodename);

#endif /* _INC_WINDVC_H_ */
