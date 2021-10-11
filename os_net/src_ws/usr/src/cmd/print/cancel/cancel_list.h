/*
 * Copyright (c) 1994, 1995, 1996 by Sun Microsystems, Inc.
 * All Rights Reserved
 */

#ifndef	_CANCEL_LIST_H
#define	_CANCEL_LIST_H

#pragma ident	"@(#)cancel_list.h	1.2	96/04/22 SMI"

#ifdef _cplusplus
extern "C" {
#endif

typedef struct _cancel_req cancel_req_t;
struct _cancel_req {
	char *printer;
	ns_bsd_addr_t *binding;
	char **list;
};

extern cancel_req_t ** cancel_list_add_item(cancel_req_t **list, char *printer,
		char *item);
extern cancel_req_t ** cancel_list_add_list(cancel_req_t **list, char *printer,
		char **items);
extern cancel_req_t ** cancel_list_add_binding_list(cancel_req_t **list,
		ns_bsd_addr_t *binding, char **items);

#ifdef _cplusplus
}
#endif

#endif
