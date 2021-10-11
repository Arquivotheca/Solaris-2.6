/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)serial_impl.c	1.23	95/05/19 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "sysman_impl.h"
#include "serial_iface.h"


static char	*device_prefix = "/dev/term";


int
_root_modify_serial(void *arg_p, char *buf, int len)
{

	int		status;
	int		retval;
	char		ttyflags_buf[32];
	char		portflags_buf[32];
	char		device[PATH_MAX];
	SysmanSerialArg	*sa_p = (SysmanSerialArg *)arg_p;
	PmtabInfo	pmi;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	pmi.pmtag_key = sa_p->pmtag_key;
	pmi.svctag_key = sa_p->svctag_key;
	pmi.pmtag = sa_p->pmtag;
	pmi.svctag = sa_p->svctag;

	/* pmi.pmtype = ; */

	if (sa_p->portflags != NULL) {
		strcpy(portflags_buf, sa_p->portflags);
	} else {
		portflags_buf[0] = '\0';
	}
	if (sa_p->create_utmp_entry == B_TRUE) {
		strcat(portflags_buf, "u");
	}
	if (sa_p->service_enabled == s_disabled) {
		strcat(portflags_buf, "x");
	}
	pmi.portflags = portflags_buf;

	pmi.identity = sa_p->identity;

	if (sa_p->device == NULL) {
		/* supply a reasonable default */
		sprintf(device, "%s/%s", device_prefix, sa_p->port);
	} else {
		strcpy(device, sa_p->device);
	}
	pmi.device = device;

	if (sa_p->ttyflags != NULL) {
		strcpy(ttyflags_buf, sa_p->ttyflags);
	} else {
		ttyflags_buf[0] = '\0';
	}
	if (sa_p->initialize_only == B_TRUE) {
		strcat(ttyflags_buf, "I");
	}
	if (sa_p->bidirectional == B_TRUE) {
		strcat(ttyflags_buf, "b");
	}
	if (sa_p->connect_on_carrier == B_TRUE) {
		strcat(ttyflags_buf, "c");
	}
	pmi.ttyflags = ttyflags_buf;

	/* pmi.count = ; */
	pmi.service = sa_p->service;
	pmi.timeout = sa_p->timeout;
	pmi.ttylabel = sa_p->baud_rate;
	pmi.modules = sa_p->modules;
	pmi.prompt = sa_p->prompt;
	/* pmi.disable = ; */
	pmi.termtype = sa_p->termtype;
	pmi.softcar = sa_p->softcar == B_FALSE ? "n" : "y";
	pmi.comment = sa_p->comment;
	pmi.port = sa_p->port;

	status = modify_modem(&pmi);

	switch (status) {
	case SERIAL_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case SERIAL_ERR_IS_CONSOLE:
		retval = SYSMAN_SERIAL_ERR_IS_CONSOLE;
		break;
	case SERIAL_ERR_ADD_SACADM:
		retval = SYSMAN_SERIAL_ERR_START_PORTMON;
		break;
	case SERIAL_ERR_ENABLE_PORTMON:
		retval = SYSMAN_SERIAL_ERR_ENABLE_PORTMON;
		break;
	case SERIAL_ERR_DO_EEPROM:
		retval = SYSMAN_SERIAL_ERR_EEPROM;
		break;
	case SERIAL_ERR_GET_STTYDEFS:
		retval = SYSMAN_SERIAL_ERR_TTYDEFS;
		break;
	case SERIAL_ERR_FORK_PROC_TTYADM:
	case SERIAL_ERR_TTYADM:
		retval = SYSMAN_SERIAL_ERR_TTYADM;
		break;
	case SERIAL_ERR_CHECK_PORTMON:
	default:
		retval = SYSMAN_SERIAL_FAILED;
		break;
	}

	return (retval);
}


int
_root_enable_serial(void *arg_p, char *buf, int len)
{

	int		status;
	int		retval;
	char		ttyflags_buf[32];
	char		portflags_buf[32];
	char		device[PATH_MAX];
	SysmanSerialArg	*sa_p = (SysmanSerialArg *)arg_p;
	PmtabInfo	pmi;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	pmi.pmtag_key = sa_p->pmtag_key;
	pmi.svctag_key = sa_p->svctag_key;
	pmi.pmtag = sa_p->pmtag;
	pmi.svctag = sa_p->svctag;

	/* pmi.pmtype = ; */

	if (sa_p->portflags != NULL) {
		strcpy(portflags_buf, sa_p->portflags);
	} else {
		portflags_buf[0] = '\0';
	}
	if (sa_p->create_utmp_entry == B_TRUE) {
		strcat(portflags_buf, "u");
	}
	if (sa_p->service_enabled == s_disabled) {
		strcat(portflags_buf, "x");
	}
	pmi.portflags = portflags_buf;

	pmi.identity = sa_p->identity;

	if (sa_p->device == NULL) {
		/* supply a reasonable default */
		sprintf(device, "%s/%s", device_prefix, sa_p->port);
	} else {
		strcpy(device, sa_p->device);
	}
	pmi.device = device;

	if (sa_p->ttyflags != NULL) {
		strcpy(ttyflags_buf, sa_p->ttyflags);
	} else {
		ttyflags_buf[0] = '\0';
	}
	if (sa_p->initialize_only == B_TRUE) {
		strcat(ttyflags_buf, "I");
	}
	if (sa_p->bidirectional == B_TRUE) {
		strcat(ttyflags_buf, "b");
	}
	if (sa_p->connect_on_carrier == B_TRUE) {
		strcat(ttyflags_buf, "c");
	}
	pmi.ttyflags = ttyflags_buf;

	/* pmi.count = ; */
	pmi.service = sa_p->service;
	pmi.timeout = sa_p->timeout;
	pmi.ttylabel = sa_p->baud_rate;
	pmi.modules = sa_p->modules;
	pmi.prompt = sa_p->prompt;
	/* pmi.disable = ; */
	pmi.termtype = sa_p->termtype;
	pmi.softcar = sa_p->softcar == B_FALSE ? "n" : "y";
	pmi.comment = sa_p->comment;
	pmi.port = sa_p->port;

	status = enable_modem(&pmi);

	switch (status) {
	case SERIAL_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case SERIAL_ERR_IS_CONSOLE:
		retval = SYSMAN_SERIAL_ERR_IS_CONSOLE;
		break;
	case SERIAL_ERR_ADD_SACADM:
		retval = SYSMAN_SERIAL_ERR_START_PORTMON;
		break;
	case SERIAL_ERR_ENABLE_PORTMON:
		retval = SYSMAN_SERIAL_ERR_ENABLE_PORTMON;
		break;
	case SERIAL_ERR_DO_EEPROM:
		retval = SYSMAN_SERIAL_ERR_EEPROM;
		break;
	case SERIAL_ERR_GET_STTYDEFS:
		retval = SYSMAN_SERIAL_ERR_TTYDEFS;
		break;
	case SERIAL_ERR_FORK_PROC_TTYADM:
	case SERIAL_ERR_TTYADM:
		retval = SYSMAN_SERIAL_ERR_TTYADM;
		break;
	case SERIAL_ERR_CHECK_PORTMON:
	default:
		retval = SYSMAN_SERIAL_FAILED;
		break;
	}

	return (retval);
}


int
_root_disable_serial(void *arg_p, char *buf, int len)
{

	int		status;
	int		retval;
	char		ttyflags_buf[32];
	char		portflags_buf[32];
	char		device[PATH_MAX];
	SysmanSerialArg	*sa_p = (SysmanSerialArg *)arg_p;
	PmtabInfo	pmi;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	pmi.pmtag_key = sa_p->pmtag_key;
	pmi.svctag_key = sa_p->svctag_key;
	pmi.pmtag = sa_p->pmtag;
	pmi.svctag = sa_p->svctag;

	/* pmi.pmtype = ; */

	if (sa_p->portflags != NULL) {
		strcpy(portflags_buf, sa_p->portflags);
	} else {
		portflags_buf[0] = '\0';
	}
	if (sa_p->create_utmp_entry == B_TRUE) {
		strcat(portflags_buf, "u");
	}
	if (sa_p->service_enabled == s_disabled) {
		strcat(portflags_buf, "x");
	}
	pmi.portflags = portflags_buf;

	pmi.identity = sa_p->identity;

	if (sa_p->device == NULL) {
		/* supply a reasonable default */
		sprintf(device, "%s/%s", device_prefix, sa_p->port);
	} else {
		strcpy(device, sa_p->device);
	}
	pmi.device = device;

	if (sa_p->ttyflags != NULL) {
		strcpy(ttyflags_buf, sa_p->ttyflags);
	} else {
		ttyflags_buf[0] = '\0';
	}
	if (sa_p->initialize_only == B_TRUE) {
		strcat(ttyflags_buf, "I");
	}
	if (sa_p->bidirectional == B_TRUE) {
		strcat(ttyflags_buf, "b");
	}
	if (sa_p->connect_on_carrier == B_TRUE) {
		strcat(ttyflags_buf, "c");
	}
	pmi.ttyflags = ttyflags_buf;

	/* pmi.count = ; */
	pmi.service = sa_p->service;
	pmi.timeout = sa_p->timeout;
	pmi.ttylabel = sa_p->baud_rate;
	pmi.modules = sa_p->modules;
	pmi.prompt = sa_p->prompt;
	/* pmi.disable = ; */
	pmi.termtype = sa_p->termtype;
	pmi.softcar = sa_p->softcar == B_FALSE ? "n" : "y";
	pmi.comment = sa_p->comment;
	pmi.port = sa_p->port;

	status = disable_modem(&pmi);

	switch (status) {
	case SERIAL_SUCCESS:
		retval = SYSMAN_SUCCESS;
		break;
	case SERIAL_ERR_IS_CONSOLE:
		retval = SYSMAN_SERIAL_ERR_IS_CONSOLE;
		break;
	default:
		retval = SYSMAN_SERIAL_FAILED;
		break;
	}

	return (retval);
}


int
_root_delete_serial(void *arg_p, char *buf, int len)
{

	int		status;
	SysmanSerialArg	*sa_p = (SysmanSerialArg *)arg_p;


	if (sa_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	status = delete_modem(sa_p->pmtag_key, sa_p->svctag_key);

	if (status == 0) {
		return (SYSMAN_SUCCESS);
	} else {
		return (SYSMAN_SERIAL_FAILED);
	}
}


int
_get_serial(
	SysmanSerialArg	*sa_p,
	const char	*alt_dev_dir,
	char		*buf,
	int		len)
{

	int		i;
	int		cnt;
	int		status;
	char		tmpstr[128];
	SysmanSerialArg *sal_p;


	/*
	 * NOTE -- svctag_key is not a required arg, we might be
	 * getting info about a port that doesn't have a service
	 * configured for it.
	 */

	if (sa_p == NULL || sa_p->port == NULL || sa_p->pmtag_key == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	cnt = _list_serial(&sal_p, alt_dev_dir, buf, len);

	if (cnt < 0) {
		switch (cnt) {
		case M_MALLOC_FAIL:
			return (SYSMAN_MALLOC_ERR);
			break;
		case SERIAL_ERR_PMADM:
			return (SYSMAN_SERIAL_ERR_PMADM);
			break;
		case SERIAL_FAILURE:
		default:
			return (SYSMAN_SERIAL_FAILED);
			break;
		}
	}

	status = SYSMAN_SERIAL_ERR_NOTFOUND;

	for (i = 0; i < cnt; i++) {

		/*
		 * Testing the svctag in the structure returned by the
		 * _list_serial call will indicate whether the service
		 * configured on the port is active or inactive -- if
		 * the svctag is NULL, the service is inactive.
		 * The criteria for a "match" for this _get call are
		 * as follow:
		 * If the service is active, the requested pmtag_key
		 * must match the pmtag and the requested svctag_key
		 * must match the svctag.
		 * If the service is inactive, nothing needs to match,
		 * you have a template inactive service specification
		 * for the port.
		 */

		if (strcmp(sa_p->port, sal_p[i].port) == 0) {

			if ((sal_p[i].svctag == NULL) ||
			    (strcmp(sa_p->pmtag_key, sal_p[i].pmtag) == 0 &&
			    strcmp(sa_p->svctag_key, sal_p[i].svctag) == 0)) {

				sa_p->port = strdup(sa_p->port);
				sa_p->pmtag = sal_p[i].pmtag ?
				    strdup(sal_p[i].pmtag) : NULL;
				sa_p->pmtype = sal_p[i].pmtype ?
				    strdup(sal_p[i].pmtype) : NULL;

				if (sal_p[i].svctag != NULL) {
					sa_p->svctag = strdup(sal_p[i].svctag);
				} else {
					sprintf(tmpstr, "tty%s", sa_p->port);
					sa_p->svctag = strdup(tmpstr);
				}

				sa_p->identity = sal_p[i].identity ?
				    strdup(sal_p[i].identity) : NULL;
				sa_p->portflags = sal_p[i].portflags ?
				    strdup(sal_p[i].portflags) : NULL;
				sa_p->comment = sal_p[i].comment ?
				    strdup(sal_p[i].comment) : NULL;
				sa_p->ttyflags = sal_p[i].ttyflags ?
				    strdup(sal_p[i].ttyflags) : NULL;
				sa_p->modules = sal_p[i].modules ?
				    strdup(sal_p[i].modules) : NULL;
				sa_p->prompt = sal_p[i].prompt ?
				    strdup(sal_p[i].prompt) : NULL;
				sa_p->termtype = sal_p[i].termtype ?
				    strdup(sal_p[i].termtype) : NULL;
				sa_p->service = sal_p[i].service ?
				    strdup(sal_p[i].service) : NULL;
				sa_p->device = sal_p[i].device ?
				    strdup(sal_p[i].device) : NULL;
				sa_p->baud_rate = sal_p[i].baud_rate ?
				    strdup(sal_p[i].baud_rate) : NULL;
				sa_p->timeout = sal_p[i].timeout ?
				    strdup(sal_p[i].timeout) : NULL;

				sa_p->softcar = sal_p[i].softcar;
				sa_p->service_enabled =
				    sal_p[i].service_enabled;
				sa_p->create_utmp_entry =
				    sal_p[i].create_utmp_entry;
				sa_p->initialize_only =
				    sal_p[i].initialize_only;
				sa_p->bidirectional = sal_p[i].bidirectional;
				sa_p->connect_on_carrier =
				    sal_p[i].connect_on_carrier;

				status = SYSMAN_SUCCESS;

				break;
			}
		}
	}

	return (status);
}


/*
 * helper function for list_serial
 * takes the pmadm output returned from the list_modem call and
 * pulls the pieces out, stuffing them into a serial arg structure.
 * lifted from libadmobjs/SerialPortServiceSnag.cc.
 */

static
void
crack_pmadm(char *bufp, PmtabInfo *pm)
{

	char		**sp = (char **) &pm->pmtag;
	register char  *cp;
	int		field_num = 1;

	*sp++ = cp = bufp;
	while (field_num < NUM_FIELDS_IN_PMADM_OUTPUT)
		switch (*cp) {
		case ':':
			*cp++ = '\0';	/* replace with a NULL */
			field_num++;
			*sp++ = cp;	/* next element starts after null */
			break;
		case '\\':
			if (*(cp + 1) == ':') {
				char	*holder = cp + 1;

				*cp++ = ':';
				/* shift remainder to the 'left' */
				while (*cp) {
					*cp = *(cp + 1);
					cp++;
				}
				cp = holder;
			} else
				cp++;
			break;
		default:
			cp++;
		}
	/*
	 * The "comment" field has a trailing newline and always has a "#"
	 * at its start.
	 */
	*(strchr(cp, '\n')) = '\0';	/* null terminate */
	(pm->comment)++;		/* skip past the '#' */
	/* If no comment is supplied to pmadm it still returns "# " */
	if (*pm->comment == ' ')
		(pm->comment)++;
}


static
void
init_inactive(SysmanSerialArg *sa_p)
{

	char	t[64];


	if (sa_p == NULL) {
		return;
	}

	sa_p->pmtag = strdup("zsmon");
	sa_p->pmtype = strdup("ttymon");

	sa_p->portflags = NULL;
	sa_p->identity = strdup("root");

	if (sa_p->port != NULL) {
	    sprintf(t, "/dev/term/%s", sa_p->port);
	    sa_p->device = strdup(t);
	} else {
	    sa_p->device = NULL;
	}
	sa_p->ttyflags = NULL;
	sa_p->service = strdup("/usr/bin/login");
	sa_p->timeout = strdup("");
	sa_p->baud_rate = strdup("9600");
	sa_p->modules = strdup("ldterm,ttcompat");

	if (sa_p->svctag != NULL) {
		sprintf(t, "%s login: ", sa_p->svctag);
	} else {
		strcpy(t, "login: ");
	}
	sa_p->prompt = strdup(t);

	sa_p->termtype = strdup("");
	sa_p->softcar = B_FALSE;
	sa_p->comment = strdup("");

	sa_p->service_enabled = s_inactive;
	sa_p->create_utmp_entry = B_TRUE;

	sa_p->initialize_only = B_FALSE;
	sa_p->bidirectional = B_TRUE;
	sa_p->connect_on_carrier = B_FALSE;
}


int
_list_serial(
	SysmanSerialArg	**sa_pp,
	const char	*alt_dev_dir,
	char		*buf,
	int		len)
{

	ModemInfo	*mi_p;
	int		i;
	int		cnt;
	PmtabInfo	pmi;


	if (sa_pp == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	cnt = list_modem(&mi_p, alt_dev_dir);

	if (cnt == 0) {
		switch (cnt) {
		case M_MALLOC_FAIL:
			return (SYSMAN_MALLOC_ERR);
			break;
		case SERIAL_ERR_PMADM:
			return (SYSMAN_SERIAL_ERR_PMADM);
			break;
		case SERIAL_FAILURE:
		default:
			return (SYSMAN_SERIAL_FAILED);
			break;
		}
	}

	*sa_pp = (SysmanSerialArg *)malloc((unsigned)(cnt *
	    sizeof (SysmanSerialArg)));

	if (*sa_pp == NULL) {
		return (SYSMAN_MALLOC_ERR);
	}

	memset((void *)*sa_pp, '\0', cnt * sizeof (SysmanSerialArg));

	for (i = 0; i < cnt; i++) {

		(*sa_pp)[i].pmtag_key = NULL;
		(*sa_pp)[i].svctag_key = NULL;
		(*sa_pp)[i].port = mi_p[i].port ? strdup(mi_p[i].port) : NULL;
		(*sa_pp)[i].pmadm =
		    mi_p[i].pmadm_info ? strdup(mi_p[i].pmadm_info) : NULL;

		if ((*sa_pp)[i].pmadm == NULL || (*sa_pp)[i].pmadm[0] == '\0') {

			/*
			 * This port doesn't have a service on it; fill it
			 * with prototype/default data, set the "enabled"
			 * state variable to inactive, and continue on with
			 * the next port.
			 */

			init_inactive(&((*sa_pp)[i]));

			continue;
		}

		memset((void *)&pmi, '\0', sizeof (pmi));
		crack_pmadm(mi_p[i].pmadm_info, &pmi);

		(*sa_pp)[i].pmtag = pmi.pmtag ? strdup(pmi.pmtag) : NULL;
		(*sa_pp)[i].pmtype = pmi.pmtype ? strdup(pmi.pmtype) : NULL;
		(*sa_pp)[i].svctag = pmi.svctag ? strdup(pmi.svctag) : NULL;
		(*sa_pp)[i].portflags =
		    pmi.portflags ? strdup(pmi.portflags) : NULL;
		(*sa_pp)[i].identity =
		    pmi.identity ? strdup(pmi.identity) : NULL;
		(*sa_pp)[i].device = pmi.device ? strdup(pmi.device) : NULL;
		(*sa_pp)[i].ttyflags =
		    pmi.ttyflags ? strdup(pmi.ttyflags) : NULL;
		(*sa_pp)[i].service = pmi.service ? strdup(pmi.service) : NULL;
		(*sa_pp)[i].timeout = pmi.timeout ? strdup(pmi.timeout) : NULL;
		(*sa_pp)[i].baud_rate =
		    pmi.ttylabel ? strdup(pmi.ttylabel) : NULL;
		(*sa_pp)[i].modules = pmi.modules ? strdup(pmi.modules) : NULL;
		(*sa_pp)[i].prompt = pmi.prompt ? strdup(pmi.prompt) : NULL;
		(*sa_pp)[i].termtype =
		    pmi.termtype ? strdup(pmi.termtype) : NULL;
		(*sa_pp)[i].softcar =
		    (pmi.softcar[0] == 'y') ? B_TRUE : B_FALSE;
		(*sa_pp)[i].comment = pmi.comment ? strdup(pmi.comment) : NULL;

		if ((*sa_pp)[i].portflags != NULL) {
			if (strchr((*sa_pp)[i].portflags, 'x') == NULL) {
				(*sa_pp)[i].service_enabled = s_enabled;
			} else {
				(*sa_pp)[i].service_enabled = s_disabled;
			}
			if (strchr((*sa_pp)[i].portflags, 'u') == NULL) {
				(*sa_pp)[i].create_utmp_entry = B_FALSE;
			} else {
				(*sa_pp)[i].create_utmp_entry = B_TRUE;
			}
		} else {
			(*sa_pp)[i].service_enabled = s_disabled;
			(*sa_pp)[i].create_utmp_entry = B_FALSE;
		}

		if ((*sa_pp)[i].ttyflags != NULL) {
			if (strchr((*sa_pp)[i].ttyflags, 'I') == NULL) {
				(*sa_pp)[i].initialize_only = B_FALSE;
			} else {
				(*sa_pp)[i].initialize_only = B_TRUE;
			}
			if (strchr((*sa_pp)[i].ttyflags, 'b') == NULL) {
				(*sa_pp)[i].bidirectional = B_FALSE;
			} else {
				(*sa_pp)[i].bidirectional = B_TRUE;
			}
			if (strchr((*sa_pp)[i].ttyflags, 'c') == NULL) {
				(*sa_pp)[i].connect_on_carrier = B_FALSE;
			} else {
				(*sa_pp)[i].connect_on_carrier = B_TRUE;
			}
		} else {
			(*sa_pp)[i].initialize_only = B_FALSE;
			(*sa_pp)[i].bidirectional = B_FALSE;
			(*sa_pp)[i].connect_on_carrier = B_FALSE;
		}
	}

	free((void *)mi_p);

	return (cnt);
}


void
_free_serial(SysmanSerialArg *sa_p)
{
	if (sa_p->port != NULL) {
		free((void *)sa_p->port);
	}
	if (sa_p->pmtag != NULL) {
		free((void *)sa_p->pmtag);
	}
	if (sa_p->pmtype != NULL) {
		free((void *)sa_p->pmtype);
	}
	if (sa_p->svctag != NULL) {
		free((void *)sa_p->svctag);
	}
	if (sa_p->identity != NULL) {
		free((void *)sa_p->identity);
	}
	if (sa_p->portflags != NULL) {
		free((void *)sa_p->portflags);
	}
	if (sa_p->comment != NULL) {
		free((void *)sa_p->comment);
	}
	if (sa_p->ttyflags != NULL) {
		free((void *)sa_p->ttyflags);
	}
	if (sa_p->modules != NULL) {
		free((void *)sa_p->modules);
	}
	if (sa_p->prompt != NULL) {
		free((void *)sa_p->prompt);
	}
	if (sa_p->termtype != NULL) {
		free((void *)sa_p->termtype);
	}
	if (sa_p->service != NULL) {
		free((void *)sa_p->service);
	}
	if (sa_p->device != NULL) {
		free((void *)sa_p->device);
	}
	if (sa_p->baud_rate != NULL) {
		free((void *)sa_p->baud_rate);
	}
	if (sa_p->timeout != NULL) {
		free((void *)sa_p->timeout);
	}
}


void
_free_serial_list(SysmanSerialArg *sa_p, int cnt)
{

	int	i;


	if (sa_p == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		_free_serial(sa_p + i);
	}

	free((void *)sa_p);
}
