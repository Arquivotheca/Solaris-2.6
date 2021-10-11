/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)setup_network_access.c	1.10	95/03/29 SMI"

/*
 * setup_network_access
 *
 * This subroutine setups everything that a system needs to serve
 * remotely originated printer requests.
 *
 */

#include <stdio.h>
#include <sys/types.h>

#ifndef _B_TRUE
#define _B_TRUE		B_TRUE
#define _B_FALSE	B_FALSE
#endif

#include <sys/stat.h>
#include <string.h>
#include "printer_impl.h"


int
setup_network_access(int num_restart)
{
	int err;
	FILE *result_desc;
	char workbuf[PRT_MAXSTRLEN];
	char msgbuf[PRT_MAXSTRLEN];
	char universal_addr[PRT_UNIVERSAL_ADDR_SIZE+10];
	char *tmpstr;
	boolean_t tcp_portmon_found = _B_FALSE;
	boolean_t listenBSD_found = _B_FALSE;
	boolean_t listenS5_found = _B_FALSE;
	boolean_t listen0_found = _B_FALSE;


	/*
	 * First, check to see if the tcp port monitor
	 * is already configured and enabled:
	 */

	if ((result_desc = popen(PRT_SACADM_LIST, "r")) == NULL) {
		return (PRINTER_ERR_SAC_LIST_FAILED);
	}

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {
		if (strstr(msgbuf, PRT_TCP_PORTMON)) {
			tcp_portmon_found = _B_TRUE;
			break;
		}
	}

	/* consume remaining command output from pipe on break */
	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) ;

	(void) pclose(result_desc);

	/*
	 * If not, we need to create the tcp port monitor
	 * using the Service Access Facility:
	 */
	if (tcp_portmon_found == _B_FALSE) {
		(void) sprintf(workbuf, PRT_SAC_TCP_CREATE" -n %d", num_restart);
		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_TCP_PM_CREATE_FAILED);
		}
	}

	/*
	 * Next, check to see which, if any, services are
	 * already registered with the tcp port monitor:
	 */

	if ((result_desc = popen(PRT_PMADM_LIST, "r")) == NULL) {
		return (PRINTER_ERR_PM_LIST_FAILED);
	}

	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) {
		if (strstr(msgbuf, PRT_LISTENBSD)) {
			listenBSD_found = _B_TRUE;
		} else if (strstr(msgbuf, PRT_LISTENS5)) {
			listenS5_found = _B_TRUE;
		} else if (strstr(msgbuf, PRT_LISTEN0)) {
			listen0_found = _B_TRUE;
		}
	}

	(void) pclose(result_desc);

	/*
	 * If not, we need to create the various listener
	 * services under the tcp port monitor:
	 *
	 * Service for System 5:
	 */
	if (listenS5_found == _B_FALSE) {
		(void) sprintf(workbuf, PRT_PM_LS5, PRT_LP);
		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_LS5_CREATE_FAILED);
		}
	}

	/*
	 * We now need to register the BSD listen service with
	 * the tcp port monitor.
	 *
	 * In order to do that, we need this system's "Universal"
	 * address. We can get that by doing "lpsystem -A"
	 *
	 */

	if ((result_desc = popen(PRT_LPSYSTEM_A, "r")) == NULL) {
		return (PRINTER_ERR_LPSYSTEM_FAILED);
	}

	(void) fgets(universal_addr, PRT_UNIVERSAL_ADDR_SIZE, result_desc);

	/* make sure we have exhausted pipe */
	while (fgets(msgbuf, PRT_MAXSTRLEN, result_desc) != 0) ;

	(void) pclose(result_desc);

	/*
	 * Next, the listen service for BSD:
	 */
	if (listenBSD_found == _B_FALSE) {
		(void) sprintf(workbuf, PRT_PM_LBSD, PRT_LPD, universal_addr);
		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_LBSD_CREATE_FAILED);
		}
	}

	/*
	 * Next, we need to set up Service 0. In order to
	 * do that, we need to massage the universal address
	 * by replacing port 0203 with port 0ace
	 */

	if (listen0_found == _B_FALSE) {
		tmpstr = strstr(universal_addr, PRT_LINEPRINTER_PORT_NUM);

		(void) strncpy(msgbuf, universal_addr, 4);
		msgbuf[4] = '\0';
		(void) strcat(msgbuf, PRT_SERVICE0_PORT_NUM);
		(void) strcat(msgbuf, (tmpstr+4));

		(void) sprintf(workbuf, PRT_PM_L0, PRT_SVC0, msgbuf);

		err = do_system(workbuf);

		if (err != PRINTER_SUCCESS) {
			return (PRINTER_ERR_L0_CREATE_FAILED);
		}
	}
	return (PRINTER_SUCCESS);
}
