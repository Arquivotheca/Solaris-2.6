#ifndef lint
#ident   "@(#)depend.c 1.17 95/02/24 SMI"
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
#include "sw_lib.h"

/* Local Statics and Constants */

#define p_data 		((Modinfo *)pkg->data)
#define n_data 		((Modinfo *)np->data)
#define SELECTED(x) 	((x)->m_status == SELECTED || (x)->m_status == REQUIRED)

static Depend  *dependencies = (Depend *) NULL;

/* Public function prototypes */
int     swi_check_sw_depends(void);
Depend  *swi_get_depend_pkgs(void);

/* Library function prototypes */

void	read_pkg_depends(Module *, Modinfo *);
void	parse_instance_spec(Depend *, char *);

/* Local function prototypes */

static void 	set_depend_pkgs(Depend *);
static Depend   *add_depend_pkg(Depend *, char *, char *);
static char 	*parse_depend_pkgid(char *);
static Depend   *add_depend_instance (Depend **, char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * get_depend_pkgs()
 *	Return a pointer to the current list of unresolved package
 *	dependencies.
 * Parameters:
 *	none
 * Returns:
 *	Depend *	- current list of unresolved package dependencies
 * Status:
 *	public
 */ 
Depend *
swi_get_depend_pkgs(void)
{
	return (dependencies);
}

/*
 * check_sw_depends()
 *	Determine if the current product has any packages which are SELECTED,
 *	but which have dependencies on packages which are not SELECTED. 
 * Parameters:
 *	none
 * Return:
 *	1	- selected packages have dependencies on unselected
 *		  packages
 *	0	- selected packages do not have dependencies on
 *		  unselected packages
 */
int
swi_check_sw_depends(void)
{
	Module	*prod = get_current_product();
	Node	*pkg = prod->info.prod->p_packages->list;
	Depend	*dpnd;
	Depend	*dpnd_pkgs=(Depend *)NULL;
	Node	*np;
	Modinfo	*inst;

	/* walk the package list for the current product */

	for (pkg = pkg->next; pkg != prod->info.prod->p_packages->list; pkg = pkg->next) {
		if (SELECTED(p_data)) {
		/* 
		 * walk the 'pdepends' list for the given package
		 * for each pdependency of this package 
		 *   if pre-requisite package not selected 
		 *			(pkgid, arch & version match) 
		 *	 add pre-requisite package to list of unsat pdepends
		 */
for (dpnd = p_data->m_pdepends; dpnd; dpnd = dpnd->d_next) {

	if ((np = findnode(prod->info.prod->p_packages, dpnd->d_pkgid)) == (Node *)NULL)
		continue;	/* package specified not in hash list */

	if (dpnd->d_arch || dpnd->d_version) {
		for(inst = n_data; inst != (Modinfo *)NULL; inst=next_inst(inst)) {
			if(supports_arch(dpnd->d_arch, inst->m_arch)
			      		&& strcmp(inst->m_version, dpnd->d_version) == 0
					&& SELECTED(inst) == 0)
				dpnd_pkgs = add_depend_pkg(dpnd_pkgs, p_data->m_pkgid, 
								inst->m_pkgid);
		}
	} else if (SELECTED(n_data) == 0)
		dpnd_pkgs = add_depend_pkg(dpnd_pkgs, p_data->m_pkgid, n_data->m_pkgid);
}
		/*
		 * idepends: 
		 * for each idependency of this package 
		 *   if idependency package is selected 
		 *	 (pkgid, arch & version match) 
		 *	 add idependency package to list of unsat idepends
		 */
for (dpnd = p_data->m_idepends; dpnd; dpnd = dpnd->d_next) {
	if ((np = findnode(prod->info.prod->p_packages, dpnd->d_pkgid)) == (Node *)NULL)
		continue;	/* unknown pkg specified as dependency */

	if (SELECTED(n_data) == 0)
  		if (!dpnd->d_arch && !dpnd->d_version)
			continue; /* error */

	for(inst = n_data; inst != (Modinfo *)NULL; inst = next_inst(inst))
		if(supports_arch(dpnd->d_arch, inst->m_arch)
  				&& strcmp(inst->m_version, dpnd->d_version) == 0
				&& SELECTED(inst) == 0)
			dpnd_pkgs = add_depend_pkg(dpnd_pkgs, inst->m_pkgid, p_data->m_pkgid);
}
		} else {   /* package not selected */

		/*
		 * rdepends:
		 * for each rdependency of this package 
		 *	if rdependency package is selected 
		 *		(pkgid, arch & version match) 
		 *		add package to list of unsat rdepends
		 */
for (dpnd = p_data->m_rdepends; dpnd; dpnd = dpnd->d_next) {
	if ((np = findnode(prod->info.prod->p_packages, dpnd->d_pkgid)) == (Node *)NULL)
		continue;	/* unknown pkg specified as dependency */

	if (SELECTED(n_data) != 1)
		continue;

	if (!dpnd->d_arch && !dpnd->d_version) {
		dpnd_pkgs = add_depend_pkg(dpnd_pkgs, p_data->m_pkgid, n_data->m_pkgid);
		continue;
	}

	for(inst = n_data; inst != (Modinfo *)NULL; inst=next_inst(inst)) {
		if(supports_arch(dpnd->d_arch, inst->m_arch) 
				&& strcmp(inst->m_version, dpnd->d_version) == 0
				&& SELECTED(inst) == 0)
			dpnd_pkgs = add_depend_pkg(dpnd_pkgs, inst->m_pkgid, p_data->m_pkgid);
	}
}


		}
	}

#undef p_data
#undef n_data

	if (dpnd_pkgs!=(Depend *)NULL) {
		set_depend_pkgs(dpnd_pkgs);
		return(1);
	} else
		return(0);
}

/*
 * add_depend_instance()
 * Insert a new depend structure into the linked list referenced
 * by 'dpp'. Initialize with 'pkgid'. 
 * Parameters:	dpp	- pointer to head of depend link list; modified 
 *			  only if the list was NULL
 *		pkgid	- package id initialization string
 * Return:	NULL	- Error: could not allocate a Depend structure
 *		Depend*	- pointer to newly created Depend structure 
 * Note:	alloc routine. Should add the entries to the front of
 *		the list to improve performance
 */
static Depend *
add_depend_instance (Depend ** dpp, char * pkgid)
{
	Depend 	*newdp, *walkdp;

	/* create the new depend structure and initialize it */
	newdp = (Depend *)xcalloc(sizeof(Depend));
	if (newdp == (Depend *)NULL)
		return ((Depend *)NULL);

	newdp->d_pkgid = xstrdup(pkgid);

	/* add the new structure to the *dpp linked list */
	if (*dpp == (Depend *)NULL)
		*dpp = newdp;
	else {
		for (walkdp = *dpp; walkdp->d_next ; walkdp = walkdp->d_next)
			;
		walkdp->d_next = newdp;
		newdp->d_prev = walkdp;
	}

	return (newdp);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * parse_instance_spec()
 *	Set the 'arch' or the 'version' fields of the Depend parameter
 *	structure according to the value in the 'cp' instance specification
 *	string. Valid values for 'cp' are:
 *
 * 		(<arch>)<version>
 *		(<arch>)
 *		version
 * Parameters:
 *	dp	- pointer to depend structure
 *	cp	- string containing instance specification string
 * Returns:
 *	none
 * Status:
 *	semi-private
 * Note:
 *	There is nothing to handle error conditions in this routine 
 */
void 
parse_instance_spec(Depend * dp, char * cp)
{
	char	*cp1, *cp2;

	if (dp == (Depend *)NULL)
		return;

	if (*cp == '(') {
		if ((cp1 = strrchr(cp, ')')) == NULL)
			/*
			 * This is an error, but there is no way to return
			 * that fact.
			 */
			return;
		cp2 = cp1 + 1;
		if (cp2 && *cp2) {		
			/* (<arch>)<version> */
			dp->d_version = (char *)xstrdup(cp2);
			*cp1='\0';
			dp->d_arch = (char *)xstrdup(cp+1);
		} else {
			/* (<arch>) only */
			*cp1='\0';
			dp->d_arch = (char *)xstrdup(cp+1);
		}
	} else
		/* <version> */
		dp->d_version = (char *)xstrdup(cp);
}

/*
 * read_pkg_depends()
 *	Open the "depend" file and create depend chains from the input.
 * Parameters:
 *	prod	- product module pointer
 *	info	- modinfo structure pointer
 * Return:
 *	none
 * Note:
 *	This routine does not check to see if there are already
 *	depend chains hooked to 'info'. This could be a possible
 *	memory leak.
 * Status:
 *	semi-private (internal library use only)
 */
void 
read_pkg_depends(Module * prod, Modinfo * info)
{
	FILE	*fp = (FILE *) NULL;
	char	path[MAXPATHLEN];
	char	buf[BUFSIZ + 1];
	char	*bp;
	Depend	*dp, **dpp;

	/* open "depend" file for package, and reset the P,I,R fields in the
	 * modinfo structure 
	 */
	if (prod->parent->info.media->med_type == INSTALLED ||
	    prod->parent->info.media->med_type == INSTALLED_SVC)
		(void) sprintf(path, "%s/%s/%s/install/depend",
		    get_rootdir(), prod->info.prod->p_pkgdir, info->m_pkg_dir);
	else
		(void) sprintf(path, "%s/%s/install/depend",
		    prod->info.prod->p_pkgdir, info->m_pkg_dir);

	if (path_is_readable(path) == FAILURE)
		return;

	fp = fopen(path, "r");
	info->m_pdepends = info->m_idepends = info->m_rdepends = (Depend *) NULL;
	dp = (Depend *)NULL;

	/* parse out dependency info.  keep three lists, one each for P, I, R
	 * dependenecies.  Remember most recent dependency package (dp) since
	 * we may have to deal with instance specifiers later.
	 */
	while (fgets(buf, BUFSIZ, fp)) {
		/* ignore comment fields and NULL lines */
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		/* strip trailing spaces and the delimiting '\n' */
		for (bp = &buf[strlen(buf) - 2] ; *bp == ' ' && bp >= buf ; bp--)
				;
		bp++;
		*bp = '\0';
		/*
		 * instance specifications for previous depend lines
		 * start with white space 
		 */
		if (buf[0] == ' ' || buf[0] == '\t') {
			bp = buf;
			while (bp && *bp && (*bp == ' ' || *bp == '\t'))
				++bp;

			parse_instance_spec(dp, bp);

	  	} else {
			if (strncmp(buf, "P ", 2) == 0) {
				dpp = &info->m_pdepends;
			} else if (strncmp(buf, "I ", 2) == 0) {
				dpp = &info->m_idepends;
			} else if (strncmp(buf, "R ", 2) == 0) {
				dpp = &info->m_rdepends;
			} else
				continue;
	
			dp = add_depend_instance(dpp, parse_depend_pkgid(buf));
		}
	}
	(void) fclose(fp);
	return;
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * parse_depend_pkgid()
 *	Parse a depend line for the pkgid.
 * Parameters:
 *	buf	- buffer containing line from depend file
 * Return:
 *	char *	- pkgid  string parsed out of 'buf'
 *	NULL 	- invalid string
 * Status:
 *	private
 */
static char *
parse_depend_pkgid(char * buf)
{
	char  *cp = (char *)NULL;
	char  *cp1;

	cp = buf + 2;
	for (cp1 = cp; cp1 && *cp1; cp1++)
		if (*cp1 == ' ' || *cp1 == '\t')
			break;

	if (cp1 && *cp1 && (*cp1 == ' ' || *cp1 == '\t'))
		*cp1 = '\0';

	return (cp);
}

/*
 * add_depend_pkg()
 *	Create a Depend structure, initialize it to the parameter data, and add 
 *	it to the end of the list of of Depend structures pointed to by
 *	'dpnd_pkgs'.
 * Parameters:
 *	dpnd_pkgs	- pointer to existing Depend list, or NULL
 *			  if starting a new list
 *	pkgid		- package ID to initialize the depend structure
 *	pkgidb		- data structure to initialize the depend structure 
 * Return:
 *	Depend *	- pointer to new Depend structure if new depend list,
 *			  or 'dpnd_pkgs' if adding to an existing list
 *	NULL		- xalloc failed
 * Status:
 *	private
 * Note:
 *	alloc routine
 */
static Depend *
add_depend_pkg(Depend * dpnd_pkgs, char *pkgid, char * pkgidb)
{
	Depend	*dp = (Depend *)xcalloc(sizeof(Depend));
	Depend	*tmp, *last;

	/* xalloc check */
	if (dp == (Depend *)NULL)
		return ((Depend *)NULL);

	dp->d_pkgid = pkgid;
	dp->d_pkgidb = (char *)xstrdup(pkgidb);

	if (dpnd_pkgs == (Depend *)NULL)
		dpnd_pkgs = dp;
	else {
		for (last = tmp = dpnd_pkgs; tmp; last = tmp, tmp = tmp->d_next)
			;
		last->d_next = dp;
		dp->d_prev = last;
	}

	return (dpnd_pkgs);
}

/*
 * set_depend_pkgs()
 *	Set the global "dependencies" to the parameter value
 * Parameters:
 *	dp	- depend packages structure pointer
 * Returns:
 *	none
 * Status:
 *	private
 */
static void 
set_depend_pkgs(Depend * dp)
{
	dependencies = dp;
}

