/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#ifndef lint
#ident	"@(#)space.c 1.8 93/04/15"
#endif

#include "defs.h"

static int add_module_space(Modinfo *info, caddr_t data);
static int add_instance_space(Node *np, caddr_t data);
static void pkg_space(Modinfo *info, Space **sp);

/*
 * Calculate the space required for a module
 * and its descendents.
 */
u_long
calc_total_space(Module *mod)
{
	Space	**spacetab;
	u_long	space;
	int	i;

	space = 0L;

	spacetab = calc_module_space(mod);
	for (i = 0; spacetab[i]; i++)
		space += spacetab[i]->bused;

	return (space);
}

Space **
calc_module_space(Module *mod)
{
	extern char *def_mnt_pnt[];
	static Space **spacetab;
	Node	dummy;
	int	i;

	if (spacetab == (Space **)0) {
		for (i = 0; def_mnt_pnt[i]; i++)
			;
		spacetab = xcalloc((sizeof (Space *)) * (i + 1));
		for (i = 0; def_mnt_pnt[i]; i++) {
			spacetab[i] = (Space *)xcalloc(sizeof (Space));
			spacetab[i]->mountp = xstrdup(def_mnt_pnt[i]);
		}
		spacetab[i] = (Space *)0;
	}

	for (i = 0; spacetab[i]; i++) {
		spacetab[i]->bused = spacetab[i]->fused = 0L;
	}

	switch (mod->type) {
	case PRODUCT:
		set_current(mod);
		(void) walklist(mod->info.prod->p_packages,
			add_instance_space, (caddr_t)spacetab);
		break;
	case PACKAGE:
		dummy.data = (void *)mod->info.mod;
		(void) add_instance_space(&dummy, (caddr_t)spacetab);
		break;
	case METACLUSTER:
	case CLUSTER:
		(void) walktree(mod, add_module_space, (caddr_t)spacetab);
		break;
	default:
		break;
	}
	return (spacetab);
}

/*
 * Add package space paramters to those
 * in the pointed-to space table
 */
static void
pkg_space(Modinfo *info, Space **sp)
{
	int	i;

	for (i = 0; sp[i]; i++)
		sp[i]->bused += info->m_deflt_fs[i];
}

u_long
get_fs_space(Space **sp, char *fs)
{
	int	i;

	for (i = 0; sp[i]; i++) {
		if (strcmp(sp[i]->mountp, fs) == 0)
			return (sp[i]->bused);
	}
	return (0L);
}

static int
add_module_space(Modinfo *info, caddr_t data)
{
	/*LINTED [alignment ok]*/
	Space	**sp = (Space **)data;
	Module	*prod;
	Arch	*arch;

	if (info->m_shared != NOTDUPLICATE &&
	    info->m_shared != SPOOLED_NOTDUP)
		return (SUCCESS);

	prod = get_current_product();

	if (prod != (Module *)0 &&
	    (arch = get_all_arches(prod)) != (Arch *)0) {
		/*
		 * Count packages supporting the selected
		 * architectures.  We need the product
		 * in order to get the list of supported
		 * or selected architectures.
		 */
		while (arch != (Arch *)0) {
			if (arch->a_selected) {
				if (info->m_arch == (char *)0 ||
				    supports_arch(arch->a_arch, info->m_arch) ||
				    strcmp(info->m_arch, "all") == 0 ||
				    strcmp(info->m_arch, "all.all") == 0) {
					pkg_space(info, sp);
					break;
				}
			}
			arch = arch->a_next;
		}
	} else	/* XXX count it? */
		pkg_space(info, sp);

	return (SUCCESS);
}

static int
add_instance_space(Node *np, caddr_t data)
{
	Modinfo *info = (Modinfo *)np->data;
	Modinfo *inst, *patch;

	for (inst = info; inst != (Modinfo *)0; inst = next_inst(inst)) {
		for (patch = inst;
		    patch != (Modinfo *)0;
		    patch = next_patch(patch))
			(void) add_module_space(patch, data);
	}
	return (SUCCESS);
}
