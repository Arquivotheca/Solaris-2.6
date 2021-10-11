/*
 * Copyright (c) 1994, by Sun Microsystems, Inc.
 *
 * bootparam.h: Bootparam manipulation publice interface.
 *
 * Description:
 *  This file provides the interface definition for bootparam handler
 *  functions. These are the routines that provide the accewss and rewriting
 *  of the server's /etc/bootparams file when kdmconfig is invoked with the
 *  -s option.
 *
 * The following exported routines are found in this file
 *
 *  void bootparam_commit(char *);
 *  int bootparam_get(char *);
 *
 * This file also provides interface definitions for these routines.
 *
 */

#ifndef _INC_BOOTPARAM_H_
#define	_INC_BOOTPARAM_H_

#pragma ident "@(#)bootparam.h 1.2 94/03/09 SMI"

extern void bootparam_commit(char *);
extern int bootparam_get(char *);
extern void bootparam_remove(char *);

#endif /* _INC_BOOTPARAM_H_ */
