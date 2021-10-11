#ifndef lint
#pragma ident "@(#)v_pfg_sw.c 1.19 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_pfg_sw.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

static pfSw_t *cluster_head = NULL;
static pfSw_t *package_head = NULL;

static void clearNonnativeArches();
static void resetModuleSelects(pfSw_t * sw);

static Module *CurrentCluster = 0;

pfErCode
pfLoadCD(void)
{
	int status;

	/* load software modules */
	set_default(add_media(MEDIANAME(pfProfile)));
	if ((status = load_media(NULL, TRUE)) != SUCCESS)
		switch (status) {  /* conversion to retval (used to printf) */
		case ERR_NOMEDIA:
			return (pfErNOMEDIA);
		case ERR_INVALIDTYPE:
			return (pfErBADMEDIA);
		case ERR_UMOUNTED:
			return (pfErMOUNTMEDIA);
		case ERR_NOPROD:
			return (pfErPRODUCTMEDIA);
		default:
			return (pfErMEDIA);
	}
	return (pfOK);
}

/*
 * function to Nullout the package and cluster lists
 * which are maintained as deltas from the selected
 * metacluster
 */
void
pfNullPackClusterLists()
{
	cluster_head = NULL;
	package_head = NULL;
}


pfSw_t *
pfGetClusterList()
{
	return (cluster_head);
}

pfSw_t *
pfGetPackageList()
{
	return (package_head);
}

pfSw_t **
pfGetClusterListPtr()
{
	return (&cluster_head);
}

pfSw_t **
pfGetPackageListPtr()
{
	return (&package_head);
}

int
pfSW_add(pfSw_t ** head, pfSw_t ** curr, Module * module)
{

	if ((*head) == NULL) {
		*curr = (pfSw_t *) xmalloc(sizeof (pfSw_t));
		*head = *curr;
	} else {
		(*curr)->next = (pfSw_t *) xmalloc(sizeof (pfSw_t));
		*curr = (*curr)->next;
	}
	if (*curr == NULL) {
		return (FAILURE);
	}
	(*curr)->next = NULL;
	(*curr)->name = xstrdup(module->info.mod->m_pkgid);
	(*curr)->mod = module;
	(*curr)->delta = (module->info.mod->m_status == SELECTED) ?
	    PF_ADD : PF_REMOVE;

	return (SUCCESS);
}

int
getModuleSize(Module * module, ModStatus status)
{
	FSspace **space;
	int i, total = 0;
	ModStatus moduleStatus;

	moduleStatus = mod_status(module);
	if (moduleStatus == SELECTED || moduleStatus == PARTIALLY_SELECTED ||
	    moduleStatus == REQUIRED || status == UNSELECTED) {
		space = calc_cluster_space(module, status);

		for (i = 0; space[i] != NULL; i++) {
			total = total + space[i]->fsp_reqd_contents_space;
		}
	}
	return (total);
}


/*
 * get fullpath to pkg: need to trim off leading path to get to pkgid
 * if pkgid has an extension (XXX.[cmde]), need to chop this off too.
 */
char *
pkgid_from_pkgdir(char *path)
{
	char *cp;
	static char buf[64];

	/*
	 * path = "/blah/blah/blah/pkgdir[.{c|e|d|m|...}]
	 *						 ^
	 *						 |
	 * cp	--------------------+
	 */

	/* put into private buffer, and trim off any extension */

	if (path && (cp = strrchr(path, '/')))
		(void) strcpy(buf, cp+1);
	else
		(void) strcpy(buf, path);

	if (cp = strrchr(buf, '.'))
	*cp = '\0';

	return (buf);
}


int
get_total_kb_to_install(void)
{
	FSspace	**spaceinfo;
	uint	sum;
	int	i;
	Module	*mod;

	/*
	 * determine # of KB to be installed , find all metacluster to make
	 * sure that *all* packages are looked at by the space code.
	 */
	/*
	 * Do NOT assume that SUNWCall is the ALL metacluster.
	for (mod = get_current_metacluster();
		mod && (strcmp(mod->info.mod->m_pkgid, ALL_METACLUSTER) != 0);
		mod = get_next(mod));

	if (mod == (Module *) NULL)
		mod = get_current_metacluster();
	 */
	mod = get_head(get_current_metacluster());

	spaceinfo = calc_cluster_space(mod, SELECTED);

	/* add up Kbytes used in each file system */
	for (sum = 0, i = 0; spaceinfo[i];
	    sum += spaceinfo[i]->fsp_reqd_contents_space, i++);

	return (sum);
}


int
get_size_in_kbytes(char *pkgid)
{
	Module *prod = get_current_product();
	List   *pkgs = prod->info.prod->p_packages;
	Node   *tmp = findnode(pkgs, pkgid);

	if (tmp && ((Modinfo *) tmp->data != (Modinfo *) NULL))
		return ((int) tot_pkg_space((Modinfo *)tmp->data));
	else
		return (0);
}


/*
 * function to reset the status of the modules selected/deselected from the
 * customize software screen.  This function is called by pfgGetMetaSize to
 * reset these modules because the selections are lost due to the necessity
 * of selected a meta cluster to determine the actual disk space need to
 * install the meta cluster including file system overhead.
 */

void
resetPackClustSelects()
{
	pfSw_t *package, *cluster;

	cluster = pfGetClusterList();
	package = pfGetPackageList();

	resetModuleSelects(cluster);
	resetModuleSelects(package);
}

/*
 * reset the module status customized from the customize software screen
 */
static void
resetModuleSelects(pfSw_t * sw)
{

	for (; sw; sw = sw->next) {
		if (sw->delta) {
			mark_module(sw->mod, SELECTED);
		} else {
			mark_module(sw->mod, UNSELECTED);
		}
		sw->mod->info.mod->m_status = mod_status(sw->mod);
	}
}


pfErCode
pfInitializeSw()
{
	Module *m;
	Module *prod;
	static int first = True;

	write_debug(GUI_DEBUG_L1, "Entering pfInitializeSw");

	prod = get_current_product();

	if (first == True) {
		set_instdir_svc_svr(prod);
		first = False;
	}

	sw_lib_init(NULL);
	set_percent_free_space(15);

	for (m = prod->sub; m; m = m->next)
		if (strcmp(m->info.mod->m_pkgid, REQD_METACLUSTER) == 0) {
			mark_required(m);
			set_current(m);
			break;
		} else {
			mark_module(m, UNSELECTED);
		}

	/*
	 * set default meta cluster of end user if this is the first
	 * time through, otherwise, set to current metacluster.
	 */
	if (CurrentCluster == NULL) {
		for (m = prod->sub; m; m = m->next) {
			if (strcmp(m->info.mod->m_pkgid,
					ENDUSER_METACLUSTER) == 0) {
				pfSetMetaCluster(m);
				break;
			}
		}
	} else {
		pfSetMetaCluster(CurrentCluster);
	}

	return (pfOK);
}

Module *
pfGetCurrentMeta()
{
	return (CurrentCluster);
}

void
pfSetMetaCluster(Module * module)
{
	if (CurrentCluster != NULL) {
		CurrentCluster->info.mod->m_status = UNSELECTED;
		mark_module(CurrentCluster, UNSELECTED);
		/*
		 * clear modifications of previous meta cluster
		 */
		pfNullPackClusterLists();
	}
	module->info.mod->m_status = SELECTED;
	mark_module(module, SELECTED);
	set_current(module);
	set_default(module);
	CurrentCluster = module;
}

/* select default locale */
int
setDefaultLocale(char *loc)
{
	Module *prod = get_current_product();

	if (loc == (char *) NULL)
		return (FAILURE);
	if (prod == (Module *) NULL)
		return (FAILURE);

	if (select_locale(prod, loc) == SUCCESS) {

		return (SUCCESS);

	} else
		return (FAILURE);
}

/*
 * function to set system type. Called when user changes system type from
 * within the system screen and by main at initialization time
 */

static int NumClients = DEFAULT_NUMBER_OF_CLIENTS;
static int ClientSwap = DEFAULT_SWAP_PER_CLIENT;
static int ClientRoot = DEFAULT_ROOT_PER_CLIENT;

void
setSystemType(MachineType type)
{
	Module *prod;

	switch (type) {
	case MT_SERVER:
		saveDefaultMountList(MT_SERVER);
		set_machinetype(MT_SERVER);
		set_client_space(NumClients, mb_to_sectors(15),
			mb_to_sectors(ClientSwap));
		break;
	default:
		clearNonnativeArches();
		set_machinetype(MT_STANDALONE);
		saveDefaultMountList(MT_STANDALONE);
		break;
	}
	prod = get_current_product();
	set_action_for_machine_type(prod);

}

int
getNumClients()
{
	return (NumClients);
}

int
getSwapPerClient()
{
	return (ClientSwap);
}

int
getRootPerClient()
{
	return (ClientRoot);
}


void
setNumClients(int numClients)
{
	NumClients = numClients;
	set_client_space(NumClients, mb_to_sectors(ClientRoot),
		mb_to_sectors(ClientSwap));
}

void
setRootPerClient(int root)
{
	ClientRoot = root;
	set_client_space(NumClients, mb_to_sectors(ClientRoot),
		mb_to_sectors(ClientSwap));
}

void
setSwapPerClient(int swap)
{
	ClientSwap = swap;
	set_client_space(NumClients, mb_to_sectors(ClientRoot),
		mb_to_sectors(ClientSwap));
}


static void
clearNonnativeArches(void)
{
	Arch *ptr, *arch;
	char *nativeArch;
	Module *prod = get_current_product();

	nativeArch = get_default_arch();
	arch = get_all_arches(NULL);

	for (ptr = arch; ptr != NULL; ptr = ptr->a_next) {
		if (strcmp(nativeArch, ptr->a_arch) != 0)
			deselect_arch(prod, ptr->a_arch);
	}
}


char *
pfPackagename(char *pkgid)
{
	Node *np;
	Module *prod = get_current_product();
	static char buf[256];

	buf[0] = '\0';

	if (np = findnode((List *) (prod->info.prod->p_packages), pkgid))
		(void) strncpy(buf, ((Modinfo *) np->data)->m_name, 255);

	return (buf);
}

char *
pfClustername(char *pkgid)
{
	Node *np;
	Module *prod = get_current_product();
	static char buf[256];

	buf[0] = '\0';

	if (np = findnode((List *) (prod->info.prod->p_clusters), pkgid))
		(void) strncpy(buf,
		    ((Module *) np->data)->info.mod->m_name, 255);

	return (buf);
}
