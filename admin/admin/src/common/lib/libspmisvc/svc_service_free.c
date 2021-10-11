#ifndef lint
#pragma ident "@(#)svc_service_free.c 1.2 96/01/09 SMI"
#endif
/*
 * Copyright (c) 1994 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#include "spmicommon_lib.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"
#include <stdlib.h>

/* Public Prototype Specifications */

void	free_service_list(SW_service_list *);
void	free_error_info(SW_error_info *);
void	free_createroot_info(SW_createroot_info *);

/* Private Prototype Specifications */

static void	free_service(SW_service *);

void
free_service_list(SW_service_list *svc_list)
{
	SW_service	*cursvc, *nextone;

	cursvc = svc_list->sw_svl_services;
	while (cursvc) {
		nextone = cursvc->next;
		free_service(cursvc);
		free(cursvc);
		cursvc = nextone;
	}
	svc_list->sw_svl_services = NULL;
	free(svc_list);
}

void
free_platform_list(StringList *platlist)
{
	StringListFree(platlist);
}

void
free_error_info(SW_error_info *err_info)
{
	switch (err_info->sw_error_code) {
	  case SW_INSUFFICIENT_SPACE:
		free_final_space_report(err_info->sw_space_results);
		break;

	}
	free(err_info);
}

void
free_createroot_info(SW_createroot_info *cri)
{
	SW_pkgadd_def	*nextpkg, *pkg;
	SW_remmnt	*nextrmnt, *rmnt;

	pkg = cri->sw_root_packages;
	cri->sw_root_packages = NULL;
	while (pkg) {
		nextpkg = pkg->next;
		if (pkg->sw_pkg_dir) {
			free(pkg->sw_pkg_dir);
			pkg->sw_pkg_dir = NULL;
		}
		if (pkg->sw_pkg_name) {
			free(pkg->sw_pkg_name);
			pkg->sw_pkg_name = NULL;
		}
		free(pkg);
		pkg = nextpkg;
	}
	rmnt = cri->sw_root_remmnt;
	cri->sw_root_remmnt = NULL;
	while (rmnt) {
		nextrmnt = rmnt->next;
		if (rmnt->sw_remmnt_mntpnt) {
			free(rmnt->sw_remmnt_mntpnt);
			rmnt->sw_remmnt_mntpnt = NULL;
		}
		if (rmnt->sw_remmnt_mntdir) {
			free(rmnt->sw_remmnt_mntdir);
			rmnt->sw_remmnt_mntdir = NULL;
		}
		free(rmnt);
		rmnt = nextrmnt;
	}
	free(cri);
}

static void
free_service(SW_service *svc)
{
	free(svc->sw_svc_os);
	svc->sw_svc_os = NULL;
	free(svc->sw_svc_version);
	svc->sw_svc_version = NULL;
	free(svc->sw_svc_isa);
	svc->sw_svc_isa = NULL;
	free(svc->sw_svc_plat);
	svc->sw_svc_plat = NULL;
}
