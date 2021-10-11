#ifndef lint
#pragma ident "@(#)tty_pfc.c 1.7 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>

#include "spmitty_api.h"
#include "tty_pfc.h"

static _fkeys_wmenu_check_func(u_long keys, int ch);
static _fkeys_notice_check_func(u_long keys, int ch);

/*
 * Note:  for i18n, the "Description" strings must be encapsulated in calls to
 * gettext() somewhere else in the program in order for xgettext to
 * automatically extract them.
 *
 * Entries are grouped by function key with groups in presentation order.
 * Entries within a group are obviously mutually exclusive...
 */
static Fkey f_keys[] = {
	/* Terminfo DB	Special Name	Fallback	Label */
	{"kf2", "F2", "Esc-2"},	/* OK */
	{"kf2", "F2", "Esc-2"},	/* Auto Layout */
	{"kf2", "F2", "Esc-2"},	/* Continue */
	{"kf2", "F2", "Esc-2"},	/* Begin */
	{"kf2", "F2", "Esc-2"},	/* Exit Install */
	{"kf2", "F2", "Esc-2"},	/* Upgrade System */
	{"kf3", "F3", "Esc-3"},	/* Go Back */
	{"kf3", "F3", "Esc-3"},	/* Test Mount */
	{"kf3", "F3", "Esc-3"},	/* Delete */
	{"kf4", "F4", "Esc-4"},	/* Preserve */
	{"kf4", "F4", "Esc-4"},	/* Re-Layout */
	{"kf4", "F4", "Esc-4"},	/* Show Exports */
	{"kf4", "F4", "Esc-4"},	/* Change */
	{"kf4", "F4", "Esc-4"},	/* Change Type */
	{"kf4", "F4", "Esc-4"},	/* Remote Mounts */
	{"kf4", "F4", "Esc-4"},	/* Customize */
	{"kf4", "F4", "Esc-4"},	/* Edit */
	{"kf4", "F4", "Esc-4"},	/* Create */
	{"kf4", "F4", "Esc-4"},	/* More */
	{"kf4", "F4", "Esc-4"},	/* New Install */
	{"kf4", "F4", "Esc-4"},	/* Manual Layout */
	{"kf5", "F5", "Esc-5"},	/* Add New */
	{"kf5", "F4", "Esc-4"},	/* Allocate */
#if 0
	/* overridden by allocate */
	{"kf5", "F5", "Esc-5"},	/* Halt */
#endif
	{"kf5", "F5", "Esc-5"},	/* Cancel */
	{"kf5", "F5", "Esc-5"},	/* Exit */
	{"kf6", "F6", "Esc-6"},	/* Help */
	{"kf2", "F2", "Esc-2"},	/* Go To */
	{"kf3", "F3", "Esc-3"},	/* Top Level */
	{"kf3", "F3", "Esc-3"},	/* Topics */
	{"kf3", "F3", "Esc-3"},	/* Subjects */
	{"kf3", "F3", "Esc-3"},	/* How To */
	{"kf5", "F5", "Esc-5"},	/* Exit Help */
};

#define	MAXFUNC	(sizeof (f_keys) / sizeof (Fkey))

#define	BACK_KEYS	\
	(F_GOBACK | F_MAININDEX | F_TOPICS | F_HOWTO | F_REFER)

void
pfc_wfooter_fkeys_init(int force_alternates)
{
	char keystr[256];
	int i;

	/*
	 * Ugly, but the only semi-automated way of getting i18n'ed strings
	 * into the table such that xgettext can extract them.
	 */

	/* have libspmitty fill in the ones it knows about now */
	wfooter_fkeys_func_init(f_keys, 0);

	/* now fill in our own */
	f_keys[fkey_index(F_AUTO)].f_func = DESC_F_AUTO;
	f_keys[fkey_index(F_BEGIN)].f_func = DESC_F_BEGIN;
	f_keys[fkey_index(F_EXITINSTALL)].f_func = DESC_F_EXITINSTALL;
	f_keys[fkey_index(F_UPGRADE)].f_func = LABEL_UPGRADE_BUTTON;
	f_keys[fkey_index(F_GOBACK)].f_func = DESC_F_GOBACK;
	f_keys[fkey_index(F_TESTMOUNT)].f_func = DESC_F_TESTMOUNT;
	f_keys[fkey_index(F_PRESERVE)].f_func = DESC_F_PRESERVE;
	f_keys[fkey_index(F_REDOAUTO)].f_func = LABEL_REPEAT_AUTOLAYOUT;
	f_keys[fkey_index(F_SHOWEXPORTS)].f_func = DESC_F_SHOWEXPORTS;
	f_keys[fkey_index(F_CHANGE)].f_func = DESC_F_CHANGE;
	f_keys[fkey_index(F_CHANGETYPE)].f_func = DESC_F_CHANGETYPE;
	f_keys[fkey_index(F_DOREMOTES)].f_func = DESC_F_DOREMOTES;
	f_keys[fkey_index(F_CUSTOMIZE)].f_func = DESC_F_CUSTOMIZE;
	f_keys[fkey_index(F_EDIT)].f_func = DESC_F_EDIT;
	f_keys[fkey_index(F_CREATE)].f_func = DESC_F_CREATE;
	f_keys[fkey_index(F_OPTIONS)].f_func = DESC_F_OPTIONS;
	f_keys[fkey_index(F_INSTALL)].f_func = LABEL_INITIAL_BUTTON;
	f_keys[fkey_index(F_ALLOCATE)].f_func = DESC_F_ALLOCATE;
	f_keys[fkey_index(F_ADDNEW)].f_func = DESC_F_ADDNEW;
	f_keys[fkey_index(F_DELETE)].f_func = DESC_F_DELETE;
	f_keys[fkey_index(F_MANUAL)].f_func = DESC_F_MANUAL;

	/*
	 * Does terminal have all the appropriate "special" keys?
	 */
	for (i = 0; i < MAXFUNC; i++) {
		if (!force_alternates && tigetstr(f_keys[i].f_keycap)) {
			/* XXX need to gettext f_special */
			(void) sprintf(keystr, "%s_%s",
			    f_keys[i].f_special,
			    f_keys[i].f_func);
		} else {
			/* XXX need to gettext f_fallback */
			(void) sprintf(keystr, "%s_%s",
			    f_keys[i].f_fallback,
			    f_keys[i].f_func);
		}
		f_keys[i].f_label = xstrdup(keystr);
	}
}

/*
 * register the fkeys array and the fkeys initialization function with
 * the tty library
 */
void
pfc_fkeys_init(void)
{
	wfooter_fkeys_init(f_keys, MAXFUNC, pfc_wfooter_fkeys_init);
	fkey_wmenu_check_register(_fkeys_wmenu_check_func);
	fkey_notice_check_register(_fkeys_notice_check_func);
}

static
_fkeys_wmenu_check_func(u_long fkeys, int ch)
{

	if (((fkeys & BACK_KEYS) && is_goback(ch)) ||
		((fkeys & F_EDIT) && is_edit(ch)) ||
		((fkeys & F_CUSTOMIZE) && is_customize(ch)))
		return (1);

	return (0);
}

static
_fkeys_notice_check_func(u_long fkeys, int ch)
{

	if (is_goback(ch) && (fkeys & F_GOBACK))
		return (1);
	else if (is_manual(ch) && (fkeys & F_MANUAL))
		return (1);

	return (0);
}
