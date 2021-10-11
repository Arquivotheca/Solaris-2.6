/*
 * Copyright (c) 1994, 1995, 1996 by Sun Microsystems, Inc.
 * All Rights Reserved
 */

#pragma ident "@(#)ns_cvt_nisplus.c	1.3	96/04/22 SMI"

/*
 * This file contains code that converts a xfn reference into a printer object
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <xfn/xfn.h>
#include <rpc/rpc.h>

#include <print/ns.h>
#include <print/list.h>
#include <print/misc.h>


#define	PREFIX		"onc_printer_"


/*
 * FUNCTION:
 *	ns_printer_t *_cvt_nisplus_entry_to_printer(char *name, FN_ref_t *ref,
 *						char *svc)
 * INPUT:
 *	char *name - the name of the printer entry
 *	FN_ref_t *ref - the XFN printer context reference
 *	char *svc - the source of the reference
 * OUTPUT:
 *	ns_printer_t *(return) - a printer object
 * DESCRIPTION:
 *	This function will walk through the XFN reference and decode the
 *	attributes, adding them to a printer object.
 */
ns_printer_t *
_cvt_nisplus_entry_to_printer(char *name, FN_ref_t *ref, char *svc)
{
	ns_printer_t	*printer = NULL;
	const FN_ref_addr_t *ref_addr;
	void	*iter = NULL;
	char	*tmp;
	int	found_nisplus_key = 0;

	if ((tmp = strrchr(name, '/')) == NULL)
		tmp = strdup(name);
	else
		tmp = strdup(++tmp);

	if ((printer = ns_printer_create(tmp, NULL, svc, NULL)) == NULL)
		return (NULL);

	for (ref_addr = fn_ref_first(ref, &iter); ref_addr != NULL;
	    ref_addr = fn_ref_next(ref, &iter)) {
		const FN_identifier_t *id;
		char key[128];
		char *value = NULL;
		XDR xdr;

		if ((id = fn_ref_addr_type(ref_addr)) == NULL)
			continue;

		memset(key, '\0', sizeof (key));
		strncpy(key, ((char *)id->contents + sizeof (PREFIX)),
			id->length - sizeof (PREFIX));

		if (strcmp(key, "r_nisplus") == 0) {
			found_nisplus_key++;
			continue;
		}

		xdrmem_create(&xdr, (caddr_t)fn_ref_addr_data(ref_addr),
				fn_ref_addr_length(ref_addr), XDR_DECODE);

		xdr_string(&xdr, &value, ~0);

		ns_set_value_from_string(strdup(key), value, printer);
	}

	return ((found_nisplus_key ? printer : NULL));
}


static void
_append_attribute(ns_kvp_t *kvp, va_list ap)
{
	char *buf = va_arg(ap, char *);

	if ((kvp != NULL) && (kvp->key != NULL) && (kvp->value != NULL)) {
		strcat(buf, " \"");
		strcat(buf, kvp->key);
		strcat(buf, "=");
		strcat(buf, kvp->value);
		strcat(buf, "\"");
	}
}


/*
 * FUNCTION:
 *	char *_cvt_printer_to_nisplus_entry(const ns_printer_t *printer)
 * INPUT:
 *	const ns_printer_t *printer - a printer object
 * OUTPUT:
 *	FN_ref_t *(return) - the entry
 * DESCRIPTION:
 *	This is implemented to create a string of arguments to pass to the
 *	xfn command fncreate_printer.  This command is to be called to do
 *	updates.  Hopefully this will change when the printer context is
 *	more exposed.
 */
FN_ref_t *
_cvt_printer_to_nisplus_entry(const ns_printer_t *printer)
{
	if ((printer != NULL) && (printer->attributes != NULL)) {
		char cmd[BUFSIZ];

		memset(cmd, 0, sizeof (cmd));
		list_iterate((void **)printer->attributes,
				(VFUNC_T)_append_attribute, cmd);

		return ((FN_ref_t *)strdup(cmd));
	} else
		return ((FN_ref_t *)NULL);
}
