#ifndef lint
#pragma ident "@(#)v_sw.c 1.91 96/07/09 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_sw.c
 * Group:	ttinstall
 * Description:
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <libintl.h>

#include "pf.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_sw.h"
#include "v_lfs.h"
#include "v_upgrade.h"

/*
 * This file contains the View interface layer to the software library.
 * (libsw.a).
 *
 * It implements and exposes an abstraction of the underlying software
 * functionality that serves to insulate the UI code from specifics.
 * It also provides the user-level model of how software is installed.
 *
 * The paradigm/model implemented is a hierachy:
 * 	the top level consists of Products
 * 	the second level consists of meta-clusters
 * 	the 3rd through nth levels consist of clusters and/or packages.
 *
 * The first and second levels may have at most 1 selected element.
 *
 * a `module' may be a cluster or a package.
 * a module has at most two siblings: next and previous.
 * a module may have a parent.
 * a module may have zero or more children.
 * a module has properties (instance variables...):
 *	name
 *	id
 *	description
 *	vendor
 *	version
 *	supported architectures
 *	size (not applicable for Products and Meta-clusters?)
 *	basedir (only meaningful for packages and clusters?)
 *	status (SELECTED, UNSELECTED, REQUIRED)
 *
 * navigating the module hierarchy, metaclusters and clusters may be
 *	expanded, which shows their components.
 *
 *
 * The sw library provides access to the various localizations
 * that are supported by a `product'.
 *
 * The abstraction we use is a simple list of localizations, where
 * a localization consists of:
 *	a locale name
 *	the language for that locale
 *
 * A localization may be:
 *	selected or unselected for installation
 *
 * The sw library provides access to the various architectures
 * that are supported by a `product'.
 *
 * The abstraction we use is a simple list of architectures, where
 * an architecture consists of:
 *	a architecture name
 *
 * An architecture may be:
 *	selected or unselected for installation
 *
 */

/* typedefs and defines */

/* Static Globals: */
static int _current_arch_index = 0;
static int _current_locale_index = 0;
static Module *_current_metaclst = (Module *) NULL;

static Module *_curmod = (Module *) NULL;

static int _curindex = -1;

/* Forward declarations: */
static Module *_v_get_current_product(void);
static Module *_v_int_get_ith_mod(int);
static int _v_int_get_kbytes(FSspace **, char *);
static int _v_int_count_mods(void);
static int _v_int_set_module_status(Module *);
static char *_v_int_get_module_size(Module *);

/* stuff to track & manipulate custimizations to metaclusters */
static int _find_any_delta(Module *);
static void _clear_all_deltas(Module *);
static int _count_all_deltas(Module *, unsigned int);
static void _recover_state_from_deltas(Module *);
int v_get_n_metaclst_deltas(int);
int v_get_delta_type(int);
int v_metaclst_edited(void);
char *v_get_delta_package_name(int);

/* initialize software hierarchy. */
int
v_init_sw(char *dir)
{
	static int first = TRUE;
	Module *media;
	Module *prod;

	if (first) {

		if (media = add_media(dir))
			if (load_media(media, TRUE) != SUCCESS)
				return (V_FAILURE);

		first = FALSE;

		/* assume that there is only one product... */
		prod = get_current_product();

		/*
		 * this inits all the module data structure fields that I
		 * really don't care about, but that the underlying libraries
		 * do...
		 */
		set_instdir_svc_svr(prod);

	} else {
		/* assume that there is only one product... */
		prod = get_current_product();
	}

	/*
	 * call to set_percent_free_space gives us the fudge for free file
	 * system space.
	 * The space numbers calculated by the space code in the sw library
	 * are off a bit, need a larger fudge for now.
	 */

	/* 15% free fs space */
	sw_lib_init(NULL);
	set_percent_free_space(15);
	set_memalloc_failure_func(v_int_error_exit);

	/*
	 * make first level of sw hierarchy the product's sub-modules if
	 * they exist
	 */
	if (prod->sub)
		v_set_swhier_root(prod->sub);
	else
		v_set_swhier_root(prod);

	return (V_OK);

}

void
v_restore_default_view()
{
	set_instdir_svc_svr(get_current_product());
	load_default_view(get_current_product());

}


typedef struct {
	double original;
	double current;
} Metaclst_size_t;

static Metaclst_size_t *metaclst_sizes = (Metaclst_size_t *) NULL;

/*
 * this will compute the disk space required to install the default system
 * configurations with each of the metaclusters.
 *
 * the default sizes are calculated based on the current system type, and any
 * parameters unique to it (# of clients, etc).
 *
 * this `single-number-metric' is of dubious value, but people seem to want
 * some indication of how much space it will take to install a metaclusters,
 * regardless of the other variables which affect the space requirements
 *
 * this function selects & unselects each metacluster in order to calculate its
 * default size.  It relies on the `deltas' that are marked as modules are
 * selected and unselected to restore the customized state of the current
 * metacluster.
 *
 */
void
v_set_metaclst_dflt_sizes()
{
	int i;
	Module *tmp = get_head(_current_metaclst);

	/* count metaclusters */
	for (i = 0; tmp; i++, tmp = get_next(tmp));

	/* malloc size array */
	if (metaclst_sizes == (Metaclst_size_t *) NULL) {
		metaclst_sizes = (Metaclst_size_t *) xmalloc(i *
		    sizeof (Metaclst_size_t));
	}
	(void) memset(metaclst_sizes, '\0', i * sizeof (Metaclst_size_t));

	/*
	 * for each metacluster, select it. compute default file system
	 * space required. deselect it.
	 *
	 * reset current metaluster
	 */
	v_set_n_lfs();
	tmp = get_head(_current_metaclst);
	for (i = 0; tmp; i++, tmp = get_next(tmp)) {
		mark_module(tmp, SELECTED);

		metaclst_sizes[i].original = metaclst_sizes[i].current =
		    v_get_dflt_lfs_space_total();

		mark_module(get_head(tmp), UNSELECTED);
	}
	mark_module(_current_metaclst, SELECTED);
	_recover_state_from_deltas(get_head(_current_metaclst));
}

void
v_set_swhier_root(Module * mod)
{
	/* tell SW lib what we think is current & default */
	set_current(mod);
	set_default(mod);

	_current_metaclst = mod;
	_curindex = 0;

}

/*
 * default sw selection: choose default product, default
 * meta-cluster, etc.
 * mark SUNWCreq metacluster as REQUIRED
 * mark SUNWCuser metacluster as SELECTED
 */
int
v_set_init_sw_config(void)
{
	Module *meta = get_head(_current_metaclst);

	if (meta) {
		/*
		 * wipe out any existing state
		 */
		mark_module(meta, UNSELECTED);
		_clear_all_deltas(meta);
	}
	while (meta) {

		if (strcmp(meta->info.mod->m_pkgid, REQD_METACLUSTER) == 0) {

			mark_required(meta);

		} else if (strcmp(meta->info.mod->m_pkgid,
			ENDUSER_METACLUSTER) == 0) {

			mark_module(meta, SELECTED);
			v_set_swhier_root(meta);

		}
		meta = get_next(meta);

	}

	return (V_OK);
}

/*
 * get number of metaclusters
 */
int
v_get_n_metaclsts(void)
{
	Module *tmp = get_head(_current_metaclst);
	int i;

	for (i = 0; tmp; i++, tmp = get_next(tmp));

	return (i);
}

/*
 * get i'th metacluster's name
 */
char *
v_get_metaclst_name(int i)
{
	Module *tmp = get_head(_current_metaclst);

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp && tmp->info.mod)
		return (tmp->info.mod->m_name);
	else
		return ("");
}

/*
 * get i'th metacluster's `size' which is a rough approximation of how much
 * disk space is required to install it.
 */
char *
v_get_metaclst_size(int i)
{
	int j;
	static char buf[16];
	Module *tmp = get_head(_current_metaclst);

	(void) sprintf(buf, "%6.2f MB", 0.0);

	for (j = i; j && tmp; j--, tmp = get_next(tmp));

	if (tmp != _current_metaclst)
		(void) sprintf(buf, "%6.2f MB", (float)
		    metaclst_sizes[i].original);
	else {
		metaclst_sizes[i].current = v_get_dflt_lfs_space_total();
		(void) sprintf(buf, "%6.2f MB", (float)
		    metaclst_sizes[i].current);
	}

	return (buf);

}

/*
 * get/set current metacluster
 */
void
v_set_current_metaclst(int i)
{
	Module *tmp = get_head(_current_metaclst);

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp && tmp->info.mod) {

		if (tmp != _current_metaclst) {

			/*
			 * deselect current metacluster, clear any existing
			 * modifications.
			 */
			_current_metaclst->info.mod->m_status = UNSELECTED;
			mark_module(get_head(tmp), UNSELECTED);

			/*
			 * switching metaclsts erases all deltas to the
			 * hierarchy... this used to be in
			 * _v_int_set_module_status() but got moved here so
			 * that it is only done once.
			 */
			_clear_all_deltas(get_head(tmp));

			if (v_is_upgrade())
				update_action(_current_metaclst);

		}
		_current_metaclst = tmp;
		set_current(_current_metaclst);
		mark_module(_current_metaclst, SELECTED);
		_current_metaclst->info.mod->m_status = SELECTED;

		if (v_is_upgrade())
			update_action(_current_metaclst);

	}
}

int
v_get_current_metaclst(void)
{
	Module *tmp = get_head(_current_metaclst);
	int i = 0;

	for (i = 0; tmp && tmp != _current_metaclst; i++,
	    tmp = get_next(tmp));

	return (i);
}

/*
 * is the i'th module at this level the currently selected metacluster?
 */
int
v_is_current_metaclst(int i)
{
	Module *tmp = get_head(_current_metaclst);
	int j;

	for (j = i; j && tmp; tmp = get_next(tmp), j--);

	if (tmp)
		return (tmp == _current_metaclst);
	else
		return (FALSE);
}

/* return number of products */
int
v_count_products(void)
{
	Module *tmp = get_head(get_current_product());
	int i;

	for (i = 0; tmp; i++, tmp = get_next(tmp));

	return (i);

}

static Module *
_v_get_current_product(void)
{
	return (get_head(get_current_product()));
}

char *
v_get_product_name(void)
{
	Module *current = _v_get_current_product();

	if (current && current->info.prod->p_name)
		return (current->info.prod->p_name);
	else
		return ("");

}

char *
v_get_product_version(void)
{
	Module *current = _v_get_current_product();

	if (current && current->info.prod->p_version)
		return (current->info.prod->p_version);
	else
		return ("");

}

/*
 * generic module stuff:
 *	v_int_*() - typically takes a Module * argument as the module
 *			to operate on.
 *
 *	v_*()	- typically only takes an integer index as the module
 *			to operate on (i.e., the i'th module).
 *
 *	separation motivated by need to expose metaclusters differently
 *	than regular modules (e.g., packages and clusters).  Operations
 *	on either are the same, but need to access them via different
 *	interfaces (get_metacluster_*() instead of get_module_*()).  Wanted
 *	to share underlying algorithms between the two different `types'
 *
 */

/*
 * internal function to count number of `visible' modules.
 *
 * does a traversal of the current cluster/package hierarchy,
 * taking into account clusters which have been `expanded' (i.e., their
 * sub-components are currently being displayed)
 *
 * think of this as a depth-first traversal of the module hierarchy.
 * each time a cluster is encountered, see if it is expanded, and if so,
 * `push' down a level and count it's children:
 *
 *   ------>clst1-------->clst2 - - -  +->clst3			(depth == 0)
 *            |             |          |
 *           pkga          pkgc        |			(depth == 1)
 *            |             |          |
 *           pkgb          pkgd--------+			(depth == 2)
 *
 * in this example, there are 5 `visible' modules, clst1 is 0, clst2 is 1,
 * pkgc is 2, pkgd is 3, clst3 is 4.
 *
 */
static int
_v_int_count_mods(void)
{
	int i;

	Module *head_meta = get_head(_current_metaclst);
	Module *tmp;
	Module *next[10];	/* remember next for up to 10 levels */
	int depth = 0;

	for (i = 0; i < 10; i++)
		next[i] = (Module *) NULL;

	if (head_meta && head_meta->sub)
		tmp = head_meta->sub;

	i = 0;
	for (;;) {

		if (tmp == (Module *) NULL) {

			if (depth == 0)
				break;	/* no more mods! */

			else if (next[depth - 1] != (Module *) NULL) {

				tmp = next[--depth];
				next[depth] = (Module *) NULL;

			} else if (next[depth - 1] == (Module *) NULL) {
				--depth;	/* last cluster expanded */
				break;	/* terminate loop */
			}
		}
		if (tmp && tmp->info.mod->m_flags & UI_SHOW_CHLD) {
			/* cluster is expanded */

			/* remember next module `after' tmp */
			next[depth++] = get_next(tmp);

			/* first module `below' tmp */
			tmp = tmp->sub;

			i++;
		} else if (tmp) {	/* not expanded... keep going */

			tmp = get_next(tmp);
			i++;

		}
	}

	return (i);
}

/*
 * retreive i'th visible module (see comment above)
 */
static Module *
_v_int_get_ith_mod(int i)
{

	/*
	 * want All children, not just those that are part of meta cluster
	 * go get head meta-cluster, 'guaranteeed' to be the All
	 * metacluster... its `sub' pointer points to the entire cluster
	 * hierarchy...
	 */
	Module *head_meta = get_head(_current_metaclst);
	Module *tmp;
	Module *next[10];	/* remember next for up to 10 levels */
	int depth = 0;
	int j;

	/*
	 * this might be really dangerous! assume that whoever is accessing
	 * the modules sequentially or repeatedly first calls
	 * v_set_current_module() to set the 'context'... if so, it saves a
	 * lot of looping...
	 */
	if (i == _curindex && (_curmod != (Module *) NULL))
		return (_curmod);

	for (j = 0; j < 10; j++)
		next[j] = (Module *) NULL;

	if (head_meta->sub)
		tmp = head_meta->sub;

	while (i) {

		if (tmp->info.mod->m_flags & UI_SHOW_CHLD) {
			/* cluster is expanded */

			/* remember next mod `after' tmp */
			next[depth++] = get_next(tmp);

			/* resume counting with first mod `below' tmp */
			tmp = tmp->sub;

			--i;
		} else {	/* not expanded... keep going */
			tmp = get_next(tmp);
			--i;
		}

		if (tmp == (Module *) NULL) {

			if (depth == 0)
				break;	/* unexpectedly ran out of mods! */
			else if (next[depth - 1] != (Module *) NULL) {

				tmp = next[--depth];
				next[depth] = (Module *) NULL;

			}
		}
	}

	return (tmp);

}

/* return number of modules at this level */
int
v_get_n_modules(void)
{
	return (_v_int_count_mods());
}

/* does i'th *displayed* module have submodules?: */
int
v_get_module_has_submods(int i)
{

	Module *mod;

	mod = _v_int_get_ith_mod(i);
	if (mod && mod->sub)
		return (TRUE);
	else
		return (FALSE);
}

/* is i'th *displayed* module set to have its submodules shown? */
int
v_get_submods_are_shown(int i)
{

	Module *mod;

	mod = _v_int_get_ith_mod(i);

	if (mod && (mod->info.mod->m_flags & UI_SHOW_CHLD))
		return (TRUE);
	else
		return (FALSE);

}

/*
 * at what `level' is the i'th module?
 *
 * 0 -> top-level cluster
 * 1 -> package
 * 2 -> sub-package	(not supprted (yet?))
 * 3 -> sub-sub-package	(not supprted (yet?))
 */
int
v_get_module_level(int i)
{

	Module *mod;

	mod = _v_int_get_ith_mod(i);
	if (mod && (mod->info.mod->m_flags & UI_SUBMOD_L1))
		return (1);
	if (mod && (mod->info.mod->m_flags & UI_SUBMOD_L2))
		return (2);
	if (mod && (mod->info.mod->m_flags & UI_SUBMOD_L3))
		return (3);
	else
		return (0);
}

/*
 * expand i'th module:
 * add i'th modules `children' to the list of modules to
 * be displayed mark these children with their sub- level depth
 */
int
v_expand_module(int i)
{
	Module *tmp;
	Module *mod;
	int flag = UI_SUBMOD_L1;

	mod = _v_int_get_ith_mod(i);
	if (mod && mod->sub) {

		/*
		 * mark this module as expanded.
		 */
		mod->info.mod->m_flags |= UI_SHOW_CHLD;

		if (mod->info.mod->m_flags & UI_SUBMOD_L1)
			flag = UI_SUBMOD_L2;
		else if (mod->info.mod->m_flags & UI_SUBMOD_L2)
			flag = UI_SUBMOD_L3;

		/*
		 * mark this module's sub-components with their sub-level
		 * depth
		 */
		for (tmp = mod->sub; tmp; tmp = get_next(tmp))
			tmp->info.mod->m_flags |= flag;
		return (V_OK);

	} else
		return (V_FAILURE);
}

/*
 * clear out any of the UI specific `expand/contract' flags from `mod'
 */
static void
clear_ui_flags(Module * mod, int flags)
{
	Module *tmp;

	for (tmp = mod; tmp; tmp = get_next(tmp)) {
		tmp->info.mod->m_flags ^= flags;

		if (tmp->sub != (Module *) NULL)
			clear_ui_flags(tmp->sub, flags);
	}
}

/* unexpand a module */
int
v_contract_module(int i)
{
	Module *mod;
	int flag = UI_SUBMOD_L1 | UI_SUBMOD_L2 | UI_SUBMOD_L3;

	mod = _v_int_get_ith_mod(i);

	if (mod && (mod->info.mod->m_flags & UI_SHOW_CHLD)) {

		/*
		 * mark this module is no longer expanded, unset the flag
		 */
		mod->info.mod->m_flags ^= UI_SHOW_CHLD;

		/*
		 * mark this module's sub-components with their sub-level
		 * depth
		 */
		clear_ui_flags(mod->sub, flag);

		return (V_OK);

	} else
		return (V_FAILURE);
}

/*
 * get/set current module
 */
int
v_set_current_module(int i)
{

	Module *tmp;

	/*
	 * this forces _v_int_get_ith_mod() to actually go find the i'th
	 * module...
	 */
	_curindex = -1;
	_curmod = (Module *) NULL;

	if ((tmp = _v_int_get_ith_mod(i)) != (Module *) NULL) {

		_curmod = tmp;
		_curindex = i;
		return (V_OK);

	} else
		return (V_FAILURE);

}

int
v_get_current_module(void)
{
	return (_curindex);
}

/*
 * predicate which indicates if the current module is a metacluster
 */
int
v_current_module_is_metaclst(void)
{
	return (_curmod->type == METACLUSTER);
}

/* get current_module's type */
V_ModType_t
v_get_module_type(int i)
{
	Module *tmp;
	V_ModType_t type;

	if (tmp = _v_int_get_ith_mod(i)) {

		switch (tmp->type) {
		case PRODUCT:
			type = V_PRODUCT;
			break;

		case METACLUSTER:
			type = V_METACLUSTER;
			break;

		case CLUSTER:
			type = V_CLUSTER;
			break;

		case PACKAGE:
			type = V_PACKAGE;
			break;

		}
	} else
		type = V_PACKAGE;

	return (type);

}

/* get current_module's status */
int
v_get_module_status(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp) {

			if (tmp->type == METACLUSTER)
				return (tmp->info.mod->m_status);
			else {
				/*
				 * fixes selection feedback bug in UI
				 */
				tmp->info.mod->m_status = mod_status(tmp);
				return (tmp->info.mod->m_status);
			}

		} else
			return (UNSELECTED);

	} else
		return (UNSELECTED);
}

int
_v_int_set_module_status(Module * tmp)
{
	int stat;

	if (tmp->type == PRODUCT || tmp->type == MEDIA) {

		return (V_FAILURE);

	} else {

#define	UI_ADDED	0x1000UL
#define	UI_REMVD	0x2000UL

		/*
		 * track the net delta's to packages and clusters, anything
		 * added or removed from the default configuration
		 *
		 * if module is partial it will be SELECTED after toggling
		 *	-> if it was 'removed' clear the removedflag
		 *	-> else mark it as `added'
		 * if module is unselected it will be SELECTED after toggling
		 *	-> if it was 'removed' clear the remvd flag
		 *	-> else mark it as `added'
		 * if module is selected it will be UNSELECTED after toggling
		 *	-> if it was 'added' clear the added flag
		 *	-> else mark it as `remvd'
		 */

		stat = mod_status(tmp);
		if (stat != REQUIRED) {
			if (stat == PARTIALLY_SELECTED) {
				if (tmp->info.mod->m_flags & UI_REMVD)
					tmp->info.mod->m_flags ^= UI_REMVD;
				else
					tmp->info.mod->m_flags |= UI_ADDED;
			} else if (stat == UNSELECTED) {
				if (tmp->info.mod->m_flags & UI_REMVD)
					tmp->info.mod->m_flags ^= UI_REMVD;
				else
					tmp->info.mod->m_flags |= UI_ADDED;
			} else if (stat == SELECTED) {
				if (tmp->info.mod->m_flags & UI_ADDED)
					tmp->info.mod->m_flags ^= UI_ADDED;
				else
					tmp->info.mod->m_flags |= UI_REMVD;
			}
		}

		tmp->info.mod->m_status = toggle_module(tmp);

		/*
		 * if this is a cluster, switching it's state erases any
		 * deltas to it's submodules
		 */
		if (tmp->type == CLUSTER)
			_clear_all_deltas(tmp->sub);

		if (v_is_upgrade())
			update_action(tmp);
	}

	return (V_OK);

}

static int _max_metaclst_deltas = 128;
static int _n_metaclst_deltas = 0;
static char **metaclst_deltas = (char **) NULL;

int
v_metaclst_edited(void)
{
	Module *mp = get_head(_current_metaclst);

	return (_find_any_delta(mp));

}

/*
 * pretty much the same as _count_all_deltas(), but terminates when first
 * delta is found
 */
static int
_find_any_delta(Module * mp)
{

	for (; mp != (Module *) NULL; mp = get_next(mp)) {

		if (mp->info.mod->m_flags & UI_ADDED)
			return (1);
		if (mp->info.mod->m_flags & UI_REMVD)
			return (1);

		if (mp->sub != (Module *) NULL) {
			if (_find_any_delta(mp->sub) == 1)
				return (1);
		}
	}

	return (0);

}

static void
_recover_state_from_deltas(Module * mp)
{
	for (; mp != (Module *) NULL; mp = get_next(mp)) {

		if (mp->info.mod->m_flags & UI_ADDED)
			mark_module(mp, SELECTED);
		if (mp->info.mod->m_flags & UI_REMVD)
			mark_module(mp, UNSELECTED);

		if (mp->sub != (Module *) NULL) {
			_recover_state_from_deltas(mp->sub);
		}
		mp->info.mod->m_status = mod_status(mp);
	}
}

static void
_clear_all_deltas(Module * mp)
{
	for (; mp != (Module *) NULL; mp = get_next(mp)) {

		if (mp->info.mod->m_flags & UI_ADDED)
			mp->info.mod->m_flags ^= UI_ADDED;
		if (mp->info.mod->m_flags & UI_REMVD)
			mp->info.mod->m_flags ^= UI_REMVD;

		if (mp->sub != (Module *) NULL) {
			_clear_all_deltas(mp->sub);
		}
	}
}

/*
 * examine each cluster. if its subcomponents have all been added or
 * deleted, clear the subcomponent delta(s) and move the delta up to the
 * cluster.
 *
 * trying to detect several cases:
 *	(de)selecting a cluster and
 *	(de)selecting all of its subcomponents should result in a 0 net delta.
 *
 *	(de)selecting all of a cluster's subcomponents should collapse
 *	into a delta on the cluster, not the subcomponents
 */
static void
_collapse_metaclst_deltas(Module * mp)
{
	int poss;
	int add, rem;
	Module *sub;

	for (; mp != (Module *) NULL; mp = get_next(mp)) {

		/*
		 * skip non-clusters and required clusters
		 */
		if (mp->type != CLUSTER || mod_status(mp) == REQUIRED)
			continue;

		poss = add = rem = 0;
		for (sub = mp->sub;
		    sub != (Module *) NULL;
		    sub = get_next(sub)) {

			/*
			 * if subcomponent has been added or removed, incr
			 * appropriate counter
			 */
			if (sub->info.mod->m_flags & UI_ADDED) {
				++add;
			} else if (sub->info.mod->m_flags & UI_REMVD) {
				++rem;
			}
			if (mod_status(sub) != REQUIRED) {
				++poss;	/* incr count of possible deltas */
			}
		}

		/*
		 * if adds or removes equals possible deltas then all
		 * subcomponents of the cluster have been added or removed.
		 *
		 * mark the cluster and clear the subcomponents.
		 *
		 * if the cluster itself was added or removed, clear that delta
		 * flag.
		 */
		if (add == poss) {

			_clear_all_deltas(mp->sub);
			mp->info.mod->m_flags |= UI_ADDED;

			if (mp->info.mod->m_flags & UI_REMVD)
				mp->info.mod->m_flags ^= UI_REMVD;

		} else if (rem == poss) {

			_clear_all_deltas(mp->sub);
			mp->info.mod->m_flags |= UI_REMVD;

			if (mp->info.mod->m_flags & UI_ADDED)
				mp->info.mod->m_flags ^= UI_ADDED;

		}
	}

}

/*
 * internal function to count the number of deltas made to the current
 * metacluster.
 *
 * the delta type of interest is passed in, it is the bit-field flag
 * representing the type to count.  It should be either UI_ADDED or UI_REMVD
 *
 * a side effect of this counting process is the construction of an array of
 * module ids (e.g., pkgid or clusterids) representing the `deltas'
 */
static int
_count_all_deltas(Module * mp, unsigned int delta_type)
{
	for (; mp != (Module *) NULL; mp = get_next(mp)) {

		if (mp->info.mod->m_flags & delta_type) {

			metaclst_deltas[_n_metaclst_deltas] = (char *)
			    mp->info.mod->m_pkgid;

			_n_metaclst_deltas++;

			/* grow array if necessary */
			if (_n_metaclst_deltas == _max_metaclst_deltas) {
				_max_metaclst_deltas += 128;

				metaclst_deltas =
				    (char **) xrealloc((void *) metaclst_deltas,
				    _max_metaclst_deltas * sizeof (char *));
			}
		}
		if (mp->sub != (Module *) NULL) {
			_n_metaclst_deltas =
			    _count_all_deltas(mp->sub, delta_type);
		}
	}

	return (_n_metaclst_deltas);
}

/*
 * a function to count what net changes have been made to the metacluster.
 *
 * non-zero argument counts additions to the default metacluster config, zero
 * arg counts deletions
 *
 * also collapses cluster/subcomponent changes where possible (see
 * collapse_metaclst_deltas()).
 *
 * also sets up an array of pkg ids for later retrieval by the UI.
 *
 * returns number of deltas of the appropriate type.
 */
int
v_get_n_metaclst_deltas(int added)
{
	Module *head_meta = get_head(_current_metaclst);
	Module *tmp;
	unsigned int delta_type = (added != 0 ? UI_ADDED : UI_REMVD);

	_n_metaclst_deltas = 0;

	if (metaclst_deltas == (char **) NULL)
		metaclst_deltas =
		    (char **) xcalloc(_max_metaclst_deltas * sizeof (char *));

	(void) memset(metaclst_deltas, '\0',
	    _max_metaclst_deltas * sizeof (char *));

	if (head_meta && head_meta->sub)
		tmp = head_meta->sub;

	_collapse_metaclst_deltas(tmp);
	_n_metaclst_deltas = _count_all_deltas(tmp, delta_type);

	return (_n_metaclst_deltas);
}

/*
 * look up the full name of the i'th module added or deleted fomr the default
 * configuration.  Module may be a package or a cluster, be sure to check
 * both hashed lists before returning ""
 */
char *
v_get_delta_package_name(int i)
{
	Node *np;
	Module *prod = get_current_product();
	char *cp;

	if (np = findnode((List *) (prod->info.prod->p_packages),
		metaclst_deltas[i]))
		cp = ((Modinfo *) np->data)->m_name;
	else if (np = findnode((List *) (prod->info.prod->p_clusters),
		metaclst_deltas[i]))
		cp = ((Module *) np->data)->info.mod->m_name;
	else
		cp = "";

	return (cp);
}

/*
 * toggle's module state between selected and unselected. returns new state
 * if successful.  V_FAILURE if unsuccessful.
 */
int
v_set_module_status(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		return (_v_int_set_module_status(tmp));

	} else
		return (V_FAILURE);

}

/* returns pointer to a module's identifier (name, mount point or pkgid) */
char *
v_get_module_id(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp->type == PRODUCT)
			return (tmp->info.prod->p_name);
		else if (tmp->type == MEDIA)
			return (tmp->info.media->med_dir);
		else
			return (tmp->info.mod->m_pkgid);

	} else
		return ("");
}

/* returns name of current module */
char *
v_get_module_name(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp->type == PRODUCT)
			return (tmp->info.prod->p_name);
		else if (tmp->type == MEDIA)
			return (tmp->info.media->med_dir);
		else
			return (tmp->info.mod->m_name);

	} else
		return ("");
}

/* returns description of current module */
char *
v_get_module_description(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp->type == PRODUCT)
			return (NULL);
		else if (tmp->type == MEDIA)
			return (NULL);
		else
			return (tmp->info.mod->m_desc);

	} else
		return ("");
}

/* returns vendor information for current module */
char *
v_get_module_vendor(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {
		if (tmp->type == PRODUCT)
			return (NULL);
		else if (tmp->type == MEDIA)
			return (NULL);
		else
			return (tmp->info.mod->m_vendor);
	} else
		return ("");
}

/* returns version information for current module */
char *
v_get_module_version(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {
		if (tmp->type == PRODUCT)
			return (NULL);
		else if (tmp->type == MEDIA)
			return (NULL);
		else
			return (tmp->info.mod->m_version);
	} else
		return ("");
}

/*
 * sets the i'th module's basedir to 'val' useful only should initial
 * install ever support installing things into non-default locations.
 */
int
v_set_module_basedir(int i, char *val)
{

	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp) {

			if (tmp->type == PRODUCT)
				return (V_FAILURE);
			else if (tmp->type == MEDIA)
				return (V_FAILURE);
			else if (tmp->info.mod) {
				tmp->info.mod->m_basedir = xstrdup(val);
				return (V_OK);

			}
		} else
			return (V_FAILURE);

	}
	return (V_FAILURE);
}

/* retuns pointer to buffer continaing a module's basedir */
char *
v_get_module_basedir(int i)
{

	static char buf[MAXPATHLEN];
	Module *tmp;

	buf[0] = '\0';

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp->type != MEDIA && tmp->type != PRODUCT)
			if (tmp && tmp->info.mod && tmp->info.mod->m_basedir)
				(void) strcpy(buf, tmp->info.mod->m_basedir);

	}
	return (buf);

}

/*
 * retuns pointer to array containing estimated file system space usage for
 * i'th module.  Only useful for packages, clusters or meta-clusters.
 *
 */
Sizes_t *
v_get_module_fsspace_used(int i)
{
	FSspace **spaceinfo;
	static Sizes_t sizes;
	Sizes_t *s;
	Module *tmp;

	s = &sizes;

	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp &&
		    (tmp->type == PACKAGE || tmp->type == CLUSTER ||
			tmp->type == METACLUSTER)) {
			/*
			 * get size of module, this is it's size if
			 * everything were installed
			 */
			spaceinfo = calc_cluster_space(tmp, UNSELECTED);

			(void) sprintf(s->sz[ROOT_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/") / KBYTE);
			(void) sprintf(s->sz[USR_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/usr") / KBYTE);
			(void) sprintf(s->sz[OPT_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/opt") / KBYTE);
			(void) sprintf(s->sz[VAR_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/var") / KBYTE);
			(void) sprintf(s->sz[EXPORT_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/export") / KBYTE);
			(void) sprintf(s->sz[USR_OWN_FS], "%6.2f MB",
			    _v_int_get_kbytes(spaceinfo, "/usr/openwin") /
			    KBYTE);
		}
		return (s);

	} else
		return ((Sizes_t *) NULL);
}

/* returns total size of package `pkgid' in kbytes */
int
v_get_size_in_kbytes(char *pkgid)
{
	Module *prod = get_current_product();
	List *pkgs = prod->info.prod->p_packages;
	Node *tmp = findnode(pkgs, pkgid);

	if (tmp && ((Modinfo *) tmp->data != (Modinfo *) NULL))
		return ((int) tot_pkg_space((Modinfo *) tmp->data));
	else
		return (0);
}

/*
 * this function is only used by the time-to-install estimating code in the
 * progres display (c_progress.c)
 */
unsigned int
v_get_total_kb_to_install(void)
{
	Module *prod = get_current_product();

	return (calc_tot_space(prod->info.prod));
}

static int
_v_int_get_kbytes(FSspace ** spaceinfo, char *fs)
{
	int i;

	for (i = 0;
	    spaceinfo[i] && (strcmp(spaceinfo[i]->fsp_mntpnt, fs) != 0);
	    i++);

	if (spaceinfo[i]) {
		/*
		 * round up to 10 (Kbytes) since that is the display
		 * threshold
		 */
		if (spaceinfo[i]->fsp_reqd_contents_space < 10)
			return (10);
		else
			return (spaceinfo[i]->fsp_reqd_contents_space);
	} else
		return (0);
}

char *
_v_int_get_module_size(Module * tmp)
{
	FSspace **sp;
	static char buf[32];
	int sum = 0;
	int i;
	int stat;

	(void) sprintf(buf, "%6.2f MB", 0.0);

	if (tmp) {
		if (tmp->type == PACKAGE || tmp->type == CLUSTER) {

			/*
			 * get size of module, return size based on the
			 * sub-components that are selected
			 */
			stat = mod_status(tmp);

			if (stat == SELECTED ||
			    stat == PARTIALLY_SELECTED ||
			    stat == REQUIRED) {

				sp = calc_cluster_space(tmp, SELECTED);

				for (sum = 0, i = 0;
				    sp[i];
				    sum += sp[i]->fsp_reqd_contents_space,
				    i++);

				/*
				 * round up to 10 (Kbytes) since that is the
				 * display threshold
				 */
				if (sum < 10)
					sum = 10;

				(void) sprintf(buf, "%6.2f MB", sum / KBYTE);
			}
		} else if (tmp->type == METACLUSTER) {

			if (tmp == _current_metaclst)
				sp = calc_cluster_space(get_head(tmp),
				    SELECTED);
			else
				sp = calc_cluster_space(tmp, UNSELECTED);

			for (sum = 0, i = 0; sp[i];
			    sum += sp[i]->fsp_reqd_contents_space, i++);

			/*
			 * round up to 10 (Kbytes) since that is the display
			 * threshold
			 */
			if (sum < 10)
				sum = 10;

			(void) sprintf(buf, "%6.2f MB", sum / KBYTE);
		}
	}
	return (buf);

}


/* returns total size approximation (in MB) for current module */
char *
v_get_module_size(int i)
{
	Module *tmp;

	if (tmp = _v_int_get_ith_mod(i)) {
		return (_v_int_get_module_size(tmp));
	} else
		return ("");

}

/* returns string describing architectures supported by current module */
char *
v_get_module_arches(int i)
{
	static char buf[BUFSIZ];
	Module *tmp;
	int first = TRUE;

	buf[0] = '\0';


	if (tmp = _v_int_get_ith_mod(i)) {

		if (tmp->type == PRODUCT) {

			Arch *atmp = tmp->info.prod->p_arches;

			while (atmp) {

				if (first) {

					(void) strcpy(buf, atmp->a_arch);
					first = FALSE;

				} else {

					(void) strcat(buf, ", ");
					(void) strcat(buf, atmp->a_arch);

				}

				atmp = atmp->a_next;
			}

		} else if (tmp->type == PACKAGE && tmp->info.mod->m_arch) {
			/* FIX FIX FIX ... Maybe do instance things? */
			(void) strcpy(buf, tmp->info.mod->m_arch);
		}
	}
	return (buf);
}

/*
 * stuff for locales.
 * a product has zero or more localizations.
 * zero or more of these locales may be selected for installation.
 * a locale consists of a locale name and a language.
 */
static char default_locale[128] = "C";

/* select default locale */
int
v_set_default_locale(char *loc)
{
	Module *prod = get_current_product();

	if (loc == (char *) NULL)
		return (V_FAILURE);
	if (prod == (Module *) NULL)
		return (V_FAILURE);

	if (select_locale(prod, loc) == SUCCESS) {

		(void) strcpy(default_locale, loc);
		return (V_OK);

	} else
		return (V_FAILURE);
}

char *
v_get_default_locale(void)
{
	return (default_locale);
}

/* return number of locales associated with current product */
int
v_get_n_locales(void)
{
	Module *tmp = get_all_locales();
	int i;

	for (i = 0; tmp; i++, tmp = get_next(tmp));

	return (i);
}

/* return index of current locale */
int
v_get_current_locale(void)
{
	return (_current_locale_index);
}

/* set internal memory of current locale to i'th */
int
v_set_current_locale(int i)
{
	Module *tmp = get_all_locales();

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp) {

		_current_locale_index = i;
		return (V_OK);

	} else
		return (V_FAILURE);
}

/* get i'th locale's status */
int
v_get_locale_status(int i)
{
	Module *tmp = get_all_locales();

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp)
		return (tmp->info.locale->l_selected);
	else
		return (V_FAILURE);
}


/* set i'th locale's status */
int
v_set_locale_status(int i, int status)
{
	char *locale = v_get_locale_name(i);
	Module *prod = get_current_product();
	extern void upg_select_locale(Module *, char *);
	int stat;

	if (locale == (char *) NULL)
		return (V_FAILURE);
	if (prod == (Module *) NULL)
		return (V_FAILURE);

	if (status == TRUE)
		if (v_is_upgrade())
			upg_select_locale(prod, locale);
		else
			stat = select_locale(prod, locale);
	else if (v_is_upgrade())
		upg_deselect_locale(prod, locale);
	else
		stat = deselect_locale(prod, locale);

	if (stat != SUCCESS)
		return (V_FAILURE);
	else
		return (V_OK);

}

/* get i'th locale's name */
char *
v_get_locale_name(int i)
{
	Module *tmp = get_all_locales();

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp)
		return (tmp->info.locale->l_locale);
	else
		return (NULL);
}

/* get i'th locale's language */
char *
v_get_locale_language(int i)
{
	Module *tmp = get_all_locales();

	for (; i && tmp; i--, tmp = get_next(tmp));

	if (tmp)
		return (tmp->info.locale->l_language);
	else
		return (NULL);
}

/*
 * stuff for architectures. a product has one or more supported
 * architectures. one or more of these architectures may be selected for
 * installation. an architecture consists of a name.
 */

static char *native_arch = (char *) NULL;

/* mark native arch as selected */
void
v_init_native_arch(void)
{
	int narches = v_get_n_arches();
	int i;

	native_arch = get_default_arch();

	for (i = 0; i < narches; i++) {

		if (strcmp(native_arch, v_get_arch_name(i)) == 0) {
			(void) v_set_arch_status(i, TRUE);
			break;

		}
	}
}

/* unselect all non-native arches */
void
v_clear_nonnative_arches(void)
{
	int narches = v_get_n_arches();
	int i;

	native_arch = get_default_arch();

	for (i = 0; i < narches; i++)
		if (strcmp(native_arch, v_get_arch_name(i)) != 0)
			(void) v_set_arch_status(i, FALSE);
}

/*
 * return a string containing a comma separated list of currently selected
 * architectures
 */
char *
v_get_selected_arches(void)
{
	Module *prod = _v_get_current_product();
	Arch *tmp = get_all_arches(prod);
	static char buf[BUFSIZ];
	int first = TRUE;

	buf[0] = '\0';
	while (tmp) {

		if (tmp->a_selected)
			if (first) {

				(void) strcpy(buf, tmp->a_arch);
				first = FALSE;

			} else {

				(void) strcat(buf, ", ");
				(void) strcat(buf, tmp->a_arch);

			}

	}
	return (buf);
}

/* return number of architectures associated with current product */
int
v_get_n_arches(void)
{
	Module *prod = _v_get_current_product();
	Arch *tmp = get_all_arches(prod);
	int i;

	for (i = 0; tmp; i++, tmp = tmp->a_next);

	return (i);
}

/* return index of current architecture */
int
v_get_current_arch(void)
{
	return (_current_arch_index);
}

/* set internal memory of current architecture to i'th */
int
v_set_current_arch(int i)
{
	Module *prod = _v_get_current_product();
	Arch *tmp = get_all_arches(prod);

	for (; i && tmp; i--, tmp = tmp->a_next);

	if (tmp) {

		_current_arch_index = i;
		return (V_OK);

	} else
		return (V_FAILURE);
}

/* get i'th architecture's status */
int
v_get_arch_status(int i)
{
	Module *prod = _v_get_current_product();
	Arch *tmp = get_all_arches(prod);

	for (; i && tmp; i--, tmp = tmp->a_next);

	if (tmp)
		return (tmp->a_selected ? TRUE : FALSE);
	return (V_FAILURE);
}

/* set i'th architecture's status */
int
v_set_arch_status(int i, int status)
{
	Module *prod = _v_get_current_product();
	char *arch = v_get_arch_name(i);
	int ret = SUCCESS;

	if (*arch) {

		/*
		 * select/deselect i'th arch, ensure that native arch is
		 * always selected
		 */
		if (status == TRUE)
			ret = select_arch(prod, arch);
		else if (strcmp(arch, native_arch) != 0)
			ret = deselect_arch(prod, arch);

		/*
		 * update the status of all architecture specific instances
		 * when new architectures are selected or deselected.
		 * Software library handles the details...
		 */
		mark_arch(prod);

	} else
		ret = ERR_BADARCH;	/* ?? */

	if (ret == SUCCESS)
		return (V_OK);
	else
		return (V_FAILURE);

}

/* is i'th arch the native arch? */
int
v_is_native_arch(int i)
{
	char *arch = v_get_arch_name(i);

	if (arch && *arch)
		return (strcmp(arch, native_arch) == 0);

	return (0);
}

/* get i'th architecture's name */
char *
v_get_arch_name(int i)
{
	Module *prod = _v_get_current_product();
	Arch *tmp = get_all_arches(prod);

	for (; i && tmp; i--, tmp = tmp->a_next);

	if (tmp)
		return (tmp->a_arch);
	else
		return ("");
}

int
v_get_n_selected_packages(void)
{
	Module *prod = get_current_product();

	return (walklist(prod->info.prod->p_packages, package_selected, ""));
}

/*
 * this is mostly for the progress display, although two functions in
 * v_check.c have been changed to use it also.
 */
char *
v_get_pkgname_from_pkgid(char *pkgid)
{
	Node *np;
	Module *prod = get_current_product();
	static char buf[256];

	buf[0] = '\0';

	if (np = findnode((List *) (prod->info.prod->p_packages), pkgid))
		(void) strncpy(buf, ((Modinfo *) np->data)->m_name, 255);

	return (buf);
}
