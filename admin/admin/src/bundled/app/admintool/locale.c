/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)locale.c	1.5 95/08/28 Sun Microsystems"

/*	locale.c 	*/

#include "software.h"

Module * 
get_locale_list(Module * m)
{
	Module * p;

	p = m;
	while (p->type != PRODUCT && p->type != NULLPRODUCT)
		p = p->parent;

	return(p->info.prod->p_locale);
}

/* 
 * For a given module return True if it localized, ie there
 * are pkgs that need to be installed for any locale.
 * 
 * For aggregate modules, call recursively.
 */
Boolean
is_localized(Module * m)
{
	Module * s;

	if (m->type == PACKAGE)
		return(m->info.mod->m_l10n ? True : False);
	
	if (m->type == PRODUCT || m->type == NULLPRODUCT)  
		return(m->info.prod->p_locale ? True : False);

	if (m->type == CLUSTER || m->type == METACLUSTER ) {
		Boolean rc = False;
		s = m->sub;
		do {
		 	rc |= is_localized(s);

		} while (s = get_next(s));
		return(rc);
	}
}
		
/*
 * Return the L10N list associated with a module. If the
 * module is a CLUSTER or METACLUSTER, travserse sub-list
 * and return L10N list of first package. 
 * Assumption: all pkgs within a cluster are localized for
 * the same set of L10Ns.
 */ 
L10N * 
getL10Ns(Module * m)
{
    Module * s;
    L10N * l = NULL;
   
    if (m->type == CLUSTER || m->type == METACLUSTER) {
	s = get_sub(m);
	while (s) {	
	    if (l = getL10Ns(s))
		return(l);
	    s = get_next(s);
	}
	return(l);
    }
    else if (m->type == PACKAGE) 
	return(m->info.mod->m_l10n);
    else 
	return(l);
}
