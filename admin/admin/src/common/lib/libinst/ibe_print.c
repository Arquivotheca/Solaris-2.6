#ifndef lint
#pragma ident "@(#)ibe_print.c 1.25 94/09/20"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "disk_lib.h"
#include "ibe_lib.h"
#include <fcntl.h>

/* Local Statics */

static ModStatus	cur_stat;
static short		have_one;
static char		product[32];

/* Public Function Prototypes */

/* Library Function Prototypes */

void		_print_results(Module *);

/* Local Function Prototypes */

static int 	_pkg_status(Node *, caddr_t);

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _print_results()
 *	Walk through the linked list of products. Walk the package
 *	chain for each product and print out the names of those
 *	packages which have an INSTALL_SUCCESS status. Thne walk
 *	through the chain and print out then names of those packages
 *	which have an INSTALL_FAILED status (partials).
 * Parameters:
 *	prod	  - pointer to the head of the product list to be printed
 * Return:
 *	none
 * Status:
 *	public
 */
void
_print_results(Module *prod)
{
	Module	*t;

	for (t = prod; t != NULL; t = t->next) {
		(void) sprintf(product, "%s %s",
			t->info.prod->p_name, t->info.prod->p_version);

		/* look for all packages with a successful install status */

		have_one = 0;
		cur_stat = INSTALL_SUCCESS;
		(void) walklist(t->info.prod->p_packages, _pkg_status,
				(caddr_t) NULL);
		if (have_one == 0)
			write_status(LOG, LEVEL2, NONE_STRING);

		/* look for all packages with an unsuccessful install status */
		have_one = 0;
		cur_stat = INSTALL_FAILED;
		(void) walklist(t->info.prod->p_packages, _pkg_status,
				(caddr_t) NULL);
	}
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */

/*
 * _pkg_status()
 *	Function used in walklist() to print the status of the node.
 * Parameters:
 *	np	- node pointer to current node being processed
 *	dummy	- required parameter for walklist, but not used here
 * Return:
 *	0	- always returns this value
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
_pkg_status(Node * np, caddr_t dummy)
{
	Modinfo * 	mp;
	u_char		log;

	mp = (Modinfo *) np->data;

	/* log successful packages only if debugging is turned on */
	log = (get_install_debug() == 0 ? LOG : LOGSCR);

	if (mp->m_status == cur_stat) {
		if (cur_stat == INSTALL_SUCCESS) {
			if (have_one == 0) {
				write_status(log, LEVEL0,
					PKGS_FULLY_INSTALLED, product);
			}
			write_status(log, LEVEL2, mp->m_pkgid);
		} else if (cur_stat == INSTALL_FAILED) {
			if (have_one == 0) {
				write_status(LOGSCR, LEVEL0,
					PKGS_PART_INSTALLED, product);
			}
			write_status(LOGSCR, LEVEL2, mp->m_pkgid);
		}
		have_one++;
	}
	return (0);
}
