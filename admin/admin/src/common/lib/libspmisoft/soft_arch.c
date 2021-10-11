#ifndef lint
#pragma ident "@(#)soft_arch.c 1.2 96/04/30 SMI"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "spmisoft_lib.h"
#include <sys/systeminfo.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Local Statics */

static char	default_impl[ARCH_LENGTH] = "";
static char	default_architecture[(2 * ARCH_LENGTH + 1)] = "";

/* Public Function Prototypes */

Arch *  	swi_get_all_arches(Module *);
int		swi_package_selected(Node *, char *);
char *		swi_get_default_arch(void);
char *  	swi_get_default_impl(void);
int		swi_select_arch(Module *, char *);
int		swi_deselect_arch(Module *, char *);
void    	swi_mark_arch(Module *);
int		swi_valid_arch(Module *, char *);

/* Library Function Prototypes */

int		supports_arch(char *, char *);
void		expand_arch(Modinfo *);
void		add_arch(Module *, char *);
void		add_package(Module *, Modinfo *);
int		supports_arch(char *, char *);
void		add_4x(Module *, Modinfo *);
int		media_supports_arch(Product *, char *);
void		extract_isa(char *, char *);
int		media_supports_isa(Product *, char *);
int		fullarch_is_selected(Product *, char *);
int		fullarch_is_loaded(Product *, char *);
int		isa_is_selected(Product *, char *);
int		isa_is_loaded(Product *, char *);
int		arch_is_selected(Product *, char *);
int		isa_of_arch_is(char *, char *);
int		_arch_cmp(char *, char *, char *);

/* Local Function Prototypes */

static int	plat_ident(Platform *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * get_all_arches()
 *	Return a pointer to the head of a list of architectures
 *	supported by a product.
 * Parameters:
 *	mod	- pointer to product module (NULL if current product is to
 *		  be used)
 * Return:
 *	NULL	- 'mod' is NULL and there is no current product
 *	Arch *	- pointer to head of architecture list associated with either
 *		  the 'mod' product, or the current product if 'mod' is NULL
 */
Arch	*
swi_get_all_arches(Module *mod)
{
	Module		*prod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_all_arches");
#endif

	if (mod == NULL) {
		if ((prod = get_current_product()) == NULL)
			return (NULL);
	} else
		prod = mod;
	return (prod->info.prod->p_arches);
}

/*
 * package_selected()
 *	Is package selected. (use the total number of packages selected to
 *	derive an estimate of time to complete installation.)
 * Parameters:
 *	np	-
 *	foo	-
 * Return:
 * Status:
 *	public
 */
/*ARGSUSED1*/
int
swi_package_selected(Node * np, char * foo)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("package_selected");
#endif


#define		n_data	((Modinfo *) np->data)

	if (n_data->m_status == SELECTED || n_data->m_status == REQUIRED)
		if (get_machinetype() == MT_DATALESS)
			if (n_data->m_sunw_ptype == PTYPE_ROOT)
				/* only count root pacakges for dataless */
				return (1);
			else
				return (0);
		else
			return (1);
	else
		return (0);
#undef n_data
}

/*
 * swi_get_default_arch()
 *	Get the default architecture which corresponds to the local
 *	system. Returned in the form <instance>.<implementation>
 *	(e.g. sparc.sun4c, sparc.sun4m).
 * Parameters:
 *	none
 * Return:
 *	NULL	- the nodename, the architecture, or the machine name
 *		  returned by sysinfo() were too long for local variables
 *	char *	- name of architecture
 * Status:
 *	public
 */
char  *
swi_get_default_arch(void)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_arch");
#endif

	if (default_architecture[0] == '\0') {
		if (default_impl[0] == '\0')
			(void) get_default_impl();
		(void) sprintf(default_architecture, "%s.%s",
		    get_default_inst(), default_impl);
	}
	return (default_architecture);
}

/*
 * swi_get_default_impl()
 *	Returns the default implemtation architecture of the machine it
 *	is executed on. (eg. sun4c, sun4m, ...)
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to a string containing the default architecture
 * Status:
 *	public
 */
char *
swi_get_default_impl(void)
{
	PlatGroup	*pgp;
	Platform	*pp;
	Module		*mod;
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_impl");
#endif
	get_actual_platform();
	if (*(get_actual_platform()) != '\0') {
		for (mod = get_media_head(); mod != NULL; mod = mod->next)
			if (mod->info.media->med_type != INSTALLED &&
			    mod->info.media->med_type != INSTALLED_SVC)
				break;

		if (mod && mod->sub && mod->sub->info.prod)
			for (pgp = mod->sub->info.prod->p_platgrp; pgp;
			    pgp = pgp->next)
				for (pp = pgp->pltgrp_members; pp;
				    pp = pp->next)
					if (plat_ident(pp)) {
						(void) strcpy(default_impl,
						    pgp->pltgrp_name);
						return (default_impl);
					}

		mod = get_localmedia();

		if (mod && mod->sub && mod->sub->info.prod)
			for (pgp = mod->sub->info.prod->p_platgrp; pgp;
			    pgp = pgp->next)
				for (pp = pgp->pltgrp_members; pp;
				    pp = pp->next)
					if (plat_ident(pp)) {
						(void) strcpy(default_impl,
						    pgp->pltgrp_name);
						return (default_impl);
					}

	}

	/*
	 *  If we've made it this far, we either couldn't find the
	 *  local platform type (pre-KBI environment), or we couldn't
	 *  map it.  Assume that the platform group is the machine type.
	 */
	(void) strcpy(default_impl, get_default_machine());
	return (default_impl);
}

/*
 * select_arch()
 *	Scan the architecture list of 'prod'. For each architecture which
 *	matches 'arch', the selected state is set to TRUE. If 'arch' applies to
 *	more than one of the product architectures, all affected architectures
 *	are set (e.g. arch = sparc may set sparc.sun4c and sparc.sun4m).
 * Parameters:
 *	prod	- pointer to product structure containing architecture chain
 *	arch	- string representing architecture set to be marked TRUE
 * Return:
 *	SUCCESS	     - 'arch' made at least one match in the product
 *			architecture chain
 *	ERR_BADARCH  - 'arch' was not found in the product architecture chain
 * Status:
 *	public
 */
int
swi_select_arch(Module * prod, char * arch)
{
	Arch	*a;
	int	match;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("select_arch");
#endif

	for (match = 0, a = get_all_arches(prod); a != NULL; a = a->a_next) {
		if (supports_arch(a->a_arch, arch) == TRUE) {
			a->a_selected = TRUE;
			match++;
		}
	}
	if (match == 0)
		return (ERR_BADARCH);
	else
		return (SUCCESS);
}

/*
 * valid_arch()
 *	Scan the architecture list of 'prod' to determine if the architecture
 *	specified is valid for the given product. If 'prod' is NULL, then
 *	the current product is searched.
 * Parameters:
 *	prod	- pointer to product structure containing architecture chain
 *		  (NULL if current product is to be used)
 *	arch	- string representing architecture set to be marked TRUE
 * Return:
 *	SUCCESS	     - 'arch' made at least one match in the product
 *			architecture chain
 *	ERR_BADARCH  - 'arch' was not found in the product architecture chain
 * Status:
 *	public
 */
int
swi_valid_arch(Module *prod, char *arch)
{
	Arch	*a;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("valid_arch");
#endif
	if (prod == (Module *)NULL) {
		if ((prod = get_current_product()) == (Module *)NULL)
			return (ERR_BADARCH);
	}

	for (a = get_all_arches(prod); a != NULL; a = a->a_next) {
		if (supports_arch(a->a_arch, arch) == TRUE)
			return (SUCCESS);
	}

	return (ERR_BADARCH);
}

/*
 * deselect_arch()
 *	'prod' specifies the product for which the architecture should be
 *	deselected. 'arch' represents the architecture which should be
 *	deselected.  If 'arch' can refer to multiple architectures (eg.
 *	arch = sparc), then all architecture which match (eg. sparc.sun4c)
 *	are deselected.
 * Parameters:
 *	prod	- pointer to product module containing architecture list
 *	arch	- string specifying architecture to deselect (e.g. sparc
 *		  or sparc.sun4c)
 * Return:
 *	SUCCESS	    - 'arch' matched at least once in 'prod's architecture
 *			list
 *	ERR_BADARCH - 'prod' does not support the architecture 'arch'
 * Status:
 *	public
 */
int
swi_deselect_arch(Module * prod, char * arch)
{
	Arch	*a;
	int	match;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("deselect_arch");
#endif

	for (match = 0, a = get_all_arches(prod); a != NULL; a = a->a_next) {
		if (supports_arch(a->a_arch, arch) == TRUE) {
			a->a_selected = FALSE;
			match++;
		}
	}
	if (match == 0)
		return (ERR_BADARCH);
	else
		return (SUCCESS);
}

/*
 * mark_arch()
 * 	Walks the package list associated with the product "prod",
 *	selecting or deselecting architecture specific packages based on
 *	selected architectures and whether the package associated with
 *	the default architecture is selected/deselected. Should be
 *	called after selecting (select_arch()) or deselecting
 *	(deselect_arch()) architectures.
 * Parameters:
 *	prod	- pointer to product module for which the architectures
 *		  are being marked
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_mark_arch(Module * prod)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mark_arch");
#endif

	walklist(prod->info.prod->p_packages, update_selected_arch,
	    (caddr_t) prod);
}

/*
 * supports_arch()
 *	Determine if the machine architecture 'macharch' is a member
 *	of the architecture set defined by the package ('pkgarch').
 *	NOTE:   The current implementation does not consider package
 *		architecture "strings" (comma separated lists) or NULL
 *		machine architectures to be valid.
 * Parameters:
 * 	macharch - single architecture specifier of the form:
 *			<instance>.<implementation>
 *			(e.g. sparc.sun4c)
 *	pkgarch  - package architecture specifier of the form:
 *			<instance>.<implementation>
 *			<instance>.all
 *			all
 *			all.all
 *			(e.g.  sparc.sun4)
 * Return:
 *	TRUE	- 'macharch' is a member of 'pkgarch'
 *	FALSE	- 'macharch' is not a member of 'pkgarch', or
 *		  'macharch' is NULL
 * Status:
 *	Public
 */
int
supports_arch(char * macharch, char * pkgarch)
{
	char *cp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("supports_arch");
#endif

	if (!strcmp(pkgarch, "all") ||
			!strcmp(pkgarch, "all.all") ||
			!strcmp(macharch, pkgarch))
		return (TRUE);

	if (!strchr(pkgarch, ARCH_DELIMITER)) {
		cp = strchr(macharch, ARCH_SEPARATOR);
		*cp = '\0';
		if (!strcmp(macharch, pkgarch)) {
			*cp = ARCH_SEPARATOR;
			return (TRUE);
		}
		*cp++ = ARCH_SEPARATOR;
		if (!strcmp(cp, pkgarch))
			return (TRUE);
	}
	return (FALSE);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * update_selected_arch()
 *
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	semi-private (internal library use only)
 */
int
update_selected_arch(Node * np, caddr_t data)
{
	Modinfo	*i, *k;
	Arch 	*a;

	for (i = (Modinfo*)np->data; i != NULL; i = next_inst(i)) {
		/*LINTED [alignment ok]*/
		for (a = get_all_arches((Module*)data); a != NULL;
							a = a->a_next) {
			if (i->m_shared == NULLPKG)
				continue;

			if (supports_arch(get_default_arch(), i->m_arch)) {
				for (k = i; k != NULL; k = next_patch(k)) {
					k->m_status =
						((Modinfo*)np->data)->m_status;
				}
				break;
			} else if (supports_arch(a->a_arch, i->m_arch)) {
				for (k = i; k != NULL; k = next_patch(k)) {
					if (a->a_selected &&
						((Modinfo*)np->data)->m_status){
						if (k->m_status != REQUIRED)
							k->m_status = SELECTED;
					} else {
						k->m_status = UNSELECTED;
					}
				}
				/*
				 *  The reason we only break if the arch
				 *  was selected is as follows:  if the
				 *  package being examined is karch-specific,
				 *  looking at the rest of the architectures
				 *  is harmless because supports_arch will
				 *  fail on all of them, so we'll never
				 *  get into this block.  If the package
				 *  is karch-neutral, supports_arch will
				 *  succeed for each architecture whose
				 *  ISA matches the package's.  As soon we
				 *  find an arch that is selected, we quit,
				 *  because that means the package should
				 *  be selected, and we don't want it
				 *  unselected by looking at a subsequent
				 *  unselected karch.
				 */
				if (a->a_selected)
					break;
			}
		}
	}
	return (SUCCESS);
}

/*
 * add_4x()
 *	Add 4x software to list.
 * Parameters:
 *	prod	-
 *	info	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
add_4x(Module * prod, Modinfo * info)
{
	Node		*np;

	np = getnode();			/* new list node */
	np->key = xstrdup(info->m_pkgid);	/* make pkgid key */
	np->data = (char *) info;	/* put Modinfo into node */
	np->delproc = &free_np_modinfo; /* set delete function */
	(void) addnode(prod->info.prod->p_sw_4x, np);
	return;
}

/*
 * expand_arch()
 *
 * Parameters:
 *	info	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
expand_arch(Modinfo * info)
{
	char		*cp, *cp1;
	char		*buf = NULL;

	/* parse each architecture out, break tokens at ARCH_DELIMITER */
	cp = info->m_arch;
	do {
		if (cp1 = strchr(cp, ARCH_DELIMITER))
			*cp1 = '\0';	/* zap ARCH_DELIMITER */

		/* look for unique isa.impl strings */
		if (buf == NULL) {
			buf = (char *)xcalloc(sizeof (char)*MAXPATHLEN);
			strcpy(buf, cp);
		} else
			(void) strcat(buf, cp);
		if (strchr(cp, ARCH_SEPARATOR) == NULL) {
			(void) strcat(buf, ".all");
		}
		/* cp1 non-NULL -> more tokens */
		if (cp1) {
			(void) strcat(buf, ","); /* add delimiter to new str */
			*cp1 = ',';		/* put delimiter back */
			while (*++cp1 == ' ' || *cp1 == '\t')
				;

			cp = cp1;
		}
	} while (cp1);

	if (buf != NULL) {
		info->m_expand_arch = xstrdup(buf);
		free(buf);
	}
	return;
}

int
media_supports_arch(Product *prodinfo, char *arch)
{
	Arch	*ap;

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next)
		if (strcmp(ap->a_arch, arch) == 0)
			return (1);
	return (0);
}

int
media_supports_isa(Product *prodinfo, char *isa)
{
	Arch	*ap;
	char	isabuf[ARCH_LENGTH];

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next) {
		extract_isa(ap->a_arch, isabuf);
		if (strcmp(isabuf, isa) == 0)
			return (1);
	}
	return (0);
}

int
fullarch_is_selected(Product *prodinfo, char *arch)
{
	Arch	*ap;

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next)
		if (strcmp(ap->a_arch, arch) == 0) {
			if (ap->a_selected)
				return (1);
			else
				return (0);
		}

	return (0);
}

int
fullarch_is_loaded(Product *prodinfo, char *arch)
{
	Arch	*ap;

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next)
		if (strcmp(ap->a_arch, arch) == 0) {
			if (ap->a_loaded)
				return (1);
			else
				return (0);
		}

	return (0);
}

int
isa_is_selected(Product *prodinfo, char *isa)
{
	Arch	*ap;
	char	isabuf[ARCH_LENGTH];

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next) {
		extract_isa(ap->a_arch, isabuf);
		if (strcmp(isabuf, isa) == 0 && ap->a_selected)
			return (1);
	}
	return (0);
}

int
arch_is_selected(Product *prodinfo, char *arch)
{
	char	*cp;

	cp = (char *)strchr(arch, '.');
	if (cp != NULL)
		return (fullarch_is_selected(prodinfo, arch));
	else
		return (isa_is_selected(prodinfo, arch));
}

int
isa_is_loaded(Product *prodinfo, char *isa)
{
	Arch	*ap;
	char	isabuf[ARCH_LENGTH];

	for (ap = prodinfo->p_arches; ap; ap = ap->a_next) {
		extract_isa(ap->a_arch, isabuf);
		if (strcmp(isabuf, isa) == 0 && ap->a_loaded)
			return (1);
	}
	return (0);
}

int
isa_of_arch_is(char *arch, char *isa)
{
	char	isabuf[ARCH_LENGTH];

	extract_isa(arch, isabuf);
	if (strcmp(isabuf, isa) == 0)
		return (TRUE);
	else
		return (FALSE);
}

/*
 * extract instruction set architecture field from full architecture
 * (example: "sparc.sun4c") and copy it to the buffer pointed by "isa".
 */
void
extract_isa(char *fullarch, char *isa)
{
	char	*cp;
	int	len;

	cp = (char *)strchr(fullarch, '.');
	if (cp != NULL)
		len = cp - fullarch;
	else
		len = strlen(fullarch);
	(void) strncpy(isa, fullarch, len);
	isa[len] = '\0';
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * add_arch()
 *	Cope with the architecture string. Need to track all unique
 *	architecture specifications. reducing the architectures to those
 *	in <isa.impl> form. This derivation tells us which architectures
 *	to offer in the list of possible `client architectures' for
 *	servers... the possible architectures are:
 *
 *		sparc.sun4
 *		sparc.sun4c   campus
 *		sparc.sun4d   Dragon
 *		sparc.sun4e   4e (6U vme cards)
 *		sparc.sun4m   galaxy/c2
 *
 *	what we'll see is stuff like this:
 *
 *		<>:		any
 *		all:		any
 *		sparc:		all sparc
 *		sparc.all:	all sparc
 *		sparc.sun4c:	sparc.sun4c only
 *		none:		not supported any more
 * Parameters:
 *	prod	-
 *	archstr -
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
add_arch(Module * prod, char * archstr)
{
	char		*cp, *cp1, *cp2;
	Arch		*ap, *last_arch;

	if (prod->type != PRODUCT && prod->type != NULLPRODUCT)
		return;
	/*
	 * parse each architecture out, break tokens at ARCH_DELIMITER
	 */
	cp = archstr;
	do
	{
		if (cp1 = strchr(cp, ARCH_DELIMITER))
			*cp1 = '\0';	/* zap ARCH_DELIMITER */

		/*
		 * look for unique isa.impl strings
		 */
		if ((cp2 = strchr(cp, ARCH_SEPARATOR)) != NULL &&
					(strcmp(cp2, ".all") != 0)) {
			for (ap = prod->info.prod->p_arches; ap; ap = ap->a_next)
				if (strcmp(cp, ap->a_arch) == 0)
					break;	/* found a duplicate */
				else
					last_arch = ap;

			/* need to add this architecture to the list? */
			if (ap == (Arch *) NULL) {
				ap = (Arch *) xcalloc(sizeof (Arch));
				ap->a_arch = (char *)xstrdup(cp);
				ap->a_selected = 0;
				ap->a_loaded = 0;
				ap->a_next = NULL;

				if (prod->info.prod->p_arches == (Arch *) NULL)
					prod->info.prod->p_arches = ap;
				else if (last_arch)
					last_arch->a_next = ap;	/* add to end */
			}
		}
		/*
		 * cp1 non-NULL -> more tokens
		 */
		if (cp1) {
			*cp1 = ',';		/* put delimiter back */
			while (*++cp1 == ' ' || *cp1 == '\t')
				;

			cp = cp1;
		}
	} while (cp1);

	return;
}

/*
 * add_package()
 *	Add package to list of packages associated with a product. Deal with
 *	duplicates by assuming that they are architecture specific instances
 *	of the same package.  Keep duplicates on separate list
 * Parameters:
 *	prod	-
 *	info	- package modinfo structure being added to product hash list
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
add_package(Module * prod, Modinfo * info)
{
	Node		*np, *tn;
	Modinfo		*infop, *prev_patch;

	/*
	 * allocate a new node, install the pkgid as the key, link
	 * the Modinfo structure as the data component of the node,
	 * set the delete function
	 */
	np = getnode();
	np->key = xstrdup(info->m_pkgid);
	np->data = (char *) info;
	np->delproc = &free_np_modinfo;

	/* deal with patches and finding what they patch */

	if (is_patch(info) == TRUE) {
		if ((tn = findnode(prod->info.prod->p_packages,
					info->m_pkgid)) == NULL)
			infop = NULL;
		else
			infop = (Modinfo*)tn->data;

		for (; infop != NULL; infop = next_inst(infop)) {
			if (is_patch_of(info, infop) == TRUE) {
				info->m_next_patch = infop->m_next_patch;
				infop->m_next_patch = np;
				info->m_patchof = infop;
				break;
			}
		}
		if (infop == NULL) {
			info->m_next_patch = prod->info.prod->p_orphan_patch;
			prod->info.prod->p_orphan_patch = np;
			return;
		}
		return;
	} else {
		/* check for 'orphan patches' that patch this */
		prev_patch = NULL;
		for (tn = prod->info.prod->p_orphan_patch; tn != NULL; ) {
			infop = (Modinfo *)tn->data;

			if (is_patch_of(infop, info) == TRUE) {
				if (prev_patch != NULL) {
					prev_patch->m_next_patch =
							infop->m_next_patch;
				} else {
					prod->info.prod->p_orphan_patch =
							infop->m_next_patch;
				}
				infop->m_patchof = info;
				infop->m_next_patch = info->m_next_patch;
				info->m_next_patch = tn;
				if (prev_patch != NULL) {
					tn = prev_patch->m_next_patch;
				} else {
					tn = prod->info.prod->p_orphan_patch;
				}

			} else {
				prev_patch = infop;
				tn = prev_patch->m_next_patch;
			}
		}
	}


	if (addnode(prod->info.prod->p_packages, np) == -1) {
		/* duplicate package. */
		tn = findnode(prod->info.prod->p_packages, info->m_pkgid);

		if (((Modinfo *)np->data)->m_shared == NULLPKG) {
			infop = (Modinfo *)tn->data;
			tn->data = (char *)np->data;
			np->data = (char *)infop;
			delnode(np);
			return;
		}

		if (((Modinfo*)tn->data)->m_pkginst != NULL) {
			if ((((Modinfo*)np->data)->m_pkginst != NULL) &&
			    (strcmp(((Modinfo*)tn->data)->m_pkginst,
					((Modinfo*)np->data)->m_pkginst) > 0)) {
				infop = (Modinfo *)tn->data;
				tn->data = (char *)np->data;
				np->data = (char *)infop;
			}
		} else if (strcmp(get_default_arch(), info->m_arch) == 0) {
			infop = (Modinfo *)tn->data;
			tn->data = (char *)np->data;
			np->data = (char *)infop;
		}

		if ((((Modinfo *)tn->data)->m_shared == SPOOLED_DUP) ||
		    (((Modinfo *)tn->data)->m_shared == SPOOLED_NOTDUP)) {
			if ((((Modinfo *)np->data)->m_shared != SPOOLED_DUP) ||
			    (((Modinfo *)np->data)->m_shared != SPOOLED_NOTDUP)) {
				infop = (Modinfo *)tn->data;
				tn->data = (char *)np->data;
				np->data = (char *)infop;
			}
		}
		/*
		 * np points to the duplicate (secondary) modules.
		 * add this to the list of package instances
		 */

		if (((Modinfo *)tn->data)->m_instances == (Node *) NULL)
			((Modinfo *) tn->data)->m_instances = np;
		else
		{
			for (tn = ((Modinfo *) tn->data)->m_instances;
			    ((Modinfo *)tn->data)->m_instances;
			    tn = ((Modinfo *)tn->data)->m_instances);
				;
			((Modinfo*)tn->data)->m_instances = np;
		}
	}
}


/*
 * Function:	_arch_cmp
 * Description:	Compare the architecture of the current machine to that
 *		listed in the package to determine if the specified
 *		package applies to this machine
 * Scope:	internal
 * Parameters:	arch	- m_arch field listed in the package (e.g. sparc)
 *		impl	- machine hardware implementation
 *		inst	- instruction set architecture
 * Return:	TRUE	- the package does apply to this system
 *		FALSE	- the package does not apply to this system
 */
int
_arch_cmp(char *arch, char *impl, char *inst)
{
	char   inst_impl[MAXNAMELEN];
	char   inst_all[MAXNAMELEN];
	char   *cp;

	(void) sprintf(inst_impl, "%s.%s", inst, impl);
	(void) sprintf(inst_all, "%s.all", inst);

	/* truncate arch to contain a single architecture */
	if (cp = strchr(arch, ','))
		*cp = '\0';

	/* exact match of architectures */
	if (strcmp(arch, inst_impl) == 0) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* all.all or just all */
	if ((strcmp(arch, "all.all") == 0) ||
			(strcmp(arch, "all") == 0)) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* arch.all or just arch */
	if ((strcmp(arch, inst_all) == 0) ||
			(strcmp(arch, inst) == 0)) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* no architecture specified, all assumed */
	if (strcmp(arch, " ") == 0) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* Put architecture string back to original value */
	if (cp)
		*cp++ = ',';

	return (FALSE);
}

static int
plat_ident(Platform *pp)
{
	if ((pp->plat_uname_id &&
	    strcmp(pp->plat_uname_id, get_actual_platform()) == 0 &&
	    (pp->plat_machine == NULL ||
	    strcmp(pp->plat_machine, get_default_machine()) == 0)) ||
	    pp->plat_uname_id == NULL &&
	    strcmp(pp->plat_machine, get_default_machine()) == 0)
		return (1);
	else
		return (0);
}

