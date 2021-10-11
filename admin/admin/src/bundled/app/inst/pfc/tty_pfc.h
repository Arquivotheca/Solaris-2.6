#ifndef lint
#pragma ident "@(#)tty_pfc.h 1.6 96/06/17 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_pfc.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _TTY_PFC_H
#define	_TTY_PFC_H

#include "spmitty_api.h"
#include <curses.h>
#include <libintl.h>

/*
 * These should be defined in desired
 * presentation order.
 * Be careful to fit your defines here around the defines in
 * libspmitty w/o overlapping any defined there.
 */
#define	F_AUTO		0x4UL
#define	F_BEGIN		0x8UL
#define	F_EXITINSTALL	0x10UL
#define	F_UPGRADE	0x20UL
#define	F_GOBACK	0x40UL
#define	F_TESTMOUNT	0x80UL
#define	F_DELETE	0x100UL
#define	F_PRESERVE	0x200UL
#define	F_REDOAUTO	0x400UL
#define	F_SHOWEXPORTS	0x800UL
#define	F_CHANGE	0x1000UL
#define	F_CHANGETYPE	0x2000UL
#define	F_DOREMOTES	0x4000UL
#define	F_CUSTOMIZE	0x8000UL
#define	F_EDIT		0x10000UL
#define	F_CREATE	0x20000UL
#define	F_OPTIONS	0x40000UL
#define	F_INSTALL	0x80000UL
#define	F_MANUAL	0x100000UL
#define	F_ADDNEW	0x200000UL
/* this overrides F_HALT!! which is not used in the app now... */
#define	F_ALLOCATE	0x400000UL

/*
 * These can be defined in any order you want.
 * They are matched up to the appropriate entries
 * in the function key descriptor table by the
 * initialization code in wfooter().
 */
#define	DESC_F_AUTO		gettext("Auto Layout")
#define	DESC_F_BEGIN		gettext("Begin Installation")
#define	DESC_F_EXITINSTALL	gettext("Exit Installation")
#define	DESC_F_GOBACK		gettext("Go Back")
#define	DESC_F_TESTMOUNT	gettext("Test Mount")
#define	DESC_F_ADDNEW		gettext("New")
#define	DESC_F_PRESERVE		gettext("Preserve")
#define	DESC_F_SHOWEXPORTS	gettext("Show Exports")
#define	DESC_F_CHANGE		gettext("Change")
#define	DESC_F_CHANGETYPE	gettext("Change Type")
#define	DESC_F_DOREMOTES	gettext("Remote Mounts")
#define	DESC_F_CUSTOMIZE	gettext("Customize")
#define	DESC_F_ALLOCATE		gettext("Allocate")
#define	DESC_F_EDIT		gettext("Edit")
#define	DESC_F_CREATE 		gettext("Create")
#define	DESC_F_OPTIONS		gettext("Options")
#define	DESC_F_MANUAL		gettext("Manual Layout")
#define	DESC_F_DELETE		gettext("Delete")

#define	is_auto(c)		((c) == KEY_F(2))
#define	is_index(c)		((c) == KEY_F(2))
#define	is_begin(c)		((c) == KEY_F(2))
#define	is_exitinstall(c)	((c) == KEY_F(2))
#define	is_upgrade(c)		((c) == KEY_F(2))

#define	is_goback(c)		((c) == KEY_F(3))
#define	is_testmount(c)		((c) == KEY_F(3))
#define	is_delete(c)		((c) == KEY_F(3))

#define	is_preserve(c)		((c) == KEY_F(4))
#define	is_showexports(c)	((c) == KEY_F(4))
#define	is_change(c)		((c) == KEY_F(4))
#define	is_changetype(c)	((c) == KEY_F(4))
#define	is_doremotes(c)		((c) == KEY_F(4))
#define	is_customize(c)		((c) == KEY_F(4))
#define	is_allocate(c)		((c) == KEY_F(4))
#define	is_edit(c)		((c) == KEY_F(4))
#define	is_create(c)		((c) == KEY_F(4))
#define	is_options(c)		((c) == KEY_F(4))
#define	is_manual(c)		((c) == KEY_F(4))
#define	is_redoauto(c)		((c) == KEY_F(4))
#define	is_install(c)		((c) == KEY_F(4))

#define	is_addnew(c)		((c) == KEY_F(5))

#ifdef __cplusplus
extern	"C" {
#endif

extern void pfc_fkeys_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* _TTY_PFC_H */
