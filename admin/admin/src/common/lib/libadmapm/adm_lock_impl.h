
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_adm_lock_impl_h
#define _adm_lock_impl_h

/*
 * FILE:  adm_lock_impl.h
 *
 *	Admin Framework header file for internal locking definitions.
 */

#pragma	ident	"@(#)adm_lock_impl.h	1.3	93/05/18 SMI"

#include <sys/types.h>

/* Admin Framework lock types and miscellaneous definitions */
#define ADM_LOCK_SH		0	/* Lock for shared access */
#define ADM_LOCK_EX		1	/* Lock for exclusive access */
#define ADM_LOCK_AP		2	/* Lock for append access */
#define ADM_LOCK_MAXWAIT	60	/* 60 second max wait for lock */
#define ADM_LOCK_SLEEP		5	/* 5 second sleep between attempts*/

/* Function Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

extern int adm_lock(int, int, int);		/* Lock a file */
extern int adm_unlock(int);			/* Unlock a file */

#ifdef __cplusplus
}
#endif

#endif /* !_adm_lock_impl_h */
