/*
 * Copyright (c) 1994 by SunSoft, Inc.
 */
#ifndef _LIST_H
#define _LIST_H

#pragma ident	"@(#)list.h	1.1	94/12/23 SunSoft"

#ifdef __cplusplus
extern "C" {
#endif

#define	 VFUNC_T	int (*)(void *, va_list)	/* for casting */
#define	 COMP_T		int (*)(void *, void *)		/* for casting */

extern void **list_append(void **list, void *item);
extern void **list_concatenate(void **list1, void **list2);
extern void * list_locate(void **list, int (*compair)(void *, void *),
				void *elem);
#ifndef SUNOS_4
extern int list_iterate(void **list, int (*vfunc)(void *, va_list), ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIST_H */
