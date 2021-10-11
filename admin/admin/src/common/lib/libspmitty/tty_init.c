#ifndef lint
#pragma ident "@(#)tty_init.c 1.3 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_init.c
 * Group:	libspmitty
 * Description:
 *	Curses functions to initialize common key mapping/naming
 *	and to register functions.
 *	This should all be done first thing by any app using this
 *	library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <libintl.h>
#include <sys/types.h>

#include "spmitty_api.h"
#include "tty_utils.h"
#include "tty_strings.h"

Fkey_check_func	_fkey_notice_check_func = NULL;
Fkey_check_func	_fkey_mvwgets_check_func = NULL;

Fkeys_init_func _fkeys_init_func = NULL;
Fkey *_fkeys = NULL;
int _num_fkeys = 0;

/*
 * Function: fkey_index
 * Description:
 *	convert a function keys specifier (like F_DELETE) into
 *	a corresponding array index.
 * Scope:       PUBLIC
 * Parameters:
 *	bits - function key specifier (UL bit pattern)
 * Return:
 *	- array index value corresponding to 'bits'
 * Globals:
 * Notes:
 */
int
fkey_index(u_long bits)
{
	u_long b;
	int i;

	if (bits == 0L)		/* sanity check */
		return (0);
	b = 1;
	for (i = 0; i < 64; i++) {
		if (b == bits)
			break;
		b <<= 1;
	}
	return (i);
}

/*
 * Function: wfooter_fkeys_init
 * Description:
 *	Allows the application to initialize the set of function keys
 *	that will  be used throught the app to recognize all user input.
 * Scope:       PUBLIC
 * Parameters:
 *	fkeys - the apps array of function key information
 *	num_fkeys - # of keys in the fkeys array
 *	fkeys_init_func - app specific function key init function
 *			  This might be called if things ever need to
 *			  be re-initialized (e.g. if we enter 'escape' mode.)
 * Return: none
 * Notes:
 */
void
wfooter_fkeys_init(
	Fkey *fkeys,
	int num_fkeys,
	Fkeys_init_func fkeys_init_func)
{
	_fkeys = fkeys;
	_num_fkeys = num_fkeys;
	_fkeys_init_func = fkeys_init_func;
}

/*
 * Function: wfooter_fkeys_func_init
 * Description:
 *	fill in the function keys information that is common
 *	across all apps.
 * Scope:       PUBLIC
 * Parameters:
 *	f_keys - the array of function key information
 *	full_init - should we reinit from scratch or leave in any f_func
 *		strings that already exist
 * Return:
 *	- array index value corresponding to 'bits'
 * Globals:
 * Notes:
 */
void
wfooter_fkeys_func_init(Fkey *f_keys, int full_init)
{
	if (full_init || !f_keys[fkey_index(F_OKEYDOKEY)].f_func)
		f_keys[fkey_index(F_OKEYDOKEY)].f_func = DESC_F_OKEYDOKEY;

	if (full_init || !f_keys[fkey_index(F_CONTINUE)].f_func)
		f_keys[fkey_index(F_CONTINUE)].f_func = DESC_F_CONTINUE;

	if (full_init || !f_keys[fkey_index(F_HALT)].f_func)
		f_keys[fkey_index(F_HALT)].f_func = DESC_F_HALT;

	if (full_init || !f_keys[fkey_index(F_CANCEL)].f_func)
		f_keys[fkey_index(F_CANCEL)].f_func = DESC_F_CANCEL;

	if (full_init || !f_keys[fkey_index(F_EXIT)].f_func)
		f_keys[fkey_index(F_EXIT)].f_func = DESC_F_EXIT;

	if (full_init || !f_keys[fkey_index(F_HELP)].f_func)
		f_keys[fkey_index(F_HELP)].f_func = DESC_F_HELP;

	if (full_init || !f_keys[fkey_index(F_GOTO)].f_func)
		f_keys[fkey_index(F_GOTO)].f_func = DESC_F_GOTO;

	if (full_init || !f_keys[fkey_index(F_MAININDEX)].f_func)
		f_keys[fkey_index(F_MAININDEX)].f_func = DESC_F_MAININDEX;

	if (full_init || !f_keys[fkey_index(F_TOPICS)].f_func)
		f_keys[fkey_index(F_TOPICS)].f_func = DESC_F_TOPICS;

	if (full_init || !f_keys[fkey_index(F_REFER)].f_func)
		f_keys[fkey_index(F_REFER)].f_func = DESC_F_REFER;

	if (full_init || !f_keys[fkey_index(F_HOWTO)].f_func)
		f_keys[fkey_index(F_HOWTO)].f_func = DESC_F_HOWTO;

	if (full_init || !f_keys[fkey_index(F_EXITHELP)].f_func)
		f_keys[fkey_index(F_EXITHELP)].f_func = DESC_F_EXITHELP;

}

/*
 * Function: fkey_notice_check_register
 * Description:
 *	Allow the app to register the function it wants to use
 *	for handling input on 'notice' screens.
 * Scope:       PUBLIC
 * Parameters:
 *	func - function to register
 * Return: none
 * Globals:
 * Notes:
 */
void
fkey_notice_check_register(Fkey_check_func func)
{
	_fkey_notice_check_func = func;
}

/*
 * Function: fkey_mvwgets_check_register
 * Description:
 *	Allow the app to register the function it wants to use
 *	for handling input in mvwgets.
 * Scope:       PUBLIC
 * Parameters:
 *	func - function to register
 * Return: none
 * Globals:
 * Notes:
 */
void
fkey_mvwgets_check_register(Fkey_check_func func)
{
	_fkey_mvwgets_check_func = func;
}
